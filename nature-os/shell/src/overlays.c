#include "shell_internal.h"
#include "nature_gfx.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

static const char *QS_AUDIO_TITLES[MAX_AUDIO_TRACKS] = {
    "Rainforest Canopy",
    "Alpine Wind & Cedar",
    "Night Moss Stream",
    "Horizon Startup Chime"
};

/* --- Quick Settings Tile Helper (shared glyphs live in icons.c) --- */

static void draw_qs_tile_bg(cairo_t *cr, int x, int y, int w, int h, int active, int hover,
                            double ar, double ag, double ab,
                            double ar_hi, double ag_hi, double ab_hi,
                            const NaturePalette *pal) {
    if (active) {
        ngfx_fill_gradient(cr, x, y, w, h, pal->r_card,
                           ar, ag, ab, hover ? 0.98 : 0.92,
                           ar_hi, ag_hi, ab_hi, hover ? 0.98 : 0.92, 1);
        ngfx_stroke_rect(cr, x + 0.5, y + 0.5, w - 1, h - 1, pal->r_card, 1.0, 1.0, 1.0, 1.0, 0.22);
    } else {
        ngfx_fill_rect(cr, x, y, w, h, pal->r_card,
                       pal->elev_r, pal->elev_g, pal->elev_b, hover ? 0.95 : pal->elev_alpha);
        ngfx_stroke_rect(cr, x + 0.5, y + 0.5, w - 1, h - 1, pal->r_card, 0.8,
                         pal->border_r, pal->border_g, pal->border_b, hover ? pal->border_a * 1.5 : pal->border_a);
    }
}

