/*
 *      SkyStar2 driver based on chip FlexCopII
 */

#include <sys/types.h>
#include <sys/errno.h>

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/sysctl.h>

#include <sys/socket.h>
#include <net/if.h>
#include <net/if_var.h>
#include <net/bpf.h>
#include <netinet/in.h>
#include <net/if_arp.h>
#include <net/ethernet.h>
#include <net/route.h>
#include <sys/mbuf.h>
#include <sys/sockio.h>
#if __FreeBSD_version >= 600000
#include <net/if_types.h>
#include <net/if_dl.h>
#include <net/if_var.h>
#endif

#include "skystar2.h"
#include "../include/net.h"
#include "debug.h"

// >= 6.0
#if __FreeBSD_version >= 600000
#define	IFF_RUNNING	IFF_DRV_RUNNING
#define	IFF_OACTIVE	IFF_DRV_OACTIVE
#endif

static int debug = 1;
SYSCTL_INT(_debug, OID_AUTO, debug_net, CTLFLAG_RW, &debug, 0,
        "enable verbose debug messages: 0-9");

/*---------------------------------------------------*/
static void
dvb_net_sec(struct ifnet *ifp, u_int8_t *pkt, int pkt_len)
{
	struct ether_header eh;
//	u_int8_t ethb[/* sizeof(struct ether_header) + pkt_len-12 */4096];
	u_int8_t *eth;
	u_int8_t *ethf;
	struct mbuf *m;
	int snap = 0;

	eth = (u_int8_t *)&eh;

	DBG2("\n");

	/* note: pkt_len includes a 32bit checksum */
	if (pkt_len < 16)
	{
		DBG("IP/MPE packet length = %d too small\n", pkt_len);
		ifp->if_ierrors++;
		return;
	}

	/* it seems some ISPs manage to screw up here, so we have to
	 * relax the error checks... */
#if 0
	if ((pkt[5] & 0xfd) != 0xc1)
	{
		/* drop scrambled or broken packets */
#else
	if ((pkt[5] & 0x3c) != 0x00)
	{
		/* drop scrambled */
#endif
		ifp->if_ierrors++;
		return;
	}

	if (pkt[5] & 0x02)
	{
		/* FIXME: handle LLC/SNAP, see rfc-1042 */
		if (pkt_len < 24 || memcmp(&pkt[12], "\xaa\xaa\x03\0\0\0", 6)) 
		{
			ifp->if_iqdrops++;
			return;
		}
		snap = 8;
	}

	if (pkt[7])
	{
		/* FIXME: assemble datagram from multiple sections */
		ifp->if_ierrors++;
		return;
	}

	/* we have 
	 * +14 byte ethernet header (ip header follows);	
	 * -12 byte MPE header; 
	 *  -4 byte checksum; +2 byte alignment
	 *  -8 byte LLC/SNAP
	 */

	/* ether_dhost[6] */
	eth[0]  = pkt[11];
	eth[1]  = pkt[10];
	eth[2]  = pkt[9];
	eth[3]  = pkt[8];
	eth[4]  = pkt[4];
	eth[5]  = pkt[3];

	/* ether_shost[6] */
	eth[6]  = 0;
	eth[7]  = 0;
	eth[8]  = 0;
	eth[9]  = 0;
	eth[10] = 0;
	eth[11] = 0;

	if (snap) 
	{
		eth[12] = pkt[18];
		eth[13] = pkt[19];
	} else {
		/* protocol numbers are from rfc-1700 or
		 * http://www.iana.org/assignments/ethernet-numbers
		 */
		if (pkt[12] >> 4 == 6) /* version field from IP header */
		{
			eth[12] = 0x86; /* IPv6 */
			eth[13] = 0xdd;
		} else {
			eth[12] = 0x08; /* IPv4 */
			eth[13] = 0x00;
		}
	}

	/* ether_type = ETHERTYPE_IP 0x0800 = htohs() */
//	eth[12] = 8;
//	eth[13] = 0;

		DBG3("MAC: %6D -> %6D : type %u of %u bytes\n",
			eh.ether_shost, ":", eh.ether_dhost, ":",
			eh.ether_type, pkt_len -12 -4);

	if (eh.ether_type == ntohs(ETHERTYPE_IP))
	{
		DBG3("IP: %u.%u.%u.%u -> %u.%u.%u.%u\n",
			pkt[12+12], pkt[12+13], pkt[12+14], pkt[12+15],
			pkt[12+16], pkt[12+17], pkt[12+18], pkt[12+19]);

	}

	/* skip filled Ethernet Header eth +14
	 * skip MPE header pkt +12
	 * total_len = -12(MPE) -4(crc) -(snap)
	 */

#if __FreeBSD_version < 500000
	m = m_devget((caddr_t)(pkt +12), pkt_len -12, 0, ifp, NULL);
	if (m == NULL)
	{
		ifp->if_ierrors++;
		DBG("out of mbuf's\n");
		return;
	}
#else
	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if (m == NULL)
		return;
	m->m_pkthdr.rcvif = ifp;
	m->m_pkthdr.len = m->m_len = (pkt_len - 12) +14;
	
	if (((pkt_len-12) + 14 + 2) > MHLEN) 
	{
		MCLGET(m, M_DONTWAIT);
		if ((m->m_flags & M_EXT) == 0) 
		{
			m_freem(m);
			return;
		}
	}
	m->m_data += 2;
	ethf = (u_int8_t *)mtod(m, struct ether_header *);
	bcopy(eth, ethf, 14);
	bcopy(pkt +12 + snap, ethf +14, pkt_len-12-snap);

//	m->m_pkthdr.len = m->m_len = (pkt_len-12) +14;
	if (m == NULL) 
	{
		ifp->if_ierrors++;
		DBG("out of mbuf's\n");
		return;
	}
		
#endif

	ifp->if_ipackets++;

#if __FreeBSD_version < 500000
	if (ifp->if_bpf)
		bpf_mtap(ifp, m);
// 4.0
	ether_input(ifp, &eh, m);
#else
//	BPF_MTAP(ifp, m);
// 5.3
	ether_input(ifp, m);
//	(*ifp->if_input)(ifp, m);
#endif

	return;
}

/*---------------------------------------------------*/
static int
dvb_net_callback(const u_int8_t *buf1, size_t len1,
		 const u_int8_t *buf2, size_t len2,
		 struct dmx_sec_filter *filter,
		 dmx_success_t success)
{
	struct ifnet *ifp = (struct ifnet *) filter->priv;

	DBG2("\n");
	dvb_net_sec(ifp, (u_int8_t *)buf1, len1);
	return 0;
}

/*---------------------------------------------------*/

static u_int8_t mask_normal   [6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
static u_int8_t mask_allmulti [6] = {0xff, 0xff, 0xff, 0x00, 0x00, 0x00};
static u_int8_t mac_allmulti  [6] = {0x01, 0x00, 0x5e, 0x00, 0x00, 0x00};
static u_int8_t mask_promisc  [6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};


static int
dvb_net_filter_set(struct ifnet *ifp,
		   struct dmx_sec_filter **secfilter,
		   u_int8_t *mac, u_int8_t *mac_mask)
{
	struct dvb_net_priv *priv = (struct dvb_net_priv *)ifp->if_softc;
	int ret;

	*secfilter = 0;
	ret = priv->secfeed->alloc_filter(priv->secfeed, secfilter);
	if (ret < 0)
	{
		printf("could not get filter\n");
		return ret;
	}

	(*secfilter)->priv = (void *)ifp;
	
	memset((*secfilter)->filter_val , 0x00, DMX_MAX_FILTER_SIZE);
	memset((*secfilter)->filter_mask, 0x00, DMX_MAX_FILTER_SIZE);
	memset((*secfilter)->filter_mode, 0xff, DMX_MAX_FILTER_SIZE);

	(*secfilter)->filter_val[0]  = 0x3e;
	(*secfilter)->filter_val[3]  = mac[5];
	(*secfilter)->filter_val[4]  = mac[4];
	(*secfilter)->filter_val[8]  = mac[3];
	(*secfilter)->filter_val[9]  = mac[2];
	(*secfilter)->filter_val[10] = mac[1];
	(*secfilter)->filter_val[11] = mac[0];

	(*secfilter)->filter_mask[0]  = 0xff;
	(*secfilter)->filter_mask[3]  = mac_mask[5];
	(*secfilter)->filter_mask[4]  = mac_mask[4];
	(*secfilter)->filter_mask[8]  = mac_mask[3];
	(*secfilter)->filter_mask[9]  = mac_mask[2];
	(*secfilter)->filter_mask[10] = mac_mask[1];
	(*secfilter)->filter_mask[11] = mac_mask[0];

	printf("filter mac = %6D\n", mac, ":");
	printf("filter mask = %6D\n", mac_mask, ":");

	return 0;
}

static int
dvb_net_feed_start(struct ifnet *ifp)
{
	int ret, i;
	struct dvb_net_priv *priv = (struct dvb_net_priv *)ifp->if_softc;
	struct dmx_demux *demux = priv->demux;
#if __FreeBSD_version < 600000
	u_int8_t *mac = (u_int8_t *)priv->arpcom.ac_enaddr;
#elif __FreeBSD_version < 700000
	u_int8_t *mac = (u_int8_t *)priv->arpcom._ac_enaddr;
#else
	u_int8_t *mac = (u_int8_t *)IF_LLADDR(ifp);
#endif
	DBG("rx mode %d\n", priv->rx_mode);

	if (priv->secfeed || priv->secfilter || priv->multi_secfilter[0])
		printf("BUG %d\n", __LINE__);

	priv->secfeed = 0;
	priv->secfilter = 0;

	DBG("alloc secfeed\n");
	ret = demux->alloc_sec_feed(demux, &priv->secfeed, dvb_net_callback);
	if (ret < 0)
	{
		printf("could't alloc sec feed\n");
		return ret;
	}

	ret = priv->secfeed->set(priv->secfeed, priv->pid, 32768, 0, 1);
	if (ret < 0)
	{
		printf("could't not set sec feed\n");
		demux->free_sec_feed(priv->demux, priv->secfeed);
		priv->secfeed = 0;
		return ret;
	}


	if (priv->rx_mode != RX_MODE_PROMISC)
	{
		DBG("set secfilter !PROMISC\n");
		dvb_net_filter_set(ifp, &priv->secfilter, mac, mask_normal);
	}

	switch(priv->rx_mode){
	case RX_MODE_MULTI:
		for (i = 0; i < priv->nmulti; i++)
		{
			DBG("set multi_secfilter[%d]\n", i);
			dvb_net_filter_set(ifp, &priv->multi_secfilter[i],
					priv->multi_macs[i], mask_normal);
		}
		break;
	case RX_MODE_ALL_MULTI:
		priv->nmulti = 1;
		DBG("set multi_secfilter[0]\n");
		dvb_net_filter_set(ifp, &priv->multi_secfilter[0], 
					mac_allmulti, mask_allmulti);
		break;
	case RX_MODE_PROMISC:
		priv->nmulti = 0;
		DBG("set secfilter PROMISC\n");
		dvb_net_filter_set(ifp, &priv->secfilter, mac, mask_promisc);
		break;
	}

	DBG("start secfeed filter\n");
	priv->secfeed->start_filter(priv->secfeed);
	return 0;
}

static int
dvb_net_feed_stop(struct ifnet *ifp)
{
	struct dvb_net_priv *priv = (struct dvb_net_priv *)ifp->if_softc;
	struct dmx_demux *demux = priv->demux;
	int i;

	DBG("\n");
	if (priv->secfeed)
	{
		if (priv->secfeed->is_filter)
		{
			DBG("stop secfeed filter\n");
			priv->secfeed->stop_filter(priv->secfeed);
		}

		if (priv->secfilter)
		{
			DBG("free secfilter\n");
			priv->secfeed->free_filter(priv->secfeed, 
						priv->secfilter);
			priv->secfilter = 0;
		}

		for (i = 0; i< priv->nmulti; i++)
		{
			if (priv->multi_secfilter[i])
			{
				DBG("free multi_filter[%d]\n", i);
				priv->secfeed->free_filter(priv->secfeed, 
						priv->multi_secfilter[i]);
				priv->multi_secfilter[i] = 0;
			}
		}

		demux->free_sec_feed(priv->demux, priv->secfeed);
		priv->secfeed = 0;
	} else {
		DBG("no feed to stop\n");
	}
	return 0;
}


/*-------------------------------------------------------*/
static int
dvb_net_open(struct ifnet *ifp)
{
	DBG("\n");

	dvb_net_feed_start(ifp);
	return 0;
}

static int
dvb_net_stop(struct ifnet *ifp)
{
	DBG("\n");

	dvb_net_feed_stop(ifp);
	return 0;
}

static int
dvb_net_update_rx_mode(struct ifnet *ifp)
{
	struct dvb_net_priv *priv = (struct dvb_net_priv *)ifp->if_softc;


	if (ifp->if_flags & IFF_PROMISC && priv->rx_mode != RX_MODE_PROMISC)
	{
		priv->rx_mode = RX_MODE_PROMISC;
		ifp->if_flags &= ~ IFF_RUNNING;
	} else
	if (!(ifp->if_flags & IFF_PROMISC) && priv->rx_mode == RX_MODE_PROMISC)
	{
		priv->rx_mode = RX_MODE_UNI;
		ifp->if_flags &= ~ IFF_RUNNING;
	}

	return 0;
}

static void
net_init(void *arg)
{
	struct dvb_net_priv *sc = (struct dvb_net_priv *)arg;
#if __FreeBSD_version < 600000
	struct ifnet *ifp = &sc->arpcom.ac_if;
#else
	struct ifnet *ifp = sc->arpcom.ac_ifp;
#endif
	int s;
	DBG("\n");
	s = splimp();

	ifp->if_flags &= ~(IFF_RUNNING | IFF_OACTIVE);

	ifp->if_mtu = 4096;

	dvb_net_stop(ifp);
	dvb_net_open(ifp);

	ifp->if_flags |= IFF_RUNNING;
	ifp->if_flags &= ~IFF_OACTIVE;

	splx(s);
	return;
}

static void
net_stop(struct ifnet *ifp)
{
	int s;
	DBG("\n");
	s = splimp();

	ifp->if_flags &= ~(IFF_RUNNING | IFF_OACTIVE);
	dvb_net_stop(ifp);

	splx(s);
	return;	
}

static void
net_watchdog(struct ifnet *ifp)
{
	DBG("\n");
	return;
}

static int
net_ioctl(struct ifnet *ifp, u_long cmd, caddr_t arg)
{
	int s, err;

	DBG("\n");

	s = splimp();

	switch(cmd){
	case SIOCSIFADDR:
		DBG2("SIOCSIFADDR\n");
		goto ether_cont;
	case SIOCGIFADDR:
		DBG2("SIOCGIFADDR\n");
		goto ether_cont;
	case SIOCSIFMTU:
		DBG2("SIOCSIFMTU\n");
		ifp->if_mtu = 4096;
		/*goto ether_cont;*/
		err = 0;
		break;
ether_cont:
		err = ether_ioctl(ifp, cmd, arg);
		break;

	case SIOCSIFFLAGS:
		DBG2("SIOCSIFFLAGS\n");

		dvb_net_update_rx_mode(ifp);

		if (ifp->if_flags & IFF_UP)
		{
			if ((ifp->if_flags & IFF_RUNNING) == 0)
				net_init(ifp->if_softc);
		} else {
			if (ifp->if_flags & IFF_RUNNING)
				net_stop(ifp);
		}
		err = 0;
		break;

	case SIOCADDMULTI:
	case SIOCDELMULTI:
		DBG2("SIOC___MULTI\n");
		err = EINVAL;
		break;

	case SIOCGIFMEDIA:
		DBG2("SIOCGIFMEDIA\n");
		err = EINVAL;
		break;

	case SIOCSIFMEDIA:
		DBG2("SIOCSIFMEDIA\n");
		err = EINVAL;
		break;	

	default:
		DBG2("default\n");
		err = EINVAL;
		break;
	}
	splx(s);

	return err;
}

static void
net_start(struct ifnet *ifp)
{
	DBG("\n");

	if (ifp->if_flags & IFF_OACTIVE)
	{
		return;
	}
#if 0
	dvb_net_open(ifp);
#endif
	return;
}

static int
net_output(struct ifnet *ifp,
	   struct mbuf *m,
	   struct sockaddr *sa,
#if __FreeBSD_version < 500000
	   struct rentry *rt)
#else
	   struct rtentry *rt)
#endif
{
	DBG("\n");

	m_freem(m);
	
	return EIO;
}

static int
dvb_net_init_dev(struct ifnet *ifp, int if_num, u_int8_t *eaddr)
{

	DBG("\n");

#if __FreeBSD_version < 500000
	ifp->if_name = "dvb";
	ifp->if_unit = if_num;
#elif __FreeBSD_version >= 501113
	if_initname(ifp, "dvb", if_num);
#endif
	ifp->if_mtu = 4096;
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
	ifp->if_ioctl = net_ioctl;
	ifp->if_output = net_output;
	ifp->if_start = net_start;
	ifp->if_watchdog = net_watchdog;
	ifp->if_init = net_init;
	ifp->if_baudrate = 40000000; /* 40 MBit */

#if __FreeBSD_version < 500000
	ether_ifattach(ifp, ETHER_BPF_SUPPORTED);
#else
	ether_ifattach(ifp, eaddr);
#endif

	return 0;
}

static int
dvb_net_free_dev(struct ifnet *ifp)
{
	DBG("\n");

#if __FreeBSD_version < 500000
	ether_ifdetach(ifp, ETHER_BPF_SUPPORTED);
#else
	ether_ifdetach(ifp);
#endif
	
	return 0;
}

static int
dvb_get_if(struct dvb_net *dvbnet)
{
	int i;

	for (i = 0; i < DVB_NET_DEVICES_MAX; i++)
	{
		DBG("state[%d] = %d\n", i, dvbnet->state[i]);
		if (!dvbnet->state[i])
			break;
	}

	if (i == DVB_NET_DEVICES_MAX)
		return -1;

	dvbnet->state[i] = 1;
	DBG("state[%d] = %d ok!\n", i, dvbnet->state[i]);
	return i;
}


/* call from Ioctl net Device */
static int
dvb_net_add_if(struct dvb_net *dvbnet, u_int16_t pid)
{
	struct ifnet *ifp;
	struct ifnet *net;
	struct adapter *sc = dvbnet->priv;
	struct dmx_demux *demux;
	struct dvb_net_priv *priv;
	int res;
	int if_num;
	int s;
	
	DBG("\n");

	s = splimp();
	if ((if_num = dvb_get_if(dvbnet)) < 0)
		return -EINVAL;

	DBG("num %d\n", if_num);

	demux = dvbnet->demux;

	if (!(priv = 
	  malloc(sizeof(struct dvb_net_priv), M_DEVBUF, M_NOWAIT | M_ZERO))){
		splx(s);
		return -ENOMEM;
	}

/*	bzero(priv, sizeof(struct dvb_net_priv));*/

#if __FreeBSD_version < 600000
	dvbnet->device[if_num] = &priv->arpcom.ac_if;
#else
	priv->arpcom.ac_ifp = if_alloc(IFT_ETHER);
	if (priv->arpcom.ac_ifp == NULL)
	{
		free(priv, M_DEVBUF);
		splx(s);	
		return -ENOMEM;
	}

	dvbnet->device[if_num] = priv->arpcom.ac_ifp;
#endif
	priv->priv = sc;

#if __FreeBSD_version < 600000
	priv->arpcom.ac_if.if_softc = priv;
	bcopy(sc->eaddr, (char *)&priv->arpcom.ac_enaddr, ETHER_ADDR_LEN); 
#else
	priv->arpcom.ac_ifp->if_softc = priv;
#if __FreeBSD_version < 700000
	bcopy(sc->eaddr, (char *)&priv->arpcom._ac_enaddr, ETHER_ADDR_LEN); 
#endif
#endif

	priv->demux = demux;
	priv->pid = pid;
	priv->rx_mode = RX_MODE_UNI;


#if __FreeBSD_version < 600000
	dvb_net_init_dev(&priv->arpcom.ac_if, if_num, sc->eaddr);
#else
	dvb_net_init_dev(priv->arpcom.ac_ifp, if_num, sc->eaddr);
#endif

	splx(s);

	return if_num;
}

static int
dvb_net_remove_if(struct dvb_net *dvbnet, int num)
{
	struct adapter *sc = dvbnet->priv;
	struct ifnet *ifp;
	struct dvb_net_priv *priv;
	int s;

	DBG("num %d\n", num);

	if (!dvbnet->state[num])
		return -EINVAL;

	DBG("num %d delete\n", num);
	s = splimp();

	ifp = dvbnet->device[num];
	priv = (struct dvb_net_priv *)ifp->if_softc;

	dvb_net_stop(ifp);
	dvb_net_free_dev(ifp);

#if __FreeBSD_version < 600000

#else
	if (priv->arpcom.ac_ifp)
		if_free(priv->arpcom.ac_ifp);
#endif
	if (priv)
		free(priv, M_DEVBUF);

	dvbnet->state[num] = 0;
	dvbnet->device[num] = NULL;
	splx(s);

	return 0;
}

int
dvb_net_ioctl(struct adapter *sc, unsigned int cmd, void *arg)
{
	struct dvb_net *dvbnet = &sc->dvbnet;

	DBG("\n");

	switch(cmd){
	case NET_ADD_IF: {
		struct dvb_net_if *dvbnetif = (struct dvb_net_if *)arg;
		int res;
#if __FreeBSD_version < 500000
		if ((res = suser(curproc)) != 0)
#else
		if ((res = suser(curthread)) != 0)
#endif
			return res;

		res = dvb_net_add_if(dvbnet, dvbnetif->pid);
		if (res < 0)
			return res;
		dvbnetif->if_num = res;
		break; 
	}
	case NET_GET_IF: {
		struct ifnet *ifp;
		struct dvb_net_priv *priv;
		struct dvb_net_if *dvbnetif = (struct dvb_net_if *)arg;
		
		if (dvbnetif->if_num >= DVB_NET_DEVICES_MAX ||
			!dvbnet->state[dvbnetif->if_num]){
				return -EINVAL;
		}

		ifp = dvbnet->device[dvbnetif->if_num];

		priv = (struct dvb_net_priv *)ifp->if_softc;
		dvbnetif->pid = priv->pid;
		
		break; 
	}
	case NET_REMOVE_IF: {
		int res;
#if __FreeBSD_version < 500000
		if ((res = suser(curproc)) != 0)
#else
		if ((res = suser(curthread)) != 0)
#endif
			return res;
		return dvb_net_remove_if(dvbnet, *(int *)arg);
		break; 
	}
	default:
		return -EOPNOTSUPP;
	}
	return 0;	
}

int
dvb_net_init(struct adapter *sc, struct dvb_net *dvbnet, struct dmx_demux *dmx)
{
	int i;
	DBG("\n");
	bzero(dvbnet, sizeof(struct dvb_net));
	dvbnet->demux = dmx;
	dvbnet->priv = sc;

	for (i = 0; i < DVB_NET_DEVICES_MAX; i++)
	{
			dvbnet->state[i] = 0;
			DBG("state[%d] = %d\n", i, dvbnet->state[i]);
	}
	return 0;
}

void
dvb_net_release(struct dvb_net *dvbnet)
{
	int i;
	DBG("\n");

	for (i = 0; i < DVB_NET_DEVICES_MAX; i++)
	{
		DBG("state[%d] = %d check\n", i, dvbnet->state[i]);
		if (!dvbnet->state[i])
			continue;
		DBG("state[%d] = %d del\n", i, dvbnet->state[i]);
		dvb_net_remove_if(dvbnet, i);
	}

}
