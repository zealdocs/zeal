# Zeal [![Build Status](https://api.shippable.com/projects/54ac2ce4d46935d5fbc19b84/badge?branchName=master)](https://app.shippable.com/projects/54ac2ce4d46935d5fbc19b84/builds/latest)

**zeal**
*noun*

 1. a feeling of strong eagerness (usually in favor of a person or cause)
 2. excessive fervor to do something or accomplish some end
 3. prompt willingness

(from WordNet 3.0)

Zeal is a simple offline documentation browser inspired by [Dash](http://kapeli.com/dash/).

![Screenshot](http://i.imgur.com/SiLvpz8.png)

[More screenshots (imgur)](http://imgur.com/a/eVi97)

## Download

For details about binary packages (currently available for Windows and Ubuntu) see [downloads page](http://zealdocs.org/download.html). Also, the latest unstable builds are available [here]( https://bitbucket.org/jerzykozera/zeal-win32-binary-downloads/downloads).

## How to use

After installing Zeal, you need to download docsets. Go to *File->Options->Docsets*, select the ones you want, and click the *Download* button.

## How to compile

If you prefer to compile Zeal manually.

### Requirements
* Qt 5.2+
* `bsdtar` is required for the built-in docset extracting to work
* `libappindicator` and `libappindicator-devel` for notifications
* you may need to install `libqt5webkit5-dev` and `qtbase5-private-dev`

To compile it, run `qmake` and `make`. On Linux, a final `make install` is required to install icons.

## Query & Filter docsets

You can limit the search scope by using ':' to indicate the desired docsets.

`java:BaseDAO`

You can also search multiple docsets separating them with a comma:

`python,django:string`

## Command line

If you prefer, you can start with Zeal queries from command line, for this, use the option `--query`:

`zeal --query python:bomb`

## TODO

 * [Issues](https://github.com/zealdocs/zeal/issues)
 * Search enhancements - some ideas:
   * Allow selecting subset of docsets to search in (enable/disable docsets, docset groups)
   * Grouping of similar results (like overloaded functions)
 * Code cleanup
   * Refactoring to reuse common code between `ZealDocsetsRegistry` and `ZealListModel`

## Creating your own docsets

You can use [Dash's instructions for generating docsets](http://kapeli.com/docsets).
