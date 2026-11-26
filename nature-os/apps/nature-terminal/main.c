#include "nature_os.h"
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>

static const char *QUICK_CMDS[6] = {
    "neofetch",
    "ls -la",
    "htop",
    "node -v",
    "free -m",
    "clear"
};

static const char *KNOWN_CMDS[] = {
    "neofetch", "help", "clear", "htop", "top", "free", "df", "ip", "ping",
    "theme", "reset-settings", "wc", "grep", "cp", "mv", "rm", "rmdir",
    "history", "sensors", "hostname", "uname", "tree", "ls", "cd", "cat",
    "head", "tail", "find", "sort", "uniq", "stat", "which", "env", "id",
    "export", "unset", "cal", "calc", "open", "echo", "mkdir", "touch", "ps",
    "apt", "systemctl", "whoami", "date", "uptime", "lscpu", "nproc", "man",
    "pwd", "notify", "stress", "node", "npm", "npx", "python3", "git",
    "curl", "wget", "nano"
};

static void push_term_line_raw(NatureOSState *st, const char *txt, uint8_t col) {
    if (st->term_line_count >= MAX_TERM_LINES) {
        for (int i = 0; i < MAX_TERM_LINES - 1; ++i) {
            memcpy(st->term_lines[i], st->term_lines[i + 1], sizeof(st->term_lines[0]));
            st->term_line_colors[i] = st->term_line_colors[i + 1];
        }
        st->term_line_count = MAX_TERM_LINES - 1;
    }
    snprintf(st->term_lines[st->term_line_count], sizeof(st->term_lines[0]), "%.*s",
             (int)sizeof(st->term_lines[0]) - 1, txt ? txt : "");
    st->term_line_colors[st->term_line_count] = col;
    st->term_line_count++;
}

static void push_term_line(NatureOSState *st, const char *txt, uint8_t col) {
    if (!txt) {
        push_term_line_raw(st, "", col);
        return;
    }
    size_t len = strlen(txt);
    const size_t wrap_col = 98;
    if (len <= wrap_col) {
        push_term_line_raw(st, txt, col);
        return;
    }
    size_t off = 0;
    while (off < len) {
        size_t rem = len - off;
        if (rem <= wrap_col) {
            push_term_line_raw(st, txt + off, col);
            break;
        }
        size_t take = wrap_col;
        for (size_t k = wrap_col; k > wrap_col / 2; --k) {
            if (txt[off + k] == ' ') {
                take = k;
                break;
            }
        }
        char chunk[160];
        if (take >= sizeof(chunk)) take = sizeof(chunk) - 1;
        memcpy(chunk, txt + off, take);
        chunk[take] = '\0';
        push_term_line_raw(st, chunk, col);
        off += take;
        while (txt[off] == ' ') off++;
    }
}

static char *trim_inplace(char *s) {
    while (*s && isspace((unsigned char)*s)) s++;
    size_t len = strlen(s);
    while (len > 0 && isspace((unsigned char)s[len - 1])) s[--len] = '\0';
    return s;
}

static int str_ci_has(const char *hay, const char *needle) {
    if (!needle || !*needle) return 1;
    if (!hay) return 0;
    size_t hlen = strlen(hay), nlen = strlen(needle);
    if (nlen > hlen) return 0;
    for (size_t i = 0; i <= hlen - nlen; ++i) {
        size_t j = 0;
        while (j < nlen && tolower((unsigned char)hay[i + j]) == tolower((unsigned char)needle[j])) j++;
        if (j == nlen) return 1;
    }
    return 0;
}

static void resolve_term_path(const NatureOSState *st, const char *arg, char *out, size_t cap) {
    char raw[256];
    if (!arg || !*arg || strcmp(arg, "~") == 0) {
        snprintf(raw, sizeof(raw), "/home/nature");
    } else if (strncmp(arg, "~/", 2) == 0) {
        snprintf(raw, sizeof(raw), "/home/nature/%s", arg + 2);
    } else if (arg[0] == '/') {
        snprintf(raw, sizeof(raw), "%s", arg);
    } else {
        const char *base = (st->term_cwd[0]) ? st->term_cwd : "/home/nature";
        snprintf(raw, sizeof(raw), "%s/%s", strcmp(base, "/") == 0 ? "" : base, arg);
    }

    char segs[16][48];
    int nseg = 0;
    char *p = raw;
    while (*p) {
        while (*p == '/') p++;
        if (!*p) break;
        char *sl = strchr(p, '/');
        size_t len = sl ? (size_t)(sl - p) : strlen(p);
        if (len == 1 && p[0] == '.') {
            /* current dir */
        } else if (len == 2 && p[0] == '.' && p[1] == '.') {
            if (nseg > 0) nseg--;
        } else if (nseg < 16) {
            size_t cpy = len < 47 ? len : 47;
            memcpy(segs[nseg], p, cpy);
            segs[nseg][cpy] = '\0';
            nseg++;
        }
        if (!sl) break;
        p = sl + 1;
    }

    if (nseg == 0) {
        snprintf(out, cap, "/");
        return;
    }
    size_t pos = 0;
    out[0] = '\0';
    for (int i = 0; i < nseg; ++i) {
        int n = snprintf(out + pos, cap - pos, "/%s", segs[i]);
        if (n < 0 || pos + (size_t)n >= cap) break;
        pos += (size_t)n;
    }
}

static int find_vfs_by_path(const NatureOSState *st, const char *path) {
    for (int i = 0; i < st->vfs_count; ++i) {
        if (strcmp(st->vfs[i].path, path) == 0) return i;
    }
    return -1;
}

static void split_parent_name(const char *full, char *parent_out, size_t pcap, char *name_out, size_t ncap) {
    const char *sl = strrchr(full, '/');
    const char *base = sl ? sl + 1 : full;
    if (ncap > 0) snprintf(name_out, ncap, "%.*s", (int)(ncap - 1), base);
    if (sl && sl != full) {
        int plen = (int)(sl - full);
        if ((size_t)plen >= pcap) plen = (int)pcap - 1;
        snprintf(parent_out, pcap, "%.*s", plen, full);
    } else {
        snprintf(parent_out, pcap, "/");
    }
}

static const char *lookup_env_var(const NatureOSState *st, const char *name, char *scratch, size_t scap) {
    const char *host = st->cfg.device_name[0] ? st->cfg.device_name : "nature-pc";
    if (strcmp(name, "HOME") == 0) return "/home/nature";
    if (strcmp(name, "USER") == 0) return "nature";
    if (strcmp(name, "PWD") == 0) return st->term_cwd[0] ? st->term_cwd : "/home/nature";
    if (strcmp(name, "OLDPWD") == 0) return st->term_prev_cwd[0] ? st->term_prev_cwd : "/home/nature";
    if (strcmp(name, "HOSTNAME") == 0) return host;
    if (strcmp(name, "SHELL") == 0) return "/bin/bash";
    if (strcmp(name, "TERM") == 0) return "xterm-256color";
    if (strcmp(name, "NODE_VERSION") == 0) return "v22.12.0";
    if (strcmp(name, "PYTHON_VERSION") == 0) return "3.12.3";
    if (strcmp(name, "?") == 0) return "0";
    if (strcmp(name, "NPROC") == 0) {
        snprintf(scratch, scap, "%d", st->cpu_count > 0 ? st->cpu_count : 4);
        return scratch;
    }
    for (int i = 0; i < st->term_env_count; ++i) {
        if (strcmp(st->term_env_keys[i], name) == 0) return st->term_env_vals[i];
    }
    return "";
}

static void expand_env_vars(const NatureOSState *st, const char *in, char *out, size_t cap, int respect_single_quotes) {
    size_t pos = 0;
    char q = 0;
    for (size_t i = 0; in[i] && pos + 1 < cap; ++i) {
        if (respect_single_quotes && (in[i] == '\'' || in[i] == '"') && (i == 0 || in[i - 1] != '\\')) {
            if (q == 0) q = in[i];
            else if (q == in[i]) q = 0;
            out[pos++] = in[i];
            continue;
        }
        if (in[i] == '$' && (!respect_single_quotes || q != '\'')) {
            if (in[i + 1] == '?') {
                const char *v = "0";
                size_t vl = strlen(v);
                if (pos + vl < cap) { memcpy(out + pos, v, vl); pos += vl; }
                i += 1;
                continue;
            }
            if (in[i + 1] == '{') {
                const char *end = strchr(in + i + 2, '}');
                if (end) {
                    char key[32];
                    size_t kl = (size_t)(end - (in + i + 2));
                    if (kl >= sizeof(key)) kl = sizeof(key) - 1;
                    memcpy(key, in + i + 2, kl);
                    key[kl] = '\0';
                    char sc[32];
                    const char *v = lookup_env_var(st, key, sc, sizeof(sc));
                    size_t vl = strlen(v);
                    if (pos + vl >= cap) vl = cap - 1 - pos;
                    memcpy(out + pos, v, vl);
                    pos += vl;
                    i = (size_t)(end - in);
                    continue;
                }
            }
            if (isalpha((unsigned char)in[i + 1]) || in[i + 1] == '_') {
                size_t k = i + 1;
                while (isalnum((unsigned char)in[k]) || in[k] == '_') k++;
                char key[32];
                size_t kl = k - (i + 1);
                if (kl >= sizeof(key)) kl = sizeof(key) - 1;
                memcpy(key, in + i + 1, kl);
                key[kl] = '\0';
                char sc[32];
                const char *v = lookup_env_var(st, key, sc, sizeof(sc));
                size_t vl = strlen(v);
                if (pos + vl >= cap) vl = cap - 1 - pos;
                memcpy(out + pos, v, vl);
                pos += vl;
                i = k - 1;
                continue;
            }
        }
        out[pos++] = in[i];
    }
    out[pos] = '\0';
}

static void infer_mime_tag(const char *name, char *out, size_t cap) {
    const char *dot = strrchr(name, '.');
    if (!dot) { snprintf(out, cap, "TXT"); return; }
    if (strcasecmp(dot, ".js") == 0 || strcasecmp(dot, ".mjs") == 0) snprintf(out, cap, "JS");
    else if (strcasecmp(dot, ".json") == 0) snprintf(out, cap, "JSON");
    else if (strcasecmp(dot, ".py") == 0) snprintf(out, cap, "PY");
    else if (strcasecmp(dot, ".md") == 0) snprintf(out, cap, "MD");
    else if (strcasecmp(dot, ".sh") == 0) snprintf(out, cap, "SH");
    else if (strcasecmp(dot, ".png") == 0 || strcasecmp(dot, ".jpg") == 0) snprintf(out, cap, "IMG");
    else if (strcasecmp(dot, ".mp4") == 0 || strcasecmp(dot, ".webm") == 0) snprintf(out, cap, "VID");
    else if (strcasecmp(dot, ".mp3") == 0 || strcasecmp(dot, ".wav") == 0) snprintf(out, cap, "AUD");
    else snprintf(out, cap, "TXT");
}

static int upsert_vfs_file(NatureOSState *st, const char *path, const char *text, int append) {
    int idx = find_vfs_by_path(st, path);
    if (idx >= 0 && st->vfs[idx].is_dir) return -2;
    if (idx >= 0) {
        if (append && st->vfs[idx].content[0]) {
            size_t cur = strlen(st->vfs[idx].content);
            if (cur + 2 < sizeof(st->vfs[idx].content)) {
                snprintf(st->vfs[idx].content + cur, sizeof(st->vfs[idx].content) - cur, "\n%s", text);
            }
        } else {
            snprintf(st->vfs[idx].content, sizeof(st->vfs[idx].content), "%s", text);
        }
        st->vfs[idx].size_bytes = (int)strlen(st->vfs[idx].content);
        snprintf(st->vfs[idx].modified, sizeof(st->vfs[idx].modified), "Just now");
        nature_os_save_state(st);
        return idx;
    }
    if (st->vfs_count >= MAX_VFS) return -1;
    VfsNode *n = &st->vfs[st->vfs_count++];
    memset(n, 0, sizeof(*n));
    split_parent_name(path, n->parent, sizeof(n->parent), n->name, sizeof(n->name));
    snprintf(n->path, sizeof(n->path), "%.127s", path);
    infer_mime_tag(n->name, n->mime_tag, sizeof(n->mime_tag));
    snprintf(n->modified, sizeof(n->modified), "Just now");
    snprintf(n->content, sizeof(n->content), "%s", text);
    n->is_dir = 0;
    n->size_bytes = (int)strlen(n->content);
    n->image_idx = -1;
    n->audio_idx = -1;
    n->video_idx = -1;
    nature_os_save_state(st);
    return st->vfs_count - 1;
}

static int mkdir_recursive(NatureOSState *st, const char *target, int parents) {
    if (strcmp(target, "/") == 0) return 0;
    int existing = find_vfs_by_path(st, target);
    if (existing >= 0) return st->vfs[existing].is_dir ? (parents ? 0 : -2) : -2;

    char parent[128], name[48];
    split_parent_name(target, parent, sizeof(parent), name, sizeof(name));
    if (strcmp(parent, "/") != 0 && find_vfs_by_path(st, parent) < 0) {
        if (!parents) return -3;
        int rc = mkdir_recursive(st, parent, 1);
        if (rc != 0) return rc;
    }
    if (st->vfs_count >= MAX_VFS) return -1;
    VfsNode *n = &st->vfs[st->vfs_count++];
    memset(n, 0, sizeof(*n));
    snprintf(n->parent, sizeof(n->parent), "%s", parent);
    snprintf(n->name, sizeof(n->name), "%s", name);
    snprintf(n->path, sizeof(n->path), "%.127s", target);
    snprintf(n->mime_tag, sizeof(n->mime_tag), "DIR");
    snprintf(n->modified, sizeof(n->modified), "Just now");
    n->is_dir = 1;
    n->size_bytes = 4096;
    n->image_idx = -1;
    n->audio_idx = -1;
    n->video_idx = -1;
    return 0;
}

static int tokenize_cmd(char *buf, char *argv[], int max_args) {
    int argc = 0;
    char *p = buf;
    while (*p && argc < max_args) {
        while (*p && isspace((unsigned char)*p)) p++;
        if (!*p) break;
        char q = 0;
        if (*p == '"' || *p == '\'') {
            q = *p++;
            argv[argc++] = p;
            while (*p && *p != q) p++;
            if (*p == q) *p++ = '\0';
        } else {
            argv[argc++] = p;
            while (*p && !isspace((unsigned char)*p)) p++;
            if (*p) *p++ = '\0';
        }
    }
    return argc;
}

static const char *find_unquoted_redir(const char *s, int *is_append) {
    char q = 0;
    for (const char *p = s; *p; ++p) {
        if ((*p == '"' || *p == '\'') && (p == s || p[-1] != '\\')) {
            if (q == 0) q = *p;
            else if (q == *p) q = 0;
        } else if (q == 0 && *p == '>') {
            *is_append = (p[1] == '>');
            return p;
        }
    }
    *is_append = 0;
    return NULL;
}

static void push_multiline_text(NatureOSState *st, const char *txt, int max_lines, int from_tail) {
    const char *lines_ptr[64];
    size_t lines_len[64];
    int total = 0;
    const char *p = txt;
    while (total < 64) {
        const char *eol = strchr(p, '\n');
        lines_ptr[total] = p;
        lines_len[total] = eol ? (size_t)(eol - p) : strlen(p);
        total++;
        if (!eol) break;
        p = eol + 1;
    }
    int start = 0;
    int end = total;
    if (max_lines > 0 && total > max_lines) {
        if (from_tail) start = total - max_lines;
        else end = max_lines;
    }
    for (int i = start; i < end; ++i) {
        char line[256];
        size_t cpy = lines_len[i] < sizeof(line) - 1 ? lines_len[i] : sizeof(line) - 1;
        memcpy(line, lines_ptr[i], cpy);
        line[cpy] = '\0';
        push_term_line(st, line, 0);
    }
}

/* ==================== Lightweight Node.js & Python Evaluators ==================== */

typedef struct {
    char name[32];
    int is_num;
    double num;
    char str[160];
} ScriptVar;

typedef struct {
    ScriptVar vars[24];
    int count;
    int server_started;
    int server_port;
} ScriptCtx;

static ScriptVar *script_find_var(ScriptCtx *ctx, const char *name) {
    for (int i = 0; i < ctx->count; ++i) {
        if (strcmp(ctx->vars[i].name, name) == 0) return &ctx->vars[i];
    }
    return NULL;
}

