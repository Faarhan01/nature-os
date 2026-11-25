#include "nature_os.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NUM_PLACES 9

static const char *PLACES_PATHS[NUM_PLACES] = {
    "/home/nature",
    "/home/nature/Desktop",
    "/home/nature/Documents",
    "/home/nature/Downloads",
    "/home/nature/Pictures",
    "/home/nature/Music",
    "/home/nature/Projects",
    "/etc/nature-os",
    "/var/log"
};

static const char *PLACES_LABELS[NUM_PLACES] = {
    "Home",
    "Desktop",
    "Documents",
    "Downloads",
    "Pictures",
    "Music",
    "Projects",
    "System /etc",
    "Logs /var"
};

static int str_ci_match(const char *hay, const char *needle) {
    if (!needle || !*needle) return 1;
    size_t hlen = strlen(hay), nlen = strlen(needle);
    if (nlen > hlen) return 0;
    for (size_t i = 0; i <= hlen - nlen; ++i) {
        size_t j = 0;
        while (j < nlen && tolower((unsigned char)hay[i + j]) == tolower((unsigned char)needle[j])) j++;
        if (j == nlen) return 1;
    }
    return 0;
}

static void draw_place_icon(cairo_t *cr, int idx, double x, double y, double r, double g, double b, double a) {
    g_ngfx.set_source_rgba(cr, r, g, b, a);
    g_ngfx.set_line_width(cr, 1.4);
    switch (idx) {
        case 0: /* Home */
            g_ngfx.move_to(cr, x + 2, y + 8);
            g_ngfx.line_to(cr, x + 8, y + 2);
            g_ngfx.line_to(cr, x + 14, y + 8);
            g_ngfx.move_to(cr, x + 4, y + 7);
            g_ngfx.line_to(cr, x + 4, y + 13);
            g_ngfx.line_to(cr, x + 12, y + 13);
            g_ngfx.line_to(cr, x + 12, y + 7);
            g_ngfx.stroke(cr);
            break;
        case 1: /* Desktop */
            ngfx_stroke_rect(cr, x + 2, y + 2.5, 12, 8.5, 1.5, 1.3, r, g, b, a);
            g_ngfx.move_to(cr, x + 5, y + 13.5);
            g_ngfx.line_to(cr, x + 11, y + 13.5);
            g_ngfx.stroke(cr);
            break;
        case 3: /* Downloads */
            g_ngfx.move_to(cr, x + 8, y + 2.5);
            g_ngfx.line_to(cr, x + 8, y + 10);
            g_ngfx.move_to(cr, x + 5, y + 7.2);
            g_ngfx.line_to(cr, x + 8, y + 10.2);
            g_ngfx.line_to(cr, x + 11, y + 7.2);
            g_ngfx.move_to(cr, x + 3, y + 12.8);
            g_ngfx.line_to(cr, x + 13, y + 12.8);
            g_ngfx.stroke(cr);
            break;
        case 4: /* Pictures */
            ngfx_stroke_rect(cr, x + 2, y + 3, 12, 10, 2, 1.35, r, g, b, a);
            ngfx_fill_circle(cr, x + 5.8, y + 6.2, 1.3, r, g, b, a);
            g_ngfx.move_to(cr, x + 3.5, y + 12);
            g_ngfx.line_to(cr, x + 7.5, y + 8);
            g_ngfx.line_to(cr, x + 10, y + 10);
            g_ngfx.line_to(cr, x + 12.5, y + 8.5);
            g_ngfx.stroke(cr);
            break;
        case 5: /* Music */
            g_ngfx.arc(cr, x + 5, y + 11.5, 2.2, 0, 6.283);
            g_ngfx.fill(cr);
            g_ngfx.move_to(cr, x + 7, y + 11);
            g_ngfx.line_to(cr, x + 7, y + 3.5);
            g_ngfx.line_to(cr, x + 13, y + 5);
            g_ngfx.stroke(cr);
            break;
        case 6: /* Projects </> */
            g_ngfx.move_to(cr, x + 5.5, y + 4.5);
            g_ngfx.line_to(cr, x + 2.5, y + 8.0);
            g_ngfx.line_to(cr, x + 5.5, y + 11.5);
            g_ngfx.move_to(cr, x + 10.5, y + 4.5);
            g_ngfx.line_to(cr, x + 13.5, y + 8.0);
            g_ngfx.line_to(cr, x + 10.5, y + 11.5);
            g_ngfx.stroke(cr);
            break;
        case 7:
        case 8: /* Logs / System */
            ngfx_stroke_rect(cr, x + 2.5, y + 2.5, 11, 11, 2.5, 1.35, r, g, b, a);
            g_ngfx.move_to(cr, x + 5, y + 6);
            g_ngfx.line_to(cr, x + 11, y + 6);
            g_ngfx.move_to(cr, x + 5, y + 9.5);
            g_ngfx.line_to(cr, x + 9, y + 9.5);
            g_ngfx.stroke(cr);
            break;
        default: /* Folder */
            ngfx_fill_rect(cr, x + 2, y + 4, 12, 9, 2, r, g, b, a * 0.85);
            ngfx_fill_rect(cr, x + 2, y + 2.5, 6, 3, 1.5, r, g, b, a);
            break;
    }
}

