# Fedora COPR packaging

RPM packaging for Zeal and the workflow that publishes it to
[Fedora COPR](https://copr.fedorainfracloud.org/). COPR builds binaries from a
source RPM, once per enabled chroot.

`publish-copr.yaml` runs two flows:

| Flow    | COPR project             | Trigger                                      |
| ------- | ------------------------ | -------------------------------------------- |
| Release | `@zealdocs/zeal`         | GitHub release, or manual run with a version |
| Nightly | `@zealdocs/zeal-nightly` | daily cron, or manual run with no version    |

Both build the source RPM in the workflow and submit it with `copr-cli`. The
nightly run skips when `main` has not advanced since its last build.

## Files

`zeal.spec` is the RPM spec. `%build` and `%install` are `%cmake` macros, since
CMake installs the binary, desktop file, metainfo, and icons under the prefix.
toml++ comes from `tomlplusplus-devel`; cpp-httplib stays bundled in `src/contrib`.

`Version:` is a placeholder. The workflow sets it with `--define "zeal_version <v>"`:
the tag for releases, a `git describe` snapshot for nightlies (which sorts above
the last release and below the next).

## Chroots

Chroots live on the COPR projects, not in the repo. Targets are the supported
Fedora releases plus rawhide, and the EPEL releases that ship `qt6-qtwebengine`
(RHEL, AlmaLinux, Rocky, CentOS Stream; EPEL 8 lacks it). "Follow Fedora branching"
adds new Fedora chroots automatically; add new EPEL releases by hand.

Support windows: [Fedora](https://endoflife.date/fedora),
[CentOS Stream](https://endoflife.date/centos-stream),
[RHEL](https://endoflife.date/rhel).

## Setup

1. Create `@zealdocs/zeal` and `@zealdocs/zeal-nightly`, enable the chroots, and turn
   on "Follow Fedora branching". Enable "Generate AppStream metadata" on the
   release project.
2. Add the repository secret `COPR_API_TOKEN`: the `[copr-cli]` block from
   <https://copr.fedorainfracloud.org/api/>. Both flows use it; refresh it when it
   expires.

## Local testing

```sh
VERSION=$(git describe --tags --abbrev=0 | sed 's/^v//')
mkdir -p ~/rpmbuild/SOURCES ~/rpmbuild/SPECS
git archive --prefix="zeal-$VERSION/" "v$VERSION" | gzip -9 \
    > ~/rpmbuild/SOURCES/zeal-$VERSION.tar.gz
cp pkg/copr/zeal.spec ~/rpmbuild/SPECS/
rpmbuild -bs ~/rpmbuild/SPECS/zeal.spec \
    --define "_topdir $HOME/rpmbuild" --define "zeal_version $VERSION"
mock ~/rpmbuild/SRPMS/zeal-$VERSION-1*.src.rpm
```