static void script_set_num(ScriptCtx *ctx, const char *name, double v) {
    ScriptVar *sv = script_find_var(ctx, name);
    if (!sv && ctx->count < 24) sv = &ctx->vars[ctx->count++];
    if (!sv) return;
    snprintf(sv->name, sizeof(sv->name), "%.*s", (int)sizeof(sv->name) - 1, name);
    sv->is_num = 1;
    sv->num = v;
    if (v == (double)(long long)v) snprintf(sv->str, sizeof(sv->str), "%lld", (long long)v);
    else snprintf(sv->str, sizeof(sv->str), "%.6g", v);
}

static void script_set_str(ScriptCtx *ctx, const char *name, const char *s) {
    ScriptVar *sv = script_find_var(ctx, name);
    if (!sv && ctx->count < 24) sv = &ctx->vars[ctx->count++];
    if (!sv) return;
    snprintf(sv->name, sizeof(sv->name), "%.*s", (int)sizeof(sv->name) - 1, name);
    sv->is_num = 0;
    sv->num = atof(s);
    snprintf(sv->str, sizeof(sv->str), "%.*s", (int)sizeof(sv->str) - 1, s ? s : "");
}

static void js_eval_expr(NatureOSState *st, ScriptCtx *ctx, const char *raw_expr,
                         char *out_str, size_t scap, double *out_num, int *out_is_num);

static void js_split_args(const char *s, char parts[][192], int *nparts, int max_parts) {
    *nparts = 0;
    int depth = 0;
    char q = 0;
    const char *start = s;
    for (const char *p = s; *p; ++p) {
        if ((*p == '"' || *p == '\'' || *p == '`') && (p == s || p[-1] != '\\')) {
            if (q == 0) q = *p;
            else if (q == *p) q = 0;
        } else if (q == 0) {
            if (*p == '(' || *p == '[' || *p == '{') depth++;
            else if (*p == ')' || *p == ']' || *p == '}') { if (depth > 0) depth--; }
            else if (*p == ',' && depth == 0 && *nparts + 1 < max_parts) {
                size_t len = (size_t)(p - start);
                if (len >= 192) len = 191;
                memcpy(parts[*nparts], start, len);
                parts[*nparts][len] = '\0';
                trim_inplace(parts[*nparts]);
                (*nparts)++;
                start = p + 1;
            }
        }
    }
    if (*nparts < max_parts) {
        snprintf(parts[*nparts], 192, "%s", start);
        trim_inplace(parts[*nparts]);
        if (parts[*nparts][0]) (*nparts)++;
    }
}

static void js_eval_atom(NatureOSState *st, ScriptCtx *ctx, const char *raw,
                         char *out_str, size_t scap, double *out_num, int *out_is_num) {
    char buf[256];
    snprintf(buf, sizeof(buf), "%s", raw ? raw : "");
    char *s = trim_inplace(buf);
    size_t len = strlen(s);
    *out_is_num = 0;
    *out_num = 0.0;
    out_str[0] = '\0';
    if (!len) return;

    /* Strip balanced outer parens */
    if (s[0] == '(' && s[len - 1] == ')') {
        int d = 0, wraps = 1;
        char q = 0;
        for (size_t i = 0; i < len - 1; ++i) {
            if ((s[i] == '"' || s[i] == '\'') && (i == 0 || s[i - 1] != '\\')) {
                if (q == 0) q = s[i]; else if (q == s[i]) q = 0;
            } else if (q == 0) {
                if (s[i] == '(') d++;
                else if (s[i] == ')') d--;
                if (d == 0) { wraps = 0; break; }
            }
        }
        if (wraps) {
            s[len - 1] = '\0';
            js_eval_expr(st, ctx, s + 1, out_str, scap, out_num, out_is_num);
            return;
        }
    }

    /* String literals */
    if ((s[0] == '"' && s[len - 1] == '"') || (s[0] == '\'' && s[len - 1] == '\'')) {
        s[len - 1] = '\0';
        snprintf(out_str, scap, "%s", s + 1);
        return;
    }

    /* Template literal `...${expr}...` */
    if (s[0] == '`' && s[len - 1] == '`') {
        s[len - 1] = '\0';
        const char *p = s + 1;
        size_t pos = 0;
        while (*p && pos + 1 < scap) {
            if (p[0] == '$' && p[1] == '{') {
                const char *end = strchr(p + 2, '}');
                if (end) {
                    char sub[128], sv[128];
                    size_t sl = (size_t)(end - (p + 2));
                    if (sl >= sizeof(sub)) sl = sizeof(sub) - 1;
                    memcpy(sub, p + 2, sl);
                    sub[sl] = '\0';
                    double dn = 0; int isn = 0;
                    js_eval_expr(st, ctx, sub, sv, sizeof(sv), &dn, &isn);
                    size_t svl = strlen(sv);
                    if (pos + svl >= scap) svl = scap - 1 - pos;
                    memcpy(out_str + pos, sv, svl);
                    pos += svl;
                    p = end + 1;
                    continue;
                }
            }
            out_str[pos++] = *p++;
        }
        out_str[pos] = '\0';
        return;
    }

    /* Numeric literal */
    char *endptr = NULL;
    double nv = strtod(s, &endptr);
    if (endptr && *endptr == '\0' && s[0] != '\0') {
        *out_is_num = 1;
        *out_num = nv;
        if (nv == (double)(long long)nv) snprintf(out_str, scap, "%lld", (long long)nv);
        else snprintf(out_str, scap, "%.6g", nv);
        return;
    }

    /* Built-in properties & globals */
    const char *host = st->cfg.device_name[0] ? st->cfg.device_name : "nature-pc";
    if (strcmp(s, "true") == 0) { snprintf(out_str, scap, "true"); *out_is_num = 1; *out_num = 1; return; }
    if (strcmp(s, "false") == 0) { snprintf(out_str, scap, "false"); *out_is_num = 1; *out_num = 0; return; }
    if (strcmp(s, "Math.PI") == 0) { *out_is_num = 1; *out_num = 3.141592653589793; snprintf(out_str, scap, "%.15g", *out_num); return; }
    if (strcmp(s, "Math.E") == 0) { *out_is_num = 1; *out_num = 2.718281828459045; snprintf(out_str, scap, "%.15g", *out_num); return; }
    if (strcmp(s, "process.version") == 0) { snprintf(out_str, scap, "v22.12.0"); return; }
    if (strcmp(s, "process.platform") == 0 || strcmp(s, "os.platform()") == 0) { snprintf(out_str, scap, "linux"); return; }
    if (strcmp(s, "process.arch") == 0 || strcmp(s, "os.arch()") == 0) { snprintf(out_str, scap, "x64"); return; }
    if (strcmp(s, "process.pid") == 0) { *out_is_num = 1; *out_num = 1042; snprintf(out_str, scap, "1042"); return; }
    if (strcmp(s, "process.cwd()") == 0) { snprintf(out_str, scap, "%s", st->term_cwd); return; }
    if (strcmp(s, "process.uptime()") == 0 || strcmp(s, "os.uptime()") == 0) {
        *out_is_num = 1; *out_num = (double)st->uptime_sec;
        snprintf(out_str, scap, "%ld", st->uptime_sec);
        return;
    }
    if (strcmp(s, "os.hostname()") == 0) { snprintf(out_str, scap, "%s", host); return; }
    if (strcmp(s, "os.cpus().length") == 0 || strcmp(s, "os.cpu_count()") == 0) {
        int nc = st->cpu_count > 0 ? st->cpu_count : 4;
        *out_is_num = 1; *out_num = (double)nc;
        snprintf(out_str, scap, "%d", nc);
        return;
    }
    if (strcmp(s, "os.cpus()") == 0) {
        snprintf(out_str, scap, "[%d x %.48s @ %.1f%%]",
                 st->cpu_count > 0 ? st->cpu_count : 4, st->cpu_model, st->total_cpu_pct);
        return;
    }
    if (strcmp(s, "os.totalmem()") == 0) {
        unsigned long long bytes = (unsigned long long)(st->total_ram_mb > 0 ? st->total_ram_mb : 4096) * 1024ULL * 1024ULL;
        *out_is_num = 1; *out_num = (double)bytes;
        snprintf(out_str, scap, "%llu", bytes);
        return;
    }
    if (strcmp(s, "os.freemem()") == 0) {
        unsigned long long bytes = (unsigned long long)(st->avail_ram_mb > 0 ? st->avail_ram_mb : 2048) * 1024ULL * 1024ULL;
        *out_is_num = 1; *out_num = (double)bytes;
        snprintf(out_str, scap, "%llu", bytes);
        return;
    }
    if (strcmp(s, "os.loadavg()") == 0) {
        snprintf(out_str, scap, "[%.2f, %.2f, %.2f]", st->load_avg[0], st->load_avg[1], st->load_avg[2]);
        return;
    }
    if (strcmp(s, "process.memoryUsage()") == 0) {
        snprintf(out_str, scap, "{ rss: %llu, heapTotal: 33554432, heapUsed: 18874368 }",
                 (unsigned long long)(st->os_rss_mb > 0 ? st->os_rss_mb : 42) * 1024ULL * 1024ULL);
        return;
    }
    if (strncmp(s, "process.env.", 12) == 0) {
        char sc[32];
        const char *ev = lookup_env_var(st, s + 12, sc, sizeof(sc));
        snprintf(out_str, scap, "%s", ev[0] ? ev : "undefined");
        return;
    }
    if (strcmp(s, "Date.now()") == 0) {
        unsigned long long ms = (unsigned long long)time(NULL) * 1000ULL;
        *out_is_num = 1; *out_num = (double)ms;
        snprintf(out_str, scap, "%llu", ms);
        return;
    }
    if (strcmp(s, "sys.version") == 0) { snprintf(out_str, scap, "3.12.3 (Nature OS)"); return; }
    if (strcmp(s, "platform.system()") == 0) { snprintf(out_str, scap, "Linux"); return; }

    /* Function calls: Math.*, JSON.stringify(...), fs.readFileSync(...), fs.existsSync(...) */
    const char *lp = strchr(s, '(');
    if (lp && s[len - 1] == ')') {
        char fn[64];
        size_t fl = (size_t)(lp - s);
        if (fl >= sizeof(fn)) fl = sizeof(fn) - 1;
        memcpy(fn, s, fl);
        fn[fl] = '\0';
        trim_inplace(fn);

        char inner[224];
        size_t il = len - (size_t)(lp - s) - 2;
        if (il >= sizeof(inner)) il = sizeof(inner) - 1;
        memcpy(inner, lp + 1, il);
        inner[il] = '\0';

        char args[4][192];
        int nargs = 0;
        js_split_args(inner, args, &nargs, 4);

        if (strncmp(fn, "Math.", 5) == 0 && nargs >= 1) {
            const char *mfn = fn + 5;
            double a = 0, b = 0; char as[96], bs[96]; int ia = 0, ib = 0;
            js_eval_expr(st, ctx, args[0], as, sizeof(as), &a, &ia);
            if (nargs >= 2) js_eval_expr(st, ctx, args[1], bs, sizeof(bs), &b, &ib);
            double r = 0;
            if (strcmp(mfn, "sqrt") == 0) r = sqrt(a < 0 ? 0 : a);
            else if (strcmp(mfn, "floor") == 0) r = floor(a);
            else if (strcmp(mfn, "ceil") == 0) r = ceil(a);
            else if (strcmp(mfn, "round") == 0) r = round(a);
            else if (strcmp(mfn, "abs") == 0) r = fabs(a);
            else if (strcmp(mfn, "pow") == 0) r = pow(a, b);
            else if (strcmp(mfn, "max") == 0) r = (nargs >= 2 && b > a) ? b : a;
            else if (strcmp(mfn, "min") == 0) r = (nargs >= 2 && b < a) ? b : a;
            *out_is_num = 1; *out_num = r;
            if (r == (double)(long long)r) snprintf(out_str, scap, "%lld", (long long)r);
            else snprintf(out_str, scap, "%.6g", r);
            return;
        }
        if (strcmp(fn, "JSON.stringify") == 0 && nargs >= 1) {
            char vs[192]; double vn = 0; int isn = 0;
            js_eval_expr(st, ctx, args[0], vs, sizeof(vs), &vn, &isn);
            if (isn || vs[0] == '{' || vs[0] == '[') snprintf(out_str, scap, "%s", vs);
            else snprintf(out_str, scap, "\"%.*s\"", (int)(scap > 3 ? scap - 3 : 0), vs);
            return;
        }
        if (strcmp(fn, "fs.readFileSync") == 0 && nargs >= 1) {
            char pstr[160], resolved[160]; double pn = 0; int isn = 0;
            js_eval_expr(st, ctx, args[0], pstr, sizeof(pstr), &pn, &isn);
            resolve_term_path(st, pstr, resolved, sizeof(resolved));
            int idx = find_vfs_by_path(st, resolved);
            if (idx >= 0 && !st->vfs[idx].is_dir) {
                snprintf(out_str, scap, "%.*s", (int)scap - 1, st->vfs[idx].content);
            } else {
                snprintf(out_str, scap, "Error: ENOENT: %s", resolved);
            }
            return;
        }
        if (strcmp(fn, "fs.existsSync") == 0 && nargs >= 1) {
            char pstr[160], resolved[160]; double pn = 0; int isn = 0;
            js_eval_expr(st, ctx, args[0], pstr, sizeof(pstr), &pn, &isn);
            resolve_term_path(st, pstr, resolved, sizeof(resolved));
            int ok = find_vfs_by_path(st, resolved) >= 0;
            *out_is_num = 1; *out_num = ok ? 1 : 0;
            snprintf(out_str, scap, "%s", ok ? "true" : "false");
            return;
        }
    }

    /* Script variables */
    ScriptVar *sv = script_find_var(ctx, s);
    if (sv) {
        *out_is_num = sv->is_num;
        *out_num = sv->num;
        snprintf(out_str, scap, "%s", sv->str);
        return;
    }

    snprintf(out_str, scap, "%s", s);
}

static void js_eval_expr(NatureOSState *st, ScriptCtx *ctx, const char *raw_expr,
                         char *out_str, size_t scap, double *out_num, int *out_is_num) {
    char buf[256];
    snprintf(buf, sizeof(buf), "%s", raw_expr ? raw_expr : "");
    char *s = trim_inplace(buf);
    if (!s[0]) { out_str[0] = '\0'; *out_num = 0; *out_is_num = 0; return; }

    /* Find lowest-precedence binary operator outside quotes & parens */
    int depth = 0;
    char q = 0;
    const char *op_pos = NULL;
    char op_ch = 0;
    int op_prio = 99;
    for (const char *p = s; *p; ++p) {
        if ((*p == '"' || *p == '\'' || *p == '`') && (p == s || p[-1] != '\\')) {
            if (q == 0) q = *p;
            else if (q == *p) q = 0;
        } else if (q == 0) {
            if (*p == '(' || *p == '[') depth++;
            else if (*p == ')' || *p == ']') { if (depth > 0) depth--; }
            else if (depth == 0) {
                int pr = 99;
                if ((*p == '+' || *p == '-') && p > s) pr = 1;
                else if (*p == '*' || *p == '/' || *p == '%') pr = 2;
                if (pr <= op_prio) {
                    op_prio = pr;
                    op_pos = p;
                    op_ch = *p;
                }
            }
        }
    }

    if (op_pos) {
        char left_raw[192], right_raw[192];
        size_t ll = (size_t)(op_pos - s);
        if (ll >= sizeof(left_raw)) ll = sizeof(left_raw) - 1;
        memcpy(left_raw, s, ll);
        left_raw[ll] = '\0';
        snprintf(right_raw, sizeof(right_raw), "%s", op_pos + 1);

        char ls[192], rs[192];
        double ln = 0, rn = 0;
        int lin = 0, rin = 0;
        js_eval_expr(st, ctx, left_raw, ls, sizeof(ls), &ln, &lin);
        js_eval_expr(st, ctx, right_raw, rs, sizeof(rs), &rn, &rin);

        if (op_ch == '+' && (!lin || !rin)) {
            *out_is_num = 0;
            *out_num = 0;
            snprintf(out_str, scap, "%.*s%.*s", (int)(scap / 2), ls, (int)(scap / 2 - 1), rs);
            return;
        }
        double res = 0;
        if (op_ch == '+') res = ln + rn;
        else if (op_ch == '-') res = ln - rn;
        else if (op_ch == '*') res = ln * rn;
        else if (op_ch == '/') res = (rn != 0.0) ? (ln / rn) : 0.0;
        else if (op_ch == '%') res = ((long long)rn != 0) ? (double)((long long)ln % (long long)rn) : 0.0;
        *out_is_num = 1;
        *out_num = res;
        if (res == (double)(long long)res) snprintf(out_str, scap, "%lld", (long long)res);
        else snprintf(out_str, scap, "%.6g", res);
        return;
    }

    js_eval_atom(st, ctx, s, out_str, scap, out_num, out_is_num);
}

