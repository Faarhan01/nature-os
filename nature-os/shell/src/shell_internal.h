#ifndef SHELL_INTERNAL_H
#define SHELL_INTERNAL_H

#include "nature_os.h"
#include "nature_theme.h"
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Common Utilities */
int64_t get_current_time_ms(void);
void format_clock_str(const NatureOSState *st, const struct tm *tm_info, int include_date, char *buf, size_t sz);
int str_contains_ci(const char *haystack, const char *needle);
int app_matches_launcher_cat(AppType app, int cat);
extern const char *APP_NAMES[APP_COUNT];

/* Icons & Artwork Subsystem */
cairo_surface_t *create_aurora_art(void);
cairo_surface_t *create_canopy_rings_art(void);
void ensure_cached_wallpaper(NatureOSState *st, int w, int h);
void draw_app_icon(cairo_t *cr, AppType app, double x, double y, double size);
void draw_folder_glyph(cairo_t *cr, double x, double y, double w, double h, double r, double g, double b, double a);
void draw_file_glyph_simple(cairo_t *cr, double x, double y, double w, double h, double r, double g, double b, double a);
void draw_wifi_glyph(cairo_t *cr, double cx, double cy, double r, double g, double b, double a);
void draw_speaker_glyph(cairo_t *cr, double cx, double cy, int muted, double r, double g, double b, double a);
void draw_sun_glyph(cairo_t *cr, double cx, double cy, double r, double g, double b, double a);

/* Window Manager & Input Dispatch */
typedef struct {
    int kind;     /* 0=App, 1=File, 2=Setting, 3=Action */
    int arg0;     /* app_idx / vfs_idx / setting_sec / action_id */
    char title[48];
    char subtitle[64];
    char tag[12];
} SpotlightItem;

void clamp_window_to_screen(NatureOSState *st, OSWindow *w);
void render_window(NatureOSState *st, cairo_t *cr, OSWindow *w, const NaturePalette *pal);
int wm_collect_alt_tab_wins(const NatureOSState *st, int *out_indices);
void wm_cycle_alt_tab(NatureOSState *st, int step);
void wm_commit_alt_tab(NatureOSState *st);
int wm_collect_spotlight(const NatureOSState *st, SpotlightItem *out, int max_items);
void wm_activate_spotlight(NatureOSState *st, const SpotlightItem *item);

/* Shell Overlays, Popovers & Dialogs */
void render_quick_settings_popover(NatureOSState *st, cairo_t *cr, const NaturePalette *pal);
void render_calendar_popover(NatureOSState *st, cairo_t *cr, const NaturePalette *pal);
void render_launcher_overlay(NatureOSState *st, cairo_t *cr, const NaturePalette *pal);
void render_overview_overlay(NatureOSState *st, cairo_t *cr, const NaturePalette *pal);
void render_desktop_ctx_menu(NatureOSState *st, cairo_t *cr, const NaturePalette *pal);
void render_lock_screen(NatureOSState *st, cairo_t *cr, const NaturePalette *pal);
void render_power_dialog(NatureOSState *st, cairo_t *cr, const NaturePalette *pal);
void render_alt_tab_hud(NatureOSState *st, cairo_t *cr, const NaturePalette *pal);

#ifdef __cplusplus
}
#endif

#endif /* SHELL_INTERNAL_H */

