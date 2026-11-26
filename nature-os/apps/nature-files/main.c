#include "nature_os.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NUM_PLACES 10

static const char *PLACES_PATHS[NUM_PLACES] = {
    "/home/nature",
    "/home/nature/Desktop",
    "/home/nature/Documents",
    "/home/nature/Downloads",
    "/home/nature/Pictures",
    "/home/nature/Music",
    "/home/nature/Videos",
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
    "Videos",
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
        case 6: /* Videos */
            ngfx_stroke_rect(cr, x + 2, y + 3.2, 12, 9.6, 2, 1.3, r, g, b, a);
            g_ngfx.move_to(cr, x + 6.5, y + 5.5);
            g_ngfx.line_to(cr, x + 10.5, y + 8.0);
            g_ngfx.line_to(cr, x + 6.5, y + 10.5);
            g_ngfx.close_path(cr);
            g_ngfx.fill(cr);
            break;
        case 7: /* Projects </> */
            g_ngfx.move_to(cr, x + 5.5, y + 4.5);
            g_ngfx.line_to(cr, x + 2.5, y + 8.0);
            g_ngfx.line_to(cr, x + 5.5, y + 11.5);
            g_ngfx.move_to(cr, x + 10.5, y + 4.5);
            g_ngfx.line_to(cr, x + 13.5, y + 8.0);
            g_ngfx.line_to(cr, x + 10.5, y + 11.5);
            g_ngfx.stroke(cr);
            break;
        case 8:
        case 9: /* Logs / System */
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
        if (sw > 0 && sh > 0) {
            double dw = w - 8.0, dh = h - 4.0;
            double sc_x = dw / sw, sc_y = dh / sh;
            double sc = sc_x > sc_y ? sc_x : sc_y;
            double off_x = (dw - sw * sc) * 0.5;
            double off_y = (dh - sh * sc) * 0.5;
            g_ngfx.translate(cr, x + 4 + off_x, y + 2 + off_y);
            g_ngfx.scale(cr, sc, sc);
            g_ngfx.set_source_surface(cr, st->media_images[n->image_idx], 0, 0);
            g_ngfx.paint(cr);
        }
        g_ngfx.restore(cr);
        ngfx_stroke_rect(cr, x + 4, y + 2, w - 8, h - 4, 6, 1.0, 1, 1, 1, 0.20);
        if (n->video_idx >= 0) {
            double pcx = x + w * 0.5, pcy = y + h * 0.5;
            ngfx_fill_circle(cr, pcx, pcy, 11.5, 0.03, 0.05, 0.08, 0.72);
            g_ngfx.set_source_rgba(cr, 1.0, 1.0, 1.0, 0.96);
            g_ngfx.move_to(cr, pcx - 3.2, pcy - 5.2);
            g_ngfx.line_to(cr, pcx + 5.8, pcy);
            g_ngfx.line_to(cr, pcx - 3.2, pcy + 5.2);
            g_ngfx.close_path(cr);
            g_ngfx.fill(cr);
        }
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
    else if (strcmp(n->mime_tag, "VID") == 0) { br = 0.72; bg = 0.34; bb = 0.78; }
    else if (strcmp(n->mime_tag, "JS") == 0 || strcmp(n->mime_tag, "JSON") == 0) { br = 0.85; bg = 0.62; bb = 0.14; }
    else if (strcmp(n->mime_tag, "PY") == 0) { br = 0.22; bg = 0.62; bb = 0.84; }
    else if (strcmp(n->mime_tag, "MD") == 0) { br = 0.18; bg = 0.70; bb = 0.72; }
    else if (strcmp(n->mime_tag, "C") == 0 || strcmp(n->mime_tag, "CPP") == 0) { br = 0.22; bg = 0.54; bb = 0.90; }
    else if (strcmp(n->mime_tag, "DEB") == 0 || strcmp(n->mime_tag, "ISO") == 0) { br = 0.88; bg = 0.46; bb = 0.15; }
    else if (strcmp(n->mime_tag, "CONF") == 0 || strcmp(n->mime_tag, "LOG") == 0) { br = 0.62; bg = 0.44; bb = 0.84; }

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

    g_ngfx.set_source_rgba(cr, br, bg, bb, 0.35);
    g_ngfx.move_to(cr, dx + 26, dy);
    g_ngfx.line_to(cr, dx + 26, dy + 10);
    g_ngfx.line_to(cr, dx + 36, dy + 10);
    g_ngfx.close_path(cr);
    g_ngfx.fill(cr);

    ngfx_fill_rect(cr, dx + 6, dy + 11, 16, 2, 1, dark ? 1 : 0, dark ? 1 : 0, dark ? 1 : 0, 0.18);
    ngfx_fill_rect(cr, dx + 6, dy + 16, 22, 2, 1, dark ? 1 : 0, dark ? 1 : 0, dark ? 1 : 0, 0.14);

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

static void format_bytes_total(long long total, char *out, size_t cap) {
    if (total >= 1073741824LL) snprintf(out, cap, "%.2f GiB", total / 1073741824.0);
    else if (total >= 1048576LL) snprintf(out, cap, "%.1f MiB", total / 1048576.0);
    else if (total >= 1024LL) snprintf(out, cap, "%.1f KiB", total / 1024.0);
    else snprintf(out, cap, "%lld B", total);
}

static void apply_rename(NatureOSState *st) {
    int idx = st->files_selected_idx;
    if (idx < 0 || idx >= st->vfs_count) {
        st->files_renaming = 0;
        return;
    }
    char clean[48];
    size_t j = 0;
    for (size_t i = 0; st->files_rename_buf[i] && j + 1 < sizeof(clean); ++i) {
        unsigned char c = (unsigned char)st->files_rename_buf[i];
        if (c >= 32 && c != '/' && c < 127) clean[j++] = (char)c;
    }
    clean[j] = '\0';
    if (!clean[0]) {
        st->files_renaming = 0;
        return;
    }
    VfsNode *n = &st->vfs[idx];
    char old_path[128];
    snprintf(old_path, sizeof(old_path), "%s", n->path);

    char new_path[192];
    snprintf(new_path, sizeof(new_path), "%s/%s", strcmp(n->parent, "/") == 0 ? "" : n->parent, clean);
    snprintf(n->name, sizeof(n->name), "%s", clean);
    snprintf(n->path, sizeof(n->path), "%.127s", new_path);
    snprintf(n->modified, sizeof(n->modified), "Just now");

    if (n->is_dir) {
        size_t old_len = strlen(old_path);
        for (int k = 0; k < st->vfs_count; ++k) {
            if (k == idx) continue;
            if (strcmp(st->vfs[k].parent, old_path) == 0) {
                snprintf(st->vfs[k].parent, sizeof(st->vfs[k].parent), "%s", n->path);
                char child_p[192];
                snprintf(child_p, sizeof(child_p), "%s/%s", n->path, st->vfs[k].name);
                snprintf(st->vfs[k].path, sizeof(st->vfs[k].path), "%.127s", child_p);
            } else if (strncmp(st->vfs[k].parent, old_path, old_len) == 0 && st->vfs[k].parent[old_len] == '/') {
                char updated_parent[192];
                snprintf(updated_parent, sizeof(updated_parent), "%s%s", n->path, st->vfs[k].parent + old_len);
                snprintf(st->vfs[k].parent, sizeof(st->vfs[k].parent), "%.127s", updated_parent);
                char updated_path[192];
                snprintf(updated_path, sizeof(updated_path), "%s/%s", st->vfs[k].parent, st->vfs[k].name);
                snprintf(st->vfs[k].path, sizeof(st->vfs[k].path), "%.127s", updated_path);
            }
        }
    }
    st->files_renaming = 0;
    st->active_text_target = 0;
    nature_notify(st, "Files", "Item renamed cleanly.");
    nature_os_save_state(st);
}

void nature_files_render(NatureOSState *st, cairo_t *cr, OSWindow *w, int cx, int cy, int cw, int ch) {
    (void)w;
    NaturePalette pal;
    nature_get_palette(st, &pal);
    int dark = pal.dark_mode;
    double ar = pal.ar, ag = pal.ag, ab = pal.ab;

    int compact = cw < 540;
    int side_w = compact ? 114 : 162;
    double fg_r = pal.fg_r, fg_g = pal.fg_g, fg_b = pal.fg_b;
    double mut_r = pal.mut_r, mut_g = pal.mut_g, mut_b = pal.mut_b;

    /* Sidebar */
    ngfx_fill_rect(cr, cx, cy, side_w, ch, 0, pal.sidebar_r, pal.sidebar_g, pal.sidebar_b, 1.0);
    g_ngfx.set_source_rgba(cr, pal.border_r, pal.border_g, pal.border_b, pal.border_a);
    g_ngfx.set_line_width(cr, 1.0);
    g_ngfx.move_to(cr, cx + side_w + 0.5, cy);
    g_ngfx.line_to(cr, cx + side_w + 0.5, cy + ch);
    g_ngfx.stroke(cr);

    ngfx_text(cr, "Plus Jakarta Sans", 1, 10.2, cx + 12, cy + 20, mut_r, mut_g, mut_b, 0.86, "PLACES");
    for (int i = 0; i < NUM_PLACES; ++i) {
        int py = cy + 27 + i * 27;
        if (py + 26 > cy + ch - 42) break;
        int act = strcmp(st->files_cwd, PLACES_PATHS[i]) == 0;
        int hov = (st->hover_mx >= cx + 6 && st->hover_mx <= cx + side_w - 6 && st->hover_my >= py && st->hover_my <= py + 24);
        if (act || hov) {
            ngfx_fill_rect(cr, cx + 6, py, side_w - 12, 24, 7,
                           act ? ar : pal.hover_r, act ? ag : pal.hover_g, act ? ab : pal.hover_b,
                           act ? (dark ? 0.24 : 0.16) : (dark ? 0.45 : 0.65));
        }
        if (act) {
            ngfx_fill_rect(cr, cx + 6, py + 4, 3, 16, 1.5, pal.ar_hi, pal.ag_hi, pal.ab_hi, 1.0);
        }
        draw_place_icon(cr, i, cx + 11, py + 4,
                        act ? (dark ? pal.ar_hi : ar) : mut_r,
                        act ? (dark ? pal.ag_hi : ag) : mut_g,
                        act ? (dark ? pal.ab_hi : ab) : mut_b, 0.95);
        ngfx_text_clipped(cr, "Noto Sans", act, 11.2, cx + 32, py + 16, side_w - 38,
                          fg_r, fg_g, fg_b, act ? 1.0 : 0.84, PLACES_LABELS[i]);
        nature_add_hit(st, cx + 6, py, side_w - 12, 24, HIT_FILES_PLACE, i, 0);
    }

    double disk_frac = st->disk_used_pct > 0 ? (st->disk_used_pct / 100.0) : 0.24;
    if (disk_frac > 1.0) disk_frac = 1.0;
    ngfx_fill_rect(cr, cx + 10, cy + ch - 34, side_w - 20, 6, 3, pal.elev_r, pal.elev_g, pal.elev_b, 1.0);
    ngfx_fill_rect(cr, cx + 10, cy + ch - 34, (side_w - 20) * disk_frac, 6, 3, ar, ag, ab, 0.9);
    char disk_buf[48];
    if (compact) snprintf(disk_buf, sizeof(disk_buf), "%ldG free", st->disk_avail_gb);
    else snprintf(disk_buf, sizeof(disk_buf), "ext4 · %ld GiB free", st->disk_avail_gb);
    ngfx_text_clipped(cr, "JetBrains Mono", 0, 9.5, cx + 10, cy + ch - 12, side_w - 20, mut_r, mut_g, mut_b, 0.88, disk_buf);

    /* Top Header Toolbar */
    int bx = cx + side_w;
    int bw = cw - side_w;
    ngfx_fill_rect(cr, bx, cy, bw, 42, 0, pal.surface_r, pal.surface_g, pal.surface_b, 1.0);
    g_ngfx.set_source_rgba(cr, pal.border_r, pal.border_g, pal.border_b, pal.border_a);
    g_ngfx.move_to(cr, bx, cy + 42.5);
    g_ngfx.line_to(cr, bx + bw, cy + 42.5);
    g_ngfx.stroke(cr);

    int can_up = strcmp(st->files_cwd, "/") != 0;
    ngfx_fill_rect(cr, bx + 8, cy + 8, 26, 26, 7, pal.elev_r, pal.elev_g, pal.elev_b, can_up ? 0.95 : 0.45);
    ngfx_text(cr, "Plus Jakarta Sans", 1, 13.0, bx + 16, cy + 25, fg_r, fg_g, fg_b, can_up ? 0.95 : 0.30, "^");
    if (can_up) nature_add_hit(st, bx + 8, cy + 8, 26, 26, HIT_FILES_OPEN_PARENT, 0, 0);

    int show_note = (!compact || bw >= 320);
    int path_w = bw - (compact ? (show_note ? 286 : 196) : 344);
    if (path_w < 54) path_w = 54;
    int path_x = bx + 40;
    ngfx_fill_rect(cr, path_x, cy + 8, path_w, 26, 7, pal.card_r, pal.card_g, pal.card_b, 1.0);
    ngfx_stroke_rect(cr, path_x + 0.5, cy + 8.5, path_w - 1, 25, 7, 1.0, pal.border_r, pal.border_g, pal.border_b, pal.border_a);

    /* Interactive Breadcrumb Chips */
    {
        typedef struct { char label[36]; int end_off; } CrumbSeg;
        CrumbSeg segs[8];
        int nseg = 0;
        snprintf(segs[nseg].label, sizeof(segs[nseg].label), "/");
        segs[nseg].end_off = 1;
        nseg++;
        const char *p = st->files_cwd;
        if (*p == '/') p++;
        while (*p && nseg < 8) {
            const char *sl = strchr(p, '/');
            size_t len = sl ? (size_t)(sl - p) : strlen(p);
            if (len > 0) {
                size_t cpy = len < sizeof(segs[nseg].label) - 1 ? len : sizeof(segs[nseg].label) - 1;
                memcpy(segs[nseg].label, p, cpy);
                segs[nseg].label[cpy] = '\0';
                segs[nseg].end_off = (int)((p + len) - st->files_cwd);
                nseg++;
            }
            if (!sl) break;
            p = sl + 1;
        }
        int total_cw = 0;
        int widths[8];
        for (int s = 0; s < nseg; ++s) {
            widths[s] = (int)ngfx_text_width(cr, "JetBrains Mono", s == nseg - 1, 10.6, segs[s].label) + 12;
            total_cw += widths[s] + (s > 0 ? 8 : 0);
        }
        int start_seg = 0;
        while (start_seg < nseg - 1 && total_cw > path_w - 12) {
            total_cw -= widths[start_seg] + 8;
            start_seg++;
        }
        int cx_crumb = path_x + 5;
        for (int s = start_seg; s < nseg; ++s) {
            if (s > start_seg) {
                ngfx_text(cr, "JetBrains Mono", 0, 10.2, cx_crumb - 1, cy + 24.5, mut_r, mut_g, mut_b, 0.72, "/");
                cx_crumb += 8;
            }
            int cw_seg = widths[s];
            if (cx_crumb + cw_seg > path_x + path_w - 4) cw_seg = path_x + path_w - 4 - cx_crumb;
            if (cw_seg < 14) break;
            int is_last = (s == nseg - 1);
            int chov = (st->hover_mx >= cx_crumb && st->hover_mx <= cx_crumb + cw_seg &&
                        st->hover_my >= cy + 10 && st->hover_my <= cy + 32);
            if (is_last || chov) {
                ngfx_fill_rect(cr, cx_crumb, cy + 11, cw_seg, 20, 5,
                               is_last ? ar : pal.elev_r, is_last ? ag : pal.elev_g, is_last ? ab : pal.elev_b,
                               is_last ? (dark ? 0.24 : 0.16) : 0.90);
            }
            ngfx_text_clipped(cr, "JetBrains Mono", is_last, 10.4, cx_crumb + 6, cy + 24.5, cw_seg - 8,
                              is_last ? (dark ? pal.ar_hi : ar) : fg_r,
                              is_last ? (dark ? pal.ag_hi : ag) : fg_g,
                              is_last ? (dark ? pal.ab_hi : ab) : fg_b,
                              is_last ? 1.0 : 0.88, segs[s].label);
            nature_add_hit(st, cx_crumb, cy + 10, cw_seg, 22, HIT_FILES_BREADCRUMB, segs[s].end_off, 0);
            cx_crumb += cw_seg + 2;
        }
    }

    if (show_note) {
        int hx = compact ? (bx + bw - 224) : (bx + bw - 288);
        int hhov = (st->hover_hit_action == HIT_FILES_HIDDEN_TOGGLE);
        ngfx_fill_rect(cr, hx, cy + 8, 28, 26, 7,
                       st->files_show_hidden ? ar : (hhov ? pal.hover_r : pal.elev_r),
                       st->files_show_hidden ? ag : (hhov ? pal.hover_g : pal.elev_g),
                       st->files_show_hidden ? ab : (hhov ? pal.hover_b : pal.elev_b),
                       st->files_show_hidden ? 0.24 : 0.95);
        if (st->files_show_hidden || hhov) {
            ngfx_stroke_rect(cr, hx + 0.5, cy + 8.5, 27, 25, 7, 0.9, ar, ag, ab, st->files_show_hidden ? 0.75 : 0.45);
        }
        ngfx_text(cr, "JetBrains Mono", 1, 10.4, hx + 6, cy + 25,
                  st->files_show_hidden ? (dark ? pal.ar_hi : ar) : mut_r,
                  st->files_show_hidden ? (dark ? pal.ag_hi : ag) : mut_g,
                  st->files_show_hidden ? (dark ? pal.ab_hi : ab) : mut_b, 0.95, ".*");
        nature_add_hit(st, hx, cy + 8, 28, 26, HIT_FILES_HIDDEN_TOGGLE, 0, 0);

        if (!compact) {
            int sx = bx + bw - 252;
            int shov = (st->hover_hit_action == HIT_FILES_SORT);
            ngfx_fill_rect(cr, sx, cy + 8, 54, 26, 7,
                           shov ? pal.hover_r : pal.elev_r, shov ? pal.hover_g : pal.elev_g, shov ? pal.hover_b : pal.elev_b, 0.95);
            if (shov) ngfx_stroke_rect(cr, sx + 0.5, cy + 8.5, 53, 25, 7, 0.8, ar, ag, ab, 0.45);
            ngfx_text(cr, "Noto Sans", 1, 11.0, sx + 10, cy + 25, fg_r, fg_g, fg_b, 0.9,
                      st->files_sort_mode ? "Size" : "Name");
            nature_add_hit(st, sx, cy + 8, 54, 26, HIT_FILES_SORT, 0, 0);
        }

        int nfx2 = bx + bw - 190;
        int nhov = (st->hover_hit_action == HIT_FILES_NEW_FILE);
        ngfx_fill_rect(cr, nfx2, cy + 8, 56, 26, 7,
                       nhov ? pal.hover_r : pal.elev_r, nhov ? pal.hover_g : pal.elev_g, nhov ? pal.hover_b : pal.elev_b, 0.95);
        if (nhov) ngfx_stroke_rect(cr, nfx2 + 0.5, cy + 8.5, 55, 25, 7, 0.8, ar, ag, ab, 0.45);
        ngfx_text(cr, "Noto Sans", 1, 10.6, nfx2 + 7, cy + 25, fg_r, fg_g, fg_b, 0.95, "+ Note");
        nature_add_hit(st, nfx2, cy + 8, 56, 26, HIT_FILES_NEW_FILE, 0, 0);
    }

    int nfx = bx + bw - 128;
    int ndhov = (st->hover_hit_action == HIT_FILES_NEW_DIR);
    ngfx_fill_rect(cr, nfx, cy + 8, 68, 26, 7, ar, ag, ab, ndhov ? 1.0 : 0.92);
    ngfx_text(cr, "Noto Sans", 1, 11.0, nfx + 8, cy + 25, pal.on_acc_r, pal.on_acc_g, pal.on_acc_b, 1, "+ Folder");
    nature_add_hit(st, nfx, cy + 8, 68, 26, HIT_FILES_NEW_DIR, 0, 0);

    int vmx = bx + bw - 54;
    int vmhov = (st->hover_hit_action == HIT_FILES_VIEW_MODE);
    ngfx_fill_rect(cr, vmx, cy + 8, 44, 26, 7,
                   vmhov ? pal.hover_r : pal.elev_r, vmhov ? pal.hover_g : pal.elev_g, vmhov ? pal.hover_b : pal.elev_b, 0.95);
    if (vmhov) ngfx_stroke_rect(cr, vmx + 0.5, cy + 8.5, 43, 25, 7, 0.8, ar, ag, ab, 0.45);
    ngfx_text(cr, "Noto Sans", 1, 11.0, vmx + 9, cy + 25, fg_r, fg_g, fg_b, 0.9,
              st->files_list_mode ? "Grid" : "List");
    nature_add_hit(st, vmx, cy + 8, 44, 26, HIT_FILES_VIEW_MODE, 0, 0);

    /* Sub-bar: Interactive Filter / Search Input */
    int search_bar_y = cy + 46;
    int search_active = (st->active_text_target == 4);
    int sbx = bx + 10, sbw = bw - 20, sbh = 26;
    ngfx_fill_rect(cr, sbx, search_bar_y, sbw, sbh, 7, pal.card_r, pal.card_g, pal.card_b, 1.0);
    ngfx_stroke_rect(cr, sbx + 0.5, search_bar_y + 0.5, sbw - 1, sbh - 1, 7, 1.0,
                     search_active ? ar : pal.border_r, search_active ? ag : pal.border_g, search_active ? ab : pal.border_b,
                     search_active ? 0.80 : pal.border_a);
    if (st->files_query[0]) {
        ngfx_text_clipped(cr, "Noto Sans", 1, 11.5, sbx + 10, search_bar_y + 17, sbw - 40, fg_r, fg_g, fg_b, 1.0, st->files_query);
        ngfx_fill_rect(cr, sbx + sbw - 26, search_bar_y + 4, 20, 18, 5, pal.elev_r, pal.elev_g, pal.elev_b, 0.95);
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
    int dir_cnt = 0, file_cnt = 0;
    long long total_bytes = 0;
    for (int i = 0; i < st->vfs_count; ++i) {
        if (!st->files_show_hidden && st->vfs[i].name[0] == '.') continue;
        if (strcmp(st->vfs[i].parent, st->files_cwd) == 0 && str_ci_match(st->vfs[i].name, st->files_query)) {
            indices[n_items++] = i;
            if (st->vfs[i].is_dir) dir_cnt++;
            else {
                file_cnt++;
                total_bytes += st->vfs[i].size_bytes;
            }
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
    int preview_w = (has_sel && bw >= 360) ? (compact ? 162 : 206) : 0;
    int list_w = bw - preview_w - 14;
    int iy_start = cy + 80;
    int status_h = 24;
    int content_h = ch - 82 - status_h;
    if (content_h < 40) content_h = 40;

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
            if (ry + 30 > cy + ch - status_h - 4) break;
            int hov = (st->hover_mx >= bx + 10 && st->hover_mx <= bx + list_w - 2 && st->hover_my >= ry && st->hover_my <= ry + 28);
            ngfx_fill_rect(cr, bx + 10, ry, list_w - 12, 28, 7,
                           (sel || hov) ? ar : (dark ? 1 : 0),
                           (sel || hov) ? ag : (dark ? 1 : 0),
                           (sel || hov) ? ab : (dark ? 1 : 0),
                           sel ? 0.24 : (hov ? 0.10 : (slot % 2 == 0 ? 0.03 : 0.0)));
            if (sel) ngfx_stroke_rect(cr, bx + 10.5, ry + 0.5, list_w - 13, 27, 7, 1.0, ar, ag, ab, 0.72);
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
            if (n->is_dir) {
                int sub_cnt = 0;
                for (int c = 0; c < st->vfs_count; ++c) {
                    if (strcmp(st->vfs[c].parent, n->path) == 0) sub_cnt++;
                }
                snprintf(sz_buf, sizeof(sz_buf), "%d item%s", sub_cnt, sub_cnt == 1 ? "" : "s");
            } else {
                format_size(n, sz_buf, sizeof(sz_buf));
            }
            ngfx_text(cr, "JetBrains Mono", 0, 10.0, bx + list_w - 78, ry + 18, mut_r, mut_g, mut_b, 0.9, sz_buf);
            nature_add_hit(st, bx + 10, ry, list_w - 12, 28, HIT_FILES_NODE, i, 0);
        } else {
            int c = slot % col_count;
            int r = slot / col_count;
            int gx = bx + 12 + c * 114;
            int gy = iy_start + r * 96;
            if (gy + 88 > cy + ch - status_h - 4) break;
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

    /* Bottom Status Bar inside Files pane */
    int st_y = cy + ch - status_h;
    ngfx_fill_rect(cr, bx, st_y, list_w + 14, status_h, 0, pal.surface_r, pal.surface_g, pal.surface_b, 1.0);
    g_ngfx.set_source_rgba(cr, pal.border_r, pal.border_g, pal.border_b, pal.border_a);
    g_ngfx.move_to(cr, bx, st_y + 0.5);
    g_ngfx.line_to(cr, bx + list_w + 14, st_y + 0.5);
    g_ngfx.stroke(cr);

    char tot_buf[32], status_txt[96];
    format_bytes_total(total_bytes, tot_buf, sizeof(tot_buf));
    snprintf(status_txt, sizeof(status_txt), "%d folder%s · %d file%s · %s",
             dir_cnt, dir_cnt == 1 ? "" : "s",
             file_cnt, file_cnt == 1 ? "" : "s", tot_buf);
    ngfx_text_clipped(cr, "JetBrains Mono", 0, 9.8, bx + 12, st_y + 15.5, list_w - 8, mut_r, mut_g, mut_b, 0.94, status_txt);

    /* Right Inspector Panel */
    if (preview_w > 0) {
        VfsNode *sn = &st->vfs[st->files_selected_idx];
        int px = bx + list_w + 14;
        ngfx_fill_rect(cr, px, cy + 43, preview_w, ch - 43, 0, pal.sidebar_r, pal.sidebar_g, pal.sidebar_b, 1.0);
        g_ngfx.set_source_rgba(cr, pal.border_r, pal.border_g, pal.border_b, pal.border_a);
        g_ngfx.move_to(cr, px + 0.5, cy + 43);
        g_ngfx.line_to(cr, px + 0.5, cy + ch);
        g_ngfx.stroke(cr);

        draw_file_glyph(cr, st, sn, px + (preview_w - 130) * 0.5, cy + 52, 130, 72, ar, ag, ab, dark);
        ngfx_text_clipped(cr, "Plus Jakarta Sans", 1, 12.2, px + 12, cy + 142, preview_w - 24, fg_r, fg_g, fg_b, 1.0, sn->name);

        char meta_buf[64], sz_buf[24];
        format_size(sn, sz_buf, sizeof(sz_buf));
        snprintf(meta_buf, sizeof(meta_buf), "%s · %s", sn->is_dir ? "Folder" : (sn->mime_tag[0] ? sn->mime_tag : "File"), sz_buf);
        ngfx_text(cr, "JetBrains Mono", 0, 9.6, px + 12, cy + 159, mut_r, mut_g, mut_b, 0.95, meta_buf);
        ngfx_text(cr, "JetBrains Mono", 0, 9.3, px + 12, cy + 173, mut_r, mut_g, mut_b, 0.75,
                  sn->modified[0] ? sn->modified : "Nov 25 2026");

        /* Rename / Duplicate Bar */
        int r_y = cy + 182;
        if (st->files_renaming) {
            int rb_w = preview_w - 68;
            int rf_focus = (st->active_text_target == 7);
            ngfx_fill_rect(cr, px + 10, r_y, rb_w, 24, 6, pal.card_r, pal.card_g, pal.card_b, 1.0);
            ngfx_stroke_rect(cr, px + 10.5, r_y + 0.5, rb_w - 1, 23, 6, rf_focus ? 1.2 : 0.9,
                             rf_focus ? ar : pal.border_r, rf_focus ? ag : pal.border_g, rf_focus ? ab : pal.border_b,
                             rf_focus ? 0.9 : pal.border_a);
            char rdisp[60];
            snprintf(rdisp, sizeof(rdisp), "%s%s", st->files_rename_buf, rf_focus ? "|" : "");
            ngfx_text_clipped(cr, "JetBrains Mono", 0, 9.8, px + 15, r_y + 15.5, rb_w - 10, fg_r, fg_g, fg_b, 1.0, rdisp);
            nature_add_hit(st, px + 10, r_y, rb_w, 24, HIT_FILES_RENAME_BOX, 0, 0);
            nature_add_text_rect(st, px + 10, r_y, rb_w, 24, 7);

            ngfx_fill_rect(cr, px + 14 + rb_w, r_y, 44, 24, 6, ar, ag, ab, 0.95);
            ngfx_text(cr, "Noto Sans", 1, 9.8, px + 22 + rb_w, r_y + 15.5, pal.on_acc_r, pal.on_acc_g, pal.on_acc_b, 1.0, "Save");
            nature_add_hit(st, px + 14 + rb_w, r_y, 44, 24, HIT_FILES_RENAME_APPLY, 0, 0);
        } else {
            int half_w = (preview_w - 26) / 2;
            ngfx_fill_rect(cr, px + 10, r_y, half_w, 24, 6, pal.elev_r, pal.elev_g, pal.elev_b, 0.95);
            ngfx_text_clipped(cr, "Noto Sans", 1, 10.0, px + 18, r_y + 15.5, half_w - 12, fg_r, fg_g, fg_b, 0.95, "Rename");
            nature_add_hit(st, px + 10, r_y, half_w, 24, HIT_FILES_RENAME_START, st->files_selected_idx, 0);

            ngfx_fill_rect(cr, px + 16 + half_w, r_y, half_w, 24, 6, pal.elev_r, pal.elev_g, pal.elev_b, 0.95);
            ngfx_text_clipped(cr, "Noto Sans", 1, 10.0, px + 22 + half_w, r_y + 15.5, half_w - 10, fg_r, fg_g, fg_b, 0.95, "Duplicate");
            nature_add_hit(st, px + 16 + half_w, r_y, half_w, 24, HIT_FILES_DUPLICATE, st->files_selected_idx, 0);
        }

        int is_runnable = (!sn->is_dir && (strcmp(sn->mime_tag, "JS") == 0 || strcmp(sn->mime_tag, "PY") == 0));
        int bottom_reserved = (sn->image_idx >= 0 || is_runnable) ? 114 : 82;
        if (sn->content[0] && ch > 350) {
            int box_y = cy + 214;
            int box_h = ch - (214 + bottom_reserved);
            if (box_h > 36) {
                ngfx_fill_rect(cr, px + 10, box_y, preview_w - 20, box_h, 8, pal.card_r, pal.card_g, pal.card_b, 1.0);
                char line_buf[64];
                const char *p = sn->content;
                int line_y = box_y + 15;
                while (*p && line_y < box_y + box_h - 8) {
                    int k = 0;
                    while (p[k] && p[k] != '\n' && k < 23) { line_buf[k] = p[k]; k++; }
                    line_buf[k] = '\0';
                    ngfx_text(cr, "JetBrains Mono", 0, 9.0, px + 16, line_y, mut_r, mut_g, mut_b, 0.95, line_buf);
                    line_y += 14;
                    p += k;
                    if (*p == '\n') p++;
                    else while (*p && *p != '\n') p++;
                    if (*p == '\n') p++;
                }
            }
        }

        int btn_y = cy + ch - 76;
        if (sn->image_idx >= 0) {
            ngfx_fill_rect(cr, px + 10, btn_y - 32, preview_w - 20, 26, 7, pal.elev_r, pal.elev_g, pal.elev_b, 0.95);
            ngfx_text_clipped(cr, "Noto Sans", 1, 10.6, px + 26, btn_y - 15, preview_w - 40, fg_r, fg_g, fg_b, 1.0, "Set Wallpaper");
            nature_add_hit(st, px + 10, btn_y - 32, preview_w - 20, 26, HIT_FILES_SET_WP, sn->image_idx, 0);
        } else if (is_runnable) {
            const char *rlbl = (strcmp(sn->mime_tag, "JS") == 0) ? "Run with Node.js" : "Run with Python3";
            ngfx_fill_rect(cr, px + 10, btn_y - 32, preview_w - 20, 26, 7, pal.elev_r, pal.elev_g, pal.elev_b, 0.95);
            ngfx_stroke_rect(cr, px + 10.5, btn_y - 31.5, preview_w - 21, 25, 7, 0.8, ar, ag, ab, 0.55);
            ngfx_text_clipped(cr, "Noto Sans", 1, 10.4, px + 20, btn_y - 15, preview_w - 32,
                              dark ? pal.ar_hi : ar, dark ? pal.ag_hi : ag, dark ? pal.ab_hi : ab, 1.0, rlbl);
            nature_add_hit(st, px + 10, btn_y - 32, preview_w - 20, 26, HIT_FILES_RUN_TERM, st->files_selected_idx, 0);
        }
        const char *act_lbl = sn->is_dir ? "Open Folder" :
                              (sn->video_idx >= 0 ? "Play Video" :
                               (sn->audio_idx >= 0 ? "Play Audio" :
                                (sn->image_idx >= 0 ? "Open in Media" : "Read File")));
        ngfx_fill_rect(cr, px + 10, btn_y, preview_w - 20, 28, 7, ar, ag, ab, 0.94);
        double alw = ngfx_text_width(cr, "Noto Sans", 1, 10.6, act_lbl);
        ngfx_text(cr, "Noto Sans", 1, 10.6, px + 10 + ((preview_w - 20) - alw) * 0.5, btn_y + 18, pal.on_acc_r, pal.on_acc_g, pal.on_acc_b, 1, act_lbl);
        nature_add_hit(st, px + 10, btn_y, preview_w - 20, 28, HIT_FILES_OPEN_ACTION, st->files_selected_idx, 0);

        int armed_del = (st->files_confirm_delete_idx == st->files_selected_idx);
        const char *del_lbl = armed_del ? "Confirm Delete?" : "Delete Item";
        ngfx_fill_rect(cr, px + 10, cy + ch - 40, preview_w - 20, 28, 7,
                       pal.danger_r, pal.danger_g, pal.danger_b, armed_del ? 0.34 : 0.16);
        if (armed_del) {
            ngfx_stroke_rect(cr, px + 10.5, cy + ch - 39.5, preview_w - 21, 27, 7, 1.0, pal.danger_r, pal.danger_g, pal.danger_b, 0.85);
        }
        double dlw = ngfx_text_width(cr, "Noto Sans", 1, 10.6, del_lbl);
        ngfx_text(cr, "Noto Sans", 1, 10.6, px + 10 + ((preview_w - 20) - dlw) * 0.5, cy + ch - 22,
                  pal.danger_r, pal.danger_g, pal.danger_b, 1.0, del_lbl);
        nature_add_hit(st, px + 10, cy + ch - 40, preview_w - 20, 28, HIT_FILES_DELETE, st->files_selected_idx, 0);
    }

    /* Inline Quick Text / Config / Source Reader Modal */
    if (st->files_viewer_open && st->files_viewer_idx >= 0 && st->files_viewer_idx < st->vfs_count) {
        VfsNode *vn = &st->vfs[st->files_viewer_idx];
        ngfx_fill_rect(cr, cx, cy, cw, ch, 0, 0.01, 0.02, 0.03, 0.72);
        int mw = cw - 48;
        if (mw > 540) mw = 540;
        int mh = ch - 44;
        if (mh > 360) mh = 360;
        int mx = cx + (cw - mw) / 2;
        int my = cy + (ch - mh) / 2;

        ngfx_shadow_rect(cr, mx, my, mw, mh, 12, 3.0, 0.55);
        ngfx_fill_rect(cr, mx, my, mw, mh, 12, pal.win_bg_r, pal.win_bg_g, pal.win_bg_b, 1.0);
        ngfx_stroke_rect(cr, mx + 0.5, my + 0.5, mw - 1, mh - 1, 12, 1.2, ar, ag, ab, 0.65);

        ngfx_fill_rect(cr, mx, my, mw, 38, 0, pal.surface_r, pal.surface_g, pal.surface_b, 1.0);
        char vsize[24];
        format_size(vn, vsize, sizeof(vsize));
        ngfx_text_clipped(cr, "JetBrains Mono", 1, 11.2, mx + 14, my + 24, mw - 270, fg_r, fg_g, fg_b, 1.0, vn->path);
        char vmeta[48];
        snprintf(vmeta, sizeof(vmeta), "%s · %s", vn->mime_tag[0] ? vn->mime_tag : "TXT", vsize);
        ngfx_text_clipped(cr, "JetBrains Mono", 0, 9.4, mx + mw - 252, my + 23.5, 102, mut_r, mut_g, mut_b, 0.90, vmeta);

        if (!st->files_editing) {
            ngfx_fill_rect(cr, mx + mw - 212, my + 7, 62, 24, 6, ar, ag, ab, 0.92);
            ngfx_text(cr, "Noto Sans", 1, 10.0, mx + mw - 198, my + 23, 1, 1, 1, 1, "Edit");
            nature_add_hit(st, mx + mw - 212, my + 7, 62, 24, HIT_FILES_EDIT_MODE, st->files_viewer_idx, 0);

            ngfx_fill_rect(cr, mx + mw - 144, my + 7, 66, 24, 6, pal.elev_r, pal.elev_g, pal.elev_b, 0.95);
            ngfx_stroke_rect(cr, mx + mw - 143.5, my + 7.5, 65, 23, 6, 0.8, pal.border_r, pal.border_g, pal.border_b, pal.border_a);
            ngfx_text(cr, "Noto Sans", 1, 10.0, mx + mw - 134, my + 23, fg_r, fg_g, fg_b, 0.95, "Terminal");
            nature_add_hit(st, mx + mw - 144, my + 7, 66, 24, HIT_FILES_OPEN_TERM, st->files_viewer_idx, 0);

            ngfx_fill_rect(cr, mx + mw - 72, my + 7, 60, 24, 6, pal.danger_r, pal.danger_g, pal.danger_b, 0.20);
            ngfx_text(cr, "Noto Sans", 1, 10.5, mx + mw - 56, my + 23, pal.danger_r, pal.danger_g, pal.danger_b, 1.0, "Close");
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
                double lr = fg_r, lg = fg_g, lb = fg_b;
                if ((lbuf[0] == '#' && lbuf[1] == ' ') || lbuf[0] == '[') {
                    lr = dark ? pal.ar_hi : ar; lg = dark ? pal.ag_hi : ag; lb = dark ? pal.ab_hi : ab;
                } else if (lbuf[0] == '#' || (lbuf[0] == '/' && (lbuf[1] == '/' || lbuf[1] == '*'))) {
                    lr = pal.success_r; lg = pal.success_g; lb = pal.success_b;
                } else if (strchr(lbuf, '=') != NULL && lbuf[0] != ' ') {
                    lr = pal.fg_sec_r; lg = pal.fg_sec_g; lb = pal.fg_sec_b;
                }
                ngfx_text_clipped(cr, "JetBrains Mono", (lbuf[0] == '#' && lbuf[1] == ' ') || lbuf[0] == '[',
                                  10.8, mx + 38, ty, mw - 52, lr, lg, lb, 0.96, lbuf);
                ty += 19;
                p += k;
                if (*p == '\n') p++;
                else while (*p && *p != '\n') p++;
                if (*p == '\n') p++;
            }
        } else {
            /* Interactive Live Note / Text Editor Mode */
            int ed_chars = (int)strlen(st->files_edit_buf);
            int ed_lines = st->files_edit_buf[0] ? 1 : 0;
            for (int k = 0; st->files_edit_buf[k]; ++k) {
                if (st->files_edit_buf[k] == '\n') ed_lines++;
            }
            if (mw > 420) {
                char estat[48];
                snprintf(estat, sizeof(estat), "%d / 767 · %dL", ed_chars, ed_lines);
                ngfx_fill_rect(cr, mx + mw - 306, my + 8, 100, 22, 6, pal.elev_r, pal.elev_g, pal.elev_b, 0.90);
                ngfx_text_clipped(cr, "JetBrains Mono", 0, 9.2, mx + mw - 299, my + 22.5, 90, mut_r, mut_g, mut_b, 0.95, estat);
            }

            ngfx_fill_rect(cr, mx + mw - 198, my + 7, 58, 24, 6, pal.success_r, pal.success_g, pal.success_b, 0.95);
            ngfx_text(cr, "Noto Sans", 1, 10.0, mx + mw - 186, my + 23, 1, 1, 1, 1, "Save");
            nature_add_hit(st, mx + mw - 198, my + 7, 58, 24, HIT_FILES_EDIT_SAVE, 0, 0);

            ngfx_fill_rect(cr, mx + mw - 134, my + 7, 58, 24, 6, pal.elev_r, pal.elev_g, pal.elev_b, 0.95);
            ngfx_stroke_rect(cr, mx + mw - 133.5, my + 7.5, 57, 23, 6, 0.8, pal.border_r, pal.border_g, pal.border_b, pal.border_a);
            ngfx_text(cr, "Noto Sans", 1, 10.0, mx + mw - 124, my + 23, fg_r, fg_g, fg_b, 0.95, "Cancel");
            nature_add_hit(st, mx + mw - 134, my + 7, 58, 24, HIT_FILES_EDIT_CANCEL, 0, 0);

            ngfx_fill_rect(cr, mx + mw - 70, my + 7, 58, 24, 6, pal.danger_r, pal.danger_g, pal.danger_b, 0.20);
            ngfx_text(cr, "Noto Sans", 1, 10.0, mx + mw - 54, my + 23, pal.danger_r, pal.danger_g, pal.danger_b, 1.0, "Close");
            nature_add_hit(st, mx + mw - 70, my + 7, 58, 24, HIT_FILES_CLOSE_VIEWER, 0, 0);

            /* Editor Text Area */
            int ed_x = mx + 12, ed_y = my + 46, ed_w = mw - 24, ed_h = mh - 56;
            ngfx_fill_rect(cr, ed_x, ed_y, ed_w, ed_h, 8, pal.card_r, pal.card_g, pal.card_b, 0.98);
            ngfx_stroke_rect(cr, ed_x + 0.5, ed_y + 0.5, ed_w - 1, ed_h - 1, 8, 1.2, ar, ag, ab, 0.85);

            nature_add_hit(st, ed_x, ed_y, ed_w, ed_h, HIT_FILES_EDIT_BOX, 0, 0);
            nature_add_text_rect(st, ed_x, ed_y, ed_w, ed_h, 9);

            int ty = ed_y + 20;
            const char *p = st->files_edit_buf;
            char lbuf[96];
            int lno = 1;
            double last_tx = ed_x + 42;
            int last_ty = ty;

            while (*p && ty < ed_y + ed_h - 16) {
                int k = 0;
                while (p[k] && p[k] != '\n' && k < 68) { lbuf[k] = p[k]; k++; }
                lbuf[k] = '\0';
                char num[16];
                snprintf(num, sizeof(num), "%2d", lno++);
                ngfx_text(cr, "JetBrains Mono", 0, 10.0, ed_x + 10, ty, mut_r, mut_g, mut_b, 0.60, num);
                ngfx_text_clipped(cr, "JetBrains Mono", 0, 10.8, ed_x + 40, ty, ed_w - 54, fg_r, fg_g, fg_b, 0.98, lbuf);
                double lw = ngfx_text_width(cr, "JetBrains Mono", 0, 10.8, lbuf);
                last_tx = ed_x + 40 + lw;
                last_ty = ty;
                ty += 19;
                p += k;
                if (*p == '\n') p++;
                else while (*p && *p != '\n') p++;
                if (*p == '\n') p++;
            }
            if (!st->files_edit_buf[0]) {
                ngfx_text(cr, "JetBrains Mono", 0, 10.0, ed_x + 10, ty, mut_r, mut_g, mut_b, 0.60, " 1");
                ngfx_text(cr, "Noto Sans", 0, 10.6, ed_x + 40, ty, mut_r, mut_g, mut_b, 0.70, "Type your notes or document text here...");
                last_tx = ed_x + 40;
                last_ty = ty;
            }
            /* Active pulsing editor cursor */
            ngfx_fill_rect(cr, last_tx + 2, last_ty - 11, 2.2, 13, 1, ar, ag, ab, 0.95);
        }
    }
}

static void open_vfs_node(NatureOSState *st, int idx) {
    if (idx < 0 || idx >= st->vfs_count) return;
    VfsNode *n = &st->vfs[idx];
    if (n->is_dir) {
        snprintf(st->files_cwd, sizeof(st->files_cwd), "%s", n->path);
        st->files_selected_idx = -1;
        st->files_confirm_delete_idx = -1;
        st->files_renaming = 0;
        st->files_scroll = 0;
        st->files_query[0] = '\0';
    } else if (n->video_idx >= 0) {
        st->media_mode = 1;
        st->media_folder_idx = 2;
        st->video_selected = n->video_idx % MAX_VIDEO_CLIPS;
        st->video_playing = 1;
        st->video_progress_ms = 0;
        nature_open_app(st, APP_MEDIA);
    } else if (n->image_idx >= 0) {
        st->media_mode = 0;
        st->media_folder_idx = 1;
        st->media_grid_mode = 0;
        st->media_selected = n->image_idx;
        st->media_zoom_pct = 100;
        st->media_rotation = 0;
        nature_open_app(st, APP_MEDIA);
    } else if (n->audio_idx >= 0) {
        st->media_mode = 2;
        st->media_folder_idx = 3;
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
    if (hit->action_id != HIT_FILES_DELETE) {
        st->files_confirm_delete_idx = -1;
    }
    if (hit->action_id == HIT_FILES_PLACE) {
        snprintf(st->files_cwd, sizeof(st->files_cwd), "%s", PLACES_PATHS[hit->arg0]);
        st->files_selected_idx = -1;
        st->files_renaming = 0;
        st->files_scroll = 0;
        st->files_query[0] = '\0';
    } else if (hit->action_id == HIT_FILES_VIEW_MODE) {
        st->files_list_mode = !st->files_list_mode;
        st->files_scroll = 0;
        nature_os_save_state(st);
    } else if (hit->action_id == HIT_FILES_SORT) {
        st->files_sort_mode = !st->files_sort_mode;
        nature_os_save_state(st);
    } else if (hit->action_id == HIT_FILES_HIDDEN_TOGGLE) {
        st->files_show_hidden = !st->files_show_hidden;
        st->files_scroll = 0;
        nature_os_save_state(st);
    } else if (hit->action_id == HIT_FILES_BREADCRUMB) {
        int end_off = hit->arg0;
        int cur_len = (int)strlen(st->files_cwd);
        if (end_off >= 1 && end_off <= cur_len) {
            st->files_cwd[end_off] = '\0';
            st->files_selected_idx = -1;
            st->files_renaming = 0;
            st->files_scroll = 0;
            st->files_query[0] = '\0';
        }
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
        st->files_renaming = 0;
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
            snprintf(n->path, sizeof(n->path), "%.127s", path);
            snprintf(n->parent, sizeof(n->parent), "%s", st->files_cwd);
            snprintf(n->mime_tag, sizeof(n->mime_tag), "DIR");
            snprintf(n->modified, sizeof(n->modified), "Just now");
            n->is_dir = 1; n->size_bytes = 4096; n->image_idx = -1; n->audio_idx = -1; n->video_idx = -1;
            st->files_selected_idx = st->vfs_count - 1;
            st->files_renaming = 0;
            nature_os_save_state(st);
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
            snprintf(n->path, sizeof(n->path), "%.127s", path);
            snprintf(n->parent, sizeof(n->parent), "%s", st->files_cwd);
            snprintf(n->mime_tag, sizeof(n->mime_tag), "TXT");
            snprintf(n->modified, sizeof(n->modified), "Just now");
            snprintf(n->content, sizeof(n->content), "Nature OS Note #%d\nCreated in Files Manager.\nReady to edit.", idx);
            n->is_dir = 0; n->size_bytes = 128; n->image_idx = -1; n->audio_idx = -1; n->video_idx = -1;
            st->files_selected_idx = st->vfs_count - 1;
            st->files_renaming = 0;
            st->files_viewer_open = 1;
            st->files_viewer_idx = st->vfs_count - 1;
            st->files_editing = 1;
            st->active_text_target = 9;
            snprintf(st->files_edit_buf, sizeof(st->files_edit_buf), "%s", n->content);
            nature_notify(st, "Files", "Created new note. Start typing to edit.");
            nature_os_save_state(st);
        }
    } else if (hit->action_id == HIT_FILES_NODE) {
        int idx = hit->arg0;
        if (idx >= 0 && idx < st->vfs_count) {
            if (st->files_selected_idx == idx) {
                open_vfs_node(st, idx);
            } else {
                st->files_selected_idx = idx;
                st->files_renaming = 0;
            }
        }
    } else if (hit->action_id == HIT_FILES_OPEN_ACTION) {
        open_vfs_node(st, hit->arg0);
    } else if (hit->action_id == HIT_FILES_SET_WP) {
        nature_os_set_wallpaper_from_media(st, hit->arg0);
        nature_os_save_state(st);
    } else if (hit->action_id == HIT_FILES_RENAME_START) {
        int idx = hit->arg0;
        if (idx >= 0 && idx < st->vfs_count) {
            st->files_selected_idx = idx;
            st->files_renaming = 1;
            snprintf(st->files_rename_buf, sizeof(st->files_rename_buf), "%s", st->vfs[idx].name);
            st->active_text_target = 7;
            st->input_focus_pulse = 1;
        }
    } else if (hit->action_id == HIT_FILES_RENAME_BOX) {
        st->active_text_target = 7;
        st->input_focus_pulse = 1;
    } else if (hit->action_id == HIT_FILES_RENAME_APPLY) {
        apply_rename(st);
    } else if (hit->action_id == HIT_FILES_DUPLICATE) {
        int idx = hit->arg0;
        if (idx >= 0 && idx < st->vfs_count && st->vfs_count < MAX_VFS) {
            VfsNode src = st->vfs[idx];
            VfsNode *dst = &st->vfs[st->vfs_count++];
            *dst = src;
            const char *dot = strrchr(src.name, '.');
            if (!src.is_dir && dot && dot != src.name) {
                int stem = (int)(dot - src.name);
                if (stem > 32) stem = 32;
                snprintf(dst->name, sizeof(dst->name), "%.*s-copy%.12s", stem, src.name, dot);
            } else {
                snprintf(dst->name, sizeof(dst->name), "%.38s-copy", src.name);
            }
            char full[192];
            snprintf(full, sizeof(full), "%s/%s", strcmp(src.parent, "/") == 0 ? "" : src.parent, dst->name);
            snprintf(dst->path, sizeof(dst->path), "%.127s", full);
            snprintf(dst->modified, sizeof(dst->modified), "Just now");
            st->files_selected_idx = st->vfs_count - 1;
            nature_notify(st, "Files", "Created duplicate copy.");
            nature_os_save_state(st);
        }
    } else if (hit->action_id == HIT_FILES_EDIT_MODE) {
        int idx = hit->arg0;
        if (idx >= 0 && idx < st->vfs_count) {
            st->files_viewer_idx = idx;
            st->files_viewer_open = 1;
            st->files_editing = 1;
            st->active_text_target = 9;
            snprintf(st->files_edit_buf, sizeof(st->files_edit_buf), "%s", st->vfs[idx].content);
            st->dirty = 1;
        }
    } else if (hit->action_id == HIT_FILES_EDIT_SAVE) {
        int idx = st->files_viewer_idx;
        if (idx >= 0 && idx < st->vfs_count) {
            snprintf(st->vfs[idx].content, sizeof(st->vfs[idx].content), "%s", st->files_edit_buf);
            st->vfs[idx].size_bytes = (int)strlen(st->vfs[idx].content);
            snprintf(st->vfs[idx].modified, sizeof(st->vfs[idx].modified), "Just now");
            st->files_editing = 0;
            st->active_text_target = 0;
            nature_os_save_state(st);
            nature_notify(st, "Files", "Note saved successfully.");
            st->dirty = 1;
        }
    } else if (hit->action_id == HIT_FILES_EDIT_CANCEL) {
        st->files_editing = 0;
        st->active_text_target = 0;
        st->dirty = 1;
    } else if (hit->action_id == HIT_FILES_EDIT_BOX) {
        st->active_text_target = 9;
        st->dirty = 1;
    } else if (hit->action_id == HIT_FILES_CLOSE_VIEWER) {
        st->files_viewer_open = 0;
        st->files_editing = 0;
        if (st->active_text_target == 9) st->active_text_target = 0;
        st->dirty = 1;
    } else if (hit->action_id == HIT_FILES_OPEN_TERM) {
        int idx = hit->arg0;
        if (idx >= 0 && idx < st->vfs_count) {
            st->files_viewer_open = 0;
            st->files_editing = 0;
            snprintf(st->term_cwd, sizeof(st->term_cwd), "%s", st->vfs[idx].parent);
            nature_open_app(st, APP_TERMINAL);
            char cmd[160];
            snprintf(cmd, sizeof(cmd), "cat %.120s", st->vfs[idx].path);
            nature_terminal_exec(st, cmd);
        }
    } else if (hit->action_id == HIT_FILES_RUN_TERM) {
        int idx = hit->arg0;
        if (idx >= 0 && idx < st->vfs_count) {
            st->files_viewer_open = 0;
            st->files_editing = 0;
            snprintf(st->term_cwd, sizeof(st->term_cwd), "%s", st->vfs[idx].parent);
            nature_open_app(st, APP_TERMINAL);
            char cmd[160];
            const char *runner = (strcmp(st->vfs[idx].mime_tag, "PY") == 0) ? "python3" : "node";
            snprintf(cmd, sizeof(cmd), "%s %.120s", runner, st->vfs[idx].path);
            nature_terminal_exec(st, cmd);
        }
    } else if (hit->action_id == HIT_FILES_OPEN_MEDIA) {
        open_vfs_node(st, hit->arg0);
    } else if (hit->action_id == HIT_FILES_DELETE) {
        int idx = hit->arg0;
        if (idx >= 0 && idx < st->vfs_count) {
            if (st->files_confirm_delete_idx != idx) {
                st->files_confirm_delete_idx = idx;
            } else {
                for (int j = idx; j < st->vfs_count - 1; ++j) st->vfs[j] = st->vfs[j + 1];
                st->vfs_count--;
                st->files_selected_idx = -1;
                st->files_confirm_delete_idx = -1;
                st->files_viewer_open = 0;
                st->files_editing = 0;
                st->files_renaming = 0;
                nature_notify(st, "Files", "Item removed.");
                nature_os_save_state(st);
            }
        }
    }
}

void nature_files_key(NatureOSState *st, const char *key) {
    if (!key || !*key) return;
    if (strcmp(key, "Space") == 0) key = " ";
    if (st->active_text_target == 9 && st->files_editing) {
        if (strcmp(key, "Backspace") == 0) {
            size_t len = strlen(st->files_edit_buf);
            if (len > 0) st->files_edit_buf[len - 1] = '\0';
        } else if (strcmp(key, "Enter") == 0) {
            size_t len = strlen(st->files_edit_buf);
            if (len + 1 < sizeof(st->files_edit_buf)) {
                st->files_edit_buf[len] = '\n';
                st->files_edit_buf[len + 1] = '\0';
            }
        } else if (strcmp(key, "Escape") == 0) {
            st->files_editing = 0;
            st->active_text_target = 0;
        } else if (strlen(key) == 1 && (unsigned char)key[0] >= 32 && (unsigned char)key[0] < 127) {
            size_t len = strlen(st->files_edit_buf);
            if (len + 1 < sizeof(st->files_edit_buf)) {
                st->files_edit_buf[len] = key[0];
                st->files_edit_buf[len + 1] = '\0';
            }
        }
        st->dirty = 1;
        return;
    }

    if (st->active_text_target == 7) {
        if (strcmp(key, "Backspace") == 0) {
            size_t len = strlen(st->files_rename_buf);
            if (len > 0) st->files_rename_buf[len - 1] = '\0';
        } else if (strcmp(key, "Enter") == 0) {
            apply_rename(st);
        } else if (strcmp(key, "Escape") == 0) {
            st->files_renaming = 0;
            st->active_text_target = 0;
        } else if (strlen(key) == 1 && (unsigned char)key[0] >= 32 && (unsigned char)key[0] < 127 && key[0] != '/') {
            size_t len = strlen(st->files_rename_buf);
            if (len + 1 < sizeof(st->files_rename_buf)) {
                st->files_rename_buf[len] = key[0];
                st->files_rename_buf[len + 1] = '\0';
            }
        }
        return;
    }

    if (strcmp(key, "F2") == 0) {
        int idx = st->files_selected_idx;
        if (idx >= 0 && idx < st->vfs_count) {
            st->files_renaming = 1;
            snprintf(st->files_rename_buf, sizeof(st->files_rename_buf), "%s", st->vfs[idx].name);
            st->active_text_target = 7;
            st->input_focus_pulse = 1;
        }
        return;
    }

    if (strcmp(key, "Delete") == 0) {
        int idx = st->files_selected_idx;
        if (idx >= 0 && idx < st->vfs_count) {
            if (st->files_confirm_delete_idx != idx) {
                st->files_confirm_delete_idx = idx;
                nature_notify(st, "Files", "Press Delete again to confirm removal.");
            } else {
                for (int j = idx; j < st->vfs_count - 1; ++j) st->vfs[j] = st->vfs[j + 1];
                st->vfs_count--;
                st->files_selected_idx = -1;
                st->files_confirm_delete_idx = -1;
                st->files_viewer_open = 0;
                st->files_editing = 0;
                st->files_renaming = 0;
                nature_notify(st, "Files", "Item removed.");
                nature_os_save_state(st);
            }
        }
        return;
    }

    if (st->active_text_target == 0) {
        if (strcmp(key, "ArrowUp") == 0 || strcmp(key, "ArrowDown") == 0) {
            int indices[MAX_VFS];
            int n_items = 0;
            for (int i = 0; i < st->vfs_count; ++i) {
                if (!st->files_show_hidden && st->vfs[i].name[0] == '.') continue;
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
            if (n_items > 0) {
                int pos = -1;
                for (int k = 0; k < n_items; ++k) {
                    if (indices[k] == st->files_selected_idx) { pos = k; break; }
                }
                if (strcmp(key, "ArrowDown") == 0) {
                    pos = (pos < 0) ? 0 : (pos + 1 < n_items ? pos + 1 : 0);
                } else {
                    pos = (pos <= 0) ? (n_items - 1) : (pos - 1);
                }
                st->files_selected_idx = indices[pos];
            }
            return;
        }
        if ((strcmp(key, "Enter") == 0 || strcmp(key, " ") == 0) &&
            st->files_selected_idx >= 0 && st->files_selected_idx < st->vfs_count &&
            strcmp(st->vfs[st->files_selected_idx].parent, st->files_cwd) == 0) {
            open_vfs_node(st, st->files_selected_idx);
            return;
        }
    }

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
