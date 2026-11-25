#include "nature_os.h"
#include <math.h>
#include <stdio.h>

static const char *MEDIA_TITLES[MAX_MEDIA_IMAGES] = {
    "Woodland Fern Fronds",
    "Canopy Dawn (4K)",
    "Alpine Twilight (4K)",
    "Basalt Moss Dew (4K)",
    "Aurora Mist Horizon",
    "Verdant Canopy Rings"
};

static const char *MEDIA_META[MAX_MEDIA_IMAGES] = {
    "2400x1800 · Leica M11 35mm · f/2.0",
    "3840x2160 · Hasselblad X2D · ISO 100",
    "3840x2160 · Sony A7R V · f/4.0",
    "3840x2160 · Fujifilm GFX 100S · f/2.8",
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
    {"Night Moss Stream", "Olympic Field Archive", "48 kHz Uncompressed WAV", 175}
};

void nature_media_tick(NatureOSState *st) {
    if (!st->audio_playing) return;
    st->audio_tick_accum++;
    if (st->audio_tick_accum >= 7) {
        st->audio_tick_accum = 0;
        st->audio_progress_sec++;
        int dur = TRACKS[st->audio_selected % MAX_AUDIO_TRACKS].duration_sec;
        if (st->audio_progress_sec >= dur) {
            st->audio_progress_sec = 0;
            st->audio_selected = (st->audio_selected + 1) % MAX_AUDIO_TRACKS;
        }
    }
}

