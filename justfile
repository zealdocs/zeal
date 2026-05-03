# Zeal dev and release recipes.
# Uses CMake presets defined in CMakePresets.json.
# Run `just` (no args) to see the recipe list.

set shell := ["bash", "-cu"]
set windows-shell := ["sh", "-cu"]

[private]
default:
    @just --list

# Configure a build directory using a preset.
[group('dev')]
configure preset="dev":
    cmake --preset {{preset}}

# Build using a preset.
[group('dev')]
build preset="dev":
    cmake --build --preset {{preset}}

# Configure, build, and run the test suite.
[group('dev')]
test:
    cmake --preset testing
    cmake --build --preset testing
    ctest --preset testing

# Apply clang-format across the tree (use -Fix to apply, -Staged for staged files only).
[group('dev')]
format *args:
    pwsh tools/run-clang-format.ps1 {{args}}

# Run clang-tidy (use -Fix to apply, -Staged for staged files only, -Check <name> for a single check).
[group('dev')]
lint *args: configure
    pwsh tools/run-clang-tidy.ps1 {{args}}

# Remove all build directories.
[group('dev')]
clean:
    rm -rf build

# Generate release notes for the given version, insert the corresponding
# <release> entry into the appdata, and bump CMakeLists.txt versions.
# Review the resulting changes with `git diff` before running `just release-push`.
# Maintainer use only.
[group('release')]
release-prepare version:
    bash tools/release-prepare.sh {{version}}

# Commit appdata + version bump, push, create the draft release with the
# generated notes, then tag and push the tag (which triggers build CI).
# The actual GitHub Publish click still happens manually in the UI.
# `remote` controls which remote (and matching GitHub repo) receives the push;
# default is `origin` (works for fork testing). For an upstream release, run
# `just release-push 0.8.2 upstream` (or whatever you've named the upstream remote).
# Maintainer use only.
[group('release')]
release-push version remote="origin":
    git add assets/freedesktop/org.zealdocs.zeal.appdata.xml.in CMakeLists.txt
    git commit -m "chore: release v{{version}}"
    git push {{remote}} HEAD
    gh release create v{{version}} --draft --notes-file build/release-notes.md \
        --repo "$(git remote get-url {{remote}} | sed -E 's|^.*github\.com[:/]||; s|\.git$||')"
    git tag v{{version}}
    git push {{remote}} v{{version}}
    @echo "Pushed v{{version}} with draft release. CI will upload artifacts."
    @echo "When CI is done, open the draft on GitHub and click Publish."
