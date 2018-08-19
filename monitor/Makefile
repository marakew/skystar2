#

PATH= ${.CURDIR}
SRCS= monitor.c
PROG= monitor
NOMAN=
BINDIR= /usr/local/bin
BINOWN= root
BINGRP= wheel
BINMODE=4550

CFLAGS+= -s
# enable optimization
CFLAGS+= -mpush-args
CFLAGS+= -mno-accumulate-outgoing-args
CFLAGS+= -O

LDFLAGS+= -lcurses
.include<bsd.prog.mk>
