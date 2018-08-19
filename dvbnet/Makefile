BINDIR= /usr/local/bin

PROG= dvbnet
SRCS= dvbnet.c
NOMAN=

BINMODE=4550
BINOWN=root
BINGRP=wheel

CFLAGS+= -s
# enable optimization
CFLAGS+= -mpush-args
CFLAGS+= -mno-accumulate-outgoing-args
CFLAGS+= -O

.include<bsd.prog.mk>
