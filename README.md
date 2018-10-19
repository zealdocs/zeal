# Zeal

[![Changelog](https://img.shields.io/github/release/zealdocs/zeal.svg?style=flat-square)](https://github.com/zealdocs/zeal/releases)
[![Gitter](https://img.shields.io/gitter/room/zealdocs/zeal.svg?style=flat-square)](https://gitter.im/zealdocs/zeal)
[![IRC](https://img.shields.io/badge/chat-on%20irc-blue.svg?style=flat-square)](https://kiwiirc.com/client/irc.freenode.net/#zealdocs)
[![Telegram Channel](https://img.shields.io/badge/follow-on%20telegram-179cde.svg?style=flat-square)](https://telegram.me/zealdocs)
[![Twitter](https://img.shields.io/badge/follow-on%20twitter-1da1f2.svg?style=flat-square)](https://twitter.com/zealdocs)

[![AppVeyor](https://img.shields.io/appveyor/ci/zealdocs/zeal/master.svg?style=flat-square)](https://ci.appveyor.com/project/zealdocs/zeal)
[![Coverity Scan](https://img.shields.io/coverity/scan/4271.svg?style=flat-square)](https://scan.coverity.com/projects/4271)

Zeal is a simple offline documentation browser inspired by [Dash](https://kapeli.com/dash).

![Screenshot](https://i.imgur.com/qBkZduS.png)

## Download

Get binary builds for Windows and Linux from the [download page](https://zealdocs.org/download.html).

## How to use

After installing Zeal, go to *Tools->Docsets*, select the ones you want, and click the *Download* button.

## How to compile

### Required dependencies

* [CMake](https://cmake.org/).
* [Qt](https://www.qt.io/) version 5.9.5 or above. Required module: Qt WebKit Widgets.
* [libarchive](http://libarchive.org/).
* [SQLite](https://sqlite.org/).
* X11 platforms only: Qt X11 Extras and `xcb-util-keysyms`.

### Building instructions

```sh
mkdir build && cd build
cmake ..
make
```

More detailed instructions are available in the [Wiki](https://github.com/zealdocs/zeal/wiki).

## Query & Filter docsets

You can limit the search scope by using ':' to indicate the desired docsets:

`java:BaseDAO`

You can also search multiple docsets separating them with a comma:

`python,django:string`

## Command line

If you prefer, you can start Zeal with a query from the command line:

`zeal python:pprint`

## Creating your own docsets

Follow the [Dash Docset Generation Guide](https://kapeli.com/docsets).

## Contact and Support

We want your feedback! Here's a list of different ways to contact developers and request help:
* Report bugs and submit feature requests to [GitHub issues](https://github.com/zealdocs/zeal/issues).
* Reach developers and other Zeal users on [Gitter](https://gitter.im/zealdocs/zeal), or IRC channel #zealdocs on [Freenode](https://freenode.net/).
* Ask any questions in our [Google Group](https://groups.google.com/d/forum/zealdocs). You can simply send an email to zealdocs@googlegroups.com.
* Finally, for private communications email us at zeal@zealdocs.org.
* And do not forget to follow [@zealdocs](https://twitter.com/zealdocs) on Twitter!

## License

This software is licensed under the terms of the GNU General Public License version 3 (GPLv3). Full text of the license is available in the [COPYING](https://github.com/zealdocs/zeal/blob/master/COPYING) file and [online](http://opensource.org/licenses/gpl-3.0.html).
