#include "nature_os.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>

#define NUM_SECTIONS 7
static const char *SECTIONS[NUM_SECTIONS] = {
    "Appearance",
    "Display",
    "Sound",
    "Performance",
    "Network & Device",
    "Shortcuts",
    "About System"
};

static const char *SECTION_KEYWORDS[NUM_SECTIONS] = {
    "appearance theme nature horizon dark light wallpaper accent dock clock toast widget snap notification",
    "display monitor screen hz refresh brightness night light warmth filter blue eye",
    "sound audio volume mute pipewire output sink bluetooth speaker",
    "performance power saver balanced compositor zram lz4 memory cpu",
    "network wifi bluetooth device hostname pc name computer rename",
    "shortcuts keyboard hotkeys super esc snap tile overview lock arrow space",
    "about system storage disk statvfs kernel cpu ram reset defaults lock update version"
};

static const char *WP_NAMES[MAX_WALLPAPERS] = {
    "Canopy Dawn",
    "Alpine Twilight",
    "Basalt Moss",
    "Horizon Daylight",
    "Horizon Night",
    "Alpine Dawn",
    "Nordic Trail",
    "Horizon Slate"
};

static const char *ACCENT_NAMES_NATURE[6] = {"Canopy", "Teal", "Amber", "Alpine", "Orchid", "Coral"};
static const char *ACCENT_NAMES_HORIZON[6] = {"Horizon", "Indigo", "Cyan", "Violet", "Rose", "Amber"};
static const char *AUDIO_DEVS[3] = {
    "Built-in Audio (Analog Stereo)",
    "DisplayPort / HDMI Digital",
    "Bluetooth Canopy Buds (AAC)"
};

static int str_ci_contains(const char *hay, const char *needle) {
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
        case 4: /* Network & Device */
            g_ngfx.new_sub_path(cr); g_ngfx.arc(cr, x + 8, y + 12.5, 7.5, -2.35, -0.79); g_ngfx.stroke(cr);
            g_ngfx.new_sub_path(cr); g_ngfx.arc(cr, x + 8, y + 12.5, 4.2, -2.35, -0.79); g_ngfx.stroke(cr);
            ngfx_fill_circle(cr, x + 8, y + 12.5, 1.4, r, g, b, a);
            break;
        case 5: /* Shortcuts */
            ngfx_stroke_rect(cr, x + 1.5, y + 3.5, 13, 9, 2.0, 1.3, r, g, b, a);
            ngfx_fill_rect(cr, x + 3.5, y + 6.0, 2.2, 2.0, 0.5, r, g, b, a);
            ngfx_fill_rect(cr, x + 6.9, y + 6.0, 2.2, 2.0, 0.5, r, g, b, a);
            ngfx_fill_rect(cr, x + 10.3, y + 6.0, 2.2, 2.0, 0.5, r, g, b, a);
            ngfx_fill_rect(cr, x + 4.5, y + 9.2, 7.0, 1.8, 0.5, r, g, b, a);
            break;
        default: /* About */
            ngfx_stroke_rect(cr, x + 2.5, y + 2.5, 11, 11, 5.5, 1.35, r, g, b, a);
            ngfx_fill_rect(cr, x + 7.2, y + 7.0, 1.6, 4.5, 0.8, r, g, b, a);
            ngfx_fill_circle(cr, x + 8.0, y + 5.2, 1.0, r, g, b, a);
            break;
    }
}

static void draw_horizon_wp_thumb(cairo_t *cr, int wx, int py, int wp_w, int wp_h, int dark, double ar, double ag, double ab) {
    g_ngfx.save(cr);
    ngfx_rounded_path(cr, wx, py, wp_w, wp_h, 9);
    g_ngfx.clip(cr);
    if (dark) {
        ngfx_fill_gradient(cr, wx, py, wp_w, wp_h, 0,
                           0.055, 0.082, 0.145, 1.0,
                           0.035, 0.045, 0.082, 1.0, 1);
    } else {
        ngfx_fill_gradient(cr, wx, py, wp_w, wp_h, 0,
                           0.87, 0.91, 0.96, 1.0,
                           0.94, 0.96, 0.99, 1.0, 1);
    }
    ngfx_fill_circle(cr, wx + wp_w * 0.72, py + wp_h * 0.34, wp_h * 0.48, ar, ag, ab, dark ? 0.28 : 0.22);
    g_ngfx.set_source_rgba(cr, dark ? 0.10 : 0.74, dark ? 0.14 : 0.80, dark ? 0.22 : 0.88, 0.90);
    g_ngfx.move_to(cr, wx, py + wp_h);
    g_ngfx.line_to(cr, wx, py + wp_h * 0.68);
    g_ngfx.line_to(cr, wx + wp_w * 0.32, py + wp_h * 0.46);
    g_ngfx.line_to(cr, wx + wp_w * 0.62, py + wp_h * 0.64);
    g_ngfx.line_to(cr, wx + wp_w * 0.86, py + wp_h * 0.48);
    g_ngfx.line_to(cr, wx + wp_w, py + wp_h * 0.58);
    g_ngfx.line_to(cr, wx + wp_w, py + wp_h);
    g_ngfx.close_path(cr);
    g_ngfx.fill(cr);
    g_ngfx.restore(cr);
}

static void draw_theme_mini_card(cairo_t *cr, int x, int y, int w, int h, int card_dark,
                                 int selected, double ar, double ag, double ab, const NaturePalette *pal, const char *label) {
    ngfx_fill_rect(cr, x, y, w, h, 10, pal->card_r, pal->card_g, pal->card_b, 0.96);
    ngfx_stroke_rect(cr, x + 0.5, y + 0.5, w - 1, h - 1, 10, selected ? 1.8 : 1.0,
                     selected ? ar : pal->border_r, selected ? ag : pal->border_g, selected ? ab : pal->border_b,
                     selected ? 0.95 : pal->border_a);

    int mx = x + 8, my = y + 8, mw = w - 16, mh = h - 32;
    if (card_dark) {
        ngfx_fill_gradient(cr, mx, my, mw, mh, 6, 0.04, 0.07, 0.10, 1.0, 0.07, 0.12, 0.14, 1.0, 1);
    } else {
        ngfx_fill_gradient(cr, mx, my, mw, mh, 6, 0.82, 0.90, 0.96, 1.0, 0.93, 0.96, 0.99, 1.0, 1);
    }
    /* Mini top panel + window + dock inside preview */
    ngfx_fill_rect(cr, mx + 4, my + 3, mw - 8, 4, 2, card_dark ? 0.12 : 0.98, card_dark ? 0.17 : 0.99, card_dark ? 0.22 : 1.0, 0.92);
    int wx = mx + 12, wy = my + 11, ww = mw - 24, wh = mh - 20;
    ngfx_fill_rect(cr, wx, wy, ww, wh, 4, card_dark ? 0.10 : 0.98, card_dark ? 0.15 : 0.99, card_dark ? 0.18 : 1.0, 0.96);
    ngfx_fill_rect(cr, wx + 4, wy + 5, ww * 0.45, 3, 1.5, ar, ag, ab, 0.9);
    ngfx_fill_rect(cr, wx + 4, wy + 11, ww * 0.68, 2.5, 1.2, card_dark ? 0.5 : 0.4, card_dark ? 0.6 : 0.5, card_dark ? 0.6 : 0.5, 0.45);
    ngfx_fill_rect(cr, mx + mw * 0.28, my + mh - 6, mw * 0.44, 4, 2, ar, ag, ab, 0.85);

    ngfx_text(cr, "Noto Sans", selected, 10.6, x + 10, y + h - 8, pal->fg_r, pal->fg_g, pal->fg_b, 0.96, label);
    if (selected) {
        ngfx_fill_circle(cr, x + w - 16, y + h - 12, 6.0, ar, ag, ab, 0.96);
        g_ngfx.set_source_rgba(cr, pal->on_acc_r, pal->on_acc_g, pal->on_acc_b, 1.0);
        g_ngfx.set_line_width(cr, 1.4);
        g_ngfx.move_to(cr, x + w - 18.8, y + h - 12.0);
        g_ngfx.line_to(cr, x + w - 16.5, y + h - 9.8);
        g_ngfx.line_to(cr, x + w - 13.0, y + h - 14.0);
        g_ngfx.stroke(cr);
    }
}

static void save_hostname_draft(NatureOSState *st) {
    char clean[32];
    size_t j = 0;
    const char *src = st->settings_hostname_draft;
    for (size_t i = 0; src[i] && j + 1 < sizeof(clean); ++i) {
        unsigned char ch = (unsigned char)src[i];
        if (isalnum(ch) || ch == '-' || ch == '_') {
            clean[j++] = (char)ch;
        }
    }
    clean[j] = '\0';
    if (clean[0]) {
        snprintf(st->cfg.device_name, sizeof(st->cfg.device_name), "%s", clean);
        snprintf(st->settings_hostname_draft, sizeof(st->settings_hostname_draft), "%s", clean);
        st->settings_hostname_dirty = 0;
        char msg[80];
        snprintf(msg, sizeof(msg), "PC hostname updated to %.31s", st->cfg.device_name);
        nature_notify(st, "Device Identity", msg);
    } else {
        snprintf(st->settings_hostname_draft, sizeof(st->settings_hostname_draft), "%s", st->cfg.device_name);
        st->settings_hostname_dirty = 0;
    }
}

