# Nature OS 24.04 LTS ("Canopy")

Nature OS is a custom, ultra-lightweight desktop operating system built directly on **Ubuntu 24.04 LTS (Noble Numbat)**. It pairs Ubuntu's APT/dpkg/systemd foundation with a custom Wayland compositor (`nature-compositor`) written in C and native system applications written in **C and C++** (`GTK4`, `GIO`, `VTE`, `libapt-pkg`, `WebKitGTK`).

## Resource Profile

- **Idle Memory Footprint**: ~310 MiB RSS (vs ~1.1 GiB on standard GNOME Shell)
- **Idle CPU Overhead**: < 1.2% on modern x86_64 dual/quad-core processors
- **Compositor Latency**: Damage-region tracked Wayland rendering with zero-copy DMA-BUF presentation
- **Memory Management**: `zram` LZ4 compressed swap (`50%` physical RAM ratio) + `systemd-oomd` cgroup isolation

## Directory Layout

- `compositor/` — `wlroots`-based C Wayland compositor & frame-pacing scheduler
- `shell/` — Vala + GTK4 desktop panel, botanical dock, and app overview
- `apps/` — Native C & C++ system applications (`nature-files`, `nature-terminal`, `nature-monitor`, `nature-settings`, `nature-store`, `nature-media`, `nature-browser`)
- `themes/` — GTK4 & Libadwaita stylesheets (`gtk.css`)
- `schemas/` — GSettings XML configuration schemas (`org.natureos.desktop`)
- `systemd/` — User session unit and low-RAM `zram` generator config
- `packaging/` — Debian/Ubuntu `dpkg` control metadata (`nature-os-desktop`)
- `scripts/` — `debootstrap` + `squashfs` ISO builder for Ubuntu Noble base

## Building on Ubuntu 24.04

```bash
sudo apt install meson ninja-build libwlroots-dev libgtk-4-dev valac libapt-pkg-dev libvte-2.91-gtk4-dev
meson setup build --prefix=/usr
ninja -C build
sudo ninja -C build install
```
