#include "shell_internal.h"
#include <string.h>

void nature_get_accent(int accent_idx, double *r, double *g, double *b) {
    nature_get_accent_ex(THEME_NATURE, accent_idx, 1, r, g, b);
}

void nature_get_accent_ex(int theme_family, int accent_idx, int dark_mode, double *r, double *g, double *b) {
    if (theme_family == THEME_HORIZON) {
        if (dark_mode) {
            const double accents[6][3] = {
                {0.14, 0.39, 0.92}, /* Horizon Royal Blue */
                {0.39, 0.30, 0.88}, /* Indigo */
                {0.06, 0.62, 0.74}, /* Cyan */
                {0.55, 0.24, 0.84}, /* Violet */
                {0.86, 0.22, 0.44}, /* Rose */
                {0.82, 0.52, 0.14}  /* Horizon Amber */
            };
            int idx = (accent_idx >= 0 && accent_idx < 6) ? accent_idx : 0;
            *r = accents[idx][0]; *g = accents[idx][1]; *b = accents[idx][2];
        } else {
            const double accents[6][3] = {
                {0.10, 0.34, 0.82}, /* Horizon Daylight Blue */
                {0.32, 0.24, 0.78}, /* Rich Indigo */
                {0.04, 0.52, 0.64}, /* Deep Cyan */
                {0.48, 0.18, 0.74}, /* Deep Violet */
                {0.78, 0.16, 0.36}, /* Deep Rose */
                {0.72, 0.44, 0.10}  /* Deep Amber */
            };
            int idx = (accent_idx >= 0 && accent_idx < 6) ? accent_idx : 0;
            *r = accents[idx][0]; *g = accents[idx][1]; *b = accents[idx][2];
        }
        return;
    }

    /* Nature Botanical Palettes */
    if (dark_mode) {
        const double accents[6][3] = {
            {0.18, 0.54, 0.34}, /* Canopy Emerald */
            {0.12, 0.52, 0.54}, /* River Teal */
            {0.76, 0.52, 0.16}, /* Autumn Amber */
            {0.22, 0.46, 0.74}, /* Alpine Blue */
            {0.58, 0.32, 0.68}, /* Orchid */
            {0.78, 0.34, 0.28}  /* Basalt Coral */
        };
        int idx = (accent_idx >= 0 && accent_idx < 6) ? accent_idx : 0;
        *r = accents[idx][0]; *g = accents[idx][1]; *b = accents[idx][2];
    } else {
        const double accents[6][3] = {
            {0.13, 0.45, 0.28}, /* Forest Green */
            {0.08, 0.44, 0.46}, /* Stream Teal */
            {0.68, 0.44, 0.10}, /* Warm Amber */
            {0.16, 0.38, 0.64}, /* Sky Alpine */
            {0.50, 0.24, 0.60}, /* Heather Orchid */
            {0.70, 0.26, 0.20}  /* Terracotta Coral */
        };
        int idx = (accent_idx >= 0 && accent_idx < 6) ? accent_idx : 0;
        *r = accents[idx][0]; *g = accents[idx][1]; *b = accents[idx][2];
    }
}

