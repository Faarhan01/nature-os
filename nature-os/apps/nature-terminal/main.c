#include "nature_os.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static const char *QUICK_CMDS[6] = {
    "neofetch",
    "ls -la",
    "htop",
    "free -m",
    "df -h",
    "clear"
};

static void push_term_line(NatureOSState *st, const char *txt, uint8_t col) {
    if (st->term_line_count >= MAX_TERM_LINES) {
        for (int i = 0; i < MAX_TERM_LINES - 1; ++i) {
            memcpy(st->term_lines[i], st->term_lines[i + 1], 128);
            st->term_line_colors[i] = st->term_line_colors[i + 1];
        }
        st->term_line_count = MAX_TERM_LINES - 1;
    }
    snprintf(st->term_lines[st->term_line_count], 128, "%.127s", txt);
    st->term_line_colors[st->term_line_count] = col;
    st->term_line_count++;
}

void nature_terminal_exec(NatureOSState *st, const char *raw) {
    st->term_scroll = 0;
    const char *host = st->cfg.device_name[0] ? st->cfg.device_name : "nature-pc";
    char prompt_line[180];
    snprintf(prompt_line, sizeof(prompt_line), "nature@%.31s:%s$ %s",
             host, strcmp(st->term_cwd, "/home/nature") == 0 ? "~" : st->term_cwd, raw);
    push_term_line(st, prompt_line, 1);

    char buf[128];
    snprintf(buf, sizeof(buf), "%s", raw);
    char *cmd = strtok(buf, " \t");
    if (!cmd) return;
    char *arg1 = strtok(NULL, " \t");
    char *arg2 = strtok(NULL, " \t");

    if (strcmp(cmd, "help") == 0) {
        push_term_line(st, "Ubuntu 24.04 / Nature OS Shell Commands:", 2);
        push_term_line(st, "  ls [-la] [dir]    cd <dir>         pwd              cat <file>", 0);
        push_term_line(st, "  cp <src> <dst>    mv <src> <dst>   rm <file>        grep <pat> <file>", 0);
        push_term_line(st, "  wc <file>         tree             history          neofetch / htop", 0);
        push_term_line(st, "  free -m           df -h            ip addr          ping [host]", 0);
        push_term_line(st, "  sensors           hostname [name]  theme [mode]     uname -a", 0);
        push_term_line(st, "  mkdir <name>      touch <file>     ps aux           systemctl", 0);
        push_term_line(st, "  apt list          apt install <p>  apt remove <p>   notify <msg>", 0);
        push_term_line(st, "  stress --cpu 4    stress --stop    reset-settings   clear", 0);
    } else if (strcmp(cmd, "reset-settings") == 0) {
        nature_os_reset_defaults(st);
        push_term_line(st, "Settings restored to Nature OS defaults and saved.", 2);
    } else if (strcmp(cmd, "clear") == 0) {
        st->term_line_count = 0;
        st->term_scroll = 0;
    } else if (strcmp(cmd, "pwd") == 0) {
        push_term_line(st, st->term_cwd, 0);
    } else if (strcmp(cmd, "whoami") == 0) {
        push_term_line(st, "nature (uid=1000, gid=1000, groups=adm,sudo,pipewire,video)", 0);
    } else if (strcmp(cmd, "date") == 0) {
        time_t now = time(NULL);
        char dbuf[64];
        strftime(dbuf, sizeof(dbuf), "%a %b %d %H:%M:%S UTC %Y", localtime(&now));
        push_term_line(st, dbuf, 0);
    } else if (strcmp(cmd, "uptime") == 0) {
        time_t now = time(NULL);
        char tbuf[16], uline[128];
        strftime(tbuf, sizeof(tbuf), "%H:%M:%S", localtime(&now));
        long hrs = st->uptime_sec / 3600;
        long mins = (st->uptime_sec % 3600) / 60;
        snprintf(uline, sizeof(uline), " %s up %ld:%02ld, 1 user, CPU load: %.1f%% (%d cores)",
                 tbuf, hrs, mins, st->total_cpu_pct, st->cpu_count > 0 ? st->cpu_count : 4);
        push_term_line(st, uline, 2);
    } else if (strcmp(cmd, "hostname") == 0) {
        if (arg1) {
            char clean[32];
            size_t j = 0;
            for (size_t i = 0; arg1[i] && j + 1 < sizeof(clean); ++i) {
                unsigned char c = (unsigned char)arg1[i];
                if (isalnum(c) || c == '-' || c == '_') clean[j++] = (char)c;
            }
            clean[j] = '\0';
            if (clean[0]) {
                snprintf(st->cfg.device_name, sizeof(st->cfg.device_name), "%s", clean);
                snprintf(st->settings_hostname_draft, sizeof(st->settings_hostname_draft), "%s", clean);
                st->settings_hostname_dirty = 0;
                char hmsg[80];
                snprintf(hmsg, sizeof(hmsg), "Hostname set to %.31s", st->cfg.device_name);
                push_term_line(st, hmsg, 2);
                nature_os_save_state(st);
            } else {
                push_term_line(st, "hostname: use letters, digits, '-' or '_'", 3);
            }
        } else {
            push_term_line(st, host, 0);
        }
    } else if (strcmp(cmd, "uname") == 0) {
        char kbuf[128];
        snprintf(kbuf, sizeof(kbuf), "Linux %.31s %.48s #1 SMP PREEMPT_DYNAMIC GNU/Linux", host, st->kernel_ver);
        push_term_line(st, kbuf, 0);
    } else if (strcmp(cmd, "lscpu") == 0) {
        char l1[128], l2[128], l3[128];
        snprintf(l1, sizeof(l1), "CPU(s):               %d online", st->cpu_count > 0 ? st->cpu_count : 4);
        snprintf(l2, sizeof(l2), "Model / Arch:         %s", st->cpu_model);
        snprintf(l3, sizeof(l3), "Live Utilization:     %.1f%% (C0:%.0f%% C1:%.0f%% C2:%.0f%% C3:%.0f%%)",
                 st->total_cpu_pct, st->core_usage[0], st->core_usage[1], st->core_usage[2], st->core_usage[3]);
        push_term_line(st, l1, 2);
        push_term_line(st, l2, 0);
        push_term_line(st, l3, 0);
    } else if (strcmp(cmd, "df") == 0) {
        long used_gb = st->disk_total_gb > st->disk_avail_gb ? (st->disk_total_gb - st->disk_avail_gb) : 0;
        char dline[128];
        push_term_line(st, "Filesystem      Size  Used Avail Use% Mounted on", 2);
        snprintf(dline, sizeof(dline), "/dev/vda1       %3ldG  %3ldG  %3ldG  %2d%% /",
                 st->disk_total_gb, used_gb, st->disk_avail_gb, st->disk_used_pct);
        push_term_line(st, dline, 0);
        snprintf(dline, sizeof(dline), "tmpfs          %4dM   12M %4dM   1%% /dev/shm",
                 st->total_ram_mb / 2, st->total_ram_mb / 2);
        push_term_line(st, dline, 0);
    } else if (strcmp(cmd, "ip") == 0) {
        push_term_line(st, "1: lo: <LOOPBACK,UP> mtu 65536 inet 127.0.0.1/8", 0);
        if (st->cfg.wifi_enabled) {
            push_term_line(st, "2: wlp2s0: <BROADCAST,MULTICAST,UP> inet 10.42.0.14/24 (Canopy-5GHz)", 2);
        } else {
            push_term_line(st, "2: wlp2s0: <DOWN> state DOWN (Wi-Fi radio disabled)", 3);
        }
    } else if (strcmp(cmd, "ping") == 0) {
        const char *target = arg1 ? arg1 : "1.1.1.1";
        if (!st->cfg.wifi_enabled) {
            push_term_line(st, "ping: connect: Network is unreachable (Wi-Fi disabled)", 3);
        } else {
            char p0[128], p1[128];
            snprintf(p0, sizeof(p0), "PING %.48s (1.1.1.1) 56(84) bytes of data.", target);
            snprintf(p1, sizeof(p1), "64 bytes from %.48s: icmp_seq=1 ttl=57 time=12.1 ms (0%% loss)", target);
            push_term_line(st, p0, 0);
            push_term_line(st, p1, 2);
            st->settings_net_ping_ms = 12;
        }
    } else if (strcmp(cmd, "theme") == 0) {
        if (!arg1) {
            char tbuf[96];
            snprintf(tbuf, sizeof(tbuf), "Active style: %s (%s mode)",
                     st->cfg.theme_family == THEME_HORIZON ? "horizon" : "nature",
                     st->cfg.dark_mode ? "dark" : "light");
            push_term_line(st, tbuf, 2);
        } else if (strcmp(arg1, "nature") == 0) {
            st->cfg.theme_family = THEME_NATURE;
            if (st->cfg.wallpaper_idx == 3 || st->cfg.wallpaper_idx == 4 || st->cfg.wallpaper_idx == 7) {
                nature_os_set_wallpaper_preset(st, 0);
            }
            push_term_line(st, "Switched theme family to Nature Canopy.", 2);
        } else if (strcmp(arg1, "horizon") == 0) {
            st->cfg.theme_family = THEME_HORIZON;
            nature_os_set_wallpaper_preset(st, st->cfg.dark_mode ? 4 : 3);
            push_term_line(st, "Switched theme family to Horizon Slate.", 2);
        } else if (strcmp(arg1, "dark") == 0) {
            st->cfg.dark_mode = 1;
            if (st->cfg.theme_family == THEME_HORIZON && st->cfg.wallpaper_idx == 3) {
                nature_os_set_wallpaper_preset(st, 4);
            }
            push_term_line(st, "Switched appearance to Midnight Dark.", 2);
        } else if (strcmp(arg1, "light") == 0) {
            st->cfg.dark_mode = 0;
            if (st->cfg.theme_family == THEME_HORIZON && st->cfg.wallpaper_idx == 4) {
                nature_os_set_wallpaper_preset(st, 3);
            }
            push_term_line(st, "Switched appearance to Daylight Light.", 2);
        } else {
            push_term_line(st, "Usage: theme [nature|horizon|dark|light]", 3);
        }
        if (arg1 && (strcmp(arg1, "nature") == 0 || strcmp(arg1, "horizon") == 0 ||
                     strcmp(arg1, "dark") == 0 || strcmp(arg1, "light") == 0)) {
            nature_os_save_state(st);
        }
    } else if (strcmp(cmd, "sensors") == 0) {
        push_term_line(st, "k10temp-pci-00c3 · Adapter: PCI adapter", 2);
        push_term_line(st, st->stress_active ? "  Tctl:        +68.4 C  (high = +90.0 C, 4-core load)" : "  Tctl:        +38.2 C  (idle)", st->stress_active ? 3 : 0);
        push_term_line(st, "nvme-pci-0100 · Composite: +34.5 C", 0);
    } else if (strcmp(cmd, "echo") == 0 && arg1) {
        push_term_line(st, raw + 5, 0);
    } else if (strcmp(cmd, "notify") == 0 && arg1) {
        nature_notify(st, "Terminal Message", arg1);
        push_term_line(st, "Notification broadcast to desktop shell.", 2);
    } else if (strcmp(cmd, "tree") == 0) {
        push_term_line(st, st->term_cwd, 2);
        for (int i = 0; i < st->vfs_count; ++i) {
            if (strcmp(st->vfs[i].parent, st->term_cwd) == 0) {
                char tree_l[128];
                snprintf(tree_l, sizeof(tree_l), "|-- %s%s", st->vfs[i].name, st->vfs[i].is_dir ? "/" : "");
                push_term_line(st, tree_l, st->vfs[i].is_dir ? 2 : 0);
            }
        }
    } else if (strcmp(cmd, "ls") == 0) {
        const char *target_dir = st->term_cwd;
        int detailed = 0;
        if (arg1 && arg1[0] == '-') detailed = 1;
        else if (arg1) target_dir = arg1;
        if (arg2 && arg2[0] != '-') target_dir = arg2;

        if (detailed) {
            push_term_line(st, "total 48", 0);
            for (int i = 0; i < st->vfs_count; ++i) {
                if (strcmp(st->vfs[i].parent, target_dir) == 0) {
                    char line[128];
                    snprintf(line, sizeof(line), "%crwxr-xr-x 1 nature nature %7d Nov 25 %s",
                             st->vfs[i].is_dir ? 'd' : '-', st->vfs[i].size_bytes, st->vfs[i].name);
                    push_term_line(st, line, st->vfs[i].is_dir ? 2 : 0);
                }
            }
        } else {
            char line[128] = {0};
            for (int i = 0; i < st->vfs_count; ++i) {
                if (strcmp(st->vfs[i].parent, target_dir) == 0) {
                    char item[52];
                    snprintf(item, sizeof(item), "%s%s  ", st->vfs[i].name, st->vfs[i].is_dir ? "/" : "");
                    if (strlen(line) + strlen(item) < 120) strcat(line, item);
                }
            }
            push_term_line(st, line[0] ? line : "(empty)", 0);
        }
    } else if (strcmp(cmd, "cd") == 0) {
        if (!arg1 || strcmp(arg1, "~") == 0) {
            snprintf(st->term_cwd, sizeof(st->term_cwd), "/home/nature");
        } else if (strcmp(arg1, "..") == 0) {
            char *slash = strrchr(st->term_cwd, '/');
            if (slash && slash != st->term_cwd) *slash = '\0';
            else snprintf(st->term_cwd, sizeof(st->term_cwd), "/");
        } else {
            char target[180];
            if (arg1[0] == '/') snprintf(target, sizeof(target), "%s", arg1);
            else snprintf(target, sizeof(target), "%s/%s",
                          strcmp(st->term_cwd, "/") == 0 ? "" : st->term_cwd, arg1);
            int found = 0;
            for (int i = 0; i < st->vfs_count; ++i) {
                if (st->vfs[i].is_dir && strcmp(st->vfs[i].path, target) == 0) { found = 1; break; }
            }
            if (found) snprintf(st->term_cwd, sizeof(st->term_cwd), "%.*s", (int)sizeof(st->term_cwd) - 1, target);
            else push_term_line(st, "bash: cd: No such directory", 3);
        }
    } else if (strcmp(cmd, "mkdir") == 0 && arg1) {
        if (st->vfs_count < MAX_VFS) {
            char full[180];
            snprintf(full, sizeof(full), "%s/%s", strcmp(st->term_cwd, "/") == 0 ? "" : st->term_cwd, arg1);
            VfsNode *n = &st->vfs[st->vfs_count++];
            memset(n, 0, sizeof(*n));
            snprintf(n->name, sizeof(n->name), "%s", arg1);
            snprintf(n->path, sizeof(n->path), "%.*s", (int)sizeof(n->path) - 1, full);
            snprintf(n->parent, sizeof(n->parent), "%s", st->term_cwd);
            snprintf(n->mime_tag, sizeof(n->mime_tag), "DIR");
            snprintf(n->modified, sizeof(n->modified), "Just now");
            n->is_dir = 1; n->size_bytes = 4096; n->image_idx = -1; n->audio_idx = -1;
            push_term_line(st, "Directory created.", 2);
            nature_os_save_state(st);
        }
    } else if (strcmp(cmd, "touch") == 0 && arg1) {
        if (st->vfs_count < MAX_VFS) {
            char full[180];
            snprintf(full, sizeof(full), "%s/%s", strcmp(st->term_cwd, "/") == 0 ? "" : st->term_cwd, arg1);
            VfsNode *n = &st->vfs[st->vfs_count++];
            memset(n, 0, sizeof(*n));
            snprintf(n->name, sizeof(n->name), "%s", arg1);
            snprintf(n->path, sizeof(n->path), "%.*s", (int)sizeof(n->path) - 1, full);
            snprintf(n->parent, sizeof(n->parent), "%s", st->term_cwd);
            snprintf(n->mime_tag, sizeof(n->mime_tag), "TXT");
            snprintf(n->modified, sizeof(n->modified), "Just now");
            snprintf(n->content, sizeof(n->content), "Created via terminal touch command.");
            n->is_dir = 0; n->size_bytes = 64; n->image_idx = -1; n->audio_idx = -1;
            push_term_line(st, "File created.", 2);
            nature_os_save_state(st);
        }
    } else if (strcmp(cmd, "cat") == 0 && arg1) {
        char target[180];
        if (arg1[0] == '/') snprintf(target, sizeof(target), "%s", arg1);
        else snprintf(target, sizeof(target), "%s/%s", strcmp(st->term_cwd, "/") == 0 ? "" : st->term_cwd, arg1);
        int found = 0;
        for (int i = 0; i < st->vfs_count; ++i) {
            if (!st->vfs[i].is_dir && strcmp(st->vfs[i].path, target) == 0) {
                found = 1;
                char tmp[768];
                snprintf(tmp, sizeof(tmp), "%s", st->vfs[i].content[0] ? st->vfs[i].content : "[Binary media stream data]");
                char *ln = strtok(tmp, "\n");
                while (ln) { push_term_line(st, ln, 0); ln = strtok(NULL, "\n"); }
                break;
            }
        }
        if (!found) push_term_line(st, "cat: No such file", 3);
    } else if (strcmp(cmd, "rm") == 0 && arg1) {
        char target[180];
        if (arg1[0] == '/') snprintf(target, sizeof(target), "%s", arg1);
        else snprintf(target, sizeof(target), "%s/%s", strcmp(st->term_cwd, "/") == 0 ? "" : st->term_cwd, arg1);
        int idx = -1;
        for (int i = 0; i < st->vfs_count; ++i) {
            if (strcmp(st->vfs[i].path, target) == 0) { idx = i; break; }
        }
        if (idx < 0) push_term_line(st, "rm: cannot remove: No such file", 3);
        else if (st->vfs[idx].is_dir) push_term_line(st, "rm: cannot remove directory (files only)", 3);
        else {
            for (int j = idx; j < st->vfs_count - 1; ++j) st->vfs[j] = st->vfs[j + 1];
            st->vfs_count--;
            if (st->files_selected_idx == idx) st->files_selected_idx = -1;
            push_term_line(st, "Removed file cleanly.", 2);
            nature_os_save_state(st);
        }
    } else if ((strcmp(cmd, "cp") == 0 || strcmp(cmd, "mv") == 0) && arg1 && arg2) {
        char src_p[180], dst_p[180];
        if (arg1[0] == '/') snprintf(src_p, sizeof(src_p), "%s", arg1);
        else snprintf(src_p, sizeof(src_p), "%s/%s", strcmp(st->term_cwd, "/") == 0 ? "" : st->term_cwd, arg1);
        if (arg2[0] == '/') snprintf(dst_p, sizeof(dst_p), "%s", arg2);
        else snprintf(dst_p, sizeof(dst_p), "%s/%s", strcmp(st->term_cwd, "/") == 0 ? "" : st->term_cwd, arg2);

        int sidx = -1;
        for (int i = 0; i < st->vfs_count; ++i) {
            if (strcmp(st->vfs[i].path, src_p) == 0) { sidx = i; break; }
        }
        if (sidx < 0 || st->vfs[sidx].is_dir) {
            push_term_line(st, "cp/mv: source file not found", 3);
        } else if (strcmp(cmd, "cp") == 0) {
            if (st->vfs_count >= MAX_VFS) {
                push_term_line(st, "cp: VFS node limit reached", 3);
            } else {
                VfsNode copy = st->vfs[sidx];
                const char *slash = strrchr(dst_p, '/');
                snprintf(copy.name, sizeof(copy.name), "%.47s", slash ? slash + 1 : dst_p);
                snprintf(copy.path, sizeof(copy.path), "%.127s", dst_p);
                if (slash && slash != dst_p) {
                    int plen = (int)(slash - dst_p);
                    if (plen > 127) plen = 127;
                    snprintf(copy.parent, sizeof(copy.parent), "%.*s", plen, dst_p);
                } else if (slash == dst_p) {
                    snprintf(copy.parent, sizeof(copy.parent), "/");
                }
                snprintf(copy.modified, sizeof(copy.modified), "Just now");
                st->vfs[st->vfs_count++] = copy;
                push_term_line(st, "File copied.", 2);
                nature_os_save_state(st);
            }
        } else {
            const char *slash = strrchr(dst_p, '/');
            snprintf(st->vfs[sidx].name, sizeof(st->vfs[sidx].name), "%.47s", slash ? slash + 1 : dst_p);
            snprintf(st->vfs[sidx].path, sizeof(st->vfs[sidx].path), "%.127s", dst_p);
            if (slash && slash != dst_p) {
                int plen = (int)(slash - dst_p);
                if (plen > 127) plen = 127;
                snprintf(st->vfs[sidx].parent, sizeof(st->vfs[sidx].parent), "%.*s", plen, dst_p);
            } else if (slash == dst_p) {
                snprintf(st->vfs[sidx].parent, sizeof(st->vfs[sidx].parent), "/");
            }
            snprintf(st->vfs[sidx].modified, sizeof(st->vfs[sidx].modified), "Just now");
            push_term_line(st, "File moved/renamed.", 2);
            nature_os_save_state(st);
        }
    } else if (strcmp(cmd, "grep") == 0 && arg1 && arg2) {
        char target[180];
        if (arg2[0] == '/') snprintf(target, sizeof(target), "%s", arg2);
        else snprintf(target, sizeof(target), "%s/%s", strcmp(st->term_cwd, "/") == 0 ? "" : st->term_cwd, arg2);
        int found = 0, hits = 0;
        size_t nlen = strlen(arg1);
        for (int i = 0; i < st->vfs_count; ++i) {
            if (!st->vfs[i].is_dir && strcmp(st->vfs[i].path, target) == 0) {
                found = 1;
                const char *p = st->vfs[i].content;
                int lno = 1;
                while (*p) {
                    const char *eol = strchr(p, '\n');
                    size_t len = eol ? (size_t)(eol - p) : strlen(p);
                    int matched = 0;
                    if (nlen <= len) {
                        for (size_t k = 0; k + nlen <= len; ++k) {
                            size_t m = 0;
                            while (m < nlen && tolower((unsigned char)p[k + m]) == tolower((unsigned char)arg1[m])) m++;
                            if (m == nlen) { matched = 1; break; }
                        }
                    }
                    if (matched) {
                        char gline[128];
                        size_t cpy = len < 106 ? len : 106;
                        snprintf(gline, sizeof(gline), "%3d: %.*s", lno, (int)cpy, p);
                        push_term_line(st, gline, 2);
                        hits++;
                    }
                    lno++;
                    if (!eol) break;
                    p = eol + 1;
                }
                break;
            }
        }
        if (!found) push_term_line(st, "grep: No such file", 3);
        else if (hits == 0) push_term_line(st, "(no matches)", 0);
    } else if (strcmp(cmd, "history") == 0) {
        if (st->term_hist_count == 0) {
            push_term_line(st, "(history empty)", 0);
        } else {
            for (int i = 0; i < st->term_hist_count; ++i) {
                char hline[140];
                snprintf(hline, sizeof(hline), " %2d  %.116s", i + 1, st->term_history[i]);
                push_term_line(st, hline, 0);
            }
        }
    } else if (strcmp(cmd, "wc") == 0 && arg1) {
        char target[180];
        if (arg1[0] == '/') snprintf(target, sizeof(target), "%s", arg1);
        else snprintf(target, sizeof(target), "%s/%s", strcmp(st->term_cwd, "/") == 0 ? "" : st->term_cwd, arg1);
        int found = 0;
        for (int i = 0; i < st->vfs_count; ++i) {
            if (!st->vfs[i].is_dir && strcmp(st->vfs[i].path, target) == 0) {
                found = 1;
                const char *s = st->vfs[i].content;
                int lines = s[0] ? 1 : 0, words = 0, in_word = 0, bytes = (int)strlen(s);
                for (int k = 0; s[k]; ++k) {
                    if (s[k] == '\n') lines++;
                    if (isspace((unsigned char)s[k])) in_word = 0;
                    else if (!in_word) { in_word = 1; words++; }
                }
                char wbuf[128];
                snprintf(wbuf, sizeof(wbuf), "  %4d lines  %5d words  %6d bytes  %.48s", lines, words, bytes, st->vfs[i].name);
                push_term_line(st, wbuf, 2);
                break;
            }
        }
        if (!found) push_term_line(st, "wc: No such file", 3);
    } else if (strcmp(cmd, "free") == 0) {
        char m[128];
        int free_mb = st->total_ram_mb - st->used_ram_mb - st->cached_ram_mb;
        if (free_mb < 0) free_mb = st->avail_ram_mb / 2;
        push_term_line(st, "               total        used        free   buff/cache   available", 0);
        snprintf(m, sizeof(m), "Mem:          %5d M     %5d M     %5d M      %5d M     %5d M",
                 st->total_ram_mb, st->used_ram_mb, free_mb, st->cached_ram_mb, st->avail_ram_mb);
        push_term_line(st, m, 2);
        char s[128];
        int zcap = st->cfg.zram_enabled ? (st->total_ram_mb / 2) : 0;
        snprintf(s, sizeof(s), "Swap (zram):  %5d M     %5d M     %5d M  [LZ4 + nature RSS %dM]",
                 zcap, st->cfg.zram_enabled ? st->zram_used_mb : 0,
                 zcap > st->zram_used_mb ? zcap - st->zram_used_mb : 0, st->os_rss_mb);
        push_term_line(st, s, 2);
    } else if (strcmp(cmd, "htop") == 0 || strcmp(cmd, "top") == 0) {
        push_term_line(st, "htop 3.3.0 - (Nature OS live /proc telemetry)", 2);
        char c0[128], c1[128];
        snprintf(c0, sizeof(c0), "  0 [%4.1f%%]  1 [%4.1f%%]   Mem: %dM/%dM · OS RSS: %dM",
                 st->core_usage[0], st->core_usage[1], st->used_ram_mb, st->total_ram_mb, st->os_rss_mb);
        snprintf(c1, sizeof(c1), "  2 [%4.1f%%]  3 [%4.1f%%]   Total CPU: %.1f%% · %.1fms frame",
                 st->core_usage[2], st->core_usage[3], st->total_cpu_pct, st->frame_ms);
        push_term_line(st, c0, 2);
        push_term_line(st, c1, 2);
        push_term_line(st, "  PID USER     PRI  NI   RES S CPU% MEM%  Command", 1);
        int lim = st->proc_count < 6 ? st->proc_count : 6;
        double denom = st->total_ram_mb > 0 ? (double)st->total_ram_mb : 4096.0;
        for (int i = 0; i < lim; ++i) {
            char pl[128];
            snprintf(pl, sizeof(pl), "%5d %-8s  20 %3d %4dM S %4.1f %4.1f  %s",
                     st->procs[i].pid, st->procs[i].user, st->procs[i].nice_val,
                     st->procs[i].ram_mb, st->procs[i].cpu_pct, (st->procs[i].ram_mb * 100.0) / denom,
                     st->procs[i].name);
            push_term_line(st, pl, 0);
        }
    } else if (strcmp(cmd, "neofetch") == 0) {
        char l0[128], l1[128], l2[128], l3[128], l4[128], l5[128];
        snprintf(l0, sizeof(l0), "        &&&            nature@%.31s", host);
        snprintf(l1, sizeof(l1), "      &&&&&&&          ----------------");
        snprintf(l2, sizeof(l2), "     &&&&&&&&&         OS: Nature OS 24.04.1 LTS (Ubuntu Noble)");
        snprintf(l3, sizeof(l3), "      |||||||          Kernel: %.48s", st->kernel_ver);
        snprintf(l4, sizeof(l4), "       |||||           Compositor: %dx%d @ %.0f FPS (%.1f ms)",
                 st->screen_w, st->screen_h, st->measured_fps, st->frame_ms);
        snprintf(l5, sizeof(l5), "        |||            Memory: %d MiB / %d MiB (OS RSS %d MiB)",
                 st->used_ram_mb, st->total_ram_mb, st->os_rss_mb);
        push_term_line(st, l0, 2);
        push_term_line(st, l1, 2);
        push_term_line(st, l2, 0);
        push_term_line(st, l3, 0);
        push_term_line(st, l4, 2);
        push_term_line(st, l5, 2);
    } else if (strcmp(cmd, "ps") == 0) {
        push_term_line(st, "  PID  USER     %CPU   RAM   NICE  COMMAND", 2);
        int lim = st->proc_count < 8 ? st->proc_count : 8;
        for (int i = 0; i < lim; ++i) {
            char pl[128];
            snprintf(pl, sizeof(pl), "%5d  %-7s %4.1f%%  %3dM  %+3d   %s",
                     st->procs[i].pid, st->procs[i].user, st->procs[i].cpu_pct,
                     st->procs[i].ram_mb, st->procs[i].nice_val, st->procs[i].name);
            push_term_line(st, pl, 0);
        }
    } else if (strcmp(cmd, "apt") == 0 && arg1) {
        if (strcmp(arg1, "list") == 0) {
            for (int i = 0; i < st->pkg_count; ++i) {
                char pl[128];
                snprintf(pl, sizeof(pl), "%-18s %-20s [%s]",
                         st->pkgs[i].name, st->pkgs[i].version,
                         st->pkgs[i].installed ? "installed" : "available");
                push_term_line(st, pl, st->pkgs[i].installed ? 2 : 0);
            }
        } else if (strcmp(arg1, "install") == 0 && arg2) {
            int ok = 0;
            for (int i = 0; i < st->pkg_count; ++i) {
                if (strcmp(st->pkgs[i].name, arg2) == 0) {
                    st->pkgs[i].installed = 1;
                    ok = 1;
                    char msg[128];
                    snprintf(msg, sizeof(msg), "Setting up %s (%s)... Done.", st->pkgs[i].name, st->pkgs[i].version);
                    push_term_line(st, msg, 2);
                    nature_notify(st, "Ubuntu APT", msg);
                    nature_os_save_state(st);
                    break;
                }
            }
            if (!ok) push_term_line(st, "E: Unable to locate package in noble-universe", 3);
        } else if (strcmp(arg1, "remove") == 0 && arg2) {
            for (int i = 0; i < st->pkg_count; ++i) {
                if (strcmp(st->pkgs[i].name, arg2) == 0) {
                    st->pkgs[i].installed = 0;
                    push_term_line(st, "Package removed via dpkg.", 0);
                    nature_os_save_state(st);
                    break;
                }
            }
        }
    } else if (strcmp(cmd, "systemctl") == 0) {
        push_term_line(st, "nature-session.service - active (running) · CPUWeight=300", 2);
        push_term_line(st, "  pipewire.service - active (running)", 0);
        push_term_line(st, "  NetworkManager.service - active (running)", 0);
        push_term_line(st, "  systemd-oomd.service - active (monitoring slices)", 0);
    } else if (strcmp(cmd, "stress") == 0) {
        if (arg1 && strcmp(arg1, "--stop") == 0) {
            nature_monitor_set_stress(st, 0);
            push_term_line(st, "stress: stopped background C++ worker threads.", 2);
        } else {
            nature_monitor_set_stress(st, 1);
            push_term_line(st, "stress: spawned worker threads (nice +10); compositor stays 60 FPS.", 2);
        }
    } else {
        push_term_line(st, "bash: command not found (type 'help' for options)", 3);
    }
}

