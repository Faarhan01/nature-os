#include "nature_os.h"
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static const char *APP_NAMES[APP_COUNT] = {
    "Files",
    "Sylva Browser",
    "Terminal",
    "Ubuntu Software",
    "System Monitor",
    "Media Viewer",
    "Settings"
};

static int str_contains_ci(const char *hay, const char *needle) {
    if (!needle || !*needle) return 1;
    size_t hlen = strlen(hay), nlen = strlen(needle);
    if (nlen > hlen) return 0;
    for (size_t i = 0; i <= hlen - nlen; ++i) {
        size_t j = 0;
        while (j < nlen && tolower((unsigned char)hay[i + j]) == tolower((unsigned char)needle[j])) j++;
        if (j == nlen) return 1;
    }
    return 0;
}

void nature_get_accent(int accent_idx, double *r, double *g, double *b) {
    switch (accent_idx) {
        case 1: *r = 0.059; *g = 0.463; *b = 0.431; break; /* Glacial Teal */
        case 2: *r = 0.851; *g = 0.467; *b = 0.024; break; /* Cedar Amber */
        case 3: *r = 0.145; *g = 0.388; *b = 0.922; break; /* Alpine Slate */
        default: *r = 0.180; *g = 0.545; *b = 0.341; break; /* Botanical Moss */
    }
}

void nature_notify(NatureOSState *st, const char *title, const char *msg) {
    if (!st || !title || !msg) return;
    snprintf(st->toast_title, sizeof(st->toast_title), "%s", title);
    snprintf(st->toast_msg, sizeof(st->toast_msg), "%s", msg);
    st->toast_timer = 180;
    int shift = st->notif_count < 4 ? st->notif_count : 3;
    for (int i = shift; i > 0; --i) {
        snprintf(st->notif_titles[i], sizeof(st->notif_titles[i]), "%s", st->notif_titles[i - 1]);
        snprintf(st->notif_msgs[i], sizeof(st->notif_msgs[i]), "%s", st->notif_msgs[i - 1]);
    }
    snprintf(st->notif_titles[0], sizeof(st->notif_titles[0]), "%s", title);
    snprintf(st->notif_msgs[0], sizeof(st->notif_msgs[0]), "%s", msg);
    if (st->notif_count < 4) st->notif_count++;
    st->dirty = 1;
}

void nature_add_hit(NatureOSState *st, int x, int y, int w, int h, int action_id, int arg0, int arg1) {
    if (st->hit_count >= MAX_HIT_RECTS || w <= 0 || h <= 0) return;
    HitRect *hr = &st->hits[st->hit_count++];
    hr->x = x; hr->y = y; hr->w = w; hr->h = h;
    hr->action_id = action_id;
    hr->arg0 = arg0;
    hr->arg1 = arg1;
}

void nature_add_text_rect(NatureOSState *st, int x, int y, int w, int h, int target_id) {
    if (st->text_rect_count >= MAX_TEXT_RECTS || w <= 0 || h <= 0) return;
    TextInputRect *tr = &st->text_rects[st->text_rect_count++];
    tr->x = x; tr->y = y; tr->w = w; tr->h = h;
    tr->target_id = target_id;
}

void nature_occlude_text_rects(NatureOSState *st, int x, int y, int w, int h) {
    int out = 0;
    for (int i = 0; i < st->text_rect_count; ++i) {
        TextInputRect tr = st->text_rects[i];
        int cx = tr.x + tr.w / 2;
        int cy = tr.y + tr.h / 2;
        if (cx >= x && cx <= x + w && cy >= y && cy <= y + h) {
            continue; /* Covered by higher window or modal */
        }
        st->text_rects[out++] = tr;
    }
    st->text_rect_count = out;
}

static int *get_scroll_ptr(NatureOSState *st, int target_kind) {
    switch (target_kind) {
        case SCROLL_FILES: return &st->files_scroll;
        case SCROLL_TERMINAL: return &st->term_scroll;
        case SCROLL_MONITOR: return &st->monitor_scroll;
        case SCROLL_SETTINGS: return &st->settings_scroll;
        case SCROLL_STORE: return &st->store_scroll;
        case SCROLL_BROWSER: return &st->browser_scroll;
        case SCROLL_MEDIA_AUDIO: return &st->audio_scroll;
        default: return NULL;
    }
}

void nature_draw_scrollbar(NatureOSState *st, cairo_t *cr, int x, int y, int h,
                           int total_items, int visible_items, int scroll_pos,
                           int max_scroll, int target_kind, int dark) {
    if (h < 32 || max_scroll <= 0 || total_items <= visible_items) return;

    double ar, ag, ab;
    nature_get_accent(st->cfg.accent_idx, &ar, &ag, &ab);

    int track_x = x;
    int track_y = y + 4;
    int track_h = h - 8;
    int track_w = 7;

    int hov = (st->hover_mx >= track_x - 8 && st->hover_mx <= track_x + 12 &&
               st->hover_my >= track_y && st->hover_my <= track_y + track_h) ||
              (st->drag_mode == 3 && st->scroll_drag_target == target_kind);

    /* Subtle track */
    ngfx_fill_rect(cr, track_x, track_y, track_w, track_h, 3.5,
                   dark ? 1.0 : 0.0, dark ? 1.0 : 0.0, dark ? 1.0 : 0.0,
                   hov ? 0.14 : 0.06);

    double ratio = (double)visible_items / (double)total_items;
    if (ratio < 0.14) ratio = 0.14;
    if (ratio > 0.92) ratio = 0.92;
    int thumb_h = (int)(track_h * ratio);
    if (thumb_h < 26) thumb_h = 26;

    double frac = max_scroll > 0 ? (double)scroll_pos / (double)max_scroll : 0.0;
    if (frac < 0.0) frac = 0.0;
    if (frac > 1.0) frac = 1.0;

    /* Note: Terminal scroll_pos=0 means pinned to bottom */
    double visual_frac = (target_kind == SCROLL_TERMINAL) ? (1.0 - frac) : frac;
    int thumb_y = track_y + (int)((track_h - thumb_h) * visual_frac);

    if (hov) {
        ngfx_fill_rect(cr, track_x - 1, thumb_y, track_w + 2, thumb_h, 4.5, ar, ag, ab, 0.92);
    } else {
        ngfx_fill_rect(cr, track_x, thumb_y, track_w, thumb_h, 3.5,
                       dark ? 0.72 : 0.30, dark ? 0.82 : 0.36, dark ? 0.77 : 0.33, 0.45);
    }

    /* Generous 18px touch/mouse hit area */
    nature_add_hit(st, track_x - 7, track_y, 18, track_h, HIT_SCROLLBAR, target_kind, max_scroll);
}

static void add_vfs(NatureOSState *st, const char *name, const char *path,
                    const char *parent, int is_dir, int size_bytes,
                    int img_idx, int audio_idx, const char *mime_tag,
                    const char *modified, const char *content) {
    if (st->vfs_count >= MAX_VFS) return;
    VfsNode *n = &st->vfs[st->vfs_count++];
    memset(n, 0, sizeof(*n));
    snprintf(n->name, sizeof(n->name), "%s", name);
    snprintf(n->path, sizeof(n->path), "%s", path);
    snprintf(n->parent, sizeof(n->parent), "%s", parent);
    n->is_dir = is_dir;
    n->size_bytes = size_bytes;
    n->image_idx = img_idx;
    n->audio_idx = audio_idx;
    snprintf(n->mime_tag, sizeof(n->mime_tag), "%s", mime_tag ? mime_tag : (is_dir ? "DIR" : "TXT"));
    snprintf(n->modified, sizeof(n->modified), "%s", modified ? modified : "Nov 25 2026");
    if (content) snprintf(n->content, sizeof(n->content), "%s", content);
}

static cairo_surface_t *load_png(const char *asset_dir, const char *filename) {
    char full[256];
    snprintf(full, sizeof(full), "%s/%s", asset_dir, filename);
    return g_ngfx.image_surface_create_from_png(full);
}

static cairo_surface_t *create_aurora_art(void) {
    int w = 960, h = 540;
    cairo_surface_t *s = g_ngfx.image_surface_create(CAIRO_FORMAT_ARGB32, w, h);
    cairo_t *cr = g_ngfx.create(s);

    ngfx_fill_gradient(cr, 0, 0, w, h, 0, 0.02, 0.05, 0.09, 1.0, 0.04, 0.14, 0.16, 1.0, 0);
    ngfx_fill_gradient(cr, 0, 60, w, 260, 0, 0.08, 0.72, 0.48, 0.36, 0.05, 0.25, 0.45, 0.0, 0);
    ngfx_fill_circle(cr, 720, 120, 96, 0.65, 0.92, 0.78, 0.14);
    ngfx_fill_circle(cr, 720, 120, 42, 0.85, 0.98, 0.92, 0.45);

    g_ngfx.set_source_rgba(cr, 0.02, 0.06, 0.05, 0.95);
    g_ngfx.move_to(cr, 0, h);
    g_ngfx.line_to(cr, 0, 390);
    g_ngfx.line_to(cr, 180, 310);
    g_ngfx.line_to(cr, 370, 385);
    g_ngfx.line_to(cr, 560, 275);
    g_ngfx.line_to(cr, 780, 360);
    g_ngfx.line_to(cr, 960, 300);
    g_ngfx.line_to(cr, 960, h);
    g_ngfx.close_path(cr);
    g_ngfx.fill(cr);

    g_ngfx.destroy(cr);
    return s;
}

static cairo_surface_t *create_canopy_rings_art(void) {
    int w = 960, h = 540;
    cairo_surface_t *s = g_ngfx.image_surface_create(CAIRO_FORMAT_ARGB32, w, h);
    cairo_t *cr = g_ngfx.create(s);

    ngfx_fill_gradient(cr, 0, 0, w, h, 0, 0.04, 0.09, 0.07, 1.0, 0.09, 0.22, 0.16, 1.0, 1);
    for (int r = 240; r >= 40; r -= 32) {
        g_ngfx.set_source_rgba(cr, 0.24, 0.78, 0.52, 0.14 + (240 - r) * 0.0012);
        g_ngfx.set_line_width(cr, 2.2);
        g_ngfx.new_sub_path(cr);
        g_ngfx.arc(cr, w * 0.5, h * 0.5, r, 0, 6.28318);
        g_ngfx.stroke(cr);
    }
    ngfx_fill_circle(cr, w * 0.5, h * 0.5, 28, 0.35, 0.90, 0.60, 0.85);
    g_ngfx.destroy(cr);
    return s;
}

static void clamp_window_to_screen(NatureOSState *st, OSWindow *w) {
    int sw = st->screen_w;
    int sh = st->screen_h;
    int left_off = st->cfg.dock_position == 1 ? (sw < 640 ? 58 : 74) : 8;
    int bot_off = st->cfg.dock_position == 0 ? (sh < 540 ? 64 : 74) : 8;
    int min_w = sw < 480 ? sw - 16 : 360;
    int min_h = sh < 420 ? sh - 100 : 250;

    if (w->maximized) {
        w->x = left_off;
        w->y = 46;
        w->w = sw - left_off - 8;
        w->h = sh - 46 - bot_off;
        return;
    }
    if (w->snapped == 1 || w->snapped == 2) {
        int half = (sw - left_off - 16) / 2;
        if (sw < 620) {
            w->x = left_off;
            w->y = 46;
            w->w = sw - left_off - 8;
            w->h = sh - 46 - bot_off;
        } else {
            w->x = (w->snapped == 1) ? left_off : (left_off + half + 8);
            w->y = 46;
            w->w = half;
            w->h = sh - 46 - bot_off;
        }
        return;
    }

    int max_w = sw - left_off - 8;
    int max_h = sh - 46 - bot_off;
    if (max_w < 280) max_w = sw - 12;
    if (max_h < 220) max_h = sh - 100;

    if (w->w > max_w) w->w = max_w;
    if (w->h > max_h) w->h = max_h;
    if (w->w < min_w && min_w <= max_w) w->w = min_w;
    if (w->h < min_h && min_h <= max_h) w->h = min_h;

    if (w->x + w->w > sw - 6) w->x = sw - w->w - 6;
    if (w->x < left_off) w->x = left_off;
    if (w->y + w->h > sh - bot_off) w->y = sh - bot_off - w->h;
    if (w->y < 46) w->y = 46;
}

void nature_os_resize(NatureOSState *st, int w, int h) {
    if (w < 360) w = 360;
    if (w > 1920) w = 1920;
    if (h < 360) h = 360;
    if (h > 1440) h = 1440;
    int prev_w = st->screen_w;
    int prev_h = st->screen_h;
    st->screen_w = w;
    st->screen_h = h;

    /* Proportionally adapt floating windows so smooth resizes never bunch into top-left */
    if (prev_w > 0 && prev_h > 0 && (prev_w != w || prev_h != h)) {
        double sx = (double)w / (double)prev_w;
        double sy = (double)h / (double)prev_h;
        for (int i = 0; i < st->window_count; ++i) {
            OSWindow *win = &st->windows[i];
            if (!win->maximized && win->snapped == 0) {
                win->x = (int)lround(win->x * sx);
                win->y = (int)lround(win->y * sy);
                win->w = (int)lround(win->w * sx);
                win->h = (int)lround(win->h * sy);
            }
        }
    }

    /* Smart layout adaptation on sudden orientation / aspect flips */
    int was_wide = (prev_w >= 800 && prev_w >= prev_h);
    int is_portrait = (w < 780 && h > w);
    int left_off = st->cfg.dock_position == 1 ? (w < 640 ? 58 : 74) : 10;
    int bot_off = st->cfg.dock_position == 0 ? (h < 540 ? 64 : 74) : 10;

    if (is_portrait && was_wide) {
        int vis_idx[2] = {-1, -1};
        int vc = 0;
        for (int i = 0; i < st->window_count && vc < 2; ++i) {
            if (!st->windows[i].minimized && st->windows[i].workspace == st->active_workspace) {
                vis_idx[vc++] = i;
            }
        }
        if (vc == 2) {
            int avail_h = h - 48 - bot_off;
            int each_h = (avail_h - 10) / 2;
            if (each_h >= 210) {
                for (int k = 0; k < 2; ++k) {
                    OSWindow *win = &st->windows[vis_idx[k]];
                    win->maximized = 0;
                    win->snapped = 0;
                    win->x = left_off;
                    win->w = w - left_off - 10;
                    win->y = 48 + k * (each_h + 10);
                    win->h = each_h;
                }
            }
        }
    } else if (!is_portrait && prev_w > 0 && prev_w < 780 && w >= 900) {
        int vis_idx[2] = {-1, -1};
        int vc = 0;
        for (int i = 0; i < st->window_count && vc < 2; ++i) {
            if (!st->windows[i].minimized && st->windows[i].workspace == st->active_workspace) {
                vis_idx[vc++] = i;
            }
        }
        if (vc == 2) {
            int half_w = (w - left_off - 24) / 2;
            int avail_h = h - 54 - bot_off;
            for (int k = 0; k < 2; ++k) {
                OSWindow *win = &st->windows[vis_idx[k]];
                win->maximized = 0;
                win->snapped = 0;
                win->x = left_off + k * (half_w + 12);
                win->y = 52 + k * 20;
                win->w = half_w;
                win->h = avail_h - 24;
            }
        }
    }

    for (int i = 0; i < st->window_count; ++i) {
        clamp_window_to_screen(st, &st->windows[i]);
    }
    st->dirty = 1;
}

