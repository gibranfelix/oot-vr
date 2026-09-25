# 0002: One repository, with the port inside

Date: 2026-09-23
Status: accepted

## Context

The port started as forks of the work of ShinyWindow:

- the game in `Shipwright-VR` (branch `MotionControls-2`);
- the engine in `libultraship-vr` (branch `vr-port`);
- `ZAPDTR` and `OTRExporter` as submodules from Harbour Masters.

That was four repositories with our commits, and one more repository for the
roadmap. The forks were only on a local disk. The submodules pointed to local
paths. A clean clone did not compile on a different computer.

Forks and submodules are useful when you want to stay in sync with upstream.
They let you merge the upstream changes. But we decided not to do this. Upstream
`libultraship` changed to SDL3, and it added a component system of 232 files.
The game and the Android wrapper use SDL2. Thus, an upstream update is a
migration, not a merge.

## Decision

**The code of the game is in this repository, in `port/`.** It is our code. There
are no separate forks and no submodules.

We imported the code with `git archive`. This command exports only the files in
version control. Thus, the ROM, the `.o2r` files, and the build outputs did not
come in. The three directories that were submodules are the same as their
source, byte for byte. `port/.gitattributes` marks them `-text`, so that Git
does not change their line endings.

## What we lose

- **The commit history of the forks.** The code came in as a snapshot. The
  issues [#8](https://github.com/oot-vr/oot-vr/issues/8), [#9](https://github.com/oot-vr/oot-vr/issues/9), [#14](https://github.com/oot-vr/oot-vr/issues/14), and [#15](https://github.com/oot-vr/oot-vr/issues/15)
  keep the reasons for each change.
- **An upstream fix is not a `git merge`.** It is a manual cherry-pick or a
  patch. The VR changes in `interpreter.cpp` are in 8 functions. While those 8
  functions do not move, a single fix is easy to copy.
- **The attribution is not automatic.** GitHub shows the source of a fork. Here,
  only the `README` shows the sources. You must keep the credits in the
  `README` correct.

## Consequences

- A clone compiles with `port/Android/build-apk.sh`. It needs only the Android
  SDK and NDK. CMake downloads the OpenXR loader from Maven and checks its hash.
- The `.gitignore` of Ship of Harkinian ignores directories with the names
  `debug/` and `log/`. Without a fix, Git ignores 23 source files of
  `libultraship`. We added these files with force. When you add a new file in
  such a directory, use `git add -f`. Read `AGENTS.md`.
