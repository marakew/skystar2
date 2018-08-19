#ifndef _dvbfrontend_h_
#define _dvbfrontend_h_

#ifndef _KERNEL
#include <sys/types.h>
#include <machine/endian.h>
#endif
#include <sys/ioccom.h>
#include <machine/clock.h>


/* frontend types */
typedef enum fe_type {
	FE_QPSK = 0,	/*	DVB-S	*/
} fe_type_t;

/* frontend caps        */
typedef enum fe_caps {
	FE_IS_STUPID                  = 0,
	FE_CAN_INVERSION_AUTO         = 0x1,
	FE_CAN_FEC_1_2                = 0x2,
	FE_CAN_FEC_2_3                = 0x4,
	FE_CAN_FEC_3_4                = 0x8,
	FE_CAN_FEC_4_5                = 0x10,
	FE_CAN_FEC_5_6                = 0x20,
	FE_CAN_FEC_6_7                = 0x40,
	FE_CAN_FEC_7_8                = 0x80,
	FE_CAN_FEC_8_9                = 0x100,
	FE_CAN_FEC_AUTO               = 0x200,
	FE_CAN_QPSK                   = 0x400,
	FE_CAN_QAM_16                 = 0x800,
	FE_CAN_QAM_32                 = 0x1000,
	FE_CAN_QAM_64                 = 0x2000,
	FE_CAN_QAM_128                = 0x4000,
	FE_CAN_QAM_256                = 0x8000,
	FE_CAN_QAM_AUTO               = 0x10000,
	FE_CAN_TRANSMISSION_MODE_AUTO = 0x20000,
	FE_CAN_BANDWIDTH_AUTO         = 0x40000,
	FE_CAN_GUARD_INTERVAL_AUTO    = 0x80000,
	FE_CAN_HIERARCHY_AUTO         = 0x100000,
	FE_CAN_MUTE_TS                = 0x80000000,
	FE_CAN_CLEAN_SETUP            = 0x40000000,
	FE_CAN_RECOVER                = 0x20000000
} fe_caps_t;

struct dvb_frontend_info {
	char       name[128];
	fe_type_t  type;
	u_int32_t  frequency_min;
	u_int32_t  frequency_max;
	u_int32_t  frequency_stepsize;
	u_int32_t  frequency_tolerance;
	u_int32_t  symbol_rate_min;
	u_int32_t  symbol_rate_max;
	u_int32_t  symbol_rate_tolerance;
	u_int32_t  notifier_delay;
	fe_caps_t  caps;
};

/**
 *  Check out the DiSEqC bus spec available on http://www.eutelsat.org/ for
 *  the meaning of this struct...
 */
struct dvb_diseqc_master_cmd {
        uint8_t msg [6];        /*  { framing, address, command, data [3] } */
        uint8_t msg_len;        /*  valid values are 3...6  */
};

struct dvb_diseqc_slave_reply {
        uint8_t msg [4];        /*  { framing, data [3] } */
        uint8_t msg_len;        /*  valid values are 0...4, 0 means no msg  */
        int     timeout;        /*  return from ioctl after timeout ms with */
};                              /*  errorcode when no message was received  */

/* LNB voltage control  */
typedef enum fe_sec_voltage {
        SEC_VOLTAGE_13 = 0,
        SEC_VOLTAGE_18,
        SEC_VOLTAGE_OFF
} fe_sec_voltage_t;

/* LNB 22k control	*/
typedef enum fe_sec_tone_mode {
        SEC_TONE_ON = 0,
        SEC_TONE_OFF
} fe_sec_tone_mode_t;

typedef enum fe_sec_mini_cmd {
        SEC_MINI_A = 0,
        SEC_MINI_B
} fe_sec_mini_cmd_t;

typedef enum fe_status {
        FE_HAS_SIGNAL     = 0x01,   /*  found something above the noise level */
        FE_HAS_CARRIER    = 0x02,   /*  found a DVB signal  */
        FE_HAS_VITERBI    = 0x04,   /*  FEC is stable  */
        FE_HAS_SYNC       = 0x08,   /*  found sync bytes  */
        FE_HAS_LOCK       = 0x10,   /*  everything's working... */
        FE_TIMEDOUT       = 0x20,   /*  no lock within the last ~2 seconds */
        FE_REINIT         = 0x40    /*  frontend was reinitialized,  */
} fe_status_t;                      /*  application is recommended to reset */
                                    /*  DiSEqC, tone and parameters */

/* spectral inversion control */
typedef enum fe_spectral_inversion {
        INVERSION_OFF = 0,
        INVERSION_ON,
        INVERSION_AUTO
} fe_spectral_inversion_t;

typedef enum fe_code_rate {
        FEC_NONE = 0,
        FEC_1_2,
        FEC_2_3,
        FEC_3_4,
        FEC_4_5,	//
        FEC_5_6,
        FEC_6_7,
        FEC_7_8,
        FEC_8_9,	//
        FEC_AUTO
} fe_code_rate_t;

/* DVB-S frontend */
struct dvb_qpsk_parameters {
        uint32_t        symbol_rate;  /* symbol rate in Symbols per second */
        fe_code_rate_t  fec_inner;    /* forward error correction (see above) */
};

/* for ioctl access */
struct dvb_frontend_parameters {
        uint32_t frequency;       /* (absolute) frequency in Hz for QAM/OFDM */
                                  /* intermediate frequency in kHz for QPSK */
        fe_spectral_inversion_t inversion;
        union {
                struct dvb_qpsk_parameters qpsk;
        } u;
};

#define FE_GET_INFO		   _IOR('o', 61, struct dvb_frontend_info)
#define FE_DISEQC_SEND_MASTER_CMD  _IOW('o', 63, struct dvb_diseqc_master_cmd)
#define FE_DISEQC_SEND_BURST       _IO('o', 65)  /* fe_sec_mini_cmd_t */

#define FE_SET_TONE                _IO('o', 66) /* , fe_sec_tone_mode_t)*/
#define FE_SET_VOLTAGE             _IO('o', 67) /*, fe_sec_voltage_t) */

#define FE_READ_STATUS             _IOR('o', 69, fe_status_t)
#define FE_READ_SIGNAL_STRENGTH    _IOR('o', 71, u_int32_t)
#define FE_READ_SNR                _IOR('o', 72, u_int32_t)

#define FE_SET_FRONTEND            _IOW('o', 76, struct dvb_frontend_parameters)
#define FE_GET_FRONTEND            _IOR('o', 77, struct dvb_frontend_parameters)

#define FE_SLEEP		   _IO('v', 80)
#define FE_INIT                    _IO('v', 81)
#define FE_RESET                   _IO('v', 82)

#define DVB_GET_MAC		   _IOC(IOC_OUT, 's', 90, 8)
#define DVB_SET_MAC		   _IOW('s', 91, u_int8_t *)

#endif
