#include "nature_os.h"
#include <algorithm>
#include <atomic>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <sys/statvfs.h>
#include <sys/utsname.h>
#include <thread>
#include <time.h>
#include <unistd.h>
#include <unordered_map>
#include <vector>

namespace {
std::atomic<bool> g_stress_running{false};
std::vector<std::thread> g_workers;

struct CpuJiffies {
    unsigned long long active = 0;
    unsigned long long total = 0;
    bool valid = false;
};

CpuJiffies g_prev_total{};
CpuJiffies g_prev_cores[8]{};
bool g_stat_seeded = false;
unsigned long long g_last_dt_jiffies = 0;

struct timespec g_prev_wall{};
struct timespec g_prev_proc_cpu{};
bool g_proc_cpu_seeded = false;

unsigned long long g_prev_net_rx = 0;
unsigned long long g_prev_net_tx = 0;
struct timespec g_prev_net_ts{};
bool g_net_seeded = false;

std::unordered_map<int, unsigned long long> g_prev_pid_jiffies;

void stress_worker_loop() {
    volatile double acc = 1.13;
    while (g_stress_running.load(std::memory_order_relaxed)) {
        for (int i = 0; i < 45000; ++i) {
            acc = std::sin(acc + i * 0.0007) * std::cos(acc * 0.9991) + 0.25;
        }
        std::this_thread::sleep_for(std::chrono::microseconds(250));
    }
}

void read_static_sys_info(NatureOSState *st) {
    struct utsname u{};
    if (uname(&u) == 0) {
        std::snprintf(st->kernel_ver, sizeof(st->kernel_ver), "%.42s %.18s", u.release, u.machine);
    } else {
        std::snprintf(st->kernel_ver, sizeof(st->kernel_ver), "6.8.0-45-lowlatency x86_64");
    }

    st->cpu_model[0] = '\0';
    if (FILE *f = std::fopen("/proc/cpuinfo", "r")) {
        char line[256];
        while (std::fgets(line, sizeof(line), f)) {
            if (std::strncmp(line, "model name", 10) == 0) {
                const char *col = std::strchr(line, ':');
                if (col) {
                    ++col;
                    while (*col == ' ' || *col == '\t') ++col;
                    size_t len = std::strlen(col);
                    while (len > 0 && (col[len - 1] == '\n' || col[len - 1] == '\r')) --len;
                    if (len > 0 && std::strncmp(col, "unknown", 7) != 0) {
                        std::snprintf(st->cpu_model, sizeof(st->cpu_model), "%.*s",
                                      static_cast<int>(std::min(len, sizeof(st->cpu_model) - 1)), col);
                    }
                    break;
                }
            }
        }
        std::fclose(f);
    }
    if (!st->cpu_model[0]) {
        std::snprintf(st->cpu_model, sizeof(st->cpu_model), "%.24s Linux SMP",
                      u.machine[0] ? u.machine : "x86_64");
    }
}

void read_mem_telemetry(NatureOSState *st) {
    long long mem_total_kb = 0, mem_avail_kb = 0, mem_free_kb = 0;
    long long buffers_kb = 0, cached_kb = 0, sreclaim_kb = 0;
    long long swap_total_kb = 0, swap_free_kb = 0;

    if (FILE *f = std::fopen("/proc/meminfo", "r")) {
        char key[64];
        long long val = 0;
        char unit[16];
        while (std::fscanf(f, "%63s %lld %15s", key, &val, unit) >= 2) {
            if (std::strcmp(key, "MemTotal:") == 0) mem_total_kb = val;
            else if (std::strcmp(key, "MemAvailable:") == 0) mem_avail_kb = val;
            else if (std::strcmp(key, "MemFree:") == 0) mem_free_kb = val;
            else if (std::strcmp(key, "Buffers:") == 0) buffers_kb = val;
            else if (std::strcmp(key, "Cached:") == 0) cached_kb = val;
            else if (std::strcmp(key, "SReclaimable:") == 0) sreclaim_kb = val;
            else if (std::strcmp(key, "SwapTotal:") == 0) swap_total_kb = val;
            else if (std::strcmp(key, "SwapFree:") == 0) swap_free_kb = val;
        }
        std::fclose(f);
    }

    if (mem_avail_kb <= 0) mem_avail_kb = mem_free_kb;

    if (FILE *sf = std::fopen("/proc/self/status", "r")) {
        char line[128];
        while (std::fgets(line, sizeof(line), sf)) {
            long long rss_kb = 0;
            if (std::sscanf(line, "VmRSS: %lld kB", &rss_kb) == 1) {
                st->os_rss_mb = std::max(1, static_cast<int>((rss_kb + 512) / 1024));
                break;
            }
        }
        std::fclose(sf);
    }
    if (st->os_rss_mb <= 0) st->os_rss_mb = 24;

    st->cgroup_limited = 0;
    if (FILE *cf = std::fopen("/sys/fs/cgroup/memory.max", "r")) {
        char buf[64]{};
        if (std::fgets(buf, sizeof(buf), cf) && std::strncmp(buf, "max", 3) != 0) {
            long long bytes = std::atoll(buf);
            if (bytes > 0) {
                long long cg_kb = bytes / 1024;
                if (mem_total_kb == 0 || cg_kb < mem_total_kb) {
                    mem_total_kb = cg_kb;
                    st->cgroup_limited = 1;
                }
            }
        }
        std::fclose(cf);
    }

    long long used_kb = 0;
    long long cache_pool_kb = buffers_kb + cached_kb + sreclaim_kb;
    long long avail_kb = mem_avail_kb;

    if (st->cgroup_limited) {
        long long cg_cur_kb = 0, cg_file_kb = 0, cg_inact_file_kb = 0, cg_slab_rec_kb = 0;
        if (FILE *cf = std::fopen("/sys/fs/cgroup/memory.current", "r")) {
            long long bytes = 0;
            if (std::fscanf(cf, "%lld", &bytes) == 1 && bytes >= 0) cg_cur_kb = bytes / 1024;
            std::fclose(cf);
        }
        if (FILE *ms = std::fopen("/sys/fs/cgroup/memory.stat", "r")) {
            char key[64];
            long long bytes = 0;
            while (std::fscanf(ms, "%63s %lld", key, &bytes) == 2) {
                if (std::strcmp(key, "file") == 0) cg_file_kb = bytes / 1024;
                else if (std::strcmp(key, "inactive_file") == 0) cg_inact_file_kb = bytes / 1024;
                else if (std::strcmp(key, "slab_reclaimable") == 0) cg_slab_rec_kb = bytes / 1024;
            }
            std::fclose(ms);
        }
        used_kb = (cg_cur_kb > cg_inact_file_kb) ? (cg_cur_kb - cg_inact_file_kb) : cg_cur_kb;
        cache_pool_kb = cg_file_kb + cg_slab_rec_kb;
        avail_kb = (mem_total_kb > used_kb) ? (mem_total_kb - used_kb) : 0;

        if (FILE *swm = std::fopen("/sys/fs/cgroup/memory.swap.max", "r")) {
            char sbuf[64]{};
            if (std::fgets(sbuf, sizeof(sbuf), swm) && std::strncmp(sbuf, "max", 3) != 0) {
                long long sbytes = std::atoll(sbuf);
                if (sbytes >= 0) swap_total_kb = sbytes / 1024;
            }
            std::fclose(swm);
        }
        if (FILE *swc = std::fopen("/sys/fs/cgroup/memory.swap.current", "r")) {
            long long sbytes = 0;
            if (std::fscanf(swc, "%lld", &sbytes) == 1 && sbytes >= 0) {
                long long cg_sw_used_kb = sbytes / 1024;
                swap_free_kb = (swap_total_kb > cg_sw_used_kb) ? (swap_total_kb - cg_sw_used_kb) : 0;
            }
            std::fclose(swc);
        }
    } else {
        used_kb = (mem_total_kb > mem_avail_kb) ? (mem_total_kb - mem_avail_kb) : 0;
    }

    if (mem_total_kb > 0) {
        st->total_ram_mb = std::max(1, static_cast<int>(mem_total_kb / 1024));
        int used_mb = std::clamp(static_cast<int>((used_kb + 512) / 1024), st->os_rss_mb, st->total_ram_mb);
        st->used_ram_mb = used_mb;
        st->avail_ram_mb = std::clamp(static_cast<int>(avail_kb / 1024), 0, st->total_ram_mb);
        if (st->avail_ram_mb + st->used_ram_mb > st->total_ram_mb) {
            st->avail_ram_mb = std::max(0, st->total_ram_mb - st->used_ram_mb);
        }
        st->cached_ram_mb = std::clamp(static_cast<int>(cache_pool_kb / 1024), 0, st->total_ram_mb);
    }

    st->swap_total_mb = static_cast<int>(std::max(0LL, swap_total_kb) / 1024);
    long long swap_used_kb = (swap_total_kb > swap_free_kb) ? (swap_total_kb - swap_free_kb) : 0;
    st->swap_used_mb = std::min(st->swap_total_mb, static_cast<int>((swap_used_kb + 512) / 1024));

    long long zram_kb = 0;
    if (FILE *zs = std::fopen("/proc/swaps", "r")) {
        char line[256];
        if (std::fgets(line, sizeof(line), zs)) {
            while (std::fgets(line, sizeof(line), zs)) {
                char dev[128], type[32];
                long long sz = 0, used = 0;
                int prio = 0;
                if (std::sscanf(line, "%127s %31s %lld %lld %d", dev, type, &sz, &used, &prio) >= 4) {
                    if (std::strstr(dev, "zram") != nullptr) zram_kb += used;
                }
            }
        }
        std::fclose(zs);
    }
    st->zram_used_mb = (st->cfg.zram_enabled && zram_kb > 0) ? static_cast<int>((zram_kb + 512) / 1024) : 0;
}

void read_disk_load_net(NatureOSState *st) {
    struct statvfs vfs{};
    if (statvfs("/", &vfs) == 0 && vfs.f_blocks > 0) {
        unsigned long long total_bytes = static_cast<unsigned long long>(vfs.f_blocks) * vfs.f_frsize;
        unsigned long long avail_bytes = static_cast<unsigned long long>(vfs.f_bavail) * vfs.f_frsize;
        unsigned long long used_bytes = total_bytes > avail_bytes ? (total_bytes - avail_bytes) : 0;
        st->disk_total_gb = static_cast<long>(total_bytes / (1024ULL * 1024ULL * 1024ULL));
        st->disk_avail_gb = static_cast<long>(avail_bytes / (1024ULL * 1024ULL * 1024ULL));
        st->disk_used_pct = static_cast<int>((used_bytes * 100ULL) / total_bytes);
        if (st->disk_used_pct < 1 && used_bytes > 0) st->disk_used_pct = 1;
    }
    if (FILE *uf = std::fopen("/proc/uptime", "r")) {
        double up = 0.0;
        if (std::fscanf(uf, "%lf", &up) == 1) st->uptime_sec = static_cast<long>(up);
        std::fclose(uf);
    }
    if (FILE *lf = std::fopen("/proc/loadavg", "r")) {
        double l1 = 0.0, l5 = 0.0, l15 = 0.0;
        if (std::fscanf(lf, "%lf %lf %lf", &l1, &l5, &l15) == 3) {
            st->load_avg[0] = l1;
            st->load_avg[1] = l5;
            st->load_avg[2] = l15;
        }
        std::fclose(lf);
    }
    if (FILE *nf = std::fopen("/proc/net/dev", "r")) {
        char line[256];
        unsigned long long sum_rx = 0, sum_tx = 0;
        while (std::fgets(line, sizeof(line), nf)) {
            char *colon = std::strchr(line, ':');
            if (!colon) continue;
            *colon = '\0';
            const char *iface = line;
            while (*iface == ' ' || *iface == '\t') ++iface;
            if (std::strcmp(iface, "lo") == 0) continue;
            unsigned long long rx_b = 0, tx_b = 0;
            if (std::sscanf(colon + 1, "%llu %*u %*u %*u %*u %*u %*u %*u %llu", &rx_b, &tx_b) == 2) {
                sum_rx += rx_b;
                sum_tx += tx_b;
            }
        }
        std::fclose(nf);
        struct timespec now_ts{};
        clock_gettime(CLOCK_MONOTONIC, &now_ts);
        if (g_net_seeded) {
            double dt = (now_ts.tv_sec - g_prev_net_ts.tv_sec) + (now_ts.tv_nsec - g_prev_net_ts.tv_nsec) * 1e-9;
            if (dt >= 0.05) {
                double rx_rate = (sum_rx >= g_prev_net_rx) ? ((sum_rx - g_prev_net_rx) / 1024.0 / dt) : 0.0;
                double tx_rate = (sum_tx >= g_prev_net_tx) ? ((sum_tx - g_prev_net_tx) / 1024.0 / dt) : 0.0;
                st->net_rx_kbps = st->net_rx_kbps * 0.35 + rx_rate * 0.65;
                st->net_tx_kbps = st->net_tx_kbps * 0.35 + tx_rate * 0.65;
            }
        } else {
            g_net_seeded = true;
        }
        g_prev_net_rx = sum_rx;
        g_prev_net_tx = sum_tx;
        g_prev_net_ts = now_ts;
    }
}

void read_cpu_telemetry(NatureOSState *st) {
    int cores_found = 0;
    CpuJiffies cur_total{};
    CpuJiffies cur_cores[8]{};

    if (FILE *f = std::fopen("/proc/stat", "r")) {
        char line[256];
        while (std::fgets(line, sizeof(line), f)) {
            unsigned long long u = 0, n = 0, s = 0, id = 0, io = 0, irq = 0, sirq = 0, st_v = 0;
            if (std::strncmp(line, "cpu ", 4) == 0) {
                if (std::sscanf(line + 4, "%llu %llu %llu %llu %llu %llu %llu %llu",
                                &u, &n, &s, &id, &io, &irq, &sirq, &st_v) >= 4) {
                    cur_total.active = u + n + s + irq + sirq + st_v;
                    cur_total.total = cur_total.active + id + io;
                    cur_total.valid = true;
                }
            } else if (std::strncmp(line, "cpu", 3) == 0 && line[3] >= '0' && line[3] <= '9') {
                int cidx = std::atoi(line + 3);
                if (cidx >= 0 && cidx < 8) {
                    const char *sp = std::strchr(line, ' ');
                    if (sp && std::sscanf(sp + 1, "%llu %llu %llu %llu %llu %llu %llu %llu",
                                          &u, &n, &s, &id, &io, &irq, &sirq, &st_v) >= 4) {
                        cur_cores[cidx].active = u + n + s + irq + sirq + st_v;
                        cur_cores[cidx].total = cur_cores[cidx].active + id + io;
                        cur_cores[cidx].valid = true;
                        cores_found = std::max(cores_found, cidx + 1);
                    }
                }
            }
        }
        std::fclose(f);
    }

    if (cores_found > 0) st->cpu_count = cores_found;
    else if (st->cpu_count <= 0) st->cpu_count = 4;

    if (g_stat_seeded && cur_total.valid && g_prev_total.valid) {
        unsigned long long dt = cur_total.total - g_prev_total.total;
        unsigned long long da = cur_total.active - g_prev_total.active;
        g_last_dt_jiffies = dt;
        if (dt > 0) {
            double pct = std::clamp((static_cast<double>(da) * 100.0) / static_cast<double>(dt), 0.0, 100.0);
            st->total_cpu_pct = st->total_cpu_pct * 0.45 + pct * 0.55;
        }
        for (int c = 0; c < 8 && c < st->cpu_count; ++c) {
            if (cur_cores[c].valid && g_prev_cores[c].valid) {
                unsigned long long cdt = cur_cores[c].total - g_prev_cores[c].total;
                unsigned long long cda = cur_cores[c].active - g_prev_cores[c].active;
                if (cdt > 0) {
                    double cpct = std::clamp((static_cast<double>(cda) * 100.0) / static_cast<double>(cdt), 0.0, 100.0);
                    st->core_usage[c] = st->core_usage[c] * 0.45 + cpct * 0.55;
                }
            }
        }
    } else if (cur_total.valid && cur_total.total > 0) {
        st->total_cpu_pct = std::clamp(
            (static_cast<double>(cur_total.active) * 100.0) / static_cast<double>(cur_total.total), 0.1, 100.0);
        for (int c = 0; c < 8 && c < st->cpu_count; ++c) {
            if (cur_cores[c].valid && cur_cores[c].total > 0) {
                st->core_usage[c] = std::clamp(
                    (static_cast<double>(cur_cores[c].active) * 100.0) / static_cast<double>(cur_cores[c].total),
                    0.1, 100.0);
            }
        }
        g_stat_seeded = true;
    }

    if (cur_total.valid) g_prev_total = cur_total;
    for (int c = 0; c < 8; ++c) {
        if (cur_cores[c].valid) g_prev_cores[c] = cur_cores[c];
    }

    struct timespec now_wall{}, now_cpu{};
    clock_gettime(CLOCK_MONOTONIC, &now_wall);
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &now_cpu);
    if (g_proc_cpu_seeded) {
        double d_wall = (now_wall.tv_sec - g_prev_wall.tv_sec) + (now_wall.tv_nsec - g_prev_wall.tv_nsec) * 1e-9;
        double d_cpu = (now_cpu.tv_sec - g_prev_proc_cpu.tv_sec) + (now_cpu.tv_nsec - g_prev_proc_cpu.tv_nsec) * 1e-9;
        if (d_wall > 0.05) {
            double raw_pct = (d_cpu / d_wall) * 100.0;
            if (raw_pct < 0.1) raw_pct = 0.1;
            if (raw_pct > 100.0 * st->cpu_count) raw_pct = 100.0 * st->cpu_count;
            st->compositor_cpu_pct = st->compositor_cpu_pct * 0.4 + raw_pct * 0.6;
        }
    } else {
        st->compositor_cpu_pct = 0.4;
        g_proc_cpu_seeded = true;
    }
    g_prev_wall = now_wall;
    g_prev_proc_cpu = now_cpu;
}