void nature_get_palette(const NatureOSState *st, NaturePalette *out) {
    int fam = st ? st->cfg.theme_family : THEME_NATURE;
    int dark = st ? st->cfg.dark_mode : 1;
    int acc_idx = st ? st->cfg.accent_idx : 0;

    memset(out, 0, sizeof(*out));
    out->theme_family = fam;
    out->dark_mode = dark;

    nature_get_accent_ex(fam, acc_idx, dark, &out->ar, &out->ag, &out->ab);

    if (dark) {
        out->ar_hi = out->ar * 1.25 > 1.0 ? 1.0 : out->ar * 1.25;
        out->ag_hi = out->ag * 1.25 > 1.0 ? 1.0 : out->ag * 1.25;
        out->ab_hi = out->ab * 1.25 > 1.0 ? 1.0 : out->ab * 1.25;
        out->on_acc_r = 1.0; out->on_acc_g = 1.0; out->on_acc_b = 1.0;

        if (fam == THEME_HORIZON) {
            /* Horizon Dark (Slate / Indigo navy) */
            out->win_bg_r  = 0.035; out->win_bg_g  = 0.051; out->win_bg_b  = 0.086;
            out->surface_r = 0.067; out->surface_g = 0.094; out->surface_b = 0.153;
            out->sidebar_r = 0.051; out->sidebar_g = 0.075; out->sidebar_b = 0.122;
            out->card_r    = 0.082; out->card_g    = 0.114; out->card_b    = 0.184;
            out->elev_r    = 0.106; out->elev_g    = 0.145; out->elev_b    = 0.227;
            out->hover_r   = 0.137; out->hover_g   = 0.184; out->hover_b   = 0.282;

            out->hdr_act_top_r   = 0.094; out->hdr_act_top_g   = 0.129; out->hdr_act_top_b   = 0.208;
            out->hdr_act_bot_r   = 0.063; out->hdr_act_bot_g   = 0.090; out->hdr_act_bot_b   = 0.145;
            out->hdr_inact_top_r = 0.055; out->hdr_inact_top_g = 0.075; out->hdr_inact_top_b = 0.118;
            out->hdr_inact_bot_r = 0.039; out->hdr_inact_bot_g = 0.055; out->hdr_inact_bot_b = 0.086;

            out->panel_top_r = 0.031; out->panel_top_g = 0.047; out->panel_top_b = 0.078; out->panel_alpha = 0.86;
            out->panel_bot_r = 0.024; out->panel_bot_g = 0.035; out->panel_bot_b = 0.059;

            out->dock_top_r = 0.055; out->dock_top_g = 0.082; out->dock_top_b = 0.133; out->dock_alpha = 0.88;
            out->dock_bot_r = 0.039; out->dock_bot_g = 0.059; out->dock_bot_b = 0.098;

            out->border_r = 0.94; out->border_g = 0.96; out->border_b = 1.0; out->border_a = 0.09;
            out->strong_brd_r = 0.94; out->strong_brd_g = 0.96; out->strong_brd_b = 1.0; out->strong_brd_a = 0.18;

            out->fg_r = 0.94; out->fg_g = 0.96; out->fg_b = 0.99;
            out->fg_sec_r = 0.74; out->fg_sec_g = 0.80; out->fg_sec_b = 0.89;
            out->mut_r = 0.54; out->mut_g = 0.61; out->mut_b = 0.72;
        } else {
            /* Nature Dark (Evergreen forest) */
            out->win_bg_r  = 0.051; out->win_bg_g  = 0.075; out->win_bg_b  = 0.067;
            out->surface_r = 0.082; out->surface_g = 0.122; out->surface_b = 0.106;
            out->sidebar_r = 0.067; out->sidebar_g = 0.098; out->sidebar_b = 0.086;
            out->card_r    = 0.098; out->card_g    = 0.145; out->card_b    = 0.125;
            out->elev_r    = 0.122; out->elev_g    = 0.176; out->elev_b    = 0.153;
            out->hover_r   = 0.157; out->hover_g   = 0.224; out->hover_b   = 0.196;

            out->hdr_act_top_r   = 0.114; out->hdr_act_top_g   = 0.169; out->hdr_act_top_b   = 0.145;
            out->hdr_act_bot_r   = 0.078; out->hdr_act_bot_g   = 0.118; out->hdr_act_bot_b   = 0.102;
            out->hdr_inact_top_r = 0.071; out->hdr_inact_top_g = 0.102; out->hdr_inact_top_b = 0.090;
            out->hdr_inact_bot_r = 0.047; out->hdr_inact_bot_g = 0.071; out->hdr_inact_bot_b = 0.059;

            out->panel_top_r = 0.043; out->panel_top_g = 0.063; out->panel_top_b = 0.055; out->panel_alpha = 0.86;
            out->panel_bot_r = 0.031; out->panel_bot_g = 0.047; out->panel_bot_b = 0.039;

            out->dock_top_r = 0.075; out->dock_top_g = 0.110; out->dock_top_b = 0.094; out->dock_alpha = 0.88;
            out->dock_bot_r = 0.051; out->dock_bot_g = 0.078; out->dock_bot_b = 0.067;

            out->border_r = 0.91; out->border_g = 0.94; out->border_b = 0.92; out->border_a = 0.09;
            out->strong_brd_r = 0.91; out->strong_brd_g = 0.94; out->strong_brd_b = 0.92; out->strong_brd_a = 0.18;

            out->fg_r = 0.91; out->fg_g = 0.94; out->fg_b = 0.92;
            out->fg_sec_r = 0.73; out->fg_sec_g = 0.80; out->fg_sec_b = 0.76;
            out->mut_r = 0.53; out->mut_g = 0.61; out->mut_b = 0.56;
        }

        out->danger_r  = 0.94; out->danger_g  = 0.33; out->danger_b  = 0.33;
        out->success_r = 0.27; out->success_g = 0.77; out->success_b = 0.49;
        out->warning_r = 0.96; out->warning_g = 0.68; out->warning_b = 0.24;
    } else {
        /* Light Mode */
        out->ar_hi = out->ar;
        out->ag_hi = out->ag;
        out->ab_hi = out->ab;
        out->on_acc_r = 1.0; out->on_acc_g = 1.0; out->on_acc_b = 1.0;

        if (fam == THEME_HORIZON) {
            /* Horizon Light */
            out->win_bg_r  = 0.957; out->win_bg_g  = 0.969; out->win_bg_b  = 0.984;
            out->surface_r = 0.988; out->surface_g = 0.992; out->surface_b = 1.000;
            out->sidebar_r = 0.929; out->sidebar_g = 0.945; out->sidebar_b = 0.969;
            out->card_r    = 1.000; out->card_g    = 1.000; out->card_b    = 1.000;
            out->elev_r    = 0.898; out->elev_g    = 0.918; out->elev_b    = 0.945;
            out->hover_r   = 0.855; out->hover_g   = 0.882; out->hover_b   = 0.918;

            out->hdr_act_top_r   = 0.988; out->hdr_act_top_g   = 0.992; out->hdr_act_top_b   = 1.000;
            out->hdr_act_bot_r   = 0.933; out->hdr_act_bot_g   = 0.949; out->hdr_act_bot_b   = 0.973;
            out->hdr_inact_top_r = 0.945; out->hdr_inact_top_g = 0.957; out->hdr_inact_top_b = 0.973;
            out->hdr_inact_bot_r = 0.906; out->hdr_inact_bot_g = 0.922; out->hdr_inact_bot_b = 0.945;

            out->panel_top_r = 0.976; out->panel_top_g = 0.984; out->panel_top_b = 0.992; out->panel_alpha = 0.90;
            out->panel_bot_r = 0.941; out->panel_bot_g = 0.953; out->panel_bot_b = 0.969;

            out->dock_top_r = 0.953; out->dock_top_g = 0.965; out->dock_top_b = 0.984; out->dock_alpha = 0.92;
            out->dock_bot_r = 0.914; out->dock_bot_g = 0.929; out->dock_bot_b = 0.953;

            out->border_r = 0.06; out->border_g = 0.09; out->border_b = 0.16; out->border_a = 0.12;
            out->strong_brd_r = 0.06; out->strong_brd_g = 0.09; out->strong_brd_b = 0.16; out->strong_brd_a = 0.22;

            out->fg_r = 0.059; out->fg_g = 0.090; out->fg_b = 0.165;
            out->fg_sec_r = 0.27; out->fg_sec_g = 0.33; out->fg_sec_b = 0.42;
            out->mut_r = 0.45; out->mut_g = 0.52; out->mut_b = 0.62;
        } else {
            /* Nature Light */
            out->win_bg_r  = 0.953; out->win_bg_g  = 0.973; out->win_bg_b  = 0.957;
            out->surface_r = 0.984; out->surface_g = 0.992; out->surface_b = 0.984;
            out->sidebar_r = 0.925; out->sidebar_g = 0.949; out->sidebar_b = 0.929;
            out->card_r    = 1.000; out->card_g    = 1.000; out->card_b    = 1.000;
            out->elev_r    = 0.890; out->elev_g    = 0.922; out->elev_b    = 0.898;
            out->hover_r   = 0.847; out->hover_g   = 0.886; out->hover_b   = 0.855;

            out->hdr_act_top_r   = 0.984; out->hdr_act_top_g   = 0.992; out->hdr_act_top_b   = 0.984;
            out->hdr_act_bot_r   = 0.925; out->hdr_act_bot_g   = 0.949; out->hdr_act_bot_b   = 0.929;
            out->hdr_inact_top_r = 0.937; out->hdr_inact_top_g = 0.957; out->hdr_inact_top_b = 0.941;
            out->hdr_inact_bot_r = 0.894; out->hdr_inact_bot_g = 0.918; out->hdr_inact_bot_b = 0.898;

            out->panel_top_r = 0.976; out->panel_top_g = 0.988; out->panel_top_b = 0.980; out->panel_alpha = 0.90;
            out->panel_bot_r = 0.937; out->panel_bot_g = 0.953; out->panel_bot_b = 0.945;

            out->dock_top_r = 0.953; out->dock_top_g = 0.973; out->dock_top_b = 0.957; out->dock_alpha = 0.92;
            out->dock_bot_r = 0.910; out->dock_bot_g = 0.933; out->dock_bot_b = 0.914;

            out->border_r = 0.07; out->border_g = 0.13; out->border_b = 0.10; out->border_a = 0.12;
            out->strong_brd_r = 0.07; out->strong_brd_g = 0.13; out->strong_brd_b = 0.10; out->strong_brd_a = 0.22;

            out->fg_r = 0.071; out->fg_g = 0.129; out->fg_b = 0.098;
            out->fg_sec_r = 0.24; out->fg_sec_g = 0.35; out->fg_sec_b = 0.29;
            out->mut_r = 0.42; out->mut_g = 0.52; out->mut_b = 0.46;
        }

        out->danger_r  = 0.84; out->danger_g  = 0.22; out->danger_b  = 0.22;
        out->success_r = 0.18; out->success_g = 0.60; out->success_b = 0.36;
        out->warning_r = 0.84; out->warning_g = 0.54; out->warning_b = 0.12;
    }

    /* Design Tokens */
    int compact = (st && st->cfg.compact_chrome) ? 1 : 0;
    out->r_win = compact ? 9.0 : NTHEME_RADIUS_WIN;
    out->r_card = compact ? 8.0 : NTHEME_RADIUS_CARD;
    out->r_btn = NTHEME_RADIUS_BTN;
    out->r_pill = NTHEME_RADIUS_PILL;
    out->r_dock = NTHEME_RADIUS_DOCK;
    out->card_alpha = 0.96;
    out->elev_alpha = 0.94;
    out->shadow_alpha = dark ? 0.42 : 0.18;
}

