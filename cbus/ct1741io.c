#include	"compiler.h"
#include	"cpucore.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"ct1741io.h"
#include	"ct1745io.h"
#include	"sound.h"
#include	"fmboard.h"

#ifdef SUPPORT_SOUND_SB16

/**
 * Creative SoundBlaster16 DSP CT1741
 *
 */

static char * const copyright_string = "COPYRIGHT (C) CREATIVE TECHNOLOGY LTD, 1992.";

static const UINT8 ct1741_cmd_len[256] = {
  0,0,0,0, 1,2,0,0, 1,0,0,0, 0,0,2,1,  // 0x00
  1,0,0,0, 2,2,2,2, 0,0,0,0, 0,0,0,0,  // 0x10
  0,0,0,0, 2,0,0,0, 0,0,0,0, 0,0,0,0,  // 0x20
  0,0,0,0, 0,0,0,0, 1,0,0,0, 0,0,0,0,  // 0x30

  1,2,2,0, 0,0,0,0, 2,0,0,0, 0,0,0,0,  // 0x40
  0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,  // 0x50
  0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,  // 0x60
  0,0,0,0, 2,2,2,2, 0,0,0,0, 0,0,0,0,  // 0x70

  2,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,  // 0x80
  0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,  // 0x90
  0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,  // 0xa0
  3,3,3,3, 3,3,3,3, 3,3,3,3, 3,3,3,3,  // 0xb0

  3,3,3,3, 3,3,3,3, 3,3,3,3, 3,3,3,3,  // 0xc0
  0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,  // 0xd0
  1,0,1,0, 1,0,0,0, 0,0,0,0, 0,0,0,0,  // 0xe0
  0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0   // 0xf0
};

static UINT ct1741_np2_rate = 0; 

void ct1741_initialize(UINT rate) {

	g_sb16.dsp_info.dma.rate2 = ct1741_np2_rate = rate;
}

void ct1741_set_dma_irq(UINT8 irq) {
	g_sb16.dsp_info.dmairq = irq;
	switch(irq) {
		case 1:
			g_sb16.dmairq = 3;
			break;
		case 8:
			g_sb16.dmairq = 5;
			break;
		case 2:
			g_sb16.dmairq = 10;
			break;
		case 4:
			g_sb16.dmairq = 12;
			break;
	}
}

UINT8 ct1741_get_dma_irq() {
	switch(g_sb16.dsp_info.dmairq){
		case  3:return 1;
		case  5:return 8;
		case 10:return 2;
		case 12:return 4;
	}
	return 0x00;
}

void ct1741_set_dma_ch(UINT8 dmach) {
	g_sb16.dsp_info.dmach = dmach;
	if (dmach & 0x01) g_sb16.dmach = 0;
	if (dmach & 0x02) g_sb16.dmach = 3;
}

UINT8 ct1741_get_dma_ch() {
	switch(g_sb16.dsp_info.dmach){
		case 0:return 0x21;
		case 3:return 0x42;
	}
	return 0x00;
}

static void ct1741_change_mode(DSP_MODES mode) {
	if (g_sb16.dsp_info.mode == mode) return;
//	else g_sb16.dsp_info.chan->FillUp();
	g_sb16.dsp_info.mode = mode;
}

static void ct1741_reset(void)
{
	ct1741_change_mode(DSP_MODE_NONE);
	g_sb16.dsp_info.freq=22050;
	g_sb16.dsp_info.cmd_len=0;
	g_sb16.dsp_info.in.pos=0;
	g_sb16.dsp_info.write_busy = 0;
       g_sb16.dsp_info.dmairq = g_sb16.dmairq;
       g_sb16.dsp_info.dmach = g_sb16.dmach;
}

static void ct1741_flush_data(void)
{
	g_sb16.dsp_info.out.used=0;
	g_sb16.dsp_info.out.pos=0;
}

static void ct1741_dma_transfer(DMA_MODES mode, UINT32 freq, BOOL stereo) {
	g_sb16.dsp_info.mode = DSP_MODE_DMA_MASKED;
//	g_sb16.dsp_info.chan->FillUp();
	g_sb16.dsp_info.dma.left = g_sb16.dsp_info.dma.total;
	g_sb16.dsp_info.dma.mode = mode;
	g_sb16.dsp_info.dma.stereo = stereo;
//	sb.irq.pending_8bit = false;
//	sb.irq.pending_16bit = false;
	printf("dma_transfer mode = %x\n",mode);
	switch (mode) {
	case DSP_DMA_2:
		// "2-bits ADPCM";
		g_sb16.dsp_info.dma.mul=(1 << SB_SH)/4;
		break;
	case DSP_DMA_3:
		// "3-bits ADPCM";
		g_sb16.dsp_info.dma.mul=(1 << SB_SH)/3;
		break;
	case DSP_DMA_4:
		// "4-bits ADPCM";
		g_sb16.dsp_info.dma.mul=(1 << SB_SH)/2;
		break;
	case DSP_DMA_8:
		// "8-bits PCM";
		g_sb16.dsp_info.dma.mul=(1 << SB_SH);
		break;
	case DSP_DMA_16_ALIASED:
		//type="16-bits(aliased) PCM";
		g_sb16.dsp_info.dma.mul=(1 << SB_SH)*2;
		break;
	case DSP_DMA_16:
		// type="16-bits PCM";
		g_sb16.dsp_info.dma.mul=(1 << SB_SH);
		break;
	default:
//		LOG(LOG_SB,LOG_ERROR)("DSP:Illegal transfer mode %d",mode);
		return;
	}
	if (g_sb16.dsp_info.dma.stereo) g_sb16.dsp_info.dma.mul *= 2;
	g_sb16.dsp_info.dma.rate = (g_sb16.dsp_info.freq * g_sb16.dsp_info.dma.mul) >> SB_SH;
	g_sb16.dsp_info.dma.min = (g_sb16.dsp_info.dma.rate * 3) / 1000;
//	g_sb16.dsp_info.chan->SetFreq(freq);
	g_sb16.dsp_info.dma.mode = mode;
//	PIC_RemoveEvents(END_DMA_Event);
//	g_sb16.dsp_info.dma.chan->Register_Callback(DSP_DMA_CallBack);
	g_sb16.dsp_info.dma.chan->ready = 1;
	dmac_check();
}

