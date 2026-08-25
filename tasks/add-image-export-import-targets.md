# Add image-export / image-import Makefile targets (Dockerfile/Makefile contract)

**Status:** proposed — needs go-ahead. Created 2026-08-16 (William Emerison Six <billsix@gmail.com>).
**Priority:** 5
**Difficulty:** 2

## Goal

gltron's container Makefile (**`Makefile.docker`**) is missing the standard **`image-export` /
`image-import`** target pair from my container-per-project Makefile contract. Add them, matching the
projects that already have them (geometricalgebra, spimulator, hanoi, texExpToPng, modelviewprojection).

## What to add (per the contract)

- **`image-export`** — `## export the OCI image to a timestamped tar in the repo root`:
  `$(CONTAINER_CMD) save $(CONTAINER_NAME) -o $(CONTAINER_NAME)-$(shell date +%m-%d-%Y_%H-%M-%S).tar`.
- **`image-import`** — `## import an OCI image tar: make image-import FILE=foo.tar`:
  `$(CONTAINER_CMD) load -i $(FILE)`.
- Both **`.PHONY`**, both `## `-documented (so they show in `make help`). Use `$(CONTAINER_CMD)` /
  `$(CONTAINER_NAME)` variables — **not** hardcoded `podman` / image name (match gltron's existing vars;
  gltron's docker Makefile uses `$(shell pwd):/gltron/`, so the container/image name to use is whatever
  it already tags in `image`).
- **Gitignore the artifacts** — add `*.tar` (or `gltron-*.tar`) to `.gitignore`; the tars are large and
  must never be committed. gltron currently does **not** gitignore them.

## Notes

- `save`/`load` start no container, so these need **no** `--cgroups=disabled` and run fine nested.
- This is a pure Makefile + `.gitignore` change; no Dockerfile impact.
- Precedent to copy verbatim: `geometricalgebra/Makefile` (or `spimulator/Makefile`) `image-export` /
  `image-import` targets. Reference: the "Makefile contract" in `~/.ai-coding-conventions.personal.md`.
- gltron's `image-export` roundtrip should then be smoke-tested (`make -f Makefile.docker image-export`,
  `rmi`, `image-import`) — optionally with the offline export test (per "Self-contained images + live
  source › Verifying it").