void nature_os_init(NatureOSState *st, const char *asset_dir) {
    memset(st, 0, sizeof(*st));
    st->screen_w = OS_WIDTH;
    st->screen_h = OS_HEIGHT;
    st->cfg.dark_mode = 1;
    st->cfg.wallpaper_idx = 0;
    st->cfg.accent_idx = 0;
    st->cfg.dock_position = 0;
    st->cfg.low_power_mode = 0;
    st->cfg.zram_enabled = 1;
    st->cfg.wifi_enabled = 1;
    st->cfg.bluetooth_enabled = 1;
    st->cfg.night_light = 0;
    st->cfg.audio_muted = 0;
    st->cfg.audio_device = 0;
    st->cfg.target_hz = 60;
    st->cfg.volume = 76;
    st->cfg.brightness = 100;
    st->cfg.power_profile = 1;

    st->active_workspace = 0;
    st->next_z = 10;
    st->next_win_id = 1;
    st->next_pid = 1200;
    st->hover_dock_idx = -1;
    st->files_selected_idx = -1;
    st->media_selected = 0;
    st->media_zoom_pct = 100;
    st->measured_fps = 60.0;
    st->frame_ms = 2.0;
    st->total_ram_mb = 16384;
    st->used_ram_mb = 304;
    st->zram_used_mb = 18;
    snprintf(st->browser_url_input, sizeof(st->browser_url_input), "nature://portal");
    st->dirty = 1;

    const char *dir = (asset_dir && *asset_dir) ? asset_dir : "/usr/share/nature-os/assets";
    st->wallpapers[0] = load_png(dir, "wallpaper_canopy.png");
    st->wallpapers[1] = load_png(dir, "wallpaper_alpine.png");
    st->wallpapers[2] = load_png(dir, "wallpaper_moss.png");

    st->media_images[0] = load_png(dir, "media_fern.png");
    st->media_images[1] = st->wallpapers[0];
    st->media_images[2] = st->wallpapers[1];
    st->media_images[3] = st->wallpapers[2];
    st->media_images[4] = create_aurora_art();
    st->media_images[5] = create_canopy_rings_art();
    st->avatar_surface = load_png(dir, "avatar_user.png");

    snprintf(st->files_cwd, sizeof(st->files_cwd), "/home/nature");
    snprintf(st->term_cwd, sizeof(st->term_cwd), "/home/nature");

    /* Directories */
    add_vfs(st, "/", "/", "", 1, 4096, -1, -1, "DIR", "Nov 25 2026", NULL);
    add_vfs(st, "home", "/home", "/", 1, 4096, -1, -1, "DIR", "Nov 25 2026", NULL);
    add_vfs(st, "nature", "/home/nature", "/home", 1, 4096, -1, -1, "DIR", "Nov 25 2026", NULL);
    add_vfs(st, "Desktop", "/home/nature/Desktop", "/home/nature", 1, 4096, -1, -1, "DIR", "Nov 25 2026", NULL);
    add_vfs(st, "Documents", "/home/nature/Documents", "/home/nature", 1, 4096, -1, -1, "DIR", "Nov 25 2026", NULL);
    add_vfs(st, "Downloads", "/home/nature/Downloads", "/home/nature", 1, 4096, -1, -1, "DIR", "Nov 25 2026", NULL);
    add_vfs(st, "Music", "/home/nature/Music", "/home/nature", 1, 4096, -1, -1, "DIR", "Nov 25 2026", NULL);
    add_vfs(st, "Pictures", "/home/nature/Pictures", "/home/nature", 1, 4096, -1, -1, "DIR", "Nov 25 2026", NULL);
    add_vfs(st, "Projects", "/home/nature/Projects", "/home/nature", 1, 4096, -1, -1, "DIR", "Nov 25 2026", NULL);
    add_vfs(st, "etc", "/etc", "/", 1, 4096, -1, -1, "DIR", "Nov 25 2026", NULL);
    add_vfs(st, "nature-os", "/etc/nature-os", "/etc", 1, 4096, -1, -1, "DIR", "Nov 25 2026", NULL);
    add_vfs(st, "var", "/var", "/", 1, 4096, -1, -1, "DIR", "Nov 25 2026", NULL);
    add_vfs(st, "log", "/var/log", "/var", 1, 4096, -1, -1, "DIR", "Nov 25 2026", NULL);

    /* /home/nature root quick files */
    add_vfs(st, "welcome.txt", "/home/nature/welcome.txt", "/home/nature", 0, 340, -1, -1, "TXT", "Nov 25 2026",
            "Welcome to Nature OS 24.04.1 LTS (Noble Numbat).\nEngineered in C17 & C++20 for minimal RAM/CPU overhead.\nRight-click or long-press desktop for quick actions.");
    add_vfs(st, ".bashrc", "/home/nature/.bashrc", "/home/nature", 0, 412, -1, -1, "CONF", "Nov 24 2026",
            "# Nature OS Ubuntu Shell Profile\nexport NATURE_COMPOSITOR=wlroots\nexport EDITOR=nano\nalias ll='ls -la'");

    /* Desktop */
    add_vfs(st, "QuickStart.md", "/home/nature/Desktop/QuickStart.md", "/home/nature/Desktop", 0, 512, -1, -1, "MD", "Nov 25 2026",
            "# Quick Start Guide\n- Super key opens Activities & App Search\n- Drag window edges to snap left/right\n- Drag scrollbars or swipe to scroll inside any app");
    add_vfs(st, "nature-manual.txt", "/home/nature/Desktop/nature-manual.txt", "/home/nature/Desktop", 0, 420, -1, -1, "TXT", "Nov 25 2026",
            "Nature OS Shortcuts:\n- Super / Activities: App Overview\n- Right-Click / Long-Press Desktop: Context Menu\n- Media Viewer: View 4K photos & play PipeWire FLAC audio");

    /* Documents */
    add_vfs(st, "os-release", "/home/nature/Documents/os-release", "/home/nature/Documents", 0, 290, -1, -1, "CONF", "Nov 25 2026",
            "NAME=\"Nature OS\"\nVERSION=\"24.04.1 LTS (Canopy)\"\nID=natureos\nID_LIKE=ubuntu debian\nUBUNTU_CODENAME=noble");
    add_vfs(st, "zram-tuning.txt", "/home/nature/Documents/zram-tuning.txt", "/home/nature/Documents", 0, 412, -1, -1, "TXT", "Nov 25 2026",
            "zram0 algorithm=lz4\nvm.swappiness=120\nidle_rss=304MiB\ncompositor=wlroots-damage");
    add_vfs(st, "architecture-notes.md", "/home/nature/Documents/architecture-notes.md", "/home/nature/Documents", 0, 680, -1, -1, "MD", "Nov 25 2026",
            "# Nature OS Stack\n1. C17 wlroots/Cairo damage compositor\n2. C++20 System Monitor & Ubuntu Store\n3. systemd cgroup v2 CPUWeight=300 UI shield");
    add_vfs(st, "ubuntu-packages.list", "/home/nature/Documents/ubuntu-packages.list", "/home/nature/Documents", 0, 520, -1, -1, "TXT", "Nov 23 2026",
            "linux-image-6.8.0-45-lowlatency\nnature-compositor\nnature-shell\npipewire-audio\nnetwork-manager\nzram-tools");
    add_vfs(st, "wayland-protocol.md", "/home/nature/Documents/wayland-protocol.md", "/home/nature/Documents", 0, 610, -1, -1, "MD", "Nov 22 2026",
            "xdg-shell surface scaling & damage tracking notes.\nAdaptive output sizing supports portrait, square, and ultrawide.");

    /* Pictures (6 rich images) */
    add_vfs(st, "woodland-fern.jpg", "/home/nature/Pictures/woodland-fern.jpg", "/home/nature/Pictures", 0, 3565158, 0, -1, "IMG", "Nov 25 2026",
            "Leica M11 35mm · f/2.0 · 2400x1800\nTap twice to open in Media Viewer.");
    add_vfs(st, "canopy-dawn.jpg", "/home/nature/Pictures/canopy-dawn.jpg", "/home/nature/Pictures", 0, 5033164, 1, -1, "IMG", "Nov 25 2026",
            "Hasselblad X2D 100C · 3840x2160 4K HDR");
    add_vfs(st, "alpine-twilight.jpg", "/home/nature/Pictures/alpine-twilight.jpg", "/home/nature/Pictures", 0, 4404019, 2, -1, "IMG", "Nov 25 2026",
            "Sony A7R V · 3840x2160 4K");
    add_vfs(st, "basalt-moss.jpg", "/home/nature/Pictures/basalt-moss.jpg", "/home/nature/Pictures", 0, 4089446, 3, -1, "IMG", "Nov 25 2026",
            "Fujifilm GFX 100S · Macro 80mm");
    add_vfs(st, "aurora-horizon.png", "/home/nature/Pictures/aurora-horizon.png", "/home/nature/Pictures", 0, 2890120, 4, -1, "IMG", "Nov 25 2026",
            "Procedural Aurora Horizon · 4K HDR Canvas");
    add_vfs(st, "canopy-rings.png", "/home/nature/Pictures/canopy-rings.png", "/home/nature/Pictures", 0, 1984320, 5, -1, "IMG", "Nov 25 2026",
            "Verdant Botanical Rings Vector Render");

    /* Music (3 playable sample audio tracks) */
    add_vfs(st, "rainforest-canopy.flac", "/home/nature/Music/rainforest-canopy.flac", "/home/nature/Music", 0, 38402000, -1, 0, "AUD", "Nov 25 2026",
            "Artist: Nature Soundscapes\nFormat: 24-bit / 96 kHz FLAC\nDuration: 03:42 · Tap Open in Media to play.");
    add_vfs(st, "alpine-wind.ogg", "/home/nature/Music/alpine-wind.ogg", "/home/nature/Music", 0, 8940000, -1, 1, "AUD", "Nov 24 2026",
            "Artist: Nordic Ambient Studio\nFormat: 48 kHz Opus/OGG\nDuration: 04:18");
    add_vfs(st, "night-moss-stream.wav", "/home/nature/Music/night-moss-stream.wav", "/home/nature/Music", 0, 31200000, -1, 2, "AUD", "Nov 23 2026",
            "Artist: Olympic Field Archive\nFormat: 48 kHz WAV\nDuration: 02:55");

    /* Downloads */
    add_vfs(st, "nature-os-24.04.1-amd64.iso", "/home/nature/Downloads/nature-os-24.04.1-amd64.iso", "/home/nature/Downloads", 0, 1524713390, -1, -1, "ISO", "Nov 25 2026",
            "SHA256: 9f4e812c08e5a7f902...\nBootable Ubuntu 24.04 LTS + Nature OS image.");
    add_vfs(st, "mesa-vulkan-24.0.9.deb", "/home/nature/Downloads/mesa-vulkan-24.0.9.deb", "/home/nature/Downloads", 0, 50541200, -1, -1, "DEB", "Nov 24 2026",
            "Package: mesa-vulkan-drivers\nArchitecture: amd64\nStatus: verified signature");
    add_vfs(st, "jetbrains-mono-fonts.tar.xz", "/home/nature/Downloads/jetbrains-mono-fonts.tar.xz", "/home/nature/Downloads", 0, 23488102, -1, -1, "TXT", "Nov 22 2026",
            "Compressed system monospace font archive.");

    /* Projects */
    add_vfs(st, "render_scheduler.c", "/home/nature/Projects/render_scheduler.c", "/home/nature/Projects", 0, 1240, -1, -1, "C", "Nov 25 2026",
            "void nature_scheduler_init(struct nature_scheduler *s, uint32_t hz) {\n  s->target_hz = hz;\n  s->input_priority_boost = true;\n}");
    add_vfs(st, "fb_transport.cpp", "/home/nature/Projects/fb_transport.cpp", "/home/nature/Projects", 0, 4820, -1, -1, "CPP", "Nov 25 2026",
            "// Single-pass NAT3 zlib dirty-tile framebuffer transport\ng_wake_cv.notify_one(); // zero-latency input wake");
    add_vfs(st, "meson.build", "/home/nature/Projects/meson.build", "/home/nature/Projects", 0, 920, -1, -1, "CONF", "Nov 24 2026",
            "project('nature-os', ['c', 'cpp'], version: '24.04.1')\ndep_cairo = dependency('cairo')");

    /* /etc/nature-os */
    add_vfs(st, "compositor.conf", "/etc/nature-os/compositor.conf", "/etc/nature-os", 0, 296, -1, -1, "CONF", "Nov 25 2026",
            "[compositor]\nbackend=drm-kms\ndamage_tracking=full\nzero_copy_dmabuf=true\nadaptive_scaling=true");
    add_vfs(st, "zram.conf", "/etc/nature-os/zram.conf", "/etc/nature-os", 0, 184, -1, -1, "CONF", "Nov 25 2026",
            "[zram0]\nzram-size = ram / 2\ncompression-algorithm = lz4\nswap-priority = 100");

    /* /var/log */
    add_vfs(st, "nature-compositor.log", "/var/log/nature-compositor.log", "/var/log", 0, 1820, -1, -1, "LOG", "Nov 25 2026",
            "[0.042] DRM/KMS adaptive mode bound\n[0.088] Cairo + zlib NAT3 pipeline initialized\n[0.145] nature-shell ready (304 MiB RSS)");
    add_vfs(st, "syslog", "/var/log/syslog", "/var/log", 0, 4210, -1, -1, "LOG", "Nov 25 2026",
            "Nov 25 systemd[1]: Started nature-session.service.\nNov 25 NetworkManager: wlp2s0 link up 866Mbps\nNov 25 pipewire: Quantum 1024/48000 active");

    snprintf(st->term_lines[0], 128, "Nature OS 24.04.1 LTS (Ubuntu Noble) -- C/C++ Native Compositor");
    st->term_line_colors[0] = 2;
    snprintf(st->term_lines[1], 128, "Type 'help', 'neofetch', 'htop', 'df -h', or tap a command chip above");
    st->term_line_colors[1] = 0;
    st->term_line_count = 2;
    st->term_hist_idx = -1;

    nature_monitor_init(st);
    nature_store_init(st);

    OSWindow *mon = nature_open_app(st, APP_MONITOR);
    if (mon) { mon->x = 46; mon->y = 54; mon->w = 590; mon->h = 436; clamp_window_to_screen(st, mon); }
    OSWindow *term = nature_open_app(st, APP_TERMINAL);
    if (term) { term->x = 656; term->y = 84; term->w = 576; term->h = 416; clamp_window_to_screen(st, term); }
}

OSWindow *nature_open_app(NatureOSState *st, AppType app) {
    st->launcher_open = 0;
    st->launcher_query[0] = '\0';
    st->quick_settings_open = 0;
    st->ctx_menu_open = 0;
    for (int i = 0; i < st->window_count; ++i) {
        if (st->windows[i].app == app) {
            st->windows[i].minimized = 0;
            st->active_workspace = st->windows[i].workspace;
            clamp_window_to_screen(st, &st->windows[i]);
            nature_focus_window(st, st->windows[i].id);
            return &st->windows[i];
        }
    }
    if (st->window_count >= MAX_WINDOWS) return NULL;

    OSWindow *w = &st->windows[st->window_count++];
    memset(w, 0, sizeof(*w));
    w->id = st->next_win_id++;
    w->app = app;
    snprintf(w->title, sizeof(w->title), "%s", APP_NAMES[app]);
    int offset = (st->window_count - 1) * 24;
    w->w = (app == APP_BROWSER || app == APP_STORE || app == APP_FILES) ? 740 : 660;
    w->h = 464;
    w->x = 70 + (offset % 160);
    w->y = 52 + (offset % 96);
    if (st->screen_w < 680) {
        w->maximized = 1;
    }
    clamp_window_to_screen(st, w);
    w->z_order = st->next_z++;
    w->workspace = st->active_workspace;
    w->pid = st->next_pid;
    st->next_pid += 7;
    st->active_window_id = w->id;
    st->dirty = 1;
    return w;
}

void nature_focus_window(NatureOSState *st, int win_id) {
    for (int i = 0; i < st->window_count; ++i) {
        if (st->windows[i].id == win_id) {
            st->windows[i].z_order = st->next_z++;
            st->windows[i].minimized = 0;
            st->active_window_id = win_id;
            st->dirty = 1;
            break;
        }
    }
}

void nature_close_window(NatureOSState *st, int win_id) {
    for (int i = 0; i < st->window_count; ++i) {
        if (st->windows[i].id == win_id) {
            for (int j = i; j < st->window_count - 1; ++j) st->windows[j] = st->windows[j + 1];
            st->window_count--;
            if (st->active_window_id == win_id) {
                st->active_window_id = st->window_count > 0 ? st->windows[st->window_count - 1].id : -1;
            }
            st->dirty = 1;
            break;
        }
    }
}