static void ct1741_prepare_dma_old(DMA_MODES mode, BOOL autoinit) {
	g_sb16.dsp_info.dma.autoinit = autoinit;
	if (!autoinit)
		g_sb16.dsp_info.dma.total = 1 + g_sb16.dsp_info.in.data[0] + (g_sb16.dsp_info.in.data[1] << 8);
	g_sb16.dsp_info.dma.chan = dmac.dmach + g_sb16.dmach;	// 8bit dma irq
	ct1741_dma_transfer(mode, g_sb16.dsp_info.freq / 1, FALSE);
//	ct1741_dma_transfer(mode, g_sb16.dsp_info.freq / (sb.mixer.stereo ? 2 : 1), sb.mixer.stereo);
}

static void ct1741_prepare_dma(DMA_MODES mode, UINT32 length, BOOL autoinit, BOOL stereo) {
	UINT32 freq = g_sb16.dsp_info.freq;
	g_sb16.dsp_info.dma.total = length;
	g_sb16.dsp_info.dma.autoinit = autoinit;
	if (mode==DSP_DMA_16) {
		g_sb16.mixreg[0x82] |= 0x2;
		if (g_sb16.dmairq != 0xff) {
			g_sb16.dsp_info.dma.chan = dmac.dmach + g_sb16.dmach;
//			g_sb16.dsp_info.dma.chan = GetDMAChannel(sb.hw.dma16);
		} else {
		g_sb16.mixreg[0x82] |= 0x1;
			g_sb16.dsp_info.dma.chan = dmac.dmach + g_sb16.dmach;
//			g_sb16.dsp_info.dma.chan = GetDMAChannel(sb.hw.dma8);
			mode = DSP_DMA_16_ALIASED;
			freq /= 2;
		}
	} else {
		g_sb16.mixreg[0x82] |= 0x1;
		g_sb16.dsp_info.dma.chan = dmac.dmach + g_sb16.dmach;
//		sb.dma.chan = GetDMAChannel(sb.hw.dma8);
	}
	ct1741_dma_transfer(mode, freq, stereo);
}

static void ct1741_add_data(UINT8 dat)
{
	if (g_sb16.dsp_info.out.used < DSP_BUFSIZE) {
		UINT32 start = g_sb16.dsp_info.out.used + g_sb16.dsp_info.out.pos;
		if (start >= DSP_BUFSIZE)
			start -= DSP_BUFSIZE;
		g_sb16.dsp_info.out.data[start] = dat;
		g_sb16.dsp_info.out.used++;
	}
	printf("dsp data = %x,pos = %x\n",dat,g_sb16.dsp_info.out.pos);
}

