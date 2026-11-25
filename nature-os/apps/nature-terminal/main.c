#include "nature_os.h"
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
    char prompt_line[180];
    snprintf(prompt_line, sizeof(prompt_line), "nature@nature-pc:%s$ %s",
             strcmp(st->term_cwd, "/home/nature") == 0 ? "~" : st->term_cwd, raw);
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
        push_term_line(st, "  tree              neofetch         htop / lscpu     free -m", 0);
        push_term_line(st, "  df -h             ip addr          sensors          touch <file>", 0);
        push_term_line(st, "  mkdir <name>      ps aux           uname -a         whoami", 0);
        push_term_line(st, "  apt list          apt install <p>  apt remove <p>   systemctl", 0);
        push_term_line(st, "  stress --cpu 4    stress --stop    notify <msg>     clear", 0);
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
    } else if (strcmp(cmd, "uname") == 0) {
        char kbuf[128];
        snprintf(kbuf, sizeof(kbuf), "Linux nature-pc %s #1 SMP PREEMPT_DYNAMIC GNU/Linux", st->kernel_ver);
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
        snprintf(l0, sizeof(l0), "        &&&            nature@nature-pc");
        snprintf(l1, sizeof(l1), "      &&&&&&&          ----------------");
        snprintf(l2, sizeof(l2), "     &&&&&&&&&         OS: Nature OS 24.04.1 LTS (Ubuntu Noble)");
        snprintf(l3, sizeof(l3), "      |||||||          Kernel: %s", st->kernel_ver);
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
                    break;
                }
            }
            if (!ok) push_term_line(st, "E: Unable to locate package in noble-universe", 3);
        } else if (strcmp(arg1, "remove") == 0 && arg2) {
            for (int i = 0; i < st->pkg_count; ++i) {
                if (strcmp(st->pkgs[i].name, arg2) == 0) {
                    st->pkgs[i].installed = 0;
                    push_term_line(st, "Package removed via dpkg.", 0);
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
        const char *known[] = {"neofetch", "help", "clear", "htop", "free", "df", "ip", "sensors", "uname", "tree", "ls", "cd", "cat", "stress"};
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
    double ar, ag, ab;
    nature_get_accent(st->cfg.accent_idx, &ar, &ag, &ab);
    ngfx_fill_rect(cr, cx, cy, cw, ch, 0, 0.032, 0.048, 0.042, 1.0);

    /* Interactive Command Chips Bar */
    ngfx_fill_rect(cr, cx, cy, cw, 34, 0, 0.044, 0.066, 0.058, 1.0);
    g_ngfx.set_source_rgba(cr, 1, 1, 1, 0.07);
    g_ngfx.set_line_width(cr, 1.0);
    g_ngfx.move_to(cr, cx, cy + 34.5);
    g_ngfx.line_to(cr, cx + cw, cy + 34.5);
    g_ngfx.stroke(cr);

    int tx = cx + 10;
    for (int i = 0; i < 6; ++i) {
        int bw = (int)ngfx_text_width(cr, "JetBrains Mono", 0, 10.5, QUICK_CMDS[i]) + 16;
        if (tx + bw > cx + cw - 10) break;
        int hov = (st->hover_mx >= tx && st->hover_mx <= tx + bw && st->hover_my >= cy + 6 && st->hover_my <= cy + 28);
        ngfx_fill_rect(cr, tx, cy + 6, bw, 22, 6, hov ? ar : 1.0, hov ? ag : 1.0, hov ? ab : 1.0, hov ? 0.22 : 0.07);
        ngfx_stroke_rect(cr, tx + 0.5, cy + 6.5, bw - 1, 21, 6, 1.0, ar, ag, ab, hov ? 0.85 : 0.35);
        ngfx_text(cr, "JetBrains Mono", hov, 10.5, tx + 8, cy + 21, 0.88, 0.96, 0.92, 1.0, QUICK_CMDS[i]);
        nature_add_hit(st, tx, cy + 6, bw, 22, HIT_TERM_CHIP, i, 0);
        tx += bw + 6;
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
        double r = 0.76, g = 0.86, b = 0.82;
        if (col == 1) { r = 0.97; g = 0.99; b = 0.98; }
        else if (col == 2) { r = 0.36; g = 0.92; b = 0.62; }
        else if (col == 3) { r = 0.96; g = 0.44; b = 0.46; }
        ngfx_text_clipped(cr, "JetBrains Mono", col == 1, 11.5, cx + 14, y, cw - 36, r, g, b, 1.0, st->term_lines[i]);
        y += 18;
    }

    /* Register touch input hit area for prompt & terminal body (minus scrollbar track) */
    nature_add_hit(st, cx, body_y, cw - 20, ch - 36, HIT_TERM_INPUT_AREA, 0, 0);
    nature_add_text_rect(st, cx, body_y, cw - 20, ch - 36, 1);

    nature_draw_scrollbar(st, cr, cx + cw - 11, body_y, body_h,
                          st->term_line_count, max_visible, st->term_scroll, max_scroll, SCROLL_TERMINAL, 1);

    int term_focused = (st->active_text_target == 1);
    int py_bar = cy + ch - 34;
    ngfx_fill_rect(cr, cx + 8, py_bar, cw - 16, 28, 8, 0.022, 0.036, 0.031, 0.95);
    ngfx_stroke_rect(cr, cx + 8.5, py_bar + 0.5, cw - 17, 27, 8, term_focused ? 1.2 : 1.0,
                     term_focused ? ar : 1.0, term_focused ? ag : 1.0, term_focused ? ab : 1.0,
                     term_focused ? 0.75 : 0.10);

    int py = py_bar + 18;
    char prompt_prefix[160];
    snprintf(prompt_prefix, sizeof(prompt_prefix), "nature@nature-pc:%s$ ",
             strcmp(st->term_cwd, "/home/nature") == 0 ? "~" : st->term_cwd);
    double pw = ngfx_text_width(cr, "JetBrains Mono", 1, 11.5, prompt_prefix);
    ngfx_text(cr, "JetBrains Mono", 1, 11.5, cx + 16, py, 0.36, 0.92, 0.62, 1.0, prompt_prefix);
    ngfx_text_clipped(cr, "JetBrains Mono", 0, 11.5, cx + 16 + pw, py, cw - pw - 38, 0.96, 0.98, 0.97, 1.0, st->term_input);

    double iw = ngfx_text_width(cr, "JetBrains Mono", 0, 11.5, st->term_input);
    if (17 + pw + iw < cw - 24) {
        ngfx_fill_rect(cr, cx + 17 + pw + iw, py - 11, 7, 14, 1.5, 0.36, 0.92, 0.62, 0.92);
    }
}
