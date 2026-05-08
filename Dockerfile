FROM registry.fedoraproject.org/fedora:44

ARG USE_GRAPHICS=1

RUN --mount=type=cache,target=/var/cache/libdnf5 \
    --mount=type=cache,target=/var/lib/dnf \
    sed -i -e "s@tsflags=nodocs@#tsflags=nodocs@g" /etc/dnf/dnf.conf && \
    echo "keepcache=True" >> /etc/dnf/dnf.conf && \
    dnf upgrade -y && \
    dnf install -y --skip-unavailable \
                   SDL2 \
                   SDL2-devel \
                   SDL2_sound \
                   SDL2_sound-devel \
                   clang  \
                   clang-tools-extra  \
                   cmake \
                   desktop-file-utils \
                   emacs  \
                   ffmpeg \
                   file \
                   fuse-libs \
                   g++  \
                   gcc  \
                   gdb  \
                   glew  \
                   glew-devel \
                   libogg libogg-devel \
                   libpng \
                   libpng-devel \
                   libvorbis-devel \
                   lldb  \
                   man  \
                   man-db  \
                   man-pages  \
                   mesa-libGLU-devel \
                   mikmod \
                   mikmod-devel \
                   nano  \
                   ninja-build \
                   patchelf \
                   squashfs-tools \
                   tmux  \
                   vorbis-tools \
                   wget \
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
COPY lib3ds /gltron/lib3ds/
COPY lua5 /gltron/lua5/
COPY music /gltron/music/
COPY nebu /gltron/nebu/
COPY nebutest /gltron/nebutest/
COPY README /gltron/README
COPY resource /gltron/resource/
COPY scripts /gltron/scripts/
COPY sounds /gltron/sounds/
COPY src /gltron/src/
COPY TODO /gltron/TODO
COPY tools /gltron/tools/


RUN cmake -S /gltron -B /bld -G Ninja \
          -DCMAKE_BUILD_TYPE=Debug \
          -DCMAKE_INSTALL_PREFIX=/bldInstall \
    && cmake --build /bld -j \
    && cmake --install /bld \
    && (cd /bld && ctest --output-on-failure)

COPY .clang-format /gltron/

RUN echo "source ~/.extrabashrc" >> ~/.bashrc

ENTRYPOINT ["/entrypoint.sh"]
