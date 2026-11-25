#ifndef NATURE_OS_H
#define NATURE_OS_H

#include "nature_gfx.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define OS_WIDTH 1280
#define OS_HEIGHT 720
#define TILE_W 64
#define TILE_H 45

typedef enum {
    APP_FILES = 0,
    APP_BROWSER = 1,
    APP_TERMINAL = 2,
    APP_STORE = 3,
    APP_MONITOR = 4,
    APP_MEDIA = 5,
    APP_SETTINGS = 6,
    APP_COUNT = 7
} AppType;

typedef struct {
    int dark_mode;
    int wallpaper_idx;      /* 0=Canopy, 1=Alpine, 2=Moss */
    int accent_idx;         /* 0=Moss, 1=Teal, 2=Amber, 3=Alpine */
    int dock_position;      /* 0=Bottom, 1=Left (Ubuntu style) */
    int low_power_mode;
    int zram_enabled;
    int wifi_enabled;
    int bluetooth_enabled;
    int night_light;        /* Warm blue-light filter */
    int audio_muted;
    int audio_device;       /* 0=Analog Stereo, 1=HDMI/DP, 2=Bluetooth Buds */
    int target_hz;
    int volume;
    int brightness;
    int power_profile;     /* 0=Power Saver, 1=Balanced, 2=Performance */
} OSConfig;

typedef struct {
    int id;
    AppType app;
    char title[64];
    int x, y, w, h;
    int z_order;
    int minimized;
    int maximized;
    int snapped;            /* 0=none, 1=left, 2=right */
    int workspace;          /* 0 or 1 */
    int pid;
} OSWindow;

#define MAX_WINDOWS 16
#define MAX_VFS 96
#define MAX_TERM_LINES 140
#define MAX_HIT_RECTS 420
#define MAX_TEXT_RECTS 8
#define MAX_APT_PKGS 12
#define MAX_MEDIA_IMAGES 6
#define MAX_AUDIO_TRACKS 3

typedef struct {
    char name[48];
    char path[128];
    char parent[128];
    int is_dir;
    int size_bytes;
    int image_idx;          /* >= 0 for images */
    int audio_idx;          /* >= 0 for audio files */
    char mime_tag[16];      /* DIR, IMG, AUD, C, MD, CONF, DEB, ISO, LOG, TXT */
    char modified[24];
    char content[768];
} VfsNode;

typedef struct {
    char name[32];
    char display_name[48];
    char category[24];
    char version[32];
    char source[32];
    char summary[128];
    int download_mb_x10;
    int ram_mb;
    int installed;
    int progress;           /* 0..100 for install animation */
} AptPkg;

typedef struct {
    int pid;
    char name[32];
    char user[16];
    char state[16];
    double cpu_pct;
    int ram_mb;
    int nice_val;
    int win_id;
    int killable;
} ProcEntry;

typedef struct {
    int x, y, w, h;
    int action_id;
    int arg0;
    int arg1;
} HitRect;

typedef struct {
    int x, y, w, h;
    int target_id;          /* 1=Terminal, 2=Spotlight, 3=BrowserURL, 4=FilesSearch */
} TextInputRect;

typedef enum {
    SCROLL_NONE = 0,
    SCROLL_FILES = 1,
    SCROLL_TERMINAL = 2,
    SCROLL_MONITOR = 3,
    SCROLL_SETTINGS = 4,
    SCROLL_STORE = 5,
    SCROLL_BROWSER = 6,
    SCROLL_MEDIA_AUDIO = 7
} ScrollTargetKind;

