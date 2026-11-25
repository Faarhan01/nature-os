#ifndef NATURE_GFX_H
#define NATURE_GFX_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _cairo cairo_t;
typedef struct _cairo_surface cairo_surface_t;
typedef struct _cairo_pattern cairo_pattern_t;

typedef enum {
    CAIRO_FORMAT_ARGB32 = 0,
    CAIRO_FORMAT_RGB24 = 1
} cairo_format_t;

typedef enum {
    CAIRO_FONT_SLANT_NORMAL = 0,
    CAIRO_FONT_SLANT_ITALIC = 1
} cairo_font_slant_t;

typedef enum {
    CAIRO_FONT_WEIGHT_NORMAL = 0,
    CAIRO_FONT_WEIGHT_BOLD = 1
} cairo_font_weight_t;

typedef enum {
    CAIRO_LINE_CAP_BUTT = 0,
    CAIRO_LINE_CAP_ROUND = 1,
    CAIRO_LINE_CAP_SQUARE = 2
} cairo_line_cap_t;

typedef enum {
    CAIRO_LINE_JOIN_MITER = 0,
    CAIRO_LINE_JOIN_ROUND = 1,
    CAIRO_LINE_JOIN_BEVEL = 2
} cairo_line_join_t;

typedef struct {
    double x_bearing;
    double y_bearing;
    double width;
    double height;
    double x_advance;
    double y_advance;
} cairo_text_extents_t;

typedef struct {
    cairo_surface_t *(*image_surface_create)(cairo_format_t format, int width, int height);
    cairo_surface_t *(*image_surface_create_from_png)(const char *filename);
    unsigned char *(*image_surface_get_data)(cairo_surface_t *surface);
    int (*image_surface_get_stride)(cairo_surface_t *surface);
    int (*image_surface_get_width)(cairo_surface_t *surface);
    int (*image_surface_get_height)(cairo_surface_t *surface);
    void (*surface_flush)(cairo_surface_t *surface);
    void (*surface_destroy)(cairo_surface_t *surface);

    cairo_t *(*create)(cairo_surface_t *target);
    void (*destroy)(cairo_t *cr);
    void (*save)(cairo_t *cr);
    void (*restore)(cairo_t *cr);
    void (*set_source_rgba)(cairo_t *cr, double r, double g, double b, double a);
    void (*set_source_surface)(cairo_t *cr, cairo_surface_t *surface, double x, double y);
    void (*set_source)(cairo_t *cr, cairo_pattern_t *source);
    void (*set_line_width)(cairo_t *cr, double width);
    void (*set_line_cap)(cairo_t *cr, cairo_line_cap_t cap);
    void (*set_line_join)(cairo_t *cr, cairo_line_join_t join);
    void (*paint)(cairo_t *cr);
    void (*fill)(cairo_t *cr);
    void (*fill_preserve)(cairo_t *cr);
    void (*stroke)(cairo_t *cr);
    void (*stroke_preserve)(cairo_t *cr);
    void (*clip)(cairo_t *cr);
    void (*new_sub_path)(cairo_t *cr);
    void (*close_path)(cairo_t *cr);
    void (*move_to)(cairo_t *cr, double x, double y);
    void (*line_to)(cairo_t *cr, double x, double y);
    void (*curve_to)(cairo_t *cr, double x1, double y1, double x2, double y2, double x3, double y3);
    void (*rectangle)(cairo_t *cr, double x, double y, double w, double h);
    void (*arc)(cairo_t *cr, double xc, double yc, double radius, double angle1, double angle2);
    void (*translate)(cairo_t *cr, double tx, double ty);
    void (*scale)(cairo_t *cr, double sx, double sy);
    void (*rotate)(cairo_t *cr, double angle);

    cairo_pattern_t *(*pattern_create_linear)(double x0, double y0, double x1, double y1);
    cairo_pattern_t *(*pattern_create_radial)(double cx0, double cy0, double r0, double cx1, double cy1, double r1);
    void (*pattern_add_color_stop_rgba)(cairo_pattern_t *pattern, double offset, double r, double g, double b, double a);
    void (*pattern_destroy)(cairo_pattern_t *pattern);

    void (*select_font_face)(cairo_t *cr, const char *family, cairo_font_slant_t slant, cairo_font_weight_t weight);
    void (*set_font_size)(cairo_t *cr, double size);
    void (*show_text)(cairo_t *cr, const char *utf8);
    void (*text_extents)(cairo_t *cr, const char *utf8, cairo_text_extents_t *extents);

    int (*z_compress2)(uint8_t *dest, unsigned long *destLen, const uint8_t *source, unsigned long sourceLen, int level);
} NatureGfxAPI;

extern NatureGfxAPI g_ngfx;

int nature_gfx_load(void);
void ngfx_rounded_path(cairo_t *cr, double x, double y, double w, double h, double r);
void ngfx_fill_rect(cairo_t *cr, double x, double y, double w, double h, double r,
                    double cr_r, double cr_g, double cr_b, double cr_a);
void ngfx_fill_gradient(cairo_t *cr, double x, double y, double w, double h, double r,
                        double r0, double g0, double b0, double a0,
                        double r1, double g1, double b1, double a1, int horizontal);
void ngfx_fill_circle(cairo_t *cr, double cx, double cy, double radius,
                      double r, double g, double b, double a);
void ngfx_stroke_rect(cairo_t *cr, double x, double y, double w, double h, double r,
                      double line_w, double cr_r, double cr_g, double cr_b, double cr_a);
void ngfx_shadow_rect(cairo_t *cr, double x, double y, double w, double h, double r,
                      double spread, double alpha);
void ngfx_radial_glow(cairo_t *cr, double cx, double cy, double r_inner, double r_outer,
                      double r, double g, double b, double a_inner, double a_outer);
void ngfx_top_specular(cairo_t *cr, double x, double y, double w, double r, double alpha);
void ngfx_toggle_switch(cairo_t *cr, double x, double y, double w, double h, int on,
                        double ar, double ag, double ab, int dark);
void ngfx_text(cairo_t *cr, const char *font, int bold, double size,
               double x, double y, double r, double g, double b, double a, const char *text);
double ngfx_text_width(cairo_t *cr, const char *font, int bold, double size, const char *text);
void ngfx_text_clipped(cairo_t *cr, const char *font, int bold, double size,
                       double x, double y, double max_w,
                       double r, double g, double b, double a, const char *text);

#ifdef __cplusplus
}
#endif

#endif
