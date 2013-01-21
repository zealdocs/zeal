# Zeal

**zeal**  
*noun*  

 1. a feeling of strong eagerness (usually in favor of a person or cause)
 2. excessive fervor to do something or accomplish some end
 3. prompt willingness

(from WordNet 3.0)

Zeal is a simple documentation browser inspired by [Dash](http://kapeli.com/dash/), but designed from scratch to avoid copyright issues.

[Screenshots (imgur)](http://imgur.com/a/crL3P)

## How to compile

Currently Zeal requires Qt 5.0. To compile it, run `qmake` and `make` in the `zeal` directory.

## How to use

After compiling it you need to download docsets and put them in `$HOME/.local/share/zeal/docsets/` -- after creating the directory first. Currently there are docsets available from Qt 5 and Python 2.7.3, generated using scripts from `gendoctests` directory, but for convenience can be downloaded from Dropbox:

 * [Qt5.tar.bz2](https://www.dropbox.com/s/qnpjfphph2z1yqw/Qt5.tar.bz2) (55M)
 * [python-2.7.3-docs-html.tar.bz2](https://www.dropbox.com/s/fjopk1jvpmjldgb/python-2.7.3-docs-html.tar.bz2) (4.3M)

Do `tar -jxvf file.tar.bz2` in docsets directory to enable given docset.

## TODO

 * Configuration (customisable hotkey instead of hardcoded Alt+Space, remember window size, etc.)
 * Search enhancements - some ideas:
   1. Allow selecting subset of docsets to search in.
   2. Substring match in case current startswith matching doesn't return anything.
   3. Grouping of similar results (like overloaded functions)
   4. Better docsets formatting (without headers, sidebars etc.)


## Contributions

Any feedback and pull requests are welcome. Before starting to implement anything larger, especially items from the TODO list above, it would be good to contact me at jerzy dot kozera at gmail, to avoid duplicating work.

Please keep in mind I'm not an experienced C++ programmer, so the code quality might be not great.

Also you can send bitcoins to 1Zea1QhPV5CGp8SNMpb2RHUvRrfNhKqGV to encourage development.
