/*
 *      SkyStar2 driver based on chip FlexCopII
 */

#include <sys/types.h>
#include <sys/queue.h>

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/sysctl.h>

#include "skystar2.h"
#include "crc32.h"
#include "debug.h"


static int debug = 1;
SYSCTL_INT(_debug, OID_AUTO, debug_dmx, CTLFLAG_RW, &debug, 0,
        "enable verbose debug messages: 0-9");

/* to monitor payload loss in the syslog */
//#define DVB_DEMUX_SEC_LOSS_LOG


static __inline u_int16_t
sec_length(const u_int8_t *buf)
{
	return (3 + ((buf[1] & 0x0f) << 8) + buf[2]);
}

static __inline u_int16_t
ts_pid(const u_int8_t *buf)
{
	return (((buf[1] & 0x1f) << 8) + buf[2]);
}

static __inline u_int8_t
payload(const u_int8_t *tsp)
{
	if (!(tsp[3] & 0x10)) /* no payload ? */
		return 0;

	if (tsp[3] & 0x20)  /* adaptation field ? */
	{
		if (tsp[4] > 183) /* corrupted data */
			return 0;
		else	return (184 - 1 - tsp[4]);
	}
	return 184;
}

/*-----------------------------------------------------------------------*/
void
dvb_set_crc32(u_int8_t *data, int length)
{
	u_int32_t crc;

	crc = crc32_be(~0, data, length);
	
	data[length    ] = (crc >> 24) & 0xff;
	data[length + 1] = (crc >> 16) & 0xff;
	data[length + 2] = (crc >>  8) & 0xff;
	data[length + 3] = (crc)       & 0xff;
}

/*-----------------------------------------------------------------------*/
static u_int32_t
dvb_dmx_crc32(struct dvb_demux_feed *f, const u_int8_t *src, size_t len)
{
	f->feed.sec.crc_val = crc32_be(f->feed.sec.crc_val, src, len);
	return f->feed.sec.crc_val;
}

/*-----------------------------------------------------------------------*/
static void
dvb_dmx_memcpy(struct dvb_demux_feed *f, u_int8_t *d, const u_int8_t *s, size_t len)
{
	memcpy(d, s, len);
}

/*-----------------------------------------------------------------------*/
static int
dvb_dmx_swfilter_secfilter(struct dvb_demux_feed *feed, struct dvb_demux_filter * f)
{
	u_int8_t neq = 0;
	int i;

	for(i = 0; i < DVB_DEMUX_MASK_MAX; i++)
	{
		u_int8_t xor = f->filter.filter_val[i] ^ 
				feed->feed.sec.secbuf[i]; 

		if (f->maskandmode[i] & xor)
			return 0;

		neq |= f->maskandnotmode[i] & xor;
	}

	if (f->doneq && !neq)
		return 0;
	
	return feed->cb.sec(feed->feed.sec.secbuf, feed->feed.sec.seclen,
			    0, 0, &f->filter, DMX_OK);
}

/*-----------------------------------------------------------------------*/
static __inline int 
dvb_dmx_swfilter_sec_feed(struct dvb_demux_feed *feed)
{
	struct dvb_demux *demux = feed->demux;
	struct dvb_demux_filter *f = feed->filter;
	struct dmx_sec_feed *sec = &feed->feed.sec;
	u_int8_t *buf = sec->secbuf;
	int sec_syntax_indicator;
#if 0
	if (sec->secbufp != sec->seclen)
		return -1;
#endif
	if (!sec->is_filter)
		return 0;

	if (!f)
		return 0;

	if (sec->check_crc)
	{
		sec_syntax_indicator = ((sec->secbuf[1] & 0x80) != 0);
		if (sec_syntax_indicator &&
		    demux->check_crc32(feed, sec->secbuf, sec->seclen))
			return -1;
	}

	do {
		if (dvb_dmx_swfilter_secfilter(feed, f) < 0)
			return -1;
	} while ((f = f->next) && sec->is_filter);

	sec->secbufp = sec->seclen = 0;

	return 0;
}

