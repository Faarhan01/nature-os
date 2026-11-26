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

typedef enum {
    THEME_NATURE = 0,
    THEME_HORIZON = 1
} ThemeFamily;

typedef struct {
    int theme_family;       /* 0=Nature OS, 1=Horizon theme */
    int dark_mode;          /* 1=Dark, 0=Light */
    int wallpaper_idx;      /* 0..7 preset wallpapers, 8=custom from media */
    int accent_idx;         /* 0..5 */
    int dock_position;      /* 0=Bottom, 1=Left (Ubuntu style) */
    int dock_size;          /* 0=Compact, 1=Standard, 2=Large */
    int animations_enabled; /* 1=Smooth transitions, 0=Reduced motion */
    int show_panel_stats;   /* 1=Show top bar live telemetry pill */
    int telemetry_ms;       /* 250, 500, 1000 ms telemetry cadence */
    int auto_lock_min;      /* 0=Never, 5, 15 minutes */
    int low_power_mode;
    int zram_enabled;
    int wifi_enabled;
    int bluetooth_enabled;
    int night_light;        /* Warm blue-light filter */
    int night_light_warmth; /* 20..100 intensity */
    int audio_muted;
    int audio_device;       /* 0=Analog Stereo, 1=HDMI/DP, 2=Bluetooth Buds */
    int target_hz;
    int volume;
    int brightness;
    int power_profile;      /* 0=Power Saver, 1=Balanced, 2=Performance */
    int clock_24h;          /* 1=24h, 0=12h AM/PM */
    int show_seconds;       /* 0=HH:MM, 1=HH:MM:SS */
    int window_snap;        /* 1=Edge drag snap enabled */
    int show_desktop_widgets; /* 1=Ambient desktop clock & shortcut cards */
    int show_toast_popups;  /* 1=Show desktop notification popups */
    int term_font_size;     /* 10..14 Terminal monospace font size */
    int compact_chrome;     /* 0=Standard, 1=Compact window headers */
    int show_dock_labels;   /* 1=Show hover tooltips & active badges on dock */
    char device_name[32];   /* Editable PC hostname */
} OSConfig;

typedef struct {
    int theme_family;       /* 0=Nature OS, 1=Horizon */
    int dark_mode;          /* 1=Dark, 0=Light */
    double ar, ag, ab;      /* Primary accent color */
    double ar_hi, ag_hi, ab_hi; /* Brighter highlight accent */
    double on_acc_r, on_acc_g, on_acc_b; /* Text/icon color on accent fills */
    double win_bg_r, win_bg_g, win_bg_b;
    double surface_r, surface_g, surface_b;
    double sidebar_r, sidebar_g, sidebar_b;
    double card_r, card_g, card_b;
    double elev_r, elev_g, elev_b;
    double hover_r, hover_g, hover_b;
    double hdr_act_top_r, hdr_act_top_g, hdr_act_top_b;
    double hdr_act_bot_r, hdr_act_bot_g, hdr_act_bot_b;
    double hdr_inact_top_r, hdr_inact_top_g, hdr_inact_top_b;
    double hdr_inact_bot_r, hdr_inact_bot_g, hdr_inact_bot_b;
    double panel_top_r, panel_top_g, panel_top_b, panel_alpha;
    double panel_bot_r, panel_bot_g, panel_bot_b;
    double dock_top_r, dock_top_g, dock_top_b, dock_alpha;
    double dock_bot_r, dock_bot_g, dock_bot_b;
    double border_r, border_g, border_b, border_a;
    double strong_brd_r, strong_brd_g, strong_brd_b, strong_brd_a;
    double fg_r, fg_g, fg_b;
    double fg_sec_r, fg_sec_g, fg_sec_b;
    double mut_r, mut_g, mut_b;
    double danger_r, danger_g, danger_b;
    double success_r, success_g, success_b;
    double warning_r, warning_g, warning_b;
    /* Design Tokens */
    double r_win, r_card, r_btn, r_pill, r_dock;
    double card_alpha, elev_alpha, shadow_alpha;
} NaturePalette;

typedef struct {
    int id;
    AppType app;
    char title[64];
    int x, y, w, h;
    int restore_x, restore_y, restore_w, restore_h; /* Bounds before maximize/snap */
    int z_order;
    int minimized;
    int maximized;
    int snapped;            /* 0=none, 1=left, 2=right */
    int workspace;          /* 0 or 1 */
    int pid;
} OSWindow;

