/*
 * fb_transport.cpp: High-Speed Dynamic Native Display Transport Bridge
 * Hosts the /nature-os C/C++ engine, streaming single-pass zlib-compressed
 * dirty framebuffer tiles over stdout (NAT3 protocol with dynamic resize +
 * active text-input rects for mobile keyboard auto-open) and feeding pointer /
 * wheel / touch / keyboard events from stdin with zero-latency wakeup.
 */

#include "nature_os.h"
#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <string>
#include <thread>
#include <unistd.h>
#include <vector>

static std::mutex g_state_mtx;
static std::condition_variable g_wake_cv;
static NatureOSState g_os;
static int g_force_keyframe = 1;
static int g_running = 1;
static auto g_last_activity = std::chrono::steady_clock::now();

static int g_fb_w = OS_WIDTH;
static int g_fb_h = OS_HEIGHT;
static int g_req_w = OS_WIDTH;
static int g_req_h = OS_HEIGHT;
static int g_resize_pending = 0;

static uint64_t fnv1a_tile(const uint8_t *rgba, int stride_bytes, int tx, int ty, int tw, int th) {
    uint64_t h = 14695981039346656037ULL;
    for (int row = 0; row < th; ++row) {
        const uint32_t *p = reinterpret_cast<const uint32_t *>(rgba + (ty + row) * stride_bytes + tx * 4);
        for (int col = 0; col < tw; ++col) {
            h ^= p[col];
            h *= 1099511628211ULL;
        }
    }
    return h;
}