static void draw_app_icon(cairo_t *cr, AppType app, double x, double y, double sz, double ar, double ag, double ab) {
    double pad = sz * 0.17;
    double ix = x + pad, iy = y + pad, iw = sz - pad * 2, ih = sz - pad * 2;
    double r = sz * 0.27;
    double cx = x + sz * 0.5, cy = y + sz * 0.5;

    g_ngfx.save(cr);
    g_ngfx.set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
    g_ngfx.set_line_join(cr, CAIRO_LINE_JOIN_ROUND);

    switch (app) {
        case APP_FILES: {
            ngfx_fill_gradient(cr, x, y, sz, sz, r, 0.12, 0.42, 0.29, 1.0, 0.06, 0.20, 0.15, 1.0, 0);
            ngfx_stroke_rect(cr, x + 0.5, y + 0.5, sz - 1, sz - 1, r, 1.0, 1.0, 1.0, 1.0, 0.20);
            ngfx_top_specular(cr, x, y, sz, r, 0.34);
            /* Folder back & tab */
            ngfx_fill_rect(cr, ix, iy + ih * 0.08, iw * 0.46, ih * 0.26, sz * 0.07, 0.86, 0.63, 0.18, 1.0);
            ngfx_fill_rect(cr, ix, iy + ih * 0.16, iw, ih * 0.28, sz * 0.09, 0.78, 0.56, 0.14, 1.0);
            /* Folder front flap with warm gold sheen */
            ngfx_fill_gradient(cr, ix, iy + ih * 0.26, iw, ih * 0.66, sz * 0.11,
                               0.99, 0.84, 0.38, 1.0, 0.87, 0.62, 0.15, 1.0, 0);
            ngfx_stroke_rect(cr, ix + 0.5, iy + ih * 0.26 + 0.5, iw - 1, ih * 0.66 - 1, sz * 0.11,
                             0.8, 1.0, 0.96, 0.76, 0.45);
            break;
        }
        case APP_BROWSER: {
            ngfx_fill_gradient(cr, x, y, sz, sz, r, 0.08, 0.36, 0.62, 1.0, 0.03, 0.14, 0.28, 1.0, 0);
            ngfx_stroke_rect(cr, x + 0.5, y + 0.5, sz - 1, sz - 1, r, 1.0, 1.0, 1.0, 1.0, 0.22);
            ngfx_top_specular(cr, x, y, sz, r, 0.36);
            ngfx_radial_glow(cr, cx, cy, 0.0, iw * 0.58, 0.24, 0.82, 0.96, 0.26, 0.0);
            /* Outer compass dial */
            g_ngfx.set_source_rgba(cr, 0.55, 0.92, 1.0, 0.90);
            g_ngfx.set_line_width(cr, sz * 0.048 > 1.3 ? sz * 0.048 : 1.3);
            g_ngfx.new_sub_path(cr);
            g_ngfx.arc(cr, cx, cy, iw * 0.44, 0, 6.28318);
            g_ngfx.stroke(cr);
            /* Rotated diamond compass needle */
            double rad = iw * 0.34;
            double side = iw * 0.11;
            /* North bright needle */
            g_ngfx.set_source_rgba(cr, 0.98, 0.45, 0.36, 0.98);
            g_ngfx.move_to(cr, cx + rad * 0.70, cy - rad * 0.70);
            g_ngfx.line_to(cr, cx - side * 0.70, cy - side * 0.70);
            g_ngfx.line_to(cr, cx + side * 0.70, cy + side * 0.70);
            g_ngfx.close_path(cr);
            g_ngfx.fill(cr);
            /* South frost needle */
            g_ngfx.set_source_rgba(cr, 0.92, 0.98, 1.0, 0.96);
            g_ngfx.move_to(cr, cx - rad * 0.70, cy + rad * 0.70);
            g_ngfx.line_to(cr, cx - side * 0.70, cy - side * 0.70);
            g_ngfx.line_to(cr, cx + side * 0.70, cy + side * 0.70);
            g_ngfx.close_path(cr);
            g_ngfx.fill(cr);
            ngfx_fill_circle(cr, cx, cy, iw * 0.075, 0.10, 0.22, 0.34, 1.0);
            break;
        }
        case APP_TERMINAL: {
            ngfx_fill_gradient(cr, x, y, sz, sz, r, 0.11, 0.15, 0.13, 1.0, 0.04, 0.06, 0.05, 1.0, 0);
            ngfx_stroke_rect(cr, x + 0.5, y + 0.5, sz - 1, sz - 1, r, 1.0, 0.34, 0.88, 0.56, 0.42);
            ngfx_top_specular(cr, x, y, sz, r, 0.28);
            /* Subtle phosphor top bar + vector chevron & underscore */
            ngfx_radial_glow(cr, x + sz * 0.40, cy, 0.0, sz * 0.42, 0.22, 0.90, 0.54, 0.14, 0.0);
            double lw = sz * 0.065 > 1.6 ? sz * 0.065 : 1.6;
            g_ngfx.set_line_width(cr, lw);
            g_ngfx.set_source_rgba(cr, 0.34, 0.95, 0.60, 1.0);
            g_ngfx.move_to(cr, ix + iw * 0.08, iy + ih * 0.28);
            g_ngfx.line_to(cr, ix + iw * 0.40, iy + ih * 0.50);
            g_ngfx.line_to(cr, ix + iw * 0.08, iy + ih * 0.72);
            g_ngfx.stroke(cr);
            ngfx_fill_rect(cr, ix + iw * 0.48, iy + ih * 0.64, iw * 0.40, lw * 1.15, 1.5, 0.90, 0.98, 0.94, 0.96);
            break;
        }
        case APP_STORE: {
            ngfx_fill_gradient(cr, x, y, sz, sz, r, 0.93, 0.42, 0.16, 1.0, 0.66, 0.21, 0.06, 1.0, 0);
            ngfx_stroke_rect(cr, x + 0.5, y + 0.5, sz - 1, sz - 1, r, 1.0, 1.0, 1.0, 1.0, 0.26);
            ngfx_top_specular(cr, x, y, sz, r, 0.36);
            /* Handle arch */
            g_ngfx.set_source_rgba(cr, 1.0, 0.95, 0.90, 0.95);
            g_ngfx.set_line_width(cr, sz * 0.055 > 1.5 ? sz * 0.055 : 1.5);
            g_ngfx.new_sub_path(cr);
            g_ngfx.arc(cr, cx, iy + ih * 0.30, iw * 0.21, 3.14159, 6.28318);
            g_ngfx.stroke(cr);
            /* Shopping tote body */
            ngfx_fill_gradient(cr, ix + iw * 0.11, iy + ih * 0.28, iw * 0.78, ih * 0.66, sz * 0.11,
                               1.0, 0.98, 0.95, 0.96, 0.95, 0.88, 0.82, 0.94, 0);
            /* Accent emblem circle inside tote */
            ngfx_fill_circle(cr, cx, iy + ih * 0.62, iw * 0.17, 0.88, 0.34, 0.12, 0.92);
            ngfx_fill_circle(cr, cx, iy + ih * 0.62, iw * 0.075, 1.0, 0.97, 0.93, 1.0);
            break;
        }
        case APP_MONITOR: {
            ngfx_fill_gradient(cr, x, y, sz, sz, r, 0.08, 0.19, 0.16, 1.0, 0.03, 0.08, 0.07, 1.0, 0);
            ngfx_stroke_rect(cr, x + 0.5, y + 0.5, sz - 1, sz - 1, r, 1.0, 0.28, 0.88, 0.62, 0.38);
            ngfx_top_specular(cr, x, y, sz, r, 0.30);
            /* Faint oscilloscope baseline */
            g_ngfx.set_source_rgba(cr, 0.25, 0.92, 0.62, 0.18);
            g_ngfx.set_line_width(cr, 1.0);
            g_ngfx.move_to(cr, ix, cy + sz * 0.06);
            g_ngfx.line_to(cr, ix + iw, cy + sz * 0.06);
            g_ngfx.stroke(cr);
            /* Glowing ECG trace */
            double lw = sz * 0.055 > 1.5 ? sz * 0.055 : 1.5;
            g_ngfx.set_source_rgba(cr, 0.24, 0.94, 0.64, 0.28);
            g_ngfx.set_line_width(cr, lw * 2.1);
            g_ngfx.move_to(cr, ix, y + sz * 0.56);
            g_ngfx.line_to(cr, ix + iw * 0.25, y + sz * 0.56);
            g_ngfx.line_to(cr, ix + iw * 0.42, y + sz * 0.24);
            g_ngfx.line_to(cr, ix + iw * 0.60, y + sz * 0.76);
            g_ngfx.line_to(cr, ix + iw * 0.76, y + sz * 0.44);
            g_ngfx.line_to(cr, ix + iw, y + sz * 0.44);
            g_ngfx.stroke(cr);

            g_ngfx.set_source_rgba(cr, 0.42, 0.98, 0.74, 1.0);
            g_ngfx.set_line_width(cr, lw);
            g_ngfx.move_to(cr, ix, y + sz * 0.56);
            g_ngfx.line_to(cr, ix + iw * 0.25, y + sz * 0.56);
            g_ngfx.line_to(cr, ix + iw * 0.42, y + sz * 0.24);
            g_ngfx.line_to(cr, ix + iw * 0.60, y + sz * 0.76);
            g_ngfx.line_to(cr, ix + iw * 0.76, y + sz * 0.44);
            g_ngfx.line_to(cr, ix + iw, y + sz * 0.44);
            g_ngfx.stroke(cr);
            ngfx_fill_circle(cr, ix + iw, y + sz * 0.44, lw * 0.9, 0.85, 1.0, 0.92, 1.0);
            break;
        }
        case APP_MEDIA: {
            ngfx_fill_gradient(cr, x, y, sz, sz, r, 0.16, 0.48, 0.40, 1.0, 0.06, 0.20, 0.19, 1.0, 0);
            ngfx_stroke_rect(cr, x + 0.5, y + 0.5, sz - 1, sz - 1, r, 1.0, 1.0, 1.0, 1.0, 0.24);
            ngfx_top_specular(cr, x, y, sz, r, 0.34);
            double fy = iy + ih * 0.08, fh = ih * 0.84;
            ngfx_fill_gradient(cr, ix, fy, iw, fh, sz * 0.11, 0.92, 0.98, 0.95, 0.96, 0.74, 0.90, 0.84, 0.96, 0);
            /* Warm sun orb */
            ngfx_fill_circle(cr, ix + iw * 0.72, fy + fh * 0.30, iw * 0.13, 0.98, 0.72, 0.26, 0.95);
            /* Layered mountains */
            g_ngfx.save(cr);
            ngfx_rounded_path(cr, ix, fy, iw, fh, sz * 0.11);
            g_ngfx.clip(cr);
            g_ngfx.set_source_rgba(cr, 0.18, 0.56, 0.38, 0.95);
            g_ngfx.move_to(cr, ix, fy + fh);
            g_ngfx.line_to(cr, ix + iw * 0.38, fy + fh * 0.40);
            g_ngfx.line_to(cr, ix + iw * 0.76, fy + fh);
            g_ngfx.close_path(cr);
            g_ngfx.fill(cr);

            g_ngfx.set_source_rgba(cr, 0.09, 0.36, 0.25, 0.98);
            g_ngfx.move_to(cr, ix + iw * 0.34, fy + fh);
            g_ngfx.line_to(cr, ix + iw * 0.70, fy + fh * 0.48);
            g_ngfx.line_to(cr, ix + iw, fy + fh * 0.80);
            g_ngfx.line_to(cr, ix + iw, fy + fh);
            g_ngfx.close_path(cr);
            g_ngfx.fill(cr);
            g_ngfx.restore(cr);
            break;
        }
        case APP_SETTINGS:
        default: {
            ngfx_fill_gradient(cr, x, y, sz, sz, r, 0.24, 0.30, 0.29, 1.0, 0.11, 0.14, 0.14, 1.0, 0);
            ngfx_stroke_rect(cr, x + 0.5, y + 0.5, sz - 1, sz - 1, r, 1.0, 1.0, 1.0, 1.0, 0.22);
            ngfx_top_specular(cr, x, y, sz, r, 0.32);
            double gr = ar * 1.4 > 1 ? 1 : ar * 1.4;
            double gg = ag * 1.35 > 1 ? 1 : ag * 1.35;
            double gb = ab * 1.35 > 1 ? 1 : ab * 1.35;
            /* 8-tooth precision gear */
            for (int t = 0; t < 8; ++t) {
                double ang = t * (3.14159265 / 4.0);
                g_ngfx.save(cr);
                g_ngfx.translate(cr, cx, cy);
                g_ngfx.rotate(cr, ang);
                ngfx_fill_rect(cr, -iw * 0.085, -iw * 0.44, iw * 0.17, iw * 0.88, sz * 0.03,
                               0.88, 0.93, 0.91, 0.94);
                g_ngfx.restore(cr);
            }
            ngfx_fill_circle(cr, cx, cy, iw * 0.31, 0.90, 0.95, 0.93, 1.0);
            ngfx_fill_circle(cr, cx, cy, iw * 0.18, 0.14, 0.18, 0.17, 1.0);
            ngfx_fill_circle(cr, cx, cy, iw * 0.11, gr, gg, gb, 0.96);
            break;
        }
    }
    g_ngfx.restore(cr);
}

static void render_window(NatureOSState *st, cairo_t *cr, OSWindow *w, int is_active) {
    if (w->minimized) return;

    clamp_window_to_screen(st, w);
    int wx = w->x, wy = w->y, ww = w->w, wh = w->h;

    /* Any text input rects from lower windows covered by this window are hidden */
    nature_occlude_text_rects(st, wx, wy, ww, wh);

    int dark = st->cfg.dark_mode;
    double ar, ag, ab;
    nature_get_accent(st->cfg.accent_idx, &ar, &ag, &ab);

    if (!w->maximized) {
        ngfx_shadow_rect(cr, wx, wy, ww, wh, 14, is_active ? 5.0 : 2.6, is_active ? 0.62 : 0.34);
    }

    double win_alpha = st->cfg.low_power_mode ? 1.0 : 0.975;
    if (dark) ngfx_fill_rect(cr, wx, wy, ww, wh, 14, 0.048, 0.072, 0.064, win_alpha);
    else ngfx_fill_rect(cr, wx, wy, ww, wh, 14, 0.968, 0.982, 0.975, win_alpha);

    int hh = 38;
    g_ngfx.save(cr);
    ngfx_rounded_path(cr, wx, wy, ww, wh, 14);
    g_ngfx.clip(cr);

    /* CSD integrated headerbar with refined gradient sheen */
    if (dark) {
        ngfx_fill_gradient(cr, wx, wy, ww, hh, 0,
                           is_active ? 0.092 : 0.064, is_active ? 0.130 : 0.092, is_active ? 0.116 : 0.082, 1.0,
                           is_active ? 0.068 : 0.056, is_active ? 0.098 : 0.082, is_active ? 0.088 : 0.072, 1.0, 0);
        g_ngfx.set_source_rgba(cr, 1, 1, 1, is_active ? 0.09 : 0.05);
    } else {
        ngfx_fill_gradient(cr, wx, wy, ww, hh, 0,
                           is_active ? 0.965 : 0.955, is_active ? 0.980 : 0.965, is_active ? 0.972 : 0.960, 1.0,
                           is_active ? 0.905 : 0.935, is_active ? 0.930 : 0.945, is_active ? 0.920 : 0.940, 1.0, 0);
        g_ngfx.set_source_rgba(cr, 0, 0, 0, 0.075);
    }
    g_ngfx.set_line_width(cr, 1.0);
    g_ngfx.move_to(cr, wx, wy + hh + 0.5);
    g_ngfx.line_to(cr, wx + ww, wy + hh + 0.5);
    g_ngfx.stroke(cr);

    /* Traffic Light Controls (crisp vector glyphs + hover halos + specular) */
    double dot_y = wy + 19;
    int hdr_hov = (st->hover_mx >= wx && st->hover_mx <= wx + 102 && st->hover_my >= wy && st->hover_my <= wy + hh);
    int hov_close = (st->hover_mx >= wx + 8 && st->hover_mx <= wx + 30 && st->hover_my >= wy + 5 && st->hover_my <= wy + 33);
    int hov_min = (st->hover_mx >= wx + 30 && st->hover_mx <= wx + 52 && st->hover_my >= wy + 5 && st->hover_my <= wy + 33);
    int hov_max = (st->hover_mx >= wx + 52 && st->hover_mx <= wx + 74 && st->hover_my >= wy + 5 && st->hover_my <= wy + 33);
    int hov_snap = (st->hover_mx >= wx + 74 && st->hover_mx <= wx + 96 && st->hover_my >= wy + 5 && st->hover_my <= wy + 33);

    if (hov_close) ngfx_fill_circle(cr, wx + 19, dot_y, 10.5, 1.0, 0.37, 0.34, 0.22);
    ngfx_fill_circle(cr, wx + 19, dot_y, 6.6, 0.98, 0.37, 0.34, is_active ? 1.0 : 0.60);
    ngfx_fill_circle(cr, wx + 18.0, dot_y - 2.0, 2.2, 1.0, 1.0, 1.0, is_active ? 0.22 : 0.10);
    if (is_active || hdr_hov) {
        g_ngfx.set_source_rgba(cr, 0.28, 0.04, 0.04, 0.88);
        g_ngfx.set_line_width(cr, 1.35);
        g_ngfx.move_to(cr, wx + 16.5, dot_y - 2.5); g_ngfx.line_to(cr, wx + 21.5, dot_y + 2.5);
        g_ngfx.move_to(cr, wx + 21.5, dot_y - 2.5); g_ngfx.line_to(cr, wx + 16.5, dot_y + 2.5);
        g_ngfx.stroke(cr);
    }
    nature_add_hit(st, wx + 8, wy + 5, 22, 28, HIT_WIN_CLOSE, w->id, 0);

    if (hov_min) ngfx_fill_circle(cr, wx + 41, dot_y, 10.5, 1.0, 0.74, 0.18, 0.22);
    ngfx_fill_circle(cr, wx + 41, dot_y, 6.6, 0.98, 0.74, 0.18, is_active ? 1.0 : 0.60);
    ngfx_fill_circle(cr, wx + 40.0, dot_y - 2.0, 2.2, 1.0, 1.0, 1.0, is_active ? 0.22 : 0.10);
    if (is_active || hdr_hov) {
        g_ngfx.set_source_rgba(cr, 0.30, 0.18, 0.02, 0.88);
        g_ngfx.set_line_width(cr, 1.35);
        g_ngfx.move_to(cr, wx + 38.0, dot_y); g_ngfx.line_to(cr, wx + 44.0, dot_y);
        g_ngfx.stroke(cr);
    }
    nature_add_hit(st, wx + 30, wy + 5, 22, 28, HIT_WIN_MIN, w->id, 0);

    if (hov_max) ngfx_fill_circle(cr, wx + 63, dot_y, 10.5, 0.15, 0.79, 0.25, 0.22);
    ngfx_fill_circle(cr, wx + 63, dot_y, 6.6, 0.18, 0.79, 0.32, is_active ? 1.0 : 0.60);
    ngfx_fill_circle(cr, wx + 62.0, dot_y - 2.0, 2.2, 1.0, 1.0, 1.0, is_active ? 0.22 : 0.10);
    if (is_active || hdr_hov) {
        g_ngfx.set_source_rgba(cr, 0.03, 0.24, 0.06, 0.88);
        g_ngfx.set_line_width(cr, 1.35);
        g_ngfx.move_to(cr, wx + 60.0, dot_y); g_ngfx.line_to(cr, wx + 66.0, dot_y);
        g_ngfx.move_to(cr, wx + 63.0, dot_y - 3.0); g_ngfx.line_to(cr, wx + 63.0, dot_y + 3.0);
        g_ngfx.stroke(cr);
    }
    nature_add_hit(st, wx + 52, wy + 5, 22, 28, HIT_WIN_MAX, w->id, 0);

    if (hov_snap) ngfx_fill_circle(cr, wx + 85, dot_y, 10.5, 0.16, 0.76, 0.64, 0.22);
    ngfx_fill_circle(cr, wx + 85, dot_y, 6.6, 0.16, 0.76, 0.64, is_active ? 1.0 : 0.60);
    ngfx_fill_circle(cr, wx + 84.0, dot_y - 2.0, 2.2, 1.0, 1.0, 1.0, is_active ? 0.22 : 0.10);
    if (is_active || hdr_hov) {
        g_ngfx.set_source_rgba(cr, 0.02, 0.22, 0.18, 0.88);
        g_ngfx.set_line_width(cr, 1.2);
        g_ngfx.rectangle(cr, wx + 81.8, dot_y - 2.6, 6.4, 5.2);
        g_ngfx.move_to(cr, wx + 85.0, dot_y - 2.6); g_ngfx.line_to(cr, wx + 85.0, dot_y + 2.6);
        g_ngfx.stroke(cr);
    }
    nature_add_hit(st, wx + 74, wy + 5, 22, 28, HIT_WIN_SNAP, w->id, 0);

    draw_app_icon(cr, w->app, wx + 108, wy + 9, 20, ar, ag, ab);

    double tr = dark ? 0.94 : 0.09, tg = dark ? 0.97 : 0.13, tb = dark ? 0.95 : 0.11;
    ngfx_text_clipped(cr, "Plus Jakarta Sans", 1, 12.5, wx + 135, wy + 23.5, ww - 160,
                      tr, tg, tb, is_active ? 1.0 : 0.62, w->title);

    nature_add_hit(st, wx + 98, wy, ww - 104, hh, HIT_WIN_TITLE, w->id, 0);

    int cx = wx, cy = wy + hh, cw = ww, ch = wh - hh;
    g_ngfx.save(cr);
    g_ngfx.rectangle(cr, cx, cy, cw, ch);
    g_ngfx.clip(cr);

    switch (w->app) {
        case APP_FILES:    nature_files_render(st, cr, w, cx, cy, cw, ch); break;
        case APP_TERMINAL: nature_terminal_render(st, cr, w, cx, cy, cw, ch); break;
        case APP_MONITOR:  nature_monitor_render(st, cr, w, cx, cy, cw, ch); break;
        case APP_SETTINGS: nature_settings_render(st, cr, w, cx, cy, cw, ch); break;
        case APP_STORE:    nature_store_render(st, cr, w, cx, cy, cw, ch); break;
        case APP_MEDIA:    nature_media_render(st, cr, w, cx, cy, cw, ch); break;
        case APP_BROWSER:  nature_browser_render(st, cr, w, cx, cy, cw, ch); break;
        default: break;
    }

    g_ngfx.restore(cr);

    if (!w->maximized) {
        int res_hov = (st->hover_mx >= wx + ww - 24 && st->hover_mx <= wx + ww &&
                       st->hover_my >= wy + wh - 24 && st->hover_my <= wy + wh);
        g_ngfx.set_source_rgba(cr, res_hov ? ar : (dark ? 1 : 0), res_hov ? ag : (dark ? 1 : 0),
                               res_hov ? ab : (dark ? 1 : 0), res_hov ? 0.85 : 0.28);
        g_ngfx.set_line_width(cr, 1.4);
        g_ngfx.move_to(cr, wx + ww - 14, wy + wh - 5);
        g_ngfx.line_to(cr, wx + ww - 5, wy + wh - 14);
        g_ngfx.move_to(cr, wx + ww - 9, wy + wh - 5);
        g_ngfx.line_to(cr, wx + ww - 5, wy + wh - 9);
        g_ngfx.stroke(cr);
        nature_add_hit(st, wx + ww - 24, wy + wh - 24, 26, 26, HIT_WIN_RESIZE, w->id, 0);
    }

    g_ngfx.restore(cr);

    ngfx_top_specular(cr, wx, wy, ww, 14, dark ? (is_active ? 0.24 : 0.10) : 0.72);
    if (is_active) {
        ngfx_stroke_rect(cr, wx + 0.5, wy + 0.5, ww - 1, wh - 1, 14, 1.3,
                         ar * 1.25 > 1 ? 1 : ar * 1.25, ag * 1.25 > 1 ? 1 : ag * 1.25, ab * 1.25 > 1 ? 1 : ab * 1.25,
                         dark ? 0.72 : 0.65);
    } else {
        ngfx_stroke_rect(cr, wx + 0.5, wy + 0.5, ww - 1, wh - 1, 14, 1.0,
                         dark ? 1.0 : 0.0, dark ? 1.0 : 0.0, dark ? 1.0 : 0.0, dark ? 0.12 : 0.14);
    }
}