void read_real_linux_procs(int self_pid, int cpu_count, std::vector<ProcEntry> &out) {
    out.clear();
    DIR *dir = opendir("/proc");
    if (!dir) return;

    std::unordered_map<int, unsigned long long> next_map;
    struct dirent *ent = nullptr;
    while ((ent = readdir(dir)) != nullptr) {
        if (!std::isdigit(static_cast<unsigned char>(ent->d_name[0]))) continue;
        int pid = std::atoi(ent->d_name);
        if (pid <= 0 || pid == self_pid) continue;

        char stat_path[64], status_path[64];
        std::snprintf(stat_path, sizeof(stat_path), "/proc/%d/stat", pid);
        FILE *sf = std::fopen(stat_path, "r");
        if (!sf) continue;
        char buf[512]{};
        size_t nread = std::fread(buf, 1, sizeof(buf) - 1, sf);
        std::fclose(sf);
        if (nread == 0) continue;

        char *lp = std::strchr(buf, '(');
        char *rp = std::strrchr(buf, ')');
        if (!lp || !rp || rp <= lp) continue;

        char comm[32]{};
        size_t clen = std::min(static_cast<size_t>(rp - (lp + 1)), sizeof(comm) - 1);
        std::memcpy(comm, lp + 1, clen);
        comm[clen] = '\0';

        char state_ch = 'S';
        unsigned long long utime = 0, stime = 0;
        long nice_v = 0;
        if (std::sscanf(rp + 2, "%c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %llu %llu %*d %*d %*d %ld",
                        &state_ch, &utime, &stime, &nice_v) < 3) {
            continue;
        }

        unsigned long long tot_jiffies = utime + stime;
        next_map[pid] = tot_jiffies;
        double cpu_pct = 0.0;
        auto it = g_prev_pid_jiffies.find(pid);
        if (it != g_prev_pid_jiffies.end() && g_last_dt_jiffies > 0 && tot_jiffies >= it->second) {
            unsigned long long d_proc = tot_jiffies - it->second;
            cpu_pct = (static_cast<double>(d_proc) * 100.0 * std::max(1, cpu_count)) / static_cast<double>(g_last_dt_jiffies);
            cpu_pct = std::clamp(cpu_pct, 0.0, 100.0 * std::max(1, cpu_count));
        }

        int rss_mb = 1, uid_v = 1000;
        std::snprintf(status_path, sizeof(status_path), "/proc/%d/status", pid);
        if (FILE *stf = std::fopen(status_path, "r")) {
            char line[160];
            while (std::fgets(line, sizeof(line), stf)) {
                long long kb = 0;
                int u0 = 0;
                if (std::sscanf(line, "VmRSS: %lld kB", &kb) == 1) {
                    rss_mb = std::max(1, static_cast<int>((kb + 512) / 1024));
                } else if (std::sscanf(line, "Uid: %d", &u0) == 1) {
                    uid_v = u0;
                }
            }
            std::fclose(stf);
        }

        ProcEntry pe{};
        pe.pid = pid;
        std::snprintf(pe.name, sizeof(pe.name), "%s", comm);
        std::snprintf(pe.user, sizeof(pe.user), "%s", uid_v == 0 ? "root" : "user");
        std::snprintf(pe.state, sizeof(pe.state), "%s", (state_ch == 'R') ? "Run" : "Sleep");
        pe.cpu_pct = cpu_pct;
        pe.ram_mb = rss_mb;
        pe.nice_val = static_cast<int>(nice_v);
        pe.win_id = -1;
        pe.killable = 0;
        out.push_back(pe);
    }
    closedir(dir);
    g_prev_pid_jiffies.swap(next_map);

    std::sort(out.begin(), out.end(), [](const ProcEntry &a, const ProcEntry &b) {
        if (a.cpu_pct != b.cpu_pct) return a.cpu_pct > b.cpu_pct;
        return a.ram_mb > b.ram_mb;
    });
}
} // namespace

