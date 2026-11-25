#include "shell_internal.h"
#include <math.h>

cairo_surface_t *create_aurora_art(void) {
    int w = 960, h = 540;
    cairo_surface_t *s = g_ngfx.image_surface_create(CAIRO_FORMAT_ARGB32, w, h);
    cairo_t *cr = g_ngfx.create(s);

    ngfx_fill_gradient(cr, 0, 0, w, h, 0, 0.02, 0.05, 0.09, 1.0, 0.04, 0.14, 0.16, 1.0, 0);
    ngfx_fill_gradient(cr, 0, 60, w, 260, 0, 0.08, 0.72, 0.48, 0.36, 0.05, 0.25, 0.45, 0.0, 0);
    ngfx_fill_circle(cr, 720, 120, 96, 0.65, 0.92, 0.78, 0.14);
    ngfx_fill_circle(cr, 720, 120, 42, 0.85, 0.98, 0.92, 0.45);

    g_ngfx.set_source_rgba(cr, 0.02, 0.06, 0.05, 0.95);
    g_ngfx.move_to(cr, 0, h);
    g_ngfx.line_to(cr, 0, 390);
    g_ngfx.line_to(cr, 180, 310);
    g_ngfx.line_to(cr, 370, 385);
    g_ngfx.line_to(cr, 560, 275);
    g_ngfx.line_to(cr, 780, 360);
    g_ngfx.line_to(cr, 960, 300);
    g_ngfx.line_to(cr, 960, h);
    g_ngfx.close_path(cr);
    g_ngfx.fill(cr);

    g_ngfx.destroy(cr);
    return s;
}

cairo_surface_t *create_canopy_rings_art(void) {
    int w = 960, h = 540;
    cairo_surface_t *s = g_ngfx.image_surface_create(CAIRO_FORMAT_ARGB32, w, h);
    cairo_t *cr = g_ngfx.create(s);

    ngfx_fill_gradient(cr, 0, 0, w, h, 0, 0.04, 0.09, 0.07, 1.0, 0.09, 0.22, 0.16, 1.0, 1);
    for (int r = 240; r >= 40; r -= 32) {
        g_ngfx.set_source_rgba(cr, 0.24, 0.78, 0.52, 0.14 + (240 - r) * 0.0012);
        g_ngfx.set_line_width(cr, 2.2);
        g_ngfx.new_sub_path(cr);
        g_ngfx.arc(cr, w * 0.5, h * 0.5, r, 0, 6.28318);
        g_ngfx.stroke(cr);
    }
    ngfx_fill_circle(cr, w * 0.5, h * 0.5, 28, 0.35, 0.90, 0.60, 0.85);
    g_ngfx.destroy(cr);
    return s;
}

