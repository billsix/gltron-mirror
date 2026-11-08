FROM registry.fedoraproject.org/fedora:45

ARG USE_GRAPHICS=1
# Run the UBSan(trap)+ASan sanitizer gate at image-build time. Default off here
# (fast path); Makefile.docker passes 1. Requires USE_GRAPHICS=1 (needs Xvfb + Mesa
# for the headless GL run). See tools/sanitizer-gate.sh and tasks/add-sanitizer-gate.md.
ARG RUN_SANITIZERS=0
# Install the Emacs binary (03-install-emacs.sh). Default off here (fast path); the
# Makefile passes 1. clangd itself (clang-tools-extra) is always installed by
# 01-install-base.sh so it is available regardless. Emacs's packages are never
# installed here: `make shell` bind-mounts the vendored entrypoint/dotfiles/.emacs.d
# and init.el byte-compiles its elpa/ on Emacs's first start.
ARG USE_EMACS=0

# System-package installation lives in per-group scripts (entrypoint/0N-install-*.sh),
# host-runnable with no container runtime; the scripts take no options -- WHICH optional
# groups run is decided here by the ARG `if` blocks. The dnf cache mount, the tsflags
# `sed`, and keepcache stay in the Dockerfile (build plumbing). 01-install-base.sh also
# does `dnf upgrade`.
COPY entrypoint/01-install-base.sh entrypoint/02-install-graphics.sh \
     entrypoint/03-install-emacs.sh /usr/local/bin/

RUN --mount=type=cache,target=/var/cache/libdnf5 \
    --mount=type=cache,target=/var/lib/dnf \
    sed -i -e "s@tsflags=nodocs@#tsflags=nodocs@g" /etc/dnf/dnf.conf && \
    echo "keepcache=True" >> /etc/dnf/dnf.conf && \
    /usr/local/bin/01-install-base.sh && \
    if [ "$USE_GRAPHICS" = "1" ]; then /usr/local/bin/02-install-graphics.sh; fi && \
    if [ "$USE_EMACS" = "1" ]; then /usr/local/bin/03-install-emacs.sh; fi && \
    echo 'set debuginfod enabled off' > /root/.gdbinit

# The source tree, in one layer. .dockerignore drops everything the build does not
# need (VCS and agent state, tasks/, entrypoint/, the Makefile/Dockerfile, build
# trees, tarballs), so an edit to those does not re-run the cmake build below.
COPY . /gltron/


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

RUN echo "source ~/.extrabashrc" >> ~/.bashrc

