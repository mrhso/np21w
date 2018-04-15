/**
 * @file	cs4231c.c
 * @brief	Implementation of the CS4231
 */

#include "compiler.h"
#include "cs4231.h"
#include "iocore.h"
#include "fmboard.h"
#include "dmac.h"
#include	"cpucore.h"

	CS4231CFG	cs4231cfg;

	int cs4231_dma_compflag = 1;
	int cs4231_dma_enable = 0;

enum {
	CS4231REG_LINPUT	= 0x00,
	CS4231REG_RINPUT	= 0x01,
	CS4231REG_AUX1L		= 0x02,
	CS4231REG_AUX1R		= 0x03,
	CS4231REG_AUX2L		= 0x04,
	CS4231REG_AUX2R		= 0x05,
	CS4231REG_LOUTPUT	= 0x06,
	CS4231REG_ROUTPUT	= 0x07,
	CS4231REG_PLAYFMT	= 0x08,
	CS4231REG_INTERFACE	= 0x09,
	CS4231REG_PINCTRL	= 0x0a,
	CS4231REG_TESTINIT	= 0x0b,
	CS4231REG_MISCINFO	= 0x0c,
	CS4231REG_LOOPBACK	= 0x0d,
	CS4231REG_PLAYCNTM	= 0x0e,
	CS4231REG_PLAYCNTL	= 0x0f,

	CS4231REG_FEATURE1	= 0x10,
	CS4231REG_FEATURE2	= 0x11,
	CS4231REG_LLINEIN	= 0x12,
	CS4231REG_RLINEIN	= 0x13,
	CS4231REG_TIMERL	= 0x14,
	CS4231REG_TIMERH	= 0x15,
	CS4231REG_RESERVED1= 0x16,
	CS4231REG_RESERVED2= 0x17,
	CS4231REG_IRQSTAT	= 0x18,
	CS4231REG_VERSION	= 0x19,
	CS4231REG_MONOCTRL	= 0x1a,
	CS4231REG_RECFMT	= 0x1c,
	CS4231REG_PLAYFREQ	= 0x1d,
	CS4231REG_RECCNTM	= 0x1e,
	CS4231REG_RECCNTL	= 0x1f
};


UINT dmac_getdata_(DMACH dmach, UINT8 *buf, UINT size);
static const UINT32 cs4231xtal64[2] = {24576000/64, 16934400/64};

static const UINT8 cs4231cnt64[8] = {
				3072/64,	//  8000/ 5510
				1536/64,	// 16000/11025
				 896/64,	// 27420/18900
				 768/64,	// 32000/22050
				 448/64,	// 54840/37800
				 384/64,	// 64000/44100
				 512/64,	// 48000/33075
				2560/64};	//  9600/ 6620

//    640:441

static int cs4231_hThreadDMA_enable = 0;
static HANDLE	cs4231_hThreadDMA = NULL; // DMA用スレッド
static DWORD WINAPI cs4231_hThreadDMAFunc(LPVOID vdParam) {
	_NEVENTITEM nitem = {NULL};
	nitem.flag |= NEVENT_SETEVENT;
	while (cs4231_hThreadDMA_enable) {
		if(cs4231_dma_enable) cs4231_dma(&nitem);
		Sleep(1);
		//SleepEx(0, TRUE);
	}
	return 0;
}

void cs4231_initialize(UINT rate) {

	cs4231cfg.rate = rate;
}

void cs4231_setvol(UINT vol) {

	(void)vol;
}