/*-----------------------------------------------------------------------*/
static int
dvb_dmx_swfilter_sec_new(struct dvb_demux_feed *feed)
{
	struct dmx_sec_feed *sec = &feed->feed.sec;

#ifdef DVB_DEMUX_SEC_LOSS_LOG
	if (sec->secbufp < sec->tsfeedp)
	{
		int i,n = sec->tsfeedp - sec->secbufp;
		
		/* section padding is done with 0xff bytes entirely.
		 * due to speed reasons, we won't check all of them
		 * but just first and last
		 */
		if (sec->secbuf[0] != 0xff || sec->secbuf[n-1] != 0xff){
			DBG("section ts padding loss: %d/%d\n",
				n, sec->tsfeedp);
			DBG("pad data:");
			for (i = 0; i < n; i++)
				DBG(" %02x", sec->secbuf[i]);
			DBG("\n");
		}
	}
#endif

	sec->tsfeedp = sec->secbufp = sec->seclen = 0;
	sec->secbuf = sec->secbuf_base;
}

/*-----------------------------------------------------------------------*/
static int
dvb_dmx_swfilter_sec_copy_dump(struct dvb_demux_feed *feed, const u_int8_t *buf, u_int8_t len)
{
	struct dvb_demux *demux = feed->demux;
	struct dmx_sec_feed *sec = &feed->feed.sec;
	u_int16_t limit, seclen, n;

	if (sec->tsfeedp >= DMX_MAX_SECFEED_SIZE)
		return 0;

	if (sec->tsfeedp + len > DMX_MAX_SECFEED_SIZE)
	{
#ifdef DVB_DEMUX_SEC_LOSS_LOG
		DBG("section buffer full loss: %d/%d\n",
			sec->tsfeedp + len - DMX_MAX_SECFEED_SIZE,
			DMX_MAX_SECFEED_SIZE);
#endif
		len = DMX_MAX_SECFEED_SIZE - sec->tsfeedp;
	}

	if (len <= 0)
		return 0;

	demux->memcpy(feed, sec->secbuf_base + sec->tsfeedp, buf, len);
	sec->tsfeedp += len;

	/*
	 * Dump all the sections we can find in the data (Emard)
	 */
	limit = sec->tsfeedp;
	if (limit > DMX_MAX_SECFEED_SIZE)
		return -1;

	/* to be sure always set secbuf */
	sec->secbuf = sec->secbuf_base + sec->secbufp;

	for (n = 0; sec->secbufp + 2 < limit; n++)
	{

		seclen = sec_length(sec->secbuf);
		if (seclen <= 0 || seclen > DMX_MAX_SECFEED_SIZE ||
		    (seclen + sec->secbufp) > limit)
			return 0;

		sec->seclen = seclen;
		sec->crc_val = ~0;

		/* dump [secbuf .. secbuf+seclen) */
		if (feed->pusi_seen)
			dvb_dmx_swfilter_sec_feed(feed);
#ifdef DVB_DEMUX_SEC_LOSS_LOG
		else
			DBG("pusi not seen, discarding section data\n");
#endif

		sec->secbufp += seclen;
		sec->secbuf += seclen;
	}		
	
	return 0;
}

