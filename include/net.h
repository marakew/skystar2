#ifndef _net_h_
#define _net_h_

#include <sys/types.h>
#include <sys/ioccom.h>

struct dvb_net_if {
	u_int16_t	pid;
	u_int16_t	if_num;
};

#define	NET_ADD_IF	_IOWR('o', 52, struct dvb_net_if)
#define	NET_REMOVE_IF	_IO('o', 53)
#define NET_GET_IF	_IOWR('o', 54, struct dvb_net_if)

#define DVB_NET_DEVICES_MAX     10

#endif
