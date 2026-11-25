#include "shell_internal.h"
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


void clamp_window_to_screen(NatureOSState *st, OSWindow *w) {
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
    }

    for (int i = 0; i < st->window_count; ++i) {
        clamp_window_to_screen(st, &st->windows[i]);
    }
    st->dirty = 1;
}

void nature_tile_windows(NatureOSState *st) {
    int vis[MAX_WINDOWS];
    int count = 0;
    for (int i = 0; i < st->window_count; ++i) {
        if (!st->windows[i].minimized && st->windows[i].workspace == st->active_workspace) {
            vis[count++] = i;
        }
    }
    if (count == 0) return;

    int sw = st->screen_w > 0 ? st->screen_w : OS_WIDTH;
    int sh = st->screen_h > 0 ? st->screen_h : OS_HEIGHT;
    int left_off = st->cfg.dock_position == 1 ? (sw < 640 ? 58 : 74) : 10;
    int bot_off = st->cfg.dock_position == 0 ? (sh < 540 ? 64 : 74) : 10;
    int avail_w = sw - left_off - 10;
    int avail_h = sh - 48 - bot_off;

    if (count == 1) {
        OSWindow *w = &st->windows[vis[0]];
        w->maximized = 0;
        w->snapped = 0;
        w->w = (avail_w > 860) ? 860 : (avail_w - 20);
        w->h = (avail_h > 560) ? 560 : (avail_h - 20);
        w->x = left_off + (avail_w - w->w) / 2;
        w->y = 48 + (avail_h - w->h) / 2;
    } else if (count == 2) {
        int half_w = (avail_w - 10) / 2;
        for (int i = 0; i < 2; ++i) {
            OSWindow *w = &st->windows[vis[i]];
            w->maximized = 0;
            w->snapped = (i == 0) ? 1 : 2;
            w->x = left_off + i * (half_w + 10);
            w->y = 48;
            w->w = half_w;
            w->h = avail_h;
        }
    } else {
        int cols = (count > 2) ? 2 : 1;
        int rows = (count + 1) / 2;
        int cell_w = (avail_w - (cols - 1) * 10) / cols;
        int cell_h = (avail_h - (rows - 1) * 10) / rows;
        for (int i = 0; i < count; ++i) {
            int c = i % cols;
            int r = i / cols;
            OSWindow *w = &st->windows[vis[i]];
            w->maximized = 0;
            w->snapped = 0;
            w->x = left_off + c * (cell_w + 10);
            w->y = 48 + r * (cell_h + 10);
            w->w = cell_w;
            w->h = cell_h;
        }
    }
    st->dirty = 1;
}