void ntheme_draw_card(cairo_t *cr, double x, double y, double w, double h, double r,
                      const NaturePalette *pal, int hovered, int selected) {
    if (r <= 0) r = pal->r_card;
    ngfx_shadow_rect(cr, x, y, w, h, r, selected ? 2.5 : 1.5, pal->shadow_alpha);
    ngfx_fill_rect(cr, x, y, w, h, r,
                   selected ? pal->elev_r : (hovered ? pal->hover_r : pal->card_r),
                   selected ? pal->elev_g : (hovered ? pal->hover_g : pal->card_g),
                   selected ? pal->elev_b : (hovered ? pal->hover_b : pal->card_b),
                   pal->card_alpha);
    ngfx_stroke_rect(cr, x + 0.5, y + 0.5, w - 1, h - 1, r, selected ? 1.4 : 1.0,
                     selected ? pal->ar : (hovered ? pal->ar : pal->border_r),
                     selected ? pal->ag : (hovered ? pal->ag : pal->border_g),
                     selected ? pal->ab : (hovered ? pal->ab : pal->border_b),
                     selected ? 0.85 : (hovered ? 0.55 : pal->border_a));
    ngfx_top_specular(cr, x, y, w, r, pal->dark_mode ? 0.14 : 0.40);
}

void ntheme_draw_glass_pill(cairo_t *cr, double x, double y, double w, double h, double r,
                            const NaturePalette *pal, int active, const char *text) {
    if (r <= 0) r = h * 0.5;
    if (active) {
        ngfx_fill_gradient(cr, x, y, w, h, r,
                           pal->ar, pal->ag, pal->ab, 0.96,
                           pal->ar * 0.82, pal->ag * 0.82, pal->ab * 0.82, 0.96, 0);
        ngfx_top_specular(cr, x, y, w, r, 0.24);
    } else {
        ngfx_fill_rect(cr, x, y, w, h, r, pal->elev_r, pal->elev_g, pal->elev_b, 0.92);
        ngfx_stroke_rect(cr, x + 0.5, y + 0.5, w - 1, h - 1, r, 0.9,
                         pal->border_r, pal->border_g, pal->border_b, pal->border_a);
    }
    if (text && *text) {
        double tw = ngfx_text_width(cr, "Noto Sans", active, 10.6, text);
        double tx = x + (w - tw) * 0.5;
        double ty = y + h * 0.5 + 3.8;
        ngfx_text(cr, "Noto Sans", active, 10.6, tx, ty,
                  active ? pal->on_acc_r : pal->fg_r,
                  active ? pal->on_acc_g : pal->fg_g,
                  active ? pal->on_acc_b : pal->fg_b, 1.0, text);
    }
}

