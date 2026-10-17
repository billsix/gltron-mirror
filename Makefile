.DEFAULT_GOAL := help

USE_GRAPHICS ?= 1
RUN_SANITIZERS ?= 1
USE_EMACS ?= 1

CONTAINER_CMD ?= $(shell command -v podman >/dev/null 2>&1 && echo podman || echo docker)
CONTAINER_NAME = gltron

# USE_EMACS=1 (the default) bind-mounts the vendored .emacs.d/ into the container so
# an interactive `make shell` can USE its clangd/eglot C config + packages (:U chowns
# to the container user, :z relabels for SELinux). USE_EMACS=0 skips the mount. To
# REFRESH the vendored elpa packages, use `make update-emacs-packages`.
ifeq ($(USE_EMACS),1)
  ELPA_MOUNT = -v $(CURDIR)/entrypoint/dotfiles/.emacs.d/:/root/.emacs.d/:U,z
else
  ELPA_MOUNT =
endif

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
	$(ELPA_MOUNT) \
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
	                 --build-arg USE_EMACS=$(USE_EMACS) \
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

# Refresh the vendored Emacs packages. Forces USE_EMACS=1 and rebuilds the image
# first (RUN_SANITIZERS=0 -- an emacs refresh should not drag in the sanitizer
# gate). Then, in the container, wipes ONLY the elpa/ subtree (init.el /
# preferences.el / helm.el / install-melpa-packages.el are hand-written config,
# left untouched) and reinstalls from MELPA into the host's bind-mounted
# .emacs.d/elpa. Finally strips compiled *.elc/*.eln (machine-specific) and
# force-stages the elpa tree. Needs network. Mirrors billsEmacsConfigs/c.
.PHONY: update-emacs-packages
update-emacs-packages: ## USE_EMACS=1: rebuild image, wipe+reinstall elpa, strip *.elc/*.eln, git add -f
	$(MAKE) image USE_EMACS=1 RUN_SANITIZERS=0
	$(CONTAINER_CMD) run --rm $(PODMAN_RUN_FLAGS) \
		-v $(CURDIR)/entrypoint/dotfiles/.emacs.d/:/root/.emacs.d/:U,z \
		--entrypoint /bin/bash \
		$(CONTAINER_NAME) \
		-c 'mkdir -p /root/.emacs.d/elpa; find /root/.emacs.d/elpa -mindepth 1 -delete; \
		    emacs --batch --load /root/.emacs.d/install-melpa-packages.el'
	mkdir -p $(CURDIR)/entrypoint/dotfiles/.emacs.d/elpa && cd $(CURDIR)/entrypoint/dotfiles/.emacs.d/elpa && \
		find . \( -iname '*.elc' -o -iname '*.eln' \) -delete && \
		git add -A -f .
	@echo "Done: reinstalled packages, stripped *.elc/*.eln, staged elpa -- review and commit."

.PHONY: help
help:
	@grep --extended-regexp '^[a-zA-Z0-9_-]+:.*?## .*$$' $(MAKEFILE_LIST) | sort | awk 'BEGIN {FS = ":.*?## "}; {printf "\033[36m%-30s\033[0m %s\n", $$1, $$2}'
