#!/bin/sh
"${CC:-gcc}" ttyopen.c -o ttyopen
install -m 755 ttyopen "${PREFIX}/sbin"