void ensure_cached_wallpaper(NatureOSState *st, int SW, int SH) {
    int wp_idx = st->cfg.wallpaper_idx;
    if (wp_idx < 0) wp_idx = 0;
    if (wp_idx > MAX_WALLPAPERS) wp_idx = MAX_WALLPAPERS;
    int dark = st->cfg.dark_mode;
    int fam = st->cfg.theme_family;
    int cust = st->custom_wp_media_idx;

    if (st->cached_wp && st->cached_wp_w == SW && st->cached_wp_h == SH &&
        st->cached_wp_idx == wp_idx && st->cached_wp_dark == dark &&
        st->cached_wp_family == fam && st->cached_wp_custom == cust) {
        return;
    }
    if (st->cached_wp) {
        g_ngfx.surface_destroy(st->cached_wp);
        st->cached_wp = NULL;
    }
    st->cached_wp = g_ngfx.image_surface_create(CAIRO_FORMAT_ARGB32, SW, SH);
    cairo_t *wcr = g_ngfx.create(st->cached_wp);
    cairo_surface_t *wp = NULL;
    if (wp_idx == MAX_WALLPAPERS && cust >= 0 && cust < MAX_MEDIA_IMAGES) {
        wp = st->media_images[cust];
    } else if (wp_idx >= 0 && wp_idx < MAX_WALLPAPERS) {
        wp = st->wallpapers[wp_idx];
    }

    if (wp) {
        double wp_w = g_ngfx.image_surface_get_width(wp);
        double wp_h = g_ngfx.image_surface_get_height(wp);
        if (wp_w > 0 && wp_h > 0) {
            double sc_x = (double)SW / wp_w;
            double sc_y = (double)SH / wp_h;
            double scale = sc_x > sc_y ? sc_x : sc_y;
            double off_x = ((double)SW - wp_w * scale) * 0.5;
            double off_y = ((double)SH - wp_h * scale) * 0.5;
            g_ngfx.save(wcr);
            g_ngfx.translate(wcr, off_x, off_y);
            g_ngfx.scale(wcr, scale, scale);
            g_ngfx.set_source_surface(wcr, wp, 0, 0);
            g_ngfx.paint(wcr);
            g_ngfx.restore(wcr);
        }
    } else if (wp_idx == 7 || fam == THEME_HORIZON) {
        /* Horizon Slate multi-stop procedural geometric gradient wallpaper */
        if (dark) {
            ngfx_fill_gradient(wcr, 0, 0, SW, SH, 0, 0.025, 0.045, 0.110, 1.0, 0.065, 0.115, 0.240, 1.0, 0);
            ngfx_radial_glow(wcr, SW * 0.28, SH * 0.30, 10.0, SW * 0.55, 0.18, 0.42, 0.92, 0.26, 0.0);
            ngfx_radial_glow(wcr, SW * 0.78, SH * 0.68, 10.0, SW * 0.45, 0.38, 0.28, 0.88, 0.22, 0.0);
        } else {
            ngfx_fill_gradient(wcr, 0, 0, SW, SH, 0, 0.845, 0.905, 0.985, 1.0, 0.925, 0.955, 0.992, 1.0, 0);
            ngfx_radial_glow(wcr, SW * 0.25, SH * 0.28, 10.0, SW * 0.55, 0.22, 0.56, 0.98, 0.22, 0.0);
            ngfx_radial_glow(wcr, SW * 0.76, SH * 0.66, 10.0, SW * 0.45, 0.48, 0.42, 0.96, 0.16, 0.0);
        }
        /* Crisp subtle Horizon architectural grid + mountain silhouette */
        g_ngfx.set_source_rgba(wcr, dark ? 0.55 : 0.20, dark ? 0.72 : 0.36, dark ? 0.98 : 0.65, dark ? 0.055 : 0.065);
        g_ngfx.set_line_width(wcr, 1.0);
        for (int gx = 0; gx < SW; gx += 64) {
            g_ngfx.move_to(wcr, gx + 0.5, 0); g_ngfx.line_to(wcr, gx + 0.5, SH);
        }
        for (int gy = 0; gy < SH; gy += 64) {
            g_ngfx.move_to(wcr, 0, gy + 0.5); g_ngfx.line_to(wcr, SW, gy + 0.5);
        }
        g_ngfx.stroke(wcr);
        g_ngfx.set_source_rgba(wcr, dark ? 0.03 : 0.68, dark ? 0.05 : 0.78, dark ? 0.12 : 0.90, dark ? 0.65 : 0.32);
        g_ngfx.move_to(wcr, 0, SH);
        g_ngfx.line_to(wcr, 0, SH * 0.72);
        g_ngfx.line_to(wcr, SW * 0.24, SH * 0.58);
        g_ngfx.line_to(wcr, SW * 0.48, SH * 0.69);
        g_ngfx.line_to(wcr, SW * 0.74, SH * 0.52);
        g_ngfx.line_to(wcr, SW, SH * 0.64);
        g_ngfx.line_to(wcr, SW, SH);
        g_ngfx.close_path(wcr);
        g_ngfx.fill(wcr);
    } else {
        ngfx_fill_rect(wcr, 0, 0, SW, SH, 0, dark ? 0.05 : 0.88, dark ? 0.09 : 0.94, dark ? 0.08 : 0.91, 1.0);
    }

    if (fam == THEME_HORIZON && wp) {
        ngfx_fill_rect(wcr, 0, 0, SW, SH, 0, 0.04, 0.08, 0.20, dark ? 0.26 : 0.06);
    } else if (!dark) {
        ngfx_fill_rect(wcr, 0, 0, SW, SH, 0, 0.95, 0.98, 0.96, 0.20);
    } else {
        ngfx_fill_rect(wcr, 0, 0, SW, SH, 0, 0.02, 0.04, 0.03, 0.24);
    }

    /* Atmospheric soft top & bottom scrim vignettes + radial center aura */
    ngfx_radial_glow(wcr, SW * 0.5, SH * 0.36, 20.0, (SW > SH ? SW : SH) * 0.45,
                     fam == THEME_HORIZON ? 0.08 : 0.06,
                     fam == THEME_HORIZON ? 0.14 : 0.14,
                     fam == THEME_HORIZON ? 0.28 : 0.11,
                     dark ? 0.20 : 0.08, 0.0);
    ngfx_fill_gradient(wcr, 0, 0, SW, 110, 0, 0.0, 0.02, 0.03, dark ? 0.44 : 0.18, 0.0, 0.0, 0.0, 0.0, 0);
    ngfx_fill_gradient(wcr, 0, SH - 120, SW, 120, 0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.02, 0.03, dark ? 0.48 : 0.20, 0);
    g_ngfx.destroy(wcr);
    st->cached_wp_w = SW;
    st->cached_wp_h = SH;
    st->cached_wp_idx = wp_idx;
    st->cached_wp_dark = dark;
    st->cached_wp_family = fam;
    st->cached_wp_custom = cust;
}