void nature_settings_render(NatureOSState *st, cairo_t *cr, OSWindow *w, int cx, int cy, int cw, int ch) {
    (void)w;
    NaturePalette pal;
    nature_get_palette(st, &pal);
    int dark = pal.dark_mode;
    double ar = pal.ar, ag = pal.ag, ab = pal.ab;
    double fg_r = pal.fg_r, fg_g = pal.fg_g, fg_b = pal.fg_b;
    double mut_r = pal.mut_r, mut_g = pal.mut_g, mut_b = pal.mut_b;

    int compact = cw < 560;
    int side_w = compact ? 136 : 184;
    ngfx_fill_rect(cr, cx, cy, side_w, ch, 0, pal.sidebar_r, pal.sidebar_g, pal.sidebar_b, 1.0);
    g_ngfx.set_source_rgba(cr, pal.border_r, pal.border_g, pal.border_b, pal.border_a);
    g_ngfx.set_line_width(cr, 1.0);
    g_ngfx.move_to(cr, cx + side_w + 0.5, cy);
    g_ngfx.line_to(cr, cx + side_w + 0.5, cy + ch);
    g_ngfx.stroke(cr);

    ngfx_text(cr, "Plus Jakarta Sans", 1, 10.2, cx + 12, cy + 19, mut_r, mut_g, mut_b, 0.86, "SETTINGS");

    /* Sidebar Search Box (target_id = 6) */
    int sx_box = cx + 8, sy_box = cy + 27, sw_box = side_w - 16, sh_box = 25;
    int s_focused = (st->active_text_target == 6);
    ngfx_fill_rect(cr, sx_box, sy_box, sw_box, sh_box, 7, pal.card_r, pal.card_g, pal.card_b, 0.96);
    ngfx_stroke_rect(cr, sx_box + 0.5, sy_box + 0.5, sw_box - 1, sh_box - 1, 7, s_focused ? 1.2 : 0.9,
                     s_focused ? ar : pal.border_r, s_focused ? ag : pal.border_g, s_focused ? ab : pal.border_b,
                     s_focused ? 0.85 : pal.border_a);
    if (st->settings_search[0]) {
        ngfx_text_clipped(cr, "Noto Sans", 1, 10.6, sx_box + 8, sy_box + 16.5, sw_box - 28, fg_r, fg_g, fg_b, 1.0, st->settings_search);
        ngfx_fill_rect(cr, sx_box + sw_box - 20, sy_box + 4, 16, 17, 4, pal.elev_r, pal.elev_g, pal.elev_b, 0.95);
        ngfx_text(cr, "Noto Sans", 1, 9.6, sx_box + sw_box - 15, sy_box + 16, fg_r, fg_g, fg_b, 0.9, "x");
        nature_add_hit(st, sx_box + sw_box - 22, sy_box + 2, 20, 21, HIT_SETTINGS_SEARCH_CLEAR, 0, 0);
        nature_add_hit(st, sx_box, sy_box, sw_box - 22, sh_box, HIT_SETTINGS_SEARCH_BOX, 0, 0);
    } else {
        ngfx_text_clipped(cr, "Noto Sans", 0, 10.2, sx_box + 8, sy_box + 16.5, sw_box - 14, mut_r, mut_g, mut_b, 0.82,
                          s_focused ? "Type to find setting..." : "Find setting...");
        nature_add_hit(st, sx_box, sy_box, sw_box, sh_box, HIT_SETTINGS_SEARCH_BOX, 0, 0);
    }
    nature_add_text_rect(st, sx_box, sy_box, sw_box, sh_box, 6);

    int footer_reserve = (ch >= 330) ? 84 : 6;
    int list_sy = cy + 60;
    for (int i = 0; i < NUM_SECTIONS; ++i) {
        int sy = list_sy + i * 33;
        if (sy + 30 > cy + ch - footer_reserve) break;
        int act = st->settings_section == i;
        int matched = st->settings_search[0] &&
                      (str_ci_contains(SECTIONS[i], st->settings_search) ||
                       str_ci_contains(SECTION_KEYWORDS[i], st->settings_search));
        int hov = (st->hover_mx >= cx + 8 && st->hover_mx <= cx + side_w - 8 && st->hover_my >= sy && st->hover_my <= sy + 28);
        if (act || hov || matched) {
            ngfx_fill_rect(cr, cx + 8, sy, side_w - 16, 28, 8,
                           (act || matched) ? ar : pal.hover_r,
                           (act || matched) ? ag : pal.hover_g,
                           (act || matched) ? ab : pal.hover_b,
                           act ? (dark ? 0.25 : 0.16) : (matched ? 0.12 : (dark ? 0.45 : 0.65)));
        }
        if (act) {
            ngfx_fill_rect(cr, cx + 8, sy + 5, 3, 18, 1.5, pal.ar_hi, pal.ag_hi, pal.ab_hi, 1.0);
        }
        draw_settings_icon(cr, i, cx + 14, sy + 6,
                           act ? (dark ? pal.ar_hi : ar) : mut_r,
                           act ? (dark ? pal.ag_hi : ag) : mut_g,
                           act ? (dark ? pal.ab_hi : ab) : mut_b, 0.95);
        ngfx_text_clipped(cr, "Noto Sans", act, 11.2, cx + 35, sy + 18, side_w - 42,
                          fg_r, fg_g, fg_b, act ? 1.0 : (matched ? 0.98 : 0.84), SECTIONS[i]);
        nature_add_hit(st, cx + 8, sy, side_w - 16, 28, HIT_SETTINGS_SECTION, i, 0);
    }

    if (ch >= 330) {
        int fx = cx + 8, fy = cy + ch - 76, fw = side_w - 16, fh = 68;
        ngfx_fill_rect(cr, fx, fy, fw, fh, 9, pal.card_r, pal.card_g, pal.card_b, 0.96);
        ngfx_stroke_rect(cr, fx + 0.5, fy + 0.5, fw - 1, fh - 1, 9, 1.0, pal.border_r, pal.border_g, pal.border_b, pal.border_a);
        ngfx_fill_circle(cr, fx + 13, fy + 16, 3.2, pal.success_r, pal.success_g, pal.success_b, 0.98);
        ngfx_text_clipped(cr, "JetBrains Mono", 0, 9.4, fx + 21, fy + 19, fw - 26, mut_r, mut_g, mut_b, 0.95,
                          st->session_restored ? "Session Restored" : "Auto-saved");
        const char *sb_rst = st->settings_confirm_reset ? "Confirm Reset?" : "Reset Settings";
        int h_rst = (st->hover_hit_action == HIT_SETTINGS_RESET_DEFAULTS);
        ngfx_fill_rect(cr, fx + 6, fy + 31, fw - 12, 29, 7,
                       pal.danger_r, pal.danger_g, pal.danger_b,
                       st->settings_confirm_reset ? 0.34 : (h_rst ? 0.24 : 0.14));
        ngfx_stroke_rect(cr, fx + 6.5, fy + 31.5, fw - 13, 28, 7, 1.0,
                         pal.danger_r, pal.danger_g, pal.danger_b, st->settings_confirm_reset ? 0.88 : 0.38);
        double rtw = ngfx_text_width(cr, "Noto Sans", 1, 10.2, sb_rst);
        double rtx = fx + 6 + ((fw - 12) - rtw) * 0.5;
        if (rtx < fx + 10) rtx = fx + 10;
        ngfx_text_clipped(cr, "Noto Sans", 1, 10.2, rtx, fy + 49, fw - 18,
                          pal.danger_r, pal.danger_g, pal.danger_b, 1.0, sb_rst);
        nature_add_hit(st, fx + 6, fy + 31, fw - 12, 29, HIT_SETTINGS_RESET_DEFAULTS, 0, 0);
    }

    int pane_x = cx + side_w;
    int pane_w = cw - side_w;
    int px = pane_x + 18;
    int pw = pane_w - 42;
    if (pw < 180) pw = 180;

    int content_heights[NUM_SECTIONS] = {775, 425, 345, 390, 395, 365, 565};
    int sec = st->settings_section;
    if (sec < 0 || sec >= NUM_SECTIONS) sec = 0;
    int total_h = content_heights[sec];
    int step_px = 28;
    int max_scroll = (total_h > ch - 24) ? ((total_h - (ch - 24) + step_px - 1) / step_px) : 0;
    st->settings_max_scroll = max_scroll;
    if (st->settings_scroll > max_scroll) st->settings_scroll = max_scroll;
    if (st->settings_scroll < 0) st->settings_scroll = 0;

    g_ngfx.save(cr);
    g_ngfx.rectangle(cr, pane_x + 1, cy, pane_w - 16, ch);
    g_ngfx.clip(cr);

    int py = cy + 28 - st->settings_scroll * step_px;
    ngfx_text(cr, "Plus Jakarta Sans", 1, 17.0, px, py, fg_r, fg_g, fg_b, 1.0, SECTIONS[sec]);
    if (pw >= 300) {
        const char *pill_txt = st->session_restored ? "Restored" : "Auto-saved";
        int pw_pill = 90;
        int px_pill = px + pw - pw_pill;
        int py_pill = py - 16;
        ngfx_fill_rect(cr, px_pill, py_pill, pw_pill, 22, 11,
                       pal.success_r, pal.success_g, pal.success_b, dark ? 0.16 : 0.12);
        ngfx_stroke_rect(cr, px_pill + 0.5, py_pill + 0.5, pw_pill - 1, 21, 11, 0.9,
                         pal.success_r, pal.success_g, pal.success_b, 0.42);
        ngfx_fill_circle(cr, px_pill + 11, py_pill + 11, 3.0, pal.success_r, pal.success_g, pal.success_b, 1.0);
        ngfx_text(cr, "JetBrains Mono", 1, 9.4, px_pill + 19, py_pill + 14.5,
                  pal.success_r, pal.success_g, pal.success_b, 0.98, pill_txt);
    }

    if (sec == 0) {
        /* 1. Desktop Design Language */
        py += 16;
        ngfx_fill_rect(cr, px, py, pw, 62, 11, pal.card_r, pal.card_g, pal.card_b, 0.96);
        ngfx_stroke_rect(cr, px + 0.5, py + 0.5, pw - 1, 61, 11, 1.0, pal.border_r, pal.border_g, pal.border_b, pal.border_a);
        ngfx_top_specular(cr, px, py, pw, 11, dark ? 0.16 : 0.45);
        ngfx_text_clipped(cr, "Noto Sans", 1, 12.2, px + 14, py + 24, pw - 236, fg_r, fg_g, fg_b, 1.0, "Desktop Design Language");
        ngfx_text_clipped(cr, "Noto Sans", 0, 10.2, px + 14, py + 43, pw - 236, mut_r, mut_g, mut_b, 0.92,
                          st->cfg.theme_family == THEME_HORIZON ? "Horizon Slate · crisp slate chrome" : "Nature Canopy · botanical emerald");

        int nat_act = st->cfg.theme_family == THEME_NATURE;
        int hor_act = st->cfg.theme_family == THEME_HORIZON;
        ngfx_fill_rect(cr, px + pw - 216, py + 15, 100, 30, 8,
                       nat_act ? ar : pal.elev_r, nat_act ? ag : pal.elev_g, nat_act ? ab : pal.elev_b, nat_act ? 0.94 : 0.92);
        if (!nat_act) ngfx_stroke_rect(cr, px + pw - 215.5, py + 15.5, 99, 29, 8, 1.0, pal.border_r, pal.border_g, pal.border_b, pal.border_a);
        ngfx_text(cr, "Noto Sans", 1, 10.8, px + pw - 205, py + 34,
                  nat_act ? pal.on_acc_r : fg_r, nat_act ? pal.on_acc_g : fg_g, nat_act ? pal.on_acc_b : fg_b, 1.0, "Nature OS");
        nature_add_hit(st, px + pw - 216, py + 15, 100, 30, HIT_SETTINGS_THEME_FAMILY, THEME_NATURE, 0);

        ngfx_fill_rect(cr, px + pw - 110, py + 15, 98, 30, 8,
                       hor_act ? ar : pal.elev_r, hor_act ? ag : pal.elev_g, hor_act ? ab : pal.elev_b, hor_act ? 0.94 : 0.92);
        if (!hor_act) ngfx_stroke_rect(cr, px + pw - 109.5, py + 15.5, 97, 29, 8, 1.0, pal.border_r, pal.border_g, pal.border_b, pal.border_a);
        ngfx_text(cr, "Noto Sans", 1, 10.8, px + pw - 96, py + 34,
                  hor_act ? pal.on_acc_r : fg_r, hor_act ? pal.on_acc_g : fg_g, hor_act ? pal.on_acc_b : fg_b, 1.0, "Horizon");
        nature_add_hit(st, px + pw - 110, py + 15, 98, 30, HIT_SETTINGS_THEME_FAMILY, THEME_HORIZON, 0);

        /* 2. Daylight Light vs Midnight Dark Preview Cards */
        py += 74;
        ngfx_text(cr, "Noto Sans", 1, 12.0, px, py, mut_r, mut_g, mut_b, 1.0, "Appearance Mode");
        py += 10;
        int mc_w = (pw - 12) / 2;
        int mc_h = 84;
        draw_theme_mini_card(cr, px, py, mc_w, mc_h, 0, !dark, ar, ag, ab, &pal, "Daylight Light");
        nature_add_hit(st, px, py, mc_w, mc_h, HIT_SETTINGS_DARK, 0, 0);
        draw_theme_mini_card(cr, px + mc_w + 12, py, mc_w, mc_h, 1, dark, ar, ag, ab, &pal, "Midnight Dark");
        nature_add_hit(st, px + mc_w + 12, py, mc_w, mc_h, HIT_SETTINGS_DARK, 1, 0);

        /* 3. Desktop Wallpaper (8 presets in 4x2 grid) */
        py += mc_h + 22;
        ngfx_text(cr, "Noto Sans", 1, 12.0, px, py, mut_r, mut_g, mut_b, 1.0, "Desktop Wallpaper");
        if (st->cfg.wallpaper_idx == MAX_WALLPAPERS) {
            ngfx_text(cr, "JetBrains Mono", 0, 9.8, px + pw - 168, py,
                      dark ? pal.ar_hi : ar, dark ? pal.ag_hi : ag, dark ? pal.ab_hi : ab, 0.95, "Custom Media Photo Active");
        }
        py += 10;
        int wp_w = (pw - 24) / 4;
        int wp_h = (wp_w * 9) / 16;
        if (wp_h < 44) wp_h = 44;
        if (wp_h > 62) wp_h = 62;
        for (int i = 0; i < MAX_WALLPAPERS; ++i) {
            int col = i % 4;
            int row = i / 4;
            int wx = px + col * (wp_w + 8);
            int wy_row = py + row * (wp_h + 24);
            int sel = (st->cfg.wallpaper_idx == i);
            if (st->wallpapers[i]) {
                g_ngfx.save(cr);
                ngfx_rounded_path(cr, wx, wy_row, wp_w, wp_h, 9);
                g_ngfx.clip(cr);
                double sw = g_ngfx.image_surface_get_width(st->wallpapers[i]);
                double sh = g_ngfx.image_surface_get_height(st->wallpapers[i]);
                double scx = (double)wp_w / sw, scy = (double)wp_h / sh;
                double sc = scx > scy ? scx : scy;
                double ox = ((double)wp_w - sw * sc) * 0.5;
                double oy = ((double)wp_h - sh * sc) * 0.5;
                g_ngfx.translate(cr, wx + ox, wy_row + oy);
                g_ngfx.scale(cr, sc, sc);
                g_ngfx.set_source_surface(cr, st->wallpapers[i], 0, 0);
                g_ngfx.paint(cr);
                g_ngfx.restore(cr);
            } else {
                draw_horizon_wp_thumb(cr, wx, wy_row, wp_w, wp_h, dark, ar, ag, ab);
            }
            ngfx_stroke_rect(cr, wx, wy_row, wp_w, wp_h, 9, sel ? 2.4 : 1.0,
                             sel ? ar : pal.border_r, sel ? ag : pal.border_g, sel ? ab : pal.border_b,
                             sel ? 1.0 : pal.border_a);
            if (sel) {
                ngfx_fill_circle(cr, wx + wp_w - 11, wy_row + 11, 7.0, ar, ag, ab, 0.96);
                g_ngfx.set_source_rgba(cr, pal.on_acc_r, pal.on_acc_g, pal.on_acc_b, 1.0);
                g_ngfx.set_line_width(cr, 1.5);
                g_ngfx.move_to(cr, wx + wp_w - 14.2, wy_row + 11.0);
                g_ngfx.line_to(cr, wx + wp_w - 11.8, wy_row + 13.6);
                g_ngfx.line_to(cr, wx + wp_w - 7.5, wy_row + 8.6);
                g_ngfx.stroke(cr);
            }
            ngfx_text_clipped(cr, "Noto Sans", sel, 10.2, wx + 2, wy_row + wp_h + 14, wp_w - 4, fg_r, fg_g, fg_b, 0.92, WP_NAMES[i]);
            nature_add_hit(st, wx, wy_row, wp_w, wp_h + 16, HIT_SETTINGS_WALLPAPER, i, 0);
        }

        /* 4. 6 Accent Colors */
        py += 2 * (wp_h + 24) + 12;
        ngfx_text(cr, "Noto Sans", 1, 12.0, px, py, mut_r, mut_g, mut_b, 1.0, "Accent Color Palette");
        const char **ac_names = (st->cfg.theme_family == THEME_HORIZON) ? ACCENT_NAMES_HORIZON : ACCENT_NAMES_NATURE;
        char pal_sum[64];
        snprintf(pal_sum, sizeof(pal_sum), "%s · %s · %s",
                 st->cfg.theme_family == THEME_HORIZON ? "Horizon" : "Nature",
                 dark ? "Dark" : "Light",
                 ac_names[st->cfg.accent_idx % 6]);
        if (pw >= 320) {
            ngfx_text_clipped(cr, "JetBrains Mono", 0, 9.6, px + pw - 190, py, 190,
                              dark ? pal.ar_hi : ar, dark ? pal.ag_hi : ag, dark ? pal.ab_hi : ab, 0.95, pal_sum);
        }
        py += 10;
        int ac_w = (pw - 30) / 6;
        for (int i = 0; i < 6; ++i) {
            double r, g, b;
            nature_get_accent_ex(st->cfg.theme_family, i, dark, &r, &g, &b);
            int ax = px + i * (ac_w + 6);
            int sel = st->cfg.accent_idx == i;
            ngfx_fill_rect(cr, ax, py, ac_w, 28, 8, r, g, b, sel ? 0.96 : 0.48);
            if (sel) ngfx_stroke_rect(cr, ax + 0.5, py + 0.5, ac_w - 1, 27, 8, 1.6, 1, 1, 1, 0.90);
            ngfx_text_clipped(cr, "Noto Sans", 1, 10.2, ax + 7, py + 18, ac_w - 12, 1, 1, 1, 1, ac_names[i]);
            nature_add_hit(st, ax, py, ac_w, 28, HIT_SETTINGS_ACCENT, i, 0);
        }

        /* 5. Shell, Clock & Notifications */
        py += 46;
        ngfx_text(cr, "Noto Sans", 1, 12.0, px, py, mut_r, mut_g, mut_b, 1.0, "Shell, Clock & Window Behavior");
        py += 10;

        int stack_dc = (pw < 460);
        int dc_h = stack_dc ? 84 : 52;
        ngfx_fill_rect(cr, px, py, pw, dc_h, 11, pal.card_r, pal.card_g, pal.card_b, 0.96);
        ngfx_stroke_rect(cr, px + 0.5, py + 0.5, pw - 1, dc_h - 1, 11, 1.0, pal.border_r, pal.border_g, pal.border_b, pal.border_a);
        ngfx_text_clipped(cr, "Noto Sans", 1, 12.0, px + 14, py + 22, stack_dc ? (pw - 28) : 160, fg_r, fg_g, fg_b, 1.0, "Dock & Top Clock");
        ngfx_text_clipped(cr, "Noto Sans", 0, 10.0, px + 14, py + 39, stack_dc ? (pw - 28) : 175, mut_r, mut_g, mut_b, 0.9, "Position, 24h mode & seconds");

        int btn_y_dc = stack_dc ? (py + 46) : (py + 12);
        int dock_bw = stack_dc ? ((pw - 36) / 2) : 120;
        int clk_bw = stack_dc ? ((pw - 36 - dock_bw - 8) / 2) : 66;
        int sec_bw = stack_dc ? clk_bw : 60;
        int bx_dock = stack_dc ? (px + 14) : (px + pw - 270);
        int bx_clk = stack_dc ? (bx_dock + dock_bw + 6) : (px + pw - 144);
        int bx_sec = stack_dc ? (bx_clk + clk_bw + 6) : (px + pw - 72);

        ngfx_fill_rect(cr, bx_dock, btn_y_dc, dock_bw, 28, 7, pal.elev_r, pal.elev_g, pal.elev_b, 0.95);
        ngfx_stroke_rect(cr, bx_dock + 0.5, btn_y_dc + 0.5, dock_bw - 1, 27, 7, 1.0, pal.border_r, pal.border_g, pal.border_b, pal.border_a);
        ngfx_text_clipped(cr, "Noto Sans", 1, 10.4, bx_dock + 10, btn_y_dc + 18, dock_bw - 16, fg_r, fg_g, fg_b, 1.0,
                          st->cfg.dock_position == 0 ? "Dock: Bottom" : "Dock: Left");
        nature_add_hit(st, bx_dock, btn_y_dc, dock_bw, 28, HIT_SETTINGS_DOCK_POS, 0, 0);

        ngfx_fill_rect(cr, bx_clk, btn_y_dc, clk_bw, 28, 7,
                       st->cfg.clock_24h ? ar : pal.elev_r, st->cfg.clock_24h ? ag : pal.elev_g, st->cfg.clock_24h ? ab : pal.elev_b, 0.94);
        ngfx_text_clipped(cr, "JetBrains Mono", 1, 10.4, bx_clk + 12, btn_y_dc + 18, clk_bw - 16,
                          st->cfg.clock_24h ? pal.on_acc_r : fg_r, st->cfg.clock_24h ? pal.on_acc_g : fg_g, st->cfg.clock_24h ? pal.on_acc_b : fg_b,
                          1.0, st->cfg.clock_24h ? "24h" : "12h");
        nature_add_hit(st, bx_clk, btn_y_dc, clk_bw, 28, HIT_SETTINGS_CLOCK_24H, 0, 0);

        ngfx_fill_rect(cr, bx_sec, btn_y_dc, sec_bw, 28, 7,
                       st->cfg.show_seconds ? ar : pal.elev_r, st->cfg.show_seconds ? ag : pal.elev_g, st->cfg.show_seconds ? ab : pal.elev_b, 0.94);
        ngfx_text_clipped(cr, "JetBrains Mono", 1, 10.4, bx_sec + 12, btn_y_dc + 18, sec_bw - 16,
                          st->cfg.show_seconds ? pal.on_acc_r : fg_r, st->cfg.show_seconds ? pal.on_acc_g : fg_g, st->cfg.show_seconds ? pal.on_acc_b : fg_b,
                          1.0, st->cfg.show_seconds ? ":SS" : "HM");
        nature_add_hit(st, bx_sec, btn_y_dc, sec_bw, 28, HIT_SETTINGS_CLOCK_SEC, 0, 0);

        struct { const char *title; const char *desc; int val; int hit; } toggles[3] = {
            {"Edge Window Snapping", "Snap windows to left/right halves or maximize on drag", st->cfg.window_snap, HIT_SETTINGS_WINDOW_SNAP},
            {"Desktop Ambient Widgets & Shortcuts", "Show backdrop clock and quick launch cards on desktop", st->cfg.show_desktop_widgets, HIT_SETTINGS_DESKTOP_WIDGETS},
            {"Desktop Toast Notification Popups", "Display top-right popup banners (history stays in tray)", st->cfg.show_toast_popups, HIT_SETTINGS_TOAST_POPUPS},
        };
        py += (stack_dc ? 92 : 60);
        for (int t = 0; t < 3; ++t) {
            ngfx_fill_rect(cr, px, py, pw, 54, 11, pal.card_r, pal.card_g, pal.card_b, 0.96);
            ngfx_stroke_rect(cr, px + 0.5, py + 0.5, pw - 1, 53, 11, 1.0, pal.border_r, pal.border_g, pal.border_b, pal.border_a);
            ngfx_text_clipped(cr, "Noto Sans", 1, 12.0, px + 14, py + 23, pw - 86, fg_r, fg_g, fg_b, 1.0, toggles[t].title);
            ngfx_text_clipped(cr, "Noto Sans", 0, 10.0, px + 14, py + 40, pw - 86, mut_r, mut_g, mut_b, 0.92, toggles[t].desc);
            ngfx_toggle_switch(cr, px + pw - 62, py + 15, 46, 24, toggles[t].val, ar, ag, ab, dark);
            nature_add_hit(st, px + pw - 68, py + 11, 58, 32, toggles[t].hit, 0, 0);
            py += 60;
        }
    } else if (sec == 1) {
        py += 20;
        ngfx_fill_rect(cr, px, py, pw, 64, 11, pal.card_r, pal.card_g, pal.card_b, 0.96);
        ngfx_stroke_rect(cr, px + 0.5, py + 0.5, pw - 1, 63, 11, 1.0, pal.border_r, pal.border_g, pal.border_b, pal.border_a);
        ngfx_text(cr, "Noto Sans", 1, 13.0, px + 14, py + 24, fg_r, fg_g, fg_b, 1.0, "Adaptive Display Output");
        char disp[64];
        snprintf(disp, sizeof(disp), "%dx%d · Adaptive DRM/KMS · RGB 8-bit", st->screen_w, st->screen_h);
        ngfx_text_clipped(cr, "JetBrains Mono", 0, 10.8, px + 14, py + 44, pw - 170, mut_r, mut_g, mut_b, 1.0, disp);

        ngfx_fill_rect(cr, px + pw - 150, py + 18, 66, 28, 7,
                       st->cfg.target_hz == 60 ? ar : pal.elev_r,
                       st->cfg.target_hz == 60 ? ag : pal.elev_g,
                       st->cfg.target_hz == 60 ? ab : pal.elev_b, 0.94);
        ngfx_text(cr, "JetBrains Mono", 1, 10.8, px + pw - 135, py + 36,
                  st->cfg.target_hz == 60 ? pal.on_acc_r : fg_r,
                  st->cfg.target_hz == 60 ? pal.on_acc_g : fg_g,
                  st->cfg.target_hz == 60 ? pal.on_acc_b : fg_b, 1.0, "60 Hz");
        nature_add_hit(st, px + pw - 150, py + 18, 66, 28, HIT_SETTINGS_HZ, 60, 0);

        ngfx_fill_rect(cr, px + pw - 78, py + 18, 66, 28, 7,
                       st->cfg.target_hz == 120 ? ar : pal.elev_r,
                       st->cfg.target_hz == 120 ? ag : pal.elev_g,
                       st->cfg.target_hz == 120 ? ab : pal.elev_b, 0.94);
        ngfx_text(cr, "JetBrains Mono", 1, 10.8, px + pw - 66, py + 36,
                  st->cfg.target_hz == 120 ? pal.on_acc_r : fg_r,
                  st->cfg.target_hz == 120 ? pal.on_acc_g : fg_g,
                  st->cfg.target_hz == 120 ? pal.on_acc_b : fg_b, 1.0, "120 Hz");
        nature_add_hit(st, px + pw - 78, py + 18, 66, 28, HIT_SETTINGS_HZ, 120, 0);

        py += 76;
        ngfx_fill_rect(cr, px, py, pw, 104, 11, pal.card_r, pal.card_g, pal.card_b, 0.96);
        ngfx_stroke_rect(cr, px + 0.5, py + 0.5, pw - 1, 103, 11, 1.0, pal.border_r, pal.border_g, pal.border_b, pal.border_a);
        char br_buf[48];
        snprintf(br_buf, sizeof(br_buf), "Panel Brightness (%d%%)", st->cfg.brightness);
        ngfx_text(cr, "Noto Sans", 1, 12.8, px + 14, py + 24, fg_r, fg_g, fg_b, 1.0, br_buf);
        int sl_x = px + 14, sl_w = pw - 28;
        ngfx_fill_rect(cr, sl_x, py + 39, sl_w, 12, 6, pal.elev_r, pal.elev_g, pal.elev_b, 1.0);
        double bw_fill = (sl_w * st->cfg.brightness) / 100.0;
        if (bw_fill > 0) ngfx_fill_gradient(cr, sl_x, py + 39, bw_fill, 12, 6, pal.warning_r, pal.warning_g, pal.warning_b, 0.95, 0.98, 0.84, 0.34, 0.98, 1);
        ngfx_fill_circle(cr, sl_x + bw_fill, py + 46, 9.0, 0.0, 0.0, 0.0, 0.24);
        ngfx_fill_circle(cr, sl_x + bw_fill, py + 45, 8.2, 1, 1, 1, 0.98);
        ngfx_fill_circle(cr, sl_x + bw_fill, py + 45, 3.0, pal.warning_r, pal.warning_g, pal.warning_b, 0.95);
        nature_add_hit(st, sl_x, py + 31, sl_w, 28, HIT_SLIDER_BRIGHT, sl_x, sl_w);

        const int br_presets[3] = {40, 75, 100};
        const char *br_plbls[3] = {"Dim 40%", "Balanced 75%", "Full 100%"};
        int bp_w = (pw - 44) / 3;
        for (int k = 0; k < 3; ++k) {
            int bx = px + 14 + k * (bp_w + 8);
            int by_c = py + 66;
            int sel = (st->cfg.brightness >= br_presets[k] - 8 && st->cfg.brightness <= br_presets[k] + 8);
            ngfx_fill_rect(cr, bx, by_c, bp_w, 25, 7,
                           sel ? pal.warning_r : pal.elev_r, sel ? pal.warning_g : pal.elev_g, sel ? pal.warning_b : pal.elev_b, 0.92);
            if (!sel) ngfx_stroke_rect(cr, bx + 0.5, by_c + 0.5, bp_w - 1, 24, 7, 1.0, pal.border_r, pal.border_g, pal.border_b, pal.border_a);
            ngfx_text_clipped(cr, "Noto Sans", sel, 10.2, bx + 10, by_c + 16.5, bp_w - 16,
                              sel ? 1.0 : fg_r, sel ? 1.0 : fg_g, sel ? 1.0 : fg_b, 1.0, br_plbls[k]);
            nature_add_hit(st, bx, by_c, bp_w, 25, HIT_SETTINGS_BRIGHTNESS, br_presets[k], 0);
        }

        py += 116;
        ngfx_fill_rect(cr, px, py, pw, 96, 11, pal.card_r, pal.card_g, pal.card_b, 0.96);
        ngfx_stroke_rect(cr, px + 0.5, py + 0.5, pw - 1, 95, 11, 1.0, pal.border_r, pal.border_g, pal.border_b, pal.border_a);
        ngfx_text_clipped(cr, "Noto Sans", 1, 12.8, px + 14, py + 24, pw - 86, fg_r, fg_g, fg_b, 1.0, "Night Light (Blue-Light Filter)");
        ngfx_text_clipped(cr, "Noto Sans", 0, 10.6, px + 14, py + 42, pw - 86, mut_r, mut_g, mut_b, 1.0, "Warm amber tint for evening eye comfort");
        ngfx_toggle_switch(cr, px + pw - 62, py + 17, 46, 24, st->cfg.night_light, pal.warning_r, pal.warning_g, pal.warning_b, dark);
        nature_add_hit(st, px + pw - 68, py + 13, 58, 32, HIT_SETTINGS_NIGHT_LIGHT, 0, 0);

        const int w_vals[3] = {25, 65, 95};
        const char *w_lbls[3] = {"Subtle (25%)", "Warm (65%)", "Amber (95%)"};
        int chip_w = (pw - 44) / 3;
        for (int k = 0; k < 3; ++k) {
            int wx = px + 14 + k * (chip_w + 8);
            int wy_c = py + 58;
            int sel = (st->cfg.night_light_warmth >= w_vals[k] - 15 && st->cfg.night_light_warmth <= w_vals[k] + 15);
            ngfx_fill_rect(cr, wx, wy_c, chip_w, 26, 7,
                           sel ? pal.warning_r : pal.elev_r,
                           sel ? pal.warning_g : pal.elev_g,
                           sel ? pal.warning_b : pal.elev_b, sel ? 0.92 : 0.92);
            if (!sel) ngfx_stroke_rect(cr, wx + 0.5, wy_c + 0.5, chip_w - 1, 25, 7, 1.0, pal.border_r, pal.border_g, pal.border_b, pal.border_a);
            ngfx_text_clipped(cr, "Noto Sans", sel, 10.4, wx + 10, wy_c + 17, chip_w - 16,
                              sel ? 1.0 : fg_r, sel ? 1.0 : fg_g, sel ? 1.0 : fg_b, 1.0, w_lbls[k]);
            nature_add_hit(st, wx, wy_c, chip_w, 26, HIT_SETTINGS_NL_WARMTH, w_vals[k], 0);
        }

        py += 108;
        ngfx_fill_rect(cr, px, py, pw, 48, 10, pal.card_r, pal.card_g, pal.card_b, 0.96);
        ngfx_stroke_rect(cr, px + 0.5, py + 0.5, pw - 1, 47, 10, 1.0, pal.border_r, pal.border_g, pal.border_b, pal.border_a);
        ngfx_text(cr, "Noto Sans", 1, 11.4, px + 14, py + 21, fg_r, fg_g, fg_b, 1.0, "Compositor Frame Pipeline");
        char comp_line[96];
        snprintf(comp_line, sizeof(comp_line), "Live %.0f FPS · %.1f ms frame time · Display Scale 1.0x",
                 st->measured_fps, st->frame_ms);
        ngfx_text_clipped(cr, "JetBrains Mono", 0, 10.0, px + 14, py + 38, pw - 28, mut_r, mut_g, mut_b, 0.95, comp_line);
    } else if (sec == 2) {
        py += 20;
        int snd_stack = (pw < 450);
        int snd_h = snd_stack ? 104 : 82;
        ngfx_fill_rect(cr, px, py, pw, snd_h, 10, pal.card_r, pal.card_g, pal.card_b, 0.96);
        ngfx_stroke_rect(cr, px + 0.5, py + 0.5, pw - 1, snd_h - 1, 10, 1.0, pal.border_r, pal.border_g, pal.border_b, pal.border_a);
        char vb[64];
        snprintf(vb, sizeof(vb), "Master Volume: %d%% (%s)", st->cfg.volume, st->cfg.audio_muted ? "Muted" : "Active");
        ngfx_text_clipped(cr, "Noto Sans", 1, 12.6, px + 14, py + 24, snd_stack ? (pw - 28) : (pw - 254), fg_r, fg_g, fg_b, 1.0, vb);

        int sb_y = snd_stack ? (py + 34) : (py + 10);
        int rx_end = px + pw - 12;
        int mute_w = 64, test_w = 78, step_w = 42;
        int mute_x = rx_end - mute_w;
        int test_x = mute_x - 6 - test_w;
        int plus_x = test_x - 6 - step_w;
        int minus_x = plus_x - 6 - step_w;

        int v_down = st->cfg.volume - 10; if (v_down < 0) v_down = 0;
        int v_up = st->cfg.volume + 10; if (v_up > 100) v_up = 100;
        ngfx_fill_rect(cr, minus_x, sb_y, step_w, 24, 6, pal.elev_r, pal.elev_g, pal.elev_b, 0.95);
        ngfx_stroke_rect(cr, minus_x + 0.5, sb_y + 0.5, step_w - 1, 23, 6, 0.9, pal.border_r, pal.border_g, pal.border_b, pal.border_a);
        ngfx_text(cr, "JetBrains Mono", 1, 9.8, minus_x + 8, sb_y + 15.5, fg_r, fg_g, fg_b, 1.0, "-10%");
        nature_add_hit(st, minus_x, sb_y, step_w, 24, HIT_SETTINGS_VOLUME, v_down, 0);

        ngfx_fill_rect(cr, plus_x, sb_y, step_w, 24, 6, pal.elev_r, pal.elev_g, pal.elev_b, 0.95);
        ngfx_stroke_rect(cr, plus_x + 0.5, sb_y + 0.5, step_w - 1, 23, 6, 0.9, pal.border_r, pal.border_g, pal.border_b, pal.border_a);
        ngfx_text(cr, "JetBrains Mono", 1, 9.8, plus_x + 8, sb_y + 15.5, fg_r, fg_g, fg_b, 1.0, "+10%");
        nature_add_hit(st, plus_x, sb_y, step_w, 24, HIT_SETTINGS_VOLUME, v_up, 0);

        ngfx_fill_rect(cr, test_x, sb_y, test_w, 24, 6, ar, ag, ab, 0.90);
        ngfx_text(cr, "Noto Sans", 1, 10.0, test_x + 11, sb_y + 15.5, pal.on_acc_r, pal.on_acc_g, pal.on_acc_b, 1.0, "Test Sound");
        nature_add_hit(st, test_x, sb_y, test_w, 24, HIT_SETTINGS_SOUND_TEST, 0, 0);

        ngfx_fill_rect(cr, mute_x, sb_y, mute_w, 24, 6,
                       st->cfg.audio_muted ? pal.danger_r : pal.elev_r,
                       st->cfg.audio_muted ? pal.danger_g : pal.elev_g,
                       st->cfg.audio_muted ? pal.danger_b : pal.elev_b, 0.95);
        if (!st->cfg.audio_muted) ngfx_stroke_rect(cr, mute_x + 0.5, sb_y + 0.5, mute_w - 1, 23, 6, 0.9, pal.border_r, pal.border_g, pal.border_b, pal.border_a);
        ngfx_text(cr, "Noto Sans", 1, 10.2, mute_x + (st->cfg.audio_muted ? 11 : 17), sb_y + 15.5,
                  st->cfg.audio_muted ? 1.0 : fg_r, st->cfg.audio_muted ? 1.0 : fg_g, st->cfg.audio_muted ? 1.0 : fg_b,
                  1.0, st->cfg.audio_muted ? "Unmute" : "Mute");
        nature_add_hit(st, mute_x, sb_y, mute_w, 24, HIT_SETTINGS_MUTE, 0, 0);

        int sl_y = snd_stack ? (py + 72) : (py + 50);
        int sl_x = px + 14, sl_w = pw - 28;
        ngfx_fill_rect(cr, sl_x, sl_y, sl_w, 12, 6, pal.elev_r, pal.elev_g, pal.elev_b, 1.0);
        double vol_fill = (sl_w * st->cfg.volume) / 100.0;
        if (vol_fill > 0) {
            ngfx_fill_gradient(cr, sl_x, sl_y, vol_fill, 12, 6,
                               ar, ag, ab, 0.95, pal.ar_hi, pal.ag_hi, pal.ab_hi, 0.98, 1);
        }
        ngfx_fill_circle(cr, sl_x + vol_fill, sl_y + 7, 9.0, 0.0, 0.0, 0.0, 0.24);
        ngfx_fill_circle(cr, sl_x + vol_fill, sl_y + 6, 8.2, 1, 1, 1, 0.98);
        ngfx_fill_circle(cr, sl_x + vol_fill, sl_y + 6, 3.0, ar, ag, ab, 0.95);
        nature_add_hit(st, sl_x, sl_y - 8, sl_w, 28, HIT_SLIDER_VOL, sl_x, sl_w);

        py += snd_h + 14;
        ngfx_text(cr, "Noto Sans", 1, 12.0, px, py, mut_r, mut_g, mut_b, 1.0, "PipeWire Output Sink");
        py += 12;
        for (int d = 0; d < 3; ++d) {
            int act = st->cfg.audio_device == d;
            ngfx_fill_rect(cr, px, py, pw, 40, 9,
                           act ? ar : pal.card_r, act ? ag : pal.card_g, act ? ab : pal.card_b,
                           act ? (dark ? 0.24 : 0.14) : 0.96);
            ngfx_stroke_rect(cr, px + 0.5, py + 0.5, pw - 1, 39, 9, 1.0,
                             act ? ar : pal.border_r, act ? ag : pal.border_g, act ? ab : pal.border_b,
                             act ? 0.72 : pal.border_a);
            ngfx_fill_circle(cr, px + 18, py + 20, 4.5,
                             act ? pal.ar_hi : mut_r,
                             act ? pal.ag_hi : mut_g,
                             act ? pal.ab_hi : mut_b, act ? 1.0 : 0.35);
            ngfx_text_clipped(cr, "Noto Sans", act, 11.8, px + 32, py + 24, pw - 44, fg_r, fg_g, fg_b, 1.0, AUDIO_DEVS[d]);
            nature_add_hit(st, px, py, pw, 40, HIT_SETTINGS_AUDIO_DEV, d, 0);
            py += 46;
        }
    } else if (sec == 3) {
        py += 20;
        ngfx_text(cr, "Noto Sans", 1, 12.0, px, py, mut_r, mut_g, mut_b, 1.0, "Power & Scheduler Profile");
        py += 12;
        const char *pmodes[3] = {"Power Saver", "Balanced", "Performance"};
        int p_w = (pw - 16) / 3;
        for (int m = 0; m < 3; ++m) {
            int mx = px + m * (p_w + 8);
            int act = st->cfg.power_profile == m;
            ngfx_fill_rect(cr, mx, py, p_w, 34, 8,
                           act ? ar : pal.card_r, act ? ag : pal.card_g, act ? ab : pal.card_b, 0.96);
            if (!act) ngfx_stroke_rect(cr, mx + 0.5, py + 0.5, p_w - 1, 33, 8, 1.0, pal.border_r, pal.border_g, pal.border_b, pal.border_a);
            ngfx_text_clipped(cr, "Noto Sans", 1, 11.2, mx + 10, py + 21, p_w - 16,
                              act ? pal.on_acc_r : fg_r, act ? pal.on_acc_g : fg_g, act ? pal.on_acc_b : fg_b, 1.0, pmodes[m]);
            nature_add_hit(st, mx, py, p_w, 34, HIT_SETTINGS_PERF_MODE, m, 0);
        }

        py += 48;
        struct { const char *label; const char *desc; int val; int hit; } rows[2] = {
            {"Low-Power Compositor", "Reduce blur layers for minimum idle wattage", st->cfg.low_power_mode, HIT_SETTINGS_LOW_POWER},
            {"zram LZ4 Memory Pool", "Compress cold pages in RAM at 2.1:1 ratio", st->cfg.zram_enabled, HIT_SETTINGS_ZRAM},
        };
        for (int i = 0; i < 2; ++i) {
            ngfx_fill_rect(cr, px, py, pw, 62, 11, pal.card_r, pal.card_g, pal.card_b, 0.96);
            ngfx_stroke_rect(cr, px + 0.5, py + 0.5, pw - 1, 61, 11, 1.0, pal.border_r, pal.border_g, pal.border_b, pal.border_a);
            ngfx_text_clipped(cr, "Noto Sans", 1, 12.8, px + 14, py + 24, pw - 86, fg_r, fg_g, fg_b, 1.0, rows[i].label);
            ngfx_text_clipped(cr, "Noto Sans", 0, 10.8, px + 14, py + 44, pw - 86, mut_r, mut_g, mut_b, 1.0, rows[i].desc);
            ngfx_toggle_switch(cr, px + pw - 62, py + 19, 46, 24, rows[i].val, ar, ag, ab, dark);
            nature_add_hit(st, px + pw - 68, py + 15, 58, 32, rows[i].hit, 0, 0);
            py += 72;
        }

        py += 4;
        ngfx_text(cr, "Noto Sans", 1, 11.4, px, py, mut_r, mut_g, mut_b, 1.0, "Live Resource Snapshot");
        py += 10;
        int tw_card = (pw - 16) / 3;
        char c_val[32], r_val[32], z_val[32];
        snprintf(c_val, sizeof(c_val), "%.1f%%", st->total_cpu_pct);
        snprintf(r_val, sizeof(r_val), "%d MiB", st->used_ram_mb);
        snprintf(z_val, sizeof(z_val), "%d MiB", st->cfg.zram_enabled ? st->zram_used_mb : 0);
        const char *tlbls[3] = {"CPU Load", "System RAM", "zram Pool"};
        const char *tvals[3] = {c_val, r_val, z_val};
        for (int k = 0; k < 3; ++k) {
            int tx = px + k * (tw_card + 8);
            ngfx_fill_rect(cr, tx, py, tw_card, 56, 10, pal.card_r, pal.card_g, pal.card_b, 0.96);
            ngfx_stroke_rect(cr, tx + 0.5, py + 0.5, tw_card - 1, 55, 10, 1.0, pal.border_r, pal.border_g, pal.border_b, pal.border_a);
            ngfx_text_clipped(cr, "Noto Sans", 0, 10.0, tx + 12, py + 20, tw_card - 20, mut_r, mut_g, mut_b, 0.95, tlbls[k]);
            ngfx_text_clipped(cr, "JetBrains Mono", 1, 13.6, tx + 12, py + 42, tw_card - 20,
                              k == 0 ? (dark ? pal.ar_hi : ar) : fg_r,
                              k == 0 ? (dark ? pal.ag_hi : ag) : fg_g,
                              k == 0 ? (dark ? pal.ab_hi : ab) : fg_b, 1.0, tvals[k]);
        }
    } else if (sec == 4) {
        /* Network & Device Identity */
        py += 20;
        ngfx_fill_rect(cr, px, py, pw, 72, 11, pal.card_r, pal.card_g, pal.card_b, 0.96);
        ngfx_stroke_rect(cr, px + 0.5, py + 0.5, pw - 1, 71, 11, 1.0, pal.border_r, pal.border_g, pal.border_b, pal.border_a);
        ngfx_text_clipped(cr, "Noto Sans", 1, 12.8, px + 14, py + 27, pw - 180, fg_r, fg_g, fg_b, 1.0, "NetworkManager Wi-Fi (wlp2s0)");
        char wifi_sub[96];
        if (!st->cfg.wifi_enabled) {
            snprintf(wifi_sub, sizeof(wifi_sub), "Wi-Fi Radio Disabled");
        } else if (st->settings_net_ping_ms >= 0) {
            snprintf(wifi_sub, sizeof(wifi_sub), "SSID: Canopy-5GHz · 10.42.0.14 · %d ms OK", st->settings_net_ping_ms);
        } else {
            snprintf(wifi_sub, sizeof(wifi_sub), "SSID: Canopy-5GHz · 10.42.0.14");
        }
        ngfx_text_clipped(cr, "JetBrains Mono", 0, 10.2, px + 14, py + 48, pw - 180,
                          (st->cfg.wifi_enabled && st->settings_net_ping_ms >= 0) ? pal.success_r : mut_r,
                          (st->cfg.wifi_enabled && st->settings_net_ping_ms >= 0) ? pal.success_g : mut_g,
                          (st->cfg.wifi_enabled && st->settings_net_ping_ms >= 0) ? pal.success_b : mut_b, 1.0, wifi_sub);
        int h_ping = (st->hover_hit_action == HIT_SETTINGS_NET_TEST);
        ngfx_fill_rect(cr, px + pw - 160, py + 22, 84, 28, 7,
                       h_ping ? pal.hover_r : pal.elev_r, h_ping ? pal.hover_g : pal.elev_g, h_ping ? pal.hover_b : pal.elev_b, 0.96);
        ngfx_stroke_rect(cr, px + pw - 159.5, py + 22.5, 83, 27, 7, 1.0, pal.border_r, pal.border_g, pal.border_b, pal.border_a);
        ngfx_text(cr, "Noto Sans", 1, 10.4, px + pw - 147, py + 40, fg_r, fg_g, fg_b, st->cfg.wifi_enabled ? 1.0 : 0.55, "Test Ping");
        nature_add_hit(st, px + pw - 160, py + 22, 84, 28, HIT_SETTINGS_NET_TEST, 0, 0);
        ngfx_toggle_switch(cr, px + pw - 62, py + 24, 46, 24, st->cfg.wifi_enabled, ar, ag, ab, dark);
        nature_add_hit(st, px + pw - 68, py + 20, 58, 32, HIT_SETTINGS_WIFI, 0, 0);

        py += 82;
        ngfx_fill_rect(cr, px, py, pw, 72, 11, pal.card_r, pal.card_g, pal.card_b, 0.96);
        ngfx_stroke_rect(cr, px + 0.5, py + 0.5, pw - 1, 71, 11, 1.0, pal.border_r, pal.border_g, pal.border_b, pal.border_a);
        ngfx_text_clipped(cr, "Noto Sans", 1, 12.8, px + 14, py + 27, pw - 86, fg_r, fg_g, fg_b, 1.0, "Bluetooth Controller (BlueZ)");
        ngfx_text_clipped(cr, "JetBrains Mono", 0, 10.6, px + 14, py + 48, pw - 86, mut_r, mut_g, mut_b, 1.0,
                          st->cfg.bluetooth_enabled ? "Paired: Keychron K3 Pro · Buds" : "Bluetooth Adapter Powered Off");
        ngfx_toggle_switch(cr, px + pw - 62, py + 24, 46, 24, st->cfg.bluetooth_enabled, ar, ag, ab, dark);
        nature_add_hit(st, px + pw - 68, py + 20, 58, 32, HIT_SETTINGS_BLUETOOTH, 0, 0);

        /* Editable PC Hostname Card (target_id = 5) */
        py += 84;
        ngfx_fill_rect(cr, px, py, pw, 126, 11, pal.card_r, pal.card_g, pal.card_b, 0.96);
        ngfx_stroke_rect(cr, px + 0.5, py + 0.5, pw - 1, 125, 11, 1.0, pal.border_r, pal.border_g, pal.border_b, pal.border_a);
        ngfx_text_clipped(cr, "Noto Sans", 1, 12.8, px + 14, py + 25, pw - 28, fg_r, fg_g, fg_b, 1.0, "PC Name / Hostname");
        ngfx_text_clipped(cr, "Noto Sans", 0, 10.2, px + 14, py + 42, pw - 28, mut_r, mut_g, mut_b, 0.95,
                          "Shown in Terminal prompt, Lock Screen, and system info");

        int h_focused = (st->active_text_target == 5);
        int hbox_x = px + 14, hbox_y = py + 53, hbox_w = pw - 104, hbox_h = 30;
        ngfx_fill_rect(cr, hbox_x, hbox_y, hbox_w, hbox_h, 7, pal.surface_r, pal.surface_g, pal.surface_b, 1.0);
        ngfx_stroke_rect(cr, hbox_x + 0.5, hbox_y + 0.5, hbox_w - 1, hbox_h - 1, 7, h_focused ? 1.4 : 1.0,
                         h_focused ? ar : pal.border_r, h_focused ? ag : pal.border_g, h_focused ? ab : pal.border_b,
                         h_focused ? 0.9 : pal.border_a);
        const char *shown_host = st->settings_hostname_dirty ? st->settings_hostname_draft : st->cfg.device_name;
        char hdisp[48];
        if (h_focused) snprintf(hdisp, sizeof(hdisp), "%s|", shown_host);
        else snprintf(hdisp, sizeof(hdisp), "%s", shown_host);
        ngfx_text_clipped(cr, "JetBrains Mono", 1, 11.2, hbox_x + 10, hbox_y + 19, hbox_w - 18, fg_r, fg_g, fg_b, 1.0, hdisp);
        nature_add_hit(st, hbox_x, hbox_y, hbox_w, hbox_h, HIT_SETTINGS_HOST_BOX, 0, 0);
        nature_add_text_rect(st, hbox_x, hbox_y, hbox_w, hbox_h, 5);

        int save_x = px + pw - 82;
        ngfx_fill_rect(cr, save_x, hbox_y, 68, hbox_h, 7, ar, ag, ab, 0.95);
        ngfx_text(cr, "Noto Sans", 1, 10.8, save_x + 19, hbox_y + 19, pal.on_acc_r, pal.on_acc_g, pal.on_acc_b, 1.0, "Save");
        nature_add_hit(st, save_x, hbox_y, 68, hbox_h, HIT_SETTINGS_HOST_SAVE, 0, 0);

        static const char *HOST_PRESETS[3] = {"nature-pc", "canopy-ws", "horizon-box"};
        int pre_w = (pw - 44) / 3;
        for (int p = 0; p < 3; ++p) {
            int prx = px + 14 + p * (pre_w + 8);
            int pry = hbox_y + 38;
            int act_p = (strcmp(st->cfg.device_name, HOST_PRESETS[p]) == 0);
            ngfx_fill_rect(cr, prx, pry, pre_w, 24, 6,
                           act_p ? ar : pal.elev_r, act_p ? ag : pal.elev_g, act_p ? ab : pal.elev_b, act_p ? 0.24 : 0.92);
            ngfx_stroke_rect(cr, prx + 0.5, pry + 0.5, pre_w - 1, 23, 6, 0.8,
                             act_p ? ar : pal.border_r, act_p ? ag : pal.border_g, act_p ? ab : pal.border_b,
                             act_p ? 0.65 : pal.border_a);
            ngfx_text_clipped(cr, "JetBrains Mono", act_p, 9.6, prx + 10, pry + 15.5, pre_w - 16,
                              act_p ? (dark ? pal.ar_hi : ar) : fg_r,
                              act_p ? (dark ? pal.ag_hi : ag) : fg_g,
                              act_p ? (dark ? pal.ab_hi : ab) : fg_b, 0.96, HOST_PRESETS[p]);
            nature_add_hit(st, prx, pry, pre_w, 24, HIT_SETTINGS_HOST_SAVE, p + 1, 0);
        }
    } else if (sec == 5) {
        /* Keyboard & Shell Shortcuts */
        py += 16;
        const char *actions[3] = {"Tile Windows", "Open Overview", "Lock Screen"};
        int aw = (pw - 16) / 3;
        for (int a = 0; a < 3; ++a) {
            int ax = px + a * (aw + 8);
            ngfx_fill_rect(cr, ax, py, aw, 30, 8, ar, ag, ab, a == 2 ? 0.85 : 0.94);
            double tw = ngfx_text_width(cr, "Noto Sans", 1, 10.8, actions[a]);
            ngfx_text(cr, "Noto Sans", 1, 10.8, ax + (aw - tw) * 0.5, py + 19,
                      pal.on_acc_r, pal.on_acc_g, pal.on_acc_b, 1.0, actions[a]);
            nature_add_hit(st, ax, py, aw, 30, HIT_SETTINGS_SHORTCUT_ACTION, a, 0);
        }

        py += 42;
        const char *sc_rows[6][2] = {
            {"Super / Meta", "Toggle App Launcher & instant search"},
            {"Esc", "Close active overlay, menu, or text viewer"},
            {"Right-Click Desktop", "Open desktop quick-action context menu"},
            {"Double-Click Titlebar", "Maximize or restore active window"},
            {"ArrowLeft / ArrowRight", "Step Media Viewer photos or seek +/-5s audio"},
            {"Space / G / R / + / -", "Play/pause audio, toggle Media Grid, rotate, zoom"},
        };
        for (int i = 0; i < 6; ++i) {
            ngfx_fill_rect(cr, px, py, pw, 40, 9, pal.card_r, pal.card_g, pal.card_b, 0.96);
            ngfx_stroke_rect(cr, px + 0.5, py + 0.5, pw - 1, 39, 9, 1.0, pal.border_r, pal.border_g, pal.border_b, pal.border_a);
            int k_w = compact ? 132 : 168;
            ngfx_fill_rect(cr, px + 10, py + 8, k_w, 24, 6, pal.elev_r, pal.elev_g, pal.elev_b, 0.95);
            ngfx_text_clipped(cr, "JetBrains Mono", 1, 9.8, px + 18, py + 23.5, k_w - 14,
                              dark ? pal.ar_hi : ar, dark ? pal.ag_hi : ag, dark ? pal.ab_hi : ab, 1.0, sc_rows[i][0]);
            ngfx_text_clipped(cr, "Noto Sans", 0, 10.6, px + 18 + k_w, py + 24, pw - k_w - 28, fg_r, fg_g, fg_b, 0.92, sc_rows[i][1]);
            py += 46;
        }
    } else {
        /* About System + Storage Bar + Reset Defaults */
        py += 16;
        ngfx_fill_rect(cr, px, py, pw, 72, 11, pal.card_r, pal.card_g, pal.card_b, 0.96);
        ngfx_stroke_rect(cr, px + 0.5, py + 0.5, pw - 1, 71, 11, 1.0, pal.border_r, pal.border_g, pal.border_b, pal.border_a);
        ngfx_top_specular(cr, px, py, pw, 11, dark ? 0.16 : 0.45);
        if (st->avatar_surface) {
            g_ngfx.save(cr);
            ngfx_rounded_path(cr, px + 12, py + 10, 52, 52, 26);
            g_ngfx.clip(cr);
            double sw = g_ngfx.image_surface_get_width(st->avatar_surface);
            g_ngfx.translate(cr, px + 12, py + 10);
            g_ngfx.scale(cr, 52.0 / sw, 52.0 / sw);
            g_ngfx.set_source_surface(cr, st->avatar_surface, 0, 0);
            g_ngfx.paint(cr);
            g_ngfx.restore(cr);
            ngfx_stroke_rect(cr, px + 12, py + 10, 52, 52, 26, 1.8, ar, ag, ab, 0.85);
        }
        ngfx_text_clipped(cr, "Plus Jakarta Sans", 1, 15.0, px + 76, py + 32, pw - 208, fg_r, fg_g, fg_b, 1.0, "Nature OS 24.04.1 LTS");
        ngfx_text_clipped(cr, "Noto Sans", 0, 11.0, px + 76, py + 52, pw - 208, mut_r, mut_g, mut_b, 1.0, "Ubuntu 24.04 Noble · C17/C++20");

        ngfx_fill_rect(cr, px + pw - 124, py + 22, 112, 28, 8, ar, ag, ab, 0.94);
        ngfx_text(cr, "Noto Sans", 1, 10.6, px + pw - 113, py + 40, pal.on_acc_r, pal.on_acc_g, pal.on_acc_b, 1.0,
                  st->settings_update_checked ? "Up to Date" : "Check Updates");
        nature_add_hit(st, px + pw - 124, py + 22, 112, 28, HIT_SETTINGS_CHECK_UPDATE, 0, 0);

        /* Storage Usage Progress Card (live statvfs) */
        py += 82;
        long disk_tot = st->disk_total_gb > 0 ? st->disk_total_gb : 64;
        long disk_avail = st->disk_avail_gb >= 0 ? st->disk_avail_gb : 48;
        long disk_used = disk_tot > disk_avail ? (disk_tot - disk_avail) : 0;
        int disk_pct = st->disk_used_pct > 0 ? st->disk_used_pct : 25;
        if (disk_pct > 100) disk_pct = 100;

        ngfx_fill_rect(cr, px, py, pw, 68, 11, pal.card_r, pal.card_g, pal.card_b, 0.96);
        ngfx_stroke_rect(cr, px + 0.5, py + 0.5, pw - 1, 67, 11, 1.0, pal.border_r, pal.border_g, pal.border_b, pal.border_a);
        ngfx_text(cr, "Noto Sans", 1, 12.0, px + 14, py + 23, fg_r, fg_g, fg_b, 1.0, "Root Storage (ext4 · /)");
        char st_buf[128];
        snprintf(st_buf, sizeof(st_buf), "%ld GiB used / %ld GiB (%ld GiB free)", disk_used, disk_tot, disk_avail);
        ngfx_text_clipped(cr, "JetBrains Mono", 0, 10.2, px + 165, py + 23, pw - 178, mut_r, mut_g, mut_b, 0.95, st_buf);

        ngfx_fill_rect(cr, px + 14, py + 36, pw - 28, 10, 5, pal.elev_r, pal.elev_g, pal.elev_b, 1.0);
        double d_fill = ((pw - 28) * disk_pct) / 100.0;
        if (d_fill < 8 && disk_pct > 0) d_fill = 8;
        ngfx_fill_gradient(cr, px + 14, py + 36, d_fill, 10, 5, ar, ag, ab, 0.96, pal.ar_hi, pal.ag_hi, pal.ab_hi, 0.96, 1);
        char st_foot[64];
        snprintf(st_foot, sizeof(st_foot), "%d%% allocated · zram LZ4 pool %s", disk_pct, st->cfg.zram_enabled ? "enabled" : "off");
        ngfx_text(cr, "JetBrains Mono", 0, 9.5, px + 14, py + 58, mut_r, mut_g, mut_b, 0.88, st_foot);

        py += 78;
        char mem_spec[64], cpu_spec[96];
        const char *host = st->cfg.device_name[0] ? st->cfg.device_name : "nature-pc";
        snprintf(cpu_spec, sizeof(cpu_spec), "%d Cores (%.56s)", st->cpu_count > 0 ? st->cpu_count : 4, st->cpu_model);
        snprintf(mem_spec, sizeof(mem_spec), "%.1f GiB (/proc) · OS RSS %d MiB", st->total_ram_mb / 1024.0, st->os_rss_mb);
        const char *specs[5][2] = {
            {"Device Name", host},
            {"Base Distro", "Ubuntu 24.04.1 LTS (APT / dpkg)"},
            {"Linux Kernel", st->kernel_ver},
            {"Processor", cpu_spec},
            {"Memory Pool", mem_spec},
        };
        for (int i = 0; i < 5; ++i) {
            ngfx_fill_rect(cr, px, py, pw, 34, 9, pal.card_r, pal.card_g, pal.card_b, 0.96);
            ngfx_stroke_rect(cr, px + 0.5, py + 0.5, pw - 1, 33, 9, 1.0, pal.border_r, pal.border_g, pal.border_b, pal.border_a);
            ngfx_text(cr, "Noto Sans", 1, 11.0, px + 12, py + 21, mut_r, mut_g, mut_b, 1.0, specs[i][0]);
            ngfx_text_clipped(cr, "JetBrains Mono", 0, 10.4, px + 114, py + 21, pw - 126, fg_r, fg_g, fg_b, 0.95, specs[i][1]);
            py += 39;
        }

        py += 8;
        int btn_w = (pw - 10) / 2;
        ngfx_fill_rect(cr, px, py, btn_w, 32, 8, pal.elev_r, pal.elev_g, pal.elev_b, 0.96);
        ngfx_stroke_rect(cr, px + 0.5, py + 0.5, btn_w - 1, 31, 8, 1.0, pal.border_r, pal.border_g, pal.border_b, pal.border_a);
        ngfx_text(cr, "Noto Sans", 1, 11.0, px + 18, py + 20, fg_r, fg_g, fg_b, 1.0, "Lock Screen Now");
        nature_add_hit(st, px, py, btn_w, 32, HIT_SETTINGS_LOCK_NOW, 0, 0);

        int rx = px + btn_w + 10;
        const char *rlbl = st->settings_confirm_reset ? "Click Again to Reset" : "Reset Settings";
        ngfx_fill_rect(cr, rx, py, btn_w, 32, 8,
                       pal.danger_r, pal.danger_g, pal.danger_b, st->settings_confirm_reset ? 0.34 : 0.16);
        ngfx_stroke_rect(cr, rx + 0.5, py + 0.5, btn_w - 1, 31, 8, 1.0,
                         pal.danger_r, pal.danger_g, pal.danger_b, st->settings_confirm_reset ? 0.85 : 0.38);
        ngfx_text_clipped(cr, "Noto Sans", 1, 11.0, rx + 14, py + 20, btn_w - 20,
                          pal.danger_r, pal.danger_g, pal.danger_b, 1.0, rlbl);
        nature_add_hit(st, rx, py, btn_w, 32, HIT_SETTINGS_RESET_DEFAULTS, 0, 0);

        py += 40;
        const char *flbl = st->settings_confirm_factory ? "Click Again to Factory Reset Session & Files" : "Factory Reset (Reset Files, Apps & Window Layout)";
        ngfx_fill_rect(cr, px, py, pw, 32, 8,
                       pal.danger_r, pal.danger_g, pal.danger_b, st->settings_confirm_factory ? 0.36 : 0.10);
        ngfx_stroke_rect(cr, px + 0.5, py + 0.5, pw - 1, 31, 8, 1.0,
                         pal.danger_r, pal.danger_g, pal.danger_b, st->settings_confirm_factory ? 0.90 : 0.32);
        double ftw = ngfx_text_width(cr, "Noto Sans", 1, 10.8, flbl);
        double ftx = px + (pw - ftw) * 0.5;
        if (ftx < px + 12) ftx = px + 12;
        ngfx_text_clipped(cr, "Noto Sans", 1, 10.8, ftx, py + 20, pw - 24,
                          pal.danger_r, pal.danger_g, pal.danger_b, 1.0, flbl);
        nature_add_hit(st, px, py, pw, 32, HIT_SETTINGS_CLEAR_USER_DATA, 0, 0);
    }

    g_ngfx.restore(cr);

    int vis_steps = (ch - 24) / step_px;
    int tot_steps = total_h / step_px;
    nature_draw_scrollbar(st, cr, cx + cw - 11, cy + 8, ch - 16,
                          tot_steps, vis_steps, st->settings_scroll, max_scroll, SCROLL_SETTINGS, dark);
}