/*-----------------------------------------------------------------------*/
static int
dvb_dmx_swfilter_sec_packet(struct dvb_demux_feed *feed, const u_int8_t *buf)
{
	u_int8_t p, count;
	int ccok, dc_i = 0;
	u_int8_t cc;
	
	DBG2("\n");

	count = payload(buf);

	if (count == 0)		/* count == 0 if no payload or out of range */
		return -1;

	p = 188 - count;	/* payload start */
	
	cc = buf[3] & 0x0f;
	ccok = ((feed->cc + 1) & 0x0f) == cc;// ? 1:0;
	feed->cc = cc;

	if (buf[3] & 0x20)
	{
		/* adaption field present, check for discontinuity_indicator */
		if ((buf[4] > 0) && (buf[5] & 0x80))
			dc_i = 1;
	}

//	if (ccok == 0){
	if (!ccok || dc_i)
	{
#ifdef DVB_DEMUX_SEC_LOSS_LOG

		DBG("discontinuity detect %d bytes lost\n", count);
#endif
		feed->pusi_seen = 0;
		dvb_dmx_swfilter_sec_new(feed);
		//return 0;
	}

	if (buf[1] & 0x40)
	{
		/* PUSI=1 (is set), section boundary is here */
		if (count > 1 && buf[p] < count)
		{
			const u_int8_t *before = &buf[p + 1];
			u_int8_t before_len = buf[p];
			const u_int8_t *after = &before[before_len];
			u_int8_t after_len = count - 1 - before_len;

			dvb_dmx_swfilter_sec_copy_dump(feed, before, before_len);

			/* before start of new section, set pusi_seen = 1 */
			feed->pusi_seen = 1;
			dvb_dmx_swfilter_sec_new(feed);
			dvb_dmx_swfilter_sec_copy_dump(feed, after,  after_len);
		}
#ifdef DVB_DEMUX_SEC_LOSS_LOG
		 else	if (count > 0)
				DBG("PUSI=1 but %d bytes lost\n", count);
#endif
	} else {
		/* PUSI=0 (is not set), no section boundary */

		//const u_int8_t *entire = &buf[p];
		//u_int8_t entire_len = count;
		//dvb_dmx_swfilter_sec_copy_dump(feed, entire, entire_len);

		dvb_dmx_swfilter_sec_copy_dump(feed, &buf[p], count);
	}	
	return 0;
}

/*--------------------------------------------------------------------------*/
static void
dvb_dmx_swfilter_packet(struct dvb_demux *demux, const u_int8_t *buf)
{
	struct dvb_demux_feed *feed;
	u_int16_t pid = ts_pid(buf);

	DBG2("pid =%d\n", pid);

	LIST_FOREACH(feed, &demux->dvb_demux_feed_head, dvb_demux_feed_next)
	{
		if ((feed->pid != pid) && (feed->pid != 0x2000))
			continue;

		if (feed->pid == pid)
		{
			if (!feed->feed.sec.is_filter)
				continue;

			if (dvb_dmx_swfilter_sec_packet(feed, buf) < 0)
			{
				feed->feed.sec.seclen = 0;
				feed->feed.sec.secbufp = 0;
			}

		}
	}
}

/*--------------------------------------------------------------------------*/
void
dvb_dmx_swfilter_packets(struct dvb_demux *demux, const u_int8_t *buf, size_t count)
{
	int s;
	DBG2("\n");

		/* spin_lock demux->lock */
		s = splhigh();

	while (count--)
	{
					/* byte SYNC */
		if (buf[0] == 0x47)
		{	
			dvb_dmx_swfilter_packet(demux, buf);
		}
		 else
		{
			DBG("desync (%x) in frame [%d]\n", buf[0], count);
		}

		buf += 188;
	}	
		/* spin_unlock demux->lock */
		splx(s);
}


/*--------------------------------------------------------------------------*/
static struct dvb_demux_filter *
dvb_dmx_filter_alloc(struct dvb_demux *demux)
{
	int i;

	DBG("\n");

	for (i = 0; i < demux->nfilter; i++)
		if (demux->filter[i].state == DMX_STATE_FREE)
			break;

	if (i == demux->nfilter)
		return NULL;

	demux->filter[i].state = DMX_STATE_ALLOC;
	return &demux->filter[i];
}

