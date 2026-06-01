# Ubuntu PPA packaging

Debian/Ubuntu source packaging for Zeal, plus the automation that publishes it
to Launchpad. Two flows feed two PPAs:

| Flow    | PPA                            | Trigger                                   | Signed by         |
| ------- | ------------------------------ | ----------------------------------------- | ----------------- |
| Release | `ppa:zealdocs`          | `publish-ppa.yaml` on each GitHub release | repo GPG secret   |
| Nightly | `ppa:zealdocs/nightly`  | Launchpad daily-build recipe (`main`)     | Launchpad buildds |

The stable PPA is the team's default (named `ppa`), so `ppa:zealdocs` is shorthand
for `ppa:zealdocs/ppa`.

Launchpad builds the binaries from the uploaded *source* package, once per
target series, for every architecture the PPA enables.

## Layout

- `debian/` — the Debian source package (Qt 6). `rules` is a thin `dh` wrapper
  because CMake already installs the binary, `.desktop`, AppStream metainfo, and
  icons under `/usr` (see `assets/freedesktop/CMakeLists.txt`).
- `zeal.recipe` — the Launchpad git-build recipe used for nightly builds.

## Target series

Zeal requires **Qt ≥ 6.4.2** (`src/app/CMakeLists.txt`), so **24.04 (noble)** is
the oldest buildable series; 22.04 (Qt 6.2) cannot build it. Targets are noble
and every newer supported series.

For releases, the series list is the single `SERIES` variable at the top of
`.github/workflows/publish-ppa.yaml` — trim or extend it as releases come and go.
Launchpad rejects uploads for EOL or unknown series, so a stale entry only fails
its own upload. For nightlies, the series are chosen in the recipe's settings on
Launchpad.

## Release uploads (`publish-ppa.yaml`)

On a published release (or via **Run workflow** with a version), the workflow
builds a GPG-signed source package per series and `dput`s it to
`ppa:zealdocs/ppa`, versioned `1:<version>-0ubuntu1~ubuntu<series-version>.1`
(e.g. `1:0.8.2-0ubuntu1~ubuntu24.04.1`). The `1:` epoch is mandatory, not
cosmetic: the official archive package carries it too (`1:0.7.x`), so without
it a PPA build would sort *below* the archive and users would never receive it.
The numeric `~ubuntu<version>` suffix (Launchpad's recommended scheme) orders
correctly across series upgrades and lets the archive reclaim users once it
ships the same upstream.

### One-time setup (releases)

1. Create a GPG key for CI signing (a dedicated, ideally passphrase-less, key is
   simplest):

   ```sh
   gpg --quick-generate-key "Zeal Release <release@zealdocs.org>" default sign never
   ```

2. Upload its **public** key to a Launchpad account that has signed the Ubuntu
   Code of Conduct and has upload rights to `ppa:zealdocs/ppa`
   (`gpg --keyserver keyserver.ubuntu.com --send-keys <keyid>` — Launchpad imports
   from that keyserver — then register the fingerprint at
   <https://launchpad.net/~/+editpgpkeys>).
3. Add repository secrets:
   - `LAUNCHPAD_GPG_PRIVATE_KEY` — ASCII-armored private key
     (`gpg --armor --export-secret-keys <keyid>`).
   - `LAUNCHPAD_GPG_PASSPHRASE` — optional; only if the key has a passphrase.

## Nightly builds (Launchpad recipe)

Launchpad rebuilds nightlies itself from `main` — no GitHub cron and no signing
secret, since Launchpad's build farm signs internally. It only rebuilds when
`main` has new commits.

### One-time setup (nightly)

1. Create the PPA `ppa:zealdocs/nightly`.
2. Import the GitHub repo into Launchpad as a Git repository (Code → Import), so
   `lp:zeal` is available to recipes.
3. Create a new source package recipe
   (<https://launchpad.net/~zealdocs/+recipes>) and paste the contents of
   `zeal.recipe`. The recipe nests `pkg/ppa/debian` into `debian/` via
   `nest-part` and derives the version from `deb-version`
   (`1:{debupstream}+git{time}`).
4. Set the recipe to build into `nightly`, select the target series, and
   tick **Build daily**.
5. Request an initial build to confirm it produces binaries.

## Local testing

Build a source package the way the workflow does, then build the binary in a
clean chroot:

```sh
VERSION=0.8.2
ZEAL=~/zeal   # your checkout

# Pristine upstream tarball, placed in /tmp — debuild looks for the .orig tarball
# in the build dir's parent. Use the release tag, or HEAD/a branch to test
# unreleased changes.
git -C "$ZEAL" archive --prefix="zeal-${VERSION}/" "v${VERSION}" \
    | gzip -9 > "/tmp/zeal_${VERSION}.orig.tar.gz"

cd /tmp && tar xf "zeal_${VERSION}.orig.tar.gz" && cd "zeal-${VERSION}"
cp -r "$ZEAL/pkg/ppa/debian" debian

# Overwrite the seed changelog with a target-series entry, exactly as the workflow
# does. (debian/changelog already exists in pkg/ppa/debian, so `dch --create` fails.)
cat > debian/changelog <<EOF
zeal (1:${VERSION}-0ubuntu1~ubuntu24.04.1) noble; urgency=medium

  * Local test build.

 -- Zeal Release <release@zealdocs.org>  $(date -R)
EOF

debuild -S -us -uc                 # build an unsigned source package
sudo pbuilder build ../zeal_*.dsc  # build the binary against noble
```
