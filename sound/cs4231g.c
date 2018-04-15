/**
 * @file	cs4231g.c
 * @brief	Implementation of the CS4231
 */

#include "compiler.h"
#include "cs4231.h"
#include "iocore.h"

extern	CS4231CFG	cs4231cfg;

static void SOUNDCALL pcm8m(CS4231 cs, SINT32 *pcm, UINT count) {

	UINT32	leng;
	UINT32	pos12;
	SINT32	fract;
	UINT32	samppos;
const UINT8	*ptr1;
const UINT8	*ptr2;
	SINT32	samp1;
	SINT32	samp2;

	leng = cs->bufdatas >> 0;
	if (!leng) {
		return;
	}
	pos12 = cs->pos12;
	do {
		samppos = pos12 >> 12;
		if (leng <= samppos) {
			break;
		}
		fract = pos12 & ((1 << 12) - 1);
		ptr1 = cs->buffer +
						((cs->bufpos + (samppos << 0) + 0) & CS4231_BUFMASK);
		ptr2 = cs->buffer +
						((cs->bufpos + (samppos << 0) + 1) & CS4231_BUFMASK);
		samp1 = (ptr1[0] - 0x80) << 8;
		samp2 = (ptr2[0] - 0x80) << 8;
		samp1 += ((samp2 - samp1) * fract) >> 12;
		pcm[0] += samp1;
		pcm[1] += samp1;
		pcm += 2;
		pos12 += cs->step12;
	} while(--count);

	leng = min(leng, (pos12 >> 12));
	cs->bufdatas -= (leng << 0);
	cs->bufpos = (cs->bufpos + (leng << 0)) & CS4231_BUFMASK;
	cs->pos12 = pos12 & ((1 << 12) - 1);
}

static void SOUNDCALL pcm8s(CS4231 cs, SINT32 *pcm, UINT count) {

	UINT32	leng;
	UINT32	pos12;
	SINT32	fract;
	UINT32	samppos;
const UINT8	*ptr1;
const UINT8	*ptr2;
	SINT32	samp1;
	SINT32	samp2;

	leng = cs->bufdatas >> 1;
	if (!leng) {
		return;
	}
	pos12 = cs->pos12;
	do {
		samppos = pos12 >> 12;
		if (leng <= samppos) {
			break;
		}
		fract = pos12 & ((1 << 12) - 1);
		ptr1 = cs->buffer +
						((cs->bufpos + (samppos << 1) + 0) & CS4231_BUFMASK);
		ptr2 = cs->buffer +
						((cs->bufpos + (samppos << 1) + 2) & CS4231_BUFMASK);
		samp1 = (ptr1[0] - 0x80) << 8;
		samp2 = (ptr2[0] - 0x80) << 8;
		samp1 += ((samp2 - samp1) * fract) >> 12;
		pcm[0] += samp1;
		samp1 = (ptr1[1] - 0x80) << 8;
		samp2 = (ptr2[1] - 0x80) << 8;
		samp1 += ((samp2 - samp1) * fract) >> 12;
		pcm[1] += samp1;
		pcm += 2;
		pos12 += cs->step12;
	} while(--count);

	leng = min(leng, (pos12 >> 12));
	cs->bufdatas -= (leng << 1);
	cs->bufpos = (cs->bufpos + (leng << 1)) & CS4231_BUFMASK;
	cs->pos12 = pos12 & ((1 << 12) - 1);
}