void nature_terminal_key(NatureOSState *st, const char *key) {
    st->term_scroll = 0;
    if (strcmp(key, "Enter") == 0) {
        char cmd_copy[128];
        snprintf(cmd_copy, sizeof(cmd_copy), "%s", st->term_input);
        if (cmd_copy[0] && st->term_hist_count < 24) {
            snprintf(st->term_history[st->term_hist_count++], 128, "%s", cmd_copy);
        }
        st->term_hist_idx = -1;
        st->term_input[0] = '\0';
        nature_terminal_exec(st, cmd_copy);
    } else if (strcmp(key, "Backspace") == 0) {
        size_t len = strlen(st->term_input);
        if (len > 0) st->term_input[len - 1] = '\0';
    } else if (strcmp(key, "ArrowUp") == 0) {
        if (st->term_hist_count > 0) {
            if (st->term_hist_idx < 0) st->term_hist_idx = st->term_hist_count - 1;
            else if (st->term_hist_idx > 0) st->term_hist_idx--;
            snprintf(st->term_input, 128, "%s", st->term_history[st->term_hist_idx]);
        }
    } else if (strcmp(key, "ArrowDown") == 0) {
        if (st->term_hist_idx >= 0) {
            if (st->term_hist_idx < st->term_hist_count - 1) {
                st->term_hist_idx++;
                snprintf(st->term_input, 128, "%s", st->term_history[st->term_hist_idx]);
            } else {
                st->term_hist_idx = -1;
                st->term_input[0] = '\0';
            }
        }
    } else if (strcmp(key, "Tab") == 0) {
        const char *known[] = {"neofetch", "help", "clear", "htop", "free", "df", "ip", "ping", "theme", "reset-settings", "wc", "grep", "cp", "mv", "rm", "history", "sensors", "hostname", "uname", "tree", "ls", "cd", "cat", "stress"};
        for (size_t i = 0; i < sizeof(known)/sizeof(known[0]); ++i) {
            if (strncmp(known[i], st->term_input, strlen(st->term_input)) == 0) {
                snprintf(st->term_input, 128, "%s", known[i]);
                break;
            }
        }
    } else if (strlen(key) == 1 && (unsigned char)key[0] >= 32 && (unsigned char)key[0] < 127) {
        size_t len = strlen(st->term_input);
        if (len + 1 < sizeof(st->term_input)) {
            st->term_input[len] = key[0];
            st->term_input[len + 1] = '\0';
        }
    }
}