static void draw_file_glyph(cairo_t *cr, NatureOSState *st, const VfsNode *n,
                            double x, double y, double w, double h,
                            double ar, double ag, double ab, int dark) {
    if (n->image_idx >= 0 && n->image_idx < MAX_MEDIA_IMAGES && st->media_images[n->image_idx]) {
        ngfx_shadow_rect(cr, x + 4, y + 2, w - 8, h - 4, 6, 2.0, 0.30);
        g_ngfx.save(cr);
        ngfx_rounded_path(cr, x + 4, y + 2, w - 8, h - 4, 6);
        g_ngfx.clip(cr);
        double sw = g_ngfx.image_surface_get_width(st->media_images[n->image_idx]);
        double sh = g_ngfx.image_surface_get_height(st->media_images[n->image_idx]);
        g_ngfx.translate(cr, x + 4, y + 2);
        g_ngfx.scale(cr, (w - 8) / sw, (h - 4) / sh);
        g_ngfx.set_source_surface(cr, st->media_images[n->image_idx], 0, 0);
        g_ngfx.paint(cr);
        g_ngfx.restore(cr);
        ngfx_stroke_rect(cr, x + 4, y + 2, w - 8, h - 4, 6, 1.0, 1, 1, 1, 0.20);
        return;
    }
    if (n->is_dir) {
        double fx = x + (w - 50) * 0.5;
        double fy = y + (h - 36) * 0.5;
        ngfx_shadow_rect(cr, fx, fy + 2, 50, 32, 6, 2.0, 0.25);
        ngfx_fill_rect(cr, fx, fy - 4, 22, 10, 3,
                       ar * 1.15 > 1 ? 1 : ar * 1.15, ag * 1.15 > 1 ? 1 : ag * 1.15, ab * 1.15 > 1 ? 1 : ab * 1.15, 0.96);
        ngfx_fill_gradient(cr, fx, fy + 2, 50, 32, 6,
                           ar * 1.15 > 1 ? 1 : ar * 1.15, ag * 1.15 > 1 ? 1 : ag * 1.15, ab * 1.15 > 1 ? 1 : ab * 1.15, 0.98,
                           ar * 0.62, ag * 0.62, ab * 0.62, 0.98, 0);
        ngfx_top_specular(cr, fx, fy + 2, 50, 6, 0.35);
        ngfx_stroke_rect(cr, fx + 0.5, fy + 2.5, 49, 31, 6, 1.0, 1, 1, 1, 0.20);
        return;
    }

    double dx = x + (w - 36) * 0.5;
    double dy = y + (h - 42) * 0.5;
    double br = 0.25, bg = 0.58, bb = 0.44;
    if (n->audio_idx >= 0) { br = 0.16; bg = 0.68; bb = 0.56; }
    else if (strcmp(n->mime_tag, "C") == 0 || strcmp(n->mime_tag, "CPP") == 0) { br = 0.22; bg = 0.54; bb = 0.90; }
    else if (strcmp(n->mime_tag, "DEB") == 0 || strcmp(n->mime_tag, "ISO") == 0) { br = 0.88; bg = 0.46; bb = 0.15; }
    else if (strcmp(n->mime_tag, "CONF") == 0 || strcmp(n->mime_tag, "LOG") == 0) { br = 0.62; bg = 0.44; bb = 0.84; }

    /* Classic document sheet with folded top-right dog-ear corner */
    ngfx_shadow_rect(cr, dx, dy, 36, 42, 5, 1.8, 0.22);
    g_ngfx.set_source_rgba(cr, dark ? 0.13 : 0.94, dark ? 0.18 : 0.97, dark ? 0.16 : 0.96, 1.0);
    g_ngfx.move_to(cr, dx + 5, dy);
    g_ngfx.line_to(cr, dx + 26, dy);
    g_ngfx.line_to(cr, dx + 36, dy + 10);
    g_ngfx.line_to(cr, dx + 36, dy + 37);
    g_ngfx.arc(cr, dx + 31, dy + 37, 5, 0, 1.5708);
    g_ngfx.arc(cr, dx + 5, dy + 37, 5, 1.5708, 3.14159);
    g_ngfx.arc(cr, dx + 5, dy + 5, 5, 3.14159, 4.71238);
    g_ngfx.close_path(cr);
    g_ngfx.fill_preserve(cr);
    g_ngfx.set_source_rgba(cr, br, bg, bb, 0.62);
    g_ngfx.set_line_width(cr, 1.0);
    g_ngfx.stroke(cr);

    /* Dog-ear fold flap */
    g_ngfx.set_source_rgba(cr, br, bg, bb, 0.35);
    g_ngfx.move_to(cr, dx + 26, dy);
    g_ngfx.line_to(cr, dx + 26, dy + 10);
    g_ngfx.line_to(cr, dx + 36, dy + 10);
    g_ngfx.close_path(cr);
    g_ngfx.fill(cr);

    /* Faint text lines */
    ngfx_fill_rect(cr, dx + 6, dy + 11, 16, 2, 1, dark ? 1 : 0, dark ? 1 : 0, dark ? 1 : 0, 0.18);
    ngfx_fill_rect(cr, dx + 6, dy + 16, 22, 2, 1, dark ? 1 : 0, dark ? 1 : 0, dark ? 1 : 0, 0.14);

    /* Color-coded MIME tag badge */
    ngfx_fill_rect(cr, dx + 4, dy + 23, 28, 14, 3.5, br, bg, bb, 0.92);
    const char *tag = n->mime_tag[0] ? n->mime_tag : "TXT";
    double tw = ngfx_text_width(cr, "JetBrains Mono", 1, 8.2, tag);
    ngfx_text(cr, "JetBrains Mono", 1, 8.2, dx + 18 - tw * 0.5, dy + 33, 1, 1, 1, 1, tag);
}

