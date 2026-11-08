# Emacs packages are not installed or compiled in the Dockerfile

Done 2026-11-08 (William Emerison Six <billsix@gmail.com> asked for it). Part of github.com/billsix/runClaudeInContainer
`tasks/emacs-contract-conformance.md` (the build-time MELPA install and the run-time compile items).

## What was wrong

Under `USE_EMACS=1` the `Dockerfile` ran `dnf install -y emacs` and
`emacs --batch --load install-melpa-packages.el`: a network download plus byte-compile of the whole MELPA
set at image build, re-run whenever an earlier layer changed. `make shell` then mounted the vendored
`entrypoint/dotfiles/.emacs.d/` over `/root/.emacs.d`, so the build-time packages were never used.
`emacs` was also in `01-install-base.sh`, so `USE_EMACS=0` still installed it.

## What changed

- `entrypoint/03-install-emacs.sh` installs the Emacs binary (`emacs emacs-gtk+x11 emacs-pgtk`); the
  `Dockerfile` dispatches it under `USE_EMACS` in the install `RUN`, and the MELPA `COPY` + `RUN` are
  gone. `emacs` left `01-install-base.sh`. `.dockerignore` re-includes the new script instead of
  `install-melpa-packages.el`.
- `init.el` runs `(byte-recompile-directory package-user-dir 0)` after `(package-initialize)`, so the
  first Emacs start in `make shell` compiles the vendored packages into the mounted host `elpa/`
  (gitignored).
- Makefile comments (`ELPA_MOUNT`, `update-emacs-packages`) and `CLAUDE.md` describe this.
- **Kept the whole-`.emacs.d` mount** instead of Craft's elpa-only mount: gltron copies its source with
  one `COPY . /gltron/` right before the cmake build, so baking the Emacs config would re-run the game
  build on every config edit. With the mount, config edits are live and no image change is needed.

## Verification (2026-11-08, nested podman in the runClaudeInContainer sandbox)

- `make image` (default flags: graphics, sanitizer gate, `USE_EMACS=1`) succeeded with no MELPA fetch;
  ctest and the sanitizer gate passed. The image has `/usr/bin/emacs` and no `/root/.emacs.d`.
- Emacs in batch with the Makefile's `.emacs.d` mount, `--network=none`
  (runClaudeInContainer `tasks/adhoc/emacs-no-build-time-compile/emacs-start-check.sh`): exit 0,
  317 `.elc` compiled into the host `elpa/`; `git status` unchanged (the `.elc` are ignored).
- `tools/check_helm_file_at_point.sh` (runClaudeInContainer): 7/7.
- Not run: `make image USE_EMACS=0` (a full cmake + sanitizer rebuild); the `if` dispatch is the same
  shape Craft verified.
- Found, not fixed (contract-level, also true of every project using the run-time compile): the
  compile logs 7 `Cannot open load file … compat-macs.el` errors for compat's `compat-26.el`…`compat-30.el`
  backport files on every start. Emacs 31 never loads those files and a repeat start takes under a
  second, but no `.elc` is written for them, so each start retries them.