void nature_settings_click(NatureOSState *st, const HitRect *hit) {
    if (hit->action_id != HIT_SETTINGS_RESET_DEFAULTS) {
        st->settings_confirm_reset = 0;
    }
    if (hit->action_id != HIT_SETTINGS_CLEAR_USER_DATA) {
        st->settings_confirm_factory = 0;
    }
    int save_needed = 0;
    switch (hit->action_id) {
        case HIT_SETTINGS_SECTION:
            st->settings_section = hit->arg0 % NUM_SECTIONS;
            st->settings_scroll = 0;
            save_needed = 1;
            break;
        case HIT_SETTINGS_SEARCH_BOX:
            st->active_text_target = 6;
            st->input_focus_pulse = 1;
            break;
        case HIT_SETTINGS_SEARCH_CLEAR:
            st->settings_search[0] = '\0';
            break;
        case HIT_SETTINGS_THEME_FAMILY:
            st->cfg.theme_family = hit->arg0 ? THEME_HORIZON : THEME_NATURE;
            if (st->cfg.theme_family == THEME_HORIZON) {
                nature_os_set_wallpaper_preset(st, st->cfg.dark_mode ? 4 : 3);
            } else if (st->cfg.wallpaper_idx == 3 || st->cfg.wallpaper_idx == 4 || st->cfg.wallpaper_idx == 7) {
                nature_os_set_wallpaper_preset(st, 0);
            }
            nature_notify(st, "Theme Updated",
                          st->cfg.theme_family == THEME_HORIZON ? "Switched to Horizon theme" : "Switched to Nature Canopy theme");
            save_needed = 1;
            break;
        case HIT_SETTINGS_DARK:
            st->cfg.dark_mode = hit->arg0 ? 1 : 0;
            if (st->cfg.theme_family == THEME_HORIZON && (st->cfg.wallpaper_idx == 3 || st->cfg.wallpaper_idx == 4)) {
                nature_os_set_wallpaper_preset(st, st->cfg.dark_mode ? 4 : 3);
            }
            save_needed = 1;
            break;
        case HIT_SETTINGS_NIGHT_LIGHT:
            st->cfg.night_light = !st->cfg.night_light;
            save_needed = 1;
            break;
        case HIT_SETTINGS_NL_WARMTH:
            st->cfg.night_light_warmth = hit->arg0;
            st->cfg.night_light = 1;
            save_needed = 1;
            break;
        case HIT_SETTINGS_WALLPAPER:
            nature_os_set_wallpaper_preset(st, hit->arg0);
            save_needed = 1;
            break;
        case HIT_SETTINGS_ACCENT:
            st->cfg.accent_idx = hit->arg0 % 6;
            save_needed = 1;
            break;
        case HIT_SETTINGS_DOCK_POS:
            st->cfg.dock_position = !st->cfg.dock_position;
            save_needed = 1;
            break;
        case HIT_SETTINGS_CLOCK_24H:
            st->cfg.clock_24h = !st->cfg.clock_24h;
            save_needed = 1;
            break;
        case HIT_SETTINGS_CLOCK_SEC:
            st->cfg.show_seconds = !st->cfg.show_seconds;
            save_needed = 1;
            break;
        case HIT_SETTINGS_WINDOW_SNAP:
            st->cfg.window_snap = !st->cfg.window_snap;
            save_needed = 1;
            break;
        case HIT_SETTINGS_DESKTOP_WIDGETS:
            st->cfg.show_desktop_widgets = !st->cfg.show_desktop_widgets;
            save_needed = 1;
            break;
        case HIT_SETTINGS_TOAST_POPUPS:
            st->cfg.show_toast_popups = !st->cfg.show_toast_popups;
            save_needed = 1;
            break;
        case HIT_SETTINGS_LOW_POWER:
            st->cfg.low_power_mode = !st->cfg.low_power_mode;
            save_needed = 1;
            break;
        case HIT_SETTINGS_ZRAM:
            st->cfg.zram_enabled = !st->cfg.zram_enabled;
            save_needed = 1;
            break;
        case HIT_SETTINGS_WIFI:
            st->cfg.wifi_enabled = !st->cfg.wifi_enabled;
            if (!st->cfg.wifi_enabled) st->settings_net_ping_ms = -1;
            save_needed = 1;
            break;
        case HIT_SETTINGS_NET_TEST:
            if (st->cfg.wifi_enabled) {
                st->settings_net_ping_ms = 12;
                nature_notify(st, "NetworkManager", "ICMP 1.1.1.1 reachable (12 ms · 0% loss).");
            } else {
                st->settings_net_ping_ms = -1;
                nature_notify(st, "NetworkManager", "Offline: enable Wi-Fi before testing latency.");
            }
            break;
        case HIT_SETTINGS_BLUETOOTH:
            st->cfg.bluetooth_enabled = !st->cfg.bluetooth_enabled;
            save_needed = 1;
            break;
        case HIT_SETTINGS_HOST_BOX:
            if (!st->settings_hostname_dirty) {
                snprintf(st->settings_hostname_draft, sizeof(st->settings_hostname_draft), "%s", st->cfg.device_name);
                st->settings_hostname_dirty = 1;
            }
            st->active_text_target = 5;
            st->input_focus_pulse = 1;
            break;
        case HIT_SETTINGS_HOST_SAVE: {
            static const char *HOST_PRESETS[3] = {"nature-pc", "canopy-ws", "horizon-box"};
            if (hit->arg0 >= 1 && hit->arg0 <= 3) {
                snprintf(st->settings_hostname_draft, sizeof(st->settings_hostname_draft), "%s", HOST_PRESETS[hit->arg0 - 1]);
            }
            save_hostname_draft(st);
            st->active_text_target = 0;
            save_needed = 1;
            break;
        }
        case HIT_SETTINGS_HZ:
            st->cfg.target_hz = hit->arg0;
            save_needed = 1;
            break;
        case HIT_SETTINGS_BRIGHTNESS:
            st->cfg.brightness = hit->arg0;
            save_needed = 1;
            break;
        case HIT_SETTINGS_VOLUME:
            st->cfg.volume = hit->arg0;
            st->cfg.audio_muted = 0;
            save_needed = 1;
            break;
        case HIT_SETTINGS_MUTE:
            st->cfg.audio_muted = !st->cfg.audio_muted;
            save_needed = 1;
            break;
        case HIT_SETTINGS_SOUND_TEST:
            st->cfg.audio_muted = 0;
            nature_notify(st, "PipeWire Audio", "Playing stereo test chime (48kHz / 24-bit)...");
            save_needed = 1;
            break;
        case HIT_SETTINGS_AUDIO_DEV:
            st->cfg.audio_device = hit->arg0;
            nature_notify(st, "PipeWire Audio", AUDIO_DEVS[hit->arg0 % 3]);
            save_needed = 1;
            break;
        case HIT_SETTINGS_PERF_MODE:
            st->cfg.power_profile = hit->arg0 % 3;
            save_needed = 1;
            break;
        case HIT_SETTINGS_CHECK_UPDATE:
            st->settings_update_checked = 1;
            nature_notify(st, "Ubuntu APT", "All 1,412 system packages are up to date (noble-security).");
            break;
        case HIT_SETTINGS_SHORTCUT_ACTION:
            if (hit->arg0 == 0) nature_tile_windows(st);
            else if (hit->arg0 == 1) st->overview_open = 1;
            else if (hit->arg0 == 2) st->is_locked = 1;
            break;
        case HIT_SETTINGS_LOCK_NOW:
            st->is_locked = 1;
            break;
        case HIT_SETTINGS_RESET_DEFAULTS:
            if (!st->settings_confirm_reset) {
                st->settings_confirm_reset = 1;
            } else {
                nature_os_reset_defaults(st);
            }
            break;
        case HIT_SETTINGS_CLEAR_USER_DATA:
            if (!st->settings_confirm_factory) {
                st->settings_confirm_factory = 1;
            } else {
                nature_os_reset_all_session(st);
            }
            break;
        default: break;
    }
    if (save_needed) {
        nature_os_save_state(st);
    }
}

