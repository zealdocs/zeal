# Zeal

**zeal**  
*noun*  

 1. a feeling of strong eagerness (usually in favor of a person or cause)
 2. excessive fervor to do something or accomplish some end
 3. prompt willingness

(from WordNet 3.0)

Zeal is a simple documentation browser inspired by [Dash](http://kapeli.com/dash/).

![Screenshot](http://i.imgur.com/SiLvpz8.png)

[More screenshots (imgur)](http://imgur.com/a/eVi97)

## Download

For details about binary packages (currently available for Windows and Ubuntu) see [downloads page](http://zealdocs.org/download.html).

## How to use

After installing/compiling it you need to download docsets. It can be done automatically by clicking 'Edit', 'Options', 'Docsets', 'Download...'.

## How to compile

If you prefer to compile Zeal manually.

### Requirements
* Qt 5.0
* X C Binding – keysyms (Ubuntu `libxcb-keysyms1-dev`)
* `bsdtar` is required for the built-in docset extracting to work

To compile it, run `qmake` and `make` in the `zeal` directory.

## TODO

 * [Issues](https://github.com/jkozera/zeal/issues)
 * Search enhancements - some ideas:
   * Allow selecting subset of docsets to search in (enable/disable docsets, docset groups)
   * Grouping of similar results (like overloaded funcdtions)
 * Code cleanup
   * Refactoring to reuse common code between `ZealDocsetsRegistry` and `ZealListModel`
   * ...

## Creating your own docsets

You can use [Dash's instructions for generating docsets](http://kapeli.com/docsets).

## Contributions / Project Status

Any feedback, feature requests, or pull requests are welcome. However the project is not under active development at the moment, so unless someone is interested in contributing code, no new features are expected.

I'm going to continue updating docsets that are part of Dash/Zeal exchange programme, and try to fix critical stability/compatibility issues if any arise, to keep the project somewhat alive, but will not implement new features in foreseeable future. Anyway, I should be able to review pull requests.

Please keep in mind I'm not an experienced C++ programmer, so the code quality might be not great.
