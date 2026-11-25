#include "shell_internal.h"
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>


static void ensure_state_dir(const char *file_path) {
    if (!file_path || !*file_path) return;
    char dir[256];
    snprintf(dir, sizeof(dir), "%s", file_path);
    for (char *p = dir + 1; *p; ++p) {
        if (*p == '/') {
            *p = '\0';
            if (mkdir(dir, 0755) != 0 && errno != EEXIST) {
                /* ignore intermediate */
            }
            *p = '/';
        }
    }
}

static void resolve_state_path(NatureOSState *st) {
    if (!st) return;
    if (st->state_file_path[0]) return;
    const char *env_path = getenv("NATURE_OS_STATE_FILE");
    if (env_path && *env_path) {
        snprintf(st->state_file_path, sizeof(st->state_file_path), "%s", env_path);
        return;
    }
    const char *home = getenv("HOME");
    if (home && *home) {
        snprintf(st->state_file_path, sizeof(st->state_file_path), "%.210s/.config/nature-os/session.cfg", home);
    } else {
        snprintf(st->state_file_path, sizeof(st->state_file_path), "/tmp/nature-os-session.cfg");
    }
}

static void write_escaped_line(FILE *f, const char *key, const char *val) {
    fprintf(f, "%s=", key);
    if (val) {
        for (const char *p = val; *p; ++p) {
            if (*p == '\\') fputs("\\\\", f);
            else if (*p == '\n') fputs("\\n", f);
            else if (*p == '\r') continue;
            else fputc(*p, f);
        }
    }
    fputc('\n', f);
}

static void unescape_str(const char *src, char *dst, size_t dst_sz) {
    if (!dst || dst_sz == 0) return;
    size_t j = 0;
    for (size_t i = 0; src && src[i] && j + 1 < dst_sz; ++i) {
        if (src[i] == '\\' && src[i + 1]) {
            if (src[i + 1] == 'n') {
                dst[j++] = '\n';
                i++;
                continue;
            } else if (src[i + 1] == '\\') {
                dst[j++] = '\\';
                i++;
                continue;
            }
        }
        dst[j++] = src[i];
    }
    dst[j] = '\0';
}