static void ct1741_exec_command()
{
	switch (g_sb16.dsp_info.cmd) {
	case 0x0f:
		g_sb16.dsp_info.out.used = 1;
		break;
	case 0x04:	/* DSP Status SB 2.0/pro version */
		ct1741_flush_data();
		ct1741_add_data(0xff);			//Everthing enabled
		break;
	case 0x10:	/* Direct DAC */
		ct1741_change_mode(DSP_MODE_DAC);
//		if (sb.dac.used<DSP_DACSIZE) {
//			sb.dac.data[sb.dac.used++]=(Bit8s(sb.dsp.in.data[0] ^ 0x80)) << 8;
//			sb.dac.data[sb.dac.used++]=(Bit8s(sb.dsp.in.data[0] ^ 0x80)) << 8;
//		}
		break;
	case 0x24:	/* Singe Cycle 8-Bit DMA ADC */
		g_sb16.dsp_info.dma.left = g_sb16.dsp_info.dma.total= 1 + g_sb16.dsp_info.in.data[0] + (g_sb16.dsp_info.in.data[1] << 8);
//		LOG(LOG_SB,LOG_ERROR)("DSP:Faked ADC for %d bytes",sb.dma.total);
//		GetDMAChannel(sb.hw.dma8)->Register_Callback(DSP_ADC_CallBack);
		break;
	case 0x14:	/* Singe Cycle 8-Bit DMA DAC */
	case 0x91:	/* Singe Cycle 8-Bit DMA High speed DAC */
		/* Note: 0x91 is documented only for DSP ver.2.x and 3.x, not 4.x */
		ct1741_prepare_dma_old(DSP_DMA_8, FALSE);
		break;
	case 0x90:	/* Auto Init 8-bit DMA High Speed */
	case 0x1c:	/* Auto Init 8-bit DMA */
		ct1741_prepare_dma_old(DSP_DMA_8, TRUE);
		break;
	case 0x38:  /* Write to SB MIDI Output */
//		if (sb.midi == true) MIDI_RawOutByte(g_sb16.dsp_info.in.data[0]);
		break;
	case 0x40:	/* Set Timeconstant */
		g_sb16.dsp_info.freq=(1000000 / (256 - g_sb16.dsp_info.in.data[0]));
		/* Nasty kind of hack to allow runtime changing of frequency */
		if (g_sb16.dsp_info.dma.mode != DSP_DMA_NONE && g_sb16.dsp_info.dma.autoinit) {
			ct1741_prepare_dma_old(g_sb16.dsp_info.dma.mode, g_sb16.dsp_info.dma.autoinit);
		}
		break;
	case 0x41:	/* Set Output Samplerate */
	case 0x42:	/* Set Input Samplerate */
		g_sb16.dsp_info.freq=(g_sb16.dsp_info.in.data[0] << 8)  | g_sb16.dsp_info.in.data[1];
//		g_sb16.dsp_info.dma.step12 = (g_sb16.dsp_info.freq << 12)/g_sb16.dsp_info.dma.rate2;
//		g_sb16.dsp_info.dma.bufpos = 1;
		break;
	case 0x48:	/* Set DMA Block Size */
		//TODO Maybe check limit for new irq?
		g_sb16.dsp_info.dma.total= 1 + g_sb16.dsp_info.in.data[0] + (g_sb16.dsp_info.in.data[1] << 8);
		break;
	case 0x75:	/* 075h : Single Cycle 4-bit ADPCM Reference */
//		sb.adpcm.haveref = true;
	case 0x74:	/* 074h : Single Cycle 4-bit ADPCM */	
		ct1741_prepare_dma_old(DSP_DMA_4, FALSE);
		break;
	case 0x77:	/* 077h : Single Cycle 3-bit(2.6bit) ADPCM Reference*/
//		sb.adpcm.haveref=true;
	case 0x76:  /* 074h : Single Cycle 3-bit(2.6bit) ADPCM */
		ct1741_prepare_dma_old(DSP_DMA_3, FALSE);
		break;
	case 0x17:	/* 017h : Single Cycle 2-bit ADPCM Reference*/
//		sb.adpcm.haveref=true;
	case 0x16:  /* 074h : Single Cycle 2-bit ADPCM */
		ct1741_prepare_dma_old(DSP_DMA_2, FALSE);
		break;
	case 0x80:	/* Silence DAC */
//		PIC_AddEvent(&DSP_RaiseIRQEvent,
//			(1000.0f*(1 + g_sb16.dsp_info.in.data[0] + (g_sb16.dsp_info.in.data[1] << 8))/sb.freq));
		break;
	case 0xb0:	case 0xb1:	case 0xb2:	case 0xb3:  case 0xb4:	case 0xb5:	case 0xb6:	case 0xb7:
	case 0xb8:	case 0xb9:	case 0xba:	case 0xbb:  case 0xbc:	case 0xbd:	case 0xbe:	case 0xbf:
	case 0xc0:	case 0xc1:	case 0xc2:	case 0xc3:  case 0xc4:	case 0xc5:	case 0xc6:	case 0xc7:
	case 0xc8:	case 0xc9:	case 0xca:	case 0xcb:  case 0xcc:	case 0xcd:	case 0xce:	case 0xcf:
		/* Generic 8/16 bit DMA */
		g_sb16.dsp_info.dma.sign=(g_sb16.dsp_info.in.data[0] & 0x10) > 0;
		ct1741_prepare_dma((g_sb16.dsp_info.cmd & 0x10) ? DSP_DMA_16 : DSP_DMA_8,
			1 + g_sb16.dsp_info.in.data[1] + (g_sb16.dsp_info.in.data[2] << 8),
			(g_sb16.dsp_info.cmd & 0x4)>0,
			(g_sb16.dsp_info.in.data[0] & 0x20) > 0
		);
		break;
	case 0xd5:	/* Halt 16-bit DMA */
	case 0xd0:	/* Halt 8-bit DMA */
		g_sb16.dsp_info.mode = DSP_MODE_DMA_PAUSE;
		g_sb16.dsp_info.dma.mode = DSP_DMA_NONE;
//		PIC_RemoveEvents(END_DMA_Event);
		break;
	case 0xd1:	/* Enable Speaker */
//		DSP_SetSpeaker(true);
		break;
	case 0xd3:	/* Disable Speaker */
//		DSP_SetSpeaker(false);
		break;
	case 0xd8:  /* Speaker status */
		ct1741_flush_data();
//		if (sb.speaker)
			ct1741_add_data(0xff);
//		else
//			ct1741_add_data(0x00);
		break;
	case 0xd6:	/* Continue DMA 16-bit */
	case 0xd4:	/* Continue DMA 8-bit*/
		if (g_sb16.dsp_info.mode == DSP_MODE_DMA_PAUSE) {
			g_sb16.dsp_info.mode = DSP_MODE_DMA;
//			sb.dma.chan->Register_Callback(DSP_DMA_CallBack);
		}
		break;
	case 0xd9:  /* Exit Autoinitialize 16-bit */
	case 0xda:	/* Exit Autoinitialize 8-bit */
		/* Set mode to single transfer so it ends with current block */
		g_sb16.dsp_info.dma.autoinit = FALSE;		//Should stop itself
		g_sb16.dsp_info.dma.mode = DSP_DMA_NONE;//必要？
		break;
	case 0xe0:	/* DSP Identification - SB2.0+ */
		ct1741_flush_data();
		ct1741_add_data(~g_sb16.dsp_info.in.data[0]);
		break;
	case 0xe1:	/* Get DSP Version */
		ct1741_flush_data();
		ct1741_add_data(0x4);
		ct1741_add_data(0xc);
		break;
	case 0xe2:	/* Weird DMA identification write routine */
		{
//			LOG(LOG_SB,LOG_NORMAL)("DSP Function 0xe2");
//			for (Bitu i = 0; i < 8; i++)
//				if ((sb.dsp.in.data[0] >> i) & 0x01) sb.e2.value += E2_incr_table[sb.e2.count % 4][i];
//			 sb.e2.value += E2_incr_table[sb.e2.count % 4][8];
//			 sb.e2.count++;
//			 GetDMAChannel(sb.hw.dma8)->Register_Callback(DSP_E2_DMA_CallBack);
		}
		break;
	case 0xe3:	/* DSP Copyright */
		{
			UINT32 i;
			ct1741_flush_data();
			for (i=0; i <= strlen(copyright_string); i++) {
				ct1741_add_data(copyright_string[i]);
			}
		}
		break;
	case 0xe4:	/* Write Test Register */
		g_sb16.dsp_info.test_register = g_sb16.dsp_info.in.data[0];
		break;
	case 0xe8:	/* Read Test Register */
		ct1741_flush_data();
		ct1741_add_data(g_sb16.dsp_info.test_register);;
		break;
	case 0xf2:	/* Trigger 8bit IRQ */
//		SB_RaiseIRQ(SB_IRQ_8);
		ct1741_flush_data();
		ct1741_add_data (0xaa);
		pic_setirq(g_sb16.dmairq);
		g_sb16.mixreg[0x82] |= 1;
		break;
	case 0x30: case 0x31:
//		LOG(LOG_SB,LOG_ERROR)("DSP:Unimplemented MIDI I/O command %2X",sb.dsp.cmd);
		break;
	case 0x34: case 0x35: case 0x36: case 0x37:
//		LOG(LOG_SB,LOG_ERROR)("DSP:Unimplemented MIDI UART command %2X",sb.dsp.cmd);
		break;
	case 0x7d: case 0x7f: case 0x1f:
//		LOG(LOG_SB,LOG_ERROR)("DSP:Unimplemented auto-init DMA ADPCM command %2X",sb.dsp.cmd);
		break;
	case 0x20:
	case 0x2c:
	case 0x98: case 0x99: /* Documented only for DSP 2.x and 3.x */
	case 0xa0: case 0xa8: /* Documented only for DSP 3.x */
//		LOG(LOG_SB,LOG_ERROR)("DSP:Unimplemented input command %2X",sb.dsp.cmd);
		break;
	default:
//		LOG(LOG_SB,LOG_ERROR)("DSP:Unhandled (undocumented) command %2X",sb.dsp.cmd);
		break;
	}
	g_sb16.dsp_info.cmd = DSP_NO_COMMAND;
	g_sb16.dsp_info.cmd_len = 0;
	g_sb16.dsp_info.in.pos = 0;

}