typedef enum {
    HIT_NONE = 0,
    HIT_DOCK_APP = 1,
    HIT_DOCK_LAUNCHER = 2,
    HIT_PANEL_ACTIVITIES = 3,
    HIT_PANEL_WS = 4,
    HIT_PANEL_THEME = 5,
    HIT_PANEL_STRESS = 6,
    HIT_PANEL_TRAY = 7,
    HIT_PANEL_CLOCK = 8,
    HIT_WIN_CLOSE = 10,
    HIT_WIN_MAX = 11,
    HIT_WIN_MIN = 12,
    HIT_WIN_SNAP = 13,
    HIT_WIN_TITLE = 14,
    HIT_WIN_RESIZE = 15,
    HIT_SCROLLBAR = 16,
    HIT_LAUNCHER_APP = 20,
    HIT_LAUNCHER_SEARCH = 21,
    HIT_DESKTOP_ICON = 25,
    HIT_OVERVIEW_WIN = 30,
    HIT_CTX_MENU_ITEM = 35,

    /* Quick Settings Popover Hits */
    HIT_QS_WIFI = 40,
    HIT_QS_BLUETOOTH = 41,
    HIT_QS_DARK = 42,
    HIT_QS_PERF = 43,
    HIT_QS_VOL_DOWN = 44,
    HIT_QS_VOL_UP = 45,
    HIT_QS_REBOOT = 46,
    HIT_QS_SETTINGS = 47,
    HIT_QS_NIGHT_LIGHT = 48,
    HIT_QS_BRIGHT_DOWN = 49,
    HIT_QS_BRIGHT_UP = 50,
    HIT_SLIDER_VOL = 51,
    HIT_SLIDER_BRIGHT = 52,
    HIT_CAL_CLEAR_NOTIFS = 53,

    /* Files Hits */
    HIT_FILES_PLACE = 100,
    HIT_FILES_NODE = 101,
    HIT_FILES_NEW_DIR = 102,
    HIT_FILES_NEW_FILE = 103,
    HIT_FILES_VIEW_MODE = 104,
    HIT_FILES_DELETE = 105,
    HIT_FILES_OPEN_PARENT = 106,
    HIT_FILES_OPEN_ACTION = 107,
    HIT_FILES_SET_WP = 108,
    HIT_FILES_SORT = 109,
    HIT_FILES_SCROLL = 110,
    HIT_FILES_SEARCH_BOX = 111,
    HIT_FILES_SEARCH_CLEAR = 112,
    HIT_FILES_CLOSE_VIEWER = 113,

    /* Monitor Hits */
    HIT_MONITOR_TAB = 200,
    HIT_MONITOR_KILL = 201,
    HIT_MONITOR_RENICE = 202,
    HIT_MONITOR_STRESS = 203,
    HIT_MONITOR_CLEAR_STRESS = 204,
    HIT_MONITOR_STRESS_THREADS = 205,

    /* Settings Hits */
    HIT_SETTINGS_SECTION = 300,
    HIT_SETTINGS_DARK = 301,
    HIT_SETTINGS_WALLPAPER = 302,
    HIT_SETTINGS_ACCENT = 303,
    HIT_SETTINGS_DOCK_POS = 304,
    HIT_SETTINGS_LOW_POWER = 305,
    HIT_SETTINGS_ZRAM = 306,
    HIT_SETTINGS_WIFI = 307,
    HIT_SETTINGS_HZ = 308,
    HIT_SETTINGS_PERF_MODE = 309,
    HIT_SETTINGS_NIGHT_LIGHT = 310,
    HIT_SETTINGS_BLUETOOTH = 311,
    HIT_SETTINGS_BRIGHTNESS = 312,
    HIT_SETTINGS_VOLUME = 313,
    HIT_SETTINGS_MUTE = 314,
    HIT_SETTINGS_AUDIO_DEV = 315,
    HIT_SETTINGS_CHECK_UPDATE = 316,

    /* Store Hits */
    HIT_STORE_CAT = 400,
    HIT_STORE_TOGGLE_PKG = 401,
    HIT_STORE_SELECT_PKG = 402,

    /* Media Hits */
    HIT_MEDIA_SELECT = 500,
    HIT_MEDIA_ZOOM = 501,
    HIT_MEDIA_SET_WP = 502,
    HIT_MEDIA_PREV = 503,
    HIT_MEDIA_NEXT = 504,
    HIT_MEDIA_MODE = 505,
    HIT_MEDIA_ROTATE = 506,
    HIT_MEDIA_AUDIO_PLAY = 507,
    HIT_MEDIA_AUDIO_SELECT = 508,
    HIT_MEDIA_AUDIO_SEEK = 509,

    /* Browser Hits */
    HIT_BROWSER_TAB = 600,
    HIT_BROWSER_RELOAD = 601,
    HIT_BROWSER_BACK = 602,
    HIT_BROWSER_FWD = 603,
    HIT_BROWSER_ACTION = 604,
    HIT_BROWSER_URL_BAR = 605,

    /* Terminal Hits */
    HIT_TERM_CHIP = 700,
    HIT_TERM_INPUT_AREA = 701
} HitActionType;

