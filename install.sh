#!/bin/sh
"${CC:-gcc}" ttyinit.c -o ttyinit
install -m 755 ttyinit "${PREFIX}/sbin"