static int month_days_for(int mon_0_11, int year_since_1900) {
    static const int mdays[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    int d = mdays[(mon_0_11 % 12 + 12) % 12];
    int y = year_since_1900 + 1900;
    if ((mon_0_11 % 12) == 1 && ((y % 4 == 0 && y % 100 != 0) || (y % 400 == 0))) d = 29;
    return d;
}

/* --- Quick Settings Popover Menu --- */

void render_quick_settings_popover(NatureOSState *st, cairo_t *cr, const NaturePalette *pal) {
    int sw = st->screen_w;
    int sh = st->screen_h > 0 ? st->screen_h : OS_HEIGHT;
    int pw = sw < 358 ? sw - 20 : 338;
    int ph = 492;
    int px = sw - pw - 10;
    int py = 48;
    if (px < 10) px = 10;
    if (py + ph > sh - 12) ph = sh - py - 12;

    nature_occlude_text_rects(st, px, py, pw, ph);

    int dark = pal->dark_mode;
    double ar = pal->ar, ag = pal->ag, ab = pal->ab;
    double fg_r = pal->fg_r, fg_g = pal->fg_g, fg_b = pal->fg_b;
    double mut_r = pal->mut_r, mut_g = pal->mut_g, mut_b = pal->mut_b;

    ngfx_shadow_rect(cr, px, py, pw, ph, 18, 5.2, dark ? 0.62 : 0.28);
    ngfx_fill_gradient(cr, px, py, pw, ph, 18,
                       pal->card_r, pal->card_g, pal->card_b, 0.985,
                       pal->win_bg_r, pal->win_bg_g, pal->win_bg_b, 0.985, 0);
    ngfx_stroke_rect(cr, px + 0.5, py + 0.5, pw - 1, ph - 1, 18, 1.0,
                     pal->strong_brd_r, pal->strong_brd_g, pal->strong_brd_b, pal->strong_brd_a);
    ngfx_top_specular(cr, px, py, pw, 18, dark ? 0.28 : 0.78);

    /* 1. Identity & Quick Power Header */
    int hdr_x = px + 12, hdr_y = py + 11, hdr_w = pw - 24, hdr_h = 40;
    ngfx_fill_rect(cr, hdr_x, hdr_y, hdr_w, hdr_h, 12, pal->elev_r, pal->elev_g, pal->elev_b, 0.72);
    ngfx_stroke_rect(cr, hdr_x + 0.5, hdr_y + 0.5, hdr_w - 1, hdr_h - 1, 12, 0.8,
                     pal->border_r, pal->border_g, pal->border_b, pal->border_a);

    if (st->avatar_surface) {
        double aw = g_ngfx.image_surface_get_width(st->avatar_surface);
        double ah = g_ngfx.image_surface_get_height(st->avatar_surface);
        if (aw > 0 && ah > 0) {
            g_ngfx.save(cr);
            g_ngfx.new_sub_path(cr);
            g_ngfx.arc(cr, hdr_x + 20, hdr_y + 20, 13.0, 0, 6.28318);
            g_ngfx.clip(cr);
            g_ngfx.translate(cr, hdr_x + 7, hdr_y + 7);
            g_ngfx.scale(cr, 26.0 / aw, 26.0 / ah);
            g_ngfx.set_source_surface(cr, st->avatar_surface, 0, 0);
            g_ngfx.paint(cr);
            g_ngfx.restore(cr);
            ngfx_stroke_rect(cr, hdr_x + 7, hdr_y + 7, 26, 26, 13, 1.1, ar, ag, ab, 0.78);
        }
    } else {
        ngfx_fill_circle(cr, hdr_x + 20, hdr_y + 20, 13.0, ar, ag, ab, 0.92);
        ngfx_text(cr, "Plus Jakarta Sans", 1, 11.5, hdr_x + 15.5, hdr_y + 24, 1, 1, 1, 1, "N");
    }

    char user_hdr[48], sub_hdr[64];
    const char *host = st->cfg.device_name[0] ? st->cfg.device_name : "nature-pc";
    snprintf(user_hdr, sizeof(user_hdr), "nature@%s", host);
    snprintf(sub_hdr, sizeof(sub_hdr), "%s · %s",
             st->cfg.theme_family == THEME_HORIZON ? "Horizon" : "Canopy",
             st->cfg.power_profile == 0 ? "Saver" : (st->cfg.power_profile == 2 ? "Perf" : "Balanced"));
    ngfx_text_clipped(cr, "Plus Jakarta Sans", 1, 11.2, hdr_x + 40, hdr_y + 18, hdr_w - 138, fg_r, fg_g, fg_b, 0.98, user_hdr);
    ngfx_text_clipped(cr, "JetBrains Mono", 0, 8.8, hdr_x + 40, hdr_y + 32, hdr_w - 138, mut_r, mut_g, mut_b, 0.92, sub_hdr);

    /* Right-side circular action buttons: Settings, Lock, Power Dialog */
    int bx_r = hdr_x + hdr_w - 94;
    int h_set = (st->hover_mx >= bx_r && st->hover_mx <= bx_r + 26 && st->hover_my >= hdr_y + 7 && st->hover_my <= hdr_y + 33);
    ngfx_fill_circle(cr, bx_r + 13, hdr_y + 20, 13,
                     h_set ? ar : pal->card_r, h_set ? ag : pal->card_g, h_set ? ab : pal->card_b, h_set ? 0.25 : 0.96);
    ngfx_stroke_rect(cr, bx_r + 0.5, hdr_y + 7.5, 25, 25, 12.5, 0.8, pal->border_r, pal->border_g, pal->border_b, pal->border_a);
    g_ngfx.set_source_rgba(cr, fg_r, fg_g, fg_b, 0.92);
    g_ngfx.set_line_width(cr, 1.3);
    g_ngfx.new_sub_path(cr); g_ngfx.arc(cr, bx_r + 13, hdr_y + 20, 3.8, 0, 6.283); g_ngfx.stroke(cr);
    for (int k = 0; k < 6; ++k) {
        double a = k * (3.14159265 / 3.0);
        g_ngfx.move_to(cr, bx_r + 13 + cos(a) * 4.8, hdr_y + 20 + sin(a) * 4.8);
        g_ngfx.line_to(cr, bx_r + 13 + cos(a) * 6.6, hdr_y + 20 + sin(a) * 6.6);
    }
    g_ngfx.stroke(cr);
    nature_add_hit(st, bx_r, hdr_y + 6, 26, 28, HIT_QS_SETTINGS, 0, 0);

    int bx_l = bx_r + 30;
    int h_lck = (st->hover_mx >= bx_l && st->hover_mx <= bx_l + 26 && st->hover_my >= hdr_y + 7 && st->hover_my <= hdr_y + 33);
    ngfx_fill_circle(cr, bx_l + 13, hdr_y + 20, 13, ar, ag, ab, h_lck ? 0.36 : 0.20);
    ngfx_stroke_rect(cr, bx_l + 0.5, hdr_y + 7.5, 25, 25, 12.5, 0.8, ar, ag, ab, 0.50);
    g_ngfx.set_source_rgba(cr, fg_r, fg_g, fg_b, 0.95);
    g_ngfx.set_line_width(cr, 1.3);
    g_ngfx.new_sub_path(cr); g_ngfx.arc(cr, bx_l + 13, hdr_y + 18.2, 2.8, 3.14159, 6.28318); g_ngfx.stroke(cr);
    ngfx_fill_rect(cr, bx_l + 9.2, hdr_y + 18.5, 7.6, 5.8, 1.4, fg_r, fg_g, fg_b, 0.94);
    nature_add_hit(st, bx_l, hdr_y + 6, 26, 28, HIT_QS_LOCK, 0, 0);

    int bx_p = bx_l + 30;
    int h_reb = (st->hover_mx >= bx_p && st->hover_mx <= bx_p + 26 && st->hover_my >= hdr_y + 7 && st->hover_my <= hdr_y + 33);
    ngfx_fill_circle(cr, bx_p + 13, hdr_y + 20, 13, pal->danger_r, pal->danger_g, pal->danger_b, h_reb ? 0.36 : 0.20);
    ngfx_stroke_rect(cr, bx_p + 0.5, hdr_y + 7.5, 25, 25, 12.5, 0.8, pal->danger_r, pal->danger_g, pal->danger_b, 0.48);
    g_ngfx.set_source_rgba(cr, pal->danger_r, pal->danger_g, pal->danger_b, 0.98);
    g_ngfx.set_line_width(cr, 1.4);
    g_ngfx.new_sub_path(cr); g_ngfx.arc(cr, bx_p + 13, hdr_y + 20.5, 4.6, -1.0, 4.15); g_ngfx.stroke(cr);
    g_ngfx.move_to(cr, bx_p + 13, hdr_y + 14.5); g_ngfx.line_to(cr, bx_p + 13, hdr_y + 19.2); g_ngfx.stroke(cr);
    nature_add_hit(st, bx_p, hdr_y + 6, 26, 28, HIT_POWER_DIALOG_OPEN, 0, 0);

    /* 2. 2x3 Glassmorphic Quick Toggle Grid */
    int t_w = (pw - 32) / 2;
    int col0_x = px + 12;
    int col1_x = px + 20 + t_w;
    int row_y0 = py + 58;
    int tile_h = 44;
    int row_gap = 6;

    /* Tile 1: Wi-Fi */
    int r1_y = row_y0;
    int wifi_on = st->cfg.wifi_enabled && st->svc_network;
    int h_wifi = (st->hover_mx >= col0_x && st->hover_mx <= col0_x + t_w && st->hover_my >= r1_y && st->hover_my <= r1_y + tile_h);
    draw_qs_tile_bg(cr, col0_x, r1_y, t_w, tile_h, wifi_on, h_wifi, ar, ag, ab, pal->ar_hi, pal->ag_hi, pal->ab_hi, pal);
    ngfx_fill_circle(cr, col0_x + 21, r1_y + 22, 13.5, wifi_on ? 1.0 : pal->card_r, wifi_on ? 1.0 : pal->card_g, wifi_on ? 1.0 : pal->card_b, wifi_on ? 0.22 : 0.90);
    draw_wifi_glyph(cr, col0_x + 21, r1_y + 21, wifi_on ? 1.0 : fg_r, wifi_on ? 1.0 : fg_g, wifi_on ? 1.0 : fg_b, 0.96);
    ngfx_text(cr, "Plus Jakarta Sans", 1, 11.4, col0_x + 41, r1_y + 19, wifi_on ? 1.0 : fg_r, wifi_on ? 1.0 : fg_g, wifi_on ? 1.0 : fg_b, 1.0, "Wi-Fi");
    ngfx_text_clipped(cr, "Noto Sans", 0, 9.5, col0_x + 41, r1_y + 34, t_w - 50, wifi_on ? 0.92 : mut_r, wifi_on ? 0.97 : mut_g, wifi_on ? 0.94 : mut_b, 0.92,
                      !st->svc_network ? "NM stopped" : (st->cfg.wifi_enabled ? "Canopy-5GHz" : "Off"));
    nature_add_hit(st, col0_x, r1_y, t_w, tile_h, HIT_QS_WIFI, 0, 0);

    /* Tile 2: Bluetooth */
    int h_bt = (st->hover_mx >= col1_x && st->hover_mx <= col1_x + t_w && st->hover_my >= r1_y && st->hover_my <= r1_y + tile_h);
    draw_qs_tile_bg(cr, col1_x, r1_y, t_w, tile_h, st->cfg.bluetooth_enabled, h_bt, ar, ag, ab, pal->ar_hi, pal->ag_hi, pal->ab_hi, pal);
    ngfx_fill_circle(cr, col1_x + 21, r1_y + 22, 13.5, st->cfg.bluetooth_enabled ? 1.0 : pal->card_r, st->cfg.bluetooth_enabled ? 1.0 : pal->card_g, st->cfg.bluetooth_enabled ? 1.0 : pal->card_b, st->cfg.bluetooth_enabled ? 0.22 : 0.90);
    double bcol_r = st->cfg.bluetooth_enabled ? 1.0 : fg_r;
    double bcol_g = st->cfg.bluetooth_enabled ? 1.0 : fg_g;
    double bcol_b = st->cfg.bluetooth_enabled ? 1.0 : fg_b;
    g_ngfx.set_source_rgba(cr, bcol_r, bcol_g, bcol_b, 0.96);
    g_ngfx.set_line_width(cr, 1.3);
    g_ngfx.move_to(cr, col1_x + 21, r1_y + 14); g_ngfx.line_to(cr, col1_x + 21, r1_y + 30);
    g_ngfx.line_to(cr, col1_x + 25.5, r1_y + 26); g_ngfx.line_to(cr, col1_x + 17, r1_y + 19);
    g_ngfx.move_to(cr, col1_x + 17, r1_y + 25); g_ngfx.line_to(cr, col1_x + 25.5, r1_y + 18); g_ngfx.line_to(cr, col1_x + 21, r1_y + 14);
    g_ngfx.stroke(cr);
    ngfx_text(cr, "Plus Jakarta Sans", 1, 11.4, col1_x + 41, r1_y + 19, bcol_r, bcol_g, bcol_b, 1.0, "Bluetooth");
    ngfx_text_clipped(cr, "Noto Sans", 0, 9.5, col1_x + 41, r1_y + 34, t_w - 50, st->cfg.bluetooth_enabled ? 0.92 : mut_r, st->cfg.bluetooth_enabled ? 0.97 : mut_g, st->cfg.bluetooth_enabled ? 0.94 : mut_b, 0.92, st->cfg.bluetooth_enabled ? "Connected" : "Off");
    nature_add_hit(st, col1_x, r1_y, t_w, tile_h, HIT_QS_BLUETOOTH, 0, 0);

    /* Tile 3: Appearance */
    int r2_y = r1_y + tile_h + row_gap;
    int h_dk = (st->hover_mx >= col0_x && st->hover_mx <= col0_x + t_w && st->hover_my >= r2_y && st->hover_my <= r2_y + tile_h);
    draw_qs_tile_bg(cr, col0_x, r2_y, t_w, tile_h, st->cfg.dark_mode, h_dk, ar, ag, ab, pal->ar_hi, pal->ag_hi, pal->ab_hi, pal);
    ngfx_fill_circle(cr, col0_x + 21, r2_y + 22, 13.5, st->cfg.dark_mode ? 1.0 : pal->card_r, st->cfg.dark_mode ? 1.0 : pal->card_g, st->cfg.dark_mode ? 1.0 : pal->card_b, st->cfg.dark_mode ? 0.22 : 0.90);
    if (st->cfg.dark_mode) {
        ngfx_fill_circle(cr, col0_x + 20.5, r2_y + 22, 5.5, 1.0, 1.0, 1.0, 0.96);
        ngfx_fill_circle(cr, col0_x + 23.0, r2_y + 20.2, 4.4, ar * 0.85, ag * 0.85, ab * 0.85, 0.98);
    } else {
        draw_sun_glyph(cr, col0_x + 21, r2_y + 22, fg_r, fg_g, fg_b, 0.92);
    }
    ngfx_text(cr, "Plus Jakarta Sans", 1, 11.4, col0_x + 41, r2_y + 19, st->cfg.dark_mode ? 1.0 : fg_r, st->cfg.dark_mode ? 1.0 : fg_g, st->cfg.dark_mode ? 1.0 : fg_b, 1.0, "Appearance");
    ngfx_text_clipped(cr, "Noto Sans", 0, 9.5, col0_x + 41, r2_y + 34, t_w - 50, st->cfg.dark_mode ? 0.92 : mut_r, st->cfg.dark_mode ? 0.97 : mut_g, st->cfg.dark_mode ? 0.94 : mut_b, 0.92, st->cfg.dark_mode ? "Dark Mode" : "Light Mode");
    nature_add_hit(st, col0_x, r2_y, t_w, tile_h, HIT_QS_DARK, 0, 0);

    /* Tile 4: Night Light */
    int h_nl = (st->hover_mx >= col1_x && st->hover_mx <= col1_x + t_w && st->hover_my >= r2_y && st->hover_my <= r2_y + tile_h);
    draw_qs_tile_bg(cr, col1_x, r2_y, t_w, tile_h, st->cfg.night_light, h_nl, 0.92, 0.52, 0.16, 0.99, 0.74, 0.36, pal);
    ngfx_fill_circle(cr, col1_x + 21, r2_y + 22, 13.5, st->cfg.night_light ? 1.0 : pal->card_r, st->cfg.night_light ? 1.0 : pal->card_g, st->cfg.night_light ? 1.0 : pal->card_b, st->cfg.night_light ? 0.22 : 0.90);
    ngfx_fill_circle(cr, col1_x + 21, r2_y + 22, 5.2, st->cfg.night_light ? 1.0 : 0.94, st->cfg.night_light ? 0.96 : 0.64, st->cfg.night_light ? 0.85 : 0.26, 0.95);
    ngfx_text(cr, "Plus Jakarta Sans", 1, 11.4, col1_x + 41, r2_y + 19, st->cfg.night_light ? 1.0 : fg_r, st->cfg.night_light ? 1.0 : fg_g, st->cfg.night_light ? 1.0 : fg_b, 1.0, "Night Light");
    ngfx_text_clipped(cr, "Noto Sans", 0, 9.5, col1_x + 41, r2_y + 34, t_w - 50, st->cfg.night_light ? 0.94 : mut_r, st->cfg.night_light ? 0.96 : mut_g, st->cfg.night_light ? 0.92 : mut_b, 0.92, st->cfg.night_light ? "Warm 3800K" : "Off");
    nature_add_hit(st, col1_x, r2_y, t_w, tile_h, HIT_QS_NIGHT_LIGHT, 0, 0);

    /* Tile 5: Style */
    int r3_y = r2_y + tile_h + row_gap;
    int h_fam = (st->hover_mx >= col0_x && st->hover_mx <= col0_x + t_w && st->hover_my >= r3_y && st->hover_my <= r3_y + tile_h);
    draw_qs_tile_bg(cr, col0_x, r3_y, t_w, tile_h, 0, h_fam, ar, ag, ab, pal->ar_hi, pal->ag_hi, pal->ab_hi, pal);
    ngfx_fill_circle(cr, col0_x + 21, r3_y + 22, 13.5, ar, ag, ab, 0.22);
    ngfx_fill_circle(cr, col0_x + 21, r3_y + 22, 5.5, pal->ar_hi, pal->ag_hi, pal->ab_hi, 1.0);
    ngfx_text(cr, "Plus Jakarta Sans", 1, 11.2, col0_x + 41, r3_y + 19, fg_r, fg_g, fg_b, 1.0, "Style");
    ngfx_text_clipped(cr, "Noto Sans", 0, 9.5, col0_x + 41, r3_y + 34, t_w - 50, dark ? pal->ar_hi : ar, dark ? pal->ag_hi : ag, dark ? pal->ab_hi : ab, 0.98,
                      st->cfg.theme_family == THEME_HORIZON ? "Horizon Slate" : "Nature Canopy");
    nature_add_hit(st, col0_x, r3_y, t_w, tile_h, HIT_QS_THEME_FAMILY, 0, 0);

    /* Tile 6: Tile Windows */
    int h_tile = (st->hover_mx >= col1_x && st->hover_mx <= col1_x + t_w && st->hover_my >= r3_y && st->hover_my <= r3_y + tile_h);
    draw_qs_tile_bg(cr, col1_x, r3_y, t_w, tile_h, 0, h_tile, ar, ag, ab, pal->ar_hi, pal->ag_hi, pal->ab_hi, pal);
    ngfx_fill_circle(cr, col1_x + 21, r3_y + 22, 13.5, pal->card_r, pal->card_g, pal->card_b, 0.90);
    ngfx_stroke_rect(cr, col1_x + 15.5, r3_y + 16.5, 11, 11, 2.0, 1.2, fg_r, fg_g, fg_b, 0.88);
    g_ngfx.set_source_rgba(cr, fg_r, fg_g, fg_b, 0.88);
    g_ngfx.set_line_width(cr, 1.1);
    g_ngfx.move_to(cr, col1_x + 21.0, r3_y + 16.5); g_ngfx.line_to(cr, col1_x + 21.0, r3_y + 27.5);
    g_ngfx.stroke(cr);
    ngfx_text(cr, "Plus Jakarta Sans", 1, 11.2, col1_x + 41, r3_y + 19, fg_r, fg_g, fg_b, 1.0, "Tile Grid");
    ngfx_text(cr, "Noto Sans", 0, 9.5, col1_x + 41, r3_y + 34, mut_r, mut_g, mut_b, 0.92, "Snap Visible");
    nature_add_hit(st, col1_x, r3_y, t_w, tile_h, HIT_QS_TILE, 0, 0);

    /* 3. Recessed Power & Accent Card */
    int pa_y = r3_y + tile_h + 7;
    int pa_h = 74;
    ngfx_fill_rect(cr, px + 12, pa_y, pw - 24, pa_h, 12, pal->elev_r, pal->elev_g, pal->elev_b, 0.72);
    ngfx_stroke_rect(cr, px + 12.5, pa_y + 0.5, pw - 25, pa_h - 1, 12, 0.8, pal->border_r, pal->border_g, pal->border_b, pal->border_a);

    const char *pmodes[3] = {"Saver", "Balanced", "Perf"};
    ngfx_text(cr, "Plus Jakarta Sans", 1, 10.6, px + 22, pa_y + 23, fg_r, fg_g, fg_b, 0.94, "Power");
    int seg_x = px + 74, seg_w = (pw - 98) / 3;
    ngfx_fill_rect(cr, seg_x - 2, pa_y + 7, seg_w * 3 + 4, 24, 8, pal->card_r, pal->card_g, pal->card_b, 0.85);
    for (int m = 0; m < 3; ++m) {
        int act = (st->cfg.power_profile % 3) == m;
        int sx = seg_x + m * seg_w;
        int m_hov = (st->hover_mx >= sx && st->hover_mx <= sx + seg_w && st->hover_my >= pa_y + 7 && st->hover_my <= pa_y + 31);
        if (act) ngfx_fill_gradient(cr, sx, pa_y + 9, seg_w - 1, 20, 6, ar, ag, ab, 0.95, pal->ar_hi, pal->ag_hi, pal->ab_hi, 0.95, 1);
        else if (m_hov) ngfx_fill_rect(cr, sx, pa_y + 9, seg_w - 1, 20, 6, pal->hover_r, pal->hover_g, pal->hover_b, 0.90);
        double mw = ngfx_text_width(cr, "Noto Sans", act, 9.8, pmodes[m]);
        ngfx_text(cr, "Noto Sans", act, 9.8, sx + ((seg_w - 1) - mw) * 0.5, pa_y + 22.5,
                  act ? 1.0 : (m_hov ? fg_r : mut_r), act ? 1.0 : (m_hov ? fg_g : mut_g), act ? 1.0 : (m_hov ? fg_b : mut_b), 1.0, pmodes[m]);
    }
    nature_add_hit(st, seg_x - 2, pa_y + 6, seg_w * 3 + 4, 26, HIT_QS_PERF, 0, 0);

    static const char *QS_ACC_N[6] = {"Canopy", "Teal", "Amber", "Alpine", "Orchid", "Coral"};
    static const char *QS_ACC_H[6] = {"Horizon", "Indigo", "Cyan", "Violet", "Rose", "Amber"};
    const char *aname = (st->cfg.theme_family == THEME_HORIZON ? QS_ACC_H : QS_ACC_N)[st->cfg.accent_idx % 6];
    ngfx_text(cr, "Plus Jakarta Sans", 1, 10.4, px + 22, pa_y + 57, mut_r, mut_g, mut_b, 0.95, "Accent");
    for (int a = 0; a < 6; ++a) {
        double sr, sg, sb;
        nature_get_accent_ex(st->cfg.theme_family, a, dark, &sr, &sg, &sb);
        int cx = px + 84 + a * 30;
        int cy = pa_y + 53;
        int sel = (st->cfg.accent_idx % 6) == a;
        int ahov = (st->hover_mx >= cx - 12 && st->hover_mx <= cx + 12 && st->hover_my >= cy - 12 && st->hover_my <= cy + 12);
        if (sel || ahov) {
            ngfx_fill_circle(cr, cx, cy, 11.2, sr, sg, sb, sel ? 0.28 : 0.16);
            if (sel) ngfx_stroke_rect(cr, cx - 10.0, cy - 10.0, 20, 20, 10, 1.5, fg_r, fg_g, fg_b, 0.90);
        }
        ngfx_fill_circle(cr, cx, cy, 7.6, sr, sg, sb, 1.0);
        nature_add_hit(st, cx - 13, cy - 13, 26, 26, HIT_QS_ACCENT, a, 0);
    }
    ngfx_fill_rect(cr, px + pw - 74, pa_y + 44, 52, 18, 9, ar, ag, ab, 0.20);
    double aw_t = ngfx_text_width(cr, "JetBrains Mono", 1, 8.6, aname);
    ngfx_text(cr, "JetBrains Mono", 1, 8.6, px + pw - 74 + (52 - aw_t) * 0.5, pa_y + 56,
              dark ? pal->ar_hi : ar, dark ? pal->ag_hi : ag, dark ? pal->ab_hi : ab, 0.98, aname);

    /* 4. Recessed Audio & Display Card */
    int ad_y = pa_y + pa_h + 7;
    int ad_h = 98;
    ngfx_fill_rect(cr, px + 12, ad_y, pw - 24, ad_h, 12, pal->elev_r, pal->elev_g, pal->elev_b, 0.72);
    ngfx_stroke_rect(cr, px + 12.5, ad_y + 0.5, pw - 25, ad_h - 1, 12, 0.8, pal->border_r, pal->border_g, pal->border_b, pal->border_a);

    int sl_x = px + 22, sl_w = pw - 44;
    draw_speaker_glyph(cr, sl_x, ad_y + 16, st->cfg.audio_muted, fg_r, fg_g, fg_b, 0.92);
    ngfx_text(cr, "Plus Jakarta Sans", 1, 10.4, sl_x + 18, ad_y + 20, fg_r, fg_g, fg_b, 0.96,
              st->cfg.audio_muted ? "Muted" : "Audio");

    static const char *QS_SINK_SHORT[3] = {"Analog", "HDMI", "BT Buds"};
    int sink_x = px + pw - 156;
    int h_sink = (st->hover_mx >= sink_x && st->hover_mx <= sink_x + 54 && st->hover_my >= ad_y + 7 && st->hover_my <= ad_y + 25);
    ngfx_fill_rect(cr, sink_x, ad_y + 8, 54, 17, 6, pal->card_r, pal->card_g, pal->card_b, h_sink ? 0.98 : 0.82);
    ngfx_stroke_rect(cr, sink_x + 0.5, ad_y + 8.5, 53, 16, 6, 0.7, pal->border_r, pal->border_g, pal->border_b, pal->border_a);
    ngfx_text_clipped(cr, "JetBrains Mono", 1, 8.6, sink_x + 6, ad_y + 19.5, 44, pal->fg_sec_r, pal->fg_sec_g, pal->fg_sec_b, 0.95,
                      QS_SINK_SHORT[st->cfg.audio_device % 3]);
    nature_add_hit(st, sink_x, ad_y + 6, 54, 21, HIT_QS_AUDIO_DEV, 0, 0);

    int h_mute = (st->hover_mx >= px + pw - 98 && st->hover_mx <= px + pw - 56 && st->hover_my >= ad_y + 7 && st->hover_my <= ad_y + 25);
    ngfx_fill_rect(cr, px + pw - 98, ad_y + 8, 40, 17, 6,
                   st->cfg.audio_muted ? pal->danger_r : pal->card_r,
                   st->cfg.audio_muted ? pal->danger_g : pal->card_g,
                   st->cfg.audio_muted ? pal->danger_b : pal->card_b, h_mute ? 0.98 : 0.82);
    ngfx_text(cr, "JetBrains Mono", 1, 8.6, px + pw - 90, ad_y + 19.5,
              st->cfg.audio_muted ? 1.0 : pal->fg_sec_r, st->cfg.audio_muted ? 1.0 : pal->fg_sec_g, st->cfg.audio_muted ? 1.0 : pal->fg_sec_b, 0.95,
              st->cfg.audio_muted ? "UNMT" : "MUTE");
    nature_add_hit(st, px + pw - 98, ad_y + 6, 42, 21, HIT_QS_MUTE, 0, 0);

    char vb[16];
    snprintf(vb, sizeof(vb), "%d%%", st->cfg.volume);
    ngfx_text(cr, "JetBrains Mono", 1, 10.2, px + pw - 50, ad_y + 20, fg_r, fg_g, fg_b, 0.95, vb);

    ngfx_fill_rect(cr, sl_x, ad_y + 31, sl_w, 10, 5, dark ? 1 : 0, dark ? 1 : 0, dark ? 1 : 0, 0.16);
    double vol_w = (sl_w * (st->cfg.audio_muted ? 0 : st->cfg.volume)) / 100.0;
    if (vol_w > 0) {
        ngfx_fill_gradient(cr, sl_x, ad_y + 31, vol_w, 10, 5, ar, ag, ab, 0.96, pal->ar_hi, pal->ag_hi, pal->ab_hi, 0.98, 1);
    }
    ngfx_fill_circle(cr, sl_x + vol_w, ad_y + 37, 8.2, 0.0, 0.0, 0.0, 0.25);
    ngfx_fill_circle(cr, sl_x + vol_w, ad_y + 36, 7.6, 1, 1, 1, 0.98);
    ngfx_fill_circle(cr, sl_x + vol_w, ad_y + 36, 2.8, ar, ag, ab, 0.95);
    nature_add_hit(st, sl_x, ad_y + 25, sl_w, 24, HIT_SLIDER_VOL, sl_x, sl_w);

    /* Brightness row */
    draw_sun_glyph(cr, sl_x + 6, ad_y + 62, 0.94, 0.72, 0.22, 0.96);
    ngfx_text(cr, "Plus Jakarta Sans", 1, 10.4, sl_x + 18, ad_y + 66, fg_r, fg_g, fg_b, 0.96, "Brightness");
    char bb[16];
    snprintf(bb, sizeof(bb), "%d%%", st->cfg.brightness);
    ngfx_text(cr, "JetBrains Mono", 1, 10.2, px + pw - 50, ad_y + 66, fg_r, fg_g, fg_b, 0.95, bb);

    ngfx_fill_rect(cr, sl_x, ad_y + 77, sl_w, 10, 5, dark ? 1 : 0, dark ? 1 : 0, dark ? 1 : 0, 0.16);
    double br_w = (sl_w * st->cfg.brightness) / 100.0;
    if (br_w > 0) ngfx_fill_gradient(cr, sl_x, ad_y + 77, br_w, 10, 5, 0.88, 0.65, 0.18, 0.96, 0.98, 0.84, 0.34, 0.98, 1);
    ngfx_fill_circle(cr, sl_x + br_w, ad_y + 83, 8.2, 0.0, 0.0, 0.0, 0.25);
    ngfx_fill_circle(cr, sl_x + br_w, ad_y + 82, 7.6, 1, 1, 1, 0.98);
    ngfx_fill_circle(cr, sl_x + br_w, ad_y + 82, 2.8, 0.94, 0.72, 0.22, 0.95);
    nature_add_hit(st, sl_x, ad_y + 71, sl_w, 24, HIT_SLIDER_BRIGHT, sl_x, sl_w);

    /* 5. Now Playing Strip with Album Art Thumbnail */
    int mp_y = ad_y + ad_h + 7;
    if (mp_y + 44 <= py + ph - 32) {
        ngfx_fill_rect(cr, px + 12, mp_y, pw - 24, 42, 11, pal->elev_r, pal->elev_g, pal->elev_b, 0.86);
        ngfx_stroke_rect(cr, px + 12.5, mp_y + 0.5, pw - 25, 41, 11, 0.9, pal->border_r, pal->border_g, pal->border_b, pal->border_a);

        cairo_surface_t *art = st->media_images[(st->audio_selected + 1) % MAX_MEDIA_IMAGES];
        if (art) {
            double aw = g_ngfx.image_surface_get_width(art);
            double ah = g_ngfx.image_surface_get_height(art);
            if (aw > 0 && ah > 0) {
                g_ngfx.save(cr);
                ngfx_rounded_path(cr, px + 18, mp_y + 6, 30, 30, 7);
                g_ngfx.clip(cr);
                double sc = fmax(30.0 / aw, 30.0 / ah);
                g_ngfx.translate(cr, px + 18, mp_y + 6);
                g_ngfx.scale(cr, sc, sc);
                g_ngfx.set_source_surface(cr, art, 0, 0);
                g_ngfx.paint(cr);
                g_ngfx.restore(cr);
                ngfx_stroke_rect(cr, px + 18, mp_y + 6, 30, 30, 7, 0.8, ar, ag, ab, 0.55);
            }
        } else {
            ngfx_fill_rect(cr, px + 18, mp_y + 6, 30, 30, 7, ar, ag, ab, 0.25);
        }

        ngfx_text_clipped(cr, "Plus Jakarta Sans", 1, 10.6, px + 56, mp_y + 18, pw - 138, fg_r, fg_g, fg_b, 0.98,
                          QS_AUDIO_TITLES[st->audio_selected % MAX_AUDIO_TRACKS]);
        ngfx_text_clipped(cr, "JetBrains Mono", 0, 8.6, px + 56, mp_y + 32, pw - 138,
                          st->audio_playing ? pal->success_r : mut_r,
                          st->audio_playing ? pal->success_g : mut_g,
                          st->audio_playing ? pal->success_b : mut_b, 0.92,
                          st->audio_playing ? "Playing · PipeWire Audio" : "Paused · Tap play");

        int pbtn_x = px + pw - 78;
        ngfx_fill_rect(cr, pbtn_x, mp_y + 8, 28, 26, 8, ar, ag, ab, 0.94);
        if (st->audio_playing) {
            ngfx_fill_rect(cr, pbtn_x + 10, mp_y + 15, 2.8, 12, 1, 1, 1, 1, 0.96);
            ngfx_fill_rect(cr, pbtn_x + 15.2, mp_y + 15, 2.8, 12, 1, 1, 1, 1, 0.96);
        } else {
            g_ngfx.set_source_rgba(cr, 1, 1, 1, 0.96);
            g_ngfx.move_to(cr, pbtn_x + 11, mp_y + 15);
            g_ngfx.line_to(cr, pbtn_x + 20, mp_y + 21);
            g_ngfx.line_to(cr, pbtn_x + 11, mp_y + 27);
            g_ngfx.close_path(cr);
            g_ngfx.fill(cr);
        }
        nature_add_hit(st, pbtn_x, mp_y + 7, 28, 28, HIT_QS_MEDIA_TOGGLE, 0, 0);

        ngfx_fill_rect(cr, pbtn_x + 32, mp_y + 8, 28, 26, 8, pal->card_r, pal->card_g, pal->card_b, 0.96);
        ngfx_stroke_rect(cr, pbtn_x + 32.5, mp_y + 8.5, 27, 25, 8, 0.8, pal->border_r, pal->border_g, pal->border_b, pal->border_a);
        ngfx_text(cr, "JetBrains Mono", 1, 9.2, pbtn_x + 39, mp_y + 24, fg_r, fg_g, fg_b, 0.95, ">|");
        nature_add_hit(st, pbtn_x + 32, mp_y + 7, 28, 28, HIT_QS_MEDIA_NEXT, 0, 0);
    }

    /* 6. Compact Footer Telemetry + DND Capsule */
    int f_y = py + ph - 29;
    char qs_stat[64];
    long up_m = st->uptime_sec / 60;
    snprintf(qs_stat, sizeof(qs_stat), "CPU %.1f%% · %d/%dM · Up %ldh%ldm",
             st->total_cpu_pct, st->used_ram_mb, st->total_ram_mb, up_m / 60, up_m % 60);
    ngfx_text_clipped(cr, "JetBrains Mono", 0, 9.2, px + 14, f_y + 14, pw - 104, mut_r, mut_g, mut_b, 0.95, qs_stat);

    int dnd_on = !st->cfg.show_toast_popups;
    int dnd_x = px + pw - 82;
    ngfx_fill_rect(cr, dnd_x, f_y + 1, 70, 20, 10,
                   dnd_on ? pal->warning_r : pal->elev_r,
                   dnd_on ? pal->warning_g : pal->elev_g,
                   dnd_on ? pal->warning_b : pal->elev_b, dnd_on ? 0.90 : 0.85);
    ngfx_stroke_rect(cr, dnd_x + 0.5, f_y + 1.5, 69, 19, 10, 0.7, pal->border_r, pal->border_g, pal->border_b, pal->border_a);
    ngfx_text(cr, "JetBrains Mono", 1, 8.6, dnd_x + 9, f_y + 14,
              dnd_on ? 1.0 : pal->fg_sec_r, dnd_on ? 1.0 : pal->fg_sec_g, dnd_on ? 1.0 : pal->fg_sec_b, 0.96,
              dnd_on ? "DND On" : "Alerts On");
    nature_add_hit(st, dnd_x, f_y, 70, 22, HIT_QS_DND, 0, 0);
}

/* --- Calendar & Notification Feed Popover --- */

void render_calendar_popover(NatureOSState *st, cairo_t *cr, const NaturePalette *pal) {
    int sw = st->screen_w;
    int pw = sw < 380 ? sw - 20 : 356;
    int ph = 372;
    int px = (sw - pw) / 2;
    int py = 48;

    nature_occlude_text_rects(st, px, py, pw, ph);

    int dark = pal->dark_mode;
    double ar = pal->ar, ag = pal->ag, ab = pal->ab;
    double fg_r = pal->fg_r, fg_g = pal->fg_g, fg_b = pal->fg_b;
    double mut_r = pal->mut_r, mut_g = pal->mut_g, mut_b = pal->mut_b;

    ngfx_shadow_rect(cr, px, py, pw, ph, 18, 5.0, dark ? 0.60 : 0.26);
    ngfx_fill_gradient(cr, px, py, pw, ph, 18,
                       pal->card_r, pal->card_g, pal->card_b, 0.985,
                       pal->win_bg_r, pal->win_bg_g, pal->win_bg_b, 0.985, 0);
    ngfx_stroke_rect(cr, px + 0.5, py + 0.5, pw - 1, ph - 1, 18, 1.0,
                     pal->strong_brd_r, pal->strong_brd_g, pal->strong_brd_b, pal->strong_brd_a);
    ngfx_top_specular(cr, px, py, pw, 18, dark ? 0.28 : 0.75);

    time_t raw_t = time(NULL);
    struct tm tm_buf;
    const struct tm *tm_info = localtime_r(&raw_t, &tm_buf);
    if (!tm_info) tm_info = &tm_buf;

    struct tm tgt = *tm_info;
    tgt.tm_mday = 1;
    tgt.tm_hour = 12;
    tgt.tm_mon += st->cal_month_offset;
    mktime(&tgt);

    char hdr_date[64], live_tm[24];
    format_clock_str(st, tm_info, 0, live_tm, sizeof(live_tm));
    if (st->cal_month_offset == 0) {
        strftime(hdr_date, sizeof(hdr_date), "%A, %B %d", tm_info);
    } else {
        strftime(hdr_date, sizeof(hdr_date), "%B %Y", &tgt);
    }
    ngfx_text_clipped(cr, "Plus Jakarta Sans", 1, 13.4, px + 16, py + 24, pw - 134, fg_r, fg_g, fg_b, 1.0, hdr_date);
    ngfx_text(cr, "JetBrains Mono", 0, 9.6, px + 16, py + 38, dark ? pal->ar_hi : ar, dark ? pal->ag_hi : ag, dark ? pal->ab_hi : ab, 0.92, live_tm);

    /* Month navigation controls (< Today >) */
    int nav_x = px + pw - 114;
    int h_prev = (st->hover_mx >= nav_x && st->hover_mx <= nav_x + 24 && st->hover_my >= py + 12 && st->hover_my <= py + 36);
    ngfx_fill_rect(cr, nav_x, py + 12, 24, 24, 8, h_prev ? pal->hover_r : pal->elev_r, h_prev ? pal->hover_g : pal->elev_g, h_prev ? pal->hover_b : pal->elev_b, 0.95);
    ngfx_stroke_rect(cr, nav_x + 0.5, py + 12.5, 23, 23, 8, 0.8, h_prev ? ar : pal->border_r, h_prev ? ag : pal->border_g, h_prev ? ab : pal->border_b, h_prev ? 0.72 : pal->border_a);
    ngfx_text(cr, "JetBrains Mono", 1, 10.5, nav_x + 8, py + 28, fg_r, fg_g, fg_b, 0.95, "<");
    nature_add_hit(st, nav_x, py + 12, 24, 24, HIT_CAL_MONTH_NAV, -1, 0);

    int h_today = (st->hover_mx >= nav_x + 28 && st->hover_mx <= nav_x + 74 && st->hover_my >= py + 12 && st->hover_my <= py + 36);
    ngfx_fill_rect(cr, nav_x + 28, py + 12, 46, 24, 8,
                   st->cal_month_offset == 0 ? ar : (h_today ? pal->hover_r : pal->elev_r),
                   st->cal_month_offset == 0 ? ag : (h_today ? pal->hover_g : pal->elev_g),
                   st->cal_month_offset == 0 ? ab : (h_today ? pal->hover_b : pal->elev_b), 0.94);
    ngfx_text(cr, "Noto Sans", 1, 9.4, nav_x + 36, py + 27.5,
              st->cal_month_offset == 0 ? pal->on_acc_r : fg_r,
              st->cal_month_offset == 0 ? pal->on_acc_g : fg_g,
              st->cal_month_offset == 0 ? pal->on_acc_b : fg_b, 0.98, "Today");
    nature_add_hit(st, nav_x + 28, py + 12, 46, 24, HIT_CAL_MONTH_NAV, 0, 0);

    int h_next = (st->hover_mx >= nav_x + 78 && st->hover_mx <= nav_x + 102 && st->hover_my >= py + 12 && st->hover_my <= py + 36);
    ngfx_fill_rect(cr, nav_x + 78, py + 12, 24, 24, 8, h_next ? pal->hover_r : pal->elev_r, h_next ? pal->hover_g : pal->elev_g, h_next ? pal->hover_b : pal->elev_b, 0.95);
    ngfx_stroke_rect(cr, nav_x + 78.5, py + 12.5, 23, 23, 8, 0.8, h_next ? ar : pal->border_r, h_next ? ag : pal->border_g, h_next ? ab : pal->border_b, h_next ? 0.72 : pal->border_a);
    ngfx_text(cr, "JetBrains Mono", 1, 10.5, nav_x + 86, py + 28, fg_r, fg_g, fg_b, 0.95, ">");
    nature_add_hit(st, nav_x + 78, py + 12, 24, 24, HIT_CAL_MONTH_NAV, 1, 0);

    /* Framed Calendar Month Grid Card */
    int cal_y = py + 46, cal_h = 152;
    ngfx_fill_rect(cr, px + 14, cal_y, pw - 28, cal_h, 12, pal->elev_r, pal->elev_g, pal->elev_b, 0.62);
    ngfx_stroke_rect(cr, px + 14.5, cal_y + 0.5, pw - 29, cal_h - 1, 12, 0.8, pal->border_r, pal->border_g, pal->border_b, pal->border_a);

    const char *wdays[7] = {"Su", "Mo", "Tu", "We", "Th", "Fr", "Sa"};
    int cw = (pw - 40) / 7;
    for (int d = 0; d < 7; ++d) {
        ngfx_text(cr, "JetBrains Mono", 1, 9.2, px + 21 + d * cw + 7, cal_y + 18, mut_r, mut_g, mut_b, 0.82, wdays[d]);
    }
    int today = (st->cal_month_offset == 0) ? tm_info->tm_mday : -1;
    int first_wday = tgt.tm_wday;
    int days_in_mo = month_days_for(tgt.tm_mon, tgt.tm_year);
    for (int d = 1; d <= days_in_mo; ++d) {
        int slot = first_wday + (d - 1);
        int col = slot % 7;
        int row = slot / 7;
        int dx = px + 21 + col * cw;
        int dy = cal_y + 25 + row * 20;
        if (d == today) {
            ngfx_radial_glow(cr, dx + cw * 0.45, dy + 9, 2.0, 15.0, ar, ag, ab, 0.36, 0.0);
            ngfx_fill_rect(cr, dx + 2, dy, cw - 6, 18, 7, ar, ag, ab, 0.96);
        }
        char db[16];
        snprintf(db, sizeof(db), "%2d", d);
        ngfx_text(cr, "JetBrains Mono", d == today, 9.8, dx + 7, dy + 12.5,
                  d == today ? 1.0 : fg_r, d == today ? 1.0 : fg_g, d == today ? 1.0 : fg_b,
                  d == today ? 1.0 : 0.84, db);
    }

    /* Notification Feed */
    int ny = cal_y + cal_h + 10;
    ngfx_text(cr, "Plus Jakarta Sans", 1, 11.4, px + 16, ny + 16, fg_r, fg_g, fg_b, 1.0, "Notifications");
    if (st->notif_count > 0) {
        char ncnt[12];
        snprintf(ncnt, sizeof(ncnt), "%d", st->notif_count);
        ngfx_fill_rect(cr, px + 102, ny + 3, 20, 16, 8, ar, ag, ab, 0.25);
        ngfx_text(cr, "JetBrains Mono", 1, 8.8, px + 109, ny + 14.5, dark ? pal->ar_hi : ar, dark ? pal->ag_hi : ag, dark ? pal->ab_hi : ab, 0.98, ncnt);
    }
    ngfx_fill_rect(cr, px + pw - 122, ny + 1, 46, 21, 7, ar, ag, ab, 0.22);
    ngfx_stroke_rect(cr, px + pw - 121.5, ny + 1.5, 45, 20, 7, 0.8, ar, ag, ab, 0.48);
    ngfx_text(cr, "Noto Sans", 1, 9.4, px + pw - 110, ny + 15, dark ? pal->ar_hi : ar, dark ? pal->ag_hi : ag, dark ? pal->ab_hi : ab, 0.98, "Test");
    nature_add_hit(st, px + pw - 122, ny + 1, 46, 21, HIT_CAL_TEST_NOTIF, 0, 0);

    if (st->notif_count > 0) {
        ngfx_fill_rect(cr, px + pw - 70, ny + 1, 54, 21, 7, pal->elev_r, pal->elev_g, pal->elev_b, 0.92);
        ngfx_stroke_rect(cr, px + pw - 69.5, ny + 1.5, 53, 20, 7, 0.7, pal->border_r, pal->border_g, pal->border_b, pal->border_a);
        ngfx_text(cr, "Noto Sans", 1, 9.4, px + pw - 57, ny + 15, fg_r, fg_g, fg_b, 0.92, "Clear");
        nature_add_hit(st, px + pw - 70, ny + 1, 54, 21, HIT_CAL_CLEAR_NOTIFS, 0, 0);
    }

    int ly = ny + 28;
    if (st->notif_count == 0) {
        ngfx_fill_rect(cr, px + 14, ly + 4, pw - 28, 72, 11, pal->elev_r, pal->elev_g, pal->elev_b, 0.50);
        ngfx_stroke_rect(cr, px + 14.5, ly + 4.5, pw - 29, 71, 11, 0.8, pal->border_r, pal->border_g, pal->border_b, pal->border_a);
        ngfx_text(cr, "Noto Sans", 0, 11.0, px + 78, ly + 44, mut_r, mut_g, mut_b, 0.82, "All caught up — no unread alerts");
    } else {
        int show = st->notif_count < 3 ? st->notif_count : 3;
        for (int i = 0; i < show; ++i) {
            ngfx_fill_rect(cr, px + 14, ly, pw - 28, 40, 10, pal->elev_r, pal->elev_g, pal->elev_b, 0.72);
            ngfx_stroke_rect(cr, px + 14.5, ly + 0.5, pw - 29, 39, 10, 0.8, pal->border_r, pal->border_g, pal->border_b, pal->border_a);
            ngfx_fill_rect(cr, px + 20, ly + 8, 3, 24, 1.5, pal->ar_hi, pal->ag_hi, pal->ab_hi, 0.96);
            ngfx_text_clipped(cr, "Plus Jakarta Sans", 1, 10.4, px + 30, ly + 16, pw - 134,
                              pal->ar_hi, pal->ag_hi, pal->ab_hi, 1.0, st->notif_titles[i]);
            if (st->notif_times[i][0]) {
                double tw = ngfx_text_width(cr, "JetBrains Mono", 0, 8.6, st->notif_times[i]);
                ngfx_text(cr, "JetBrains Mono", 0, 8.6, px + pw - 46 - tw, ly + 16, mut_r, mut_g, mut_b, 0.85, st->notif_times[i]);
            }
            ngfx_fill_rect(cr, px + pw - 40, ly + 6, 18, 18, 6, pal->card_r, pal->card_g, pal->card_b, 0.86);
            ngfx_text(cr, "Noto Sans", 1, 9.2, px + pw - 34.5, ly + 18, mut_r, mut_g, mut_b, 0.92, "x");
            nature_add_hit(st, px + pw - 42, ly + 4, 22, 22, HIT_CAL_DISMISS_NOTIF, i, 0);

            ngfx_text_clipped(cr, "Noto Sans", 0, 10.0, px + 30, ly + 32, pw - 68, mut_r, mut_g, mut_b, 0.95, st->notif_msgs[i]);
            ly += 44;
        }
    }
}

/* --- Desktop Right-Click Context Menu --- */

void render_desktop_ctx_menu(NatureOSState *st, cairo_t *cr, const NaturePalette *pal) {
    int sw = st->screen_w;
    int sh = st->screen_h > 0 ? st->screen_h : OS_HEIGHT;
    int mw = 228, mh = 288;
    int mx = st->ctx_menu_x;
    int my = st->ctx_menu_y;
    if (mx + mw > sw - 12) mx = sw - mw - 12;
    if (my + mh > sh - 80) my = sh - mh - 80;
    if (mx < 10) mx = 10;
    if (my < 46) my = 46;

    nature_occlude_text_rects(st, mx, my, mw, mh);

    int dark = pal->dark_mode;
    double ar = pal->ar, ag = pal->ag, ab = pal->ab;

    ngfx_shadow_rect(cr, mx, my, mw, mh, 13, 4.0, dark ? 0.55 : 0.25);
    ngfx_fill_gradient(cr, mx, my, mw, mh, 13, pal->card_r, pal->card_g, pal->card_b, 0.97, pal->win_bg_r, pal->win_bg_g, pal->win_bg_b, 0.97, 0);
    ngfx_stroke_rect(cr, mx + 0.5, my + 0.5, mw - 1, mh - 1, 13, 1.0, pal->strong_brd_r, pal->strong_brd_g, pal->strong_brd_b, pal->strong_brd_a);
    ngfx_top_specular(cr, mx, my, mw, 13, dark ? 0.24 : 0.70);

    const char *c_labels[8] = {
        "Open Terminal Here",
        "Open Files Manager",
        "Tile Visible Windows",
        "Show Desktop",
        dark ? "Switch to Light Mode" : "Switch to Dark Mode",
        st->cfg.theme_family == THEME_HORIZON ? "Use Nature Canopy" : "Use Horizon Slate",
        "Next Wallpaper",
        "Lock Session"
    };
    const char *c_tags[8] = {">_", "FS", "TL", "SD", "MD", "TH", "WP", "LK"};
    for (int i = 0; i < 8; ++i) {
        int ry = my + 8 + i * 34;
        int hov = (st->hover_mx >= mx + 6 && st->hover_mx <= mx + mw - 6 && st->hover_my >= ry && st->hover_my <= ry + 29);
        if (hov) ngfx_fill_rect(cr, mx + 6, ry, mw - 12, 29, 8, ar, ag, ab, 0.88);
        ngfx_text(cr, "JetBrains Mono", 1, 9.5, mx + 14, ry + 18.5,
                  hov ? 1.0 : pal->ar_hi, hov ? 1.0 : pal->ag_hi, hov ? 1.0 : pal->ab_hi, 0.92, c_tags[i]);
        ngfx_text(cr, "Noto Sans", hov, 11.4, mx + 38, ry + 19,
                  hov ? 1.0 : pal->fg_r, hov ? 1.0 : pal->fg_g, hov ? 1.0 : pal->fg_b, 1.0, c_labels[i]);
        nature_add_hit(st, mx + 6, ry, mw - 12, 29, HIT_CTX_MENU_ITEM, i, 0);
    }
}

/* --- Activities / Spotlight Overview Overlay --- */

void render_launcher_overlay(NatureOSState *st, cairo_t *cr, const NaturePalette *pal) {
    int sw = st->screen_w;
    int sh = st->screen_h > 0 ? st->screen_h : OS_HEIGHT;
    int dark = pal->dark_mode;
    double ar = pal->ar, ag = pal->ag, ab = pal->ab;

    st->text_rect_count = 0;
    ngfx_fill_rect(cr, 0, 0, sw, sh, 0, pal->sidebar_r, pal->sidebar_g, pal->sidebar_b, dark ? 0.88 : 0.76);
    ngfx_radial_glow(cr, sw * 0.5, sh * 0.35, 30.0, (sw > sh ? sw : sh) * 0.55, ar, ag, ab, 0.16, 0.0);

    /* Workspace Switcher */
    int wsw = sw < 380 ? sw - 24 : 336, wsh = 38;
    int wsx = (sw - wsw) / 2, wsy = 48;
    ngfx_fill_rect(cr, wsx, wsy, wsw, wsh, 13, pal->card_r, pal->card_g, pal->card_b, 0.95);
    ngfx_stroke_rect(cr, wsx + 0.5, wsy + 0.5, wsw - 1, wsh - 1, 13, 1.0, pal->border_r, pal->border_g, pal->border_b, pal->border_a);
    int bw = (wsw - 22) / 2;
    for (int ws = 0; ws < 2; ++ws) {
        int bx = wsx + 7 + ws * (bw + 8);
        int act = st->active_workspace == ws;
        ngfx_fill_rect(cr, bx, wsy + 5, bw, 28, 9,
                       act ? ar : pal->elev_r, act ? ag : pal->elev_g, act ? ab : pal->elev_b,
                       act ? 0.92 : 0.72);
        char lbl[32];
        snprintf(lbl, sizeof(lbl), "Workspace %d", ws + 1);
        double tw = ngfx_text_width(cr, "Plus Jakarta Sans", 1, 11.5, lbl);
        ngfx_text(cr, "Plus Jakarta Sans", 1, 11.5, bx + (bw - tw) * 0.5, wsy + 23,
                  act ? 1.0 : pal->fg_sec_r, act ? 1.0 : pal->fg_sec_g, act ? 1.0 : pal->fg_sec_b, act ? 1.0 : 0.85, lbl);
        nature_add_hit(st, bx, wsy + 5, bw, 28, HIT_PANEL_WS, ws, 0);
    }

    /* Spotlight Search Box */
    int s_w = sw < 480 ? sw - 28 : 450, s_h = 40;
    int sx = (sw - s_w) / 2, sy = wsy + 48;
    ngfx_shadow_rect(cr, sx, sy, s_w, s_h, 20, 3.0, dark ? 0.35 : 0.16);
    ngfx_fill_rect(cr, sx, sy, s_w, s_h, 20, pal->card_r, pal->card_g, pal->card_b, 0.97);
    ngfx_stroke_rect(cr, sx + 0.5, sy + 0.5, s_w - 1, s_h - 1, 20, 1.3, ar, ag, ab, 0.82);
    g_ngfx.set_source_rgba(cr, pal->ar_hi, pal->ag_hi, pal->ab_hi, 0.92);
    g_ngfx.set_line_width(cr, 1.6);
    g_ngfx.new_sub_path(cr);
    g_ngfx.arc(cr, sx + 21, sy + 18, 5.5, 0, 6.283);
    g_ngfx.stroke(cr);
    g_ngfx.move_to(cr, sx + 25, sy + 22);
    g_ngfx.line_to(cr, sx + 29, sy + 26);
    g_ngfx.stroke(cr);

    if (st->launcher_query[0]) {
        ngfx_text_clipped(cr, "Plus Jakarta Sans", 1, 13.0, sx + 40, sy + 24.5, s_w - 60, pal->fg_r, pal->fg_g, pal->fg_b, 1.0, st->launcher_query);
    } else {
        ngfx_text_clipped(cr, "Noto Sans", 0, 12.2, sx + 40, sy + 24.5, s_w - 60, pal->mut_r, pal->mut_g, pal->mut_b, 0.92,
                          "Spotlight search: apps, files, settings or commands...");
    }
    nature_add_hit(st, sx, sy, s_w, s_h, HIT_LAUNCHER_SEARCH, 0, 0);
    nature_add_text_rect(st, sx, sy, s_w, s_h, 2);

    int thumb_y = sy + 50;
    int open_count = 0;
    for (int w = 0; w < st->window_count; ++w) {
        if (st->windows[w].workspace == st->active_workspace) open_count++;
    }
    if (open_count > 0 && !st->launcher_query[0] && sw >= 580) {
        int tw = 192, th = 84;
        int max_thumbs = (sw - 40) / (tw + 14);
        if (max_thumbs < 1) max_thumbs = 1;
        int show_n = open_count < max_thumbs ? open_count : max_thumbs;
        int total_w = show_n * tw + (show_n - 1) * 14;
        int start_x = (sw - total_w) / 2;
        int slot = 0;
        for (int w = 0; w < st->window_count && slot < show_n; ++w) {
            if (st->windows[w].workspace != st->active_workspace) continue;
            int tx = start_x + slot * (tw + 14);
            int thov = (st->hover_mx >= tx && st->hover_mx <= tx + tw && st->hover_my >= thumb_y && st->hover_my <= thumb_y + th);
            ngfx_shadow_rect(cr, tx, thumb_y, tw, th, 12, thov ? 3.5 : 2.0, dark ? 0.40 : 0.18);
            ngfx_fill_rect(cr, tx, thumb_y, tw, th, 12, pal->card_r, pal->card_g, pal->card_b, thov ? 0.99 : 0.94);
            ngfx_stroke_rect(cr, tx + 0.5, thumb_y + 0.5, tw - 1, th - 1, 12, thov ? 1.5 : 1.0,
                             ar, ag, ab, thov ? 0.95 : 0.55);
            draw_app_icon(cr, st->windows[w].app, tx + 12, thumb_y + 12, 28);
            ngfx_text_clipped(cr, "Plus Jakarta Sans", 1, 11.8, tx + 48, thumb_y + 30, tw - 58, pal->fg_r, pal->fg_g, pal->fg_b, 1.0, st->windows[w].title);
            ngfx_text(cr, "Noto Sans", 0, 10.0, tx + 14, thumb_y + 64, pal->mut_r, pal->mut_g, pal->mut_b, 1.0,
                      st->windows[w].minimized ? "Minimized · Tap to restore" : "Tap to focus window");
            nature_add_hit(st, tx, thumb_y, tw, th, HIT_OVERVIEW_WIN, st->windows[w].id, 0);
            slot++;
        }
    }

    int lw = sw < 760 ? sw - 24 : 748;
    int ly = thumb_y + (open_count > 0 && !st->launcher_query[0] && sw >= 580 ? 102 : 14);
    int lh = sh - ly - 80;
    if (lh > 338) lh = 338;
    int lx = (sw - lw) / 2;
    ngfx_shadow_rect(cr, lx, ly, lw, lh, 18, 4.5, dark ? 0.55 : 0.24);
    ngfx_fill_gradient(cr, lx, ly, lw, lh, 18, pal->card_r, pal->card_g, pal->card_b, 0.98, pal->win_bg_r, pal->win_bg_g, pal->win_bg_b, 0.98, 0);
    ngfx_stroke_rect(cr, lx + 0.5, ly + 0.5, lw - 1, lh - 1, 18, 1.0, pal->strong_brd_r, pal->strong_brd_g, pal->strong_brd_b, pal->strong_brd_a);
    ngfx_top_specular(cr, lx, ly, lw, 18, dark ? 0.25 : 0.72);

    if (st->launcher_query[0]) {
        SpotlightItem items[7];
        int n_res = wm_collect_spotlight(st, items, 7);
        ngfx_text(cr, "Plus Jakarta Sans", 1, 13.8, lx + 24, ly + 30, pal->fg_r, pal->fg_g, pal->fg_b, 1.0, "Spotlight Results");
        char rcnt[32];
        snprintf(rcnt, sizeof(rcnt), "%d match%s", n_res, n_res == 1 ? "" : "es");
        ngfx_text(cr, "JetBrains Mono", 0, 9.8, lx + lw - 96, ly + 29, pal->mut_r, pal->mut_g, pal->mut_b, 0.88, rcnt);

        if (n_res == 0) {
            char empty_l[96];
            snprintf(empty_l, sizeof(empty_l), "No apps, files, settings or commands matching \"%.30s\"", st->launcher_query);
            double ew = ngfx_text_width(cr, "Noto Sans", 0, 11.8, empty_l);
            ngfx_text_clipped(cr, "Noto Sans", 0, 11.8, lx + (lw - ew > 40 ? (lw - ew) * 0.5 : 24), ly + lh * 0.52, lw - 48,
                              pal->mut_r, pal->mut_g, pal->mut_b, 0.88, empty_l);
        } else {
            int ry = ly + 44;
            int row_h = 37;
            int cur_sel = st->launcher_sel % n_res;
            if (cur_sel < 0) cur_sel += n_res;
            for (int r = 0; r < n_res; ++r) {
                if (ry + row_h > ly + lh - 22) break;
                const SpotlightItem *it = &items[r];
                int rhov = (st->hover_mx >= lx + 18 && st->hover_mx <= lx + lw - 18 && st->hover_my >= ry && st->hover_my <= ry + row_h - 3);
                int ract = (r == cur_sel || rhov);
                ngfx_fill_rect(cr, lx + 18, ry, lw - 36, row_h - 3, 10,
                               ract ? ar : pal->elev_r, ract ? ag : pal->elev_g, ract ? ab : pal->elev_b,
                               r == cur_sel ? 0.22 : (rhov ? 0.14 : 0.58));
                ngfx_stroke_rect(cr, lx + 18.5, ry + 0.5, lw - 37, row_h - 4, 10, r == cur_sel ? 1.2 : 0.8,
                                 ract ? ar : pal->border_r, ract ? ag : pal->border_g, ract ? ab : pal->border_b,
                                 r == cur_sel ? 0.85 : pal->border_a);

                if (it->kind == 0) {
                    draw_app_icon(cr, (AppType)it->arg0, lx + 28, ry + 5, 24);
                } else {
                    ngfx_fill_rect(cr, lx + 28, ry + 6, 24, 22, 6, ar, ag, ab, 0.20);
                    ngfx_text(cr, "JetBrains Mono", 1, 9.2, lx + 34, ry + 20.5, pal->ar_hi, pal->ag_hi, pal->ab_hi, 0.96,
                              it->kind == 1 ? "FS" : (it->kind == 2 ? "CF" : ">_"));
                }

                ngfx_text_clipped(cr, "Plus Jakarta Sans", 1, 11.4, lx + 62, ry + 16, lw - 186, pal->fg_r, pal->fg_g, pal->fg_b, 1.0, it->title);
                ngfx_text_clipped(cr, "Noto Sans", 0, 9.4, lx + 62, ry + 29, lw - 186, pal->mut_r, pal->mut_g, pal->mut_b, 0.90, it->subtitle);

                ngfx_fill_rect(cr, lx + lw - 92, ry + 8, 62, 18, 6, pal->card_r, pal->card_g, pal->card_b, 0.88);
                double tw_tag = ngfx_text_width(cr, "JetBrains Mono", 1, 8.6, it->tag);
                ngfx_text(cr, "JetBrains Mono", 1, 8.6, lx + lw - 92 + (62 - tw_tag) * 0.5, ry + 20,
                          pal->ar_hi, pal->ag_hi, pal->ab_hi, 0.98, it->tag);

                int act_hit = HIT_LAUNCHER_APP;
                if (it->kind == 1) act_hit = HIT_LAUNCHER_FILE;
                else if (it->kind == 2) act_hit = HIT_LAUNCHER_SETTING;
                else if (it->kind == 3) act_hit = HIT_LAUNCHER_ACTION;
                nature_add_hit(st, lx + 18, ry, lw - 36, row_h - 3, act_hit, it->arg0, 0);
                ry += row_h;
            }
        }
        if (lw >= 440) {
            ngfx_text(cr, "JetBrains Mono", 0, 9.0, lx + lw - 190, ly + lh - 10, pal->mut_r, pal->mut_g, pal->mut_b, 0.72, "Up/Down · Enter run · Esc");
        }
        return;
    }

    ngfx_text(cr, "Plus Jakarta Sans", 1, 13.8, lx + 24, ly + 30, pal->fg_r, pal->fg_g, pal->fg_b, 1.0, "Applications");

    /* Category Filter Tabs */
    const char *l_cats[5] = {"All", "System", "Internet", "Media", "Utilities"};
    int cat_x = lx + (lw >= 560 ? 150 : 122);
    for (int c = 0; c < 5; ++c) {
        double cw_c = ngfx_text_width(cr, "Noto Sans", 1, 10.2, l_cats[c]) + 18;
        if (cat_x + cw_c > lx + lw - 16) break;
        int act = (st->launcher_cat == c);
        int chov = (st->hover_mx >= cat_x && st->hover_mx <= cat_x + cw_c && st->hover_my >= ly + 13 && st->hover_my <= ly + 35);
        ngfx_fill_rect(cr, cat_x, ly + 13, cw_c, 22, 11,
                       act ? ar : pal->elev_r, act ? ag : pal->elev_g, act ? ab : pal->elev_b,
                       act ? 0.92 : (chov ? 0.95 : 0.65));
        ngfx_text(cr, "Noto Sans", act, 10.2, cat_x + 9, ly + 27.5,
                  act ? 1.0 : pal->fg_sec_r, act ? 1.0 : pal->fg_sec_g, act ? 1.0 : pal->fg_sec_b, 0.96, l_cats[c]);
        nature_add_hit(st, cat_x, ly + 13, (int)cw_c, 22, HIT_LAUNCHER_CAT, c, 0);
        cat_x += (int)cw_c + 6;
    }

    int cols = lw >= 640 ? 4 : (lw >= 420 ? 3 : 2);
    int cell_w = (lw - 48 - (cols - 1) * 14) / cols;
    int cell_h = lh >= 280 ? 102 : 76;
    int shown = 0;
    for (int i = 0; i < APP_COUNT; ++i) {
        if (!app_matches_launcher_cat((AppType)i, st->launcher_cat)) continue;
        int col = shown % cols;
        int row = shown / cols;
        int ax = lx + 24 + col * (cell_w + 14);
        int ay = ly + 48 + row * (cell_h + 10);
        if (ay + cell_h > ly + lh - 10) break;
        int ahov = (st->hover_mx >= ax && st->hover_mx <= ax + cell_w && st->hover_my >= ay && st->hover_my <= ay + cell_h);
        if (ahov) ngfx_radial_glow(cr, ax + cell_w * 0.5, ay + cell_h * 0.45, 4.0, cell_w * 0.5, ar, ag, ab, 0.18, 0.0);
        ngfx_fill_rect(cr, ax, ay, cell_w, cell_h, 13,
                       ahov ? ar : pal->elev_r, ahov ? ag : pal->elev_g, ahov ? ab : pal->elev_b, ahov ? 0.24 : 0.55);
        ngfx_stroke_rect(cr, ax + 0.5, ay + 0.5, cell_w - 1, cell_h - 1, 13, 1.0,
                         ahov ? ar : pal->border_r, ahov ? ag : pal->border_g, ahov ? ab : pal->border_b, ahov ? 0.72 : pal->border_a);
        int isz = cell_h >= 96 ? 48 : 36;
        draw_app_icon(cr, (AppType)i, ax + (cell_w - isz) * 0.5, ay + 11, isz);
        double tw = ngfx_text_width(cr, "Noto Sans", 1, 11.2, APP_NAMES[i]);
        if (tw < cell_w - 12) {
            ngfx_text(cr, "Noto Sans", 1, 11.2, ax + (cell_w - tw) * 0.5, ay + cell_h - 14, pal->fg_r, pal->fg_g, pal->fg_b, 1.0, APP_NAMES[i]);
        } else {
            ngfx_text_clipped(cr, "Noto Sans", 1, 11.2, ax + 8, ay + cell_h - 14, cell_w - 16, pal->fg_r, pal->fg_g, pal->fg_b, 1.0, APP_NAMES[i]);
        }
        nature_add_hit(st, ax, ay, cell_w, cell_h, HIT_LAUNCHER_APP, i, 0);
        shown++;
    }
    if (lw >= 440) {
        ngfx_text(cr, "JetBrains Mono", 0, 9.2, lx + lw - 122, ly + lh - 12, pal->mut_r, pal->mut_g, pal->mut_b, 0.68, "Enter launch · Esc");
    }
}

/* --- Dedicated Window Overview Mode with Live Previews & Workspace Filter --- */

static void draw_overview_mini_preview(const NatureOSState *st, cairo_t *cr, const OSWindow *w,
                                       int x, int y, int pw, int ph, const NaturePalette *pal) {
    double ar = pal->ar, ag = pal->ag, ab = pal->ab;
    g_ngfx.save(cr);
    ngfx_rounded_path(cr, x, y, pw, ph, 4);
    g_ngfx.clip(cr);

    if (w->app == APP_MONITOR) {
        /* Live mini CPU telemetry curve + RAM bar */
        g_ngfx.set_source_rgba(cr, pal->ar_hi, pal->ag_hi, pal->ab_hi, 0.92);
        g_ngfx.set_line_width(cr, 1.3);
        int n = 24;
        for (int i = 0; i < n; ++i) {
            int idx = (48 - n) + i;
            double v = st->cpu_history[idx];
            if (v > 100.0) v = 100.0;
            if (v < 0.0) v = 0.0;
            double sx = x + 6 + ((pw - 12) * i) / (double)(n - 1);
            double sy = y + ph - 9 - (v / 100.0) * (ph - 13);
            if (i == 0) g_ngfx.move_to(cr, sx, sy);
            else g_ngfx.line_to(cr, sx, sy);
        }
        g_ngfx.stroke(cr);
        double mem_f = st->total_ram_mb > 0 ? (double)st->used_ram_mb / (double)st->total_ram_mb : 0.3;
        if (mem_f > 1.0) mem_f = 1.0;
        ngfx_fill_rect(cr, x + 6, y + ph - 6, pw - 12, 3, 1.5, pal->elev_r, pal->elev_g, pal->elev_b, 0.8);
        ngfx_fill_rect(cr, x + 6, y + ph - 6, (pw - 12) * mem_f, 3, 1.5, pal->success_r, pal->success_g, pal->success_b, 0.9);
    } else if (w->app == APP_TERMINAL) {
        int start = st->term_line_count > 3 ? st->term_line_count - 3 : 0;
        int ly = y + 10;
        for (int i = start; i < st->term_line_count && ly < y + ph - 2; ++i) {
            uint8_t c = st->term_line_colors[i];
            ngfx_text_clipped(cr, "JetBrains Mono", 0, 7.2, x + 6, ly, pw - 12,
                              c == 2 ? pal->ar_hi : pal->fg_sec_r,
                              c == 2 ? pal->ag_hi : pal->fg_sec_g,
                              c == 2 ? pal->ab_hi : pal->fg_sec_b, 0.90, st->term_lines[i]);
            ly += 9;
        }
    } else if (w->app == APP_MEDIA) {
        if (st->media_mode == 0 && st->media_images[st->media_selected % MAX_MEDIA_IMAGES]) {
            cairo_surface_t *img = st->media_images[st->media_selected % MAX_MEDIA_IMAGES];
            double iw = g_ngfx.image_surface_get_width(img);
            double ih = g_ngfx.image_surface_get_height(img);
            if (iw > 0 && ih > 0) {
                double sc = fmax((double)pw / iw, (double)ph / ih);
                g_ngfx.translate(cr, x + (pw - iw * sc) * 0.5, y + (ph - ih * sc) * 0.5);
                g_ngfx.scale(cr, sc, sc);
                g_ngfx.set_source_surface(cr, img, 0, 0);
                g_ngfx.paint(cr);
            }
        } else if (st->media_mode == 1) {
            static const int vthumbs[MAX_VIDEO_CLIPS] = {6, 1, 9, 4};
            int ti = vthumbs[st->video_selected % MAX_VIDEO_CLIPS];
            cairo_surface_t *img = st->media_images[ti];
            if (img) {
                double iw = g_ngfx.image_surface_get_width(img);
                double ih = g_ngfx.image_surface_get_height(img);
                if (iw > 0 && ih > 0) {
                    double sc = fmax((double)pw / iw, (double)ph / ih);
                    g_ngfx.save(cr);
                    g_ngfx.translate(cr, x + (pw - iw * sc) * 0.5, y + (ph - ih * sc) * 0.5);
                    g_ngfx.scale(cr, sc, sc);
                    g_ngfx.set_source_surface(cr, img, 0, 0);
                    g_ngfx.paint(cr);
                    g_ngfx.restore(cr);
                }
            }
            ngfx_fill_rect(cr, x, y, pw, ph, 0, 0.04, 0.06, 0.08, 0.34);
            ngfx_fill_circle(cr, x + pw * 0.5, y + ph * 0.5, 9.0, ar, ag, ab, 0.92);
            double prog = (double)(st->video_progress_ms % 32000) / 32000.0;
            ngfx_fill_rect(cr, x + 6, y + ph - 5, (pw - 12) * prog, 2.5, 1.2, pal->ar_hi, pal->ag_hi, pal->ab_hi, 0.96);
        } else {
            for (int b = 0; b < 14; ++b) {
                double bh = 6 + 18 * fabs(sin(b * 0.8 + st->audio_progress_sec));
                ngfx_fill_rect(cr, x + 10 + b * ((pw - 20) / 14.0), y + ph - 5 - bh, 4, bh, 2, pal->ar_hi, pal->ag_hi, pal->ab_hi, 0.88);
            }
        }
    } else if (w->app == APP_FILES) {
        ngfx_fill_rect(cr, x, y, 32, ph, 0, pal->card_r, pal->card_g, pal->card_b, 0.65);
        for (int b = 0; b < 3; ++b) {
            ngfx_fill_rect(cr, x + 42 + b * 36, y + 6, 28, 22, 4, ar, ag, ab, 0.28);
            ngfx_fill_rect(cr, x + 46 + b * 36, y + 11, 14, 10, 2, pal->ar_hi, pal->ag_hi, pal->ab_hi, 0.85);
        }
    } else if (w->app == APP_BROWSER) {
        ngfx_fill_rect(cr, x + 6, y + 4, pw - 12, 8, 3, pal->card_r, pal->card_g, pal->card_b, 0.90);
        ngfx_fill_gradient(cr, x + 6, y + 15, pw - 12, ph - 19, 4, ar, ag, ab, 0.42, pal->ar_hi, pal->ag_hi, pal->ab_hi, 0.16, 0);
    } else if (w->app == APP_SETTINGS) {
        ngfx_fill_rect(cr, x, y, 36, ph, 0, pal->card_r, pal->card_g, pal->card_b, 0.65);
        for (int a = 0; a < 4; ++a) {
            double sr, sg, sb;
            nature_get_accent_ex(st->cfg.theme_family, a, pal->dark_mode, &sr, &sg, &sb);
            ngfx_fill_circle(cr, x + 52 + a * 18, y + ph * 0.5, 5.5, sr, sg, sb, 0.95);
        }
    } else {
        ngfx_fill_gradient(cr, x + 6, y + 4, pw - 12, 14, 4, 0.91, 0.33, 0.13, 0.78, ar, ag, ab, 0.65, 1);
        ngfx_fill_rect(cr, x + 6, y + 21, (pw - 18) / 2, 12, 3, pal->card_r, pal->card_g, pal->card_b, 0.88);
        ngfx_fill_rect(cr, x + 12 + (pw - 18) / 2, y + 21, (pw - 18) / 2, 12, 3, pal->card_r, pal->card_g, pal->card_b, 0.88);
    }
    g_ngfx.restore(cr);
}

void render_overview_overlay(NatureOSState *st, cairo_t *cr, const NaturePalette *pal) {
    int sw = st->screen_w;
    int sh = st->screen_h > 0 ? st->screen_h : OS_HEIGHT;
    int dark = pal->dark_mode;
    double ar = pal->ar, ag = pal->ag, ab = pal->ab;

    st->text_rect_count = 0;
    ngfx_fill_rect(cr, 0, 0, sw, sh, 0, pal->sidebar_r, pal->sidebar_g, pal->sidebar_b, dark ? 0.88 : 0.78);
    ngfx_radial_glow(cr, sw * 0.5, sh * 0.4, 24.0, sw * 0.5, ar, ag, ab, 0.14, 0.0);

    int hdr_w = sw < 760 ? sw - 24 : 740;
    int hx = (sw - hdr_w) / 2, hy = 52;
    ngfx_fill_rect(cr, hx, hy, hdr_w, 44, 14, pal->card_r, pal->card_g, pal->card_b, 0.96);
    ngfx_stroke_rect(cr, hx + 0.5, hy + 0.5, hdr_w - 1, 43, 14, 1.0, pal->strong_brd_r, pal->strong_brd_g, pal->strong_brd_b, pal->strong_brd_a);
    ngfx_text(cr, "Plus Jakarta Sans", 1, 13.6, hx + 14, hy + 27, pal->fg_r, pal->fg_g, pal->fg_b, 1.0, "Overview");

    /* Workspace filter pills: All | WS1 | WS2 */
    const char *f_lbls[3] = {"All", "WS1", "WS2"};
    int fx = hx + 96;
    for (int f = 0; f < 3 && hdr_w >= 460; ++f) {
        int fw = 36;
        int act = (st->overview_ws_filter == f);
        ngfx_fill_rect(cr, fx, hy + 10, fw, 24, 8,
                       act ? ar : pal->elev_r, act ? ag : pal->elev_g, act ? ab : pal->elev_b, act ? 0.92 : 0.84);
        ngfx_text(cr, "JetBrains Mono", 1, 9.2, fx + (fw - ngfx_text_width(cr, "JetBrains Mono", 1, 9.2, f_lbls[f])) * 0.5,
                  hy + 25.5, act ? pal->on_acc_r : pal->fg_sec_r, act ? pal->on_acc_g : pal->fg_sec_g, act ? pal->on_acc_b : pal->fg_sec_b, 0.98, f_lbls[f]);
        nature_add_hit(st, fx, hy + 10, fw, 24, HIT_OVERVIEW_FILTER, f, 0);
        fx += fw + 5;
    }

    /* Show Desktop, Tile & Exit buttons */
    int ex_x = hx + hdr_w - 78;
    ngfx_fill_rect(cr, ex_x, hy + 9, 68, 26, 8, ar, ag, ab, 0.90);
    ngfx_text(cr, "Noto Sans", 1, 10.6, ex_x + 19, hy + 26, 1, 1, 1, 1, "Done");
    nature_add_hit(st, ex_x, hy + 9, 68, 26, HIT_OVERVIEW_EXIT, 0, 0);

    int tl_x = ex_x - 82;
    ngfx_fill_rect(cr, tl_x, hy + 9, 76, 26, 8, pal->elev_r, pal->elev_g, pal->elev_b, 0.92);
    ngfx_stroke_rect(cr, tl_x + 0.5, hy + 9.5, 75, 25, 8, 0.8, pal->border_r, pal->border_g, pal->border_b, pal->border_a);
    ngfx_text(cr, "Noto Sans", 1, 10.4, tl_x + 11, hy + 26, pal->fg_r, pal->fg_g, pal->fg_b, 0.95, "Tile Grid");
    nature_add_hit(st, tl_x, hy + 9, 76, 26, HIT_QS_TILE, 0, 0);

    if (hdr_w >= 500) {
        int sd_x = tl_x - 106;
        ngfx_fill_rect(cr, sd_x, hy + 9, 100, 26, 8, pal->elev_r, pal->elev_g, pal->elev_b, 0.92);
        ngfx_stroke_rect(cr, sd_x + 0.5, hy + 9.5, 99, 25, 8, 0.8, pal->border_r, pal->border_g, pal->border_b, pal->border_a);
        ngfx_text(cr, "Noto Sans", 1, 10.2, sd_x + 11, hy + 26, pal->fg_r, pal->fg_g, pal->fg_b, 0.95, "Show Desktop");
        nature_add_hit(st, sd_x, hy + 9, 100, 26, HIT_OVERVIEW_SHOW_DESKTOP, 0, 0);

        if (st->window_count > 0 && hdr_w >= 620) {
            int ca_x = sd_x - 84;
            ngfx_fill_rect(cr, ca_x, hy + 9, 78, 26, 8, pal->danger_r, pal->danger_g, pal->danger_b, 0.20);
            ngfx_stroke_rect(cr, ca_x + 0.5, hy + 9.5, 77, 25, 8, 0.8, pal->danger_r, pal->danger_g, pal->danger_b, 0.45);
            ngfx_text(cr, "Noto Sans", 1, 10.2, ca_x + 13, hy + 26, pal->danger_r, pal->danger_g, pal->danger_b, 0.98, "Close All");
            nature_add_hit(st, ca_x, hy + 9, 78, 26, HIT_OVERVIEW_CLOSE_ALL, 0, 0);
        }
    }

    int ov_idx[MAX_WINDOWS];
    int ov_cnt = 0;
    for (int i = 0; i < st->window_count; ++i) {
        if (st->overview_ws_filter == 1 && st->windows[i].workspace != 0) continue;
        if (st->overview_ws_filter == 2 && st->windows[i].workspace != 1) continue;
        ov_idx[ov_cnt++] = i;
    }

    int cols = sw >= 900 ? 3 : (sw >= 560 ? 2 : 1);
    int cw = (sw - 80 - (cols - 1) * 18) / cols;
    if (cw > 340) cw = 340;
    int ch = 152;
    int grid_w = cols * cw + (cols - 1) * 18;
    int gx0 = (sw - grid_w) / 2;
    int gy0 = hy + 64;

    int total_rows = (ov_cnt + cols - 1) / cols;
    int vis_rows = (sh - 82 - gy0 + 18) / (ch + 18);
    if (vis_rows < 1) vis_rows = 1;
    int max_sc = total_rows - vis_rows;
    if (max_sc < 0) max_sc = 0;
    if (st->overview_scroll > max_sc) st->overview_scroll = max_sc;
    if (st->overview_scroll < 0) st->overview_scroll = 0;

    if (ov_cnt == 0) {
        ngfx_text(cr, "Noto Sans", 0, 13.0, (sw - 210) * 0.5, gy0 + 80, pal->mut_r, pal->mut_g, pal->mut_b, 0.9,
                  st->window_count == 0 ? "No open windows across workspaces" : "No windows on this workspace");
    } else {
        int start_i = st->overview_scroll * cols;
        for (int k = start_i; k < ov_cnt; ++k) {
            OSWindow *w = &st->windows[ov_idx[k]];
            int slot = k - start_i;
            int col = slot % cols;
            int row = slot / cols;
            int cx = gx0 + col * (cw + 18);
            int cy = gy0 + row * (ch + 18);
            if (cy + ch > sh - 82) break;
            int hov = (st->hover_mx >= cx && st->hover_mx <= cx + cw && st->hover_my >= cy && st->hover_my <= cy + ch);
            ngfx_shadow_rect(cr, cx, cy, cw, ch, 14, hov ? 4.0 : 2.2, dark ? 0.45 : 0.20);
            ngfx_fill_rect(cr, cx, cy, cw, ch, 14, pal->card_r, pal->card_g, pal->card_b, 0.96);
            ngfx_stroke_rect(cr, cx + 0.5, cy + 0.5, cw - 1, ch - 1, 14, hov ? 1.6 : 1.0,
                             (w->id == st->active_window_id || hov) ? ar : pal->border_r,
                             (w->id == st->active_window_id || hov) ? ag : pal->border_g,
                             (w->id == st->active_window_id || hov) ? ab : pal->border_b,
                             (w->id == st->active_window_id || hov) ? 0.85 : pal->border_a);

            draw_app_icon(cr, w->app, cx + 14, cy + 14, 34);
            ngfx_text_clipped(cr, "Plus Jakarta Sans", 1, 13.0, cx + 58, cy + 28, cw - 72, pal->fg_r, pal->fg_g, pal->fg_b, 1.0, w->title);
            char meta[48];
            snprintf(meta, sizeof(meta), "WS%d · %dx%d · %s", w->workspace + 1, w->w, w->h, w->minimized ? "Minimized" : "Active");
            ngfx_text(cr, "JetBrains Mono", 0, 9.8, cx + 58, cy + 46, pal->mut_r, pal->mut_g, pal->mut_b, 0.92, meta);

            int px_m = cx + 14, py_m = cy + 58, pw_m = cw - 28, ph_m = 48;
            ngfx_fill_rect(cr, px_m, py_m, pw_m, ph_m, 8, pal->sidebar_r, pal->sidebar_g, pal->sidebar_b, 0.88);
            ngfx_fill_rect(cr, px_m, py_m, pw_m, 11, 6, pal->elev_r, pal->elev_g, pal->elev_b, 0.95);
            ngfx_fill_circle(cr, px_m + 9, py_m + 5.5, 2.0, 0.98, 0.37, 0.34, 0.9);
            ngfx_fill_circle(cr, px_m + 16, py_m + 5.5, 2.0, 0.98, 0.74, 0.18, 0.9);
            ngfx_fill_circle(cr, px_m + 23, py_m + 5.5, 2.0, 0.18, 0.79, 0.32, 0.9);
            draw_overview_mini_preview(st, cr, w, px_m + 2, py_m + 12, pw_m - 4, ph_m - 14, pal);

            nature_add_hit(st, cx, cy, cw, ch - 38, HIT_OVERVIEW_WIN, w->id, 0);

            /* Bottom card action buttons: Move WS + Close */
            int act_y = cy + ch - 34;
            int bw_c = (cw - 36) / 2;
            ngfx_fill_rect(cr, cx + 14, act_y, bw_c, 24, 7, pal->elev_r, pal->elev_g, pal->elev_b, 0.90);
            ngfx_text(cr, "Noto Sans", 1, 9.8, cx + 22, act_y + 16, pal->fg_sec_r, pal->fg_sec_g, pal->fg_sec_b, 0.95,
                      w->workspace == 0 ? "Move to WS2" : "Move to WS1");
            nature_add_hit(st, cx + 14, act_y, bw_c, 24, HIT_OVERVIEW_SWAP, w->id, 0);

            ngfx_fill_rect(cr, cx + 22 + bw_c, act_y, bw_c, 24, 7, pal->danger_r, pal->danger_g, pal->danger_b, 0.18);
            ngfx_text(cr, "Noto Sans", 1, 9.8, cx + 22 + bw_c + (bw_c - 32) * 0.5, act_y + 16, pal->danger_r, pal->danger_g, pal->danger_b, 0.98, "Close");
            nature_add_hit(st, cx + 22 + bw_c, act_y, bw_c, 24, HIT_OVERVIEW_CLOSE, w->id, 0);
        }
    }
}

/* --- Lock Screen Overlay --- */

void render_lock_screen(NatureOSState *st, cairo_t *cr, const NaturePalette *pal) {
    int sw = st->screen_w;
    int sh = st->screen_h > 0 ? st->screen_h : OS_HEIGHT;
    int dark = pal->dark_mode;
    double ar = pal->ar, ag = pal->ag, ab = pal->ab;

    st->hit_count = 0;
    st->text_rect_count = 0;
    if (st->cached_wp) {
        g_ngfx.set_source_surface(cr, st->cached_wp, 0, 0);
        g_ngfx.paint(cr);
    }
    ngfx_fill_rect(cr, 0, 0, sw, sh, 0, pal->sidebar_r, pal->sidebar_g, pal->sidebar_b, dark ? 0.68 : 0.46);
    ngfx_radial_glow(cr, sw * 0.5, sh * 0.45, 24.0, (sw > sh ? sw : sh) * 0.45, ar, ag, ab, 0.22, 0.0);

    time_t raw_t = time(NULL);
    struct tm tm_buf;
    const struct tm *tm_info = localtime_r(&raw_t, &tm_buf);
    if (!tm_info) tm_info = &tm_buf;

    char l_time[24], l_date[64];
    format_clock_str(st, tm_info, 0, l_time, sizeof(l_time));
    strftime(l_date, sizeof(l_date), "%A, %B %d", tm_info);

    int cw = sw < 380 ? sw - 32 : 344;
    int ch = 290;
    int cx = (sw - cw) / 2;
    int cy = (sh - ch) / 2;

    ngfx_shadow_rect(cr, cx, cy, cw, ch, 22, 6.0, dark ? 0.55 : 0.25);
    ngfx_fill_gradient(cr, cx, cy, cw, ch, 22, pal->card_r, pal->card_g, pal->card_b, 0.92, pal->win_bg_r, pal->win_bg_g, pal->win_bg_b, 0.94, 0);
    ngfx_stroke_rect(cr, cx + 0.5, cy + 0.5, cw - 1, ch - 1, 22, 1.2, pal->strong_brd_r, pal->strong_brd_g, pal->strong_brd_b, pal->strong_brd_a);
    ngfx_top_specular(cr, cx, cy, cw, 22, dark ? 0.28 : 0.72);

    double t_w = ngfx_text_width(cr, "Plus Jakarta Sans", 1, 38.0, l_time);
    ngfx_text(cr, "Plus Jakarta Sans", 1, 38.0, cx + (cw - t_w) * 0.5, cy + 52, pal->fg_r, pal->fg_g, pal->fg_b, 1.0, l_time);
    double d_w = ngfx_text_width(cr, "Noto Sans", 0, 12.2, l_date);
    ngfx_text(cr, "Noto Sans", 0, 12.2, cx + (cw - d_w) * 0.5, cy + 74, pal->mut_r, pal->mut_g, pal->mut_b, 0.95, l_date);

    /* Avatar medallion */
    int av_r = 28;
    int av_cx = cx + cw / 2, av_cy = cy + 116;
    ngfx_fill_circle(cr, av_cx, av_cy, av_r + 3, ar, ag, ab, 0.45);
    if (st->avatar_surface) {
        double aw = g_ngfx.image_surface_get_width(st->avatar_surface);
        double ah = g_ngfx.image_surface_get_height(st->avatar_surface);
        if (aw > 0 && ah > 0) {
            g_ngfx.save(cr);
            g_ngfx.new_sub_path(cr);
            g_ngfx.arc(cr, av_cx, av_cy, av_r, 0, 6.28318);
            g_ngfx.clip(cr);
            g_ngfx.translate(cr, av_cx - av_r, av_cy - av_r);
            g_ngfx.scale(cr, (av_r * 2.0) / aw, (av_r * 2.0) / ah);
            g_ngfx.set_source_surface(cr, st->avatar_surface, 0, 0);
            g_ngfx.paint(cr);
            g_ngfx.restore(cr);
        }
    } else {
        ngfx_fill_circle(cr, av_cx, av_cy, av_r, ar, ag, ab, 0.92);
        ngfx_text(cr, "Plus Jakarta Sans", 1, 22.0, av_cx - 8, av_cy + 8, 1, 1, 1, 1, "N");
    }

    char u_lbl[64];
    snprintf(u_lbl, sizeof(u_lbl), "nature@%s", st->cfg.device_name[0] ? st->cfg.device_name : "nature-pc");
    double uw = ngfx_text_width(cr, "Plus Jakarta Sans", 1, 12.5, u_lbl);
    ngfx_text(cr, "Plus Jakarta Sans", 1, 12.5, cx + (cw - uw) * 0.5, cy + 164, pal->fg_r, pal->fg_g, pal->fg_b, 0.96, u_lbl);

    const char *p_lbl = st->cfg.power_profile == 0 ? "Saver" : (st->cfg.power_profile == 2 ? "Perf" : "Balanced");
    char alert_sfx[28] = "";
    if (st->notif_count > 0) {
        snprintf(alert_sfx, sizeof(alert_sfx), "  ·  %d alert%s", st->notif_count, st->notif_count == 1 ? "" : "s");
    }
    char stat_row[128];
    snprintf(stat_row, sizeof(stat_row), "%s  ·  %s%s%s",
             st->cfg.wifi_enabled ? "Wi-Fi" : "Offline", p_lbl,
             st->cfg.night_light ? "  ·  Night Light" : "", alert_sfx);
    double srw = ngfx_text_width(cr, "JetBrains Mono", 0, 9.6, stat_row);
    int spw = (int)srw + 22;
    if (spw > cw - 24) spw = cw - 24;
    int spx = cx + (cw - spw) / 2;
    ngfx_fill_rect(cr, spx, cy + 174, spw, 20, 10, pal->elev_r, pal->elev_g, pal->elev_b, 0.78);
    ngfx_stroke_rect(cr, spx + 0.5, cy + 174.5, spw - 1, 19, 10, 0.8, pal->border_r, pal->border_g, pal->border_b, pal->border_a);
    ngfx_text_clipped(cr, "JetBrains Mono", 0, 9.6, spx + 11, cy + 187.5, spw - 20, pal->fg_sec_r, pal->fg_sec_g, pal->fg_sec_b, 0.94, stat_row);

    int ub_w = 210, ub_h = 36;
    int ub_x = cx + (cw - ub_w) / 2, ub_y = cy + 206;
    int u_hov = (st->hover_mx >= ub_x && st->hover_mx <= ub_x + ub_w && st->hover_my >= ub_y && st->hover_my <= ub_y + ub_h);
    ngfx_fill_gradient(cr, ub_x, ub_y, ub_w, ub_h, 18, ar, ag, ab, u_hov ? 1.0 : 0.90, pal->ar_hi, pal->ag_hi, pal->ab_hi, u_hov ? 1.0 : 0.90, 1);
    const char *btn_lbl = "Unlock Session";
    double blw = ngfx_text_width(cr, "Plus Jakarta Sans", 1, 12.0, btn_lbl);
    ngfx_text(cr, "Plus Jakarta Sans", 1, 12.0, ub_x + (ub_w - blw) * 0.5, ub_y + 22.5, 1, 1, 1, 1, btn_lbl);

    const char *hint = "Click anywhere or press Enter to unlock";
    double htw = ngfx_text_width(cr, "Noto Sans", 0, 10.0, hint);
    ngfx_text(cr, "Noto Sans", 0, 10.0, cx + (cw - htw) * 0.5, cy + 266, pal->mut_r, pal->mut_g, pal->mut_b, 0.86, hint);
    nature_add_hit(st, 0, 0, sw, sh, HIT_LOCK_UNLOCK, 0, 0);
}

/* --- Power & Session Dialog (Modal) --- */

void render_power_dialog(NatureOSState *st, cairo_t *cr, const NaturePalette *pal) {
    if (!st->power_dialog_open) return;

    int sw = st->screen_w;
    int sh = st->screen_h > 0 ? st->screen_h : OS_HEIGHT;
    int dark = pal->dark_mode;
    double ar = pal->ar, ag = pal->ag, ab = pal->ab;

    /* Scrim over full display */
    ntheme_draw_modal_scrim(cr, sw, sh, dark ? 0.72 : 0.45);
    nature_add_hit(st, 0, 0, sw, sh, HIT_POWER_CANCEL, 0, 0);

    /* Centered glass modal */
    int dw = sw < 440 ? sw - 32 : 400;
    int dh = 248;
    int dx = (sw - dw) / 2;
    int dy = (sh - dh) / 2;

    nature_occlude_text_rects(st, dx, dy, dw, dh);

    ngfx_shadow_rect(cr, dx, dy, dw, dh, 18, 6.0, dark ? 0.65 : 0.30);
    ngfx_fill_gradient(cr, dx, dy, dw, dh, 18,
                       pal->card_r, pal->card_g, pal->card_b, 0.98,
                       pal->win_bg_r, pal->win_bg_g, pal->win_bg_b, 0.98, 0);
    ngfx_stroke_rect(cr, dx + 0.5, dy + 0.5, dw - 1, dh - 1, 18, 1.2,
                     pal->strong_brd_r, pal->strong_brd_g, pal->strong_brd_b, pal->strong_brd_a);
    ngfx_top_specular(cr, dx, dy, dw, 18, dark ? 0.28 : 0.75);

    /* Title & Session Info */
    ngfx_text(cr, "Plus Jakarta Sans", 1, 14.5, dx + 22, dy + 32, pal->fg_r, pal->fg_g, pal->fg_b, 1.0, "Power & Session");
    char sess_info[64];
    snprintf(sess_info, sizeof(sess_info), "nature@%s · Active Session", st->cfg.device_name[0] ? st->cfg.device_name : "nature-pc");
    ngfx_text(cr, "JetBrains Mono", 0, 9.8, dx + 22, dy + 48, pal->mut_r, pal->mut_g, pal->mut_b, 0.92, sess_info);

    /* Close "x" */
    int cx_hov = (st->hover_mx >= dx + dw - 38 && st->hover_mx <= dx + dw - 14 && st->hover_my >= dy + 16 && st->hover_my <= dy + 40);
    ngfx_fill_rect(cr, dx + dw - 36, dy + 18, 22, 22, 7, pal->elev_r, pal->elev_g, pal->elev_b, cx_hov ? 0.95 : 0.70);
    ngfx_text(cr, "Noto Sans", 1, 11.0, dx + dw - 30, dy + 33, pal->fg_sec_r, pal->fg_sec_g, pal->fg_sec_b, 0.95, "x");
    nature_add_hit(st, dx + dw - 36, dy + 18, 22, 22, HIT_POWER_CANCEL, 0, 0);

    /* 3 Action Cards */
    int card_w = (dw - 44 - 16) / 3;
    int card_h = 100;
    int card_y = dy + 66;

    /* 1. Lock */
    int c1_x = dx + 22;
    int h1 = (st->hover_mx >= c1_x && st->hover_mx <= c1_x + card_w && st->hover_my >= card_y && st->hover_my <= card_y + card_h);
    ngfx_fill_rect(cr, c1_x, card_y, card_w, card_h, 12,
                   h1 ? ar : pal->elev_r, h1 ? ag : pal->elev_g, h1 ? ab : pal->elev_b, h1 ? 0.25 : 0.68);
    ngfx_stroke_rect(cr, c1_x + 0.5, card_y + 0.5, card_w - 1, card_h - 1, 12, 1.0,
                     h1 ? ar : pal->border_r, h1 ? ag : pal->border_g, h1 ? ab : pal->border_b, h1 ? 0.85 : pal->border_a);
    /* Padlock Icon */
    ngfx_fill_circle(cr, c1_x + card_w * 0.5, card_y + 30, 16, ar, ag, ab, 0.22);
    g_ngfx.set_source_rgba(cr, h1 ? 1.0 : pal->ar_hi, h1 ? 1.0 : pal->ag_hi, h1 ? 1.0 : pal->ab_hi, 0.98);
    g_ngfx.set_line_width(cr, 1.4);
    g_ngfx.new_sub_path(cr); g_ngfx.arc(cr, c1_x + card_w * 0.5, card_y + 26.5, 3.8, 3.14159, 6.28318); g_ngfx.stroke(cr);
    ngfx_fill_rect(cr, c1_x + card_w * 0.5 - 5.5, card_y + 27, 11, 8.5, 2.0, pal->ar_hi, pal->ag_hi, pal->ab_hi, 0.98);
    ngfx_text(cr, "Plus Jakarta Sans", 1, 11.4, c1_x + (card_w - 28) * 0.5, card_y + 58, pal->fg_r, pal->fg_g, pal->fg_b, 1.0, "Lock");
    ngfx_text(cr, "Noto Sans", 0, 9.0, c1_x + 8, card_y + 74, pal->mut_r, pal->mut_g, pal->mut_b, 0.88, "Secure session");
    nature_add_hit(st, c1_x, card_y, card_w, card_h, HIT_POWER_LOCK, 0, 0);

    /* 2. Restart */
    int c2_x = c1_x + card_w + 8;
    int h2 = (st->hover_mx >= c2_x && st->hover_mx <= c2_x + card_w && st->hover_my >= card_y && st->hover_my <= card_y + card_h);
    ngfx_fill_rect(cr, c2_x, card_y, card_w, card_h, 12,
                   h2 ? pal->danger_r : pal->elev_r, h2 ? pal->danger_g : pal->elev_g, h2 ? pal->danger_b : pal->elev_b, h2 ? 0.25 : 0.68);
    ngfx_stroke_rect(cr, c2_x + 0.5, card_y + 0.5, card_w - 1, card_h - 1, 12, 1.0,
                     h2 ? pal->danger_r : pal->border_r, h2 ? pal->danger_g : pal->border_g, h2 ? pal->danger_b : pal->border_b, h2 ? 0.85 : pal->border_a);
    /* Restart Icon */
    ngfx_fill_circle(cr, c2_x + card_w * 0.5, card_y + 30, 16, pal->danger_r, pal->danger_g, pal->danger_b, 0.22);
    g_ngfx.set_source_rgba(cr, pal->danger_r, pal->danger_g, pal->danger_b, 0.98);
    g_ngfx.set_line_width(cr, 1.4);
    g_ngfx.new_sub_path(cr); g_ngfx.arc(cr, c2_x + card_w * 0.5, card_y + 30, 6.0, -1.0, 4.2); g_ngfx.stroke(cr);
    g_ngfx.move_to(cr, c2_x + card_w * 0.5, card_y + 22.0); g_ngfx.line_to(cr, c2_x + card_w * 0.5, card_y + 28.0); g_ngfx.stroke(cr);
    ngfx_text(cr, "Plus Jakarta Sans", 1, 11.4, c2_x + (card_w - 42) * 0.5, card_y + 58, pal->fg_r, pal->fg_g, pal->fg_b, 1.0, "Restart");
    ngfx_text(cr, "Noto Sans", 0, 9.0, c2_x + 8, card_y + 74, pal->mut_r, pal->mut_g, pal->mut_b, 0.88, "Reload session");
    nature_add_hit(st, c2_x, card_y, card_w, card_h, HIT_POWER_RESTART, 0, 0);

    /* 3. Sleep */
    int c3_x = c2_x + card_w + 8;
    int h3 = (st->hover_mx >= c3_x && st->hover_mx <= c3_x + card_w && st->hover_my >= card_y && st->hover_my <= card_y + card_h);
    ngfx_fill_rect(cr, c3_x, card_y, card_w, card_h, 12,
                   h3 ? 0.38 : pal->elev_r, h3 ? 0.40 : pal->elev_g, h3 ? 0.94 : pal->elev_b, h3 ? 0.25 : 0.68);
    ngfx_stroke_rect(cr, c3_x + 0.5, card_y + 0.5, card_w - 1, card_h - 1, 12, 1.0,
                     h3 ? 0.38 : pal->border_r, h3 ? 0.40 : pal->border_g, h3 ? 0.94 : pal->border_b, h3 ? 0.85 : pal->border_a);
    /* Moon Icon */
    ngfx_fill_circle(cr, c3_x + card_w * 0.5, card_y + 30, 16, 0.38, 0.40, 0.94, 0.22);
    ngfx_fill_circle(cr, c3_x + card_w * 0.5, card_y + 30, 6.5, 0.70, 0.75, 1.0, 0.95);
    ngfx_fill_circle(cr, c3_x + card_w * 0.5 + 3.2, card_y + 27.8, 5.2, pal->card_r, pal->card_g, pal->card_b, 0.98);
    ngfx_text(cr, "Plus Jakarta Sans", 1, 11.4, c3_x + (card_w - 32) * 0.5, card_y + 58, pal->fg_r, pal->fg_g, pal->fg_b, 1.0, "Sleep");
    ngfx_text(cr, "Noto Sans", 0, 9.0, c3_x + 8, card_y + 74, pal->mut_r, pal->mut_g, pal->mut_b, 0.88, "Dim display");
    nature_add_hit(st, c3_x, card_y, card_w, card_h, HIT_POWER_SLEEP, 0, 0);

    /* Bottom Cancel Button */
    int btn_w = 110, btn_h = 32;
    int btn_x = dx + (dw - btn_w) / 2;
    int btn_y = dy + dh - 48;
    int h_can = (st->hover_mx >= btn_x && st->hover_mx <= btn_x + btn_w && st->hover_my >= btn_y && st->hover_my <= btn_y + btn_h);
    ngfx_fill_rect(cr, btn_x, btn_y, btn_w, btn_h, 10, pal->elev_r, pal->elev_g, pal->elev_b, h_can ? 0.95 : 0.78);
    ngfx_stroke_rect(cr, btn_x + 0.5, btn_y + 0.5, btn_w - 1, btn_h - 1, 10, 0.9, pal->border_r, pal->border_g, pal->border_b, pal->border_a);
    ngfx_text(cr, "Noto Sans", 1, 11.0, btn_x + 32, btn_y + 20.5, pal->fg_r, pal->fg_g, pal->fg_b, 0.96, "Cancel");
    nature_add_hit(st, btn_x, btn_y, btn_w, btn_h, HIT_POWER_CANCEL, 0, 0);
}

/* --- Interactive Alt-Tab Window Switcher HUD --- */

void render_alt_tab_hud(NatureOSState *st, cairo_t *cr, const NaturePalette *pal) {
    if (!st->alt_tab_open) return;

    int sw = st->screen_w;
    int sh = st->screen_h > 0 ? st->screen_h : OS_HEIGHT;
    int dark = pal->dark_mode;
    double ar = pal->ar, ag = pal->ag, ab = pal->ab;

    /* Light modal scrim (click outside dismisses HUD) */
    ntheme_draw_modal_scrim(cr, sw, sh, dark ? 0.65 : 0.38);
    nature_add_hit(st, 0, 0, sw, sh, HIT_ALT_TAB_SELECT, -1, 0);

    int win_idxs[MAX_WINDOWS];
    int n_win = wm_collect_alt_tab_wins(st, win_idxs);
    if (n_win == 0) {
        int pw = 240, ph = 64;
        int px = (sw - pw) / 2, py = (sh - ph) / 2;
        ngfx_fill_rect(cr, px, py, pw, ph, 14, pal->card_r, pal->card_g, pal->card_b, 0.96);
        ngfx_stroke_rect(cr, px + 0.5, py + 0.5, pw - 1, ph - 1, 14, 1.0, pal->border_r, pal->border_g, pal->border_b, pal->border_a);
        ngfx_text(cr, "Noto Sans", 0, 11.5, px + 36, py + 37, pal->mut_r, pal->mut_g, pal->mut_b, 0.95, "No open windows to switch");
        return;
    }

    int tile_w = 126, tile_h = 104, gap = 12;
    int max_visible = (sw - 80) / (tile_w + gap);
    if (max_visible < 1) max_visible = 1;
    int show_cnt = n_win < max_visible ? n_win : max_visible;
    int hud_w = show_cnt * tile_w + (show_cnt - 1) * gap + 40;
    int hud_h = 176;
    int hud_x = (sw - hud_w) / 2;
    int hud_y = (sh - hud_h) / 2;

    nature_occlude_text_rects(st, hud_x, hud_y, hud_w, hud_h);

    ngfx_shadow_rect(cr, hud_x, hud_y, hud_w, hud_h, 19, 6.0, dark ? 0.68 : 0.32);
    ngfx_fill_gradient(cr, hud_x, hud_y, hud_w, hud_h, 19,
                       pal->card_r, pal->card_g, pal->card_b, 0.985,
                       pal->win_bg_r, pal->win_bg_g, pal->win_bg_b, 0.985, 0);
    ngfx_stroke_rect(cr, hud_x + 0.5, hud_y + 0.5, hud_w - 1, hud_h - 1, 19, 1.2,
                     pal->strong_brd_r, pal->strong_brd_g, pal->strong_brd_b, pal->strong_brd_a);
    ngfx_top_specular(cr, hud_x, hud_y, hud_w, 19, dark ? 0.28 : 0.75);

    /* HUD Header */
    ngfx_text(cr, "Plus Jakarta Sans", 1, 12.8, hud_x + 22, hud_y + 26, pal->fg_r, pal->fg_g, pal->fg_b, 1.0, "Switch Application");
    if (hud_w >= 340) {
        ngfx_text(cr, "JetBrains Mono", 0, 9.4, hud_x + hud_w - 220, hud_y + 26, pal->mut_r, pal->mut_g, pal->mut_b, 0.88, "Tab to cycle · Click to select");
    }

    /* Render window tiles in synced Alt-Tab order */
    int start_x = hud_x + 20;
    int tile_y = hud_y + 42;
    int cur_sel = st->alt_tab_selected % n_win;
    if (cur_sel < 0) cur_sel += n_win;
    for (int i = 0; i < show_cnt; ++i) {
        OSWindow *w = &st->windows[win_idxs[i]];
        int tx = start_x + i * (tile_w + gap);
        int is_sel = (i == cur_sel);
        int hov = (st->hover_mx >= tx && st->hover_mx <= tx + tile_w && st->hover_my >= tile_y && st->hover_my <= tile_y + tile_h);

        if (is_sel) {
            ngfx_radial_glow(cr, tx + tile_w * 0.5, tile_y + tile_h * 0.5, 4.0, tile_w * 0.6, ar, ag, ab, 0.32, 0.0);
            ngfx_fill_gradient(cr, tx, tile_y, tile_w, tile_h, 14,
                               ar, ag, ab, 0.92,
                               pal->ar_hi, pal->ag_hi, pal->ab_hi, 0.92, 1);
            ngfx_stroke_rect(cr, tx + 0.5, tile_y + 0.5, tile_w - 1, tile_h - 1, 14, 1.5,
                             1.0, 1.0, 1.0, 0.85);
        } else {
            ngfx_fill_rect(cr, tx, tile_y, tile_w, tile_h, 14,
                           pal->elev_r, pal->elev_g, pal->elev_b, hov ? 0.92 : 0.68);
            ngfx_stroke_rect(cr, tx + 0.5, tile_y + 0.5, tile_w - 1, tile_h - 1, 14, 1.0,
                             hov ? ar : pal->border_r, hov ? ag : pal->border_g, hov ? ab : pal->border_b,
                             hov ? 0.80 : pal->border_a);
        }

        /* App Icon */
        draw_app_icon(cr, w->app, tx + (tile_w - 38) * 0.5, tile_y + 12, 38);

        /* Window Title */
        ngfx_text_clipped(cr, "Plus Jakarta Sans", 1, 10.8, tx + 8, tile_y + 68, tile_w - 16,
                          is_sel ? 1.0 : pal->fg_r, is_sel ? 1.0 : pal->fg_g, is_sel ? 1.0 : pal->fg_b, 1.0,
                          w->title);

        /* Workspace & State Pill */
        char ws_lbl[32];
        snprintf(ws_lbl, sizeof(ws_lbl), "WS%d%s", w->workspace + 1, w->minimized ? " · Min" : "");
        ngfx_text(cr, "JetBrains Mono", 0, 8.8, tx + (tile_w - 34) * 0.5, tile_y + 86,
                  is_sel ? 0.92 : pal->mut_r, is_sel ? 0.95 : pal->mut_g, is_sel ? 0.92 : pal->mut_b, 0.92,
                  ws_lbl);

        nature_add_hit(st, tx, tile_y, tile_w, tile_h, HIT_ALT_TAB_SELECT, w->id, 0);
    }

    /* Bottom cycling click hints */
    int btn_prev_x = hud_x + (hud_w - 96) / 2;
    int btn_next_x = btn_prev_x + 52;
    int nav_y = hud_y + hud_h - 22;
    ngfx_fill_rect(cr, btn_prev_x, nav_y - 6, 40, 18, 6, pal->elev_r, pal->elev_g, pal->elev_b, 0.85);
    ngfx_text(cr, "JetBrains Mono", 1, 9.5, btn_prev_x + 12, nav_y + 6, pal->fg_sec_r, pal->fg_sec_g, pal->fg_sec_b, 0.95, "< Prev");
    nature_add_hit(st, btn_prev_x, nav_y - 6, 40, 18, HIT_ALT_TAB_PREV, 0, 0);

    ngfx_fill_rect(cr, btn_next_x, nav_y - 6, 40, 18, 6, pal->elev_r, pal->elev_g, pal->elev_b, 0.85);
    ngfx_text(cr, "JetBrains Mono", 1, 9.5, btn_next_x + 10, nav_y + 6, pal->fg_sec_r, pal->fg_sec_g, pal->fg_sec_b, 0.95, "Next >");
    nature_add_hit(st, btn_next_x, nav_y - 6, 40, 18, HIT_ALT_TAB_NEXT, 0, 0);
}