static void format_size(const VfsNode *n, char *out, size_t cap) {
    if (n->is_dir) snprintf(out, cap, "Folder");
    else if (n->size_bytes >= 1073741824) snprintf(out, cap, "%.2f GiB", n->size_bytes / 1073741824.0);
    else if (n->size_bytes >= 1048576) snprintf(out, cap, "%.1f MiB", n->size_bytes / 1048576.0);
    else if (n->size_bytes >= 1024) snprintf(out, cap, "%.1f KiB", n->size_bytes / 1024.0);
    else snprintf(out, cap, "%d B", n->size_bytes);
}

void nature_files_render(NatureOSState *st, cairo_t *cr, OSWindow *w, int cx, int cy, int cw, int ch) {
    (void)w;
    int dark = st->cfg.dark_mode;
    double ar, ag, ab;
    nature_get_accent(st->cfg.accent_idx, &ar, &ag, &ab);

    int compact = cw < 540;
    int side_w = compact ? 114 : 162;
    double fg_r = dark ? 0.92 : 0.10, fg_g = dark ? 0.95 : 0.14, fg_b = dark ? 0.93 : 0.12;
    double mut_r = dark ? 0.62 : 0.44, mut_g = dark ? 0.70 : 0.50, mut_b = dark ? 0.66 : 0.47;

    /* Sidebar */
    ngfx_fill_rect(cr, cx, cy, side_w, ch, 0,
                   dark ? 0.048 : 0.93, dark ? 0.072 : 0.95, dark ? 0.064 : 0.94, 1.0);
    g_ngfx.set_source_rgba(cr, dark ? 1 : 0, dark ? 1 : 0, dark ? 1 : 0, 0.08);
    g_ngfx.set_line_width(cr, 1.0);
    g_ngfx.move_to(cr, cx + side_w + 0.5, cy);
    g_ngfx.line_to(cr, cx + side_w + 0.5, cy + ch);
    g_ngfx.stroke(cr);

    ngfx_text(cr, "Plus Jakarta Sans", 1, 10.2, cx + 12, cy + 20, mut_r, mut_g, mut_b, 0.80, "PLACES");
    for (int i = 0; i < NUM_PLACES; ++i) {
        int py = cy + 28 + i * 29;
        if (py + 28 > cy + ch - 42) break;
        int act = strcmp(st->files_cwd, PLACES_PATHS[i]) == 0;
        int hov = (st->hover_mx >= cx + 6 && st->hover_mx <= cx + side_w - 6 && st->hover_my >= py && st->hover_my <= py + 26);
        if (act || hov) {
            ngfx_fill_rect(cr, cx + 6, py, side_w - 12, 26, 8,
                           act ? ar : (dark ? 1 : 0), act ? ag : (dark ? 1 : 0), act ? ab : (dark ? 1 : 0),
                           act ? 0.24 : 0.06);
        }
        if (act) {
            ngfx_fill_rect(cr, cx + 6, py + 5, 3, 16, 1.5, ar * 1.3 > 1 ? 1 : ar * 1.3,
                           ag * 1.3 > 1 ? 1 : ag * 1.3, ab * 1.3 > 1 ? 1 : ab * 1.3, 1.0);
        }
        draw_place_icon(cr, i, cx + 11, py + 5,
                        act ? (ar * 1.3 > 1 ? 1 : ar * 1.3) : mut_r,
                        act ? (ag * 1.3 > 1 ? 1 : ag * 1.3) : mut_g,
                        act ? (ab * 1.3 > 1 ? 1 : ab * 1.3) : mut_b, 0.95);
        ngfx_text_clipped(cr, "Noto Sans", act, 11.5, cx + 32, py + 17, side_w - 38,
                          fg_r, fg_g, fg_b, act ? 1.0 : 0.84, PLACES_LABELS[i]);
        nature_add_hit(st, cx + 6, py, side_w - 12, 26, HIT_FILES_PLACE, i, 0);
    }

    double disk_frac = st->disk_used_pct > 0 ? (st->disk_used_pct / 100.0) : 0.24;
    if (disk_frac > 1.0) disk_frac = 1.0;
    ngfx_fill_rect(cr, cx + 10, cy + ch - 34, side_w - 20, 6, 3, dark ? 1 : 0, dark ? 1 : 0, dark ? 1 : 0, 0.12);
    ngfx_fill_rect(cr, cx + 10, cy + ch - 34, (side_w - 20) * disk_frac, 6, 3, ar, ag, ab, 0.9);
    char disk_buf[48];
    if (compact) snprintf(disk_buf, sizeof(disk_buf), "%ldG free", st->disk_avail_gb);
    else snprintf(disk_buf, sizeof(disk_buf), "ext4 · %ld GiB free", st->disk_avail_gb);
    ngfx_text_clipped(cr, "JetBrains Mono", 0, 9.5, cx + 10, cy + ch - 12, side_w - 20, mut_r, mut_g, mut_b, 0.88, disk_buf);

    /* Top Header Toolbar */
    int bx = cx + side_w;
    int bw = cw - side_w;
    ngfx_fill_rect(cr, bx, cy, bw, 42, 0,
                   dark ? 0.062 : 0.97, dark ? 0.092 : 0.985, dark ? 0.080 : 0.98, 1.0);
    g_ngfx.set_source_rgba(cr, dark ? 1 : 0, dark ? 1 : 0, dark ? 1 : 0, 0.08);
    g_ngfx.move_to(cr, bx, cy + 42.5);
    g_ngfx.line_to(cr, bx + bw, cy + 42.5);
    g_ngfx.stroke(cr);

    int can_up = strcmp(st->files_cwd, "/") != 0;
    ngfx_fill_rect(cr, bx + 8, cy + 8, 26, 26, 7, dark ? 1 : 0, dark ? 1 : 0, dark ? 1 : 0, can_up ? 0.10 : 0.03);
    ngfx_text(cr, "Plus Jakarta Sans", 1, 13.0, bx + 16, cy + 25, fg_r, fg_g, fg_b, can_up ? 0.95 : 0.30, "^");
    if (can_up) nature_add_hit(st, bx + 8, cy + 8, 26, 26, HIT_FILES_OPEN_PARENT, 0, 0);

    int path_w = bw - (compact ? 186 : 298);
    if (path_w < 64) path_w = 64;
    ngfx_fill_rect(cr, bx + 40, cy + 8, path_w, 26, 7, dark ? 0.04 : 0.92, dark ? 0.06 : 0.94, dark ? 0.05 : 0.93, 1.0);
    ngfx_text_clipped(cr, "JetBrains Mono", 0, 11.2, bx + 48, cy + 25, path_w - 14, fg_r, fg_g, fg_b, 0.92, st->files_cwd);

    if (!compact) {
        int sx = bx + bw - 252;
        ngfx_fill_rect(cr, sx, cy + 8, 54, 26, 7, dark ? 1 : 0, dark ? 1 : 0, dark ? 1 : 0, 0.09);
        ngfx_text(cr, "Noto Sans", 1, 11.0, sx + 10, cy + 25, fg_r, fg_g, fg_b, 0.9,
                  st->files_sort_mode ? "Size" : "Name");
        nature_add_hit(st, sx, cy + 8, 54, 26, HIT_FILES_SORT, 0, 0);

        int nfx2 = bx + bw - 192;
        ngfx_fill_rect(cr, nfx2, cy + 8, 58, 26, 7, dark ? 1 : 0, dark ? 1 : 0, dark ? 1 : 0, 0.09);
        ngfx_text(cr, "Noto Sans", 1, 11.0, nfx2 + 8, cy + 25, fg_r, fg_g, fg_b, 0.95, "+ Note");
        nature_add_hit(st, nfx2, cy + 8, 58, 26, HIT_FILES_NEW_FILE, 0, 0);
    }

    int nfx = bx + bw - 128;
    ngfx_fill_rect(cr, nfx, cy + 8, 68, 26, 7, ar, ag, ab, 0.90);
    ngfx_text(cr, "Noto Sans", 1, 11.0, nfx + 8, cy + 25, 1, 1, 1, 1, "+ Folder");
    nature_add_hit(st, nfx, cy + 8, 68, 26, HIT_FILES_NEW_DIR, 0, 0);

    int vmx = bx + bw - 54;
    ngfx_fill_rect(cr, vmx, cy + 8, 44, 26, 7, dark ? 1 : 0, dark ? 1 : 0, dark ? 1 : 0, 0.10);
    ngfx_text(cr, "Noto Sans", 1, 11.0, vmx + 9, cy + 25, fg_r, fg_g, fg_b, 0.9,
              st->files_list_mode ? "Grid" : "List");
    nature_add_hit(st, vmx, cy + 8, 44, 26, HIT_FILES_VIEW_MODE, 0, 0);

    /* Sub-bar: Interactive Filter / Search Input */
    int search_bar_y = cy + 46;
    int search_active = (st->active_text_target == 4);
    int sbx = bx + 10, sbw = bw - 20, sbh = 26;
    ngfx_fill_rect(cr, sbx, search_bar_y, sbw, sbh, 7,
                   dark ? 0.036 : 0.92, dark ? 0.052 : 0.94, dark ? 0.046 : 0.93, 1.0);
    ngfx_stroke_rect(cr, sbx + 0.5, search_bar_y + 0.5, sbw - 1, sbh - 1, 7, 1.0,
                     search_active ? ar : 1.0, search_active ? ag : 1.0, search_active ? ab : 1.0,
                     search_active ? 0.75 : 0.08);
    if (st->files_query[0]) {
        ngfx_text_clipped(cr, "Noto Sans", 1, 11.5, sbx + 10, search_bar_y + 17, sbw - 40, fg_r, fg_g, fg_b, 1.0, st->files_query);
        ngfx_fill_rect(cr, sbx + sbw - 26, search_bar_y + 4, 20, 18, 5, dark ? 1 : 0, dark ? 1 : 0, dark ? 1 : 0, 0.12);
        ngfx_text(cr, "Noto Sans", 1, 10.5, sbx + sbw - 19, search_bar_y + 17, fg_r, fg_g, fg_b, 0.9, "x");
        nature_add_hit(st, sbx + sbw - 28, search_bar_y + 2, 24, 22, HIT_FILES_SEARCH_CLEAR, 0, 0);
        nature_add_hit(st, sbx, search_bar_y, sbw - 30, sbh, HIT_FILES_SEARCH_BOX, 0, 0);
    } else {
        ngfx_text_clipped(cr, "Noto Sans", 0, 11.2, sbx + 10, search_bar_y + 17, sbw - 20, mut_r, mut_g, mut_b, 0.85,
                          "Filter folder items (tap to search)...");
        nature_add_hit(st, sbx, search_bar_y, sbw, sbh, HIT_FILES_SEARCH_BOX, 0, 0);
    }
    nature_add_text_rect(st, sbx, search_bar_y, sbw, sbh, 4);

    /* Collect and filter/sort items */
    int indices[MAX_VFS];
    int n_items = 0;
    for (int i = 0; i < st->vfs_count; ++i) {
        if (strcmp(st->vfs[i].parent, st->files_cwd) == 0 && str_ci_match(st->vfs[i].name, st->files_query)) {
            indices[n_items++] = i;
        }
    }
    for (int a = 0; a < n_items; ++a) {
        for (int b = a + 1; b < n_items; ++b) {
            VfsNode *na = &st->vfs[indices[a]];
            VfsNode *nb = &st->vfs[indices[b]];
            int swap = 0;
            if (na->is_dir != nb->is_dir) {
                if (!na->is_dir && nb->is_dir) swap = 1;
            } else if (st->files_sort_mode == 1) {
                if (na->size_bytes < nb->size_bytes) swap = 1;
            } else {
                if (strcmp(na->name, nb->name) > 0) swap = 1;
            }
            if (swap) {
                int tmp = indices[a]; indices[a] = indices[b]; indices[b] = tmp;
            }
        }
    }

    int has_sel = (st->files_selected_idx >= 0 && st->files_selected_idx < st->vfs_count &&
                   strcmp(st->vfs[st->files_selected_idx].parent, st->files_cwd) == 0);
    int preview_w = (has_sel && bw >= 360) ? (compact ? 154 : 198) : 0;
    int list_w = bw - preview_w - 14;
    int iy_start = cy + 80;
    int content_h = ch - 82;

    int col_count = st->files_list_mode ? 1 : ((list_w - 16) / 114);
    if (col_count < 1) col_count = 1;

    int total_rows = st->files_list_mode ? n_items : ((n_items + col_count - 1) / col_count);
    int vis_rows = st->files_list_mode ? (content_h / 32) : (content_h / 96);
    if (vis_rows < 1) vis_rows = 1;
    int max_scroll = total_rows - vis_rows;
    if (max_scroll < 0) max_scroll = 0;
    st->files_max_scroll = max_scroll;
    if (st->files_scroll > max_scroll) st->files_scroll = max_scroll;
    if (st->files_scroll < 0) st->files_scroll = 0;

    if (n_items == 0) {
        ngfx_text(cr, "Plus Jakarta Sans", 1, 13.5, bx + list_w * 0.5 - 56, iy_start + content_h * 0.45, mut_r, mut_g, mut_b, 0.7,
                  st->files_query[0] ? "No matching items" : "Folder is empty");
    }

    int start_item = st->files_list_mode ? st->files_scroll : (st->files_scroll * col_count);
    for (int k = start_item; k < n_items; ++k) {
        int i = indices[k];
        VfsNode *n = &st->vfs[i];
        int slot = k - start_item;
        int sel = (st->files_selected_idx == i);

        if (st->files_list_mode) {
            int ry = iy_start + slot * 32;
            if (ry + 30 > cy + ch - 6) break;
            int hov = (st->hover_mx >= bx + 10 && st->hover_mx <= bx + list_w - 2 && st->hover_my >= ry && st->hover_my <= ry + 28);
            ngfx_fill_rect(cr, bx + 10, ry, list_w - 12, 28, 7,
                           (sel || hov) ? ar : (dark ? 1 : 0),
                           (sel || hov) ? ag : (dark ? 1 : 0),
                           (sel || hov) ? ab : (dark ? 1 : 0),
                           sel ? 0.24 : (hov ? 0.10 : (slot % 2 == 0 ? 0.03 : 0.0)));
            if (sel) ngfx_stroke_rect(cr, bx + 10.5, ry + 0.5, list_w - 13, 27, 7, 1.0, ar, ag, ab, 0.72);
            /* Miniature folder/file badge dot */
            ngfx_fill_rect(cr, bx + 16, ry + 7, 32, 14, 4,
                           n->is_dir ? ar : (dark ? 1 : 0),
                           n->is_dir ? ag : (dark ? 1 : 0),
                           n->is_dir ? ab : (dark ? 1 : 0),
                           n->is_dir ? 0.25 : 0.08);
            const char *mtag = n->is_dir ? "DIR" : (n->mime_tag[0] ? n->mime_tag : "FILE");
            ngfx_text(cr, "JetBrains Mono", 1, 8.8, bx + 20, ry + 17,
                      n->is_dir ? (ar * 1.3 > 1 ? 1 : ar * 1.3) : mut_r,
                      n->is_dir ? (ag * 1.3 > 1 ? 1 : ag * 1.3) : mut_g,
                      n->is_dir ? (ab * 1.3 > 1 ? 1 : ab * 1.3) : mut_b, 1.0, mtag);
            ngfx_text_clipped(cr, "Noto Sans", sel, 12.0, bx + 56, ry + 18, list_w - 144, fg_r, fg_g, fg_b, 1.0, n->name);
            char sz_buf[24];
            format_size(n, sz_buf, sizeof(sz_buf));
            ngfx_text(cr, "JetBrains Mono", 0, 10.0, bx + list_w - 78, ry + 18, mut_r, mut_g, mut_b, 0.9, sz_buf);
            nature_add_hit(st, bx + 10, ry, list_w - 12, 28, HIT_FILES_NODE, i, 0);
        } else {
            int c = slot % col_count;
            int r = slot / col_count;
            int gx = bx + 12 + c * 114;
            int gy = iy_start + r * 96;
            if (gy + 88 > cy + ch - 4) break;
            int hov = (st->hover_mx >= gx && st->hover_mx <= gx + 104 && st->hover_my >= gy && st->hover_my <= gy + 88);

            ngfx_fill_rect(cr, gx, gy, 104, 88, 11,
                           sel ? ar : (hov ? ar : (dark ? 1 : 0)),
                           sel ? ag : (hov ? ag : (dark ? 1 : 0)),
                           sel ? ab : (hov ? ab : (dark ? 1 : 0)),
                           sel ? 0.22 : (hov ? 0.10 : 0.04));
            if (sel || hov) {
                ngfx_stroke_rect(cr, gx + 0.5, gy + 0.5, 103, 87, 11, sel ? 1.3 : 1.0, ar, ag, ab, sel ? 0.85 : 0.42);
            }
            draw_file_glyph(cr, st, n, gx + 13, gy + 8, 78, 46, ar, ag, ab, dark);
            double tw = ngfx_text_width(cr, "Noto Sans", sel, 11.0, n->name);
            if (tw < 90) {
                ngfx_text(cr, "Noto Sans", sel, 11.0, gx + (104 - tw) * 0.5, gy + 72, fg_r, fg_g, fg_b, 0.96, n->name);
            } else {
                ngfx_text_clipped(cr, "Noto Sans", sel, 11.0, gx + 6, gy + 72, 92, fg_r, fg_g, fg_b, 0.96, n->name);
            }
            nature_add_hit(st, gx, gy, 104, 88, HIT_FILES_NODE, i, 0);
        }
    }

    /* Interactive Scrollbar */
    nature_draw_scrollbar(st, cr, bx + list_w + 3, iy_start, content_h,
                          total_rows, vis_rows, st->files_scroll, max_scroll, SCROLL_FILES, dark);

    /* Right Inspector Panel */
    if (preview_w > 0) {
        VfsNode *sn = &st->vfs[st->files_selected_idx];
        int px = bx + list_w + 14;
        ngfx_fill_rect(cr, px, cy + 43, preview_w, ch - 43, 0,
                       dark ? 0.048 : 0.94, dark ? 0.072 : 0.96, dark ? 0.064 : 0.95, 1.0);
        g_ngfx.set_source_rgba(cr, dark ? 1 : 0, dark ? 1 : 0, dark ? 1 : 0, 0.08);
        g_ngfx.move_to(cr, px + 0.5, cy + 43);
        g_ngfx.line_to(cr, px + 0.5, cy + ch);
        g_ngfx.stroke(cr);

        draw_file_glyph(cr, st, sn, px + (preview_w - 130) * 0.5, cy + 54, 130, 74, ar, ag, ab, dark);
        ngfx_text_clipped(cr, "Plus Jakarta Sans", 1, 12.5, px + 12, cy + 146, preview_w - 24, fg_r, fg_g, fg_b, 1.0, sn->name);

        char meta_buf[64], sz_buf[24];
        format_size(sn, sz_buf, sizeof(sz_buf));
        snprintf(meta_buf, sizeof(meta_buf), "%s · %s", sn->is_dir ? "Folder" : (sn->mime_tag[0] ? sn->mime_tag : "File"), sz_buf);
        ngfx_text(cr, "JetBrains Mono", 0, 9.8, px + 12, cy + 164, mut_r, mut_g, mut_b, 0.95, meta_buf);
        ngfx_text(cr, "JetBrains Mono", 0, 9.5, px + 12, cy + 179, mut_r, mut_g, mut_b, 0.75,
                  sn->modified[0] ? sn->modified : "Nov 25 2026");

        if (sn->content[0] && ch > 320) {
            ngfx_fill_rect(cr, px + 10, cy + 190, preview_w - 20, ch - 294, 8,
                           dark ? 0.03 : 0.98, dark ? 0.045 : 0.99, dark ? 0.04 : 0.985, 1.0);
            char line_buf[64];
            const char *p = sn->content;
            int line_y = cy + 206;
            while (*p && line_y < cy + ch - 110) {
                int k = 0;
                while (p[k] && p[k] != '\n' && k < 23) { line_buf[k] = p[k]; k++; }
                line_buf[k] = '\0';
                ngfx_text(cr, "JetBrains Mono", 0, 9.2, px + 16, line_y, mut_r, mut_g, mut_b, 0.95, line_buf);
                line_y += 14;
                p += k;
                if (*p == '\n') p++;
                else while (*p && *p != '\n') p++;
                if (*p == '\n') p++;
            }
        }

        int btn_y = cy + ch - 80;
        if (sn->image_idx >= 0) {
            ngfx_fill_rect(cr, px + 10, btn_y - 32, preview_w - 20, 26, 7, dark ? 1 : 0, dark ? 1 : 0, dark ? 1 : 0, 0.12);
            ngfx_text_clipped(cr, "Noto Sans", 1, 10.8, px + 26, btn_y - 15, preview_w - 40, fg_r, fg_g, fg_b, 1.0, "Set Wallpaper");
            nature_add_hit(st, px + 10, btn_y - 32, preview_w - 20, 26, HIT_FILES_SET_WP, sn->image_idx, 0);
        }
        const char *act_lbl = sn->is_dir ? "Open Folder" : (sn->image_idx >= 0 || sn->audio_idx >= 0 ? "Open in Media" : "Read File");
        ngfx_fill_rect(cr, px + 10, btn_y, preview_w - 20, 28, 7, ar, ag, ab, 0.92);
        double alw = ngfx_text_width(cr, "Noto Sans", 1, 10.8, act_lbl);
        ngfx_text(cr, "Noto Sans", 1, 10.8, px + 10 + ((preview_w - 20) - alw) * 0.5, btn_y + 18, 1, 1, 1, 1, act_lbl);
        nature_add_hit(st, px + 10, btn_y, preview_w - 20, 28, HIT_FILES_OPEN_ACTION, st->files_selected_idx, 0);

        ngfx_fill_rect(cr, px + 10, cy + ch - 42, preview_w - 20, 28, 7, 0.85, 0.25, 0.25, 0.16);
        double dlw = ngfx_text_width(cr, "Noto Sans", 1, 10.8, "Delete Item");
        ngfx_text(cr, "Noto Sans", 1, 10.8, px + 10 + ((preview_w - 20) - dlw) * 0.5, cy + ch - 24, 0.95, 0.42, 0.42, 1.0, "Delete Item");
        nature_add_hit(st, px + 10, cy + ch - 42, preview_w - 20, 28, HIT_FILES_DELETE, st->files_selected_idx, 0);
    }

    /* Inline Quick Text / Config / Source Reader Modal */
    if (st->files_viewer_open && st->files_viewer_idx >= 0 && st->files_viewer_idx < st->vfs_count) {
        VfsNode *vn = &st->vfs[st->files_viewer_idx];
        ngfx_fill_rect(cr, cx, cy, cw, ch, 0, 0.01, 0.02, 0.015, 0.72);
        int mw = cw - 48;
        if (mw > 540) mw = 540;
        int mh = ch - 44;
        if (mh > 360) mh = 360;
        int mx = cx + (cw - mw) / 2;
        int my = cy + (ch - mh) / 2;

        ngfx_shadow_rect(cr, mx, my, mw, mh, 12, 3.0, 0.55);
        ngfx_fill_rect(cr, mx, my, mw, mh, 12, dark ? 0.055 : 0.98, dark ? 0.082 : 0.99, dark ? 0.074 : 0.985, 1.0);
        ngfx_stroke_rect(cr, mx + 0.5, my + 0.5, mw - 1, mh - 1, 12, 1.2, ar, ag, ab, 0.65);

        ngfx_fill_rect(cr, mx, my, mw, 38, 0, dark ? 0.08 : 0.93, dark ? 0.12 : 0.95, dark ? 0.10 : 0.94, 1.0);
        ngfx_text_clipped(cr, "JetBrains Mono", 1, 11.5, mx + 14, my + 24, mw - 92, fg_r, fg_g, fg_b, 1.0, vn->path);
        ngfx_fill_rect(cr, mx + mw - 72, my + 7, 60, 24, 6, 0.85, 0.25, 0.25, 0.20);
        ngfx_text(cr, "Noto Sans", 1, 10.5, mx + mw - 56, my + 23, 0.95, 0.48, 0.48, 1.0, "Close");
        nature_add_hit(st, mx + mw - 72, my + 7, 60, 24, HIT_FILES_CLOSE_VIEWER, 0, 0);

        int ty = my + 58;
        const char *p = vn->content[0] ? vn->content : "[Empty file]";
        char lbuf[96];
        int lno = 1;
        while (*p && ty < my + mh - 20) {
            int k = 0;
            while (p[k] && p[k] != '\n' && k < 70) { lbuf[k] = p[k]; k++; }
            lbuf[k] = '\0';
            char num[16];
            snprintf(num, sizeof(num), "%2d", lno++);
            ngfx_text(cr, "JetBrains Mono", 0, 10.0, mx + 14, ty, mut_r, mut_g, mut_b, 0.55, num);
            ngfx_text_clipped(cr, "JetBrains Mono", 0, 10.8, mx + 38, ty, mw - 52, fg_r, fg_g, fg_b, 0.96, lbuf);
            ty += 19;
            p += k;
            if (*p == '\n') p++;
            else while (*p && *p != '\n') p++;
            if (*p == '\n') p++;
        }
    }
}

