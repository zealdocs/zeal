# Zeal

[![Changelog](https://img.shields.io/github/release/zealdocs/zeal.svg?style=flat-square)](https://github.com/zealdocs/zeal/releases)
[![IRC](https://img.shields.io/badge/chat-on%20irc-blue.svg?style=flat-square)](https://kiwiirc.com/client/irc.freenode.net/#zealdocs)
[![Telegram Chat](https://img.shields.io/badge/chat-on%20telegram-179cde.svg?style=flat-square)](https://telegram.me/zealdocschat)
[![Telegram Channel](https://img.shields.io/badge/follow-on%20telegram-179cde.svg?style=flat-square)](https://telegram.me/zealdocs)
[![Twitter](https://img.shields.io/badge/follow-on%20twitter-1da1f2.svg?style=flat-square)](https://twitter.com/zealdocs)

> **zeal** *noun*
>
> 1. a feeling of strong eagerness (usually in favor of a person or cause)
> 2. excessive fervor to do something or accomplish some end
> 3. prompt willingness
>
> (from WordNet 3.0)

Zeal is a simple offline documentation browser inspired by [Dash](https://kapeli.com/dash).

![Screenshot](https://i.imgur.com/v63u1ZJ.png)

[More screenshots](https://imgur.com/a/eVi97)

## Build Status

[![Coverity Scan](https://img.shields.io/coverity/scan/4271.svg?style=flat-square)](https://scan.coverity.com/projects/4271)

OS \ Branch | master | stable
------------|--------|-------
Linux | [![Shippable](https://img.shields.io/shippable/54ac2ce4d46935d5fbc19b84/master.svg?style=flat-square)](https://app.shippable.com/projects/54ac2ce4d46935d5fbc19b84) | [![Shippable](https://img.shields.io/shippable/54ac2ce4d46935d5fbc19b84/stable.svg?style=flat-square)](https://app.shippable.com/projects/54ac2ce4d46935d5fbc19b84)
Windows | [![AppVeyor](https://img.shields.io/appveyor/ci/trollixx/zeal/master.svg?style=flat-square)](https://ci.appveyor.com/project/trollixx/zeal) | [![AppVeyor](https://img.shields.io/appveyor/ci/trollixx/zeal/stable.svg?style=flat-square)](https://ci.appveyor.com/project/trollixx/zeal)

## Download

Get binary builds for Windows and Linux from the [download page](https://zealdocs.org/download.html).

## How to use

After installing Zeal, you need to download docsets. Go to *Tools->Docsets*, select the ones you want, and click the *Download* button.

## How to compile

### Requirements
* [Qt](https://www.qt.io/) version 5.2.0 or above. Required modules: Qt WebKit Widgets, Qt X11 Extras (X11 only).
* [libarchive](http://libarchive.org/).
* X11 only: `xcb-util-keysyms`.
* Ubuntu Unity only: [libappindicator](https://launchpad.net/libappindicator).

To compile Zeal run `qmake` and then `make`. Linux users can install Zeal with `make install` command.

## Query & Filter docsets

You can limit the search scope by using ':' to indicate the desired docsets:

`java:BaseDAO`

You can also search multiple docsets separating them with a comma:

`python,django:string`

## Command line

If you prefer, you can start Zeal with a query from command line:

`zeal python:bomb`

## Creating your own docsets

You can use [Dash's instructions for generating docsets](https://kapeli.com/docsets).

## Contact and Support

We want your feedback! Here's a list of different ways to contact developers and request help:
* Report bugs and submit feature requests to [GitHub issues](https://github.com/zealdocs/zeal/issues).
* Ask any questions in our [Google Group](https://groups.google.com/d/forum/zealdocs). You can simply send an email to zealdocs@googlegroups.com.
* For a quick chat with developers and other Zeal users use our IRC channel: #zealdocs on [Freenode](https://freenode.net/). Also available through Kiwi IRC [web interface](https://kiwiirc.com/client/irc.freenode.net/#zealdocs). Please, take into consideration possible time zone differences.
* Finally, for a private communications send us [email](mailto:zeal@zealdocs.org).
* And do not forget to follow [@zealdocs](https://twitter.com/zealdocs) on Twitter!

## License

This software is licensed under the terms of the GNU General Public License version 3 (GPLv3). Full text of the license is available in the [COPYING](https://github.com/zealdocs/zeal/blob/master/COPYING) file and [online](http://opensource.org/licenses/gpl-3.0.html).
