#!/usr/bin/env bash
#
# 01-install-base.sh -- install the always-needed Fedora packages for this project.
#
# One package group per script, no options: WHICH optional groups also get installed is
# decided by the Dockerfile's ARG `if` blocks (or by a human choosing which scripts to
# run). Same packages during `podman build`, on a bare Fedora host, or in a guest with
# no container runtime. Run base first, then any 0N-install-*.sh group you want.
#
# `--skip-unavailable` matches the original Dockerfile: skip any package missing from
# the enabled repos rather than failing the whole install. Multiple dnf calls, so
# accumulate a non-zero exit if any fails.
set -uo pipefail

if ! command -v dnf >/dev/null 2>&1; then
    echo "01-install-base.sh: needs 'dnf' (this installs Fedora packages), not found." >&2
    echo "Run on a Fedora host/guest, or inside the project's Fedora-based image." >&2
    exit 1
fi

status=0

dnf upgrade -y || status=1

dnf install -y --skip-unavailable \
              SDL3 \
              SDL3-devel \
              SDL3_sound \
              SDL3_sound-devel \
              clang \
              clang-tools-extra \
              cmake \
              desktop-file-utils \
              emacs \
              ffmpeg \
              file \
              fuse-libs \
              g++ \
              gcc \
              gdb \
              glew \
              glew-devel \
              libogg libogg-devel \
              libpng \
              libpng-devel \
              libvorbis-devel \
              lua-devel \
              lldb \
              man \
              man-db \
              man-pages \
              mesa-libGLU-devel \
              mikmod \
              mikmod-devel \
              nano \
              ninja-build \
              patchelf \
              squashfs-tools \
              tmux \
              vorbis-tools \
              wget \
              zlib \
              zlib-devel || status=1

exit $status
