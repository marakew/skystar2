#ifndef _dvb_dev_h_
#define _dvb_dev_h_

#include <sys/bus.h>

struct dvb_dev {
#if (__FreeBSD_version < 500000)
        dev_t                   dvb_node;
#else
        struct cdev             *dvb_node;
#endif
	void	*priv;
};

extern int
dvb_dev_init(struct dvb_dev *dvbdev, int unit, device_t dev);

extern int
dvb_dev_release(struct dvb_dev *dvbdev);

extern devclass_t skystar2_devclass;
#endif