static int clamp_i(int v, int lo, int hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

void nature_os_save_state(const NatureOSState *st_in) {
    if (!st_in) return;
    NatureOSState *st = (NatureOSState *)st_in;
    resolve_state_path(st);
    ensure_state_dir(st->state_file_path);

    char tmp_path[280];
    snprintf(tmp_path, sizeof(tmp_path), "%.250s.tmp", st->state_file_path);
    FILE *f = fopen(tmp_path, "w");
    if (!f) return;

    fprintf(f, "# Nature OS Session State v2\n");
    fprintf(f, "theme_family=%d\n", st->cfg.theme_family);
    fprintf(f, "dark_mode=%d\n", st->cfg.dark_mode);
    fprintf(f, "wallpaper_idx=%d\n", st->cfg.wallpaper_idx);
    fprintf(f, "custom_wp_media_idx=%d\n", st->custom_wp_media_idx);
    fprintf(f, "accent_idx=%d\n", st->cfg.accent_idx);
    fprintf(f, "dock_position=%d\n", st->cfg.dock_position);
    fprintf(f, "low_power_mode=%d\n", st->cfg.low_power_mode);
    fprintf(f, "zram_enabled=%d\n", st->cfg.zram_enabled);
    fprintf(f, "wifi_enabled=%d\n", st->cfg.wifi_enabled);
    fprintf(f, "bluetooth_enabled=%d\n", st->cfg.bluetooth_enabled);
    fprintf(f, "night_light=%d\n", st->cfg.night_light);
    fprintf(f, "night_light_warmth=%d\n", st->cfg.night_light_warmth);
    fprintf(f, "audio_muted=%d\n", st->cfg.audio_muted);
    fprintf(f, "audio_device=%d\n", st->cfg.audio_device);
    fprintf(f, "target_hz=%d\n", st->cfg.target_hz);
    fprintf(f, "volume=%d\n", st->cfg.volume);
    fprintf(f, "brightness=%d\n", st->cfg.brightness);
    fprintf(f, "power_profile=%d\n", st->cfg.power_profile);
    fprintf(f, "clock_24h=%d\n", st->cfg.clock_24h);
    fprintf(f, "show_seconds=%d\n", st->cfg.show_seconds);
    fprintf(f, "window_snap=%d\n", st->cfg.window_snap);
    fprintf(f, "show_desktop_widgets=%d\n", st->cfg.show_desktop_widgets);
    fprintf(f, "show_toast_popups=%d\n", st->cfg.show_toast_popups);
    write_escaped_line(f, "device_name", st->cfg.device_name);

    fprintf(f, "active_workspace=%d\n", st->active_workspace);
    fprintf(f, "settings_section=%d\n", st->settings_section);
    fprintf(f, "settings_update_checked=%d\n", st->settings_update_checked);
    fprintf(f, "files_list_mode=%d\n", st->files_list_mode);
    fprintf(f, "files_sort_mode=%d\n", st->files_sort_mode);
    fprintf(f, "files_show_hidden=%d\n", st->files_show_hidden);
    fprintf(f, "media_mode=%d\n", st->media_mode);
    fprintf(f, "media_grid_mode=%d\n", st->media_grid_mode);
    fprintf(f, "media_show_exif=%d\n", st->media_show_exif);
    fprintf(f, "media_selected=%d\n", st->media_selected);
    fprintf(f, "audio_selected=%d\n", st->audio_selected);
    fprintf(f, "audio_loop=%d\n", st->audio_loop);
    fprintf(f, "audio_shuffle=%d\n", st->audio_shuffle);
    fprintf(f, "audio_eq_preset=%d\n", st->audio_eq_preset);
    fprintf(f, "audio_speed_x10=%d\n", st->audio_speed_x10);
    fprintf(f, "browser_tab=%d\n", st->browser_tab);

    fprintf(f, "pkg_count=%d\n", st->pkg_count);
    for (int i = 0; i < st->pkg_count && i < MAX_APT_PKGS; ++i) {
        fprintf(f, "pkg_%d=%d\n", i, st->pkgs[i].installed ? 1 : 0);
    }

    fprintf(f, "win_count=%d\n", st->window_count);
    for (int i = 0; i < st->window_count && i < MAX_WINDOWS; ++i) {
        const OSWindow *w = &st->windows[i];
        fprintf(f, "win_%d=%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
                i, (int)w->app, w->x, w->y, w->w, w->h,
                w->minimized ? 1 : 0, w->maximized ? 1 : 0, w->snapped, w->workspace & 1,
                (w->id == st->active_window_id) ? 1 : 0,
                w->restore_x, w->restore_y, w->restore_w, w->restore_h);
    }

    int hkeep = st->term_hist_count < 12 ? st->term_hist_count : 12;
    int hstart = st->term_hist_count - hkeep;
    fprintf(f, "term_hist_count=%d\n", hkeep);
    for (int i = 0; i < hkeep; ++i) {
        char hk[32];
        snprintf(hk, sizeof(hk), "term_hist_%d", i);
        write_escaped_line(f, hk, st->term_history[hstart + i]);
    }

    fprintf(f, "vfs_count=%d\n", st->vfs_count);
    for (int i = 0; i < st->vfs_count && i < MAX_VFS; ++i) {
        const VfsNode *n = &st->vfs[i];
        fprintf(f, "vfs_%d_meta=%d,%d,%d,%d\n", i, n->is_dir ? 1 : 0, n->size_bytes, n->image_idx, n->audio_idx);
        char kbuf[32];
        snprintf(kbuf, sizeof(kbuf), "vfs_%d_name", i); write_escaped_line(f, kbuf, n->name);
        snprintf(kbuf, sizeof(kbuf), "vfs_%d_path", i); write_escaped_line(f, kbuf, n->path);
        snprintf(kbuf, sizeof(kbuf), "vfs_%d_parent", i); write_escaped_line(f, kbuf, n->parent);
        snprintf(kbuf, sizeof(kbuf), "vfs_%d_mime", i); write_escaped_line(f, kbuf, n->mime_tag);
        snprintf(kbuf, sizeof(kbuf), "vfs_%d_mod", i); write_escaped_line(f, kbuf, n->modified);
        snprintf(kbuf, sizeof(kbuf), "vfs_%d_content", i); write_escaped_line(f, kbuf, n->content);
    }

    fclose(f);
    rename(tmp_path, st->state_file_path);
}

void nature_os_load_state(NatureOSState *st) {
    if (!st) return;
    resolve_state_path(st);
    FILE *f = fopen(st->state_file_path, "r");
    if (!f) return;

    char line[2048];
    int has_wins = 0;
    int loaded_wins = 0;
    int has_vfs = 0;
    int loaded_vfs = 0;
    int loaded_active_slot = -1;

    while (fgets(line, sizeof(line), f)) {
        size_t len = strlen(line);
        while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) line[--len] = '\0';
        if (len == 0 || line[0] == '#') continue;

        char *eq = strchr(line, '=');
        if (!eq) continue;
        *eq = '\0';
        const char *k = line;
        const char *v = eq + 1;

        if (strcmp(k, "theme_family") == 0) st->cfg.theme_family = clamp_i(atoi(v), 0, 1);
        else if (strcmp(k, "dark_mode") == 0) st->cfg.dark_mode = clamp_i(atoi(v), 0, 1);
        else if (strcmp(k, "wallpaper_idx") == 0) st->cfg.wallpaper_idx = clamp_i(atoi(v), 0, MAX_WALLPAPERS);
        else if (strcmp(k, "custom_wp_media_idx") == 0) st->custom_wp_media_idx = clamp_i(atoi(v), -1, MAX_MEDIA_IMAGES - 1);
        else if (strcmp(k, "accent_idx") == 0) st->cfg.accent_idx = clamp_i(atoi(v), 0, 5);
        else if (strcmp(k, "dock_position") == 0) st->cfg.dock_position = clamp_i(atoi(v), 0, 1);
        else if (strcmp(k, "low_power_mode") == 0) st->cfg.low_power_mode = clamp_i(atoi(v), 0, 1);
        else if (strcmp(k, "zram_enabled") == 0) st->cfg.zram_enabled = clamp_i(atoi(v), 0, 1);
        else if (strcmp(k, "wifi_enabled") == 0) st->cfg.wifi_enabled = clamp_i(atoi(v), 0, 1);
        else if (strcmp(k, "bluetooth_enabled") == 0) st->cfg.bluetooth_enabled = clamp_i(atoi(v), 0, 1);
        else if (strcmp(k, "night_light") == 0) st->cfg.night_light = clamp_i(atoi(v), 0, 1);
        else if (strcmp(k, "night_light_warmth") == 0) st->cfg.night_light_warmth = clamp_i(atoi(v), 20, 100);
        else if (strcmp(k, "audio_muted") == 0) st->cfg.audio_muted = clamp_i(atoi(v), 0, 1);
        else if (strcmp(k, "audio_device") == 0) st->cfg.audio_device = clamp_i(atoi(v), 0, 2);
        else if (strcmp(k, "target_hz") == 0) st->cfg.target_hz = (atoi(v) == 120) ? 120 : 60;
        else if (strcmp(k, "volume") == 0) st->cfg.volume = clamp_i(atoi(v), 0, 100);
        else if (strcmp(k, "brightness") == 0) st->cfg.brightness = clamp_i(atoi(v), 25, 100);
        else if (strcmp(k, "power_profile") == 0) st->cfg.power_profile = clamp_i(atoi(v), 0, 2);
        else if (strcmp(k, "clock_24h") == 0) st->cfg.clock_24h = clamp_i(atoi(v), 0, 1);
        else if (strcmp(k, "show_seconds") == 0) st->cfg.show_seconds = clamp_i(atoi(v), 0, 1);
        else if (strcmp(k, "window_snap") == 0) st->cfg.window_snap = clamp_i(atoi(v), 0, 1);
        else if (strcmp(k, "show_desktop_widgets") == 0) st->cfg.show_desktop_widgets = clamp_i(atoi(v), 0, 1);
        else if (strcmp(k, "show_toast_popups") == 0) st->cfg.show_toast_popups = clamp_i(atoi(v), 0, 1);
        else if (strcmp(k, "device_name") == 0) {
            unescape_str(v, st->cfg.device_name, sizeof(st->cfg.device_name));
            if (!st->cfg.device_name[0]) snprintf(st->cfg.device_name, sizeof(st->cfg.device_name), "nature-pc");
            snprintf(st->settings_hostname_draft, sizeof(st->settings_hostname_draft), "%s", st->cfg.device_name);
        }
        else if (strcmp(k, "active_workspace") == 0) st->active_workspace = clamp_i(atoi(v), 0, 1);
        else if (strcmp(k, "settings_section") == 0) st->settings_section = clamp_i(atoi(v), 0, 6);
        else if (strcmp(k, "settings_update_checked") == 0) st->settings_update_checked = clamp_i(atoi(v), 0, 1);
        else if (strcmp(k, "files_list_mode") == 0) st->files_list_mode = clamp_i(atoi(v), 0, 1);
        else if (strcmp(k, "files_sort_mode") == 0) st->files_sort_mode = clamp_i(atoi(v), 0, 1);
        else if (strcmp(k, "files_show_hidden") == 0) st->files_show_hidden = clamp_i(atoi(v), 0, 1);
        else if (strcmp(k, "media_mode") == 0) st->media_mode = clamp_i(atoi(v), 0, 1);
        else if (strcmp(k, "media_grid_mode") == 0) st->media_grid_mode = clamp_i(atoi(v), 0, 1);
        else if (strcmp(k, "media_show_exif") == 0) st->media_show_exif = clamp_i(atoi(v), 0, 1);
        else if (strcmp(k, "media_selected") == 0) st->media_selected = clamp_i(atoi(v), 0, MAX_MEDIA_IMAGES - 1);
        else if (strcmp(k, "audio_selected") == 0) st->audio_selected = clamp_i(atoi(v), 0, MAX_AUDIO_TRACKS - 1);
        else if (strcmp(k, "audio_loop") == 0) st->audio_loop = clamp_i(atoi(v), 0, 1);
        else if (strcmp(k, "audio_shuffle") == 0) st->audio_shuffle = clamp_i(atoi(v), 0, 1);
        else if (strcmp(k, "audio_eq_preset") == 0) st->audio_eq_preset = clamp_i(atoi(v), 0, 3);
        else if (strcmp(k, "audio_speed_x10") == 0) st->audio_speed_x10 = clamp_i(atoi(v), 5, 20);
        else if (strcmp(k, "browser_tab") == 0) st->browser_tab = clamp_i(atoi(v), 0, 3);
        else if (strncmp(k, "pkg_", 4) == 0 && isdigit((unsigned char)k[4])) {
            int pi = atoi(k + 4);
            if (pi >= 0 && pi < st->pkg_count && pi < MAX_APT_PKGS) {
                st->pkgs[pi].installed = atoi(v) ? 1 : 0;
                st->pkgs[pi].progress = st->pkgs[pi].installed ? 100 : 0;
            }
        } else if (strcmp(k, "win_count") == 0) {
            has_wins = 1;
            loaded_wins = clamp_i(atoi(v), 0, MAX_WINDOWS);
            st->window_count = 0;
            st->active_window_id = -1;
        } else if (has_wins && strncmp(k, "win_", 4) == 0 && isdigit((unsigned char)k[4])) {
            int wi = atoi(k + 4);
            if (wi >= 0 && wi < loaded_wins && st->window_count < MAX_WINDOWS) {
                int app = 0, wx = 60, wy = 50, ww = 660, wh = 460, wmin = 0, wmax = 0, wsnap = 0, wws = 0, wact = 0;
                int rx = 0, ry = 0, rw = 0, rh = 0;
                int fields = sscanf(v, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
                                    &app, &wx, &wy, &ww, &wh, &wmin, &wmax, &wsnap, &wws, &wact,
                                    &rx, &ry, &rw, &rh);
                if (fields >= 9 && app >= 0 && app < APP_COUNT) {
                    OSWindow *nw = &st->windows[st->window_count++];
                    memset(nw, 0, sizeof(*nw));
                    nw->id = st->next_win_id++;
                    nw->app = (AppType)app;
                    snprintf(nw->title, sizeof(nw->title), "%s", APP_NAMES[app]);
                    nw->x = wx; nw->y = wy; nw->w = ww; nw->h = wh;
                    nw->minimized = wmin ? 1 : 0;
                    nw->maximized = wmax ? 1 : 0;
                    nw->snapped = clamp_i(wsnap, 0, 2);
                    nw->workspace = wws & 1;
                    nw->z_order = st->next_z++;
                    nw->pid = st->next_pid;
                    st->next_pid += 7;
                    if (fields >= 14 && rw > 100 && rh > 100) {
                        nw->restore_x = rx; nw->restore_y = ry;
                        nw->restore_w = rw; nw->restore_h = rh;
                    } else {
                        nw->restore_x = wx; nw->restore_y = wy;
                        nw->restore_w = ww; nw->restore_h = wh;
                    }
                    clamp_window_to_screen(st, nw);
                    if (wact) loaded_active_slot = st->window_count - 1;
                }
            }
        } else if (strcmp(k, "term_hist_count") == 0) {
            st->term_hist_count = 0;
        } else if (strncmp(k, "term_hist_", 10) == 0 && isdigit((unsigned char)k[10])) {
            int hi = atoi(k + 10);
            if (hi >= 0 && hi < 24) {
                unescape_str(v, st->term_history[hi], sizeof(st->term_history[hi]));
                if (hi + 1 > st->term_hist_count) st->term_hist_count = hi + 1;
            }
        } else if (strcmp(k, "vfs_count") == 0) {
            int vc = clamp_i(atoi(v), 1, MAX_VFS);
            has_vfs = 1;
            loaded_vfs = vc;
            memset(st->vfs, 0, sizeof(st->vfs));
            st->vfs_count = vc;
        } else if (has_vfs && strncmp(k, "vfs_", 4) == 0 && isdigit((unsigned char)k[4])) {
            int vi = atoi(k + 4);
            const char *sub = strchr(k + 4, '_');
            if (sub && vi >= 0 && vi < loaded_vfs && vi < MAX_VFS) {
                VfsNode *n = &st->vfs[vi];
                if (strcmp(sub, "_meta") == 0) {
                    int isd = 0, szb = 0, imgi = -1, audi = -1;
                    if (sscanf(v, "%d,%d,%d,%d", &isd, &szb, &imgi, &audi) == 4) {
                        n->is_dir = isd ? 1 : 0;
                        n->size_bytes = szb;
                        n->image_idx = imgi;
                        n->audio_idx = audi;
                    }
                } else if (strcmp(sub, "_name") == 0) {
                    unescape_str(v, n->name, sizeof(n->name));
                } else if (strcmp(sub, "_path") == 0) {
                    unescape_str(v, n->path, sizeof(n->path));
                } else if (strcmp(sub, "_parent") == 0) {
                    unescape_str(v, n->parent, sizeof(n->parent));
                } else if (strcmp(sub, "_mime") == 0) {
                    unescape_str(v, n->mime_tag, sizeof(n->mime_tag));
                } else if (strcmp(sub, "_mod") == 0) {
                    unescape_str(v, n->modified, sizeof(n->modified));
                } else if (strcmp(sub, "_content") == 0) {
                    unescape_str(v, n->content, sizeof(n->content));
                }
            }
        }
    }
    fclose(f);

    if (has_wins) {
        if (loaded_active_slot >= 0 && loaded_active_slot < st->window_count) {
            nature_focus_window(st, st->windows[loaded_active_slot].id);
        } else if (st->window_count > 0) {
            st->active_window_id = st->windows[st->window_count - 1].id;
        }
    }
    if (has_vfs) {
        int valid = 0;
        for (int i = 0; i < st->vfs_count; ++i) {
            if (st->vfs[i].path[0] && st->vfs[i].name[0]) {
                if (valid != i) st->vfs[valid] = st->vfs[i];
                valid++;
            }
        }
        st->vfs_count = valid;
    }

    st->session_restored = 1;
    st->dirty = 1;
}