static void open_vfs_node(NatureOSState *st, int idx) {
    if (idx < 0 || idx >= st->vfs_count) return;
    VfsNode *n = &st->vfs[idx];
    if (n->is_dir) {
        snprintf(st->files_cwd, sizeof(st->files_cwd), "%s", n->path);
        st->files_selected_idx = -1;
        st->files_scroll = 0;
        st->files_query[0] = '\0';
    } else if (n->image_idx >= 0) {
        st->media_mode = 0;
        st->media_selected = n->image_idx;
        st->media_zoom_pct = 100;
        st->media_rotation = 0;
        nature_open_app(st, APP_MEDIA);
    } else if (n->audio_idx >= 0) {
        st->media_mode = 1;
        st->audio_selected = n->audio_idx;
        st->audio_playing = 1;
        st->audio_progress_sec = 0;
        nature_open_app(st, APP_MEDIA);
    } else {
        st->files_viewer_open = 1;
        st->files_viewer_idx = idx;
    }
}

void nature_files_click(NatureOSState *st, const HitRect *hit) {
    if (hit->action_id == HIT_FILES_PLACE) {
        snprintf(st->files_cwd, sizeof(st->files_cwd), "%s", PLACES_PATHS[hit->arg0]);
        st->files_selected_idx = -1;
        st->files_scroll = 0;
        st->files_query[0] = '\0';
    } else if (hit->action_id == HIT_FILES_VIEW_MODE) {
        st->files_list_mode = !st->files_list_mode;
        st->files_scroll = 0;
    } else if (hit->action_id == HIT_FILES_SORT) {
        st->files_sort_mode = !st->files_sort_mode;
    } else if (hit->action_id == HIT_FILES_SEARCH_BOX) {
        st->active_text_target = 4;
        st->input_focus_pulse = 1;
    } else if (hit->action_id == HIT_FILES_SEARCH_CLEAR) {
        st->files_query[0] = '\0';
        st->files_scroll = 0;
    } else if (hit->action_id == HIT_FILES_OPEN_PARENT) {
        char *slash = strrchr(st->files_cwd, '/');
        if (slash && slash != st->files_cwd) *slash = '\0';
        else snprintf(st->files_cwd, sizeof(st->files_cwd), "/");
        st->files_selected_idx = -1;
        st->files_scroll = 0;
        st->files_query[0] = '\0';
    } else if (hit->action_id == HIT_FILES_NEW_DIR) {
        if (st->vfs_count < MAX_VFS) {
            char name[48], path[192];
            int idx = st->vfs_count;
            snprintf(name, sizeof(name), "folder-%d", idx);
            snprintf(path, sizeof(path), "%s/%s", strcmp(st->files_cwd, "/") == 0 ? "" : st->files_cwd, name);
            VfsNode *n = &st->vfs[st->vfs_count++];
            memset(n, 0, sizeof(*n));
            snprintf(n->name, sizeof(n->name), "%s", name);
            snprintf(n->path, sizeof(n->path), "%.*s", (int)sizeof(n->path) - 1, path);
            snprintf(n->parent, sizeof(n->parent), "%s", st->files_cwd);
            snprintf(n->mime_tag, sizeof(n->mime_tag), "DIR");
            snprintf(n->modified, sizeof(n->modified), "Just now");
            n->is_dir = 1; n->size_bytes = 4096; n->image_idx = -1; n->audio_idx = -1;
            st->files_selected_idx = st->vfs_count - 1;
        }
    } else if (hit->action_id == HIT_FILES_NEW_FILE) {
        if (st->vfs_count < MAX_VFS) {
            char name[48], path[192];
            int idx = st->vfs_count;
            snprintf(name, sizeof(name), "note-%d.txt", idx);
            snprintf(path, sizeof(path), "%s/%s", strcmp(st->files_cwd, "/") == 0 ? "" : st->files_cwd, name);
            VfsNode *n = &st->vfs[st->vfs_count++];
            memset(n, 0, sizeof(*n));
            snprintf(n->name, sizeof(n->name), "%s", name);
            snprintf(n->path, sizeof(n->path), "%.*s", (int)sizeof(n->path) - 1, path);
            snprintf(n->parent, sizeof(n->parent), "%s", st->files_cwd);
            snprintf(n->mime_tag, sizeof(n->mime_tag), "TXT");
            snprintf(n->modified, sizeof(n->modified), "Just now");
            snprintf(n->content, sizeof(n->content), "Nature OS Note #%d\nCreated in Files Manager.\nReady to inspect in Terminal.", idx);
            n->is_dir = 0; n->size_bytes = 128; n->image_idx = -1; n->audio_idx = -1;
            st->files_selected_idx = st->vfs_count - 1;
        }
    } else if (hit->action_id == HIT_FILES_NODE) {
        int idx = hit->arg0;
        if (idx >= 0 && idx < st->vfs_count) {
            if (st->files_selected_idx == idx) {
                open_vfs_node(st, idx);
            } else {
                st->files_selected_idx = idx;
            }
        }
    } else if (hit->action_id == HIT_FILES_OPEN_ACTION) {
        open_vfs_node(st, hit->arg0);
    } else if (hit->action_id == HIT_FILES_SET_WP) {
        int img = hit->arg0;
        if (img >= 1 && img <= 3) {
            st->cfg.wallpaper_idx = (img - 1) % 3;
            nature_notify(st, "Desktop Appearance", "Wallpaper updated from Files.");
        } else {
            nature_notify(st, "Desktop Appearance", "Custom photo applied to preview.");
            st->cfg.wallpaper_idx = img % 3;
        }
    } else if (hit->action_id == HIT_FILES_CLOSE_VIEWER) {
        st->files_viewer_open = 0;
    } else if (hit->action_id == HIT_FILES_DELETE) {
        int idx = hit->arg0;
        if (idx >= 0 && idx < st->vfs_count) {
            for (int j = idx; j < st->vfs_count - 1; ++j) st->vfs[j] = st->vfs[j + 1];
            st->vfs_count--;
            st->files_selected_idx = -1;
            st->files_viewer_open = 0;
        }
    }
}

void nature_files_key(NatureOSState *st, const char *key) {
    if (strcmp(key, "Backspace") == 0) {
        size_t len = strlen(st->files_query);
        if (len > 0) st->files_query[len - 1] = '\0';
        st->files_scroll = 0;
    } else if (strcmp(key, "Enter") == 0) {
        st->active_text_target = 0;
    } else if (strlen(key) == 1 && (unsigned char)key[0] >= 32 && (unsigned char)key[0] < 127) {
        size_t len = strlen(st->files_query);
        if (len + 1 < sizeof(st->files_query)) {
            st->files_query[len] = key[0];
            st->files_query[len + 1] = '\0';
            st->files_scroll = 0;
        }
    }
}

void nature_files_wheel(NatureOSState *st, int delta_y) {
    if (delta_y > 0) {
        if (st->files_scroll < st->files_max_scroll) st->files_scroll++;
    } else if (delta_y < 0 && st->files_scroll > 0) {
        st->files_scroll--;
    }
}