extern "C" {

void nature_monitor_set_stress(NatureOSState *st, int enable) {
    if (enable && !st->stress_active) {
        st->stress_active = 1;
        if (st->stress_threads <= 0) st->stress_threads = 4;
        g_stress_running.store(true);
        for (int i = 0; i < st->stress_threads; ++i) {
            g_workers.emplace_back(stress_worker_loop);
        }
        nature_notify(st, "Performance Scheduler", "Multi-core C++ worker load active. UI compositor stays locked.");
    } else if (!enable && st->stress_active) {
        st->stress_active = 0;
        g_stress_running.store(false);
        for (auto &t : g_workers) {
            if (t.joinable()) t.join();
        }
        g_workers.clear();
        nature_notify(st, "Performance Scheduler", "Compute workload halted cleanly.");
    }
    nature_monitor_tick(st);
}

void nature_monitor_set_threads(NatureOSState *st, int threads) {
    int max_th = std::clamp(st->cpu_count > 0 ? st->cpu_count : 4, 4, 8);
    if (threads < 1) threads = 1;
    if (threads > max_th) threads = max_th;
    st->stress_threads = threads;
    if (st->stress_active) {
        nature_monitor_set_stress(st, 0);
        nature_monitor_set_stress(st, 1);
    }
}

void nature_monitor_init(NatureOSState *st) {
    read_static_sys_info(st);
    read_mem_telemetry(st);
    read_disk_load_net(st);
    read_cpu_telemetry(st);
    double init_ram_pct = st->total_ram_mb > 0
        ? std::clamp((static_cast<double>(st->used_ram_mb) * 100.0) / static_cast<double>(st->total_ram_mb), 0.0, 100.0)
        : 0.0;
    for (int i = 0; i < 48; ++i) {
        st->cpu_history[i] = st->total_cpu_pct;
        st->ram_history[i] = init_ram_pct;
    }
    st->stress_threads = 4;
    st->monitor_scroll = 0;
    st->monitor_max_scroll = 0;
    nature_monitor_tick(st);
}

void nature_monitor_tick(NatureOSState *st) {
    read_mem_telemetry(st);
    read_disk_load_net(st);
    read_cpu_telemetry(st);

    int count = 0;
    auto push_proc = [&](int pid, const char *name, const char *user, const char *state,
                         double cpu, int ram, int nice_val, int win_id, int killable) {
        if (count >= 32) return;
        ProcEntry &p = st->procs[count++];
        p.pid = pid;
        std::snprintf(p.name, sizeof(p.name), "%s", name);
        std::snprintf(p.user, sizeof(p.user), "%s", user);
        std::snprintf(p.state, sizeof(p.state), "%s", state);
        p.cpu_pct = cpu;
        p.ram_mb = ram;
        p.nice_val = nice_val;
        p.win_id = win_id;
        p.killable = killable;
    };

    int self_pid = static_cast<int>(getpid());
    double comp_cpu = st->stress_active
        ? std::min(4.8, std::max(0.4, st->compositor_cpu_pct * 0.25))
        : std::max(0.2, st->compositor_cpu_pct);
    push_proc(self_pid, "nature-compositor", "nature", "Run", comp_cpu, st->os_rss_mb, -10, -1, 0);

    static const char *PROC_NAMES[APP_COUNT] = {
        "nature-files", "nature-browser", "nature-terminal", "nature-store",
        "nature-monitor", "nature-media", "nature-settings"
    };
    static const int APP_SHARE_MB[APP_COUNT] = {6, 14, 4, 8, 6, 9, 5};

    for (int i = 0; i < st->window_count; ++i) {
        const OSWindow &w = st->windows[i];
        double app_cpu = w.minimized ? 0.0 : (w.id == st->active_window_id ? 0.4 : 0.1);
        push_proc(w.pid, PROC_NAMES[w.app], "nature", w.minimized ? "Sleep" : "Run",
                  app_cpu, APP_SHARE_MB[w.app], -2, w.id, 1);
    }

    if (st->stress_active) {
        int th = std::clamp(st->stress_threads > 0 ? st->stress_threads : 4, 1, 8);
        double worker_share = std::max(8.0, (st->total_cpu_pct * std::max(1, st->cpu_count)) / th * 0.72);
        for (int t = 0; t < th; ++t) {
            char wname[32];
            std::snprintf(wname, sizeof(wname), "nature-stress#%d", t);
            push_proc(4801 + t * 17, wname, "nature", "Load",
                      std::min(99.0, worker_share), 8, 10, -1, 1);
        }
    }

    std::vector<ProcEntry> linux_procs;
    read_real_linux_procs(self_pid, st->cpu_count > 0 ? st->cpu_count : 4, linux_procs);
    for (const auto &lp : linux_procs) {
        if (count >= 32) break;
        st->procs[count++] = lp;
    }

    std::sort(st->procs, st->procs + count, [st](const ProcEntry &a, const ProcEntry &b) {
        if (st->monitor_sort == 1) {
            if (a.ram_mb != b.ram_mb) return a.ram_mb > b.ram_mb;
            return a.cpu_pct > b.cpu_pct;
        }
        if (st->monitor_sort == 2) {
            return a.pid < b.pid;
        }
        if (a.cpu_pct != b.cpu_pct) return a.cpu_pct > b.cpu_pct;
        return a.ram_mb > b.ram_mb;
    });
    st->proc_count = count;

    double ram_pct = st->total_ram_mb > 0
        ? std::clamp((static_cast<double>(st->used_ram_mb) * 100.0) / static_cast<double>(st->total_ram_mb), 0.0, 100.0)
        : 0.0;
    for (int i = 0; i < 47; ++i) {
        st->cpu_history[i] = st->cpu_history[i + 1];
        st->ram_history[i] = st->ram_history[i + 1];
    }
    st->cpu_history[47] = st->total_cpu_pct;
    st->ram_history[47] = ram_pct;
}

void nature_monitor_render(NatureOSState *st, cairo_t *cr, OSWindow *w, int cx, int cy, int cw, int ch) {
    (void)w;
    NaturePalette pal;
    nature_get_palette(st, &pal);
    int dark = pal.dark_mode;
    double ar = pal.ar, ag = pal.ag, ab = pal.ab;

    double fg_r = pal.fg_r, fg_g = pal.fg_g, fg_b = pal.fg_b;
    double mut_r = pal.mut_r, mut_g = pal.mut_g, mut_b = pal.mut_b;
    int compact = cw < 540;

    ngfx_fill_rect(cr, cx, cy, cw, ch, 0, pal.win_bg_r, pal.win_bg_g, pal.win_bg_b, 1.0);

    /* Top Tabs + Stress Control */
    ngfx_fill_rect(cr, cx, cy, cw, 42, 0, pal.sidebar_r, pal.sidebar_g, pal.sidebar_b, 1.0);
    g_ngfx.set_source_rgba(cr, pal.border_r, pal.border_g, pal.border_b, pal.border_a);
    g_ngfx.set_line_width(cr, 1.0);
    g_ngfx.move_to(cr, cx, cy + 42.5);
    g_ngfx.line_to(cr, cx + cw, cy + 42.5);
    g_ngfx.stroke(cr);

    const char *tabs_full[3] = {"Resources & Cores", "Processes & Tasks", "Disks & Network"};
    const char *tabs_short[3] = {"Resources", "Tasks", "Disks/Net"};
    int tab_w = compact ? 78 : 124;
    for (int i = 0; i < 3; ++i) {
        int tx = cx + 10 + i * (tab_w + 6);
        int act = st->monitor_tab == i;
        ngfx_fill_rect(cr, tx, cy + 8, tab_w, 26, 7,
                       act ? ar : pal.elev_r, act ? ag : pal.elev_g, act ? ab : pal.elev_b, act ? 0.94 : 0.90);
        ngfx_text(cr, "Noto Sans", act, compact ? 10.0 : 11.2, tx + (compact ? 8 : 12), cy + 25,
                  act ? pal.on_acc_r : fg_r, act ? pal.on_acc_g : fg_g, act ? pal.on_acc_b : fg_b, 1.0,
                  compact ? tabs_short[i] : tabs_full[i]);
        nature_add_hit(st, tx, cy + 8, tab_w, 26, HIT_MONITOR_TAB, i, 0);
    }

    int btn_w = compact ? 96 : 162;
    int sb_x = cx + cw - btn_w - 10;
    if (!compact || cw >= 420) {
        int max_c = std::clamp(st->cpu_count > 0 ? st->cpu_count : 4, 4, 8);
        int n_opts = (max_c > 4) ? 4 : 3;
        int c_opts[4] = {1, 2, 4, max_c};
        int c_start = sb_x - (n_opts * 31 + 8);
        for (int i = 0; i < n_opts; ++i) {
            int bx = c_start + i * 31;
            if (bx < cx + 210) continue;
            int act = (st->stress_threads == c_opts[i]);
            ngfx_fill_rect(cr, bx, cy + 8, 27, 26, 6,
                           act ? ar : pal.elev_r, act ? ag : pal.elev_g, act ? ab : pal.elev_b,
                           act ? 0.35 : 0.90);
            char lbl[8];
            std::snprintf(lbl, sizeof(lbl), "%dC", c_opts[i]);
            ngfx_text(cr, "JetBrains Mono", 1, 10.0, bx + 6, cy + 25, fg_r, fg_g, fg_b, 1.0, lbl);
            nature_add_hit(st, bx, cy + 8, 27, 26, HIT_MONITOR_STRESS_THREADS, c_opts[i], 0);
        }
    }

    ngfx_fill_rect(cr, sb_x, cy + 8, btn_w, 26, 7,
                   st->stress_active ? pal.danger_r : ar, st->stress_active ? pal.danger_g : ag, st->stress_active ? pal.danger_b : ab, 0.94);
    ngfx_text(cr, "Noto Sans", 1, compact ? 10.5 : 11.2, sb_x + (compact ? 10 : 12), cy + 25,
              st->stress_active ? 1.0 : pal.on_acc_r, st->stress_active ? 1.0 : pal.on_acc_g, st->stress_active ? 1.0 : pal.on_acc_b, 1.0,
              compact ? (st->stress_active ? "Halt Load" : "CPU Load")
                      : (st->stress_active ? "Halt Load Test" : "Simulate CPU Load"));
    nature_add_hit(st, sb_x, cy + 8, btn_w, 26, st->stress_active ? HIT_MONITOR_CLEAR_STRESS : HIT_MONITOR_STRESS, 0, 0);

    if (st->monitor_tab == 0) {
        int px = cx + 16, pw = cw - 32, py = cy + 52;
        int card_w = (pw - 20) / 3;
        char v0[32], s0[48], v1[32], s1[48], v2[32], s2[48];
        int show_cores = std::clamp(st->cpu_count > 0 ? st->cpu_count : 4, 1, 8);
        std::snprintf(v0, sizeof(v0), "%.1f%%", st->total_cpu_pct);
        std::snprintf(s0, sizeof(s0), "%dC · load %.2f %.2f", show_cores, st->load_avg[0], st->load_avg[1]);
        std::snprintf(v1, sizeof(v1), "%d MiB", st->used_ram_mb);
        std::snprintf(s1, sizeof(s1), "of %dM%s · RSS %dM", st->total_ram_mb, st->cgroup_limited ? " (cg2)" : "", st->os_rss_mb);
        std::snprintf(v2, sizeof(v2), "%.0f FPS", st->measured_fps);
        std::snprintf(s2, sizeof(s2), "%.1f ms · Net %.0f/%.0fK", st->frame_ms, st->net_rx_kbps, st->net_tx_kbps);

        const char *kpis[3][3] = {
            {compact ? "CPU Load" : "Live CPU (/proc/stat)", v0, s0},
            {compact ? "System RAM" : (st->cgroup_limited ? "Memory (cgroup v2)" : "Memory (/proc/meminfo)"), v1, s1},
            {compact ? "Pacing" : "Compositor & Net", v2, s2},
        };
        for (int i = 0; i < 3; ++i) {
            int kx = px + i * (card_w + 10);
            ngfx_fill_rect(cr, kx, py, card_w, 70, 11, pal.card_r, pal.card_g, pal.card_b, 0.96);
            ngfx_stroke_rect(cr, kx + 0.5, py + 0.5, card_w - 1, 69, 11, 1.0, pal.border_r, pal.border_g, pal.border_b, pal.border_a);
            double sr = (i == 0 && st->stress_active) ? pal.warning_r : pal.ar_hi;
            double sg = (i == 0 && st->stress_active) ? pal.warning_g : pal.ag_hi;
            double sb = (i == 0 && st->stress_active) ? pal.warning_b : pal.ab_hi;
            ngfx_fill_rect(cr, kx + 12, py + 1, 28, 2.5, 1.2, sr, sg, sb, 0.85);
            ngfx_text_clipped(cr, "Noto Sans", 1, compact ? 10.2 : 11.0, kx + 12, py + 21, card_w - 20, mut_r, mut_g, mut_b, 1.0, kpis[i][0]);
            ngfx_text_clipped(cr, "JetBrains Mono", 1, compact ? 14.5 : 18.0, kx + 12, py + 45, card_w - 20, fg_r, fg_g, fg_b, 1.0, kpis[i][1]);
            ngfx_text_clipped(cr, "JetBrains Mono", 0, 9.2, kx + 12, py + 61, card_w - 20, mut_r, mut_g, mut_b, 0.9, kpis[i][2]);
        }

        /* Dynamic CPU Cores Bar Strips (1..8 cores) */
        int cy_cores = py + 80;
        int multi_row = (compact || show_cores > 4);
        int cores_h = multi_row ? (show_cores > 4 && compact ? 116 : 74) : 58;
        ngfx_fill_rect(cr, px, cy_cores, pw, cores_h, 11, pal.card_r, pal.card_g, pal.card_b, 0.96);
        ngfx_stroke_rect(cr, px + 0.5, cy_cores + 0.5, pw - 1, cores_h - 1, 11, 1.0, pal.border_r, pal.border_g, pal.border_b, pal.border_a);
        char core_hdr[32];
        std::snprintf(core_hdr, sizeof(core_hdr), "CPU Cores (%d)", st->cpu_count > 0 ? st->cpu_count : show_cores);
        ngfx_text(cr, "Noto Sans", 1, 11.3, px + 14, cy_cores + 20, fg_r, fg_g, fg_b, 1.0, core_hdr);

        if (compact) {
            int core_w = (pw - 36) / 2;
            for (int c = 0; c < show_cores; ++c) {
                int col = c % 2;
                int row = c / 2;
                int cx_pos = px + 14 + col * (core_w + 10);
                int ry_pos = cy_cores + 26 + row * 21;
                char cname[24];
                std::snprintf(cname, sizeof(cname), "C%d %.0f%%", c, st->core_usage[c]);
                ngfx_text(cr, "JetBrains Mono", 0, 9.6, cx_pos, ry_pos + 12, mut_r, mut_g, mut_b, 1.0, cname);
                int bx = cx_pos + 52;
                int bw = std::max(20, core_w - 54);
                ngfx_fill_rect(cr, bx, ry_pos + 2, bw, 11, 5.5, dark ? 1 : 0, dark ? 1 : 0, dark ? 1 : 0, 0.12);
                double fill_w = (std::min(100.0, st->core_usage[c]) / 100.0) * bw;
                if (fill_w > 0) {
                    double cr0 = st->core_usage[c] > 70 ? 0.92 : ar;
                    double cg0 = st->core_usage[c] > 70 ? 0.35 : ag;
                    double cb0 = st->core_usage[c] > 70 ? 0.25 : ab;
                    ngfx_fill_rect(cr, bx, ry_pos + 2, fill_w, 11, 5.5, cr0, cg0, cb0, 0.95);
                }
            }
        } else if (show_cores <= 4) {
            int core_w = (pw - 122) / show_cores;
            for (int c = 0; c < show_cores; ++c) {
                int cx_pos = px + 106 + c * (core_w + 8);
                char cname[24];
                std::snprintf(cname, sizeof(cname), "C%d  %.1f%%", c, st->core_usage[c]);
                ngfx_text(cr, "JetBrains Mono", 0, 10.0, cx_pos, cy_cores + 20, mut_r, mut_g, mut_b, 1.0, cname);

                ngfx_fill_rect(cr, cx_pos, cy_cores + 29, core_w, 13, 6.5, dark ? 1 : 0, dark ? 1 : 0, dark ? 1 : 0, 0.12);
                for (int q = 1; q <= 3; ++q) {
                    double tx = cx_pos + (core_w * q) / 4.0;
                    ngfx_fill_rect(cr, tx, cy_cores + 31, 1.0, 9, 0.5, dark ? 1 : 0, dark ? 1 : 0, dark ? 1 : 0, 0.08);
                }
                double fill_w = (std::min(100.0, st->core_usage[c]) / 100.0) * core_w;
                if (fill_w > 0) {
                    double cr0 = st->core_usage[c] > 70 ? 0.92 : ar;
                    double cg0 = st->core_usage[c] > 70 ? 0.35 : ag;
                    double cb0 = st->core_usage[c] > 70 ? 0.25 : ab;
                    ngfx_fill_gradient(cr, cx_pos, cy_cores + 29, fill_w, 13, 6.5,
                                       cr0, cg0, cb0, 0.95,
                                       cr0 * 1.25 > 1 ? 1 : cr0 * 1.25, cg0 * 1.25 > 1 ? 1 : cg0 * 1.25, cb0 * 1.25 > 1 ? 1 : cb0 * 1.25, 0.95, 1);
                }
            }
        } else {
            int core_w = (pw - 134) / 4;
            for (int c = 0; c < show_cores; ++c) {
                int col = c % 4;
                int row = c / 4;
                int cx_pos = px + 108 + col * (core_w + 8);
                int ry_pos = cy_cores + 14 + row * 26;
                char cname[24];
                std::snprintf(cname, sizeof(cname), "C%d %.0f%%", c, st->core_usage[c]);
                ngfx_text(cr, "JetBrains Mono", 0, 9.2, cx_pos, ry_pos + 2, mut_r, mut_g, mut_b, 1.0, cname);
                ngfx_fill_rect(cr, cx_pos, ry_pos + 7, core_w, 9, 4.5, dark ? 1 : 0, dark ? 1 : 0, dark ? 1 : 0, 0.12);
                double fill_w = (std::min(100.0, st->core_usage[c]) / 100.0) * core_w;
                if (fill_w > 0) {
                    double cr0 = st->core_usage[c] > 70 ? 0.92 : ar;
                    double cg0 = st->core_usage[c] > 70 ? 0.35 : ag;
                    double cb0 = st->core_usage[c] > 70 ? 0.25 : ab;
                    ngfx_fill_rect(cr, cx_pos, ry_pos + 7, fill_w, 9, 4.5, cr0, cg0, cb0, 0.95);
                }
            }
        }

        /* Real-time telemetry graph with CPU/RAM mode toggle, Peak/Avg stats & glowing live beacon */
        int gy = cy_cores + cores_h + 10;
        int gh = ch - (gy - cy) - 14;
        if (gh > 64) {
            int ram_mode = (st->monitor_chart_mode == 1);
            const double *series = ram_mode ? st->ram_history : st->cpu_history;
            double peak_v = 0.0, sum_v = 0.0;
            for (int i = 0; i < 48; ++i) {
                double v = std::clamp(series[i], 0.0, 100.0);
                if (v > peak_v) peak_v = v;
                sum_v += v;
            }
            double avg_v = sum_v / 48.0;

            ngfx_fill_rect(cr, px, gy, pw, gh, 11, pal.card_r, pal.card_g, pal.card_b, 0.96);
            ngfx_stroke_rect(cr, px + 0.5, gy + 0.5, pw - 1, gh - 1, 11, 1.0, pal.border_r, pal.border_g, pal.border_b, pal.border_a);

            const char *m_lbls[2] = {"CPU %", "RAM %"};
            for (int m = 0; m < 2; ++m) {
                int mx_tab = px + 12 + m * 54;
                int act = (st->monitor_chart_mode == m);
                int thov = (st->hover_hit_action == HIT_MONITOR_CHART_MODE && st->hover_hit_arg0 == m);
                ngfx_fill_rect(cr, mx_tab, gy + 7, 50, 20, 6,
                               act ? ar : (thov ? pal.hover_r : pal.elev_r),
                               act ? ag : (thov ? pal.hover_g : pal.elev_g),
                               act ? ab : (thov ? pal.hover_b : pal.elev_b), act ? 0.94 : 0.85);
                if (thov && !act) ngfx_stroke_rect(cr, mx_tab + 0.5, gy + 7.5, 49, 19, 6, 0.8, ar, ag, ab, 0.45);
                ngfx_text(cr, "JetBrains Mono", 1, 9.6, mx_tab + 8, gy + 20.5,
                          act ? pal.on_acc_r : fg_r, act ? pal.on_acc_g : fg_g, act ? pal.on_acc_b : fg_b, 1.0, m_lbls[m]);
                nature_add_hit(st, mx_tab, gy + 7, 50, 20, HIT_MONITOR_CHART_MODE, m, 0);
            }

            char peak_buf[48];
            std::snprintf(peak_buf, sizeof(peak_buf), "Peak %.1f%% · Avg %.1f%%", peak_v, avg_v);
            ngfx_text_clipped(cr, "JetBrains Mono", 0, 9.4, px + 126, gy + 20.5, 160,
                              dark ? pal.ar_hi : ar, dark ? pal.ag_hi : ag, dark ? pal.ab_hi : ab, 0.95, peak_buf);

            const char *host = st->cfg.device_name[0] ? st->cfg.device_name : "nature-pc";
            long d_used = st->disk_total_gb > st->disk_avail_gb ? (st->disk_total_gb - st->disk_avail_gb) : 0;
            char mem_line[128];
            std::snprintf(mem_line, sizeof(mem_line), "%.10s · Disk %ld/%ldG · PW:%s NM:%s",
                          host, d_used, st->disk_total_gb,
                          st->svc_pipewire ? "on" : "off",
                          st->svc_network ? "on" : "off");
            if (pw > 500) {
                ngfx_text_clipped(cr, "JetBrains Mono", 0, 9.2, px + pw - 260, gy + 20.5, 250, mut_r, mut_g, mut_b, 0.92, mem_line);
            }

            int chart_y = gy + 32;
            if (gh > 125) {
                double tot_m = st->total_ram_mb > 0 ? static_cast<double>(st->total_ram_mb) : 4096.0;
                double act_f = std::clamp(static_cast<double>(st->used_ram_mb) / tot_m, 0.0, 1.0);
                double cache_f = std::clamp(static_cast<double>(st->cached_ram_mb) / tot_m, 0.0, 1.0 - act_f);
                double swap_f = std::clamp(static_cast<double>(st->swap_used_mb) / tot_m, 0.0, 1.0 - act_f - cache_f);
                int mb_x = px + 12, mb_w = pw - 24, mb_y = gy + 32;
                ngfx_fill_rect(cr, mb_x, mb_y, mb_w, 7, 3.5, pal.elev_r, pal.elev_g, pal.elev_b, 0.95);
                double x_cur = mb_x;
                if (act_f > 0.0) {
                    double w0 = mb_w * act_f;
                    ngfx_fill_rect(cr, x_cur, mb_y, w0, 7, 3.5, ar, ag, ab, 0.96);
                    x_cur += w0;
                }
                if (cache_f > 0.0) {
                    double w1 = mb_w * cache_f;
                    ngfx_fill_rect(cr, x_cur, mb_y, w1, 7, 2.0, pal.success_r, pal.success_g, pal.success_b, 0.78);
                    x_cur += w1;
                }
                if (swap_f > 0.0) {
                    double w2 = std::max(4.0, mb_w * swap_f);
                    if (x_cur + w2 <= mb_x + mb_w) {
                        ngfx_fill_rect(cr, x_cur, mb_y, w2, 7, 2.0, pal.warning_r, pal.warning_g, pal.warning_b, 0.90);
                    }
                }
                char map_lbl[120];
                if (st->zram_used_mb > 0) {
                    std::snprintf(map_lbl, sizeof(map_lbl), "RAM Map: %dM active · %dM cache · zram %dM · %dM free",
                                  st->used_ram_mb, st->cached_ram_mb, st->zram_used_mb, st->avail_ram_mb);
                } else {
                    std::snprintf(map_lbl, sizeof(map_lbl), "RAM Map: %dM active · %dM cache · Swap %d/%dM · %dM free",
                                  st->used_ram_mb, st->cached_ram_mb, st->swap_used_mb, st->swap_total_mb, st->avail_ram_mb);
                }
                ngfx_text_clipped(cr, "JetBrains Mono", 0, 8.6, mb_x, mb_y + 18, mb_w, mut_r, mut_g, mut_b, 0.88, map_lbl);
                chart_y = gy + 56;
            }
            int gx = px + 36, gw = pw - 52, chart_h = (gy + gh - 12) - chart_y;
            const char *g_lbls[3] = {"100%", "50%", "0%"};
            g_ngfx.set_source_rgba(cr, pal.border_r, pal.border_g, pal.border_b, pal.border_a);
            g_ngfx.set_line_width(cr, 1.0);
            for (int g = 0; g <= 2; ++g) {
                double ly = chart_y + (chart_h * g) / 2.0;
                g_ngfx.move_to(cr, gx, ly);
                g_ngfx.line_to(cr, gx + gw, ly);
            }
            for (int v = 1; v <= 3; ++v) {
                double vx = gx + (gw * v) / 4.0;
                g_ngfx.move_to(cr, vx, chart_y);
                g_ngfx.line_to(cr, vx, chart_y + chart_h);
            }
            g_ngfx.stroke(cr);
            for (int g = 0; g <= 2; ++g) {
                double ly = chart_y + (chart_h * g) / 2.0;
                ngfx_text(cr, "JetBrains Mono", 0, 8.8, px + 8, ly + 3, mut_r, mut_g, mut_b, 0.72, g_lbls[g]);
            }

            double cr_c = ram_mode ? pal.success_r : ar;
            double cg_c = ram_mode ? pal.success_g : ag;
            double cb_c = ram_mode ? pal.success_b : ab;

            /* Gradient area fill under curve */
            cairo_pattern_t *cpat = g_ngfx.pattern_create_linear(gx, chart_y, gx, chart_y + chart_h);
            g_ngfx.pattern_add_color_stop_rgba(cpat, 0.0, cr_c, cg_c, cb_c, 0.36);
            g_ngfx.pattern_add_color_stop_rgba(cpat, 1.0, cr_c, cg_c, cb_c, 0.02);
            g_ngfx.new_sub_path(cr);
            for (int i = 0; i < 48; ++i) {
                double sx = gx + (gw * i) / 47.0;
                double sy = chart_y + chart_h - (std::min(100.0, series[i]) / 100.0) * chart_h;
                if (i == 0) g_ngfx.move_to(cr, sx, sy);
                else g_ngfx.line_to(cr, sx, sy);
            }
            g_ngfx.line_to(cr, gx + gw, chart_y + chart_h);
            g_ngfx.line_to(cr, gx, chart_y + chart_h);
            g_ngfx.close_path(cr);
            g_ngfx.set_source(cr, cpat);
            g_ngfx.fill(cr);
            g_ngfx.pattern_destroy(cpat);

            g_ngfx.save(cr);
            g_ngfx.set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
            g_ngfx.set_line_join(cr, CAIRO_LINE_JOIN_ROUND);

            /* Subtle outer glow line */
            g_ngfx.set_source_rgba(cr, cr_c, cg_c, cb_c, 0.25);
            g_ngfx.set_line_width(cr, 5.0);
            for (int i = 0; i < 48; ++i) {
                double sx = gx + (gw * i) / 47.0;
                double sy = chart_y + chart_h - (std::min(100.0, series[i]) / 100.0) * chart_h;
                if (i == 0) g_ngfx.move_to(cr, sx, sy);
                else g_ngfx.line_to(cr, sx, sy);
            }
            g_ngfx.stroke(cr);

            /* Crisp primary chart stroke */
            g_ngfx.set_source_rgba(cr, ram_mode ? pal.success_r : pal.ar_hi, ram_mode ? pal.success_g : pal.ag_hi, ram_mode ? pal.success_b : pal.ab_hi, 0.98);
            g_ngfx.set_line_width(cr, 2.2);
            double last_sy = chart_y + chart_h;
            for (int i = 0; i < 48; ++i) {
                double sx = gx + (gw * i) / 47.0;
                double sy = chart_y + chart_h - (std::min(100.0, series[i]) / 100.0) * chart_h;
                last_sy = sy;
                if (i == 0) g_ngfx.move_to(cr, sx, sy);
                else g_ngfx.line_to(cr, sx, sy);
            }
            g_ngfx.stroke(cr);
            g_ngfx.restore(cr);

            /* Live endpoint glow beacon */
            ngfx_radial_glow(cr, gx + gw, last_sy, 1.0, 12.0, cr_c, cg_c, cb_c, 0.55, 0.0);
            ngfx_fill_circle(cr, gx + gw, last_sy, 3.6, 1.0, 1.0, 1.0, 0.96);
        }
    } else if (st->monitor_tab == 1) {
        /* Task Manager Table with interactive sortable headers & scrollbar */
        int px = cx + 14, pw = cw - 34, py = cy + 48;
        ngfx_fill_rect(cr, px, py, pw, 22, 6, pal.sidebar_r, pal.sidebar_g, pal.sidebar_b, 0.95);
        int s_pid = (st->monitor_sort == 2);
        int s_cpu = (st->monitor_sort == 0);
        int s_mem = (st->monitor_sort == 1);

        ngfx_text(cr, "Noto Sans", 1, 10.6, px + 8, py + 15,
                  s_pid ? (dark ? pal.ar_hi : ar) : mut_r,
                  s_pid ? (dark ? pal.ag_hi : ag) : mut_g,
                  s_pid ? (dark ? pal.ab_hi : ab) : mut_b, 1.0, s_pid ? "PID ^" : "PID");
        nature_add_hit(st, px + 4, py, 44, 22, HIT_MONITOR_SORT, 2, 0);

        ngfx_text(cr, "Noto Sans", 1, 10.6, px + (compact ? 54 : 68), py + 15, mut_r, mut_g, mut_b, 1.0, "Process");
        if (!compact) ngfx_text(cr, "Noto Sans", 1, 10.6, px + 216, py + 15, mut_r, mut_g, mut_b, 1.0, "User");
        int col_cpu = compact ? (pw - 220) : 284;
        int col_mem = compact ? (pw - 166) : 352;
        if (col_cpu < px + 140) col_cpu = px + 140;
        if (col_mem < col_cpu + 48) col_mem = col_cpu + 48;

        ngfx_text(cr, "Noto Sans", 1, 10.6, px + col_cpu, py + 15,
                  s_cpu ? (dark ? pal.ar_hi : ar) : mut_r,
                  s_cpu ? (dark ? pal.ag_hi : ag) : mut_g,
                  s_cpu ? (dark ? pal.ab_hi : ab) : mut_b, 1.0, s_cpu ? "CPU v" : "CPU");
        nature_add_hit(st, px + col_cpu - 4, py, 56, 22, HIT_MONITOR_SORT, 0, 0);

        ngfx_text(cr, "Noto Sans", 1, 10.6, px + col_mem, py + 15,
                  s_mem ? (dark ? pal.ar_hi : ar) : mut_r,
                  s_mem ? (dark ? pal.ag_hi : ag) : mut_g,
                  s_mem ? (dark ? pal.ab_hi : ab) : mut_b, 1.0, s_mem ? "Mem v" : "Mem");
        nature_add_hit(st, px + col_mem - 4, py, 56, 22, HIT_MONITOR_SORT, 1, 0);

        if (!compact) ngfx_text(cr, "Noto Sans", 1, 10.6, px + 430, py + 15, mut_r, mut_g, mut_b, 1.0, "Nice");
        ngfx_text(cr, "Noto Sans", 1, 10.6, px + pw - (compact ? 68 : 158), py + 15, mut_r, mut_g, mut_b, 1.0, "Action");

        int row_h = 27;
        int list_y = py + 26;
        int avail_h = (cy + ch) - list_y - 8;
        int vis_rows = std::max(1, avail_h / row_h);
        st->monitor_max_scroll = std::max(0, st->proc_count - vis_rows);
        if (st->monitor_scroll > st->monitor_max_scroll) st->monitor_scroll = st->monitor_max_scroll;
        if (st->monitor_scroll < 0) st->monitor_scroll = 0;

        int ry = list_y;
        for (int i = st->monitor_scroll; i < st->proc_count && ry + row_h <= cy + ch - 4; ++i) {
            const ProcEntry &p = st->procs[i];
            int row_hov = (st->hover_mx >= px && st->hover_mx <= px + pw && st->hover_my >= ry && st->hover_my <= ry + 25);
            if (row_hov) ngfx_fill_rect(cr, px, ry, pw, 25, 6, ar, ag, ab, 0.14);
            else if (i % 2 == 0) ngfx_fill_rect(cr, px, ry, pw, 25, 6, pal.card_r, pal.card_g, pal.card_b, 0.72);
            char b_pid[16], b_cpu[16], b_ram[16], b_nice[16];
            std::snprintf(b_pid, sizeof(b_pid), "%d", p.pid);
            std::snprintf(b_cpu, sizeof(b_cpu), "%.1f%%", p.cpu_pct);
            std::snprintf(b_ram, sizeof(b_ram), "%dM", p.ram_mb);
            std::snprintf(b_nice, sizeof(b_nice), "%+d", p.nice_val);

            ngfx_text(cr, "JetBrains Mono", 0, 10.6, px + 8, ry + 16, mut_r, mut_g, mut_b, 1.0, b_pid);
            int running_s = (std::strcmp(p.state, "Run") == 0);
            int load_s = (std::strcmp(p.state, "Load") == 0);
            ngfx_fill_circle(cr, px + (compact ? 47 : 60), ry + 12.5, 2.5,
                             load_s ? pal.warning_r : (running_s ? pal.ar_hi : mut_r),
                             load_s ? pal.warning_g : (running_s ? pal.ag_hi : mut_g),
                             load_s ? pal.warning_b : (running_s ? pal.ab_hi : mut_b), 0.9);
            ngfx_text_clipped(cr, "Noto Sans", 1, 11.2, px + (compact ? 54 : 68), ry + 16,
                              col_cpu - (compact ? 60 : 74), fg_r, fg_g, fg_b, 1.0, p.name);
            if (!compact) ngfx_text(cr, "Noto Sans", 0, 11.0, px + 216, ry + 16, mut_r, mut_g, mut_b, 1.0, p.user);
            ngfx_text(cr, "JetBrains Mono", 0, 10.8, px + col_cpu, ry + 16,
                      p.cpu_pct > 10.0 ? pal.warning_r : fg_r, p.cpu_pct > 10.0 ? pal.warning_g : fg_g, p.cpu_pct > 10.0 ? pal.warning_b : fg_b, 1.0, b_cpu);
            ngfx_text(cr, "JetBrains Mono", 0, 10.8, px + col_mem, ry + 16, fg_r, fg_g, fg_b, 0.9, b_ram);
            if (!compact) ngfx_text(cr, "JetBrains Mono", 0, 10.8, px + 430, ry + 16, mut_r, mut_g, mut_b, 0.9, b_nice);

            if (p.killable) {
                int btn_x = px + pw - (compact ? 64 : 72);
                int khov = (st->hover_mx >= btn_x && st->hover_mx <= btn_x + (compact ? 58 : 64) && st->hover_my >= ry + 3 && st->hover_my <= ry + 22);
                ngfx_fill_rect(cr, btn_x, ry + 3, compact ? 58 : 64, 19, 5, pal.danger_r, pal.danger_g, pal.danger_b, khov ? 0.30 : 0.16);
                ngfx_text(cr, "Noto Sans", 1, 10.0, btn_x + (compact ? 7 : 9), ry + 16, pal.danger_r, pal.danger_g, pal.danger_b, 1.0, "End Task");
                nature_add_hit(st, btn_x, ry + 3, compact ? 58 : 64, 19, HIT_MONITOR_KILL, p.pid, 0);

                if (compact && p.win_id >= 0) {
                    int foc_x = btn_x - 38;
                    int fhov = (st->hover_mx >= foc_x && st->hover_mx <= foc_x + 34 && st->hover_my >= ry + 3 && st->hover_my <= ry + 22);
                    ngfx_fill_rect(cr, foc_x, ry + 3, 34, 19, 5, ar, ag, ab, fhov ? 0.96 : 0.24);
                    ngfx_text(cr, "Noto Sans", 1, 9.4, foc_x + 6, ry + 16,
                              fhov ? pal.on_acc_r : (dark ? pal.ar_hi : ar),
                              fhov ? pal.on_acc_g : (dark ? pal.ag_hi : ag),
                              fhov ? pal.on_acc_b : (dark ? pal.ab_hi : ab), 1.0, "Foc");
                    nature_add_hit(st, foc_x, ry + 3, 34, 19, HIT_MONITOR_FOCUS, p.win_id, 0);
                } else if (!compact) {
                    int renice_x = px + pw - 134;
                    int rhov = (st->hover_mx >= renice_x && st->hover_mx <= renice_x + 56 && st->hover_my >= ry + 3 && st->hover_my <= ry + 22);
                    ngfx_fill_rect(cr, renice_x, ry + 3, 56, 19, 5, pal.elev_r, pal.elev_g, pal.elev_b, rhov ? 1.0 : 0.85);
                    ngfx_text(cr, "Noto Sans", 0, 10.0, renice_x + 9, ry + 16, fg_r, fg_g, fg_b, 1.0, "Renice");
                    nature_add_hit(st, renice_x, ry + 3, 56, 19, HIT_MONITOR_RENICE, p.pid, 0);

                    if (p.win_id >= 0) {
                        int foc_x = px + pw - 190;
                        int fhov = (st->hover_mx >= foc_x && st->hover_mx <= foc_x + 50 && st->hover_my >= ry + 3 && st->hover_my <= ry + 22);
                        ngfx_fill_rect(cr, foc_x, ry + 3, 50, 19, 5, ar, ag, ab, fhov ? 0.96 : 0.24);
                        ngfx_text(cr, "Noto Sans", 1, 9.8, foc_x + 9, ry + 16,
                                  fhov ? pal.on_acc_r : (dark ? pal.ar_hi : ar),
                                  fhov ? pal.on_acc_g : (dark ? pal.ag_hi : ag),
                                  fhov ? pal.on_acc_b : (dark ? pal.ab_hi : ab), 1.0, "Focus");
                        nature_add_hit(st, foc_x, ry + 3, 50, 19, HIT_MONITOR_FOCUS, p.win_id, 0);
                    }
                }
            }
            ry += row_h;
        }

        nature_draw_scrollbar(st, cr, cx + cw - 11, list_y, avail_h,
                              st->proc_count, vis_rows, st->monitor_scroll,
                              st->monitor_max_scroll, SCROLL_MONITOR, dark);
    } else {
        /* Tab 2: Disks, Network I/O & Core Units */
        int px = cx + 16, pw = cw - 32, py = cy + 52;
        int half_w = (pw - 14) / 2;
        if (compact) half_w = pw;

        /* Card 1: Storage & VFS Volume */
        int h_card1 = 148;
        ngfx_fill_rect(cr, px, py, half_w, h_card1, 11, pal.card_r, pal.card_g, pal.card_b, 0.96);
        ngfx_stroke_rect(cr, px + 0.5, py + 0.5, half_w - 1, h_card1 - 1, 11, 1.0, pal.border_r, pal.border_g, pal.border_b, pal.border_a);
        ngfx_fill_rect(cr, px + 14, py + 1, 32, 2.5, 1.2, ar, ag, ab, 0.85);

        ngfx_text(cr, "Noto Sans", 1, 11.5, px + 14, py + 22, fg_r, fg_g, fg_b, 1.0, "Storage & VFS Volume");
        ngfx_text(cr, "Noto Sans", 0, 9.5, px + 14, py + 36, mut_r, mut_g, mut_b, 0.9, "Root filesystem · statvfs() telemetry");

        long used_gb = (st->disk_total_gb > st->disk_avail_gb) ? (st->disk_total_gb - st->disk_avail_gb) : 0;
        char d_val[64], d_sub[64];
        std::snprintf(d_val, sizeof(d_val), "%ld GB used / %ld GB", used_gb, st->disk_total_gb);
        std::snprintf(d_sub, sizeof(d_sub), "%ld GB free · %d%% full", st->disk_avail_gb, st->disk_used_pct);

        ngfx_text(cr, "JetBrains Mono", 1, compact ? 13.5 : 15.0, px + 14, py + 62, fg_r, fg_g, fg_b, 1.0, d_val);
        ngfx_text(cr, "JetBrains Mono", 0, 9.5, px + 14, py + 78, mut_r, mut_g, mut_b, 1.0, d_sub);

        /* Progress Bar */
        int bar_w = half_w - 28;
        ngfx_fill_rect(cr, px + 14, py + 88, bar_w, 10, 5, dark ? 1 : 0, dark ? 1 : 0, dark ? 1 : 0, 0.12);
        double d_fill = (std::clamp(st->disk_used_pct, 1, 100) / 100.0) * bar_w;
        ngfx_fill_rect(cr, px + 14, py + 88, d_fill, 10, 5,
                       st->disk_used_pct > 85 ? pal.danger_r : ar,
                       st->disk_used_pct > 85 ? pal.danger_g : ag,
                       st->disk_used_pct > 85 ? pal.danger_b : ab, 0.95);

        /* Mini chips for storage */
        char swp_lbl[48];
        std::snprintf(swp_lbl, sizeof(swp_lbl), "Swap/ZRAM: %dM", st->zram_used_mb > 0 ? st->zram_used_mb : st->swap_used_mb);
        ngfx_text(cr, "JetBrains Mono", 0, 9.2, px + 14, py + 116, mut_r, mut_g, mut_b, 0.95, "Mount: / (rw)");
        ngfx_text(cr, "JetBrains Mono", 0, 9.2, px + (compact ? 130 : 160), py + 116, mut_r, mut_g, mut_b, 0.95, swp_lbl);
        ngfx_text(cr, "JetBrains Mono", 0, 9.2, px + 14, py + 132, mut_r, mut_g, mut_b, 0.95,
                  st->session_restored ? "Session: Synchronized (~/.config/nature-os)" : "Session: Ready");

        /* Card 2: Network Throughput & I/O */
        int n_px = compact ? px : (px + half_w + 14);
        int n_py = compact ? (py + h_card1 + 10) : py;
        int h_card2 = 148;
        ngfx_fill_rect(cr, n_px, n_py, half_w, h_card2, 11, pal.card_r, pal.card_g, pal.card_b, 0.96);
        ngfx_stroke_rect(cr, n_px + 0.5, n_py + 0.5, half_w - 1, h_card2 - 1, 11, 1.0, pal.border_r, pal.border_g, pal.border_b, pal.border_a);
        ngfx_fill_rect(cr, n_px + 14, n_py + 1, 32, 2.5, 1.2, pal.ar_hi, pal.ag_hi, pal.ab_hi, 0.85);

        ngfx_text(cr, "Noto Sans", 1, 11.5, n_px + 14, n_py + 22, fg_r, fg_g, fg_b, 1.0, "Network I/O & Sockets");
        ngfx_text(cr, "Noto Sans", 0, 9.5, n_px + 14, n_py + 36, mut_r, mut_g, mut_b, 0.9, "/proc/net/dev · Real interface bandwidth");

        char rx_str[64], tx_str[64];
        std::snprintf(rx_str, sizeof(rx_str), "In:  %.1f KB/s", st->net_rx_kbps);
        std::snprintf(tx_str, sizeof(tx_str), "Out: %.1f KB/s", st->net_tx_kbps);

        ngfx_text(cr, "JetBrains Mono", 1, compact ? 13.5 : 15.0, n_px + 14, n_py + 62,
                  st->net_rx_kbps > 50.0 ? pal.ar_hi : fg_r,
                  st->net_rx_kbps > 50.0 ? pal.ag_hi : fg_g,
                  st->net_rx_kbps > 50.0 ? pal.ab_hi : fg_b, 1.0, rx_str);
        ngfx_text(cr, "JetBrains Mono", 1, compact ? 13.5 : 15.0, n_px + (compact ? 130 : 160), n_py + 62,
                  st->net_tx_kbps > 50.0 ? pal.warning_r : fg_r,
                  st->net_tx_kbps > 50.0 ? pal.warning_g : fg_g,
                  st->net_tx_kbps > 50.0 ? pal.warning_b : fg_b, 1.0, tx_str);

        char net_dt[80];
        std::snprintf(net_dt, sizeof(net_dt), "Pacing: %.0f FPS (%.1f ms) · NAT3: :3000",
                      st->measured_fps, st->frame_ms);
        ngfx_text(cr, "JetBrains Mono", 0, 9.5, n_px + 14, n_py + 84, mut_r, mut_g, mut_b, 1.0, net_dt);

        ngfx_text(cr, "JetBrains Mono", 0, 9.2, n_px + 14, n_py + 116, mut_r, mut_g, mut_b, 0.95, "Interface: eth0 / loopback");
        ngfx_text(cr, "JetBrains Mono", 0, 9.2, n_px + (compact ? 130 : 160), n_py + 116, mut_r, mut_g, mut_b, 0.95,
                  st->settings_net_ping_ms >= 0 ? "Ping: Tested" : "Gateway: Active");
        ngfx_text(cr, "JetBrains Mono", 0, 9.2, n_px + 14, n_py + 132, mut_r, mut_g, mut_b, 0.95, "Socket Protocol: WebSocket / Binary Tiles");

        /* Card 3: System Environment & Core Daemons */
        int py_c3 = compact ? (n_py + h_card2 + 10) : (py + h_card1 + 12);
        int h_card3 = (cy + ch) - py_c3 - 10;
        if (h_card3 < 140) h_card3 = 140;

        ngfx_fill_rect(cr, px, py_c3, pw, h_card3, 11, pal.card_r, pal.card_g, pal.card_b, 0.96);
        ngfx_stroke_rect(cr, px + 0.5, py_c3 + 0.5, pw - 1, h_card3 - 1, 11, 1.0, pal.border_r, pal.border_g, pal.border_b, pal.border_a);

        ngfx_text(cr, "Noto Sans", 1, 11.5, px + 14, py_c3 + 22, fg_r, fg_g, fg_b, 1.0, "Kernel Subsystems & Core Daemons");

        /* Uptime & Load Averages */
        long uph = st->uptime_sec / 3600;
        long upm = (st->uptime_sec % 3600) / 60;
        long ups = st->uptime_sec % 60;
        char sys_summary[128];
        std::snprintf(sys_summary, sizeof(sys_summary),
                      "Uptime: %ldh %02ldm %02lds · Load avg: %.2f, %.2f, %.2f · CPU: %.30s",
                      uph, upm, ups, st->load_avg[0], st->load_avg[1], st->load_avg[2],
                      st->cpu_model[0] ? st->cpu_model : "SMP");
        ngfx_text_clipped(cr, "JetBrains Mono", 0, 9.5, px + 14, py_c3 + 40, pw - 28, mut_r, mut_g, mut_b, 1.0, sys_summary);

        /* Daemons Grid (4 service cards) */
        int d_cols = compact ? 2 : 4;
        int d_w = (pw - 28 - (d_cols - 1) * 8) / d_cols;
        int d_y = py_c3 + 52;
        int d_h = 56;

        struct SvcInfo {
            const char *name;
            const char *desc;
            int active;
            int toggle_idx;
        } svcs[4] = {
            {"PipeWire Audio", "Low-latency sound", st->svc_pipewire, 0},
            {"NetworkManager", "DHCP & DNS routes", st->svc_network, 1},
            {"systemd-oomd", "OOM guardian", st->svc_oomd, 2},
            {"nature-comp", "Cairo 120Hz pipeline", 1, -1}
        };

        for (int s = 0; s < 4; ++s) {
            int col = s % d_cols;
            int row = s / d_cols;
            int sx = px + 14 + col * (d_w + 8);
            int sy = d_y + row * (d_h + 8);
            if (sy + d_h > py_c3 + h_card3 - 4) break;

            int is_hov = (st->hover_mx >= sx && st->hover_mx <= sx + d_w && st->hover_my >= sy && st->hover_my <= sy + d_h);
            ngfx_fill_rect(cr, sx, sy, d_w, d_h, 8, pal.elev_r, pal.elev_g, pal.elev_b, is_hov ? 0.98 : 0.82);

            double dot_r = svcs[s].active ? pal.ar_hi : pal.mut_r;
            double dot_g = svcs[s].active ? pal.ag_hi : pal.mut_g;
            double dot_b = svcs[s].active ? pal.ab_hi : pal.mut_b;
            ngfx_fill_circle(cr, sx + 12, sy + 18, 3.5, dot_r, dot_g, dot_b, 1.0);

            ngfx_text_clipped(cr, "Noto Sans", 1, 10.5, sx + 22, sy + 22, d_w - 26, fg_r, fg_g, fg_b, 1.0, svcs[s].name);
            ngfx_text_clipped(cr, "Noto Sans", 0, 9.0, sx + 12, sy + 38, d_w - 18, mut_r, mut_g, mut_b, 0.9, svcs[s].desc);

            char st_lbl[24];
            std::snprintf(st_lbl, sizeof(st_lbl), "%s", svcs[s].active ? "RUNNING" : "STOPPED");
            ngfx_text(cr, "JetBrains Mono", 1, 8.5, sx + 12, sy + 49,
                      svcs[s].active ? pal.ar_hi : pal.danger_r,
                      svcs[s].active ? pal.ag_hi : pal.danger_g,
                      svcs[s].active ? pal.ab_hi : pal.danger_b, 0.95, st_lbl);

            if (svcs[s].toggle_idx >= 0) {
                nature_add_hit(st, sx, sy, d_w, d_h, HIT_SETTINGS_SVC_TOGGLE, svcs[s].toggle_idx, 0);
            }
        }
    }
}

void nature_monitor_wheel(NatureOSState *st, int delta_y) {
    if (st->monitor_tab != 1) return;
    int step = (delta_y > 0) ? 1 : -1;
    st->monitor_scroll += step;
    if (st->monitor_scroll < 0) st->monitor_scroll = 0;
    if (st->monitor_scroll > st->monitor_max_scroll) st->monitor_scroll = st->monitor_max_scroll;
}

void nature_monitor_click(NatureOSState *st, const HitRect *hit) {
    if (hit->action_id == HIT_MONITOR_TAB) {
        st->monitor_tab = hit->arg0;
        st->monitor_scroll = 0;
    } else if (hit->action_id == HIT_MONITOR_STRESS) {
        nature_monitor_set_stress(st, 1);
    } else if (hit->action_id == HIT_MONITOR_CLEAR_STRESS) {
        nature_monitor_set_stress(st, 0);
    } else if (hit->action_id == HIT_MONITOR_STRESS_THREADS) {
        nature_monitor_set_threads(st, hit->arg0);
    } else if (hit->action_id == HIT_MONITOR_KILL) {
        int pid = hit->arg0;
        for (int i = 0; i < st->proc_count; ++i) {
            if (st->procs[i].pid == pid && st->procs[i].killable) {
                if (st->procs[i].win_id >= 0) {
                    nature_close_window(st, st->procs[i].win_id);
                } else if (pid >= 4800) {
                    nature_monitor_set_stress(st, 0);
                }
                nature_notify(st, "Task Manager", "Process terminated cleanly.");
                break;
            }
        }
        nature_monitor_tick(st);
    } else if (hit->action_id == HIT_MONITOR_CHART_MODE) {
        st->monitor_chart_mode = hit->arg0 ? 1 : 0;
    } else if (hit->action_id == HIT_MONITOR_SORT) {
        st->monitor_sort = hit->arg0 % 3;
        nature_monitor_tick(st);
    } else if (hit->action_id == HIT_MONITOR_FOCUS) {
        int wid = hit->arg0;
        for (int i = 0; i < st->window_count; ++i) {
            if (st->windows[i].id == wid) {
                st->active_workspace = st->windows[i].workspace;
                nature_focus_window(st, wid);
                break;
            }
        }
    } else if (hit->action_id == HIT_MONITOR_RENICE) {
        int pid = hit->arg0;
        for (int i = 0; i < st->proc_count; ++i) {
            if (st->procs[i].pid == pid) {
                st->procs[i].nice_val = (st->procs[i].nice_val == 10) ? -5 : 10;
                break;
            }
        }
        nature_notify(st, "Task Scheduler", "Process scheduling nice priority adjusted.");
    } else if (hit->action_id == HIT_SETTINGS_SVC_TOGGLE) {
        if (hit->arg0 == 0) {
            st->svc_pipewire = !st->svc_pipewire;
            nature_notify(st, "Audio Server", st->svc_pipewire ? "PipeWire sound server active." : "PipeWire sound server paused.");
        } else if (hit->arg0 == 1) {
            st->svc_network = !st->svc_network;
            nature_notify(st, "Network Engine", st->svc_network ? "NetworkManager connection online." : "NetworkManager paused.");
        } else if (hit->arg0 == 2) {
            st->svc_oomd = !st->svc_oomd;
            nature_notify(st, "OOM Daemon", st->svc_oomd ? "systemd-oomd guardian active." : "systemd-oomd guardian paused.");
        }
    }
}

} // extern "C"