static void SOUNDCALL pcm16m(CS4231 cs, SINT32 *pcm, UINT count) {

	UINT32	leng;
	UINT32	pos12;
	SINT32	fract;
	UINT32	samppos;
const UINT8	*ptr1;
const UINT8	*ptr2;
	SINT32	samp1;
	SINT32	samp2;

	leng = cs->bufdatas >> 1;
	if (!leng) {
		return;
	}
	pos12 = cs->pos12;
	do {
		samppos = pos12 >> 12;
		if (leng <= samppos) {
			break;
		}
		fract = pos12 & ((1 << 12) - 1);
		ptr1 = cs->buffer +
						((cs->bufpos + (samppos << 1) + 0) & CS4231_BUFMASK);
		ptr2 = cs->buffer +
						((cs->bufpos + (samppos << 1) + 2) & CS4231_BUFMASK);
		samp1 = (((SINT8)ptr1[0]) << 8) + ptr1[1];
		samp2 = (((SINT8)ptr2[0]) << 8) + ptr2[1];
		samp1 += ((samp2 - samp1) * fract) >> 12;
		pcm[0] += samp1;
		pcm[1] += samp1;
		pcm += 2;
		pos12 += cs->step12;
	} while(--count);

	leng = min(leng, (pos12 >> 12));
	cs->bufdatas -= (leng << 1);
	cs->bufpos = (cs->bufpos + (leng << 1)) & CS4231_BUFMASK;
	cs->pos12 = pos12 & ((1 << 12) - 1);
}

static void SOUNDCALL pcm16s(CS4231 cs, SINT32 *pcm, UINT count) {

	UINT32	leng;
	UINT32	pos12;
	SINT32	fract;
	UINT32	samppos;
const UINT8	*ptr1;
const UINT8	*ptr2;
	SINT32	samp1;
	SINT32	samp2;
	
	leng = cs->bufdatas >> 2;
	if (!leng) {
		return;
	}
	pos12 = cs->pos12;
	do {
		samppos = pos12 >> 12;
		if (leng <= samppos) {
			break;
		}
		fract = pos12 & ((1 << 12) - 1);
		ptr1 = cs->buffer +
						((cs->bufpos + (samppos << 2) + 0) & CS4231_BUFMASK);
		ptr2 = cs->buffer +
						((cs->bufpos + (samppos << 2) + 4) & CS4231_BUFMASK);
		samp1 = (((SINT8)ptr1[0]) << 8) + ptr1[1];
		samp2 = (((SINT8)ptr2[0]) << 8) + ptr2[1];
		samp1 += ((samp2 - samp1) * fract) >> 12;
		pcm[0] += samp1;
		samp1 = (((SINT8)ptr1[2]) << 8) + ptr1[3];
		samp2 = (((SINT8)ptr2[2]) << 8) + ptr2[3];
		samp1 += ((samp2 - samp1) * fract) >> 12;
		pcm[1] += samp1;
		pcm += 2;
		pos12 += cs->step12;
	} while(--count);
	
	leng = min(leng, (pos12 >> 12));
	cs->bufdatas -= (leng << 2);
	cs->bufpos = (cs->bufpos + (leng << 2)) & CS4231_BUFMASK;
	cs->pos12 = pos12 & ((1 << 12) - 1);
}

static void SOUNDCALL pcm16m_ex(CS4231 cs, SINT32 *pcm, UINT count) {

	UINT32	leng;
	UINT32	pos12;
	SINT32	fract;
	UINT32	samppos;
const UINT8	*ptr1;
const UINT8	*ptr2;
	SINT32	samp1;
	SINT32	samp2;
	
	leng = cs->bufdatas >> 1;
	if (!leng) {
		return;
	}
	pos12 = cs->pos12;
	do {
		samppos = pos12 >> 12;
		if (leng <= samppos) {
			break;
		}
		fract = pos12 & ((1 << 12) - 1);
		ptr1 = cs->buffer +
						((cs->bufpos + (samppos << 1) + 0) & CS4231_BUFMASK);
		ptr2 = cs->buffer +
						((cs->bufpos + (samppos << 1) + 2) & CS4231_BUFMASK);
		samp1 = (((SINT8)ptr1[1]) << 8) + ptr1[0];
		samp2 = (((SINT8)ptr2[1]) << 8) + ptr2[0];
		samp1 += ((samp2 - samp1) * fract) >> 12;
		pcm[0] += samp1;
		pcm[1] += samp1;
		pcm += 2;
		pos12 += cs->step12;
	} while(--count);
	
	leng = min(leng, (pos12 >> 12));
	cs->bufdatas -= (leng << 1);
	cs->bufpos = (cs->bufpos + (leng << 1)) & CS4231_BUFMASK;
	cs->pos12 = pos12 & ((1 << 12) - 1);
}

