# Contributing to Zeal

Thanks for your interest in improving Zeal. This document covers the practical side: how to build the project, which
conventions to follow, and what to expect from review.

## Before you start

Bug reports and small fixes can go straight to [GitHub issues](https://github.com/zealdocs/zeal/issues) or a pull
request. For anything larger, such as a new feature or refactoring, please open an issue or a
[discussion](https://github.com/zealdocs/zeal/discussions) first so we can agree on the approach before you spend
time on the implementation.

Questions are welcome in [GitHub discussions](https://github.com/zealdocs/zeal/discussions) or on
[Discord](https://go.zealdocs.org/l/discord). Project spaces follow a short
[code of conduct](https://go.zealdocs.org/l/conduct).

## Development setup

Build dependencies are listed in the [README](README.md#build-dependencies), and platform-specific instructions are
in the [wiki](https://github.com/zealdocs/zeal/wiki).

The repository includes a [justfile](https://just.systems/) with recipes for common tasks:

```shell
just configure   # configure a build directory
just build       # compile
just run         # compile and launch Zeal
just test        # configure, build, and run the test suite
```

Recipes use the `dev` CMake preset by default; override it with `PRESET=release just build`. If you prefer plain
CMake, the presets work on their own:

```shell
cmake --preset dev
cmake --build --preset dev
```

## Code style

Formatting is defined by `.clang-format` and `.editorconfig`. Run clang-format on the code you change, and match the
style of the surrounding code for anything the tools do not cover.

## Commits

Commit messages follow [Conventional Commits](https://www.conventionalcommits.org/): a type, an optional scope, and
a short description in the imperative mood.

```text
fix(core): guard setColorScheme behind Qt 6.8
feat(ui): improve tab bar sizing and styling
build(cmake): append to CMAKE_MODULE_PATH
```

Common scopes are `app`, `core`, `ui`, `util`, `assets`, and `cmake`. When in doubt, `git log` has plenty of examples.

## Tests

Run the suite with `just test`. New logic in `src/libs` should come with tests where practical.

## Licensing

Zeal is licensed under GPL-3.0-or-later, and the repository follows the [REUSE](https://reuse.software/) specification:

* New source files need a copyright notice and an SPDX license identifier; copy them from an existing file:

  ```cpp
  // Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
  // SPDX-License-Identifier: GPL-3.0-or-later
  ```

* If you bring in third-party code, add its license text to `LICENSES/` and declare it in `REUSE.toml`.

By submitting a contribution, you agree to provide it under the project license.

## Pull requests

* Keep pull requests small and focused; unrelated changes belong in separate PRs.
* CI must pass. It runs builds for all supported platforms and CodeQL analysis.
* Zeal is maintained in spare time, so a review can take a while. If a PR sits without a response for a couple of
  weeks, a polite ping is fine.

## Security

If you think you have found a security issue, please report it privately to <support@zealdocs.org> instead of
opening a public issue.
