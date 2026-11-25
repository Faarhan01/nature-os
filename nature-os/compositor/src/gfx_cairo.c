#include "nature_gfx.h"
#include <dlfcn.h>
#include <stdio.h>

NatureGfxAPI g_ngfx = {0};

#define LOAD_SYM(handle, field, sym) \
    do { \
        void *p = dlsym(handle, sym); \
        if (!p) { \
            fprintf(stderr, "nature-compositor: missing symbol %s: %s\n", sym, dlerror()); \
            return -1; \
        } \
        *(void **)(&g_ngfx.field) = p; \
    } while (0)

int nature_gfx_load(void) {
    void *cairo_lib = dlopen("libcairo.so.2", RTLD_LAZY | RTLD_GLOBAL);
    if (!cairo_lib) {
        fprintf(stderr, "nature-compositor: dlopen libcairo.so.2 failed: %s\n", dlerror());
        return -1;
    }

    void *z_lib = dlopen("libz.so.1", RTLD_LAZY | RTLD_GLOBAL);
    if (!z_lib) {
        fprintf(stderr, "nature-compositor: dlopen libz.so.1 failed: %s\n", dlerror());
        return -1;
    }

    LOAD_SYM(cairo_lib, image_surface_create, "cairo_image_surface_create");
    LOAD_SYM(cairo_lib, image_surface_create_from_png, "cairo_image_surface_create_from_png");
    LOAD_SYM(cairo_lib, image_surface_get_data, "cairo_image_surface_get_data");
    LOAD_SYM(cairo_lib, image_surface_get_stride, "cairo_image_surface_get_stride");
    LOAD_SYM(cairo_lib, image_surface_get_width, "cairo_image_surface_get_width");
    LOAD_SYM(cairo_lib, image_surface_get_height, "cairo_image_surface_get_height");
    LOAD_SYM(cairo_lib, surface_flush, "cairo_surface_flush");
    LOAD_SYM(cairo_lib, surface_destroy, "cairo_surface_destroy");

    LOAD_SYM(cairo_lib, create, "cairo_create");
    LOAD_SYM(cairo_lib, destroy, "cairo_destroy");
    LOAD_SYM(cairo_lib, save, "cairo_save");
    LOAD_SYM(cairo_lib, restore, "cairo_restore");
    LOAD_SYM(cairo_lib, set_source_rgba, "cairo_set_source_rgba");
    LOAD_SYM(cairo_lib, set_source_surface, "cairo_set_source_surface");
    LOAD_SYM(cairo_lib, set_source, "cairo_set_source");
    LOAD_SYM(cairo_lib, set_line_width, "cairo_set_line_width");
    LOAD_SYM(cairo_lib, set_line_cap, "cairo_set_line_cap");
    LOAD_SYM(cairo_lib, set_line_join, "cairo_set_line_join");
    LOAD_SYM(cairo_lib, paint, "cairo_paint");
    LOAD_SYM(cairo_lib, fill, "cairo_fill");
    LOAD_SYM(cairo_lib, fill_preserve, "cairo_fill_preserve");
    LOAD_SYM(cairo_lib, stroke, "cairo_stroke");
    LOAD_SYM(cairo_lib, stroke_preserve, "cairo_stroke_preserve");
    LOAD_SYM(cairo_lib, clip, "cairo_clip");
    LOAD_SYM(cairo_lib, new_sub_path, "cairo_new_sub_path");
    LOAD_SYM(cairo_lib, close_path, "cairo_close_path");
    LOAD_SYM(cairo_lib, move_to, "cairo_move_to");
    LOAD_SYM(cairo_lib, line_to, "cairo_line_to");
    LOAD_SYM(cairo_lib, curve_to, "cairo_curve_to");
    LOAD_SYM(cairo_lib, rectangle, "cairo_rectangle");
    LOAD_SYM(cairo_lib, arc, "cairo_arc");
    LOAD_SYM(cairo_lib, translate, "cairo_translate");
    LOAD_SYM(cairo_lib, scale, "cairo_scale");
    LOAD_SYM(cairo_lib, rotate, "cairo_rotate");

    LOAD_SYM(cairo_lib, pattern_create_linear, "cairo_pattern_create_linear");
    LOAD_SYM(cairo_lib, pattern_create_radial, "cairo_pattern_create_radial");
    LOAD_SYM(cairo_lib, pattern_add_color_stop_rgba, "cairo_pattern_add_color_stop_rgba");
    LOAD_SYM(cairo_lib, pattern_destroy, "cairo_pattern_destroy");

    LOAD_SYM(cairo_lib, select_font_face, "cairo_select_font_face");
    LOAD_SYM(cairo_lib, set_font_size, "cairo_set_font_size");
    LOAD_SYM(cairo_lib, show_text, "cairo_show_text");
    LOAD_SYM(cairo_lib, text_extents, "cairo_text_extents");

    LOAD_SYM(z_lib, z_compress2, "compress2");
    return 0;
}

