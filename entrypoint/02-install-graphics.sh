#!/usr/bin/env bash
#
# 02-install-graphics.sh -- X11 / Mesa / Vulkan libraries for on-screen graphics.
# Corresponds to the Dockerfile's USE_GRAPHICS flag; the Dockerfile runs this only when
# USE_GRAPHICS=1. No options -- see 01-install-base.sh for the design.
#
# Single dnf call, so its own exit status is this script's exit status.
set -uo pipefail

dnf install -y \
    mesa-dri-drivers \
    xorg-x11-server-Xvfb \
    libXScrnSaver \
    libXtst \
    libXcomposite \
    libXcursor \
    libXdamage \
    libXfixes \
    libXft \
    libXi \
    libXinerama \
    libXmu \
    libXrandr \
    libXrender \
    libXres \
    libXv \
    libXxf86vm \
    libglvnd-gles \
    mesa-demos \
    vulkan-tools