void draw_app_icon(cairo_t *cr, AppType app, double x, double y, double sz) {
    double pad = sz * 0.17;
    double ix = x + pad, iy = y + pad, iw = sz - pad * 2, ih = sz - pad * 2;
    double r = sz * 0.27;
    double cx = x + sz * 0.5, cy = y + sz * 0.5;

    g_ngfx.save(cr);
    g_ngfx.set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
    g_ngfx.set_line_join(cr, CAIRO_LINE_JOIN_ROUND);

    switch (app) {
        case APP_FILES: {
            ngfx_fill_gradient(cr, x, y, sz, sz, r, 0.12, 0.42, 0.29, 1.0, 0.06, 0.20, 0.15, 1.0, 0);
            ngfx_stroke_rect(cr, x + 0.5, y + 0.5, sz - 1, sz - 1, r, 1.0, 1.0, 1.0, 1.0, 0.20);
            ngfx_top_specular(cr, x, y, sz, r, 0.34);
            /* Folder back & tab */
            ngfx_fill_rect(cr, ix, iy + ih * 0.08, iw * 0.46, ih * 0.26, sz * 0.07, 0.86, 0.63, 0.18, 1.0);
            ngfx_fill_rect(cr, ix, iy + ih * 0.16, iw, ih * 0.28, sz * 0.09, 0.78, 0.56, 0.14, 1.0);
            /* Folder front flap with warm gold sheen */
            ngfx_fill_gradient(cr, ix, iy + ih * 0.26, iw, ih * 0.66, sz * 0.11,
                               0.99, 0.84, 0.38, 1.0, 0.87, 0.62, 0.15, 1.0, 0);
            ngfx_stroke_rect(cr, ix + 0.5, iy + ih * 0.26 + 0.5, iw - 1, ih * 0.66 - 1, sz * 0.11,
                             0.8, 1.0, 0.96, 0.76, 0.45);
            break;
        }
        case APP_BROWSER: {
            ngfx_fill_gradient(cr, x, y, sz, sz, r, 0.08, 0.36, 0.62, 1.0, 0.03, 0.14, 0.28, 1.0, 0);
            ngfx_stroke_rect(cr, x + 0.5, y + 0.5, sz - 1, sz - 1, r, 1.0, 1.0, 1.0, 1.0, 0.22);
            ngfx_top_specular(cr, x, y, sz, r, 0.36);
            ngfx_radial_glow(cr, cx, cy, 0.0, iw * 0.58, 0.24, 0.82, 0.96, 0.26, 0.0);
            /* Outer compass dial */
            g_ngfx.set_source_rgba(cr, 0.55, 0.92, 1.0, 0.90);
            g_ngfx.set_line_width(cr, sz * 0.048 > 1.3 ? sz * 0.048 : 1.3);
            g_ngfx.new_sub_path(cr);
            g_ngfx.arc(cr, cx, cy, iw * 0.44, 0, 6.28318);
            g_ngfx.stroke(cr);
            /* Rotated diamond compass needle */
            double rad = iw * 0.34;
            double side = iw * 0.11;
            /* North bright needle */
            g_ngfx.set_source_rgba(cr, 0.98, 0.45, 0.36, 0.98);
            g_ngfx.move_to(cr, cx + rad * 0.70, cy - rad * 0.70);
            g_ngfx.line_to(cr, cx - side * 0.70, cy - side * 0.70);
            g_ngfx.line_to(cr, cx + side * 0.70, cy + side * 0.70);
            g_ngfx.close_path(cr);
            g_ngfx.fill(cr);
            /* South frost needle */
            g_ngfx.set_source_rgba(cr, 0.92, 0.98, 1.0, 0.96);
            g_ngfx.move_to(cr, cx - rad * 0.70, cy + rad * 0.70);
            g_ngfx.line_to(cr, cx - side * 0.70, cy + side * 0.70);
            g_ngfx.line_to(cr, cx + side * 0.70, cy + side * 0.70);
            g_ngfx.close_path(cr);
            g_ngfx.fill(cr);
            ngfx_fill_circle(cr, cx, cy, iw * 0.075, 0.10, 0.22, 0.34, 1.0);
            break;
        }
        case APP_TERMINAL: {
            ngfx_fill_gradient(cr, x, y, sz, sz, r, 0.11, 0.15, 0.13, 1.0, 0.04, 0.06, 0.05, 1.0, 0);
            ngfx_stroke_rect(cr, x + 0.5, y + 0.5, sz - 1, sz - 1, r, 1.0, 0.34, 0.88, 0.56, 0.42);
            ngfx_top_specular(cr, x, y, sz, r, 0.28);
            /* Subtle phosphor glow + vector chevron & prompt cursor */
            ngfx_radial_glow(cr, x + sz * 0.40, cy, 0.0, sz * 0.42, 0.22, 0.90, 0.54, 0.14, 0.0);
            double lw = sz * 0.065 > 1.6 ? sz * 0.065 : 1.6;
            g_ngfx.set_line_width(cr, lw);
            g_ngfx.set_source_rgba(cr, 0.34, 0.95, 0.60, 1.0);
            g_ngfx.move_to(cr, ix + iw * 0.08, iy + ih * 0.28);
            g_ngfx.line_to(cr, ix + iw * 0.40, iy + ih * 0.50);
            g_ngfx.line_to(cr, ix + iw * 0.08, iy + ih * 0.72);
            g_ngfx.stroke(cr);
            ngfx_fill_rect(cr, ix + iw * 0.48, iy + ih * 0.64, iw * 0.40, lw * 1.15, 1.5, 0.90, 0.98, 0.94, 0.96);
            break;
        }
        case APP_STORE: {
            ngfx_fill_gradient(cr, x, y, sz, sz, r, 0.93, 0.42, 0.16, 1.0, 0.66, 0.21, 0.06, 1.0, 0);
            ngfx_stroke_rect(cr, x + 0.5, y + 0.5, sz - 1, sz - 1, r, 1.0, 1.0, 1.0, 1.0, 0.26);
            ngfx_top_specular(cr, x, y, sz, r, 0.36);
            /* Handle arch */
            g_ngfx.set_source_rgba(cr, 1.0, 0.95, 0.90, 0.95);
            g_ngfx.set_line_width(cr, sz * 0.055 > 1.5 ? sz * 0.055 : 1.5);
            g_ngfx.new_sub_path(cr);
            g_ngfx.arc(cr, cx, iy + ih * 0.30, iw * 0.21, 3.14159, 6.28318);
            g_ngfx.stroke(cr);
            /* Shopping tote body */
            ngfx_fill_gradient(cr, ix + iw * 0.11, iy + ih * 0.28, iw * 0.78, ih * 0.66, sz * 0.11,
                               1.0, 0.98, 0.95, 0.96, 0.95, 0.88, 0.82, 0.94, 0);
            /* Accent emblem circle inside tote */
            ngfx_fill_circle(cr, cx, iy + ih * 0.62, iw * 0.17, 0.88, 0.34, 0.12, 0.92);
            ngfx_fill_circle(cr, cx, iy + ih * 0.62, iw * 0.075, 1.0, 0.97, 0.93, 1.0);
            break;
        }
        case APP_MONITOR: {
            ngfx_fill_gradient(cr, x, y, sz, sz, r, 0.08, 0.19, 0.16, 1.0, 0.03, 0.08, 0.07, 1.0, 0);
            ngfx_stroke_rect(cr, x + 0.5, y + 0.5, sz - 1, sz - 1, r, 1.0, 0.28, 0.88, 0.62, 0.38);
            ngfx_top_specular(cr, x, y, sz, r, 0.30);
            /* Faint oscilloscope baseline */
            g_ngfx.set_source_rgba(cr, 0.25, 0.92, 0.62, 0.18);
            g_ngfx.set_line_width(cr, 1.0);
            g_ngfx.move_to(cr, ix, cy + sz * 0.06);
            g_ngfx.line_to(cr, ix + iw, cy + sz * 0.06);
            g_ngfx.stroke(cr);
            /* Glowing ECG trace */
            double lw = sz * 0.055 > 1.5 ? sz * 0.055 : 1.5;
            g_ngfx.set_source_rgba(cr, 0.24, 0.94, 0.64, 0.28);
            g_ngfx.set_line_width(cr, lw * 2.1);
            g_ngfx.move_to(cr, ix, y + sz * 0.56);
            g_ngfx.line_to(cr, ix + iw * 0.25, y + sz * 0.56);
            g_ngfx.line_to(cr, ix + iw * 0.42, y + sz * 0.24);
            g_ngfx.line_to(cr, ix + iw * 0.60, y + sz * 0.76);
            g_ngfx.line_to(cr, ix + iw * 0.76, y + sz * 0.44);
            g_ngfx.line_to(cr, ix + iw, y + sz * 0.44);
            g_ngfx.stroke(cr);

            g_ngfx.set_source_rgba(cr, 0.42, 0.98, 0.74, 1.0);
            g_ngfx.set_line_width(cr, lw);
            g_ngfx.move_to(cr, ix, y + sz * 0.56);
            g_ngfx.line_to(cr, ix + iw * 0.25, y + sz * 0.56);
            g_ngfx.line_to(cr, ix + iw * 0.42, y + sz * 0.24);
            g_ngfx.line_to(cr, ix + iw * 0.60, y + sz * 0.76);
            g_ngfx.line_to(cr, ix + iw * 0.76, y + sz * 0.44);
            g_ngfx.line_to(cr, ix + iw, y + sz * 0.44);
            g_ngfx.stroke(cr);
            ngfx_fill_circle(cr, ix + iw, y + sz * 0.44, lw * 0.9, 0.85, 1.0, 0.92, 1.0);
            break;
        }
        case APP_MEDIA: {
            ngfx_fill_gradient(cr, x, y, sz, sz, r, 0.16, 0.48, 0.40, 1.0, 0.06, 0.20, 0.19, 1.0, 0);
            ngfx_stroke_rect(cr, x + 0.5, y + 0.5, sz - 1, sz - 1, r, 1.0, 1.0, 1.0, 1.0, 0.24);
            ngfx_top_specular(cr, x, y, sz, r, 0.34);
            double fy = iy + ih * 0.08, fh = ih * 0.84;
            ngfx_fill_gradient(cr, ix, fy, iw, fh, sz * 0.11, 0.92, 0.98, 0.95, 0.96, 0.74, 0.90, 0.84, 0.96, 0);
            /* Warm sun orb */
            ngfx_fill_circle(cr, ix + iw * 0.72, fy + fh * 0.30, iw * 0.13, 0.98, 0.72, 0.26, 0.95);
            /* Layered mountains */
            g_ngfx.save(cr);
            ngfx_rounded_path(cr, ix, fy, iw, fh, sz * 0.11);
            g_ngfx.clip(cr);
            g_ngfx.set_source_rgba(cr, 0.18, 0.56, 0.38, 0.95);
            g_ngfx.move_to(cr, ix, fy + fh);
            g_ngfx.line_to(cr, ix + iw * 0.38, fy + fh * 0.40);
            g_ngfx.line_to(cr, ix + iw * 0.76, fy + fh);
            g_ngfx.close_path(cr);
            g_ngfx.fill(cr);

            g_ngfx.set_source_rgba(cr, 0.09, 0.36, 0.25, 0.98);
            g_ngfx.move_to(cr, ix + iw * 0.34, fy + fh);
            g_ngfx.line_to(cr, ix + iw * 0.70, fy + fh * 0.48);
            g_ngfx.line_to(cr, ix + iw, fy + fh * 0.80);
            g_ngfx.line_to(cr, ix + iw, fy + fh);
            g_ngfx.close_path(cr);
            g_ngfx.fill(cr);
            g_ngfx.restore(cr);
            break;
        }
        case APP_SETTINGS:
        default: {
            ngfx_fill_gradient(cr, x, y, sz, sz, r, 0.24, 0.30, 0.29, 1.0, 0.11, 0.14, 0.14, 1.0, 0);
            ngfx_stroke_rect(cr, x + 0.5, y + 0.5, sz - 1, sz - 1, r, 1.0, 1.0, 1.0, 1.0, 0.22);
            ngfx_top_specular(cr, x, y, sz, r, 0.32);
            /* 8-tooth precision gear */
            for (int t = 0; t < 8; ++t) {
                double ang = t * (3.14159265 / 4.0);
                g_ngfx.save(cr);
                g_ngfx.translate(cr, cx, cy);
                g_ngfx.rotate(cr, ang);
                ngfx_fill_rect(cr, -iw * 0.085, -iw * 0.44, iw * 0.17, iw * 0.88, sz * 0.03,
                               0.88, 0.93, 0.91, 0.94);
                g_ngfx.restore(cr);
            }
            ngfx_fill_circle(cr, cx, cy, iw * 0.31, 0.90, 0.95, 0.93, 1.0);
            ngfx_fill_circle(cr, cx, cy, iw * 0.18, 0.14, 0.18, 0.17, 1.0);
            ngfx_fill_circle(cr, cx, cy, iw * 0.11, 0.28, 0.72, 0.46, 0.96);
            break;
        }
    }
    g_ngfx.restore(cr);
}

void draw_folder_glyph(cairo_t *cr, double x, double y, double w, double h, double r, double g, double b, double a) {
    g_ngfx.save(cr);
    ngfx_fill_rect(cr, x, y, w * 0.44, h * 0.30, 2.5, r, g, b, a * 0.85);
    ngfx_fill_rect(cr, x, y + h * 0.22, w, h * 0.78, 3.5, r, g, b, a);
    g_ngfx.restore(cr);
}

void draw_file_glyph_simple(cairo_t *cr, double x, double y, double w, double h, double r, double g, double b, double a) {
    g_ngfx.save(cr);
    ngfx_fill_rect(cr, x, y, w, h, 3.0, r, g, b, a * 0.20);
    ngfx_stroke_rect(cr, x + 0.5, y + 0.5, w - 1, h - 1, 3.0, 1.0, r, g, b, a * 0.80);
    ngfx_fill_rect(cr, x + w * 0.20, y + h * 0.30, w * 0.60, 2.0, 1.0, r, g, b, a * 0.90);
    ngfx_fill_rect(cr, x + w * 0.20, y + h * 0.52, w * 0.45, 2.0, 1.0, r, g, b, a * 0.70);
    g_ngfx.restore(cr);
}