/* DSP reset */

static void IOOUTCALL ct1741_write_reset(UINT port, REG8 dat)
{
printf ("write 26d2 %x\n",dat);

	if ((dat & 0x01)) {
		/* status reset */
		ct1741_reset();
		g_sb16.dsp_info.state = DSP_STATUS_RESET;
	} else {
		/* status normal */
		ct1741_flush_data();
		ct1741_add_data(0xaa);
		g_sb16.dsp_info.state = DSP_STATUS_NORMAL;
	}

if(dat == 0xc6){ct1741_flush_data();ct1741_reset();g_sb16.dsp_info.state = 2;}
	
}

/* DSP Write Command/Data */
static void IOOUTCALL ct1741_write_data(UINT port, REG8 dat) {
	TRACEOUT(("CT1741 DSP command: %.2x", dat));
	switch (g_sb16.dsp_info.cmd) {
	case DSP_NO_COMMAND:
		g_sb16.dsp_info.cmd = g_sb16.dsp_info.cmd_o = dat;
		g_sb16.dsp_info.cmd_len = ct1741_cmd_len[dat];
		g_sb16.dsp_info.in.pos=0;
		if (!g_sb16.dsp_info.cmd_len) ct1741_exec_command();
		break;
	default:
		if(g_sb16.dsp_info.in.pos < sizeof(g_sb16.dsp_info.in.data)/sizeof(g_sb16.dsp_info.in.data[0])){
			g_sb16.dsp_info.in.data[g_sb16.dsp_info.in.pos] = dat;
			g_sb16.dsp_info.in.pos++;
			if (g_sb16.dsp_info.in.pos >= g_sb16.dsp_info.cmd_len) ct1741_exec_command();
		}
	}
	port = dat;
}

static REG8 IOINPCALL ct1741_read_reset(UINT port)
{
printf ("%x\n",port);
if((port & 0x0d00) == 0x0d00)	/* DSP reset */
	return 0x0;		//ok
else return 0xff;//
}

/* DSP read data */
static REG8 IOINPCALL ct1741_read_data(UINT port)
{
	static REG8 data = 0;
	if (g_sb16.dsp_info.out.used) {
		if(g_sb16.dsp_info.out.pos < sizeof(g_sb16.dsp_info.out.data)/sizeof(g_sb16.dsp_info.out.data[0])){
			data = g_sb16.dsp_info.out.data[g_sb16.dsp_info.out.pos];
			g_sb16.dsp_info.out.pos++;
			if (g_sb16.dsp_info.out.pos >= DSP_BUFSIZE)
				g_sb16.dsp_info.out.pos -= DSP_BUFSIZE;
			g_sb16.dsp_info.out.used--;
		}
	}
	printf("read 2ad2 %x pos %d\n",data,g_sb16.dsp_info.out.pos);
	return data;
}

/* DSP read write status */
static REG8 IOINPCALL ct1741_read_wstatus(UINT port)
{
printf("read 2cd2 g_sb16.dsp_info.state = %x g_sb16.dsp_info.write_busy =%x\n",g_sb16.dsp_info.state,g_sb16.dsp_info.write_busy);

	switch(g_sb16.dsp_info.state) {
		case DSP_STATUS_NORMAL:
//			g_sb16.dsp_info.write_busy++;
//			if (g_sb16.dsp_info.write_busy & 8) return 0x80;//0xff;
			return 0x0;//x7f;

		case DSP_STATUS_RESET:
			return 0xff;
			break;
		case 2:
			return 0;
		default://????変な数値とるときがある　意味不明
			return 0;
	}
	return 0xff;
}

/* DSP read read status */
static REG8 IOINPCALL ct1741_read_rstatus(UINT port)
{
printf("read 2ed2 g_sb16.dsp_info.out.used =%x mixer[82] = %x cmd = %x\n",g_sb16.dsp_info.out.used,g_sb16.mixreg[0x82],g_sb16.dsp_info.cmd_o);
	// bit7が0ならDSPバッファは空
	if(g_sb16.dsp_info.cmd_o == 0xf2){g_sb16.dsp_info.cmd_o =0;return 0;}
	if(g_sb16.mixreg[0x82] & 1){
		g_sb16.mixreg[0x82] &= ~1;
		pic_resetirq(g_sb16.dmairq);
		if(g_sb16.dsp_info.dma.autoinit){
			if (!(g_sb16.dsp_info.dma.chan->leng.w)) {
			//if(g_sb16.dsp_info.dma.chan->startcount > g_sb16.dsp_info.dma.total){
			//	if(g_sb16.dsp_info.dma.total > 0){
					//g_sb16.dsp_info.dma.chan->leng.w = g_sb16.dsp_info.dma.total; // 戻す
					//g_sb16.dsp_info.dma.chan->adrs.d = g_sb16.dsp_info.dma.chan->startaddr; // 戻す
					//g_sb16.dsp_info.dma.total = 0;
			//	}
			//}else{
			//	//g_sb16.dsp_info.dma.total -= g_sb16.dsp_info.dma.chan->startcount;
				g_sb16.dsp_info.dma.chan->leng.w = g_sb16.dsp_info.dma.chan->startcount; // 戻す
				g_sb16.dsp_info.dma.chan->adrs.d = g_sb16.dsp_info.dma.chan->startaddr; // 戻す
					g_sb16.dsp_info.smpcounter2 = 0;
			//}
			}
		}
	}

	if (g_sb16.dsp_info.dma.mode!=DSP_DMA_16)
		return 0x80;
	else
		return 0x00;
}
/* DSP read read status */
static REG8 IOINPCALL ct1741_read_rstatus16(UINT port)
{
printf("read 2ed2 g_sb16.dsp_info.out.used =%x mixer[82] = %x cmd = %x\n",g_sb16.dsp_info.out.used,g_sb16.mixreg[0x82],g_sb16.dsp_info.cmd_o);
	// bit7が0ならDSPバッファは空
	if(g_sb16.mixreg[0x82] & 2){
		g_sb16.mixreg[0x82] &= ~2;
		pic_resetirq(g_sb16.dmairq);
		if(g_sb16.dsp_info.dma.autoinit){
			if (!(g_sb16.dsp_info.dma.chan->leng.w)) {
				g_sb16.dsp_info.dma.chan->leng.w = g_sb16.dsp_info.dma.chan->startcount; // 戻す
				g_sb16.dsp_info.dma.chan->adrs.d = g_sb16.dsp_info.dma.chan->startaddr; // 戻す
				g_sb16.dsp_info.smpcounter2 = 0;
			}
		}
	}
	return 0xff;
}