static void input_reader_thread() {
    char line[256];
    while (g_running && std::fgets(line, sizeof(line), stdin)) {
        size_t len = std::strlen(line);
        while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
            line[--len] = '\0';
        }
        if (len == 0) continue;

        {
            std::lock_guard<std::mutex> lock(g_state_mtx);
            if (line[0] != 'Z' && line[0] != 'F' && line[0] != 'Q') {
                g_last_activity = std::chrono::steady_clock::now();
            }
            if (line[0] == 'Z') {
                int rw = 0, rh = 0;
                if (std::sscanf(line + 1, "%d %d", &rw, &rh) == 2) {
                    if (rw < 360) rw = 360;
                    if (rw > 1920) rw = 1920;
                    if (rh < 360) rh = 360;
                    if (rh > 1440) rh = 1440;
                    if (rw != g_fb_w || rh != g_fb_h) {
                        g_req_w = rw;
                        g_req_h = rh;
                        g_resize_pending = 1;
                    }
                }
            } else if (line[0] == 'D') {
                int x = 0, y = 0;
                if (std::sscanf(line + 1, "%d %d", &x, &y) == 2) {
                    nature_os_pointer_down(&g_os, x, y);
                    g_os.dirty = 1;
                }
            } else if (line[0] == 'R') {
                int x = 0, y = 0;
                if (std::sscanf(line + 1, "%d %d", &x, &y) == 2) {
                    nature_os_right_click(&g_os, x, y);
                    g_os.dirty = 1;
                }
            } else if (line[0] == 'M') {
                int x = 0, y = 0;
                if (std::sscanf(line + 1, "%d %d", &x, &y) == 2) {
                    nature_os_pointer_move(&g_os, x, y);
                }
            } else if (line[0] == 'U') {
                int x = 0, y = 0;
                if (std::sscanf(line + 1, "%d %d", &x, &y) == 2) {
                    nature_os_pointer_up(&g_os, x, y);
                    g_os.dirty = 1;
                }
            } else if (line[0] == 'K' && line[1] == ' ') {
                const char *key_arg = line + 2;
                if (std::strcmp(key_arg, "Space") == 0) key_arg = " ";
                nature_os_key(&g_os, key_arg);
                g_os.dirty = 1;
            } else if (line[0] == 'O') {
                int app = 0;
                if (std::sscanf(line + 1, "%d", &app) == 1 && app >= 0 && app < APP_COUNT) {
                    nature_open_app(&g_os, static_cast<AppType>(app));
                    g_os.dirty = 1;
                }
            } else if (line[0] == 'W') {
                int dy = 0;
                if (std::sscanf(line + 1, "%d", &dy) == 1) {
                    nature_os_wheel(&g_os, dy);
                    g_os.dirty = 1;
                }
            } else if (line[0] == 'T') {
                nature_monitor_set_stress(&g_os, !g_os.stress_active);
                nature_notify(&g_os, "Performance Scheduler", g_os.stress_active ? "4-Core Compute Workload Started" : "Compute Workload Halted");
                g_os.dirty = 1;
            } else if (line[0] == 'S' && line[1] == ' ') {
                int th = 4;
                if (std::sscanf(line + 2, "%d", &th) == 1) {
                    nature_monitor_set_threads(&g_os, th);
                    g_os.dirty = 1;
                }
            } else if (line[0] == 'P' && line[1] == ' ') {
                int wp = 0;
                if (std::sscanf(line + 2, "%d", &wp) == 1 && wp >= 0 && wp < MAX_WALLPAPERS) {
                    nature_os_set_wallpaper_preset(&g_os, wp);
                    nature_os_save_state(&g_os);
                }
            } else if (line[0] == 'A' && line[1] == ' ') {
                int ac = 0;
                if (std::sscanf(line + 2, "%d", &ac) == 1 && ac >= 0 && ac <= 5) {
                    g_os.cfg.accent_idx = ac;
                    g_os.dirty = 1;
                    nature_os_save_state(&g_os);
                }
            } else if (line[0] == 'H' && line[1] == ' ') {
                int tf = 0;
                if (std::sscanf(line + 2, "%d", &tf) == 1 && (tf == THEME_NATURE || tf == THEME_HORIZON)) {
                    g_os.cfg.theme_family = tf;
                    nature_os_set_wallpaper_preset(&g_os, (tf == THEME_HORIZON) ? (g_os.cfg.dark_mode ? 4 : 3) : 0);
                    nature_os_save_state(&g_os);
                }
            } else if (line[0] == 'C' && line[1] == ' ') {
                nature_terminal_exec(&g_os, line + 2);
                g_os.dirty = 1;
                nature_os_save_state(&g_os);
            } else if (line[0] == 'N' && line[1] == ' ') {
                char title[64] = "System Notification";
                char msg[128] = "";
                const char *sep = std::strchr(line + 2, '|');
                if (sep) {
                    size_t tlen = static_cast<size_t>(sep - (line + 2));
                    if (tlen >= sizeof(title)) tlen = sizeof(title) - 1;
                    std::memcpy(title, line + 2, tlen);
                    title[tlen] = '\0';
                    size_t mlen = std::strlen(sep + 1);
                    if (mlen >= sizeof(msg)) mlen = sizeof(msg) - 1;
                    std::memcpy(msg, sep + 1, mlen);
                    msg[mlen] = '\0';
                } else {
                    size_t mlen = std::strlen(line + 2);
                    if (mlen >= sizeof(msg)) mlen = sizeof(msg) - 1;
                    std::memcpy(msg, line + 2, mlen);
                    msg[mlen] = '\0';
                }
                nature_notify(&g_os, title, msg);
                g_os.dirty = 1;
            } else if (line[0] == 'F') {
                g_force_keyframe = 1;
                g_os.dirty = 1;
            } else if (line[0] == 'Q') {
                g_running = 0;
            }
        }
        g_wake_cv.notify_one();
    }
    g_running = 0;
    g_wake_cv.notify_all();
}

#pragma pack(push, 1)
struct InputRectWire {
    uint16_t x;
    uint16_t y;
    uint16_t w;
    uint16_t h;
};

struct FrameHeaderV3 {
    char magic[4];          /* "NAT3" */
    uint16_t width;         /* dynamic width */
    uint16_t height;        /* dynamic height */
    uint16_t fps_x10;       /* measured fps * 10 */
    uint16_t ram_mb;        /* system used RAM MiB */
    uint16_t cpu_x10;       /* total cpu * 10 */
    uint8_t flags;          /* bit0: text_focused, bit1: focus_pulse, bit2: dragging */
    uint8_t input_count;    /* 0..MAX_TEXT_RECTS */
    uint16_t active_target; /* 0..4 */
    uint16_t tile_count;    /* number of changed tiles */
    uint32_t zlen;          /* compressed payload byte length */
    uint16_t total_ram_mb;  /* /proc/meminfo MemTotal MiB */
    uint16_t os_rss_mb;     /* /proc/self/status VmRSS MiB */
    uint16_t frame_ms_x10;  /* smoothed render ms * 10 */
    uint8_t cores[4];       /* per-core usage 0..100 */
    uint16_t reserved;      /* 36-byte aligned packet header */
};
#pragma pack(pop)