static void draw_wifi_glyph(cairo_t *cr, double cx, double cy, double r, double g, double b, double a) {
    g_ngfx.set_source_rgba(cr, r, g, b, a);
    g_ngfx.set_line_width(cr, 1.4);
    g_ngfx.new_sub_path(cr);
    g_ngfx.arc(cr, cx, cy + 3.5, 8.0, -2.35, -0.79);
    g_ngfx.stroke(cr);
    g_ngfx.new_sub_path(cr);
    g_ngfx.arc(cr, cx, cy + 3.5, 4.8, -2.35, -0.79);
    g_ngfx.stroke(cr);
    ngfx_fill_circle(cr, cx, cy + 3.5, 1.5, r, g, b, a);
}

static void draw_speaker_glyph(cairo_t *cr, double x, double y, int muted, double r, double g, double b, double a) {
    g_ngfx.set_source_rgba(cr, r, g, b, a);
    g_ngfx.move_to(cr, x, y - 2.5);
    g_ngfx.line_to(cr, x + 3.2, y - 2.5);
    g_ngfx.line_to(cr, x + 6.8, y - 5.5);
    g_ngfx.line_to(cr, x + 6.8, y + 5.5);
    g_ngfx.line_to(cr, x + 3.2, y + 2.5);
    g_ngfx.line_to(cr, x, y + 2.5);
    g_ngfx.close_path(cr);
    g_ngfx.fill(cr);
    g_ngfx.set_line_width(cr, 1.3);
    if (muted) {
        g_ngfx.set_source_rgba(cr, 0.96, 0.42, 0.42, a);
        g_ngfx.move_to(cr, x + 9.0, y - 3.0); g_ngfx.line_to(cr, x + 13.5, y + 1.5);
        g_ngfx.move_to(cr, x + 13.5, y - 3.0); g_ngfx.line_to(cr, x + 9.0, y + 1.5);
        g_ngfx.stroke(cr);
    } else {
        g_ngfx.new_sub_path(cr);
        g_ngfx.arc(cr, x + 7.5, y, 4.2, -0.75, 0.75);
        g_ngfx.stroke(cr);
    }
}

static void render_quick_settings_popover(NatureOSState *st, cairo_t *cr) {
    int sw = st->screen_w;
    int pw = sw < 340 ? sw - 20 : 316;
    int ph = 338;
    int px = sw - pw - 10;
    int py = 48;
    if (px < 10) px = 10;

    nature_occlude_text_rects(st, px, py, pw, ph);

    int dark = st->cfg.dark_mode;
    double ar, ag, ab;
    nature_get_accent(st->cfg.accent_idx, &ar, &ag, &ab);

    ngfx_shadow_rect(cr, px, py, pw, ph, 16, 4.5, 0.58);
    ngfx_fill_gradient(cr, px, py, pw, ph, 16,
                       dark ? 0.080 : 0.975, dark ? 0.115 : 0.988, dark ? 0.104 : 0.982, 0.98,
                       dark ? 0.052 : 0.945, dark ? 0.078 : 0.965, dark ? 0.070 : 0.958, 0.98, 0);
    ngfx_stroke_rect(cr, px + 0.5, py + 0.5, pw - 1, ph - 1, 16, 1.0,
                     dark ? 1.0 : 0.0, dark ? 1.0 : 0.0, dark ? 1.0 : 0.0, 0.16);
    ngfx_top_specular(cr, px, py, pw, 16, dark ? 0.25 : 0.75);

    double fg_r = dark ? 0.94 : 0.10, fg_g = dark ? 0.97 : 0.13, fg_b = dark ? 0.95 : 0.11;
    double mut_r = dark ? 0.62 : 0.45, mut_g = dark ? 0.70 : 0.50, mut_b = dark ? 0.66 : 0.48;

    int t_w = (pw - 36) / 2;
    int bt_x = px + 22 + t_w;

    /* Row 1: Wi-Fi + Bluetooth */
    int h_wifi = (st->hover_mx >= px + 14 && st->hover_mx <= px + 14 + t_w && st->hover_my >= py + 14 && st->hover_my <= py + 60);
    ngfx_fill_rect(cr, px + 14, py + 14, t_w, 46, 12,
                   st->cfg.wifi_enabled ? ar : (dark ? 1.0 : 0.0),
                   st->cfg.wifi_enabled ? ag : (dark ? 1.0 : 0.0),
                   st->cfg.wifi_enabled ? ab : (dark ? 1.0 : 0.0),
                   st->cfg.wifi_enabled ? (h_wifi ? 1.0 : 0.90) : (h_wifi ? 0.14 : 0.08));
    draw_wifi_glyph(cr, px + 28, py + 36,
                    st->cfg.wifi_enabled ? 1.0 : fg_r, st->cfg.wifi_enabled ? 1.0 : fg_g, st->cfg.wifi_enabled ? 1.0 : fg_b, 0.95);
    ngfx_text(cr, "Plus Jakarta Sans", 1, 11.8, px + 44, py + 32,
              st->cfg.wifi_enabled ? 1.0 : fg_r, st->cfg.wifi_enabled ? 1.0 : fg_g, st->cfg.wifi_enabled ? 1.0 : fg_b, 1.0, "Wi-Fi");
    ngfx_text_clipped(cr, "Noto Sans", 0, 10.0, px + 44, py + 48, t_w - 36,
                      st->cfg.wifi_enabled ? 0.92 : mut_r, st->cfg.wifi_enabled ? 0.97 : mut_g, st->cfg.wifi_enabled ? 0.94 : mut_b, 0.92,
                      st->cfg.wifi_enabled ? "Canopy-5GHz" : "Off");
    nature_add_hit(st, px + 14, py + 14, t_w, 46, HIT_QS_WIFI, 0, 0);

    int h_bt = (st->hover_mx >= bt_x && st->hover_mx <= bt_x + t_w && st->hover_my >= py + 14 && st->hover_my <= py + 60);
    ngfx_fill_rect(cr, bt_x, py + 14, t_w, 46, 12,
                   st->cfg.bluetooth_enabled ? ar : (dark ? 1.0 : 0.0),
                   st->cfg.bluetooth_enabled ? ag : (dark ? 1.0 : 0.0),
                   st->cfg.bluetooth_enabled ? ab : (dark ? 1.0 : 0.0),
                   st->cfg.bluetooth_enabled ? (h_bt ? 1.0 : 0.90) : (h_bt ? 0.14 : 0.08));
    /* Bluetooth B rune */
    double bcol_r = st->cfg.bluetooth_enabled ? 1.0 : fg_r;
    double bcol_g = st->cfg.bluetooth_enabled ? 1.0 : fg_g;
    double bcol_b = st->cfg.bluetooth_enabled ? 1.0 : fg_b;
    g_ngfx.set_source_rgba(cr, bcol_r, bcol_g, bcol_b, 0.95);
    g_ngfx.set_line_width(cr, 1.3);
    g_ngfx.move_to(cr, bt_x + 17, py + 29); g_ngfx.line_to(cr, bt_x + 17, py + 45);
    g_ngfx.line_to(cr, bt_x + 22, py + 41); g_ngfx.line_to(cr, bt_x + 13, py + 34);
    g_ngfx.move_to(cr, bt_x + 13, py + 40); g_ngfx.line_to(cr, bt_x + 22, py + 33); g_ngfx.line_to(cr, bt_x + 17, py + 29);
    g_ngfx.stroke(cr);
    ngfx_text(cr, "Plus Jakarta Sans", 1, 11.8, bt_x + 32, py + 32, bcol_r, bcol_g, bcol_b, 1.0, "Bluetooth");
    ngfx_text(cr, "Noto Sans", 0, 10.0, bt_x + 32, py + 48,
              st->cfg.bluetooth_enabled ? 0.92 : mut_r, st->cfg.bluetooth_enabled ? 0.97 : mut_g, st->cfg.bluetooth_enabled ? 0.94 : mut_b, 0.92,
              st->cfg.bluetooth_enabled ? "Connected" : "Off");
    nature_add_hit(st, bt_x, py + 14, t_w, 46, HIT_QS_BLUETOOTH, 0, 0);

    /* Row 2: Dark Style + Night Light */
    int h_dk = (st->hover_mx >= px + 14 && st->hover_mx <= px + 14 + t_w && st->hover_my >= py + 68 && st->hover_my <= py + 114);
    ngfx_fill_rect(cr, px + 14, py + 68, t_w, 46, 12,
                   st->cfg.dark_mode ? ar : (dark ? 1.0 : 0.0),
                   st->cfg.dark_mode ? ag : (dark ? 1.0 : 0.0),
                   st->cfg.dark_mode ? ab : (dark ? 1.0 : 0.0),
                   st->cfg.dark_mode ? (h_dk ? 1.0 : 0.90) : (h_dk ? 0.14 : 0.08));
    ngfx_fill_circle(cr, px + 28, py + 91, 6.0, st->cfg.dark_mode ? 1.0 : fg_r, st->cfg.dark_mode ? 1.0 : fg_g, st->cfg.dark_mode ? 1.0 : fg_b, 0.92);
    ngfx_fill_circle(cr, px + 30.5, py + 89.5, 4.5, st->cfg.dark_mode ? ar : 0.96, st->cfg.dark_mode ? ag : 0.97, st->cfg.dark_mode ? ab : 0.97, 0.98);
    ngfx_text(cr, "Plus Jakarta Sans", 1, 11.8, px + 44, py + 86,
              st->cfg.dark_mode ? 1.0 : fg_r, st->cfg.dark_mode ? 1.0 : fg_g, st->cfg.dark_mode ? 1.0 : fg_b, 1.0, "Theme");
    ngfx_text(cr, "Noto Sans", 0, 10.0, px + 44, py + 102,
              st->cfg.dark_mode ? 0.92 : mut_r, st->cfg.dark_mode ? 0.97 : mut_g, st->cfg.dark_mode ? 0.94 : mut_b, 0.92,
              st->cfg.dark_mode ? "Dark Glass" : "Light Glass");
    nature_add_hit(st, px + 14, py + 68, t_w, 46, HIT_QS_DARK, 0, 0);

    int h_nl = (st->hover_mx >= bt_x && st->hover_mx <= bt_x + t_w && st->hover_my >= py + 68 && st->hover_my <= py + 114);
    ngfx_fill_rect(cr, bt_x, py + 68, t_w, 46, 12,
                   st->cfg.night_light ? 0.90 : (dark ? 1.0 : 0.0),
                   st->cfg.night_light ? 0.54 : (dark ? 1.0 : 0.0),
                   st->cfg.night_light ? 0.20 : (dark ? 1.0 : 0.0),
                   st->cfg.night_light ? (h_nl ? 1.0 : 0.92) : (h_nl ? 0.14 : 0.08));
    ngfx_fill_circle(cr, bt_x + 17, py + 91, 5.2, st->cfg.night_light ? 1.0 : fg_r, st->cfg.night_light ? 0.96 : fg_g, st->cfg.night_light ? 0.85 : fg_b, 0.95);
    ngfx_text(cr, "Plus Jakarta Sans", 1, 11.8, bt_x + 32, py + 86,
              st->cfg.night_light ? 1.0 : fg_r, st->cfg.night_light ? 1.0 : fg_g, st->cfg.night_light ? 1.0 : fg_b, 1.0, "Night Light");
    ngfx_text(cr, "Noto Sans", 0, 10.0, bt_x + 32, py + 102,
              st->cfg.night_light ? 0.94 : mut_r, st->cfg.night_light ? 0.96 : mut_g, st->cfg.night_light ? 0.92 : mut_b, 0.92,
              st->cfg.night_light ? "Warm 3800K" : "Off");
    nature_add_hit(st, bt_x, py + 68, t_w, 46, HIT_QS_NIGHT_LIGHT, 0, 0);

    /* Row 3: Power Mode Segmented Strip */
    const char *pmodes[3] = {"Saver", "Balanced", "Perf"};
    ngfx_fill_rect(cr, px + 14, py + 122, pw - 28, 34, 10,
                   dark ? 1.0 : 0.0, dark ? 1.0 : 0.0, dark ? 1.0 : 0.0, 0.06);
    ngfx_text(cr, "Plus Jakarta Sans", 1, 11.0, px + 24, py + 143, fg_r, fg_g, fg_b, 0.92, "Power");
    int seg_x = px + 76, seg_w = (pw - 96) / 3;
    for (int m = 0; m < 3; ++m) {
        int act = (st->cfg.power_profile % 3) == m;
        int sx = seg_x + m * seg_w;
        if (act) ngfx_fill_rect(cr, sx, py + 126, seg_w - 2, 26, 7, ar, ag, ab, 0.90);
        double mw = ngfx_text_width(cr, "Noto Sans", act, 10.0, pmodes[m]);
        ngfx_text(cr, "Noto Sans", act, 10.0, sx + ((seg_w - 2) - mw) * 0.5, py + 143,
                  act ? 1.0 : mut_r, act ? 1.0 : mut_g, act ? 1.0 : mut_b, 1.0, pmodes[m]);
    }
    nature_add_hit(st, px + 14, py + 122, pw - 28, 34, HIT_QS_PERF, 0, 0);

    /* Continuous Interactive Volume Slider bar */
    int sl_x = px + 14, sl_w = pw - 28;
    ngfx_text(cr, "Plus Jakarta Sans", 1, 10.8, sl_x, py + 178, mut_r, mut_g, mut_b, 1.0, "Audio Volume");
    ngfx_fill_rect(cr, sl_x, py + 186, sl_w, 12, 6, dark ? 1 : 0, dark ? 1 : 0, dark ? 1 : 0, 0.14);
    double vol_w = (sl_w * st->cfg.volume) / 100.0;
    if (vol_w > 0) {
        ngfx_fill_gradient(cr, sl_x, py + 186, vol_w, 12, 6,
                           ar, ag, ab, 0.95,
                           ar * 1.25 > 1 ? 1 : ar * 1.25, ag * 1.25 > 1 ? 1 : ag * 1.25, ab * 1.25 > 1 ? 1 : ab * 1.25, 0.98, 1);
    }
    ngfx_fill_circle(cr, sl_x + vol_w, py + 193, 9.0, 0.0, 0.0, 0.0, 0.25);
    ngfx_fill_circle(cr, sl_x + vol_w, py + 192, 8.2, 1, 1, 1, 0.98);
    ngfx_fill_circle(cr, sl_x + vol_w, py + 192, 3.0, ar, ag, ab, 0.95);
    char vb[16];
    snprintf(vb, sizeof(vb), "%d%%", st->cfg.volume);
    ngfx_text(cr, "JetBrains Mono", 0, 10.5, px + pw - 44, py + 178, fg_r, fg_g, fg_b, 0.95, vb);
    nature_add_hit(st, sl_x, py + 178, sl_w, 28, HIT_SLIDER_VOL, sl_x, sl_w);

    /* Continuous Interactive Brightness Slider bar */
    ngfx_text(cr, "Plus Jakarta Sans", 1, 10.8, sl_x, py + 222, mut_r, mut_g, mut_b, 1.0, "Display Brightness");
    ngfx_fill_rect(cr, sl_x, py + 230, sl_w, 12, 6, dark ? 1 : 0, dark ? 1 : 0, dark ? 1 : 0, 0.14);
    double br_w = (sl_w * st->cfg.brightness) / 100.0;
    if (br_w > 0) ngfx_fill_gradient(cr, sl_x, py + 230, br_w, 12, 6, 0.88, 0.65, 0.18, 0.95, 0.98, 0.84, 0.34, 0.98, 1);
    ngfx_fill_circle(cr, sl_x + br_w, py + 237, 9.0, 0.0, 0.0, 0.0, 0.25);
    ngfx_fill_circle(cr, sl_x + br_w, py + 236, 8.2, 1, 1, 1, 0.98);
    ngfx_fill_circle(cr, sl_x + br_w, py + 236, 3.0, 0.94, 0.72, 0.22, 0.95);
    char bb[16];
    snprintf(bb, sizeof(bb), "%d%%", st->cfg.brightness);
    ngfx_text(cr, "JetBrains Mono", 0, 10.5, px + pw - 44, py + 222, fg_r, fg_g, fg_b, 0.95, bb);
    nature_add_hit(st, sl_x, py + 222, sl_w, 28, HIT_SLIDER_BRIGHT, sl_x, sl_w);

    char qs_stat[64];
    long up_m = st->uptime_sec / 60;
    snprintf(qs_stat, sizeof(qs_stat), "CPU %.1f%% · RAM %d/%dM · Up %ldh %ldm",
             st->total_cpu_pct, st->used_ram_mb, st->total_ram_mb, up_m / 60, up_m % 60);
    ngfx_text_clipped(cr, "JetBrains Mono", 0, 9.6, px + 14, py + 270, pw - 28, mut_r, mut_g, mut_b, 0.95, qs_stat);

    int f_y = py + ph - 46;
    int h_set = (st->hover_mx >= px + 14 && st->hover_mx <= px + 146 && st->hover_my >= f_y && st->hover_my <= f_y + 30);
    ngfx_fill_rect(cr, px + 14, f_y, 132, 30, 8, dark ? 1 : 0, dark ? 1 : 0, dark ? 1 : 0, h_set ? 0.16 : 0.10);
    ngfx_stroke_rect(cr, px + 14.5, f_y + 0.5, 131, 29, 8, 1.0, dark ? 1 : 0, dark ? 1 : 0, dark ? 1 : 0, 0.08);
    ngfx_text(cr, "Plus Jakarta Sans", 1, 11.2, px + 30, f_y + 19, fg_r, fg_g, fg_b, 1.0, "System Settings");
    nature_add_hit(st, px + 14, f_y, 132, 30, HIT_QS_SETTINGS, 0, 0);

    int h_reb = (st->hover_mx >= px + pw - 134 && st->hover_mx <= px + pw - 14 && st->hover_my >= f_y && st->hover_my <= f_y + 30);
    ngfx_fill_rect(cr, px + pw - 134, f_y, 120, 30, 8, 0.85, 0.25, 0.25, h_reb ? 0.30 : 0.18);
    ngfx_stroke_rect(cr, px + pw - 133.5, f_y + 0.5, 119, 29, 8, 1.0, 0.92, 0.38, 0.38, 0.28);
    ngfx_text(cr, "Plus Jakarta Sans", 1, 11.2, px + pw - 117, f_y + 19, 0.96, 0.48, 0.48, 1.0, "Restart Session");
    nature_add_hit(st, px + pw - 134, f_y, 120, 30, HIT_QS_REBOOT, 0, 0);
}

