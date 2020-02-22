#include	"compiler.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"cbuscore.h"
#include	"boardso.h"
#include	"sound.h"
#include	"ct1741io.h"
#include	"ct1745io.h"
#include	"fmboard.h"
//#include	"s98.h"

#ifdef SUPPORT_SOUND_SB16

/**
 * Creative Sound Blaster 16(98)
 * YMF262-M(OPL3) + CT1741(PCM) + CT1745(MIXER) + YM2203(OPN - option)
 *
 * �f�t�H���g�d�l IO:D2 DMA:3 INT:5 
 */

static void *opl3 = NULL;
static const UINT8 sb16base[] = {0xd2,0xd4,0xd6,0xd8,0xda,0xdc,0xde};
static int samplerate;

#ifdef USE_MAME
void *YMF262Init(INT clock, INT rate);
void YMF262ResetChip(void *chip);
void YMF262Shutdown(void *chip);
INT YMF262Write(void *chip, INT a, INT v);
UINT8 YMF262Read(void *chip, INT a);
void YMF262UpdateOne(void *chip, INT16 **buffer, INT length);
#endif

static void IOOUTCALL sb16_o0400(UINT port, REG8 dat) {
	port = dat;
}
static void IOOUTCALL sb16_o0500(UINT port, REG8 dat) {
	port = dat;
}
static void IOOUTCALL sb16_o0600(UINT port, REG8 dat) {
	port = dat;
}
static void IOOUTCALL sb16_o0700(UINT port, REG8 dat) {
	port = dat;
}
static void IOOUTCALL sb16_o8000(UINT port, REG8 dat) {
	/* MIDI Data Port */
//	TRACEOUT(("SB16-midi commands: %.2x", dat));
	port = dat;
}
static void IOOUTCALL sb16_o8100(UINT port, REG8 dat) {
	/* MIDI Stat Port */
//	TRACEOUT(("SB16-midi status: %.2x", dat));
	port = dat;
}

static void IOOUTCALL sb16_o2000(UINT port, REG8 dat) {
	(void)port;
	g_opl.addr = dat;
#ifdef USE_MAME
	YMF262Write(opl3, 0, dat);
	g_opl3.s.addrl = dat; // Key Display�p
#endif
}

static void IOOUTCALL sb16_o2100(UINT port, REG8 dat) {
	(void)port;
	g_opl.reg[g_opl.addr] = dat;
	//S98_put(NORMAL2608, g_opl.addr, dat);
#ifdef USE_MAME
	YMF262Write(opl3, 1, dat);
	opl3_writeRegister(&g_opl3, g_opl3.s.addrl, dat); // Key Display�p
#endif
}
static void IOOUTCALL sb16_o2200(UINT port, REG8 dat) {
	(void)port;
	g_opl.addr2 = dat;
#ifdef USE_MAME
	YMF262Write(opl3, 2, dat);
	g_opl3.s.addrh = dat; // Key Display�p
#endif
}

static void IOOUTCALL sb16_o2300(UINT port, REG8 dat) {
	(void)port;
	g_opl.reg[g_opl.addr2 + 0x100] = dat;
	//S98_put(EXTEND2608, opl.addr2, dat);
#ifdef USE_MAME
	YMF262Write(opl3, 3, dat);
	opl3_writeExtendedRegister(&g_opl3, g_opl3.s.addrh, dat); // Key Display�p
#endif
}

static void IOOUTCALL sb16_o2800(UINT port, REG8 dat) {
	/**
	 * ������PC/AT�Ō����Ƃ����Adlib�݊��|�[�g
	 * UltimaUnderWorld�ł͂������@��
	 */
	port = dat;
#ifdef USE_MAME
	YMF262Write(opl3, 0, dat);
#endif
}
static void IOOUTCALL sb16_o2900(UINT port, REG8 dat) {
	port = dat;
#ifdef USE_MAME
	YMF262Write(opl3, 1, dat);
#endif
}