void cs4231_dma(NEVENTITEM item) {

	DMACH	dmach;
	UINT	rem;
	UINT	pos;
	UINT	size;
	UINT	r;
	SINT32	cnt;
	static int xx = 0;
	if (item->flag & NEVENT_SETEVENT) {
		if (cs4231.dmach != 0xff) {
			sound_sync();
			if(!cs4231_lock && !cs4231_dma_compflag){
				cs4231_lock = 1;
				dmach = dmac.dmach + cs4231.dmach;
				if (cs4231.bufsize > cs4231.bufdatas) {
					rem = cs4231.bufsize - cs4231.bufdatas;
					pos = (cs4231.bufpos + cs4231.bufdatas) & CS4231_BUFMASK;
					size = min(rem, CS4231_BUFFERS - pos);
					r = dmac_getdata_(dmach, (cs4231.buffer + pos), size);
					rem -= r;
					cs4231.bufdatas += r;
					if (r != size) {
						if ((dmach->leng.w ==0) && rem ){
							dmach->leng.w = rem;
						}
						pos = (cs4231.bufpos + cs4231.bufdatas) & CS4231_BUFMASK;
						r = dmac_getdata_(dmach,cs4231.buffer+pos, rem);
						cs4231.bufdatas += r;
					}
				}
				if ((dmach->leng.w) && (cs4231cfg.rate)) {
					cnt = pccore.realclock * 16 / cs4231cfg.rate;
					nevent_set(NEVENT_CS4231, cnt, cs4231_dma, NEVENT_RELATIVE);		
				}else{
					//cs4231_hThreadDMA_enable = 0;
					cs4231_dma_enable = 0;
				}
				cs4231_lock = 0;
			}
		}

	}
	(void)item;
}

void cs4231_datasend(REG8 dat) {
	UINT	pos;
	if (cs4231.reg.iface & PPIO) {		// PIO play enable
		if (cs4231.bufsize <= cs4231.bufdatas) {
			sound_sync();
		}
		if (cs4231.bufsize > cs4231.bufdatas) {
			pos = (cs4231.bufpos + cs4231.bufdatas) & CS4231_BUFMASK;
			cs4231.buffer[pos] = dat;
			cs4231.bufdatas++;
		}
	}
}

REG8 DMACCALL cs4231dmafunc(REG8 func) {
	SINT32	cnt;
	switch(func) {
		case DMAEXT_START:
			if (cs4231cfg.rate) {
				DWORD dwID = 0;
				DMACH	dmach;
				dmach = dmac.dmach + cs4231.dmach;
				//if(cs4231_hThreadDMA){
				//	cs4231_hThreadDMA_enable = 0;
				//	WaitForSingleObject(cs4231_hThreadDMA,  INFINITE);
				//	CloseHandle(cs4231_hThreadDMA);
				//}
				cs4231_dma_enable = 1;
				cs4231_lock = 0;
				cs4231.bufdatas = 0;
				cs4231_dma_compflag = 0;
				//if(!cs4231_hThreadDMA){
				//	cs4231_hThreadDMA_enable = 1;
				//	cs4231_hThreadDMA = CreateThread(NULL , 0 , cs4231_hThreadDMAFunc  , NULL , 0 , &dwID);
				//}
				cnt = pccore.realclock * 16 / cs4231cfg.rate;
				nevent_set(NEVENT_CS4231, cnt, cs4231_dma, NEVENT_ABSOLUTE);
			}
			break;
		case DMAEXT_END:
			if ((cs4231.reg.pinctrl & 2) && (cs4231.dmairq != 0xff)) {
				cs4231.intflag |= INt;
				cs4231.reg.featurestatus |= PI;
				pic_setirq(cs4231.dmairq);
			}
			break;

		case DMAEXT_BREAK:
			nevent_reset(NEVENT_CS4231);
			cs4231_dma_enable = 0;
			//cs4231_hThreadDMA_enable = 0;
			//if(cs4231_hThreadDMA){
			//	WaitForSingleObject(cs4231_hThreadDMA,  INFINITE);
			//	CloseHandle(cs4231_hThreadDMA);
			//	cs4231_hThreadDMA = NULL;
			//}
			cs4231_lock = 0;
			break;
	}
	return(0);
}

