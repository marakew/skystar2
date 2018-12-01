#include <sys/types.h>
#include <sys/errno.h>
#include <sys/param.h>
#include <sys/conf.h>
#include <sys/kernel.h>
#include <sys/bus.h>

#include <sys/poll.h>
#if __FreeBSD_version >= 500014
#include <sys/selinfo.h>
#else
#include <sys/select.h>
#endif
#include <sys/vnode.h>

#include <sys/sysctl.h>

#include "../include/frontend.h"
#include "../include/net.h"

#include "skystar2.h"
#include "dvbnet.h"
#include "dvbdev.h"
#include "frontend.h"
#include "debug.h"

static int debug = 1;
SYSCTL_INT(_debug, OID_AUTO, debug_dev, CTLFLAG_RW, &debug, 0,
        "enable verbose debug messages: 0-9");

/* driver device methods */
static d_open_t         dvb_open;
static d_close_t        dvb_close;
static d_read_t         dvb_read;
static d_write_t        dvb_write;
static d_ioctl_t        dvb_ioctl;
static d_mmap_t         dvb_mmap;
static d_poll_t         dvb_poll;


#if defined(__FreeBSD__) && (__FreeBSD_version < 500000)
#define CDEV_MAJOR      92
static struct cdevsw dvb_cdevsw = {
        /* open */      dvb_open,
        /* close */     dvb_close,
        /* read */      dvb_read,
        /* write */     dvb_write,
        /* ioctl */     dvb_ioctl,
        /* poll */      dvb_poll,
        /* mmap */      dvb_mmap,
        /* strategy */  nostrategy,
        /* name */      "dvb",
        /* maj */       CDEV_MAJOR,
        /* dump */      nodump,
        /* psize */     nopsize,
        /* flags */     0,
#if __FreeBSD_version < 500005
        /* bmaj */      -1
#endif
};
#else
static struct cdevsw dvb_cdevsw = {
        .d_version =    D_VERSION,
        .d_flags =      D_NEEDGIANT,
        .d_open =       dvb_open,
        .d_close =      dvb_close,
        .d_read =       dvb_read,
        .d_write =      dvb_write,
        .d_ioctl =      dvb_ioctl,
        .d_poll =       dvb_poll,
        .d_mmap =       dvb_mmap,
        .d_name =       "dvb",
};
#endif

int
dvb_dev_init(struct dvb_dev *dvbdev, int unit, device_t dev)
{
	DBG("\n");

	dvbdev->dvb_node = make_dev(&dvb_cdevsw,
				unit,
				UID_ROOT,
				GID_WHEEL,
				0600,
				"dvb%d", unit);

	dvbdev->dvb_node->si_drv1 = dev;

	return 0;
}

int
dvb_dev_release(struct dvb_dev *dvbdev)
{
	DBG("\n");

	destroy_dev(dvbdev->dvb_node);

	return 0;
}

#define UNIT(x)         ((x) & 0x0f)
#define FUNCTION(x)     (x >> 4)

static int
#if (__FreeBSD_version < 500000)
dvb_open(dev_t dev, int flags, int fmt, struct proc *p)
#else
dvb_open(struct cdev *dev, int flags, int fmt, struct thread *td)
#endif
{
	struct adapter  *sc;
	int unit;
	int err;

	//unit = UNIT(minor(dev));

	//DBG("unit %d\n", unit);
	sc = device_get_softc(dev->si_drv1);
	if (sc == NULL)
	{
		DBG("\n");
		return ENXIO;
	}

	device_busy(dev->si_drv1);

	//switch( FUNCTION(minor(dev)) )
	//{


	//}

        return 0;
}

static int
#if (__FreeBSD_version < 500000)
dvb_close(dev_t dev, int flags, int fmt, struct proc *p)
#else
dvb_close(struct cdev *dev, int flags, int fmt, struct thread *td)
#endif
{
        struct adapter  *sc;
        int unit;
	int err;

	//unit = UNIT(minor(dev));

        //DBG("unit %d\n", unit);
	sc = device_get_softc(dev->si_drv1);
	if (sc == NULL)
	{
		DBG("\n");
		return ENXIO;
	}


	//switch ( FUNCTION(minor(dev)) )
	//{


	//}

	device_unbusy(dev->si_drv1);

        return 0;
}

static int
#if (__FreeBSD_version < 500000)
dvb_read(dev_t dev, struct uio *uio, int ioflag)
#else
dvb_read(struct cdev *dev, struct uio *uio, int ioflag)
#endif
{
	DBG("\n");
	return ENXIO;
}

static int
#if (__FreeBSD_version < 500000)
dvb_write(dev_t dev, struct uio *uio, int ioflag)
#else
dvb_write(struct cdev *dev, struct uio *uio, int ioflag)
#endif
{
	DBG("\n");
	return ENXIO;
}

static int
#if (__FreeBSD_version < 500000)
dvb_ioctl(dev_t dev, u_long cmd, caddr_t arg, int flag, struct proc *p)
#else
dvb_ioctl(struct cdev *dev, u_long cmd, caddr_t arg, int flag, struct thread *td)
#endif
{
	struct adapter  *sc;
	int unit;
	int err = 0;

	//unit = UNIT(minor(dev));

	//DBG1("unit %d\n", unit);
	sc = device_get_softc(dev->si_drv1);
	if (sc == NULL)
	{
		return ENXIO;
	}

	switch (cmd){

        /* --------- [ ] -------- */
	case FE_DISEQC_SEND_MASTER_CMD:
	case FE_DISEQC_SEND_BURST:
	case FE_SET_TONE:
	case FE_SET_VOLTAGE:

	case FE_GET_INFO:
	case FE_READ_STATUS:
	case FE_READ_SIGNAL_STRENGTH:
	case FE_READ_SNR:
	case FE_SET_FRONTEND:
	case FE_GET_FRONTEND:
	case FE_SLEEP:
	case FE_INIT:
	case FE_RESET:

		err = fe_ioctl(sc->fe, cmd, arg);
		break;

	/* --------- [ ] -------- */
	case DVB_GET_MAC:
		bcopy(sc->eaddr, (u_int8_t *)arg, ETHER_ADDR_LEN);
		err = 0;
		break;

	/* --------- [ NET section ] ------------ */
	case NET_ADD_IF:
	case NET_REMOVE_IF:
	case NET_GET_IF:

		err = dvb_net_ioctl(sc, cmd, arg);
		break;

	default:
		DBG1("unsupported ioctl %08x\n", (unsigned)cmd);
		err = EINVAL;
        }
        return err;
}

static int
#if (__FreeBSD_version < 500000)
dvb_mmap(dev_t dev, vm_offset_t offset, int nprot)
#else
dvb_mmap(struct cdev *dev, vm_offset_t offset, vm_paddr_t *paddr, int nprot)
#endif
{
	DBG("\n");
	return ENXIO;
}

static int
#if (__FreeBSD_version < 500000)
dvb_poll(dev_t dev, int events, struct proc *p)
#else
dvb_poll(struct cdev *dev, int events, struct thread *tp)
#endif
{
	DBG("\n");
	return ENXIO;
}