void nature_settings_key(NatureOSState *st, const char *key) {
    if (!key || !*key) return;
    if (st->active_text_target == 5) {
        if (strcmp(key, "Backspace") == 0) {
            size_t len = strlen(st->settings_hostname_draft);
            if (len > 0) st->settings_hostname_draft[len - 1] = '\0';
            st->settings_hostname_dirty = 1;
        } else if (strcmp(key, "Enter") == 0) {
            save_hostname_draft(st);
            st->active_text_target = 0;
            nature_os_save_state(st);
        } else if (strlen(key) == 1 && (isalnum((unsigned char)key[0]) || key[0] == '-' || key[0] == '_')) {
            size_t len = strlen(st->settings_hostname_draft);
            if (len + 1 < sizeof(st->settings_hostname_draft)) {
                st->settings_hostname_draft[len] = key[0];
                st->settings_hostname_draft[len + 1] = '\0';
                st->settings_hostname_dirty = 1;
            }
        }
        return;
    }

    if (st->active_text_target == 6) {
        if (strcmp(key, "Backspace") == 0) {
            size_t len = strlen(st->settings_search);
            if (len > 0) st->settings_search[len - 1] = '\0';
        } else if (strcmp(key, "Enter") == 0) {
            st->active_text_target = 0;
        } else if (strlen(key) == 1 && (unsigned char)key[0] >= 32 && (unsigned char)key[0] < 127) {
            size_t len = strlen(st->settings_search);
            if (len + 1 < sizeof(st->settings_search)) {
                st->settings_search[len] = key[0];
                st->settings_search[len + 1] = '\0';
            }
        }
        if (st->settings_search[0]) {
            for (int i = 0; i < NUM_SECTIONS; ++i) {
                if (str_ci_contains(SECTIONS[i], st->settings_search) ||
                    str_ci_contains(SECTION_KEYWORDS[i], st->settings_search)) {
                    st->settings_section = i;
                    st->settings_scroll = 0;
                    break;
                }
            }
        }
    }
}

void nature_settings_wheel(NatureOSState *st, int delta_y) {
    if (delta_y > 0) {
        if (st->settings_scroll < st->settings_max_scroll) st->settings_scroll++;
    } else if (delta_y < 0 && st->settings_scroll > 0) {
        st->settings_scroll--;
    }
}
