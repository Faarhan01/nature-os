#include "nature_os.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

static const char *MEDIA_TITLES[MAX_MEDIA_IMAGES] = {
    "Woodland Fern Fronds",
    "Canopy Dawn (4K)",
    "Alpine Twilight (4K)",
    "Basalt Moss Dew (4K)",
    "Horizon Daylight (4K)",
    "Horizon Night (4K)",
    "Alpine Sunrise Peak",
    "Nordic Forest Trail",
    "Scandinavian Courtyard",
    "Aurora Mist Horizon",
    "Verdant Canopy Rings"
};

static const char *MEDIA_META[MAX_MEDIA_IMAGES] = {
    "2400x1800 · Leica M11 35mm · f/2.0",
    "3840x2160 · Hasselblad X2D · ISO 100",
    "3840x2160 · Sony A7R V · f/4.0",
    "3840x2160 · Fujifilm GFX 100S · f/2.8",
    "3840x2160 · Horizon Alpine Ridge · Day",
    "3840x2160 · Horizon Starlight Ridge · Night",
    "3840x2160 · Nikkor Z 24-70mm · ISO 64",
    "3840x2160 · Fujinon XF 23mm · f/2.8",
    "3840x2160 · Zeiss Planar 50mm · f/4.0",
    "3840x2160 · Procedural Wayland HDR",
    "3840x2160 · Botanical Vector Render"
};

typedef struct {
    const char *title;
    const char *artist;
    const char *codec;
    int duration_sec;
} AudioTrack;

static const AudioTrack TRACKS[MAX_AUDIO_TRACKS] = {
    {"Rainforest Canopy", "Nature Soundscapes", "24-bit / 96 kHz FLAC · PipeWire", 222},
    {"Alpine Wind & Cedar", "Nordic Ambient Studio", "48 kHz Opus/OGG · PipeWire", 258},
    {"Night Moss Stream", "Olympic Field Archive", "48 kHz Uncompressed WAV", 175},
    {"Horizon Startup Chime", "Nature Audio Lab", "48 kHz OGG · Spatial Mix", 94}
};

static void clamp_zoom(NatureOSState *st) {
    if (st->media_zoom_pct < 50) st->media_zoom_pct = 50;
    if (st->media_zoom_pct > 250) st->media_zoom_pct = 250;
}

void nature_media_tick(NatureOSState *st) {
    if (st->media_slideshow && st->media_mode == 0 && !st->media_grid_mode) {
        st->media_slideshow_ticks++;
        if (st->media_slideshow_ticks >= 25) {
            st->media_slideshow_ticks = 0;
            st->media_selected = (st->media_selected + 1) % MAX_MEDIA_IMAGES;
            st->media_zoom_pct = 100;
            st->media_rotation = 0;
        }
    }

    if (!st->audio_playing) return;
    int sp = st->audio_speed_x10 > 0 ? st->audio_speed_x10 : 10;
    st->audio_tick_accum += sp;
    if (st->audio_tick_accum >= 70) {
        st->audio_tick_accum -= 70;
        st->audio_progress_sec++;
        int dur = TRACKS[st->audio_selected % MAX_AUDIO_TRACKS].duration_sec;
        if (st->audio_progress_sec >= dur) {
            st->audio_progress_sec = 0;
            if (!st->audio_loop) {
                if (st->audio_shuffle) {
                    st->audio_selected = (st->audio_selected + 2) % MAX_AUDIO_TRACKS;
                } else {
                    st->audio_selected = (st->audio_selected + 1) % MAX_AUDIO_TRACKS;
                }
            }
        }
    }
}