void ntheme_draw_button(cairo_t *cr, double x, double y, double w, double h, double r,
                        const NaturePalette *pal, int is_accent, int hovered, const char *label) {
    if (r <= 0) r = pal->r_btn;
    if (is_accent) {
        double mult = hovered ? 1.10 : 1.0;
        ngfx_fill_gradient(cr, x, y, w, h, r,
                           pal->ar * mult > 1 ? 1 : pal->ar * mult,
                           pal->ag * mult > 1 ? 1 : pal->ag * mult,
                           pal->ab * mult > 1 ? 1 : pal->ab * mult, 0.96,
                           pal->ar * 0.80, pal->ag * 0.80, pal->ab * 0.80, 0.96, 0);
        ngfx_top_specular(cr, x, y, w, r, 0.22);
    } else {
        ngfx_fill_rect(cr, x, y, w, h, r,
                       hovered ? pal->hover_r : pal->elev_r,
                       hovered ? pal->hover_g : pal->elev_g,
                       hovered ? pal->hover_b : pal->elev_b, 0.94);
        ngfx_stroke_rect(cr, x + 0.5, y + 0.5, w - 1, h - 1, r, 1.0,
                         hovered ? pal->ar : pal->border_r,
                         hovered ? pal->ag : pal->border_g,
                         hovered ? pal->ab : pal->border_b,
                         hovered ? 0.65 : pal->border_a);
    }
    if (label && *label) {
        double tw = ngfx_text_width(cr, "Noto Sans", 1, 10.5, label);
        double tx = x + (w - tw) * 0.5;
        double ty = y + h * 0.5 + 3.8;
        ngfx_text(cr, "Noto Sans", 1, 10.5, tx, ty,
                  is_accent ? pal->on_acc_r : pal->fg_r,
                  is_accent ? pal->on_acc_g : pal->fg_g,
                  is_accent ? pal->on_acc_b : pal->fg_b, 1.0, label);
    }
}