void nature_media_render(NatureOSState *st, cairo_t *cr, OSWindow *w, int cx, int cy, int cw, int ch) {
    (void)w;
    int dark = st->cfg.dark_mode;
    double ar, ag, ab;
    nature_get_accent(st->cfg.accent_idx, &ar, &ag, &ab);

    ngfx_fill_rect(cr, cx, cy, cw, ch, 0,
                   dark ? 0.036 : 0.96, dark ? 0.050 : 0.975, dark ? 0.046 : 0.97, 1.0);

    double fg_r = dark ? 0.94 : 0.10, fg_g = dark ? 0.97 : 0.14, fg_b = dark ? 0.95 : 0.12;
    double mut_r = dark ? 0.62 : 0.44, mut_g = dark ? 0.72 : 0.50, mut_b = dark ? 0.67 : 0.47;
    double brd_c = dark ? 1.0 : 0.0;

    /* Header Toolbar */
    ngfx_fill_rect(cr, cx, cy, cw, 44, 0,
                   dark ? 0.052 : 0.92, dark ? 0.074 : 0.94, dark ? 0.068 : 0.93, 1.0);
    g_ngfx.set_source_rgba(cr, brd_c, brd_c, brd_c, dark ? 0.08 : 0.09);
    g_ngfx.move_to(cr, cx, cy + 44.5);
    g_ngfx.line_to(cr, cx + cw, cy + 44.5);
    g_ngfx.stroke(cr);

    const char *modes[2] = {"Photos (6)", "Audio (3)"};
    for (int m = 0; m < 2; ++m) {
        int mx = cx + 10 + m * 82;
        int act = st->media_mode == m;
        int hov = (st->hover_hit_action == HIT_MEDIA_MODE && st->hover_hit_arg0 == m);
        if (act) {
            ngfx_fill_gradient(cr, mx, cy + 9, 78, 26, 7, ar, ag, ab, 0.95,
                               ar * 0.78, ag * 0.78, ab * 0.78, 0.95, 0);
            ngfx_top_specular(cr, mx, cy + 9, 78, 7, 0.22);
        } else {
            ngfx_fill_rect(cr, mx, cy + 9, 78, 26, 7, brd_c, brd_c, brd_c, hov ? 0.14 : 0.08);
            if (hov) ngfx_stroke_rect(cr, mx + 0.5, cy + 9.5, 77, 25, 7, 0.8, ar, ag, ab, 0.40);
        }
        ngfx_text(cr, "Noto Sans", act, 10.8, mx + 10, cy + 26,
                  act ? 1.0 : fg_r, act ? 1.0 : fg_g, act ? 1.0 : fg_b, 1.0, modes[m]);
        nature_add_hit(st, mx, cy + 9, 78, 26, HIT_MEDIA_MODE, m, 0);
    }

    if (st->media_mode == 0) {
        int sel = st->media_selected % MAX_MEDIA_IMAGES;
        if (cw >= 580) {
            ngfx_text_clipped(cr, "Plus Jakarta Sans", 1, 11.8, cx + 182, cy + 21, cw - 470, fg_r, fg_g, fg_b, 1.0, MEDIA_TITLES[sel]);
            ngfx_text_clipped(cr, "JetBrains Mono", 0, 9.5, cx + 182, cy + 36, cw - 470, mut_r, mut_g, mut_b, 0.9, MEDIA_META[sel]);
        }

        int px = cx + cw - 278;
        if (cw < 480) px = cx + 178;

        int hov_prev = (st->hover_hit_action == HIT_MEDIA_PREV);
        ngfx_fill_rect(cr, px, cy + 9, 26, 26, 7, brd_c, brd_c, brd_c, hov_prev ? 0.18 : 0.09);
        ngfx_stroke_rect(cr, px + 0.5, cy + 9.5, 25, 25, 7, 0.8, brd_c, brd_c, brd_c, hov_prev ? 0.22 : 0.08);
        g_ngfx.set_source_rgba(cr, fg_r, fg_g, fg_b, 0.95);
        g_ngfx.set_line_width(cr, 1.6);
        g_ngfx.set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
        g_ngfx.set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
        g_ngfx.move_to(cr, px + 15.5, cy + 17.5);
        g_ngfx.line_to(cr, px + 10.5, cy + 22.0);
        g_ngfx.line_to(cr, px + 15.5, cy + 26.5);
        g_ngfx.stroke(cr);
        nature_add_hit(st, px, cy + 9, 26, 26, HIT_MEDIA_PREV, 0, 0);

        int hov_next = (st->hover_hit_action == HIT_MEDIA_NEXT);
        ngfx_fill_rect(cr, px + 30, cy + 9, 26, 26, 7, brd_c, brd_c, brd_c, hov_next ? 0.18 : 0.09);
        ngfx_stroke_rect(cr, px + 30.5, cy + 9.5, 25, 25, 7, 0.8, brd_c, brd_c, brd_c, hov_next ? 0.22 : 0.08);
        g_ngfx.set_source_rgba(cr, fg_r, fg_g, fg_b, 0.95);
        g_ngfx.move_to(cr, px + 40.5, cy + 17.5);
        g_ngfx.line_to(cr, px + 45.5, cy + 22.0);
        g_ngfx.line_to(cr, px + 40.5, cy + 26.5);
        g_ngfx.stroke(cr);
        nature_add_hit(st, px + 30, cy + 9, 26, 26, HIT_MEDIA_NEXT, 0, 0);

        int hov_rot = (st->hover_hit_action == HIT_MEDIA_ROTATE);
        ngfx_fill_rect(cr, px + 60, cy + 9, 40, 26, 7, brd_c, brd_c, brd_c, hov_rot ? 0.18 : 0.09);
        ngfx_stroke_rect(cr, px + 60.5, cy + 9.5, 39, 25, 7, 0.8, brd_c, brd_c, brd_c, hov_rot ? 0.22 : 0.08);
        ngfx_text(cr, "Noto Sans", 1, 10.5, px + 69, cy + 26, fg_r, fg_g, fg_b, 1.0, "Rot");
        nature_add_hit(st, px + 60, cy + 9, 40, 26, HIT_MEDIA_ROTATE, 0, 0);

        int hov_zoom = (st->hover_hit_action == HIT_MEDIA_ZOOM);
        ngfx_fill_rect(cr, px + 104, cy + 9, 48, 26, 7, brd_c, brd_c, brd_c, hov_zoom ? 0.18 : 0.09);
        ngfx_stroke_rect(cr, px + 104.5, cy + 9.5, 47, 25, 7, 0.8, brd_c, brd_c, brd_c, hov_zoom ? 0.22 : 0.08);
        char zb[16];
        snprintf(zb, sizeof(zb), "%d%%", st->media_zoom_pct);
        ngfx_text(cr, "JetBrains Mono", 1, 10.2, px + 111, cy + 26, fg_r, fg_g, fg_b, 1.0, zb);
        nature_add_hit(st, px + 104, cy + 9, 48, 26, HIT_MEDIA_ZOOM, 0, 0);

        if (px + 266 <= cx + cw) {
            int hov_wp = (st->hover_hit_action == HIT_MEDIA_SET_WP);
            ngfx_fill_gradient(cr, px + 156, cy + 9, 110, 26, 7,
                               ar * (hov_wp ? 1.12 : 1.0), ag * (hov_wp ? 1.12 : 1.0), ab * (hov_wp ? 1.12 : 1.0), 0.96,
                               ar * 0.78, ag * 0.78, ab * 0.78, 0.96, 0);
            ngfx_top_specular(cr, px + 156, cy + 9, 110, 7, 0.22);
            ngfx_text(cr, "Noto Sans", 1, 10.5, px + 166, cy + 26, 1, 1, 1, 1, "Set Wallpaper");
            nature_add_hit(st, px + 156, cy + 9, 110, 26, HIT_MEDIA_SET_WP, sel, 0);
        }

        int view_h = ch - 122;
        cairo_surface_t *img = st->media_images[sel];
        if (img && view_h > 40) {
            double sw = g_ngfx.image_surface_get_width(img);
            double sh = g_ngfx.image_surface_get_height(img);
            int rot_side = (st->media_rotation == 90 || st->media_rotation == 270);
            double eff_w = rot_side ? sh : sw;
            double eff_h = rot_side ? sw : sh;
            double fit = (cw - 36) / eff_w;
            if ((view_h - 20) / eff_h < fit) fit = (view_h - 20) / eff_h;
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
                ngfx_stroke_rect(cr, mid_x - dw * 0.5, mid_y - dh * 0.5, dw, dh, 10, 1.0, 1.0, 1.0, 1.0, 0.16);
                ngfx_top_specular(cr, mid_x - dw * 0.5, mid_y - dh * 0.5, dw, 10, 0.15);
            }
        }

        int fy = cy + ch - 76;
        ngfx_fill_rect(cr, cx, fy, cw, 76, 0,
                       dark ? 0.044 : 0.92, dark ? 0.064 : 0.94, dark ? 0.058 : 0.93, 1.0);
        g_ngfx.set_source_rgba(cr, brd_c, brd_c, brd_c, dark ? 0.07 : 0.08);
        g_ngfx.move_to(cr, cx, fy + 0.5);
        g_ngfx.line_to(cr, cx + cw, fy + 0.5);
        g_ngfx.stroke(cr);

        int thumb_w = (cw - 24 - (MAX_MEDIA_IMAGES - 1) * 8) / MAX_MEDIA_IMAGES;
        if (thumb_w > 104) thumb_w = 104;
        int total_tw = MAX_MEDIA_IMAGES * thumb_w + (MAX_MEDIA_IMAGES - 1) * 8;
        int start_tx = cx + (cw - total_tw) / 2;

        for (int i = 0; i < MAX_MEDIA_IMAGES; ++i) {
            int tx = start_tx + i * (thumb_w + 8);
            int ty = fy + 9;
            int is_sel = (sel == i);
            int is_hov = (st->hover_hit_action == HIT_MEDIA_SELECT && st->hover_hit_arg0 == i);
            if (is_sel) {
                ngfx_shadow_rect(cr, tx, ty, thumb_w, 52, 7, 1.6, 0.35);
            }
            if (st->media_images[i]) {
                g_ngfx.save(cr);
                ngfx_rounded_path(cr, tx, ty, thumb_w, 52, 7);
                g_ngfx.clip(cr);
                double sw = g_ngfx.image_surface_get_width(st->media_images[i]);
                double sh = g_ngfx.image_surface_get_height(st->media_images[i]);
                g_ngfx.translate(cr, tx, ty);
                g_ngfx.scale(cr, (double)thumb_w / sw, 52.0 / sh);
                g_ngfx.set_source_surface(cr, st->media_images[i], 0, 0);
                g_ngfx.paint(cr);
                g_ngfx.restore(cr);
            }
            ngfx_stroke_rect(cr, tx, ty, thumb_w, 52, 7, is_sel ? 2.2 : (is_hov ? 1.5 : 1.0),
                             (is_sel || is_hov) ? ar : brd_c, (is_sel || is_hov) ? ag : brd_c, (is_sel || is_hov) ? ab : brd_c,
                             is_sel ? 1.0 : (is_hov ? 0.65 : 0.16));
            if (is_sel) {
                ngfx_fill_circle(cr, tx + thumb_w * 0.5, ty + 59.0, 2.6, ar * 1.35 > 1 ? 1 : ar * 1.35,
                                 ag * 1.35 > 1 ? 1 : ag * 1.35, ab * 1.35 > 1 ? 1 : ab * 1.35, 1.0);
            }
            nature_add_hit(st, tx, ty, thumb_w, 52, HIT_MEDIA_SELECT, i, 0);
        }
    } else {
        int sel = st->audio_selected % MAX_AUDIO_TRACKS;
        const AudioTrack *trk = &TRACKS[sel];
        int compact = cw < 520;
        int px = cx + 18, py = cy + 58, pw = cw - 44;

        ngfx_shadow_rect(cr, px, py, pw, 178, 14, 2.0, 0.24);
        ngfx_fill_rect(cr, px, py, pw, 178, 14, brd_c, brd_c, brd_c, 0.055);
        ngfx_stroke_rect(cr, px + 0.5, py + 0.5, pw - 1, 177, 14, 1.0, brd_c, brd_c, brd_c, dark ? 0.09 : 0.11);
        ngfx_top_specular(cr, px, py, pw, 14, dark ? 0.12 : 0.30);

        int asz = compact ? 96 : 140;
        int ax = px + 14, ay = py + (178 - asz) / 2;
        ngfx_shadow_rect(cr, ax, ay, asz, asz, 12, 1.8, 0.35);
        ngfx_fill_gradient(cr, ax, ay, asz, asz, 12,
                           ar * 0.95, ag * 0.95, ab * 0.95, 0.98,
                           0.05, 0.12, 0.16, 1.0, 0);
        ngfx_top_specular(cr, ax, ay, asz, 12, 0.24);

        /* Concentric vinyl disc + grooves */
        double acx = ax + asz * 0.5;
        double acy = ay + asz * 0.5;
        ngfx_fill_circle(cr, acx, acy, asz * 0.34, 0.05, 0.07, 0.06, 0.90);
        g_ngfx.set_source_rgba(cr, 1.0, 1.0, 1.0, 0.10);
        g_ngfx.set_line_width(cr, 0.9);
        g_ngfx.arc(cr, acx, acy, asz * 0.28, 0, 6.28318);
        g_ngfx.stroke(cr);
        g_ngfx.arc(cr, acx, acy, asz * 0.21, 0, 6.28318);
        g_ngfx.stroke(cr);
        ngfx_fill_circle(cr, acx, acy, asz * 0.11,
                         ar * 1.4 > 1 ? 1 : ar * 1.4,
                         ag * 1.4 > 1 ? 1 : ag * 1.4,
                         ab * 1.4 > 1 ? 1 : ab * 1.4, 1.0);
        ngfx_fill_circle(cr, acx, acy, 2.8, 0.05, 0.07, 0.06, 1.0);

        int info_x = ax + asz + 18;
        int info_w = pw - asz - 46;
        ngfx_text_clipped(cr, "Plus Jakarta Sans", 1, 17.0, info_x, py + 36, info_w, fg_r, fg_g, fg_b, 1.0, trk->title);
        ngfx_text_clipped(cr, "Noto Sans", 0, 12.0, info_x, py + 58, info_w, mut_r, mut_g, mut_b, 1.0, trk->artist);
        ngfx_text_clipped(cr, "JetBrains Mono", 0, 10.2, info_x, py + 76, info_w, ar * 1.3 > 1 ? 1 : ar * 1.3,
                          ag * 1.3 > 1 ? 1 : ag * 1.3, ab * 1.3 > 1 ? 1 : ab * 1.3, 0.95, trk->codec);

        int bars = compact ? 16 : 24;
        int bar_w = (info_w - (bars - 1) * 3) / bars;
        if (bar_w >= 3) {
            for (int b = 0; b < bars; ++b) {
                double h_norm = st->audio_playing
                    ? (0.25 + 0.70 * fabs(sin(b * 0.55 + st->audio_tick_accum * 0.9 + st->audio_progress_sec * 0.7)))
                    : (0.15 + 0.25 * fabs(sin(b * 0.7)));
                double bh = 32.0 * h_norm;
                double bx_bar = info_x + b * (bar_w + 3);
                double by_bar = py + 122 - bh;
                ngfx_fill_gradient(cr, bx_bar, by_bar, bar_w, bh, 2,
                                   ar * 1.35 > 1 ? 1 : ar * 1.35, ag * 1.35 > 1 ? 1 : ag * 1.35, ab * 1.35 > 1 ? 1 : ab * 1.35,
                                   st->audio_playing ? 0.95 : 0.42,
                                   ar * 0.7, ag * 0.7, ab * 0.7,
                                   st->audio_playing ? 0.65 : 0.25, 0);
                if (st->audio_playing) {
                    ngfx_fill_rect(cr, bx_bar, by_bar, bar_w, 2.0, 1, 1.0, 1.0, 1.0, 0.65);
                }
            }
        }

        int pby = py + 134;
        ngfx_fill_rect(cr, info_x, pby + 1, info_w, 8, 4, brd_c, brd_c, brd_c, 0.14);
        double pct = (double)st->audio_progress_sec / (double)trk->duration_sec;
        if (pct > 1.0) pct = 1.0;
        if (pct > 0.0) {
            double pw_fill = info_w * pct;
            if (pw_fill < 6) pw_fill = 6;
            ngfx_fill_gradient(cr, info_x, pby + 1, pw_fill, 8, 4,
                               ar * 1.25 > 1 ? 1 : ar * 1.25, ag * 1.25 > 1 ? 1 : ag * 1.25, ab * 1.25 > 1 ? 1 : ab * 1.25, 0.98,
                               ar * 0.85, ag * 0.85, ab * 0.85, 0.98, 1);
            ngfx_fill_circle(cr, info_x + pw_fill, pby + 5, 6.2, ar, ag, ab, 0.35);
            ngfx_fill_circle(cr, info_x + pw_fill, pby + 5, 4.4, 1.0, 1.0, 1.0, 0.98);
        }
        nature_add_hit(st, info_x, pby - 6, info_w, 22, HIT_MEDIA_AUDIO_SEEK, info_w, 0);

        char tbuf[32];
        snprintf(tbuf, sizeof(tbuf), "%02d:%02d / %02d:%02d",
                 st->audio_progress_sec / 60, st->audio_progress_sec % 60,
                 trk->duration_sec / 60, trk->duration_sec % 60);
        ngfx_text(cr, "JetBrains Mono", 0, 10.5, info_x, pby + 28, mut_r, mut_g, mut_b, 1.0, tbuf);

        int play_x = info_x + info_w - 88;
        int hov_play = (st->hover_hit_action == HIT_MEDIA_AUDIO_PLAY);
        ngfx_fill_gradient(cr, play_x, pby + 14, 88, 27, 8,
                           ar * (hov_play ? 1.12 : 1.0), ag * (hov_play ? 1.12 : 1.0), ab * (hov_play ? 1.12 : 1.0), 0.98,
                           ar * 0.78, ag * 0.78, ab * 0.78, 0.98, 0);
        ngfx_top_specular(cr, play_x, pby + 14, 88, 8, 0.24);
        if (st->audio_playing) {
            ngfx_fill_rect(cr, play_x + 15, pby + 22, 3.2, 11, 1, 1, 1, 1, 0.96);
            ngfx_fill_rect(cr, play_x + 21, pby + 22, 3.2, 11, 1, 1, 1, 1, 0.96);
        } else {
            g_ngfx.set_source_rgba(cr, 1.0, 1.0, 1.0, 0.96);
            g_ngfx.move_to(cr, play_x + 16, pby + 21.5);
            g_ngfx.line_to(cr, play_x + 25, pby + 27.5);
            g_ngfx.line_to(cr, play_x + 16, pby + 33.5);
            g_ngfx.close_path(cr);
            g_ngfx.fill(cr);
        }
        ngfx_text(cr, "Plus Jakarta Sans", 1, 11.2, play_x + 32, pby + 31.5, 1, 1, 1, 1,
                  st->audio_playing ? "Pause" : "Play");
        nature_add_hit(st, play_x, pby + 14, 88, 27, HIT_MEDIA_AUDIO_PLAY, 0, 0);

        /* Scrollable Playlist Rows */
        int ly_head = py + 196;
        ngfx_text(cr, "Plus Jakarta Sans", 1, 11.8, px, ly_head, mut_r, mut_g, mut_b, 1.0, "Sample Library (/home/nature/Music)");
        int list_y = ly_head + 12;
        int list_h = cy + ch - list_y - 8;
        int vis = list_h / 46;
        if (vis < 1) vis = 1;
        int max_scroll = MAX_AUDIO_TRACKS - vis;
        if (max_scroll < 0) max_scroll = 0;
        st->audio_max_scroll = max_scroll;
        if (st->audio_scroll > max_scroll) st->audio_scroll = max_scroll;
        if (st->audio_scroll < 0) st->audio_scroll = 0;

        int ly = list_y;
        for (int i = st->audio_scroll; i < MAX_AUDIO_TRACKS; ++i) {
            if (ly + 40 > cy + ch - 4) break;
            int is_sel = (sel == i);
            int is_hov = (st->hover_hit_action == HIT_MEDIA_AUDIO_SELECT && st->hover_hit_arg0 == i);
            ngfx_fill_rect(cr, px, ly, pw, 40, 9,
                           is_sel ? ar : brd_c, is_sel ? ag : brd_c, is_sel ? ab : brd_c,
                           is_sel ? 0.22 : (is_hov ? 0.08 : 0.04));
            if (is_sel) {
                ngfx_stroke_rect(cr, px + 0.5, ly + 0.5, pw - 1, 39, 9, 1.0, ar, ag, ab, 0.65);
                ngfx_fill_rect(cr, px + 1, ly + 8, 3, 24, 1.5, ar * 1.3 > 1 ? 1 : ar * 1.3,
                               ag * 1.3 > 1 ? 1 : ag * 1.3, ab * 1.3 > 1 ? 1 : ab * 1.3, 0.98);
            } else if (is_hov) {
                ngfx_stroke_rect(cr, px + 0.5, ly + 0.5, pw - 1, 39, 9, 0.8, brd_c, brd_c, brd_c, 0.14);
            }

            char dur[16];
            snprintf(dur, sizeof(dur), "%02d:%02d", TRACKS[i].duration_sec / 60, TRACKS[i].duration_sec % 60);
            ngfx_text_clipped(cr, "Plus Jakarta Sans", 1, 12.0, px + 16, ly + 18, pw - 88, fg_r, fg_g, fg_b, 1.0, TRACKS[i].title);
            ngfx_text_clipped(cr, "Noto Sans", 0, 10.2, px + 16, ly + 33, pw - 88, mut_r, mut_g, mut_b, 0.95, TRACKS[i].codec);
            ngfx_text(cr, "JetBrains Mono", 0, 10.8, px + pw - 54, ly + 25, mut_r, mut_g, mut_b, 1.0, dur);
            nature_add_hit(st, px, ly, pw, 40, HIT_MEDIA_AUDIO_SELECT, i, 0);
            ly += 46;
        }
        nature_draw_scrollbar(st, cr, cx + cw - 11, list_y, list_h,
                              MAX_AUDIO_TRACKS, vis, st->audio_scroll, max_scroll, SCROLL_MEDIA_AUDIO, dark);
    }
}

