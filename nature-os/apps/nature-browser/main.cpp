#include "nature_os.h"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>

#define NUM_TABS 4

static const char *BROWSER_URLS[NUM_TABS] = {
    "nature://portal",
    "nature://architecture",
    "nature://ubuntu-integration",
    "nature://multitasking-benchmarks"
};

static const char *BROWSER_TITLES[NUM_TABS] = {
    "System Portal",
    "Architecture",
    "Ubuntu 24.04 Base",
    "Benchmarks"
};

static int str_ci_contains(const char *hay, const char *needle) {
    if (!hay || !needle || !*needle) return 0;
    size_t hl = std::strlen(hay), nl = std::strlen(needle);
    if (nl > hl) return 0;
    for (size_t i = 0; i <= hl - nl; ++i) {
        size_t j = 0;
        while (j < nl && std::tolower((unsigned char)hay[i + j]) == std::tolower((unsigned char)needle[j])) ++j;
        if (j == nl) return 1;
    }
    return 0;
}

extern "C" {

void nature_browser_render(NatureOSState *st, cairo_t *cr, OSWindow *w, int cx, int cy, int cw, int ch) {
    (void)w;
    NaturePalette pal;
    nature_get_palette(st, &pal);
    int dark = pal.dark_mode;
    double ar = pal.ar, ag = pal.ag, ab = pal.ab;

    double fg_r = pal.fg_r, fg_g = pal.fg_g, fg_b = pal.fg_b;
    double mut_r = pal.mut_r, mut_g = pal.mut_g, mut_b = pal.mut_b;
    int compact = cw < 540;

    ngfx_fill_rect(cr, cx, cy, cw, ch, 0, pal.win_bg_r, pal.win_bg_g, pal.win_bg_b, 1.0);

    /* Tab strip */
    ngfx_fill_rect(cr, cx, cy, cw, 36, 0, pal.sidebar_r, pal.sidebar_g, pal.sidebar_b, 1.0);
    g_ngfx.set_source_rgba(cr, pal.border_r, pal.border_g, pal.border_b, pal.border_a);
    g_ngfx.move_to(cr, cx, cy + 36.5);
    g_ngfx.line_to(cr, cx + cw, cy + 36.5);
    g_ngfx.stroke(cr);

    int tab_w = (cw - 28) / NUM_TABS;
    if (tab_w > 165) tab_w = 165;
    for (int i = 0; i < NUM_TABS; ++i) {
        int tx = cx + 10 + i * (tab_w + 6);
        int act = (st->browser_tab % NUM_TABS) == i;
        int hov = (st->hover_hit_action == HIT_BROWSER_TAB && st->hover_hit_arg0 == i);
        ngfx_fill_rect(cr, tx, cy + 5, tab_w, 26, 7,
                       act ? pal.surface_r : (hov ? pal.hover_r : pal.elev_r),
                       act ? pal.surface_g : (hov ? pal.hover_g : pal.elev_g),
                       act ? pal.surface_b : (hov ? pal.hover_b : pal.elev_b),
                       act ? 1.0 : (hov ? 0.92 : 0.72));
        if (act) {
            ngfx_stroke_rect(cr, tx + 0.5, cy + 5.5, tab_w - 1, 25, 7, 0.8, pal.border_r, pal.border_g, pal.border_b, pal.border_a);
            ngfx_top_specular(cr, tx, cy + 5, tab_w, 7, dark ? 0.14 : 0.35);
            ngfx_fill_rect(cr, tx + 12, cy + 28, tab_w - 24, 2, 1, ar, ag, ab, 0.95);
        }
        ngfx_text_clipped(cr, "Noto Sans", act, compact ? 10.4 : 11.2, tx + 10, cy + 22, tab_w - 18,
                          fg_r, fg_g, fg_b, act ? 1.0 : (hov ? 0.92 : 0.74), BROWSER_TITLES[i]);
        nature_add_hit(st, tx, cy + 5, tab_w, 26, HIT_BROWSER_TAB, i, 0);
    }

    /* Address Bar */
    ngfx_fill_rect(cr, cx, cy + 37, cw, 40, 0, pal.surface_r, pal.surface_g, pal.surface_b, 1.0);
    g_ngfx.set_source_rgba(cr, pal.border_r, pal.border_g, pal.border_b, pal.border_a);
    g_ngfx.move_to(cr, cx, cy + 77.5);
    g_ngfx.line_to(cr, cx + cw, cy + 77.5);
    g_ngfx.stroke(cr);

    int hov_back = (st->hover_hit_action == HIT_BROWSER_BACK);
    ngfx_fill_rect(cr, cx + 12, cy + 44, 28, 26, 7,
                   hov_back ? pal.hover_r : pal.elev_r, hov_back ? pal.hover_g : pal.elev_g, hov_back ? pal.hover_b : pal.elev_b, 0.94);
    ngfx_stroke_rect(cr, cx + 12.5, cy + 44.5, 27, 25, 7, 0.8, pal.border_r, pal.border_g, pal.border_b, pal.border_a);
    g_ngfx.set_source_rgba(cr, fg_r, fg_g, fg_b, hov_back ? 1.0 : 0.85);
    g_ngfx.set_line_width(cr, 1.6);
    g_ngfx.set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
    g_ngfx.set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
    g_ngfx.move_to(cr, cx + 28.5, cy + 52.5);
    g_ngfx.line_to(cr, cx + 23.0, cy + 57.0);
    g_ngfx.line_to(cr, cx + 28.5, cy + 61.5);
    g_ngfx.stroke(cr);
    nature_add_hit(st, cx + 12, cy + 44, 28, 26, HIT_BROWSER_BACK, 0, 0);

    int hov_fwd = (st->hover_hit_action == HIT_BROWSER_FWD);
    ngfx_fill_rect(cr, cx + 44, cy + 44, 28, 26, 7,
                   hov_fwd ? pal.hover_r : pal.elev_r, hov_fwd ? pal.hover_g : pal.elev_g, hov_fwd ? pal.hover_b : pal.elev_b, 0.94);
    ngfx_stroke_rect(cr, cx + 44.5, cy + 44.5, 27, 25, 7, 0.8, pal.border_r, pal.border_g, pal.border_b, pal.border_a);
    g_ngfx.set_source_rgba(cr, fg_r, fg_g, fg_b, hov_fwd ? 1.0 : 0.85);
    g_ngfx.set_line_width(cr, 1.6);
    g_ngfx.move_to(cr, cx + 55.5, cy + 52.5);
    g_ngfx.line_to(cr, cx + 61.0, cy + 57.0);
    g_ngfx.line_to(cr, cx + 55.5, cy + 61.5);
    g_ngfx.stroke(cr);
    nature_add_hit(st, cx + 44, cy + 44, 28, 26, HIT_BROWSER_FWD, 0, 0);

    int url_x = cx + 80;
    int url_w = cw - (compact ? 152 : 170);
    int url_focused = (st->active_text_target == 3);
    int url_hov = (st->hover_hit_action == HIT_BROWSER_URL_BAR);
    ngfx_fill_rect(cr, url_x, cy + 44, url_w, 26, 8, pal.card_r, pal.card_g, pal.card_b, 1.0);
    if (url_focused) {
        ngfx_stroke_rect(cr, url_x + 0.5, cy + 44.5, url_w - 1, 25, 8, 1.3, ar, ag, ab, 0.92);
    } else {
        ngfx_stroke_rect(cr, url_x + 0.5, cy + 44.5, url_w - 1, 25, 8, 0.9,
                         url_hov ? ar : pal.border_r, url_hov ? ag : pal.border_g, url_hov ? ab : pal.border_b,
                         url_hov ? 0.55 : pal.border_a);
    }

    /* Vector padlock icon inside omnibox */
    g_ngfx.set_source_rgba(cr, pal.success_r, pal.success_g, pal.success_b, 0.92);
    g_ngfx.set_line_width(cr, 1.3);
    g_ngfx.arc(cr, url_x + 15.0, cy + 54.5, 2.5, 3.14159, 0.0);
    g_ngfx.stroke(cr);
    ngfx_fill_rect(cr, url_x + 11.5, cy + 55.0, 7.0, 5.5, 1.5, pal.success_r, pal.success_g, pal.success_b, 0.92);

    if (url_focused) {
        if (st->browser_url_input[0] == '\0') {
            ngfx_text_clipped(cr, "JetBrains Mono", 0, 10.6, url_x + 25, cy + 61, url_w - 34,
                              mut_r, mut_g, mut_b, 0.78, "Type portal, arch, ubuntu, bench + Enter...");
        } else {
            char disp[104];
            std::snprintf(disp, sizeof(disp), "%s|", st->browser_url_input);
            ngfx_text_clipped(cr, "JetBrains Mono", 0, 11.0, url_x + 25, cy + 61, url_w - 34,
                              fg_r, fg_g, fg_b, 1.0, disp);
        }
    } else {
        ngfx_text_clipped(cr, "JetBrains Mono", 0, 11.0, url_x + 25, cy + 61, url_w - 34,
                          dark ? pal.ar_hi : ar, dark ? pal.ag_hi : ag, dark ? pal.ab_hi : ab, 1.0,
                          BROWSER_URLS[st->browser_tab % NUM_TABS]);
    }
    nature_add_hit(st, url_x, cy + 44, url_w, 26, HIT_BROWSER_URL_BAR, 0, 0);
    nature_add_text_rect(st, url_x, cy + 44, url_w, 26, 3);

    int reload_w = compact ? 54 : 68;
    int reload_x = cx + cw - reload_w - 10;
    int hov_rel = (st->hover_hit_action == HIT_BROWSER_RELOAD);
    ngfx_fill_gradient(cr, reload_x, cy + 44, reload_w, 26, 7,
                       ar * (hov_rel ? 1.12 : 1.0), ag * (hov_rel ? 1.12 : 1.0), ab * (hov_rel ? 1.12 : 1.0), 0.96,
                       ar * 0.78, ag * 0.78, ab * 0.78, 0.96, 0);
    ngfx_top_specular(cr, reload_x, cy + 44, reload_w, 7, 0.22);
    ngfx_text(cr, "Noto Sans", 1, 11.0, reload_x + (compact ? 9 : 14), cy + 61, pal.on_acc_r, pal.on_acc_g, pal.on_acc_b, 1, "Reload");
    nature_add_hit(st, reload_x, cy + 44, reload_w, 26, HIT_BROWSER_RELOAD, 0, 0);

    /* Bookmarks & Quick-Links Strip */
    ngfx_fill_rect(cr, cx, cy + 78, cw, 26, 0, pal.sidebar_r, pal.sidebar_g, pal.sidebar_b, 0.96);
    g_ngfx.set_source_rgba(cr, pal.border_r, pal.border_g, pal.border_b, pal.border_a);
    g_ngfx.move_to(cr, cx, cy + 104.5);
    g_ngfx.line_to(cr, cx + cw, cy + 104.5);
    g_ngfx.stroke(cr);

    struct { const char *lbl; int act; int arg; } bms[7] = {
        {"Portal", HIT_BROWSER_TAB, 0},
        {"Architecture", HIT_BROWSER_TAB, 1},
        {"Ubuntu Base", HIT_BROWSER_TAB, 2},
        {"Benchmarks", HIT_BROWSER_TAB, 3},
        {"Files", HIT_BROWSER_ACTION, APP_FILES},
        {"Monitor", HIT_BROWSER_ACTION, APP_MONITOR},
        {"Settings", HIT_BROWSER_ACTION, APP_SETTINGS},
    };
    int bx = cx + 10;
    for (int i = 0; i < 7; ++i) {
        int bw = (int)ngfx_text_width(cr, "Noto Sans", 0, 9.8, bms[i].lbl) + 16;
        if (bx + bw > cx + cw - 10) break;
        int is_cur = (bms[i].act == HIT_BROWSER_TAB && (st->browser_tab % NUM_TABS) == bms[i].arg);
        int bhov = (st->hover_hit_action == bms[i].act && st->hover_hit_arg0 == bms[i].arg);
        ngfx_fill_rect(cr, bx, cy + 81, bw, 20, 6,
                       is_cur ? ar : (bhov ? pal.hover_r : pal.elev_r),
                       is_cur ? ag : (bhov ? pal.hover_g : pal.elev_g),
                       is_cur ? ab : (bhov ? pal.hover_b : pal.elev_b),
                       is_cur ? (dark ? 0.28 : 0.16) : (bhov ? 0.95 : 0.72));
        if (is_cur) ngfx_stroke_rect(cr, bx + 0.5, cy + 81.5, bw - 1, 19, 6, 0.8, ar, ag, ab, 0.65);
        ngfx_text(cr, "Noto Sans", is_cur, 9.8, bx + 8, cy + 94.5,
                  is_cur ? (dark ? pal.ar_hi : ar) : pal.fg_sec_r,
                  is_cur ? (dark ? pal.ag_hi : ag) : pal.fg_sec_g,
                  is_cur ? (dark ? pal.ab_hi : ab) : pal.fg_sec_b, 0.96, bms[i].lbl);
        nature_add_hit(st, bx, cy + 81, bw, 20, bms[i].act, bms[i].arg, 0);
        bx += bw + 6;
    }

    /* Scrollable Document Viewport */
    int doc_y = cy + 105;
    int doc_h = ch - 109;
    if (doc_h < 60) return;

    int tab = st->browser_tab % NUM_TABS;
    int content_h = 340;
    if (tab == 0) content_h = compact ? 426 : 362;
    else if (tab == 1) content_h = 345;
    else if (tab == 2) content_h = 295;
    else content_h = 345;

    st->browser_max_scroll = std::max(0, content_h - doc_h);
    if (st->browser_scroll > st->browser_max_scroll) st->browser_scroll = st->browser_max_scroll;
    if (st->browser_scroll < 0) st->browser_scroll = 0;

    g_ngfx.save(cr);
    g_ngfx.rectangle(cr, cx, doc_y, cw - 14, doc_h);
    g_ngfx.clip(cr);

    int px = cx + (compact ? 16 : 24);
    int pw = cw - (compact ? 42 : 60);
    int py = doc_y + 18 - st->browser_scroll;

    if (tab == 0) {
        /* System Portal */
        ngfx_shadow_rect(cr, px, py, pw, 84, 14, 1.8, 0.24);
        ngfx_fill_gradient(cr, px, py, pw, 84, 14,
                           ar * 0.48, ag * 0.48, ab * 0.48, 0.96,
                           pal.sidebar_r, pal.sidebar_g, pal.sidebar_b, 0.98, 1);

        /* Clipped ambient circles inside hero banner */
        g_ngfx.save(cr);
        ngfx_rounded_path(cr, px, py, pw, 84, 14);
        g_ngfx.clip(cr);
        g_ngfx.set_source_rgba(cr, 1.0, 1.0, 1.0, 0.06);
        g_ngfx.arc(cr, px + pw - 36, py + 42, 54, 0, 6.28318);
        g_ngfx.fill(cr);
        g_ngfx.set_source_rgba(cr, 1.0, 1.0, 1.0, 0.08);
        g_ngfx.set_line_width(cr, 1.2);
        g_ngfx.arc(cr, px + pw - 36, py + 42, 76, 0, 6.28318);
        g_ngfx.stroke(cr);
        g_ngfx.restore(cr);

        ngfx_stroke_rect(cr, px + 0.5, py + 0.5, pw - 1, 83, 14, 1.0, ar, ag, ab, 0.50);
        ngfx_top_specular(cr, px, py, pw, 14, 0.20);

        ngfx_text_clipped(cr, "Plus Jakarta Sans", 1, compact ? 15.5 : 19.0, px + 20, py + 33, pw - 40, 1, 1, 1, 1,
                          "Sylva Web · Nature OS Portal");
        ngfx_text_clipped(cr, "Noto Sans", 0, 11.2, px + 20, py + 57, pw - 40, 0.88, 0.96, 0.92, 0.92,
                          "Native C++20 WebKit/GTK4 browser engine on Ubuntu 24.04 Noble");

        int qy = py + 98;
        const char *qtitles[4] = {"Launch Terminal", "System Settings", "Software Store", "Media Viewer"};
        const char *qdescs[4] = {"Run bash & apt commands", "Customize theme & display", "Browse Ubuntu packages", "Photos, grid & audio player"};
        const int qapps[4] = {APP_TERMINAL, APP_SETTINGS, APP_STORE, APP_MEDIA};

        if (compact) {
            int qw = (pw - 10) / 2;
            for (int i = 0; i < 4; ++i) {
                int col = i % 2;
                int row = i / 2;
                int qx = px + col * (qw + 10);
                int ycard = qy + row * 70;
                int hov = (st->hover_hit_action == HIT_BROWSER_ACTION && st->hover_hit_arg0 == qapps[i]);
                ngfx_fill_rect(cr, qx, ycard, qw, 62, 11,
                               hov ? pal.hover_r : pal.card_r, hov ? pal.hover_g : pal.card_g, hov ? pal.hover_b : pal.card_b, 0.96);
                ngfx_stroke_rect(cr, qx + 0.5, ycard + 0.5, qw - 1, 61, 11, 1.0,
                                 hov ? ar : pal.border_r, hov ? ag : pal.border_g, hov ? ab : pal.border_b, hov ? 0.55 : pal.border_a);
                ngfx_fill_rect(cr, qx + 1, ycard + 12, 3, 38, 1.5, ar, ag, ab, hov ? 0.95 : 0.45);
                /* Small app badge icon */
                ngfx_fill_rect(cr, qx + 10, ycard + 11, 20, 20, 6, ar, ag, ab, 0.18);
                g_ngfx.set_source_rgba(cr, dark ? pal.ar_hi : ar, dark ? pal.ag_hi : ag, dark ? pal.ab_hi : ab, 0.95);
                g_ngfx.set_line_width(cr, 1.3);
                if (i == 0) {
                    g_ngfx.move_to(cr, qx + 15, ycard + 17); g_ngfx.line_to(cr, qx + 20, ycard + 21); g_ngfx.line_to(cr, qx + 15, ycard + 25); g_ngfx.stroke(cr);
                    ngfx_fill_rect(cr, qx + 21, ycard + 24, 5, 1.5, 0.5, dark ? pal.ar_hi : ar, dark ? pal.ag_hi : ag, dark ? pal.ab_hi : ab, 0.95);
                } else if (i == 1) {
                    g_ngfx.new_sub_path(cr); g_ngfx.arc(cr, qx + 20, ycard + 21, 4.5, 0, 6.283); g_ngfx.stroke(cr);
                    ngfx_fill_circle(cr, qx + 20, ycard + 21, 1.6, dark ? pal.ar_hi : ar, dark ? pal.ag_hi : ag, dark ? pal.ab_hi : ab, 0.95);
                } else if (i == 2) {
                    ngfx_stroke_rect(cr, qx + 15, ycard + 18, 10, 8, 1.5, 1.2, dark ? pal.ar_hi : ar, dark ? pal.ag_hi : ag, dark ? pal.ab_hi : ab, 0.95);
                    g_ngfx.new_sub_path(cr); g_ngfx.arc(cr, qx + 20, ycard + 18, 2.5, 3.14159, 0); g_ngfx.stroke(cr);
                } else {
                    ngfx_stroke_rect(cr, qx + 14.5, ycard + 16, 11, 10, 1.5, 1.2, dark ? pal.ar_hi : ar, dark ? pal.ag_hi : ag, dark ? pal.ab_hi : ab, 0.95);
                    ngfx_fill_circle(cr, qx + 22, ycard + 19, 1.4, pal.warning_r, pal.warning_g, pal.warning_b, 0.95);
                }
                ngfx_text_clipped(cr, "Plus Jakarta Sans", 1, 11.4, qx + 35, ycard + 24, qw - 44, fg_r, fg_g, fg_b, 1.0, qtitles[i]);
                ngfx_text_clipped(cr, "Noto Sans", 0, 9.8, qx + 12, ycard + 46, qw - 20, mut_r, mut_g, mut_b, 1.0, qdescs[i]);
                if (ycard + 62 > doc_y && ycard < doc_y + doc_h) {
                    nature_add_hit(st, qx, std::max(doc_y, ycard), qw, 62, HIT_BROWSER_ACTION, qapps[i], 0);
                }
            }
            qy += 150;
        } else {
            int qw = (pw - 30) / 4;
            for (int i = 0; i < 4; ++i) {
                int qx = px + i * (qw + 10);
                int hov = (st->hover_hit_action == HIT_BROWSER_ACTION && st->hover_hit_arg0 == qapps[i]);
                ngfx_fill_rect(cr, qx, qy, qw, 76, 11,
                               hov ? pal.hover_r : pal.card_r, hov ? pal.hover_g : pal.card_g, hov ? pal.hover_b : pal.card_b, 0.96);
                ngfx_stroke_rect(cr, qx + 0.5, qy + 0.5, qw - 1, 75, 11, 1.0,
                                 hov ? ar : pal.border_r, hov ? ag : pal.border_g, hov ? ab : pal.border_b, hov ? 0.58 : pal.border_a);
                ngfx_fill_rect(cr, qx + 12, qy + 1, qw - 24, 2.5, 1.2, ar, ag, ab, hov ? 0.95 : 0.40);
                ngfx_fill_rect(cr, qx + 12, qy + 12, 22, 22, 6, ar, ag, ab, 0.16);
                g_ngfx.set_source_rgba(cr, dark ? pal.ar_hi : ar, dark ? pal.ag_hi : ag, dark ? pal.ab_hi : ab, 0.95);
                g_ngfx.set_line_width(cr, 1.35);
                if (i == 0) {
                    g_ngfx.move_to(cr, qx + 17, qy + 19); g_ngfx.line_to(cr, qx + 22, qy + 23); g_ngfx.line_to(cr, qx + 17, qy + 27); g_ngfx.stroke(cr);
                    ngfx_fill_rect(cr, qx + 23, qy + 26, 5, 1.6, 0.5, dark ? pal.ar_hi : ar, dark ? pal.ag_hi : ag, dark ? pal.ab_hi : ab, 0.95);
                } else if (i == 1) {
                    g_ngfx.new_sub_path(cr); g_ngfx.arc(cr, qx + 23, qy + 23, 5.0, 0, 6.283); g_ngfx.stroke(cr);
                    ngfx_fill_circle(cr, qx + 23, qy + 23, 1.8, dark ? pal.ar_hi : ar, dark ? pal.ag_hi : ag, dark ? pal.ab_hi : ab, 0.95);
                } else if (i == 2) {
                    ngfx_stroke_rect(cr, qx + 17.5, qy + 20, 11, 8.5, 1.5, 1.25, dark ? pal.ar_hi : ar, dark ? pal.ag_hi : ag, dark ? pal.ab_hi : ab, 0.95);
                    g_ngfx.new_sub_path(cr); g_ngfx.arc(cr, qx + 23, qy + 20, 2.8, 3.14159, 0); g_ngfx.stroke(cr);
                } else {
                    ngfx_stroke_rect(cr, qx + 17, qy + 18, 12, 10, 1.5, 1.25, dark ? pal.ar_hi : ar, dark ? pal.ag_hi : ag, dark ? pal.ab_hi : ab, 0.95);
                    ngfx_fill_circle(cr, qx + 25.5, qy + 21, 1.5, pal.warning_r, pal.warning_g, pal.warning_b, 0.95);
                }
                ngfx_text_clipped(cr, "Plus Jakarta Sans", 1, 11.4, qx + 40, qy + 27, qw - 58, fg_r, fg_g, fg_b, 1.0, qtitles[i]);
                ngfx_text_clipped(cr, "Noto Sans", 0, 10.0, qx + 12, qy + 54, qw - 20, mut_r, mut_g, mut_b, 1.0, qdescs[i]);
                ngfx_text(cr, "JetBrains Mono", 1, 11.5, qx + qw - 18, qy + 28,
                          hov ? ar : mut_r, hov ? ag : mut_g, hov ? ab : mut_b, hov ? 1.0 : 0.50, ">");
                if (qy + 76 > doc_y && qy < doc_y + doc_h) {
                    nature_add_hit(st, qx, std::max(doc_y, qy), qw, 76, HIT_BROWSER_ACTION, qapps[i], 0);
                }
            }
            qy += 94;
        }

        ngfx_fill_rect(cr, px, qy, pw, 64, 11, pal.card_r, pal.card_g, pal.card_b, 0.96);
        ngfx_stroke_rect(cr, px + 0.5, qy + 0.5, pw - 1, 63, 11, 0.9, pal.border_r, pal.border_g, pal.border_b, pal.border_a);
        const char *host = st->cfg.device_name[0] ? st->cfg.device_name : "nature-pc";
        char live[160];
        std::snprintf(live, sizeof(live), "%.20s · %.0f FPS (%.1f ms) · RAM %d/%dM · Disk %ldG free · %s",
                      host, st->measured_fps, st->frame_ms, st->used_ram_mb, st->total_ram_mb,
                      st->disk_avail_gb, st->cfg.wifi_enabled ? "Wi-Fi OK" : "Offline");
        ngfx_fill_circle(cr, px + 21, qy + 21, 3.8, pal.success_r, pal.success_g, pal.success_b, 0.95);
        ngfx_text(cr, "Plus Jakarta Sans", 1, 12.0, px + 31, qy + 25, fg_r, fg_g, fg_b, 1.0, "Active Session Status");
        ngfx_text_clipped(cr, "JetBrains Mono", 0, 10.4, px + 16, qy + 47, pw - 32,
                          dark ? pal.ar_hi : ar, dark ? pal.ag_hi : ag, dark ? pal.ab_hi : ab, 1.0, live);

        /* Theme & Display Quick-Switcher Row */
        int ty = qy + 74;
        int tw = (pw - 10) / 2;
        int hov_t0 = (st->hover_hit_action == HIT_BROWSER_THEME_ACTION && st->hover_hit_arg0 == 0);
        ngfx_fill_rect(cr, px, ty, tw, 36, 9,
                       hov_t0 ? pal.hover_r : pal.card_r, hov_t0 ? pal.hover_g : pal.card_g, hov_t0 ? pal.hover_b : pal.card_b, 0.96);
        ngfx_stroke_rect(cr, px + 0.5, ty + 0.5, tw - 1, 35, 9, 0.9, hov_t0 ? ar : pal.border_r, hov_t0 ? ag : pal.border_g, hov_t0 ? ab : pal.border_b, hov_t0 ? 0.65 : pal.border_a);
        ngfx_fill_circle(cr, px + 16, ty + 18, 4.5, ar, ag, ab, 0.95);
        ngfx_text_clipped(cr, "Noto Sans", 1, 10.6, px + 28, ty + 22, tw - 38, fg_r, fg_g, fg_b, 0.96,
                          st->cfg.theme_family == THEME_HORIZON ? "Style: Horizon -> Switch Nature" : "Style: Nature -> Switch Horizon");
        if (ty + 36 > doc_y && ty < doc_y + doc_h) {
            nature_add_hit(st, px, std::max(doc_y, ty), tw, 36, HIT_BROWSER_THEME_ACTION, 0, 0);
        }

        int tx2 = px + tw + 10;
        int hov_t1 = (st->hover_hit_action == HIT_BROWSER_THEME_ACTION && st->hover_hit_arg0 == 1);
        ngfx_fill_rect(cr, tx2, ty, tw, 36, 9,
                       hov_t1 ? pal.hover_r : pal.card_r, hov_t1 ? pal.hover_g : pal.card_g, hov_t1 ? pal.hover_b : pal.card_b, 0.96);
        ngfx_stroke_rect(cr, tx2 + 0.5, ty + 0.5, tw - 1, 35, 9, 0.9, hov_t1 ? ar : pal.border_r, hov_t1 ? ag : pal.border_g, hov_t1 ? ab : pal.border_b, hov_t1 ? 0.65 : pal.border_a);
        ngfx_fill_circle(cr, tx2 + 16, ty + 18, 4.5, dark ? 0.95 : 0.16, dark ? 0.78 : 0.24, dark ? 0.26 : 0.38, 0.95);
        ngfx_text_clipped(cr, "Noto Sans", 1, 10.6, tx2 + 28, ty + 22, tw - 38, fg_r, fg_g, fg_b, 0.96,
                          dark ? "Appearance: Dark -> Light Mode" : "Appearance: Light -> Dark Mode");
        if (ty + 36 > doc_y && ty < doc_y + doc_h) {
            nature_add_hit(st, tx2, std::max(doc_y, ty), tw, 36, HIT_BROWSER_THEME_ACTION, 1, 0);
        }
    } else if (tab == 1) {
        ngfx_text_clipped(cr, "Plus Jakarta Sans", 1, compact ? 15.5 : 19.0, px, py + 14, pw, fg_r, fg_g, fg_b, 1.0,
                          "Nature OS: Low-Overhead Wayland Desktop");
        ngfx_text_clipped(cr, "Noto Sans", 0, 11.5, px, py + 38, pw, mut_r, mut_g, mut_b, 1.0,
                          "Engineered in C17 and C++20 on Ubuntu 24.04 LTS for minimal RAM & CPU usage.");

        const char *stack_pills[4] = {"DRM/KMS", "Cairo + zlib", "NAT3 Tiles", "cgroup v2"};
        int spx = px;
        for (int p = 0; p < 4; ++p) {
            int spw = (int)ngfx_text_width(cr, "JetBrains Mono", 1, 9.5, stack_pills[p]) + 16;
            if (spx + spw > px + pw) break;
            ngfx_fill_rect(cr, spx, py + 48, spw, 21, 6, ar, ag, ab, dark ? 0.22 : 0.14);
            ngfx_stroke_rect(cr, spx + 0.5, py + 48.5, spw - 1, 20, 6, 0.8, ar, ag, ab, 0.55);
            ngfx_text(cr, "JetBrains Mono", 1, 9.5, spx + 8, py + 62,
                      dark ? pal.ar_hi : ar, dark ? pal.ag_hi : ag, dark ? pal.ab_hi : ab, 0.98, stack_pills[p]);
            spx += spw + 8;
        }

        const char *cards[3][2] = {
            {"1. C17 Cairo Tile Compositor", "Cached wallpaper + FNV-1a dirty-tile hashing skips idle redraws and isolates pointer pacing."},
            {"2. Native C17 & C++20 Apps", "Zero Electron bloat. Full desktop, shell, monitor, and browser run inside one lean process."},
            {"3. Real /proc & cgroup Telemetry", "Reads live /proc/stat, /proc/meminfo, and statvfs metrics with zero synthetic throttling."},
        };
        int cy_card = py + 80;
        for (int i = 0; i < 3; ++i) {
            ngfx_fill_rect(cr, px, cy_card, pw, 62, 10, pal.card_r, pal.card_g, pal.card_b, 0.96);
            ngfx_stroke_rect(cr, px + 0.5, cy_card + 0.5, pw - 1, 61, 10, 0.9, pal.border_r, pal.border_g, pal.border_b, pal.border_a);
            ngfx_fill_rect(cr, px + 1, cy_card + 12, 3, 38, 1.5, ar, ag, ab, 0.78);
            ngfx_text_clipped(cr, "Noto Sans", 1, 12.2, px + 16, cy_card + 24, pw - 32, fg_r, fg_g, fg_b, 1.0, cards[i][0]);
            ngfx_text_clipped(cr, "Noto Sans", 0, 11.0, px + 16, cy_card + 44, pw - 32, mut_r, mut_g, mut_b, 1.0, cards[i][1]);
            cy_card += 72;
        }
    } else if (tab == 2) {
        ngfx_text_clipped(cr, "Plus Jakarta Sans", 1, compact ? 15.5 : 19.0, px, py + 14, pw, fg_r, fg_g, fg_b, 1.0,
                          "Ubuntu 24.04 Noble Base & APT Integration");
        ngfx_text_clipped(cr, "Noto Sans", 0, 11.5, px, py + 38, pw, mut_r, mut_g, mut_b, 1.0,
                          "Binary compatible with Ubuntu .deb repos, libapt-pkg, PipeWire & systemd.");
        ngfx_shadow_rect(cr, px, py + 54, pw, 146, 10, 1.4, 0.20);
        ngfx_fill_rect(cr, px, py + 54, pw, 146, 10, pal.sidebar_r, pal.sidebar_g, pal.sidebar_b, 1.0);
        ngfx_stroke_rect(cr, px + 0.5, py + 54.5, pw - 1, 145, 10, 0.9, ar, ag, ab, 0.32);
        ngfx_text_clipped(cr, "JetBrains Mono", 0, 11.0, px + 14, py + 82, pw - 28,
                          dark ? pal.ar_hi : ar, dark ? pal.ag_hi : ag, dark ? pal.ab_hi : ab, 1.0,
                          "$ sudo apt install nature-os-desktop");
        ngfx_text_clipped(cr, "JetBrains Mono", 0, 10.6, px + 14, py + 106, pw - 28, fg_r, fg_g, fg_b, 0.9,
                          "Depends: ubuntu-minimal, systemd, pipewire, libcairo2, zlib1g, zram-tools");
        ngfx_text_clipped(cr, "JetBrains Mono", 0, 10.6, px + 14, py + 130, pw - 28, fg_r, fg_g, fg_b, 0.9,
                          "Schemas: /usr/share/glib-2.0/schemas/org.natureos.desktop.gschema.xml");
        ngfx_text_clipped(cr, "JetBrains Mono", 0, 10.6, px + 14, py + 154, pw - 28, fg_r, fg_g, fg_b, 0.9,
                          "Session: /usr/lib/systemd/user/nature-session.service (CPUWeight=300)");

        int bw_u = (pw - 12) / 2;
        int by_u = py + 214;
        ngfx_fill_gradient(cr, px, by_u, bw_u, 34, 9, ar, ag, ab, 0.94, ar * 0.78, ag * 0.78, ab * 0.78, 0.94, 0);
        ngfx_text_clipped(cr, "Noto Sans", 1, 11.0, px + 16, by_u + 21, bw_u - 24, pal.on_acc_r, pal.on_acc_g, pal.on_acc_b, 1.0, "Open Ubuntu Software");
        if (by_u + 34 > doc_y && by_u < doc_y + doc_h) {
            nature_add_hit(st, px, std::max(doc_y, by_u), bw_u, 34, HIT_BROWSER_ACTION, APP_STORE, 0);
        }

        int b2x = px + bw_u + 12;
        ngfx_fill_rect(cr, b2x, by_u, bw_u, 34, 9, pal.card_r, pal.card_g, pal.card_b, 0.96);
        ngfx_stroke_rect(cr, b2x + 0.5, by_u + 0.5, bw_u - 1, 33, 9, 0.9, pal.border_r, pal.border_g, pal.border_b, pal.border_a);
        ngfx_text_clipped(cr, "Noto Sans", 1, 11.0, b2x + 16, by_u + 21, bw_u - 24, fg_r, fg_g, fg_b, 0.96, "Inspect /etc/nature-os");
        if (by_u + 34 > doc_y && by_u < doc_y + doc_h) {
            nature_add_hit(st, b2x, std::max(doc_y, by_u), bw_u, 34, HIT_BROWSER_ACTION, APP_FILES, 1);
        }
    } else {
        ngfx_text_clipped(cr, "Plus Jakarta Sans", 1, compact ? 15.5 : 19.0, px, py + 14, pw - 178, fg_r, fg_g, fg_b, 1.0,
                          "Live Runtime & Memory Footprint");

        int bbtn_w = compact ? 132 : 168;
        int bbtn_x = px + pw - bbtn_w;
        ngfx_fill_rect(cr, bbtn_x, py - 4, bbtn_w, 28, 8,
                       st->stress_active ? pal.danger_r : ar,
                       st->stress_active ? pal.danger_g : ag,
                       st->stress_active ? pal.danger_b : ab, 0.94);
        ngfx_text_clipped(cr, "Noto Sans", 1, 10.4, bbtn_x + 12, py + 14, bbtn_w - 20,
                          st->stress_active ? 1.0 : pal.on_acc_r,
                          st->stress_active ? 1.0 : pal.on_acc_g,
                          st->stress_active ? 1.0 : pal.on_acc_b, 1.0,
                          st->stress_active ? "Stop Benchmark" : "Run 4-Core Benchmark");
        if (py + 24 > doc_y && py - 4 < doc_y + doc_h) {
            nature_add_hit(st, bbtn_x, std::max(doc_y, py - 4), bbtn_w, 28, HIT_BROWSER_RUN_STRESS, 0, 0);
        }

        char b0[36], b1[36], b2[36], b3[36];
        std::snprintf(b0, sizeof(b0), "%d MiB OS RSS", st->os_rss_mb);
        std::snprintf(b1, sizeof(b1), "%.1f ms / frame", st->frame_ms);
        std::snprintf(b2, sizeof(b2), "%.1f%% (/proc)", st->total_cpu_pct);
        std::snprintf(b3, sizeof(b3), "%.0f FPS locked", st->measured_fps);
        const char *rows[4][3] = {
            {"Compositor Resident RAM", b0, "1,180 MiB (Std GNOME)"},
            {"Frame Render Latency", b1, "16.6 ms budget (60Hz)"},
            {"Live System CPU Load", b2, "Isolated UI cgroup"},
            {"Interactive UI Pacing", b3, "0 skipped resize frames"},
        };
        double bars_frac[4] = {
            std::clamp((double)st->os_rss_mb / 256.0, 0.05, 1.0),
            std::clamp(st->frame_ms / 16.6, 0.06, 1.0),
            std::clamp(st->total_cpu_pct / 100.0, 0.03, 1.0),
            std::clamp(st->measured_fps / 60.0, 0.15, 1.0),
        };
        int ry = py + 38;
        int c1 = compact ? (pw * 40 / 100) : 200;
        int c2 = compact ? (pw * 72 / 100) : 370;
        for (int i = 0; i < 4; ++i) {
            ngfx_fill_rect(cr, px, ry, pw, 52, 8,
                           (i % 2 == 0) ? pal.card_r : pal.surface_r,
                           (i % 2 == 0) ? pal.card_g : pal.surface_g,
                           (i % 2 == 0) ? pal.card_b : pal.surface_b, 0.95);
            ngfx_stroke_rect(cr, px + 0.5, ry + 0.5, pw - 1, 51, 8, 0.8, pal.border_r, pal.border_g, pal.border_b, pal.border_a);
            ngfx_text_clipped(cr, "Noto Sans", 1, 11.2, px + 14, ry + 23, c1 - 18, fg_r, fg_g, fg_b, 1.0, rows[i][0]);
            ngfx_text_clipped(cr, "JetBrains Mono", 1, 10.8, px + c1, ry + 23, c2 - c1 - 8,
                              dark ? pal.ar_hi : ar, dark ? pal.ag_hi : ag, dark ? pal.ab_hi : ab, 1.0, rows[i][1]);
            ngfx_text_clipped(cr, "JetBrains Mono", 0, 10.6, px + c2, ry + 23, pw - c2 - 10, mut_r, mut_g, mut_b, 1.0, rows[i][2]);
            ngfx_fill_rect(cr, px + 14, ry + 35, pw - 28, 6, 3, pal.elev_r, pal.elev_g, pal.elev_b, 0.95);
            ngfx_fill_rect(cr, px + 14, ry + 35, (pw - 28) * bars_frac[i], 6, 3,
                           (i == 2 && st->stress_active) ? pal.warning_r : ar,
                           (i == 2 && st->stress_active) ? pal.warning_g : ag,
                           (i == 2 && st->stress_active) ? pal.warning_b : ab, 0.94);
            ry += 60;
        }
    }

    g_ngfx.restore(cr);

    nature_draw_scrollbar(st, cr, cx + cw - 11, doc_y, doc_h,
                          content_h, doc_h, st->browser_scroll,
                          st->browser_max_scroll, SCROLL_BROWSER, dark);
}

void nature_browser_wheel(NatureOSState *st, int delta_y) {
    int step = (delta_y > 0) ? 34 : -34;
    st->browser_scroll += step;
    if (st->browser_scroll < 0) st->browser_scroll = 0;
    if (st->browser_scroll > st->browser_max_scroll) st->browser_scroll = st->browser_max_scroll;
}

void nature_browser_key(NatureOSState *st, const char *key) {
    if (!key || !*key) return;
    if (std::strcmp(key, "Backspace") == 0) {
        size_t len = std::strlen(st->browser_url_input);
        if (len > 0) st->browser_url_input[len - 1] = '\0';
        return;
    }
    if (std::strcmp(key, "Enter") == 0) {
        const char *q = st->browser_url_input;
        if (*q) {
            if (str_ci_contains(q, "portal") || std::strcmp(q, "0") == 0) st->browser_tab = 0;
            else if (str_ci_contains(q, "arch") || std::strcmp(q, "1") == 0) st->browser_tab = 1;
            else if (str_ci_contains(q, "ubuntu") || str_ci_contains(q, "noble") || std::strcmp(q, "2") == 0) st->browser_tab = 2;
            else if (str_ci_contains(q, "bench") || str_ci_contains(q, "multi") || std::strcmp(q, "3") == 0) st->browser_tab = 3;
            else nature_notify(st, "Sylva Browser", "Local routes: portal, arch, ubuntu, bench");
        }
        st->browser_scroll = 0;
        st->active_text_target = 0;
        st->browser_url_input[0] = '\0';
        nature_os_save_state(st);
        return;
    }
    if (std::strlen(key) == 1 && (unsigned char)key[0] >= 32 && (unsigned char)key[0] < 127) {
        size_t len = std::strlen(st->browser_url_input);
        if (len + 1 < sizeof(st->browser_url_input)) {
            st->browser_url_input[len] = key[0];
            st->browser_url_input[len + 1] = '\0';
        }
    }
}

void nature_browser_click(NatureOSState *st, const HitRect *hit) {
    if (hit->action_id == HIT_BROWSER_TAB) {
        st->browser_tab = hit->arg0;
        st->browser_scroll = 0;
        nature_os_save_state(st);
    } else if (hit->action_id == HIT_BROWSER_BACK) {
        st->browser_tab = (st->browser_tab + NUM_TABS - 1) % NUM_TABS;
        st->browser_scroll = 0;
        nature_os_save_state(st);
    } else if (hit->action_id == HIT_BROWSER_FWD) {
        st->browser_tab = (st->browser_tab + 1) % NUM_TABS;
        st->browser_scroll = 0;
        nature_os_save_state(st);
    } else if (hit->action_id == HIT_BROWSER_RELOAD) {
        st->browser_reloads++;
    } else if (hit->action_id == HIT_BROWSER_URL_BAR) {
        st->active_text_target = 3;
        st->input_focus_pulse = 1;
        st->browser_url_input[0] = '\0';
    } else if (hit->action_id == HIT_BROWSER_ACTION) {
        if (hit->arg0 == APP_FILES && hit->arg1 == 1) {
            std::snprintf(st->files_cwd, sizeof(st->files_cwd), "/etc/nature-os");
            st->files_selected_idx = -1;
            st->files_scroll = 0;
        }
        nature_open_app(st, (AppType)hit->arg0);
    } else if (hit->action_id == HIT_BROWSER_RUN_STRESS) {
        nature_monitor_set_stress(st, !st->stress_active);
    } else if (hit->action_id == HIT_BROWSER_THEME_ACTION) {
        if (hit->arg0 == 0) {
            st->cfg.theme_family = (st->cfg.theme_family == THEME_NATURE) ? THEME_HORIZON : THEME_NATURE;
            if (st->cfg.theme_family == THEME_HORIZON) {
                nature_os_set_wallpaper_preset(st, st->cfg.dark_mode ? 4 : 3);
            } else if (st->cfg.wallpaper_idx == 3 || st->cfg.wallpaper_idx == 4 || st->cfg.wallpaper_idx == 7) {
                nature_os_set_wallpaper_preset(st, 0);
            }
            nature_notify(st, "Theme Updated",
                          st->cfg.theme_family == THEME_HORIZON ? "Switched to Horizon theme." : "Switched to Nature Canopy theme.");
        } else {
            st->cfg.dark_mode = !st->cfg.dark_mode;
            if (st->cfg.theme_family == THEME_HORIZON && (st->cfg.wallpaper_idx == 3 || st->cfg.wallpaper_idx == 4)) {
                nature_os_set_wallpaper_preset(st, st->cfg.dark_mode ? 4 : 3);
            }
        }
        nature_os_save_state(st);
    }
}

} // extern "C"
