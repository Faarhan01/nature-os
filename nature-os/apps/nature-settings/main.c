#include "nature_os.h"
#include <stdio.h>

#define NUM_SECTIONS 6
static const char *SECTIONS[NUM_SECTIONS] = {
    "Appearance",
    "Display",
    "Sound",
    "Performance",
    "Network",
    "About System"
};

static const char *WP_NAMES[3] = {"Canopy Dawn", "Alpine Twilight", "Basalt Moss"};
static const char *ACCENT_NAMES[4] = {"Moss", "Teal", "Amber", "Alpine"};
static const char *AUDIO_DEVS[3] = {
    "Built-in Audio (Analog Stereo)",
    "DisplayPort / HDMI Digital",
    "Bluetooth Canopy Buds (AAC)"
};

static void draw_settings_icon(cairo_t *cr, int idx, double x, double y, double r, double g, double b, double a) {
    g_ngfx.set_source_rgba(cr, r, g, b, a);
    g_ngfx.set_line_width(cr, 1.4);
    switch (idx) {
        case 0: /* Appearance */
            ngfx_stroke_rect(cr, x + 2, y + 2, 12, 12, 6, 1.35, r, g, b, a);
            ngfx_fill_circle(cr, x + 6.5, y + 6.5, 1.5, r, g, b, a);
            ngfx_fill_circle(cr, x + 10.0, y + 9.0, 1.5, r, g, b, a);
            break;
        case 1: /* Display */
            ngfx_stroke_rect(cr, x + 2, y + 2.5, 12, 8.5, 1.8, 1.35, r, g, b, a);
            g_ngfx.move_to(cr, x + 5, y + 13.5); g_ngfx.line_to(cr, x + 11, y + 13.5);
            g_ngfx.stroke(cr);
            break;
        case 2: /* Sound */
            g_ngfx.move_to(cr, x + 2.5, y + 6.5); g_ngfx.line_to(cr, x + 5.5, y + 6.5);
            g_ngfx.line_to(cr, x + 9.0, y + 3.5); g_ngfx.line_to(cr, x + 9.0, y + 12.5);
            g_ngfx.line_to(cr, x + 5.5, y + 9.5); g_ngfx.line_to(cr, x + 2.5, y + 9.5);
            g_ngfx.close_path(cr); g_ngfx.fill(cr);
            g_ngfx.new_sub_path(cr); g_ngfx.arc(cr, x + 9.5, y + 8.0, 3.8, -0.75, 0.75);
            g_ngfx.stroke(cr);
            break;
        case 3: /* Performance */
            g_ngfx.move_to(cr, x + 9, y + 2); g_ngfx.line_to(cr, x + 4.5, y + 8.5);
            g_ngfx.line_to(cr, x + 8.2, y + 8.5); g_ngfx.line_to(cr, x + 7.0, y + 14);
            g_ngfx.line_to(cr, x + 11.5, y + 7.5); g_ngfx.line_to(cr, x + 7.8, y + 7.5);
            g_ngfx.close_path(cr); g_ngfx.fill(cr);
            break;
        case 4: /* Network */
            g_ngfx.new_sub_path(cr); g_ngfx.arc(cr, x + 8, y + 12.5, 7.5, -2.35, -0.79); g_ngfx.stroke(cr);
            g_ngfx.new_sub_path(cr); g_ngfx.arc(cr, x + 8, y + 12.5, 4.2, -2.35, -0.79); g_ngfx.stroke(cr);
            ngfx_fill_circle(cr, x + 8, y + 12.5, 1.4, r, g, b, a);
            break;
        default: /* About */
            ngfx_stroke_rect(cr, x + 2.5, y + 2.5, 11, 11, 5.5, 1.35, r, g, b, a);
            ngfx_fill_rect(cr, x + 7.2, y + 7.0, 1.6, 4.5, 0.8, r, g, b, a);
            ngfx_fill_circle(cr, x + 8.0, y + 5.2, 1.0, r, g, b, a);
            break;
    }
}