void nature_media_render(NatureOSState *st, cairo_t *cr, OSWindow *w, int cx, int cy, int cw, int ch) {
    (void)w;
    NaturePalette pal;
    nature_get_palette(st, &pal);
    int dark = pal.dark_mode;
    double ar = pal.ar, ag = pal.ag, ab = pal.ab;

    ngfx_fill_rect(cr, cx, cy, cw, ch, 0, pal.win_bg_r, pal.win_bg_g, pal.win_bg_b, 1.0);

    double fg_r = pal.fg_r, fg_g = pal.fg_g, fg_b = pal.fg_b;
    double mut_r = pal.mut_r, mut_g = pal.mut_g, mut_b = pal.mut_b;

    /* Header Toolbar */
    ngfx_fill_rect(cr, cx, cy, cw, 44, 0, pal.sidebar_r, pal.sidebar_g, pal.sidebar_b, 1.0);
    g_ngfx.set_source_rgba(cr, pal.border_r, pal.border_g, pal.border_b, pal.border_a);
    g_ngfx.move_to(cr, cx, cy + 44.5);
    g_ngfx.line_to(cr, cx + cw, cy + 44.5);
    g_ngfx.stroke(cr);

    const char *modes[2] = {"Photos (11)", "Audio (4)"};
    for (int m = 0; m < 2; ++m) {
        int mw = (m == 0) ? 84 : 74;
        int mx = cx + 10 + (m == 0 ? 0 : 88);
        int act = st->media_mode == m;
        int hov = (st->hover_hit_action == HIT_MEDIA_MODE && st->hover_hit_arg0 == m);
        if (act) {
            ngfx_fill_gradient(cr, mx, cy + 9, mw, 26, 7, ar, ag, ab, 0.95,
                               ar * 0.78, ag * 0.78, ab * 0.78, 0.95, 0);
            ngfx_top_specular(cr, mx, cy + 9, mw, 7, 0.22);
        } else {
            ngfx_fill_rect(cr, mx, cy + 9, mw, 26, 7,
                           hov ? pal.hover_r : pal.elev_r, hov ? pal.hover_g : pal.elev_g, hov ? pal.hover_b : pal.elev_b, 0.94);
            if (hov) ngfx_stroke_rect(cr, mx + 0.5, cy + 9.5, mw - 1, 25, 7, 0.8, ar, ag, ab, 0.45);
        }
        ngfx_text(cr, "Noto Sans", act, 10.4, mx + 9, cy + 26,
                  act ? pal.on_acc_r : fg_r, act ? pal.on_acc_g : fg_g, act ? pal.on_acc_b : fg_b, 1.0, modes[m]);
        nature_add_hit(st, mx, cy + 9, mw, 26, HIT_MEDIA_MODE, m, 0);
    }

    if (st->media_mode == 0) {
        int sel = st->media_selected % MAX_MEDIA_IMAGES;
        if (sel < 0) sel = 0;
        int compact = cw < 640;

        /* Grid/Stage & Slideshow toggles after mode pills */
        int gx_btn = cx + 178;
        if (gx_btn + 96 < cx + cw - (compact ? 160 : 320)) {
            ngfx_fill_rect(cr, gx_btn, cy + 9, 46, 26, 7,
                           st->media_grid_mode ? ar : pal.elev_r,
                           st->media_grid_mode ? ag : pal.elev_g,
                           st->media_grid_mode ? ab : pal.elev_b, 0.94);
            ngfx_text(cr, "Noto Sans", 1, 10.2, gx_btn + 10, cy + 26,
                      st->media_grid_mode ? pal.on_acc_r : fg_r,
                      st->media_grid_mode ? pal.on_acc_g : fg_g,
                      st->media_grid_mode ? pal.on_acc_b : fg_b, 1.0,
                      st->media_grid_mode ? "Stage" : "Grid");
            nature_add_hit(st, gx_btn, cy + 9, 46, 26, HIT_MEDIA_GRID_TOGGLE, 0, 0);

            int sx_btn = gx_btn + 50;
            ngfx_fill_rect(cr, sx_btn, cy + 9, 48, 26, 7,
                           st->media_slideshow ? pal.warning_r : pal.elev_r,
                           st->media_slideshow ? pal.warning_g : pal.elev_g,
                           st->media_slideshow ? pal.warning_b : pal.elev_b, 0.94);
            ngfx_text(cr, "Noto Sans", 1, 10.2, sx_btn + 9, cy + 26,
                      st->media_slideshow ? 1.0 : fg_r,
                      st->media_slideshow ? 1.0 : fg_g,
                      st->media_slideshow ? 1.0 : fg_b, 1.0,
                      st->media_slideshow ? "Play*" : "Slide");
            nature_add_hit(st, sx_btn, cy + 9, 48, 26, HIT_MEDIA_SLIDESHOW, 0, 0);
        }

        /* Right side stage controls */
        int ctrl_w = compact ? 258 : 382;
        int px = cx + cw - ctrl_w - 8;
        if (px < cx + 178) px = cx + 178;

        /* Prev / Next */
        ngfx_fill_rect(cr, px, cy + 9, 25, 26, 7, pal.elev_r, pal.elev_g, pal.elev_b, 0.94);
        ngfx_stroke_rect(cr, px + 0.5, cy + 9.5, 24, 25, 7, 0.8, pal.border_r, pal.border_g, pal.border_b, pal.border_a);
        g_ngfx.set_source_rgba(cr, fg_r, fg_g, fg_b, 0.95);
        g_ngfx.set_line_width(cr, 1.6);
        g_ngfx.move_to(cr, px + 14.5, cy + 17.5); g_ngfx.line_to(cr, px + 9.5, cy + 22.0); g_ngfx.line_to(cr, px + 14.5, cy + 26.5);
        g_ngfx.stroke(cr);
        nature_add_hit(st, px, cy + 9, 25, 26, HIT_MEDIA_PREV, 0, 0);

        ngfx_fill_rect(cr, px + 28, cy + 9, 25, 26, 7, pal.elev_r, pal.elev_g, pal.elev_b, 0.94);
        ngfx_stroke_rect(cr, px + 28.5, cy + 9.5, 24, 25, 7, 0.8, pal.border_r, pal.border_g, pal.border_b, pal.border_a);
        g_ngfx.move_to(cr, px + 38.5, cy + 17.5); g_ngfx.line_to(cr, px + 43.5, cy + 22.0); g_ngfx.line_to(cr, px + 38.5, cy + 26.5);
        g_ngfx.stroke(cr);
        nature_add_hit(st, px + 28, cy + 9, 25, 26, HIT_MEDIA_NEXT, 0, 0);

        int next_x = px + 57;
        if (!compact) {
            /* Zoom - / % / + / Fit / Rot */
            ngfx_fill_rect(cr, next_x, cy + 9, 24, 26, 7, pal.elev_r, pal.elev_g, pal.elev_b, 0.94);
            ngfx_text(cr, "JetBrains Mono", 1, 11.5, next_x + 8, cy + 26, fg_r, fg_g, fg_b, 1.0, "-");
            nature_add_hit(st, next_x, cy + 9, 24, 26, HIT_MEDIA_ZOOM_OUT, 0, 0);
            next_x += 26;

            char zb[16];
            snprintf(zb, sizeof(zb), "%d%%", st->media_zoom_pct);
            ngfx_fill_rect(cr, next_x, cy + 9, 44, 26, 7, pal.elev_r, pal.elev_g, pal.elev_b, 0.94);
            ngfx_text(cr, "JetBrains Mono", 1, 9.8, next_x + 6, cy + 26, fg_r, fg_g, fg_b, 1.0, zb);
            nature_add_hit(st, next_x, cy + 9, 44, 26, HIT_MEDIA_ZOOM, 0, 0);
            next_x += 46;

            ngfx_fill_rect(cr, next_x, cy + 9, 24, 26, 7, pal.elev_r, pal.elev_g, pal.elev_b, 0.94);
            ngfx_text(cr, "JetBrains Mono", 1, 11.5, next_x + 8, cy + 26, fg_r, fg_g, fg_b, 1.0, "+");
            nature_add_hit(st, next_x, cy + 9, 24, 26, HIT_MEDIA_ZOOM_IN, 0, 0);
            next_x += 27;

            ngfx_fill_rect(cr, next_x, cy + 9, 30, 26, 7, pal.elev_r, pal.elev_g, pal.elev_b, 0.94);
            ngfx_text(cr, "Noto Sans", 1, 10.0, next_x + 8, cy + 26, fg_r, fg_g, fg_b, 1.0, "Fit");
            nature_add_hit(st, next_x, cy + 9, 30, 26, HIT_MEDIA_ZOOM_FIT, 0, 0);
            next_x += 33;
        }

        ngfx_fill_rect(cr, next_x, cy + 9, 34, 26, 7, pal.elev_r, pal.elev_g, pal.elev_b, 0.94);
        ngfx_text(cr, "Noto Sans", 1, 10.2, next_x + 7, cy + 26, fg_r, fg_g, fg_b, 1.0, "Rot");
        nature_add_hit(st, next_x, cy + 9, 34, 26, HIT_MEDIA_ROTATE, 0, 0);
        next_x += 37;

        ngfx_fill_rect(cr, next_x, cy + 9, 36, 26, 7,
                       st->media_show_exif ? ar : pal.elev_r,
                       st->media_show_exif ? ag : pal.elev_g,
                       st->media_show_exif ? ab : pal.elev_b, 0.94);
        ngfx_text(cr, "Noto Sans", 1, 10.0, next_x + 8, cy + 26,
                  st->media_show_exif ? pal.on_acc_r : fg_r,
                  st->media_show_exif ? pal.on_acc_g : fg_g,
                  st->media_show_exif ? pal.on_acc_b : fg_b, 1.0, "Info");
        nature_add_hit(st, next_x, cy + 9, 36, 26, HIT_MEDIA_INFO_TOGGLE, 0, 0);
        next_x += 39;

        int wp_btn_w = (cx + cw - next_x - 8);
        if (wp_btn_w >= 84) {
            if (wp_btn_w > 106) wp_btn_w = 106;
            ngfx_fill_gradient(cr, next_x, cy + 9, wp_btn_w, 26, 7, ar, ag, ab, 0.96, ar * 0.78, ag * 0.78, ab * 0.78, 0.96, 0);
            ngfx_top_specular(cr, next_x, cy + 9, wp_btn_w, 7, 0.22);
            ngfx_text(cr, "Noto Sans", 1, 10.2, next_x + 10, cy + 26, pal.on_acc_r, pal.on_acc_g, pal.on_acc_b, 1, "Set Wallpaper");
            nature_add_hit(st, next_x, cy + 9, wp_btn_w, 26, HIT_MEDIA_SET_WP, sel, 0);
        }

        if (st->media_grid_mode) {
            /* Gallery Grid Mode */
            int cols = cw >= 580 ? 3 : 2;
            int gx_pad = 16;
            int gap = 12;
            int card_w = (cw - gx_pad * 2 - 14 - (cols - 1) * gap) / cols;
            int card_h = (card_w * 9) / 16 + 34;
            if (card_h < 106) card_h = 106;
            int total_rows = (MAX_MEDIA_IMAGES + cols - 1) / cols;
            int body_y = cy + 52;
            int body_h = ch - 58;
            int vis_rows = body_h / (card_h + 10);
            if (vis_rows < 1) vis_rows = 1;
            int max_sc = total_rows - vis_rows;
            if (max_sc < 0) max_sc = 0;
            st->media_grid_max_scroll = max_sc;
            if (st->media_grid_scroll > max_sc) st->media_grid_scroll = max_sc;
            if (st->media_grid_scroll < 0) st->media_grid_scroll = 0;

            g_ngfx.save(cr);
            g_ngfx.rectangle(cr, cx, cy + 45, cw - 14, ch - 46);
            g_ngfx.clip(cr);

            int start_idx = st->media_grid_scroll * cols;
            for (int k = start_idx; k < MAX_MEDIA_IMAGES; ++k) {
                int slot = k - start_idx;
                int c = slot % cols;
                int r = slot / cols;
                int xcard = cx + gx_pad + c * (card_w + gap);
                int ycard = body_y + r * (card_h + 10);
                if (ycard > cy + ch - 4) break;
                int is_sel = (sel == k);

                ngfx_fill_rect(cr, xcard, ycard, card_w, card_h, 10, pal.card_r, pal.card_g, pal.card_b, 0.96);
                ngfx_stroke_rect(cr, xcard + 0.5, ycard + 0.5, card_w - 1, card_h - 1, 10, is_sel ? 1.8 : 0.9,
                                 is_sel ? ar : pal.border_r, is_sel ? ag : pal.border_g, is_sel ? ab : pal.border_b,
                                 is_sel ? 0.95 : pal.border_a);

                int ih = card_h - 32;
                if (st->media_images[k]) {
                    g_ngfx.save(cr);
                    ngfx_rounded_path(cr, xcard + 4, ycard + 4, card_w - 8, ih - 4, 7);
                    g_ngfx.clip(cr);
                    double sw = g_ngfx.image_surface_get_width(st->media_images[k]);
                    double sh = g_ngfx.image_surface_get_height(st->media_images[k]);
                    if (sw > 0 && sh > 0) {
                        double dw = (double)(card_w - 8), dh = (double)(ih - 4);
                        double sc = (dw / sw > dh / sh) ? (dw / sw) : (dh / sh);
                        g_ngfx.translate(cr, xcard + 4 + (dw - sw * sc) * 0.5, ycard + 4 + (dh - sh * sc) * 0.5);
                        g_ngfx.scale(cr, sc, sc);
                        g_ngfx.set_source_surface(cr, st->media_images[k], 0, 0);
                        g_ngfx.paint(cr);
                    }
                    g_ngfx.restore(cr);
                }
                ngfx_text_clipped(cr, "Noto Sans", is_sel, 10.6, xcard + 8, ycard + ih + 16, card_w - 16, fg_r, fg_g, fg_b, 1.0, MEDIA_TITLES[k]);
                nature_add_hit(st, xcard, ycard, card_w, card_h, HIT_MEDIA_SELECT, k, 1);
            }
            g_ngfx.restore(cr);

            nature_draw_scrollbar(st, cr, cx + cw - 11, body_y, body_h,
                                  total_rows, vis_rows, st->media_grid_scroll, max_sc, SCROLL_MEDIA_GRID, dark);
        } else {
            /* Stage Mode */
            int foot_meta_h = 26;
            int thumb_bar_h = 74;
            int view_h = ch - 44 - foot_meta_h - thumb_bar_h;
            cairo_surface_t *img = st->media_images[sel];
            if (img && view_h > 40) {
                double sw = g_ngfx.image_surface_get_width(img);
                double sh = g_ngfx.image_surface_get_height(img);
                int rot_side = (st->media_rotation == 90 || st->media_rotation == 270);
                double eff_w = rot_side ? sh : sw;
                double eff_h = rot_side ? sw : sh;
                double fit = (cw - 36) / eff_w;
                if ((view_h - 16) / eff_h < fit) fit = (view_h - 16) / eff_h;
                double scale = fit * (st->media_zoom_pct / 100.0);
                double dw = sw * scale;
                double dh = sh * scale;
                double mid_x = cx + cw * 0.5;
                double mid_y = cy + 44 + view_h * 0.5;

                g_ngfx.save(cr);
                g_ngfx.rectangle(cr, cx, cy + 45, cw, view_h - 1);
                g_ngfx.clip(cr);

                if (st->media_rotation == 0) {
                    ngfx_shadow_rect(cr, mid_x - dw * 0.5, mid_y - dh * 0.5, dw, dh, 10, 3.6, 0.50);
                    ngfx_rounded_path(cr, mid_x - dw * 0.5, mid_y - dh * 0.5, dw, dh, 10);
                    g_ngfx.clip(cr);
                }
                g_ngfx.translate(cr, mid_x, mid_y);
                if (st->media_rotation != 0) {
                    g_ngfx.rotate(cr, st->media_rotation * (3.14159265358979 / 180.0));
                }
                g_ngfx.scale(cr, scale, scale);
                g_ngfx.set_source_surface(cr, img, -sw * 0.5, -sh * 0.5);
                g_ngfx.paint(cr);
                g_ngfx.restore(cr);
                if (st->media_rotation == 0) {
                    ngfx_stroke_rect(cr, mid_x - dw * 0.5, mid_y - dh * 0.5, dw, dh, 10, 1.0, pal.border_r, pal.border_g, pal.border_b, pal.border_a);
                    ngfx_top_specular(cr, mid_x - dw * 0.5, mid_y - dh * 0.5, dw, 10, 0.15);
                }
            }

            /* Metadata Caption Strip above thumbnails + optional EXIF HUD & Slideshow bar */
            int my_meta = cy + ch - thumb_bar_h - foot_meta_h;
            if (st->media_show_exif && view_h > 92) {
                int hw = cw < 320 ? cw - 24 : 272;
                int hx = cx + 14, hy = my_meta - 76;
                ngfx_shadow_rect(cr, hx, hy, hw, 66, 10, 2.2, 0.45);
                ngfx_fill_rect(cr, hx, hy, hw, 66, 10, pal.sidebar_r, pal.sidebar_g, pal.sidebar_b, 0.92);
                ngfx_stroke_rect(cr, hx + 0.5, hy + 0.5, hw - 1, 65, 10, 1.0, ar, ag, ab, 0.65);
                ngfx_text_clipped(cr, "Plus Jakarta Sans", 1, 11.4, hx + 12, hy + 20, hw - 24, fg_r, fg_g, fg_b, 1.0, MEDIA_TITLES[sel]);
                ngfx_text_clipped(cr, "JetBrains Mono", 0, 9.2, hx + 12, hy + 38, hw - 24,
                                  dark ? pal.ar_hi : ar, dark ? pal.ag_hi : ag, dark ? pal.ab_hi : ab, 0.98, MEDIA_META[sel]);
                char hint_exif[64];
                snprintf(hint_exif, sizeof(hint_exif), "Zoom %d%% · Rot %d deg · Space: Slideshow", st->media_zoom_pct, st->media_rotation);
                ngfx_text_clipped(cr, "JetBrains Mono", 0, 8.8, hx + 12, hy + 55, hw - 24, mut_r, mut_g, mut_b, 0.92, hint_exif);
            }
            if (st->media_slideshow) {
                double s_frac = st->media_slideshow_ticks / 25.0;
                if (s_frac < 0.0) s_frac = 0.0;
                if (s_frac > 1.0) s_frac = 1.0;
                ngfx_fill_rect(cr, cx, my_meta - 3, cw, 3, 0, pal.elev_r, pal.elev_g, pal.elev_b, 0.55);
                ngfx_fill_rect(cr, cx, my_meta - 3, cw * s_frac, 3, 0, pal.ar_hi, pal.ag_hi, pal.ab_hi, 0.96);
            }
            ngfx_fill_rect(cr, cx, my_meta, cw, foot_meta_h, 0, pal.surface_r, pal.surface_g, pal.surface_b, 0.94);
            ngfx_text_clipped(cr, "Plus Jakarta Sans", 1, 11.0, cx + 14, my_meta + 17, cw * 0.38, fg_r, fg_g, fg_b, 1.0, MEDIA_TITLES[sel]);
            char rmeta[160];
            snprintf(rmeta, sizeof(rmeta), "%d/%d · %d%% · %d deg · %s",
                     sel + 1, MAX_MEDIA_IMAGES, st->media_zoom_pct, st->media_rotation, MEDIA_META[sel]);
            ngfx_text_clipped(cr, "JetBrains Mono", 0, 9.5, cx + cw * 0.40, my_meta + 17, cw * 0.58 - 14, mut_r, mut_g, mut_b, 0.95, rmeta);

            int fy = cy + ch - thumb_bar_h;
            ngfx_fill_rect(cr, cx, fy, cw, thumb_bar_h, 0, pal.sidebar_r, pal.sidebar_g, pal.sidebar_b, 1.0);
            g_ngfx.set_source_rgba(cr, pal.border_r, pal.border_g, pal.border_b, pal.border_a);
            g_ngfx.move_to(cr, cx, fy + 0.5);
            g_ngfx.line_to(cr, cx + cw, fy + 0.5);
            g_ngfx.stroke(cr);

            /* Fit visible thumbnail strip centered around sel */
            int max_vis = (cw - 24) / 72;
            if (max_vis < 4) max_vis = 4;
            if (max_vis > MAX_MEDIA_IMAGES) max_vis = MAX_MEDIA_IMAGES;
            int win_start = sel - max_vis / 2;
            if (win_start < 0) win_start = 0;
            if (win_start + max_vis > MAX_MEDIA_IMAGES) win_start = MAX_MEDIA_IMAGES - max_vis;

            int thumb_w = (cw - 24 - (max_vis - 1) * 7) / max_vis;
            if (thumb_w > 96) thumb_w = 96;
            int total_tw = max_vis * thumb_w + (max_vis - 1) * 7;
            int start_tx = cx + (cw - total_tw) / 2;

            for (int slot = 0; slot < max_vis; ++slot) {
                int i = win_start + slot;
                int tx = start_tx + slot * (thumb_w + 7);
                int ty = fy + 9;
                int is_sel = (sel == i);
                int is_hov = (st->hover_hit_action == HIT_MEDIA_SELECT && st->hover_hit_arg0 == i);
                if (is_sel) ngfx_shadow_rect(cr, tx, ty, thumb_w, 50, 7, 1.6, 0.35);
                if (st->media_images[i]) {
                    g_ngfx.save(cr);
                    ngfx_rounded_path(cr, tx, ty, thumb_w, 50, 7);
                    g_ngfx.clip(cr);
                    double sw = g_ngfx.image_surface_get_width(st->media_images[i]);
                    double sh = g_ngfx.image_surface_get_height(st->media_images[i]);
                    if (sw > 0 && sh > 0) {
                        double sc = ((double)thumb_w / sw > 50.0 / sh) ? ((double)thumb_w / sw) : (50.0 / sh);
                        g_ngfx.translate(cr, tx + ((double)thumb_w - sw * sc) * 0.5, ty + (50.0 - sh * sc) * 0.5);
                        g_ngfx.scale(cr, sc, sc);
                        g_ngfx.set_source_surface(cr, st->media_images[i], 0, 0);
                        g_ngfx.paint(cr);
                    }
                    g_ngfx.restore(cr);
                }
                ngfx_stroke_rect(cr, tx, ty, thumb_w, 50, 7, is_sel ? 2.2 : (is_hov ? 1.5 : 1.0),
                                 (is_sel || is_hov) ? ar : pal.border_r, (is_sel || is_hov) ? ag : pal.border_g, (is_sel || is_hov) ? ab : pal.border_b,
                                 is_sel ? 1.0 : (is_hov ? 0.65 : pal.border_a));
                if (is_sel) {
                    ngfx_fill_circle(cr, tx + thumb_w * 0.5, ty + 57.0, 2.5, pal.ar_hi, pal.ag_hi, pal.ab_hi, 1.0);
                }
                nature_add_hit(st, tx, ty, thumb_w, 50, HIT_MEDIA_SELECT, i, 0);
            }
        }
    } else {
        int sel = st->audio_selected % MAX_AUDIO_TRACKS;
        if (sel < 0) sel = 0;
        const AudioTrack *trk = &TRACKS[sel];
        int compact = cw < 540;
        int px = cx + 18, py = cy + 56, pw = cw - 44;

        ngfx_shadow_rect(cr, px, py, pw, 184, 14, 2.0, 0.24);
        ngfx_fill_rect(cr, px, py, pw, 184, 14, pal.card_r, pal.card_g, pal.card_b, 0.96);
        ngfx_stroke_rect(cr, px + 0.5, py + 0.5, pw - 1, 183, 14, 1.0, pal.border_r, pal.border_g, pal.border_b, pal.border_a);
        ngfx_top_specular(cr, px, py, pw, 14, dark ? 0.12 : 0.30);

        int asz = compact ? 92 : 138;
        int ax = px + 14, ay = py + (184 - asz) / 2;
        ngfx_shadow_rect(cr, ax, ay, asz, asz, 12, 1.8, 0.35);
        ngfx_fill_gradient(cr, ax, ay, asz, asz, 12,
                           ar * 0.95, ag * 0.95, ab * 0.95, 0.98,
                           pal.sidebar_r, pal.sidebar_g, pal.sidebar_b, 1.0, 0);
        cairo_surface_t *art = st->media_images[(sel + 1) % MAX_MEDIA_IMAGES];
        if (art) {
            double sw = g_ngfx.image_surface_get_width(art);
            double sh = g_ngfx.image_surface_get_height(art);
            if (sw > 0 && sh > 0) {
                double sc = ((double)asz / sw > (double)asz / sh) ? ((double)asz / sw) : ((double)asz / sh);
                g_ngfx.save(cr);
                ngfx_rounded_path(cr, ax, ay, asz, asz, 12);
                g_ngfx.clip(cr);
                g_ngfx.translate(cr, ax + (asz - sw * sc) * 0.5, ay + (asz - sh * sc) * 0.5);
                g_ngfx.scale(cr, sc, sc);
                g_ngfx.set_source_surface(cr, art, 0, 0);
                g_ngfx.paint(cr);
                g_ngfx.restore(cr);
                ngfx_fill_rect(cr, ax, ay, asz, asz, 12, 0.02, 0.04, 0.05, 0.42);
            }
        }
        ngfx_top_specular(cr, ax, ay, asz, 12, 0.24);

        double acx = ax + asz * 0.5;
        double acy = ay + asz * 0.5;
        ngfx_fill_circle(cr, acx, acy, asz * 0.34, 0.05, 0.07, 0.06, 0.90);
        g_ngfx.set_source_rgba(cr, 1.0, 1.0, 1.0, 0.10);
        g_ngfx.set_line_width(cr, 0.9);
        g_ngfx.arc(cr, acx, acy, asz * 0.28, 0, 6.28318);
        g_ngfx.stroke(cr);
        g_ngfx.arc(cr, acx, acy, asz * 0.21, 0, 6.28318);
        g_ngfx.stroke(cr);
        ngfx_fill_circle(cr, acx, acy, asz * 0.11, pal.ar_hi, pal.ag_hi, pal.ab_hi, 1.0);
        ngfx_fill_circle(cr, acx, acy, 2.8, 0.05, 0.07, 0.06, 1.0);

        static const char *EQ_NAMES[4] = {"Natural", "Bass", "Studio", "Ambient"};
        static const char *SINK_SHORT[3] = {"Analog", "HDMI", "BT Buds"};
        int eq_idx = st->audio_eq_preset % 4;
        if (eq_idx < 0) eq_idx = 0;

        int info_x = ax + asz + 18;
        int info_w = pw - asz - 46;
        ngfx_text_clipped(cr, "Plus Jakarta Sans", 1, 16.5, info_x, py + 34, info_w, fg_r, fg_g, fg_b, 1.0, trk->title);
        ngfx_text_clipped(cr, "Noto Sans", 0, 11.6, info_x, py + 54, info_w, mut_r, mut_g, mut_b, 1.0, trk->artist);
        char codec_line[160];
        snprintf(codec_line, sizeof(codec_line), "%s · %s · EQ: %s", trk->codec, SINK_SHORT[st->cfg.audio_device % 3], EQ_NAMES[eq_idx]);
        ngfx_text_clipped(cr, "JetBrains Mono", 0, 9.8, info_x, py + 72, info_w,
                          dark ? pal.ar_hi : ar, dark ? pal.ag_hi : ag, dark ? pal.ab_hi : ab, 0.95, codec_line);

        double vol_gain = st->cfg.audio_muted ? 0.12 : (0.35 + 0.65 * (st->cfg.volume / 100.0));
        int bars = compact ? 16 : 24;
        int bar_w = (info_w - (bars - 1) * 3) / bars;
        if (bar_w >= 3) {
            for (int b = 0; b < bars; ++b) {
                double pos = (double)b / (double)(bars - 1);
                double eq_w = 1.0;
                if (eq_idx == 1) eq_w = 1.28 - 0.50 * pos;
                else if (eq_idx == 2) eq_w = 0.85 + 0.35 * sin(pos * 3.14159);
                else if (eq_idx == 3) eq_w = 0.72 + 0.48 * pos;
                double h_norm = (st->audio_playing && !st->cfg.audio_muted)
                    ? (0.20 + 0.74 * fabs(sin(b * 0.55 + st->audio_tick_accum * 0.14 + st->audio_progress_sec * 0.7))) * vol_gain * eq_w
                    : (0.14 + 0.18 * fabs(sin(b * 0.7))) * vol_gain * eq_w;
                if (h_norm > 1.0) h_norm = 1.0;
                double bh = 32.0 * h_norm;
                if (bh < 3.0) bh = 3.0;
                double bx_bar = info_x + b * (bar_w + 3);
                double by_bar = py + 116 - bh;
                ngfx_fill_gradient(cr, bx_bar, by_bar, bar_w, bh, 2,
                                   pal.ar_hi, pal.ag_hi, pal.ab_hi,
                                   st->audio_playing ? 0.95 : 0.42,
                                   ar * 0.7, ag * 0.7, ab * 0.7,
                                   st->audio_playing ? 0.65 : 0.25, 0);
            }
        }

        int pby = py + 128;
        ngfx_fill_rect(cr, info_x, pby + 1, info_w, 8, 4, pal.elev_r, pal.elev_g, pal.elev_b, 1.0);
        double pct = (double)st->audio_progress_sec / (double)trk->duration_sec;
        if (pct > 1.0) pct = 1.0;
        if (pct > 0.0) {
            double pw_fill = info_w * pct;
            if (pw_fill < 6) pw_fill = 6;
            ngfx_fill_gradient(cr, info_x, pby + 1, pw_fill, 8, 4,
                               pal.ar_hi, pal.ag_hi, pal.ab_hi, 0.98,
                               ar * 0.85, ag * 0.85, ab * 0.85, 0.98, 1);
            ngfx_fill_circle(cr, info_x + pw_fill, pby + 5, 5.8, ar, ag, ab, 0.35);
            ngfx_fill_circle(cr, info_x + pw_fill, pby + 5, 4.2, 1.0, 1.0, 1.0, 0.98);
        }
        nature_add_hit(st, info_x, pby - 6, info_w, 22, HIT_MEDIA_AUDIO_SEEK, info_w, 0);

        char tbuf[32];
        snprintf(tbuf, sizeof(tbuf), "%02d:%02d / %02d:%02d",
                 st->audio_progress_sec / 60, st->audio_progress_sec % 60,
                 trk->duration_sec / 60, trk->duration_sec % 60);
        ngfx_text(cr, "JetBrains Mono", 0, 10.2, info_x, pby + 30, mut_r, mut_g, mut_b, 1.0, tbuf);

        /* Right-aligned Audio Controls: Prev, Next, Shuffle, Mute, Loop, Speed, -10s, +10s, Play/Pause */
        int play_x = info_x + info_w - 82;
        ngfx_fill_gradient(cr, play_x, pby + 16, 82, 26, 7, ar, ag, ab, 0.98, ar * 0.78, ag * 0.78, ab * 0.78, 0.98, 0);
        ngfx_top_specular(cr, play_x, pby + 16, 82, 7, 0.24);
        if (st->audio_playing) {
            ngfx_fill_rect(cr, play_x + 14, pby + 23.5, 3.0, 11, 1, pal.on_acc_r, pal.on_acc_g, pal.on_acc_b, 0.96);
            ngfx_fill_rect(cr, play_x + 20, pby + 23.5, 3.0, 11, 1, pal.on_acc_r, pal.on_acc_g, pal.on_acc_b, 0.96);
        } else {
            g_ngfx.set_source_rgba(cr, pal.on_acc_r, pal.on_acc_g, pal.on_acc_b, 0.96);
            g_ngfx.move_to(cr, play_x + 15, pby + 23.0);
            g_ngfx.line_to(cr, play_x + 24, pby + 29.0);
            g_ngfx.line_to(cr, play_x + 15, pby + 35.0);
            g_ngfx.close_path(cr);
            g_ngfx.fill(cr);
        }
        ngfx_text(cr, "Plus Jakarta Sans", 1, 10.8, play_x + 30, pby + 32.5, pal.on_acc_r, pal.on_acc_g, pal.on_acc_b, 1,
                  st->audio_playing ? "Pause" : "Play");
        nature_add_hit(st, play_x, pby + 16, 82, 26, HIT_MEDIA_AUDIO_PLAY, 0, 0);

        int cx_btn = play_x - 32;
        if (cx_btn >= info_x + 92) {
            ngfx_fill_rect(cr, cx_btn, pby + 16, 28, 26, 7, pal.elev_r, pal.elev_g, pal.elev_b, 0.94);
            ngfx_text(cr, "JetBrains Mono", 1, 9.8, cx_btn + 6, pby + 32.5, fg_r, fg_g, fg_b, 0.95, ">|");
            nature_add_hit(st, cx_btn, pby + 16, 28, 26, HIT_MEDIA_AUDIO_NEXT, 0, 0);
            cx_btn -= 32;
        }
        if (cx_btn >= info_x + 92) {
            ngfx_fill_rect(cr, cx_btn, pby + 16, 28, 26, 7, pal.elev_r, pal.elev_g, pal.elev_b, 0.94);
            ngfx_text(cr, "JetBrains Mono", 1, 9.8, cx_btn + 6, pby + 32.5, fg_r, fg_g, fg_b, 0.95, "|<");
            nature_add_hit(st, cx_btn, pby + 16, 28, 26, HIT_MEDIA_AUDIO_PREV, 0, 0);
            cx_btn -= 38;
        }

        if (!compact) {
            if (cx_btn >= info_x + 86) {
                ngfx_fill_rect(cr, cx_btn, pby + 16, 34, 26, 7, pal.elev_r, pal.elev_g, pal.elev_b, 0.94);
                ngfx_text(cr, "JetBrains Mono", 1, 9.5, cx_btn + 4, pby + 32.5, fg_r, fg_g, fg_b, 0.95, "+10s");
                nature_add_hit(st, cx_btn, pby + 16, 34, 26, HIT_MEDIA_AUDIO_STEP, 10, 0);
                cx_btn -= 38;
            }
            if (cx_btn >= info_x + 86) {
                ngfx_fill_rect(cr, cx_btn, pby + 16, 34, 26, 7, pal.elev_r, pal.elev_g, pal.elev_b, 0.94);
                ngfx_text(cr, "JetBrains Mono", 1, 9.5, cx_btn + 4, pby + 32.5, fg_r, fg_g, fg_b, 0.95, "-10s");
                nature_add_hit(st, cx_btn, pby + 16, 34, 26, HIT_MEDIA_AUDIO_STEP, -10, 0);
                cx_btn -= 42;
            }
            if (cx_btn >= info_x + 86) {
                int sp = st->audio_speed_x10 > 0 ? st->audio_speed_x10 : 10;
                char sp_lbl[12];
                if (sp == 10) snprintf(sp_lbl, sizeof(sp_lbl), "1x");
                else if (sp == 20) snprintf(sp_lbl, sizeof(sp_lbl), "2x");
                else snprintf(sp_lbl, sizeof(sp_lbl), "%.1fx", sp / 10.0);
                ngfx_fill_rect(cr, cx_btn, pby + 16, 38, 26, 7,
                               sp != 10 ? ar : pal.elev_r, sp != 10 ? ag : pal.elev_g, sp != 10 ? ab : pal.elev_b, 0.94);
                ngfx_text(cr, "JetBrains Mono", 1, 9.8, cx_btn + 7, pby + 32.5,
                          sp != 10 ? pal.on_acc_r : fg_r, sp != 10 ? pal.on_acc_g : fg_g, sp != 10 ? pal.on_acc_b : fg_b, 1.0, sp_lbl);
                nature_add_hit(st, cx_btn, pby + 16, 38, 26, HIT_MEDIA_AUDIO_SPEED, 0, 0);
                cx_btn -= 42;
            }
            if (cx_btn >= info_x + 86) {
                ngfx_fill_rect(cr, cx_btn, pby + 16, 38, 26, 7,
                               st->audio_loop ? ar : pal.elev_r, st->audio_loop ? ag : pal.elev_g, st->audio_loop ? ab : pal.elev_b, 0.94);
                ngfx_text(cr, "Noto Sans", 1, 9.8, cx_btn + 6, pby + 32.5,
                          st->audio_loop ? pal.on_acc_r : fg_r, st->audio_loop ? pal.on_acc_g : fg_g, st->audio_loop ? pal.on_acc_b : fg_b, 1.0, "Loop");
                nature_add_hit(st, cx_btn, pby + 16, 38, 26, HIT_MEDIA_AUDIO_LOOP, 0, 0);
                cx_btn -= 42;
            }
            if (cx_btn >= info_x + 86) {
                ngfx_fill_rect(cr, cx_btn, pby + 16, 38, 26, 7,
                               st->audio_shuffle ? ar : pal.elev_r, st->audio_shuffle ? ag : pal.elev_g, st->audio_shuffle ? ab : pal.elev_b, 0.94);
                ngfx_text(cr, "Noto Sans", 1, 9.6, cx_btn + 6, pby + 32.5,
                          st->audio_shuffle ? pal.on_acc_r : fg_r, st->audio_shuffle ? pal.on_acc_g : fg_g, st->audio_shuffle ? pal.on_acc_b : fg_b, 1.0, "Shuf");
                nature_add_hit(st, cx_btn, pby + 16, 38, 26, HIT_MEDIA_AUDIO_SHUFFLE, 0, 0);
                cx_btn -= 46;
            }
            if (cx_btn >= info_x + 86) {
                ngfx_fill_rect(cr, cx_btn, pby + 16, 42, 26, 7,
                               st->cfg.audio_muted ? pal.danger_r : pal.elev_r,
                               st->cfg.audio_muted ? pal.danger_g : pal.elev_g,
                               st->cfg.audio_muted ? pal.danger_b : pal.elev_b, 0.94);
                ngfx_text(cr, "Noto Sans", 1, 9.8, cx_btn + 7, pby + 32.5,
                          st->cfg.audio_muted ? 1.0 : fg_r, st->cfg.audio_muted ? 1.0 : fg_g, st->cfg.audio_muted ? 1.0 : fg_b, 1.0,
                          st->cfg.audio_muted ? "Muted" : "Mute");
                nature_add_hit(st, cx_btn, pby + 16, 42, 26, HIT_MEDIA_AUDIO_MUTE, 0, 0);
                cx_btn -= 46;
            }
            if (cx_btn >= info_x + 86) {
                ngfx_fill_rect(cr, cx_btn, pby + 16, 42, 26, 7,
                               eq_idx != 0 ? ar : pal.elev_r, eq_idx != 0 ? ag : pal.elev_g, eq_idx != 0 ? ab : pal.elev_b, 0.94);
                ngfx_text_clipped(cr, "Noto Sans", 1, 9.5, cx_btn + 5, pby + 32.5, 34,
                                  eq_idx != 0 ? pal.on_acc_r : fg_r, eq_idx != 0 ? pal.on_acc_g : fg_g, eq_idx != 0 ? pal.on_acc_b : fg_b, 1.0,
                                  EQ_NAMES[eq_idx]);
                nature_add_hit(st, cx_btn, pby + 16, 42, 26, HIT_MEDIA_AUDIO_EQ, 0, 0);
            }
        }

        /* Scrollable Playlist Rows */
        int ly_head = py + 202;
        ngfx_text(cr, "Plus Jakarta Sans", 1, 11.8, px, ly_head, mut_r, mut_g, mut_b, 1.0, "Sample Library (/home/nature/Music)");
        int list_y = ly_head + 12;
        int list_h = cy + ch - list_y - 8;
        int vis = list_h / 44;
        if (vis < 1) vis = 1;
        int max_scroll = MAX_AUDIO_TRACKS - vis;
        if (max_scroll < 0) max_scroll = 0;
        st->audio_max_scroll = max_scroll;
        if (st->audio_scroll > max_scroll) st->audio_scroll = max_scroll;
        if (st->audio_scroll < 0) st->audio_scroll = 0;

        int ly = list_y;
        for (int i = st->audio_scroll; i < MAX_AUDIO_TRACKS; ++i) {
            if (ly + 38 > cy + ch - 4) break;
            int is_sel = (sel == i);
            int is_hov = (st->hover_hit_action == HIT_MEDIA_AUDIO_SELECT && st->hover_hit_arg0 == i);
            ngfx_fill_rect(cr, px, ly, pw, 38, 9,
                           is_sel ? ar : (is_hov ? pal.hover_r : pal.card_r),
                           is_sel ? ag : (is_hov ? pal.hover_g : pal.card_g),
                           is_sel ? ab : (is_hov ? pal.hover_b : pal.card_b),
                           is_sel ? (dark ? 0.24 : 0.16) : 0.96);
            if (is_sel) {
                ngfx_stroke_rect(cr, px + 0.5, ly + 0.5, pw - 1, 37, 9, 1.0, ar, ag, ab, 0.65);
                ngfx_fill_rect(cr, px + 1, ly + 7, 3, 24, 1.5, pal.ar_hi, pal.ag_hi, pal.ab_hi, 0.98);
            } else {
                ngfx_stroke_rect(cr, px + 0.5, ly + 0.5, pw - 1, 37, 9, 0.8, pal.border_r, pal.border_g, pal.border_b, pal.border_a);
            }

            char dur[16];
            snprintf(dur, sizeof(dur), "%02d:%02d", TRACKS[i].duration_sec / 60, TRACKS[i].duration_sec % 60);
            ngfx_text_clipped(cr, "Plus Jakarta Sans", 1, 11.8, px + 16, ly + 17, pw - 88, fg_r, fg_g, fg_b, 1.0, TRACKS[i].title);
            ngfx_text_clipped(cr, "Noto Sans", 0, 10.0, px + 16, ly + 31, pw - 88, mut_r, mut_g, mut_b, 0.95, TRACKS[i].codec);
            ngfx_text(cr, "JetBrains Mono", 0, 10.5, px + pw - 54, ly + 23, mut_r, mut_g, mut_b, 1.0, dur);
            nature_add_hit(st, px, ly, pw, 38, HIT_MEDIA_AUDIO_SELECT, i, 0);
            ly += 44;
        }
        nature_draw_scrollbar(st, cr, cx + cw - 11, list_y, list_h,
                              MAX_AUDIO_TRACKS, vis, st->audio_scroll, max_scroll, SCROLL_MEDIA_AUDIO, dark);
    }
}