void ntheme_draw_badge(cairo_t *cr, double x, double y, double w, double h, double r,
                       const NaturePalette *pal, const char *label, int is_accent) {
    if (r <= 0) r = h * 0.5;
    if (is_accent) {
        ngfx_fill_rect(cr, x, y, w, h, r, pal->ar, pal->ag, pal->ab, pal->dark_mode ? 0.22 : 0.16);
        ngfx_stroke_rect(cr, x + 0.5, y + 0.5, w - 1, h - 1, r, 0.9, pal->ar, pal->ag, pal->ab, 0.60);
    } else {
        ngfx_fill_rect(cr, x, y, w, h, r, pal->elev_r, pal->elev_g, pal->elev_b, 0.85);
        ngfx_stroke_rect(cr, x + 0.5, y + 0.5, w - 1, h - 1, r, 0.8, pal->border_r, pal->border_g, pal->border_b, pal->border_a);
    }
    if (label && *label) {
        double tw = ngfx_text_width(cr, "JetBrains Mono", 1, 9.4, label);
        double tx = x + (w - tw) * 0.5;
        double ty = y + h * 0.5 + 3.5;
        ngfx_text(cr, "JetBrains Mono", 1, 9.4, tx, ty,
                  is_accent ? (pal->dark_mode ? pal->ar_hi : pal->ar) : pal->mut_r,
                  is_accent ? (pal->dark_mode ? pal->ag_hi : pal->ag) : pal->mut_g,
                  is_accent ? (pal->dark_mode ? pal->ab_hi : pal->ab) : pal->mut_b, 0.98, label);
    }
}

void ntheme_draw_slider(cairo_t *cr, double x, double y, double w, double h, double val_pct,
                        const NaturePalette *pal, int is_accent) {
    if (val_pct < 0.0) val_pct = 0.0;
    if (val_pct > 1.0) val_pct = 1.0;
    double r = h * 0.5;
    ngfx_fill_rect(cr, x, y, w, h, r, pal->elev_r, pal->elev_g, pal->elev_b, 1.0);
    double fill_w = w * val_pct;
    if (fill_w > 0.0) {
        double fr = is_accent ? pal->ar : pal->warning_r;
        double fg = is_accent ? pal->ag : pal->warning_g;
        double fb = is_accent ? pal->ab : pal->warning_b;
        ngfx_fill_gradient(cr, x, y, fill_w, h, r,
                           fr, fg, fb, 0.95,
                           fr * 1.15 > 1 ? 1 : fr * 1.15,
                           fg * 1.15 > 1 ? 1 : fg * 1.15,
                           fb * 1.15 > 1 ? 1 : fb * 1.15, 0.98, 1);
    }
    double kx = x + fill_w;
    double ky = y + h * 0.5;
    ngfx_fill_circle(cr, kx, ky + 1.0, r + 2.5, 0.0, 0.0, 0.0, 0.28);
    ngfx_fill_circle(cr, kx, ky, r + 2.0, 1.0, 1.0, 1.0, 0.98);
    ngfx_fill_circle(cr, kx, ky, r - 1.0,
                     is_accent ? pal->ar : pal->warning_r,
                     is_accent ? pal->ag : pal->warning_g,
                     is_accent ? pal->ab : pal->warning_b, 0.95);
}

