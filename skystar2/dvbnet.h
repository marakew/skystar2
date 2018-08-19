/*
 *      SkyStar2 driver based on chip FlexCopII
 */

#ifndef _dvb_net_h_
#define _dvb_net_h_

#if 1
#include <sys/socket.h>
#include <net/if.h>
#include <net/if_var.h>
#include <netinet/in.h>
#include <net/if_arp.h> /* arpcom */
#include <net/ethernet.h>
#endif

#include "skystar2.h"
#include "dvbdmx.h"

#define DVB_NET_MULTICAST_MAX	10

struct dvb_net_priv {
	struct arpcom	arpcom;
	u_int8_t	name[6];
	u_int16_t	pid;
	struct dmx_demux *demux;
	struct dmx_sec_feed *secfeed;
	struct dmx_sec_filter *secfilter;
	int nmulti;
	struct dmx_sec_filter *multi_secfilter[DVB_NET_MULTICAST_MAX];
	u_int8_t multi_macs[DVB_NET_MULTICAST_MAX][6];
	int rx_mode;
#define RX_MODE_UNI		0
#define RX_MODE_MULTI		1
#define RX_MODE_ALL_MULTI	2
#define RX_MODE_PROMISC		3

	struct adapter		*priv;	
};

#define DVB_NET_DEVICES_MAX	10

struct dvb_net {
	struct adapter	*priv;
	struct dmx_demux *demux;
	struct ifnet	*device[DVB_NET_DEVICES_MAX];
	int		state[DVB_NET_DEVICES_MAX];
};


int
dvb_net_init(struct adapter *, struct dvb_net *, struct dmx_demux *);

void
dvb_net_release(struct dvb_net *);

int
dvb_net_ioctl(struct adapter *sc, unsigned int cmd, void *arg);
#endif
