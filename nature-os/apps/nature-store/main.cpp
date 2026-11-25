#include "nature_os.h"
#include <algorithm>
#include <cstdio>
#include <cstring>

static const char *STORE_CATS[5] = {"All", "Development", "Creative", "Productivity", "Utilities"};

extern "C" {

void nature_store_init(NatureOSState *st) {
    const AptPkg initial[8] = {
        {"neofetch", "Neofetch System Info", "Utilities", "7.1.0-4ubuntu1", "apt (noble-universe)",
         "CLI system summary utility for Ubuntu/Nature OS.", 4, 4, 1, 100},
        {"htop", "htop Process Viewer", "Utilities", "3.3.0-4build1", "apt (noble-main)",
         "Interactive ncurses process & CPU viewer.", 6, 6, 1, 100},
        {"code", "Visual Studio Code", "Development", "1.95.2-noble", "apt (noble-universe)",
         "Native Wayland code editor with Git & C/C++ tooling.", 964, 240, 0, 0},
        {"gimp", "GNU Image Editor", "Creative", "2.10.36-3ubuntu1", "apt (noble-universe)",
         "Open-source raster graphics & photo retouching suite.", 382, 160, 0, 0},
        {"vlc", "VLC Media Player", "Creative", "3.0.20-3build6", "apt (noble-universe)",
         "VA-API hardware-accelerated multimedia player.", 228, 70, 1, 100},
        {"blender", "Blender 3D Studio", "Creative", "4.0.2+dfsg-1", "apt (noble-universe)",
         "3D modeling & Vulkan/Wayland rendering pipeline.", 1420, 410, 0, 0},
        {"libreoffice", "LibreOffice Writer", "Productivity", "24.2.5-noble", "apt (noble-main)",
         "GTK4-integrated open-source document processor.", 645, 148, 0, 0},
        {"docker.io", "Docker Engine", "Development", "24.0.7-ubuntu4", "apt (noble-universe)",
         "Linux container daemon with cgroup v2 isolation.", 441, 68, 0, 0},
    };
    st->pkg_count = 8;
    for (int i = 0; i < 8; ++i) st->pkgs[i] = initial[i];
    st->store_scroll = 0;
    st->store_max_scroll = 0;
}

void nature_store_render(NatureOSState *st, cairo_t *cr, OSWindow *w, int cx, int cy, int cw, int ch) {
    (void)w;
    int dark = st->cfg.dark_mode;
    double ar, ag, ab;
    nature_get_accent(st->cfg.accent_idx, &ar, &ag, &ab);

    double fg_r = dark ? 0.92 : 0.10, fg_g = dark ? 0.95 : 0.13, fg_b = dark ? 0.93 : 0.11;
    double mut_r = dark ? 0.62 : 0.44, mut_g = dark ? 0.70 : 0.50, mut_b = dark ? 0.66 : 0.47;
    double brd_c = dark ? 1.0 : 0.0;
    int compact = cw < 560;

    ngfx_fill_rect(cr, cx, cy, cw, ch, 0,
                   dark ? 0.050 : 0.97, dark ? 0.074 : 0.98, dark ? 0.066 : 0.975, 1.0);

    /* Header + Category Tabs */
    ngfx_fill_rect(cr, cx, cy, cw, 44, 0,
                   dark ? 0.038 : 0.93, dark ? 0.058 : 0.95, dark ? 0.052 : 0.94, 1.0);
    g_ngfx.set_source_rgba(cr, brd_c, brd_c, brd_c, dark ? 0.07 : 0.08);
    g_ngfx.move_to(cr, cx, cy + 44.5);
    g_ngfx.line_to(cr, cx + cw, cy + 44.5);
    g_ngfx.stroke(cr);

    ngfx_text(cr, "Plus Jakarta Sans", 1, compact ? 12.5 : 13.5, cx + 14, cy + 27, fg_r, fg_g, fg_b, 1.0,
              compact ? "APT Store" : "Ubuntu Software");

    int inst_count = 0;
    for (int i = 0; i < st->pkg_count; ++i) if (st->pkgs[i].installed) inst_count++;

    int tx = cx + (compact ? 94 : 156);
    int max_tx = cx + cw - (compact ? 12 : 104);
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
            ngfx_fill_rect(cr, tx, cy + 9, tw, 26, 7, brd_c, brd_c, brd_c, hov ? 0.12 : 0.06);
            if (hov) ngfx_stroke_rect(cr, tx + 0.5, cy + 9.5, tw - 1, 25, 7, 0.8, ar, ag, ab, 0.40);
        }
        ngfx_text(cr, "Noto Sans", act, compact ? 10.5 : 11.2, tx + (compact ? 7 : 9), cy + 26,
                  act ? 1.0 : fg_r, act ? 1.0 : fg_g, act ? 1.0 : fg_b, 1.0, lbl);
        nature_add_hit(st, tx, cy + 9, tw, 26, HIT_STORE_CAT, i, 0);
        tx += tw + 5;
    }

    if (!compact) {
        char icount[32];
        std::snprintf(icount, sizeof(icount), "%d Installed", inst_count);
        ngfx_text(cr, "JetBrains Mono", 0, 10.5, cx + cw - 98, cy + 26, mut_r, mut_g, mut_b, 0.95, icount);
    }

    int px = cx + 14, py = cy + 54, pw = cw - 34;

    /* Featured Highlight Banner */
    if (st->store_cat == 0 && ch > 280) {
        ngfx_shadow_rect(cr, px, py, pw, 60, 12, 1.6, 0.22);
        ngfx_fill_gradient(cr, px, py, pw, 60, 12,
                           ar * 0.48, ag * 0.48, ab * 0.48, 0.96,
                           dark ? 0.06 : 0.14, dark ? 0.12 : 0.22, dark ? 0.10 : 0.18, 0.96, 1);

        g_ngfx.save(cr);
        ngfx_rounded_path(cr, px, py, pw, 60, 12);
        g_ngfx.clip(cr);
        g_ngfx.set_source_rgba(cr, 1.0, 1.0, 1.0, 0.06);
        g_ngfx.arc(cr, px + pw - 28, py + 30, 42, 0, 6.28318);
        g_ngfx.fill(cr);
        g_ngfx.set_source_rgba(cr, 1.0, 1.0, 1.0, 0.09);
        g_ngfx.set_line_width(cr, 1.1);
        g_ngfx.arc(cr, px + pw - 28, py + 30, 58, 0, 6.28318);
        g_ngfx.stroke(cr);
        g_ngfx.restore(cr);

        ngfx_stroke_rect(cr, px + 0.5, py + 0.5, pw - 1, 59, 12, 1.0, ar, ag, ab, 0.48);
        ngfx_top_specular(cr, px, py, pw, 12, 0.18);
        ngfx_text_clipped(cr, "Plus Jakarta Sans", 1, compact ? 12.5 : 14.0, px + 16, py + 25, pw - 32, 1, 1, 1, 1,
                          "Featured: Ubuntu 24.04 Noble Native Stack");
        ngfx_text_clipped(cr, "Noto Sans", 0, 10.8, px + 16, py + 44, pw - 32, 0.88, 0.95, 0.92, 0.9,
                          "Verified native C/C++ & Wayland packages from archive.ubuntu.com");
        py += 70;
    }

    /* Filter packages */
    int filtered[MAX_APT_PKGS];
    int fcount = 0;
    for (int i = 0; i < st->pkg_count; ++i) {
        if (st->store_cat == 0 || std::strcmp(st->pkgs[i].category, STORE_CATS[st->store_cat]) == 0) {
            filtered[fcount++] = i;
        }
    }

    int cols = (cw >= 560) ? 2 : 1;
    int col_w = (cols == 2) ? (pw - 12) / 2 : pw;
    int row_h = 80;
    int total_rows = (fcount + cols - 1) / cols;
    int avail_h = (cy + ch) - py - 6;
    int vis_rows = std::max(1, avail_h / row_h);
    st->store_max_scroll = std::max(0, total_rows - vis_rows);
    if (st->store_scroll > st->store_max_scroll) st->store_scroll = st->store_max_scroll;
    if (st->store_scroll < 0) st->store_scroll = 0;

    int start_idx = st->store_scroll * cols;
    for (int slot = 0; start_idx + slot < fcount; ++slot) {
        int row = slot / cols;
        int col = slot % cols;
        int by = py + row * row_h;
        if (by + 72 > cy + ch - 4) break;

        int i = filtered[start_idx + slot];
        const AptPkg &p = st->pkgs[i];
        int bx = px + col * (col_w + 12);

        int is_sel = (st->store_selected == i);
        int is_hov = ((st->hover_hit_action == HIT_STORE_SELECT_PKG || st->hover_hit_action == HIT_STORE_TOGGLE_PKG) && st->hover_hit_arg0 == i);
        int btn_hov = (st->hover_hit_action == HIT_STORE_TOGGLE_PKG && st->hover_hit_arg0 == i);

        ngfx_fill_rect(cr, bx, by, col_w, 72, 11, brd_c, brd_c, brd_c, is_sel ? 0.095 : (is_hov ? 0.072 : 0.042));
        ngfx_stroke_rect(cr, bx + 0.5, by + 0.5, col_w - 1, 71, 11, is_sel ? 1.2 : 0.9,
                         (is_sel || is_hov) ? ar : brd_c, (is_sel || is_hov) ? ag : brd_c, (is_sel || is_hov) ? ab : brd_c,
                         is_sel ? 0.78 : (is_hov ? 0.44 : (dark ? 0.08 : 0.10)));

        /* Curated gradient package emblem */
        double er = ar, eg = ag, eb = ab;
        if (std::strcmp(p.category, "Development") == 0) { er = 0.20; eg = 0.56; eb = 0.92; }
        else if (std::strcmp(p.category, "Creative") == 0) { er = 0.86; eg = 0.44; eb = 0.24; }
        else if (std::strcmp(p.category, "Productivity") == 0) { er = 0.26; eg = 0.72; eb = 0.52; }
        else { er = 0.44; eg = 0.54; eb = 0.72; }

        double ea = p.installed ? 0.96 : 0.42;
        ngfx_fill_gradient(cr, bx + 12, by + 16, 40, 40, 10,
                           er, eg, eb, ea,
                           er * 0.65, eg * 0.65, eb * 0.65, ea, 0);
        ngfx_top_specular(cr, bx + 12, by + 16, 40, 10, p.installed ? 0.24 : 0.12);
        char cap[2] = {(char)(p.display_name[0]), '\0'};
        ngfx_text(cr, "Plus Jakarta Sans", 1, 16.0, bx + 26, by + 42, 1, 1, 1, 1, cap);

        /* Installed checkmark badge on emblem corner */
        if (p.installed) {
            ngfx_fill_circle(cr, bx + 49, by + 53, 7.0, 0.18, 0.78, 0.48, 1.0);
            g_ngfx.set_source_rgba(cr, 1.0, 1.0, 1.0, 0.98);
            g_ngfx.set_line_width(cr, 1.6);
            g_ngfx.set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
            g_ngfx.set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
            g_ngfx.move_to(cr, bx + 45.8, by + 53.2);
            g_ngfx.line_to(cr, bx + 48.2, by + 55.6);
            g_ngfx.line_to(cr, bx + 52.4, by + 50.6);
            g_ngfx.stroke(cr);
        }

        ngfx_text_clipped(cr, "Plus Jakarta Sans", 1, 12.2, bx + 62, by + 25, col_w - 150, fg_r, fg_g, fg_b, 1.0, p.display_name);

        char meta[96];
        std::snprintf(meta, sizeof(meta), "%s · %.1f MiB · %s", p.name, p.download_mb_x10 / 10.0, p.category);
        ngfx_text_clipped(cr, "JetBrains Mono", 0, 9.8, bx + 62, by + 42, col_w - 150, mut_r, mut_g, mut_b, 0.95, meta);
        ngfx_text_clipped(cr, "Noto Sans", 0, 10.2, bx + 62, by + 58, col_w - 150, mut_r, mut_g, mut_b, 0.88, p.summary);

        nature_add_hit(st, bx, by, col_w - 92, 72, HIT_STORE_SELECT_PKG, i, 0);

        int btn_x = bx + col_w - 80;
        if (p.installed) {
            ngfx_fill_rect(cr, btn_x, by + 22, 68, 28, 7, 0.88, 0.28, 0.28, btn_hov ? 0.30 : 0.16);
            ngfx_stroke_rect(cr, btn_x + 0.5, by + 22.5, 67, 27, 7, 0.8, 0.92, 0.36, 0.36, btn_hov ? 0.72 : 0.35);
            ngfx_text(cr, "Noto Sans", 1, 10.8, btn_x + 11, by + 40, 0.96, 0.48, 0.48, 1.0, "Remove");
        } else {
            ngfx_fill_gradient(cr, btn_x, by + 22, 68, 28, 7,
                               ar * (btn_hov ? 1.12 : 1.0), ag * (btn_hov ? 1.12 : 1.0), ab * (btn_hov ? 1.12 : 1.0), 0.96,
                               ar * 0.78, ag * 0.78, ab * 0.78, 0.96, 0);
            ngfx_top_specular(cr, btn_x, by + 22, 68, 7, 0.22);
            ngfx_text(cr, "Noto Sans", 1, 10.8, btn_x + 15, by + 40, 1.0, 1.0, 1.0, 1.0, "Install");
        }
        nature_add_hit(st, btn_x, by + 22, 68, 28, HIT_STORE_TOGGLE_PKG, i, 0);
    }

    nature_draw_scrollbar(st, cr, cx + cw - 11, py, avail_h,
                          total_rows, vis_rows, st->store_scroll,
                          st->store_max_scroll, SCROLL_STORE, dark);
}

void nature_store_wheel(NatureOSState *st, int delta_y) {
    int step = (delta_y > 0) ? 1 : -1;
    st->store_scroll += step;
    if (st->store_scroll < 0) st->store_scroll = 0;
    if (st->store_scroll > st->store_max_scroll) st->store_scroll = st->store_max_scroll;
}

void nature_store_click(NatureOSState *st, const HitRect *hit) {
    if (hit->action_id == HIT_STORE_CAT) {
        st->store_cat = hit->arg0;
        st->store_scroll = 0;
    } else if (hit->action_id == HIT_STORE_SELECT_PKG) {
        st->store_selected = hit->arg0;
    } else if (hit->action_id == HIT_STORE_TOGGLE_PKG) {
        int idx = hit->arg0;
        if (idx >= 0 && idx < st->pkg_count) {
            st->pkgs[idx].installed = !st->pkgs[idx].installed;
            char msg[128];
            std::snprintf(msg, sizeof(msg), "%s %s via apt-get.",
                          st->pkgs[idx].display_name,
                          st->pkgs[idx].installed ? "installed" : "removed");
            nature_notify(st, "Ubuntu Software", msg);
        }
    }
}

} // extern "C"
