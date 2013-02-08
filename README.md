# Zeal

**zeal**  
*noun*  

 1. a feeling of strong eagerness (usually in favor of a person or cause)
 2. excessive fervor to do something or accomplish some end
 3. prompt willingness

(from WordNet 3.0)

Zeal is a simple documentation browser inspired by [Dash](http://kapeli.com/dash/).

![Screenshot](http://i.imgur.com/SiLvpz8.png)

[More screenshots (imgur)](http://imgur.com/a/EFmzc)


## How to use

After compiling/unzipping it you need to download docsets and put them in `$HOME/.local/share/zeal/docsets/` (Linux) or `C:\Users\[your username]\AppData\Local\zeal\docsets\` (Windows) -- after creating the directory first. Currently there are docsets available from Qt 5 and Python 2.7.3, generated using scripts from `gendoctests` directory, but for convenience can be downloaded from Dropbox:

 * [Qt5.tar.bz2](https://www.dropbox.com/s/xlisxarbg09220a/Qt5.tar.bz2) (55M)
 * [python-2.7.3-docs-html.tar.bz2](https://www.dropbox.com/s/fcng55tc48hnwe3/python-2.7.3-docs-html.tar.bz2) (4.3M)

Do `tar -jxvf file.tar.bz2` in docsets directory to enable given docset.

You can also use Dash's docsets by putting `.docset` directories in the same directory as above.

## How to compile

Currently Zeal requires Qt 5.0. To compile it, run `qmake` and `make` in the `zeal` directory.

## Windows binary

A 64-bit Windows binary with all dependencies is available to download from Dropbox - [zeal.zip](https://www.dropbox.com/s/odiu8vqvnec2jre/zeal.zip) (24M).

## TODO

 * Support for global hotkeys under platforms other than Linux/X11 and Windows (OSX)
 * Search enhancements - some ideas:
   1. Allow selecting subset of docsets to search in.
   2. Grouping of similar results (like overloaded functions)
   3. Better docsets formatting (without headers, sidebars etc.)
 * More docsets
 * Refactoring to reuse common code between `ZealDocsetsRegistry` and `ZealListModel`


## Contributions

Any feedback, feature requests, or pull requests are welcome. Before starting to implement anything larger, especially items from the TODO list above, it would be good to contact me at jerzy dot kozera at gmail, to avoid duplicating work.

Please keep in mind I'm not an experienced C++ programmer, so the code quality might be not great.

Also you can send bitcoins to 1Zea1QhPV5CGp8SNMpb2RHUvRrfNhKqGV to encourage development.


[![Bitdeli Badge](https://d2weczhvl823v0.cloudfront.net/jkozera/zeal/trend.png)](https://bitdeli.com/free "Bitdeli Badge")