typedef struct {
    OSConfig cfg;
    int screen_w;
    int screen_h;

    OSWindow windows[MAX_WINDOWS];
    int window_count;
    int active_window_id;
    int active_workspace;
    int next_z;
    int next_win_id;
    int next_pid;
    int launcher_open;
    int quick_settings_open;
    int calendar_open;
    int hover_dock_idx;
    int hover_mx, hover_my;
    int hover_hit_action;
    int hover_hit_arg0;

    /* Desktop Context Menu */
    int ctx_menu_open;
    int ctx_menu_x, ctx_menu_y;

    /* Interactive drag, resize, scrollbar & slider state */
    int drag_mode;          /* 0=none, 1=move win, 2=resize win, 3=scrollbar, 4=vol slider, 5=bright slider */
    int drag_win_id;
    int drag_start_mx, drag_start_my;
    int drag_orig_x, drag_orig_y, drag_orig_w, drag_orig_h;
    int snap_preview;

    int scroll_drag_target;
    int scroll_drag_track_y;
    int scroll_drag_track_h;
    int scroll_drag_max;
    int slider_x, slider_w;

    /* Hit testing & Text Input detection */
    HitRect hits[MAX_HIT_RECTS];
    int hit_count;
    TextInputRect text_rects[MAX_TEXT_RECTS];
    int text_rect_count;
    int active_text_target; /* 0=none, 1=Terminal, 2=Spotlight, 3=BrowserURL, 4=FilesSearch */
    int input_focus_pulse;

    /* Background & Media surfaces */
    cairo_surface_t *wallpapers[3];
    cairo_surface_t *cached_wp;
    int cached_wp_w, cached_wp_h, cached_wp_idx, cached_wp_dark;
    cairo_surface_t *media_images[MAX_MEDIA_IMAGES];
    cairo_surface_t *avatar_surface;

    /* Notification toast + history */
    char toast_title[64];
    char toast_msg[128];
    int toast_timer;        /* Frames remaining to show */
    char notif_titles[4][48];
    char notif_msgs[4][96];
    int notif_count;

    /* Launcher Search */
    char launcher_query[48];

    /* Virtual Filesystem & Files App */
    VfsNode vfs[MAX_VFS];
    int vfs_count;
    char files_cwd[128];
    int files_selected_idx;
    int files_list_mode;
    int files_sort_mode;    /* 0=Name, 1=Size */
    int files_scroll;
    int files_max_scroll;
    char files_query[48];
    int files_viewer_open;
    int files_viewer_idx;

    /* Terminal Emulator */
    char term_cwd[128];
    char term_lines[MAX_TERM_LINES][128];
    uint8_t term_line_colors[MAX_TERM_LINES];
    int term_line_count;
    char term_input[128];
    char term_history[24][128];
    int term_hist_count;
    int term_hist_idx;
    int term_cursor_visible;
    int term_scroll;        /* 0=bottom, >0 lines scrolled back */
    int term_max_scroll;

    /* System Monitor & Real Linux Telemetry */
    int monitor_tab;
    double cpu_history[48];
    double core_usage[4];   /* Live per-core usage from /proc/stat */
    double measured_fps;
    double frame_ms;
    double compositor_cpu_pct;
    int cpu_count;
    int total_ram_mb;
    int used_ram_mb;
    int avail_ram_mb;
    int cached_ram_mb;
    int os_rss_mb;
    int zram_used_mb;
    double total_cpu_pct;
    long disk_total_gb;
    long disk_avail_gb;
    int disk_used_pct;
    long uptime_sec;
    char cpu_model[64];
    char kernel_ver[64];
    int stress_active;
    int stress_threads;
    ProcEntry procs[32];
    int proc_count;
    int monitor_scroll;
    int monitor_max_scroll;

    /* Settings */
    int settings_section;   /* 0..5 */
    int settings_update_checked;
    int settings_scroll;
    int settings_max_scroll;

    /* Ubuntu Store */
    AptPkg pkgs[MAX_APT_PKGS];
    int pkg_count;
    int store_cat;
    int store_selected;
    int store_scroll;
    int store_max_scroll;

    /* Media Viewer & Audio Player */
    int media_mode;         /* 0=Photos, 1=Audio */
    int media_selected;
    int media_zoom_pct;
    int media_rotation;     /* 0, 90, 180, 270 */
    int audio_selected;
    int audio_playing;
    int audio_progress_sec;
    int audio_tick_accum;
    int audio_scroll;
    int audio_max_scroll;

    /* Web Browser */
    int browser_tab;
    int browser_reloads;
    int browser_scroll;
    int browser_max_scroll;
    char browser_url_input[96];

    int dirty;
} NatureOSState;

