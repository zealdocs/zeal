# Agent instructions

Guidance for AI coding agents working in this repository. Everything in [CONTRIBUTING.md](CONTRIBUTING.md) applies;
this file only covers what agents tend to get wrong.

## Build and test

```shell
just configure             # configure the dev preset; needed once per checkout
just build                 # compile
just test                  # configure, build, and run the test suite
just format -Staged -Fix   # run clang-format on the staged C++ files
```

Run `just test` before proposing a change, and report failures plainly instead of describing the change as working.
Run `just format -Staged -Fix` before committing; it rewrites the files in place, so stage them again afterwards.

On Windows, run the recipes inside an MSVC environment, for example `vsenv exec -- just build`; see
[Build Instructions for Windows](https://github.com/zealdocs/zeal/wiki/Build-Instructions-for-Windows).

## Comments

Comment sparingly, and only where the code cannot speak for itself: why a workaround exists, what upstream behavior
forced it, why an order or a limit matters. Do not restate what the next line already says, and do not narrate the
change you are making or what the code used to do; that belongs in the commit message or the pull request.

## Commits

Follow the [commit conventions](CONTRIBUTING.md#commits). Three rules agents break by default:

* Do not credit yourself in the commit message; see
  [AI-assisted contributions](CONTRIBUTING.md#ai-assisted-contributions). This holds even when the tool you run
  appends a trailer of its own accord.
* Do not write the pull request number into the subject; GitHub appends `(#NNNN)` when it squashes the merge.
* Keep the body short: a sentence or two on why, and only when the diff does not make it obvious. No summary
  of what changed, no file lists, no test reports.

## Scope

* Name the branch `<type>-<scope>-<description>`, reusing the type and scope of the commit, for example
  `fix-ui-tray-icon-dark-panels`. Leave the scope out when the commit has none.
* One branch, one commit, one pull request. See [pull requests](CONTRIBUTING.md#pull-requests).
* Change only what the task requires. Do not reformat, rename, or refactor code the task did not touch.
* New source files carry the copyright and SPDX header shown under [licensing](CONTRIBUTING.md#licensing). Files
  that cannot hold a comment, such as images or JSON, go in `REUSE.toml` unless a pattern there already covers them.
  CI rejects files that have neither.
