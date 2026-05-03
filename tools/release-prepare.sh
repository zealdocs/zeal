#!/usr/bin/env bash
#
# release-prepare.sh - Generate release notes and stage version bumps locally.
#
# SPDX-FileCopyrightText: Oleg Shparber, et al. <https://zealdocs.org>
# SPDX-License-Identifier: MIT
#
# Generates release notes via git-cliff and AppStream description bullets via
# Claude, inserts the new <release> entry into the appdata, and bumps the
# version markers in CMakeLists.txt. Review the result with `git diff` before
# running `just release-push <version>`.
#
# Usage: tools/release-prepare.sh <version>
#

set -euo pipefail
trap 'echo "ERROR: failed at line ${LINENO}: ${BASH_COMMAND}" >&2' ERR

if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <version>" >&2
    exit 2
fi

VERSION="$1"

if ! [[ "$VERSION" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
    echo "Invalid version: '${VERSION}'. Expected major.minor.patch (e.g. 0.9.0)." >&2
    exit 2
fi

# Canonical upstream repo for milestone lookups, regardless of which fork the
# maintainer is currently working on. Cliff itself is hardcoded to the same
# repo via [remote.github] in tools/cliff.toml.
CANONICAL_REPO="zealdocs/zeal"

# Lowercase to avoid clobbering the Windows env var of the same name (APPDATA
# is what gh uses to find its config). Local script vars stay lowercase.
appdata="assets/freedesktop/org.zealdocs.zeal.appdata.xml.in"
cmakelists="CMakeLists.txt"

mkdir -p build

if grep -qF "version=\"${VERSION}\"" "$appdata"; then
    echo "${appdata} already has a <release> for ${VERSION}; aborting." >&2
    echo "Run 'git restore .' to redo." >&2
    exit 1
fi

milestone=$(gh api "repos/${CANONICAL_REPO}/milestones?state=all" \
    --jq ".[] | select(.title == \"${VERSION}\") | .number" || true)

if [ -n "$milestone" ]; then
    echo "Linking upstream milestone #${milestone} (\"${VERSION}\")"
    # cliff's --context flag in 2.x is a debug print, not a way to set template
    # vars. The documented mechanism for custom template data is the `extra`
    # field on release/commit objects: dump cliff's context, jq-inject the
    # milestone into the release's `extra`, render via --from-context.
    git cliff --tag "v${VERSION}" --unreleased --config tools/cliff.toml --context \
        > build/cliff-context.json
    jq ".[0].extra.milestone = ${milestone}" build/cliff-context.json \
        > build/cliff-context.with-milestone.json
    git cliff --config tools/cliff.toml --from-context build/cliff-context.with-milestone.json \
        > build/release-notes.md
else
    echo "No upstream milestone titled \"${VERSION}\"; skipping milestone link."
    git cliff --tag "v${VERSION}" --unreleased --config tools/cliff.toml > build/release-notes.md
fi

claude -p "Read this git-cliff release changelog and summarize the \
user-facing changes as up to 10 <li>...</li> bullets for an AppStream \
<description>. AppStream is read by Linux software centers, so skip \
Windows-only and macOS-only changes. Each bullet should be concrete: the \
specific problem fixed or capability added, in terms a non-developer \
recognizes. Use past tense and start each bullet with one of: Added, \
Changed, Removed, Deprecated, Fixed. Order bullets by category in that \
sequence (Added first, Fixed last). No trailing period on bullets. Skip \
internal refactors, build/CI/dependency commits, and changes a user \
wouldn't notice. Avoid common AI cliches (em-dashes, 'delve into', \
'leverage', 'robust', 'ensure', 'seamless', 'comprehensive'). Output \
bullets only, no wrapping <ul>." \
    < build/release-notes.md \
    > build/appdata-bullets.txt

today=$(date -u +%Y-%m-%d)
indented=$(sed 's/^[[:space:]]*//; s/^/          /' build/appdata-bullets.txt)
cat > build/release-block.xml <<EOF
    <release date="${today}" version="${VERSION}" type="stable">
      <description>
        <ul>
${indented}
        </ul>
      </description>
      <url>https://github.com/${CANONICAL_REPO}/releases/tag/v${VERSION}</url>
    </release>
EOF
sed -i "/<releases>@ZEAL_APPSTREAM_DEV_RELEASE@/r build/release-block.xml" "$appdata"

sed -i "s/    VERSION [0-9.]\+ # x-version/    VERSION ${VERSION} # x-version/" "$cmakelists"
sed -i "s/set(ZEAL_RELEASE_VERSION \"[0-9.]\+\") # x-version/set(ZEAL_RELEASE_VERSION \"${VERSION}\") # x-version/" "$cmakelists"

echo "---"
echo "Applied:"
printf '  %-55s  %s\n' \
    "${appdata}"             "new <release> entry for v${VERSION}" \
    "${cmakelists}"          "VERSION and ZEAL_RELEASE_VERSION bumped to ${VERSION}" \
    "build/release-notes.md" "used by 'just release-push' as --notes-file"
echo
echo "Review with: git diff"
echo "Then run:    just release-push ${VERSION}"
echo
echo "To redo:     git restore . && just release-prepare ${VERSION}"
