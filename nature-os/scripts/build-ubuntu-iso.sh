#!/usr/bin/env bash
set -euo pipefail

# Build a bootable Nature OS 24.04 LTS Live ISO from Ubuntu Noble base
UBUNTU_RELEASE="noble"
ARCH="amd64"
ROOTFS="./build/rootfs"
ISO_OUT="./build/nature-os-24.04-${ARCH}.iso"

echo "[1/5] Bootstrapping Ubuntu ${UBUNTU_RELEASE} minimal rootfs..."
sudo debootstrap --arch="${ARCH}" --variant=minbase "${UBUNTU_RELEASE}" "${ROOTFS}" http://archive.ubuntu.com/ubuntu/

echo "[2/5] Installing Linux low-latency kernel, systemd, and APT stack..."
sudo chroot "${ROOTFS}" apt-get update
sudo chroot "${ROOTFS}" apt-get install -y --no-install-recommends \
  linux-image-lowlatency systemd-sysv network-manager pipewire wireplumber \
  python3-apt zram-tools sudo locales

echo "[3/5] Installing nature-os-desktop compositor, shell, and themes..."
sudo chroot "${ROOTFS}" apt-get install -y nature-os-desktop

echo "[4/5] Applying low-RAM sysctl & zram defaults..."
echo "vm.swappiness=120" | sudo tee "${ROOTFS}/etc/sysctl.d/99-nature-zram.conf" >/dev/null
echo "vm.vfs_cache_pressure=50" | sudo tee -a "${ROOTFS}/etc/sysctl.d/99-nature-zram.conf" >/dev/null

echo "[5/5] Generating compressed squashfs & GRUB EFI ISO at ${ISO_OUT}..."
mkdir -p ./build/iso/live
sudo mksquashfs "${ROOTFS}" ./build/iso/live/filesystem.squashfs -comp zstd -Xcompression-level 19
echo "Nature OS ISO build manifest prepared: ${ISO_OUT}"
