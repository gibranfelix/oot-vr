# Issue tracker: GitHub Issues

Issues live in GitHub Issues on `gibranfelix/zelda-oot-vr` (private). Use the `gh` CLI, which is
authenticated as `gibranfelix`.

Moved here from `.scratch/oot-quest-3s/` on 2026-09-23. Commit messages and docs from before that
date say "ticket NN"; the map issue has a table translating those to issue numbers.

## Conventions

- One issue per ticket. Title is the ticket's name; refer to it by that name, with the number riding
  inside a link.
- Comments and conversation history go in issue comments.
- Triage state is expressed with labels (see `triage-labels.md`).

## When a skill says "publish to the issue tracker"

`gh issue create -R gibranfelix/zelda-oot-vr --title ... --body-file ...`

## When a skill says "fetch the relevant ticket"

`gh issue view <number> -R gibranfelix/zelda-oot-vr --comments`

## Wayfinding operations

Used by `/wayfinder`. The map is **issue #2**:
https://github.com/gibranfelix/zelda-oot-vr/issues/2

- **Map**: the issue labelled `wayfinder:map`. Its body holds Destination / Notes /
  Decisions so far / Not yet specified / Out of scope.
- **Child ticket**: a GitHub **sub-issue** of the map. Its body is the question.
  - Add: `gh api -X POST repos/gibranfelix/zelda-oot-vr/issues/2/sub_issues -F sub_issue_id=<id>`
    (`<id>` is the issue's database id, `gh api repos/.../issues/<n> --jq .id`, not its number).
- **Type**: one label of `wayfinder:research`, `wayfinder:prototype`, `wayfinder:grilling`,
  `wayfinder:task`.
- **Blocking**: GitHub's native issue dependencies, so the frontier shows in the UI.
  - Add: `gh api -X POST repos/gibranfelix/zelda-oot-vr/issues/<n>/dependencies/blocked_by -F issue_id=<id>`
  - Read: `gh api repos/gibranfelix/zelda-oot-vr/issues/<n>/dependencies/blocked_by`
- **Frontier**: open sub-issues of #2 with no open blocker and no assignee; lowest number first.
- **Claim**: assign it to `gibranfelix` before any work: `gh issue edit <n> --add-assignee gibranfelix`.
- **Resolve**: post the answer as a comment starting `## Answer`, close with
  `gh issue close <n> --reason completed`, then add a one-line context pointer (gist + link) to the
  map's *Decisions so far* by editing issue #2.
- **Out of scope**: close with `--reason "not planned"` and add a line to the map's *Out of scope*.
