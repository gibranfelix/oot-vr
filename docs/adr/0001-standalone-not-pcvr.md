# 0001: Standalone on the headset, not PCVR

Date: 2026-09-10
Status: accepted

## Context

`ShinyWindow/Shipwright-VR` (branch `MotionControls-2`) is Ocarina of Time in VR,
in first person. It works now as PCVR on D3D11. You compile it on a PC and play
with the Quest through a cable or Air Link.

A standalone port needs more work. At the start, we thought that it needed three
migrations:

- `vr_openxr` from D3D11 to GLES;
- the hooks in `gfx_pc.cpp` to `Fast::Interpreter`;
- approximately 118 call sites from Ship of Harkinian 9.0.0 to 9.2.3.

Each migration needs weeks. Nothing runs until all three are complete. The only
result of the three migrations is that the PC is not necessary. The game at the
end is the same game.

Thus, the question was: why not stop at PCVR?

## Decision

Standalone. PCVR is a test bench, not a destination.

## Reason

Ocarina of Time needs 25 to 30 hours. Players do not play a long game in two
long sessions. They play it in approximately forty short sessions. PCVR adds a
start cost to each session: start the PC, start Air Link, connect the cable.
The player pays that cost forty times.

This cost does not stop a player from playing. It stops a player from coming
back. For a game of two hours, the cost is not important. For a game of thirty
hours, the cost decides if the player finishes the game.

## Consequences

- The three migrations are in the scope of the project, and they are most of
  the work. Later, we found that the game uses the branch `vr-port`, not
  `vr-integration`. Thus, only one migration was necessary: D3D11 to GLES.
- `Shipwright-VR` is a reference and a test bench, not a base to improve.
- Performance is critical. The Quest 3S must render two passes without a PC.
- If the Quest 3S cannot render two passes, we change the route (resolution,
  foveation, cuts). We do not change the destination. A return to PCVR is a new
  project, not a continuation.
