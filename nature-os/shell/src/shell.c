#include "nature_os.h"
#include "shell_internal.h"
#include "nature_gfx.h"
#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

const char *APP_NAMES[APP_COUNT] = {
    "Files",
    "Sylva Browser",
    "Terminal",
    "Ubuntu Software",
    "System Monitor",
    "Media Viewer",
    "Settings"
};

int64_t get_current_time_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (int64_t)ts.tv_sec * 1000LL + (int64_t)(ts.tv_nsec / 1000000LL);
}

int str_contains_ci(const char *hay, const char *needle) {
    if (!needle || !*needle) return 1;
    if (!hay) return 0;
    size_t hlen = strlen(hay), nlen = strlen(needle);
    if (nlen > hlen) return 0;
    for (size_t i = 0; i <= hlen - nlen; ++i) {
        size_t j = 0;
        while (j < nlen && tolower((unsigned char)hay[i + j]) == tolower((unsigned char)needle[j])) j++;
        if (j == nlen) return 1;
    }
    return 0;
}

void format_clock_str(const NatureOSState *st, const struct tm *tm_info,
                      int include_date, char *buf, size_t sz) {
    if (!tm_info || !buf || sz == 0) return;
    const char *time_fmt;
    if (st->cfg.clock_24h) {
        time_fmt = st->cfg.show_seconds ? "%H:%M:%S" : "%H:%M";
    } else {
        time_fmt = st->cfg.show_seconds ? "%I:%M:%S %p" : "%I:%M %p";
    }
    if (include_date) {
        char fmt[48];
        snprintf(fmt, sizeof(fmt), "%%a %%b %%d  ·  %s", time_fmt);
        strftime(buf, sz, fmt, tm_info);
    } else {
        strftime(buf, sz, time_fmt, tm_info);
    }
}

int app_matches_launcher_cat(AppType app, int cat) {
    if (cat <= 0) return 1; /* All */
    switch (app) {
        case APP_FILES:
        case APP_STORE:
        case APP_SETTINGS: return cat == 1; /* System */
        case APP_BROWSER:  return cat == 2; /* Internet */
        case APP_MEDIA:    return cat == 3; /* Media */
        case APP_TERMINAL:
        case APP_MONITOR:  return cat == 4; /* Utilities */
        default: return 1;
    }
}

void nature_notify(NatureOSState *st, const char *title, const char *msg) {
    if (!st) return;
    snprintf(st->toast_title, sizeof(st->toast_title), "%s", title ? title : "Notification");
    snprintf(st->toast_msg, sizeof(st->toast_msg), "%s", msg ? msg : "");
    st->toast_timer = 240; /* ~4s at 60fps */
    if (st->notif_count < 4) {
        snprintf(st->notif_titles[st->notif_count], sizeof(st->notif_titles[0]), "%s", title ? title : "Alert");
        snprintf(st->notif_msgs[st->notif_count], sizeof(st->notif_msgs[0]), "%s", msg ? msg : "");
        time_t now = time(NULL);
        struct tm tm_b;
        const struct tm *tm_p = localtime_r(&now, &tm_b);
        if (tm_p) strftime(st->notif_times[st->notif_count], sizeof(st->notif_times[0]), "%H:%M", tm_p);
        st->notif_count++;
    }
    st->dirty = 1;
}

void nature_add_hit(NatureOSState *st, int x, int y, int w, int h, int action, int arg0, int arg1) {
    if (st->hit_count >= MAX_HIT_RECTS) return;
    HitRect *r = &st->hits[st->hit_count++];
    r->x = x; r->y = y; r->w = w; r->h = h;
    r->action_id = action;
    r->arg0 = arg0;
    r->arg1 = arg1;
}

void nature_add_text_rect(NatureOSState *st, int x, int y, int w, int h, int target_id) {
    if (st->text_rect_count >= MAX_TEXT_RECTS) return;
    TextInputRect *r = &st->text_rects[st->text_rect_count++];
    r->x = x; r->y = y; r->w = w; r->h = h;
    r->target_id = target_id;
}

void nature_occlude_text_rects(NatureOSState *st, int x, int y, int w, int h) {
    int valid = 0;
    for (int i = 0; i < st->text_rect_count; ++i) {
        TextInputRect *r = &st->text_rects[i];
        int ix0 = r->x > x ? r->x : x;
        int iy0 = r->y > y ? r->y : y;
        int ix1 = (r->x + r->w) < (x + w) ? (r->x + r->w) : (x + w);
        int iy1 = (r->y + r->h) < (y + h) ? (r->y + r->h) : (y + h);
        if (ix1 > ix0 && iy1 > iy0) {
            continue;
        }
        if (valid != i) st->text_rects[valid] = *r;
        valid++;
    }
    st->text_rect_count = valid;
}

void nature_draw_scrollbar(NatureOSState *st, cairo_t *cr, int x, int y, int h,
                           int total_items, int visible_items, int scroll_pos,
                           int max_scroll, int target_kind, int dark) {
    if (h < 32 || max_scroll <= 0 || total_items <= visible_items) return;

    NaturePalette pal;
    nature_get_palette(st, &pal);
    double ar = pal.ar, ag = pal.ag, ab = pal.ab;

    int track_x = x;
    int track_y = y + 4;
    int track_h = h - 8;
    int track_w = 7;

    int hov = (st->hover_mx >= track_x - 8 && st->hover_mx <= track_x + 12 &&
               st->hover_my >= track_y && st->hover_my <= track_y + track_h) ||
              (st->drag_mode == 3 && st->scroll_drag_target == target_kind);

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

    double visual_frac = (target_kind == SCROLL_TERMINAL) ? (1.0 - frac) : frac;
    int thumb_y = track_y + (int)((track_h - thumb_h) * visual_frac);

    if (hov) {
        ngfx_fill_rect(cr, track_x - 1, thumb_y, track_w + 2, thumb_h, 4.5, ar, ag, ab, 0.92);
    } else {
        ngfx_fill_rect(cr, track_x, thumb_y, track_w, thumb_h, 3.5,
                       pal.mut_r, pal.mut_g, pal.mut_b, 0.55);
    }

    nature_add_hit(st, track_x - 7, track_y, 18, track_h, HIT_SCROLLBAR, target_kind, max_scroll);
}

void nature_os_set_wallpaper_preset(NatureOSState *st, int idx) {
    if (!st) return;
    st->cfg.wallpaper_idx = idx % MAX_WALLPAPERS;
    st->custom_wp_media_idx = -1;
    st->cached_wp_idx = -1;
    nature_os_save_state(st);
    st->dirty = 1;
}

void nature_os_set_wallpaper_from_media(NatureOSState *st, int media_idx) {
    if (!st || media_idx < 0 || media_idx >= MAX_MEDIA_IMAGES) return;
    st->custom_wp_media_idx = media_idx;
    st->cached_wp_custom = -1;
    nature_os_save_state(st);
    st->dirty = 1;
}

static cairo_surface_t *load_png(const char *asset_dir, const char *filename) {
    char full[256];
    snprintf(full, sizeof(full), "%s/%s", asset_dir, filename);
    return g_ngfx.image_surface_create_from_png(full);
}

static void add_vfs(NatureOSState *st, const char *name, const char *path,
                    const char *parent, int is_dir, int size_bytes,
                    int img_idx, int audio_idx, int vid_idx, const char *mime_tag,
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
    n->video_idx = vid_idx;
    snprintf(n->mime_tag, sizeof(n->mime_tag), "%s", mime_tag ? mime_tag : (is_dir ? "DIR" : "TXT"));
    snprintf(n->modified, sizeof(n->modified), "%s", modified ? modified : "Nov 25 2026");
    if (content) snprintf(n->content, sizeof(n->content), "%s", content);
}

/* --- Subsystem Initialization --- */

