FROM registry.fedoraproject.org/fedora:43

ARG USE_GRAPHICS=1

RUN --mount=type=cache,target=/var/cache/libdnf5 \
    --mount=type=cache,target=/var/lib/dnf \
    sed -i -e "s@tsflags=nodocs@#tsflags=nodocs@g" /etc/dnf/dnf.conf && \
    echo "keepcache=True" >> /etc/dnf/dnf.conf && \
    dnf upgrade -y && \
    dnf install -y --skip-unavailable \
                   SDL_sound \
                   SDL_sound-devel \
                   autoconf  \
                   automake  \
                   clang  \
                   clang-tools-extra  \
                   emacs  \
                   ffmpeg \
                   g++  \
                   gcc  \
                   gdb  \
                   glew  \
                   glew-devel \
                   libogg libogg-devel \
                   libpng \
                   libpng-devel \
                   libtool  \
                   libvorbis-devel \
                   lldb  \
                   man  \
                   man-db  \
                   man-pages  \
                   mesa-libGLU-devel \
                   mikmod \
                   mikmod-devel \
                   nano  \
                   sdl12-compat  \
                   sdl12-compat-devel \
                   tmux  \
                   vorbis-tools \
                   zlib \
                   zlib-devel; \
    if [ "$USE_GRAPHICS" = "1" ]; then \
       dnf install -y \
                      mesa-dri-drivers  \
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
                      vulkan-tools  ; \
         fi ; \
    echo 'set debuginfod enabled off' > /root/.gdbinit


COPY .clang-format /gltron/

RUN echo "source ~/.extrabashrc" >> ~/.bashrc

ENTRYPOINT ["/entrypoint.sh"]