#define BUF_SPEED	(g_sb16.dsp_info.dma.mode==DSP_DMA_16 || g_sb16.dsp_info.dma.mode==DSP_DMA_16_ALIASED ? (g_sb16.dsp_info.dma.stereo ? 7 : 7) : (g_sb16.dsp_info.dma.stereo ? 15 : 15)) / (g_sb16.dsp_info.freq==11025 ? 2 : 1)
static int BUF_ALIGN[] = {
			1,		// 0: STOP
			1,
			1,
			1,			// 8Bit Unsigned PCM mono　これしかチェックできておりません
			1,			// 8Bit Signed PCM mono
			2,		//16Bit Signed PCM mono
			1,
			1,

			1,
			1,
			1,
			2,			// 8Bit Unsigned PCM stereo
			2,			// 8Bit UnSigned PCM stereo
			4,		//16Bit signed PCM stereo
			1,
			1
};

void ct1741_dma(NEVENTITEM item)
{
	UINT	r;
	SINT32	cnt;
	UINT	rem;
	UINT	pos;
	UINT	size;
	UINT8	dmabuf[DMA_BUFSIZE];
	int i;
	static int dmacnt2 = 0;

	if (item->flag & NEVENT_SETEVENT) {
		if (g_sb16.dmach != 0xff) {
			sound_sync();
			if (g_sb16.dsp_info.mode == DSP_MODE_DMA || g_sb16.dsp_info.mode == DSP_MODE_DMA_MASKED) {
				g_sb16.dsp_info.write_busy = 1;
				//if(!(g_sb16.mixreg[0x82] & 3)){
					// 転送～
					rem = g_sb16.dsp_info.dma.bufsize - g_sb16.dsp_info.dma.bufdatas;
					pos = (g_sb16.dsp_info.dma.bufpos + g_sb16.dsp_info.dma.bufdatas) & (DMA_BUFSIZE -1);
					size = min(rem, (g_sb16.dsp_info.dma.stereo ? 64 : 32));//DMAの転送量を少量に変更
					r = dmac_getdatas(g_sb16.dsp_info.dma.chan, dmabuf, size);
					for(i=0;i<r;i++){
						int dstpos = (pos + i) & (DMA_BUFSIZE -1);
						if(g_sb16.dsp_info.smpcounter2 < (g_sb16.dsp_info.dma.chan->startcount & ~(BUF_ALIGN[g_sb16.dsp_info.dma.mode|g_sb16.dsp_info.dma.stereo <<3] - 1))){
							g_sb16.dsp_info.dma.buffer[dstpos] = dmabuf[i];
							g_sb16.dsp_info.smpcounter2++;
							g_sb16.dsp_info.dma.bufdatas++;
						}
					}
					printf("g_sb16.dsp_info.dma.mode = %x\n",g_sb16.dsp_info.dma.mode);
					//g_sb16.dsp_info.dma.bufdatas += r;
					g_sb16.dsp_info.smpcounter += r;
					if(g_sb16.dsp_info.smpcounter > (int)g_sb16.dsp_info.dma.chan->startcount){
						g_sb16.dsp_info.smpcounter -= (int)g_sb16.dsp_info.dma.chan->startcount;
						//g_sb16.mixreg[0x82] |= 1;//(nchan & 4) ? 2 : 1;
						g_sb16.mixreg[0x82] |= (g_sb16.dsp_info.dma.mode==DSP_DMA_16 || g_sb16.dsp_info.dma.mode==DSP_DMA_16_ALIASED ? 2 : 1);
						pic_setirq(g_sb16.dmairq);
						dmacnt2 = dmacnt2 & 0x1;
					}
				//}
				
				if ((g_sb16.dsp_info.dma.chan->leng.w) && (g_sb16.dsp_info.freq)) {
					// 再度イベント設定
					//cnt = pccore.realclock * 32 / g_sb16.dsp_info.freq *(g_sb16.dsp_info.dma.rate2/g_sb16.dsp_info.freq);
	//				cnt = pccore.realclock   /g_sb16.dsp_info.dma.rate2 * 128;
					//if(!(g_sb16.mixreg[0x82] & 1)){
						cnt = pccore.realclock / g_sb16.dsp_info.freq * g_sb16.dsp_info.dma.rate2 / g_sb16.dsp_info.freq * BUF_SPEED;
					//}else{
					//	cnt = pccore.realclock / g_sb16.dsp_info.freq * g_sb16.dsp_info.dma.rate2 / g_sb16.dsp_info.freq * 16;
					//}
					if(cnt != 0){
						//nevent_set(NEVENT_CT1741, 100, ct1741_dma, NEVENT_RELATIVE);
						nevent_set(NEVENT_CT1741, cnt, ct1741_dma, NEVENT_RELATIVE);
						//g_sb16.dsp_info.write_busy = 1;
					}else{
						//g_sb16.dsp_info.write_busy = 0;
						nevent_setbyms(NEVENT_CT1741, 1, ct1741_dma, NEVENT_RELATIVE);
					}
				}else{
					g_sb16.dsp_info.write_busy = 0;
					if(g_sb16.dsp_info.dma.bufdatas < 16){
						g_sb16.mixreg[0x82] |= (g_sb16.dsp_info.dma.mode==DSP_DMA_16 || g_sb16.dsp_info.dma.mode==DSP_DMA_16_ALIASED ? 3 : 1);
						pic_setirq(g_sb16.dmairq);
						if(!g_sb16.dsp_info.dma.autoinit){
							g_sb16.dsp_info.dma.chan->leng.w = g_sb16.dsp_info.dma.chan->startcount; // 戻す
							g_sb16.dsp_info.dma.chan->adrs.d = g_sb16.dsp_info.dma.chan->startaddr; // 戻す
					g_sb16.dsp_info.smpcounter2 = 0;
						}
						//g_sb16.dsp_info.dma.chan->leng.w = g_sb16.dsp_info.dma.chan->startcount; // 戻す
						////if(g_sb16.dsp_info.dma.chan->adrs.d - g_sb16.dsp_info.dma.chan->startaddr >= g_sb16.dsp_info.dma.chan->startcount * 2){
						//g_sb16.dsp_info.dma.chan->adrs.d = g_sb16.dsp_info.dma.chan->startaddr; // 戻す
						//}
						//g_sb16.dsp_info.dma.chan->leng.w = g_sb16.dsp_info.dma.chan->startcount; // 戻す
						//if(g_sb16.dsp_info.dma.chan->adrs.d > g_sb16.dsp_info.dma.chan->lastaddr){
						//	g_sb16.dsp_info.dma.chan->adrs.d -= g_sb16.dsp_info.dma.chan->lastaddr - g_sb16.dsp_info.dma.chan->startaddr;
						//}
						cnt = pccore.realclock / g_sb16.dsp_info.freq * g_sb16.dsp_info.dma.rate2 / g_sb16.dsp_info.freq * BUF_SPEED;
					if(cnt != 0){
						//nevent_set(NEVENT_CT1741, 100, ct1741_dma, NEVENT_RELATIVE);
						nevent_set(NEVENT_CT1741, cnt, ct1741_dma, NEVENT_RELATIVE);
						//g_sb16.dsp_info.write_busy = 1;
					}else{
						//g_sb16.dsp_info.write_busy = 0;
						nevent_setbyms(NEVENT_CT1741, 1, ct1741_dma, NEVENT_RELATIVE);
					}
						//nevent_setbyms(NEVENT_CT1741, 1, ct1741_dma, NEVENT_RELATIVE);
					}else{
						cnt = pccore.realclock / g_sb16.dsp_info.freq * g_sb16.dsp_info.dma.rate2 / g_sb16.dsp_info.freq * BUF_SPEED;
					if(cnt != 0){
						//nevent_set(NEVENT_CT1741, 100, ct1741_dma, NEVENT_RELATIVE);
						nevent_set(NEVENT_CT1741, cnt, ct1741_dma, NEVENT_RELATIVE);
						//g_sb16.dsp_info.write_busy = 1;
					}else{
						//g_sb16.dsp_info.write_busy = 0;
						nevent_setbyms(NEVENT_CT1741, 1, ct1741_dma, NEVENT_RELATIVE);
					}
						//nevent_setbyms(NEVENT_CT1741, 1, ct1741_dma, NEVENT_RELATIVE);
					}
				}
			}else{
				g_sb16.dsp_info.write_busy = 0;
				pic_setirq(g_sb16.dmairq);
				//nevent_setbyms(NEVENT_CT1741, 1, ct1741_dma, NEVENT_RELATIVE);
			}
		}
	}

}