OSWindow *nature_open_app(NatureOSState *st, AppType app) {
    for (int i = 0; i < st->window_count; ++i) {
        if (st->windows[i].app == app) {
            st->windows[i].minimized = 0;
            st->windows[i].workspace = st->active_workspace;
            nature_focus_window(st, st->windows[i].id);
            st->launcher_open = 0;
            st->dirty = 1;
            return &st->windows[i];
        }
    }

    if (st->window_count >= MAX_WINDOWS) return NULL;

    OSWindow *w = &st->windows[st->window_count++];
    memset(w, 0, sizeof(*w));
    w->id = st->next_win_id++;
    w->app = app;
    snprintf(w->title, sizeof(w->title), "%s", APP_NAMES[app]);

    int sw = st->screen_w > 0 ? st->screen_w : OS_WIDTH;
    int sh = st->screen_h > 0 ? st->screen_h : OS_HEIGHT;

    int def_w = 680, def_h = 470;
    if (app == APP_TERMINAL) { def_w = 640; def_h = 430; }
    else if (app == APP_MONITOR) { def_w = 720; def_h = 490; }
    else if (app == APP_BROWSER) { def_w = 760; def_h = 510; }
    else if (app == APP_SETTINGS) { def_w = 700; def_h = 500; }

    if (def_w > sw - 40) def_w = sw - 40;
    if (def_h > sh - 110) def_h = sh - 110;

    int offset = (st->window_count - 1) * 26;
    w->x = 50 + offset;
    w->y = 50 + offset;
    w->w = def_w;
    w->h = def_h;
    w->restore_x = w->x;
    w->restore_y = w->y;
    w->restore_w = w->w;
    w->restore_h = w->h;

    clamp_window_to_screen(st, w);
    w->z_order = st->next_z++;
    w->workspace = st->active_workspace;
    w->pid = st->next_pid;
    st->next_pid += 7;
    st->active_window_id = w->id;
    st->launcher_open = 0;
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

void wm_cycle_alt_tab(NatureOSState *st, int step) {
    int n = 0;
    for (int i = 0; i < st->window_count; ++i) {
        if (!st->windows[i].minimized && st->windows[i].workspace == st->active_workspace) {
            n++;
        }
    }
    if (n <= 0) {
        st->alt_tab_open = 0;
        return;
    }
    st->alt_tab_open = 1;
    st->alt_tab_selected = (st->alt_tab_selected + step + n) % n;
    st->dirty = 1;
}

void wm_commit_alt_tab(NatureOSState *st) {
    if (!st->alt_tab_open) return;
    int vis[MAX_WINDOWS];
    int n = 0;
    for (int i = 0; i < st->window_count; ++i) {
        if (!st->windows[i].minimized && st->windows[i].workspace == st->active_workspace) {
            vis[n++] = st->windows[i].id;
        }
    }
    if (n > 0 && st->alt_tab_selected >= 0 && st->alt_tab_selected < n) {
        nature_focus_window(st, vis[st->alt_tab_selected]);
    }
    st->alt_tab_open = 0;
    st->dirty = 1;
}

void render_window(NatureOSState *st, cairo_t *cr, OSWindow *w, const NaturePalette *pal) {
    if (w->minimized) return;

    clamp_window_to_screen(st, w);
    int wx = w->x, wy = w->y, ww = w->w, wh = w->h;
    int is_active = (w->id == st->active_window_id);

    nature_occlude_text_rects(st, wx, wy, ww, wh);

    int dark = pal->dark_mode;
    double ar = pal->ar, ag = pal->ag, ab = pal->ab;

    if (!w->maximized) {
        ngfx_shadow_rect(cr, wx, wy, ww, wh, 14, is_active ? 5.0 : 2.6,
                         is_active ? (dark ? 0.62 : 0.30) : (dark ? 0.34 : 0.18));
    }

    double win_alpha = st->cfg.low_power_mode ? 1.0 : 0.975;
    ngfx_fill_rect(cr, wx, wy, ww, wh, 14, pal->win_bg_r, pal->win_bg_g, pal->win_bg_b, win_alpha);

    int hh = 38;
    g_ngfx.save(cr);
    ngfx_rounded_path(cr, wx, wy, ww, wh, 14);
    g_ngfx.clip(cr);

    /* CSD integrated headerbar */
    ngfx_fill_gradient(cr, wx, wy, ww, hh, 0,
                       is_active ? pal->hdr_act_top_r : pal->hdr_inact_top_r,
                       is_active ? pal->hdr_act_top_g : pal->hdr_inact_top_g,
                       is_active ? pal->hdr_act_top_b : pal->hdr_inact_top_b, 1.0,
                       is_active ? pal->hdr_act_bot_r : pal->hdr_inact_bot_r,
                       is_active ? pal->hdr_act_bot_g : pal->hdr_inact_bot_g,
                       is_active ? pal->hdr_act_bot_b : pal->hdr_inact_bot_b, 1.0, 0);
    if (is_active) {
        ngfx_fill_gradient(cr, wx + 20, wy, ww - 40, 2.0, 1.0,
                           ar, ag, ab, 0.72,
                           pal->ar_hi, pal->ag_hi, pal->ab_hi, 0.88, 1);
    }
    g_ngfx.set_source_rgba(cr, pal->border_r, pal->border_g, pal->border_b, is_active ? pal->border_a : pal->border_a * 0.6);
    g_ngfx.set_line_width(cr, 1.0);
    g_ngfx.move_to(cr, wx, wy + hh + 0.5);
    g_ngfx.line_to(cr, wx + ww, wy + hh + 0.5);
    g_ngfx.stroke(cr);

    /* Traffic Lights Controls */
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

    if (hov_snap) ngfx_fill_circle(cr, wx + 85, dot_y, 10.5, ar, ag, ab, 0.24);
    ngfx_fill_circle(cr, wx + 85, dot_y, 6.6, ar, ag, ab, is_active ? 1.0 : 0.60);
    ngfx_fill_circle(cr, wx + 84.0, dot_y - 2.0, 2.2, 1.0, 1.0, 1.0, is_active ? 0.22 : 0.10);
    if (is_active || hdr_hov) {
        g_ngfx.set_source_rgba(cr, 1.0, 1.0, 1.0, 0.88);
        g_ngfx.set_line_width(cr, 1.2);
        g_ngfx.rectangle(cr, wx + 81.8, dot_y - 2.6, 6.4, 5.2);
        g_ngfx.move_to(cr, wx + 85.0, dot_y - 2.6); g_ngfx.line_to(cr, wx + 85.0, dot_y + 2.6);
        g_ngfx.stroke(cr);
    }
    nature_add_hit(st, wx + 74, wy + 5, 22, 28, HIT_WIN_SNAP, w->id, 0);

    draw_app_icon(cr, w->app, wx + 108, wy + 9, 20);

    int ws_btn_w = 40;
    int ws_btn_x = wx + ww - ws_btn_w - 10;
    int show_state_pill = (ww >= 360 && (w->maximized || w->snapped > 0));
    int title_max_w = ww - (show_state_pill ? 242 : 196);
    ngfx_text_clipped(cr, "Plus Jakarta Sans", 1, 12.5, wx + 135, wy + 23.5, title_max_w,
                      pal->fg_r, pal->fg_g, pal->fg_b, is_active ? 1.0 : 0.62, w->title);

    nature_add_hit(st, wx + 98, wy, ww - (show_state_pill ? 202 : 156), hh, HIT_WIN_TITLE, w->id, 0);

    if (show_state_pill) {
        const char *st_lbl = w->maximized ? "MAX" : (w->snapped == 1 ? "TILE L" : "TILE R");
        int sp_w = w->maximized ? 34 : 44;
        int sp_x = ws_btn_x - sp_w - 6;
        ngfx_fill_rect(cr, sp_x, wy + 10, sp_w, 18, 9, ar, ag, ab, dark ? 0.22 : 0.14);
        ngfx_stroke_rect(cr, sp_x + 0.5, wy + 10.5, sp_w - 1, 17, 9, 0.8, ar, ag, ab, 0.45);
        ngfx_text(cr, "JetBrains Mono", 1, 8.2, sp_x + 6, wy + 22,
                  dark ? pal->ar_hi : ar, dark ? pal->ag_hi : ag, dark ? pal->ab_hi : ab,
                  is_active ? 0.96 : 0.60, st_lbl);
    }

    if (ww >= 280) {
        int ws_hov = (st->hover_mx >= ws_btn_x && st->hover_mx <= ws_btn_x + ws_btn_w &&
                      st->hover_my >= wy + 8 && st->hover_my <= wy + 30);
        ngfx_fill_rect(cr, ws_btn_x, wy + 9, ws_btn_w, 20, 10,
                       ws_hov ? ar : pal->elev_r, ws_hov ? ag : pal->elev_g, ws_hov ? ab : pal->elev_b,
                       ws_hov ? 0.88 : 0.72);
        ngfx_stroke_rect(cr, ws_btn_x + 0.5, wy + 9.5, ws_btn_w - 1, 19, 10, 1.0,
                         pal->border_r, pal->border_g, pal->border_b, pal->border_a);
        char ws_tag[16];
        snprintf(ws_tag, sizeof(ws_tag), "WS%d", (w->workspace & 1) + 1);
        ngfx_text(cr, "JetBrains Mono", 1, 9.4, ws_btn_x + 9, wy + 22.5,
                  ws_hov ? 1.0 : pal->fg_sec_r, ws_hov ? 1.0 : pal->fg_sec_g, ws_hov ? 1.0 : pal->fg_sec_b,
                  is_active ? 0.96 : 0.65, ws_tag);
        nature_add_hit(st, ws_btn_x, wy + 6, ws_btn_w, 26, HIT_WIN_SPACE_SWAP, w->id, 0);
    }

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
        g_ngfx.set_source_rgba(cr, res_hov ? ar : pal->mut_r, res_hov ? ag : pal->mut_g,
                               res_hov ? ab : pal->mut_b, res_hov ? 0.90 : 0.42);
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
                         pal->ar_hi, pal->ag_hi, pal->ab_hi, dark ? 0.72 : 0.65);
    } else {
        ngfx_stroke_rect(cr, wx + 0.5, wy + 0.5, ww - 1, wh - 1, 14, 1.0,
                         pal->border_r, pal->border_g, pal->border_b, pal->border_a);
    }
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
        case SCROLL_MEDIA_GRID: return &st->media_grid_scroll;
        default: return NULL;
    }
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

