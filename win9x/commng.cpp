#include	"compiler.h"
#include	"np2.h"
#include	"commng.h"


// ---- non connect

static UINT ncread(COMMNG self, BYTE *data) {

	(void)self;
	(void)data;
	return(0);
}

static UINT ncwrite(COMMNG self, BYTE data) {

	(void)self;
	(void)data;
	return(0);
}

static BYTE ncgetstat(COMMNG self) {

	(void)self;
	return(0xf0);
}

static long ncmsg(COMMNG self, UINT msg, long param) {

	(void)self;
	(void)msg;
	(void)param;
	return(0);
}

static void ncrelease(COMMNG self) {
}

static const _COMMNG com_nc = {
		COMCONNECT_OFF, ncread, ncwrite, ncgetstat, ncmsg, ncrelease};


// ----


void commng_initialize(void) {

	cmmidi_initailize();
}

COMMNG commng_create(UINT device) {

	COMMNG	ret;
	COMCFG	*cfg;

	ret = NULL;
	switch(device) {
		case COMCREATE_SERIAL:
			cfg = &np2oscfg.com1;
			break;

		case COMCREATE_PC9861K1:
			cfg = &np2oscfg.com2;
			break;

		case COMCREATE_PC9861K2:
			cfg = &np2oscfg.com3;
			break;

		case COMCREATE_PRINTER:
			cfg = NULL;
			break;

		case COMCREATE_MPU98II:
			cfg = &np2oscfg.mpu;
			break;

		default:
			cfg = NULL;
			break;
	}
	if (cfg) {
		if ((cfg->port >= COMPORT_COM1) &&
			(cfg->port <= COMPORT_COM4)) {
			ret = cmserial_create(cfg->port - COMPORT_COM1 + 1,
												cfg->param, cfg->speed);
		}
		else if (cfg->port == COMPORT_MIDI) {
			ret = cmmidi_create(cfg->mout, cfg->min, cfg->mdl);
			if (ret) {
				ret->msg(ret, COMMSG_MIMPIDEFFILE, (long)cfg->def);
				ret->msg(ret, COMMSG_MIMPIDEFEN, (long)cfg->def_en);
			}
		}
	}
	if (ret) {
		return(ret);
	}
	return((COMMNG)&com_nc);
}

void commng_destroy(COMMNG hdl) {

	if (hdl) {
		hdl->release(hdl);
	}
}