static void render_calendar_popover(NatureOSState *st, cairo_t *cr, const struct tm *tm_info) {
    int sw = st->screen_w;
    int pw = sw < 380 ? sw - 20 : 346;
    int ph = 342;
    int px = (sw - pw) / 2;
    int py = 48;

    nature_occlude_text_rects(st, px, py, pw, ph);

    int dark = st->cfg.dark_mode;
    double ar, ag, ab;
    nature_get_accent(st->cfg.accent_idx, &ar, &ag, &ab);
    double fg_r = dark ? 0.94 : 0.10, fg_g = dark ? 0.97 : 0.13, fg_b = dark ? 0.95 : 0.11;
    double mut_r = dark ? 0.62 : 0.44, mut_g = dark ? 0.70 : 0.50, mut_b = dark ? 0.66 : 0.47;

    ngfx_shadow_rect(cr, px, py, pw, ph, 16, 4.5, 0.58);
    ngfx_fill_gradient(cr, px, py, pw, ph, 16,
                       dark ? 0.080 : 0.975, dark ? 0.115 : 0.988, dark ? 0.104 : 0.982, 0.98,
                       dark ? 0.052 : 0.945, dark ? 0.078 : 0.965, dark ? 0.070 : 0.958, 0.98, 0);
    ngfx_stroke_rect(cr, px + 0.5, py + 0.5, pw - 1, ph - 1, 16, 1.0,
                     dark ? 1.0 : 0.0, dark ? 1.0 : 0.0, dark ? 1.0 : 0.0, 0.16);
    ngfx_top_specular(cr, px, py, pw, 16, dark ? 0.25 : 0.75);

    char hdr_date[64];
    strftime(hdr_date, sizeof(hdr_date), "%A, %B %d", tm_info);
    ngfx_text(cr, "Plus Jakarta Sans", 1, 14.5, px + 18, py + 28, fg_r, fg_g, fg_b, 1.0, hdr_date);

    /* Compact Calendar Month Grid */
    const char *wdays[7] = {"Su", "Mo", "Tu", "We", "Th", "Fr", "Sa"};
    int cw = (pw - 36) / 7;
    for (int d = 0; d < 7; ++d) {
        ngfx_text(cr, "JetBrains Mono", 1, 9.4, px + 18 + d * cw + 7, py + 51, mut_r, mut_g, mut_b, 0.80, wdays[d]);
    }
    int today = tm_info->tm_mday;
    int first_wday = ((tm_info->tm_wday - (today - 1)) % 7 + 7) % 7;
    for (int d = 1; d <= 30; ++d) {
        int slot = first_wday + (d - 1);
        int col = slot % 7;
        int row = slot / 7;
        int dx = px + 18 + col * cw;
        int dy = py + 58 + row * 22;
        if (d == today) {
            ngfx_radial_glow(cr, dx + cw * 0.45, dy + 10, 2.0, 16.0, ar, ag, ab, 0.35, 0.0);
            ngfx_fill_rect(cr, dx + 2, dy, cw - 6, 19, 6, ar, ag, ab, 0.96);
        }
        char db[8];
        snprintf(db, sizeof(db), "%2d", d);
        ngfx_text(cr, "JetBrains Mono", d == today, 10.0, dx + 7, dy + 13,
                  d == today ? 1.0 : fg_r, d == today ? 1.0 : fg_g, d == today ? 1.0 : fg_b,
                  d == today ? 1.0 : 0.82, db);
    }

    /* Notification Feed */
    int ny = py + 178;
    g_ngfx.set_source_rgba(cr, dark ? 1 : 0, dark ? 1 : 0, dark ? 1 : 0, 0.08);
    g_ngfx.move_to(cr, px + 16, ny);
    g_ngfx.line_to(cr, px + pw - 16, ny);
    g_ngfx.stroke(cr);

    ngfx_text(cr, "Plus Jakarta Sans", 1, 11.5, px + 18, ny + 21, fg_r, fg_g, fg_b, 1.0, "Notification Center");
    if (st->notif_count > 0) {
        ngfx_fill_rect(cr, px + pw - 70, ny + 6, 52, 21, 6, dark ? 1 : 0, dark ? 1 : 0, dark ? 1 : 0, 0.12);
        ngfx_text(cr, "Noto Sans", 1, 9.8, px + pw - 57, ny + 20, fg_r, fg_g, fg_b, 0.9, "Clear");
        nature_add_hit(st, px + pw - 70, ny + 6, 52, 21, HIT_CAL_CLEAR_NOTIFS, 0, 0);
    }

    int ly = ny + 32;
    if (st->notif_count == 0) {
        ngfx_fill_rect(cr, px + 16, ly + 4, pw - 32, 60, 10, dark ? 1 : 0, dark ? 1 : 0, dark ? 1 : 0, 0.03);
        ngfx_text(cr, "Noto Sans", 0, 11.0, px + 76, ly + 38, mut_r, mut_g, mut_b, 0.78, "All caught up — no unread alerts");
    } else {
        int show = st->notif_count < 3 ? st->notif_count : 3;
        for (int i = 0; i < show; ++i) {
            ngfx_fill_rect(cr, px + 16, ly, pw - 32, 38, 9, dark ? 1 : 0, dark ? 1 : 0, dark ? 1 : 0, 0.05);
            ngfx_fill_rect(cr, px + 21, ly + 7, 3, 24, 1.5, ar * 1.25 > 1 ? 1 : ar * 1.25,
                           ag * 1.25 > 1 ? 1 : ag * 1.25, ab * 1.25 > 1 ? 1 : ab * 1.25, 0.95);
            ngfx_text_clipped(cr, "Plus Jakarta Sans", 1, 10.6, px + 31, ly + 15, pw - 56,
                              ar * 1.3 > 1 ? 1 : ar * 1.3, ag * 1.3 > 1 ? 1 : ag * 1.3, ab * 1.3 > 1 ? 1 : ab * 1.3, 1.0,
                              st->notif_titles[i]);
            ngfx_text_clipped(cr, "Noto Sans", 0, 10.2, px + 31, ly + 31, pw - 56, mut_r, mut_g, mut_b, 0.95, st->notif_msgs[i]);
            ly += 42;
        }
    }
}

static void ensure_cached_wallpaper(NatureOSState *st, int SW, int SH) {
    int wp_idx = st->cfg.wallpaper_idx % 3;
    int dark = st->cfg.dark_mode;
    if (st->cached_wp && st->cached_wp_w == SW && st->cached_wp_h == SH &&
        st->cached_wp_idx == wp_idx && st->cached_wp_dark == dark) {
        return;
    }
    if (st->cached_wp) {
        g_ngfx.surface_destroy(st->cached_wp);
        st->cached_wp = NULL;
    }
    st->cached_wp = g_ngfx.image_surface_create(CAIRO_FORMAT_ARGB32, SW, SH);
    cairo_t *wcr = g_ngfx.create(st->cached_wp);
    cairo_surface_t *wp = st->wallpapers[wp_idx];
    if (wp) {
        double wp_w = g_ngfx.image_surface_get_width(wp);
        double wp_h = g_ngfx.image_surface_get_height(wp);
        if (wp_w > 0 && wp_h > 0) {
            double sc_x = (double)SW / wp_w;
            double sc_y = (double)SH / wp_h;
            double scale = sc_x > sc_y ? sc_x : sc_y;
            double off_x = ((double)SW - wp_w * scale) * 0.5;
            double off_y = ((double)SH - wp_h * scale) * 0.5;
            g_ngfx.save(wcr);
            g_ngfx.translate(wcr, off_x, off_y);
            g_ngfx.scale(wcr, scale, scale);
            g_ngfx.set_source_surface(wcr, wp, 0, 0);
            g_ngfx.paint(wcr);
            g_ngfx.restore(wcr);
        }
    } else {
        ngfx_fill_rect(wcr, 0, 0, SW, SH, 0, 0.05, 0.09, 0.08, 1.0);
    }
    ngfx_fill_rect(wcr, 0, 0, SW, SH, 0, 0.02, 0.04, 0.03, dark ? 0.28 : 0.08);
    /* Atmospheric soft top & bottom scrim vignettes + radial center aura */
    ngfx_radial_glow(wcr, SW * 0.5, SH * 0.36, 20.0, (SW > SH ? SW : SH) * 0.45,
                     0.06, 0.14, 0.11, dark ? 0.22 : 0.08, 0.0);
    ngfx_fill_gradient(wcr, 0, 0, SW, 110, 0, 0.0, 0.02, 0.015, 0.44, 0.0, 0.0, 0.0, 0.0, 0);
    ngfx_fill_gradient(wcr, 0, SH - 120, SW, 120, 0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.02, 0.015, 0.50, 0);
    g_ngfx.destroy(wcr);
    st->cached_wp_w = SW;
    st->cached_wp_h = SH;
    st->cached_wp_idx = wp_idx;
    st->cached_wp_dark = dark;
}