static void eval_node_script(NatureOSState *st, const char *code, int print_last_expr) {
    ScriptCtx ctx;
    memset(&ctx, 0, sizeof(ctx));

    char buf[768];
    snprintf(buf, sizeof(buf), "%s", code ? code : "");

    char *stmts[32];
    int nstmt = 0;
    char *cur = buf;
    char q = 0;
    int brace = 0, paren = 0;
    for (char *p = buf; *p; ++p) {
        if (q == 0 && p[0] == '/' && p[1] == '/') {
            /* Strip // comment until newline */
            while (*p && *p != '\n') *p++ = ' ';
            if (!*p) break;
        }
        if ((*p == '"' || *p == '\'' || *p == '`') && (p == buf || p[-1] != '\\')) {
            if (q == 0) q = *p;
            else if (q == *p) q = 0;
        } else if (q == 0) {
            if (*p == '{') brace++;
            else if (*p == '}' && brace > 0) brace--;
            else if (*p == '(') paren++;
            else if (*p == ')' && paren > 0) paren--;
            else if ((*p == ';' || *p == '\n') && brace == 0 && paren == 0 && nstmt < 32) {
                *p = '\0';
                stmts[nstmt++] = cur;
                cur = p + 1;
            }
        }
    }
    if (nstmt < 32) stmts[nstmt++] = cur;

    char last_out[192] = {0};
    int has_last = 0;

    for (int i = 0; i < nstmt; ++i) {
        char *s = trim_inplace(stmts[i]);
        if (!s[0]) continue;
        if (strncmp(s, "import ", 7) == 0 || strstr(s, "require(") != NULL) {
            /* Check if const x = require('...') */
            if (strstr(s, "http") && strstr(s, "require")) continue;
            if (strncmp(s, "const ", 6) == 0 || strncmp(s, "let ", 4) == 0 || strncmp(s, "var ", 4) == 0 || strncmp(s, "import ", 7) == 0) {
                continue;
            }
        }
        if (strstr(s, "http.createServer") != NULL) {
            int port = 3000;
            const char *lp = strstr(s, ".listen(");
            if (lp) {
                int pv = atoi(lp + 8);
                if (pv > 0 && pv < 65535) port = pv;
            }
            ctx.server_started = 1;
            ctx.server_port = port;
            char msg[128];
            snprintf(msg, sizeof(msg), "Node HTTP server bound on http://127.0.0.1:%d (PID 1042)", port);
            push_term_line(st, msg, 2);
            continue;
        }
        if (strncmp(s, "console.log(", 12) == 0 || strncmp(s, "console.info(", 13) == 0 ||
            strncmp(s, "console.warn(", 13) == 0 || strncmp(s, "console.error(", 14) == 0) {
            int is_err = (strncmp(s, "console.error(", 14) == 0 || strncmp(s, "console.warn(", 13) == 0);
            const char *op = strchr(s, '(');
            size_t sl = strlen(s);
            if (op && s[sl - 1] == ')') {
                char inner[256];
                size_t il = sl - (size_t)(op - s) - 2;
                if (il >= sizeof(inner)) il = sizeof(inner) - 1;
                memcpy(inner, op + 1, il);
                inner[il] = '\0';
                char args[6][192];
                int nargs = 0;
                js_split_args(inner, args, &nargs, 6);
                char line[256] = {0};
                for (int a = 0; a < nargs; ++a) {
                    char vs[192]; double vn = 0; int isn = 0;
                    js_eval_expr(st, &ctx, args[a], vs, sizeof(vs), &vn, &isn);
                    if (a > 0 && strlen(line) + 1 < sizeof(line)) strcat(line, " ");
                    strncat(line, vs, sizeof(line) - strlen(line) - 1);
                }
                push_term_line(st, line, is_err ? 3 : 0);
                has_last = 0;
            }
            continue;
        }
        if (strncmp(s, "fs.writeFileSync(", 17) == 0) {
            const char *op = strchr(s, '(');
            size_t sl = strlen(s);
            if (op && s[sl - 1] == ')') {
                char inner[256];
                size_t il = sl - (size_t)(op - s) - 2;
                if (il >= sizeof(inner)) il = sizeof(inner) - 1;
                memcpy(inner, op + 1, il);
                inner[il] = '\0';
                char args[4][192]; int nargs = 0;
                js_split_args(inner, args, &nargs, 4);
                if (nargs >= 2) {
                    char fp[160], txt[192], resolved[160];
                    double dn = 0; int isn = 0;
                    js_eval_expr(st, &ctx, args[0], fp, sizeof(fp), &dn, &isn);
                    js_eval_expr(st, &ctx, args[1], txt, sizeof(txt), &dn, &isn);
                    resolve_term_path(st, fp, resolved, sizeof(resolved));
                    upsert_vfs_file(st, resolved, txt, 0);
                }
            }
            continue;
        }
        /* Variable declaration or assignment */
        const char *decl = s;
        if (strncmp(decl, "const ", 6) == 0) decl += 6;
        else if (strncmp(decl, "let ", 4) == 0) decl += 4;
        else if (strncmp(decl, "var ", 4) == 0) decl += 4;
        const char *eq = strchr(decl, '=');
        if (eq && eq[1] != '=') {
            char vname[32];
            size_t nl = (size_t)(eq - decl);
            if (nl >= sizeof(vname)) nl = sizeof(vname) - 1;
            memcpy(vname, decl, nl);
            vname[nl] = '\0';
            trim_inplace(vname);
            char val_s[192]; double val_n = 0; int isn = 0;
            js_eval_expr(st, &ctx, eq + 1, val_s, sizeof(val_s), &val_n, &isn);
            if (isn) script_set_num(&ctx, vname, val_n);
            else script_set_str(&ctx, vname, val_s);
            has_last = 0;
            continue;
        }

        double vn = 0; int isn = 0;
        js_eval_expr(st, &ctx, s, last_out, sizeof(last_out), &vn, &isn);
        has_last = 1;
    }

    if (print_last_expr && has_last && last_out[0]) {
        push_term_line(st, last_out, 2);
    }
}

static void eval_python_script(NatureOSState *st, const char *code) {
    ScriptCtx ctx;
    memset(&ctx, 0, sizeof(ctx));

    char buf[768];
    snprintf(buf, sizeof(buf), "%s", code ? code : "");
    char *p = buf;
    while (*p) {
        char *eol = strchr(p, '\n');
        if (eol) *eol = '\0';
        char *semi = strchr(p, ';');
        /* Handle simple newline or semicolon statements */
        char *line = trim_inplace(p);
        (void)semi;
        if (line[0] && line[0] != '#') {
            if (strncmp(line, "import ", 7) == 0 || strncmp(line, "from ", 5) == 0) {
                /* no-op import */
            } else if (strncmp(line, "print(", 6) == 0) {
                size_t sl = strlen(line);
                if (line[sl - 1] == ')') {
                    char inner[256];
                    size_t il = sl - 7;
                    if (il >= sizeof(inner)) il = sizeof(inner) - 1;
                    memcpy(inner, line + 6, il);
                    inner[il] = '\0';
                    /* Handle Python f"..." -> JS `...` conversion for ${...} */
                    if ((inner[0] == 'f' || inner[0] == 'F') && (inner[1] == '"' || inner[1] == '\'')) {
                        size_t fl = strlen(inner);
                        if (inner[fl - 1] == inner[1]) {
                            char templ[256];
                            size_t tp = 0;
                            templ[tp++] = '`';
                            for (size_t k = 2; k + 1 < fl && tp + 3 < sizeof(templ); ++k) {
                                if (inner[k] == '{') {
                                    templ[tp++] = '$';
                                    templ[tp++] = '{';
                                } else {
                                    templ[tp++] = inner[k];
                                }
                            }
                            templ[tp++] = '`';
                            templ[tp] = '\0';
                            snprintf(inner, sizeof(inner), "%s", templ);
                        }
                    }
                    char args[6][192]; int nargs = 0;
                    js_split_args(inner, args, &nargs, 6);
                    char out[256] = {0};
                    for (int a = 0; a < nargs; ++a) {
                        char vs[192]; double vn = 0; int isn = 0;
                        js_eval_expr(st, &ctx, args[a], vs, sizeof(vs), &vn, &isn);
                        if (a > 0 && strlen(out) + 1 < sizeof(out)) strcat(out, " ");
                        strncat(out, vs, sizeof(out) - strlen(out) - 1);
                    }
                    push_term_line(st, out, 0);
                }
            } else {
                const char *eq = strchr(line, '=');
                if (eq && eq[1] != '=') {
                    char vname[32];
                    size_t nl = (size_t)(eq - line);
                    if (nl >= sizeof(vname)) nl = sizeof(vname) - 1;
                    memcpy(vname, line, nl);
                    vname[nl] = '\0';
                    trim_inplace(vname);
                    char vs[192]; double vn = 0; int isn = 0;
                    js_eval_expr(st, &ctx, eq + 1, vs, sizeof(vs), &vn, &isn);
                    if (isn) script_set_num(&ctx, vname, vn);
                    else script_set_str(&ctx, vname, vs);
                }
            }
        }
        if (!eol) break;
        p = eol + 1;
    }
}

static int find_package_json(const NatureOSState *st) {
    char cand[160];
    snprintf(cand, sizeof(cand), "%s/package.json", strcmp(st->term_cwd, "/") == 0 ? "" : st->term_cwd);
    int idx = find_vfs_by_path(st, cand);
    if (idx >= 0 && !st->vfs[idx].is_dir) return idx;
    idx = find_vfs_by_path(st, "/home/nature/Projects/package.json");
    if (idx >= 0 && !st->vfs[idx].is_dir) return idx;
    return -1;
}

static void exec_single_cmd(NatureOSState *st, const char *single_raw);

