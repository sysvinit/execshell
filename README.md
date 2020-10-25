# execshell

This is a proof-of-concept interactive REPL
for [Laurent Bercot](https://skarnet.org)'s
[execline](https://skarnet.org/software/execline) language.

## Building

`execshell` depends on the `skalibs` and `execline` libraries for the
`execlineb` interpreter's lexing functionality, and the `linenoise` library for
line editing functionality.

A fork of `linenoise` which includes UTF-8 line editing support is included
in this repository as a submodule, so the following commands are required when
cloning:

    $ git clone https://github.com/sysvinit/execshell
    $ cd execshell
    $ git submodule update --init

A Makefile is provided for compiling the `execshell` sources, which should be
sufficient for building the project. Please note that the Makefile assumes that
you have the `libskarnet` and `libexecline` libraries installed within your system
linker's default search path.

## Running

`execshell` provides an interactive interface with
readline/Emacs-alike line editing to the command lexer used by the
[`execlineb`](https://skarnet.org/software/execline/execlineb.html) script
launcher. For example:

    $ ./execshell
    > foreground { echo foo } echo bar
    foo
    bar
    > pipeline { echo baz } sed -e "s/a/u/" -e "s/$/z/"
    buzz

(Note that the prompt string here is `> `.)

`execshell` has a single builtin command: `self`. If a command line is
prefixed with this token, then the command runner process will append its own
`argv[0]` to the command line before executing into it. This can be used for
modifying the REPL's own execution environment (such as changing directory and
manipulating environment variables). For example:

    $ # PATH manipulation needed to make self re-execution work after changing directory
    $ PATH=$PWD:$PATH execshell
    > pwd
    /home/molly/src/pub/execshell
    > # execline's cd command needs a program to exec into
    > cd /home/molly
    execline-cd: usage: cd path prog...
    > self cd /home/molly
    > pwd
    /home/molly

The REPL also has rudimentary signal handling, so control-C should hopefully
deliver `SIGINT` to the foreground process without killing the REPL, however
this has only had cursory testing.

## Licence

`execshell` is Copyright (C) 2020 Molly Miller under the ISC licence. Please
see the [LICENSE](LICENSE) file for the full licence text.

The `linenoise` library is a separate project, which is licensed under the
2-clause BSD licence.