/*-----------------------------------------------------------------------*/
static struct dvb_demux_feed *
dvb_dmx_feed_alloc(struct dvb_demux *demux)
{
	int i;

	DBG("\n");

	for (i = 0; i < demux->nfeed; i++)
		if (demux->feed[i].state == DMX_STATE_FREE)
			break;

	if (i == demux->nfeed)
		return NULL;

	demux->feed[i].state = DMX_STATE_ALLOC;
	return &demux->feed[i];
}
	

/*-----------------------------------------------------------------------*/
static int
dvb_demux_feed_find(struct dvb_demux_feed *feed)
{
	struct dvb_demux_feed *entry;
	
	DBG("\n");

	LIST_FOREACH(entry, &feed->demux->dvb_demux_feed_head, dvb_demux_feed_next)
		if (entry == feed)
			return 1;
	return 0;
}

/*-----------------------------------------------------------------------*/
static int
dvb_demux_feed_add(struct dvb_demux_feed *feed)
{
	DBG("\n");

	if (dvb_demux_feed_find(feed))
	{
		return -1;
	}
	LIST_INSERT_HEAD(&feed->demux->dvb_demux_feed_head, feed, dvb_demux_feed_next);
	return 1;
}

/*-----------------------------------------------------------------------*/
static int
dvb_demux_feed_del(struct dvb_demux_feed *feed)
{
	struct dvb_demux_feed *entry;

	DBG("\n");

	if (!(dvb_demux_feed_find(feed)))
	{
		return -1;
	}

	LIST_FOREACH(entry, &feed->demux->dvb_demux_feed_head, dvb_demux_feed_next)
	{
		if (entry == feed)
		{
			LIST_REMOVE(feed, dvb_demux_feed_next);
			break;
		}
	}

	return 1;
}
/*-------------------------------------------------------------------------*/
/*		Section Feed API					   */
/*-------------------------------------------------------------------------*/

static int
dmx_sec_feed_alloc_filter(struct dmx_sec_feed *feed,
			  struct dmx_sec_filter **filter)
{

	struct dvb_demux_feed *dvbdmxfeed = (struct dvb_demux_feed *)feed;
	struct dvb_demux *dvbdemux = dvbdmxfeed->demux;
	struct dvb_demux_filter *dvbdmxfilter;
	
	DBG("\n");

	dvbdmxfilter = dvb_dmx_filter_alloc(dvbdemux);
	if (dvbdmxfilter == NULL)
	{
		return -EBUSY;
	}

	*filter = &dvbdmxfilter->filter;
	(*filter)->parent = feed;
	(*filter)->priv = 0;
	dvbdmxfilter->feed = dvbdmxfeed;
	dvbdmxfilter->state = DMX_STATE_READY;
	dvbdmxfilter->next = dvbdmxfeed->filter;
	dvbdmxfeed->filter = dvbdmxfilter;

	return 0;
}

static int
dmx_sec_feed_set(struct dmx_sec_feed *feed,
		 u_int16_t pid, size_t circular_bufsize, 
		 int descramble, int check_crc)
{

	struct dvb_demux_feed *dvbdmxfeed = (struct dvb_demux_feed *)feed;
	struct dvb_demux *dvbdmx = dvbdmxfeed->demux;

	DBG("\n");

	if (pid > 0x1fff)
		return -EINVAL;

	if (dvb_demux_feed_add(dvbdmxfeed) < 0)
		return -EINVAL;

	dvbdmxfeed->pid = pid;
	dvbdmxfeed->bufsize = circular_bufsize;

	dvbdmxfeed->feed.sec.check_crc = check_crc;

	dvbdmxfeed->state = DMX_STATE_READY;

	return 0;
}