void nature_terminal_click(NatureOSState *st, const HitRect *hit) {
    if (hit->action_id == HIT_TERM_CHIP) {
        int idx = hit->arg0;
        if (idx >= 0 && idx < 6) {
            nature_terminal_exec(st, QUICK_CMDS[idx]);
        }
    } else if (hit->action_id == HIT_TERM_INPUT_AREA) {
        st->active_text_target = 1;
        st->input_focus_pulse = 1;
    }
}

void nature_terminal_wheel(NatureOSState *st, int delta_y) {
    if (delta_y < 0) {
        /* Scroll up into older lines */
        if (st->term_scroll < st->term_max_scroll) st->term_scroll += 2;
        if (st->term_scroll > st->term_max_scroll) st->term_scroll = st->term_max_scroll;
    } else if (delta_y > 0) {
        st->term_scroll -= 2;
        if (st->term_scroll < 0) st->term_scroll = 0;
    }
}

void nature_terminal_render(NatureOSState *st, cairo_t *cr, OSWindow *w, int cx, int cy, int cw, int ch) {
    (void)w;
    NaturePalette pal;
    nature_get_palette(st, &pal);
    int dark = pal.dark_mode;
    double ar = pal.ar, ag = pal.ag, ab = pal.ab;

    /* Terminal canvas adapts cleanly to Theme Family + Dark/Light Mode */
    double bg_r = dark ? (pal.theme_family == THEME_HORIZON ? 0.043 : 0.032) : pal.win_bg_r;
    double bg_g = dark ? (pal.theme_family == THEME_HORIZON ? 0.055 : 0.048) : pal.win_bg_g;
    double bg_b = dark ? (pal.theme_family == THEME_HORIZON ? 0.082 : 0.042) : pal.win_bg_b;
    ngfx_fill_rect(cr, cx, cy, cw, ch, 0, bg_r, bg_g, bg_b, 1.0);

    /* Interactive Command Chips Bar */
    ngfx_fill_rect(cr, cx, cy, cw, 34, 0, pal.sidebar_r, pal.sidebar_g, pal.sidebar_b, 1.0);
    g_ngfx.set_source_rgba(cr, pal.border_r, pal.border_g, pal.border_b, pal.border_a);
    g_ngfx.set_line_width(cr, 1.0);
    g_ngfx.move_to(cr, cx, cy + 34.5);
    g_ngfx.line_to(cr, cx + cw, cy + 34.5);
    g_ngfx.stroke(cr);

    int tx = cx + 10;
    for (int i = 0; i < 6; ++i) {
        int bw = (int)ngfx_text_width(cr, "JetBrains Mono", 0, 10.5, QUICK_CMDS[i]) + 16;
        if (tx + bw > cx + cw - 82) break;
        int hov = (st->hover_mx >= tx && st->hover_mx <= tx + bw && st->hover_my >= cy + 6 && st->hover_my <= cy + 28);
        ngfx_fill_rect(cr, tx, cy + 6, bw, 22, 6, hov ? ar : pal.elev_r, hov ? ag : pal.elev_g, hov ? ab : pal.elev_b, hov ? 0.28 : 0.92);
        ngfx_stroke_rect(cr, tx + 0.5, cy + 6.5, bw - 1, 21, 6, 1.0, ar, ag, ab, hov ? 0.85 : 0.35);
        ngfx_text(cr, "JetBrains Mono", hov, 10.5, tx + 8, cy + 21, pal.fg_r, pal.fg_g, pal.fg_b, 0.96, QUICK_CMDS[i]);
        nature_add_hit(st, tx, cy + 6, bw, 22, HIT_TERM_CHIP, i, 0);
        tx += bw + 6;
    }

    if (cw >= 340) {
        char badge[24];
        if (st->term_scroll > 0) snprintf(badge, sizeof(badge), "+%d up", st->term_scroll);
        else snprintf(badge, sizeof(badge), "%s", st->cfg.theme_family == THEME_HORIZON ? "Horizon" : "Canopy");
        int bw_r = 66;
        int bx_r = cx + cw - bw_r - 10;
        ngfx_fill_rect(cr, bx_r, cy + 6, bw_r, 22, 6,
                       st->term_scroll > 0 ? ar : pal.elev_r,
                       st->term_scroll > 0 ? ag : pal.elev_g,
                       st->term_scroll > 0 ? ab : pal.elev_b,
                       st->term_scroll > 0 ? 0.24 : 0.72);
        ngfx_text_clipped(cr, "JetBrains Mono", 1, 9.5, bx_r + 8, cy + 20.5, bw_r - 12,
                          st->term_scroll > 0 ? (dark ? pal.ar_hi : ar) : pal.mut_r,
                          st->term_scroll > 0 ? (dark ? pal.ag_hi : ag) : pal.mut_g,
                          st->term_scroll > 0 ? (dark ? pal.ab_hi : ab) : pal.mut_b, 0.95, badge);
    }

    int body_y = cy + 36;
    int body_h = ch - 76;
    int max_visible = body_h / 18;
    if (max_visible < 1) max_visible = 1;

    int max_scroll = st->term_line_count - max_visible;
    if (max_scroll < 0) max_scroll = 0;
    st->term_max_scroll = max_scroll;
    if (st->term_scroll > max_scroll) st->term_scroll = max_scroll;
    if (st->term_scroll < 0) st->term_scroll = 0;

    int end_idx = st->term_line_count - st->term_scroll;
    int start = end_idx > max_visible ? end_idx - max_visible : 0;

    int y = cy + 52;
    for (int i = start; i < end_idx; ++i) {
        uint8_t col = st->term_line_colors[i];
        double r = pal.fg_sec_r, g = pal.fg_sec_g, b = pal.fg_sec_b;
        if (col == 1) { r = pal.fg_r; g = pal.fg_g; b = pal.fg_b; }
        else if (col == 2) { r = dark ? pal.ar_hi : ar; g = dark ? pal.ag_hi : ag; b = dark ? pal.ab_hi : ab; }
        else if (col == 3) { r = pal.danger_r; g = pal.danger_g; b = pal.danger_b; }
        ngfx_text_clipped(cr, "JetBrains Mono", col == 1, 11.5, cx + 14, y, cw - 36, r, g, b, 1.0, st->term_lines[i]);
        y += 18;
    }

    /* Register touch input hit area for prompt & terminal body (minus scrollbar track) */
    nature_add_hit(st, cx, body_y, cw - 20, ch - 36, HIT_TERM_INPUT_AREA, 0, 0);
    nature_add_text_rect(st, cx, body_y, cw - 20, ch - 36, 1);

    nature_draw_scrollbar(st, cr, cx + cw - 11, body_y, body_h,
                          st->term_line_count, max_visible, st->term_scroll, max_scroll, SCROLL_TERMINAL, dark);

    int term_focused = (st->active_text_target == 1);
    int py_bar = cy + ch - 34;
    ngfx_fill_rect(cr, cx + 8, py_bar, cw - 16, 28, 8, pal.card_r, pal.card_g, pal.card_b, 0.96);
    ngfx_stroke_rect(cr, cx + 8.5, py_bar + 0.5, cw - 17, 27, 8, term_focused ? 1.2 : 1.0,
                     term_focused ? ar : pal.border_r, term_focused ? ag : pal.border_g, term_focused ? ab : pal.border_b,
                     term_focused ? 0.80 : pal.border_a);

    int py = py_bar + 18;
    const char *host = st->cfg.device_name[0] ? st->cfg.device_name : "nature-pc";
    char prompt_prefix[180];
    snprintf(prompt_prefix, sizeof(prompt_prefix), "nature@%.31s:%s$ ",
             host, strcmp(st->term_cwd, "/home/nature") == 0 ? "~" : st->term_cwd);
    double pw = ngfx_text_width(cr, "JetBrains Mono", 1, 11.5, prompt_prefix);
    ngfx_text(cr, "JetBrains Mono", 1, 11.5, cx + 16, py, dark ? pal.ar_hi : ar, dark ? pal.ag_hi : ag, dark ? pal.ab_hi : ab, 1.0, prompt_prefix);
    ngfx_text_clipped(cr, "JetBrains Mono", 0, 11.5, cx + 16 + pw, py, cw - pw - 38, pal.fg_r, pal.fg_g, pal.fg_b, 1.0, st->term_input);

    double iw = ngfx_text_width(cr, "JetBrains Mono", 0, 11.5, st->term_input);
    if (17 + pw + iw < cw - 24) {
        ngfx_fill_rect(cr, cx + 17 + pw + iw, py - 11, 7, 14, 1.5, dark ? pal.ar_hi : ar, dark ? pal.ag_hi : ag, dark ? pal.ab_hi : ab, 0.92);
    }
}