void nature_os_reset_defaults(NatureOSState *st) {
    if (!st) return;
    st->cfg.theme_family = THEME_NATURE;
    st->cfg.dark_mode = 1;
    st->cfg.wallpaper_idx = 0;
    st->custom_wp_media_idx = -1;
    st->cfg.accent_idx = 0;
    st->cfg.dock_position = 0;
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
    snprintf(st->cfg.device_name, sizeof(st->cfg.device_name), "nature-pc");
    snprintf(st->settings_hostname_draft, sizeof(st->settings_hostname_draft), "nature-pc");
    st->settings_hostname_dirty = 0;
    st->settings_confirm_reset = 0;
    st->settings_confirm_factory = 0;
    st->settings_net_ping_ms = -1;
    st->cal_month_offset = 0;
    st->files_show_hidden = 1;
    st->audio_eq_preset = 0;
    for (int i = 0; i < st->window_count; ++i) {
        clamp_window_to_screen(st, &st->windows[i]);
    }
    nature_os_save_state(st);
    nature_notify(st, "Settings", "Restored Nature OS default preferences & saved.");
    st->dirty = 1;
}

void nature_os_reset_all_session(NatureOSState *st) {
    if (!st) return;
    resolve_state_path(st);
    remove(st->state_file_path);
    int cur_w = st->screen_w > 0 ? st->screen_w : OS_WIDTH;
    int cur_h = st->screen_h > 0 ? st->screen_h : OS_HEIGHT;
    cairo_surface_t *wps[MAX_WALLPAPERS];
    cairo_surface_t *meds[MAX_MEDIA_IMAGES];
    cairo_surface_t *av = st->avatar_surface;
    for (int i = 0; i < MAX_WALLPAPERS; ++i) wps[i] = st->wallpapers[i];
    for (int i = 0; i < MAX_MEDIA_IMAGES; ++i) meds[i] = st->media_images[i];
    if (st->cached_wp) {
        g_ngfx.surface_destroy(st->cached_wp);
        st->cached_wp = NULL;
    }
    if (meds[9]) { g_ngfx.surface_destroy(meds[9]); meds[9] = NULL; }
    if (meds[10]) { g_ngfx.surface_destroy(meds[10]); meds[10] = NULL; }
    char saved_path[256];
    snprintf(saved_path, sizeof(saved_path), "%s", st->state_file_path);

    nature_os_init(st, NULL);
    for (int i = 0; i < MAX_WALLPAPERS; ++i) st->wallpapers[i] = wps[i];
    for (int i = 0; i < 9 && i < MAX_MEDIA_IMAGES; ++i) st->media_images[i] = meds[i];
    st->avatar_surface = av;
    snprintf(st->state_file_path, sizeof(st->state_file_path), "%s", saved_path);
    st->settings_section = 6;
    nature_os_resize(st, cur_w, cur_h);
    nature_os_save_state(st);
    nature_notify(st, "Factory Reset", "Restored stock Nature OS apps, files & settings.");
    st->dirty = 1;
}