void ntheme_draw_avatar(cairo_t *cr, cairo_surface_t *img, double x, double y, double size,
                        const NaturePalette *pal) {
    double rad = size * 0.5;
    double cx = x + rad, cy = y + rad;
    if (img) {
        g_ngfx.save(cr);
        g_ngfx.new_sub_path(cr);
        g_ngfx.arc(cr, cx, cy, rad, 0, 6.28318);
        g_ngfx.clip(cr);
        double sw = g_ngfx.image_surface_get_width(img);
        double sh = g_ngfx.image_surface_get_height(img);
        if (sw > 0 && sh > 0) {
            double sc = (size / sw > size / sh) ? (size / sw) : (size / sh);
            g_ngfx.translate(cr, x + (size - sw * sc) * 0.5, y + (size - sh * sc) * 0.5);
            g_ngfx.scale(cr, sc, sc);
            g_ngfx.set_source_surface(cr, img, 0, 0);
            g_ngfx.paint(cr);
        }
        g_ngfx.restore(cr);
    } else {
        ngfx_fill_circle(cr, cx, cy, rad, pal->ar, pal->ag, pal->ab, 0.90);
        ngfx_text(cr, "Plus Jakarta Sans", 1, size * 0.44, cx - size * 0.16, cy + size * 0.16, 1, 1, 1, 1, "N");
    }
    g_ngfx.set_source_rgba(cr, pal->ar, pal->ag, pal->ab, 0.85);
    g_ngfx.set_line_width(cr, 1.5);
    g_ngfx.arc(cr, cx, cy, rad, 0, 6.28318);
    g_ngfx.stroke(cr);
}

void ntheme_draw_modal_scrim(cairo_t *cr, double w, double h, double alpha) {
    if (alpha <= 0.0) return;
    ngfx_fill_rect(cr, 0, 0, w, h, 0, 0.02, 0.03, 0.04, alpha);
}

void ntheme_draw_section_header(cairo_t *cr, double x, double y, double w,
                                const NaturePalette *pal, const char *title, const char *right_meta) {
    if (title && *title) {
        ngfx_text(cr, "Noto Sans", 1, 10.2, x, y + 11, pal->mut_r, pal->mut_g, pal->mut_b, 0.95, title);
    }
    if (right_meta && *right_meta) {
        double rw = ngfx_text_width(cr, "JetBrains Mono", 0, 9.5, right_meta);
        ngfx_text(cr, "JetBrains Mono", 0, 9.5, x + w - rw, y + 11, pal->mut_r, pal->mut_g, pal->mut_b, 0.88, right_meta);
    }
    ngfx_fill_rect(cr, x, y + 16, w, 1, 0, pal->border_r, pal->border_g, pal->border_b, pal->border_a);
}

void ntheme_draw_kbd_chip(cairo_t *cr, double x, double y, double h,
                          const NaturePalette *pal, const char *key_label, double *out_w) {
    const char *lbl = (key_label && *key_label) ? key_label : "";
    double tw = ngfx_text_width(cr, "JetBrains Mono", 1, 9.8, lbl);
    double w = tw + 14.0;
    if (w < 22.0) w = 22.0;
    ngfx_fill_rect(cr, x, y + 1.0, w, h, 5.0, 0.0, 0.0, 0.0, pal->dark_mode ? 0.35 : 0.12);
    ngfx_fill_rect(cr, x, y, w, h, 5.0, pal->elev_r, pal->elev_g, pal->elev_b, 0.98);
    ngfx_stroke_rect(cr, x + 0.5, y + 0.5, w - 1.0, h - 1.0, 5.0, 0.9,
                     pal->strong_brd_r, pal->strong_brd_g, pal->strong_brd_b, pal->strong_brd_a);
    ngfx_text(cr, "JetBrains Mono", 1, 9.8, x + (w - tw) * 0.5, y + h * 0.5 + 3.5,
              pal->fg_r, pal->fg_g, pal->fg_b, 0.96, lbl);
    if (out_w) *out_w = w;
}