static void exec_single_cmd(NatureOSState *st, const char *single_raw) {
    char expanded[256];
    expand_env_vars(st, single_raw ? single_raw : "", expanded, sizeof(expanded), 1);

    char line_copy[256];
    snprintf(line_copy, sizeof(line_copy), "%s", expanded);
    char *trimmed = trim_inplace(line_copy);
    if (!trimmed[0]) return;

    if (strncmp(trimmed, "sudo ", 5) == 0 || strcmp(trimmed, "sudo") == 0) {
        trimmed += (trimmed[4] == ' ') ? 5 : 4;
        trimmed = trim_inplace(trimmed);
        if (!trimmed[0]) {
            push_term_line(st, "usage: sudo <command>", 0);
            return;
        }
    }

    /* Special handling for echo (preserves spaces, quotes, -n, >, >>) */
    if (strcmp(trimmed, "echo") == 0) {
        push_term_line(st, "", 0);
        return;
    }
    if (strncmp(trimmed, "echo ", 5) == 0 || strncmp(trimmed, "echo\t", 5) == 0) {
        const char *payload = trimmed + 5;
        while (*payload == ' ' || *payload == '\t') payload++;
        if (strncmp(payload, "-n ", 3) == 0) {
            payload += 3;
            while (*payload == ' ' || *payload == '\t') payload++;
        }
        int is_append = 0;
        const char *redir = find_unquoted_redir(payload, &is_append);
        if (redir) {
            char text_part[256];
            int tlen = (int)(redir - payload);
            while (tlen > 0 && (payload[tlen - 1] == ' ' || payload[tlen - 1] == '\t')) tlen--;
            if (tlen >= (int)sizeof(text_part)) tlen = (int)sizeof(text_part) - 1;
            memcpy(text_part, payload, (size_t)tlen);
            text_part[tlen] = '\0';
            if (tlen >= 2 && ((text_part[0] == '"' && text_part[tlen - 1] == '"') ||
                              (text_part[0] == '\'' && text_part[tlen - 1] == '\''))) {
                memmove(text_part, text_part + 1, (size_t)(tlen - 2));
                text_part[tlen - 2] = '\0';
            }

            const char *fname = redir + (is_append ? 2 : 1);
            while (*fname == ' ' || *fname == '\t') fname++;
            char clean_name[128];
            snprintf(clean_name, sizeof(clean_name), "%s", fname);
            char *cname = trim_inplace(clean_name);
            size_t clen = strlen(cname);
            if (clen >= 2 && ((cname[0] == '"' && cname[clen - 1] == '"') ||
                              (cname[0] == '\'' && cname[clen - 1] == '\''))) {
                memmove(cname, cname + 1, clen - 2);
                cname[clen - 2] = '\0';
            }
            if (!cname[0]) {
                push_term_line(st, "bash: syntax error near unexpected token `newline'", 3);
                return;
            }
            char target[160];
            resolve_term_path(st, cname, target, sizeof(target));
            int rc = upsert_vfs_file(st, target, text_part, is_append);
            if (rc == -2) push_term_line(st, "bash: Is a directory", 3);
            else if (rc == -1) push_term_line(st, "bash: VFS node limit reached", 3);
            else push_term_line(st, is_append ? "Appended to file in virtual filesystem." : "Wrote file to virtual filesystem.", 2);
        } else {
            char msg[256];
            snprintf(msg, sizeof(msg), "%s", payload);
            size_t mlen = strlen(msg);
            if (mlen >= 2 && ((msg[0] == '"' && msg[mlen - 1] == '"') ||
                              (msg[0] == '\'' && msg[mlen - 1] == '\''))) {
                memmove(msg, msg + 1, mlen - 2);
                msg[mlen - 2] = '\0';
            }
            push_term_line(st, msg, 0);
        }
        return;
    }

    char tok_buf[256];
    snprintf(tok_buf, sizeof(tok_buf), "%s", trimmed);
    char *argv[24] = {0};
    int argc = tokenize_cmd(tok_buf, argv, 24);
    if (argc == 0) return;

    const char *cmd = argv[0];
    const char *arg1 = argc > 1 ? argv[1] : NULL;
    const char *arg2 = argc > 2 ? argv[2] : NULL;
    const char *arg3 = argc > 3 ? argv[3] : NULL;

    if (strcmp(cmd, "help") == 0) {
        push_term_line(st, "Nature OS Bash 5.2 (supports ;, &&, $VAR, Tab completion, Spacebar):", 2);
        push_term_line(st, "  ls [-la] [dir]    cd <dir|->       pwd / tree       cat / head / tail", 0);
        push_term_line(st, "  mkdir [-p] <dir>  rm [-rf] <p>     cp / mv / touch  grep [-i|-n] / wc [-lwc]", 0);
        push_term_line(st, "  node [-v|-e|-p]   npm [run|init]   python3 [-V|-c]  git [status|log|branch]", 0);
        push_term_line(st, "  curl [-I|-O|url]  wget <url>       nano <file>      export K=V / env", 0);
        push_term_line(st, "  systemctl [act]   apt [list|...]   free -m / df -h  neofetch / htop / nproc", 0);
        push_term_line(st, "  ip / ping         theme [mode]     man <cmd>        open <app> / clear", 0);
    } else if (strcmp(cmd, "man") == 0) {
        if (!arg1) {
            push_term_line(st, "What manual page do you want? Try: man node, man curl, man systemctl, man git, man apt", 2);
        } else if (strcmp(arg1, "node") == 0) {
            push_term_line(st, "NODE(1) - Nature OS Node.js v22.12.0 Runtime", 2);
            push_term_line(st, "  node -v              Print runtime version (v22.12.0)", 0);
            push_term_line(st, "  node -e \"code\"       Evaluate JS (os.*, process.*, fs.*, Math.*, console.log)", 0);
            push_term_line(st, "  node -p \"expr\"       Evaluate JS expression and print result", 0);
            push_term_line(st, "  node <file.js>       Execute JavaScript file from VFS (e.g. ~/Projects/server.js)", 0);
        } else if (strcmp(arg1, "npm") == 0) {
            push_term_line(st, "NPM(1) - Node Package Manager 10.9.0", 2);
            push_term_line(st, "  npm init [-y]        Create package.json in current directory", 0);
            push_term_line(st, "  npm run <script>     Run script defined in package.json (e.g. npm start / npm run dev)", 0);
            push_term_line(st, "  npm list             Show project & installed packages", 0);
            push_term_line(st, "  npm install <pkg>    Install package into APT/Store & package.json", 0);
        } else if (strcmp(arg1, "curl") == 0) {
            push_term_line(st, "CURL(1) - Transfer a URL via Nature OS Web Gateway", 2);
            push_term_line(st, "  curl -V / --version    Print libcurl version", 0);
            push_term_line(st, "  curl -I / --head <url> Fetch HTTP response headers", 0);
            push_term_line(st, "  curl -O <url>          Save remote file into current directory", 0);
            push_term_line(st, "  curl wttr.in           Fetch interactive weather forecast", 0);
            push_term_line(st, "  curl cheat.sh/<lang>   Fetch quick programming cheatsheet", 0);
        } else if (strcmp(arg1, "wget") == 0) {
            push_term_line(st, "WGET(1) - Retrieve files over HTTP/HTTPS", 2);
            push_term_line(st, "  wget <url>             Download file into current directory", 0);
        } else if (strcmp(arg1, "nano") == 0) {
            push_term_line(st, "NANO(1) - Nano's ANOther editor", 2);
            push_term_line(st, "  nano [<file>]          Inspect or create file buffer", 0);
        } else if (strcmp(arg1, "systemctl") == 0) {
            push_term_line(st, "SYSTEMCTL(1) - Control Nature OS systemd units", 2);
            push_term_line(st, "  systemctl                     List unit states (pipewire, NetworkManager, systemd-oomd)", 0);
            push_term_line(st, "  systemctl start|stop|status <unit>  Live toggle service health across Settings/Shell", 0);
        } else if (strcmp(arg1, "python3") == 0) {
            push_term_line(st, "PYTHON3(1) - Python 3.12.3 Runtime", 2);
            push_term_line(st, "  python3 -V | -c \"code\" | <file.py>", 0);
        } else {
            char mbuf[128];
            snprintf(mbuf, sizeof(mbuf), "No detailed manual entry for '%s'; run 'help' for built-in usage.", arg1);
            push_term_line(st, mbuf, 0);
        }
    } else if (strcmp(cmd, "reset-settings") == 0) {
        nature_os_reset_defaults(st);
        push_term_line(st, "Settings restored to Nature OS defaults and saved.", 2);
    } else if (strcmp(cmd, "clear") == 0) {
        st->term_line_count = 0;
        st->term_scroll = 0;
    } else if (strcmp(cmd, "pwd") == 0) {
        push_term_line(st, st->term_cwd, 0);
    } else if (strcmp(cmd, "whoami") == 0) {
        push_term_line(st, "nature", 0);
    } else if (strcmp(cmd, "nproc") == 0) {
        char nbuf[24];
        snprintf(nbuf, sizeof(nbuf), "%d", st->cpu_count > 0 ? st->cpu_count : 4);
        push_term_line(st, nbuf, 2);
    } else if (strcmp(cmd, "id") == 0) {
        push_term_line(st, "uid=1000(nature) gid=1000(nature) groups=1000(nature),4(adm),27(sudo),29(audio),44(video)", 0);
    } else if (strcmp(cmd, "export") == 0) {
        if (!arg1) {
            for (int i = 0; i < st->term_env_count; ++i) {
                char l[128];
                snprintf(l, sizeof(l), "declare -x %s=\"%s\"", st->term_env_keys[i], st->term_env_vals[i]);
                push_term_line(st, l, 0);
            }
        } else {
            for (int a = 1; a < argc; ++a) {
                const char *eq = strchr(argv[a], '=');
                if (!eq || eq == argv[a]) continue;
                char k[24];
                size_t kl = (size_t)(eq - argv[a]);
                if (kl >= sizeof(k)) kl = sizeof(k) - 1;
                memcpy(k, argv[a], kl);
                k[kl] = '\0';
                const char *val = eq + 1;
                int slot = -1;
                for (int i = 0; i < st->term_env_count; ++i) {
                    if (strcmp(st->term_env_keys[i], k) == 0) { slot = i; break; }
                }
                if (slot < 0 && st->term_env_count < 8) slot = st->term_env_count++;
                if (slot >= 0) {
                    snprintf(st->term_env_keys[slot], sizeof(st->term_env_keys[0]), "%s", k);
                    snprintf(st->term_env_vals[slot], sizeof(st->term_env_vals[0]), "%.*s",
                             (int)sizeof(st->term_env_vals[0]) - 1, val);
                }
            }
        }
    } else if (strcmp(cmd, "unset") == 0 && arg1) {
        for (int i = 0; i < st->term_env_count; ++i) {
            if (strcmp(st->term_env_keys[i], arg1) == 0) {
                for (int j = i; j < st->term_env_count - 1; ++j) {
                    snprintf(st->term_env_keys[j], sizeof(st->term_env_keys[0]), "%s", st->term_env_keys[j + 1]);
                    snprintf(st->term_env_vals[j], sizeof(st->term_env_vals[0]), "%s", st->term_env_vals[j + 1]);
                }
                st->term_env_count--;
                break;
            }
        }
    } else if (strcmp(cmd, "env") == 0) {
        const char *host = st->cfg.device_name[0] ? st->cfg.device_name : "nature-pc";
        char l[160];
        push_term_line(st, "USER=nature", 0);
        push_term_line(st, "HOME=/home/nature", 0);
        snprintf(l, sizeof(l), "PWD=%s", st->term_cwd); push_term_line(st, l, 0);
        snprintf(l, sizeof(l), "HOSTNAME=%.31s", host); push_term_line(st, l, 0);
        push_term_line(st, "SHELL=/bin/bash", 0);
        push_term_line(st, "NODE_VERSION=v22.12.0", 0);
        push_term_line(st, "XDG_CURRENT_DESKTOP=NatureOS:GNOME", 0);
        snprintf(l, sizeof(l), "NATURE_THEME=%s-%s",
                 st->cfg.theme_family == THEME_HORIZON ? "horizon" : "nature",
                 st->cfg.dark_mode ? "dark" : "light");
        push_term_line(st, l, 2);
        for (int i = 0; i < st->term_env_count; ++i) {
            snprintf(l, sizeof(l), "%s=%s", st->term_env_keys[i], st->term_env_vals[i]);
            push_term_line(st, l, 0);
        }
    } else if (strcmp(cmd, "which") == 0 && arg1) {
        int known = 0;
        for (size_t i = 0; i < sizeof(KNOWN_CMDS) / sizeof(KNOWN_CMDS[0]); ++i) {
            if (strcmp(KNOWN_CMDS[i], arg1) == 0) { known = 1; break; }
        }
        if (known) {
            char wb[96];
            snprintf(wb, sizeof(wb), "/usr/bin/%s", arg1);
            push_term_line(st, wb, 2);
        } else {
            char wb[96];
            snprintf(wb, sizeof(wb), "%s not found", arg1);
            push_term_line(st, wb, 3);
        }
    } else if (strcmp(cmd, "date") == 0) {
        time_t now = time(NULL);
        char dbuf[64];
        strftime(dbuf, sizeof(dbuf), "%a %b %d %H:%M:%S UTC %Y", localtime(&now));
        push_term_line(st, dbuf, 0);
    } else if (strcmp(cmd, "uptime") == 0) {
        time_t now = time(NULL);
        char tbuf[16], uline[128];
        strftime(tbuf, sizeof(tbuf), "%H:%M:%S", localtime(&now));
        long days = st->uptime_sec / 86400;
        long hrs = (st->uptime_sec % 86400) / 3600;
        long mins = (st->uptime_sec % 3600) / 60;
        if (days > 0) {
            snprintf(uline, sizeof(uline), " %s up %ldd %ld:%02ld, 1 user, load avg: %.2f, %.2f, %.2f (CPU %.1f%%)",
                     tbuf, days, hrs, mins, st->load_avg[0], st->load_avg[1], st->load_avg[2], st->total_cpu_pct);
        } else {
            snprintf(uline, sizeof(uline), " %s up %ld:%02ld, 1 user, load avg: %.2f, %.2f, %.2f (CPU %.1f%%)",
                     tbuf, hrs, mins, st->load_avg[0], st->load_avg[1], st->load_avg[2], st->total_cpu_pct);
        }
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
            const char *host = st->cfg.device_name[0] ? st->cfg.device_name : "nature-pc";
            push_term_line(st, host, 0);
        }
    } else if (strcmp(cmd, "uname") == 0) {
        const char *host = st->cfg.device_name[0] ? st->cfg.device_name : "nature-pc";
        char kbuf[128];
        if (arg1 && strcmp(arg1, "-r") == 0) snprintf(kbuf, sizeof(kbuf), "%s", st->kernel_ver);
        else snprintf(kbuf, sizeof(kbuf), "Linux %.31s %.48s #1 SMP PREEMPT_DYNAMIC x86_64 GNU/Linux", host, st->kernel_ver);
        push_term_line(st, kbuf, 0);
    } else if (strcmp(cmd, "lscpu") == 0) {
        int ncores = st->cpu_count > 0 ? st->cpu_count : 4;
        int show_cores = ncores > 8 ? 8 : ncores;
        char l1[128], l2[128], l3[144];
        snprintf(l1, sizeof(l1), "CPU(s):               %d online", ncores);
        snprintf(l2, sizeof(l2), "Model / Arch:         %s", st->cpu_model);
        int pos = snprintf(l3, sizeof(l3), "Live Utilization:     %.1f%% (", st->total_cpu_pct);
        for (int c = 0; c < show_cores && pos < (int)sizeof(l3) - 12; ++c) {
            pos += snprintf(l3 + pos, sizeof(l3) - (size_t)pos, "%sC%d:%.0f%%", c ? " " : "", c, st->core_usage[c]);
        }
        snprintf(l3 + pos, sizeof(l3) - (size_t)pos, ")");
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
        if (st->cfg.wifi_enabled && st->svc_network) {
            char netl[128];
            snprintf(netl, sizeof(netl), "2: wlp2s0: <UP> inet 10.42.0.14/24 (RX %.1f KB/s · TX %.1f KB/s)",
                     st->net_rx_kbps, st->net_tx_kbps);
            push_term_line(st, netl, 2);
        } else {
            push_term_line(st, "2: wlp2s0: <DOWN> state DOWN (Wi-Fi/NetworkManager disabled)", 3);
        }
    } else if (strcmp(cmd, "ping") == 0) {
        const char *target = arg1 ? arg1 : "1.1.1.1";
        if (!st->cfg.wifi_enabled || !st->svc_network) {
            push_term_line(st, "ping: connect: Network is unreachable", 3);
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
            nature_os_save_state(st);
        } else if (strcmp(arg1, "horizon") == 0) {
            st->cfg.theme_family = THEME_HORIZON;
            nature_os_set_wallpaper_preset(st, st->cfg.dark_mode ? 4 : 3);
            push_term_line(st, "Switched theme family to Horizon Slate.", 2);
            nature_os_save_state(st);
        } else if (strcmp(arg1, "dark") == 0) {
            st->cfg.dark_mode = 1;
            if (st->cfg.theme_family == THEME_HORIZON && st->cfg.wallpaper_idx == 3) {
                nature_os_set_wallpaper_preset(st, 4);
            }
            push_term_line(st, "Switched appearance to Midnight Dark.", 2);
            nature_os_save_state(st);
        } else if (strcmp(arg1, "light") == 0) {
            st->cfg.dark_mode = 0;
            if (st->cfg.theme_family == THEME_HORIZON && st->cfg.wallpaper_idx == 4) {
                nature_os_set_wallpaper_preset(st, 3);
            }
            push_term_line(st, "Switched appearance to Daylight Light.", 2);
            nature_os_save_state(st);
        } else {
            push_term_line(st, "Usage: theme [nature|horizon|dark|light]", 3);
        }
    } else if (strcmp(cmd, "sensors") == 0) {
        double tctl = 34.0 + st->total_cpu_pct * 0.42;
        char sline[96];
        push_term_line(st, "k10temp-pci-00c3 · Adapter: PCI adapter", 2);
        snprintf(sline, sizeof(sline), "  Tctl:        +%.1f C  (%s)", tctl, st->total_cpu_pct > 55.0 ? "active load" : "nominal");
        push_term_line(st, sline, st->total_cpu_pct > 70.0 ? 3 : 0);
        push_term_line(st, "nvme-pci-0100 · Composite: +34.5 C", 0);
    } else if (strcmp(cmd, "notify") == 0 && arg1) {
        nature_notify(st, "Terminal Message", arg1);
        push_term_line(st, "Notification broadcast to desktop shell.", 2);
    } else if (strcmp(cmd, "tree") == 0) {
        char target_dir[160];
        if (arg1) resolve_term_path(st, arg1, target_dir, sizeof(target_dir));
        else snprintf(target_dir, sizeof(target_dir), "%s", st->term_cwd);
        push_term_line(st, target_dir, 2);
        for (int i = 0; i < st->vfs_count; ++i) {
            if (strcmp(st->vfs[i].parent, target_dir) == 0) {
                char tree_l[128];
                snprintf(tree_l, sizeof(tree_l), "|-- %s%s", st->vfs[i].name, st->vfs[i].is_dir ? "/" : "");
                push_term_line(st, tree_l, st->vfs[i].is_dir ? 2 : 0);
            }
        }
    } else if (strcmp(cmd, "ls") == 0) {
        char target_dir[160];
        snprintf(target_dir, sizeof(target_dir), "%s", st->term_cwd);
        int detailed = 0, show_all = 0;
        for (int a = 1; a < argc; ++a) {
            if (argv[a][0] == '-') {
                if (strchr(argv[a], 'l')) detailed = 1;
                if (strchr(argv[a], 'a')) show_all = 1;
            } else {
                resolve_term_path(st, argv[a], target_dir, sizeof(target_dir));
            }
        }
        int dir_idx = find_vfs_by_path(st, target_dir);
        if (strcmp(target_dir, "/") != 0 && (dir_idx < 0 || !st->vfs[dir_idx].is_dir)) {
            if (dir_idx >= 0) {
                char line[128];
                snprintf(line, sizeof(line), "-rw-r--r-- 1 nature nature %7d %s %s",
                         st->vfs[dir_idx].size_bytes,
                         st->vfs[dir_idx].modified[0] ? st->vfs[dir_idx].modified : "Nov 25",
                         st->vfs[dir_idx].name);
                push_term_line(st, line, 0);
            } else {
                push_term_line(st, "ls: cannot access: No such file or directory", 3);
            }
        } else if (detailed) {
            int cnt = 0;
            for (int i = 0; i < st->vfs_count; ++i) {
                if (strcmp(st->vfs[i].parent, target_dir) == 0 && (show_all || st->vfs[i].name[0] != '.')) cnt++;
            }
            char hdr[32];
            snprintf(hdr, sizeof(hdr), "total %d", cnt * 4);
            push_term_line(st, hdr, 0);
            for (int i = 0; i < st->vfs_count; ++i) {
                if (strcmp(st->vfs[i].parent, target_dir) == 0) {
                    if (!show_all && st->vfs[i].name[0] == '.') continue;
                    char line[144];
                    snprintf(line, sizeof(line), "%crwxr-xr-x 1 nature nature %7d %-11s %s",
                             st->vfs[i].is_dir ? 'd' : '-', st->vfs[i].size_bytes,
                             st->vfs[i].modified[0] ? st->vfs[i].modified : "Nov 25",
                             st->vfs[i].name);
                    push_term_line(st, line, st->vfs[i].is_dir ? 2 : 0);
                }
            }
        } else {
            char line[160] = {0};
            for (int i = 0; i < st->vfs_count; ++i) {
                if (strcmp(st->vfs[i].parent, target_dir) == 0) {
                    if (!show_all && st->vfs[i].name[0] == '.') continue;
                    char item[56];
                    snprintf(item, sizeof(item), "%s%s  ", st->vfs[i].name, st->vfs[i].is_dir ? "/" : "");
                    if (strlen(line) + strlen(item) >= 96) {
                        push_term_line(st, line, 0);
                        line[0] = '\0';
                    }
                    strcat(line, item);
                }
            }
            push_term_line(st, line[0] ? line : "(empty)", 0);
        }
    } else if (strcmp(cmd, "cd") == 0) {
        if (arg1 && strcmp(arg1, "-") == 0) {
            char old[128];
            snprintf(old, sizeof(old), "%s", st->term_cwd);
            if (st->term_prev_cwd[0]) {
                snprintf(st->term_cwd, sizeof(st->term_cwd), "%s", st->term_prev_cwd);
                snprintf(st->term_prev_cwd, sizeof(st->term_prev_cwd), "%s", old);
                push_term_line(st, st->term_cwd, 0);
            } else {
                push_term_line(st, "bash: cd: OLDPWD not set", 3);
            }
        } else {
            char target[160];
            resolve_term_path(st, arg1, target, sizeof(target));
            int idx = find_vfs_by_path(st, target);
            if (strcmp(target, "/") == 0 || (idx >= 0 && st->vfs[idx].is_dir)) {
                snprintf(st->term_prev_cwd, sizeof(st->term_prev_cwd), "%s", st->term_cwd);
                snprintf(st->term_cwd, sizeof(st->term_cwd), "%.*s", (int)sizeof(st->term_cwd) - 1, target);
            } else if (idx >= 0) {
                push_term_line(st, "bash: cd: Not a directory", 3);
            } else {
                push_term_line(st, "bash: cd: No such directory", 3);
            }
        }
    } else if (strcmp(cmd, "mkdir") == 0 && arg1) {
        int parents = 0;
        int created = 0;
        for (int a = 1; a < argc; ++a) {
            if (argv[a][0] == '-') {
                if (strchr(argv[a], 'p')) parents = 1;
                continue;
            }
            char target[160];
            resolve_term_path(st, argv[a], target, sizeof(target));
            int rc = mkdir_recursive(st, target, parents);
            if (rc == -2) push_term_line(st, "mkdir: cannot create directory: File exists", 3);
            else if (rc == -3) push_term_line(st, "mkdir: cannot create directory: No such parent (use -p)", 3);
            else if (rc == -1) push_term_line(st, "mkdir: VFS node limit reached", 3);
            else created++;
        }
        if (created > 0) {
            push_term_line(st, "Directory created.", 2);
            nature_os_save_state(st);
        }
    } else if (strcmp(cmd, "rmdir") == 0 && arg1) {
        char target[160];
        resolve_term_path(st, arg1, target, sizeof(target));
        int idx = find_vfs_by_path(st, target);
        if (idx < 0) push_term_line(st, "rmdir: failed to remove: No such file or directory", 3);
        else if (!st->vfs[idx].is_dir) push_term_line(st, "rmdir: failed to remove: Not a directory", 3);
        else {
            int has_child = 0;
            for (int i = 0; i < st->vfs_count; ++i) {
                if (strcmp(st->vfs[i].parent, target) == 0) { has_child = 1; break; }
            }
            if (has_child) push_term_line(st, "rmdir: failed to remove: Directory not empty", 3);
            else {
                for (int j = idx; j < st->vfs_count - 1; ++j) st->vfs[j] = st->vfs[j + 1];
                st->vfs_count--;
                if (st->files_selected_idx == idx) st->files_selected_idx = -1;
                push_term_line(st, "Directory removed.", 2);
                nature_os_save_state(st);
            }
        }
    } else if (strcmp(cmd, "touch") == 0 && arg1) {
        char target[160];
        resolve_term_path(st, arg1, target, sizeof(target));
        int existing = find_vfs_by_path(st, target);
        if (existing >= 0) {
            snprintf(st->vfs[existing].modified, sizeof(st->vfs[existing].modified), "Just now");
            push_term_line(st, "Updated timestamp.", 2);
            nature_os_save_state(st);
        } else {
            int rc = upsert_vfs_file(st, target, "", 0);
            if (rc == -1) push_term_line(st, "touch: VFS node limit reached", 3);
            else push_term_line(st, "File created.", 2);
        }
    } else if (strcmp(cmd, "cat") == 0 && arg1) {
        char target[160];
        resolve_term_path(st, arg1, target, sizeof(target));
        int idx = find_vfs_by_path(st, target);
        if (idx < 0 || st->vfs[idx].is_dir) push_term_line(st, "cat: No such file", 3);
        else push_multiline_text(st, st->vfs[idx].content[0] ? st->vfs[idx].content : "[Binary media stream data]", 0, 0);
    } else if ((strcmp(cmd, "head") == 0 || strcmp(cmd, "tail") == 0) && arg1) {
        int is_tail = (strcmp(cmd, "tail") == 0);
        int limit = 10;
        const char *fname = arg1;
        if (strcmp(arg1, "-n") == 0 && arg2 && arg3) {
            limit = atoi(arg2);
            if (limit < 1) limit = 1;
            fname = arg3;
        } else if (arg1[0] == '-' && isdigit((unsigned char)arg1[1]) && arg2) {
            limit = atoi(arg1 + 1);
            if (limit < 1) limit = 1;
            fname = arg2;
        } else if (strcmp(arg1, "-n") == 0 && !arg3) {
            fname = NULL;
        }
        if (!fname) {
            push_term_line(st, "missing file operand", 3);
        } else {
            char target[160];
            resolve_term_path(st, fname, target, sizeof(target));
            int idx = find_vfs_by_path(st, target);
            if (idx < 0 || st->vfs[idx].is_dir) push_term_line(st, "No such file", 3);
            else push_multiline_text(st, st->vfs[idx].content[0] ? st->vfs[idx].content : "[Empty file]", limit, is_tail);
        }
    } else if (strcmp(cmd, "find") == 0) {
        char base[160];
        snprintf(base, sizeof(base), "%s", st->term_cwd);
        const char *pat = NULL;
        for (int a = 1; a < argc; ++a) {
            if (strcmp(argv[a], "-name") == 0 && a + 1 < argc) {
                pat = argv[++a];
            } else if (argv[a][0] != '-') {
                resolve_term_path(st, argv[a], base, sizeof(base));
            }
        }
        size_t blen = strlen(base);
        int found_cnt = 0;
        for (int i = 0; i < st->vfs_count; ++i) {
            int under = (strcmp(base, "/") == 0) ||
                        (strncmp(st->vfs[i].path, base, blen) == 0 &&
                         (st->vfs[i].path[blen] == '/' || st->vfs[i].path[blen] == '\0'));
            if (under && (!pat || str_ci_has(st->vfs[i].name, pat))) {
                push_term_line(st, st->vfs[i].path, st->vfs[i].is_dir ? 2 : 0);
                found_cnt++;
            }
        }
        if (found_cnt == 0) push_term_line(st, "(no matching paths)", 0);
    } else if ((strcmp(cmd, "sort") == 0 || strcmp(cmd, "uniq") == 0) && arg1) {
        int is_uniq = (strcmp(cmd, "uniq") == 0);
        char target[160];
        resolve_term_path(st, arg1, target, sizeof(target));
        int idx = find_vfs_by_path(st, target);
        if (idx < 0 || st->vfs[idx].is_dir) {
            push_term_line(st, "No such file", 3);
        } else {
            char tmp[768];
            snprintf(tmp, sizeof(tmp), "%s", st->vfs[idx].content);
            char *lines[64];
            int nlines = 0;
            char *p = tmp;
            while (*p && nlines < 64) {
                char *eol = strchr(p, '\n');
                lines[nlines++] = p;
                if (!eol) break;
                *eol = '\0';
                p = eol + 1;
            }
            if (!is_uniq) {
                for (int a = 0; a < nlines; ++a) {
                    for (int b = a + 1; b < nlines; ++b) {
                        if (strcmp(lines[a], lines[b]) > 0) {
                            char *t = lines[a]; lines[a] = lines[b]; lines[b] = t;
                        }
                    }
                }
            }
            for (int i = 0; i < nlines; ++i) {
                if (is_uniq && i > 0 && strcmp(lines[i], lines[i - 1]) == 0) continue;
                push_term_line(st, lines[i], 0);
            }
        }
    } else if (strcmp(cmd, "stat") == 0 && arg1) {
        char target[160];
        resolve_term_path(st, arg1, target, sizeof(target));
        int idx = find_vfs_by_path(st, target);
        if (idx < 0) {
            push_term_line(st, "stat: cannot statx: No such file or directory", 3);
        } else {
            const VfsNode *n = &st->vfs[idx];
            char s0[128], s1[128], s2[128];
            snprintf(s0, sizeof(s0), "  File: %s", n->path);
            snprintf(s1, sizeof(s1), "  Size: %-10d  Type: %s (%s)",
                     n->size_bytes, n->is_dir ? "directory" : "regular file", n->mime_tag[0] ? n->mime_tag : "TXT");
            snprintf(s2, sizeof(s2), "Modify: %s · Access: (0%s/rwxr-xr-x) Uid: (1000/nature)",
                     n->modified[0] ? n->modified : "Nov 25", n->is_dir ? "755" : "644");
            push_term_line(st, s0, 2);
            push_term_line(st, s1, 0);
            push_term_line(st, s2, 0);
        }
    } else if (strcmp(cmd, "cal") == 0) {
        time_t now = time(NULL);
        struct tm tmv = *localtime(&now);
        char hdr[64];
        strftime(hdr, sizeof(hdr), "     %B %Y", &tmv);
        push_term_line(st, hdr, 2);
        push_term_line(st, "Su Mo Tu We Th Fr Sa", 1);
        struct tm first = tmv;
        first.tm_mday = 1;
        mktime(&first);
        int start_wd = first.tm_wday;
        int dim = 31;
        int m = tmv.tm_mon + 1, y = tmv.tm_year + 1900;
        if (m == 4 || m == 6 || m == 9 || m == 11) dim = 30;
        else if (m == 2) dim = ((y % 4 == 0 && y % 100 != 0) || (y % 400 == 0)) ? 29 : 28;
        char row[64];
        int pos = 0;
        for (int s = 0; s < start_wd; ++s) pos += snprintf(row + pos, sizeof(row) - (size_t)pos, "   ");
        for (int d = 1; d <= dim; ++d) {
            if (d == tmv.tm_mday) pos += snprintf(row + pos, sizeof(row) - (size_t)pos, "[%d]", d);
            else pos += snprintf(row + pos, sizeof(row) - (size_t)pos, "%2d ", d);
            if ((start_wd + d) % 7 == 0 || d == dim) {
                push_term_line(st, row, 0);
                pos = 0;
                row[0] = '\0';
            }
        }
    } else if (strcmp(cmd, "calc") == 0) {
        if (!arg1 || !arg2 || !arg3) {
            push_term_line(st, "Usage: calc <a> <+|-|*|/|%> <b>", 3);
        } else {
            double a = atof(arg1), b = atof(arg3), res = 0.0;
            int ok = 1;
            if (strcmp(arg2, "+") == 0) res = a + b;
            else if (strcmp(arg2, "-") == 0) res = a - b;
            else if (strcmp(arg2, "*") == 0 || strcmp(arg2, "x") == 0) res = a * b;
            else if (strcmp(arg2, "/") == 0) {
                if (b == 0.0) { ok = 0; push_term_line(st, "calc: division by zero", 3); }
                else res = a / b;
            } else if (strcmp(arg2, "%") == 0) {
                long lb = (long)b;
                if (lb == 0) { ok = 0; push_term_line(st, "calc: modulo by zero", 3); }
                else res = (double)((long)a % lb);
            } else {
                ok = 0;
                push_term_line(st, "calc: unsupported operator (use + - * / %)", 3);
            }
            if (ok) {
                char out[96];
                if (res == (double)(long long)res) snprintf(out, sizeof(out), "= %lld", (long long)res);
                else snprintf(out, sizeof(out), "= %.6g", res);
                push_term_line(st, out, 2);
            }
        }
    } else if (strcmp(cmd, "open") == 0 && arg1) {
        if (strcasecmp(arg1, "files") == 0) { nature_open_app(st, APP_FILES); push_term_line(st, "Opened Files Manager.", 2); }
        else if (strcasecmp(arg1, "monitor") == 0) { nature_open_app(st, APP_MONITOR); push_term_line(st, "Opened System Monitor.", 2); }
        else if (strcasecmp(arg1, "browser") == 0) { nature_open_app(st, APP_BROWSER); push_term_line(st, "Opened Sylva Browser.", 2); }
        else if (strcasecmp(arg1, "settings") == 0) { nature_open_app(st, APP_SETTINGS); push_term_line(st, "Opened System Settings.", 2); }
        else if (strcasecmp(arg1, "store") == 0) { nature_open_app(st, APP_STORE); push_term_line(st, "Opened Ubuntu Software.", 2); }
        else if (strcasecmp(arg1, "media") == 0) { nature_open_app(st, APP_MEDIA); push_term_line(st, "Opened Media Viewer.", 2); }
        else {
            char target[160];
            resolve_term_path(st, arg1, target, sizeof(target));
            int idx = find_vfs_by_path(st, target);
            if (idx < 0) {
                push_term_line(st, "open: target not found", 3);
            } else if (st->vfs[idx].is_dir) {
                snprintf(st->files_cwd, sizeof(st->files_cwd), "%s", st->vfs[idx].path);
                nature_open_app(st, APP_FILES);
                push_term_line(st, "Opened directory in Files.", 2);
            } else if (st->vfs[idx].video_idx >= 0) {
                st->media_mode = 1;
                st->media_folder_idx = 2;
                st->video_selected = st->vfs[idx].video_idx % MAX_VIDEO_CLIPS;
                st->video_playing = 1;
                st->video_progress_ms = 0;
                nature_open_app(st, APP_MEDIA);
                push_term_line(st, "Playing video in Media Viewer.", 2);
            } else if (st->vfs[idx].image_idx >= 0) {
                st->media_mode = 0;
                st->media_folder_idx = 1;
                st->media_grid_mode = 0;
                st->media_selected = st->vfs[idx].image_idx;
                nature_open_app(st, APP_MEDIA);
                push_term_line(st, "Opened image in Media Viewer.", 2);
            } else if (st->vfs[idx].audio_idx >= 0) {
                st->media_mode = 2;
                st->media_folder_idx = 3;
                st->audio_selected = st->vfs[idx].audio_idx;
                st->audio_playing = 1;
                st->audio_progress_sec = 0;
                nature_open_app(st, APP_MEDIA);
                push_term_line(st, "Playing audio track in Media Viewer.", 2);
            } else {
                st->files_viewer_open = 1;
                st->files_viewer_idx = idx;
                st->files_selected_idx = idx;
                snprintf(st->files_cwd, sizeof(st->files_cwd), "%s", st->vfs[idx].parent);
                nature_open_app(st, APP_FILES);
                push_term_line(st, "Opened file in Files viewer.", 2);
            }
        }
    } else if (strcmp(cmd, "rm") == 0 && arg1) {
        int rec = 0, force = 0;
        const char *path_arg = NULL;
        for (int a = 1; a < argc; ++a) {
            if (argv[a][0] == '-') {
                if (strchr(argv[a], 'r') || strchr(argv[a], 'R')) rec = 1;
                if (strchr(argv[a], 'f')) force = 1;
            } else {
                path_arg = argv[a];
            }
        }
        if (!path_arg) {
            push_term_line(st, "rm: missing operand", 3);
        } else {
            char target[160];
            resolve_term_path(st, path_arg, target, sizeof(target));
            int idx = find_vfs_by_path(st, target);
            if (idx < 0) {
                if (!force) push_term_line(st, "rm: cannot remove: No such file or directory", 3);
            } else if (st->vfs[idx].is_dir && !rec) {
                push_term_line(st, "rm: cannot remove directory (use -r or rmdir)", 3);
            } else if (strcmp(target, "/") == 0 || strcmp(target, "/home/nature") == 0) {
                push_term_line(st, "rm: refusing to remove protected root/home directory", 3);
            } else if (st->vfs[idx].is_dir && rec) {
                size_t tlen = strlen(target);
                int removed = 0;
                for (int i = st->vfs_count - 1; i >= 0; --i) {
                    if (strcmp(st->vfs[i].path, target) == 0 ||
                        (strncmp(st->vfs[i].path, target, tlen) == 0 && st->vfs[i].path[tlen] == '/')) {
                        for (int j = i; j < st->vfs_count - 1; ++j) st->vfs[j] = st->vfs[j + 1];
                        st->vfs_count--;
                        removed++;
                    }
                }
                st->files_selected_idx = -1;
                char msg[96];
                snprintf(msg, sizeof(msg), "Recursively removed %d item(s).", removed);
                push_term_line(st, msg, 2);
                nature_os_save_state(st);
            } else {
                for (int j = idx; j < st->vfs_count - 1; ++j) st->vfs[j] = st->vfs[j + 1];
                st->vfs_count--;
                if (st->files_selected_idx == idx) st->files_selected_idx = -1;
                push_term_line(st, "Removed file cleanly.", 2);
                nature_os_save_state(st);
            }
        }
    } else if ((strcmp(cmd, "cp") == 0 || strcmp(cmd, "mv") == 0) && arg1 && arg2) {
        char src_p[160], dst_p[160];
        resolve_term_path(st, arg1, src_p, sizeof(src_p));
        resolve_term_path(st, arg2, dst_p, sizeof(dst_p));
        int sidx = find_vfs_by_path(st, src_p);
        if (sidx < 0 || st->vfs[sidx].is_dir) {
            push_term_line(st, "cp/mv: source file not found", 3);
        } else {
            int didx = find_vfs_by_path(st, dst_p);
            if (didx >= 0 && st->vfs[didx].is_dir) {
                char joined[192];
                snprintf(joined, sizeof(joined), "%s/%s", strcmp(dst_p, "/") == 0 ? "" : dst_p, st->vfs[sidx].name);
                snprintf(dst_p, sizeof(dst_p), "%.159s", joined);
            }
            if (strcmp(cmd, "cp") == 0) {
                if (st->vfs_count >= MAX_VFS) {
                    push_term_line(st, "cp: VFS node limit reached", 3);
                } else {
                    VfsNode copy = st->vfs[sidx];
                    split_parent_name(dst_p, copy.parent, sizeof(copy.parent), copy.name, sizeof(copy.name));
                    snprintf(copy.path, sizeof(copy.path), "%.127s", dst_p);
                    snprintf(copy.modified, sizeof(copy.modified), "Just now");
                    st->vfs[st->vfs_count++] = copy;
                    push_term_line(st, "File copied.", 2);
                    nature_os_save_state(st);
                }
            } else {
                split_parent_name(dst_p, st->vfs[sidx].parent, sizeof(st->vfs[sidx].parent),
                                  st->vfs[sidx].name, sizeof(st->vfs[sidx].name));
                snprintf(st->vfs[sidx].path, sizeof(st->vfs[sidx].path), "%.127s", dst_p);
                snprintf(st->vfs[sidx].modified, sizeof(st->vfs[sidx].modified), "Just now");
                push_term_line(st, "File moved/renamed.", 2);
                nature_os_save_state(st);
            }
        }
    } else if (strcmp(cmd, "grep") == 0 && arg1) {
        int show_lno = 1, count_only = 0;
        const char *pat = NULL, *fname = NULL;
        for (int a = 1; a < argc; ++a) {
            if (argv[a][0] == '-') {
                if (strchr(argv[a], 'n')) show_lno = 1;
                if (strchr(argv[a], 'c')) count_only = 1;
            } else if (!pat) {
                pat = argv[a];
            } else {
                fname = argv[a];
            }
        }
        if (!pat || !fname) {
            push_term_line(st, "Usage: grep [-i|-n|-c] <pattern> <file>", 3);
        } else {
            char target[160];
            resolve_term_path(st, fname, target, sizeof(target));
            int idx = find_vfs_by_path(st, target);
            int hits = 0;
            size_t nlen = strlen(pat);
            if (idx < 0 || st->vfs[idx].is_dir) {
                push_term_line(st, "grep: No such file", 3);
            } else {
                const char *p = st->vfs[idx].content;
                int lno = 1;
                while (*p) {
                    const char *eol = strchr(p, '\n');
                    size_t len = eol ? (size_t)(eol - p) : strlen(p);
                    int matched = 0;
                    if (nlen <= len) {
                        for (size_t k = 0; k + nlen <= len; ++k) {
                            size_t m = 0;
                            while (m < nlen && tolower((unsigned char)p[k + m]) == tolower((unsigned char)pat[m])) m++;
                            if (m == nlen) { matched = 1; break; }
                        }
                    }
                    if (matched) {
                        hits++;
                        if (!count_only) {
                            char gline[160];
                            size_t cpy = len < 130 ? len : 130;
                            if (show_lno) snprintf(gline, sizeof(gline), "%3d: %.*s", lno, (int)cpy, p);
                            else snprintf(gline, sizeof(gline), "%.*s", (int)cpy, p);
                            push_term_line(st, gline, 2);
                        }
                    }
                    lno++;
                    if (!eol) break;
                    p = eol + 1;
                }
                if (count_only) {
                    char cbuf[32];
                    snprintf(cbuf, sizeof(cbuf), "%d", hits);
                    push_term_line(st, cbuf, 2);
                } else if (hits == 0) {
                    push_term_line(st, "(no matches)", 0);
                }
            }
        }
    } else if (strcmp(cmd, "history") == 0) {
        if (st->term_hist_count == 0) {
            push_term_line(st, "(history empty)", 0);
        } else {
            for (int i = 0; i < st->term_hist_count; ++i) {
                char hline[160];
                snprintf(hline, sizeof(hline), " %2d  %.144s", i + 1, st->term_history[i]);
                push_term_line(st, hline, 0);
            }
        }
    } else if (strcmp(cmd, "wc") == 0 && arg1) {
        int fl = 0, fw = 0, fc = 0;
        const char *fname = NULL;
        for (int a = 1; a < argc; ++a) {
            if (argv[a][0] == '-') {
                if (strchr(argv[a], 'l')) fl = 1;
                if (strchr(argv[a], 'w')) fw = 1;
                if (strchr(argv[a], 'c')) fc = 1;
            } else {
                fname = argv[a];
            }
        }
        if (!fname) {
            push_term_line(st, "wc: missing file operand", 3);
        } else {
            char target[160];
            resolve_term_path(st, fname, target, sizeof(target));
            int idx = find_vfs_by_path(st, target);
            if (idx < 0 || st->vfs[idx].is_dir) {
                push_term_line(st, "wc: No such file", 3);
            } else {
                const char *s = st->vfs[idx].content;
                int lines = s[0] ? 1 : 0, words = 0, in_word = 0, bytes = (int)strlen(s);
                for (int k = 0; s[k]; ++k) {
                    if (s[k] == '\n') lines++;
                    if (isspace((unsigned char)s[k])) in_word = 0;
                    else if (!in_word) { in_word = 1; words++; }
                }
                char wbuf[128];
                if (fl && !fw && !fc) snprintf(wbuf, sizeof(wbuf), "%d %.48s", lines, st->vfs[idx].name);
                else if (fw && !fl && !fc) snprintf(wbuf, sizeof(wbuf), "%d %.48s", words, st->vfs[idx].name);
                else if (fc && !fl && !fw) snprintf(wbuf, sizeof(wbuf), "%d %.48s", bytes, st->vfs[idx].name);
                else snprintf(wbuf, sizeof(wbuf), "  %4d lines  %5d words  %6d bytes  %.48s", lines, words, bytes, st->vfs[idx].name);
                push_term_line(st, wbuf, 2);
            }
        }
    } else if (strcmp(cmd, "free") == 0) {
        char m[128], s[128];
        int free_mb = st->total_ram_mb - st->used_ram_mb - st->cached_ram_mb;
        if (free_mb < 0) free_mb = st->avail_ram_mb / 2;
        push_term_line(st, "               total        used        free   buff/cache   available", 0);
        snprintf(m, sizeof(m), "Mem:          %5d M     %5d M     %5d M      %5d M     %5d M%s",
                 st->total_ram_mb, st->used_ram_mb, free_mb, st->cached_ram_mb, st->avail_ram_mb,
                 st->cgroup_limited ? " (cg2)" : "");
        push_term_line(st, m, 2);
        int sw_tot = st->swap_total_mb > 0 ? st->swap_total_mb : (st->cfg.zram_enabled ? st->total_ram_mb / 2 : 0);
        int sw_use = st->swap_total_mb > 0 ? st->swap_used_mb : (st->cfg.zram_enabled ? st->zram_used_mb : 0);
        int sw_free = sw_tot > sw_use ? sw_tot - sw_use : 0;
        snprintf(s, sizeof(s), "Swap/zram:    %5d M     %5d M     %5d M  [nature RSS %dM]",
                 sw_tot, sw_use, sw_free, st->os_rss_mb);
        push_term_line(st, s, 2);
    } else if (strcmp(cmd, "htop") == 0 || strcmp(cmd, "top") == 0) {
        push_term_line(st, "htop 3.3.0 - (Nature OS live /proc telemetry)", 2);
        int ncores = st->cpu_count > 0 ? st->cpu_count : 4;
        if (ncores > 8) ncores = 8;
        char c0[128], c1[128];
        if (ncores <= 4) {
            snprintf(c0, sizeof(c0), "  C0[%4.1f%%] C1[%4.1f%%]   Mem: %dM/%dM · OS RSS: %dM",
                     st->core_usage[0], st->core_usage[1], st->used_ram_mb, st->total_ram_mb, st->os_rss_mb);
            snprintf(c1, sizeof(c1), "  C2[%4.1f%%] C3[%4.1f%%]   Load: %.2f %.2f · %.1fms frame",
                     st->core_usage[2], st->core_usage[3], st->load_avg[0], st->load_avg[1], st->frame_ms);
        } else {
            snprintf(c0, sizeof(c0), "  C0:%3.0f%% C1:%3.0f%% C2:%3.0f%% C3:%3.0f%%  Mem: %dM/%dM · RSS %dM",
                     st->core_usage[0], st->core_usage[1], st->core_usage[2], st->core_usage[3],
                     st->used_ram_mb, st->total_ram_mb, st->os_rss_mb);
            snprintf(c1, sizeof(c1), "  C4:%3.0f%% C5:%3.0f%% C6:%3.0f%% C7:%3.0f%%  Load: %.2f · Net %.0f/%.0f KB/s",
                     st->core_usage[4], st->core_usage[5], st->core_usage[6], st->core_usage[7],
                     st->load_avg[0], st->net_rx_kbps, st->net_tx_kbps);
        }
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
        const char *host = st->cfg.device_name[0] ? st->cfg.device_name : "nature-pc";
        char l0[128], l1[128], l2[128], l3[128], l4[128], l5[128];
        snprintf(l0, sizeof(l0), "        &&&            nature@%.31s", host);
        snprintf(l1, sizeof(l1), "      &&&&&&&          ----------------");
        snprintf(l2, sizeof(l2), "     &&&&&&&&&         OS: Nature OS 24.04.1 LTS (Ubuntu Noble)");
        snprintf(l3, sizeof(l3), "      |||||||          Kernel: %.48s · %d Cores", st->kernel_ver, st->cpu_count > 0 ? st->cpu_count : 4);
        snprintf(l4, sizeof(l4), "       |||||           Compositor: %dx%d @ %.0f FPS · Node v22.12.0",
                 st->screen_w, st->screen_h, st->measured_fps);
        snprintf(l5, sizeof(l5), "        |||            Memory: %d MiB / %d MiB%s (OS RSS %d MiB)",
                 st->used_ram_mb, st->total_ram_mb, st->cgroup_limited ? " cg2" : "", st->os_rss_mb);
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
        } else if (strcmp(arg1, "update") == 0) {
            st->store_sync_count++;
            push_term_line(st, "Hit:1 http://archive.ubuntu.com/ubuntu noble InRelease", 0);
            push_term_line(st, "Hit:2 http://security.ubuntu.com/ubuntu noble-security InRelease", 0);
            push_term_line(st, "Reading package lists... Done (1,412 packages up to date).", 2);
        } else if (strcmp(arg1, "upgrade") == 0) {
            st->settings_update_checked = 1;
            push_term_line(st, "Calculating upgrade... Done. 0 upgraded, 0 newly installed.", 2);
        } else if (strcmp(arg1, "search") == 0 && arg2) {
            int hits = 0;
            for (int i = 0; i < st->pkg_count; ++i) {
                if (str_ci_has(st->pkgs[i].name, arg2) || str_ci_has(st->pkgs[i].display_name, arg2) ||
                    str_ci_has(st->pkgs[i].summary, arg2)) {
                    char pl[128];
                    snprintf(pl, sizeof(pl), "%s/noble %s [%s]", st->pkgs[i].name, st->pkgs[i].version,
                             st->pkgs[i].installed ? "installed" : "available");
                    push_term_line(st, pl, 2);
                    hits++;
                }
            }
            if (!hits) push_term_line(st, "(no matching packages)", 0);
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
            int ok = 0;
            for (int i = 0; i < st->pkg_count; ++i) {
                if (strcmp(st->pkgs[i].name, arg2) == 0) {
                    st->pkgs[i].installed = 0;
                    ok = 1;
                    push_term_line(st, "Package removed via dpkg.", 0);
                    nature_os_save_state(st);
                    break;
                }
            }
            if (!ok) push_term_line(st, "E: Package not found", 3);
        } else {
            push_term_line(st, "Usage: apt [list|update|upgrade|search <q>|install <p>|remove <p>]", 3);
        }
    } else if (strcmp(cmd, "systemctl") == 0) {
        if (!arg1 || strcmp(arg1, "list-units") == 0) {
            push_term_line(st, "nature-session.service    active (running)   CPUWeight=300", 2);
            push_term_line(st, st->svc_pipewire ? "  pipewire.service        active (running)" : "  pipewire.service        inactive (dead)", st->svc_pipewire ? 0 : 3);
            push_term_line(st, st->svc_network  ? "  NetworkManager.service  active (running)" : "  NetworkManager.service  inactive (dead)", st->svc_network ? 0 : 3);
            push_term_line(st, st->svc_oomd     ? "  systemd-oomd.service    active (monitoring)" : "  systemd-oomd.service    inactive (dead)", st->svc_oomd ? 0 : 3);
        } else {
            const char *act = arg1;
            const char *unit = arg2 ? arg2 : "";
            int *target_svc = NULL;
            const char *canon = NULL;
            if (str_ci_has(unit, "pipewire") || str_ci_has(act, "pipewire")) { target_svc = &st->svc_pipewire; canon = "pipewire.service"; }
            else if (str_ci_has(unit, "network") || str_ci_has(act, "network")) { target_svc = &st->svc_network; canon = "NetworkManager.service"; }
            else if (str_ci_has(unit, "oomd") || str_ci_has(act, "oomd")) { target_svc = &st->svc_oomd; canon = "systemd-oomd.service"; }

            if (!target_svc) {
                push_term_line(st, "Usage: systemctl [status|start|stop|restart] <pipewire|NetworkManager|systemd-oomd>", 3);
            } else if (strcmp(act, "start") == 0 || strcmp(act, "restart") == 0) {
                *target_svc = 1;
                if (target_svc == &st->svc_network) st->cfg.wifi_enabled = 1;
                char msg[96];
                snprintf(msg, sizeof(msg), "%s %s: active (running).",
                         strcmp(act, "restart") == 0 ? "Restarted" : "Started", canon);
                push_term_line(st, msg, 2);
                nature_os_save_state(st);
            } else if (strcmp(act, "stop") == 0) {
                *target_svc = 0;
                char msg[96];
                snprintf(msg, sizeof(msg), "Stopped %s: inactive (dead).", canon);
                push_term_line(st, msg, 3);
                nature_os_save_state(st);
            } else {
                char msg[96];
                snprintf(msg, sizeof(msg), "* %s - Loaded · Active: %s",
                         canon, *target_svc ? "active (running)" : "inactive (dead)");
                push_term_line(st, msg, *target_svc ? 2 : 3);
            }
        }
    } else if (strcmp(cmd, "stress") == 0) {
        if (arg1 && strcmp(arg1, "--stop") == 0) {
            nature_monitor_set_stress(st, 0);
            push_term_line(st, "stress: stopped background C++ worker threads.", 2);
        } else {
            int thr = st->cpu_count > 4 ? 4 : (st->cpu_count > 0 ? st->cpu_count : 4);
            if (arg1 && strcmp(arg1, "--cpu") == 0 && arg2) thr = atoi(arg2);
            if (thr >= 1 && thr <= 8) nature_monitor_set_threads(st, thr);
            nature_monitor_set_stress(st, 1);
            char smsg[128];
            snprintf(smsg, sizeof(smsg), "stress: spawned %d worker thread(s) (nice +10); compositor locked 60 FPS.",
                     st->stress_threads);
            push_term_line(st, smsg, 2);
        }
    } else if (strcmp(cmd, "node") == 0) {
        if (!arg1) {
            push_term_line(st, "Node.js v22.12.0 on Nature OS (try: node -v, node -e \"console.log(6*7)\", node ~/Projects/server.js)", 2);
        } else if (strcmp(arg1, "-v") == 0 || strcmp(arg1, "--version") == 0) {
            push_term_line(st, "v22.12.0", 2);
        } else if (strcmp(arg1, "--help") == 0 || strcmp(arg1, "-h") == 0) {
            push_term_line(st, "Usage: node [-v|--version] [-e \"code\"] [-p \"expr\"] [<file.js>]", 2);
        } else if ((strcmp(arg1, "-e") == 0 || strcmp(arg1, "-p") == 0) && arg2) {
            eval_node_script(st, arg2, strcmp(arg1, "-p") == 0);
        } else {
            char target[160];
            resolve_term_path(st, arg1, target, sizeof(target));
            int idx = find_vfs_by_path(st, target);
            if (idx < 0 || st->vfs[idx].is_dir) {
                char err[208];
                snprintf(err, sizeof(err), "Error: Cannot find module '%s'", target);
                push_term_line(st, err, 3);
            } else {
                eval_node_script(st, st->vfs[idx].content, 0);
            }
        }
    } else if (strcmp(cmd, "npm") == 0) {
        if (!arg1) {
            push_term_line(st, "npm@10.9.0 (commands: npm -v, npm init -y, npm list, npm run <script>, npm start, npm install <pkg>)", 2);
        } else if (strcmp(arg1, "-v") == 0 || strcmp(arg1, "--version") == 0) {
            push_term_line(st, "10.9.0", 2);
        } else if (strcmp(arg1, "init") == 0) {
            char ppath[160];
            snprintf(ppath, sizeof(ppath), "%s/package.json", strcmp(st->term_cwd, "/") == 0 ? "" : st->term_cwd);
            const char *slash = strrchr(st->term_cwd, '/');
            const char *pname = (slash && slash[1]) ? slash + 1 : "nature-app";
            char pkg_json[384];
            snprintf(pkg_json, sizeof(pkg_json),
                     "{\n  \"name\": \"%s\",\n  \"version\": \"1.0.0\",\n  \"main\": \"server.js\",\n  \"scripts\": {\n    \"start\": \"node server.js\",\n    \"test\": \"node -e \\\"console.log('tests passed')\\\"\"\n  }\n}",
                     pname);
            upsert_vfs_file(st, ppath, pkg_json, 0);
            char m[192];
            snprintf(m, sizeof(m), "Wrote to %s", ppath);
            push_term_line(st, m, 2);
        } else if (strcmp(arg1, "list") == 0 || strcmp(arg1, "ls") == 0) {
            int pidx = find_package_json(st);
            push_term_line(st, pidx >= 0 ? st->vfs[pidx].path : st->term_cwd, 2);
            push_term_line(st, "+-- nodejs@22.12.0", 0);
            push_term_line(st, "+-- typescript@5.8.2", 0);
            push_term_line(st, "`-- vite@6.2.0", 0);
        } else if (strcmp(arg1, "start") == 0 || strcmp(arg1, "test") == 0 || (strcmp(arg1, "run") == 0 && arg2)) {
            const char *sname = strcmp(arg1, "run") == 0 ? arg2 : arg1;
            int pidx = find_package_json(st);
            if (pidx < 0) {
                push_term_line(st, "npm ERR! Missing package.json in workspace", 3);
            } else {
                char key[64];
                snprintf(key, sizeof(key), "\"%s\"", sname);
                const char *found = strstr(st->vfs[pidx].content, key);
                if (!found) {
                    char err[96];
                    snprintf(err, sizeof(err), "npm ERR! Missing script: \"%s\"", sname);
                    push_term_line(st, err, 3);
                } else {
                    const char *col = strchr(found + strlen(key), ':');
                    const char *q1 = col ? strchr(col, '"') : NULL;
                    const char *q2 = q1 ? strchr(q1 + 1, '"') : NULL;
                    if (q1 && q2) {
                        char scmd[160];
                        size_t sl = (size_t)(q2 - (q1 + 1));
                        if (sl >= sizeof(scmd)) sl = sizeof(scmd) - 1;
                        memcpy(scmd, q1 + 1, sl);
                        scmd[sl] = '\0';
                        char banner[192];
                        snprintf(banner, sizeof(banner), "> %s: %s", sname, scmd);
                        push_term_line(st, banner, 2);
                        /* Temporarily resolve script relative to package.json folder */
                        char saved_cwd[128];
                        snprintf(saved_cwd, sizeof(saved_cwd), "%s", st->term_cwd);
                        snprintf(st->term_cwd, sizeof(st->term_cwd), "%s", st->vfs[pidx].parent);
                        exec_single_cmd(st, scmd);
                        snprintf(st->term_cwd, sizeof(st->term_cwd), "%s", saved_cwd);
                    }
                }
            }
        } else if ((strcmp(arg1, "install") == 0 || strcmp(arg1, "i") == 0) && arg2) {
            char msg[128];
            snprintf(msg, sizeof(msg), "added 1 package (%s) in 0.42s — 0 vulnerabilities", arg2);
            push_term_line(st, msg, 2);
        } else {
            push_term_line(st, "Usage: npm [-v|init [-y]|list|run <script>|start|test|install <pkg>]", 3);
        }
    } else if (strcmp(cmd, "npx") == 0) {
        if (!arg1) push_term_line(st, "Usage: npx <command> [args]", 3);
        else if (strcmp(arg1, "cowsay") == 0) {
            const char *msg = arg2 ? arg2 : "Nature OS Node v22";
            char top[96];
            snprintf(top, sizeof(top), "< %s >", msg);
            push_term_line(st, " ----------------", 2);
            push_term_line(st, top, 2);
            push_term_line(st, " ----------------", 2);
            push_term_line(st, "   \\   ^__^", 0);
            push_term_line(st, "    \\  (oo)\\_______", 0);
            push_term_line(st, "       (__)\\       )\\/\\", 0);
        } else {
            char out[128];
            snprintf(out, sizeof(out), "npx: executed '%s' via node v22.12.0", arg1);
            push_term_line(st, out, 2);
        }
    } else if (strcmp(cmd, "python3") == 0) {
        if (!arg1 || strcmp(arg1, "-V") == 0 || strcmp(arg1, "--version") == 0) {
            push_term_line(st, "Python 3.12.3", 2);
        } else if (strcmp(arg1, "-c") == 0 && arg2) {
            eval_python_script(st, arg2);
        } else {
            char target[160];
            resolve_term_path(st, arg1, target, sizeof(target));
            int idx = find_vfs_by_path(st, target);
            if (idx < 0 || st->vfs[idx].is_dir) {
                char err[216];
                snprintf(err, sizeof(err), "python3: can't open file '%s': No such file", target);
                push_term_line(st, err, 3);
            } else {
                eval_python_script(st, st->vfs[idx].content);
            }
        }
    } else if (strcmp(cmd, "git") == 0) {
        if (!arg1 || strcmp(arg1, "--version") == 0) {
            push_term_line(st, "git version 2.43.0 (Nature OS)", 2);
        } else if (strcmp(arg1, "branch") == 0) {
            push_term_line(st, "* main", 2);
        } else if (strcmp(arg1, "init") == 0) {
            char gdir[160];
            snprintf(gdir, sizeof(gdir), "%s/.git", strcmp(st->term_cwd, "/") == 0 ? "" : st->term_cwd);
            mkdir_recursive(st, gdir, 1);
            push_term_line(st, "Initialized empty Git repository in .git/", 2);
            nature_os_save_state(st);
        } else if (strcmp(arg1, "log") == 0) {
            push_term_line(st, "commit 8c4e1a2 (HEAD -> main) Upgrade Node/Media/Settings apps", 2);
            push_term_line(st, "commit 5b29f01 Add Nature Horizon theme & compositor optimizations", 0);
            push_term_line(st, "commit 1d04c88 Initial Nature OS 24.04 LTS kernel & shell tree", 0);
        } else if (strcmp(arg1, "status") == 0) {
            push_term_line(st, "On branch main", 2);
            int changes = 0;
            for (int i = 0; i < st->vfs_count; ++i) {
                if (!st->vfs[i].is_dir && strcmp(st->vfs[i].modified, "Just now") == 0) {
                    char l[140];
                    snprintf(l, sizeof(l), "  modified / new:  %s", st->vfs[i].path);
                    push_term_line(st, l, 2);
                    changes++;
                }
            }
            if (!changes) push_term_line(st, "nothing to commit, working tree clean", 0);
        } else {
            push_term_line(st, "Usage: git [--version|status|branch|log|init]", 3);
        }
    } else if (strcmp(cmd, "curl") == 0) {
        if (!arg1 || strcmp(arg1, "-h") == 0 || strcmp(arg1, "--help") == 0) {
            push_term_line(st, "Usage: curl [options...] <url>", 2);
            push_term_line(st, "Options: -V/--version, -I/--head, -O (save file)", 0);
        } else if (strcmp(arg1, "-V") == 0 || strcmp(arg1, "--version") == 0) {
            push_term_line(st, "curl 8.5.0 (x86_64-pc-linux-gnu) libcurl/8.5.0 OpenSSL/3.0.13 zlib/1.3", 2);
            push_term_line(st, "Protocols: dict file ftp ftps gopher http https imap pop3 rtsp smb smtp", 0);
        } else if (strcmp(arg1, "-I") == 0 || strcmp(arg1, "--head") == 0) {
            const char *target = arg2 ? arg2 : "https://nature.os";
            push_term_line(st, "HTTP/2 200 OK", 2);
            push_term_line(st, "server: NatureOS-WebServer/24.04", 0);
            push_term_line(st, "content-type: application/json; charset=utf-8", 0);
            push_term_line(st, "date: Thu, 26 Nov 2026 12:00:00 GMT", 0);
            push_term_line(st, "cache-control: max-age=3600", 0);
            char v[180];
            snprintf(v, sizeof(v), "x-served-for: %s", target);
            push_term_line(st, v, 0);
        } else if (strcmp(arg1, "-O") == 0 && arg2) {
            const char *slash = strrchr(arg2, '/');
            const char *fname = (slash && slash[1]) ? slash + 1 : "downloaded_file.txt";
            char out_p[160];
            snprintf(out_p, sizeof(out_p), "%s/%s", strcmp(st->term_cwd, "/") == 0 ? "" : st->term_cwd, fname);
            upsert_vfs_file(st, out_p, "Fetched from Nature OS Web Gateway\n{\n  \"source\": \"curl\",\n  \"status\": 200\n}\n", 0);
            push_term_line(st, "% Total    % Received % Xferd  Average Speed   Time    Time     Time  Current", 0);
            push_term_line(st, "100   320  100   320    0     0   4820      0 --:--:-- --:--:-- --:--:--  4890", 2);
            char ok[192];
            snprintf(ok, sizeof(ok), "curl: Saved '%s' in current directory", fname);
            push_term_line(st, ok, 2);
            nature_os_save_state(st);
        } else if (strstr(arg1, "wttr.in") != NULL) {
            push_term_line(st, "Weather report: Nature Valley, Alpine Ridge", 2);
            push_term_line(st, "   \\  /       Partly cloudy", 2);
            push_term_line(st, " _ /\"\".-.     18 °C · Feels like 17 °C", 2);
            push_term_line(st, "   \\_(   ).   Wind: 12 km/h SSE", 2);
            push_term_line(st, "   /(___(__)  Visibility: 10 km · Humidity: 64%", 2);
            push_term_line(st, "              Precipitation: 0.0 mm / 3h", 0);
        } else if (strstr(arg1, "cheat.sh") != NULL) {
            if (strstr(arg1, "node") != NULL) {
                push_term_line(st, "# Node.js Quick Reference", 2);
                push_term_line(st, "node -v                    # Print version", 0);
                push_term_line(st, "node -e 'console.log(...)' # Eval JS code", 0);
                push_term_line(st, "node script.js             # Run file", 0);
                push_term_line(st, "npm run start              # Run package script", 0);
            } else if (strstr(arg1, "python") != NULL) {
                push_term_line(st, "# Python 3 Quick Reference", 2);
                push_term_line(st, "python3 -V                 # Print version", 0);
                push_term_line(st, "python3 -c 'print(...)'    # Eval Python statement", 0);
                push_term_line(st, "python3 app.py             # Run script", 0);
            } else {
                push_term_line(st, "# Linux Shell Quick Reference", 2);
                push_term_line(st, "systemctl status pipewire  # Inspect audio service", 0);
                push_term_line(st, "neofetch                   # Display distro stats", 0);
                push_term_line(st, "htop                       # Interactive process view", 0);
            }
        } else if (strstr(arg1, "api") != NULL || strstr(arg1, "health") != NULL) {
            push_term_line(st, "{\"status\":\"ok\",\"distro\":\"Nature OS 24.04 LTS\",\"kernel\":\"6.8.0-45-lowlatency\",\"compositor\":\"nature-compositor\",\"services\":{\"pipewire\":\"active\",\"NetworkManager\":\"active\",\"systemd-oomd\":\"active\"}}", 2);
        } else {
            char m[192];
            snprintf(m, sizeof(m), "HTTP/2 200 OK <!DOCTYPE html><html><body>Nature OS: Connected to %s</body></html>", arg1);
            push_term_line(st, m, 2);
        }
    } else if (strcmp(cmd, "wget") == 0) {
        if (!arg1 || strcmp(arg1, "-h") == 0 || strcmp(arg1, "--help") == 0) {
            push_term_line(st, "GNU Wget 1.21.4 (Usage: wget <url>)", 2);
        } else if (strcmp(arg1, "-V") == 0 || strcmp(arg1, "--version") == 0) {
            push_term_line(st, "GNU Wget 1.21.4 built on linux-gnu.", 2);
        } else {
            const char *slash = strrchr(arg1, '/');
            const char *fname = (slash && slash[1]) ? slash + 1 : "index.html";
            char out_p[160];
            snprintf(out_p, sizeof(out_p), "%s/%s", strcmp(st->term_cwd, "/") == 0 ? "" : st->term_cwd, fname);
            upsert_vfs_file(st, out_p, "<!-- Downloaded with GNU Wget on Nature OS -->\n<!DOCTYPE html>\n<html>\n<head><title>Nature OS</title></head>\n<body><h1>Nature OS 24.04 LTS</h1></body>\n</html>\n", 0);
            char b1[192], b2[192];
            snprintf(b1, sizeof(b1), "--2026-11-26 12:00:00--  %s", arg1);
            push_term_line(st, b1, 0);
            push_term_line(st, "Resolving host... 127.0.0.1 (connected)", 0);
            push_term_line(st, "HTTP request sent, awaiting response... 200 OK", 0);
            push_term_line(st, "Length: 168 [text/html] - Saving to: output", 2);
            push_term_line(st, "100%[=====================================>] 168   --.-KB/s  in 0.001s", 2);
            snprintf(b2, sizeof(b2), "2026-11-26 12:00:00 (3.24 MB/s) - '%s' saved [168/168]", fname);
            push_term_line(st, b2, 2);
            nature_os_save_state(st);
        }
    } else if (strcmp(cmd, "nano") == 0) {
        if (!arg1) {
            push_term_line(st, "  GNU nano 7.2                     New Buffer", 2);
            push_term_line(st, "----------------------------------------------------------------", 0);
            push_term_line(st, "  [ Empty file buffer. Type 'echo \"...\" > <filename>' to save. ]", 2);
            push_term_line(st, "----------------------------------------------------------------", 0);
            push_term_line(st, "^G Get Help   ^O WriteOut   ^R Read File   ^X Exit", 0);
        } else {
            char target[160];
            resolve_term_path(st, arg1, target, sizeof(target));
            int idx = find_vfs_by_path(st, target);
            char hdr[224];
            snprintf(hdr, sizeof(hdr), "  GNU nano 7.2                     File: %s", target);
            push_term_line(st, hdr, 2);
            push_term_line(st, "----------------------------------------------------------------", 0);
            if (idx >= 0 && !st->vfs[idx].is_dir) {
                const char *p = st->vfs[idx].content;
                int lcnt = 0;
                while (*p && lcnt < 10) {
                    const char *nl = strchr(p, '\n');
                    char line[150];
                    size_t len = nl ? (size_t)(nl - p) : strlen(p);
                    if (len >= sizeof(line)) len = sizeof(line) - 1;
                    memcpy(line, p, len);
                    line[len] = '\0';
                    push_term_line(st, line, 0);
                    lcnt++;
                    if (!nl) break;
                    p = nl + 1;
                }
                push_term_line(st, "----------------------------------------------------------------", 0);
                char info[180];
                snprintf(info, sizeof(info), "[ Read %d lines from %s ] (open in Files to edit)", lcnt, arg1);
                push_term_line(st, info, 2);
            } else {
                push_term_line(st, "  [ New File ]", 2);
                push_term_line(st, "----------------------------------------------------------------", 0);
                push_term_line(st, "[ New buffer created ] ^G Help  ^O Write  ^X Exit", 2);
            }
        }
    } else {
        push_term_line(st, "bash: command not found (type 'help' or 'man <cmd>')", 3);
    }
}