void nature_os_init(NatureOSState *st, const char *asset_dir) {
    memset(st, 0, sizeof(*st));
    st->screen_w = OS_WIDTH;
    st->screen_h = OS_HEIGHT;
    st->cfg.theme_family = THEME_NATURE;
    st->cfg.dark_mode = 1;
    st->cfg.wallpaper_idx = 0;
    st->cfg.accent_idx = 0;
    st->cfg.dock_position = 0;
    st->cfg.dock_size = 1;
    st->cfg.animations_enabled = 1;
    st->cfg.show_panel_stats = 1;
    st->cfg.telemetry_ms = 500;
    st->cfg.auto_lock_min = 0;
    st->cfg.low_power_mode = 0;
    st->cfg.zram_enabled = 1;
    st->cfg.wifi_enabled = 1;
    st->cfg.bluetooth_enabled = 1;
    st->cfg.night_light = 0;
    st->cfg.night_light_warmth = 65;
    st->cfg.audio_muted = 0;
    st->cfg.audio_device = 0;
    st->cfg.target_hz = 60;
    st->cfg.volume = 76;
    st->cfg.brightness = 100;
    st->cfg.power_profile = 1;
    st->cfg.clock_24h = 1;
    st->cfg.show_seconds = 0;
    st->cfg.window_snap = 1;
    st->cfg.show_desktop_widgets = 1;
    st->cfg.show_toast_popups = 1;
    st->cfg.term_font_size = 12;
    st->cfg.compact_chrome = 0;
    st->cfg.show_dock_labels = 1;
    snprintf(st->cfg.device_name, sizeof(st->cfg.device_name), "nature-pc");
    snprintf(st->settings_hostname_draft, sizeof(st->settings_hostname_draft), "nature-pc");

    st->active_workspace = 0;
    st->next_z = 10;
    st->next_win_id = 1;
    st->next_pid = 1200;
    st->hover_dock_idx = -1;
    st->last_title_click_win = -1;
    st->custom_wp_media_idx = -1;
    st->cached_wp_family = -1;
    st->cached_wp_custom = -1;
    st->files_selected_idx = -1;
    st->files_confirm_delete_idx = -1;
    st->files_show_hidden = 1;
    st->settings_net_ping_ms = -1;
    st->cal_month_offset = 0;
    st->media_selected = 0;
    st->media_zoom_pct = 100;
    st->media_sidebar_open = 1;
    st->media_folder_idx = 0;
    st->video_speed_x10 = 10;
    st->video_hud = 1;
    st->audio_speed_x10 = 10;
    st->svc_pipewire = 1;
    st->svc_network = 1;
    st->svc_oomd = 1;
    snprintf(st->term_env_keys[0], sizeof(st->term_env_keys[0]), "NODE_ENV");
    snprintf(st->term_env_vals[0], sizeof(st->term_env_vals[0]), "production");
    snprintf(st->term_env_keys[1], sizeof(st->term_env_keys[1]), "EDITOR");
    snprintf(st->term_env_vals[1], sizeof(st->term_env_vals[1]), "nano");
    st->term_env_count = 2;
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
    st->wallpapers[3] = load_png(dir, "wallpaper_horizon_day.png");
    st->wallpapers[4] = load_png(dir, "wallpaper_horizon_night.png");
    st->wallpapers[5] = load_png(dir, "media_alpine_dawn.png");
    st->wallpapers[6] = load_png(dir, "media_nordic_trail.png");
    st->wallpapers[7] = NULL; /* Procedural Horizon Slate */

    st->media_images[0] = load_png(dir, "media_fern.png");
    st->media_images[1] = st->wallpapers[0];
    st->media_images[2] = st->wallpapers[1];
    st->media_images[3] = st->wallpapers[2];
    st->media_images[4] = st->wallpapers[3];
    st->media_images[5] = st->wallpapers[4];
    st->media_images[6] = st->wallpapers[5];
    st->media_images[7] = st->wallpapers[6];
    st->media_images[8] = load_png(dir, "media_scandinavian.png");
    st->media_images[9] = create_aurora_art();
    st->media_images[10] = create_canopy_rings_art();
    st->avatar_surface = load_png(dir, "avatar_user.png");

    snprintf(st->files_cwd, sizeof(st->files_cwd), "/home/nature");
    snprintf(st->term_cwd, sizeof(st->term_cwd), "/home/nature");
    snprintf(st->term_prev_cwd, sizeof(st->term_prev_cwd), "/home/nature");

    /* Default Virtual File System Seeding */
    add_vfs(st, "/", "/", "", 1, 4096, -1, -1, -1, "DIR", "Nov 25 2026", NULL);
    add_vfs(st, "home", "/home", "/", 1, 4096, -1, -1, -1, "DIR", "Nov 25 2026", NULL);
    add_vfs(st, "nature", "/home/nature", "/home", 1, 4096, -1, -1, -1, "DIR", "Nov 25 2026", NULL);
    add_vfs(st, "Desktop", "/home/nature/Desktop", "/home/nature", 1, 4096, -1, -1, -1, "DIR", "Nov 25 2026", NULL);
    add_vfs(st, "Documents", "/home/nature/Documents", "/home/nature", 1, 4096, -1, -1, -1, "DIR", "Nov 25 2026", NULL);
    add_vfs(st, "Downloads", "/home/nature/Downloads", "/home/nature", 1, 4096, -1, -1, -1, "DIR", "Nov 25 2026", NULL);
    add_vfs(st, "Music", "/home/nature/Music", "/home/nature", 1, 4096, -1, -1, -1, "DIR", "Nov 25 2026", NULL);
    add_vfs(st, "Pictures", "/home/nature/Pictures", "/home/nature", 1, 4096, -1, -1, -1, "DIR", "Nov 25 2026", NULL);
    add_vfs(st, "Videos", "/home/nature/Videos", "/home/nature", 1, 4096, -1, -1, -1, "DIR", "Nov 25 2026", NULL);
    add_vfs(st, "Projects", "/home/nature/Projects", "/home/nature", 1, 4096, -1, -1, -1, "DIR", "Nov 25 2026", NULL);
    add_vfs(st, "etc", "/etc", "/", 1, 4096, -1, -1, -1, "DIR", "Nov 25 2026", NULL);
    add_vfs(st, "nature-os", "/etc/nature-os", "/etc", 1, 4096, -1, -1, -1, "DIR", "Nov 25 2026", NULL);
    add_vfs(st, "var", "/var", "/", 1, 4096, -1, -1, -1, "DIR", "Nov 25 2026", NULL);
    add_vfs(st, "log", "/var/log", "/var", 1, 4096, -1, -1, -1, "DIR", "Nov 25 2026", NULL);

    add_vfs(st, "welcome.txt", "/home/nature/welcome.txt", "/home/nature", 0, 340, -1, -1, -1, "TXT", "Nov 25 2026",
            "Welcome to Nature OS 24.04.1 LTS (Noble Numbat).\nEngineered in C17 & C++20 for minimal RAM/CPU overhead.\nRight-click desktop for quick actions.");
    add_vfs(st, ".bashrc", "/home/nature/.bashrc", "/home/nature", 0, 412, -1, -1, -1, "CONF", "Nov 24 2026",
            "# Nature OS Ubuntu Shell Profile\nexport NATURE_COMPOSITOR=wlroots\nexport EDITOR=nano\nalias ll='ls -la'");

    add_vfs(st, "QuickStart.md", "/home/nature/Desktop/QuickStart.md", "/home/nature/Desktop", 0, 512, -1, -1, -1, "MD", "Nov 25 2026",
            "# Quick Start Guide\n- Super key opens Activities & App Search\n- Drag window edges to snap left/right\n- Drag scrollbars or swipe to scroll inside any app");
    add_vfs(st, "nature-manual.txt", "/home/nature/Desktop/nature-manual.txt", "/home/nature/Desktop", 0, 420, -1, -1, -1, "TXT", "Nov 25 2026",
            "Nature OS Shortcuts:\n- Super / Activities: App Overview\n- Right-Click Desktop: Context Menu\n- Media Viewer: Photos, Videos & PipeWire audio");

    add_vfs(st, "os-release", "/home/nature/Documents/os-release", "/home/nature/Documents", 0, 290, -1, -1, -1, "CONF", "Nov 25 2026",
            "NAME=\"Nature OS\"\nVERSION=\"24.04.1 LTS (Canopy)\"\nID=natureos\nID_LIKE=ubuntu debian\nUBUNTU_CODENAME=noble");
    add_vfs(st, "zram-tuning.txt", "/home/nature/Documents/zram-tuning.txt", "/home/nature/Documents", 0, 412, -1, -1, -1, "TXT", "Nov 25 2026",
            "zram0 algorithm=lz4\nvm.swappiness=120\nidle_rss=304MiB\ncompositor=wlroots-damage");
    add_vfs(st, "architecture-notes.md", "/home/nature/Documents/architecture-notes.md", "/home/nature/Documents", 0, 680, -1, -1, -1, "MD", "Nov 25 2026",
            "# Nature OS Stack\n1. C17 wlroots/Cairo damage compositor\n2. C++20 System Monitor & Ubuntu Store\n3. systemd cgroup v2 CPUWeight=300 UI shield");
    add_vfs(st, "ubuntu-packages.list", "/home/nature/Documents/ubuntu-packages.list", "/home/nature/Documents", 0, 520, -1, -1, -1, "TXT", "Nov 23 2026",
            "linux-image-6.8.0-45-lowlatency\nnature-compositor\nnature-shell\npipewire-audio\nnetwork-manager\nzram-tools");
    add_vfs(st, "wayland-protocol.md", "/home/nature/Documents/wayland-protocol.md", "/home/nature/Documents", 0, 610, -1, -1, -1, "MD", "Nov 22 2026",
            "xdg-shell surface scaling & damage tracking notes.\nAdaptive output sizing supports portrait, square, and ultrawide.");

    add_vfs(st, "woodland-fern.jpg", "/home/nature/Pictures/woodland-fern.jpg", "/home/nature/Pictures", 0, 3565158, 0, -1, -1, "IMG", "Nov 25 2026",
            "Leica M11 35mm · f/2.0 · 2400x1800\nTap twice to open in Media Viewer.");
    add_vfs(st, "canopy-dawn.jpg", "/home/nature/Pictures/canopy-dawn.jpg", "/home/nature/Pictures", 0, 5033164, 1, -1, -1, "IMG", "Nov 25 2026",
            "Hasselblad X2D 100C · 3840x2160 4K HDR");
    add_vfs(st, "alpine-twilight.jpg", "/home/nature/Pictures/alpine-twilight.jpg", "/home/nature/Pictures", 0, 4404019, 2, -1, -1, "IMG", "Nov 25 2026",
            "Sony A7R V · 3840x2160 4K");
    add_vfs(st, "basalt-moss.jpg", "/home/nature/Pictures/basalt-moss.jpg", "/home/nature/Pictures", 0, 4089446, 3, -1, -1, "IMG", "Nov 25 2026",
            "Fujifilm GFX 100S · Macro 80mm");
    add_vfs(st, "horizon-day.jpg", "/home/nature/Pictures/horizon-day.jpg", "/home/nature/Pictures", 0, 4620000, 4, -1, -1, "IMG", "Nov 25 2026",
            "Nordic Alpine Valley Daylight · 4K HDR");
    add_vfs(st, "horizon-night.jpg", "/home/nature/Pictures/horizon-night.jpg", "/home/nature/Pictures", 0, 4310000, 5, -1, -1, "IMG", "Nov 25 2026",
            "Starlit Ridge & Midnight Sky · 4K HDR");
    add_vfs(st, "alpine-dawn.jpg", "/home/nature/Pictures/alpine-dawn.jpg", "/home/nature/Pictures", 0, 4890000, 6, -1, -1, "IMG", "Nov 25 2026",
            "Alpine Sunrise Ridge · 28mm f/4.0");
    add_vfs(st, "nordic-trail.jpg", "/home/nature/Pictures/nordic-trail.jpg", "/home/nature/Pictures", 0, 5120000, 7, -1, -1, "IMG", "Nov 25 2026",
            "Nordic Conifer Moss Trail · 35mm");
    add_vfs(st, "scandinavian-courtyard.jpg", "/home/nature/Pictures/scandinavian-courtyard.jpg", "/home/nature/Pictures", 0, 4210000, 8, -1, -1, "IMG", "Nov 25 2026",
            "Scandinavian Architectural Courtyard");
    add_vfs(st, "aurora-horizon.png", "/home/nature/Pictures/aurora-horizon.png", "/home/nature/Pictures", 0, 2890120, 9, -1, -1, "IMG", "Nov 25 2026",
            "Procedural Aurora Horizon · 4K HDR Canvas");
    add_vfs(st, "canopy-rings.png", "/home/nature/Pictures/canopy-rings.png", "/home/nature/Pictures", 0, 1984320, 10, -1, -1, "IMG", "Nov 25 2026",
            "Verdant Botanical Rings Vector Render");

    add_vfs(st, "alpine-dawn-timelapse.mp4", "/home/nature/Videos/alpine-dawn-timelapse.mp4", "/home/nature/Videos", 0, 148200000, 6, -1, 0, "VID", "Nov 25 2026",
            "4K 60fps VA-API H.265 Timelapse · Duration 00:32 · Double-click to play.");
    add_vfs(st, "canopy-rain-stream.mp4", "/home/nature/Videos/canopy-rain-stream.mp4", "/home/nature/Videos", 0, 112400000, 1, -1, 1, "VID", "Nov 25 2026",
            "1440p 60fps AV1 Rain Forest Stream · Duration 00:28 · Double-click to play.");
    add_vfs(st, "aurora-horizon-hdr.mp4", "/home/nature/Videos/aurora-horizon-hdr.mp4", "/home/nature/Videos", 0, 184600000, 9, -1, 2, "VID", "Nov 24 2026",
            "4K 60fps HDR10 Polar Aurora · Duration 00:36 · Double-click to play.");
    add_vfs(st, "compositor-bench-60fps.mp4", "/home/nature/Videos/compositor-bench-60fps.mp4", "/home/nature/Videos", 0, 74500000, 4, -1, 3, "VID", "Nov 25 2026",
            "1080p 120fps Cairo Shader Motion · Duration 00:24 · Double-click to play.");

    add_vfs(st, "rainforest-canopy.flac", "/home/nature/Music/rainforest-canopy.flac", "/home/nature/Music", 0, 38402000, -1, 0, -1, "AUD", "Nov 25 2026",
            "Artist: Nature Soundscapes\nFormat: 24-bit / 96 kHz FLAC\nDuration: 03:42 · Tap Open in Media to play.");
    add_vfs(st, "alpine-wind.ogg", "/home/nature/Music/alpine-wind.ogg", "/home/nature/Music", 0, 8940000, -1, 1, -1, "AUD", "Nov 24 2026",
            "Artist: Nordic Ambient Studio\nFormat: 48 kHz Opus/OGG\nDuration: 04:18");
    add_vfs(st, "night-moss-stream.wav", "/home/nature/Music/night-moss-stream.wav", "/home/nature/Music", 0, 31200000, -1, 2, -1, "AUD", "Nov 23 2026",
            "Artist: Olympic Field Archive\nFormat: 48 kHz WAV\nDuration: 02:55");
    add_vfs(st, "horizon-startup-chime.ogg", "/home/nature/Music/horizon-startup-chime.ogg", "/home/nature/Music", 0, 4820000, -1, 3, -1, "AUD", "Nov 25 2026",
            "Artist: Nature Audio Lab\nFormat: 48 kHz OGG Spatial Mix\nDuration: 01:34");

    add_vfs(st, "nature-os-24.04.1-amd64.iso", "/home/nature/Downloads/nature-os-24.04.1-amd64.iso", "/home/nature/Downloads", 0, 1524713390, -1, -1, -1, "ISO", "Nov 25 2026",
            "SHA256: 9f4e812c08e5a7f902...\nBootable Ubuntu 24.04 LTS + Nature OS image.");
    add_vfs(st, "mesa-vulkan-24.0.9.deb", "/home/nature/Downloads/mesa-vulkan-24.0.9.deb", "/home/nature/Downloads", 0, 50541200, -1, -1, -1, "DEB", "Nov 24 2026",
            "Package: mesa-vulkan-drivers\nArchitecture: amd64\nStatus: verified signature");
    add_vfs(st, "jetbrains-mono-fonts.tar.xz", "/home/nature/Downloads/jetbrains-mono-fonts.tar.xz", "/home/nature/Downloads", 0, 23488102, -1, -1, -1, "TXT", "Nov 22 2026",
            "Compressed system monospace font archive.");

    add_vfs(st, "render_scheduler.c", "/home/nature/Projects/render_scheduler.c", "/home/nature/Projects", 0, 1240, -1, -1, -1, "C", "Nov 25 2026",
            "void nature_scheduler_init(struct nature_scheduler *s, uint32_t hz) {\n  s->target_hz = hz;\n  s->input_priority_boost = true;\n}");
    add_vfs(st, "fb_transport.cpp", "/home/nature/Projects/fb_transport.cpp", "/home/nature/Projects", 0, 4820, -1, -1, -1, "CPP", "Nov 25 2026",
            "// Single-pass NAT3 zlib dirty-tile framebuffer transport\ng_wake_cv.notify_one(); // zero-latency input wake");
    add_vfs(st, "meson.build", "/home/nature/Projects/meson.build", "/home/nature/Projects", 0, 920, -1, -1, -1, "CONF", "Nov 24 2026",
            "project('nature-os', ['c', 'cpp'], version: '24.04.1')\ndep_cairo = dependency('cairo')");
    add_vfs(st, "package.json", "/home/nature/Projects/package.json", "/home/nature/Projects", 0, 318, -1, -1, -1, "JSON", "Nov 25 2026",
            "{\n  \"name\": \"nature-cloud-service\",\n  \"version\": \"1.4.0\",\n  \"main\": \"server.js\",\n  \"scripts\": {\n    \"start\": \"node server.js\",\n    \"test\": \"node -e console.log('ok')\"\n  }\n}");
    add_vfs(st, "server.js", "/home/nature/Projects/server.js", "/home/nature/Projects", 0, 412, -1, -1, -1, "JS", "Nov 25 2026",
            "const os = require('os');\nconst port = 3000;\nconsole.log('Nature Node.js', process.version, 'on', os.hostname());\nconsole.log('CPUs:', os.cpus().length, 'RAM MiB:', Math.round(os.totalmem()/1048576));\nconsole.log('Listening on http://localhost:' + port);");
    add_vfs(st, "hello.py", "/home/nature/Projects/hello.py", "/home/nature/Projects", 0, 198, -1, -1, -1, "PY", "Nov 25 2026",
            "import platform\nprint('Nature OS Python 3.12 ready on', platform.node())");
    add_vfs(st, "README.md", "/home/nature/Projects/README.md", "/home/nature/Projects", 0, 360, -1, -1, -1, "MD", "Nov 25 2026",
            "# Nature Projects\nRun 'node ~/Projects/server.js', 'npm run start', or 'python3 ~/Projects/hello.py' in Terminal.");

    add_vfs(st, "compositor.conf", "/etc/nature-os/compositor.conf", "/etc/nature-os", 0, 296, -1, -1, -1, "CONF", "Nov 25 2026",
            "[compositor]\nbackend=drm-kms\ndamage_tracking=full\nzero_copy_dmabuf=true\nadaptive_scaling=true");
    add_vfs(st, "zram.conf", "/etc/nature-os/zram.conf", "/etc/nature-os", 0, 184, -1, -1, -1, "CONF", "Nov 25 2026",
            "[zram0]\nzram-size = ram / 2\ncompression-algorithm = lz4\nswap-priority = 100");

    add_vfs(st, "nature-compositor.log", "/var/log/nature-compositor.log", "/var/log", 0, 1820, -1, -1, -1, "LOG", "Nov 25 2026",
            "[0.042] DRM/KMS adaptive mode bound\n[0.088] Cairo + zlib NAT3 pipeline initialized\n[0.145] nature-shell ready (304 MiB RSS)");
    add_vfs(st, "syslog", "/var/log/syslog", "/var/log", 0, 4210, -1, -1, -1, "LOG", "Nov 25 2026",
            "Nov 25 systemd[1]: Started nature-session.service.\nNov 25 NetworkManager: wlp2s0 link up 866Mbps\nNov 25 pipewire: Quantum 1024/48000 active");

    st->vfs_stock_count = st->vfs_count;

    snprintf(st->term_lines[0], sizeof(st->term_lines[0]),
             "Nature OS 24.04.1 LTS (Ubuntu Noble) -- C17/C++20 Compositor + Node v22.12");
    st->term_line_colors[0] = 2;
    snprintf(st->term_lines[1], sizeof(st->term_lines[1]),
             "Try 'help', 'neofetch', 'node -v', 'npm run start', 'python3 -V', 'git status'");
    st->term_line_colors[1] = 0;
    st->term_line_count = 2;
    st->term_hist_idx = -1;

    nature_monitor_init(st);
    nature_store_init(st);

    OSWindow *mon = nature_open_app(st, APP_MONITOR);
    if (mon) { mon->x = 46; mon->y = 54; mon->w = 590; mon->h = 436; clamp_window_to_screen(st, mon); }
    OSWindow *term = nature_open_app(st, APP_TERMINAL);
    if (term) { term->x = 656; term->y = 84; term->w = 576; term->h = 416; clamp_window_to_screen(st, term); }

    if (asset_dir && *asset_dir) {
        nature_os_load_state(st);
    }
}