void ngfx_rounded_path(cairo_t *cr, double x, double y, double w, double h, double r) {
    double deg = 3.14159265358979323846 / 180.0;
    if (r > w * 0.5) r = w * 0.5;
    if (r > h * 0.5) r = h * 0.5;
    if (r <= 0.0) {
        g_ngfx.rectangle(cr, x, y, w, h);
        return;
    }
    g_ngfx.new_sub_path(cr);
    g_ngfx.arc(cr, x + w - r, y + r, r, -90.0 * deg, 0.0 * deg);
    g_ngfx.arc(cr, x + w - r, y + h - r, r, 0.0 * deg, 90.0 * deg);
    g_ngfx.arc(cr, x + r, y + h - r, r, 90.0 * deg, 180.0 * deg);
    g_ngfx.arc(cr, x + r, y + r, r, 180.0 * deg, 270.0 * deg);
    g_ngfx.close_path(cr);
}

void ngfx_fill_rect(cairo_t *cr, double x, double y, double w, double h, double r,
                    double cr_r, double cr_g, double cr_b, double cr_a) {
    if (w <= 0.0 || h <= 0.0) return;
    ngfx_rounded_path(cr, x, y, w, h, r);
    g_ngfx.set_source_rgba(cr, cr_r, cr_g, cr_b, cr_a);
    g_ngfx.fill(cr);
}

void ngfx_fill_gradient(cairo_t *cr, double x, double y, double w, double h, double r,
                        double r0, double g0, double b0, double a0,
                        double r1, double g1, double b1, double a1, int horizontal) {
    if (w <= 0.0 || h <= 0.0) return;
    cairo_pattern_t *pat = horizontal
        ? g_ngfx.pattern_create_linear(x, y, x + w, y)
        : g_ngfx.pattern_create_linear(x, y, x, y + h);
    g_ngfx.pattern_add_color_stop_rgba(pat, 0.0, r0, g0, b0, a0);
    g_ngfx.pattern_add_color_stop_rgba(pat, 1.0, r1, g1, b1, a1);
    ngfx_rounded_path(cr, x, y, w, h, r);
    g_ngfx.set_source(cr, pat);
    g_ngfx.fill(cr);
    g_ngfx.pattern_destroy(pat);
}

void ngfx_fill_circle(cairo_t *cr, double cx, double cy, double radius,
                      double r, double g, double b, double a) {
    if (radius <= 0.0) return;
    g_ngfx.new_sub_path(cr);
    g_ngfx.arc(cr, cx, cy, radius, 0, 6.28318530718);
    g_ngfx.set_source_rgba(cr, r, g, b, a);
    g_ngfx.fill(cr);
}

void ngfx_stroke_rect(cairo_t *cr, double x, double y, double w, double h, double r,
                      double line_w, double cr_r, double cr_g, double cr_b, double cr_a) {
    if (w <= 0.0 || h <= 0.0) return;
    ngfx_rounded_path(cr, x, y, w, h, r);
    g_ngfx.set_line_width(cr, line_w);
    g_ngfx.set_source_rgba(cr, cr_r, cr_g, cr_b, cr_a);
    g_ngfx.stroke(cr);
}

void ngfx_shadow_rect(cairo_t *cr, double x, double y, double w, double h, double r,
                      double spread, double alpha) {
    if (spread <= 0.0 || alpha <= 0.0) return;
    int steps = (int)(spread + 2.0);
    if (steps < 3) steps = 3;
    if (steps > 6) steps = 6;

    /* Soft atmospheric penumbra */
    for (int s = steps; s >= 1; --s) {
        double t = (double)s / (double)steps;
        double pad = s * (spread * 0.72);
        double dy = s * (spread * 0.62);
        double a = alpha * 0.24 * (1.0 - t * 0.72);
        ngfx_fill_rect(cr, x - pad * 0.55, y + dy - pad * 0.15, w + pad * 1.1, h + pad * 0.85,
                       r + pad * 0.45, 0.0, 0.0, 0.0, a);
    }
    /* Crisp contact shadow directly beneath frame */
    ngfx_fill_rect(cr, x - 0.5, y + 1.5, w + 1.0, h + 1.0, r + 0.5, 0.0, 0.0, 0.0, alpha * 0.26);
}

void ngfx_radial_glow(cairo_t *cr, double cx, double cy, double r_inner, double r_outer,
                      double r, double g, double b, double a_inner, double a_outer) {
    if (r_outer <= 0.0) return;
    cairo_pattern_t *pat = g_ngfx.pattern_create_radial(cx, cy, r_inner, cx, cy, r_outer);
    g_ngfx.pattern_add_color_stop_rgba(pat, 0.0, r, g, b, a_inner);
    g_ngfx.pattern_add_color_stop_rgba(pat, 1.0, r, g, b, a_outer);
    g_ngfx.new_sub_path(cr);
    g_ngfx.arc(cr, cx, cy, r_outer, 0, 6.28318530718);
    g_ngfx.set_source(cr, pat);
    g_ngfx.fill(cr);
    g_ngfx.pattern_destroy(pat);
}

