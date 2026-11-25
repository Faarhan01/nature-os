/*
 * nature-compositor: Lightweight Wayland/DRM compositor for Nature OS
 * Supports native wlroots Wayland mode on bare-metal Ubuntu (NATURE_USE_WLROOTS)
 * and headless Cairo framebuffer presentation for virtual display transports.
 */

#include "nature_os.h"
#include "render_scheduler.h"
#include <stdio.h>
#include <stdlib.h>

#ifdef NATURE_USE_WLROOTS
#include <wlr/backend.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_xdg_shell.h>
#endif

int nature_compositor_run_headless_frame(NatureOSState *st, cairo_t *cr) {
    if (!st || !cr) return -1;
    nature_os_render(st, cr);
    return 0;
}

#ifdef NATURE_STANDALONE_COMPOSITOR
int main(int argc, char *argv[]) {
    (void)argc; (void)argv;
    if (nature_gfx_load() != 0) {
        fprintf(stderr, "nature-compositor: failed to load Cairo/zlib backend\n");
        return EXIT_FAILURE;
    }

    struct nature_scheduler sched;
    nature_scheduler_init(&sched, 60);

    NatureOSState state;
    nature_os_init(&state, "/usr/share/nature-os/assets");

    cairo_surface_t *fb = g_ngfx.image_surface_create(CAIRO_FORMAT_ARGB32, OS_WIDTH, OS_HEIGHT);
    cairo_t *cr = g_ngfx.create(fb);
    nature_compositor_run_headless_frame(&state, cr);

    printf("nature-compositor: initialized %dx%d surface @ %u Hz\n",
           OS_WIDTH, OS_HEIGHT, sched.target_hz);

    g_ngfx.destroy(cr);
    g_ngfx.surface_destroy(fb);
    return EXIT_SUCCESS;
}
#endif