void nature_terminal_exec(NatureOSState *st, const char *raw) {
    if (!raw) return;
    st->term_scroll = 0;
    const char *host = st->cfg.device_name[0] ? st->cfg.device_name : "nature-pc";
    char prompt_line[320];
    snprintf(prompt_line, sizeof(prompt_line), "nature@%.31s:%s$ %s",
             host, strcmp(st->term_cwd, "/home/nature") == 0 ? "~" : st->term_cwd, raw);
    push_term_line(st, prompt_line, 1);

    /* Split on unquoted ';' and '&&' for multi-command chains */
    char buf[256];
    snprintf(buf, sizeof(buf), "%s", raw);
    char *seg = buf;
    char q = 0;
    for (char *p = buf; *p; ++p) {
        if ((*p == '"' || *p == '\'') && (p == buf || p[-1] != '\\')) {
            if (q == 0) q = *p;
            else if (q == *p) q = 0;
        } else if (q == 0 && (*p == ';' || (*p == '&' && p[1] == '&'))) {
            int step = (*p == '&') ? 2 : 1;
            *p = '\0';
            exec_single_cmd(st, seg);
            seg = p + step;
            p += (step - 1);
        }
    }
    exec_single_cmd(st, seg);
}

void nature_terminal_key(NatureOSState *st, const char *key) {
    if (!key || !*key) return;
    if (strcmp(key, "Space") == 0) key = " ";

    if (strcmp(key, "PageUp") == 0) {
        st->term_scroll += 6;
        if (st->term_scroll > st->term_max_scroll) st->term_scroll = st->term_max_scroll;
        return;
    }
    if (strcmp(key, "PageDown") == 0) {
        st->term_scroll -= 6;
        if (st->term_scroll < 0) st->term_scroll = 0;
        return;
    }
    st->term_scroll = 0;
    if (strcmp(key, "Ctrl+C") == 0) {
        char interrupt_line[448];
        const char *host = st->cfg.device_name[0] ? st->cfg.device_name : "nature-pc";
        snprintf(interrupt_line, sizeof(interrupt_line), "nature@%.31s:%s$ %s^C",
                 host, strcmp(st->term_cwd, "/home/nature") == 0 ? "~" : st->term_cwd, st->term_input);
        push_term_line(st, interrupt_line, 3);
        st->term_input[0] = '\0';
        st->term_hist_idx = -1;
        return;
    }
    if (strcmp(key, "Ctrl+L") == 0) {
        st->term_line_count = 0;
        st->term_scroll = 0;
        return;
    }
    if (strcmp(key, "Ctrl+U") == 0) {
        st->term_input[0] = '\0';
        return;
    }
    if (strcmp(key, "Enter") == 0) {
        char cmd_copy[256];
        snprintf(cmd_copy, sizeof(cmd_copy), "%s", st->term_input);
        if (cmd_copy[0]) {
            if (st->term_hist_count >= 24) {
                memmove(st->term_history[0], st->term_history[1], 23 * sizeof(st->term_history[0]));
                st->term_hist_count = 23;
            }
            snprintf(st->term_history[st->term_hist_count++], sizeof(st->term_history[0]), "%s", cmd_copy);
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
            snprintf(st->term_input, sizeof(st->term_input), "%s", st->term_history[st->term_hist_idx]);
        }
    } else if (strcmp(key, "ArrowDown") == 0) {
        if (st->term_hist_idx >= 0) {
            if (st->term_hist_idx < st->term_hist_count - 1) {
                st->term_hist_idx++;
                snprintf(st->term_input, sizeof(st->term_input), "%s", st->term_history[st->term_hist_idx]);
            } else {
                st->term_hist_idx = -1;
                st->term_input[0] = '\0';
            }
        }
    } else if (strcmp(key, "Tab") == 0) {
        char *sp = strrchr(st->term_input, ' ');
        if (sp) {
            const char *tok = sp + 1;
            char dir_part[160];
            char prefix[64];
            const char *sl = strrchr(tok, '/');
            if (sl) {
                char rel_dir[128];
                size_t dlen = (size_t)(sl - tok);
                if (dlen == 0) snprintf(rel_dir, sizeof(rel_dir), "/");
                else {
                    if (dlen >= sizeof(rel_dir)) dlen = sizeof(rel_dir) - 1;
                    memcpy(rel_dir, tok, dlen);
                    rel_dir[dlen] = '\0';
                }
                resolve_term_path(st, rel_dir, dir_part, sizeof(dir_part));
                snprintf(prefix, sizeof(prefix), "%.63s", sl + 1);
            } else {
                snprintf(dir_part, sizeof(dir_part), "%s", st->term_cwd);
                snprintf(prefix, sizeof(prefix), "%.63s", tok);
            }
            size_t plen = strlen(prefix);
            const char *matches[16];
            int match_is_dir[16];
            int mcnt = 0;
            for (int i = 0; i < st->vfs_count && mcnt < 16; ++i) {
                if (strcmp(st->vfs[i].parent, dir_part) == 0 &&
                    strncasecmp(st->vfs[i].name, prefix, plen) == 0) {
                    matches[mcnt] = st->vfs[i].name;
                    match_is_dir[mcnt] = st->vfs[i].is_dir;
                    mcnt++;
                }
            }
            if (mcnt == 1) {
                size_t lead = sl ? (size_t)((sl + 1) - st->term_input) : (size_t)((sp + 1) - st->term_input);
                if (lead + strlen(matches[0]) + 2 < sizeof(st->term_input)) {
                    snprintf(st->term_input + lead, sizeof(st->term_input) - lead, "%s%s", matches[0], match_is_dir[0] ? "/" : "");
                }
            } else if (mcnt > 1) {
                size_t lcp = strlen(matches[0]);
                for (int k = 1; k < mcnt; ++k) {
                    size_t j = 0;
                    while (j < lcp && matches[k][j] && tolower((unsigned char)matches[0][j]) == tolower((unsigned char)matches[k][j])) j++;
                    lcp = j;
                }
                if (lcp > plen) {
                    size_t lead = sl ? (size_t)((sl + 1) - st->term_input) : (size_t)((sp + 1) - st->term_input);
                    if (lead + lcp < sizeof(st->term_input)) {
                        memcpy(st->term_input + lead, matches[0], lcp);
                        st->term_input[lead + lcp] = '\0';
                    }
                } else {
                    char list_line[150] = {0};
                    for (int k = 0; k < mcnt; ++k) {
                        char itm[54];
                        snprintf(itm, sizeof(itm), "%s%s  ", matches[k], match_is_dir[k] ? "/" : "");
                        if (strlen(list_line) + strlen(itm) < sizeof(list_line) - 2) strcat(list_line, itm);
                    }
                    push_term_line(st, list_line, 2);
                }
            }
        } else {
            size_t plen = strlen(st->term_input);
            const char *matches[24];
            int mcnt = 0;
            for (size_t i = 0; i < sizeof(KNOWN_CMDS) / sizeof(KNOWN_CMDS[0]) && mcnt < 24; ++i) {
                if (strncmp(KNOWN_CMDS[i], st->term_input, plen) == 0) {
                    matches[mcnt++] = KNOWN_CMDS[i];
                }
            }
            if (mcnt == 1) {
                snprintf(st->term_input, sizeof(st->term_input), "%s ", matches[0]);
            } else if (mcnt > 1) {
                size_t lcp = strlen(matches[0]);
                for (int k = 1; k < mcnt; ++k) {
                    size_t j = 0;
                    while (j < lcp && matches[k][j] == matches[0][j]) j++;
                    lcp = j;
                }
                if (lcp > plen) {
                    snprintf(st->term_input, sizeof(st->term_input), "%.*s", (int)lcp, matches[0]);
                } else {
                    char list_line[150] = {0};
                    for (int k = 0; k < mcnt; ++k) {
                        if (strlen(list_line) + strlen(matches[k]) + 3 < sizeof(list_line)) {
                            strcat(list_line, matches[k]);
                            strcat(list_line, "  ");
                        }
                    }
                    push_term_line(st, list_line, 2);
                }
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
    } else if (hit->action_id == HIT_TERM_FONT) {
        int cur = st->cfg.term_font_size;
        if (cur < 10 || cur > 14) cur = 12;
        cur = (cur >= 14) ? 10 : cur + 1;
        st->cfg.term_font_size = cur;
        nature_os_save_state(st);
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

    int fpt = st->cfg.term_font_size;
    if (fpt < 10 || fpt > 14) fpt = 12;
    double fsize = (double)fpt;
    int line_h = (int)(fsize * 1.52 + 0.5);
    if (line_h < 15) line_h = 15;

    /* Terminal canvas adapts cleanly to Theme Family + Dark/Light Mode */
    double bg_r = dark ? (pal.theme_family == THEME_HORIZON ? 0.043 : 0.032) : pal.win_bg_r;
    double bg_g = dark ? (pal.theme_family == THEME_HORIZON ? 0.055 : 0.048) : pal.win_bg_g;
    double bg_b = dark ? (pal.theme_family == THEME_HORIZON ? 0.082 : 0.042) : pal.win_bg_b;
    ngfx_fill_rect(cr, cx, cy, cw, ch, 0, bg_r, bg_g, bg_b, 1.0);

    /* Interactive Command Chips + Font Size Bar */
    ngfx_fill_rect(cr, cx, cy, cw, 34, 0, pal.sidebar_r, pal.sidebar_g, pal.sidebar_b, 1.0);
    g_ngfx.set_source_rgba(cr, pal.border_r, pal.border_g, pal.border_b, pal.border_a);
    g_ngfx.set_line_width(cr, 1.0);
    g_ngfx.move_to(cr, cx, cy + 34.5);
    g_ngfx.line_to(cr, cx + cw, cy + 34.5);
    g_ngfx.stroke(cr);

    int tx = cx + 10;
    for (int i = 0; i < 6; ++i) {
        int bw = (int)ngfx_text_width(cr, "JetBrains Mono", 0, 10.5, QUICK_CMDS[i]) + 16;
        if (tx + bw > cx + cw - 134) break;
        int hov = (st->hover_mx >= tx && st->hover_mx <= tx + bw && st->hover_my >= cy + 6 && st->hover_my <= cy + 28);
        ngfx_fill_rect(cr, tx, cy + 6, bw, 22, 6, hov ? ar : pal.elev_r, hov ? ag : pal.elev_g, hov ? ab : pal.elev_b, hov ? 0.28 : 0.92);
        ngfx_stroke_rect(cr, tx + 0.5, cy + 6.5, bw - 1, 21, 6, 1.0, ar, ag, ab, hov ? 0.85 : 0.35);
        ngfx_text(cr, "JetBrains Mono", hov, 10.5, tx + 8, cy + 21, pal.fg_r, pal.fg_g, pal.fg_b, 0.96, QUICK_CMDS[i]);
        nature_add_hit(st, tx, cy + 6, bw, 22, HIT_TERM_CHIP, i, 0);
        tx += bw + 6;
    }

    if (cw >= 320) {
        char fbadge[16];
        snprintf(fbadge, sizeof(fbadge), "%dpx", fpt);
        int fx_r = cx + cw - 122;
        int fhov = (st->hover_mx >= fx_r && st->hover_mx <= fx_r + 44 && st->hover_my >= cy + 6 && st->hover_my <= cy + 28);
        ngfx_fill_rect(cr, fx_r, cy + 6, 44, 22, 6, fhov ? ar : pal.elev_r, fhov ? ag : pal.elev_g, fhov ? ab : pal.elev_b, fhov ? 0.24 : 0.85);
        ngfx_stroke_rect(cr, fx_r + 0.5, cy + 6.5, 43, 21, 6, 1.0, pal.border_r, pal.border_g, pal.border_b, fhov ? 0.75 : 0.45);
        ngfx_text(cr, "JetBrains Mono", 1, 9.5, fx_r + 9, cy + 20.5, pal.fg_sec_r, pal.fg_sec_g, pal.fg_sec_b, 0.96, fbadge);
        nature_add_hit(st, fx_r, cy + 6, 44, 22, HIT_TERM_FONT, 0, 0);

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
    int body_h = ch - 78;
    int max_visible = body_h / line_h;
    if (max_visible < 1) max_visible = 1;

    int max_scroll = st->term_line_count - max_visible;
    if (max_scroll < 0) max_scroll = 0;
    st->term_max_scroll = max_scroll;
    if (st->term_scroll > max_scroll) st->term_scroll = max_scroll;
    if (st->term_scroll < 0) st->term_scroll = 0;

    int end_idx = st->term_line_count - st->term_scroll;
    int start = end_idx > max_visible ? end_idx - max_visible : 0;

    int y = cy + 38 + (int)fsize;
    for (int i = start; i < end_idx; ++i) {
        uint8_t col = st->term_line_colors[i];
        double r = pal.fg_sec_r, g = pal.fg_sec_g, b = pal.fg_sec_b;
        if (col == 1) { r = pal.fg_r; g = pal.fg_g; b = pal.fg_b; }
        else if (col == 2) { r = dark ? pal.ar_hi : ar; g = dark ? pal.ag_hi : ag; b = dark ? pal.ab_hi : ab; }
        else if (col == 3) { r = pal.danger_r; g = pal.danger_g; b = pal.danger_b; }
        ngfx_text_clipped(cr, "JetBrains Mono", col == 1, fsize, cx + 14, y, cw - 36, r, g, b, 1.0, st->term_lines[i]);
        y += line_h;
    }

    /* Register touch input hit area for prompt & terminal body (minus scrollbar track) */
    nature_add_hit(st, cx, body_y, cw - 20, ch - 36, HIT_TERM_INPUT_AREA, 0, 0);
    nature_add_text_rect(st, cx, body_y, cw - 20, ch - 36, 1);

    nature_draw_scrollbar(st, cr, cx + cw - 11, body_y, body_h,
                          st->term_line_count, max_visible, st->term_scroll, max_scroll, SCROLL_TERMINAL, dark);

    int term_focused = (st->active_text_target == 1);
    int py_bar = cy + ch - 35;
    ngfx_fill_rect(cr, cx + 8, py_bar, cw - 16, 29, 8, pal.card_r, pal.card_g, pal.card_b, 0.96);
    ngfx_stroke_rect(cr, cx + 8.5, py_bar + 0.5, cw - 17, 28, 8, term_focused ? 1.2 : 1.0,
                     term_focused ? ar : pal.border_r, term_focused ? ag : pal.border_g, term_focused ? ab : pal.border_b,
                     term_focused ? 0.80 : pal.border_a);

    int py = py_bar + 19;
    const char *host = st->cfg.device_name[0] ? st->cfg.device_name : "nature-pc";
    char seg_user[64], seg_cwd[128];
    snprintf(seg_user, sizeof(seg_user), "nature@%.31s", host);
    snprintf(seg_cwd, sizeof(seg_cwd), "%s", strcmp(st->term_cwd, "/home/nature") == 0 ? "~" : st->term_cwd);

    double px_cur = cx + 16;
    double w_user = ngfx_text_width(cr, "JetBrains Mono", 1, fsize, seg_user);
    ngfx_text(cr, "JetBrains Mono", 1, fsize, px_cur, py, dark ? pal.ar_hi : ar, dark ? pal.ag_hi : ag, dark ? pal.ab_hi : ab, 1.0, seg_user);
    px_cur += w_user;

    double w_col = ngfx_text_width(cr, "JetBrains Mono", 0, fsize, ":");
    ngfx_text(cr, "JetBrains Mono", 0, fsize, px_cur, py, pal.mut_r, pal.mut_g, pal.mut_b, 0.9, ":");
    px_cur += w_col;

    double w_cwd = ngfx_text_width(cr, "JetBrains Mono", 1, fsize, seg_cwd);
    ngfx_text(cr, "JetBrains Mono", 1, fsize, px_cur, py, pal.fg_r, pal.fg_g, pal.fg_b, 0.96, seg_cwd);
    px_cur += w_cwd;

    double w_dlr = ngfx_text_width(cr, "JetBrains Mono", 1, fsize, "$ ");
    ngfx_text(cr, "JetBrains Mono", 1, fsize, px_cur, py, pal.mut_r, pal.mut_g, pal.mut_b, 0.95, "$ ");
    px_cur += w_dlr;

    double pw = px_cur - (cx + 16);
    double avail_w = cw - pw - 34;
    /* Pan visible input tail cleanly if user types past line width */
    const char *vis_in = st->term_input;
    double iw = ngfx_text_width(cr, "JetBrains Mono", 0, fsize, vis_in);
    while (*vis_in && iw > avail_w - 10) {
        vis_in++;
        iw = ngfx_text_width(cr, "JetBrains Mono", 0, fsize, vis_in);
    }
    ngfx_text_clipped(cr, "JetBrains Mono", 0, fsize, px_cur, py, avail_w, pal.fg_r, pal.fg_g, pal.fg_b, 1.0, vis_in);

    if (17 + pw + iw < cw - 20) {
        ngfx_fill_rect(cr, cx + 17 + pw + iw, py - (int)fsize, 7, (int)fsize + 2, 1.5,
                       dark ? pal.ar_hi : ar, dark ? pal.ag_hi : ag, dark ? pal.ab_hi : ab, 0.92);
    }
}