static void SOUNDCALL pcm16s_ex(CS4231 cs, SINT32 *pcm, UINT count) {

	UINT32	leng;
	UINT32	pos12;
	SINT32	fract;
	UINT32	samppos;
const UINT8	*ptr1;
const UINT8	*ptr2;
	SINT32	samp1;
	SINT32	samp2;

	leng = cs->bufdatas >> 2;
	if (!leng) {
		return;
	}
	pos12 = cs->pos12;
	do {
		samppos = pos12 >> 12;
		if (leng <= samppos) {
			break;
		}
		fract = pos12 & ((1 << 12) - 1);
		ptr1 = cs->buffer +
						((cs->bufpos + (samppos << 2) + 0) & CS4231_BUFMASK);
		ptr2 = cs->buffer +
						((cs->bufpos + (samppos << 2) + 4) & CS4231_BUFMASK);
		samp1 = (((SINT8)ptr1[1]) << 8) + ptr1[0];
		samp2 = (((SINT8)ptr2[1]) << 8) + ptr2[0];
		samp1 += ((samp2 - samp1) * fract) >> 12;
		pcm[0] += samp1;
		samp1 = (((SINT8)ptr1[3]) << 8) + ptr1[2];
		samp2 = (((SINT8)ptr2[3]) << 8) + ptr2[2];
		samp1 += ((samp2 - samp1) * fract) >> 12;
		pcm[1] += samp1;
		pcm += 2;
		pos12 += cs->step12;
	} while(--count);
	
	leng = min(leng, (pos12 >> 12));
	cs->bufdatas -= (leng << 2);
	cs->bufpos = (cs->bufpos + (leng << 2)) & CS4231_BUFMASK;
	cs->pos12 = pos12 & ((1 << 12) - 1);
}

static void SOUNDCALL nomake(CS4231 cs, SINT32 *pcm, UINT count) {

	(void)cs;
	(void)pcm;
	(void)count;
}


typedef void (SOUNDCALL * CS4231FN)(CS4231 cs, SINT32 *pcm, UINT count);

static const CS4231FN cs4231fn[16] = {
			pcm8m,		// 0: 8bit PCM
			pcm8s,
			nomake,		// 1: u-Law
			nomake,
			pcm16m_ex,	// 2: 16bit PCM(little endian)?
			pcm16s_ex,
			nomake,		// 3: A-Law
			nomake,
			nomake,		// 4:
			nomake,
			nomake,		// 5: ADPCM
			nomake,
			pcm16m,		// 6: 16bit PCM
			pcm16s,
			nomake,		// 7: ADPCM
			nomake};


// ----

void SOUNDCALL cs4231_getpcm(CS4231 cs, SINT32 *pcm, UINT count) {

	if ((cs->reg.iface & 1) && (count)) {
		(*cs4231fn[cs->reg.datafmt >> 4])(cs, pcm, count);
		//// CS4231タイマー割り込み（手抜き）
		//if ((cs->reg.pinctrl & 2) && (cs->dmairq != 0xff) && (cs->timer)) {
		//	static double timercount = 0;
		//	int decval = 0;
		//	timercount += (double)count/44100 * 1000 * 100; // 10usec timer
		//	decval = (int)(timercount);
		//	timercount -= (double)decval;
		//	cs->timercounter -= decval;
		//	if(cs->timercounter < 0){
		//		cs->timercounter = 3000;//cs->timer;
		//		cs->intflag |= INt;
		//		cs->reg.featurestatus |= PI;
		//		pic_setirq(cs->dmairq);
		//	}
		//}
	}
		
}