void cs4231_reset(void) {

	ZeroMemory(&cs4231, sizeof(cs4231));
	cs4231.bufsize = CS4231_BUFFERS;
//	cs4231.proc = cs4231_nodecode;
	cs4231.dmach = 0xff;
	cs4231.dmairq = 0xff;
	FillMemory(cs4231.port, sizeof(cs4231.port), 0xff);
}

void cs4231_update(void) {
}


static void setdataalign(void) {

	UINT	step;

	step = (0 - cs4231.bufpos) & 3;
	if (step) {
		cs4231.bufpos += step;
		cs4231.bufdatas -= min(step, cs4231.bufdatas);
	}
	cs4231.bufdatas &= ~3;
}

void cs4231_control(UINT idx, REG8 dat) {
	UINT8	modify;
	DMACH	dmach;
	switch(idx){
		case 0xd:break;
		case 0xc:
		dat &= 0x40;
		dat |= 0x8a;
		break;
		case 0xb://ErrorStatus 
		case 0x19://Version ID
		return;
	default:
		break;

	}
	dmach = dmac.dmach + cs4231.dmach;
	modify = ((UINT8 *)&cs4231.reg)[idx] ^ dat;
	((UINT8 *)&cs4231.reg)[idx] = dat;
	switch(idx) {
	case CS4231REG_PLAYFMT:
			if (modify & 0xf0) {
				setdataalign();
			}
			if (cs4231cfg.rate) {
				UINT32 r;
				r = cs4231xtal64[dat & 1] / cs4231cnt64[(dat >> 1) & 7];
				TRACEOUT(("samprate = %d", r));
				r <<= 12;
				r /= cs4231cfg.rate;
				cs4231.step12 = r;
				TRACEOUT(("step12 = %d", r));
			}
			else {
				cs4231.step12 = 0;
			}
			break;
		case CS4231REG_INTERFACE:
			if (modify & PEN ) {
				if (cs4231.dmach != 0xff) {
					dmach = dmac.dmach + cs4231.dmach;
					if ((dat & (PEN)) == (PEN)){
						dmach->ready = 1;
					}
					else {
						dmach->ready = 0;
					}
					dmac_check();
				}	
				if (!(dat & PEN)) {		// stop!
					cs4231.pos12 = 0; 
				}
			}
			break;
		case CS4231REG_IRQSTAT:
			if (modify & PI) {
                	/* XXX: TI CI */
                	pic_resetirq (cs4231.dmairq);
                	cs4231.intflag &= ~INt;
			}
            		break;
	}
}


UINT dmac_getdata_(DMACH dmach, UINT8 *buf, UINT size) {
	UINT	leng;
	UINT32	addr;
	UINT32 addr2;
	UINT	i;
	int over =0;
	//static INT32 dma_w2 = 0;
	//if(dma_w2 == 0) dma_w2 = dmach->leng.w;
	leng = min(dmach->leng.w, size);
	if (leng) {
		addr = dmach->adrs.d;				// + mask
		addr2 = dmach->startaddr;
		if (!(dmach->mode & 0x20)) {			// dir +
			for (i=0; i<leng ; i++) {
				buf[i] = MEMP_READ8(addr);
				addr++;
				//dma_w2--;
				if(addr > dmach->lastaddr){
					addr = addr2;
				}
			}
			dmach->adrs.d = addr;
		}
		else {									// dir -
			for (i=0; i<leng; i++) {
				buf[i] = MEMP_READ8(addr - i);
			}
			dmach->adrs.d -= leng;
		}
		dmach->leng.w -= leng;
		if (dmach->leng.w <= 0) {
		//if(dma_w2 <= 0){
			dmach->leng.w = dmach->startcount;//戻す
			dmach->proc.extproc(DMAEXT_END);
			//if((MEMP_READ8(addr)|MEMP_READ8(addr+1)|MEMP_READ8(addr+2)|MEMP_READ8(addr+3))==0){
			//	cs4231_dma_compflag = 1;
			//}
		}
	}
	return(leng);
}