static REG8 IOINPCALL sb16_i0400(UINT port) {
	return 0xff;
}
static REG8 IOINPCALL sb16_i0500(UINT port) {
	return 0;
}
static REG8 IOINPCALL sb16_i0600(UINT port) {
	return 0;
}
static REG8 IOINPCALL sb16_i0700(UINT port) {
	return 0;
}
static REG8 IOINPCALL sb16_i8000(UINT port) {
	/* Midi Port */
	return 0;
}
static REG8 IOINPCALL sb16_i8100(UINT port) {
	/* Midi Port */
	return 0;
}


static REG8 IOINPCALL sb16_i2000(UINT port) {
	(void)port;
#ifdef USE_MAME
	return YMF262Read(opl3, 0);
#else
	return 0;
#endif
}

static REG8 IOINPCALL sb16_i2200(UINT port) {
	(void)port;
#ifdef USE_MAME
	return YMF262Read(opl3, 1);
#else
	return 0;
#endif
}

static REG8 IOINPCALL sb16_i2800(UINT port) {
	(void)port;
#ifdef USE_MAME
	return YMF262Read(opl3, 0);
#else
	return 0;
#endif
}

// ----

void SOUNDCALL opl3gen_getpcm(void* opl3, SINT32 *pcm, UINT count) {
	UINT i;
	INT16 *buf[4];
	INT16 s1l,s1r,s2l,s2r;
	SINT32 *outbuf = pcm;
	SINT32 oplfm_volume;
	oplfm_volume = np2cfg.vol_fm;
	buf[0] = &s1l;
	buf[1] = &s1r;
	buf[2] = &s2l;
	buf[3] = &s2r;
	for (i=0; i < count; i++) {
		s1l = s1r = s2l = s2r = 0;
#ifdef USE_MAME
		YMF262UpdateOne(opl3, buf, 1);
#endif
		outbuf[0] += (SINT32)(((s1l << 1) * oplfm_volume) >> 5);
		outbuf[1] += (SINT32)(((s1r << 1) * oplfm_volume) >> 5);
		outbuf += 2;
	}
}

void boardsb16_reset(const NP2CFG *pConfig) {
	if (opl3) {
		if (samplerate != pConfig->samplingrate) {
#ifdef USE_MAME
			YMF262Shutdown(opl3);
			opl3 = YMF262Init(14400000, pConfig->samplingrate);
#endif
			samplerate = pConfig->samplingrate;
		} else {
#ifdef USE_MAME
			YMF262ResetChip(opl3);
#endif
		}
	}
	ZeroMemory(&g_sb16, sizeof(g_sb16));
	ZeroMemory(&g_opl, sizeof(g_opl));
	// �{�[�h�f�t�H���g IO:D2 DMA:3 IRQ:5(INT1) 
	g_sb16.base = np2cfg.sndsb16io; //0xd2;
	g_sb16.dmach = np2cfg.sndsb16dma; //0x3;
	g_sb16.dmairq = np2cfg.sndsb16irq; //0x5;
	ct1745io_reset();
	ct1741io_reset();
}