/* --- Main OS Desktop Render Loop --- */

void nature_os_render(NatureOSState *st, cairo_t *cr) {
    st->hit_count = 0;
    st->text_rect_count = 0;

    int SW = st->screen_w > 0 ? st->screen_w : OS_WIDTH;
    int SH = st->screen_h > 0 ? st->screen_h : OS_HEIGHT;

    NaturePalette pal;
    nature_get_palette(st, &pal);
    int dark = pal.dark_mode;
    double ar = pal.ar, ag = pal.ag, ab = pal.ab;

    /* 1. Fast Cached Aspect-Cover Wallpaper Blit (<0.3ms per frame!) */
    ensure_cached_wallpaper(st, SW, SH);
    if (st->cached_wp) {
        g_ngfx.set_source_surface(cr, st->cached_wp, 0, 0);
        g_ngfx.paint(cr);
    }

    time_t now = time(NULL);
    struct tm tm_buf;
    const struct tm *tm_info = localtime_r(&now, &tm_buf);
    if (!tm_info) tm_info = &tm_buf;

    /* 1b. Ambient Desktop Clock & Shortcut Cards */
    if (st->cfg.show_desktop_widgets && !st->is_locked) {
        char time_big[24];
        char date_big[64];
        format_clock_str(st, tm_info, 0, time_big, sizeof(time_big));
        strftime(date_big, sizeof(date_big), "%A, %B %d", tm_info);

        double clk_sz = SW < 560 ? 44.0 : 58.0;
        int clk_y = SW < 580 ? 114 : (SH < 540 ? 120 : 164);
        ngfx_radial_glow(cr, SW * 0.5, clk_y - 8, 12.0, 170.0, 0.02, 0.04, 0.03, dark ? 0.34 : 0.14, 0.0);
        double tb_w = ngfx_text_width(cr, "Plus Jakarta Sans", 1, clk_sz, time_big);
        ngfx_text(cr, "Plus Jakarta Sans", 1, clk_sz, (SW - tb_w) * 0.5, clk_y + 2.0, 0.0, 0.0, 0.0, dark ? 0.38 : 0.18, time_big);
        ngfx_text(cr, "Plus Jakarta Sans", 1, clk_sz, (SW - tb_w) * 0.5, clk_y,
                  dark ? 1.0 : 0.06, dark ? 1.0 : 0.10, dark ? 1.0 : 0.16, 0.96, time_big);

        double db_w = ngfx_text_width(cr, "Plus Jakarta Sans", 0, 14.5, date_big);
        ngfx_text(cr, "Plus Jakarta Sans", 0, 14.5, (SW - db_w) * 0.5, clk_y + 27, 0.0, 0.0, 0.0, dark ? 0.36 : 0.14, date_big);
        ngfx_text(cr, "Plus Jakarta Sans", 0, 14.5, (SW - db_w) * 0.5, clk_y + 26,
                  dark ? 0.92 : 0.10, dark ? 0.97 : 0.15, dark ? 0.94 : 0.22, 0.90, date_big);

        char sys_stat[112];
        const char *host = st->cfg.device_name[0] ? st->cfg.device_name : "nature-pc";
        snprintf(sys_stat, sizeof(sys_stat), "Nature OS · %s  ·  %dx%d  ·  %d/%d MiB  ·  %.1f ms",
                 host, SW, SH, st->used_ram_mb, st->total_ram_mb, st->frame_ms);
        double sb_w = ngfx_text_width(cr, "JetBrains Mono", 0, 10.2, sys_stat);
        int pill_w = (int)sb_w + 32;
        if (pill_w > SW - 24) pill_w = SW - 24;
        int pill_x = (SW - pill_w) / 2;
        int pill_y = clk_y + 38;
        int pill_hov = (st->hover_mx >= pill_x && st->hover_mx <= pill_x + pill_w && st->hover_my >= pill_y && st->hover_my <= pill_y + 22);
        ngfx_fill_rect(cr, pill_x, pill_y, pill_w, 22, 11, pal.panel_top_r, pal.panel_top_g, pal.panel_top_b, pill_hov ? 0.88 : (dark ? 0.62 : 0.78));
        ngfx_stroke_rect(cr, pill_x + 0.5, pill_y + 0.5, pill_w - 1, 21, 11, pill_hov ? 1.0 : 0.8,
                         pill_hov ? ar : pal.strong_brd_r, pill_hov ? ag : pal.strong_brd_g, pill_hov ? ab : pal.strong_brd_b,
                         pill_hov ? 0.78 : pal.strong_brd_a * 0.85);
        ngfx_fill_circle(cr, pill_x + 12, pill_y + 11, 3.0, pal.ar_hi, pal.ag_hi, pal.ab_hi, 0.96);
        ngfx_text_clipped(cr, "JetBrains Mono", 0, 10.2, pill_x + 21, pill_y + 14.5, pill_w - 28,
                          pal.fg_r, pal.fg_g, pal.fg_b, 0.92, sys_stat);
        nature_add_hit(st, pill_x, pill_y, pill_w, 22, HIT_LAUNCHER_APP, APP_MONITOR, 0);

        if (st->audio_playing && SW >= 580 && SH >= 500) {
            int np_y = pill_y + 28;
            char np_txt[64];
            snprintf(np_txt, sizeof(np_txt), "Playing Track %d · PipeWire", (st->audio_selected % MAX_AUDIO_TRACKS) + 1);
            int np_w = (int)ngfx_text_width(cr, "JetBrains Mono", 0, 9.6, np_txt) + 30;
            int np_x = (SW - np_w) / 2;
            int np_hov = (st->hover_mx >= np_x && st->hover_mx <= np_x + np_w && st->hover_my >= np_y && st->hover_my <= np_y + 20);
            ngfx_fill_rect(cr, np_x, np_y, np_w, 20, 10, pal.panel_top_r, pal.panel_top_g, pal.panel_top_b, np_hov ? 0.88 : (dark ? 0.56 : 0.72));
            ngfx_stroke_rect(cr, np_x + 0.5, np_y + 0.5, np_w - 1, 19, 10, 0.8, np_hov ? ar : pal.border_r, np_hov ? ag : pal.border_g, np_hov ? ab : pal.border_b, pal.border_a);
            ngfx_fill_circle(cr, np_x + 11, np_y + 10, 2.8, pal.success_r, pal.success_g, pal.success_b, 0.98);
            ngfx_text(cr, "JetBrains Mono", 0, 9.6, np_x + 19, np_y + 13.5, pal.fg_sec_r, pal.fg_sec_g, pal.fg_sec_b, 0.95, np_txt);
            nature_add_hit(st, np_x, np_y, np_w, 20, HIT_LAUNCHER_APP, APP_MEDIA, 0);
        }

        /* Desktop Shortcut Cards */
        const AppType desk_apps[4] = {APP_FILES, APP_STORE, APP_BROWSER, APP_TERMINAL};
        const char *desk_labels[4] = {"Files", "Software", "Browser", "Terminal"};
        if (SW < 580) {
            int row_w = 3 * 78 + 2 * 12;
            int start_x = (SW - row_w) / 2;
            int dy = clk_y + 82;
            for (int i = 0; i < 3; ++i) {
                if (dy + 82 > SH - 84) break;
                int dx = start_x + i * 90;
                int hov = (st->hover_mx >= dx && st->hover_mx <= dx + 78 && st->hover_my >= dy && st->hover_my <= dy + 82);
                if (hov) ngfx_radial_glow(cr, dx + 39, dy + 41, 10.0, 56.0, ar, ag, ab, 0.25, 0.0);
                ngfx_shadow_rect(cr, dx, dy, 78, 82, 15, hov ? 3.5 : 2.2, dark ? (hov ? 0.36 : 0.24) : 0.16);
                ngfx_fill_gradient(cr, dx, dy, 78, 82, 15,
                                   pal.dock_top_r, pal.dock_top_g, pal.dock_top_b, hov ? 0.88 : 0.68,
                                   pal.dock_bot_r, pal.dock_bot_g, pal.dock_bot_b, hov ? 0.82 : 0.62, 0);
                ngfx_stroke_rect(cr, dx + 0.5, dy + 0.5, 77, 81, 15, 1.0,
                                 hov ? ar : pal.strong_brd_r, hov ? ag : pal.strong_brd_g, hov ? ab : pal.strong_brd_b, hov ? 0.78 : pal.strong_brd_a);
                ngfx_top_specular(cr, dx, dy, 78, 15, hov ? 0.30 : 0.16);
                draw_app_icon(cr, desk_apps[i], dx + 18, dy + 10, 42);
                double l_w = ngfx_text_width(cr, "Noto Sans", 1, 11.0, desk_labels[i]);
                ngfx_text(cr, "Noto Sans", 1, 11.0, dx + (78 - l_w) * 0.5, dy + 70, pal.fg_r, pal.fg_g, pal.fg_b, 0.96, desk_labels[i]);
                nature_add_hit(st, dx, dy, 78, 82, HIT_DESKTOP_ICON, desk_apps[i], 0);
            }
        } else {
            int desk_x = st->cfg.dock_position == 1 ? (SW < 640 ? 68 : 88) : 20;
            for (int i = 0; i < 4; ++i) {
                int dy = 60 + i * 92;
                if (dy + 84 > SH - 84) break;
                int hov = (st->hover_mx >= desk_x && st->hover_mx <= desk_x + 76 && st->hover_my >= dy && st->hover_my <= dy + 82);
                if (hov) ngfx_radial_glow(cr, desk_x + 38, dy + 41, 10.0, 56.0, ar, ag, ab, 0.25, 0.0);
                ngfx_shadow_rect(cr, desk_x, dy, 76, 82, 15, hov ? 3.5 : 2.2, dark ? (hov ? 0.36 : 0.24) : 0.16);
                ngfx_fill_gradient(cr, desk_x, dy, 76, 82, 15,
                                   pal.dock_top_r, pal.dock_top_g, pal.dock_top_b, hov ? 0.88 : 0.66,
                                   pal.dock_bot_r, pal.dock_bot_g, pal.dock_bot_b, hov ? 0.82 : 0.60, 0);
                ngfx_stroke_rect(cr, desk_x + 0.5, dy + 0.5, 75, 81, 15, 1.0,
                                 hov ? ar : pal.strong_brd_r, hov ? ag : pal.strong_brd_g, hov ? ab : pal.strong_brd_b, hov ? 0.78 : pal.strong_brd_a);
                ngfx_top_specular(cr, desk_x, dy, 76, 15, hov ? 0.30 : 0.16);
                draw_app_icon(cr, desk_apps[i], desk_x + 17, dy + 10, 42);
                double l_w = ngfx_text_width(cr, "Noto Sans", 1, 11.0, desk_labels[i]);
                ngfx_text(cr, "Noto Sans", 1, 11.0, desk_x + (76 - l_w) * 0.5, dy + 70, pal.fg_r, pal.fg_g, pal.fg_b, 0.96, desk_labels[i]);
                nature_add_hit(st, desk_x, dy, 76, 82, HIT_DESKTOP_ICON, desk_apps[i], 0);
            }
        }
    }

    /* 3. Window Snap Preview Overlay (Luminous Acrylic Aura) */
    int dsz_extra = (st->cfg.dock_size == 0) ? -8 : (st->cfg.dock_size == 2 ? 8 : 0);
    int snap_left = st->cfg.dock_position == 1 ? (76 + dsz_extra) : 12;
    int snap_bot = st->cfg.dock_position == 0 ? (78 + dsz_extra) : 12;
    int snap_half = (SW - snap_left - 18) / 2;
    if (st->snap_preview > 0) {
        int sx = snap_left, sy = 46, sw_p = snap_half, sh_p = SH - 46 - snap_bot;
        if (st->snap_preview == 2) sx = snap_left + snap_half + 8;
        else if (st->snap_preview == 3) sw_p = SW - snap_left - 10;

        /* Multi-layered acrylic glow */
        ngfx_shadow_rect(cr, sx, sy, sw_p, sh_p, 16, 6.0, dark ? 0.38 : 0.22);
        ngfx_fill_gradient(cr, sx, sy, sw_p, sh_p, 16,
                           ar, ag, ab, 0.26,
                           ar, ag, ab, 0.10, 0);
        ngfx_stroke_rect(cr, sx + 0.5, sy + 0.5, sw_p - 1, sh_p - 1, 16, 2.0,
                         pal.ar_hi, pal.ag_hi, pal.ab_hi, 0.92);
        ngfx_stroke_rect(cr, sx + 2.5, sy + 2.5, sw_p - 5, sh_p - 5, 14, 1.0,
                         1.0, 1.0, 1.0, dark ? 0.18 : 0.40);
        ngfx_top_specular(cr, sx, sy, sw_p, 16, dark ? 0.35 : 0.70);

        /* Center acrylic pill indicator */
        const char *snap_tag = (st->snap_preview == 1) ? "Snap Left (Tile 50%)"
                             : ((st->snap_preview == 2) ? "Snap Right (Tile 50%)" : "Maximize (Fill Display)");
        double tag_w = ngfx_text_width(cr, "Noto Sans", 1, 11.5, snap_tag);
        int tag_box_w = (int)tag_w + 34;
        int tag_box_x = sx + (sw_p - tag_box_w) / 2;
        int tag_box_y = sy + (sh_p - 30) / 2;
        ngfx_shadow_rect(cr, tag_box_x, tag_box_y, tag_box_w, 30, 15, 3.0, 0.35);
        ngfx_fill_rect(cr, tag_box_x, tag_box_y, tag_box_w, 30, 15, pal.elev_r, pal.elev_g, pal.elev_b, 0.95);
        ngfx_stroke_rect(cr, tag_box_x + 0.5, tag_box_y + 0.5, tag_box_w - 1, 29, 15, 1.0,
                         pal.border_r, pal.border_g, pal.border_b, pal.border_a);
        ngfx_fill_circle(cr, tag_box_x + 15, tag_box_y + 15, 3.5, pal.ar_hi, pal.ag_hi, pal.ab_hi, 1.0);
        ngfx_text(cr, "Noto Sans", 1, 11.5, tag_box_x + 25, tag_box_y + 19, pal.fg_r, pal.fg_g, pal.fg_b, 1.0, snap_tag);
    }

    /* 4. Sort and Render Windows in Z-Order */
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
        render_window(st, cr, w, &pal);
    }

    /* 5. Floating Island Top Panel (bar_h=34, bar_r=17 preserved) */
    int bar_x = 10, bar_y = 6, bar_w = SW - 20, bar_h = 34, bar_r = 17;
    double p_alpha = st->cfg.low_power_mode ? 0.98 : pal.panel_alpha;
    ngfx_shadow_rect(cr, bar_x, bar_y, bar_w, bar_h, bar_r, 3.2, dark ? 0.42 : 0.18);
    ngfx_fill_gradient(cr, bar_x, bar_y, bar_w, bar_h, bar_r,
                       pal.panel_top_r, pal.panel_top_g, pal.panel_top_b, p_alpha,
                       pal.panel_bot_r, pal.panel_bot_g, pal.panel_bot_b, p_alpha, 0);
    ngfx_stroke_rect(cr, bar_x + 0.5, bar_y + 0.5, bar_w - 1, bar_h - 1, bar_r, 1.0,
                     pal.strong_brd_r, pal.strong_brd_g, pal.strong_brd_b, pal.strong_brd_a);
    ngfx_top_specular(cr, bar_x, bar_y, bar_w, bar_r, dark ? 0.24 : 0.68);

    int cur_x = bar_x + 16;
    ngfx_radial_glow(cr, cur_x, bar_y + 17, 1.0, 10.5, ar, ag, ab, 0.42, 0.0);
    ngfx_fill_circle(cr, cur_x, bar_y + 17, 5.2, pal.ar_hi, pal.ag_hi, pal.ab_hi, 1.0);
    if (SW >= 600) {
        ngfx_text(cr, "Plus Jakarta Sans", 1, 12.0, cur_x + 13, bar_y + 21.5, pal.fg_r, pal.fg_g, pal.fg_b, 1.0, "Nature OS");
        cur_x += 90;
    } else {
        cur_x += 14;
    }

    int act_w = SW < 480 ? 70 : 76;
    int act_hov = (st->hover_mx >= cur_x && st->hover_mx <= cur_x + act_w && st->hover_my >= bar_y + 4 && st->hover_my <= bar_y + 30);
    if (st->launcher_open || act_hov) {
        ngfx_fill_rect(cr, cur_x, bar_y + 4, act_w, 26, 13,
                       st->launcher_open ? ar : pal.elev_r, st->launcher_open ? ag : pal.elev_g, st->launcher_open ? ab : pal.elev_b,
                       st->launcher_open ? 0.90 : 0.85);
    }
    ngfx_text(cr, "Noto Sans", 1, 11.2, cur_x + (SW < 480 ? 10 : 12), bar_y + 21,
              st->launcher_open ? 1.0 : pal.fg_r, st->launcher_open ? 1.0 : pal.fg_g, st->launcher_open ? 1.0 : pal.fg_b, 1.0, "Activities");
    nature_add_hit(st, cur_x, bar_y + 4, act_w, 26, HIT_PANEL_ACTIVITIES, 0, 0);
    cur_x += act_w + 6;

    if (SW >= 680) {
        int ov_w = 70;
        int ov_hov = (st->hover_mx >= cur_x && st->hover_mx <= cur_x + ov_w && st->hover_my >= bar_y + 4 && st->hover_my <= bar_y + 30);
        if (st->overview_open || ov_hov) {
            ngfx_fill_rect(cr, cur_x, bar_y + 4, ov_w, 26, 13,
                           st->overview_open ? ar : pal.elev_r, st->overview_open ? ag : pal.elev_g, st->overview_open ? ab : pal.elev_b,
                           st->overview_open ? 0.90 : 0.85);
        }
        ngfx_text(cr, "Noto Sans", 1, 11.0, cur_x + 11, bar_y + 21,
                  st->overview_open ? 1.0 : pal.fg_sec_r, st->overview_open ? 1.0 : pal.fg_sec_g, st->overview_open ? 1.0 : pal.fg_sec_b, 0.96, "Overview");
        nature_add_hit(st, cur_x, bar_y + 4, ov_w, 26, HIT_PANEL_OVERVIEW, 0, 0);
        cur_x += ov_w + 6;
    }

    if (SW >= 520) {
        /* Segmented Workspace Switcher pill with activity dots */
        ngfx_fill_rect(cr, cur_x, bar_y + 4, 82, 26, 13, pal.sidebar_r, pal.sidebar_g, pal.sidebar_b, 0.72);
        ngfx_stroke_rect(cr, cur_x + 0.5, bar_y + 4.5, 81, 25, 13, 1.0, pal.border_r, pal.border_g, pal.border_b, pal.border_a);
        for (int ws = 0; ws < 2; ++ws) {
            int wx_btn = cur_x + 2 + ws * 39;
            int act = st->active_workspace == ws;
            if (act) ngfx_fill_rect(cr, wx_btn, bar_y + 6, 39, 22, 11, ar, ag, ab, 0.90);
            char wlbl[12];
            snprintf(wlbl, sizeof(wlbl), "WS%d", ws + 1);
            ngfx_text(cr, "JetBrains Mono", act, 10.2, wx_btn + 9, bar_y + 20.5,
                      act ? 1.0 : pal.mut_r, act ? 1.0 : pal.mut_g, act ? 1.0 : pal.mut_b, act ? 1.0 : 0.90, wlbl);

            /* Subtle activity dot if windows exist in this workspace */
            int has_wins = 0;
            for (int w = 0; w < st->window_count; ++w) {
                if (st->windows[w].workspace == ws && !st->windows[w].minimized) { has_wins = 1; break; }
            }
            if (has_wins && !act) {
                ngfx_fill_circle(cr, wx_btn + 32, bar_y + 11, 2.0, pal.ar_hi, pal.ag_hi, pal.ab_hi, 0.85);
            }

            nature_add_hit(st, wx_btn, bar_y + 4, 39, 26, HIT_PANEL_WS, ws, 0);
        }
        cur_x += 88;
    }

    /* Mini Now-Playing Audio Track Indicator Pill */
    if (st->audio_playing && SW >= 780) {
        int np_w = 118;
        int np_hov = (st->hover_mx >= cur_x && st->hover_mx <= cur_x + np_w && st->hover_my >= bar_y + 4 && st->hover_my <= bar_y + 30);
        ngfx_fill_rect(cr, cur_x, bar_y + 4, np_w, 26, 13,
                       np_hov ? pal.hover_r : pal.elev_r, np_hov ? pal.hover_g : pal.elev_g, np_hov ? pal.hover_b : pal.elev_b,
                       np_hov ? 0.88 : 0.60);
        ngfx_stroke_rect(cr, cur_x + 0.5, bar_y + 4.5, np_w - 1, 25, 13, 0.8,
                         pal.ar_hi, pal.ag_hi, pal.ab_hi, 0.45);
        /* Pulsing audio dot */
        ngfx_fill_circle(cr, cur_x + 11, bar_y + 17, 2.6, pal.success_r, pal.success_g, pal.success_b, 0.96);
        ngfx_text_clipped(cr, "Noto Sans", 1, 10.0, cur_x + 18, bar_y + 20.5, np_w - 24,
                          pal.fg_sec_r, pal.fg_sec_g, pal.fg_sec_b, 0.96, "Now Playing");
        nature_add_hit(st, cur_x, bar_y + 4, np_w, 26, HIT_LAUNCHER_APP, APP_MEDIA, 0);
        cur_x += np_w + 6;
    }

    char clock_buf[48];
    format_clock_str(st, tm_info, SW >= 640, clock_buf, sizeof(clock_buf));
    double cw_txt = ngfx_text_width(cr, "Plus Jakarta Sans", 1, 11.6, clock_buf);
    int has_alerts = st->notif_count > 0;
    int c_pill_w = (int)cw_txt + (has_alerts ? 34 : 24);
    int c_pill_x = (SW - c_pill_w) / 2;

    /* Active focused app title pill */
    const OSWindow *foc_win = NULL;
    for (int i = 0; i < st->window_count; ++i) {
        if (st->windows[i].id == st->active_window_id && !st->windows[i].minimized && st->windows[i].workspace == st->active_workspace) {
            foc_win = &st->windows[i];
            break;
        }
    }
    if (foc_win && SW >= 1020 && (c_pill_x - cur_x) >= 128) {
        int fw_avail = c_pill_x - cur_x - 12;
        if (fw_avail > 142) fw_avail = 142;
        int act_hov2 = (st->hover_mx >= cur_x && st->hover_mx <= cur_x + fw_avail &&
                        st->hover_my >= bar_y + 4 && st->hover_my <= bar_y + 30);
        ngfx_fill_rect(cr, cur_x, bar_y + 4, fw_avail, 26, 13,
                       act_hov2 ? pal.hover_r : pal.elev_r, act_hov2 ? pal.hover_g : pal.elev_g, act_hov2 ? pal.hover_b : pal.elev_b,
                       act_hov2 ? 0.86 : 0.58);
        ngfx_stroke_rect(cr, cur_x + 0.5, bar_y + 4.5, fw_avail - 1, 25, 13, 0.8,
                         act_hov2 ? ar : pal.border_r, act_hov2 ? ag : pal.border_g, act_hov2 ? ab : pal.border_b,
                         act_hov2 ? 0.55 : pal.border_a);
        ngfx_fill_circle(cr, cur_x + 12, bar_y + 17, 3.0, pal.ar_hi, pal.ag_hi, pal.ab_hi, 0.95);
        ngfx_text_clipped(cr, "Noto Sans", 1, 10.6, cur_x + 21, bar_y + 20.5, fw_avail - 30,
                          pal.fg_sec_r, pal.fg_sec_g, pal.fg_sec_b, 0.96, foc_win->title);
        nature_add_hit(st, cur_x, bar_y + 4, fw_avail, 26, HIT_PANEL_ACTIVE_WIN, 0, 0);
    }

    if (c_pill_x > cur_x + 6) {
        int clk_hov = (st->hover_mx >= c_pill_x && st->hover_mx <= c_pill_x + c_pill_w &&
                       st->hover_my >= bar_y + 4 && st->hover_my <= bar_y + 30);
        if (st->calendar_open || clk_hov) {
            ngfx_fill_rect(cr, c_pill_x, bar_y + 4, c_pill_w, 26, 13,
                           st->calendar_open ? ar : pal.elev_r, st->calendar_open ? ag : pal.elev_g, st->calendar_open ? ab : pal.elev_b,
                           st->calendar_open ? 0.90 : 0.82);
        }
        ngfx_text(cr, "Plus Jakarta Sans", 1, 11.6, c_pill_x + 12, bar_y + 21,
                  st->calendar_open ? 1.0 : pal.fg_r, st->calendar_open ? 1.0 : pal.fg_g, st->calendar_open ? 1.0 : pal.fg_b, 0.98, clock_buf);
        if (has_alerts) {
            ngfx_fill_circle(cr, c_pill_x + c_pill_w - 11, bar_y + 17, 2.8,
                             st->calendar_open ? 1.0 : pal.ar_hi,
                             st->calendar_open ? 1.0 : pal.ag_hi,
                             st->calendar_open ? 1.0 : pal.ab_hi, 0.96);
        }
        nature_add_hit(st, c_pill_x, bar_y + 4, c_pill_w, 26, HIT_PANEL_CLOCK, 0, 0);
    }

    int qs_w = SW < 480 ? 84 : 112;
    int qs_btn_x = bar_x + bar_w - qs_w - 5;
    int right_edge = qs_btn_x - 6;

    if (SW >= 560 && right_edge - 30 > c_pill_x + c_pill_w + 8) {
        int lck_x = right_edge - 28;
        int lck_hov = (st->hover_mx >= lck_x && st->hover_mx <= lck_x + 26 && st->hover_my >= bar_y + 4 && st->hover_my <= bar_y + 30);
        ngfx_fill_rect(cr, lck_x, bar_y + 4, 26, 26, 13, pal.elev_r, pal.elev_g, pal.elev_b, lck_hov ? 0.95 : 0.65);
        g_ngfx.set_source_rgba(cr, pal.fg_sec_r, pal.fg_sec_g, pal.fg_sec_b, 0.92);
        g_ngfx.set_line_width(cr, 1.3);
        g_ngfx.new_sub_path(cr);
        g_ngfx.arc(cr, lck_x + 13, bar_y + 14.5, 3.2, 3.14159, 6.28318);
        g_ngfx.stroke(cr);
        ngfx_fill_rect(cr, lck_x + 8.5, bar_y + 15.0, 9.0, 6.8, 1.8, pal.fg_sec_r, pal.fg_sec_g, pal.fg_sec_b, 0.92);
        nature_add_hit(st, lck_x, bar_y + 4, 26, 26, HIT_PANEL_LOCK, 0, 0);
        right_edge = lck_x - 6;
    }

    if (SW >= 820 && st->cfg.show_panel_stats) {
        char stats_buf[80];
        snprintf(stats_buf, sizeof(stats_buf), "%.0f FPS · %.0f%% CPU · %dM",
                 st->measured_fps, st->total_cpu_pct, st->used_ram_mb);
        double st_w = ngfx_text_width(cr, "JetBrains Mono", 0, 10.5, stats_buf);
        int st_pill_w = (int)st_w + 18;
        int st_pill_x = right_edge - st_pill_w - 4;
        if (st_pill_x > c_pill_x + c_pill_w + 8) {
            int m_hov = (st->hover_mx >= st_pill_x && st->hover_mx <= st_pill_x + st_pill_w &&
                         st->hover_my >= bar_y + 4 && st->hover_my <= bar_y + 30);
            if (m_hov) ngfx_fill_rect(cr, st_pill_x, bar_y + 4, st_pill_w, 26, 13, pal.elev_r, pal.elev_g, pal.elev_b, 0.80);
            ngfx_fill_circle(cr, st_pill_x + 7, bar_y + 17, 2.8,
                             st->stress_active ? pal.warning_r : pal.ar_hi,
                             st->stress_active ? pal.warning_g : pal.ag_hi,
                             st->stress_active ? pal.warning_b : pal.ab_hi, 0.95);
            ngfx_text(cr, "JetBrains Mono", 0, 10.5, st_pill_x + 15, bar_y + 21, pal.fg_sec_r, pal.fg_sec_g, pal.fg_sec_b, 0.92, stats_buf);
            nature_add_hit(st, st_pill_x, bar_y + 4, st_pill_w, 26, HIT_LAUNCHER_APP, APP_MONITOR, 0);
        }
    }

    int qs_hov = (st->hover_mx >= qs_btn_x && st->hover_mx <= qs_btn_x + qs_w &&
                  st->hover_my >= bar_y + 4 && st->hover_my <= bar_y + 30);
    ngfx_fill_rect(cr, qs_btn_x, bar_y + 4, qs_w, 26, 13,
                   st->quick_settings_open ? ar : pal.elev_r,
                   st->quick_settings_open ? ag : pal.elev_g,
                   st->quick_settings_open ? ab : pal.elev_b,
                   st->quick_settings_open ? 0.90 : (qs_hov ? 0.95 : 0.72));

    double tray_fg_r = st->quick_settings_open ? 1.0 : pal.fg_r;
    double tray_fg_g = st->quick_settings_open ? 1.0 : pal.fg_g;
    double tray_fg_b = st->quick_settings_open ? 1.0 : pal.fg_b;

    /* Wi-Fi & Speaker tray icons (shared with Quick Settings) */
    int net_up = st->cfg.wifi_enabled && st->svc_network;
    int snd_muted = st->cfg.audio_muted || !st->svc_pipewire;
    draw_wifi_glyph(cr, qs_btn_x + 16, bar_y + 16, tray_fg_r, tray_fg_g, tray_fg_b, net_up ? 0.95 : 0.32);
    draw_speaker_glyph(cr, qs_btn_x + 33, bar_y + 17, snd_muted, tray_fg_r, tray_fg_g, tray_fg_b, 0.92);

    char qs_vol[16];
    if (!st->svc_pipewire) snprintf(qs_vol, sizeof(qs_vol), "Off");
    else if (st->cfg.audio_muted) snprintf(qs_vol, sizeof(qs_vol), "Mute");
    else snprintf(qs_vol, sizeof(qs_vol), "%d%%", st->cfg.volume);
    if (SW >= 480) {
        ngfx_text(cr, "JetBrains Mono", 0, 10.2, qs_btn_x + 52, bar_y + 20.5, tray_fg_r, tray_fg_g, tray_fg_b, 0.95, qs_vol);
    }
    g_ngfx.set_source_rgba(cr, tray_fg_r, tray_fg_g, tray_fg_b, 0.78);
    g_ngfx.set_line_width(cr, 1.3);
    double chx = qs_btn_x + qs_w - 15;
    if (!st->cfg.show_toast_popups) {
        ngfx_fill_circle(cr, chx - 13, bar_y + 17, 2.5, pal.warning_r, pal.warning_g, pal.warning_b, 0.96);
    }
    if (st->cfg.night_light) {
        ngfx_fill_circle(cr, chx - 6, bar_y + 17, 2.4, 0.98, 0.68, 0.22, 0.96);
        g_ngfx.set_source_rgba(cr, tray_fg_r, tray_fg_g, tray_fg_b, 0.78);
    }
    g_ngfx.move_to(cr, chx, bar_y + 15.5); g_ngfx.line_to(cr, chx + 3.5, bar_y + 19.0); g_ngfx.line_to(cr, chx + 7.0, bar_y + 15.5);
    g_ngfx.stroke(cr);
    nature_add_hit(st, qs_btn_x, bar_y + 4, qs_w, 26, HIT_PANEL_TRAY, 0, 0);

    /* 6. Notification Toast */
    if (st->toast_timer > 0) {
        st->toast_timer--;
        if (st->cfg.show_toast_popups) {
            int tw = SW < 360 ? SW - 24 : 322, th = 62;
            int tx = SW - tw - 12, ty = 48;
            ngfx_shadow_rect(cr, tx, ty, tw, th, 14, 4.0, dark ? 0.50 : 0.24);
            ngfx_fill_gradient(cr, tx, ty, tw, th, 14, pal.card_r, pal.card_g, pal.card_b, 0.97, pal.win_bg_r, pal.win_bg_g, pal.win_bg_b, 0.97, 0);
            ngfx_stroke_rect(cr, tx + 0.5, ty + 0.5, tw - 1, th - 1, 14, 1.0, ar, ag, ab, 0.58);
            ngfx_top_specular(cr, tx, ty, tw, 14, dark ? 0.25 : 0.72);
            ngfx_fill_rect(cr, tx + 8, ty + 12, 3.5, th - 26, 1.75, pal.ar_hi, pal.ag_hi, pal.ab_hi, 0.96);

            ngfx_text_clipped(cr, "Plus Jakarta Sans", 1, 12.0, tx + 20, ty + 22, tw - 54, pal.ar_hi, pal.ag_hi, pal.ab_hi, 1.0, st->toast_title);
            ngfx_text_clipped(cr, "Noto Sans", 0, 11.0, tx + 20, ty + 41, tw - 34, pal.fg_r, pal.fg_g, pal.fg_b, 0.92, st->toast_msg);

            ngfx_fill_rect(cr, tx + tw - 26, ty + 8, 18, 18, 6, pal.elev_r, pal.elev_g, pal.elev_b, 0.85);
            ngfx_text(cr, "Noto Sans", 1, 9.6, tx + tw - 20.5, ty + 20.5, pal.mut_r, pal.mut_g, pal.mut_b, 0.95, "x");

            double t_frac = st->toast_timer / 180.0;
            if (t_frac > 1.0) t_frac = 1.0;
            if (t_frac < 0.0) t_frac = 0.0;
            ngfx_fill_rect(cr, tx + 14, ty + th - 6, (tw - 28) * t_frac, 2.5, 1.25, pal.ar_hi, pal.ag_hi, pal.ab_hi, 0.85);
            nature_add_hit(st, tx, ty, tw, th, HIT_TOAST_DISMISS, 0, 0);
        }
    }

    /* 7. Floating Glass Dock (desktop icon_sz=42 default, scaled by dock_size) */
    int base_sz = (st->cfg.dock_size == 0) ? 34 : (st->cfg.dock_size == 2 ? 48 : 42);
    int icon_sz = SW < 520 ? (base_sz > 34 ? 34 : base_sz) : base_sz;
    int gap = icon_sz <= 34 ? 7 : (icon_sz >= 48 ? 11 : 10);
    int total_items = APP_COUNT + 1;
    st->hover_dock_idx = -1;
    double d_alpha = st->cfg.low_power_mode ? 0.98 : pal.dock_alpha;

    if (st->cfg.dock_position == 0) {
        int show_lbls = (st->cfg.show_dock_labels && SW >= 700 && icon_sz >= 38);
        int extra_gap = show_lbls ? 8 : 0;
        int eff_gap = gap + extra_gap;
        int dock_w = total_items * icon_sz + (total_items - 1) * eff_gap + 32;
        if (dock_w > SW - 16) {
            show_lbls = 0;
            eff_gap = gap;
            dock_w = total_items * icon_sz + (total_items - 1) * eff_gap + 30;
        }
        int dock_h = icon_sz + (show_lbls ? 24 : 18);
        int dock_x = (SW - dock_w) / 2;
        int dock_y = SH - dock_h - (show_lbls ? 7 : 10);

        ngfx_shadow_rect(cr, dock_x, dock_y, dock_w, dock_h, 19, 4.5, dark ? 0.48 : 0.22);
        ngfx_fill_gradient(cr, dock_x, dock_y, dock_w, dock_h, 19,
                           pal.dock_top_r, pal.dock_top_g, pal.dock_top_b, d_alpha,
                           pal.dock_bot_r, pal.dock_bot_g, pal.dock_bot_b, d_alpha, 0);
        ngfx_stroke_rect(cr, dock_x + 0.5, dock_y + 0.5, dock_w - 1, dock_h - 1, 19, 1.0,
                         pal.strong_brd_r, pal.strong_brd_g, pal.strong_brd_b, pal.strong_brd_a);
        ngfx_top_specular(cr, dock_x, dock_y, dock_w, 19, dark ? 0.30 : 0.72);

        static const char *DOCK_SHORT[APP_COUNT] = {
            "Files", "Browser", "Terminal", "Software", "Monitor", "Media", "Settings"
        };

        for (int i = 0; i < APP_COUNT; ++i) {
            int ix = dock_x + 14 + i * (icon_sz + eff_gap);
            int iy = dock_y + (show_lbls ? 5 : 7);
            int hov = (st->hover_mx >= ix - 2 && st->hover_mx <= ix + icon_sz + 2 &&
                       st->hover_my >= iy - 6 && st->hover_my <= iy + icon_sz + 14);
            if (hov) {
                st->hover_dock_idx = i;
                ngfx_radial_glow(cr, ix + icon_sz * 0.5, iy + icon_sz * 0.4, 2.0, icon_sz * 0.72, ar, ag, ab, 0.32, 0.0);
                ngfx_shadow_rect(cr, ix, iy - 4, icon_sz, icon_sz, icon_sz * 0.27, 2.5, 0.40);
            }
            draw_app_icon(cr, (AppType)i, ix, hov ? iy - 4 : iy, icon_sz);
            nature_add_hit(st, ix - 2, iy, icon_sz + 4, icon_sz + (show_lbls ? 12 : 4), HIT_DOCK_APP, i, 0);

            int running = 0, focused = 0, vis_cnt = 0;
            for (int w = 0; w < st->window_count; ++w) {
                if (st->windows[w].app == (AppType)i) {
                    running++;
                    if (!st->windows[w].minimized) vis_cnt++;
                    if (st->windows[w].id == st->active_window_id && !st->windows[w].minimized) focused = 1;
                }
            }
            if (show_lbls) {
                double lw = ngfx_text_width(cr, "Noto Sans", focused, 8.6, DOCK_SHORT[i]);
                ngfx_text(cr, "Noto Sans", focused || hov, 8.6, ix + (icon_sz - lw) * 0.5, iy + icon_sz + 9.5,
                          (focused || hov) ? pal.fg_r : pal.fg_sec_r,
                          (focused || hov) ? pal.fg_g : pal.fg_sec_g,
                          (focused || hov) ? pal.fg_b : pal.fg_sec_b,
                          (focused || hov) ? 0.98 : 0.76, DOCK_SHORT[i]);
            }
            if (running > 0) {
                int ind_w = focused ? 16 : (running > 1 ? 11 : 6);
                int ind_y = show_lbls ? (dock_y + dock_h - 4) : (iy + icon_sz + 3);
                if (focused) {
                    ngfx_radial_glow(cr, ix + icon_sz * 0.5, ind_y + 1, 1.0, 12.0, ar, ag, ab, 0.45, 0.0);
                }
                ngfx_fill_rect(cr, ix + (icon_sz - ind_w) / 2, ind_y, ind_w, 2.5, 1.25,
                               pal.ar_hi, pal.ag_hi, pal.ab_hi, focused ? 1.0 : (vis_cnt > 0 ? 0.78 : 0.42));
            }
            if (hov) {
                double tw = ngfx_text_width(cr, "Noto Sans", 1, 11.0, APP_NAMES[i]);
                int tx = ix + icon_sz / 2 - (int)(tw * 0.5) - 10;
                if (tx < 8) tx = 8;
                int ty = dock_y - 32;
                ngfx_shadow_rect(cr, tx, ty, tw + 20, 23, 8, 2.2, dark ? 0.36 : 0.18);
                ngfx_fill_rect(cr, tx, ty, tw + 20, 23, 8, pal.card_r, pal.card_g, pal.card_b, 0.96);
                ngfx_stroke_rect(cr, tx + 0.5, ty + 0.5, tw + 19, 22, 8, 1.0,
                                 pal.strong_brd_r, pal.strong_brd_g, pal.strong_brd_b, pal.strong_brd_a);
                ngfx_text(cr, "Noto Sans", 1, 11.0, tx + 10, ty + 15.5, pal.fg_r, pal.fg_g, pal.fg_b, 1.0, APP_NAMES[i]);
            }
        }
        int sep_x = dock_x + 14 + APP_COUNT * (icon_sz + eff_gap) - (eff_gap / 2);
        ngfx_fill_rect(cr, sep_x, dock_y + 11, 1.5, icon_sz - 8, 1, pal.border_r, pal.border_g, pal.border_b, pal.border_a * 1.4);

        int gx = sep_x + (eff_gap / 2) + 2;
        int gy = dock_y + (show_lbls ? 5 : 7);
        int ghov = (st->hover_mx >= gx && st->hover_mx <= gx + icon_sz && st->hover_my >= gy && st->hover_my <= gy + icon_sz + 10);
        int gact = st->launcher_open;
        ngfx_fill_rect(cr, gx, gy, icon_sz, icon_sz, 12,
                       (ghov || gact) ? ar : pal.elev_r, (ghov || gact) ? ag : pal.elev_g, (ghov || gact) ? ab : pal.elev_b,
                       gact ? 0.88 : (ghov ? 0.34 : 0.72));
        ngfx_stroke_rect(cr, gx + 0.5, gy + 0.5, icon_sz - 1, icon_sz - 1, 12, 1.0,
                         (ghov || gact) ? ar : pal.border_r, (ghov || gact) ? ag : pal.border_g, (ghov || gact) ? ab : pal.border_b,
                         (ghov || gact) ? 0.75 : pal.border_a);
        double grid_off = (icon_sz - 20) * 0.5;
        for (int r = 0; r < 3; ++r) {
            for (int c = 0; c < 3; ++c) {
                ngfx_fill_rect(cr, gx + grid_off + c * 7.5, gy + grid_off + r * 7.5, 4.2, 4.2, 2,
                               gact ? 1.0 : pal.fg_r, gact ? 1.0 : pal.fg_g, gact ? 1.0 : pal.fg_b, 0.94);
            }
        }
        if (show_lbls) {
            double lw = ngfx_text_width(cr, "Noto Sans", gact, 8.6, "Apps");
            ngfx_text(cr, "Noto Sans", gact || ghov, 8.6, gx + (icon_sz - lw) * 0.5, gy + icon_sz + 9.5,
                      (gact || ghov) ? pal.fg_r : pal.fg_sec_r,
                      (gact || ghov) ? pal.fg_g : pal.fg_sec_g,
                      (gact || ghov) ? pal.fg_b : pal.fg_sec_b,
                      (gact || ghov) ? 0.98 : 0.76, "Apps");
        }
        if (ghov) {
            const char *atip = "Applications";
            double tw = ngfx_text_width(cr, "Noto Sans", 1, 11.0, atip);
            int tx = gx + icon_sz / 2 - (int)(tw * 0.5) - 10;
            if (tx + (int)tw + 24 > SW - 8) tx = SW - (int)tw - 28;
            int ty = dock_y - 32;
            ngfx_shadow_rect(cr, tx, ty, tw + 20, 23, 8, 2.2, dark ? 0.36 : 0.18);
            ngfx_fill_rect(cr, tx, ty, tw + 20, 23, 8, pal.card_r, pal.card_g, pal.card_b, 0.96);
            ngfx_stroke_rect(cr, tx + 0.5, ty + 0.5, tw + 19, 22, 8, 1.0,
                             pal.strong_brd_r, pal.strong_brd_g, pal.strong_brd_b, pal.strong_brd_a);
            ngfx_text(cr, "Noto Sans", 1, 11.0, tx + 10, ty + 15.5, pal.fg_r, pal.fg_g, pal.fg_b, 1.0, atip);
        }
        nature_add_hit(st, gx, gy, icon_sz, icon_sz + (show_lbls ? 12 : 4), HIT_DOCK_LAUNCHER, 0, 0);
    } else {
        int dock_w = icon_sz + 16;
        int dock_h = total_items * icon_sz + (total_items - 1) * gap + 24;
        int dock_x = 8;
        int dock_y = 46 + (SH - 46 - dock_h) / 2;

        ngfx_shadow_rect(cr, dock_x, dock_y, dock_w, dock_h, 19, 4.5, dark ? 0.48 : 0.22);
        ngfx_fill_gradient(cr, dock_x, dock_y, dock_w, dock_h, 19,
                           pal.dock_top_r, pal.dock_top_g, pal.dock_top_b, d_alpha,
                           pal.dock_bot_r, pal.dock_bot_g, pal.dock_bot_b, d_alpha, 1);
        ngfx_stroke_rect(cr, dock_x + 0.5, dock_y + 0.5, dock_w - 1, dock_h - 1, 19, 1.0,
                         pal.strong_brd_r, pal.strong_brd_g, pal.strong_brd_b, pal.strong_brd_a);

        for (int i = 0; i < APP_COUNT; ++i) {
            int ix = dock_x + 8;
            int iy = dock_y + 12 + i * (icon_sz + gap);
            int hov = (st->hover_mx >= ix && st->hover_mx <= ix + icon_sz && st->hover_my >= iy && st->hover_my <= iy + icon_sz);
            if (hov) ngfx_radial_glow(cr, ix + icon_sz * 0.5, iy + icon_sz * 0.5, 2.0, icon_sz * 0.7, ar, ag, ab, 0.30, 0.0);
            draw_app_icon(cr, (AppType)i, hov ? ix + 2 : ix, iy, icon_sz);
            nature_add_hit(st, ix, iy, icon_sz, icon_sz, HIT_DOCK_APP, i, 0);

            int running = 0, focused = 0, vis_cnt = 0;
            for (int w = 0; w < st->window_count; ++w) {
                if (st->windows[w].app == (AppType)i) {
                    running++;
                    if (!st->windows[w].minimized) vis_cnt++;
                    if (st->windows[w].id == st->active_window_id && !st->windows[w].minimized) focused = 1;
                }
            }
            if (running > 0) {
                int ih = focused ? 14 : (running > 1 ? 10 : 6);
                ngfx_fill_rect(cr, ix - 5, iy + (icon_sz - ih) / 2, 3, ih, 1.5,
                               pal.ar_hi, pal.ag_hi, pal.ab_hi, focused ? 1.0 : (vis_cnt > 0 ? 0.78 : 0.42));
            }
            if (hov) {
                st->hover_dock_idx = i;
                double tw = ngfx_text_width(cr, "Noto Sans", 1, 11.0, APP_NAMES[i]);
                int tx = dock_x + dock_w + 8;
                int ty = iy + (icon_sz - 23) / 2;
                ngfx_shadow_rect(cr, tx, ty, tw + 20, 23, 8, 2.2, dark ? 0.36 : 0.18);
                ngfx_fill_rect(cr, tx, ty, tw + 20, 23, 8, pal.card_r, pal.card_g, pal.card_b, 0.96);
                ngfx_stroke_rect(cr, tx + 0.5, ty + 0.5, tw + 19, 22, 8, 1.0,
                                 pal.strong_brd_r, pal.strong_brd_g, pal.strong_brd_b, pal.strong_brd_a);
                ngfx_text(cr, "Noto Sans", 1, 11.0, tx + 10, ty + 15.5, pal.fg_r, pal.fg_g, pal.fg_b, 1.0, APP_NAMES[i]);
            }
        }
        int gx = dock_x + 8;
        int gy = dock_y + 12 + APP_COUNT * (icon_sz + gap);
        int ghov = (st->hover_mx >= gx && st->hover_mx <= gx + icon_sz && st->hover_my >= gy && st->hover_my <= gy + icon_sz);
        int gact = st->launcher_open;
        ngfx_fill_rect(cr, gx, gy, icon_sz, icon_sz, 12,
                       (ghov || gact) ? ar : pal.elev_r, (ghov || gact) ? ag : pal.elev_g, (ghov || gact) ? ab : pal.elev_b,
                       gact ? 0.88 : (ghov ? 0.34 : 0.72));
        ngfx_stroke_rect(cr, gx + 0.5, gy + 0.5, icon_sz - 1, icon_sz - 1, 12, 1.0,
                         (ghov || gact) ? ar : pal.border_r, (ghov || gact) ? ag : pal.border_g, (ghov || gact) ? ab : pal.border_b,
                         (ghov || gact) ? 0.75 : pal.border_a);
        double grid_off = (icon_sz - 20) * 0.5;
        for (int r = 0; r < 3; ++r) {
            for (int c = 0; c < 3; ++c) {
                ngfx_fill_rect(cr, gx + grid_off + c * 7.5, gy + grid_off + r * 7.5, 4.2, 4.2, 2,
                               gact ? 1.0 : pal.fg_r, gact ? 1.0 : pal.fg_g, gact ? 1.0 : pal.fg_b, 0.94);
            }
        }
        if (ghov) {
            const char *atip = "Applications";
            double tw = ngfx_text_width(cr, "Noto Sans", 1, 11.0, atip);
            int tx = dock_x + dock_w + 8;
            int ty = gy + (icon_sz - 23) / 2;
            ngfx_shadow_rect(cr, tx, ty, tw + 20, 23, 8, 2.2, dark ? 0.36 : 0.18);
            ngfx_fill_rect(cr, tx, ty, tw + 20, 23, 8, pal.card_r, pal.card_g, pal.card_b, 0.96);
            ngfx_stroke_rect(cr, tx + 0.5, ty + 0.5, tw + 19, 22, 8, 1.0,
                             pal.strong_brd_r, pal.strong_brd_g, pal.strong_brd_b, pal.strong_brd_a);
            ngfx_text(cr, "Noto Sans", 1, 11.0, tx + 10, ty + 15.5, pal.fg_r, pal.fg_g, pal.fg_b, 1.0, atip);
        }
        nature_add_hit(st, gx, gy, icon_sz, icon_sz, HIT_DOCK_LAUNCHER, 0, 0);
    }

    /* 8. Overlays & Dialogs Pass */
    if (st->quick_settings_open) {
        render_quick_settings_popover(st, cr, &pal);
    } else if (st->calendar_open) {
        render_calendar_popover(st, cr, &pal);
    }

    if (st->ctx_menu_open) {
        render_desktop_ctx_menu(st, cr, &pal);
    }

    if (st->launcher_open) {
        render_launcher_overlay(st, cr, &pal);
    }

    if (st->overview_open && !st->is_locked) {
        render_overview_overlay(st, cr, &pal);
    }

    if (st->alt_tab_open) {
        render_alt_tab_hud(st, cr, &pal);
    }

    if (st->power_dialog_open) {
        render_power_dialog(st, cr, &pal);
    }

    if (st->is_locked) {
        render_lock_screen(st, cr, &pal);
    }

    /* 9. Global Night Light & Hardware Brightness Filter */
    if (st->cfg.night_light) {
        int warmth = st->cfg.night_light_warmth > 0 ? st->cfg.night_light_warmth : 65;
        if (warmth < 20) warmth = 20;
        if (warmth > 100) warmth = 100;
        double nl_alpha = 0.04 + 0.16 * (warmth / 100.0);
        ngfx_fill_rect(cr, 0, 0, SW, SH, 0, 0.96, 0.48, 0.06, nl_alpha);
    }
    if (st->cfg.brightness < 100) {
        double dim = (100 - st->cfg.brightness) * 0.006;
        if (dim > 0.55) dim = 0.55;
        if (dim > 0.0) ngfx_fill_rect(cr, 0, 0, SW, SH, 0, 0.0, 0.0, 0.0, dim);
    }
}
