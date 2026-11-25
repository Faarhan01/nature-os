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
    int dark = st->cfg.dark_mode;
    double ar, ag, ab;
    nature_get_accent(st->cfg.accent_idx, &ar, &ag, &ab);

    double fg_r = dark ? 0.92 : 0.10, fg_g = dark ? 0.95 : 0.13, fg_b = dark ? 0.93 : 0.11;
    double mut_r = dark ? 0.62 : 0.44, mut_g = dark ? 0.70 : 0.50, mut_b = dark ? 0.66 : 0.47;
    double brd_c = dark ? 1.0 : 0.0;
    int compact = cw < 540;

    ngfx_fill_rect(cr, cx, cy, cw, ch, 0,
                   dark ? 0.050 : 0.98, dark ? 0.074 : 0.99, dark ? 0.066 : 0.985, 1.0);

    /* Tab strip */
    ngfx_fill_rect(cr, cx, cy, cw, 36, 0,
                   dark ? 0.036 : 0.92, dark ? 0.052 : 0.94, dark ? 0.046 : 0.93, 1.0);
    g_ngfx.set_source_rgba(cr, brd_c, brd_c, brd_c, dark ? 0.07 : 0.08);
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
                       act ? (dark ? 0.082 : 0.99) : brd_c,
                       act ? (dark ? 0.120 : 0.99) : brd_c,
                       act ? (dark ? 0.108 : 0.99) : brd_c,
                       act ? 1.0 : (hov ? 0.09 : 0.04));
        if (act) {
            ngfx_stroke_rect(cr, tx + 0.5, cy + 5.5, tab_w - 1, 25, 7, 0.8, brd_c, brd_c, brd_c, dark ? 0.10 : 0.08);
            ngfx_top_specular(cr, tx, cy + 5, tab_w, 7, dark ? 0.14 : 0.35);
            ngfx_fill_rect(cr, tx + 12, cy + 28, tab_w - 24, 2, 1, ar, ag, ab, 0.95);
        }
        ngfx_text_clipped(cr, "Noto Sans", act, compact ? 10.4 : 11.2, tx + 10, cy + 22, tab_w - 18,
                          fg_r, fg_g, fg_b, act ? 1.0 : (hov ? 0.92 : 0.72), BROWSER_TITLES[i]);
        nature_add_hit(st, tx, cy + 5, tab_w, 26, HIT_BROWSER_TAB, i, 0);
    }

    /* Address Bar */
    ngfx_fill_rect(cr, cx, cy + 37, cw, 40, 0,
                   dark ? 0.058 : 0.96, dark ? 0.088 : 0.97, dark ? 0.076 : 0.965, 1.0);
    g_ngfx.set_source_rgba(cr, brd_c, brd_c, brd_c, dark ? 0.07 : 0.08);
    g_ngfx.move_to(cr, cx, cy + 77.5);
    g_ngfx.line_to(cr, cx + cw, cy + 77.5);
    g_ngfx.stroke(cr);

    int hov_back = (st->hover_hit_action == HIT_BROWSER_BACK);
    ngfx_fill_rect(cr, cx + 12, cy + 44, 28, 26, 7, brd_c, brd_c, brd_c, hov_back ? 0.14 : 0.07);
    ngfx_stroke_rect(cr, cx + 12.5, cy + 44.5, 27, 25, 7, 0.8, brd_c, brd_c, brd_c, hov_back ? 0.18 : 0.07);
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
    ngfx_fill_rect(cr, cx + 44, cy + 44, 28, 26, 7, brd_c, brd_c, brd_c, hov_fwd ? 0.14 : 0.07);
    ngfx_stroke_rect(cr, cx + 44.5, cy + 44.5, 27, 25, 7, 0.8, brd_c, brd_c, brd_c, hov_fwd ? 0.18 : 0.07);
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
    ngfx_fill_rect(cr, url_x, cy + 44, url_w, 26, 8,
                   dark ? 0.034 : 1.0, dark ? 0.050 : 1.0, dark ? 0.044 : 1.0, 1.0);
    if (url_focused) {
        ngfx_stroke_rect(cr, url_x + 0.5, cy + 44.5, url_w - 1, 25, 8, 1.3, ar, ag, ab, 0.92);
    } else {
        ngfx_stroke_rect(cr, url_x + 0.5, cy + 44.5, url_w - 1, 25, 8, 0.9,
                         url_hov ? ar : brd_c, url_hov ? ag : brd_c, url_hov ? ab : brd_c,
                         url_hov ? 0.45 : (dark ? 0.09 : 0.12));
    }

    /* Vector padlock icon inside omnibox */
    g_ngfx.set_source_rgba(cr, 0.30, 0.86, 0.56, 0.92);
    g_ngfx.set_line_width(cr, 1.3);
    g_ngfx.arc(cr, url_x + 15.0, cy + 54.5, 2.5, 3.14159, 0.0);
    g_ngfx.stroke(cr);
    ngfx_fill_rect(cr, url_x + 11.5, cy + 55.0, 7.0, 5.5, 1.5, 0.30, 0.86, 0.56, 0.92);

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
                          ar * 1.35 > 1 ? 1 : ar * 1.35, ag * 1.3 > 1 ? 1 : ag * 1.3,
                          ab * 1.3 > 1 ? 1 : ab * 1.3, 1.0, BROWSER_URLS[st->browser_tab % NUM_TABS]);
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
    ngfx_text(cr, "Noto Sans", 1, 11.0, reload_x + (compact ? 9 : 14), cy + 61, 1, 1, 1, 1, "Reload");
    nature_add_hit(st, reload_x, cy + 44, reload_w, 26, HIT_BROWSER_RELOAD, 0, 0);

    /* Scrollable Document Viewport */
    int doc_y = cy + 79;
    int doc_h = ch - 83;
    if (doc_h < 60) return;

    int tab = st->browser_tab % NUM_TABS;
    int content_h = 340;
    if (tab == 0) content_h = compact ? 440 : 310;
    else if (tab == 1) content_h = 320;
    else if (tab == 2) content_h = 260;
    else content_h = 290;

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
                           0.05, 0.12, 0.10, 0.98, 1);

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
        const char *qtitles[3] = {"Launch Terminal", "System Settings", "Software Store"};
        const char *qdescs[3] = {"Run bash & apt commands", "Customize theme & display", "Browse Ubuntu packages"};
        const int qapps[3] = {APP_TERMINAL, APP_SETTINGS, APP_STORE};

        if (compact) {
            for (int i = 0; i < 3; ++i) {
                int ycard = qy + i * 74;
                int hov = (st->hover_hit_action == HIT_BROWSER_ACTION && st->hover_hit_arg0 == qapps[i]);
                ngfx_fill_rect(cr, px, ycard, pw, 64, 11, brd_c, brd_c, brd_c, hov ? 0.085 : 0.045);
                ngfx_stroke_rect(cr, px + 0.5, ycard + 0.5, pw - 1, 63, 11, 1.0,
                                 hov ? ar : brd_c, hov ? ag : brd_c, hov ? ab : brd_c, hov ? 0.55 : 0.08);
                ngfx_fill_rect(cr, px + 1, ycard + 12, 3, 40, 1.5, ar, ag, ab, hov ? 0.95 : 0.45);
                ngfx_text(cr, "Plus Jakarta Sans", 1, 12.2, px + 16, ycard + 24, fg_r, fg_g, fg_b, 1.0, qtitles[i]);
                ngfx_text_clipped(cr, "Noto Sans", 0, 10.5, px + 16, ycard + 44, pw - 42, mut_r, mut_g, mut_b, 1.0, qdescs[i]);
                ngfx_text(cr, "JetBrains Mono", 1, 12.0, px + pw - 24, ycard + 36,
                          hov ? ar : mut_r, hov ? ag : mut_g, hov ? ab : mut_b, hov ? 1.0 : 0.55, ">");
                if (ycard + 64 > doc_y && ycard < doc_y + doc_h) {
                    nature_add_hit(st, px, std::max(doc_y, ycard), pw, 64, HIT_BROWSER_ACTION, qapps[i], 0);
                }
            }
            qy += 230;
        } else {
            int qw = (pw - 24) / 3;
            for (int i = 0; i < 3; ++i) {
                int qx = px + i * (qw + 12);
                int hov = (st->hover_hit_action == HIT_BROWSER_ACTION && st->hover_hit_arg0 == qapps[i]);
                ngfx_fill_rect(cr, qx, qy, qw, 76, 11, brd_c, brd_c, brd_c, hov ? 0.085 : 0.045);
                ngfx_stroke_rect(cr, qx + 0.5, qy + 0.5, qw - 1, 75, 11, 1.0,
                                 hov ? ar : brd_c, hov ? ag : brd_c, hov ? ab : brd_c, hov ? 0.58 : 0.08);
                ngfx_fill_rect(cr, qx + 12, qy + 1, qw - 24, 2.5, 1.2, ar, ag, ab, hov ? 0.95 : 0.40);
                ngfx_text(cr, "Plus Jakarta Sans", 1, 12.5, qx + 14, qy + 29, fg_r, fg_g, fg_b, 1.0, qtitles[i]);
                ngfx_text_clipped(cr, "Noto Sans", 0, 10.5, qx + 14, qy + 49, qw - 34, mut_r, mut_g, mut_b, 1.0, qdescs[i]);
                ngfx_text(cr, "JetBrains Mono", 1, 11.5, qx + qw - 20, qy + 30,
                          hov ? ar : mut_r, hov ? ag : mut_g, hov ? ab : mut_b, hov ? 1.0 : 0.50, ">");
                if (qy + 76 > doc_y && qy < doc_y + doc_h) {
                    nature_add_hit(st, qx, std::max(doc_y, qy), qw, 76, HIT_BROWSER_ACTION, qapps[i], 0);
                }
            }
            qy += 94;
        }

        ngfx_fill_rect(cr, px, qy, pw, 64, 11, brd_c, brd_c, brd_c, 0.04);
        ngfx_stroke_rect(cr, px + 0.5, qy + 0.5, pw - 1, 63, 11, 0.9, brd_c, brd_c, brd_c, 0.08);
        char live[128];
        std::snprintf(live, sizeof(live), "%.0f FPS (%.1f ms) · RAM %d/%dM (OS %dM) · CPU %.1f%% · %s",
                      st->measured_fps, st->frame_ms, st->used_ram_mb, st->total_ram_mb, st->os_rss_mb,
                      st->total_cpu_pct, st->cfg.wifi_enabled ? "Wi-Fi OK" : "Offline");
        ngfx_fill_circle(cr, px + 21, qy + 21, 3.8, 0.25, 0.88, 0.54, 0.95);
        ngfx_text(cr, "Plus Jakarta Sans", 1, 12.0, px + 31, qy + 25, fg_r, fg_g, fg_b, 1.0, "Active Session Status");
        ngfx_text_clipped(cr, "JetBrains Mono", 0, 10.6, px + 16, qy + 47, pw - 32, ar * 1.35 > 1 ? 1 : ar * 1.35,
                          ag * 1.3 > 1 ? 1 : ag * 1.3, ab * 1.3 > 1 ? 1 : ab * 1.3, 1.0, live);
    } else if (tab == 1) {
        ngfx_text_clipped(cr, "Plus Jakarta Sans", 1, compact ? 15.5 : 19.0, px, py + 14, pw, fg_r, fg_g, fg_b, 1.0,
                          "Nature OS: Low-Overhead Wayland Desktop");
        ngfx_text_clipped(cr, "Noto Sans", 0, 11.5, px, py + 38, pw, mut_r, mut_g, mut_b, 1.0,
                          "Engineered in C17 and C++20 on Ubuntu 24.04 LTS for minimal RAM & CPU usage.");

        const char *cards[3][2] = {
            {"1. C17 Cairo Tile Compositor", "Cached wallpaper + FNV-1a dirty-tile hashing skips idle redraws and isolates pointer pacing."},
            {"2. Native C17 & C++20 Apps", "Zero Electron bloat. Full desktop, shell, monitor, and browser run inside one lean process."},
            {"3. Real /proc & cgroup Telemetry", "Reads live /proc/stat, /proc/meminfo, and statvfs metrics with zero synthetic throttling."},
        };
        int cy_card = py + 58;
        for (int i = 0; i < 3; ++i) {
            ngfx_fill_rect(cr, px, cy_card, pw, 62, 10, brd_c, brd_c, brd_c, 0.045);
            ngfx_stroke_rect(cr, px + 0.5, cy_card + 0.5, pw - 1, 61, 10, 0.9, brd_c, brd_c, brd_c, 0.08);
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
        ngfx_shadow_rect(cr, px, py + 54, pw, 150, 10, 1.4, 0.20);
        ngfx_fill_rect(cr, px, py + 54, pw, 150, 10, 0.036, 0.056, 0.048, 1.0);
        ngfx_stroke_rect(cr, px + 0.5, py + 54.5, pw - 1, 149, 10, 0.9, ar, ag, ab, 0.28);
        ngfx_text_clipped(cr, "JetBrains Mono", 0, 11.0, px + 14, py + 82, pw - 28, 0.35, 0.88, 0.58, 1.0,
                          "$ sudo apt install nature-os-desktop");
        ngfx_text_clipped(cr, "JetBrains Mono", 0, 10.6, px + 14, py + 106, pw - 28, 0.78, 0.86, 0.82, 1.0,
                          "Depends: ubuntu-minimal, systemd, pipewire, libcairo2, zlib1g, zram-tools");
        ngfx_text_clipped(cr, "JetBrains Mono", 0, 10.6, px + 14, py + 130, pw - 28, 0.78, 0.86, 0.82, 1.0,
                          "Schemas: /usr/share/glib-2.0/schemas/org.natureos.desktop.gschema.xml");
        ngfx_text_clipped(cr, "JetBrains Mono", 0, 10.6, px + 14, py + 154, pw - 28, 0.78, 0.86, 0.82, 1.0,
                          "Session: /usr/lib/systemd/user/nature-session.service (CPUWeight=300)");
    } else {
        ngfx_text_clipped(cr, "Plus Jakarta Sans", 1, compact ? 15.5 : 19.0, px, py + 14, pw, fg_r, fg_g, fg_b, 1.0,
                          "Live Runtime & Memory Footprint");
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
        int ry = py + 38;
        int c1 = compact ? (pw * 40 / 100) : 200;
        int c2 = compact ? (pw * 72 / 100) : 370;
        for (int i = 0; i < 4; ++i) {
            ngfx_fill_rect(cr, px, ry, pw, 42, 8, brd_c, brd_c, brd_c, (i % 2 == 0) ? 0.055 : 0.032);
            ngfx_stroke_rect(cr, px + 0.5, ry + 0.5, pw - 1, 41, 8, 0.8, brd_c, brd_c, brd_c, 0.07);
            ngfx_text_clipped(cr, "Noto Sans", 1, 11.2, px + 14, ry + 26, c1 - 18, fg_r, fg_g, fg_b, 1.0, rows[i][0]);
            ngfx_text_clipped(cr, "JetBrains Mono", 1, 10.8, px + c1, ry + 26, c2 - c1 - 8,
                              ar * 1.35 > 1 ? 1 : ar * 1.35, ag * 1.3 > 1 ? 1 : ag * 1.3,
                              ab * 1.3 > 1 ? 1 : ab * 1.3, 1.0, rows[i][1]);
            ngfx_text_clipped(cr, "JetBrains Mono", 0, 10.6, px + c2, ry + 26, pw - c2 - 10, mut_r, mut_g, mut_b, 1.0, rows[i][2]);
            ry += 50;
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
    } else if (hit->action_id == HIT_BROWSER_BACK) {
        st->browser_tab = (st->browser_tab + NUM_TABS - 1) % NUM_TABS;
        st->browser_scroll = 0;
    } else if (hit->action_id == HIT_BROWSER_FWD) {
        st->browser_tab = (st->browser_tab + 1) % NUM_TABS;
        st->browser_scroll = 0;
    } else if (hit->action_id == HIT_BROWSER_RELOAD) {
        st->browser_reloads++;
    } else if (hit->action_id == HIT_BROWSER_URL_BAR) {
        st->active_text_target = 3;
        st->input_focus_pulse = 1;
        st->browser_url_input[0] = '\0';
    } else if (hit->action_id == HIT_BROWSER_ACTION) {
        nature_open_app(st, (AppType)hit->arg0);
    }
}

} // extern "C"
