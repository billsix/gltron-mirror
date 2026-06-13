# Investigate Flatpak: building, publishing, and save-file/config handling

**Status:** proposed — not started
**Created:** 2026-06-13

## Goal

Use gltron as a vehicle to **learn Flatpak end-to-end** — how to build it, how to
publish/deploy it, and how an app's **save files and config** behave inside the
Flatpak sandbox. gltron is a good candidate because it already has a partial
manifest (`packaging/flatpak/`), needs GL + audio (so the permission model is
exercised), and writes a config file + screenshots (so the sandbox-storage
question is real). The point is as much to *understand the workflow* as to ship.

## Plan

- [ ] **Inventory what already exists.** Read `packaging/flatpak/` — the manifest
      (`io.github.billsix.GLtron.yaml`, which the `Dockerfile` already mirrors its
      module/dep list against), `AppRun`, the `.desktop` file, and any
      appstream/metainfo XML. Establish whether it builds today and what's missing.
      Cross-reference the shipped AppImage path (`docs/plans/appimage-target.md`).
- [ ] **Learn the build workflow.** `flatpak-builder` basics: choosing a runtime +
      SDK (`org.freedesktop.Platform`/`Sdk`, which version), building locally
      (`flatpak-builder --force-clean --install build-dir manifest.yaml`), running
      the result, iterating. Note how the manifest's modules map to the CMake build
      (install prefix must be `/app`; `DATA_DIR` then bakes to `/app/share/gltron`).
- [ ] **Sandbox permissions (`finish-args`).** Work out the minimum gltron needs:
      `--device=dri` (GL), `--socket=wayland` + `--socket=fallback-x11`,
      `--socket=pulseaudio` (SDL3_sound), and `--share=network` *only* if
      `GLTRON_NETWORK=ON`. Verify GL/audio actually work under the sandbox.
- [ ] **Save files & config under the sandbox — the main learning target.** Today
      gltron writes `~/.gltronrc` (`RC_NAME`), screenshots (`GLTRON_SNAPSHOT_DIR`),
      and a prefs dir (`GLTRON_PREFERENCES_DIR`), defaulting to `$HOME`. Under
      Flatpak, `$HOME` is redirected to `~/.var/app/io.github.billsix.GLtron/`.
      Determine: do these land somewhere sane automatically, or do they leak/get
      lost? Decide whether to **migrate to XDG base dirs**
      (`$XDG_CONFIG_HOME`, `$XDG_DATA_HOME`) — cleaner under Flatpak and good
      hygiene generally — or rely on the sandbox `$HOME` redirection and leave the
      code alone. This is the one part that may touch game code (the config-path
      logic), so weigh it against the teaching-artifact constraint.
- [ ] **Publishing / deploying — learn both paths.**
      - *Flathub:* the submission flow (PR to the `flathub/flathub` repo), manifest
        requirements, reverse-DNS app-id (`io.github.billsix.GLtron`), required
        AppStream metainfo + screenshots, license fields, and validation with
        `appstreamcli validate` / `flatpak-builder --run … appstream-util`.
      - *Self-hosted:* `flatpak build-export` → a repo, `flatpak build-bundle` →
        a `.flatpak` bundle, and a `.flatpakref` for one-line install. Document
        what "deploying" means for each.
- [ ] **Write it up.** Produce a runbook/plan in `docs/plans/flatpak.md` (per
      gltron's convention) — build steps, the permission set, the config/save-file
      decision, and the chosen publish path — and add it to the
      `## Tasks / plans (in-flight)` index in `CLAUDE.md`.

## Notes / decisions

- A Flatpak manifest already exists under `packaging/flatpak/`; the `Dockerfile`
  deliberately mirrors its dependency module list so the container and the Flatpak
  ship the same versions. gltron also already ships an AppImage. So the *packaging
  scaffolding* exists — the gaps are publishing know-how and correct
  config/save-file behavior in the sandbox.
- This `tasks/` item tracks the investigation; the durable runbook belongs in
  `docs/plans/`.
- Mostly packaging, not game logic — but the config-path question can reach into
  the code, so keep changes there surgical and pedagogy-respecting.

## Open questions

- Deploy target: **Flathub** (public, more process) or a **self-hosted repo /
  bundle** (full control, simpler) — or learn both, ship neither yet?
- Migrate config to **XDG base dirs**, or rely on Flatpak's `$HOME` redirection and
  leave `~/.gltronrc` as-is?
- Confirm the app-id `io.github.billsix.GLtron` is the one to standardize on
  (it must match across the manifest, `.desktop`, and metainfo).
