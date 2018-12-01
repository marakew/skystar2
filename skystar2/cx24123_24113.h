

#ifndef _cx24123_24113_h_
#define _cx24123_24113_h_

struct dvbtuner
{
	struct adapter	*sc;

	struct _I2CBUS	demod;
	struct _I2CBUS	lnb;

	unsigned char	lnbval;
	unsigned char	lnbtone;

	unsigned int	freq;
	unsigned int	coderate;
	unsigned int	symbrate;

	unsigned int	esno;

	ACQSTATE	acqstate;
	LOCKIND		lockind;

	NIM		nim;
};

extern int cx24123_attach(struct adapter *);
#endif