void nature_os_render(NatureOSState *st, cairo_t *cr) {
    st->hit_count = 0;
    st->text_rect_count = 0;

    int SW = st->screen_w > 0 ? st->screen_w : OS_WIDTH;
    int SH = st->screen_h > 0 ? st->screen_h : OS_HEIGHT;

    double ar, ag, ab;
    nature_get_accent(st->cfg.accent_idx, &ar, &ag, &ab);

    /* 1. Fast Cached Aspect-Cover Wallpaper Blit (<0.3ms per frame!) */
    ensure_cached_wallpaper(st, SW, SH);
    if (st->cached_wp) {
        g_ngfx.set_source_surface(cr, st->cached_wp, 0, 0);
        g_ngfx.paint(cr);
    }

    /* 1b. Ambient Desktop Clock & Clean Unboxed Telemetry Lockup */
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char time_big[16];
    char date_big[64];
    strftime(time_big, sizeof(time_big), "%H:%M", tm_info);
    strftime(date_big, sizeof(date_big), "%A, %B %d", tm_info);

    double clk_sz = SW < 560 ? 48.0 : 62.0;
    int clk_y = SW < 580 ? 114 : (SH < 540 ? 120 : 164);
    ngfx_radial_glow(cr, SW * 0.5, clk_y - 8, 12.0, 170.0, 0.02, 0.04, 0.03, 0.34, 0.0);
    double tb_w = ngfx_text_width(cr, "Plus Jakarta Sans", 1, clk_sz, time_big);
    ngfx_text(cr, "Plus Jakarta Sans", 1, clk_sz, (SW - tb_w) * 0.5, clk_y + 2.0, 0.0, 0.0, 0.0, 0.38, time_big);
    ngfx_text(cr, "Plus Jakarta Sans", 1, clk_sz, (SW - tb_w) * 0.5, clk_y, 1.0, 1.0, 1.0, 0.96, time_big);

    double db_w = ngfx_text_width(cr, "Plus Jakarta Sans", 0, 14.5, date_big);
    ngfx_text(cr, "Plus Jakarta Sans", 0, 14.5, (SW - db_w) * 0.5, clk_y + 27, 0.0, 0.0, 0.0, 0.36, date_big);
    ngfx_text(cr, "Plus Jakarta Sans", 0, 14.5, (SW - db_w) * 0.5, clk_y + 26, 0.92, 0.97, 0.94, 0.88, date_big);

    char sys_stat[96];
    snprintf(sys_stat, sizeof(sys_stat), "Nature OS Noble  ·  %dx%d  ·  %d/%d MiB RAM  ·  %.1f ms",
             SW, SH, st->used_ram_mb, st->total_ram_mb, st->frame_ms);
    double sb_w = ngfx_text_width(cr, "JetBrains Mono", 0, 10.6, sys_stat);
    ngfx_fill_circle(cr, (SW - sb_w) * 0.5 - 10, clk_y + 47, 3.0,
                     ar * 1.35 > 1 ? 1 : ar * 1.35, ag * 1.35 > 1 ? 1 : ag * 1.35, ab * 1.35 > 1 ? 1 : ab * 1.35, 0.92);
    ngfx_text(cr, "JetBrains Mono", 0, 10.6, (SW - sb_w) * 0.5 + 1, clk_y + 52, 0.0, 0.0, 0.0, 0.42, sys_stat);
    ngfx_text(cr, "JetBrains Mono", 0, 10.6, (SW - sb_w) * 0.5, clk_y + 51, 0.86, 0.94, 0.90, 0.84, sys_stat);

    /* 2. Desktop Shortcut Cards */
    const AppType desk_apps[3] = {APP_FILES, APP_STORE, APP_BROWSER};
    const char *desk_labels[3] = {"Files", "Software", "Browser"};
    if (SW < 580) {
        int row_w = 3 * 78 + 2 * 12;
        int start_x = (SW - row_w) / 2;
        int dy = clk_y + 82;
        for (int i = 0; i < 3; ++i) {
            if (dy + 82 > SH - 84) break;
            int dx = start_x + i * 90;
            int hov = (st->hover_mx >= dx && st->hover_mx <= dx + 78 && st->hover_my >= dy && st->hover_my <= dy + 82);
            if (hov) ngfx_radial_glow(cr, dx + 39, dy + 41, 10.0, 56.0, ar, ag, ab, 0.25, 0.0);
            ngfx_shadow_rect(cr, dx, dy, 78, 82, 15, hov ? 3.5 : 2.2, hov ? 0.36 : 0.24);
            ngfx_fill_gradient(cr, dx, dy, 78, 82, 15,
                               0.06, 0.10, 0.09, hov ? 0.78 : 0.56,
                               0.03, 0.05, 0.04, hov ? 0.72 : 0.48, 0);
            ngfx_stroke_rect(cr, dx + 0.5, dy + 0.5, 77, 81, 15, 1.0,
                             hov ? ar : 1, hov ? ag : 1, hov ? ab : 1, hov ? 0.72 : 0.16);
            ngfx_top_specular(cr, dx, dy, 78, 15, hov ? 0.30 : 0.16);
            draw_app_icon(cr, desk_apps[i], dx + 18, dy + 10, 42, ar, ag, ab);
            double l_w = ngfx_text_width(cr, "Noto Sans", 1, 11.0, desk_labels[i]);
            ngfx_text(cr, "Noto Sans", 1, 11.0, dx + (78 - l_w) * 0.5, dy + 70, 0.96, 0.99, 0.97, 0.96, desk_labels[i]);
            nature_add_hit(st, dx, dy, 78, 82, HIT_DESKTOP_ICON, desk_apps[i], 0);
        }
    } else {
        int desk_x = st->cfg.dock_position == 1 ? (SW < 640 ? 68 : 88) : 20;
        for (int i = 0; i < 3; ++i) {
            int dy = 60 + i * 92;
            if (dy + 84 > SH - 84) break;
            int hov = (st->hover_mx >= desk_x && st->hover_mx <= desk_x + 76 && st->hover_my >= dy && st->hover_my <= dy + 82);
            if (hov) ngfx_radial_glow(cr, desk_x + 38, dy + 41, 10.0, 56.0, ar, ag, ab, 0.25, 0.0);
            ngfx_shadow_rect(cr, desk_x, dy, 76, 82, 15, hov ? 3.5 : 2.2, hov ? 0.36 : 0.24);
            ngfx_fill_gradient(cr, desk_x, dy, 76, 82, 15,
                               0.06, 0.10, 0.09, hov ? 0.78 : 0.54,
                               0.03, 0.05, 0.04, hov ? 0.72 : 0.46, 0);
            ngfx_stroke_rect(cr, desk_x + 0.5, dy + 0.5, 75, 81, 15, 1.0,
                             hov ? ar : 1, hov ? ag : 1, hov ? ab : 1, hov ? 0.72 : 0.14);
            ngfx_top_specular(cr, desk_x, dy, 76, 15, hov ? 0.30 : 0.16);
            draw_app_icon(cr, desk_apps[i], desk_x + 17, dy + 10, 42, ar, ag, ab);
            double l_w = ngfx_text_width(cr, "Noto Sans", 1, 11.0, desk_labels[i]);
            ngfx_text(cr, "Noto Sans", 1, 11.0, desk_x + (76 - l_w) * 0.5, dy + 70, 0.96, 0.99, 0.97, 0.96, desk_labels[i]);
            nature_add_hit(st, desk_x, dy, 76, 82, HIT_DESKTOP_ICON, desk_apps[i], 0);
        }
    }

    /* 3. Window Snap Preview Overlay */
    int snap_left = st->cfg.dock_position == 1 ? 76 : 12;
    int snap_bot = st->cfg.dock_position == 0 ? 78 : 12;
    int snap_half = (SW - snap_left - 18) / 2;
    if (st->snap_preview == 1) {
        ngfx_fill_rect(cr, snap_left, 46, snap_half, SH - 46 - snap_bot, 14, ar, ag, ab, 0.20);
        ngfx_stroke_rect(cr, snap_left, 46, snap_half, SH - 46 - snap_bot, 14, 1.8, ar, ag, ab, 0.88);
    } else if (st->snap_preview == 2) {
        ngfx_fill_rect(cr, snap_left + snap_half + 8, 46, snap_half, SH - 46 - snap_bot, 14, ar, ag, ab, 0.20);
        ngfx_stroke_rect(cr, snap_left + snap_half + 8, 46, snap_half, SH - 46 - snap_bot, 14, 1.8, ar, ag, ab, 0.88);
    }

    /* 4. Sort and Render Windows */
    int order[MAX_WINDOWS];
    int count = 0;
    for (int i = 0; i < st->window_count; ++i) {
        if (st->windows[i].workspace == st->active_workspace && !st->windows[i].minimized) {
            order[count++] = i;
        }
    }
    for (int i = 0; i < count; ++i) {
        for (int j = i + 1; j < count; ++j) {
            if (st->windows[order[i]].z_order > st->windows[order[j]].z_order) {
                int tmp = order[i]; order[i] = order[j]; order[j] = tmp;
            }
        }
    }
    for (int i = 0; i < count; ++i) {
        OSWindow *w = &st->windows[order[i]];
        render_window(st, cr, w, w->id == st->active_window_id);
    }

    /* 5. Floating Island Top Panel (smoked obsidian glass + vector tray icons) */
    int bar_x = 10, bar_y = 6, bar_w = SW - 20, bar_h = 34, bar_r = 17;
    ngfx_shadow_rect(cr, bar_x, bar_y, bar_w, bar_h, bar_r, 3.2, 0.42);
    ngfx_fill_gradient(cr, bar_x, bar_y, bar_w, bar_h, bar_r,
                       0.060, 0.092, 0.080, st->cfg.low_power_mode ? 0.98 : 0.88,
                       0.032, 0.052, 0.045, st->cfg.low_power_mode ? 0.98 : 0.88, 0);
    ngfx_stroke_rect(cr, bar_x + 0.5, bar_y + 0.5, bar_w - 1, bar_h - 1, bar_r, 1.0, 1.0, 1.0, 1.0, 0.14);
    ngfx_top_specular(cr, bar_x, bar_y, bar_w, bar_r, 0.24);

    int cur_x = bar_x + 16;
    ngfx_radial_glow(cr, cur_x, bar_y + 17, 1.0, 10.5, ar, ag, ab, 0.42, 0.0);
    ngfx_fill_circle(cr, cur_x, bar_y + 17, 5.2, ar * 1.25 > 1 ? 1 : ar * 1.25,
                     ag * 1.25 > 1 ? 1 : ag * 1.25, ab * 1.25 > 1 ? 1 : ab * 1.25, 1.0);
    if (SW >= 600) {
        ngfx_text(cr, "Plus Jakarta Sans", 1, 12.0, cur_x + 13, bar_y + 21.5, 0.96, 0.98, 0.97, 1.0, "Nature OS");
        cur_x += 92;
    } else {
        cur_x += 14;
    }

    int act_w = SW < 480 ? 70 : 78;
    int act_hov = (st->hover_mx >= cur_x && st->hover_mx <= cur_x + act_w && st->hover_my >= bar_y + 4 && st->hover_my <= bar_y + 30);
    if (st->launcher_open || act_hov) {
        ngfx_fill_rect(cr, cur_x, bar_y + 4, act_w, 26, 13,
                       st->launcher_open ? ar : 1.0, st->launcher_open ? ag : 1.0, st->launcher_open ? ab : 1.0,
                       st->launcher_open ? 0.88 : 0.11);
    }
    ngfx_text(cr, "Noto Sans", 1, 11.2, cur_x + (SW < 480 ? 10 : 13), bar_y + 21, 0.95, 0.98, 0.96, 1.0, "Activities");
    nature_add_hit(st, cur_x, bar_y + 4, act_w, 26, HIT_PANEL_ACTIVITIES, 0, 0);
    cur_x += act_w + 8;

    if (SW >= 520) {
        /* Unified segmented Workspace Switcher pill */
        ngfx_fill_rect(cr, cur_x, bar_y + 4, 82, 26, 13, 0.0, 0.0, 0.0, 0.22);
        ngfx_stroke_rect(cr, cur_x + 0.5, bar_y + 4.5, 81, 25, 13, 1.0, 1.0, 1.0, 1.0, 0.08);
        for (int ws = 0; ws < 2; ++ws) {
            int wx_btn = cur_x + 2 + ws * 39;
            int act = st->active_workspace == ws;
            if (act) ngfx_fill_rect(cr, wx_btn, bar_y + 6, 39, 22, 11, ar, ag, ab, 0.88);
            char wlbl[12];
            snprintf(wlbl, sizeof(wlbl), "WS%d", ws + 1);
            ngfx_text(cr, "JetBrains Mono", act, 10.2, wx_btn + 9, bar_y + 20.5, 0.96, 0.98, 0.97, act ? 1.0 : 0.65, wlbl);
            nature_add_hit(st, wx_btn, bar_y + 4, 39, 26, HIT_PANEL_WS, ws, 0);
        }
        cur_x += 88;
    }

    char clock_buf[48];
    strftime(clock_buf, sizeof(clock_buf), SW < 640 ? "%H:%M" : "%a %b %d  ·  %H:%M", tm_info);
    double cw_txt = ngfx_text_width(cr, "Plus Jakarta Sans", 1, 11.6, clock_buf);
    int c_pill_w = (int)cw_txt + 24;
    int c_pill_x = (SW - c_pill_w) / 2;
    if (c_pill_x > cur_x + 6) {
        int clk_hov = (st->hover_mx >= c_pill_x && st->hover_mx <= c_pill_x + c_pill_w &&
                       st->hover_my >= bar_y + 4 && st->hover_my <= bar_y + 30);
        if (st->calendar_open || clk_hov) {
            ngfx_fill_rect(cr, c_pill_x, bar_y + 4, c_pill_w, 26, 13,
                           st->calendar_open ? ar : 1.0, st->calendar_open ? ag : 1.0, st->calendar_open ? ab : 1.0,
                           st->calendar_open ? 0.88 : 0.10);
        }
        ngfx_text(cr, "Plus Jakarta Sans", 1, 11.6, c_pill_x + 12, bar_y + 21, 0.96, 0.99, 0.97, 0.98, clock_buf);
        nature_add_hit(st, c_pill_x, bar_y + 4, c_pill_w, 26, HIT_PANEL_CLOCK, 0, 0);
    }

    int qs_w = SW < 480 ? 84 : 112;
    int qs_btn_x = bar_x + bar_w - qs_w - 5;

    if (SW >= 820) {
        char stats_buf[80];
        snprintf(stats_buf, sizeof(stats_buf), "%.0f FPS · %.0f%% CPU · %dM",
                 st->measured_fps, st->total_cpu_pct, st->used_ram_mb);
        double st_w = ngfx_text_width(cr, "JetBrains Mono", 0, 10.5, stats_buf);
        int st_pill_w = (int)st_w + 18;
        int st_pill_x = qs_btn_x - st_pill_w - 10;
        if (st_pill_x > c_pill_x + c_pill_w + 8) {
            int m_hov = (st->hover_mx >= st_pill_x && st->hover_mx <= st_pill_x + st_pill_w &&
                         st->hover_my >= bar_y + 4 && st->hover_my <= bar_y + 30);
            if (m_hov) ngfx_fill_rect(cr, st_pill_x, bar_y + 4, st_pill_w, 26, 13, 1.0, 1.0, 1.0, 0.08);
            ngfx_fill_circle(cr, st_pill_x + 7, bar_y + 17, 2.8,
                             st->stress_active ? 0.95 : ar * 1.35 > 1 ? 1 : ar * 1.35,
                             st->stress_active ? 0.45 : ag * 1.35 > 1 ? 1 : ag * 1.35,
                             st->stress_active ? 0.25 : ab * 1.35 > 1 ? 1 : ab * 1.35, 0.95);
            ngfx_text(cr, "JetBrains Mono", 0, 10.5, st_pill_x + 15, bar_y + 21, 0.84, 0.92, 0.88, 0.88, stats_buf);
            nature_add_hit(st, st_pill_x, bar_y + 4, st_pill_w, 26, HIT_LAUNCHER_APP, APP_MONITOR, 0);
        }
    }

    int qs_hov = (st->hover_mx >= qs_btn_x && st->hover_mx <= qs_btn_x + qs_w &&
                  st->hover_my >= bar_y + 4 && st->hover_my <= bar_y + 30);
    ngfx_fill_rect(cr, qs_btn_x, bar_y + 4, qs_w, 26, 13,
                   st->quick_settings_open ? ar : 1.0, st->quick_settings_open ? ag : 1.0, st->quick_settings_open ? ab : 1.0,
                   st->quick_settings_open ? 0.88 : (qs_hov ? 0.16 : 0.09));
    /* Crisp vector system tray icons: Wi-Fi + Speaker + Vol % */
    draw_wifi_glyph(cr, qs_btn_x + 16, bar_y + 17, 0.95, 0.98, 0.96, st->cfg.wifi_enabled ? 0.95 : 0.35);
    draw_speaker_glyph(cr, qs_btn_x + 31, bar_y + 17, st->cfg.audio_muted, 0.95, 0.98, 0.96, 0.92);
    char qs_vol[16];
    if (st->cfg.audio_muted) snprintf(qs_vol, sizeof(qs_vol), "Mute");
    else snprintf(qs_vol, sizeof(qs_vol), "%d%%", st->cfg.volume);
    if (SW >= 480) {
        ngfx_text(cr, "JetBrains Mono", 0, 10.2, qs_btn_x + 52, bar_y + 20.5, 0.94, 0.97, 0.95, 0.95, qs_vol);
    }
    /* Tiny dropdown chevron */
    g_ngfx.set_source_rgba(cr, 0.95, 0.98, 0.96, 0.78);
    g_ngfx.set_line_width(cr, 1.3);
    double chx = qs_btn_x + qs_w - 15;
    g_ngfx.move_to(cr, chx, bar_y + 15.5); g_ngfx.line_to(cr, chx + 3.5, bar_y + 19.0); g_ngfx.line_to(cr, chx + 7.0, bar_y + 15.5);
    g_ngfx.stroke(cr);
    nature_add_hit(st, qs_btn_x, bar_y + 4, qs_w, 26, HIT_PANEL_TRAY, 0, 0);

    /* 6. Quick Settings & Calendar Popover Menus */
    if (st->quick_settings_open) {
        render_quick_settings_popover(st, cr);
    } else if (st->calendar_open) {
        render_calendar_popover(st, cr, tm_info);
    }

    /* 7. Notification Toast */
    if (st->toast_timer > 0) {
        st->toast_timer--;
        int tw = SW < 360 ? SW - 24 : 322, th = 60;
        int tx = SW - tw - 12, ty = 48;
        ngfx_shadow_rect(cr, tx, ty, tw, th, 14, 4.0, 0.50);
        ngfx_fill_gradient(cr, tx, ty, tw, th, 14, 0.08, 0.12, 0.10, 0.97, 0.05, 0.08, 0.07, 0.97, 0);
        ngfx_stroke_rect(cr, tx + 0.5, ty + 0.5, tw - 1, th - 1, 14, 1.0, ar, ag, ab, 0.58);
        ngfx_top_specular(cr, tx, ty, tw, 14, 0.25);
        ngfx_fill_rect(cr, tx + 8, ty + 12, 3.5, th - 24, 1.75, ar * 1.25 > 1 ? 1 : ar * 1.25,
                       ag * 1.25 > 1 ? 1 : ag * 1.25, ab * 1.25 > 1 ? 1 : ab * 1.25, 0.96);

        ngfx_text(cr, "Plus Jakarta Sans", 1, 12.0, tx + 20, ty + 22,
                  ar * 1.35 > 1 ? 1 : ar * 1.35, ag * 1.35 > 1 ? 1 : ag * 1.35, ab * 1.35 > 1 ? 1 : ab * 1.35, 1.0, st->toast_title);
        ngfx_text_clipped(cr, "Noto Sans", 0, 11.0, tx + 20, ty + 42, tw - 32, 0.90, 0.94, 0.92, 1.0, st->toast_msg);
    }

    /* 8. Botanical Floating Glass Dock */
    int icon_sz = SW < 520 ? 34 : 42;
    int gap = SW < 520 ? 7 : 10;
    int total_items = APP_COUNT + 1;
    st->hover_dock_idx = -1;
    if (st->cfg.dock_position == 0) {
        int dock_w = total_items * icon_sz + (total_items - 1) * gap + 30;
        int dock_h = icon_sz + 18;
        int dock_x = (SW - dock_w) / 2;
        int dock_y = SH - dock_h - 10;

        ngfx_shadow_rect(cr, dock_x, dock_y, dock_w, dock_h, 19, 4.5, 0.48);
        ngfx_fill_gradient(cr, dock_x, dock_y, dock_w, dock_h, 19,
                           0.072, 0.112, 0.098, st->cfg.low_power_mode ? 0.98 : 0.88,
                           0.036, 0.058, 0.050, st->cfg.low_power_mode ? 0.98 : 0.88, 0);
        ngfx_stroke_rect(cr, dock_x + 0.5, dock_y + 0.5, dock_w - 1, dock_h - 1, 19, 1.0,
                         1.0, 1.0, 1.0, 0.16);
        ngfx_top_specular(cr, dock_x, dock_y, dock_w, 19, 0.30);

        for (int i = 0; i < APP_COUNT; ++i) {
            int ix = dock_x + 12 + i * (icon_sz + gap);
            int iy = dock_y + 7;
            int hov = (st->hover_mx >= ix - 2 && st->hover_mx <= ix + icon_sz + 2 &&
                       st->hover_my >= iy - 6 && st->hover_my <= iy + icon_sz + 6);
            if (hov) {
                st->hover_dock_idx = i;
                ngfx_radial_glow(cr, ix + icon_sz * 0.5, iy + icon_sz * 0.4, 2.0, icon_sz * 0.72, ar, ag, ab, 0.32, 0.0);
                ngfx_shadow_rect(cr, ix, iy - 4, icon_sz, icon_sz, icon_sz * 0.27, 2.5, 0.40);
            }
            draw_app_icon(cr, (AppType)i, ix, hov ? iy - 4 : iy, icon_sz, ar, ag, ab);
            nature_add_hit(st, ix, iy, icon_sz, icon_sz, HIT_DOCK_APP, i, 0);

            int running = 0, focused = 0;
            for (int w = 0; w < st->window_count; ++w) {
                if (st->windows[w].app == (AppType)i) {
                    running = 1;
                    if (st->windows[w].id == st->active_window_id && !st->windows[w].minimized) focused = 1;
                }
            }
            if (running) {
                int ind_w = focused ? 16 : 6;
                if (focused) {
                    ngfx_radial_glow(cr, ix + icon_sz * 0.5, iy + icon_sz + 4, 1.0, 12.0, ar, ag, ab, 0.45, 0.0);
                }
                ngfx_fill_rect(cr, ix + (icon_sz - ind_w) / 2, iy + icon_sz + 3, ind_w, 3, 1.5,
                               ar * 1.4 > 1 ? 1 : ar * 1.4, ag * 1.35 > 1 ? 1 : ag * 1.35, ab * 1.35 > 1 ? 1 : ab * 1.35,
                               focused ? 1.0 : 0.70);
            }
            if (hov) {
                double tw = ngfx_text_width(cr, "Noto Sans", 1, 11.0, APP_NAMES[i]);
                int tx = ix + icon_sz / 2 - (int)(tw * 0.5) - 10;
                if (tx < 8) tx = 8;
                int ty = dock_y - 32;
                ngfx_shadow_rect(cr, tx, ty, tw + 20, 23, 8, 2.2, 0.36);
                ngfx_fill_rect(cr, tx, ty, tw + 20, 23, 8, 0.05, 0.08, 0.07, 0.95);
                ngfx_stroke_rect(cr, tx + 0.5, ty + 0.5, tw + 19, 22, 8, 1.0, 1, 1, 1, 0.16);
                ngfx_text(cr, "Noto Sans", 1, 11.0, tx + 10, ty + 15.5, 0.96, 0.99, 0.97, 1.0, APP_NAMES[i]);
            }
        }
        int sep_x = dock_x + 12 + APP_COUNT * (icon_sz + gap) - (gap / 2);
        ngfx_fill_rect(cr, sep_x, dock_y + 11, 1.5, icon_sz - 8, 1, 1, 1, 1, 0.16);

        int gx = sep_x + (gap / 2) + 2;
        int gy = dock_y + 7;
        int ghov = (st->hover_mx >= gx && st->hover_mx <= gx + icon_sz && st->hover_my >= gy && st->hover_my <= gy + icon_sz);
        ngfx_fill_rect(cr, gx, gy, icon_sz, icon_sz, 12,
                       ghov ? ar : 1.0, ghov ? ag : 1.0, ghov ? ab : 1.0, ghov ? 0.28 : 0.10);
        ngfx_stroke_rect(cr, gx + 0.5, gy + 0.5, icon_sz - 1, icon_sz - 1, 12, 1.0, 1.0, 1.0, 1.0, 0.14);
        double grid_off = (icon_sz - 20) * 0.5;
        for (int r = 0; r < 3; ++r) {
            for (int c = 0; c < 3; ++c) {
                ngfx_fill_rect(cr, gx + grid_off + c * 7.5, gy + grid_off + r * 7.5, 4.2, 4.2, 2, 0.92, 0.97, 0.94, 0.96);
            }
        }
        nature_add_hit(st, gx, gy, icon_sz, icon_sz, HIT_DOCK_LAUNCHER, 0, 0);
    } else {
        int dock_w = icon_sz + 16;
        int dock_h = total_items * icon_sz + (total_items - 1) * gap + 24;
        int dock_x = 8;
        int dock_y = 46 + (SH - 46 - dock_h) / 2;

        ngfx_shadow_rect(cr, dock_x, dock_y, dock_w, dock_h, 19, 4.5, 0.48);
        ngfx_fill_gradient(cr, dock_x, dock_y, dock_w, dock_h, 19,
                           0.072, 0.112, 0.098, st->cfg.low_power_mode ? 0.98 : 0.88,
                           0.036, 0.058, 0.050, st->cfg.low_power_mode ? 0.98 : 0.88, 1);
        ngfx_stroke_rect(cr, dock_x + 0.5, dock_y + 0.5, dock_w - 1, dock_h - 1, 19, 1.0,
                         1.0, 1.0, 1.0, 0.16);

        for (int i = 0; i < APP_COUNT; ++i) {
            int ix = dock_x + 8;
            int iy = dock_y + 12 + i * (icon_sz + gap);
            int hov = (st->hover_mx >= ix && st->hover_mx <= ix + icon_sz && st->hover_my >= iy && st->hover_my <= iy + icon_sz);
            if (hov) ngfx_radial_glow(cr, ix + icon_sz * 0.5, iy + icon_sz * 0.5, 2.0, icon_sz * 0.7, ar, ag, ab, 0.30, 0.0);
            draw_app_icon(cr, (AppType)i, hov ? ix + 2 : ix, iy, icon_sz, ar, ag, ab);
            nature_add_hit(st, ix, iy, icon_sz, icon_sz, HIT_DOCK_APP, i, 0);

            int running = 0, focused = 0;
            for (int w = 0; w < st->window_count; ++w) {
                if (st->windows[w].app == (AppType)i) {
                    running = 1;
                    if (st->windows[w].id == st->active_window_id && !st->windows[w].minimized) focused = 1;
                }
            }
            if (running) {
                int ih = focused ? 14 : 6;
                ngfx_fill_rect(cr, ix - 5, iy + (icon_sz - ih) / 2, 3, ih, 1.5,
                               ar * 1.4 > 1 ? 1 : ar * 1.4, ag * 1.35 > 1 ? 1 : ag * 1.35, ab * 1.35 > 1 ? 1 : ab * 1.35,
                               focused ? 1.0 : 0.70);
            }
        }
        int gx = dock_x + 8;
        int gy = dock_y + 12 + APP_COUNT * (icon_sz + gap);
        ngfx_fill_rect(cr, gx, gy, icon_sz, icon_sz, 12, 1.0, 1.0, 1.0, 0.10);
        double grid_off = (icon_sz - 20) * 0.5;
        for (int r = 0; r < 3; ++r) {
            for (int c = 0; c < 3; ++c) {
                ngfx_fill_rect(cr, gx + grid_off + c * 7.5, gy + grid_off + r * 7.5, 4.2, 4.2, 2, 0.92, 0.97, 0.94, 0.96);
            }
        }
        nature_add_hit(st, gx, gy, icon_sz, icon_sz, HIT_DOCK_LAUNCHER, 0, 0);
    }

    /* 9. Desktop Right-Click Context Menu */
    if (st->ctx_menu_open) {
        int mw = 204, mh = 152;
        int mx = st->ctx_menu_x;
        int my = st->ctx_menu_y;
        if (mx + mw > SW - 12) mx = SW - mw - 12;
        if (my + mh > SH - 80) my = SH - mh - 80;
        if (mx < 10) mx = 10;
        if (my < 46) my = 46;

        nature_occlude_text_rects(st, mx, my, mw, mh);

        ngfx_shadow_rect(cr, mx, my, mw, mh, 13, 4.0, 0.55);
        ngfx_fill_gradient(cr, mx, my, mw, mh, 13, 0.08, 0.115, 0.105, 0.97, 0.05, 0.08, 0.07, 0.97, 0);
        ngfx_stroke_rect(cr, mx + 0.5, my + 0.5, mw - 1, mh - 1, 13, 1.0, 1, 1, 1, 0.16);
        ngfx_top_specular(cr, mx, my, mw, 13, 0.24);

        const char *c_labels[4] = {
            "Open Terminal Here",
            "Open Files Manager",
            "Next Wallpaper",
            "Display Settings"
        };
        const char *c_tags[4] = {">_", "FS", "WP", "DS"};
        for (int i = 0; i < 4; ++i) {
            int ry = my + 8 + i * 34;
            int hov = (st->hover_mx >= mx + 6 && st->hover_mx <= mx + mw - 6 && st->hover_my >= ry && st->hover_my <= ry + 29);
            if (hov) ngfx_fill_rect(cr, mx + 6, ry, mw - 12, 29, 8, ar, ag, ab, 0.34);
            ngfx_text(cr, "JetBrains Mono", 1, 9.5, mx + 14, ry + 18.5,
                      hov ? 1.0 : ar * 1.25 > 1 ? 1 : ar * 1.25, hov ? 1.0 : ag * 1.25 > 1 ? 1 : ag * 1.25,
                      hov ? 1.0 : ab * 1.25 > 1 ? 1 : ab * 1.25, 0.90, c_tags[i]);
            ngfx_text(cr, "Noto Sans", hov, 11.8, mx + 38, ry + 19, 0.95, 0.98, 0.96, 1.0, c_labels[i]);
            nature_add_hit(st, mx + 6, ry, mw - 12, 29, HIT_CTX_MENU_ITEM, i, 0);
        }
    }

    /* 10. Activities / Spotlight Overview Overlay */
    if (st->launcher_open) {
        st->text_rect_count = 0;
        ngfx_fill_rect(cr, 0, 0, SW, SH, 0, 0.018, 0.034, 0.028, 0.90);
        ngfx_radial_glow(cr, SW * 0.5, SH * 0.35, 30.0, (SW > SH ? SW : SH) * 0.55, ar, ag, ab, 0.14, 0.0);

        /* Workspace Switcher */
        int wsw = SW < 380 ? SW - 24 : 336, wsh = 38;
        int wsx = (SW - wsw) / 2, wsy = 48;
        ngfx_fill_rect(cr, wsx, wsy, wsw, wsh, 13, 0.07, 0.10, 0.09, 0.95);
        ngfx_stroke_rect(cr, wsx + 0.5, wsy + 0.5, wsw - 1, wsh - 1, 13, 1.0, 1.0, 1.0, 1.0, 0.12);
        int bw = (wsw - 22) / 2;
        for (int ws = 0; ws < 2; ++ws) {
            int bx = wsx + 7 + ws * (bw + 8);
            int act = st->active_workspace == ws;
            ngfx_fill_rect(cr, bx, wsy + 5, bw, 28, 9,
                           act ? ar : 1.0, act ? ag : 1.0, act ? ab : 1.0,
                           act ? 0.88 : 0.06);
            char lbl[32];
            snprintf(lbl, sizeof(lbl), "Workspace %d", ws + 1);
            double tw = ngfx_text_width(cr, "Plus Jakarta Sans", 1, 11.5, lbl);
            ngfx_text(cr, "Plus Jakarta Sans", 1, 11.5, bx + (bw - tw) * 0.5, wsy + 23, 1, 1, 1, act ? 1.0 : 0.72, lbl);
            nature_add_hit(st, bx, wsy + 5, bw, 28, HIT_PANEL_WS, ws, 0);
        }

        /* Spotlight Search Box with vector magnifying glass */
        int s_w = SW < 480 ? SW - 28 : 450, s_h = 40;
        int sx = (SW - s_w) / 2, sy = wsy + 48;
        ngfx_shadow_rect(cr, sx, sy, s_w, s_h, 20, 3.0, 0.35);
        ngfx_fill_rect(cr, sx, sy, s_w, s_h, 20, 0.06, 0.09, 0.08, 0.97);
        ngfx_stroke_rect(cr, sx + 0.5, sy + 0.5, s_w - 1, s_h - 1, 20, 1.3, ar, ag, ab, 0.80);
        /* Vector magnifying lens */
        g_ngfx.set_source_rgba(cr, 0.70, 0.86, 0.78, 0.92);
        g_ngfx.set_line_width(cr, 1.6);
        g_ngfx.new_sub_path(cr);
        g_ngfx.arc(cr, sx + 21, sy + 18, 5.5, 0, 6.283);
        g_ngfx.stroke(cr);
        g_ngfx.move_to(cr, sx + 25, sy + 22);
        g_ngfx.line_to(cr, sx + 29, sy + 26);
        g_ngfx.stroke(cr);

        if (st->launcher_query[0]) {
            ngfx_text(cr, "Plus Jakarta Sans", 1, 13.0, sx + 40, sy + 24.5, 0.96, 0.99, 0.97, 1.0, st->launcher_query);
        } else {
            ngfx_text(cr, "Noto Sans", 0, 12.2, sx + 40, sy + 24.5, 0.62, 0.72, 0.67, 0.9,
                      "Type to search Nature OS apps...");
        }
        nature_add_hit(st, sx, sy, s_w, s_h, HIT_LAUNCHER_SEARCH, 0, 0);
        nature_add_text_rect(st, sx, sy, s_w, s_h, 2);

        int thumb_y = sy + 50;
        int open_count = 0;
        for (int w = 0; w < st->window_count; ++w) {
            if (st->windows[w].workspace == st->active_workspace) open_count++;
        }
        if (open_count > 0 && !st->launcher_query[0] && SW >= 580) {
            int tw = 192, th = 84;
            int max_thumbs = (SW - 40) / (tw + 14);
            if (max_thumbs < 1) max_thumbs = 1;
            int show_n = open_count < max_thumbs ? open_count : max_thumbs;
            int total_w = show_n * tw + (show_n - 1) * 14;
            int start_x = (SW - total_w) / 2;
            int slot = 0;
            for (int w = 0; w < st->window_count && slot < show_n; ++w) {
                if (st->windows[w].workspace != st->active_workspace) continue;
                int tx = start_x + slot * (tw + 14);
                int thov = (st->hover_mx >= tx && st->hover_mx <= tx + tw && st->hover_my >= thumb_y && st->hover_my <= thumb_y + th);
                ngfx_shadow_rect(cr, tx, thumb_y, tw, th, 12, thov ? 3.5 : 2.0, 0.40);
                ngfx_fill_rect(cr, tx, thumb_y, tw, th, 12, 0.075, 0.11, 0.098, thov ? 0.99 : 0.94);
                ngfx_stroke_rect(cr, tx + 0.5, thumb_y + 0.5, tw - 1, th - 1, 12, thov ? 1.5 : 1.0,
                                 ar, ag, ab, thov ? 0.95 : 0.55);
                draw_app_icon(cr, st->windows[w].app, tx + 12, thumb_y + 12, 28, ar, ag, ab);
                ngfx_text_clipped(cr, "Plus Jakarta Sans", 1, 11.8, tx + 48, thumb_y + 30, tw - 58, 0.95, 0.98, 0.96, 1.0, st->windows[w].title);
                ngfx_text(cr, "Noto Sans", 0, 10.0, tx + 14, thumb_y + 64, 0.65, 0.78, 0.72, 1.0,
                          st->windows[w].minimized ? "Minimized · Tap to restore" : "Tap to focus window");
                nature_add_hit(st, tx, thumb_y, tw, th, HIT_OVERVIEW_WIN, st->windows[w].id, 0);
                slot++;
            }
        }

        int lw = SW < 760 ? SW - 24 : 748;
        int ly = thumb_y + (open_count > 0 && !st->launcher_query[0] && SW >= 580 ? 102 : 14);
        int lh = SH - ly - 80;
        if (lh > 318) lh = 318;
        int lx = (SW - lw) / 2;
        ngfx_shadow_rect(cr, lx, ly, lw, lh, 18, 4.5, 0.55);
        ngfx_fill_gradient(cr, lx, ly, lw, lh, 18, 0.072, 0.105, 0.094, 0.98, 0.052, 0.078, 0.070, 0.98, 0);
        ngfx_stroke_rect(cr, lx + 0.5, ly + 0.5, lw - 1, lh - 1, 18, 1.0, 1.0, 1.0, 1.0, 0.15);
        ngfx_top_specular(cr, lx, ly, lw, 18, 0.25);

        ngfx_text(cr, "Plus Jakarta Sans", 1, 13.8, lx + 24, ly + 30, 0.95, 0.98, 0.96, 1.0,
                  "Applications");
        int cols = lw >= 640 ? 4 : (lw >= 420 ? 3 : 2);
        int cell_w = (lw - 48 - (cols - 1) * 14) / cols;
        int cell_h = lh >= 280 ? 102 : 76;
        int shown = 0;
        for (int i = 0; i < APP_COUNT; ++i) {
            if (!str_contains_ci(APP_NAMES[i], st->launcher_query)) continue;
            int col = shown % cols;
            int row = shown / cols;
            int ax = lx + 24 + col * (cell_w + 14);
            int ay = ly + 46 + row * (cell_h + 10);
            if (ay + cell_h > ly + lh - 10) break;
            int ahov = (st->hover_mx >= ax && st->hover_mx <= ax + cell_w && st->hover_my >= ay && st->hover_my <= ay + cell_h);
            if (ahov) ngfx_radial_glow(cr, ax + cell_w * 0.5, ay + cell_h * 0.45, 4.0, cell_w * 0.5, ar, ag, ab, 0.18, 0.0);
            ngfx_fill_rect(cr, ax, ay, cell_w, cell_h, 13,
                           ahov ? ar : 1.0, ahov ? ag : 1.0, ahov ? ab : 1.0, ahov ? 0.18 : 0.045);
            ngfx_stroke_rect(cr, ax + 0.5, ay + 0.5, cell_w - 1, cell_h - 1, 13, 1.0,
                             ahov ? ar : 1.0, ahov ? ag : 1.0, ahov ? ab : 1.0, ahov ? 0.65 : 0.08);
            int isz = cell_h >= 96 ? 48 : 36;
            draw_app_icon(cr, (AppType)i, ax + (cell_w - isz) * 0.5, ay + 11, isz, ar, ag, ab);
            double tw = ngfx_text_width(cr, "Noto Sans", 1, 11.2, APP_NAMES[i]);
            if (tw < cell_w - 12) {
                ngfx_text(cr, "Noto Sans", 1, 11.2, ax + (cell_w - tw) * 0.5, ay + cell_h - 14, 0.94, 0.98, 0.96, 1.0, APP_NAMES[i]);
            } else {
                ngfx_text_clipped(cr, "Noto Sans", 1, 11.2, ax + 8, ay + cell_h - 14, cell_w - 16, 0.94, 0.98, 0.96, 1.0, APP_NAMES[i]);
            }
            nature_add_hit(st, ax, ay, cell_w, cell_h, HIT_LAUNCHER_APP, i, 0);
            shown++;
        }
    }

    /* 11. Global Hardware Night Light & Brightness Post-Process Filter */
    if (st->cfg.night_light) {
        ngfx_fill_rect(cr, 0, 0, SW, SH, 0, 0.96, 0.48, 0.06, 0.13);
    }
    if (st->cfg.brightness < 100) {
        double dim = (100 - st->cfg.brightness) * 0.006;
        if (dim > 0.55) dim = 0.55;
        if (dim > 0.0) ngfx_fill_rect(cr, 0, 0, SW, SH, 0, 0.0, 0.0, 0.0, dim);
    }
}