REG8 DMACCALL ct1741dmafunc(REG8 func)
{
	SINT32	cnt;

	switch(func) {
		case DMAEXT_START:
//			if (cs4231cfg.rate) {
			//cnt = pccore.realclock * 32 / g_sb16.dsp_info.freq *(g_sb16.dsp_info.dma.rate2/g_sb16.dsp_info.freq);;
		g_sb16.mixreg[0x82] &= ~3;
		pic_resetirq(g_sb16.dmairq);
						cnt = pccore.realclock / g_sb16.dsp_info.freq * g_sb16.dsp_info.dma.rate2 / g_sb16.dsp_info.freq * BUF_SPEED;
					if(cnt != 0){
						//nevent_set(NEVENT_CT1741, 100, ct1741_dma, NEVENT_RELATIVE);
						nevent_set(NEVENT_CT1741, cnt, ct1741_dma, NEVENT_RELATIVE);
						//g_sb16.dsp_info.write_busy = 1;
					}else{
						//g_sb16.dsp_info.write_busy = 0;
						nevent_setbyms(NEVENT_CT1741, 1, ct1741_dma, NEVENT_RELATIVE);
					}
			g_sb16.dsp_info.smpcounter = 0;
			g_sb16.dsp_info.smpcounter2 = 0;
			g_sb16.dsp_info.dma.bufdatas = 0;
			g_sb16.dsp_info.dma.bufpos = 0;
//			}
			break;

		case DMAEXT_END:
//			if ((cs4231.reg.pinctrl & 2) && (cs4231.dmairq != 0xff)) {
//				cs4231.intflag = 1;
			//g_sb16.dsp_info.write_busy = 0;
			//pic_setirq(g_sb16.dmairq);
			//if (g_sb16.dsp_info.mode == DSP_MODE_DMA || g_sb16.dsp_info.mode == DSP_MODE_DMA_MASKED) {
			//	cnt = pccore.realclock / g_sb16.dsp_info.freq * g_sb16.dsp_info.dma.rate2 / g_sb16.dsp_info.freq;
			//	//cnt = pccore.realclock * 32 / g_sb16.dsp_info.freq *(g_sb16.dsp_info.dma.rate2/g_sb16.dsp_info.freq);
			//	nevent_set(NEVENT_CT1741, cnt, ct1741_dma, NEVENT_ABSOLUTE);
			//}
						g_sb16.mixreg[0x82] |= (g_sb16.dsp_info.dma.mode==DSP_DMA_16 || g_sb16.dsp_info.dma.mode==DSP_DMA_16_ALIASED ? 3 : 1);
						pic_setirq(g_sb16.dmairq);
			break;

		case DMAEXT_BREAK:
			nevent_reset(NEVENT_CT1741);
			g_sb16.mixreg[0x82] &= ~3;
			pic_resetirq(g_sb16.dmairq);

			break;
	}
	return(0);
}

