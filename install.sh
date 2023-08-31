#!/bin/sh
"${CC:-gcc}" ttyinit.c -o ttyinit ${CFLAGS}
install -m 755 ttyinit "${PREFIX}/sbin"