void nature_settings_render(NatureOSState *st, cairo_t *cr, OSWindow *w, int cx, int cy, int cw, int ch) {
    (void)w;
    int dark = st->cfg.dark_mode;
    double ar, ag, ab;
    nature_get_accent(st->cfg.accent_idx, &ar, &ag, &ab);

    double fg_r = dark ? 0.92 : 0.10, fg_g = dark ? 0.95 : 0.13, fg_b = dark ? 0.93 : 0.11;
    double mut_r = dark ? 0.62 : 0.44, mut_g = dark ? 0.70 : 0.50, mut_b = dark ? 0.66 : 0.47;

    int compact = cw < 560;
    int side_w = compact ? 128 : 172;
    ngfx_fill_rect(cr, cx, cy, side_w, ch, 0,
                   dark ? 0.046 : 0.935, dark ? 0.070 : 0.955, dark ? 0.062 : 0.948, 1.0);
    g_ngfx.set_source_rgba(cr, dark ? 1 : 0, dark ? 1 : 0, dark ? 1 : 0, 0.08);
    g_ngfx.set_line_width(cr, 1.0);
    g_ngfx.move_to(cr, cx + side_w + 0.5, cy);
    g_ngfx.line_to(cr, cx + side_w + 0.5, cy + ch);
    g_ngfx.stroke(cr);

    ngfx_text(cr, "Plus Jakarta Sans", 1, 10.2, cx + 14, cy + 20, mut_r, mut_g, mut_b, 0.82, "SETTINGS");
    for (int i = 0; i < NUM_SECTIONS; ++i) {
        int sy = cy + 32 + i * 36;
        if (sy + 32 > cy + ch - 6) break;
        int act = st->settings_section == i;
        int hov = (st->hover_mx >= cx + 8 && st->hover_mx <= cx + side_w - 8 && st->hover_my >= sy && st->hover_my <= sy + 30);
        if (act || hov) {
            ngfx_fill_rect(cr, cx + 8, sy, side_w - 16, 30, 8,
                           act ? ar : (dark ? 1 : 0), act ? ag : (dark ? 1 : 0), act ? ab : (dark ? 1 : 0),
                           act ? 0.24 : 0.06);
        }
        if (act) {
            ngfx_fill_rect(cr, cx + 8, sy + 6, 3, 18, 1.5, ar * 1.3 > 1 ? 1 : ar * 1.3,
                           ag * 1.3 > 1 ? 1 : ag * 1.3, ab * 1.3 > 1 ? 1 : ab * 1.3, 1.0);
        }
        draw_settings_icon(cr, i, cx + 14, sy + 7,
                           act ? (ar * 1.3 > 1 ? 1 : ar * 1.3) : mut_r,
                           act ? (ag * 1.3 > 1 ? 1 : ag * 1.3) : mut_g,
                           act ? (ab * 1.3 > 1 ? 1 : ab * 1.3) : mut_b, 0.95);
        ngfx_text_clipped(cr, "Noto Sans", act, 11.6, cx + 36, sy + 19, side_w - 44,
                          fg_r, fg_g, fg_b, act ? 1.0 : 0.84, SECTIONS[i]);
        nature_add_hit(st, cx + 8, sy, side_w - 16, 30, HIT_SETTINGS_SECTION, i, 0);
    }

    int pane_x = cx + side_w;
    int pane_w = cw - side_w;
    int px = pane_x + 18;
    int pw = pane_w - 42;
    if (pw < 180) pw = 180;

    int content_heights[6] = {370, 320, 320, 300, 240, 340};
    int total_h = content_heights[st->settings_section % 6];
    int step_px = 28;
    int max_scroll = (total_h > ch - 24) ? ((total_h - (ch - 24) + step_px - 1) / step_px) : 0;
    st->settings_max_scroll = max_scroll;
    if (st->settings_scroll > max_scroll) st->settings_scroll = max_scroll;
    if (st->settings_scroll < 0) st->settings_scroll = 0;

    g_ngfx.save(cr);
    g_ngfx.rectangle(cr, pane_x + 1, cy, pane_w - 16, ch);
    g_ngfx.clip(cr);

    int py = cy + 28 - st->settings_scroll * step_px;
    ngfx_text(cr, "Plus Jakarta Sans", 1, 17.0, px, py, fg_r, fg_g, fg_b, 1.0, SECTIONS[st->settings_section]);

    if (st->settings_section == 0) {
        py += 18;
        ngfx_fill_rect(cr, px, py, pw, 54, 11, dark ? 1 : 0, dark ? 1 : 0, dark ? 1 : 0, 0.045);
        ngfx_stroke_rect(cr, px + 0.5, py + 0.5, pw - 1, 53, 11, 1.0, dark ? 1 : 0, dark ? 1 : 0, dark ? 1 : 0, 0.08);
        ngfx_text_clipped(cr, "Noto Sans", 1, 12.2, px + 14, py + 23, pw - 210, fg_r, fg_g, fg_b, 1.0, "Theme & Eye Care");
        ngfx_text_clipped(cr, "Noto Sans", 0, 10.2, px + 14, py + 40, pw - 210, mut_r, mut_g, mut_b, 0.9, "Dark/Light glass & warm Night Light");

        ngfx_fill_rect(cr, px + pw - 196, py + 14, 90, 26, 7,
                       st->cfg.night_light ? 0.92 : (dark ? 1 : 0),
                       st->cfg.night_light ? 0.56 : (dark ? 1 : 0),
                       st->cfg.night_light ? 0.22 : (dark ? 1 : 0),
                       st->cfg.night_light ? 0.92 : 0.10);
        ngfx_text(cr, "Noto Sans", 1, 10.8, px + pw - 185, py + 31,
                  st->cfg.night_light ? 1.0 : fg_r, st->cfg.night_light ? 1.0 : fg_g, st->cfg.night_light ? 1.0 : fg_b,
                  1.0, st->cfg.night_light ? "Night: ON" : "Night Light");
        nature_add_hit(st, px + pw - 196, py + 14, 90, 26, HIT_SETTINGS_NIGHT_LIGHT, 0, 0);

        ngfx_fill_rect(cr, px + pw - 100, py + 14, 88, 26, 7, ar, ag, ab, 0.92);
        ngfx_text(cr, "Noto Sans", 1, 10.8, px + pw - 88, py + 31, 1, 1, 1, 1,
                  dark ? "Dark Mode" : "Light Mode");
        nature_add_hit(st, px + pw - 100, py + 14, 88, 26, HIT_SETTINGS_DARK, 0, 0);

        py += 72;
        ngfx_text(cr, "Noto Sans", 1, 12.0, px, py, mut_r, mut_g, mut_b, 1.0, "Desktop Wallpaper");
        py += 10;
        int wp_w = (pw - 20) / 3;
        int wp_h = (wp_w * 9) / 16;
        if (wp_h > 74) wp_h = 74;
        for (int i = 0; i < 3; ++i) {
            int wx = px + i * (wp_w + 10);
            int sel = st->cfg.wallpaper_idx == i;
            if (st->wallpapers[i]) {
                g_ngfx.save(cr);
                ngfx_rounded_path(cr, wx, py, wp_w, wp_h, 9);
                g_ngfx.clip(cr);
                double sw = g_ngfx.image_surface_get_width(st->wallpapers[i]);
                double sh = g_ngfx.image_surface_get_height(st->wallpapers[i]);
                g_ngfx.translate(cr, wx, py);
                g_ngfx.scale(cr, (double)wp_w / sw, (double)wp_h / sh);
                g_ngfx.set_source_surface(cr, st->wallpapers[i], 0, 0);
                g_ngfx.paint(cr);
                g_ngfx.restore(cr);
            }
            ngfx_stroke_rect(cr, wx, py, wp_w, wp_h, 9, sel ? 2.4 : 1.0,
                             sel ? ar : 0.5, sel ? ag : 0.5, sel ? ab : 0.5, sel ? 1.0 : 0.28);
            if (sel) {
                ngfx_fill_circle(cr, wx + wp_w - 13, py + 13, 8.0, ar, ag, ab, 0.96);
                g_ngfx.set_source_rgba(cr, 1, 1, 1, 1);
                g_ngfx.set_line_width(cr, 1.6);
                g_ngfx.move_to(cr, wx + wp_w - 16.5, py + 13);
                g_ngfx.line_to(cr, wx + wp_w - 13.8, py + 15.8);
                g_ngfx.line_to(cr, wx + wp_w - 9.0, py + 10.5);
                g_ngfx.stroke(cr);
            }
            ngfx_text_clipped(cr, "Noto Sans", sel, 11.0, wx + 4, py + wp_h + 16, wp_w - 6, fg_r, fg_g, fg_b, 0.92, WP_NAMES[i]);
            nature_add_hit(st, wx, py, wp_w, wp_h + 18, HIT_SETTINGS_WALLPAPER, i, 0);
        }

        py += wp_h + 42;
        ngfx_text(cr, "Noto Sans", 1, 12.0, px, py, mut_r, mut_g, mut_b, 1.0, "Accent Palette & Ubuntu Dock");
        py += 12;
        int ac_w = (pw - 24) / 4;
        for (int i = 0; i < 4; ++i) {
            double r, g, b;
            nature_get_accent(i, &r, &g, &b);
            int ax = px + i * (ac_w + 8);
            int sel = st->cfg.accent_idx == i;
            ngfx_fill_rect(cr, ax, py, ac_w, 28, 8, r, g, b, sel ? 0.96 : 0.42);
            if (sel) ngfx_stroke_rect(cr, ax + 0.5, py + 0.5, ac_w - 1, 27, 8, 1.5, 1, 1, 1, 0.85);
            ngfx_text_clipped(cr, "Noto Sans", 1, 11.2, ax + 10, py + 18, ac_w - 16, 1, 1, 1, 1, ACCENT_NAMES[i]);
            nature_add_hit(st, ax, py, ac_w, 28, HIT_SETTINGS_ACCENT, i, 0);
        }

        py += 46;
        ngfx_text(cr, "Noto Sans", 0, 12.0, px, py, fg_r, fg_g, fg_b, 0.9, "Dock Alignment:");
        ngfx_fill_rect(cr, px + 116, py - 16, 150, 28, 8, dark ? 1 : 0, dark ? 1 : 0, dark ? 1 : 0, 0.10);
        ngfx_stroke_rect(cr, px + 116.5, py - 15.5, 149, 27, 8, 1.0, dark ? 1 : 0, dark ? 1 : 0, dark ? 1 : 0, 0.08);
        ngfx_text(cr, "Noto Sans", 1, 11.2, px + 130, py + 2, fg_r, fg_g, fg_b, 1.0,
                  st->cfg.dock_position == 0 ? "Bottom Floating" : "Ubuntu Left Edge");
        nature_add_hit(st, px + 116, py - 16, 150, 28, HIT_SETTINGS_DOCK_POS, 0, 0);
    } else if (st->settings_section == 1) {
        py += 20;
        ngfx_fill_rect(cr, px, py, pw, 64, 11, dark ? 1 : 0, dark ? 1 : 0, dark ? 1 : 0, 0.045);
        ngfx_stroke_rect(cr, px + 0.5, py + 0.5, pw - 1, 63, 11, 1.0, dark ? 1 : 0, dark ? 1 : 0, dark ? 1 : 0, 0.08);
        ngfx_text(cr, "Noto Sans", 1, 13.0, px + 14, py + 24, fg_r, fg_g, fg_b, 1.0, "Adaptive Display Output");
        char disp[64];
        snprintf(disp, sizeof(disp), "%dx%d · Adaptive DRM/KMS · RGB 8-bit", st->screen_w, st->screen_h);
        ngfx_text_clipped(cr, "JetBrains Mono", 0, 10.8, px + 14, py + 44, pw - 170, mut_r, mut_g, mut_b, 1.0, disp);

        ngfx_fill_rect(cr, px + pw - 150, py + 18, 66, 28, 7,
                       st->cfg.target_hz == 60 ? ar : (dark ? 1 : 0),
                       st->cfg.target_hz == 60 ? ag : (dark ? 1 : 0),
                       st->cfg.target_hz == 60 ? ab : (dark ? 1 : 0),
                       st->cfg.target_hz == 60 ? 0.90 : 0.10);
        ngfx_text(cr, "JetBrains Mono", 1, 10.8, px + pw - 135, py + 36,
                  st->cfg.target_hz == 60 ? 1.0 : fg_r, st->cfg.target_hz == 60 ? 1.0 : fg_g, st->cfg.target_hz == 60 ? 1.0 : fg_b, 1.0, "60 Hz");
        nature_add_hit(st, px + pw - 150, py + 18, 66, 28, HIT_SETTINGS_HZ, 60, 0);

        ngfx_fill_rect(cr, px + pw - 78, py + 18, 66, 28, 7,
                       st->cfg.target_hz == 120 ? ar : (dark ? 1 : 0),
                       st->cfg.target_hz == 120 ? ag : (dark ? 1 : 0),
                       st->cfg.target_hz == 120 ? ab : (dark ? 1 : 0),
                       st->cfg.target_hz == 120 ? 0.90 : 0.10);
        ngfx_text(cr, "JetBrains Mono", 1, 10.8, px + pw - 66, py + 36,
                  st->cfg.target_hz == 120 ? 1.0 : fg_r, st->cfg.target_hz == 120 ? 1.0 : fg_g, st->cfg.target_hz == 120 ? 1.0 : fg_b, 1.0, "120 Hz");
        nature_add_hit(st, px + pw - 78, py + 18, 66, 28, HIT_SETTINGS_HZ, 120, 0);

        py += 76;
        ngfx_fill_rect(cr, px, py, pw, 72, 11, dark ? 1 : 0, dark ? 1 : 0, dark ? 1 : 0, 0.045);
        ngfx_stroke_rect(cr, px + 0.5, py + 0.5, pw - 1, 71, 11, 1.0, dark ? 1 : 0, dark ? 1 : 0, dark ? 1 : 0, 0.08);
        char br_buf[48];
        snprintf(br_buf, sizeof(br_buf), "Panel Brightness (%d%%)", st->cfg.brightness);
        ngfx_text(cr, "Noto Sans", 1, 12.8, px + 14, py + 24, fg_r, fg_g, fg_b, 1.0, br_buf);
        int sl_x = px + 14, sl_w = pw - 28;
        ngfx_fill_rect(cr, sl_x, py + 41, sl_w, 12, 6, dark ? 1 : 0, dark ? 1 : 0, dark ? 1 : 0, 0.14);
        double bw_fill = (sl_w * st->cfg.brightness) / 100.0;
        if (bw_fill > 0) ngfx_fill_gradient(cr, sl_x, py + 41, bw_fill, 12, 6, 0.88, 0.65, 0.18, 0.95, 0.98, 0.84, 0.34, 0.98, 1);
        ngfx_fill_circle(cr, sl_x + bw_fill, py + 48, 9.0, 0.0, 0.0, 0.0, 0.24);
        ngfx_fill_circle(cr, sl_x + bw_fill, py + 47, 8.2, 1, 1, 1, 0.98);
        ngfx_fill_circle(cr, sl_x + bw_fill, py + 47, 3.0, 0.94, 0.72, 0.22, 0.95);
        nature_add_hit(st, sl_x, py + 33, sl_w, 28, HIT_SLIDER_BRIGHT, sl_x, sl_w);

        py += 84;
        ngfx_fill_rect(cr, px, py, pw, 62, 11, dark ? 1 : 0, dark ? 1 : 0, dark ? 1 : 0, 0.045);
        ngfx_stroke_rect(cr, px + 0.5, py + 0.5, pw - 1, 61, 11, 1.0, dark ? 1 : 0, dark ? 1 : 0, dark ? 1 : 0, 0.08);
        ngfx_text_clipped(cr, "Noto Sans", 1, 12.8, px + 14, py + 24, pw - 86, fg_r, fg_g, fg_b, 1.0, "Night Light (3800K Shift)");
        ngfx_text_clipped(cr, "Noto Sans", 0, 10.8, px + 14, py + 44, pw - 86, mut_r, mut_g, mut_b, 1.0, "Warm screen tint for low-light comfort");
        ngfx_toggle_switch(cr, px + pw - 62, py + 19, 46, 24, st->cfg.night_light, 0.92, 0.54, 0.20, dark);
        nature_add_hit(st, px + pw - 68, py + 15, 58, 32, HIT_SETTINGS_NIGHT_LIGHT, 0, 0);
    } else if (st->settings_section == 2) {
        py += 20;
        ngfx_fill_rect(cr, px, py, pw, 78, 10, dark ? 1 : 0, dark ? 1 : 0, dark ? 1 : 0, 0.05);
        char vb[64];
        snprintf(vb, sizeof(vb), "Master Volume: %d%% (%s)", st->cfg.volume, st->cfg.audio_muted ? "Muted" : "Active");
        ngfx_text(cr, "Noto Sans", 1, 12.8, px + 14, py + 24, fg_r, fg_g, fg_b, 1.0, vb);
        ngfx_fill_rect(cr, px + pw - 84, py + 10, 72, 24, 6,
                       st->cfg.audio_muted ? 0.85 : (dark ? 1 : 0),
                       st->cfg.audio_muted ? 0.28 : (dark ? 1 : 0),
                       st->cfg.audio_muted ? 0.28 : (dark ? 1 : 0),
                       st->cfg.audio_muted ? 0.85 : 0.12);
        ngfx_text(cr, "Noto Sans", 1, 10.5, px + pw - 70, py + 26,
                  st->cfg.audio_muted ? 1.0 : fg_r, st->cfg.audio_muted ? 1.0 : fg_g, st->cfg.audio_muted ? 1.0 : fg_b,
                  1.0, st->cfg.audio_muted ? "Unmute" : "Mute");
        nature_add_hit(st, px + pw - 84, py + 10, 72, 24, HIT_SETTINGS_MUTE, 0, 0);

        int sl_x = px + 14, sl_w = pw - 28;
        ngfx_fill_rect(cr, sl_x, py + 47, sl_w, 12, 6, dark ? 1 : 0, dark ? 1 : 0, dark ? 1 : 0, 0.14);
        double vol_fill = (sl_w * st->cfg.volume) / 100.0;
        if (vol_fill > 0) {
            ngfx_fill_gradient(cr, sl_x, py + 47, vol_fill, 12, 6,
                               ar, ag, ab, 0.95,
                               ar * 1.25 > 1 ? 1 : ar * 1.25, ag * 1.25 > 1 ? 1 : ag * 1.25, ab * 1.25 > 1 ? 1 : ab * 1.25, 0.98, 1);
        }
        ngfx_fill_circle(cr, sl_x + vol_fill, py + 54, 9.0, 0.0, 0.0, 0.0, 0.24);
        ngfx_fill_circle(cr, sl_x + vol_fill, py + 53, 8.2, 1, 1, 1, 0.98);
        ngfx_fill_circle(cr, sl_x + vol_fill, py + 53, 3.0, ar, ag, ab, 0.95);
        nature_add_hit(st, sl_x, py + 39, sl_w, 28, HIT_SLIDER_VOL, sl_x, sl_w);

        py += 92;
        ngfx_text(cr, "Noto Sans", 1, 12.0, px, py, mut_r, mut_g, mut_b, 1.0, "PipeWire Output Sink");
        py += 12;
        for (int d = 0; d < 3; ++d) {
            int act = st->cfg.audio_device == d;
            ngfx_fill_rect(cr, px, py, pw, 40, 9,
                           act ? ar : (dark ? 1 : 0), act ? ag : (dark ? 1 : 0), act ? ab : (dark ? 1 : 0),
                           act ? 0.22 : 0.045);
            ngfx_stroke_rect(cr, px + 0.5, py + 0.5, pw - 1, 39, 9, 1.0,
                             act ? ar : (dark ? 1 : 0), act ? ag : (dark ? 1 : 0), act ? ab : (dark ? 1 : 0),
                             act ? 0.72 : 0.07);
            ngfx_fill_circle(cr, px + 18, py + 20, 4.5,
                             act ? ar * 1.3 > 1 ? 1 : ar * 1.3 : mut_r,
                             act ? ag * 1.3 > 1 ? 1 : ag * 1.3 : mut_g,
                             act ? ab * 1.3 > 1 ? 1 : ab * 1.3 : mut_b, act ? 1.0 : 0.35);
            ngfx_text_clipped(cr, "Noto Sans", act, 11.8, px + 32, py + 24, pw - 44, fg_r, fg_g, fg_b, 1.0, AUDIO_DEVS[d]);
            nature_add_hit(st, px, py, pw, 40, HIT_SETTINGS_AUDIO_DEV, d, 0);
            py += 46;
        }
    } else if (st->settings_section == 3) {
        py += 20;
        ngfx_text(cr, "Noto Sans", 1, 12.0, px, py, mut_r, mut_g, mut_b, 1.0, "Power & Scheduler Profile");
        py += 12;
        const char *pmodes[3] = {"Power Saver", "Balanced", "Performance"};
        int p_w = (pw - 16) / 3;
        for (int m = 0; m < 3; ++m) {
            int mx = px + m * (p_w + 8);
            int act = st->cfg.power_profile == m;
            ngfx_fill_rect(cr, mx, py, p_w, 34, 8,
                           act ? ar : (dark ? 1 : 0), act ? ag : (dark ? 1 : 0), act ? ab : (dark ? 1 : 0),
                           act ? 0.90 : 0.07);
            if (!act) ngfx_stroke_rect(cr, mx + 0.5, py + 0.5, p_w - 1, 33, 8, 1.0, dark ? 1 : 0, dark ? 1 : 0, dark ? 1 : 0, 0.08);
            ngfx_text_clipped(cr, "Noto Sans", 1, 11.2, mx + 10, py + 21, p_w - 16,
                              act ? 1.0 : fg_r, act ? 1.0 : fg_g, act ? 1.0 : fg_b, 1.0, pmodes[m]);
            nature_add_hit(st, mx, py, p_w, 34, HIT_SETTINGS_PERF_MODE, m, 0);
        }

        py += 48;
        struct { const char *label; const char *desc; int val; int hit; } rows[2] = {
            {"Low-Power Compositor", "Reduce blur layers for minimum idle wattage", st->cfg.low_power_mode, HIT_SETTINGS_LOW_POWER},
            {"zram LZ4 Memory Pool", "Compress cold pages in RAM at 2.1:1 ratio", st->cfg.zram_enabled, HIT_SETTINGS_ZRAM},
        };
        for (int i = 0; i < 2; ++i) {
            ngfx_fill_rect(cr, px, py, pw, 62, 11, dark ? 1 : 0, dark ? 1 : 0, dark ? 1 : 0, 0.045);
            ngfx_stroke_rect(cr, px + 0.5, py + 0.5, pw - 1, 61, 11, 1.0, dark ? 1 : 0, dark ? 1 : 0, dark ? 1 : 0, 0.08);
            ngfx_text_clipped(cr, "Noto Sans", 1, 12.8, px + 14, py + 24, pw - 86, fg_r, fg_g, fg_b, 1.0, rows[i].label);
            ngfx_text_clipped(cr, "Noto Sans", 0, 10.8, px + 14, py + 44, pw - 86, mut_r, mut_g, mut_b, 1.0, rows[i].desc);
            ngfx_toggle_switch(cr, px + pw - 62, py + 19, 46, 24, rows[i].val, ar, ag, ab, dark);
            nature_add_hit(st, px + pw - 68, py + 15, 58, 32, rows[i].hit, 0, 0);
            py += 72;
        }
    } else if (st->settings_section == 4) {
        py += 20;
        ngfx_fill_rect(cr, px, py, pw, 76, 11, dark ? 1 : 0, dark ? 1 : 0, dark ? 1 : 0, 0.045);
        ngfx_stroke_rect(cr, px + 0.5, py + 0.5, pw - 1, 75, 11, 1.0, dark ? 1 : 0, dark ? 1 : 0, dark ? 1 : 0, 0.08);
        ngfx_text_clipped(cr, "Noto Sans", 1, 13.0, px + 14, py + 28, pw - 86, fg_r, fg_g, fg_b, 1.0, "NetworkManager Wi-Fi (wlp2s0)");
        ngfx_text_clipped(cr, "JetBrains Mono", 0, 10.8, px + 14, py + 50, pw - 86, mut_r, mut_g, mut_b, 1.0,
                          st->cfg.wifi_enabled ? "SSID: Canopy-5GHz · 10.42.0.14" : "Wi-Fi Radio Disabled");
        ngfx_toggle_switch(cr, px + pw - 62, py + 26, 46, 24, st->cfg.wifi_enabled, ar, ag, ab, dark);
        nature_add_hit(st, px + pw - 68, py + 22, 58, 32, HIT_SETTINGS_WIFI, 0, 0);

        py += 88;
        ngfx_fill_rect(cr, px, py, pw, 76, 11, dark ? 1 : 0, dark ? 1 : 0, dark ? 1 : 0, 0.045);
        ngfx_stroke_rect(cr, px + 0.5, py + 0.5, pw - 1, 75, 11, 1.0, dark ? 1 : 0, dark ? 1 : 0, dark ? 1 : 0, 0.08);
        ngfx_text_clipped(cr, "Noto Sans", 1, 13.0, px + 14, py + 28, pw - 86, fg_r, fg_g, fg_b, 1.0, "Bluetooth Controller (BlueZ)");
        ngfx_text_clipped(cr, "JetBrains Mono", 0, 10.8, px + 14, py + 50, pw - 86, mut_r, mut_g, mut_b, 1.0,
                          st->cfg.bluetooth_enabled ? "Paired: Keychron K3 Pro · Buds" : "Bluetooth Adapter Powered Off");
        ngfx_toggle_switch(cr, px + pw - 62, py + 26, 46, 24, st->cfg.bluetooth_enabled, ar, ag, ab, dark);
        nature_add_hit(st, px + pw - 68, py + 22, 58, 32, HIT_SETTINGS_BLUETOOTH, 0, 0);
    } else {
        py += 20;
        if (st->avatar_surface) {
            g_ngfx.save(cr);
            ngfx_rounded_path(cr, px, py, 52, 52, 26);
            g_ngfx.clip(cr);
            double sw = g_ngfx.image_surface_get_width(st->avatar_surface);
            g_ngfx.translate(cr, px, py);
            g_ngfx.scale(cr, 52.0 / sw, 52.0 / sw);
            g_ngfx.set_source_surface(cr, st->avatar_surface, 0, 0);
            g_ngfx.paint(cr);
            g_ngfx.restore(cr);
            ngfx_stroke_rect(cr, px, py, 52, 52, 26, 1.8, ar, ag, ab, 0.85);
        }
        ngfx_text_clipped(cr, "Plus Jakarta Sans", 1, 15.0, px + 66, py + 22, pw - 190, fg_r, fg_g, fg_b, 1.0, "Nature OS 24.04.1 LTS");
        ngfx_text_clipped(cr, "Noto Sans", 0, 11.0, px + 66, py + 42, pw - 190, mut_r, mut_g, mut_b, 1.0, "Ubuntu 24.04 Noble · C17/C++20");

        ngfx_fill_rect(cr, px + pw - 114, py + 12, 114, 28, 8, ar, ag, ab, 0.92);
        ngfx_text(cr, "Noto Sans", 1, 10.8, px + pw - 102, py + 30, 1, 1, 1, 1,
                  st->settings_update_checked ? "Up to Date" : "Check Updates");
        nature_add_hit(st, px + pw - 114, py + 12, 114, 28, HIT_SETTINGS_CHECK_UPDATE, 0, 0);

        py += 68;
        char mem_spec[64], cpu_spec[96];
        snprintf(cpu_spec, sizeof(cpu_spec), "%d Cores (%.60s)", st->cpu_count > 0 ? st->cpu_count : 4, st->cpu_model);
        snprintf(mem_spec, sizeof(mem_spec), "%.1f GiB (/proc) · OS RSS %d MiB", st->total_ram_mb / 1024.0, st->os_rss_mb);
        const char *specs[5][2] = {
            {"Base Distro", "Ubuntu 24.04.1 LTS (APT / dpkg)"},
            {"Linux Kernel", st->kernel_ver},
            {"Processor", cpu_spec},
            {"Compositor", "nature-compositor (C17/Cairo NAT3)"},
            {"Memory Pool", mem_spec},
        };
        for (int i = 0; i < 5; ++i) {
            ngfx_fill_rect(cr, px, py, pw, 36, 9, dark ? 1 : 0, dark ? 1 : 0, dark ? 1 : 0, 0.045);
            ngfx_stroke_rect(cr, px + 0.5, py + 0.5, pw - 1, 35, 9, 1.0, dark ? 1 : 0, dark ? 1 : 0, dark ? 1 : 0, 0.06);
            ngfx_text(cr, "Noto Sans", 1, 11.2, px + 12, py + 22, mut_r, mut_g, mut_b, 1.0, specs[i][0]);
            ngfx_text_clipped(cr, "JetBrains Mono", 0, 10.6, px + 116, py + 22, pw - 128, fg_r, fg_g, fg_b, 0.95, specs[i][1]);
            py += 42;
        }
    }

    g_ngfx.restore(cr);

    int vis_steps = (ch - 24) / step_px;
    int tot_steps = total_h / step_px;
    nature_draw_scrollbar(st, cr, cx + cw - 11, cy + 8, ch - 16,
                          tot_steps, vis_steps, st->settings_scroll, max_scroll, SCROLL_SETTINGS, dark);
}

