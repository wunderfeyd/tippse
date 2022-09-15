Tippse documentation
====================

Tippse is a small text and hex editor. It's designed to have a simple interface and default hot key set based on the average editor in the wild, to handle huge files while maintaining a good working speed and to allow file and project based configuration.

## Syntax

`tippse <--state|-s statefile> <files...>`

The files given at the command line are opened at startup. If `--state` or `-s` is mentioned then the editor will restore the documents and windows on load from the state file and save these things during exit to the same state file.

## Usage

* Basic use

  The editor is using a console device for display and renders the document to the whole console screen space. Navigate with the positioning/arrow keys, type your text. When ready hit CTRL+S to save all documents and exit with CTRL+Q. If other operations are needed lookup the commands with CTRL+P which opens a panel, with all possible commands and keyboard shortcuts.

* Open a file

  CTRL+B activates the integrated file browser. Enter a target directory or file and the browser will jump to the specified location or open the file. Otherwise use the arrows keys to select a entry and ENTER to open the document. To filter a large file set type a part of the filename to filter for, it's updated automatically.

* Navigating between open files

  With CTRL+D all open documents and their status are displayed. Like the browser you can filter and select you document. The most recent document is displayed first. As an alternative CTRL+E switches back to the previous opened document.

* Save file(s)

  CTRL+S is saving all modified files. If the editor can't save or has not enough information to save a file it will display a selection box.

* Quit

  When using CTRL+Q the editor tries to quit. If documents unsaved or something else went wrong it will display a selection box.

* Search

  Hit CTRL+F enter your phrase to search for and press ENTER. The editor will highlight the next found position or display an error message.

  To start or continue searching it's possible to use the F3 key for a forward search and SHIFT+F3 for a backwards search.

  To display the last file search use CTRL+SHIFT+F3. It's started with a second CTRL+SHIFT+F3.

* Replace

  Hit CTRL+F enter your phrase to search for, hit CTRL+H enter the replacement text. Hit F7 to replace all findings. Use F6 to replace one after the other, manually. With SHIFT+F6 the process is searching backwards.

* Cut/Copy/Paste

  The standard short cuts for cut (CTRL+X), copy (CTRL+C) and paste (CTRL+V) are configured by default.

  **Hint:** If you wish to use a shared clipboard in linux you can install xsel to copy between X11/desktop clipboard and Tippse. In MacOS pbcopy/pbpaste is used.

* Advanced use

  There're many more operations Tippse can offer. CTRL+P opens the command panel. You can inspect the shortcuts, filter or start commands from here.

* Search options

  Select `searchcase...` in the command panel to switch between case sensitive or insensitive search. With `searchmode...` the algorithm can toggled between [regular expressions](regex.md) and normal text search.

* Hex editor

  With CTRL+U the document editor alternates between a hex editor and a text editor.

## Configuration

The base configuration is loaded from `(home)/.tippse/.tippse`, and incrementally overwritten by all `.tippse` files found in the subpaths to the document file.

TODO: Explain config structure, example from `config.c`.

## Tippse mental framework

* No change policy

  Tippse tries hard to keep the original document structure intact. In fact it does only change document portions the user applied commands to.

* No convert policy

  Unlike some other editors the file is not converted between an internal and the external encoding during load.

* Document detail auto detection

  The tabstop style and width, the line endings and the encoding is detected from the file. In the wild there're always files that are edge cases. If you find some, contact us. We would like to improve the auto detection.

* Document on disk

  The document above a minimal size (at the moment 1 MiB) is not loaded into memory. The document stays on disk. This allows an instant load and dealing with huge files. Only document changes are kept in memory and applied during write back onto disk. This also means that the document is written twice to disk in case some read/write regions overlap. First a temporary file which is renamed back to the original for example.

* View mask

  When a document is being displayed the document is parsed according to its document details and binary data. The parsed data is used to build a data structure which hold anchors for document position and display parameters (such a line count). When a document position is seeked and not found the file is parsed until that point. Indicated by the "Locating" bar at the top.

## License

The code is licensed under [CC0 1.0 Universal](https://creativecommons.org/publicdomain/zero/1.0/legalcode). With some stripped unicode data files by Unicode, Inc <http://unicode.org/copyright.html>.

For details consult the [license file](../LICENSE.md).

## Contact

We would like to see bug reports, suggestions, questions and anything else project related at <https://github.com/wunderfeyd/tippse>.

## Thank you

We want to thank everyone. It does not matter if you have contributed to this project or never heard about it. In the end every scenario will improve this project or will teach us how to do it better.