int main(int argc, char **argv) {
    const char *asset_dir = argc > 1 ? argv[1] : "assets";
    if (nature_gfx_load() != 0) {
        std::fprintf(stderr, "previewer: unable to load libcairo.so.2 / libz.so.1\n");
        return 1;
    }

    nature_os_init(&g_os, asset_dir);

    cairo_surface_t *fb = g_ngfx.image_surface_create(CAIRO_FORMAT_ARGB32, g_fb_w, g_fb_h);
    cairo_t *cr = g_ngfx.create(fb);
    int stride = g_ngfx.image_surface_get_stride(fb);

    int tiles_x = (g_fb_w + TILE_W - 1) / TILE_W;
    int tiles_y = (g_fb_h + TILE_H - 1) / TILE_H;
    std::vector<uint64_t> prev_hashes(static_cast<size_t>(tiles_x * tiles_y), 0);

    std::vector<uint8_t> dirty_raw;
    dirty_raw.reserve(static_cast<size_t>(tiles_x * tiles_y) * (8 + TILE_W * TILE_H * 4));
    std::vector<uint8_t> comp_payload;
    comp_payload.reserve(static_cast<size_t>(tiles_x * tiles_y) * (8 + TILE_W * TILE_H * 4) + 4096);

    std::thread reader(input_reader_thread);

    auto last_tick = std::chrono::steady_clock::now();
    auto last_media_tick = last_tick;
    auto last_fps_reset = last_tick;
    int rendered_frames = 0;
    uint8_t prev_flags = 0;

    while (g_running) {
        auto frame_start = std::chrono::steady_clock::now();
        int target_hz = 60;
        int sleep_ms = 16;

        {
            std::unique_lock<std::mutex> lock(g_state_mtx);
            target_hz = (g_os.cfg.target_hz == 120) ? 120 : 60;
            sleep_ms = (target_hz == 120) ? 8 : 16;

            if (g_resize_pending) {
                g_ngfx.destroy(cr);
                g_ngfx.surface_destroy(fb);
                g_fb_w = g_req_w;
                g_fb_h = g_req_h;
                g_resize_pending = 0;

                nature_os_resize(&g_os, g_fb_w, g_fb_h);
                fb = g_ngfx.image_surface_create(CAIRO_FORMAT_ARGB32, g_fb_w, g_fb_h);
                cr = g_ngfx.create(fb);
                stride = g_ngfx.image_surface_get_stride(fb);

                tiles_x = (g_fb_w + TILE_W - 1) / TILE_W;
                tiles_y = (g_fb_h + TILE_H - 1) / TILE_H;
                prev_hashes.assign(static_cast<size_t>(tiles_x * tiles_y), 0);
                size_t cap_needed = static_cast<size_t>(tiles_x * tiles_y) * (8 + TILE_W * TILE_H * 4) + 8192;
                if (dirty_raw.capacity() < cap_needed) dirty_raw.reserve(cap_needed);
                if (comp_payload.capacity() < cap_needed) comp_payload.reserve(cap_needed);

                g_force_keyframe = 1;
                g_os.dirty = 1;
            }

            auto since_tick = std::chrono::duration_cast<std::chrono::milliseconds>(frame_start - last_tick).count();
            int mon_ms = (g_os.cfg.telemetry_ms >= 250 && g_os.cfg.telemetry_ms <= 2000) ? g_os.cfg.telemetry_ms : 500;
            if (since_tick >= mon_ms) {
                nature_monitor_tick(&g_os);
                g_os.dirty = 1;
                last_tick = frame_start;
            }

            if (g_os.audio_playing ||
                (g_os.video_playing && g_os.media_mode == 1) ||
                (g_os.media_slideshow && g_os.media_mode == 0 && !g_os.media_grid_mode)) {
                auto since_media = std::chrono::duration_cast<std::chrono::milliseconds>(frame_start - last_media_tick).count();
                int media_ms = (g_os.video_playing && g_os.media_mode == 1) ? 50 : 140;
                if (since_media >= media_ms) {
                    nature_media_tick(&g_os);
                    g_os.dirty = 1;
                    last_media_tick = frame_start;
                }
            }

            if (g_os.toast_timer == 1) {
                g_os.dirty = 1;
            }

            if (g_os.cfg.auto_lock_min > 0 && !g_os.is_locked &&
                !g_os.audio_playing && !(g_os.video_playing && g_os.media_mode == 1)) {
                auto idle_s = std::chrono::duration_cast<std::chrono::seconds>(frame_start - g_last_activity).count();
                if (idle_s >= static_cast<long long>(g_os.cfg.auto_lock_min) * 60LL) {
                    g_os.is_locked = 1;
                    g_os.overview_open = 0;
                    g_os.launcher_open = 0;
                    g_os.quick_settings_open = 0;
                    g_os.calendar_open = 0;
                    g_os.power_dialog_open = 0;
                    g_os.alt_tab_open = 0;
                    g_os.ctx_menu_open = 0;
                    g_os.dirty = 1;
                }
            }

            if (g_os.dirty || g_force_keyframe) {
                auto r0 = std::chrono::steady_clock::now();
                nature_os_render(&g_os, cr);
                g_ngfx.surface_flush(fb);
                auto r1 = std::chrono::steady_clock::now();
                double cur_ms = std::chrono::duration<double, std::milli>(r1 - r0).count();
                g_os.frame_ms = (g_os.frame_ms <= 0.1) ? cur_ms : (g_os.frame_ms * 0.6 + cur_ms * 0.4);

                const uint8_t *bgra = g_ngfx.image_surface_get_data(fb);
                dirty_raw.clear();
                uint16_t dirty_tiles = 0;

                for (int ty_idx = 0; ty_idx < tiles_y; ++ty_idx) {
                    int ty = ty_idx * TILE_H;
                    int th = std::min(TILE_H, g_fb_h - ty);
                    if (th <= 0) continue;

                    for (int tx_idx = 0; tx_idx < tiles_x; ++tx_idx) {
                        int tx = tx_idx * TILE_W;
                        int tw = std::min(TILE_W, g_fb_w - tx);
                        if (tw <= 0) continue;

                        uint64_t h = fnv1a_tile(bgra, stride, tx, ty, tw, th);
                        int flat = ty_idx * tiles_x + tx_idx;
                        if (!g_force_keyframe && h == prev_hashes[flat]) continue;
                        prev_hashes[flat] = h;

                        uint16_t header[4] = {
                            static_cast<uint16_t>(tx),
                            static_cast<uint16_t>(ty),
                            static_cast<uint16_t>(tw),
                            static_cast<uint16_t>(th)
                        };
                        size_t old_sz = dirty_raw.size();
                        dirty_raw.resize(old_sz + sizeof(header) + static_cast<size_t>(tw * th * 4));
                        std::memcpy(dirty_raw.data() + old_sz, header, sizeof(header));

                        uint8_t *dst = dirty_raw.data() + old_sz + sizeof(header);
                        for (int row = 0; row < th; ++row) {
                            const uint8_t *src = bgra + (ty + row) * stride + tx * 4;
                            for (int col = 0; col < tw; ++col) {
                                dst[0] = src[2];
                                dst[1] = src[1];
                                dst[2] = src[0];
                                dst[3] = 255;
                                src += 4;
                                dst += 4;
                            }
                        }
                        dirty_tiles++;
                    }
                }

                uint8_t flags = 0;
                if (g_os.active_text_target > 0) flags |= 0x01;
                if (g_os.input_focus_pulse) flags |= 0x02;
                if (g_os.drag_mode != 0) flags |= 0x04;

                if (dirty_tiles > 0 || (flags != prev_flags) || g_os.input_focus_pulse) {
                    unsigned long zlen = 0;
                    bool comp_ok = true;
                    if (dirty_tiles > 0 && !dirty_raw.empty()) {
                        size_t needed = dirty_raw.size() + dirty_raw.size() / 64 + 1024;
                        if (comp_payload.size() < needed) comp_payload.resize(needed);
                        zlen = static_cast<unsigned long>(comp_payload.size());
                        comp_ok = (g_ngfx.z_compress2(comp_payload.data(), &zlen, dirty_raw.data(),
                                                      static_cast<unsigned long>(dirty_raw.size()), 1) == 0);
                    }

                    if (comp_ok) {
                        uint8_t icount = static_cast<uint8_t>(
                            std::min(g_os.text_rect_count, MAX_TEXT_RECTS));
                        InputRectWire rects[MAX_TEXT_RECTS]{};
                        for (uint8_t ri = 0; ri < icount; ++ri) {
                            rects[ri].x = static_cast<uint16_t>(std::max(0, g_os.text_rects[ri].x));
                            rects[ri].y = static_cast<uint16_t>(std::max(0, g_os.text_rects[ri].y));
                            rects[ri].w = static_cast<uint16_t>(std::max(0, g_os.text_rects[ri].w));
                            rects[ri].h = static_cast<uint16_t>(std::max(0, g_os.text_rects[ri].h));
                        }

                        FrameHeaderV3 fh{};
                        std::memcpy(fh.magic, "NAT3", 4);
                        fh.width = static_cast<uint16_t>(g_fb_w);
                        fh.height = static_cast<uint16_t>(g_fb_h);
                        fh.fps_x10 = static_cast<uint16_t>(g_os.measured_fps * 10.0);
                        fh.ram_mb = static_cast<uint16_t>(std::min(65535, std::max(0, g_os.used_ram_mb)));
                        fh.cpu_x10 = static_cast<uint16_t>(std::min(1000.0, std::max(0.0, g_os.total_cpu_pct)) * 10.0);
                        fh.flags = flags;
                        fh.input_count = icount;
                        fh.active_target = static_cast<uint16_t>(g_os.active_text_target);
                        fh.tile_count = dirty_tiles;
                        fh.zlen = static_cast<uint32_t>(zlen);
                        fh.total_ram_mb = static_cast<uint16_t>(std::min(65535, std::max(0, g_os.total_ram_mb)));
                        fh.os_rss_mb = static_cast<uint16_t>(std::min(65535, std::max(0, g_os.os_rss_mb)));
                        fh.frame_ms_x10 = static_cast<uint16_t>(std::min(999.9, std::max(0.1, g_os.frame_ms)) * 10.0);
                        for (int ci = 0; ci < 4; ++ci) {
                            int cv = static_cast<int>(g_os.core_usage[ci] + 0.5);
                            if (cv < 0) cv = 0;
                            if (cv > 100) cv = 100;
                            fh.cores[ci] = static_cast<uint8_t>(cv);
                        }
                        fh.reserved = 0;

                        std::fwrite(&fh, 1, sizeof(fh), stdout);
                        if (icount > 0) {
                            std::fwrite(rects, sizeof(InputRectWire), icount, stdout);
                        }
                        if (zlen > 0) {
                            std::fwrite(comp_payload.data(), 1, zlen, stdout);
                        }
                        std::fflush(stdout);
                        prev_flags = flags;
                        g_os.input_focus_pulse = 0;
                    }
                }

                g_os.dirty = 0;
                g_force_keyframe = 0;
                rendered_frames++;
            }

            auto fps_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(frame_start - last_fps_reset).count();
            if (fps_elapsed >= 1000) {
                g_os.measured_fps = (g_os.frame_ms < (1000.0 / target_hz)) ? static_cast<double>(target_hz) : (1000.0 / g_os.frame_ms);
                rendered_frames = 0;
                last_fps_reset = frame_start;
            }

            g_wake_cv.wait_for(lock, std::chrono::milliseconds(sleep_ms), [&]{
                return g_os.dirty || g_force_keyframe || g_resize_pending || !g_running;
            });
        }
    }

    nature_monitor_set_stress(&g_os, 0);
    if (reader.joinable()) reader.join();
    nature_os_save_state(&g_os);
    g_ngfx.destroy(cr);
    g_ngfx.surface_destroy(fb);
    return 0;
}
