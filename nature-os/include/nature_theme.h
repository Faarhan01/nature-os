#ifndef NATURE_THEME_H
#define NATURE_THEME_H

#include "nature_os.h"

#ifdef __cplusplus
extern "C" {
#endif

#define NTHEME_RADIUS_WIN  12.0
#define NTHEME_RADIUS_CARD 10.0
#define NTHEME_RADIUS_BTN   7.0
#define NTHEME_RADIUS_PILL 12.0
#define NTHEME_RADIUS_DOCK 16.0
#define NTHEME_TOPBAR_H    34.0
#define NTHEME_TOPBAR_R    17.0

/* UI Component Primitives with unified token styling */
void ntheme_draw_card(cairo_t *cr, double x, double y, double w, double h, double r,
                      const NaturePalette *pal, int hovered, int selected);

void ntheme_draw_glass_pill(cairo_t *cr, double x, double y, double w, double h, double r,
                            const NaturePalette *pal, int active, const char *text);

void ntheme_draw_button(cairo_t *cr, double x, double y, double w, double h, double r,
                        const NaturePalette *pal, int is_accent, int hovered, const char *label);

void ntheme_draw_badge(cairo_t *cr, double x, double y, double w, double h, double r,
                       const NaturePalette *pal, const char *label, int is_accent);

void ntheme_draw_slider(cairo_t *cr, double x, double y, double w, double h, double val_pct,
                        const NaturePalette *pal, int is_accent);

void ntheme_draw_avatar(cairo_t *cr, cairo_surface_t *img, double x, double y, double size,
                        const NaturePalette *pal);

void ntheme_draw_modal_scrim(cairo_t *cr, double w, double h, double alpha);

#ifdef __cplusplus
}
#endif

#endif /* NATURE_THEME_H */