void nature_media_click(NatureOSState *st, const HitRect *hit) {
    if (hit->action_id == HIT_MEDIA_MODE) {
        st->media_mode = hit->arg0;
    } else if (hit->action_id == HIT_MEDIA_SELECT) {
        st->media_selected = hit->arg0 % MAX_MEDIA_IMAGES;
        st->media_zoom_pct = 100;
        st->media_rotation = 0;
    } else if (hit->action_id == HIT_MEDIA_PREV) {
        st->media_selected = (st->media_selected + MAX_MEDIA_IMAGES - 1) % MAX_MEDIA_IMAGES;
        st->media_zoom_pct = 100;
        st->media_rotation = 0;
    } else if (hit->action_id == HIT_MEDIA_NEXT) {
        st->media_selected = (st->media_selected + 1) % MAX_MEDIA_IMAGES;
        st->media_zoom_pct = 100;
        st->media_rotation = 0;
    } else if (hit->action_id == HIT_MEDIA_ZOOM) {
        if (st->media_zoom_pct == 100) st->media_zoom_pct = 135;
        else if (st->media_zoom_pct == 135) st->media_zoom_pct = 175;
        else st->media_zoom_pct = 100;
    } else if (hit->action_id == HIT_MEDIA_ROTATE) {
        st->media_rotation = (st->media_rotation + 90) % 360;
    } else if (hit->action_id == HIT_MEDIA_SET_WP) {
        int wp = (hit->arg0 >= 1 && hit->arg0 <= 3) ? (hit->arg0 - 1) : (hit->arg0 % 3);
        st->cfg.wallpaper_idx = wp;
        nature_notify(st, "Media Viewer", "Desktop wallpaper updated.");
    } else if (hit->action_id == HIT_MEDIA_AUDIO_PLAY) {
        st->audio_playing = !st->audio_playing;
    } else if (hit->action_id == HIT_MEDIA_AUDIO_SELECT) {
        st->audio_selected = hit->arg0 % MAX_AUDIO_TRACKS;
        st->audio_progress_sec = 0;
        st->audio_playing = 1;
    } else if (hit->action_id == HIT_MEDIA_AUDIO_SEEK) {
        int dur = TRACKS[st->audio_selected % MAX_AUDIO_TRACKS].duration_sec;
        int w = hit->arg0 > 0 ? hit->arg0 : 1;
        int rel_x = hit->arg1 >= 0 ? hit->arg1 : 0;
        int sec = (rel_x * dur) / w;
        if (sec < 0) sec = 0;
        if (sec >= dur) sec = dur - 1;
        st->audio_progress_sec = sec;
    }
}

void nature_media_wheel(NatureOSState *st, int delta_y) {
    if (st->media_mode == 0) {
        if (delta_y < 0 && st->media_zoom_pct < 175) {
            st->media_zoom_pct = st->media_zoom_pct == 100 ? 135 : 175;
        } else if (delta_y > 0 && st->media_zoom_pct > 100) {
            st->media_zoom_pct = st->media_zoom_pct == 175 ? 135 : 100;
        }
    } else {
        if (delta_y > 0 && st->audio_scroll < st->audio_max_scroll) st->audio_scroll++;
        else if (delta_y < 0 && st->audio_scroll > 0) st->audio_scroll--;
    }
}
