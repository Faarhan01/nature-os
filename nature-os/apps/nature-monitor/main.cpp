#include "nature_os.h"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <sys/statvfs.h>
#include <sys/utsname.h>
#include <thread>
#include <time.h>
#include <unistd.h>
#include <vector>

namespace {
std::atomic<bool> g_stress_running{false};
std::vector<std::thread> g_workers;

struct CpuJiffies {
    unsigned long long active = 0;
    unsigned long long total = 0;
    bool valid = false;
};

CpuJiffies g_prev_total{};
CpuJiffies g_prev_cores[8]{};
bool g_stat_seeded = false;

struct timespec g_prev_wall{};
struct timespec g_prev_proc_cpu{};
bool g_proc_cpu_seeded = false;

void stress_worker_loop() {
    volatile double acc = 1.13;
    while (g_stress_running.load(std::memory_order_relaxed)) {
        for (int i = 0; i < 45000; ++i) {
            acc = std::sin(acc + i * 0.0007) * std::cos(acc * 0.9991) + 0.25;
        }
        std::this_thread::sleep_for(std::chrono::microseconds(250));
    }
}

void read_static_sys_info(NatureOSState *st) {
    struct utsname u{};
    if (uname(&u) == 0) {
        std::snprintf(st->kernel_ver, sizeof(st->kernel_ver), "%.42s %.18s", u.release, u.machine);
    } else {
        std::snprintf(st->kernel_ver, sizeof(st->kernel_ver), "6.8.0-45-lowlatency x86_64");
    }

    st->cpu_model[0] = '\0';
    if (FILE *f = std::fopen("/proc/cpuinfo", "r")) {
        char line[256];
        while (std::fgets(line, sizeof(line), f)) {
            if (std::strncmp(line, "model name", 10) == 0) {
                const char *col = std::strchr(line, ':');
                if (col) {
                    ++col;
                    while (*col == ' ' || *col == '\t') ++col;
                    size_t len = std::strlen(col);
                    while (len > 0 && (col[len - 1] == '\n' || col[len - 1] == '\r')) --len;
                    if (len > 0 && std::strncmp(col, "unknown", 7) != 0) {
                        std::snprintf(st->cpu_model, sizeof(st->cpu_model), "%.*s",
                                      static_cast<int>(std::min(len, sizeof(st->cpu_model) - 1)), col);
                    }
                    break;
                }
            }
        }
        std::fclose(f);
    }
    if (!st->cpu_model[0]) {
        std::snprintf(st->cpu_model, sizeof(st->cpu_model), "%.24s Linux SMP",
                      u.machine[0] ? u.machine : "x86_64");
    }
}

void read_mem_telemetry(NatureOSState *st) {
    long long mem_total_kb = 0;
    long long mem_avail_kb = 0;
    long long mem_free_kb = 0;
    long long buffers_kb = 0;
    long long cached_kb = 0;

    if (FILE *f = std::fopen("/proc/meminfo", "r")) {
        char key[64];
        long long val = 0;
        char unit[16];
        while (std::fscanf(f, "%63s %lld %15s", key, &val, unit) >= 2) {
            if (std::strcmp(key, "MemTotal:") == 0) mem_total_kb = val;
            else if (std::strcmp(key, "MemAvailable:") == 0) mem_avail_kb = val;
            else if (std::strcmp(key, "MemFree:") == 0) mem_free_kb = val;
            else if (std::strcmp(key, "Buffers:") == 0) buffers_kb = val;
            else if (std::strcmp(key, "Cached:") == 0) cached_kb = val;
        }
        std::fclose(f);
    }

    if (mem_avail_kb <= 0) mem_avail_kb = mem_free_kb;

    /* Respect container cgroup v2 memory cap when stricter than host MemTotal */
    if (FILE *cf = std::fopen("/sys/fs/cgroup/memory.max", "r")) {
        char buf[64]{};
        if (std::fgets(buf, sizeof(buf), cf) && std::strncmp(buf, "max", 3) != 0) {
            long long bytes = std::atoll(buf);
            if (bytes > 0) {
                long long cg_kb = bytes / 1024;
                if (mem_total_kb == 0 || cg_kb < mem_total_kb) mem_total_kb = cg_kb;
            }
        }
        std::fclose(cf);
    }

    if (mem_total_kb > 0) {
        st->total_ram_mb = static_cast<int>(mem_total_kb / 1024);
        long long used_kb = mem_total_kb - mem_avail_kb;
        if (used_kb < 0) used_kb = 0;
        st->used_ram_mb = static_cast<int>(used_kb / 1024);
        st->avail_ram_mb = static_cast<int>(mem_avail_kb / 1024);
        st->cached_ram_mb = static_cast<int>((buffers_kb + cached_kb) / 1024);
    }

    /* Actual live RSS of nature-previewer compositor + shell + apps process */
    if (FILE *sf = std::fopen("/proc/self/status", "r")) {
        char line[128];
        while (std::fgets(line, sizeof(line), sf)) {
            long long rss_kb = 0;
            if (std::sscanf(line, "VmRSS: %lld kB", &rss_kb) == 1) {
                st->os_rss_mb = std::max(1, static_cast<int>((rss_kb + 512) / 1024));
                break;
            }
        }
        std::fclose(sf);
    }
    if (st->os_rss_mb <= 0) st->os_rss_mb = 24;
}

void read_disk_and_uptime(NatureOSState *st) {
    struct statvfs vfs{};
    if (statvfs("/", &vfs) == 0 && vfs.f_blocks > 0) {
        unsigned long long total_bytes = static_cast<unsigned long long>(vfs.f_blocks) * vfs.f_frsize;
        unsigned long long avail_bytes = static_cast<unsigned long long>(vfs.f_bavail) * vfs.f_frsize;
        unsigned long long used_bytes = total_bytes > avail_bytes ? (total_bytes - avail_bytes) : 0;
        st->disk_total_gb = static_cast<long>(total_bytes / (1024ULL * 1024ULL * 1024ULL));
        st->disk_avail_gb = static_cast<long>(avail_bytes / (1024ULL * 1024ULL * 1024ULL));
        st->disk_used_pct = static_cast<int>((used_bytes * 100ULL) / total_bytes);
        if (st->disk_used_pct < 1 && used_bytes > 0) st->disk_used_pct = 1;
    }
    if (FILE *uf = std::fopen("/proc/uptime", "r")) {
        double up = 0.0;
        if (std::fscanf(uf, "%lf", &up) == 1) {
            st->uptime_sec = static_cast<long>(up);
        }
        std::fclose(uf);
    }
}

void read_cpu_telemetry(NatureOSState *st) {
    int cores_found = 0;
    CpuJiffies cur_total{};
    CpuJiffies cur_cores[8]{};

    if (FILE *f = std::fopen("/proc/stat", "r")) {
        char line[256];
        while (std::fgets(line, sizeof(line), f)) {
            unsigned long long u = 0, n = 0, s = 0, id = 0, io = 0, irq = 0, sirq = 0, st_v = 0;
            if (std::strncmp(line, "cpu ", 4) == 0) {
                if (std::sscanf(line + 4, "%llu %llu %llu %llu %llu %llu %llu %llu",
                                &u, &n, &s, &id, &io, &irq, &sirq, &st_v) >= 4) {
                    cur_total.active = u + n + s + irq + sirq + st_v;
                    cur_total.total = cur_total.active + id + io;
                    cur_total.valid = true;
                }
            } else if (std::strncmp(line, "cpu", 3) == 0 && line[3] >= '0' && line[3] <= '9') {
                int cidx = line[3] - '0';
                if (cidx >= 0 && cidx < 8) {
                    const char *sp = std::strchr(line, ' ');
                    if (sp && std::sscanf(sp + 1, "%llu %llu %llu %llu %llu %llu %llu %llu",
                                          &u, &n, &s, &id, &io, &irq, &sirq, &st_v) >= 4) {
                        cur_cores[cidx].active = u + n + s + irq + sirq + st_v;
                        cur_cores[cidx].total = cur_cores[cidx].active + id + io;
                        cur_cores[cidx].valid = true;
                        cores_found = std::max(cores_found, cidx + 1);
                    }
                }
            }
        }
        std::fclose(f);
    }

    if (cores_found > 0) st->cpu_count = cores_found;
    else if (st->cpu_count <= 0) st->cpu_count = 4;

    if (g_stat_seeded && cur_total.valid && g_prev_total.valid) {
        unsigned long long dt = cur_total.total - g_prev_total.total;
        unsigned long long da = cur_total.active - g_prev_total.active;
        if (dt > 0) {
            double pct = std::clamp((static_cast<double>(da) * 100.0) / static_cast<double>(dt), 0.0, 100.0);
            st->total_cpu_pct = st->total_cpu_pct * 0.45 + pct * 0.55;
        }
        for (int c = 0; c < 4; ++c) {
            if (cur_cores[c].valid && g_prev_cores[c].valid) {
                unsigned long long cdt = cur_cores[c].total - g_prev_cores[c].total;
                unsigned long long cda = cur_cores[c].active - g_prev_cores[c].active;
                if (cdt > 0) {
                    double cpct = std::clamp((static_cast<double>(cda) * 100.0) / static_cast<double>(cdt), 0.0, 100.0);
                    st->core_usage[c] = st->core_usage[c] * 0.45 + cpct * 0.55;
                }
            }
        }
    } else if (cur_total.valid && cur_total.total > 0) {
        st->total_cpu_pct = std::clamp(
            (static_cast<double>(cur_total.active) * 100.0) / static_cast<double>(cur_total.total), 0.1, 100.0);
        for (int c = 0; c < 4; ++c) {
            if (cur_cores[c].valid && cur_cores[c].total > 0) {
                st->core_usage[c] = std::clamp(
                    (static_cast<double>(cur_cores[c].active) * 100.0) / static_cast<double>(cur_cores[c].total),
                    0.1, 100.0);
            }
        }
        g_stat_seeded = true;
    }

    if (cur_total.valid) g_prev_total = cur_total;
    for (int c = 0; c < 8; ++c) {
        if (cur_cores[c].valid) g_prev_cores[c] = cur_cores[c];
    }

    /* Real process CPU time of nature-previewer compositor/shell */
    struct timespec now_wall{}, now_cpu{};
    clock_gettime(CLOCK_MONOTONIC, &now_wall);
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &now_cpu);
    if (g_proc_cpu_seeded) {
        double d_wall = (now_wall.tv_sec - g_prev_wall.tv_sec) + (now_wall.tv_nsec - g_prev_wall.tv_nsec) * 1e-9;
        double d_cpu = (now_cpu.tv_sec - g_prev_proc_cpu.tv_sec) + (now_cpu.tv_nsec - g_prev_proc_cpu.tv_nsec) * 1e-9;
        if (d_wall > 0.05) {
            double raw_pct = (d_cpu / d_wall) * 100.0;
            if (raw_pct < 0.1) raw_pct = 0.1;
            if (raw_pct > 100.0 * st->cpu_count) raw_pct = 100.0 * st->cpu_count;
            st->compositor_cpu_pct = st->compositor_cpu_pct * 0.4 + raw_pct * 0.6;
        }
    } else {
        st->compositor_cpu_pct = 0.4;
        g_proc_cpu_seeded = true;
    }
    g_prev_wall = now_wall;
    g_prev_proc_cpu = now_cpu;
}
} // namespace