void nature_media_click(NatureOSState *st, const HitRect *hit) {
    int save_needed = 0;
    if (hit->action_id == HIT_MEDIA_MODE) {
        st->media_mode = hit->arg0;
        save_needed = 1;
    } else if (hit->action_id == HIT_MEDIA_GRID_TOGGLE) {
        st->media_grid_mode = !st->media_grid_mode;
        save_needed = 1;
    } else if (hit->action_id == HIT_MEDIA_SLIDESHOW) {
        st->media_slideshow = !st->media_slideshow;
        st->media_slideshow_ticks = 0;
        if (st->media_slideshow) st->media_grid_mode = 0;
    } else if (hit->action_id == HIT_MEDIA_SELECT) {
        st->media_selected = hit->arg0 % MAX_MEDIA_IMAGES;
        st->media_zoom_pct = 100;
        st->media_rotation = 0;
        if (hit->arg1 == 1) st->media_grid_mode = 0;
        save_needed = 1;
    } else if (hit->action_id == HIT_MEDIA_PREV) {
        st->media_selected = (st->media_selected + MAX_MEDIA_IMAGES - 1) % MAX_MEDIA_IMAGES;
        st->media_zoom_pct = 100;
        st->media_rotation = 0;
        save_needed = 1;
    } else if (hit->action_id == HIT_MEDIA_NEXT) {
        st->media_selected = (st->media_selected + 1) % MAX_MEDIA_IMAGES;
        st->media_zoom_pct = 100;
        st->media_rotation = 0;
        save_needed = 1;
    } else if (hit->action_id == HIT_MEDIA_ZOOM) {
        if (st->media_zoom_pct == 100) st->media_zoom_pct = 135;
        else if (st->media_zoom_pct == 135) st->media_zoom_pct = 175;
        else st->media_zoom_pct = 100;
    } else if (hit->action_id == HIT_MEDIA_ZOOM_IN) {
        st->media_zoom_pct += 25;
        clamp_zoom(st);
    } else if (hit->action_id == HIT_MEDIA_ZOOM_OUT) {
        st->media_zoom_pct -= 25;
        clamp_zoom(st);
    } else if (hit->action_id == HIT_MEDIA_ZOOM_FIT) {
        st->media_zoom_pct = 100;
    } else if (hit->action_id == HIT_MEDIA_ROTATE) {
        st->media_rotation = (st->media_rotation + 90) % 360;
    } else if (hit->action_id == HIT_MEDIA_SET_WP) {
        nature_os_set_wallpaper_from_media(st, hit->arg0);
        save_needed = 1;
    } else if (hit->action_id == HIT_MEDIA_AUDIO_PLAY) {
        st->audio_playing = !st->audio_playing;
        save_needed = 1;
    } else if (hit->action_id == HIT_MEDIA_AUDIO_SELECT) {
        st->audio_selected = hit->arg0 % MAX_AUDIO_TRACKS;
        st->audio_progress_sec = 0;
        st->audio_playing = 1;
        save_needed = 1;
    } else if (hit->action_id == HIT_MEDIA_AUDIO_LOOP) {
        st->audio_loop = !st->audio_loop;
        save_needed = 1;
    } else if (hit->action_id == HIT_MEDIA_AUDIO_SHUFFLE) {
        st->audio_shuffle = !st->audio_shuffle;
        save_needed = 1;
    } else if (hit->action_id == HIT_MEDIA_AUDIO_PREV) {
        st->audio_selected = (st->audio_selected + MAX_AUDIO_TRACKS - 1) % MAX_AUDIO_TRACKS;
        st->audio_progress_sec = 0;
        st->audio_playing = 1;
        save_needed = 1;
    } else if (hit->action_id == HIT_MEDIA_AUDIO_NEXT) {
        if (st->audio_shuffle) st->audio_selected = (st->audio_selected + 2) % MAX_AUDIO_TRACKS;
        else st->audio_selected = (st->audio_selected + 1) % MAX_AUDIO_TRACKS;
        st->audio_progress_sec = 0;
        st->audio_playing = 1;
        save_needed = 1;
    } else if (hit->action_id == HIT_MEDIA_AUDIO_MUTE) {
        st->cfg.audio_muted = !st->cfg.audio_muted;
        save_needed = 1;
    } else if (hit->action_id == HIT_MEDIA_AUDIO_SPEED) {
        if (st->audio_speed_x10 <= 5) st->audio_speed_x10 = 10;
        else if (st->audio_speed_x10 == 10) st->audio_speed_x10 = 15;
        else if (st->audio_speed_x10 == 15) st->audio_speed_x10 = 20;
        else st->audio_speed_x10 = 5;
        save_needed = 1;
    } else if (hit->action_id == HIT_MEDIA_AUDIO_STEP) {
        int dur = TRACKS[st->audio_selected % MAX_AUDIO_TRACKS].duration_sec;
        st->audio_progress_sec += hit->arg0;
        if (st->audio_progress_sec < 0) st->audio_progress_sec = 0;
        if (st->audio_progress_sec >= dur) st->audio_progress_sec = dur - 1;
    } else if (hit->action_id == HIT_MEDIA_INFO_TOGGLE) {
        st->media_show_exif = !st->media_show_exif;
        save_needed = 1;
    } else if (hit->action_id == HIT_MEDIA_AUDIO_EQ) {
        st->audio_eq_preset = (st->audio_eq_preset + 1) % 4;
        save_needed = 1;
    } else if (hit->action_id == HIT_MEDIA_AUDIO_SEEK) {
        int dur = TRACKS[st->audio_selected % MAX_AUDIO_TRACKS].duration_sec;
        int w = hit->arg0 > 0 ? hit->arg0 : 1;
        int rel_x = hit->arg1 >= 0 ? hit->arg1 : 0;
        int sec = (rel_x * dur) / w;
        if (sec < 0) sec = 0;
        if (sec >= dur) sec = dur - 1;
        st->audio_progress_sec = sec;
    }
    if (save_needed) {
        nature_os_save_state(st);
    }
}

