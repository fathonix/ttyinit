# `ttyopen`

This repository holds `ttyopen` - TTY opener, based on [Busybox](https://busybox.net/)
implementation of init.

I wrote this because I'm creating a very minimal distro that uses
[Toybox](http://landley.net/toybox/about.html) and
[Rich Felker's init](https://github.com/richfelker/minimal-init),
which Toybox init isn't stable yet and would be standard (and
a bit bloated like others) and Rich Felker's doesn't do any
TTY initialization, so this tiny utility will do it.

## Usage

    ttyopen TTYPATH CMD [ARG1] [ARG2] ...

Example:

    ttyopen /dev/tty1 /bin/sh

`ttyopen` must be run as superuser.

## License

    ttyopen is licensed under GPLv2, in accordance with Busybox license.
    Copyright (C) 1995-2023 The Busybox Project
    Copyright (C) 2023 Aldo Adirajasa Fathoni