// 8bit モノラル
int position = 0;
static void SOUNDCALL pcm8m(DMA_INFO *cs, SINT32 *pcm, UINT count) {
	UINT32	leng;
	UINT32	pos12;
	SINT32	fract;
	UINT32	samppos = 0;
const UINT8	*ptr1;
const UINT8	*ptr2;
	SINT32	samp1;
	SINT32	samp2;
	int i;
	int	samplen_dst = soundcfg.rate;
	int	samplen_src;
	if(g_sb16.dsp_info.freq==44100){
		samplen_src = g_sb16.dsp_info.freq * 110/ 100;
	}else{
		samplen_src = g_sb16.dsp_info.freq * 102/ 100;
	}

	leng = cs->bufdatas;
	if (!leng) {
		return;
	}
	
	for(i=0;i<count;i++){
		samppos = (i * samplen_src / samplen_dst);
		if(samppos >= leng){
			break;
		}
		ptr1 = cs->buffer + ((cs->bufpos + samppos + 0) & DMA_BUFMASK);
		ptr2 = cs->buffer + ((cs->bufpos + samppos + 0) & DMA_BUFMASK);
		samp1 = (ptr1[0] - 0x80) << 8;
		samp2 = (ptr2[0] - 0x80) << 8;
		//samp1 += ((samp2 - samp1) * fract) >> 12;
		pcm[0] += (samp1 * np2cfg.vol_pcm * (SINT32)g_sb16.mixreg[MIXER_VOC_LEFT]  / 255 * (SINT32)g_sb16.mixreg[MIXER_MASTER_LEFT] ) >> 14;
		pcm[1] += (samp1 * np2cfg.vol_pcm * (SINT32)g_sb16.mixreg[MIXER_VOC_RIGHT] / 255 * (SINT32)g_sb16.mixreg[MIXER_MASTER_RIGHT]) >> 14;
		pcm += 2;
	}

	leng = min(leng, samppos);
	cs->bufdatas -= (leng << 0);
	cs->bufpos = (cs->bufpos + (leng << 0)) & CS4231_BUFMASK;
}

// 8bit ステレオ
static void SOUNDCALL pcm8s(DMA_INFO *cs, SINT32 *pcm, UINT count) {
	
	UINT32	leng;
	UINT32	pos12;
	SINT32	fract;
	UINT32	samppos = 0;
const UINT8	*ptr1;
const UINT8	*ptr2;
	SINT32	samp1;
	SINT32	samp2;
	int i;
	int	samplen_dst = soundcfg.rate;
	int	samplen_src;
	if(g_sb16.dsp_info.freq==44100){
		samplen_src = g_sb16.dsp_info.freq * 110/ 100;
	}else{
		samplen_src = g_sb16.dsp_info.freq * 102/ 100;
	}

	leng = cs->bufdatas;
	if (!leng) {
		return;
	}
	
	for(i=0;i<count;i++){
		samppos = 2*(i * samplen_src / samplen_dst);
		if(samppos >= leng){
			break;
		}
		ptr1 = cs->buffer + ((cs->bufpos + samppos + 0) & DMA_BUFMASK);
		ptr2 = cs->buffer + ((cs->bufpos + samppos + 1) & DMA_BUFMASK);
		samp1 = (ptr1[0] - 0x80) << 8;
		samp2 = (ptr2[0] - 0x80) << 8;
		//samp1 += ((samp2 - samp1) * fract) >> 12;
		pcm[0] += (samp1 * np2cfg.vol_pcm * (SINT32)g_sb16.mixreg[MIXER_VOC_LEFT]  / 255 * (SINT32)g_sb16.mixreg[MIXER_MASTER_LEFT] ) >> 14;
		pcm[1] += (samp2 * np2cfg.vol_pcm * (SINT32)g_sb16.mixreg[MIXER_VOC_RIGHT] / 255 * (SINT32)g_sb16.mixreg[MIXER_MASTER_RIGHT]) >> 14;
		pcm += 2;
	}

	leng = min(leng, samppos);
	cs->bufdatas -= (leng << 0);
	cs->bufpos = (cs->bufpos + (leng << 0)) & CS4231_BUFMASK;
}


// 16bit モノラル
static void SOUNDCALL Spcm16m(DMA_INFO *cs, SINT32 *pcm, UINT count) {
	
	UINT32	leng;
	UINT32	pos12;
	SINT32	fract;
	UINT32	samppos = 0;
const UINT8	*ptr1;
const UINT8	*ptr2;
	SINT32	samp1;
	SINT32	samp2;
	int i;
	int	samplen_dst = soundcfg.rate;
	int	samplen_src;
	if(g_sb16.dsp_info.freq==44100){
		samplen_src = g_sb16.dsp_info.freq * 120/ 100;
	}else{
		samplen_src = g_sb16.dsp_info.freq * 108/ 100;
	}

	leng = cs->bufdatas & ~0x1;
	if (!leng) {
		return;
	}
	
	for(i=0;i<count;i++){
		samppos = 2 * (i * samplen_src / samplen_dst);
		if(samppos >= leng){
			break;
		}
		ptr1 = (cs->buffer + ((cs->bufpos + samppos + 0) & DMA_BUFMASK));
		ptr2 = (cs->buffer + ((cs->bufpos + samppos + 0) & DMA_BUFMASK));
		samp1 = (((SINT8)ptr1[1]) << 8) + ptr1[0];
		samp2 = (((SINT8)ptr2[1]) << 8) + ptr2[0];
		//samp1 += ((samp2 - samp1) * fract) >> 12;
		pcm[0] += (samp1 * np2cfg.vol_pcm * (SINT32)g_sb16.mixreg[MIXER_VOC_LEFT]  / 255 * (SINT32)g_sb16.mixreg[MIXER_MASTER_LEFT] ) >> 14;
		pcm[1] += (samp1 * np2cfg.vol_pcm * (SINT32)g_sb16.mixreg[MIXER_VOC_RIGHT] / 255 * (SINT32)g_sb16.mixreg[MIXER_MASTER_RIGHT]) >> 14;
		pcm += 2;
	}

	leng = min(leng, samppos);
	cs->bufdatas -= (leng << 0);
	cs->bufpos = (cs->bufpos + (leng << 0)) & CS4231_BUFMASK;
}