void nature_media_key(NatureOSState *st, const char *key) {
    if (!key || !*key) return;
    if (st->media_mode == 0) {
        if (strcmp(key, "ArrowLeft") == 0) {
            st->media_selected = (st->media_selected + MAX_MEDIA_IMAGES - 1) % MAX_MEDIA_IMAGES;
            st->media_zoom_pct = 100;
            st->media_rotation = 0;
            nature_os_save_state(st);
        } else if (strcmp(key, "ArrowRight") == 0) {
            st->media_selected = (st->media_selected + 1) % MAX_MEDIA_IMAGES;
            st->media_zoom_pct = 100;
            st->media_rotation = 0;
            nature_os_save_state(st);
        } else if (strcmp(key, "g") == 0 || strcmp(key, "G") == 0) {
            st->media_grid_mode = !st->media_grid_mode;
            nature_os_save_state(st);
        } else if (strcmp(key, "r") == 0 || strcmp(key, "R") == 0) {
            st->media_rotation = (st->media_rotation + 90) % 360;
        } else if (strcmp(key, "+") == 0 || strcmp(key, "=") == 0) {
            st->media_zoom_pct += 25;
            clamp_zoom(st);
        } else if (strcmp(key, "-") == 0) {
            st->media_zoom_pct -= 25;
            clamp_zoom(st);
        } else if (strcmp(key, "i") == 0 || strcmp(key, "I") == 0) {
            st->media_show_exif = !st->media_show_exif;
            nature_os_save_state(st);
        } else if (strcmp(key, " ") == 0 || strcmp(key, "Space") == 0) {
            st->media_slideshow = !st->media_slideshow;
            st->media_slideshow_ticks = 0;
            if (st->media_slideshow) st->media_grid_mode = 0;
        }
    } else {
        int dur = TRACKS[st->audio_selected % MAX_AUDIO_TRACKS].duration_sec;
        if (strcmp(key, " ") == 0 || strcmp(key, "Space") == 0) {
            st->audio_playing = !st->audio_playing;
            nature_os_save_state(st);
        } else if (strcmp(key, "ArrowLeft") == 0) {
            st->audio_progress_sec -= 5;
            if (st->audio_progress_sec < 0) st->audio_progress_sec = 0;
        } else if (strcmp(key, "ArrowRight") == 0) {
            st->audio_progress_sec += 5;
            if (st->audio_progress_sec >= dur) st->audio_progress_sec = dur - 1;
        }
    }
}

void nature_media_wheel(NatureOSState *st, int delta_y) {
    if (st->media_mode == 0) {
        if (st->media_grid_mode) {
            if (delta_y > 0 && st->media_grid_scroll < st->media_grid_max_scroll) st->media_grid_scroll++;
            else if (delta_y < 0 && st->media_grid_scroll > 0) st->media_grid_scroll--;
        } else {
            if (delta_y < 0) {
                st->media_zoom_pct += 25;
                clamp_zoom(st);
            } else if (delta_y > 0) {
                st->media_zoom_pct -= 25;
                clamp_zoom(st);
            }
        }
    } else {
        if (delta_y > 0 && st->audio_scroll < st->audio_max_scroll) st->audio_scroll++;
        else if (delta_y < 0 && st->audio_scroll > 0) st->audio_scroll--;
    }
}