void nature_settings_click(NatureOSState *st, const HitRect *hit) {
    switch (hit->action_id) {
        case HIT_SETTINGS_SECTION:
            st->settings_section = hit->arg0;
            st->settings_scroll = 0;
            break;
        case HIT_SETTINGS_DARK: st->cfg.dark_mode = !st->cfg.dark_mode; break;
        case HIT_SETTINGS_NIGHT_LIGHT: st->cfg.night_light = !st->cfg.night_light; break;
        case HIT_SETTINGS_WALLPAPER: st->cfg.wallpaper_idx = hit->arg0; break;
        case HIT_SETTINGS_ACCENT: st->cfg.accent_idx = hit->arg0; break;
        case HIT_SETTINGS_DOCK_POS: st->cfg.dock_position = !st->cfg.dock_position; break;
        case HIT_SETTINGS_LOW_POWER: st->cfg.low_power_mode = !st->cfg.low_power_mode; break;
        case HIT_SETTINGS_ZRAM: st->cfg.zram_enabled = !st->cfg.zram_enabled; break;
        case HIT_SETTINGS_WIFI: st->cfg.wifi_enabled = !st->cfg.wifi_enabled; break;
        case HIT_SETTINGS_BLUETOOTH: st->cfg.bluetooth_enabled = !st->cfg.bluetooth_enabled; break;
        case HIT_SETTINGS_HZ: st->cfg.target_hz = hit->arg0; break;
        case HIT_SETTINGS_BRIGHTNESS: st->cfg.brightness = hit->arg0; break;
        case HIT_SETTINGS_VOLUME: st->cfg.volume = hit->arg0; st->cfg.audio_muted = 0; break;
        case HIT_SETTINGS_MUTE: st->cfg.audio_muted = !st->cfg.audio_muted; break;
        case HIT_SETTINGS_AUDIO_DEV:
            st->cfg.audio_device = hit->arg0;
            nature_notify(st, "PipeWire Audio", AUDIO_DEVS[hit->arg0 % 3]);
            break;
        case HIT_SETTINGS_PERF_MODE:
            st->cfg.power_profile = hit->arg0 % 3;
            break;
        case HIT_SETTINGS_CHECK_UPDATE:
            st->settings_update_checked = 1;
            nature_notify(st, "Ubuntu APT", "All 1,412 system packages are up to date (noble-security).");
            break;
        default: break;
    }
}

void nature_settings_wheel(NatureOSState *st, int delta_y) {
    if (delta_y > 0) {
        if (st->settings_scroll < st->settings_max_scroll) st->settings_scroll++;
    } else if (delta_y < 0 && st->settings_scroll > 0) {
        st->settings_scroll--;
    }
}