void nature_os_right_click(NatureOSState *st, int mx, int my) {
    st->quick_settings_open = 0;
    st->launcher_open = 0;
    for (int i = 0; i < st->window_count; ++i) {
        OSWindow *w = &st->windows[i];
        if (!w->minimized && w->workspace == st->active_workspace) {
            if (mx >= w->x && mx <= w->x + w->w && my >= w->y && my <= w->y + w->h) {
                nature_focus_window(st, w->id);
                st->ctx_menu_open = 0;
                return;
            }
        }
    }
    st->ctx_menu_open = 1;
    st->ctx_menu_x = mx;
    st->ctx_menu_y = my;
    st->dirty = 1;
}

static void update_scrollbar_from_y(NatureOSState *st, int my) {
    int *sptr = get_scroll_ptr(st, st->scroll_drag_target);
    if (!sptr || st->scroll_drag_track_h <= 0 || st->scroll_drag_max <= 0) return;
    double frac = (double)(my - st->scroll_drag_track_y) / (double)st->scroll_drag_track_h;
    if (frac < 0.0) frac = 0.0;
    if (frac > 1.0) frac = 1.0;
    if (st->scroll_drag_target == SCROLL_TERMINAL) {
        frac = 1.0 - frac;
    }
    *sptr = (int)lround(frac * st->scroll_drag_max);
    if (*sptr < 0) *sptr = 0;
    if (*sptr > st->scroll_drag_max) *sptr = st->scroll_drag_max;
}