void nature_os_right_click(NatureOSState *st, int mx, int my) {
    st->quick_settings_open = 0;
    st->launcher_open = 0;
    st->power_dialog_open = 0;
    st->alt_tab_open = 0;
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
                st->overview_open = 0;
                st->quick_settings_open = 0;
                st->calendar_open = 0;
                st->ctx_menu_open = 0;
                st->power_dialog_open = 0;
                if (st->launcher_open) st->active_text_target = 2;
                st->dirty = 1;
                return;
            case HIT_PANEL_OVERVIEW:
                st->overview_open = !st->overview_open;
                st->launcher_open = 0;
                st->quick_settings_open = 0;
                st->calendar_open = 0;
                st->ctx_menu_open = 0;
                st->power_dialog_open = 0;
                st->dirty = 1;
                return;
            case HIT_PANEL_ACTIVE_WIN: {
                int ids[MAX_WINDOWS];
                int n = 0, cur_idx = -1;
                for (int w = 0; w < st->window_count; ++w) {
                    if (st->windows[w].workspace == st->active_workspace && !st->windows[w].minimized) {
                        if (st->windows[w].id == st->active_window_id) cur_idx = n;
                        ids[n++] = st->windows[w].id;
                    }
                }
                if (n > 1 && cur_idx >= 0) {
                    nature_focus_window(st, ids[(cur_idx + 1) % n]);
                } else {
                    st->overview_open = !st->overview_open;
                }
                st->dirty = 1;
                return;
            }
            case HIT_OVERVIEW_SHOW_DESKTOP: {
                int any_vis = 0;
                for (int w = 0; w < st->window_count; ++w) {
                    if (st->windows[w].workspace == st->active_workspace && !st->windows[w].minimized) { any_vis = 1; break; }
                }
                for (int w = 0; w < st->window_count; ++w) {
                    if (st->windows[w].workspace == st->active_workspace) st->windows[w].minimized = any_vis ? 1 : 0;
                }
                st->overview_open = 0;
                st->dirty = 1;
                return;
            }
            case HIT_PANEL_LOCK:
            case HIT_QS_LOCK:
            case HIT_POWER_LOCK:
                st->is_locked = 1;
                st->overview_open = 0;
                st->launcher_open = 0;
                st->quick_settings_open = 0;
                st->calendar_open = 0;
                st->ctx_menu_open = 0;
                st->power_dialog_open = 0;
                st->dirty = 1;
                return;
            case HIT_LOCK_UNLOCK:
                st->is_locked = 0;
                st->dirty = 1;
                return;
            case HIT_POWER_DIALOG_OPEN:
            case HIT_QS_REBOOT:
                st->power_dialog_open = 1;
                st->quick_settings_open = 0;
                st->calendar_open = 0;
                st->launcher_open = 0;
                st->overview_open = 0;
                st->dirty = 1;
                return;
            case HIT_POWER_CANCEL:
                st->power_dialog_open = 0;
                st->dirty = 1;
                return;
            case HIT_POWER_RESTART:
                st->power_dialog_open = 0;
                nature_notify(st, "Session", "Nature OS desktop session refreshed.");
                nature_os_load_state(st);
                st->dirty = 1;
                return;
            case HIT_POWER_SLEEP:
                st->power_dialog_open = 0;
                st->cfg.brightness = 25;
                nature_notify(st, "Power", "Display entered power saving standby.");
                nature_os_save_state(st);
                st->dirty = 1;
                return;
            case HIT_ALT_TAB_PREV:
                wm_cycle_alt_tab(st, -1);
                return;
            case HIT_ALT_TAB_NEXT:
                wm_cycle_alt_tab(st, 1);
                return;
            case HIT_ALT_TAB_SELECT:
                nature_focus_window(st, h->arg0);
                st->alt_tab_open = 0;
                st->dirty = 1;
                return;
            case HIT_LAUNCHER_CAT:
                st->launcher_cat = h->arg0;
                st->dirty = 1;
                return;
            case HIT_OVERVIEW_EXIT:
                st->overview_open = 0;
                st->dirty = 1;
                return;
            case HIT_OVERVIEW_CLOSE:
                nature_close_window(st, h->arg0);
                if (st->window_count == 0) st->overview_open = 0;
                nature_os_save_state(st);
                st->dirty = 1;
                return;
            case HIT_OVERVIEW_SWAP:
            case HIT_WIN_SPACE_SWAP:
                for (int w = 0; w < st->window_count; ++w) {
                    if (st->windows[w].id == h->arg0) {
                        st->windows[w].workspace = (st->windows[w].workspace == 0) ? 1 : 0;
                        if (h->action_id == HIT_WIN_SPACE_SWAP) {
                            st->active_workspace = st->windows[w].workspace;
                            nature_focus_window(st, st->windows[w].id);
                        }
                        nature_os_save_state(st);
                        st->dirty = 1;
                        return;
                    }
                }
                return;
            case HIT_PANEL_TRAY:
                st->quick_settings_open = !st->quick_settings_open;
                st->calendar_open = 0;
                st->launcher_open = 0;
                st->overview_open = 0;
                st->ctx_menu_open = 0;
                st->power_dialog_open = 0;
                st->dirty = 1;
                return;
            case HIT_PANEL_CLOCK:
                st->calendar_open = !st->calendar_open;
                st->quick_settings_open = 0;
                st->launcher_open = 0;
                st->overview_open = 0;
                st->ctx_menu_open = 0;
                st->power_dialog_open = 0;
                st->dirty = 1;
                return;
            case HIT_CAL_MONTH_NAV:
                st->cal_month_offset = (h->arg0 == 0) ? 0 : (st->cal_month_offset + h->arg0);
                st->dirty = 1;
                return;
            case HIT_CAL_TEST_NOTIF:
                nature_notify(st, "Nature OS Alert", "Desktop notifications & tray history active.");
                st->dirty = 1;
                return;
            case HIT_CAL_DISMISS_NOTIF: {
                int idx = h->arg0;
                if (idx >= 0 && idx < st->notif_count) {
                    for (int k = idx; k < st->notif_count - 1; ++k) {
                        memcpy(st->notif_titles[k], st->notif_titles[k + 1], sizeof(st->notif_titles[k]));
                        memcpy(st->notif_msgs[k], st->notif_msgs[k + 1], sizeof(st->notif_msgs[k]));
                        memcpy(st->notif_times[k], st->notif_times[k + 1], sizeof(st->notif_times[k]));
                    }
                    st->notif_count--;
                    st->dirty = 1;
                }
                return;
            }
            case HIT_CAL_CLEAR_NOTIFS:
                st->notif_count = 0;
                st->dirty = 1;
                return;
            case HIT_TOAST_DISMISS:
                st->toast_timer = 0;
                st->dirty = 1;
                return;
            case HIT_OVERVIEW_CLOSE_ALL:
                st->window_count = 0;
                st->active_window_id = -1;
                st->overview_open = 0;
                nature_os_save_state(st);
                st->dirty = 1;
                return;
            case HIT_QS_MEDIA_TOGGLE:
                st->audio_playing = !st->audio_playing;
                nature_os_save_state(st);
                st->dirty = 1;
                return;
            case HIT_QS_MEDIA_NEXT:
                st->audio_selected = (st->audio_selected + 1) % MAX_AUDIO_TRACKS;
                st->audio_progress_sec = 0;
                st->audio_playing = 1;
                nature_os_save_state(st);
                st->dirty = 1;
                return;
            case HIT_CTX_MENU_ITEM:
                st->ctx_menu_open = 0;
                if (h->arg0 == 0) nature_open_app(st, APP_TERMINAL);
                else if (h->arg0 == 1) nature_open_app(st, APP_FILES);
                else if (h->arg0 == 2) {
                    nature_tile_windows(st);
                    nature_notify(st, "Window Manager", "Tiled visible windows cleanly.");
                } else if (h->arg0 == 3) {
                    int any_vis = 0;
                    for (int w = 0; w < st->window_count; ++w) {
                        if (st->windows[w].workspace == st->active_workspace && !st->windows[w].minimized) { any_vis = 1; break; }
                    }
                    for (int w = 0; w < st->window_count; ++w) {
                        if (st->windows[w].workspace == st->active_workspace) st->windows[w].minimized = any_vis ? 1 : 0;
                    }
                } else if (h->arg0 == 4) {
                    st->cfg.dark_mode = !st->cfg.dark_mode;
                    if (st->cfg.theme_family == THEME_HORIZON && (st->cfg.wallpaper_idx == 3 || st->cfg.wallpaper_idx == 4)) {
                        nature_os_set_wallpaper_preset(st, st->cfg.dark_mode ? 4 : 3);
                    }
                } else if (h->arg0 == 5) {
                    st->cfg.theme_family = (st->cfg.theme_family == THEME_NATURE) ? THEME_HORIZON : THEME_NATURE;
                    if (st->cfg.theme_family == THEME_HORIZON) {
                        nature_os_set_wallpaper_preset(st, st->cfg.dark_mode ? 4 : 3);
                    } else if (st->cfg.wallpaper_idx == 3 || st->cfg.wallpaper_idx == 4 || st->cfg.wallpaper_idx == 7) {
                        nature_os_set_wallpaper_preset(st, 0);
                    }
                    nature_notify(st, "Desktop Style",
                                  st->cfg.theme_family == THEME_HORIZON ? "Switched to Horizon theme." : "Switched to Nature Canopy theme.");
                } else if (h->arg0 == 6) {
                    int next_wp = (st->cfg.wallpaper_idx + 1) % MAX_WALLPAPERS;
                    nature_os_set_wallpaper_preset(st, next_wp);
                    nature_notify(st, "Wallpaper", "Switched desktop wallpaper.");
                } else {
                    st->power_dialog_open = 1;
                }
                nature_os_save_state(st);
                st->dirty = 1;
                return;
            case HIT_QS_WIFI:
                st->cfg.wifi_enabled = !st->cfg.wifi_enabled;
                nature_notify(st, "NetworkManager", st->cfg.wifi_enabled ? "Wi-Fi enabled · Canopy-5GHz" : "Wi-Fi disabled.");
                nature_os_save_state(st);
                st->dirty = 1;
                return;
            case HIT_QS_BLUETOOTH:
                st->cfg.bluetooth_enabled = !st->cfg.bluetooth_enabled;
                nature_os_save_state(st);
                st->dirty = 1;
                return;
            case HIT_QS_DARK:
                st->cfg.dark_mode = !st->cfg.dark_mode;
                if (st->cfg.wallpaper_idx == 3 && st->cfg.dark_mode) {
                    nature_os_set_wallpaper_preset(st, 4);
                } else if (st->cfg.wallpaper_idx == 4 && !st->cfg.dark_mode) {
                    nature_os_set_wallpaper_preset(st, 3);
                }
                nature_os_save_state(st);
                st->dirty = 1;
                return;
            case HIT_QS_THEME_FAMILY:
                st->cfg.theme_family = (st->cfg.theme_family == THEME_NATURE) ? THEME_HORIZON : THEME_NATURE;
                if (st->cfg.theme_family == THEME_HORIZON) {
                    nature_os_set_wallpaper_preset(st, st->cfg.dark_mode ? 4 : 3);
                } else if (st->cfg.wallpaper_idx == 3 || st->cfg.wallpaper_idx == 4 || st->cfg.wallpaper_idx == 7) {
                    nature_os_set_wallpaper_preset(st, 0);
                }
                nature_notify(st, "Desktop Style",
                              st->cfg.theme_family == THEME_HORIZON ? "Switched to Horizon theme." : "Switched to Nature OS Canopy theme.");
                nature_os_save_state(st);
                st->dirty = 1;
                return;
            case HIT_QS_ACCENT:
                st->cfg.accent_idx = h->arg0 % 6;
                nature_os_save_state(st);
                st->dirty = 1;
                return;
            case HIT_QS_TILE:
                nature_tile_windows(st);
                st->quick_settings_open = 0;
                st->overview_open = 0;
                nature_notify(st, "Window Manager", "Tiled visible windows cleanly.");
                nature_os_save_state(st);
                st->dirty = 1;
                return;
            case HIT_QS_MUTE:
                st->cfg.audio_muted = !st->cfg.audio_muted;
                nature_os_save_state(st);
                st->dirty = 1;
                return;
            case HIT_QS_DND:
                st->cfg.show_toast_popups = !st->cfg.show_toast_popups;
                if (!st->cfg.show_toast_popups) st->toast_timer = 0;
                else nature_notify(st, "Notifications", "Desktop popup alerts resumed.");
                nature_os_save_state(st);
                st->dirty = 1;
                return;
            case HIT_QS_AUDIO_DEV: {
                static const char *SINK_LABELS[3] = {
                    "Built-in Audio (Analog Stereo)",
                    "DisplayPort / HDMI Digital",
                    "Bluetooth Canopy Buds (AAC)"
                };
                st->cfg.audio_device = (st->cfg.audio_device + 1) % 3;
                nature_notify(st, "PipeWire Audio", SINK_LABELS[st->cfg.audio_device]);
                nature_os_save_state(st);
                st->dirty = 1;
                return;
            }
            case HIT_QS_NIGHT_LIGHT:
                st->cfg.night_light = !st->cfg.night_light;
                nature_os_save_state(st);
                st->dirty = 1;
                return;
            case HIT_QS_PERF:
                st->cfg.power_profile = (st->cfg.power_profile + 1) % 3;
                nature_os_save_state(st);
                st->dirty = 1;
                return;
            case HIT_QS_SETTINGS:
                st->quick_settings_open = 0;
                nature_open_app(st, APP_SETTINGS);
                nature_os_save_state(st);
                return;
            case HIT_PANEL_WS:
                st->active_workspace = h->arg0;
                st->launcher_open = 0;
                st->quick_settings_open = 0;
                st->ctx_menu_open = 0;
                nature_os_save_state(st);
                st->dirty = 1;
                return;
            case HIT_OVERVIEW_WIN:
                st->launcher_open = 0;
                st->overview_open = 0;
                for (int w = 0; w < st->window_count; ++w) {
                    if (st->windows[w].id == h->arg0) {
                        st->active_workspace = st->windows[w].workspace;
                        break;
                    }
                }
                nature_focus_window(st, h->arg0);
                nature_os_save_state(st);
                return;
            case HIT_DOCK_APP: {
                AppType target = (AppType)h->arg0;
                st->overview_open = 0;
                for (int w = 0; w < st->window_count; ++w) {
                    if (st->windows[w].app == target && st->windows[w].id == st->active_window_id && !st->windows[w].minimized) {
                        st->windows[w].minimized = 1;
                        nature_os_save_state(st);
                        st->dirty = 1;
                        return;
                    }
                }
                nature_open_app(st, target);
                nature_os_save_state(st);
                return;
            }
            case HIT_LAUNCHER_APP:
            case HIT_DESKTOP_ICON:
                st->ctx_menu_open = 0;
                st->overview_open = 0;
                nature_open_app(st, (AppType)h->arg0);
                nature_os_save_state(st);
                return;
            case HIT_WIN_CLOSE:
                nature_close_window(st, h->arg0);
                nature_os_save_state(st);
                return;
            case HIT_WIN_MIN:
                for (int w = 0; w < st->window_count; ++w) {
                    if (st->windows[w].id == h->arg0) {
                        st->windows[w].minimized = 1;
                        nature_os_save_state(st);
                        st->dirty = 1;
                        return;
                    }
                }
                return;
            case HIT_WIN_MAX:
                for (int w = 0; w < st->window_count; ++w) {
                    if (st->windows[w].id == h->arg0) {
                        OSWindow *win = &st->windows[w];
                        if (!win->maximized) {
                            win->restore_x = win->x; win->restore_y = win->y;
                            win->restore_w = win->w; win->restore_h = win->h;
                            win->maximized = 1;
                            win->snapped = 0;
                        } else {
                            win->maximized = 0;
                            if (win->restore_w > 120 && win->restore_h > 100) {
                                win->x = win->restore_x; win->y = win->restore_y;
                                win->w = win->restore_w; win->h = win->restore_h;
                            }
                        }
                        clamp_window_to_screen(st, win);
                        nature_focus_window(st, h->arg0);
                        nature_os_save_state(st);
                        return;
                    }
                }
                return;
            case HIT_WIN_SNAP:
                for (int w = 0; w < st->window_count; ++w) {
                    if (st->windows[w].id == h->arg0) {
                        OSWindow *win = &st->windows[w];
                        if (win->snapped == 0 && !win->maximized) {
                            win->restore_x = win->x; win->restore_y = win->y;
                            win->restore_w = win->w; win->restore_h = win->h;
                        }
                        win->maximized = 0;
                        if (win->snapped == 0) win->snapped = 1;
                        else if (win->snapped == 1) win->snapped = 2;
                        else {
                            win->snapped = 0;
                            if (win->restore_w > 120 && win->restore_h > 100) {
                                win->x = win->restore_x; win->y = win->restore_y;
                                win->w = win->restore_w; win->h = win->restore_h;
                            } else {
                                int SW = st->screen_w > 0 ? st->screen_w : OS_WIDTH;
                                int SH = st->screen_h > 0 ? st->screen_h : OS_HEIGHT;
                                win->w = SW - 40 < 700 ? SW - 40 : 700;
                                win->h = SH - 130 < 460 ? SH - 130 : 460;
                                win->x = (SW - win->w) / 2;
                                win->y = 50 + (SH - 120 - win->h) / 2;
                            }
                        }
                        clamp_window_to_screen(st, win);
                        nature_focus_window(st, h->arg0);
                        nature_os_save_state(st);
                        return;
                    }
                }
                return;
            case HIT_WIN_TITLE: {
                int64_t now_ms = get_current_time_ms();
                int is_double = (h->arg0 == st->last_title_click_win && (now_ms - st->last_title_click_ms) <= 340);
                st->last_title_click_ms = now_ms;
                st->last_title_click_win = h->arg0;

                for (int w = 0; w < st->window_count; ++w) {
                    if (st->windows[w].id == h->arg0) {
                        nature_focus_window(st, h->arg0);
                        if (is_double) {
                            if (!st->windows[w].maximized) {
                                st->windows[w].restore_x = st->windows[w].x;
                                st->windows[w].restore_y = st->windows[w].y;
                                st->windows[w].restore_w = st->windows[w].w;
                                st->windows[w].restore_h = st->windows[w].h;
                                st->windows[w].maximized = 1;
                                st->windows[w].snapped = 0;
                            } else {
                                st->windows[w].maximized = 0;
                                if (st->windows[w].restore_w > 120 && st->windows[w].restore_h > 100) {
                                    st->windows[w].x = st->windows[w].restore_x;
                                    st->windows[w].y = st->windows[w].restore_y;
                                    st->windows[w].w = st->windows[w].restore_w;
                                    st->windows[w].h = st->windows[w].restore_h;
                                }
                            }
                            clamp_window_to_screen(st, &st->windows[w]);
                            st->drag_mode = 0;
                            nature_os_save_state(st);
                            return;
                        }
                        st->drag_mode = 1;
                        st->drag_win_id = h->arg0;
                        st->drag_start_mx = mx;
                        st->drag_start_my = my;
                        st->drag_orig_x = st->windows[w].x;
                        st->drag_orig_y = st->windows[w].y;
                        st->snap_preview = 0;
                        st->dirty = 1;
                        return;
                    }
                }
                return;
            }
            case HIT_WIN_RESIZE:
                for (int w = 0; w < st->window_count; ++w) {
                    if (st->windows[w].id == h->arg0) {
                        nature_focus_window(st, h->arg0);
                        st->drag_mode = 2;
                        st->drag_win_id = h->arg0;
                        st->drag_start_mx = mx;
                        st->drag_start_my = my;
                        st->drag_orig_w = st->windows[w].w;
                        st->drag_orig_h = st->windows[w].h;
                        st->dirty = 1;
                        return;
                    }
                }
                return;
            default:
                break;
        }

        /* Forward to native application click handler */
        if (h->action_id >= 100 && h->action_id < 200) { nature_files_click(st, h); return; }
        if (h->action_id >= 200 && h->action_id < 300) { nature_monitor_click(st, h); return; }
        if (h->action_id >= 300 && h->action_id < 400) { nature_settings_click(st, h); return; }
        if (h->action_id >= 400 && h->action_id < 500) { nature_store_click(st, h); return; }
        if (h->action_id >= 500 && h->action_id < 600) { nature_media_click(st, h); return; }
        if (h->action_id >= 600 && h->action_id < 700) { nature_browser_click(st, h); return; }
        if (h->action_id >= 700 && h->action_id < 800) { nature_terminal_click(st, h); return; }
    }

    /* Check text rect targets */
    for (int i = 0; i < st->text_rect_count; ++i) {
        const TextInputRect *tr = &st->text_rects[i];
        if (mx >= tr->x && mx <= tr->x + tr->w && my >= tr->y && my <= tr->y + tr->h) {
            st->active_text_target = tr->target_id;
            st->input_focus_pulse = 1;
            st->dirty = 1;
            return;
        }
    }

    /* Window click-to-focus */
    for (int z_pass = st->next_z; z_pass >= 0; --z_pass) {
        for (int w = 0; w < st->window_count; ++w) {
            OSWindow *win = &st->windows[w];
            if (win->minimized || win->workspace != st->active_workspace) continue;
            if (win->z_order != z_pass) continue;
            if (mx >= win->x && mx <= win->x + win->w && my >= win->y && my <= win->y + win->h) {
                nature_focus_window(st, win->id);
                st->quick_settings_open = 0;
                st->calendar_open = 0;
                st->ctx_menu_open = 0;
                st->launcher_open = 0;
                st->dirty = 1;
                return;
            }
        }
    }

    /* Backdrop click dismisses transient overlays */
    if (st->quick_settings_open || st->calendar_open || st->launcher_open || st->ctx_menu_open || st->power_dialog_open) {
        st->quick_settings_open = 0;
        st->calendar_open = 0;
        st->launcher_open = 0;
        st->ctx_menu_open = 0;
        st->power_dialog_open = 0;
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
                st->snap_preview = st->cfg.window_snap ? ((mx < 16) ? 1 : (mx > SW - 16 ? 2 : (my <= 48 ? 3 : 0))) : 0;
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
                if (st->snap_preview == 3) {
                    w->restore_x = w->x; w->restore_y = w->y;
                    w->restore_w = w->w; w->restore_h = w->h;
                    w->maximized = 1;
                    w->snapped = 0;
                } else {
                    w->restore_x = w->x; w->restore_y = w->y;
                    w->restore_w = w->w; w->restore_h = w->h;
                    w->snapped = st->snap_preview;
                }
                clamp_window_to_screen(st, w);
                break;
            }
        }
    }
    int was_persist_drag = (st->drag_mode == 1 || st->drag_mode == 2 || st->drag_mode == 4 || st->drag_mode == 5);
    st->drag_mode = 0;
    st->drag_win_id = -1;
    st->scroll_drag_target = SCROLL_NONE;
    st->snap_preview = 0;
    if (was_persist_drag) {
        nature_os_save_state(st);
    }
    st->dirty = 1;
}

