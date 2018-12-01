# ver $

.PATH:	${.CURDIR}
KMOD=	skystar2

# PCI interface
SRCS=	skystar2.c bus_if.h device_if.h pci_if.h vnode_if.h

SRCS+=	sllutil.c
SRCS+=	i2c.c
SRCS+=	eeprom.c
SRCS+=	sram.c
#SRCS+=	diseqc.c
SRCS+=	frontend.c

# frontend chip's
SRCS+=	cobra_reg.c
SRCS+=	cobra_tuner.c
SRCS+=	cobra_drv.c
SRCS+=	cobra_api.c
SRCS+=	cobra_cx24113.c
SRCS+=	cobra_cx24128.c

SRCS+=	cx24123_24113.c

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
