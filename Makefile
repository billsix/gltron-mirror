.DEFAULT_GOAL := help

USE_GRAPHICS ?= 1
RUN_SANITIZERS ?= 1

CONTAINER_CMD ?= $(shell command -v podman >/dev/null 2>&1 && echo podman || echo docker)
CONTAINER_NAME = gltron

# Extra flags for every container `run`. Auto-set when running nested inside a
# runClaudeInContainer/runCrushInContainer sandbox (which exports NESTED_PODMAN=1,
# making --cgroups=disabled apply so podman-in-podman works); empty -- and
# byte-identical behavior -- on a normal host. On `run` lines only, never `build`
# (podman build rejects --cgroups). Convention: runClaudeInContainer
# tasks/reference/nested-podman-design.md.
PODMAN_RUN_FLAGS ?= $(if $(filter 1,$(NESTED_PODMAN)),--cgroups=disabled)

FILES_TO_MOUNT = -v $(shell pwd):/gltron/:Z \
                 -v ./entrypoint/shell.sh:/usr/local/bin/shell.sh:Z \
                 -v ./entrypoint/format.sh:/usr/local/bin/format.sh:Z \
                 -v ./entrypoint/dotfiles/.tmux.conf:/root/.tmux.conf:Z \
                 -v ./entrypoint/dotfiles/.extrabashrc:/root/.extrabashrc:Z

# gltron is a GUI game: pass the host display through so `make shell` can run it.
USE_X = -e DISPLAY=$(DISPLAY) \
	-v /tmp/.X11-unix:/tmp/.X11-unix \
	--security-opt label=type:container_runtime_t
WAYLAND_FLAGS_FOR_CONTAINER = -e "WAYLAND_DISPLAY=${WAYLAND_DISPLAY}" \
                              -e "XDG_RUNTIME_DIR=${XDG_RUNTIME_DIR}" \
                              -v "${XDG_RUNTIME_DIR}:${XDG_RUNTIME_DIR}"

# shell / shell-exec share ONE run invocation (defined once so the two can never
# drift). shell is interactive; shell-exec runs a script/command with no TTY.
SHELL_RUN_FLAGS = --entrypoint /bin/bash \
	$(FILES_TO_MOUNT) \
	$(USE_X) \
	$(WAYLAND_FLAGS_FOR_CONTAINER)
SHELL_EXEC_ARGS = -c 'cd /gltron && $(if $(CMD),$(CMD),exec bash $(SCRIPT))'


.PHONY: all
all: shell ## Build the image and get a shell in it

.PHONY: image
image: ## Build podman image (builds gltron via CMake at image-build time; RUN_SANITIZERS=1 runs the UBSan+ASan gate)
	$(CONTAINER_CMD) build \
	                 --build-arg USE_GRAPHICS=$(USE_GRAPHICS) \
	                 --build-arg RUN_SANITIZERS=$(RUN_SANITIZERS) \
	                 -t $(CONTAINER_NAME) \
	                 .

.PHONY: build
build: image ## Re-run the CMake build inside the container against the mounted tree (writes ./bld)
	$(CONTAINER_CMD) run --rm $(PODMAN_RUN_FLAGS) \
		$(FILES_TO_MOUNT) \
		$(CONTAINER_NAME) \
		bash -lc 'cmake -S /gltron -B /gltron/bld -G Ninja \
		            -DCMAKE_BUILD_TYPE=Debug \
		            -DCMAKE_INSTALL_PREFIX=/gltron/bld/install \
		         && cmake --build /gltron/bld -j \
		         && cmake --install /gltron/bld \
		         && (cd /gltron/bld && ctest --output-on-failure)'

.PHONY: shell
shell: ## Get a shell in an ephemeral container (X11 + Wayland passthrough for the game)
	$(CONTAINER_CMD) run -it --rm $(PODMAN_RUN_FLAGS) \
		$(SHELL_RUN_FLAGS) \
		$(CONTAINER_NAME) \
		/usr/local/bin/shell.sh

.PHONY: shell-exec
# shell-exec depends on image (NOT format) so a batch run does not reformat the
# source as a side effect -- a runner must not mutate the tree.
shell-exec: image ## Run a script/command in the container env (no TTY): make shell-exec SCRIPT=path | CMD='...'
	@[ -n "$(SCRIPT)$(CMD)" ] || { echo 'usage: make shell-exec SCRIPT=<mounted path> | CMD="..."'; exit 2; }
	$(CONTAINER_CMD) run --rm $(PODMAN_RUN_FLAGS) \
		$(SHELL_RUN_FLAGS) \
		$(CONTAINER_NAME) \
		/usr/local/bin/shell.sh $(SHELL_EXEC_ARGS)

.PHONY: appimage
appimage: image ## Build a Linux AppImage via cpack -G External (writes ./out/)
	mkdir -p ./out
	$(CONTAINER_CMD) run --rm $(PODMAN_RUN_FLAGS) \
		--entrypoint /bin/bash \
		$(FILES_TO_MOUNT) \
		-v ./out:/gltron/out:Z \
		$(CONTAINER_NAME) \
		-lc 'rm -rf /gltron/bld-appimage \
		         && cmake -S /gltron -B /gltron/bld-appimage -G Ninja \
		              -DCMAKE_BUILD_TYPE=Release \
		              -DGLTRON_DATA_DIR=usr/share/gltron \
		         && cmake --build /gltron/bld-appimage -j \
		         && (cd /gltron/bld-appimage && cpack -G External) \
		         && cp /gltron/bld-appimage/*.appimage /gltron/out/'

.PHONY: format
format: image ## Format all C/C++ with clang-format in the container (reproducible)
	$(CONTAINER_CMD) run --rm $(PODMAN_RUN_FLAGS) \
		--entrypoint /bin/bash \
		$(FILES_TO_MOUNT) \
		$(CONTAINER_NAME) \
		/usr/local/bin/format.sh

.PHONY: image-export
image-export: ## Export the OCI image to a timestamped tar in the repo root
	$(CONTAINER_CMD) save $(CONTAINER_NAME) -o $(CONTAINER_NAME)-$(shell date +%m-%d-%Y_%H-%M-%S).tar

.PHONY: image-import
image-import: ## Import an OCI image tar: make image-import FILE=foo.tar
	$(CONTAINER_CMD) load -i $(FILE)

.PHONY: help
help:
	@grep --extended-regexp '^[a-zA-Z0-9_-]+:.*?## .*$$' $(MAKEFILE_LIST) | sort | awk 'BEGIN {FS = ":.*?## "}; {printf "\033[36m%-30s\033[0m %s\n", $$1, $$2}'