#define MAX_WINDOWS 16
#define MAX_VFS 128
#define MAX_TERM_LINES 200
#define MAX_HIT_RECTS 768
#define MAX_TEXT_RECTS 12
#define MAX_APT_PKGS 16
#define MAX_WALLPAPERS 8
#define MAX_MEDIA_IMAGES 11
#define MAX_VIDEO_CLIPS 4
#define MAX_AUDIO_TRACKS 4

typedef struct {
    char name[48];
    char path[128];
    char parent[128];
    int is_dir;
    int size_bytes;
    int image_idx;          /* >= 0 for images */
    int audio_idx;          /* >= 0 for audio files */
    int video_idx;          /* >= 0 for video files */
    char mime_tag[16];      /* DIR, IMG, AUD, VID, C, MD, CONF, DEB, ISO, LOG, TXT */
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
    int target_id;          /* 1=Term, 2=Launcher, 3=BrowserURL, 4=FilesSearch, 5=SettingsHost, 6=SettingsSearch, 7=FilesRename, 8=StoreSearch, 9=FilesEditor */
} TextInputRect;

typedef enum {
    SCROLL_NONE = 0,
    SCROLL_FILES = 1,
    SCROLL_TERMINAL = 2,
    SCROLL_MONITOR = 3,
    SCROLL_SETTINGS = 4,
    SCROLL_STORE = 5,
    SCROLL_BROWSER = 6,
    SCROLL_MEDIA_AUDIO = 7,
    SCROLL_MEDIA_GRID = 8,
    SCROLL_MEDIA_VIDEO = 9,
    SCROLL_MEDIA_FOLDER = 10
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
    HIT_PANEL_OVERVIEW = 9,
    HIT_WIN_CLOSE = 10,
    HIT_WIN_MAX = 11,
    HIT_WIN_MIN = 12,
    HIT_WIN_SNAP = 13,
    HIT_WIN_TITLE = 14,
    HIT_WIN_RESIZE = 15,
    HIT_SCROLLBAR = 16,
    HIT_WIN_SPACE_SWAP = 17,
    HIT_PANEL_LOCK = 18,
    HIT_PANEL_ACTIVE_WIN = 19,
    HIT_LAUNCHER_APP = 20,
    HIT_LAUNCHER_SEARCH = 21,
    HIT_LAUNCHER_CAT = 22,
    HIT_LAUNCHER_FILE = 23,
    HIT_LAUNCHER_SETTING = 24,
    HIT_DESKTOP_ICON = 25,
    HIT_LAUNCHER_ACTION = 26,
    HIT_OVERVIEW_WIN = 30,
    HIT_OVERVIEW_CLOSE = 31,
    HIT_OVERVIEW_SWAP = 32,
    HIT_OVERVIEW_EXIT = 33,
    HIT_OVERVIEW_SHOW_DESKTOP = 34,
    HIT_CTX_MENU_ITEM = 35,
    HIT_TOAST_DISMISS = 36,
    HIT_OVERVIEW_CLOSE_ALL = 37,
    HIT_LOCK_UNLOCK = 38,
    HIT_OVERVIEW_FILTER = 39,

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
    HIT_QS_LOCK = 54,
    HIT_QS_THEME_FAMILY = 55,
    HIT_QS_ACCENT = 56,
    HIT_QS_TILE = 57,
    HIT_QS_MUTE = 58,
    HIT_CAL_MONTH_NAV = 59,
    HIT_QS_MEDIA_TOGGLE = 60,
    HIT_QS_MEDIA_NEXT = 61,
    HIT_CAL_TEST_NOTIF = 62,
    HIT_QS_DND = 63,
    HIT_QS_AUDIO_DEV = 64,
    HIT_CAL_DISMISS_NOTIF = 65,
    HIT_POWER_DIALOG_OPEN = 70,
    HIT_POWER_LOCK = 71,
    HIT_POWER_RESTART = 72,
    HIT_POWER_SLEEP = 73,
    HIT_POWER_CANCEL = 74,
    HIT_ALT_TAB_PREV = 75,
    HIT_ALT_TAB_NEXT = 76,
    HIT_ALT_TAB_SELECT = 77,

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
    HIT_FILES_RENAME_START = 114,
    HIT_FILES_RENAME_BOX = 115,
    HIT_FILES_RENAME_APPLY = 116,
    HIT_FILES_DUPLICATE = 117,
    HIT_FILES_BREADCRUMB = 118,
    HIT_FILES_HIDDEN_TOGGLE = 119,
    HIT_FILES_OPEN_TERM = 120,
    HIT_FILES_EDIT_MODE = 121,
    HIT_FILES_EDIT_SAVE = 122,
    HIT_FILES_EDIT_CANCEL = 123,
    HIT_FILES_EDIT_BOX = 124,
    HIT_FILES_RUN_TERM = 125,
    HIT_FILES_OPEN_MEDIA = 126,

    /* Monitor Hits */
    HIT_MONITOR_TAB = 200,
    HIT_MONITOR_KILL = 201,
    HIT_MONITOR_RENICE = 202,
    HIT_MONITOR_STRESS = 203,
    HIT_MONITOR_CLEAR_STRESS = 204,
    HIT_MONITOR_STRESS_THREADS = 205,
    HIT_MONITOR_CHART_MODE = 206,
    HIT_MONITOR_SORT = 207,
    HIT_MONITOR_FOCUS = 208,

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
    HIT_SETTINGS_THEME_FAMILY = 317,
    HIT_SETTINGS_CLOCK_24H = 318,
    HIT_SETTINGS_CLOCK_SEC = 319,
    HIT_SETTINGS_WINDOW_SNAP = 320,
    HIT_SETTINGS_DESKTOP_WIDGETS = 321,
    HIT_SETTINGS_SEARCH_BOX = 322,
    HIT_SETTINGS_SEARCH_CLEAR = 323,
    HIT_SETTINGS_HOST_BOX = 324,
    HIT_SETTINGS_HOST_SAVE = 325,
    HIT_SETTINGS_TOAST_POPUPS = 326,
    HIT_SETTINGS_LOCK_NOW = 327,
    HIT_SETTINGS_RESET_DEFAULTS = 328,
    HIT_SETTINGS_NL_WARMTH = 329,
    HIT_SETTINGS_SHORTCUT_ACTION = 330,
    HIT_SETTINGS_NET_TEST = 331,
    HIT_SETTINGS_SOUND_TEST = 332,
    HIT_SETTINGS_CLEAR_USER_DATA = 333,
    HIT_SETTINGS_DOCK_SIZE = 334,
    HIT_SETTINGS_ANIMATIONS = 335,
    HIT_SETTINGS_PANEL_STATS = 336,
    HIT_SETTINGS_TELEMETRY_MS = 337,
    HIT_SETTINGS_AUTO_LOCK = 338,
    HIT_SETTINGS_TERM_FONT = 339,
    HIT_SETTINGS_COMPACT_CHROME = 340,
    HIT_SETTINGS_SVC_TOGGLE = 341,
    HIT_SETTINGS_DOCK_LABELS = 342,

    /* Store Hits */
    HIT_STORE_CAT = 400,
    HIT_STORE_TOGGLE_PKG = 401,
    HIT_STORE_SELECT_PKG = 402,
    HIT_STORE_SEARCH_BOX = 403,
    HIT_STORE_SEARCH_CLEAR = 404,
    HIT_STORE_INSTALLED_FILTER = 405,
    HIT_STORE_LAUNCH_PKG = 406,
    HIT_STORE_REFRESH = 407,

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
    HIT_MEDIA_GRID_TOGGLE = 510,
    HIT_MEDIA_SLIDESHOW = 511,
    HIT_MEDIA_ZOOM_IN = 512,
    HIT_MEDIA_ZOOM_OUT = 513,
    HIT_MEDIA_ZOOM_FIT = 514,
    HIT_MEDIA_AUDIO_LOOP = 515,
    HIT_MEDIA_AUDIO_SPEED = 516,
    HIT_MEDIA_AUDIO_STEP = 517,
    HIT_MEDIA_AUDIO_MUTE = 518,
    HIT_MEDIA_AUDIO_SHUFFLE = 519,
    HIT_MEDIA_AUDIO_PREV = 520,
    HIT_MEDIA_AUDIO_NEXT = 521,
    HIT_MEDIA_INFO_TOGGLE = 522,
    HIT_MEDIA_AUDIO_EQ = 523,
    HIT_MEDIA_FILTER = 524,
    HIT_MEDIA_VIDEO_PLAY = 525,
    HIT_MEDIA_VIDEO_SELECT = 526,
    HIT_MEDIA_VIDEO_SEEK = 527,
    HIT_MEDIA_VIDEO_PREV = 528,
    HIT_MEDIA_VIDEO_NEXT = 529,
    HIT_MEDIA_VIDEO_STEP = 530,
    HIT_MEDIA_VIDEO_SPEED = 531,
    HIT_MEDIA_VIDEO_LOOP = 532,
    HIT_MEDIA_VIDEO_MUTE = 533,
    HIT_MEDIA_VIDEO_HUD = 534,
    HIT_MEDIA_VIDEO_FIT = 535,
    HIT_MEDIA_SIDEBAR_TOGGLE = 536,
    HIT_MEDIA_FOLDER = 537,
    HIT_MEDIA_OPEN_VFS = 538,
    HIT_MEDIA_FOLDER_OPEN_FILES = 539,

    /* Browser Hits */
    HIT_BROWSER_TAB = 600,
    HIT_BROWSER_RELOAD = 601,
    HIT_BROWSER_BACK = 602,
    HIT_BROWSER_FWD = 603,
    HIT_BROWSER_ACTION = 604,
    HIT_BROWSER_URL_BAR = 605,
    HIT_BROWSER_THEME_ACTION = 606,
    HIT_BROWSER_RUN_STRESS = 607,

    /* Terminal Hits */
    HIT_TERM_CHIP = 700,
    HIT_TERM_INPUT_AREA = 701,
    HIT_TERM_FONT = 702
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
    int launcher_cat;       /* 0=All, 1=System, 2=Internet, 3=Media, 4=Utilities */
    int launcher_sel;       /* Selected Spotlight result index */
    int overview_open;
    int overview_ws_filter; /* 0=All, 1=WS1, 2=WS2 */
    int overview_scroll;
    int is_locked;
    int quick_settings_open;
    int calendar_open;
    int cal_month_offset;
    int power_dialog_open;  /* Real Power / Session modal */
    int alt_tab_open;       /* Alt-Tab / Window Switcher HUD */
    int alt_tab_selected;
    int hover_dock_idx;
    int hover_mx, hover_my;
    int hover_hit_action;
    int hover_hit_arg0;
    int hover_hit_arg1;

    /* Titlebar double-click tracking */
    int64_t last_title_click_ms;
    int last_title_click_win;

    /* Desktop Context Menu */
    int ctx_menu_open;
    int ctx_menu_x, ctx_menu_y;

    /* Interactive drag, resize, scrollbar & slider state */
    int drag_mode;          /* 0=none, 1=move win, 2=resize win, 3=scrollbar, 4=vol slider, 5=bright slider */
    int drag_win_id;
    int drag_start_mx, drag_start_my;
    int drag_orig_x, drag_orig_y, drag_orig_w, drag_orig_h;
    int snap_preview;       /* 0=none, 1=left, 2=right, 3=maximize */

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
    int active_text_target; /* 0=none, 1..9 */
    int input_focus_pulse;

    /* Background & Media surfaces */
    cairo_surface_t *wallpapers[MAX_WALLPAPERS];
    int custom_wp_media_idx; /* -1 when preset 0..7, >= 0 when using media_images[idx] */
    cairo_surface_t *cached_wp;
    int cached_wp_w, cached_wp_h, cached_wp_idx, cached_wp_dark, cached_wp_family, cached_wp_custom;
    cairo_surface_t *media_images[MAX_MEDIA_IMAGES];
    cairo_surface_t *avatar_surface;

    /* Notification toast + history */
    char toast_title[64];
    char toast_msg[128];
    int toast_timer;        /* Frames remaining to show */
    char notif_titles[4][48];
    char notif_msgs[4][96];
    char notif_times[4][16];
    int notif_count;

    /* Launcher Search */
    char launcher_query[48];

    /* Virtual Filesystem & Files App */
    VfsNode vfs[MAX_VFS];
    int vfs_count;
    int vfs_stock_count;
    char files_cwd[128];
    int files_selected_idx;
    int files_list_mode;
    int files_sort_mode;    /* 0=Name, 1=Size */
    int files_show_hidden;  /* 0=Hide .dotfiles, 1=Show .dotfiles */
    int files_scroll;
    int files_max_scroll;
    char files_query[48];
    int files_viewer_open;
    int files_viewer_idx;
    int files_renaming;
    char files_rename_buf[48];
    int files_confirm_delete_idx;
    int files_editing;
    char files_edit_buf[768];

    /* Terminal Emulator */
    char term_cwd[128];
    char term_prev_cwd[128];
    char term_lines[MAX_TERM_LINES][160];
    uint8_t term_line_colors[MAX_TERM_LINES];
    int term_line_count;
    char term_input[256];
    char term_history[24][256];
    int term_hist_count;
    int term_hist_idx;
    int term_cursor_visible;
    int term_scroll;        /* 0=bottom, >0 lines scrolled back */
    int term_max_scroll;
    char term_env_keys[8][24];
    char term_env_vals[8][64];
    int term_env_count;

    /* System Monitor & Real Linux Telemetry */
    int monitor_tab;
    int monitor_chart_mode; /* 0=CPU %, 1=RAM % */
    int monitor_sort;       /* 0=CPU, 1=Mem, 2=PID */
    double cpu_history[48];
    double ram_history[48];
    double core_usage[8];   /* Live per-core usage from /proc/stat */
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
    int swap_total_mb;
    int swap_used_mb;
    int cgroup_limited;
    double load_avg[3];
    double net_rx_kbps;
    double net_tx_kbps;
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

    /* Settings & Services */
    int settings_section;   /* 0..7 */
    int settings_update_checked;
    int settings_net_ping_ms; /* -1=untested, >=0 last check ms */
    int settings_scroll;
    int settings_max_scroll;
    char settings_hostname_draft[32];
    int settings_hostname_dirty;
    char settings_search[48];
    int settings_confirm_reset;
    int settings_confirm_factory;
    int svc_pipewire;
    int svc_network;
    int svc_oomd;
    int session_restored;
    char state_file_path[256];

    /* Ubuntu Store */
    AptPkg pkgs[MAX_APT_PKGS];
    int pkg_count;
    int store_cat;
    int store_installed_only;
    int store_selected;
    int store_scroll;
    int store_max_scroll;
    int store_sync_count;
    char store_query[48];

    /* Media Viewer, Video & Audio Player */
    int media_sidebar_open; /* 1=Left folder/library panel visible */
    int media_folder_idx;   /* 0=All Media, 1=Pictures, 2=Videos, 3=Audio, >=4=VFS folder */
    char media_vfs_dir[128];
    int media_folder_scroll;
    int media_folder_max_scroll;
    int media_mode;         /* 0=Photos, 1=Videos, 2=Audio */
    int media_grid_mode;    /* 0=Stage, 1=Gallery Grid */
    int media_slideshow;    /* 0=Off, 1=Auto-advance */
    int media_slideshow_ticks;
    int media_show_exif;    /* 0=Hidden, 1=EXIF HUD visible */
    int media_selected;
    int media_zoom_pct;
    int media_rotation;     /* 0, 90, 180, 270 */
    int media_filter;       /* 0=Natural, 1=Vivid, 2=Noir, 3=Warm */
    int media_grid_scroll;
    int media_grid_max_scroll;
    int video_selected;     /* 0..MAX_VIDEO_CLIPS-1 */
    int video_playing;
    int video_loop;
    int video_hud;
    int video_fit_fill;     /* 0=Contain 16:9, 1=Fill */
    int video_speed_x10;    /* 5, 10, 15, 20 */
    int video_progress_ms;
    int video_scroll;
    int video_max_scroll;
    int audio_selected;
    int audio_playing;
    int audio_loop;         /* 0=Queue next, 1=Loop current */
    int audio_shuffle;      /* 0=Sequential, 1=Shuffle */
    int audio_eq_preset;    /* 0=Natural, 1=Bass, 2=Studio, 3=Ambient */
    int audio_speed_x10;    /* 5, 10, 15, 20 */
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
void nature_get_accent_ex(int theme_family, int accent_idx, int dark_mode, double *r, double *g, double *b);
void nature_get_palette(const NatureOSState *st, NaturePalette *out);
void nature_os_set_wallpaper_preset(NatureOSState *st, int wp_idx);
void nature_os_set_wallpaper_from_media(NatureOSState *st, int media_idx);
void nature_os_reset_defaults(NatureOSState *st);
void nature_os_reset_all_session(NatureOSState *st);
void nature_os_save_state(const NatureOSState *st);
void nature_os_load_state(NatureOSState *st);
void nature_tile_windows(NatureOSState *st);
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
void nature_settings_key(NatureOSState *st, const char *key);

void nature_store_init(NatureOSState *st);
void nature_store_render(NatureOSState *st, cairo_t *cr, OSWindow *w, int cx, int cy, int cw, int ch);
void nature_store_click(NatureOSState *st, const HitRect *hit);
void nature_store_wheel(NatureOSState *st, int delta_y);
void nature_store_key(NatureOSState *st, const char *key);

void nature_media_render(NatureOSState *st, cairo_t *cr, OSWindow *w, int cx, int cy, int cw, int ch);
void nature_media_click(NatureOSState *st, const HitRect *hit);
void nature_media_wheel(NatureOSState *st, int delta_y);
void nature_media_tick(NatureOSState *st);
void nature_media_key(NatureOSState *st, const char *key);

void nature_browser_render(NatureOSState *st, cairo_t *cr, OSWindow *w, int cx, int cy, int cw, int ch);
void nature_browser_click(NatureOSState *st, const HitRect *hit);
void nature_browser_wheel(NatureOSState *st, int delta_y);
void nature_browser_key(NatureOSState *st, const char *key);

#ifdef __cplusplus
}
#endif

#endif