void boardsb16_bind(void) {
	opl3_reset(&g_opl3, OPL3_HAS_OPL3L|OPL3_HAS_OPL3);

	ct1745io_bind();
	ct1741io_bind();

	iocore_attachout(0x2000 + g_sb16.base, sb16_o2000);	/* FM Music Register Address Port */
	iocore_attachout(0x2100 + g_sb16.base, sb16_o2100);	/* FM Music Data Port */
	iocore_attachout(0x2200 + g_sb16.base, sb16_o2200);	/* Advanced FM Music Register Address Port */
	iocore_attachout(0x2300 + g_sb16.base, sb16_o2300);	/* Advanced FM Music Data Port */
	iocore_attachout(0x2800 + g_sb16.base, sb16_o2800);	/* FM Music Register Port */
	iocore_attachout(0x2900 + g_sb16.base, sb16_o2900);	/* FM Music Data Port */

	iocore_attachinp(0x2000 + g_sb16.base, sb16_i2000);	/* FM Music Status Port */
	iocore_attachinp(0x2200 + g_sb16.base, sb16_i2200);	/* Advanced FM Music Status Port */
	iocore_attachinp(0x2800 + g_sb16.base, sb16_i2800);	/* FM Music Status Port */

	iocore_attachout(0x0400 + g_sb16.base, sb16_o0400);	/* GAME Port */
	iocore_attachout(0x0500 + g_sb16.base, sb16_o0500);	/* GAME Port */
	iocore_attachout(0x0600 + g_sb16.base, sb16_o0600);	/* GAME Port */
	iocore_attachout(0x0700 + g_sb16.base, sb16_o0700);	/* GAME Port */
	iocore_attachinp(0x0400 + g_sb16.base, sb16_i0400);	/* GAME Port */
	iocore_attachinp(0x0500 + g_sb16.base, sb16_i0500);	/* GAME Port */
	iocore_attachinp(0x0600 + g_sb16.base, sb16_i0600);	/* GAME Port */
	iocore_attachinp(0x0700 + g_sb16.base, sb16_i0700);	/* GAME Port */

	iocore_attachout(0x8000 + g_sb16.base, sb16_o8000);	/* MIDI Port */
	iocore_attachout(0x8100 + g_sb16.base, sb16_o8100);	/* MIDI Port */
	iocore_attachinp(0x8000 + g_sb16.base, sb16_i8000);	/* MIDI Port */
	iocore_attachinp(0x8100 + g_sb16.base, sb16_i8100);	/* MIDI Port */

	if (!opl3) {
#ifdef USE_MAME
		opl3 = YMF262Init(14400000, np2cfg.samplingrate);
#endif
		samplerate = np2cfg.samplingrate;
	}
	sound_streamregist(opl3, (SOUNDCB)opl3gen_getpcm);
	opl3_bind(&g_opl3); // MAME�g�p�̏ꍇKey Display�p
}
void boardsb16_unbind(void) {
	ct1745io_unbind();
	ct1741io_unbind();

	iocore_detachout(0x2000 + g_sb16.base);	/* FM Music Register Address Port */
	iocore_detachout(0x2100 + g_sb16.base);	/* FM Music Data Port */
	iocore_detachout(0x2200 + g_sb16.base);	/* Advanced FM Music Register Address Port */
	iocore_detachout(0x2300 + g_sb16.base);	/* Advanced FM Music Data Port */
	iocore_detachout(0x2800 + g_sb16.base);	/* FM Music Register Port */
	iocore_detachout(0x2900 + g_sb16.base);	/* FM Music Data Port */

	iocore_detachinp(0x2000 + g_sb16.base);	/* FM Music Status Port */
	iocore_detachinp(0x2200 + g_sb16.base);	/* Advanced FM Music Status Port */
	iocore_detachinp(0x2800 + g_sb16.base);	/* FM Music Status Port */

	iocore_detachout(0x0400 + g_sb16.base);	/* GAME Port */
	iocore_detachout(0x0500 + g_sb16.base);	/* GAME Port */
	iocore_detachout(0x0600 + g_sb16.base);	/* GAME Port */
	iocore_detachout(0x0700 + g_sb16.base);	/* GAME Port */
	iocore_detachinp(0x0400 + g_sb16.base);	/* GAME Port */
	iocore_detachinp(0x0500 + g_sb16.base);	/* GAME Port */
	iocore_detachinp(0x0600 + g_sb16.base);	/* GAME Port */
	iocore_detachinp(0x0700 + g_sb16.base);	/* GAME Port */

	iocore_detachout(0x8000 + g_sb16.base);	/* MIDI Port */
	iocore_detachout(0x8100 + g_sb16.base);	/* MIDI Port */
	iocore_detachinp(0x8000 + g_sb16.base);	/* MIDI Port */
	iocore_detachinp(0x8100 + g_sb16.base);	/* MIDI Port */
}
void boardsb16_finalize(void)
{
#ifdef USE_MAME
	if (opl3) {
		YMF262Shutdown(opl3);
		opl3 = NULL;
	}
#endif
}

#endif