void ngfx_top_specular(cairo_t *cr, double x, double y, double w, double r, double alpha) {
    if (w <= 4.0 || alpha <= 0.0) return;
    double pad = r > 2.0 ? r * 0.65 : 2.0;
    double x0 = x + pad;
    double x1 = x + w - pad;
    if (x1 <= x0) return;
    cairo_pattern_t *pat = g_ngfx.pattern_create_linear(x0, y, x1, y);
    g_ngfx.pattern_add_color_stop_rgba(pat, 0.0, 1.0, 1.0, 1.0, 0.0);
    g_ngfx.pattern_add_color_stop_rgba(pat, 0.22, 1.0, 1.0, 1.0, alpha);
    g_ngfx.pattern_add_color_stop_rgba(pat, 0.78, 1.0, 1.0, 1.0, alpha);
    g_ngfx.pattern_add_color_stop_rgba(pat, 1.0, 1.0, 1.0, 1.0, 0.0);
    g_ngfx.set_source(cr, pat);
    g_ngfx.set_line_width(cr, 1.0);
    g_ngfx.move_to(cr, x0, y + 1.0);
    g_ngfx.line_to(cr, x1, y + 1.0);
    g_ngfx.stroke(cr);
    g_ngfx.pattern_destroy(pat);
}

void ngfx_toggle_switch(cairo_t *cr, double x, double y, double w, double h, int on,
                        double ar, double ag, double ab, int dark) {
    double rad = h * 0.5;
    if (on) {
        ngfx_fill_gradient(cr, x, y, w, h, rad,
                           ar * 1.15 > 1.0 ? 1.0 : ar * 1.15,
                           ag * 1.15 > 1.0 ? 1.0 : ag * 1.15,
                           ab * 1.15 > 1.0 ? 1.0 : ab * 1.15, 0.96,
                           ar * 0.82, ag * 0.82, ab * 0.82, 0.96, 0);
        ngfx_stroke_rect(cr, x + 0.5, y + 0.5, w - 1.0, h - 1.0, rad, 1.0, 1.0, 1.0, 1.0, 0.16);
    } else {
        ngfx_fill_rect(cr, x, y, w, h, rad,
                       dark ? 1.0 : 0.0, dark ? 1.0 : 0.0, dark ? 1.0 : 0.0, dark ? 0.14 : 0.16);
        ngfx_stroke_rect(cr, x + 0.5, y + 0.5, w - 1.0, h - 1.0, rad, 1.0,
                         dark ? 1.0 : 0.0, dark ? 1.0 : 0.0, dark ? 1.0 : 0.0, 0.14);
    }
    double kr = (h - 6.0) * 0.5;
    double kx = on ? (x + w - rad) : (x + rad);
    double ky = y + rad;
    ngfx_fill_circle(cr, kx, ky + 1.0, kr + 0.8, 0.0, 0.0, 0.0, 0.25);
    ngfx_fill_circle(cr, kx, ky, kr, 1.0, 1.0, 1.0, on ? 0.98 : (dark ? 0.84 : 0.98));
}

void ngfx_text(cairo_t *cr, const char *font, int bold, double size,
               double x, double y, double r, double g, double b, double a, const char *text) {
    if (!text || !*text) return;
    g_ngfx.select_font_face(cr, font ? font : "Noto Sans", CAIRO_FONT_SLANT_NORMAL,
                            bold ? CAIRO_FONT_WEIGHT_BOLD : CAIRO_FONT_WEIGHT_NORMAL);
    g_ngfx.set_font_size(cr, size);
    g_ngfx.set_source_rgba(cr, r, g, b, a);
    g_ngfx.move_to(cr, x, y);
    g_ngfx.show_text(cr, text);
}

double ngfx_text_width(cairo_t *cr, const char *font, int bold, double size, const char *text) {
    if (!text || !*text) return 0.0;
    cairo_text_extents_t ext;
    g_ngfx.select_font_face(cr, font ? font : "Noto Sans", CAIRO_FONT_SLANT_NORMAL,
                            bold ? CAIRO_FONT_WEIGHT_BOLD : CAIRO_FONT_WEIGHT_NORMAL);
    g_ngfx.set_font_size(cr, size);
    g_ngfx.text_extents(cr, text, &ext);
    return ext.x_advance;
}

void ngfx_text_clipped(cairo_t *cr, const char *font, int bold, double size,
                       double x, double y, double max_w,
                       double r, double g, double b, double a, const char *text) {
    if (!text || !*text) return;
    g_ngfx.save(cr);
    g_ngfx.rectangle(cr, x - 2.0, y - size - 4.0, max_w + 4.0, size + 10.0);
    g_ngfx.clip(cr);
    ngfx_text(cr, font, bold, size, x, y, r, g, b, a, text);
    g_ngfx.restore(cr);
}
