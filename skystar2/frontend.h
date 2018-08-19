/*
 *      SkyStar2 driver based on chip FlexCopII
 */

#ifndef _frontend_h_
#define _frontend_h_

#define FESTATE_IDLE		1
#define FESTATE_RETUNE		2
#define FESTATE_TUNING_FAST	4
#define FESTATE_TUNING_SLOW	8
#define FESTATE_TUNED		16
#define FESTATE_ZIGZAG_FAST	32
#define FESTATE_ZIGZAG_SLOW	64
#define FESTATE_CLEAN_SETUP	128
//#define FESTATE_DISEQC		128
#define FESTATE_SEARCHING	(FESTATE_TUNING_FAST | FESTATE_TUNING_SLOW | FESTATE_ZIGZAG_FAST | FESTATE_ZIGZAG_SLOW)
//#define FESTATE_SEARCHING	(FESTATE_TUNING_FAST | FESTATE_TUNING_SLOW | FESTATE_ZIGZAG_FAST | FESTATE_ZIGZAG_SLOW | FESTATE_DISEQC)
#define FESTATE_SEARCHING_FAST	(FESTATE_TUNING_FAST | FESTATE_ZIGZAG_FAST)
#define FESTATE_SEARCHING_SLOW	(FESTATE_TUNING_SLOW | FESTATE_ZIGZAG_SLOW)
#define FESTATE_LOSTLOCK	(FESTATE_ZIGZAG_FAST | FESTATE_ZIGZAG_SLOW)

/*
* FESTATE_IDLE No tuning parameters have been supplied and the loop is idling.
* FESTATE_RETUNE Parameters have been supplied, but we have not yet performed the first tune
* FESTATE_TUNING_FAST Tuning parameters have been supplied and fast zigzag scan is in progress.
* FESTATE_TUNING_SLOW Tuning parameters have been supplied. Fast zigzag failed , so we're trying again, but slower.
* FESTATE_TUNED The frontend has successfully locked on.
* FESTATE_ZIGZAG_FAST The lock has been lost, and a fast zigzag has been initiated to try and regain it.
* FESTATE_ZIGZAG_SLOW The lock has been lost. Fast zigzag has been failed, so we're trying again, but slower.
* FESTATE_CLEAN_SETUP Used for certain dodgy tuners which need special massaging to lock.
* FESTATE_SEARCHING When we're searching for a signal using a zigzag scan of any sort.
* FESTATE_SEARCHING_FAST When we're searching for a signal using a fast zigzag scan.
* FESTATE_SEARCHING_SLOW When we're searching for a signal using a slow zigzag scan.
* FESTATE_LOSTLOCK When the lock has been lost, and we're searching it again.
*/

struct dvb_frontend {
	struct adapter *sc;

	int (* ioctl)(struct dvb_frontend *fe, u_long cmd, void *arg);

	int (* detach)(struct adapter *sc);
	void	*data;
};

struct dvb_frontend_chip {
        int     (*attach)(struct adapter *sc);
};

struct fe_data {
        struct dvb_frontend_info *info;
	struct dvb_frontend frontend;
        struct dvb_frontend_parameters parameters;

	int id;

        int state;
        int bending;
        int lnb_drift;
        int inversion;
        int auto_step;
        int started_auto_step;
        int min_delay;
        int exit;
        fe_status_t status;
};


extern int fe_start(struct adapter *sc);
extern void fe_stop(struct adapter *sc);
extern int fe_ioctl(struct fe_data *fe, u_long cmd, caddr_t arg);
#endif
