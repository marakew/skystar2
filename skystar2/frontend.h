/*
 *      SkyStar2 driver based on chip FlexCopII
 */

#ifndef _frontend_h_
#define _frontend_h_

struct dvb_frontend {
	struct adapter *sc;

	int (* ioctl)(struct dvb_frontend *fe, u_long cmd, void *arg);

	int (* detach)(struct adapter *sc);
	void	*data;
};

struct fe_data {
        struct dvb_frontend_info *info;
	struct dvb_frontend frontend;
        struct dvb_frontend_parameters parameters;

//	int id;

//        int state;
//        int bending;
//        int lnb_drift;
//        int inversion;
//        int auto_step;
//        int started_auto_step;
//        int min_delay;
        int exit;
//        fe_status_t status;
};


extern int fe_start(struct adapter *sc);
extern void fe_stop(struct adapter *sc);
extern int fe_ioctl(struct fe_data *fe, u_long cmd, caddr_t arg);
#endif