// 16bit ステレオ(little endian)
static void SOUNDCALL Spcm16s(DMA_INFO *cs, SINT32 *pcm, UINT count) {
	
	UINT32	leng;
	UINT32	pos12;
	SINT32	fract;
	UINT32	samppos = 0;
const UINT8	*ptr1;
const UINT8	*ptr2;
	SINT32	samp1;
	SINT32	samp2;
	int i;
	int	samplen_dst = soundcfg.rate;
	int	samplen_src;
	if(g_sb16.dsp_info.freq==44100){
		samplen_src = g_sb16.dsp_info.freq * 120/ 100;
	}else{
		samplen_src = g_sb16.dsp_info.freq * 108/ 100;
	}

	leng = cs->bufdatas & ~0x1;
	if (!leng) {
		return;
	}
	
	for(i=0;i<count;i++){
		samppos = 4 * (i * samplen_src / samplen_dst);
		if(samppos >= leng){
			break;
		}
		ptr1 = (cs->buffer + ((cs->bufpos + samppos + 0) & DMA_BUFMASK));
		ptr2 = (cs->buffer + ((cs->bufpos + samppos + 2) & DMA_BUFMASK));
		samp1 = (((SINT8)ptr1[1]) << 8) + ptr1[0];
		samp2 = (((SINT8)ptr2[1]) << 8) + ptr2[0];
		//samp1 += ((samp2 - samp1) * fract) >> 12;
		pcm[0] += (samp1 * np2cfg.vol_pcm * (SINT32)g_sb16.mixreg[MIXER_VOC_LEFT]  / 255 * (SINT32)g_sb16.mixreg[MIXER_MASTER_LEFT] ) >> 14;
		pcm[1] += (samp2 * np2cfg.vol_pcm * (SINT32)g_sb16.mixreg[MIXER_VOC_RIGHT] / 255 * (SINT32)g_sb16.mixreg[MIXER_MASTER_RIGHT]) >> 14;
		pcm += 2;
	}

	leng = min(leng, samppos);
	cs->bufdatas -= (leng << 0);
	cs->bufpos = (cs->bufpos + (leng << 0)) & CS4231_BUFMASK;
}
static void SOUNDCALL nomake(DMA_INFO *ct, SINT32 *pcm, UINT count) {
	(void)ct;
	(void)pcm;
	(void)count;
	ct->bufdatas = 0;
	if(g_sb16.dsp_info.dma.chan) g_sb16.dsp_info.dma.chan->leng.w = 0;
}
typedef void (SOUNDCALL * CT1741FN)(DMA_INFO *ct, SINT32 *pcm, UINT count);

static const CT1741FN ct1741fn[16] = {
			nomake,		// 0: STOP
			nomake,
			nomake,
			pcm8m,			// 8Bit Unsigned PCM mono　これしかチェックできておりません
			pcm8m,			// 8Bit Signed PCM mono
			Spcm16m,		//16Bit Signed PCM mono
			nomake,
			nomake,

			nomake,
			nomake,
			nomake,
			pcm8s,			// 8Bit Unsigned PCM stereo
			pcm8s,			// 8Bit UnSigned PCM stereo
			Spcm16s,		//16Bit signed PCM stereo
			nomake,
			nomake
};

static CT1741FN last_ct1741fn = nomake;

static void SOUNDCALL ct1741_getpcm(DMA_INFO *ct, SINT32 *pcm, UINT count) {
	// 再生用バッファに送る
	int idx = g_sb16.dsp_info.dma.mode|g_sb16.dsp_info.dma.stereo << 3;
	if(idx!=0){
		(*ct1741fn[idx])(ct, pcm, count);
		last_ct1741fn = ct1741fn[idx];
	}else if(ct->bufdatas >= BUF_ALIGN[idx]){
		(*last_ct1741fn)(ct, pcm, count);
	}else{
		(*ct1741fn[idx])(ct, pcm, count);
	}
}

void ct1741io_reset(void)
{
	ct1741_reset();
	g_sb16.dsp_info.state = DSP_STATUS_NORMAL;
	dmac_attach(DMADEV_CT1741, g_sb16.dmach);
	g_sb16.dsp_info.dma.bufsize = DMA_BUFSIZE;
	g_sb16.dsp_info.dma.rate2 = ct1741_np2_rate;
}

void ct1741io_bind(void)
{
	sound_streamregist(&g_sb16.dsp_info.dma, (SOUNDCB)ct1741_getpcm); // CT1741用 オーディオ再生ストリーム

	iocore_attachout(0x2600 + g_sb16.base, ct1741_write_reset);	/* DSP Reset */
	iocore_attachout(0x2C00 + g_sb16.base, ct1741_write_data);	/* DSP Write Command/Data */

	iocore_attachinp(0x2600 + g_sb16.base, ct1741_read_reset);	/* DSP Reset */
	iocore_attachinp(0x2a00 + g_sb16.base, ct1741_read_data);		/* DSP Read Data Port */
	iocore_attachinp(0x2c00 + g_sb16.base, ct1741_read_wstatus);	/* DSP Write Buffer Status (Bit 7) */
	iocore_attachinp(0x2d00 + g_sb16.base, ct1741_read_reset);	/* DSP Reset */
	iocore_attachinp(0x2e00 + g_sb16.base, ct1741_read_rstatus);	/* DSP Read Buffer Status (Bit 7) */
	iocore_attachinp(0x2f00 + g_sb16.base, ct1741_read_rstatus16);	/* DSP Read Buffer Status (Bit 7) */
}
void ct1741io_unbind(void)
{
	iocore_detachout(0x2600 + g_sb16.base);	/* DSP Reset */
	iocore_detachout(0x2C00 + g_sb16.base);	/* DSP Write Command/Data */

	iocore_detachinp(0x2600 + g_sb16.base);	/* DSP Reset */
	iocore_detachinp(0x2a00 + g_sb16.base);		/* DSP Read Data Port */
	iocore_detachinp(0x2c00 + g_sb16.base);	/* DSP Write Buffer Status (Bit 7) */
	iocore_detachinp(0x2e00 + g_sb16.base);	/* DSP Read Buffer Status (Bit 7) */
	iocore_detachinp(0x2f00 + g_sb16.base);	/* DSP Read Buffer Status (Bit 7) */
}

#endif
