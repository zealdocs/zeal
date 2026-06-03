# Ubuntu PPA packaging

Debian/Ubuntu source packaging for Zeal and the automation that publishes it to
Launchpad. Two flows feed two PPAs:

| Flow    | PPA                    | Trigger                                   | Signed by         |
| ------- | ---------------------- | ----------------------------------------- | ----------------- |
| Release | `ppa:zealdocs`         | `publish-ppa.yaml` on each GitHub release | repo GPG secret   |
| Nightly | `ppa:zealdocs/nightly` | Launchpad recipe (`main`)                 | Launchpad buildds |

`ppa:zealdocs` is shorthand for the team's default PPA, `ppa:zealdocs/ppa`. Launchpad
builds the binaries from the uploaded source package, once per series and architecture.

## Files

- `debian/` is the Debian source package (Qt 6). `rules` is a thin `dh` wrapper, since
  CMake installs the binary, `.desktop`, metainfo, and icons under `/usr`.
- `zeal.recipe` is the Launchpad recipe for nightly builds.

## Target series

The series live in the `SERIES` variable at the top of `publish-ppa.yaml`. The floor is
24.04 (noble); 22.04's Qt 6.2 is too old (Zeal needs Qt >= 6.4.2). Launchpad rejects EOL
or unknown series, so a stale entry only fails its own upload. Nightly series are set in
the recipe on Launchpad.

## Release uploads (`publish-ppa.yaml`)

On a release (or manual run with a version), the workflow builds a GPG-signed source
package per series and `dput`s it to `ppa:zealdocs/ppa`, versioned
`1:<version>-0ubuntu1~ubuntu<series>.1`. The `1:` epoch is required: the official archive
package carries it (`1:0.7.x`), so without it a PPA build would sort below the archive
and never reach users. The `~ubuntu<series>` suffix orders correctly across series
upgrades.

### One-time setup (releases)

1. Create a passphrase-less ed25519 signing key with an **encryption subkey**. Launchpad
   confirms ownership via an encrypted email, so a sign-only key can't be registered:

   ```sh
   gpg --quick-generate-key "Zeal Release <release@zealdocs.org>" ed25519 sign never
   gpg --quick-add-key <keyid> cv25519 encr never
   ```

2. Publish the public key and register it on Launchpad. The account must have signed the
   Ubuntu CoC and have upload rights to `ppa:zealdocs`, and the key's UID email must be
   verified on that account:

   ```sh
   gpg --keyserver keyserver.ubuntu.com --send-keys <keyid>
   ```

   Add the fingerprint at <https://launchpad.net/~/+editpgpkeys> and confirm the email
   Launchpad sends. keyserver.ubuntu.com is append-only, so put only role addresses on
   the key, never a personal one.
3. Add the secret `RELEASE_GPG_PRIVATE_KEY` (armored key from
   `gpg --armor --export-secret-keys <keyid>`). Set `RELEASE_GPG_PASSPHRASE` only if the
   key has one (not recommended).

## Nightly builds (Launchpad recipe)

Launchpad rebuilds nightlies from `main` itself, when `main` changes. No GitHub cron or
signing secret, since the buildds sign internally.

### One-time setup (nightly)

1. Create `ppa:zealdocs/nightly`.
2. Import the GitHub repo into Launchpad (Code -> Import) so `lp:zeal` is available.
3. Create a recipe (<https://launchpad.net/~zealdocs/+recipes>) from `zeal.recipe`. It
   nests `pkg/ppa/debian` into `debian/` and versions via `1:{debupstream}-0~{revtime}`,
   keeping each nightly below the next release and above the archive.
4. Set it to build into `nightly`, pick the series, and tick **Build daily**.
5. Request an initial build to confirm it works.

## Local testing

Build a source package as the workflow does, then build the binary in a clean chroot:

```sh
VERSION=$(git describe --tags --abbrev=0 | sed 's/^v//')
ZEAL=~/zeal   # your checkout

# debuild looks for the .orig tarball in the parent directory.
git -C "$ZEAL" archive --prefix="zeal-${VERSION}/" "v${VERSION}" | gzip -9 \
    > "/tmp/zeal_${VERSION}.orig.tar.gz"
cd /tmp && tar xf "zeal_${VERSION}.orig.tar.gz" && cd "zeal-${VERSION}"
cp -r "$ZEAL/pkg/ppa/debian" debian

# Overwrite the seed changelog with a target-series entry, as the workflow does.
cat > debian/changelog <<EOF
zeal (1:${VERSION}-0ubuntu1~ubuntu24.04.1) noble; urgency=medium

  * Local test build.

 -- Zeal Release <release@zealdocs.org>  $(date -R)
EOF

debuild -S -us -uc                 # unsigned source package
sudo pbuilder build ../zeal_*.dsc  # binary against noble
```
