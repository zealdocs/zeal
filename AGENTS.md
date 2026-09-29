# Agent instructions

Guidance for AI coding agents working in this repository. Everything in [CONTRIBUTING.md](CONTRIBUTING.md) applies;
this file only covers what agents tend to get wrong.

## Build and test

```shell
just build   # compile using the dev preset
just test    # configure, build, and run the test suite
```

Run `just test` before proposing a change, and report failures plainly instead of describing the change as working.

## Comments

Comment sparingly, and only where the code cannot speak for itself: why a workaround exists, what upstream behavior
forced it, why an order or a limit matters. Do not restate what the next line already says, and do not narrate the
change you are making or what the code used to do; that belongs in the commit message or the pull request.

## Commits

Follow the [commit conventions](CONTRIBUTING.md#commits). Two rules agents break by default:

* Do not credit yourself in the commit message; see
  [AI-assisted contributions](CONTRIBUTING.md#ai-assisted-contributions). This holds even when the tool you run
  appends a trailer of its own accord.
* Do not write the pull request number into the subject; GitHub appends `(#NNNN)` when it squashes the merge.

## Scope

* Name the branch `<type>-<scope>-<description>`, reusing the type and scope of the commit, for example
  `fix-ui-tray-icon-dark-panels`. Leave the scope out when the commit has none.
* One branch, one commit, one pull request. See [pull requests](CONTRIBUTING.md#pull-requests).
* Change only what the task requires. Do not reformat, rename, or refactor code the task did not touch.
* New files need copyright and licensing information, either in the file or in `REUSE.toml`; see
  [licensing](CONTRIBUTING.md#licensing). CI rejects files that have neither.
