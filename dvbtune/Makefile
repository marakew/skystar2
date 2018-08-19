
.PATH:	${.CURDIR}

PROG=	dvbtune
SRCS=	dvbtune.c
NOMAN=

BINDIR=	/usr/local/bin
BINMODE=4550
BINOWN= root
BINGRP= wheel

CFLAGS+= -s
# enable optimization
CFLAGS+= -mpush-args
CFLAGS+= -mno-accumulate-outgoing-args
CFLAGS+= -O

.include <bsd.prog.mk>
