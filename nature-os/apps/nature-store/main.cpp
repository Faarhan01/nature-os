#include "nature_os.h"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>

static const char *STORE_CATS[5] = {"All", "Development", "Creative", "Productivity", "Utilities"};

static int str_ci_match(const char *hay, const char *needle) {
    if (!needle || !*needle) return 1;
    if (!hay) return 0;
    size_t hlen = std::strlen(hay), nlen = std::strlen(needle);
    if (nlen > hlen) return 0;
    for (size_t i = 0; i <= hlen - nlen; ++i) {
        size_t j = 0;
        while (j < nlen && std::tolower((unsigned char)hay[i + j]) == std::tolower((unsigned char)needle[j])) j++;
        if (j == nlen) return 1;
    }
    return 0;
}

extern "C" {

void nature_store_init(NatureOSState *st) {
    const AptPkg initial[16] = {
        {"nodejs", "Node.js 22 LTS Runtime", "Development", "22.12.0-1nodesource1", "apt (noble-nodesource)",
         "V8 JavaScript runtime with fs, os, http & process APIs.", 318, 42, 1, 100},
        {"npm", "npm Package Manager", "Development", "10.9.0-noble", "apt (noble-universe)",
         "Node.js package runner, script executor, and npx CLI.", 112, 24, 1, 100},
        {"python3", "Python 3.12 Interpreter", "Development", "3.12.3-0ubuntu2", "apt (noble-main)",
         "High-level scripting interpreter for automation & diagnostics.", 286, 28, 1, 100},
        {"git", "Git Distributed VCS", "Development", "2.43.0-1ubuntu7", "apt (noble-main)",
         "Fast distributed version control for ~/Projects workspaces.", 94, 18, 1, 100},
        {"neofetch", "Neofetch System Info", "Utilities", "7.1.0-4ubuntu1", "apt (noble-universe)",
         "CLI system summary utility for Ubuntu/Nature OS.", 4, 4, 1, 100},
        {"htop", "htop Process Viewer", "Utilities", "3.3.0-4build1", "apt (noble-main)",
         "Interactive ncurses process & CPU viewer.", 6, 6, 1, 100},
        {"btop", "btop++ Resource Monitor", "Utilities", "1.3.0-noble", "apt (noble-universe)",
         "C++20 TUI monitor with real-time core & memory graphs.", 14, 12, 0, 0},
        {"code", "Visual Studio Code", "Development", "1.95.2-noble", "apt (noble-universe)",
         "Native Wayland code editor with Git & C/C++ tooling.", 964, 240, 0, 0},
        {"docker.io", "Docker Engine", "Development", "24.0.7-ubuntu4", "apt (noble-universe)",
         "Linux container daemon with cgroup v2 isolation.", 441, 68, 0, 0},
        {"firefox", "Firefox Browser", "Productivity", "132.0-noble", "apt (noble-main)",
         "Wayland-native Mozilla browser with WebRender GPU path.", 842, 310, 0, 0},
        {"libreoffice", "LibreOffice Writer", "Productivity", "24.2.5-noble", "apt (noble-main)",
         "GTK4-integrated open-source document processor.", 645, 148, 0, 0},
        {"vlc", "VLC Media Player", "Creative", "3.0.20-3build6", "apt (noble-universe)",
         "VA-API hardware-accelerated multimedia player.", 228, 70, 1, 100},
        {"gimp", "GNU Image Editor", "Creative", "2.10.36-3ubuntu1", "apt (noble-universe)",
         "Open-source raster graphics & photo retouching suite.", 382, 160, 0, 0},
        {"inkscape", "Inkscape Vector Studio", "Creative", "1.3.2-noble", "apt (noble-universe)",
         "Precision SVG illustration and icon design suite.", 415, 134, 0, 0},
        {"obs-studio", "OBS Studio", "Creative", "30.1.2-noble", "apt (noble-universe)",
         "PipeWire & VA-API desktop capture and streaming studio.", 520, 195, 0, 0},
        {"blender", "Blender 3D Studio", "Creative", "4.0.2+dfsg-1", "apt (noble-universe)",
         "3D modeling & Vulkan/Wayland rendering pipeline.", 1420, 410, 0, 0},
    };
    st->pkg_count = 16;
    for (int i = 0; i < 16; ++i) st->pkgs[i] = initial[i];
    st->store_selected = 0;
    st->store_scroll = 0;
    st->store_max_scroll = 0;
    st->store_installed_only = 0;
    st->store_query[0] = '\0';
}

void nature_store_render(NatureOSState *st, cairo_t *cr, OSWindow *w, int cx, int cy, int cw, int ch) {
    (void)w;
    NaturePalette pal;
    nature_get_palette(st, &pal);
    int dark = pal.dark_mode;
    double ar = pal.ar, ag = pal.ag, ab = pal.ab;

    double fg_r = pal.fg_r, fg_g = pal.fg_g, fg_b = pal.fg_b;
    double mut_r = pal.mut_r, mut_g = pal.mut_g, mut_b = pal.mut_b;
    int compact = cw < 560;

    ngfx_fill_rect(cr, cx, cy, cw, ch, 0, pal.win_bg_r, pal.win_bg_g, pal.win_bg_b, 1.0);

    /* Header + Category Tabs */
    ngfx_fill_rect(cr, cx, cy, cw, 44, 0, pal.sidebar_r, pal.sidebar_g, pal.sidebar_b, 1.0);
    g_ngfx.set_source_rgba(cr, pal.border_r, pal.border_g, pal.border_b, pal.border_a);
    g_ngfx.move_to(cr, cx, cy + 44.5);
    g_ngfx.line_to(cr, cx + cw, cy + 44.5);
    g_ngfx.stroke(cr);

    ngfx_text(cr, "Plus Jakarta Sans", 1, compact ? 12.5 : 13.5, cx + 14, cy + 27, fg_r, fg_g, fg_b, 1.0,
              compact ? "APT Store" : "Ubuntu Software");

    int inst_count = 0;
    for (int i = 0; i < st->pkg_count; ++i) if (st->pkgs[i].installed) inst_count++;

    int tx = cx + (compact ? 94 : 156);
    int max_tx = cx + cw - (compact ? 12 : 162);
    for (int i = 0; i < 5; ++i) {
        int act = st->store_cat == i;
        int hov = (st->hover_hit_action == HIT_STORE_CAT && st->hover_hit_arg0 == i);
        const char *lbl = (compact && i == 1) ? "Dev" : (compact && i == 3) ? "Prod" : STORE_CATS[i];
        int tw = (int)ngfx_text_width(cr, "Noto Sans", act, compact ? 10.5 : 11.2, lbl) + (compact ? 14 : 18);
        if (tx + tw > max_tx) break;
        if (act) {
            ngfx_fill_gradient(cr, tx, cy + 9, tw, 26, 7, ar, ag, ab, 0.95,
                               ar * 0.78, ag * 0.78, ab * 0.78, 0.95, 0);
            ngfx_top_specular(cr, tx, cy + 9, tw, 7, 0.22);
        } else {
            ngfx_fill_rect(cr, tx, cy + 9, tw, 26, 7,
                           hov ? pal.hover_r : pal.elev_r, hov ? pal.hover_g : pal.elev_g, hov ? pal.hover_b : pal.elev_b, 0.94);
            if (hov) ngfx_stroke_rect(cr, tx + 0.5, cy + 9.5, tw - 1, 25, 7, 0.8, ar, ag, ab, 0.45);
        }
        ngfx_text(cr, "Noto Sans", act, compact ? 10.5 : 11.2, tx + (compact ? 7 : 9), cy + 26,
                  act ? pal.on_acc_r : fg_r, act ? pal.on_acc_g : fg_g, act ? pal.on_acc_b : fg_b, 1.0, lbl);
        nature_add_hit(st, tx, cy + 9, tw, 26, HIT_STORE_CAT, i, 0);
        tx += tw + 5;
    }

    if (!compact) {
        int sx_chip = cx + cw - 158;
        int sh_sync = (st->hover_hit_action == HIT_STORE_REFRESH);
        ngfx_fill_rect(cr, sx_chip, cy + 9, 48, 26, 7,
                       sh_sync ? pal.hover_r : pal.elev_r, sh_sync ? pal.hover_g : pal.elev_g, sh_sync ? pal.hover_b : pal.elev_b, 0.92);
        ngfx_stroke_rect(cr, sx_chip + 0.5, cy + 9.5, 47, 25, 7, 0.8, pal.border_r, pal.border_g, pal.border_b, pal.border_a);
        ngfx_text(cr, "Noto Sans", 1, 9.8, sx_chip + 10, cy + 25.5,
                  st->store_sync_count > 0 ? (dark ? pal.ar_hi : ar) : fg_r,
                  st->store_sync_count > 0 ? (dark ? pal.ag_hi : ag) : fg_g,
                  st->store_sync_count > 0 ? (dark ? pal.ab_hi : ab) : fg_b, 0.96, "Sync");
        nature_add_hit(st, sx_chip, cy + 9, 48, 26, HIT_STORE_REFRESH, 0, 0);

        char icount[32];
        std::snprintf(icount, sizeof(icount), "%d Installed", inst_count);
        int ix_chip = cx + cw - 104;
        int act_f = st->store_installed_only;
        ngfx_fill_rect(cr, ix_chip, cy + 9, 92, 26, 7,
                       act_f ? ar : pal.elev_r, act_f ? ag : pal.elev_g, act_f ? ab : pal.elev_b,
                       act_f ? 0.94 : 0.90);
        if (!act_f) {
            ngfx_stroke_rect(cr, ix_chip + 0.5, cy + 9.5, 91, 25, 7, 0.8, pal.border_r, pal.border_g, pal.border_b, pal.border_a);
        }
        ngfx_text(cr, "JetBrains Mono", 1, 9.8, ix_chip + 9, cy + 25.5,
                  act_f ? pal.on_acc_r : mut_r, act_f ? pal.on_acc_g : mut_g, act_f ? pal.on_acc_b : mut_b, 0.98, icount);
        nature_add_hit(st, ix_chip, cy + 9, 92, 26, HIT_STORE_INSTALLED_FILTER, 0, 0);
    }

    int px = cx + 14, py = cy + 50, pw = cw - 34;

    /* Interactive Search Box (target_id = 8) */
    int s_focused = (st->active_text_target == 8);
    ngfx_fill_rect(cr, px, py, pw, 26, 7, pal.card_r, pal.card_g, pal.card_b, 1.0);
    ngfx_stroke_rect(cr, px + 0.5, py + 0.5, pw - 1, 25, 7, s_focused ? 1.2 : 0.9,
                     s_focused ? ar : pal.border_r, s_focused ? ag : pal.border_g, s_focused ? ab : pal.border_b,
                     s_focused ? 0.85 : pal.border_a);
    if (st->store_query[0]) {
        ngfx_text_clipped(cr, "Noto Sans", 1, 11.2, px + 10, py + 17, pw - 40, fg_r, fg_g, fg_b, 1.0, st->store_query);
        ngfx_fill_rect(cr, px + pw - 26, py + 4, 20, 18, 5, pal.elev_r, pal.elev_g, pal.elev_b, 0.95);
        ngfx_text(cr, "Noto Sans", 1, 10.2, px + pw - 19, py + 17, fg_r, fg_g, fg_b, 0.9, "x");
        nature_add_hit(st, px + pw - 28, py + 2, 24, 22, HIT_STORE_SEARCH_CLEAR, 0, 0);
        nature_add_hit(st, px, py, pw - 30, 26, HIT_STORE_SEARCH_BOX, 0, 0);
    } else {
        ngfx_text_clipped(cr, "Noto Sans", 0, 11.0, px + 10, py + 17, pw - 20, mut_r, mut_g, mut_b, 0.82,
                          s_focused ? "Type package name or category..." : "Search Ubuntu Noble APT packages...");
        nature_add_hit(st, px, py, pw, 26, HIT_STORE_SEARCH_BOX, 0, 0);
    }
    nature_add_text_rect(st, px, py, pw, 26, 8);
    py += 34;

    /* Featured Highlight Banner */
    if (st->store_cat == 0 && !st->store_query[0] && ch > 320) {
        ngfx_shadow_rect(cr, px, py, pw, 54, 12, 1.6, 0.22);
        ngfx_fill_gradient(cr, px, py, pw, 54, 12,
                           ar * 0.48, ag * 0.48, ab * 0.48, 0.96,
                           pal.sidebar_r, pal.sidebar_g, pal.sidebar_b, 0.96, 1);

        g_ngfx.save(cr);
        ngfx_rounded_path(cr, px, py, pw, 54, 12);
        g_ngfx.clip(cr);
        g_ngfx.set_source_rgba(cr, 1.0, 1.0, 1.0, 0.06);
        g_ngfx.arc(cr, px + pw - 28, py + 27, 38, 0, 6.28318);
        g_ngfx.fill(cr);
        g_ngfx.restore(cr);

        ngfx_stroke_rect(cr, px + 0.5, py + 0.5, pw - 1, 53, 12, 1.0, ar, ag, ab, 0.48);
        ngfx_top_specular(cr, px, py, pw, 12, 0.18);
        ngfx_text_clipped(cr, "Plus Jakarta Sans", 1, compact ? 12.0 : 13.5, px + 16, py + 22, pw - 32, 1, 1, 1, 1,
                          "Featured: Ubuntu 24.04 Noble Native Stack");
        ngfx_text_clipped(cr, "Noto Sans", 0, 10.4, px + 16, py + 40, pw - 32, 0.88, 0.95, 0.92, 0.9,
                          st->store_sync_count > 0
                              ? "APT index synced (noble-main · 1,412 verified packages)"
                              : "Verified native C/C++ & Wayland packages from archive.ubuntu.com");
        py += 62;
    }

    /* Filter packages by category and search query */
    int filtered[MAX_APT_PKGS];
    int fcount = 0;
    for (int i = 0; i < st->pkg_count; ++i) {
        int cat_ok = (st->store_cat == 0 || std::strcmp(st->pkgs[i].category, STORE_CATS[st->store_cat]) == 0);
        int inst_ok = (!st->store_installed_only || st->pkgs[i].installed);
        int q_ok = (!st->store_query[0] ||
                    str_ci_match(st->pkgs[i].name, st->store_query) ||
                    str_ci_match(st->pkgs[i].display_name, st->store_query) ||
                    str_ci_match(st->pkgs[i].summary, st->store_query) ||
                    str_ci_match(st->pkgs[i].category, st->store_query));
        if (cat_ok && inst_ok && q_ok) {
            filtered[fcount++] = i;
        }
    }

    int has_inspector = (st->store_selected >= 0 && st->store_selected < st->pkg_count && ch >= 300);
    int insp_h = has_inspector ? 48 : 0;

    int cols = (cw >= 560) ? 2 : 1;
    int col_w = (cols == 2) ? (pw - 12) / 2 : pw;
    int row_h = 78;
    int total_rows = (fcount + cols - 1) / cols;
    int avail_h = (cy + ch - insp_h) - py - 6;
    if (avail_h < 40) avail_h = 40;
    int vis_rows = std::max(1, avail_h / row_h);
    st->store_max_scroll = std::max(0, total_rows - vis_rows);
    if (st->store_scroll > st->store_max_scroll) st->store_scroll = st->store_max_scroll;
    if (st->store_scroll < 0) st->store_scroll = 0;

    if (fcount == 0) {
        ngfx_text(cr, "Plus Jakarta Sans", 1, 13.0, px + pw * 0.5 - 70, py + avail_h * 0.45, mut_r, mut_g, mut_b, 0.78,
                  "No matching APT packages");
    }

    int start_idx = st->store_scroll * cols;
    for (int slot = 0; start_idx + slot < fcount; ++slot) {
        int row = slot / cols;
        int col = slot % cols;
        int by = py + row * row_h;
        if (by + 70 > cy + ch - insp_h - 4) break;

        int i = filtered[start_idx + slot];
        const AptPkg &p = st->pkgs[i];
        int bx = px + col * (col_w + 12);

        int is_sel = (st->store_selected == i);
        int is_hov = ((st->hover_hit_action == HIT_STORE_SELECT_PKG || st->hover_hit_action == HIT_STORE_TOGGLE_PKG) && st->hover_hit_arg0 == i);
        int btn_hov = (st->hover_hit_action == HIT_STORE_TOGGLE_PKG && st->hover_hit_arg0 == i);

        ngfx_fill_rect(cr, bx, by, col_w, 70, 11,
                       is_sel ? pal.elev_r : (is_hov ? pal.hover_r : pal.card_r),
                       is_sel ? pal.elev_g : (is_hov ? pal.hover_g : pal.card_g),
                       is_sel ? pal.elev_b : (is_hov ? pal.hover_b : pal.card_b), 0.96);
        ngfx_stroke_rect(cr, bx + 0.5, by + 0.5, col_w - 1, 69, 11, is_sel ? 1.2 : 0.9,
                         (is_sel || is_hov) ? ar : pal.border_r, (is_sel || is_hov) ? ag : pal.border_g, (is_sel || is_hov) ? ab : pal.border_b,
                         is_sel ? 0.78 : (is_hov ? 0.44 : pal.border_a));

        double er = ar, eg = ag, eb = ab;
        if (std::strcmp(p.category, "Development") == 0) { er = 0.20; eg = 0.56; eb = 0.92; }
        else if (std::strcmp(p.category, "Creative") == 0) { er = 0.86; eg = 0.44; eb = 0.24; }
        else if (std::strcmp(p.category, "Productivity") == 0) { er = 0.26; eg = 0.72; eb = 0.52; }
        else { er = 0.44; eg = 0.54; eb = 0.72; }

        double ea = p.installed ? 0.96 : 0.42;
        ngfx_fill_gradient(cr, bx + 12, by + 15, 40, 40, 10, er, eg, eb, ea, er * 0.65, eg * 0.65, eb * 0.65, ea, 0);
        ngfx_top_specular(cr, bx + 12, by + 15, 40, 10, p.installed ? 0.24 : 0.12);
        char cap[2] = {(char)(p.display_name[0]), '\0'};
        ngfx_text(cr, "Plus Jakarta Sans", 1, 16.0, bx + 26, by + 41, 1, 1, 1, 1, cap);

        if (p.installed) {
            ngfx_fill_circle(cr, bx + 49, by + 52, 7.0, pal.success_r, pal.success_g, pal.success_b, 1.0);
            g_ngfx.set_source_rgba(cr, 1.0, 1.0, 1.0, 0.98);
            g_ngfx.set_line_width(cr, 1.6);
            g_ngfx.set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
            g_ngfx.set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
            g_ngfx.move_to(cr, bx + 45.8, by + 52.2);
            g_ngfx.line_to(cr, bx + 48.2, by + 54.6);
            g_ngfx.line_to(cr, bx + 52.4, by + 49.6);
            g_ngfx.stroke(cr);
        }

        ngfx_text_clipped(cr, "Plus Jakarta Sans", 1, 12.0, bx + 62, by + 24, col_w - 150, fg_r, fg_g, fg_b, 1.0, p.display_name);

        char meta[96];
        std::snprintf(meta, sizeof(meta), "%s · %.1f MiB · %s", p.name, p.download_mb_x10 / 10.0, p.category);
        ngfx_text_clipped(cr, "JetBrains Mono", 0, 9.6, bx + 62, by + 41, col_w - 150, mut_r, mut_g, mut_b, 0.95, meta);
        ngfx_text_clipped(cr, "Noto Sans", 0, 10.0, bx + 62, by + 57, col_w - 150, mut_r, mut_g, mut_b, 0.88, p.summary);

        nature_add_hit(st, bx, by, col_w - 92, 70, HIT_STORE_SELECT_PKG, i, 0);

        int btn_x = bx + col_w - 80;
        if (p.installed) {
            ngfx_fill_rect(cr, btn_x, by + 21, 68, 28, 7, pal.danger_r, pal.danger_g, pal.danger_b, btn_hov ? 0.30 : 0.16);
            ngfx_stroke_rect(cr, btn_x + 0.5, by + 21.5, 67, 27, 7, 0.8, pal.danger_r, pal.danger_g, pal.danger_b, btn_hov ? 0.72 : 0.35);
            ngfx_text(cr, "Noto Sans", 1, 10.8, btn_x + 11, by + 39, pal.danger_r, pal.danger_g, pal.danger_b, 1.0, "Remove");
        } else {
            ngfx_fill_gradient(cr, btn_x, by + 21, 68, 28, 7,
                               ar * (btn_hov ? 1.12 : 1.0), ag * (btn_hov ? 1.12 : 1.0), ab * (btn_hov ? 1.12 : 1.0), 0.96,
                               ar * 0.78, ag * 0.78, ab * 0.78, 0.96, 0);
            ngfx_top_specular(cr, btn_x, by + 21, 68, 7, 0.22);
            ngfx_text(cr, "Noto Sans", 1, 10.8, btn_x + 15, by + 39, pal.on_acc_r, pal.on_acc_g, pal.on_acc_b, 1.0, "Install");
        }
        nature_add_hit(st, btn_x, by + 21, 68, 28, HIT_STORE_TOGGLE_PKG, i, 0);
    }

    nature_draw_scrollbar(st, cr, cx + cw - 11, py, avail_h,
                          total_rows, vis_rows, st->store_scroll,
                          st->store_max_scroll, SCROLL_STORE, dark);

    /* Bottom Selected Package Inspector Strip */
    if (has_inspector) {
        const AptPkg &sp = st->pkgs[st->store_selected];
        int iy = cy + ch - insp_h;
        ngfx_fill_rect(cr, cx, iy, cw, insp_h, 0, pal.sidebar_r, pal.sidebar_g, pal.sidebar_b, 1.0);
        g_ngfx.set_source_rgba(cr, pal.border_r, pal.border_g, pal.border_b, pal.border_a);
        g_ngfx.move_to(cr, cx, iy + 0.5);
        g_ngfx.line_to(cr, cx + cw, iy + 0.5);
        g_ngfx.stroke(cr);

        int left_pad = sp.installed ? 216 : 132;
        ngfx_text_clipped(cr, "Plus Jakarta Sans", 1, 11.6, cx + 14, iy + 19, cw - left_pad, fg_r, fg_g, fg_b, 1.0, sp.display_name);
        char sinfo[128];
        std::snprintf(sinfo, sizeof(sinfo), "v%.20s · %.24s · RAM %d MiB · %s",
                      sp.version, sp.source, sp.ram_mb, sp.installed ? "Installed" : "Available");
        ngfx_text_clipped(cr, "JetBrains Mono", 0, 9.8, cx + 14, iy + 36, cw - left_pad, mut_r, mut_g, mut_b, 0.95, sinfo);

        int bx = cx + cw - 96;
        int lhov = (st->hover_hit_action == HIT_STORE_LAUNCH_PKG && st->hover_hit_arg0 == st->store_selected);
        int thov = (st->hover_hit_action == HIT_STORE_TOGGLE_PKG && st->hover_hit_arg0 == st->store_selected);
        if (sp.installed) {
            int lx = bx - 84;
            ngfx_fill_gradient(cr, lx, iy + 10, 76, 28, 7,
                               ar * (lhov ? 1.12 : 1.0), ag * (lhov ? 1.12 : 1.0), ab * (lhov ? 1.12 : 1.0), 0.96,
                               ar * 0.78, ag * 0.78, ab * 0.78, 0.96, 0);
            ngfx_top_specular(cr, lx, iy + 10, 76, 7, 0.22);
            ngfx_text(cr, "Noto Sans", 1, 10.6, lx + 18, iy + 28, pal.on_acc_r, pal.on_acc_g, pal.on_acc_b, 1.0, "Launch");
            nature_add_hit(st, lx, iy + 10, 76, 28, HIT_STORE_LAUNCH_PKG, st->store_selected, 0);

            ngfx_fill_rect(cr, bx, iy + 10, 82, 28, 7, pal.danger_r, pal.danger_g, pal.danger_b, thov ? 0.32 : 0.18);
            if (thov) ngfx_stroke_rect(cr, bx + 0.5, iy + 10.5, 81, 27, 7, 0.8, pal.danger_r, pal.danger_g, pal.danger_b, 0.65);
            ngfx_text(cr, "Noto Sans", 1, 10.6, bx + 18, iy + 28, pal.danger_r, pal.danger_g, pal.danger_b, 1.0, "Remove");
        } else {
            ngfx_fill_rect(cr, bx, iy + 10, 82, 28, 7,
                           ar * (thov ? 1.12 : 1.0), ag * (thov ? 1.12 : 1.0), ab * (thov ? 1.12 : 1.0), 0.95);
            ngfx_text(cr, "Noto Sans", 1, 10.6, bx + 22, iy + 28, pal.on_acc_r, pal.on_acc_g, pal.on_acc_b, 1.0, "Install");
        }
        nature_add_hit(st, bx, iy + 10, 82, 28, HIT_STORE_TOGGLE_PKG, st->store_selected, 0);
    }
}

void nature_store_wheel(NatureOSState *st, int delta_y) {
    int step = (delta_y > 0) ? 1 : -1;
    st->store_scroll += step;
    if (st->store_scroll < 0) st->store_scroll = 0;
    if (st->store_scroll > st->store_max_scroll) st->store_scroll = st->store_max_scroll;
}

void nature_store_key(NatureOSState *st, const char *key) {
    if (!key || !*key) return;
    if (std::strcmp(key, "Space") == 0) key = " ";
    if (std::strcmp(key, "Backspace") == 0) {
        size_t len = std::strlen(st->store_query);
        if (len > 0) st->store_query[len - 1] = '\0';
        st->store_scroll = 0;
    } else if (std::strcmp(key, "Enter") == 0) {
        st->active_text_target = 0;
    } else if (std::strlen(key) == 1 && (unsigned char)key[0] >= 32 && (unsigned char)key[0] < 127) {
        size_t len = std::strlen(st->store_query);
        if (len + 1 < sizeof(st->store_query)) {
            st->store_query[len] = key[0];
            st->store_query[len + 1] = '\0';
            st->store_scroll = 0;
        }
    }
}

void nature_store_click(NatureOSState *st, const HitRect *hit) {
    if (hit->action_id == HIT_STORE_CAT) {
        st->store_cat = hit->arg0;
        st->store_scroll = 0;
    } else if (hit->action_id == HIT_STORE_SEARCH_BOX) {
        st->active_text_target = 8;
        st->input_focus_pulse = 1;
    } else if (hit->action_id == HIT_STORE_SEARCH_CLEAR) {
        st->store_query[0] = '\0';
        st->store_scroll = 0;
    } else if (hit->action_id == HIT_STORE_SELECT_PKG) {
        st->store_selected = hit->arg0;
    } else if (hit->action_id == HIT_STORE_INSTALLED_FILTER) {
        st->store_installed_only = !st->store_installed_only;
        st->store_scroll = 0;
    } else if (hit->action_id == HIT_STORE_REFRESH) {
        st->store_sync_count++;
        nature_notify(st, "Ubuntu Software", "APT repository metadata refreshed (noble-main · 1,412 packages).");
    } else if (hit->action_id == HIT_STORE_LAUNCH_PKG) {
        int idx = hit->arg0;
        if (idx >= 0 && idx < st->pkg_count && st->pkgs[idx].installed) {
            const char *pname = st->pkgs[idx].name;
            if (std::strcmp(pname, "neofetch") == 0 || std::strcmp(pname, "htop") == 0) {
                nature_open_app(st, APP_TERMINAL);
                nature_terminal_exec(st, pname);
            } else if (std::strcmp(pname, "nodejs") == 0) {
                nature_open_app(st, APP_TERMINAL);
                nature_terminal_exec(st, "node -v");
            } else if (std::strcmp(pname, "npm") == 0) {
                nature_open_app(st, APP_TERMINAL);
                nature_terminal_exec(st, "npm -v");
            } else if (std::strcmp(pname, "python3") == 0) {
                nature_open_app(st, APP_TERMINAL);
                nature_terminal_exec(st, "python3 -V");
            } else if (std::strcmp(pname, "git") == 0) {
                nature_open_app(st, APP_TERMINAL);
                nature_terminal_exec(st, "git status");
            } else if (std::strcmp(pname, "btop") == 0) {
                nature_open_app(st, APP_MONITOR);
            } else if (std::strcmp(pname, "vlc") == 0) {
                st->media_mode = 1;
                st->video_playing = 1;
                st->video_progress_ms = 0;
                nature_open_app(st, APP_MEDIA);
            } else if (std::strcmp(pname, "gimp") == 0 || std::strcmp(pname, "inkscape") == 0) {
                st->media_mode = 0;
                nature_open_app(st, APP_MEDIA);
            } else if (std::strcmp(pname, "firefox") == 0) {
                nature_open_app(st, APP_BROWSER);
            } else if (std::strcmp(pname, "code") == 0) {
                std::snprintf(st->files_cwd, sizeof(st->files_cwd), "/home/nature/Projects");
                nature_open_app(st, APP_FILES);
            } else {
                nature_open_app(st, APP_TERMINAL);
            }
        }
    } else if (hit->action_id == HIT_STORE_TOGGLE_PKG) {
        int idx = hit->arg0;
        if (idx >= 0 && idx < st->pkg_count) {
            st->pkgs[idx].installed = !st->pkgs[idx].installed;
            char msg[128];
            std::snprintf(msg, sizeof(msg), "%s %s via apt-get.",
                          st->pkgs[idx].display_name,
                          st->pkgs[idx].installed ? "installed" : "removed");
            nature_notify(st, "Ubuntu Software", msg);
            nature_os_save_state(st);
        }
    }
}

} // extern "C"