static void
prepare_secfilters(struct dvb_demux_feed *dvbdmxfeed)
{
	int i;
	struct dvb_demux_filter *f;
	struct dmx_sec_filter *sf;
	u_int8_t mask, mode, doneq;

	DBG("\n");

	if (!(f = dvbdmxfeed->filter))
		return;

	do {
		sf = &f->filter;
		doneq = 0;
		for (i = 0; i < DVB_DEMUX_MASK_MAX; i++)
		{
			mode = sf->filter_mode[i];
			mask = sf->filter_mask[i];
			f->maskandmode[i] = mask & mode;
			doneq |= f->maskandnotmode[i] = mask & ~mode;
		}
		f->doneq = doneq ? 1:0;
	} while ((f = f->next));
}

static int
dmx_sec_feed_start_filter(struct dmx_sec_feed *feed)
{
	struct dvb_demux_feed *dvbdmxfeed = (struct dvb_demux_feed *)feed;
	struct dvb_demux *dvbdmx = dvbdmxfeed->demux;
	int ret;

	DBG("\n");

	if (feed->is_filter)
	{
		return -EBUSY;
	}

	if (dvbdmxfeed->filter == NULL)
	{
		return -EINVAL;
	}

	dvbdmxfeed->feed.sec.tsfeedp = 0;
	dvbdmxfeed->feed.sec.secbuf = dvbdmxfeed->feed.sec.secbuf_base;
	dvbdmxfeed->feed.sec.secbufp = 0;
        dvbdmxfeed->feed.sec.seclen = 0;

	if (dvbdmx->start_feed == NULL)
	{
		return -ENODEV;
	}

	prepare_secfilters(dvbdmxfeed);

	if ((ret = dvbdmx->start_feed(dvbdmxfeed)) < 0)
	{
		return ret;
	}

	feed->is_filter = 1;
	dvbdmxfeed->state = DMX_STATE_GO;

	return 0;
}

static int
dmx_sec_feed_stop_filter(struct dmx_sec_feed *feed)
{
	struct dvb_demux_feed *dvbdmxfeed = (struct dvb_demux_feed *) feed;
	struct dvb_demux *dvbdmx = dvbdmxfeed->demux;
	int ret;
	
	DBG("\n");

	if (!dvbdmx->stop_feed)
	{
		return -ENODEV;
	}

	ret = dvbdmx->stop_feed(dvbdmxfeed);

	dvbdmxfeed->state = DMX_STATE_READY;
	feed->is_filter = 0;
	
	return ret;
}

static int
dmx_sec_feed_free_filter(struct dmx_sec_feed *feed, 
			 struct dmx_sec_filter *filter)
{

	struct dvb_demux_filter *dvbdmxfilter = (struct dvb_demux_filter *)filter, *f;
	struct dvb_demux_feed *dvbdmxfeed = (struct dvb_demux_feed *)feed;
	struct dvb_demux *dvbdmx = dvbdmxfeed->demux;

	DBG("\n");

	if (dvbdmxfilter->feed != dvbdmxfeed)
	{
		return -EINVAL;
	}

	if (feed->is_filter)
		feed->stop_filter(feed);

	f = dvbdmxfeed->filter;
	if (f == dvbdmxfilter)
	{
		dvbdmxfeed->filter = dvbdmxfilter->next;
	} else {
		while(f->next != dvbdmxfilter)
			f = f->next;
		f->next = f->next->next;
	}

	dvbdmxfilter->state = DMX_STATE_FREE;
	
	return 0;
}

