# AGENTS.md

Rules for contributors and for AI agents in this repository.

oot-vr is a standalone port of *The Legend of Zelda: Ocarina of Time* to the
Meta Quest 3S. The game runs in first person, natively on the headset. It uses
Ship of Harkinian and `libultraship`.

## Start here

1. [`STATUS.md`](STATUS.md): what works, what does not work, and the open
   decisions.
2. [`docs/architecture.md`](docs/architecture.md): where the VR layer connects
   to the game and to the engine.
3. [`CONTEXT.md`](CONTEXT.md): the terms of this project. Use these terms in
   issues, commits, and code.
4. [`docs/adr/`](docs/adr/): the decisions that are difficult to change. If
   your change goes against an ADR, say so in the pull request.
5. The roadmap is issue [#2](ISSUE:2). Its sub-issues are the tasks.

## Documentation language

Write all documentation in **ASD-STE100 Simplified Technical English**. This
rule applies to Markdown files, issues, pull requests, and release notes.

- Write short sentences: 20 words maximum in procedures, 25 words maximum in
  descriptions.
- Write one instruction in each sentence. Use the imperative for instructions.
- Use the active voice.
- Use one word for one meaning. Do not use synonyms for the same thing.
- Use the terms in `CONTEXT.md`.
- Write paragraphs of 6 sentences maximum.
- Do not use contractions or slang.

## Game assets

**This repository must never contain game assets.**

- The `.gitignore` blocks `*.z64`, `*.n64`, `*.v64`, `*.otr`, and `*.o2r`.
- No build can make an output that contains game assets and that Git can
  commit.
- The APK contains only `soh.o2r`. The player makes `oot.o2r` from a dump of a
  cartridge that the player owns.
- Do not write ROM file names, ROM sources, or ROM paths in issues, commits, or
  documentation.

## Code

All code is in `port/`: the game (`port/soh`), the engine
(`port/libultraship`), the extraction tools (`port/ZAPDTR`,
`port/OTRExporter`), and the Quest wrapper (`port/Android`). There are no
submodules. Read [`docs/adr/0002`](docs/adr/0002-one-repository-with-the-port-inside.md).

- **Build the APK** with `port/Android/build-apk.sh`. Read
  [`port/Android/README.md`](port/Android/README.md).
- **Build on a Linux file system.** The build needs symlinks and the execute
  bit. NTFS and FAT disks do not have them.
- **Mark VR changes in upstream files** with the comment `SOH [VR]`. Then
  `git grep 'SOH \[VR\]'` finds all of them.
- **New files in `port/`.** The `.gitignore` of Ship of Harkinian is a Visual
  Studio template. It ignores directories with the names `debug/` and `log/`,
  and it ignores `*.png` and `Makefile`. `libultraship` has source code in
  `src/fast/debug/` and `src/libultraship/log/`. If `git status` does not show
  a new file, add it with `git add -f`.
- **Do not change the line endings** in `port/libultraship`, `port/ZAPDTR`, or
  `port/OTRExporter`. `port/.gitattributes` marks them `-text`. Some files are
  patches that the build applies with `git apply`.

## Code Review Rules

Examine each pull request for these problems. Each one is a P1 problem.

- The pull request adds game assets, ROM data, or a ROM path.
- The pull request adds a file in a `debug/` or `log/` directory, but Git does
  not track the file.
- The pull request changes line endings in `port/libultraship`, `port/ZAPDTR`,
  or `port/OTRExporter`.
- The pull request changes an upstream file for VR, but the change does not
  have the `SOH [VR]` marker.
- The pull request adds a VR call without a `vr_is_initialized()` or
  `VR_IsInitialized()` check. The game must also run with VR off.
- The pull request adds documentation that does not use ASD-STE100.
