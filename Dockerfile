FROM registry.fedoraproject.org/fedora:44

ARG USE_GRAPHICS=1
# Run the UBSan(trap)+ASan sanitizer gate at image-build time. Default off here
# (fast path); Makefile.docker passes 1. Requires USE_GRAPHICS=1 (needs Xvfb + Mesa
# for the headless GL run). See tools/sanitizer-gate.sh and tasks/add-sanitizer-gate.md.
ARG RUN_SANITIZERS=0
# Install Emacs + its vendored C LSP config (clangd via lsp-mode). Default off
# here (fast path); the Makefile passes 1. clangd itself (clang-tools-extra) is
# always installed by 01-install-base.sh so it is available regardless.
ARG USE_EMACS=0

# System-package installation lives in per-group scripts (entrypoint/0N-install-*.sh),
# host-runnable with no container runtime; the scripts take no options -- WHICH optional
# groups run is decided here by the ARG `if` blocks. The dnf cache mount, the tsflags
# `sed`, and keepcache stay in the Dockerfile (build plumbing). 01-install-base.sh also
# does `dnf upgrade`.
COPY entrypoint/01-install-base.sh entrypoint/02-install-graphics.sh /usr/local/bin/

RUN --mount=type=cache,target=/var/cache/libdnf5 \
    --mount=type=cache,target=/var/lib/dnf \
    sed -i -e "s@tsflags=nodocs@#tsflags=nodocs@g" /etc/dnf/dnf.conf && \
    echo "keepcache=True" >> /etc/dnf/dnf.conf && \
    /usr/local/bin/01-install-base.sh && \
    if [ "$USE_GRAPHICS" = "1" ]; then /usr/local/bin/02-install-graphics.sh; fi && \
    echo 'set debuginfod enabled off' > /root/.gdbinit

# Emacs + vendored MELPA packages, only when USE_EMACS=1. Pre-populates
# ~/.emacs.d/elpa at build time; the Makefile bind-mounts the host .emacs.d
# (with its committed elpa/) over this at run time, and `make update-emacs-packages`
# refreshes it. Mirrors billsEmacsConfigs/c.
COPY entrypoint/dotfiles/.emacs.d/install-melpa-packages.el /root/.emacs.d/install-melpa-packages.el
RUN --mount=type=cache,target=/var/cache/libdnf5 \
    --mount=type=cache,target=/var/lib/dnf \
    if [ "$USE_EMACS" = "1" ]; then \
        dnf install -y emacs && \
        emacs --batch --load /root/.emacs.d/install-melpa-packages.el ; \
    fi

COPY .clang-tidy /gltron/.clang-tidy
COPY CMake /gltron/CMake/
COPY CMakeLists.txt /gltron/CMakeLists.txt
COPY art /gltron/art/
COPY packaging /gltron/packaging/
COPY ChangeLog /gltron/ChangeLog
COPY COPYING /gltron/COPYING
COPY CREDITS /gltron/CREDITS
COPY data /gltron/data/
COPY docs /gltron/docs/
COPY INSTALL /gltron/INSTALL
COPY levels /gltron/levels/
COPY music /gltron/music/
COPY nebu /gltron/nebu/
COPY README /gltron/README
COPY resource /gltron/resource/
COPY scripts /gltron/scripts/
COPY sounds /gltron/sounds/
COPY src /gltron/src/
COPY third_party /gltron/third_party/
COPY TODO /gltron/TODO
COPY tools /gltron/tools/


RUN cmake -S /gltron -B /bld -G Ninja \
          -DCMAKE_BUILD_TYPE=Debug \
          -DCMAKE_INSTALL_PREFIX=/bldInstall \
    && cmake --build /bld -j \
    && cmake --install /bld \
    && (cd /bld && ctest --output-on-failure)

# Sanitizer gate: compile under UBSan(trap) + ASan and exercise gltron headless,
# failing the image on any UB/memory error. No-op unless RUN_SANITIZERS=1.
RUN if [ "$RUN_SANITIZERS" = "1" ]; then \
        bash /gltron/tools/sanitizer-gate.sh /gltron ; \
    fi

COPY .clang-format /gltron/

RUN echo "source ~/.extrabashrc" >> ~/.bashrc