extern "C" {

void nature_monitor_set_stress(NatureOSState *st, int enable) {
    if (enable && !st->stress_active) {
        st->stress_active = 1;
        if (st->stress_threads <= 0) st->stress_threads = 4;
        g_stress_running.store(true);
        for (int i = 0; i < st->stress_threads; ++i) {
            g_workers.emplace_back(stress_worker_loop);
        }
        nature_notify(st, "Performance Scheduler", "Multi-core C++ worker load active. UI compositor stays locked.");
    } else if (!enable && st->stress_active) {
        st->stress_active = 0;
        g_stress_running.store(false);
        for (auto &t : g_workers) {
            if (t.joinable()) t.join();
        }
        g_workers.clear();
        nature_notify(st, "Performance Scheduler", "Compute workload halted cleanly.");
    }
    nature_monitor_tick(st);
}

void nature_monitor_set_threads(NatureOSState *st, int threads) {
    if (threads < 1) threads = 1;
    if (threads > 4) threads = 4;
    st->stress_threads = threads;
    if (st->stress_active) {
        nature_monitor_set_stress(st, 0);
        nature_monitor_set_stress(st, 1);
    }
}

void nature_monitor_init(NatureOSState *st) {
    read_static_sys_info(st);
    read_mem_telemetry(st);
    read_disk_and_uptime(st);
    read_cpu_telemetry(st);
    double init_ram_pct = st->total_ram_mb > 0
        ? std::clamp((static_cast<double>(st->used_ram_mb) * 100.0) / static_cast<double>(st->total_ram_mb), 0.0, 100.0)
        : 0.0;
    for (int i = 0; i < 48; ++i) {
        st->cpu_history[i] = st->total_cpu_pct;
        st->ram_history[i] = init_ram_pct;
    }
    st->zram_used_mb = st->cfg.zram_enabled ? std::max(8, st->cached_ram_mb / 16) : 0;
    st->stress_threads = 4;
    st->monitor_scroll = 0;
    st->monitor_max_scroll = 0;
    nature_monitor_tick(st);
}

void nature_monitor_tick(NatureOSState *st) {
    read_mem_telemetry(st);
    read_disk_and_uptime(st);
    read_cpu_telemetry(st);

    int count = 0;
    auto push_proc = [&](int pid, const char *name, const char *user, const char *state,
                         double cpu, int ram, int nice_val, int win_id, int killable) {
        if (count >= 32) return;
        ProcEntry &p = st->procs[count++];
        p.pid = pid;
        std::snprintf(p.name, sizeof(p.name), "%s", name);
        std::snprintf(p.user, sizeof(p.user), "%s", user);
        std::snprintf(p.state, sizeof(p.state), "%s", state);
        p.cpu_pct = cpu;
        p.ram_mb = ram;
        p.nice_val = nice_val;
        p.win_id = win_id;
        p.killable = killable;
    };

    int self_pid = static_cast<int>(getpid());
    double comp_cpu = st->stress_active
        ? std::min(4.8, std::max(0.4, st->compositor_cpu_pct * 0.25))
        : std::max(0.2, st->compositor_cpu_pct);
    push_proc(self_pid, "nature-compositor", "nature", "Run", comp_cpu, st->os_rss_mb, -10, -1, 0);
    push_proc(1, "systemd", "root", "Sleep", 0.1, 14, 0, -1, 0);
    push_proc(468, "nature-shell", "nature", "Run", 0.2, std::max(8, st->os_rss_mb / 2), -5, -1, 0);
    push_proc(519, "pipewire", "nature", st->audio_playing ? "Run" : "Sleep", st->audio_playing ? 0.5 : 0.1, 16, -11, -1, 0);
    push_proc(584, "NetworkManager", "root", "Sleep", 0.1, 18, 0, -1, 0);
    push_proc(622, "zram-swapd", "root", "Sleep", 0.0, 8, 0, -1, 0);
    push_proc(712, "systemd-oomd", "root", "Sleep", 0.0, 9, -8, -1, 0);
    push_proc(784, "xdg-desktop-portal", "nature", "Sleep", 0.1, 12, 0, -1, 0);

    static const char *PROC_NAMES[APP_COUNT] = {
        "nature-files", "nature-browser", "nature-terminal", "nature-store",
        "nature-monitor", "nature-media", "nature-settings"
    };
    static const int APP_SHARE_MB[APP_COUNT] = {6, 14, 4, 8, 6, 9, 5};

    for (int i = 0; i < st->window_count; ++i) {
        const OSWindow &w = st->windows[i];
        double app_cpu = w.minimized ? 0.0 : (w.id == st->active_window_id ? 0.4 : 0.1);
        push_proc(w.pid, PROC_NAMES[w.app], "nature", w.minimized ? "Sleep" : "Run",
                  app_cpu, APP_SHARE_MB[w.app], -2, w.id, 1);
    }

    if (st->stress_active) {
        int th = st->stress_threads > 0 ? st->stress_threads : 4;
        double worker_share = std::max(8.0, (st->total_cpu_pct * st->cpu_count) / th * 0.72);
        const char *worker_names[4] = {
            "nature-stress#0", "nature-stress#1", "nature-stress#2", "nature-stress#3"
        };
        for (int t = 0; t < th && t < 4; ++t) {
            push_proc(4801 + t * 17, worker_names[t], "nature", "Load",
                      std::min(99.0, worker_share), 8, 10, -1, 1);
        }
        st->zram_used_mb = st->cfg.zram_enabled ? std::max(24, st->cached_ram_mb / 8) : 0;
    } else {
        st->zram_used_mb = st->cfg.zram_enabled ? std::max(8, st->cached_ram_mb / 18) : 0;
    }

    std::sort(st->procs, st->procs + count, [st](const ProcEntry &a, const ProcEntry &b) {
        if (st->monitor_sort == 1) {
            if (a.ram_mb != b.ram_mb) return a.ram_mb > b.ram_mb;
            return a.cpu_pct > b.cpu_pct;
        }
        if (st->monitor_sort == 2) {
            return a.pid < b.pid;
        }
        if (a.cpu_pct != b.cpu_pct) return a.cpu_pct > b.cpu_pct;
        return a.ram_mb > b.ram_mb;
    });
    st->proc_count = count;

    double ram_pct = st->total_ram_mb > 0
        ? std::clamp((static_cast<double>(st->used_ram_mb) * 100.0) / static_cast<double>(st->total_ram_mb), 0.0, 100.0)
        : 0.0;
    for (int i = 0; i < 47; ++i) {
        st->cpu_history[i] = st->cpu_history[i + 1];
        st->ram_history[i] = st->ram_history[i + 1];
    }
    st->cpu_history[47] = st->total_cpu_pct;
    st->ram_history[47] = ram_pct;
}

void nature_monitor_render(NatureOSState *st, cairo_t *cr, OSWindow *w, int cx, int cy, int cw, int ch) {
    (void)w;
    NaturePalette pal;
    nature_get_palette(st, &pal);
    int dark = pal.dark_mode;
    double ar = pal.ar, ag = pal.ag, ab = pal.ab;

    double fg_r = pal.fg_r, fg_g = pal.fg_g, fg_b = pal.fg_b;
    double mut_r = pal.mut_r, mut_g = pal.mut_g, mut_b = pal.mut_b;
    int compact = cw < 540;

    ngfx_fill_rect(cr, cx, cy, cw, ch, 0, pal.win_bg_r, pal.win_bg_g, pal.win_bg_b, 1.0);

    /* Top Tabs + Stress Control */
    ngfx_fill_rect(cr, cx, cy, cw, 42, 0, pal.sidebar_r, pal.sidebar_g, pal.sidebar_b, 1.0);
    g_ngfx.set_source_rgba(cr, pal.border_r, pal.border_g, pal.border_b, pal.border_a);
    g_ngfx.set_line_width(cr, 1.0);
    g_ngfx.move_to(cr, cx, cy + 42.5);
    g_ngfx.line_to(cr, cx + cw, cy + 42.5);
    g_ngfx.stroke(cr);

    const char *tabs_full[2] = {"Resources & Cores", "Processes & Tasks"};
    const char *tabs_short[2] = {"Resources", "Tasks"};
    int tab_w = compact ? 92 : 136;
    for (int i = 0; i < 2; ++i) {
        int tx = cx + 10 + i * (tab_w + 6);
        int act = st->monitor_tab == i;
        ngfx_fill_rect(cr, tx, cy + 8, tab_w, 26, 7,
                       act ? ar : pal.elev_r, act ? ag : pal.elev_g, act ? ab : pal.elev_b, act ? 0.94 : 0.90);
        ngfx_text(cr, "Noto Sans", act, compact ? 11.0 : 11.8, tx + (compact ? 12 : 14), cy + 25,
                  act ? pal.on_acc_r : fg_r, act ? pal.on_acc_g : fg_g, act ? pal.on_acc_b : fg_b, 1.0,
                  compact ? tabs_short[i] : tabs_full[i]);
        nature_add_hit(st, tx, cy + 8, tab_w, 26, HIT_MONITOR_TAB, i, 0);
    }

    int btn_w = compact ? 96 : 162;
    int sb_x = cx + cw - btn_w - 10;
    if (!compact || cw >= 420) {
        const int c_opts[3] = {1, 2, 4};
        int c_start = sb_x - (compact ? 94 : 102);
        for (int i = 0; i < 3; ++i) {
            int bx = c_start + i * 31;
            if (bx < cx + 210) continue;
            int act = st->stress_threads == c_opts[i] || (c_opts[i] == 4 && st->stress_threads >= 3);
            ngfx_fill_rect(cr, bx, cy + 8, 27, 26, 6,
                           act ? ar : pal.elev_r, act ? ag : pal.elev_g, act ? ab : pal.elev_b,
                           act ? 0.35 : 0.90);
            char lbl[8];
            std::snprintf(lbl, sizeof(lbl), "%dC", c_opts[i]);
            ngfx_text(cr, "JetBrains Mono", 1, 10.0, bx + 6, cy + 25, fg_r, fg_g, fg_b, 1.0, lbl);
            nature_add_hit(st, bx, cy + 8, 27, 26, HIT_MONITOR_STRESS_THREADS, c_opts[i], 0);
        }
    }

    ngfx_fill_rect(cr, sb_x, cy + 8, btn_w, 26, 7,
                   st->stress_active ? pal.danger_r : ar, st->stress_active ? pal.danger_g : ag, st->stress_active ? pal.danger_b : ab, 0.94);
    ngfx_text(cr, "Noto Sans", 1, compact ? 10.5 : 11.2, sb_x + (compact ? 10 : 12), cy + 25,
              st->stress_active ? 1.0 : pal.on_acc_r, st->stress_active ? 1.0 : pal.on_acc_g, st->stress_active ? 1.0 : pal.on_acc_b, 1.0,
              compact ? (st->stress_active ? "Halt Load" : "CPU Load")
                      : (st->stress_active ? "Halt Load Test" : "Simulate CPU Load"));
    nature_add_hit(st, sb_x, cy + 8, btn_w, 26, st->stress_active ? HIT_MONITOR_CLEAR_STRESS : HIT_MONITOR_STRESS, 0, 0);

    if (st->monitor_tab == 0) {
        int px = cx + 16, pw = cw - 32, py = cy + 52;
        int card_w = (pw - 20) / 3;
        char v0[32], s0[48], v1[32], s1[48], v2[32], s2[48];
        std::snprintf(v0, sizeof(v0), "%.1f%%", st->total_cpu_pct);
        std::snprintf(s0, sizeof(s0), "%d cores · /proc/stat", st->cpu_count > 0 ? st->cpu_count : 4);
        std::snprintf(v1, sizeof(v1), "%d MiB", st->used_ram_mb);
        std::snprintf(s1, sizeof(s1), "of %dM · OS RSS %dM", st->total_ram_mb, st->os_rss_mb);
        std::snprintf(v2, sizeof(v2), "%.0f FPS", st->measured_fps);
        std::snprintf(s2, sizeof(s2), "%.1f ms frame render", st->frame_ms);

        const char *kpis[3][3] = {
            {compact ? "CPU Load" : "Live CPU (/proc/stat)", v0, s0},
            {compact ? "System RAM" : "Memory (/proc/meminfo)", v1, s1},
            {compact ? "Pacing" : "Compositor Pacing", v2, s2},
        };
        for (int i = 0; i < 3; ++i) {
            int kx = px + i * (card_w + 10);
            ngfx_fill_rect(cr, kx, py, card_w, 70, 11, pal.card_r, pal.card_g, pal.card_b, 0.96);
            ngfx_stroke_rect(cr, kx + 0.5, py + 0.5, card_w - 1, 69, 11, 1.0, pal.border_r, pal.border_g, pal.border_b, pal.border_a);
            /* Subtle top semantic accent strip */
            double sr = (i == 0 && st->stress_active) ? pal.warning_r : pal.ar_hi;
            double sg = (i == 0 && st->stress_active) ? pal.warning_g : pal.ag_hi;
            double sb = (i == 0 && st->stress_active) ? pal.warning_b : pal.ab_hi;
            ngfx_fill_rect(cr, kx + 12, py + 1, 28, 2.5, 1.2, sr, sg, sb, 0.85);
            ngfx_text_clipped(cr, "Noto Sans", 1, compact ? 10.2 : 11.0, kx + 12, py + 21, card_w - 20, mut_r, mut_g, mut_b, 1.0, kpis[i][0]);
            ngfx_text_clipped(cr, "JetBrains Mono", 1, compact ? 14.5 : 18.0, kx + 12, py + 45, card_w - 20, fg_r, fg_g, fg_b, 1.0, kpis[i][1]);
            ngfx_text_clipped(cr, "JetBrains Mono", 0, 9.4, kx + 12, py + 61, card_w - 20, mut_r, mut_g, mut_b, 0.9, kpis[i][2]);
        }

        /* 4 CPU Cores Bar Strips */
        int cy_cores = py + 80;
        int cores_h = compact ? 74 : 58;
        ngfx_fill_rect(cr, px, cy_cores, pw, cores_h, 11, pal.card_r, pal.card_g, pal.card_b, 0.96);
        ngfx_stroke_rect(cr, px + 0.5, cy_cores + 0.5, pw - 1, cores_h - 1, 11, 1.0, pal.border_r, pal.border_g, pal.border_b, pal.border_a);
        ngfx_text(cr, "Noto Sans", 1, 11.5, px + 14, cy_cores + 20, fg_r, fg_g, fg_b, 1.0, "CPU Cores");

        if (compact) {
            int core_w = (pw - 36) / 2;
            for (int c = 0; c < 4; ++c) {
                int col = c % 2;
                int row = c / 2;
                int cx_pos = px + 14 + col * (core_w + 10);
                int ry_pos = cy_cores + 26 + row * 22;
                char cname[24];
                std::snprintf(cname, sizeof(cname), "C%d %.0f%%", c, st->core_usage[c]);
                ngfx_text(cr, "JetBrains Mono", 0, 9.8, cx_pos, ry_pos + 12, mut_r, mut_g, mut_b, 1.0, cname);
                int bx = cx_pos + 54;
                int bw = std::max(20, core_w - 56);
                ngfx_fill_rect(cr, bx, ry_pos + 2, bw, 11, 5.5, dark ? 1 : 0, dark ? 1 : 0, dark ? 1 : 0, 0.12);
                double fill_w = (std::min(100.0, st->core_usage[c]) / 100.0) * bw;
                if (fill_w > 0) {
                    double cr0 = st->core_usage[c] > 70 ? 0.92 : ar;
                    double cg0 = st->core_usage[c] > 70 ? 0.35 : ag;
                    double cb0 = st->core_usage[c] > 70 ? 0.25 : ab;
                    ngfx_fill_rect(cr, bx, ry_pos + 2, fill_w, 11, 5.5, cr0, cg0, cb0, 0.95);
                }
            }
        } else {
            int core_w = (pw - 116) / 4;
            for (int c = 0; c < 4; ++c) {
                int cx_pos = px + 98 + c * (core_w + 8);
                char cname[24];
                std::snprintf(cname, sizeof(cname), "C%d  %.1f%%", c, st->core_usage[c]);
                ngfx_text(cr, "JetBrains Mono", 0, 10.0, cx_pos, cy_cores + 20, mut_r, mut_g, mut_b, 1.0, cname);

                ngfx_fill_rect(cr, cx_pos, cy_cores + 29, core_w, 13, 6.5, dark ? 1 : 0, dark ? 1 : 0, dark ? 1 : 0, 0.12);
                /* Quarter tick marks */
                for (int q = 1; q <= 3; ++q) {
                    double tx = cx_pos + (core_w * q) / 4.0;
                    ngfx_fill_rect(cr, tx, cy_cores + 31, 1.0, 9, 0.5, dark ? 1 : 0, dark ? 1 : 0, dark ? 1 : 0, 0.08);
                }
                double fill_w = (std::min(100.0, st->core_usage[c]) / 100.0) * core_w;
                if (fill_w > 0) {
                    double cr0 = st->core_usage[c] > 70 ? 0.92 : ar;
                    double cg0 = st->core_usage[c] > 70 ? 0.35 : ag;
                    double cb0 = st->core_usage[c] > 70 ? 0.25 : ab;
                    ngfx_fill_gradient(cr, cx_pos, cy_cores + 29, fill_w, 13, 6.5,
                                       cr0, cg0, cb0, 0.95,
                                       cr0 * 1.25 > 1 ? 1 : cr0 * 1.25, cg0 * 1.25 > 1 ? 1 : cg0 * 1.25, cb0 * 1.25 > 1 ? 1 : cb0 * 1.25, 0.95, 1);
                }
            }
        }

        /* Real-time telemetry graph with CPU/RAM mode toggle, Peak/Avg stats & glowing live beacon */
        int gy = cy_cores + cores_h + 10;
        int gh = ch - (gy - cy) - 14;
        if (gh > 64) {
            int ram_mode = (st->monitor_chart_mode == 1);
            const double *series = ram_mode ? st->ram_history : st->cpu_history;
            double peak_v = 0.0, sum_v = 0.0;
            for (int i = 0; i < 48; ++i) {
                double v = std::clamp(series[i], 0.0, 100.0);
                if (v > peak_v) peak_v = v;
                sum_v += v;
            }
            double avg_v = sum_v / 48.0;

            ngfx_fill_rect(cr, px, gy, pw, gh, 11, pal.card_r, pal.card_g, pal.card_b, 0.96);
            ngfx_stroke_rect(cr, px + 0.5, gy + 0.5, pw - 1, gh - 1, 11, 1.0, pal.border_r, pal.border_g, pal.border_b, pal.border_a);

            const char *m_lbls[2] = {"CPU %", "RAM %"};
            for (int m = 0; m < 2; ++m) {
                int mx_tab = px + 12 + m * 54;
                int act = (st->monitor_chart_mode == m);
                ngfx_fill_rect(cr, mx_tab, gy + 7, 50, 20, 6,
                               act ? ar : pal.elev_r, act ? ag : pal.elev_g, act ? ab : pal.elev_b, act ? 0.94 : 0.85);
                ngfx_text(cr, "JetBrains Mono", 1, 9.6, mx_tab + 8, gy + 20.5,
                          act ? pal.on_acc_r : fg_r, act ? pal.on_acc_g : fg_g, act ? pal.on_acc_b : fg_b, 1.0, m_lbls[m]);
                nature_add_hit(st, mx_tab, gy + 7, 50, 20, HIT_MONITOR_CHART_MODE, m, 0);
            }

            char peak_buf[48];
            std::snprintf(peak_buf, sizeof(peak_buf), "Peak %.1f%% · Avg %.1f%%", peak_v, avg_v);
            ngfx_text_clipped(cr, "JetBrains Mono", 0, 9.4, px + 126, gy + 20.5, 160,
                              dark ? pal.ar_hi : ar, dark ? pal.ag_hi : ag, dark ? pal.ab_hi : ab, 0.95, peak_buf);

            const char *host = st->cfg.device_name[0] ? st->cfg.device_name : "nature-pc";
            long d_used = st->disk_total_gb > st->disk_avail_gb ? (st->disk_total_gb - st->disk_avail_gb) : 0;
            char mem_line[96];
            std::snprintf(mem_line, sizeof(mem_line), "%.14s · Disk %ld/%ldG · zram %dM",
                          host, d_used, st->disk_total_gb, st->zram_used_mb);
            if (pw > 500) {
                ngfx_text_clipped(cr, "JetBrains Mono", 0, 9.4, px + pw - 236, gy + 20.5, 226, mut_r, mut_g, mut_b, 0.92, mem_line);
            }

            int chart_y = gy + 32;
            if (gh > 125) {
                double tot_m = st->total_ram_mb > 0 ? static_cast<double>(st->total_ram_mb) : 4096.0;
                double act_f = std::clamp(static_cast<double>(st->used_ram_mb) / tot_m, 0.0, 1.0);
                double cache_f = std::clamp(static_cast<double>(st->cached_ram_mb) / tot_m, 0.0, 1.0 - act_f);
                double zram_f = std::clamp(static_cast<double>(st->zram_used_mb) / tot_m, 0.0, 1.0 - act_f - cache_f);
                int mb_x = px + 12, mb_w = pw - 24, mb_y = gy + 32;
                ngfx_fill_rect(cr, mb_x, mb_y, mb_w, 7, 3.5, pal.elev_r, pal.elev_g, pal.elev_b, 0.95);
                double x_cur = mb_x;
                if (act_f > 0.0) {
                    double w0 = mb_w * act_f;
                    ngfx_fill_rect(cr, x_cur, mb_y, w0, 7, 3.5, ar, ag, ab, 0.96);
                    x_cur += w0;
                }
                if (cache_f > 0.0) {
                    double w1 = mb_w * cache_f;
                    ngfx_fill_rect(cr, x_cur, mb_y, w1, 7, 2.0, pal.success_r, pal.success_g, pal.success_b, 0.78);
                    x_cur += w1;
                }
                if (zram_f > 0.0) {
                    double w2 = std::max(4.0, mb_w * zram_f);
                    if (x_cur + w2 <= mb_x + mb_w) {
                        ngfx_fill_rect(cr, x_cur, mb_y, w2, 7, 2.0, pal.warning_r, pal.warning_g, pal.warning_b, 0.90);
                    }
                }
                char map_lbl[96];
                std::snprintf(map_lbl, sizeof(map_lbl), "RAM Map: %dM active · %dM cache · %dM zram · %dM free",
                              st->used_ram_mb, st->cached_ram_mb, st->zram_used_mb, st->avail_ram_mb);
                ngfx_text_clipped(cr, "JetBrains Mono", 0, 8.6, mb_x, mb_y + 18, mb_w, mut_r, mut_g, mut_b, 0.88, map_lbl);
                chart_y = gy + 56;
            }
            int gx = px + 36, gw = pw - 52, chart_h = (gy + gh - 12) - chart_y;
            const char *g_lbls[3] = {"100%", "50%", "0%"};
            g_ngfx.set_source_rgba(cr, pal.border_r, pal.border_g, pal.border_b, pal.border_a);
            g_ngfx.set_line_width(cr, 1.0);
            for (int g = 0; g <= 2; ++g) {
                double ly = chart_y + (chart_h * g) / 2.0;
                g_ngfx.move_to(cr, gx, ly);
                g_ngfx.line_to(cr, gx + gw, ly);
            }
            for (int v = 1; v <= 3; ++v) {
                double vx = gx + (gw * v) / 4.0;
                g_ngfx.move_to(cr, vx, chart_y);
                g_ngfx.line_to(cr, vx, chart_y + chart_h);
            }
            g_ngfx.stroke(cr);
            for (int g = 0; g <= 2; ++g) {
                double ly = chart_y + (chart_h * g) / 2.0;
                ngfx_text(cr, "JetBrains Mono", 0, 8.8, px + 8, ly + 3, mut_r, mut_g, mut_b, 0.72, g_lbls[g]);
            }

            double cr_c = ram_mode ? pal.success_r : ar;
            double cg_c = ram_mode ? pal.success_g : ag;
            double cb_c = ram_mode ? pal.success_b : ab;

            /* Gradient area fill under curve */
            cairo_pattern_t *cpat = g_ngfx.pattern_create_linear(gx, chart_y, gx, chart_y + chart_h);
            g_ngfx.pattern_add_color_stop_rgba(cpat, 0.0, cr_c, cg_c, cb_c, 0.36);
            g_ngfx.pattern_add_color_stop_rgba(cpat, 1.0, cr_c, cg_c, cb_c, 0.02);
            g_ngfx.new_sub_path(cr);
            for (int i = 0; i < 48; ++i) {
                double sx = gx + (gw * i) / 47.0;
                double sy = chart_y + chart_h - (std::min(100.0, series[i]) / 100.0) * chart_h;
                if (i == 0) g_ngfx.move_to(cr, sx, sy);
                else g_ngfx.line_to(cr, sx, sy);
            }
            g_ngfx.line_to(cr, gx + gw, chart_y + chart_h);
            g_ngfx.line_to(cr, gx, chart_y + chart_h);
            g_ngfx.close_path(cr);
            g_ngfx.set_source(cr, cpat);
            g_ngfx.fill(cr);
            g_ngfx.pattern_destroy(cpat);

            g_ngfx.save(cr);
            g_ngfx.set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
            g_ngfx.set_line_join(cr, CAIRO_LINE_JOIN_ROUND);

            /* Subtle outer glow line */
            g_ngfx.set_source_rgba(cr, cr_c, cg_c, cb_c, 0.25);
            g_ngfx.set_line_width(cr, 5.0);
            for (int i = 0; i < 48; ++i) {
                double sx = gx + (gw * i) / 47.0;
                double sy = chart_y + chart_h - (std::min(100.0, series[i]) / 100.0) * chart_h;
                if (i == 0) g_ngfx.move_to(cr, sx, sy);
                else g_ngfx.line_to(cr, sx, sy);
            }
            g_ngfx.stroke(cr);

            /* Crisp primary chart stroke */
            g_ngfx.set_source_rgba(cr, ram_mode ? pal.success_r : pal.ar_hi, ram_mode ? pal.success_g : pal.ag_hi, ram_mode ? pal.success_b : pal.ab_hi, 0.98);
            g_ngfx.set_line_width(cr, 2.2);
            double last_sy = chart_y + chart_h;
            for (int i = 0; i < 48; ++i) {
                double sx = gx + (gw * i) / 47.0;
                double sy = chart_y + chart_h - (std::min(100.0, series[i]) / 100.0) * chart_h;
                last_sy = sy;
                if (i == 0) g_ngfx.move_to(cr, sx, sy);
                else g_ngfx.line_to(cr, sx, sy);
            }
            g_ngfx.stroke(cr);
            g_ngfx.restore(cr);

            /* Live endpoint glow beacon */
            ngfx_radial_glow(cr, gx + gw, last_sy, 1.0, 12.0, cr_c, cg_c, cb_c, 0.55, 0.0);
            ngfx_fill_circle(cr, gx + gw, last_sy, 3.6, 1.0, 1.0, 1.0, 0.96);
        }
    } else {
        /* Task Manager Table with interactive sortable headers & scrollbar */
        int px = cx + 14, pw = cw - 34, py = cy + 48;
        ngfx_fill_rect(cr, px, py, pw, 22, 6, pal.sidebar_r, pal.sidebar_g, pal.sidebar_b, 0.95);
        int s_pid = (st->monitor_sort == 2);
        int s_cpu = (st->monitor_sort == 0);
        int s_mem = (st->monitor_sort == 1);

        ngfx_text(cr, "Noto Sans", 1, 10.6, px + 8, py + 15,
                  s_pid ? (dark ? pal.ar_hi : ar) : mut_r,
                  s_pid ? (dark ? pal.ag_hi : ag) : mut_g,
                  s_pid ? (dark ? pal.ab_hi : ab) : mut_b, 1.0, s_pid ? "PID ^" : "PID");
        nature_add_hit(st, px + 4, py, 44, 22, HIT_MONITOR_SORT, 2, 0);

        ngfx_text(cr, "Noto Sans", 1, 10.6, px + (compact ? 54 : 68), py + 15, mut_r, mut_g, mut_b, 1.0, "Process");
        if (!compact) ngfx_text(cr, "Noto Sans", 1, 10.6, px + 216, py + 15, mut_r, mut_g, mut_b, 1.0, "User");
        int col_cpu = compact ? (pw - 220) : 284;
        int col_mem = compact ? (pw - 166) : 352;
        if (col_cpu < px + 140) col_cpu = px + 140;
        if (col_mem < col_cpu + 48) col_mem = col_cpu + 48;

        ngfx_text(cr, "Noto Sans", 1, 10.6, px + col_cpu, py + 15,
                  s_cpu ? (dark ? pal.ar_hi : ar) : mut_r,
                  s_cpu ? (dark ? pal.ag_hi : ag) : mut_g,
                  s_cpu ? (dark ? pal.ab_hi : ab) : mut_b, 1.0, s_cpu ? "CPU v" : "CPU");
        nature_add_hit(st, px + col_cpu - 4, py, 56, 22, HIT_MONITOR_SORT, 0, 0);

        ngfx_text(cr, "Noto Sans", 1, 10.6, px + col_mem, py + 15,
                  s_mem ? (dark ? pal.ar_hi : ar) : mut_r,
                  s_mem ? (dark ? pal.ag_hi : ag) : mut_g,
                  s_mem ? (dark ? pal.ab_hi : ab) : mut_b, 1.0, s_mem ? "Mem v" : "Mem");
        nature_add_hit(st, px + col_mem - 4, py, 56, 22, HIT_MONITOR_SORT, 1, 0);

        if (!compact) ngfx_text(cr, "Noto Sans", 1, 10.6, px + 430, py + 15, mut_r, mut_g, mut_b, 1.0, "Nice");
        ngfx_text(cr, "Noto Sans", 1, 10.6, px + pw - (compact ? 68 : 158), py + 15, mut_r, mut_g, mut_b, 1.0, "Action");

        int row_h = 27;
        int list_y = py + 26;
        int avail_h = (cy + ch) - list_y - 8;
        int vis_rows = std::max(1, avail_h / row_h);
        st->monitor_max_scroll = std::max(0, st->proc_count - vis_rows);
        if (st->monitor_scroll > st->monitor_max_scroll) st->monitor_scroll = st->monitor_max_scroll;
        if (st->monitor_scroll < 0) st->monitor_scroll = 0;

        int ry = list_y;
        for (int i = st->monitor_scroll; i < st->proc_count && ry + row_h <= cy + ch - 4; ++i) {
            const ProcEntry &p = st->procs[i];
            int row_hov = (st->hover_mx >= px && st->hover_mx <= px + pw && st->hover_my >= ry && st->hover_my <= ry + 25);
            if (row_hov) ngfx_fill_rect(cr, px, ry, pw, 25, 6, ar, ag, ab, 0.14);
            else if (i % 2 == 0) ngfx_fill_rect(cr, px, ry, pw, 25, 6, pal.card_r, pal.card_g, pal.card_b, 0.72);
            char b_pid[16], b_cpu[16], b_ram[16], b_nice[16];
            std::snprintf(b_pid, sizeof(b_pid), "%d", p.pid);
            std::snprintf(b_cpu, sizeof(b_cpu), "%.1f%%", p.cpu_pct);
            std::snprintf(b_ram, sizeof(b_ram), "%dM", p.ram_mb);
            std::snprintf(b_nice, sizeof(b_nice), "%+d", p.nice_val);

            ngfx_text(cr, "JetBrains Mono", 0, 10.6, px + 8, ry + 16, mut_r, mut_g, mut_b, 1.0, b_pid);
            int running_s = (std::strcmp(p.state, "Run") == 0);
            int load_s = (std::strcmp(p.state, "Load") == 0);
            ngfx_fill_circle(cr, px + (compact ? 47 : 60), ry + 12.5, 2.5,
                             load_s ? pal.warning_r : (running_s ? pal.ar_hi : mut_r),
                             load_s ? pal.warning_g : (running_s ? pal.ag_hi : mut_g),
                             load_s ? pal.warning_b : (running_s ? pal.ab_hi : mut_b), 0.9);
            ngfx_text_clipped(cr, "Noto Sans", 1, 11.2, px + (compact ? 54 : 68), ry + 16,
                              col_cpu - (compact ? 60 : 74), fg_r, fg_g, fg_b, 1.0, p.name);
            if (!compact) ngfx_text(cr, "Noto Sans", 0, 11.0, px + 216, ry + 16, mut_r, mut_g, mut_b, 1.0, p.user);
            ngfx_text(cr, "JetBrains Mono", 0, 10.8, px + col_cpu, ry + 16,
                      p.cpu_pct > 10.0 ? pal.warning_r : fg_r, p.cpu_pct > 10.0 ? pal.warning_g : fg_g, p.cpu_pct > 10.0 ? pal.warning_b : fg_b, 1.0, b_cpu);
            ngfx_text(cr, "JetBrains Mono", 0, 10.8, px + col_mem, ry + 16, fg_r, fg_g, fg_b, 0.9, b_ram);
            if (!compact) ngfx_text(cr, "JetBrains Mono", 0, 10.8, px + 430, ry + 16, mut_r, mut_g, mut_b, 0.9, b_nice);

            if (p.killable) {
                int btn_x = px + pw - (compact ? 64 : 72);
                int khov = (st->hover_mx >= btn_x && st->hover_mx <= btn_x + (compact ? 58 : 64) && st->hover_my >= ry + 3 && st->hover_my <= ry + 22);
                ngfx_fill_rect(cr, btn_x, ry + 3, compact ? 58 : 64, 19, 5, pal.danger_r, pal.danger_g, pal.danger_b, khov ? 0.30 : 0.16);
                ngfx_text(cr, "Noto Sans", 1, 10.0, btn_x + (compact ? 7 : 9), ry + 16, pal.danger_r, pal.danger_g, pal.danger_b, 1.0, "End Task");
                nature_add_hit(st, btn_x, ry + 3, compact ? 58 : 64, 19, HIT_MONITOR_KILL, p.pid, 0);

                if (!compact) {
                    int renice_x = px + pw - 134;
                    int rhov = (st->hover_mx >= renice_x && st->hover_mx <= renice_x + 56 && st->hover_my >= ry + 3 && st->hover_my <= ry + 22);
                    ngfx_fill_rect(cr, renice_x, ry + 3, 56, 19, 5, pal.elev_r, pal.elev_g, pal.elev_b, rhov ? 1.0 : 0.85);
                    ngfx_text(cr, "Noto Sans", 0, 10.0, renice_x + 9, ry + 16, fg_r, fg_g, fg_b, 1.0, "Renice");
                    nature_add_hit(st, renice_x, ry + 3, 56, 19, HIT_MONITOR_RENICE, p.pid, 0);

                    if (p.win_id >= 0) {
                        int foc_x = px + pw - 190;
                        int fhov = (st->hover_mx >= foc_x && st->hover_mx <= foc_x + 50 && st->hover_my >= ry + 3 && st->hover_my <= ry + 22);
                        ngfx_fill_rect(cr, foc_x, ry + 3, 50, 19, 5, ar, ag, ab, fhov ? 0.96 : 0.24);
                        ngfx_text(cr, "Noto Sans", 1, 9.8, foc_x + 9, ry + 16,
                                  fhov ? pal.on_acc_r : (dark ? pal.ar_hi : ar),
                                  fhov ? pal.on_acc_g : (dark ? pal.ag_hi : ag),
                                  fhov ? pal.on_acc_b : (dark ? pal.ab_hi : ab), 1.0, "Focus");
                        nature_add_hit(st, foc_x, ry + 3, 50, 19, HIT_MONITOR_FOCUS, p.win_id, 0);
                    }
                }
            }
            ry += row_h;
        }

        nature_draw_scrollbar(st, cr, cx + cw - 11, list_y, avail_h,
                              st->proc_count, vis_rows, st->monitor_scroll,
                              st->monitor_max_scroll, SCROLL_MONITOR, dark);
    }
}

void nature_monitor_wheel(NatureOSState *st, int delta_y) {
    if (st->monitor_tab != 1) return;
    int step = (delta_y > 0) ? 1 : -1;
    st->monitor_scroll += step;
    if (st->monitor_scroll < 0) st->monitor_scroll = 0;
    if (st->monitor_scroll > st->monitor_max_scroll) st->monitor_scroll = st->monitor_max_scroll;
}

void nature_monitor_click(NatureOSState *st, const HitRect *hit) {
    if (hit->action_id == HIT_MONITOR_TAB) {
        st->monitor_tab = hit->arg0;
        st->monitor_scroll = 0;
    } else if (hit->action_id == HIT_MONITOR_STRESS) {
        nature_monitor_set_stress(st, 1);
    } else if (hit->action_id == HIT_MONITOR_CLEAR_STRESS) {
        nature_monitor_set_stress(st, 0);
    } else if (hit->action_id == HIT_MONITOR_STRESS_THREADS) {
        nature_monitor_set_threads(st, hit->arg0);
    } else if (hit->action_id == HIT_MONITOR_KILL) {
        int pid = hit->arg0;
        for (int i = 0; i < st->proc_count; ++i) {
            if (st->procs[i].pid == pid && st->procs[i].killable) {
                if (st->procs[i].win_id >= 0) {
                    nature_close_window(st, st->procs[i].win_id);
                } else if (pid >= 4800) {
                    nature_monitor_set_stress(st, 0);
                }
                nature_notify(st, "Task Manager", "Process terminated cleanly.");
                break;
            }
        }
        nature_monitor_tick(st);
    } else if (hit->action_id == HIT_MONITOR_CHART_MODE) {
        st->monitor_chart_mode = hit->arg0 ? 1 : 0;
    } else if (hit->action_id == HIT_MONITOR_SORT) {
        st->monitor_sort = hit->arg0 % 3;
        nature_monitor_tick(st);
    } else if (hit->action_id == HIT_MONITOR_FOCUS) {
        int wid = hit->arg0;
        for (int i = 0; i < st->window_count; ++i) {
            if (st->windows[i].id == wid) {
                st->active_workspace = st->windows[i].workspace;
                nature_focus_window(st, wid);
                break;
            }
        }
    } else if (hit->action_id == HIT_MONITOR_RENICE) {
        int pid = hit->arg0;
        for (int i = 0; i < st->proc_count; ++i) {
            if (st->procs[i].pid == pid) {
                st->procs[i].nice_val = (st->procs[i].nice_val == 10) ? -5 : 10;
                break;
            }
        }
        nature_notify(st, "Task Scheduler", "Process scheduling nice priority adjusted.");
    }
}

} // extern "C"