/* Shell / Window Manager API */
void nature_os_init(NatureOSState *st, const char *asset_dir);
void nature_os_resize(NatureOSState *st, int w, int h);
void nature_os_render(NatureOSState *st, cairo_t *cr);
void nature_os_pointer_down(NatureOSState *st, int mx, int my);
void nature_os_right_click(NatureOSState *st, int mx, int my);
void nature_os_pointer_move(NatureOSState *st, int mx, int my);
void nature_os_pointer_up(NatureOSState *st, int mx, int my);
void nature_os_wheel(NatureOSState *st, int delta_y);
void nature_os_key(NatureOSState *st, const char *key);
void nature_notify(NatureOSState *st, const char *title, const char *msg);

void nature_get_accent(int accent_idx, double *r, double *g, double *b);
void nature_add_hit(NatureOSState *st, int x, int y, int w, int h, int action_id, int arg0, int arg1);
void nature_add_text_rect(NatureOSState *st, int x, int y, int w, int h, int target_id);
void nature_occlude_text_rects(NatureOSState *st, int x, int y, int w, int h);
void nature_draw_scrollbar(NatureOSState *st, cairo_t *cr, int x, int y, int h,
                           int total_items, int visible_items, int scroll_pos,
                           int max_scroll, int target_kind, int dark);
OSWindow *nature_open_app(NatureOSState *st, AppType app);
void nature_focus_window(NatureOSState *st, int win_id);
void nature_close_window(NatureOSState *st, int win_id);

/* Native C/C++ System App hooks */
void nature_files_render(NatureOSState *st, cairo_t *cr, OSWindow *w, int cx, int cy, int cw, int ch);
void nature_files_click(NatureOSState *st, const HitRect *hit);
void nature_files_wheel(NatureOSState *st, int delta_y);
void nature_files_key(NatureOSState *st, const char *key);

void nature_terminal_render(NatureOSState *st, cairo_t *cr, OSWindow *w, int cx, int cy, int cw, int ch);
void nature_terminal_key(NatureOSState *st, const char *key);
void nature_terminal_exec(NatureOSState *st, const char *cmd);
void nature_terminal_click(NatureOSState *st, const HitRect *hit);
void nature_terminal_wheel(NatureOSState *st, int delta_y);

void nature_monitor_init(NatureOSState *st);
void nature_monitor_tick(NatureOSState *st);
void nature_monitor_render(NatureOSState *st, cairo_t *cr, OSWindow *w, int cx, int cy, int cw, int ch);
void nature_monitor_click(NatureOSState *st, const HitRect *hit);
void nature_monitor_wheel(NatureOSState *st, int delta_y);
void nature_monitor_set_stress(NatureOSState *st, int enable);
void nature_monitor_set_threads(NatureOSState *st, int threads);

void nature_settings_render(NatureOSState *st, cairo_t *cr, OSWindow *w, int cx, int cy, int cw, int ch);
void nature_settings_click(NatureOSState *st, const HitRect *hit);
void nature_settings_wheel(NatureOSState *st, int delta_y);

void nature_store_init(NatureOSState *st);
void nature_store_render(NatureOSState *st, cairo_t *cr, OSWindow *w, int cx, int cy, int cw, int ch);
void nature_store_click(NatureOSState *st, const HitRect *hit);
void nature_store_wheel(NatureOSState *st, int delta_y);

void nature_media_render(NatureOSState *st, cairo_t *cr, OSWindow *w, int cx, int cy, int cw, int ch);
void nature_media_click(NatureOSState *st, const HitRect *hit);
void nature_media_wheel(NatureOSState *st, int delta_y);
void nature_media_tick(NatureOSState *st);

void nature_browser_render(NatureOSState *st, cairo_t *cr, OSWindow *w, int cx, int cy, int cw, int ch);
void nature_browser_click(NatureOSState *st, const HitRect *hit);
void nature_browser_wheel(NatureOSState *st, int delta_y);
void nature_browser_key(NatureOSState *st, const char *key);

#ifdef __cplusplus
}
#endif

#endif