static int
dvbdmx_alloc_sec_feed(struct dmx_demux *demux,
		      struct dmx_sec_feed **feed,
		      dmx_sec_cb callback)
{

	struct dvb_demux *dvbdmx = (struct dvb_demux *)demux;
	struct dvb_demux_feed *dvbdmxfeed;

	DBG("\n");

	dvbdmxfeed = dvb_dmx_feed_alloc(dvbdmx);
	if (dvbdmxfeed == NULL)
	{
		return -EBUSY;
	}

	dvbdmxfeed->cb.sec = callback;
	dvbdmxfeed->demux = dvbdmx;
	dvbdmxfeed->pid = 0xffff;
	dvbdmxfeed->feed.sec.secbuf = dvbdmxfeed->feed.sec.secbuf_base;
	dvbdmxfeed->feed.sec.secbufp = 0;
	dvbdmxfeed->feed.sec.seclen = 0;
	dvbdmxfeed->feed.sec.tsfeedp = 0;
	dvbdmxfeed->filter = 0;
	dvbdmxfeed->buf = 0;

	(*feed) = &dvbdmxfeed->feed.sec;

		/* dmx_sec_feed */

	(*feed)->is_filter = 0;
	(*feed)->parent = demux;
	(*feed)->priv = 0;
	
	(*feed)->set = dmx_sec_feed_set;
	(*feed)->alloc_filter = dmx_sec_feed_alloc_filter;
	(*feed)->start_filter = dmx_sec_feed_start_filter;
	(*feed)->stop_filter  = dmx_sec_feed_stop_filter;
	(*feed)->free_filter  = dmx_sec_feed_free_filter;

	return 0;
}

static int
dvbdmx_free_sec_feed(struct dmx_demux *demux,
		     struct dmx_sec_feed *feed)
{

	struct dvb_demux_feed *dvbdmxfeed = (struct dvb_demux_feed *)feed;
	struct dvb_demux *dvbdmx = (struct dvb_demux *)demux;

	DBG("\n");

	if (dvbdmxfeed->state == DMX_STATE_FREE)
	{
		return -EINVAL;
	}

	dvbdmxfeed->state = DMX_STATE_FREE;

	if (dvb_demux_feed_del(dvbdmxfeed) < 0)
		return -EINVAL;

	dvbdmxfeed->pid = 0xffff;

	return 0;
}

int
dvb_dmx_init(struct dvb_demux *dvbdemux)
{
	int i, err;
	struct dmx_demux *dmx = &dvbdemux->dmx;

	DBG("\n");

	dvbdemux->filter = malloc(dvbdemux->nfilter * 
			sizeof(struct dvb_demux_filter), M_DEVBUF, M_NOWAIT);
	if (dvbdemux->filter == NULL)
	{
		DBG("error alloc mem for dvbdemux->filter\n");
		return -ENOMEM;
	}

	dvbdemux->feed = malloc(dvbdemux->nfeed *
			sizeof(struct dvb_demux_feed), M_DEVBUF, M_NOWAIT);
	if (dvbdemux->feed == NULL)
	{
		free(dvbdemux->filter, M_DEVBUF);
		DBG("error alloc mem for dvbdemux->feed\n");
		return -ENOMEM;
	}

	for (i = 0; i < dvbdemux->nfilter; i++)
	{
		dvbdemux->filter[i].state = DMX_STATE_FREE;
		dvbdemux->filter[i].index = i;
	}

	for (i = 0; i < dvbdemux->nfeed; i++)
		dvbdemux->feed[i].state = DMX_STATE_FREE;

	LIST_INIT(&dvbdemux->dvb_demux_feed_head);

	if (dvbdemux->check_crc32 == NULL)
		dvbdemux->check_crc32 = dvb_dmx_crc32;

	if (dvbdemux->memcpy == NULL)
		dvbdemux->memcpy = dvb_dmx_memcpy;

		
	dmx->priv = (void *)dvbdemux;
	dmx->alloc_sec_feed = dvbdmx_alloc_sec_feed;
	dmx->free_sec_feed = dvbdmx_free_sec_feed;

	return 0;
}

int
dvb_dmx_release(struct dvb_demux *dvbdemux)
{
	struct dmx_demux *dmx = &dvbdemux->dmx;
	struct dvb_demux_feed *feed;

	DBG("\n");

	if (dvbdemux->filter)
		free(dvbdemux->filter, M_DEVBUF);

	if (dvbdemux->feed)
		free(dvbdemux->feed, M_DEVBUF);

	return 0;
}
