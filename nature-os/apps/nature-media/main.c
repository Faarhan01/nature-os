#include "nature_os.h"
#include "nature_theme.h"
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
    const char *codec;
    int duration_sec;
    int poster_idx;
    int scene_id;
} VideoClip;

static const VideoClip VIDEOS[MAX_VIDEO_CLIPS] = {
    {"Alpine Dawn Timelapse", "3840x2160 · 60fps H.265 VA-API", 32, 6, 0},
    {"Canopy Rain Stream", "2560x1440 · 60fps AV1 HW", 28, 1, 1},
    {"Aurora Horizon HDR", "3840x2160 · 60fps HDR10", 36, 9, 2},
    {"Compositor Bench 60fps", "1920x1080 · 120fps Cairo Shader", 24, 4, 3}
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

static int collect_vfs_folders(const NatureOSState *st, int *out_idx, int max_out) {
    int cnt = 0;
    for (int i = 0; i < st->vfs_count && cnt < max_out; ++i) {
        const VfsNode *n = &st->vfs[i];
        if (!n->is_dir) continue;
        if (strncmp(n->path, "/home/nature/", 13) != 0) continue;
        out_idx[cnt++] = i;
    }
    return cnt;
}

static int count_vfs_children(const NatureOSState *st, const char *dir_path) {
    int cnt = 0;
    for (int i = 0; i < st->vfs_count; ++i) {
        if (strcmp(st->vfs[i].parent, dir_path) == 0) cnt++;
    }
    return cnt;
}

void nature_media_tick(NatureOSState *st) {
    if (st->media_slideshow && st->media_folder_idx == 1 && st->media_mode == 0 && !st->media_grid_mode) {
        st->media_slideshow_ticks++;
        if (st->media_slideshow_ticks >= 25) {
            st->media_slideshow_ticks = 0;
            st->media_selected = (st->media_selected + 1) % MAX_MEDIA_IMAGES;
            st->media_zoom_pct = 100;
            st->media_rotation = 0;
        }
    }

    if (st->video_playing && (st->media_mode == 1 || st->media_folder_idx == 2)) {
        int vsp = st->video_speed_x10 > 0 ? st->video_speed_x10 : 10;
        int vsel = st->video_selected % MAX_VIDEO_CLIPS;
        if (vsel < 0) vsel = 0;
        int dur_ms = VIDEOS[vsel].duration_sec * 1000;
        st->video_progress_ms += (50 * vsp) / 10;
        if (st->video_progress_ms >= dur_ms) {
            st->video_progress_ms = 0;
            if (!st->video_loop) {
                st->video_selected = (vsel + 1) % MAX_VIDEO_CLIPS;
            }
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

static void draw_folder_glyph(cairo_t *cr, int x, int y, double r, double g, double b, double a) {
    g_ngfx.set_source_rgba(cr, r, g, b, a * 0.22);
    g_ngfx.move_to(cr, x + 1, y + 3);
    g_ngfx.line_to(cr, x + 6, y + 3);
    g_ngfx.line_to(cr, x + 7.5, y + 5);
    g_ngfx.line_to(cr, x + 13, y + 5);
    g_ngfx.line_to(cr, x + 13, y + 12.5);
    g_ngfx.line_to(cr, x + 1, y + 12.5);
    g_ngfx.close_path(cr);
    g_ngfx.fill_preserve(cr);
    g_ngfx.set_source_rgba(cr, r, g, b, a);
    g_ngfx.set_line_width(cr, 1.15);
    g_ngfx.stroke(cr);
}

static void render_media_sidebar(NatureOSState *st, cairo_t *cr, int sx, int sy, int sw, int sh, const NaturePalette *pal) {
    int dark = pal->dark_mode;
    double ar = pal->ar, ag = pal->ag, ab = pal->ab;
    double fg_r = pal->fg_r, fg_g = pal->fg_g, fg_b = pal->fg_b;
    double mut_r = pal->mut_r, mut_g = pal->mut_g, mut_b = pal->mut_b;

    ngfx_fill_rect(cr, sx, sy, sw, sh, 0, pal->sidebar_r, pal->sidebar_g, pal->sidebar_b, 0.96);
    g_ngfx.set_source_rgba(cr, pal->border_r, pal->border_g, pal->border_b, pal->border_a);
    g_ngfx.set_line_width(cr, 1.0);
    g_ngfx.move_to(cr, sx + sw - 0.5, sy);
    g_ngfx.line_to(cr, sx + sw - 0.5, sy + sh);
    g_ngfx.stroke(cr);

    int cur_y = sy + 10;
    ntheme_draw_section_header(cr, sx + 12, cur_y, sw - 24, pal, "LIBRARY", NULL);
    cur_y += 16;

    static const char *LIB_NAMES[4] = {"All Media", "Pictures", "Videos", "Music"};
    static const char *LIB_TAGS[4] = {"ALL", "IMG", "VID", "AUD"};
    const int lib_counts[4] = {MAX_MEDIA_IMAGES + MAX_VIDEO_CLIPS + MAX_AUDIO_TRACKS, MAX_MEDIA_IMAGES, MAX_VIDEO_CLIPS, MAX_AUDIO_TRACKS};

    for (int i = 0; i < 4; ++i) {
        int act = (st->media_folder_idx == i);
        int hov = (st->hover_hit_action == HIT_MEDIA_FOLDER && st->hover_hit_arg0 == i);
        if (act) {
            ngfx_fill_gradient(cr, sx + 8, cur_y, sw - 16, 26, 7,
                               ar, ag, ab, dark ? 0.28 : 0.16,
                               pal->ar_hi, pal->ag_hi, pal->ab_hi, dark ? 0.18 : 0.10, 1);
            ngfx_stroke_rect(cr, sx + 8.5, cur_y + 0.5, sw - 17, 25, 7, 0.9, ar, ag, ab, 0.65);
            ngfx_fill_rect(cr, sx + 9, cur_y + 5, 2.5, 16, 1.2, pal->ar_hi, pal->ag_hi, pal->ab_hi, 0.98);
        } else if (hov) {
            ngfx_fill_rect(cr, sx + 8, cur_y, sw - 16, 26, 7, pal->hover_r, pal->hover_g, pal->hover_b, 0.90);
        }
        ngfx_fill_rect(cr, sx + 15, cur_y + 6, 24, 14, 4,
                       act ? ar : pal->elev_r, act ? ag : pal->elev_g, act ? ab : pal->elev_b, act ? 0.90 : 0.85);
        ngfx_text(cr, "JetBrains Mono", 1, 7.6, sx + 18.5, cur_y + 15.5,
                  act ? pal->on_acc_r : mut_r, act ? pal->on_acc_g : mut_g, act ? pal->on_acc_b : mut_b, 0.96, LIB_TAGS[i]);
        ngfx_text_clipped(cr, "Noto Sans", act, 10.6, sx + 44, cur_y + 16.5, sw - 74,
                          act ? fg_r : pal->fg_sec_r, act ? fg_g : pal->fg_sec_g, act ? fg_b : pal->fg_sec_b, 0.98, LIB_NAMES[i]);
        char cbuf[12];
        snprintf(cbuf, sizeof(cbuf), "%d", lib_counts[i]);
        double cw_c = ngfx_text_width(cr, "JetBrains Mono", 0, 8.6, cbuf);
        ngfx_text(cr, "JetBrains Mono", 0, 8.6, sx + sw - 14 - cw_c, cur_y + 16,
                  act ? (dark ? pal->ar_hi : ar) : mut_r,
                  act ? (dark ? pal->ag_hi : ag) : mut_g,
                  act ? (dark ? pal->ab_hi : ab) : mut_b, 0.92, cbuf);
        nature_add_hit(st, sx + 8, cur_y, sw - 16, 26, HIT_MEDIA_FOLDER, i, 0);
        cur_y += 28;
    }

    cur_y += 8;
    ntheme_draw_section_header(cr, sx + 12, cur_y, sw - 24, pal, "FOLDERS", NULL);
    cur_y += 16;

    int f_idxs[24];
    int f_cnt = collect_vfs_folders(st, f_idxs, 24);
    int list_h = sy + sh - cur_y - 6;
    if (list_h < 28) list_h = 28;
    int vis = list_h / 27;
    if (vis < 1) vis = 1;
    int max_sc = f_cnt - vis;
    if (max_sc < 0) max_sc = 0;
    st->media_folder_max_scroll = max_sc;
    if (st->media_folder_scroll > max_sc) st->media_folder_scroll = max_sc;
    if (st->media_folder_scroll < 0) st->media_folder_scroll = 0;

    g_ngfx.save(cr);
    g_ngfx.rectangle(cr, sx, cur_y, sw - 8, list_h);
    g_ngfx.clip(cr);

    int fy = cur_y;
    for (int k = st->media_folder_scroll; k < f_cnt; ++k) {
        if (fy + 25 > sy + sh - 4) break;
        const VfsNode *fn = &st->vfs[f_idxs[k]];
        int fid = 4 + k;
        int act = (st->media_folder_idx == fid);
        int hov = (st->hover_hit_action == HIT_MEDIA_FOLDER && st->hover_hit_arg0 == fid);
        if (act) {
            ngfx_fill_gradient(cr, sx + 8, fy, sw - 20, 25, 7,
                               ar, ag, ab, dark ? 0.28 : 0.16,
                               pal->ar_hi, pal->ag_hi, pal->ab_hi, dark ? 0.18 : 0.10, 1);
            ngfx_stroke_rect(cr, sx + 8.5, fy + 0.5, sw - 21, 24, 7, 0.9, ar, ag, ab, 0.65);
        } else if (hov) {
            ngfx_fill_rect(cr, sx + 8, fy, sw - 20, 25, 7, pal->hover_r, pal->hover_g, pal->hover_b, 0.90);
        }
        draw_folder_glyph(cr, sx + 14, fy + 5,
                          act ? pal->ar_hi : pal->fg_sec_r,
                          act ? pal->ag_hi : pal->fg_sec_g,
                          act ? pal->ab_hi : pal->fg_sec_b, 0.95);
        ngfx_text_clipped(cr, "Noto Sans", act, 10.4, sx + 32, fy + 16, sw - 66,
                          act ? fg_r : pal->fg_sec_r, act ? fg_g : pal->fg_sec_g, act ? fg_b : pal->fg_sec_b, 0.96, fn->name);
        int nchild = count_vfs_children(st, fn->path);
        char nbuf[12];
        snprintf(nbuf, sizeof(nbuf), "%d", nchild);
        double ncw = ngfx_text_width(cr, "JetBrains Mono", 0, 8.4, nbuf);
        ngfx_text(cr, "JetBrains Mono", 0, 8.4, sx + sw - 16 - ncw, fy + 15.5, mut_r, mut_g, mut_b, 0.85, nbuf);
        nature_add_hit(st, sx + 8, fy, sw - 20, 25, HIT_MEDIA_FOLDER, fid, f_idxs[k]);
        fy += 27;
    }
    g_ngfx.restore(cr);

    nature_draw_scrollbar(st, cr, sx + sw - 10, cur_y, list_h, f_cnt, vis,
                          st->media_folder_scroll, max_sc, SCROLL_MEDIA_FOLDER, dark);
}

static void render_all_media_hub(NatureOSState *st, cairo_t *cr, int mx0, int my0, int mw0, int mh0, const NaturePalette *pal) {
    int dark = pal->dark_mode;
    double ar = pal->ar, ag = pal->ag, ab = pal->ab;
    double fg_r = pal->fg_r, fg_g = pal->fg_g, fg_b = pal->fg_b;
    double mut_r = pal->mut_r, mut_g = pal->mut_g, mut_b = pal->mut_b;

    int pad = 16;
    int px = mx0 + pad;
    int pw = mw0 - pad * 2 - 10;
    int total_h = 560;
    int vis_rows = mh0 > 0 ? mh0 / 120 : 3;
    if (vis_rows < 1) vis_rows = 1;
    int max_sc = total_h > mh0 ? (total_h - mh0 + 95) / 100 : 0;
    st->media_grid_max_scroll = max_sc;
    if (st->media_grid_scroll > max_sc) st->media_grid_scroll = max_sc;
    if (st->media_grid_scroll < 0) st->media_grid_scroll = 0;
    int y_off = st->media_grid_scroll * 100;

    g_ngfx.save(cr);
    g_ngfx.rectangle(cr, mx0, my0, mw0 - 12, mh0);
    g_ngfx.clip(cr);

    int y = my0 + 14 - y_off;
    ngfx_text(cr, "Plus Jakarta Sans", 1, 15.0, px, y + 14, fg_r, fg_g, fg_b, 1.0, "Media Library");
    ngfx_text(cr, "JetBrains Mono", 0, 9.6, px, y + 30, mut_r, mut_g, mut_b, 0.92,
              "11 Photos  ·  4 HD/4K Videos  ·  4 PipeWire Audio Tracks  ·  Browse folders on left");
    y += 46;

    /* Videos Row */
    ntheme_draw_section_header(cr, px, y, pw, pal, "VIDEOS (/home/nature/Videos)", NULL);
    y += 16;
    int vcols = pw >= 500 ? 2 : 1;
    int vcard_w = (pw - (vcols - 1) * 10) / vcols;
    for (int i = 0; i < MAX_VIDEO_CLIPS; ++i) {
        int c = i % vcols;
        int r = i / vcols;
        int vx = px + c * (vcard_w + 10);
        int vy = y + r * 66;
        int hov = (st->hover_hit_action == HIT_MEDIA_VIDEO_SELECT && st->hover_hit_arg0 == i);
        ngfx_fill_rect(cr, vx, vy, vcard_w, 58, 10, pal->card_r, pal->card_g, pal->card_b, 0.96);
        ngfx_stroke_rect(cr, vx + 0.5, vy + 0.5, vcard_w - 1, 57, 10, hov ? 1.3 : 0.9,
                         hov ? ar : pal->border_r, hov ? ag : pal->border_g, hov ? ab : pal->border_b,
                         hov ? 0.80 : pal->border_a);
        cairo_surface_t *poster = st->media_images[VIDEOS[i].poster_idx % MAX_MEDIA_IMAGES];
        if (poster) {
            g_ngfx.save(cr);
            ngfx_rounded_path(cr, vx + 6, vy + 6, 74, 46, 6);
            g_ngfx.clip(cr);
            double sw = g_ngfx.image_surface_get_width(poster), sh = g_ngfx.image_surface_get_height(poster);
            if (sw > 0 && sh > 0) {
                double sc = (74.0 / sw > 46.0 / sh) ? (74.0 / sw) : (46.0 / sh);
                g_ngfx.translate(cr, vx + 6 + (74.0 - sw * sc) * 0.5, vy + 6 + (46.0 - sh * sc) * 0.5);
                g_ngfx.scale(cr, sc, sc);
                g_ngfx.set_source_surface(cr, poster, 0, 0);
                g_ngfx.paint(cr);
            }
            g_ngfx.restore(cr);
        }
        ngfx_fill_circle(cr, vx + 43, vy + 29, 10, 0.04, 0.06, 0.08, 0.72);
        g_ngfx.set_source_rgba(cr, 1.0, 1.0, 1.0, 0.95);
        g_ngfx.move_to(cr, vx + 40, vy + 24.5);
        g_ngfx.line_to(cr, vx + 48, vy + 29);
        g_ngfx.line_to(cr, vx + 40, vy + 33.5);
        g_ngfx.close_path(cr);
        g_ngfx.fill(cr);

        ngfx_text_clipped(cr, "Plus Jakarta Sans", 1, 11.2, vx + 88, vy + 22, vcard_w - 134, fg_r, fg_g, fg_b, 1.0, VIDEOS[i].title);
        ngfx_text_clipped(cr, "JetBrains Mono", 0, 8.8, vx + 88, vy + 37, vcard_w - 100, mut_r, mut_g, mut_b, 0.92, VIDEOS[i].codec);
        char vdur[16];
        snprintf(vdur, sizeof(vdur), "00:%02d", VIDEOS[i].duration_sec);
        ngfx_text(cr, "JetBrains Mono", 1, 8.8, vx + vcard_w - 38, vy + 22, pal->ar_hi, pal->ag_hi, pal->ab_hi, 0.96, vdur);
        nature_add_hit(st, vx, vy, vcard_w, 58, HIT_MEDIA_VIDEO_SELECT, i, 0);
    }
    y += ((MAX_VIDEO_CLIPS + vcols - 1) / vcols) * 66 + 10;

    /* Pictures Row */
    ntheme_draw_section_header(cr, px, y, pw, pal, "PICTURES (/home/nature/Pictures)", NULL);
    y += 16;
    int pcols = pw >= 560 ? 4 : (pw >= 360 ? 3 : 2);
    int pcard_w = (pw - (pcols - 1) * 10) / pcols;
    int pcard_h = 92;
    int show_pics = pcols * 2;
    if (show_pics > MAX_MEDIA_IMAGES) show_pics = MAX_MEDIA_IMAGES;
    for (int i = 0; i < show_pics; ++i) {
        int c = i % pcols;
        int r = i / pcols;
        int cx_p = px + c * (pcard_w + 10);
        int cy_p = y + r * (pcard_h + 8);
        int hov = (st->hover_hit_action == HIT_MEDIA_SELECT && st->hover_hit_arg0 == i);
        ngfx_fill_rect(cr, cx_p, cy_p, pcard_w, pcard_h, 9, pal->card_r, pal->card_g, pal->card_b, 0.96);
        ngfx_stroke_rect(cr, cx_p + 0.5, cy_p + 0.5, pcard_w - 1, pcard_h - 1, 9, hov ? 1.3 : 0.9,
                         hov ? ar : pal->border_r, hov ? ag : pal->border_g, hov ? ab : pal->border_b,
                         hov ? 0.80 : pal->border_a);
        if (st->media_images[i]) {
            g_ngfx.save(cr);
            ngfx_rounded_path(cr, cx_p + 4, cy_p + 4, pcard_w - 8, pcard_h - 28, 6);
            g_ngfx.clip(cr);
            double sw = g_ngfx.image_surface_get_width(st->media_images[i]), sh = g_ngfx.image_surface_get_height(st->media_images[i]);
            if (sw > 0 && sh > 0) {
                double dw = (double)(pcard_w - 8), dh = (double)(pcard_h - 28);
                double sc = (dw / sw > dh / sh) ? (dw / sw) : (dh / sh);
                g_ngfx.translate(cr, cx_p + 4 + (dw - sw * sc) * 0.5, cy_p + 4 + (dh - sh * sc) * 0.5);
                g_ngfx.scale(cr, sc, sc);
                g_ngfx.set_source_surface(cr, st->media_images[i], 0, 0);
                g_ngfx.paint(cr);
            }
            g_ngfx.restore(cr);
        }
        ngfx_text_clipped(cr, "Noto Sans", 1, 9.6, cx_p + 7, cy_p + pcard_h - 9, pcard_w - 14, fg_r, fg_g, fg_b, 0.98, MEDIA_TITLES[i]);
        nature_add_hit(st, cx_p, cy_p, pcard_w, pcard_h, HIT_MEDIA_SELECT, i, 1);
    }
    y += ((show_pics + pcols - 1) / pcols) * (pcard_h + 8) + 10;

    /* Audio Row */
    ntheme_draw_section_header(cr, px, y, pw, pal, "MUSIC (/home/nature/Music)", NULL);
    y += 16;
    for (int i = 0; i < MAX_AUDIO_TRACKS; ++i) {
        int hov = (st->hover_hit_action == HIT_MEDIA_AUDIO_SELECT && st->hover_hit_arg0 == i);
        ngfx_fill_rect(cr, px, y, pw, 36, 8, pal->card_r, pal->card_g, pal->card_b, 0.96);
        ngfx_stroke_rect(cr, px + 0.5, y + 0.5, pw - 1, 35, 8, hov ? 1.2 : 0.8,
                         hov ? ar : pal->border_r, hov ? ag : pal->border_g, hov ? ab : pal->border_b,
                         hov ? 0.78 : pal->border_a);
        ngfx_fill_rect(cr, px + 8, y + 8, 28, 20, 5, ar, ag, ab, 0.22);
        ngfx_text(cr, "JetBrains Mono", 1, 8.2, px + 12, y + 21, pal->ar_hi, pal->ag_hi, pal->ab_hi, 0.96, "AUD");
        ngfx_text_clipped(cr, "Plus Jakarta Sans", 1, 10.8, px + 44, y + 16, pw - 110, fg_r, fg_g, fg_b, 1.0, TRACKS[i].title);
        ngfx_text_clipped(cr, "Noto Sans", 0, 9.2, px + 44, y + 29, pw - 110, mut_r, mut_g, mut_b, 0.92, TRACKS[i].codec);
        char tdur[16];
        snprintf(tdur, sizeof(tdur), "%02d:%02d", TRACKS[i].duration_sec / 60, TRACKS[i].duration_sec % 60);
        ngfx_text(cr, "JetBrains Mono", 0, 9.8, px + pw - 48, y + 21, mut_r, mut_g, mut_b, 1.0, tdur);
        nature_add_hit(st, px, y, pw, 36, HIT_MEDIA_AUDIO_SELECT, i, 0);
        y += 42;
    }

    g_ngfx.restore(cr);
    nature_draw_scrollbar(st, cr, mx0 + mw0 - 11, my0 + 8, mh0 - 16,
                          4, 3, st->media_grid_scroll, max_sc, SCROLL_MEDIA_GRID, dark);
}

static void render_vfs_folder_view(NatureOSState *st, cairo_t *cr, int mx0, int my0, int mw0, int mh0, const NaturePalette *pal) {
    int dark = pal->dark_mode;
    double ar = pal->ar, ag = pal->ag, ab = pal->ab;
    double fg_r = pal->fg_r, fg_g = pal->fg_g, fg_b = pal->fg_b;
    double mut_r = pal->mut_r, mut_g = pal->mut_g, mut_b = pal->mut_b;

    const char *dir = st->media_vfs_dir[0] ? st->media_vfs_dir : "/home/nature/Pictures";
    int items[64];
    int n_items = 0;
    for (int i = 0; i < st->vfs_count && n_items < 64; ++i) {
        if (strcmp(st->vfs[i].parent, dir) == 0) items[n_items++] = i;
    }

    int pad = 16;
    int px = mx0 + pad;
    int pw = mw0 - pad * 2 - 10;

    ngfx_text_clipped(cr, "Plus Jakarta Sans", 1, 14.5, px, my0 + 26, pw - 130, fg_r, fg_g, fg_b, 1.0, dir);
    char sub[64];
    snprintf(sub, sizeof(sub), "%d item%s in folder · click to open or play", n_items, n_items == 1 ? "" : "s");
    ngfx_text(cr, "JetBrains Mono", 0, 9.4, px, my0 + 42, mut_r, mut_g, mut_b, 0.92, sub);

    int of_x = mx0 + mw0 - 118;
    int of_hov = (st->hover_hit_action == HIT_MEDIA_FOLDER_OPEN_FILES);
    ngfx_fill_rect(cr, of_x, my0 + 14, 102, 26, 7, of_hov ? ar : pal->elev_r, of_hov ? ag : pal->elev_g, of_hov ? ab : pal->elev_b, 0.94);
    ngfx_stroke_rect(cr, of_x + 0.5, my0 + 14.5, 101, 25, 7, 0.8, of_hov ? ar : pal->border_r, of_hov ? ag : pal->border_g, of_hov ? ab : pal->border_b, pal->border_a);
    ngfx_text(cr, "Noto Sans", 1, 9.8, of_x + 12, my0 + 30.5,
              of_hov ? pal->on_acc_r : fg_r, of_hov ? pal->on_acc_g : fg_g, of_hov ? pal->on_acc_b : fg_b, 0.98, "Open in Files");
    nature_add_hit(st, of_x, my0 + 14, 102, 26, HIT_MEDIA_FOLDER_OPEN_FILES, 0, 0);

    int ly0 = my0 + 54;
    int lh = mh0 - 60;
    int vis = lh / 46;
    if (vis < 1) vis = 1;
    int max_sc = n_items - vis;
    if (max_sc < 0) max_sc = 0;
    st->media_grid_max_scroll = max_sc;
    if (st->media_grid_scroll > max_sc) st->media_grid_scroll = max_sc;
    if (st->media_grid_scroll < 0) st->media_grid_scroll = 0;

    if (n_items == 0) {
        ngfx_fill_rect(cr, px, ly0 + 8, pw, 84, 12, pal->card_r, pal->card_g, pal->card_b, 0.80);
        ngfx_stroke_rect(cr, px + 0.5, ly0 + 8.5, pw - 1, 83, 12, 0.8, pal->border_r, pal->border_g, pal->border_b, pal->border_a);
        ngfx_text(cr, "Noto Sans", 0, 11.5, px + 20, ly0 + 54, mut_r, mut_g, mut_b, 0.90, "This folder has no media items yet.");
        return;
    }

    int ly = ly0;
    for (int k = st->media_grid_scroll; k < n_items; ++k) {
        if (ly + 40 > my0 + mh0 - 4) break;
        const VfsNode *vn = &st->vfs[items[k]];
        int hov = (st->hover_hit_action == HIT_MEDIA_OPEN_VFS && st->hover_hit_arg0 == items[k]);
        ngfx_fill_rect(cr, px, ly, pw, 40, 9, pal->card_r, pal->card_g, pal->card_b, 0.96);
        ngfx_stroke_rect(cr, px + 0.5, ly + 0.5, pw - 1, 39, 9, hov ? 1.2 : 0.8,
                         hov ? ar : pal->border_r, hov ? ag : pal->border_g, hov ? ab : pal->border_b,
                         hov ? 0.78 : pal->border_a);

        if (vn->image_idx >= 0 && st->media_images[vn->image_idx % MAX_MEDIA_IMAGES]) {
            cairo_surface_t *th = st->media_images[vn->image_idx % MAX_MEDIA_IMAGES];
            g_ngfx.save(cr);
            ngfx_rounded_path(cr, px + 6, ly + 5, 44, 30, 6);
            g_ngfx.clip(cr);
            double sw = g_ngfx.image_surface_get_width(th), sh = g_ngfx.image_surface_get_height(th);
            if (sw > 0 && sh > 0) {
                double sc = (44.0 / sw > 30.0 / sh) ? (44.0 / sw) : (30.0 / sh);
                g_ngfx.translate(cr, px + 6 + (44.0 - sw * sc) * 0.5, ly + 5 + (30.0 - sh * sc) * 0.5);
                g_ngfx.scale(cr, sc, sc);
                g_ngfx.set_source_surface(cr, th, 0, 0);
                g_ngfx.paint(cr);
            }
            g_ngfx.restore(cr);
        } else {
            ngfx_fill_rect(cr, px + 8, ly + 8, 38, 24, 6, ar, ag, ab, 0.18);
            ngfx_text(cr, "JetBrains Mono", 1, 8.6, px + 14, ly + 23.5, pal->ar_hi, pal->ag_hi, pal->ab_hi, 0.96,
                      vn->is_dir ? "DIR" : (vn->mime_tag[0] ? vn->mime_tag : "FILE"));
        }

        ngfx_text_clipped(cr, "Plus Jakarta Sans", 1, 11.4, px + 58, ly + 17.5, pw - 140, fg_r, fg_g, fg_b, 1.0, vn->name);
        ngfx_text_clipped(cr, "Noto Sans", 0, 9.4, px + 58, ly + 31.5, pw - 140, mut_r, mut_g, mut_b, 0.90,
                          vn->content[0] ? vn->content : vn->path);

        const char *act_lbl = vn->video_idx >= 0 ? "Play Video" : (vn->audio_idx >= 0 ? "Play Audio" : (vn->image_idx >= 0 ? "View Photo" : "Open"));
        ngfx_text(cr, "JetBrains Mono", 1, 8.8, px + pw - 74, ly + 23.5, pal->ar_hi, pal->ag_hi, pal->ab_hi, 0.95, act_lbl);
        nature_add_hit(st, px, ly, pw, 40, HIT_MEDIA_OPEN_VFS, items[k], 0);
        ly += 46;
    }

    nature_draw_scrollbar(st, cr, mx0 + mw0 - 11, ly0, lh, n_items, vis,
                          st->media_grid_scroll, max_sc, SCROLL_MEDIA_GRID, dark);
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

    int m_cur_x = cx + 10;

    /* Left Sidebar Toggle Button */
    int sb_hov = (st->hover_hit_action == HIT_MEDIA_SIDEBAR_TOGGLE);
    int sb_act = st->media_sidebar_open;
    ngfx_fill_rect(cr, m_cur_x, cy + 9, 64, 26, 7,
                   sb_act ? ar : (sb_hov ? pal.hover_r : pal.elev_r),
                   sb_act ? ag : (sb_hov ? pal.hover_g : pal.elev_g),
                   sb_act ? ab : (sb_hov ? pal.hover_b : pal.elev_b), sb_act ? 0.25 : 0.94);
    ngfx_stroke_rect(cr, m_cur_x + 0.5, cy + 9.5, 63, 25, 7, 0.8,
                     sb_act ? ar : pal.border_r, sb_act ? ag : pal.border_g, sb_act ? ab : pal.border_b,
                     sb_act ? 0.65 : pal.border_a);
    draw_folder_glyph(cr, m_cur_x + 6, cy + 14.5, sb_act ? pal.ar_hi : fg_r, sb_act ? pal.ag_hi : fg_g, sb_act ? pal.ab_hi : fg_b, 0.95);
    ngfx_text(cr, "Noto Sans", 1, 9.6, m_cur_x + 23, cy + 25.5,
              sb_act ? (dark ? pal.ar_hi : ar) : fg_r,
              sb_act ? (dark ? pal.ag_hi : ag) : fg_g,
              sb_act ? (dark ? pal.ab_hi : ab) : fg_b, 0.98, "Folders");
    nature_add_hit(st, m_cur_x, cy + 9, 64, 26, HIT_MEDIA_SIDEBAR_TOGGLE, 0, 0);
    m_cur_x += 70;

    const char *modes[3] = {"Photos", "Videos", "Audio"};
    const int mode_ws[3] = {58, 58, 52};
    for (int m = 0; m < 3; ++m) {
        int mw = mode_ws[m];
        int mx = m_cur_x;
        int act = (st->media_folder_idx == m + 1) || (st->media_folder_idx < 0 && st->media_mode == m);
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
        ngfx_text(cr, "Noto Sans", act, 10.0, mx + 9, cy + 25.5,
                  act ? pal.on_acc_r : fg_r, act ? pal.on_acc_g : fg_g, act ? pal.on_acc_b : fg_b, 1.0, modes[m]);
        nature_add_hit(st, mx, cy + 9, mw, 26, HIT_MEDIA_MODE, m, 0);
        m_cur_x += mw + 4;
    }

    int sidebar_w = (st->media_sidebar_open && cw >= 420) ? (cw >= 600 ? 162 : 138) : 0;
    int mx0 = cx + sidebar_w;
    int my0 = cy + 44;
    int mw0 = cw - sidebar_w;
    int mh0 = ch - 44;

    if (sidebar_w > 0) {
        render_media_sidebar(st, cr, cx, my0, sidebar_w, mh0, &pal);
    }

    if (st->media_folder_idx == 0) {
        render_all_media_hub(st, cr, mx0, my0, mw0, mh0, &pal);
        return;
    }
    if (st->media_folder_idx >= 4) {
        render_vfs_folder_view(st, cr, mx0, my0, mw0, mh0, &pal);
        return;
    }

    if (st->media_mode == 0) {
        int sel = st->media_selected % MAX_MEDIA_IMAGES;
        if (sel < 0) sel = 0;
        int compact = cw < 720;

        /* Grid/Stage & Slideshow toggles after mode pills */
        int gx_btn = m_cur_x + 4;
        int ctrl_w = compact ? 236 : 368;
        if (gx_btn + 94 < cx + cw - ctrl_w - 6) {
            int ghov = (st->hover_hit_action == HIT_MEDIA_GRID_TOGGLE);
            ngfx_fill_rect(cr, gx_btn, cy + 9, 44, 26, 7,
                           st->media_grid_mode ? ar : (ghov ? pal.hover_r : pal.elev_r),
                           st->media_grid_mode ? ag : (ghov ? pal.hover_g : pal.elev_g),
                           st->media_grid_mode ? ab : (ghov ? pal.hover_b : pal.elev_b), 0.94);
            ngfx_text(cr, "Noto Sans", 1, 9.8, gx_btn + 9, cy + 25.5,
                      st->media_grid_mode ? pal.on_acc_r : fg_r,
                      st->media_grid_mode ? pal.on_acc_g : fg_g,
                      st->media_grid_mode ? pal.on_acc_b : fg_b, 1.0,
                      st->media_grid_mode ? "Stage" : "Grid");
            nature_add_hit(st, gx_btn, cy + 9, 44, 26, HIT_MEDIA_GRID_TOGGLE, 0, 0);

            int sx_btn = gx_btn + 48;
            int shov = (st->hover_hit_action == HIT_MEDIA_SLIDESHOW);
            ngfx_fill_rect(cr, sx_btn, cy + 9, 44, 26, 7,
                           st->media_slideshow ? pal.warning_r : (shov ? pal.hover_r : pal.elev_r),
                           st->media_slideshow ? pal.warning_g : (shov ? pal.hover_g : pal.elev_g),
                           st->media_slideshow ? pal.warning_b : (shov ? pal.hover_b : pal.elev_b), 0.94);
            ngfx_text(cr, "Noto Sans", 1, 9.8, sx_btn + 7, cy + 25.5,
                      st->media_slideshow ? 1.0 : fg_r,
                      st->media_slideshow ? 1.0 : fg_g,
                      st->media_slideshow ? 1.0 : fg_b, 1.0,
                      st->media_slideshow ? "Play*" : "Slide");
            nature_add_hit(st, sx_btn, cy + 9, 44, 26, HIT_MEDIA_SLIDESHOW, 0, 0);
        }

        /* Right side stage controls */
        int px = cx + cw - ctrl_w - 8;
        if (px < m_cur_x + 4) px = m_cur_x + 4;

        /* Prev / Next */
        int pv_hov = (st->hover_hit_action == HIT_MEDIA_PREV);
        ngfx_fill_rect(cr, px, cy + 9, 24, 26, 7, pv_hov ? pal.hover_r : pal.elev_r, pv_hov ? pal.hover_g : pal.elev_g, pv_hov ? pal.hover_b : pal.elev_b, 0.94);
        ngfx_stroke_rect(cr, px + 0.5, cy + 9.5, 23, 25, 7, 0.8, pv_hov ? ar : pal.border_r, pv_hov ? ag : pal.border_g, pv_hov ? ab : pal.border_b, pv_hov ? 0.55 : pal.border_a);
        g_ngfx.set_source_rgba(cr, fg_r, fg_g, fg_b, 0.95);
        g_ngfx.set_line_width(cr, 1.6);
        g_ngfx.move_to(cr, px + 14.0, cy + 17.5); g_ngfx.line_to(cr, px + 9.0, cy + 22.0); g_ngfx.line_to(cr, px + 14.0, cy + 26.5);
        g_ngfx.stroke(cr);
        nature_add_hit(st, px, cy + 9, 24, 26, HIT_MEDIA_PREV, 0, 0);

        int nx_hov = (st->hover_hit_action == HIT_MEDIA_NEXT);
        ngfx_fill_rect(cr, px + 27, cy + 9, 24, 26, 7, nx_hov ? pal.hover_r : pal.elev_r, nx_hov ? pal.hover_g : pal.elev_g, nx_hov ? pal.hover_b : pal.elev_b, 0.94);
        ngfx_stroke_rect(cr, px + 27.5, cy + 9.5, 23, 25, 7, 0.8, nx_hov ? ar : pal.border_r, nx_hov ? ag : pal.border_g, nx_hov ? ab : pal.border_b, nx_hov ? 0.55 : pal.border_a);
        g_ngfx.move_to(cr, px + 37.0, cy + 17.5); g_ngfx.line_to(cr, px + 42.0, cy + 22.0); g_ngfx.line_to(cr, px + 37.0, cy + 26.5);
        g_ngfx.stroke(cr);
        nature_add_hit(st, px + 27, cy + 9, 24, 26, HIT_MEDIA_NEXT, 0, 0);

        int next_x = px + 55;
        if (!compact) {
            int zo_hov = (st->hover_hit_action == HIT_MEDIA_ZOOM_OUT);
            ngfx_fill_rect(cr, next_x, cy + 9, 22, 26, 7, zo_hov ? pal.hover_r : pal.elev_r, zo_hov ? pal.hover_g : pal.elev_g, zo_hov ? pal.hover_b : pal.elev_b, 0.94);
            ngfx_text(cr, "JetBrains Mono", 1, 11.2, next_x + 7, cy + 25.5, fg_r, fg_g, fg_b, 1.0, "-");
            nature_add_hit(st, next_x, cy + 9, 22, 26, HIT_MEDIA_ZOOM_OUT, 0, 0);
            next_x += 24;

            char zb[16];
            snprintf(zb, sizeof(zb), "%d%%", st->media_zoom_pct);
            int zp_hov = (st->hover_hit_action == HIT_MEDIA_ZOOM);
            ngfx_fill_rect(cr, next_x, cy + 9, 42, 26, 7, zp_hov ? pal.hover_r : pal.elev_r, zp_hov ? pal.hover_g : pal.elev_g, zp_hov ? pal.hover_b : pal.elev_b, 0.94);
            ngfx_text(cr, "JetBrains Mono", 1, 9.6, next_x + 5, cy + 25.5, fg_r, fg_g, fg_b, 1.0, zb);
            nature_add_hit(st, next_x, cy + 9, 42, 26, HIT_MEDIA_ZOOM, 0, 0);
            next_x += 44;

            int zi_hov = (st->hover_hit_action == HIT_MEDIA_ZOOM_IN);
            ngfx_fill_rect(cr, next_x, cy + 9, 22, 26, 7, zi_hov ? pal.hover_r : pal.elev_r, zi_hov ? pal.hover_g : pal.elev_g, zi_hov ? pal.hover_b : pal.elev_b, 0.94);
            ngfx_text(cr, "JetBrains Mono", 1, 11.2, next_x + 7, cy + 25.5, fg_r, fg_g, fg_b, 1.0, "+");
            nature_add_hit(st, next_x, cy + 9, 22, 26, HIT_MEDIA_ZOOM_IN, 0, 0);
            next_x += 25;

            int zf_hov = (st->hover_hit_action == HIT_MEDIA_ZOOM_FIT);
            ngfx_fill_rect(cr, next_x, cy + 9, 28, 26, 7, zf_hov ? pal.hover_r : pal.elev_r, zf_hov ? pal.hover_g : pal.elev_g, zf_hov ? pal.hover_b : pal.elev_b, 0.94);
            ngfx_text(cr, "Noto Sans", 1, 9.8, next_x + 7, cy + 25.5, fg_r, fg_g, fg_b, 1.0, "Fit");
            nature_add_hit(st, next_x, cy + 9, 28, 26, HIT_MEDIA_ZOOM_FIT, 0, 0);
            next_x += 31;
        }

        int rt_hov = (st->hover_hit_action == HIT_MEDIA_ROTATE);
        ngfx_fill_rect(cr, next_x, cy + 9, 32, 26, 7, rt_hov ? pal.hover_r : pal.elev_r, rt_hov ? pal.hover_g : pal.elev_g, rt_hov ? pal.hover_b : pal.elev_b, 0.94);
        ngfx_text(cr, "Noto Sans", 1, 9.8, next_x + 6, cy + 25.5, fg_r, fg_g, fg_b, 1.0, "Rot");
        nature_add_hit(st, next_x, cy + 9, 32, 26, HIT_MEDIA_ROTATE, 0, 0);
        next_x += 35;

        static const char *FILTER_SHORT[4] = {"Raw", "Vivid", "Noir", "Warm"};
        int flt_idx = st->media_filter % 4;
        if (flt_idx < 0) flt_idx = 0;
        int flt_hov = (st->hover_hit_action == HIT_MEDIA_FILTER);
        ngfx_fill_rect(cr, next_x, cy + 9, 42, 26, 7,
                       flt_idx != 0 ? ar : (flt_hov ? pal.hover_r : pal.elev_r),
                       flt_idx != 0 ? ag : (flt_hov ? pal.hover_g : pal.elev_g),
                       flt_idx != 0 ? ab : (flt_hov ? pal.hover_b : pal.elev_b), 0.94);
        ngfx_text_clipped(cr, "Noto Sans", 1, 9.6, next_x + 6, cy + 25.5, 34,
                          flt_idx != 0 ? pal.on_acc_r : fg_r,
                          flt_idx != 0 ? pal.on_acc_g : fg_g,
                          flt_idx != 0 ? pal.on_acc_b : fg_b, 1.0, FILTER_SHORT[flt_idx]);
        nature_add_hit(st, next_x, cy + 9, 42, 26, HIT_MEDIA_FILTER, 0, 0);
        next_x += 45;

        int in_hov = (st->hover_hit_action == HIT_MEDIA_INFO_TOGGLE);
        ngfx_fill_rect(cr, next_x, cy + 9, 34, 26, 7,
                       st->media_show_exif ? ar : (in_hov ? pal.hover_r : pal.elev_r),
                       st->media_show_exif ? ag : (in_hov ? pal.hover_g : pal.elev_g),
                       st->media_show_exif ? ab : (in_hov ? pal.hover_b : pal.elev_b), 0.94);
        ngfx_text(cr, "Noto Sans", 1, 9.8, next_x + 7, cy + 25.5,
                  st->media_show_exif ? pal.on_acc_r : fg_r,
                  st->media_show_exif ? pal.on_acc_g : fg_g,
                  st->media_show_exif ? pal.on_acc_b : fg_b, 1.0, "Info");
        nature_add_hit(st, next_x, cy + 9, 34, 26, HIT_MEDIA_INFO_TOGGLE, 0, 0);
        next_x += 37;

        int wp_btn_w = (cx + cw - next_x - 8);
        if (wp_btn_w >= 72) {
            if (wp_btn_w > 98) wp_btn_w = 98;
            int wphov = (st->hover_hit_action == HIT_MEDIA_SET_WP);
            ngfx_fill_gradient(cr, next_x, cy + 9, wp_btn_w, 26, 7,
                               ar * (wphov ? 1.12 : 1.0), ag * (wphov ? 1.12 : 1.0), ab * (wphov ? 1.12 : 1.0), 0.96,
                               ar * 0.78, ag * 0.78, ab * 0.78, 0.96, 0);
            ngfx_top_specular(cr, next_x, cy + 9, wp_btn_w, 7, 0.22);
            ngfx_text_clipped(cr, "Noto Sans", 1, 9.8, next_x + 8, cy + 25.5, wp_btn_w - 14,
                              pal.on_acc_r, pal.on_acc_g, pal.on_acc_b, 1, "Wallpaper");
            nature_add_hit(st, next_x, cy + 9, wp_btn_w, 26, HIT_MEDIA_SET_WP, sel, 0);
        }

        if (st->media_grid_mode) {
            int cols = mw0 >= 520 ? 3 : 2;
            int gx_pad = 14;
            int gap = 10;
            int card_w = (mw0 - gx_pad * 2 - 14 - (cols - 1) * gap) / cols;
            int card_h = (card_w * 9) / 16 + 32;
            if (card_h < 100) card_h = 100;
            int total_rows = (MAX_MEDIA_IMAGES + cols - 1) / cols;
            int body_y = my0 + 10;
            int body_h = mh0 - 16;
            int vis_rows = body_h / (card_h + 10);
            if (vis_rows < 1) vis_rows = 1;
            int max_sc = total_rows - vis_rows;
            if (max_sc < 0) max_sc = 0;
            st->media_grid_max_scroll = max_sc;
            if (st->media_grid_scroll > max_sc) st->media_grid_scroll = max_sc;
            if (st->media_grid_scroll < 0) st->media_grid_scroll = 0;

            g_ngfx.save(cr);
            g_ngfx.rectangle(cr, mx0, my0 + 1, mw0 - 14, mh0 - 2);
            g_ngfx.clip(cr);

            int start_idx = st->media_grid_scroll * cols;
            for (int k = start_idx; k < MAX_MEDIA_IMAGES; ++k) {
                int slot = k - start_idx;
                int c = slot % cols;
                int r = slot / cols;
                int xcard = mx0 + gx_pad + c * (card_w + gap);
                int ycard = body_y + r * (card_h + 10);
                if (ycard > cy + ch - 4) break;
                int is_sel = (sel == k);

                ngfx_fill_rect(cr, xcard, ycard, card_w, card_h, 10, pal.card_r, pal.card_g, pal.card_b, 0.96);
                ngfx_stroke_rect(cr, xcard + 0.5, ycard + 0.5, card_w - 1, card_h - 1, 10, is_sel ? 1.8 : 0.9,
                                 is_sel ? ar : pal.border_r, is_sel ? ag : pal.border_g, is_sel ? ab : pal.border_b,
                                 is_sel ? 0.95 : pal.border_a);

                int ih = card_h - 30;
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
                ngfx_text_clipped(cr, "Noto Sans", is_sel, 10.4, xcard + 8, ycard + ih + 15, card_w - 16, fg_r, fg_g, fg_b, 1.0, MEDIA_TITLES[k]);
                nature_add_hit(st, xcard, ycard, card_w, card_h, HIT_MEDIA_SELECT, k, 1);
            }
            g_ngfx.restore(cr);

            nature_draw_scrollbar(st, cr, mx0 + mw0 - 11, body_y, body_h,
                                  total_rows, vis_rows, st->media_grid_scroll, max_sc, SCROLL_MEDIA_GRID, dark);
        } else {
            int foot_meta_h = 26;
            int thumb_bar_h = 74;
            int view_h = mh0 - foot_meta_h - thumb_bar_h;
            cairo_surface_t *img = st->media_images[sel];
            if (img && view_h > 40) {
                double sw = g_ngfx.image_surface_get_width(img);
                double sh = g_ngfx.image_surface_get_height(img);
                int rot_side = (st->media_rotation == 90 || st->media_rotation == 270);
                double eff_w = rot_side ? sh : sw;
                double eff_h = rot_side ? sw : sh;
                double fit = (mw0 - 28) / eff_w;
                if ((view_h - 16) / eff_h < fit) fit = (view_h - 16) / eff_h;
                double scale = fit * (st->media_zoom_pct / 100.0);
                double dw = sw * scale;
                double dh = sh * scale;
                double mid_x = mx0 + mw0 * 0.5;
                double mid_y = my0 + view_h * 0.5;

                g_ngfx.save(cr);
                g_ngfx.rectangle(cr, mx0, my0 + 1, mw0, view_h - 1);
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

                int flt = st->media_filter % 4;
                if (st->media_rotation == 0 && flt != 0) {
                    if (flt == 1) {
                        ngfx_fill_rect(cr, mid_x - dw * 0.5, mid_y - dh * 0.5, dw, dh, 10, pal.ar_hi, pal.ag_hi, pal.ab_hi, 0.08);
                    } else if (flt == 2) {
                        ngfx_fill_rect(cr, mid_x - dw * 0.5, mid_y - dh * 0.5, dw, dh, 10, 0.06, 0.08, 0.11, 0.36);
                    } else {
                        ngfx_fill_rect(cr, mid_x - dw * 0.5, mid_y - dh * 0.5, dw, dh, 10, 0.96, 0.58, 0.20, 0.14);
                    }
                }
                if (st->media_rotation == 0) {
                    ngfx_stroke_rect(cr, mid_x - dw * 0.5, mid_y - dh * 0.5, dw, dh, 10, 1.0, pal.border_r, pal.border_g, pal.border_b, pal.border_a);
                    ngfx_top_specular(cr, mid_x - dw * 0.5, mid_y - dh * 0.5, dw, 10, 0.15);
                }
            }

            int my_meta = cy + ch - thumb_bar_h - foot_meta_h;
            if (st->media_show_exif && view_h > 92) {
                int hw = mw0 < 320 ? mw0 - 24 : 272;
                int hx = mx0 + 14, hy = my_meta - 76;
                ngfx_shadow_rect(cr, hx, hy, hw, 66, 10, 2.2, 0.45);
                ngfx_fill_rect(cr, hx, hy, hw, 66, 10, pal.sidebar_r, pal.sidebar_g, pal.sidebar_b, 0.92);
                ngfx_stroke_rect(cr, hx + 0.5, hy + 0.5, hw - 1, 65, 10, 1.0, ar, ag, ab, 0.65);
                ngfx_text_clipped(cr, "Plus Jakarta Sans", 1, 11.4, hx + 12, hy + 20, hw - 24, fg_r, fg_g, fg_b, 1.0, MEDIA_TITLES[sel]);
                ngfx_text_clipped(cr, "JetBrains Mono", 0, 9.2, hx + 12, hy + 38, hw - 24,
                                  dark ? pal.ar_hi : ar, dark ? pal.ag_hi : ag, dark ? pal.ab_hi : ab, 0.98, MEDIA_META[sel]);
                static const char *FLT_EXIF[4] = {"Raw", "Vivid", "Noir", "Warm"};
                char hint_exif[72];
                snprintf(hint_exif, sizeof(hint_exif), "Zoom %d%% · %d deg · %s · Space: Slide",
                         st->media_zoom_pct, st->media_rotation, FLT_EXIF[(st->media_filter % 4 + 4) % 4]);
                ngfx_text_clipped(cr, "JetBrains Mono", 0, 8.8, hx + 12, hy + 55, hw - 24, mut_r, mut_g, mut_b, 0.92, hint_exif);
            }
            if (st->media_slideshow) {
                double s_frac = st->media_slideshow_ticks / 25.0;
                if (s_frac < 0.0) s_frac = 0.0;
                if (s_frac > 1.0) s_frac = 1.0;
                ngfx_fill_rect(cr, mx0, my_meta - 3, mw0, 3, 0, pal.elev_r, pal.elev_g, pal.elev_b, 0.55);
                ngfx_fill_rect(cr, mx0, my_meta - 3, mw0 * s_frac, 3, 0, pal.ar_hi, pal.ag_hi, pal.ab_hi, 0.96);
            }
            ngfx_fill_rect(cr, mx0, my_meta, mw0, foot_meta_h, 0, pal.surface_r, pal.surface_g, pal.surface_b, 0.94);
            ngfx_text_clipped(cr, "Plus Jakarta Sans", 1, 10.8, mx0 + 14, my_meta + 17, mw0 * 0.38, fg_r, fg_g, fg_b, 1.0, MEDIA_TITLES[sel]);
            static const char *FLT_META[4] = {"Raw", "Vivid", "Noir", "Warm"};
            char rmeta[160];
            snprintf(rmeta, sizeof(rmeta), "%d/%d · %d%% · %s · %s",
                     sel + 1, MAX_MEDIA_IMAGES, st->media_zoom_pct,
                     FLT_META[(st->media_filter % 4 + 4) % 4], MEDIA_META[sel]);
            ngfx_text_clipped(cr, "JetBrains Mono", 0, 9.2, mx0 + mw0 * 0.40, my_meta + 17, mw0 * 0.58 - 14, mut_r, mut_g, mut_b, 0.95, rmeta);

            int fy = cy + ch - thumb_bar_h;
            ngfx_fill_rect(cr, mx0, fy, mw0, thumb_bar_h, 0, pal.sidebar_r, pal.sidebar_g, pal.sidebar_b, 1.0);
            g_ngfx.set_source_rgba(cr, pal.border_r, pal.border_g, pal.border_b, pal.border_a);
            g_ngfx.move_to(cr, mx0, fy + 0.5);
            g_ngfx.line_to(cr, mx0 + mw0, fy + 0.5);
            g_ngfx.stroke(cr);

            int max_vis = (mw0 - 24) / 72;
            if (max_vis < 4) max_vis = 4;
            if (max_vis > MAX_MEDIA_IMAGES) max_vis = MAX_MEDIA_IMAGES;
            int win_start = sel - max_vis / 2;
            if (win_start < 0) win_start = 0;
            if (win_start + max_vis > MAX_MEDIA_IMAGES) win_start = MAX_MEDIA_IMAGES - max_vis;

            int thumb_w = (mw0 - 24 - (max_vis - 1) * 7) / max_vis;
            if (thumb_w > 96) thumb_w = 96;
            int total_tw = max_vis * thumb_w + (max_vis - 1) * 7;
            int start_tx = mx0 + (mw0 - total_tw) / 2;

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
    } else if (st->media_mode == 1) {
        int vsel = st->video_selected % MAX_VIDEO_CLIPS;
        if (vsel < 0) vsel = 0;
        const VideoClip *vc = &VIDEOS[vsel];
        int compact = mw0 < 480;
        int dur_ms = vc->duration_sec * 1000;
        double prog = dur_ms > 0 ? (double)(st->video_progress_ms % dur_ms) / (double)dur_ms : 0.0;
        double t_sec = (double)st->video_progress_ms / 1000.0;

        int vsp = st->video_speed_x10 > 0 ? st->video_speed_x10 : 10;
        char vsp_lbl[12];
        if (vsp == 10) snprintf(vsp_lbl, sizeof(vsp_lbl), "1x");
        else if (vsp == 20) snprintf(vsp_lbl, sizeof(vsp_lbl), "2x");
        else snprintf(vsp_lbl, sizeof(vsp_lbl), "%.1fx", vsp / 10.0);

        int rx = cx + cw - 12;
        int fit_w = 42, hud_w = 42;
        if (rx - fit_w - hud_w > m_cur_x + 8) {
            rx -= fit_w;
            int fhov = (st->hover_hit_action == HIT_MEDIA_VIDEO_FIT);
            ngfx_fill_rect(cr, rx, cy + 9, fit_w, 26, 7,
                           st->video_fit_fill ? ar : (fhov ? pal.hover_r : pal.elev_r),
                           st->video_fit_fill ? ag : (fhov ? pal.hover_g : pal.elev_g),
                           st->video_fit_fill ? ab : (fhov ? pal.hover_b : pal.elev_b), 0.94);
            ngfx_text(cr, "Noto Sans", 1, 9.8, rx + 9, cy + 25.5,
                      st->video_fit_fill ? pal.on_acc_r : fg_r,
                      st->video_fit_fill ? pal.on_acc_g : fg_g,
                      st->video_fit_fill ? pal.on_acc_b : fg_b, 1.0,
                      st->video_fit_fill ? "Fill" : "16:9");
            nature_add_hit(st, rx, cy + 9, fit_w, 26, HIT_MEDIA_VIDEO_FIT, 0, 0);

            rx -= (hud_w + 6);
            int hhov = (st->hover_hit_action == HIT_MEDIA_VIDEO_HUD);
            ngfx_fill_rect(cr, rx, cy + 9, hud_w, 26, 7,
                           st->video_hud ? ar : (hhov ? pal.hover_r : pal.elev_r),
                           st->video_hud ? ag : (hhov ? pal.hover_g : pal.elev_g),
                           st->video_hud ? ab : (hhov ? pal.hover_b : pal.elev_b), 0.94);
            ngfx_text(cr, "Noto Sans", 1, 9.8, rx + 9, cy + 25.5,
                      st->video_hud ? pal.on_acc_r : fg_r,
                      st->video_hud ? pal.on_acc_g : fg_g,
                      st->video_hud ? pal.on_acc_b : fg_b, 1.0, "HUD");
            nature_add_hit(st, rx, cy + 9, hud_w, 26, HIT_MEDIA_VIDEO_HUD, 0, 0);
        }

        int px = mx0 + 16, py = my0 + 10, pw = mw0 - 36;
        int playlist_reserve = mh0 >= 380 ? 140 : 106;
        int ctrl_h = 80;
        int stage_max_h = mh0 - 14 - ctrl_h - playlist_reserve - 14;
        if (stage_max_h < 110) stage_max_h = 110;
        int vw = pw;
        int vh = st->video_fit_fill ? stage_max_h : (vw * 9) / 16;
        if (vh > stage_max_h) {
            vh = stage_max_h;
            if (!st->video_fit_fill) vw = (vh * 16) / 9;
        }
        int vx = mx0 + (mw0 - vw) / 2;
        int vy = py;

        ngfx_shadow_rect(cr, vx, vy, vw, vh, 12, 2.2, 0.36);
        ngfx_fill_rect(cr, vx, vy, vw, vh, 12, 0.03, 0.04, 0.06, 1.0);

        g_ngfx.save(cr);
        ngfx_rounded_path(cr, vx, vy, vw, vh, 12);
        g_ngfx.clip(cr);

        cairo_surface_t *poster = st->media_images[vc->poster_idx % MAX_MEDIA_IMAGES];
        if (poster) {
            double sw = g_ngfx.image_surface_get_width(poster);
            double sh = g_ngfx.image_surface_get_height(poster);
            if (sw > 0 && sh > 0) {
                double base_sc = ((double)vw / sw > (double)vh / sh) ? ((double)vw / sw) : ((double)vh / sh);
                double kb_zoom = 1.06 + 0.08 * sin(prog * 3.14159);
                double sc = base_sc * kb_zoom;
                double pan_x = sin(t_sec * 0.45) * 14.0;
                double pan_y = cos(t_sec * 0.32) * 8.0;
                g_ngfx.save(cr);
                g_ngfx.translate(cr, vx + vw * 0.5 + pan_x, vy + vh * 0.5 + pan_y);
                g_ngfx.scale(cr, sc, sc);
                g_ngfx.set_source_surface(cr, poster, -sw * 0.5, -sh * 0.5);
                g_ngfx.paint(cr);
                g_ngfx.restore(cr);
            }
        }

        if (vc->scene_id == 0) {
            double sun_x = vx + vw * (0.18 + 0.64 * prog);
            double sun_y = vy + vh * (0.48 - 0.24 * sin(prog * 3.14159));
            ngfx_fill_circle(cr, sun_x, sun_y, vh * 0.34, 1.0, 0.78, 0.42, 0.14);
            ngfx_fill_circle(cr, sun_x, sun_y, vh * 0.14, 1.0, 0.92, 0.70, 0.24);
            double mist_x = vx + vw * fmod(prog * 1.4, 1.0) - vw * 0.2;
            ngfx_fill_gradient(cr, mist_x, vy + vh * 0.64, vw * 0.7, vh * 0.36, 0,
                               0.88, 0.94, 1.0, 0.04, 0.92, 0.96, 1.0, 0.22, 1);
        } else if (vc->scene_id == 1) {
            g_ngfx.set_source_rgba(cr, 0.72, 0.92, 1.0, 0.34);
            g_ngfx.set_line_width(cr, 1.2);
            for (int r = 0; r < 24; ++r) {
                double rx_p = vx + fmod(r * 53.0 + t_sec * (160.0 + (r % 5) * 28.0), (double)(vw + 40)) - 20.0;
                double ry_p = vy + fmod(r * 37.0 + t_sec * (270.0 + (r % 4) * 35.0), (double)(vh + 30)) - 15.0;
                g_ngfx.move_to(cr, rx_p, ry_p);
                g_ngfx.line_to(cr, rx_p - 6.0, ry_p + 16.0);
            }
            g_ngfx.stroke(cr);
            for (int k = 0; k < 3; ++k) {
                double rad = fmod(t_sec * 38.0 + k * 24.0, 68.0);
                double alp = (1.0 - rad / 68.0) * 0.28;
                g_ngfx.set_source_rgba(cr, 0.78, 0.96, 0.98, alp);
                g_ngfx.set_line_width(cr, 1.3);
                g_ngfx.arc(cr, vx + vw * (0.28 + k * 0.22), vy + vh * 0.78, rad, 0, 6.28318);
                g_ngfx.stroke(cr);
            }
        } else if (vc->scene_id == 2) {
            for (int band = 0; band < 3; ++band) {
                double by_off = vy + vh * (0.24 + band * 0.10);
                g_ngfx.set_source_rgba(cr, band == 1 ? 0.22 : 0.18, band == 2 ? 0.78 : 0.92, band == 0 ? 0.62 : 0.88, 0.20);
                g_ngfx.set_line_width(cr, 14.0 - band * 3.0);
                g_ngfx.move_to(cr, vx, by_off);
                for (int s = 1; s <= 12; ++s) {
                    double sx_w = vx + (vw * s) / 12.0;
                    double sy_w = by_off + sin(t_sec * 1.4 + s * 0.65 + band * 1.1) * 14.0;
                    g_ngfx.line_to(cr, sx_w, sy_w);
                }
                g_ngfx.stroke(cr);
            }
            for (int st_i = 0; st_i < 14; ++st_i) {
                double twk = 0.25 + 0.65 * fabs(sin(t_sec * 2.1 + st_i * 1.7));
                ngfx_fill_circle(cr, vx + ((st_i * 97 + 23) % (vw - 20)) + 10,
                                 vy + ((st_i * 41 + 11) % (vh / 2)), 1.4, 0.92, 0.98, 1.0, twk);
            }
        } else {
            ngfx_fill_rect(cr, vx, vy, vw, vh, 0, 0.02, 0.05, 0.09, 0.42);
            double ocx = vx + vw * 0.5, ocy = vy + vh * 0.48;
            for (int ring = 1; ring <= 3; ++ring) {
                double rr = vh * (0.14 * ring);
                g_ngfx.set_source_rgba(cr, pal.ar_hi, pal.ag_hi, pal.ab_hi, 0.32);
                g_ngfx.set_line_width(cr, 1.2);
                g_ngfx.arc(cr, ocx, ocy, rr, 0, 6.28318);
                g_ngfx.stroke(cr);
                double ang = t_sec * (2.6 - ring * 0.55) + ring * 1.2;
                ngfx_fill_circle(cr, ocx + cos(ang) * rr, ocy + sin(ang) * rr, 5.0, pal.ar_hi, pal.ag_hi, pal.ab_hi, 0.95);
            }
            g_ngfx.set_source_rgba(cr, pal.ar_hi, pal.ag_hi, pal.ab_hi, 0.72);
            g_ngfx.set_line_width(cr, 2.0);
            g_ngfx.move_to(cr, vx + 14, vy + vh * 0.84);
            for (int p = 1; p <= 28; ++p) {
                double wx_p = vx + 14 + ((vw - 28) * p) / 28.0;
                double wy_p = vy + vh * 0.84 - fabs(sin(t_sec * 4.0 + p * 0.42)) * 22.0;
                g_ngfx.line_to(cr, wx_p, wy_p);
            }
            g_ngfx.stroke(cr);
        }

        ngfx_fill_gradient(cr, vx, vy + vh - 46, vw, 46, 0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.65, 1);

        if (st->video_hud) {
            char hud_txt[128];
            snprintf(hud_txt, sizeof(hud_txt), "%s · %.0f FPS · %s",
                     vc->title, st->measured_fps, st->video_playing ? "PLAYING" : "PAUSED");
            int hw = (int)ngfx_text_width(cr, "JetBrains Mono", 1, 9.6, hud_txt) + 18;
            if (hw > vw - 20) hw = vw - 20;
            ngfx_fill_rect(cr, vx + 10, vy + 10, hw, 22, 6, 0.04, 0.06, 0.08, 0.72);
            ngfx_stroke_rect(cr, vx + 10.5, vy + 10.5, hw - 1, 21, 6, 0.8, 1.0, 1.0, 1.0, 0.16);
            ngfx_fill_circle(cr, vx + 20, vy + 21, 3.2,
                             st->video_playing ? pal.success_r : pal.warning_r,
                             st->video_playing ? pal.success_g : pal.warning_g,
                             st->video_playing ? pal.success_b : pal.warning_b, 1.0);
            ngfx_text_clipped(cr, "JetBrains Mono", 1, 9.5, vx + 28, vy + 24.5, hw - 22, 0.94, 0.98, 0.96, 0.96, hud_txt);
        }

        if (!st->video_playing) {
            double pcx = vx + vw * 0.5, pcy = vy + vh * 0.5;
            ngfx_fill_circle(cr, pcx, pcy, 28, 0.04, 0.06, 0.08, 0.74);
            g_ngfx.set_source_rgba(cr, 1.0, 1.0, 1.0, 0.28);
            g_ngfx.set_line_width(cr, 1.4);
            g_ngfx.arc(cr, pcx, pcy, 28, 0, 6.28318);
            g_ngfx.stroke(cr);
            g_ngfx.set_source_rgba(cr, 1.0, 1.0, 1.0, 0.96);
            g_ngfx.move_to(cr, pcx - 7, pcy - 11);
            g_ngfx.line_to(cr, pcx + 12, pcy);
            g_ngfx.line_to(cr, pcx - 7, pcy + 11);
            g_ngfx.close_path(cr);
            g_ngfx.fill(cr);
        }

        ngfx_text_clipped(cr, "Plus Jakarta Sans", 1, 12.2, vx + 14, vy + vh - 20, vw - 140, 1.0, 1.0, 1.0, 0.96, vc->title);
        ngfx_text_clipped(cr, "JetBrains Mono", 0, 9.2, vx + 14, vy + vh - 6, vw - 140, 0.85, 0.92, 0.96, 0.88, vc->codec);

        g_ngfx.restore(cr);
        ngfx_stroke_rect(cr, vx + 0.5, vy + 0.5, vw - 1, vh - 1, 12, 1.0, pal.border_r, pal.border_g, pal.border_b, pal.border_a);
        nature_add_hit(st, vx, vy, vw, vh, HIT_MEDIA_VIDEO_PLAY, 0, 0);

        /* Transport Bar Card */
        int cby = vy + vh + 8;
        ngfx_fill_rect(cr, px, cby, pw, 72, 10, pal.card_r, pal.card_g, pal.card_b, 0.96);
        ngfx_stroke_rect(cr, px + 0.5, cby + 0.5, pw - 1, 71, 10, 0.9, pal.border_r, pal.border_g, pal.border_b, pal.border_a);

        int sbar_x = px + 14, sbar_y = cby + 12, sbar_w = pw - 28;
        ngfx_fill_rect(cr, sbar_x, sbar_y, sbar_w, 8, 4, pal.elev_r, pal.elev_g, pal.elev_b, 1.0);
        if (prog > 0.0) {
            double sfill = sbar_w * prog;
            if (sfill < 6) sfill = 6;
            ngfx_fill_gradient(cr, sbar_x, sbar_y, sfill, 8, 4,
                               pal.ar_hi, pal.ag_hi, pal.ab_hi, 0.98, ar * 0.82, ag * 0.82, ab * 0.82, 0.98, 1);
            ngfx_fill_circle(cr, sbar_x + sfill, sbar_y + 4, 5.5, 1.0, 1.0, 1.0, 0.98);
        }
        nature_add_hit(st, sbar_x, sbar_y - 6, sbar_w, 20, HIT_MEDIA_VIDEO_SEEK, sbar_w, 0);

        int cur_sec = st->video_progress_ms / 1000;
        char vtbuf[32];
        snprintf(vtbuf, sizeof(vtbuf), "%02d:%02d / %02d:%02d",
                 cur_sec / 60, cur_sec % 60, vc->duration_sec / 60, vc->duration_sec % 60);
        ngfx_text(cr, "JetBrains Mono", 0, 10.2, sbar_x, cby + 48, mut_r, mut_g, mut_b, 1.0, vtbuf);

        int btn_y = cby + 33;
        int vplay_x = sbar_x + sbar_w - 82;
        ngfx_fill_gradient(cr, vplay_x, btn_y, 82, 26, 7, ar, ag, ab, 0.98, ar * 0.78, ag * 0.78, ab * 0.78, 0.98, 0);
        ngfx_top_specular(cr, vplay_x, btn_y, 82, 7, 0.24);
        if (st->video_playing) {
            ngfx_fill_rect(cr, vplay_x + 14, btn_y + 7.5, 3.0, 11, 1, pal.on_acc_r, pal.on_acc_g, pal.on_acc_b, 0.96);
            ngfx_fill_rect(cr, vplay_x + 20, btn_y + 7.5, 3.0, 11, 1, pal.on_acc_r, pal.on_acc_g, pal.on_acc_b, 0.96);
        } else {
            g_ngfx.set_source_rgba(cr, pal.on_acc_r, pal.on_acc_g, pal.on_acc_b, 0.96);
            g_ngfx.move_to(cr, vplay_x + 15, btn_y + 7.0);
            g_ngfx.line_to(cr, vplay_x + 24, btn_y + 13.0);
            g_ngfx.line_to(cr, vplay_x + 15, btn_y + 19.0);
            g_ngfx.close_path(cr);
            g_ngfx.fill(cr);
        }
        ngfx_text(cr, "Plus Jakarta Sans", 1, 10.8, vplay_x + 30, btn_y + 16.5, pal.on_acc_r, pal.on_acc_g, pal.on_acc_b, 1,
                  st->video_playing ? "Pause" : "Play");
        nature_add_hit(st, vplay_x, btn_y, 82, 26, HIT_MEDIA_VIDEO_PLAY, 0, 0);

        int vx_btn = vplay_x - 32;
        if (vx_btn >= sbar_x + 92) {
            ngfx_fill_rect(cr, vx_btn, btn_y, 28, 26, 7, pal.elev_r, pal.elev_g, pal.elev_b, 0.94);
            ngfx_text(cr, "JetBrains Mono", 1, 9.8, vx_btn + 6, btn_y + 16.5, fg_r, fg_g, fg_b, 0.95, ">|");
            nature_add_hit(st, vx_btn, btn_y, 28, 26, HIT_MEDIA_VIDEO_NEXT, 0, 0);
            vx_btn -= 32;
        }
        if (vx_btn >= sbar_x + 92) {
            ngfx_fill_rect(cr, vx_btn, btn_y, 28, 26, 7, pal.elev_r, pal.elev_g, pal.elev_b, 0.94);
            ngfx_text(cr, "JetBrains Mono", 1, 9.8, vx_btn + 6, btn_y + 16.5, fg_r, fg_g, fg_b, 0.95, "|<");
            nature_add_hit(st, vx_btn, btn_y, 28, 26, HIT_MEDIA_VIDEO_PREV, 0, 0);
            vx_btn -= 36;
        }
        if (!compact) {
            if (vx_btn >= sbar_x + 92) {
                ngfx_fill_rect(cr, vx_btn, btn_y, 32, 26, 7, pal.elev_r, pal.elev_g, pal.elev_b, 0.94);
                ngfx_text(cr, "JetBrains Mono", 1, 9.5, vx_btn + 5, btn_y + 16.5, fg_r, fg_g, fg_b, 0.95, "+5s");
                nature_add_hit(st, vx_btn, btn_y, 32, 26, HIT_MEDIA_VIDEO_STEP, 5, 0);
                vx_btn -= 36;
            }
            if (vx_btn >= sbar_x + 92) {
                ngfx_fill_rect(cr, vx_btn, btn_y, 32, 26, 7, pal.elev_r, pal.elev_g, pal.elev_b, 0.94);
                ngfx_text(cr, "JetBrains Mono", 1, 9.5, vx_btn + 5, btn_y + 16.5, fg_r, fg_g, fg_b, 0.95, "-5s");
                nature_add_hit(st, vx_btn, btn_y, 32, 26, HIT_MEDIA_VIDEO_STEP, -5, 0);
                vx_btn -= 42;
            }
            if (vx_btn >= sbar_x + 92) {
                ngfx_fill_rect(cr, vx_btn, btn_y, 38, 26, 7,
                               vsp != 10 ? ar : pal.elev_r, vsp != 10 ? ag : pal.elev_g, vsp != 10 ? ab : pal.elev_b, 0.94);
                ngfx_text(cr, "JetBrains Mono", 1, 9.8, vx_btn + 7, btn_y + 16.5,
                          vsp != 10 ? pal.on_acc_r : fg_r, vsp != 10 ? pal.on_acc_g : fg_g, vsp != 10 ? pal.on_acc_b : fg_b, 1.0, vsp_lbl);
                nature_add_hit(st, vx_btn, btn_y, 38, 26, HIT_MEDIA_VIDEO_SPEED, 0, 0);
                vx_btn -= 42;
            }
            if (vx_btn >= sbar_x + 92) {
                ngfx_fill_rect(cr, vx_btn, btn_y, 38, 26, 7,
                               st->video_loop ? ar : pal.elev_r, st->video_loop ? ag : pal.elev_g, st->video_loop ? ab : pal.elev_b, 0.94);
                ngfx_text(cr, "Noto Sans", 1, 9.8, vx_btn + 6, btn_y + 16.5,
                          st->video_loop ? pal.on_acc_r : fg_r, st->video_loop ? pal.on_acc_g : fg_g, st->video_loop ? pal.on_acc_b : fg_b, 1.0, "Loop");
                nature_add_hit(st, vx_btn, btn_y, 38, 26, HIT_MEDIA_VIDEO_LOOP, 0, 0);
                vx_btn -= 46;
            }
            if (vx_btn >= sbar_x + 92) {
                ngfx_fill_rect(cr, vx_btn, btn_y, 42, 26, 7,
                               st->cfg.audio_muted ? pal.danger_r : pal.elev_r,
                               st->cfg.audio_muted ? pal.danger_g : pal.elev_g,
                               st->cfg.audio_muted ? pal.danger_b : pal.elev_b, 0.94);
                ngfx_text(cr, "Noto Sans", 1, 9.8, vx_btn + 7, btn_y + 16.5,
                          st->cfg.audio_muted ? 1.0 : fg_r, st->cfg.audio_muted ? 1.0 : fg_g, st->cfg.audio_muted ? 1.0 : fg_b, 1.0,
                          st->cfg.audio_muted ? "Muted" : "Mute");
                nature_add_hit(st, vx_btn, btn_y, 42, 26, HIT_MEDIA_VIDEO_MUTE, 0, 0);
            }
        }

        int vlist_head = cby + 88;
        ngfx_text(cr, "Plus Jakarta Sans", 1, 11.4, px, vlist_head, mut_r, mut_g, mut_b, 1.0, "Video Library (/home/nature/Videos)");
        int vlist_y = vlist_head + 10;
        int vlist_h = cy + ch - vlist_y - 6;
        if (vlist_h < 38) vlist_h = 38;
        int vvis = vlist_h / 40;
        if (vvis < 1) vvis = 1;
        int vmax_scroll = MAX_VIDEO_CLIPS - vvis;
        if (vmax_scroll < 0) vmax_scroll = 0;
        st->video_max_scroll = vmax_scroll;
        if (st->video_scroll > vmax_scroll) st->video_scroll = vmax_scroll;
        if (st->video_scroll < 0) st->video_scroll = 0;

        int vly = vlist_y;
        for (int i = st->video_scroll; i < MAX_VIDEO_CLIPS; ++i) {
            if (vly + 34 > cy + ch - 4) break;
            int is_sel = (vsel == i);
            int is_hov = (st->hover_hit_action == HIT_MEDIA_VIDEO_SELECT && st->hover_hit_arg0 == i);
            ngfx_fill_rect(cr, px, vly, pw, 34, 8,
                           is_sel ? ar : (is_hov ? pal.hover_r : pal.card_r),
                           is_sel ? ag : (is_hov ? pal.hover_g : pal.card_g),
                           is_sel ? ab : (is_hov ? pal.hover_b : pal.card_b),
                           is_sel ? (dark ? 0.24 : 0.16) : 0.96);
            ngfx_stroke_rect(cr, px + 0.5, vly + 0.5, pw - 1, 33, 8, is_sel ? 1.0 : 0.8,
                             is_sel ? ar : pal.border_r, is_sel ? ag : pal.border_g, is_sel ? ab : pal.border_b,
                             is_sel ? 0.65 : pal.border_a);
            if (st->media_images[VIDEOS[i].poster_idx % MAX_MEDIA_IMAGES]) {
                cairo_surface_t *vth = st->media_images[VIDEOS[i].poster_idx % MAX_MEDIA_IMAGES];
                g_ngfx.save(cr);
                ngfx_rounded_path(cr, px + 6, vly + 5, 40, 24, 5);
                g_ngfx.clip(cr);
                double sw = g_ngfx.image_surface_get_width(vth), sh = g_ngfx.image_surface_get_height(vth);
                if (sw > 0 && sh > 0) {
                    double sc = (40.0 / sw > 24.0 / sh) ? (40.0 / sw) : (24.0 / sh);
                    g_ngfx.translate(cr, px + 6 + (40.0 - sw * sc) * 0.5, vly + 5 + (24.0 - sh * sc) * 0.5);
                    g_ngfx.scale(cr, sc, sc);
                    g_ngfx.set_source_surface(cr, vth, 0, 0);
                    g_ngfx.paint(cr);
                }
                g_ngfx.restore(cr);
            }
            char vdur[16];
            snprintf(vdur, sizeof(vdur), "%02d:%02d", VIDEOS[i].duration_sec / 60, VIDEOS[i].duration_sec % 60);
            ngfx_text_clipped(cr, "Plus Jakarta Sans", 1, 11.2, px + 54, vly + 15.5, pw - 122, fg_r, fg_g, fg_b, 1.0, VIDEOS[i].title);
            ngfx_text_clipped(cr, "JetBrains Mono", 0, 9.2, px + 54, vly + 28.5, pw - 122, mut_r, mut_g, mut_b, 0.92, VIDEOS[i].codec);
            ngfx_text(cr, "JetBrains Mono", 0, 10.0, px + pw - 50, vly + 21, mut_r, mut_g, mut_b, 1.0, vdur);
            nature_add_hit(st, px, vly, pw, 34, HIT_MEDIA_VIDEO_SELECT, i, 0);
            vly += 40;
        }
        nature_draw_scrollbar(st, cr, mx0 + mw0 - 11, vlist_y, vlist_h,
                              MAX_VIDEO_CLIPS, vvis, st->video_scroll, vmax_scroll, SCROLL_MEDIA_VIDEO, dark);
    } else {
        int sel = st->audio_selected % MAX_AUDIO_TRACKS;
        if (sel < 0) sel = 0;
        const AudioTrack *trk = &TRACKS[sel];
        int compact = mw0 < 470;
        int px = mx0 + 16, py = my0 + 12, pw = mw0 - 40;

        ngfx_shadow_rect(cr, px, py, pw, 184, 14, 2.0, 0.24);
        ngfx_fill_rect(cr, px, py, pw, 184, 14, pal.card_r, pal.card_g, pal.card_b, 0.96);
        ngfx_stroke_rect(cr, px + 0.5, py + 0.5, pw - 1, 183, 14, 1.0, pal.border_r, pal.border_g, pal.border_b, pal.border_a);
        ngfx_top_specular(cr, px, py, pw, 14, dark ? 0.12 : 0.30);

        int asz = compact ? 86 : 132;
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

        int info_x = ax + asz + 16;
        int info_w = pw - asz - 44;
        ngfx_text_clipped(cr, "Plus Jakarta Sans", 1, 16.0, info_x, py + 34, info_w, fg_r, fg_g, fg_b, 1.0, trk->title);
        ngfx_text_clipped(cr, "Noto Sans", 0, 11.4, info_x, py + 54, info_w, mut_r, mut_g, mut_b, 1.0, trk->artist);
        char codec_line[160];
        snprintf(codec_line, sizeof(codec_line), "%s · %s · EQ: %s", trk->codec, SINK_SHORT[st->cfg.audio_device % 3], EQ_NAMES[eq_idx]);
        ngfx_text_clipped(cr, "JetBrains Mono", 0, 9.5, info_x, py + 72, info_w,
                          dark ? pal.ar_hi : ar, dark ? pal.ag_hi : ag, dark ? pal.ab_hi : ab, 0.95, codec_line);

        double vol_gain = st->cfg.audio_muted ? 0.12 : (0.35 + 0.65 * (st->cfg.volume / 100.0));
        int bars = compact ? 14 : 22;
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
        ngfx_text(cr, "JetBrains Mono", 0, 10.0, info_x, pby + 30, mut_r, mut_g, mut_b, 1.0, tbuf);

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
        nature_draw_scrollbar(st, cr, mx0 + mw0 - 11, list_y, list_h,
                              MAX_AUDIO_TRACKS, vis, st->audio_scroll, max_scroll, SCROLL_MEDIA_AUDIO, dark);
    }
}

void nature_media_click(NatureOSState *st, const HitRect *hit) {
    int save_needed = 0;
    if (hit->action_id == HIT_MEDIA_SIDEBAR_TOGGLE) {
        st->media_sidebar_open = !st->media_sidebar_open;
        save_needed = 1;
    } else if (hit->action_id == HIT_MEDIA_FOLDER) {
        st->media_folder_idx = hit->arg0;
        st->media_grid_scroll = 0;
        if (hit->arg0 == 1) {
            st->media_mode = 0;
        } else if (hit->arg0 == 2) {
            st->media_mode = 1;
        } else if (hit->arg0 == 3) {
            st->media_mode = 2;
        } else if (hit->arg0 >= 4 && hit->arg1 >= 0 && hit->arg1 < st->vfs_count) {
            snprintf(st->media_vfs_dir, sizeof(st->media_vfs_dir), "%s", st->vfs[hit->arg1].path);
        }
        save_needed = 1;
    } else if (hit->action_id == HIT_MEDIA_OPEN_VFS) {
        int idx = hit->arg0;
        if (idx >= 0 && idx < st->vfs_count) {
            const VfsNode *vn = &st->vfs[idx];
            if (vn->is_dir) {
                snprintf(st->media_vfs_dir, sizeof(st->media_vfs_dir), "%s", vn->path);
                st->media_grid_scroll = 0;
            } else if (vn->video_idx >= 0) {
                st->media_folder_idx = 2;
                st->media_mode = 1;
                st->video_selected = vn->video_idx % MAX_VIDEO_CLIPS;
                st->video_progress_ms = 0;
                st->video_playing = 1;
            } else if (vn->audio_idx >= 0) {
                st->media_folder_idx = 3;
                st->media_mode = 2;
                st->audio_selected = vn->audio_idx % MAX_AUDIO_TRACKS;
                st->audio_progress_sec = 0;
                st->audio_playing = 1;
            } else if (vn->image_idx >= 0) {
                st->media_folder_idx = 1;
                st->media_mode = 0;
                st->media_grid_mode = 0;
                st->media_selected = vn->image_idx % MAX_MEDIA_IMAGES;
                st->media_zoom_pct = 100;
                st->media_rotation = 0;
            } else {
                snprintf(st->files_cwd, sizeof(st->files_cwd), "%s", vn->parent[0] ? vn->parent : "/home/nature");
                st->files_selected_idx = idx;
                st->files_viewer_open = 1;
                st->files_viewer_idx = idx;
                st->files_editing = 0;
                nature_open_app(st, APP_FILES);
            }
            save_needed = 1;
        }
    } else if (hit->action_id == HIT_MEDIA_FOLDER_OPEN_FILES) {
        if (st->media_vfs_dir[0]) {
            snprintf(st->files_cwd, sizeof(st->files_cwd), "%s", st->media_vfs_dir);
            st->files_selected_idx = -1;
            st->files_viewer_open = 0;
            st->files_scroll = 0;
        }
        nature_open_app(st, APP_FILES);
        save_needed = 1;
    } else if (hit->action_id == HIT_MEDIA_MODE) {
        st->media_mode = hit->arg0;
        st->media_folder_idx = hit->arg0 + 1;
        save_needed = 1;
    } else if (hit->action_id == HIT_MEDIA_GRID_TOGGLE) {
        st->media_grid_mode = !st->media_grid_mode;
        st->media_folder_idx = 1;
        st->media_mode = 0;
        save_needed = 1;
    } else if (hit->action_id == HIT_MEDIA_SLIDESHOW) {
        st->media_slideshow = !st->media_slideshow;
        st->media_slideshow_ticks = 0;
        if (st->media_slideshow) {
            st->media_grid_mode = 0;
            st->media_folder_idx = 1;
            st->media_mode = 0;
        }
    } else if (hit->action_id == HIT_MEDIA_SELECT) {
        st->media_selected = hit->arg0 % MAX_MEDIA_IMAGES;
        st->media_zoom_pct = 100;
        st->media_rotation = 0;
        st->media_folder_idx = 1;
        st->media_mode = 0;
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
    } else if (hit->action_id == HIT_MEDIA_VIDEO_PLAY) {
        st->video_playing = !st->video_playing;
        save_needed = 1;
    } else if (hit->action_id == HIT_MEDIA_VIDEO_SELECT) {
        st->video_selected = hit->arg0 % MAX_VIDEO_CLIPS;
        st->video_progress_ms = 0;
        st->video_playing = 1;
        st->media_folder_idx = 2;
        st->media_mode = 1;
        save_needed = 1;
    } else if (hit->action_id == HIT_MEDIA_VIDEO_PREV) {
        st->video_selected = (st->video_selected + MAX_VIDEO_CLIPS - 1) % MAX_VIDEO_CLIPS;
        st->video_progress_ms = 0;
        st->video_playing = 1;
        save_needed = 1;
    } else if (hit->action_id == HIT_MEDIA_VIDEO_NEXT) {
        st->video_selected = (st->video_selected + 1) % MAX_VIDEO_CLIPS;
        st->video_progress_ms = 0;
        st->video_playing = 1;
        save_needed = 1;
    } else if (hit->action_id == HIT_MEDIA_VIDEO_STEP) {
        int dur_ms = VIDEOS[st->video_selected % MAX_VIDEO_CLIPS].duration_sec * 1000;
        st->video_progress_ms += hit->arg0 * 1000;
        if (st->video_progress_ms < 0) st->video_progress_ms = 0;
        if (st->video_progress_ms >= dur_ms) st->video_progress_ms = dur_ms - 500;
    } else if (hit->action_id == HIT_MEDIA_VIDEO_SPEED) {
        if (st->video_speed_x10 <= 5) st->video_speed_x10 = 10;
        else if (st->video_speed_x10 == 10) st->video_speed_x10 = 15;
        else if (st->video_speed_x10 == 15) st->video_speed_x10 = 20;
        else st->video_speed_x10 = 5;
        save_needed = 1;
    } else if (hit->action_id == HIT_MEDIA_VIDEO_LOOP) {
        st->video_loop = !st->video_loop;
        save_needed = 1;
    } else if (hit->action_id == HIT_MEDIA_VIDEO_MUTE) {
        st->cfg.audio_muted = !st->cfg.audio_muted;
        save_needed = 1;
    } else if (hit->action_id == HIT_MEDIA_VIDEO_HUD) {
        st->video_hud = !st->video_hud;
        save_needed = 1;
    } else if (hit->action_id == HIT_MEDIA_VIDEO_FIT) {
        st->video_fit_fill = !st->video_fit_fill;
        save_needed = 1;
    } else if (hit->action_id == HIT_MEDIA_VIDEO_SEEK) {
        int dur_ms = VIDEOS[st->video_selected % MAX_VIDEO_CLIPS].duration_sec * 1000;
        int w = hit->arg0 > 0 ? hit->arg0 : 1;
        int rel_x = hit->arg1 >= 0 ? hit->arg1 : 0;
        int ms = (int)(((long long)rel_x * (long long)dur_ms) / (long long)w);
        if (ms < 0) ms = 0;
        if (ms >= dur_ms) ms = dur_ms - 500;
        st->video_progress_ms = ms;
    } else if (hit->action_id == HIT_MEDIA_AUDIO_PLAY) {
        st->audio_playing = !st->audio_playing;
        save_needed = 1;
    } else if (hit->action_id == HIT_MEDIA_AUDIO_SELECT) {
        st->audio_selected = hit->arg0 % MAX_AUDIO_TRACKS;
        st->audio_progress_sec = 0;
        st->audio_playing = 1;
        st->media_folder_idx = 3;
        st->media_mode = 2;
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
    } else if (hit->action_id == HIT_MEDIA_FILTER) {
        st->media_filter = (st->media_filter + 1) % 4;
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
    if (st->media_folder_idx == 1 && st->media_mode == 0) {
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
        } else if (strcmp(key, "f") == 0 || strcmp(key, "F") == 0) {
            st->media_filter = (st->media_filter + 1) % 4;
            nature_os_save_state(st);
        } else if (strcmp(key, " ") == 0 || strcmp(key, "Space") == 0) {
            st->media_slideshow = !st->media_slideshow;
            st->media_slideshow_ticks = 0;
            if (st->media_slideshow) st->media_grid_mode = 0;
        }
    } else if (st->media_folder_idx == 2 || st->media_mode == 1) {
        int dur_ms = VIDEOS[st->video_selected % MAX_VIDEO_CLIPS].duration_sec * 1000;
        if (strcmp(key, " ") == 0 || strcmp(key, "Space") == 0) {
            st->video_playing = !st->video_playing;
            nature_os_save_state(st);
        } else if (strcmp(key, "ArrowLeft") == 0) {
            st->video_progress_ms -= 5000;
            if (st->video_progress_ms < 0) st->video_progress_ms = 0;
        } else if (strcmp(key, "ArrowRight") == 0) {
            st->video_progress_ms += 5000;
            if (st->video_progress_ms >= dur_ms) st->video_progress_ms = dur_ms - 500;
        } else if (strcmp(key, "l") == 0 || strcmp(key, "L") == 0) {
            st->video_loop = !st->video_loop;
            nature_os_save_state(st);
        } else if (strcmp(key, "m") == 0 || strcmp(key, "M") == 0) {
            st->cfg.audio_muted = !st->cfg.audio_muted;
            nature_os_save_state(st);
        } else if (strcmp(key, "h") == 0 || strcmp(key, "H") == 0) {
            st->video_hud = !st->video_hud;
            nature_os_save_state(st);
        } else if (strcmp(key, "f") == 0 || strcmp(key, "F") == 0) {
            st->video_fit_fill = !st->video_fit_fill;
            nature_os_save_state(st);
        }
    } else if (st->media_folder_idx == 3 || st->media_mode == 2) {
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
    for (int i = 0; i < st->window_count; ++i) {
        if (st->windows[i].app == APP_MEDIA && st->media_sidebar_open && st->windows[i].w >= 420) {
            int sb_w = st->windows[i].w >= 600 ? 162 : 138;
            if (st->hover_mx >= st->windows[i].x && st->hover_mx <= st->windows[i].x + sb_w) {
                if (delta_y > 0 && st->media_folder_scroll < st->media_folder_max_scroll) st->media_folder_scroll++;
                else if (delta_y < 0 && st->media_folder_scroll > 0) st->media_folder_scroll--;
                return;
            }
            break;
        }
    }
    if (st->media_folder_idx == 0 || st->media_folder_idx >= 4) {
        if (delta_y > 0 && st->media_grid_scroll < st->media_grid_max_scroll) st->media_grid_scroll++;
        else if (delta_y < 0 && st->media_grid_scroll > 0) st->media_grid_scroll--;
        return;
    }
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
    } else if (st->media_mode == 1) {
        if (delta_y > 0 && st->video_scroll < st->video_max_scroll) st->video_scroll++;
        else if (delta_y < 0 && st->video_scroll > 0) st->video_scroll--;
    } else {
        if (delta_y > 0 && st->audio_scroll < st->audio_max_scroll) st->audio_scroll++;
        else if (delta_y < 0 && st->audio_scroll > 0) st->audio_scroll--;
    }
}
