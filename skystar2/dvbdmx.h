/*
 *      SkyStar2 driver based on chip FlexCopII
 */

#ifndef _dvb_dmx_h_
#define _dvb_dmx_h_

#include <sys/types.h>
#include <sys/queue.h>

#ifndef DMX_MAX_FILTER_SIZE
#define DMX_MAX_FILTER_SIZE 18
#endif

#ifndef DMX_MAX_SECFEED_SIZE
#define DMX_MAX_SECFEED_SIZE 4096
#endif


typedef enum dmx_success {
	DMX_OK = 0,
	DMX_LENGTH_ERR,
	DMX_OVERRUN_ERR,
	DMX_CRC_ERR,
	DMX_FRAME_ERR,
	DMX_FIFO_ERR,
	DMX_MISSED_ERR
} dmx_success_t;

#define	TS_PACKET	1
#define TS_PAYLOAD_ONLY	2
#define	TS_DECODER	4


	/* -------------- Sec Feed -------------- */

#define DVB_DEMUX_MASK_MAX 18

struct dmx_sec_filter {
	u_int8_t	filter_val  [DMX_MAX_FILTER_SIZE];
	u_int8_t	filter_mask [DMX_MAX_FILTER_SIZE];
	u_int8_t	filter_mode [DMX_MAX_FILTER_SIZE];
	struct dmx_sec_feed *parent;
	void		*priv;
};

struct dmx_sec_feed {
	int		is_filter;
	struct dmx_demux *parent;
	void		*priv;

	int		check_crc;
	u_int32_t	crc_val;
	
	u_int8_t	*secbuf;
#if 0
	u_int8_t	secbuf_base[DMX_MAX_SECFEED_SIZE];  /* tcp/ip */
#else
	u_int8_t	secbuf_base[16384];  /* tcp/ip */
#endif
	u_int16_t	secbufp;
	u_int16_t	seclen;
	u_int16_t	tsfeedp;

	int		(* set)(struct dmx_sec_feed *feed,
				u_int16_t pid,
				size_t circular_bufsize,
				int descramble,
				int check_crc);
	int	(* alloc_filter)(struct dmx_sec_feed *feed,
				struct dmx_sec_filter **filter);
	int	(* free_filter)(struct dmx_sec_feed *feed,
				struct dmx_sec_filter *filter);	
	int	(* start_filter)(struct dmx_sec_feed *feed);
	int	(* stop_filter)(struct dmx_sec_feed *feed);
};

	/* -------------- Function CallBack for Sec -------------- */

typedef	int	(*dmx_sec_cb)	(const u_int8_t *buf1, size_t buf1_len,
				 const u_int8_t *buf2, size_t buf2_len,
				 struct dmx_sec_filter *src,
				 dmx_success_t success);


struct dmx_demux {
	void		*priv;

	int	(* alloc_sec_feed)(struct dmx_demux *demux,
				   struct dmx_sec_feed **feed,
				   dmx_sec_cb callback);

	int	(* free_sec_feed )(struct dmx_demux *demux,
				   struct dmx_sec_feed *feed);
};


	/* -------------- Demux Filter -------------- */

struct dvb_demux_filter {
	struct dmx_sec_filter	filter;
	u_int8_t		maskandmode    [DMX_MAX_FILTER_SIZE];
	u_int8_t		maskandnotmode [DMX_MAX_FILTER_SIZE];
	int			doneq;
	
	struct dvb_demux_filter	*next;
	struct dvb_demux_feed	*feed;
	int			index;
	int			state;

#define DMX_STATE_FREE		0
#define DMX_STATE_ALLOC		1
#define DMX_STATE_SET		2
#define DMX_STATE_READY		3
#define DMX_STATE_GO		4

	int			type;
	int			pesto;

	u_int16_t		handle;
	u_int16_t		hw_handle;
	int			ts_state;
};

	/* -------------- Demux Feed -------------- */

struct dvb_demux_feed {

	union {
		struct dmx_sec_feed	sec;
	} feed;

	union {
		dmx_sec_cb	sec;
	} cb;

	struct dvb_demux	*demux;
	void			*priv;

	int			type;
	int			state;
	u_int16_t		pid;
	u_int8_t		*buf;
	int			bufsize;
	int			descramble;

	struct dvb_demux_filter	*filter;
	int			cb_length;

	int			cc;
	int pusi_seen;			/* prevents feeding of garbage from previous section */

	LIST_ENTRY(dvb_demux_feed)	dvb_demux_feed_next;
};


struct dvb_demux {
	struct	dmx_demux	dmx;
	void			*priv;

	int			(*start_feed) (struct dvb_demux_feed *feed);
	int			(*stop_feed) (struct dvb_demux_feed *feed);
	u_int32_t		(*check_crc32) (struct dvb_demux_feed *feed,
					       const u_int8_t *buf, size_t len);
	void			(*memcpy) (struct dvb_demux_feed *feed,
				u_int8_t *dst, const u_int8_t *src, size_t len);

	int			nfilter;
	int			nfeed;
	struct dvb_demux_filter	*filter;
	struct dvb_demux_feed	*feed;

#define DMX_MAX_PID 0x2000	
	LIST_HEAD(, dvb_demux_feed) dvb_demux_feed_head;

};

extern int
dvb_dmx_init(struct dvb_demux *dvbdemux);

extern int
dvb_dmx_release(struct dvb_demux *dvbdemux);

extern void
dvb_dmx_swfilter_packets(struct dvb_demux *demux, const u_int8_t *buf, size_t count);

#endif
