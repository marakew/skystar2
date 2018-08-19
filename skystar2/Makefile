# ver $

.PATH:	${.CURDIR}
KMOD=	skystar2

# PCI interface
SRCS=	skystar2.c bus_if.h device_if.h pci_if.h vnode_if.h

SRCS+=	sllutil.c
SRCS+=	i2c.c
SRCS+=	eeprom.c
SRCS+=	sram.c
SRCS+=	diseqc.c
SRCS+=	frontend.c

# frontend chip's
SRCS+=	samsung_tbdu18132.c
SRCS+=	samsung_tbmu24112.c

# dvb-core
SRCS+=	dvbdmx.c dvbnet.c dvbdev.c
SRCS+=	crc32.c

KMODDIR= /boot/modules

#CFLAGS+= -DDEBUG_MODE

# enable optimization
CFLAGS+= -mpush-args
CFLAGS+= -mno-accumulate-outgoing-args
CFLAGS+= -O

DEBUG_FLAGS+= -s

WERROR=
#WARNS=2
#NO_WERROR=yes
#NO_WARNS=

.include <bsd.kmod.mk>
