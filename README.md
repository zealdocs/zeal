# Zeal

[![Changelog](https://img.shields.io/github/release/zealdocs/zeal.svg?style=flat-square)](https://github.com/zealdocs/zeal/releases)
[![Gitter](https://img.shields.io/gitter/room/zealdocs/zeal.svg?style=flat-square)](https://gitter.im/zealdocs/zeal)
[![IRC](https://img.shields.io/badge/chat-on%20irc-blue.svg?style=flat-square)](https://web.libera.chat/#zealdocs)
[![Telegram Channel](https://img.shields.io/badge/follow-on%20telegram-179cde.svg?style=flat-square)](https://telegram.me/zealdocsapp)
[![Twitter](https://img.shields.io/badge/follow-on%20twitter-1da1f2.svg?style=flat-square)](https://twitter.com/zealdocs)

[![Build Check](https://img.shields.io/github/actions/workflow/status/zealdocs/zeal/build-check.yaml?style=flat-square)](https://github.com/zealdocs/zeal/actions/workflows/build-check.yaml)
[![Coverity Scan](https://img.shields.io/coverity/scan/4271.svg?style=flat-square)](https://scan.coverity.com/projects/4271)

Zeal is a simple offline documentation browser inspired by [Dash](https://kapeli.com/dash).

![Screenshot](https://github.com/zealdocs/zeal/assets/714940/e8443bb4-ccb9-469b-89d6-b5b3bfc7e239)

## Download

Get binary builds for Windows and Linux from the [download page](https://zealdocs.org/download.html).

## How to use

After installing Zeal go to `Tools->Docsets`, select the ones you want, and click the `Download` button.

## How to compile

### Build dependencies

* [CMake](https://cmake.org/).
* [Qt](https://www.qt.io/) version 5.9.5 or above. Required module: Qt WebEngine Widgets.
* [libarchive](https://libarchive.org/).
* [SQLite](https://sqlite.org/).
* X11 platforms only: Qt X11 Extras and `xcb-util-keysyms`.

### Build instructions

```sh
cmake -B build
cmake --build build
```

More detailed instructions are available in the [wiki](https://github.com/zealdocs/zeal/wiki).

## Query & Filter docsets

You can limit the search scope by using ':' to indicate the desired docsets:

`java:BaseDAO`

You can also search multiple docsets separating them with a comma:

`python,django:string`

## Command line

If you prefer, you can start Zeal with a query from the command line:

`zeal python:pprint`

## Create your own docsets

Follow instructions in the [Dash docset generation guide](https://kapeli.com/docsets).

## Contact and Support

We want your feedback! Here's a list of different ways to contact developers and request help:

* Report bugs and submit feature requests to [GitHub issues](https://github.com/zealdocs/zeal/issues).
* Reach developers and other Zeal users in `#zealdocs` IRC channel on [Libera Chat](https://libera.chat) ([web client](https://web.libera.chat/#zealdocs)).
* Ask any questions in our [GitHub discussions](https://github.com/zealdocs/zeal/discussions).
* Do not forget to follow [@zealdocs](https://twitter.com/zealdocs) on Twitter!
* Finally, for private communication shoot an email to <support@zealdocs.org>.

## License

This software is licensed under the terms of the GNU General Public License version 3 (GPLv3) or later. Full text of the license is available in the [COPYING](COPYING) file and [online](https://www.gnu.org/licenses/gpl-3.0.html).