void nature_os_wheel(NatureOSState *st, int delta_y) {
    int target_app = -1;
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
    if (st->is_locked) {
        if (strcmp(key, "Enter") == 0 || strcmp(key, " ") == 0 || strcmp(key, "Space") == 0) {
            st->is_locked = 0;
            st->dirty = 1;
        }
        return;
    }
    if (strcmp(key, "Escape") == 0) {
        st->launcher_open = 0;
        st->launcher_query[0] = '\0';
        st->overview_open = 0;
        st->quick_settings_open = 0;
        st->calendar_open = 0;
        st->ctx_menu_open = 0;
        st->power_dialog_open = 0;
        st->alt_tab_open = 0;
        st->active_text_target = 0;
        st->files_viewer_open = 0;
        st->files_editing = 0;
        st->dirty = 1;
        return;
    }
    if (strcmp(key, "Super") == 0 || strcmp(key, "Meta") == 0) {
        st->launcher_open = !st->launcher_open;
        st->launcher_query[0] = '\0';
        st->overview_open = 0;
        st->active_text_target = st->launcher_open ? 2 : 0;
        st->dirty = 1;
        return;
    }
    if (strcmp(key, "Tab") == 0 && st->active_text_target == 0) {
        wm_cycle_alt_tab(st, 1);
        return;
    }
    if (st->alt_tab_open) {
        if (strcmp(key, "Enter") == 0 || strcmp(key, " ") == 0) {
            wm_commit_alt_tab(st);
            return;
        }
        if (strcmp(key, "ArrowLeft") == 0 || strcmp(key, "ArrowUp") == 0) {
            wm_cycle_alt_tab(st, -1);
            return;
        }
        if (strcmp(key, "ArrowRight") == 0 || strcmp(key, "ArrowDown") == 0) {
            wm_cycle_alt_tab(st, 1);
            return;
        }
    }
    if (st->launcher_open) {
        if (strcmp(key, "Backspace") == 0) {
            size_t len = strlen(st->launcher_query);
            if (len > 0) st->launcher_query[len - 1] = '\0';
        } else if (strcmp(key, "Enter") == 0) {
            for (int i = 0; i < APP_COUNT; ++i) {
                if (app_matches_launcher_cat((AppType)i, st->launcher_cat) &&
                    str_contains_ci(APP_NAMES[i], st->launcher_query)) {
                    nature_open_app(st, (AppType)i);
                    nature_os_save_state(st);
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
                if (strcmp(key, "Enter") == 0) nature_os_save_state(st);
                st->dirty = 1;
                return;
            }
            if (st->windows[i].app == APP_FILES &&
                (st->active_text_target == 4 || st->active_text_target == 7 || st->active_text_target == 9)) {
                nature_files_key(st, key);
                if (strcmp(key, "Enter") == 0) nature_os_save_state(st);
                st->dirty = 1;
                return;
            }
            if (st->windows[i].app == APP_SETTINGS && (st->active_text_target == 5 || st->active_text_target == 6)) {
                nature_settings_key(st, key);
                if (strcmp(key, "Enter") == 0) nature_os_save_state(st);
                st->dirty = 1;
                return;
            }
            if (st->windows[i].app == APP_STORE && st->active_text_target == 8) {
                nature_store_key(st, key);
                st->dirty = 1;
                return;
            }
            if (st->windows[i].app == APP_MEDIA && st->active_text_target == 0) {
                nature_media_key(st, key);
                nature_os_save_state(st);
                st->dirty = 1;
                return;
            }
            if (st->windows[i].app == APP_TERMINAL) {
                nature_terminal_key(st, key);
                if (strcmp(key, "Enter") == 0) nature_os_save_state(st);
                st->dirty = 1;
                return;
            }
        }
    }
}