static void update_slider_from_x(NatureOSState *st, int mx) {
    if (st->slider_w <= 0) return;
    double frac = (double)(mx - st->slider_x) / (double)st->slider_w;
    if (frac < 0.0) frac = 0.0;
    if (frac > 1.0) frac = 1.0;
    if (st->drag_mode == 4) {
        st->cfg.audio_muted = 0;
        st->cfg.volume = (int)lround(frac * 100.0);
    } else if (st->drag_mode == 5) {
        int val = 25 + (int)lround(frac * 75.0);
        if (val > 100) val = 100;
        st->cfg.brightness = val;
    }
}

void nature_os_pointer_down(NatureOSState *st, int mx, int my) {
    st->hover_mx = mx;
    st->hover_my = my;
    st->input_focus_pulse = 0;
    st->active_text_target = 0;

    for (int i = st->hit_count - 1; i >= 0; --i) {
        const HitRect *h = &st->hits[i];
        if (mx < h->x || mx > h->x + h->w || my < h->y || my > h->y + h->h) continue;

        switch (h->action_id) {
            case HIT_SCROLLBAR:
                st->drag_mode = 3;
                st->scroll_drag_target = h->arg0;
                st->scroll_drag_max = h->arg1;
                st->scroll_drag_track_y = h->y;
                st->scroll_drag_track_h = h->h;
                update_scrollbar_from_y(st, my);
                st->dirty = 1;
                return;
            case HIT_SLIDER_VOL:
                st->drag_mode = 4;
                st->slider_x = h->arg0;
                st->slider_w = h->arg1;
                update_slider_from_x(st, mx);
                st->dirty = 1;
                return;
            case HIT_SLIDER_BRIGHT:
                st->drag_mode = 5;
                st->slider_x = h->arg0;
                st->slider_w = h->arg1;
                update_slider_from_x(st, mx);
                st->dirty = 1;
                return;
            case HIT_LAUNCHER_SEARCH:
                st->active_text_target = 2;
                st->input_focus_pulse = 1;
                st->dirty = 1;
                return;
            case HIT_PANEL_ACTIVITIES:
            case HIT_DOCK_LAUNCHER:
                st->launcher_open = !st->launcher_open;
                st->launcher_query[0] = '\0';
                st->quick_settings_open = 0;
                st->calendar_open = 0;
                st->ctx_menu_open = 0;
                if (st->launcher_open) st->active_text_target = 2;
                st->dirty = 1;
                return;
            case HIT_PANEL_TRAY:
                st->quick_settings_open = !st->quick_settings_open;
                st->calendar_open = 0;
                st->launcher_open = 0;
                st->ctx_menu_open = 0;
                st->dirty = 1;
                return;
            case HIT_PANEL_CLOCK:
                st->calendar_open = !st->calendar_open;
                st->quick_settings_open = 0;
                st->launcher_open = 0;
                st->ctx_menu_open = 0;
                st->dirty = 1;
                return;
            case HIT_CAL_CLEAR_NOTIFS:
                st->notif_count = 0;
                st->dirty = 1;
                return;
            case HIT_CTX_MENU_ITEM:
                st->ctx_menu_open = 0;
                if (h->arg0 == 0) nature_open_app(st, APP_TERMINAL);
                else if (h->arg0 == 1) nature_open_app(st, APP_FILES);
                else if (h->arg0 == 2) {
                    st->cfg.wallpaper_idx = (st->cfg.wallpaper_idx + 1) % 3;
                    nature_notify(st, "Wallpaper", "Switched desktop wallpaper.");
                } else {
                    st->settings_section = 1;
                    nature_open_app(st, APP_SETTINGS);
                }
                st->dirty = 1;
                return;
            case HIT_QS_WIFI:
                st->cfg.wifi_enabled = !st->cfg.wifi_enabled;
                nature_notify(st, "NetworkManager", st->cfg.wifi_enabled ? "Wi-Fi enabled · Canopy-5GHz" : "Wi-Fi disabled.");
                st->dirty = 1;
                return;
            case HIT_QS_BLUETOOTH:
                st->cfg.bluetooth_enabled = !st->cfg.bluetooth_enabled;
                st->dirty = 1;
                return;
            case HIT_QS_DARK:
                st->cfg.dark_mode = !st->cfg.dark_mode;
                st->dirty = 1;
                return;
            case HIT_QS_NIGHT_LIGHT:
                st->cfg.night_light = !st->cfg.night_light;
                st->dirty = 1;
                return;
            case HIT_QS_PERF:
                st->cfg.power_profile = (st->cfg.power_profile + 1) % 3;
                st->dirty = 1;
                return;
            case HIT_QS_SETTINGS:
                st->quick_settings_open = 0;
                nature_open_app(st, APP_SETTINGS);
                return;
            case HIT_QS_REBOOT:
                nature_notify(st, "systemd-logind", "Session refresh scheduled cleanly.");
                st->quick_settings_open = 0;
                st->dirty = 1;
                return;
            case HIT_PANEL_WS:
                st->active_workspace = h->arg0;
                st->launcher_open = 0;
                st->quick_settings_open = 0;
                st->ctx_menu_open = 0;
                st->dirty = 1;
                return;
            case HIT_OVERVIEW_WIN:
                st->launcher_open = 0;
                nature_focus_window(st, h->arg0);
                return;
            case HIT_DOCK_APP: {
                AppType target = (AppType)h->arg0;
                for (int w = 0; w < st->window_count; ++w) {
                    if (st->windows[w].app == target && st->windows[w].id == st->active_window_id && !st->windows[w].minimized) {
                        st->windows[w].minimized = 1;
                        st->dirty = 1;
                        return;
                    }
                }
                nature_open_app(st, target);
                return;
            }
            case HIT_LAUNCHER_APP:
            case HIT_DESKTOP_ICON:
                st->ctx_menu_open = 0;
                nature_open_app(st, (AppType)h->arg0);
                return;
            case HIT_WIN_CLOSE:
                nature_close_window(st, h->arg0);
                return;
            case HIT_WIN_MIN:
                for (int w = 0; w < st->window_count; ++w) {
                    if (st->windows[w].id == h->arg0) {
                        st->windows[w].minimized = 1;
                        st->dirty = 1;
                        return;
                    }
                }
                return;
            case HIT_WIN_MAX:
                for (int w = 0; w < st->window_count; ++w) {
                    if (st->windows[w].id == h->arg0) {
                        st->windows[w].maximized = !st->windows[w].maximized;
                        clamp_window_to_screen(st, &st->windows[w]);
                        nature_focus_window(st, h->arg0);
                        return;
                    }
                }
                return;
            case HIT_WIN_SNAP:
                for (int w = 0; w < st->window_count; ++w) {
                    if (st->windows[w].id == h->arg0) {
                        OSWindow *win = &st->windows[w];
                        win->maximized = 0;
                        win->snapped = (win->snapped != 1) ? 1 : 2;
                        clamp_window_to_screen(st, win);
                        nature_focus_window(st, win->id);
                        return;
                    }
                }
                return;
            case HIT_WIN_TITLE:
                for (int w = 0; w < st->window_count; ++w) {
                    if (st->windows[w].id == h->arg0) {
                        OSWindow *win = &st->windows[w];
                        nature_focus_window(st, win->id);
                        if (!win->maximized) {
                            st->drag_mode = 1;
                            st->drag_win_id = win->id;
                            st->drag_start_mx = mx;
                            st->drag_start_my = my;
                            st->drag_orig_x = win->x;
                            st->drag_orig_y = win->y;
                        }
                        return;
                    }
                }
                return;
            case HIT_WIN_RESIZE:
                for (int w = 0; w < st->window_count; ++w) {
                    if (st->windows[w].id == h->arg0) {
                        OSWindow *win = &st->windows[w];
                        nature_focus_window(st, win->id);
                        st->drag_mode = 2;
                        st->drag_win_id = win->id;
                        st->drag_start_mx = mx;
                        st->drag_start_my = my;
                        st->drag_orig_w = win->w;
                        st->drag_orig_h = win->h;
                        return;
                    }
                }
                return;
            default:
                st->ctx_menu_open = 0;
                if (h->action_id >= 100 && h->action_id < 200) { nature_files_click(st, h); st->dirty = 1; return; }
                if (h->action_id >= 200 && h->action_id < 300) { nature_monitor_click(st, h); st->dirty = 1; return; }
                if (h->action_id >= 300 && h->action_id < 400) { nature_settings_click(st, h); st->dirty = 1; return; }
                if (h->action_id >= 400 && h->action_id < 500) { nature_store_click(st, h); st->dirty = 1; return; }
                if (h->action_id >= 500 && h->action_id < 600) {
                    /* Support direct X-seek in media progress bar */
                    if (h->action_id == HIT_MEDIA_AUDIO_SEEK && h->w > 0) {
                        HitRect seek_h = *h;
                        seek_h.arg1 = mx - h->x;
                        nature_media_click(st, &seek_h);
                    } else {
                        nature_media_click(st, h);
                    }
                    st->dirty = 1;
                    return;
                }
                if (h->action_id >= 600 && h->action_id < 700) { nature_browser_click(st, h); st->dirty = 1; return; }
                if (h->action_id >= 700 && h->action_id < 800) { nature_terminal_click(st, h); st->dirty = 1; return; }
                break;
        }
    }

    /* Click window body to focus (and check text input rect hit!) */
    for (int z_pass = st->next_z; z_pass >= 0; --z_pass) {
        for (int i = 0; i < st->window_count; ++i) {
            OSWindow *w = &st->windows[i];
            if (w->minimized || w->workspace != st->active_workspace) continue;
            if (w->z_order != z_pass) continue;
            if (mx >= w->x && mx <= w->x + w->w && my >= w->y && my <= w->y + w->h) {
                if (st->active_window_id != w->id) nature_focus_window(st, w->id);
                if (w->app == APP_TERMINAL) {
                    st->active_text_target = 1;
                    st->input_focus_pulse = 1;
                    st->dirty = 1;
                }
                st->quick_settings_open = 0;
                st->calendar_open = 0;
                st->ctx_menu_open = 0;
                return;
            }
        }
    }

    if (st->launcher_open || st->quick_settings_open || st->calendar_open || st->ctx_menu_open) {
        st->launcher_open = 0;
        st->launcher_query[0] = '\0';
        st->quick_settings_open = 0;
        st->calendar_open = 0;
        st->ctx_menu_open = 0;
        st->dirty = 1;
    }
}

void nature_os_pointer_move(NatureOSState *st, int mx, int my) {
    st->hover_mx = mx;
    st->hover_my = my;

    if (st->drag_mode == 3) {
        update_scrollbar_from_y(st, my);
        st->dirty = 1;
        return;
    }
    if (st->drag_mode == 4 || st->drag_mode == 5) {
        update_slider_from_x(st, mx);
        st->dirty = 1;
        return;
    }

    if (st->drag_mode != 0) {
        int SW = st->screen_w > 0 ? st->screen_w : OS_WIDTH;
        int SH = st->screen_h > 0 ? st->screen_h : OS_HEIGHT;
        for (int i = 0; i < st->window_count; ++i) {
            OSWindow *w = &st->windows[i];
            if (w->id != st->drag_win_id) continue;
            if (st->drag_mode == 1) {
                int nx = st->drag_orig_x + (mx - st->drag_start_mx);
                int ny = st->drag_orig_y + (my - st->drag_start_my);
                if (ny < 44) ny = 44;
                if (ny > SH - 76) ny = SH - 76;
                if (nx < -w->w + 80) nx = -w->w + 80;
                if (nx > SW - 80) nx = SW - 80;
                w->x = nx;
                w->y = ny;
                w->snapped = 0;
                st->snap_preview = (mx < 16) ? 1 : (mx > SW - 16 ? 2 : 0);
                st->dirty = 1;
            } else if (st->drag_mode == 2) {
                int nw = st->drag_orig_w + (mx - st->drag_start_mx);
                int nh = st->drag_orig_h + (my - st->drag_start_my);
                if (nw < 360) nw = 360;
                if (nh < 250) nh = 250;
                if (w->x + nw > SW - 6) nw = SW - 6 - w->x;
                if (w->y + nh > SH - 10) nh = SH - 10 - w->y;
                w->w = nw;
                w->h = nh;
                st->dirty = 1;
            }
            return;
        }
    }

    int next_act = HIT_NONE, next_arg0 = -1;
    for (int i = st->hit_count - 1; i >= 0; --i) {
        const HitRect *h = &st->hits[i];
        if (mx >= h->x && mx <= h->x + h->w && my >= h->y && my <= h->y + h->h) {
            next_act = h->action_id;
            next_arg0 = h->arg0;
            break;
        }
    }
    if (next_act != st->hover_hit_action || next_arg0 != st->hover_hit_arg0 || st->ctx_menu_open) {
        st->hover_hit_action = next_act;
        st->hover_hit_arg0 = next_arg0;
        st->dirty = 1;
    }
}

void nature_os_pointer_up(NatureOSState *st, int mx, int my) {
    (void)mx; (void)my;
    if (st->drag_mode == 1 && st->snap_preview != 0) {
        for (int i = 0; i < st->window_count; ++i) {
            OSWindow *w = &st->windows[i];
            if (w->id == st->drag_win_id) {
                w->snapped = st->snap_preview;
                clamp_window_to_screen(st, w);
                break;
            }
        }
    }
    st->drag_mode = 0;
    st->drag_win_id = -1;
    st->scroll_drag_target = SCROLL_NONE;
    st->snap_preview = 0;
    st->dirty = 1;
}

void nature_os_wheel(NatureOSState *st, int delta_y) {
    int target_app = -1;
    /* Prefer window directly under cursor */
    for (int z_pass = st->next_z; z_pass >= 0 && target_app < 0; --z_pass) {
        for (int i = 0; i < st->window_count; ++i) {
            OSWindow *w = &st->windows[i];
            if (w->minimized || w->workspace != st->active_workspace) continue;
            if (w->z_order != z_pass) continue;
            if (st->hover_mx >= w->x && st->hover_mx <= w->x + w->w &&
                st->hover_my >= w->y && st->hover_my <= w->y + w->h) {
                target_app = w->app;
                break;
            }
        }
    }
    /* Fallback to active window */
    if (target_app < 0) {
        for (int i = 0; i < st->window_count; ++i) {
            if (st->windows[i].id == st->active_window_id && !st->windows[i].minimized) {
                target_app = st->windows[i].app;
                break;
            }
        }
    }

    switch (target_app) {
        case APP_FILES:    nature_files_wheel(st, delta_y); break;
        case APP_TERMINAL: nature_terminal_wheel(st, delta_y); break;
        case APP_MONITOR:  nature_monitor_wheel(st, delta_y); break;
        case APP_SETTINGS: nature_settings_wheel(st, delta_y); break;
        case APP_STORE:    nature_store_wheel(st, delta_y); break;
        case APP_BROWSER:  nature_browser_wheel(st, delta_y); break;
        case APP_MEDIA:    nature_media_wheel(st, delta_y); break;
        default: break;
    }
    st->dirty = 1;
}

void nature_os_key(NatureOSState *st, const char *key) {
    if (!key || !*key) return;
    if (strcmp(key, "Escape") == 0) {
        st->launcher_open = 0;
        st->launcher_query[0] = '\0';
        st->quick_settings_open = 0;
        st->calendar_open = 0;
        st->ctx_menu_open = 0;
        st->active_text_target = 0;
        st->files_viewer_open = 0;
        st->dirty = 1;
        return;
    }
    if (strcmp(key, "Super") == 0 || strcmp(key, "Meta") == 0) {
        st->launcher_open = !st->launcher_open;
        st->launcher_query[0] = '\0';
        st->active_text_target = st->launcher_open ? 2 : 0;
        st->dirty = 1;
        return;
    }
    if (st->launcher_open) {
        if (strcmp(key, "Backspace") == 0) {
            size_t len = strlen(st->launcher_query);
            if (len > 0) st->launcher_query[len - 1] = '\0';
        } else if (strcmp(key, "Enter") == 0) {
            for (int i = 0; i < APP_COUNT; ++i) {
                if (str_contains_ci(APP_NAMES[i], st->launcher_query)) {
                    nature_open_app(st, (AppType)i);
                    break;
                }
            }
        } else if (strlen(key) == 1 && (unsigned char)key[0] >= 32 && (unsigned char)key[0] < 127) {
            size_t len = strlen(st->launcher_query);
            if (len + 1 < sizeof(st->launcher_query)) {
                st->launcher_query[len] = key[0];
                st->launcher_query[len + 1] = '\0';
            }
        }
        st->dirty = 1;
        return;
    }

    for (int i = 0; i < st->window_count; ++i) {
        if (st->windows[i].id == st->active_window_id) {
            if (st->windows[i].app == APP_BROWSER && st->active_text_target == 3) {
                nature_browser_key(st, key);
                st->dirty = 1;
                return;
            }
            if (st->windows[i].app == APP_FILES && st->active_text_target == 4) {
                nature_files_key(st, key);
                st->dirty = 1;
                return;
            }
            if (st->windows[i].app == APP_TERMINAL) {
                nature_terminal_key(st, key);
                st->dirty = 1;
                return;
            }
        }
    }
}
