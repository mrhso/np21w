/**
 * @file	cs4231c.c
 * @brief	Implementation of the CS4231
 */

#include "compiler.h"
#include "cs4231.h"
#include "iocore.h"
#include "fmboard.h"
#include "dmac.h"
#include "cpucore.h"
#ifndef CPU_STAT_PM
#define CPU_STAT_PM	0
#endif
#define CS4231_BUFREADSMP	128

	CS4231CFG	cs4231cfg;

	int calpenflag = 0; // XXX: CAL0����(0x04)��CAL0��PEN�������ɗ����(0x05)�ɑJ�ڂ���������������ς��饥� Win3.1+necpcm.drv�p�̂��̏ꂵ�̂�
	int w31play = 0; // XXX: CAL0����(0x04)��CAL0��PEN�������ɗ����(0x05)�ɑJ�ڂ���������������ς��饥� Win3.1+necpcm.drv�p�̂��̏ꂵ�̂�
	
	static int playcountsmp_Ictl = CS4231_BUFREADSMP; // �ϕ�����Ŗ��������T���v�����ǂނ悤�ɂ��饥�
	
// 1�T���v��������̃o�C�g���i���m����, �X�e���I�̏��j
static const SINT32 cs4231_playcountshift[16] = {
			1  ,		// 0: 8bit PCM
			1*2,
			1  ,		// 1: u-Law
			1  ,
			1*2,		// 2: 16bit PCM(little endian)?
			1*4,
			1  ,		// 3: A-Law
			1  ,
			1  ,		// 4:
			1  ,
			1  ,		// 5: ADPCM
			1  ,
			1*2,		// 6: 16bit PCM
			1*4,
			1  ,		// 7: ADPCM
			1  };

// Indirect Mapped Registers
enum {
	CS4231REG_LINPUT	= 0x00, // Left ADC Input Control (I0)
	CS4231REG_RINPUT	= 0x01, // Right ADC Input Control (I1)
	CS4231REG_AUX1L		= 0x02, // Left Auxiliary #1 Input Control (I2)
	CS4231REG_AUX1R		= 0x03, // Right Auxiliary #1 Input Control (I3)
	CS4231REG_AUX2L		= 0x04, // Left Auxiliary #2 Input Control (I4)
	CS4231REG_AUX2R		= 0x05, // Right Auxiliary #2 Input Control (I5)
	CS4231REG_LOUTPUT	= 0x06, // Left DAC Output Control (I6)
	CS4231REG_ROUTPUT	= 0x07, // Right DAC Output Control (I7)
	CS4231REG_PLAYFMT	= 0x08, // Fs and Playback Data Format (I8)
	CS4231REG_INTERFACE	= 0x09, // Interface Configuration (I9)
	CS4231REG_PINCTRL	= 0x0a, // Pin Control (I10)
	CS4231REG_TESTINIT	= 0x0b, // Error Status and Initialization (I11, Read Only)
	CS4231REG_MISCINFO	= 0x0c, // MODE and ID (I12)
	CS4231REG_LOOPBACK	= 0x0d, // Loopback Control (I13)
	CS4231REG_PLAYCNTM	= 0x0e, // Playback Upper Base (I14)
	CS4231REG_PLAYCNTL	= 0x0f, // Playback Lower Base (I15)

	CS4231REG_FEATURE1	= 0x10, // Alternate Feature Enable I (I16)
	CS4231REG_FEATURE2	= 0x11, // Alternate Feature Enable II (I17)
	CS4231REG_LLINEIN	= 0x12, // Left Line Input Control (I18)
	CS4231REG_RLINEIN	= 0x13, // Right Line Input Control (I19)
	CS4231REG_TIMERL	= 0x14, // Timer Lower Base (I20)
	CS4231REG_TIMERH	= 0x15, // Timer Upper Base (I21)
	CS4231REG_RESERVED1 = 0x16, // RESERVED (I22)
	CS4231REG_RESERVED2 = 0x17, // Alternate Feature Enable III (I23)
	CS4231REG_IRQSTAT	= 0x18, // Alternate Feature Status (I24)
	CS4231REG_VERSION	= 0x19, // Version / ID (I25)
	CS4231REG_MONOCTRL	= 0x1a, // Mono Input & Output Control (I26)
	CS4231REG_RESERVED3	= 0x1b, // RESERVED (I27)
	CS4231REG_RECFMT	= 0x1c, // Capture Data Format (I28)
	CS4231REG_PLAYFREQ	= 0x1d, // RESERVED (I29)
	CS4231REG_RECCNTM	= 0x1e, // Capture Upper Base (I30)
	CS4231REG_RECCNTL	= 0x1f  // Capture Lower Base (I31)
};


UINT dmac_getdata_(DMACH dmach, UINT8 *buf, UINT offset, UINT size);
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
				
void cs4231_initialize(UINT rate) {

	cs4231cfg.rate = rate;
}

void cs4231_reset(void) {

	ZeroMemory(&cs4231, sizeof(cs4231));
	cs4231.bufsize = CS4231_BUFFERS;
//	cs4231.proc = cs4231_nodecode;
	cs4231.dmach = 0xff;
	cs4231.dmairq = 0xff;
	cs4231.totalsample = 0;
	FillMemory(cs4231.port, sizeof(cs4231.port), 0xff);
}

void cs4231_update(void) {
}

// �p�~�Fcs4231g.c�Œ���
void cs4231_setvol(UINT vol) {

	(void)vol;
}

// CS4231 DMA����
void cs4231_dma(NEVENTITEM item) {

	DMACH	dmach;
	UINT	rem;
	UINT	pos;
	UINT	size;
	UINT	r = 0;
	//SINT32	cnt;
	if (item->flag & NEVENT_SETEVENT) {
		if (cs4231.dmach != 0xff) {
			dmach = dmac.dmach + cs4231.dmach;

			// �T�E���h�Đ��p�o�b�t�@�ɑ���H(cs4231g.c)
			sound_sync();

			// �o�b�t�@�ɋ󂫂�����΃f�[�^��ǂݏo��
			if(!w31play || !(cs4231.reg.featurestatus & (PI|TI|CI))){
				if (cs4231.bufsize * cs4231_playcountshift[cs4231.reg.datafmt >> 4] / 4 - 4 > cs4231.bufdatas) {
					rem = min(cs4231.bufsize - 4 - cs4231.bufdatas, CS4231_MAXDMAREADBYTES); //�ǂݎ��P�ʂ�16bit�X�e���I��1�T���v����(4byte)�ɂ��Ă����Ȃ��ƎG��������
					pos = cs4231.bufwpos & CS4231_BUFMASK; // �o�b�t�@�������݈ʒu
					size = min(rem, dmach->startcount); // �o�b�t�@�������݃T�C�Y
					r = dmac_getdata_(dmach, cs4231.buffer, pos, size); // DMA�ǂݎ����s
					cs4231.bufwpos = (cs4231.bufwpos + r) & CS4231_BUFMASK; // �o�b�t�@�������݈ʒu���X�V
					cs4231.bufdatas += r; // �o�b�t�@���̗L���ȃf�[�^�����X�V = (bufwpos-bufpos)&CS4231_BUFMASK
				}
			}
			// NEVENT���Z�b�g
			if (cs4231cfg.rate) {
				//SINT32 neventms;
				//int playcountsmpmax = (cs4231.reg.playcount[1]|(cs4231.reg.playcount[0] << 8)); // PI���荞�݂𔭐�������T���v����(Playback Base register)
				playcountsmp_Ictl += ((CS4231_BUFREADSMP - (int)r) / cs4231_playcountshift[cs4231.reg.datafmt >> 4])/2;
				if(playcountsmp_Ictl < 1)
					playcountsmp_Ictl = 1;
				if(playcountsmp_Ictl > CS4231_MAXDMAREADBYTES) 
					playcountsmp_Ictl = CS4231_MAXDMAREADBYTES;
				//int playcountsmp = min(playcountsmpmax, r / cs4231_playcountshift[cs4231.reg.datafmt >> 4])-4;
				//if(playcountsmp < CS4231_MINDMAREADBYTES) 
				//	playcountsmp = CS4231_MINDMAREADBYTES*2;

				//playcountsmp = min(max(r, CS4231_MAXDMAREADBYTES/4) / cs4231_playcountshift[cs4231.reg.datafmt >> 4], playcountsmp) / 2;
				//neventms = playcountsmp * 1000 / cs4231cfg.rate;
				//if(neventms <= 0) neventms = 1;
				//cnt = pccore.realclock / cs4231cfg.rate * 32;
				//nevent_set(NEVENT_CS4231, cnt, cs4231_dma, NEVENT_RELATIVE);
				//cnt = (UINT32)((UINT64)pccore.realclock * playcountsmp / cs4231cfg.rate / 10);
				//nevent_setbyms(NEVENT_CS4231, neventms, cs4231_dma, NEVENT_RELATIVE);
				nevent_set(NEVENT_CS4231, pccore.realclock / cs4231cfg.rate * playcountsmp_Ictl, cs4231_dma, NEVENT_RELATIVE);
			}

			//// NEVENT���Z�b�g
			//if (cs4231cfg.rate) {
			//	SINT32 neventms;
			//	int playcountsmp;// = (cs4231.reg.playcount[1]|(cs4231.reg.playcount[0] << 8)); // PI���荞�݂𔭐�������T���v����(Playback Base register)
			//	playcountsmp = 32 / cs4231_playcountshift[cs4231.reg.datafmt >> 4];//max(r, 32) / cs4231_playcountshift[cs4231.reg.datafmt >> 4];//min(max(r, 64) / cs4231_playcountshift[cs4231.reg.datafmt >> 4], playcountsmp) / 2; // ��
			//	//neventms = playcountsmp * 1000 / cs4231cfg.rate;
			//	//if(neventms <= 0) neventms = 1;
			//	//cnt = pccore.realclock / cs4231cfg.rate * 32;
			//	//nevent_set(NEVENT_CS4231, cnt, cs4231_dma, NEVENT_RELATIVE);
			//	//cnt = (UINT32)((UINT64)pccore.realclock * playcountsmp / cs4231cfg.rate / 10);
			//	//nevent_setbyms(NEVENT_CS4231, neventms, cs4231_dma, NEVENT_RELATIVE);
			//	nevent_set(NEVENT_CS4231, pccore.realclock / cs4231cfg.rate * playcountsmp, cs4231_dma, NEVENT_RELATIVE);
			//}
		}
	}
	(void)item;
}

// PIO�Đ��p
void cs4231_datasend(REG8 dat) {
	UINT	pos;
	if (cs4231.reg.iface & PPIO) {		// PIO play enable
		if (cs4231.bufsize <= cs4231.bufdatas) {
			sound_sync();
		}
		if (cs4231.bufsize > cs4231.bufdatas) {
			pos = (cs4231.bufwpos) & CS4231_BUFMASK;
			cs4231.buffer[pos] = dat;
			cs4231.bufdatas++;
			cs4231.bufwpos = (cs4231.bufwpos + 1) & CS4231_BUFMASK;
		}
	}
}

// DMA�Đ��J�n�E�I���E���f���ɌĂ΂��i����j
REG8 DMACCALL cs4231dmafunc(REG8 func) {
	SINT32	cnt;
	switch(func) {
		case DMAEXT_START:
			if (cs4231cfg.rate) {
				int playcount = (cs4231.reg.playcount[1]|(cs4231.reg.playcount[0] << 8)) * cs4231_playcountshift[cs4231.reg.datafmt >> 4]; // PI���荞�݂𔭐�������T���v����(Playback Base register) * �T���v��������̃o�C�g��
				// DMA�ǂݎ�萔�J�E���^��������
				cs4231.totalsample = 0; 

				// DMA�ǂݎ�菈���J�n(NEVENT�Z�b�g)
				//nevent_setbyms(NEVENT_CS4231, CS4231_MAXDMAREADBYTES * 1000 / cs4231cfg.rate, cs4231_dma, NEVENT_ABSOLUTE);
				//cnt = pccore.realclock / cs4231cfg.rate * 512;
				//nevent_set(NEVENT_CS4231, cnt, cs4231_dma, NEVENT_ABSOLUTE);
				playcountsmp_Ictl = CS4231_BUFREADSMP;
				cnt = pccore.realclock / cs4231cfg.rate * playcountsmp_Ictl;
				nevent_set(NEVENT_CS4231, cnt, cs4231_dma, NEVENT_ABSOLUTE);
			}
			break;
		case DMAEXT_END:
			// �����ł̊��荞�݂͗v��Ȃ��H
			//if ((cs4231.reg.pinctrl & IEN) && (cs4231.dmairq != 0xff)) {
			//	cs4231.intflag |= INt;
			//	cs4231.reg.featurestatus |= PI;
			//	pic_setirq(cs4231.dmairq);
			//}
			break;

		case DMAEXT_BREAK:
			// DMA�ǂݎ�菈���I��(NEVENT����)
			nevent_reset(NEVENT_CS4231);
			break;

	}
	return(0);
}

// �o�b�t�@�ʒu�̃Y���C���p�i�G�����h�~�j
static void setdataalign(void) {

	UINT	step;
	
	// �o�b�t�@�ʒu���Y���Ă�����C���i4byte�P�ʂɁj
	step = (0 - cs4231.bufpos) & 3;
	if (step) {
		cs4231.bufpos += step;
		cs4231.bufdatas -= min(step, cs4231.bufdatas);
	}
	cs4231.bufdatas &= ~3;
	step = (0 - cs4231.bufwpos) & 3;
	if (step) {
		cs4231.bufwpos += step;
	}
}

// CS4231 Indexed Data register��WRITE����
void cs4231_control(UINT idx, REG8 dat) {
	UINT8	modify;
	DMACH	dmach;
	switch(idx){
	case 0x2: // Left Auxiliary #1 Input Control
		if(g_nSoundID==SOUNDID_WAVESTAR){
			UINT i;
			if(dat >= 0x10) dat = 15;
			cs4231.devvolume[0xff] = (~dat) & 15;
			opngen_setvol(np2cfg.vol_fm * cs4231.devvolume[0xff] / 15);
			psggen_setvol(np2cfg.vol_ssg * cs4231.devvolume[0xff] / 15);
			rhythm_setvol(np2cfg.vol_rhythm * cs4231.devvolume[0xff] / 15);
#if defined(SUPPORT_FMGEN)
			if(np2cfg.usefmgen) {
				opna_fmgen_setallvolumeFM_linear(np2cfg.vol_fm * cs4231.devvolume[0xff] / 15);
				opna_fmgen_setallvolumePSG_linear(np2cfg.vol_ssg * cs4231.devvolume[0xff] / 15);
				opna_fmgen_setallvolumeRhythmTotal_linear(np2cfg.vol_rhythm * cs4231.devvolume[0xff] / 15);
			}
#endif
			for (i = 0; i < _countof(g_opna); i++)
			{
				rhythm_update(&g_opna[i].rhythm);
			}
		}
		break;
	case 0x3: // Right Auxiliary #1 Input Control
		if(g_nSoundID==SOUNDID_WAVESTAR){
			// XXX: �{���͍��E�̃{�����[���������K�v
		}
		break;
	case 0xd:
		break;
	case 0xc:
		// MODE and ID (I12)
		dat &= 0x40;
		dat |= 0x8a;
		break;
	case 0xb://ErrorStatus 
	case 0x19://Version ID
		return;
	case CS4231REG_IRQSTAT:
		// �o�b�t�@�I�[�o�[�����E�A���_�[�����⊄�荞�݂̏�Ԃ�\�����W�X�^�@Alternate Feature Status (I24)
		// 0���Z�b�g�����r�b�g���������i1�̏ꍇ�͂��̂܂܁j
		modify = ((UINT8 *)&cs4231.reg)[idx] & (~(dat|0x0f));
		((UINT8 *)&cs4231.reg)[idx] &= dat|0x0f;
		if (modify & (PI|TI|CI)) {
			// PI,TI,CI�r�b�g���S�ď�������Ă����犄�荞�݉���
			if(((((UINT8 *)&cs4231.reg)[idx]) & (PI|TI|CI)) == 0){
				pic_resetirq(cs4231.dmairq);
				cs4231.intflag &= ~INt;
			}
		}
        return; // ���Ƃ͏������Ⴄ�̂Ŕ�����
	default:
		break;

	}
	dmach = dmac.dmach + cs4231.dmach;
	modify = ((UINT8 *)&cs4231.reg)[idx] ^ dat; // �ύX���ꂽ�r�b�g���擾
	((UINT8 *)&cs4231.reg)[idx] = dat; // ���W�X�^�l��V�����l�ɕύX
	switch(idx) {
	case CS4231REG_PLAYFMT:
		// �Đ��t�H�[�}�b�g�ݒ�Ƃ��@Fs and Playback Data Format (I8)
		if (modify & 0xf0) {
			//dmach->adrs.d = dmach->startaddr;
			cs4231.bufpos = cs4231.bufwpos;
			cs4231.bufdatas = 0;
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
		// �Đ��^���̗L�������Ƃ�DMA�Ƃ��̐ݒ�@Interface Configuration (I9)
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
		// XXX: CAL0����(0x04)��CAL0��PEN�������ɗ����(0x05)�ɑJ�ڂ���������������ς��饥� Win3.1+necpcm.drv�p�̂��̏ꂵ�̂�
		if(((UINT8 *)&cs4231.reg)[idx] == 0x05 && calpenflag == 1){
			calpenflag = 2;
			w31play = 1;
		}else if(((UINT8 *)&cs4231.reg)[idx] == 0x04){
			calpenflag = 1;
			w31play = 0;
		}else{
			calpenflag = 0;
			w31play = 0;
		}
		break;
	}
}

// CS4231 DMA�f�[�^�ǂݎ��
UINT dmac_getdata_(DMACH dmach, UINT8 *buf, UINT offset, UINT size) {
	UINT	leng; // �ǂݎ�萔
	UINT	lengsum; // ���v�ǂݎ�萔
	UINT32	addr;
	UINT	i;
	SINT32	sampleirq = 0; // ���荞�݂܂łɕK�v�ȃf�[�^�]����(byte)
#define PLAYCOUNT_ADJUST_VALUE	32768
	static UINT32	playcount_adjustcounter = 0;
	
	lengsum = 0;
	while(size > 0) {
		leng = min(dmach->leng.w, size);
		if (leng) {
			int playcount = ((cs4231.reg.playcount[1]|(cs4231.reg.playcount[0] << 8))) * cs4231_playcountshift[cs4231.reg.datafmt >> 4]; // PI���荞�݂𔭐�������T���v����(Playback Base register) * �T���v��������̃o�C�g��
			if(cs4231.totalsample + (SINT32)leng > playcount){
				// DMA�Đ��T���v�����J�E���^(Playback DMA count register)��PI���荞�݂𔭐�������T���v����(Playback Base register)�𒴂��Ȃ��悤�ɒ���
				leng = playcount - cs4231.totalsample;
			}

			addr = dmach->adrs.d; // ���݂̃������ǂݎ��ʒu
			if (!(dmach->mode & 0x20)) {			// dir +
				// +������DMA�]��
				for (i=0; i<leng ; i++) {
					buf[offset] = MEMP_READ8(addr); // DMA MEM -> CS4231 BUFFER
					addr++;
					if(addr > dmach->lastaddr){
						addr = dmach->startaddr; // DMA�ǂݎ��A�h���X���A�h���X�͈͂̍Ō�ɓ��B������ŏ��ɖ߂�
					}
					offset = (offset+1) & CS4231_BUFMASK; // DMA�f�[�^�ǂݎ��o�b�t�@�̏������݈ʒu��i�߂�i���Ō�ɓ��B������ŏ��ɖ߂��j
				}

				// XXX: �Đ��ʒu�����iWin9x,Win2000�Đ��m�C�Y�΍��p�E�Ƃ肠����+���������j
				playcount_adjustcounter += leng;
				if(playcount_adjustcounter >= PLAYCOUNT_ADJUST_VALUE){
					playcount_adjustcounter -= PLAYCOUNT_ADJUST_VALUE;
					if(!w31play){
						addr += 4;
						if(addr > dmach->lastaddr){
							addr = dmach->startaddr + (addr - dmach->lastaddr - 1); // DMA�ǂݎ��A�h���X���A�h���X�͈͂̍Ō�ɓ��B������ŏ��ɖ߂�
						}
					}
				}

				dmach->adrs.d = addr; // DMA�ǂݎ��A�h���X���݈ʒu���X�V
			}
			else {									// dir -
				// -������DMA�]��
				for (i=0; i<leng; i++) {
					buf[offset] = MEMP_READ8(addr); // DMA MEM -> CS4231 BUFFER
					addr--;
					if(addr < dmach->startaddr){
						addr = dmach->lastaddr; // DMA�ǂݎ��A�h���X���A�h���X�͈͂̍ŏ��ɓ��B������Ō�ɖ߂�
					}
					offset = (offset+1) & CS4231_BUFMASK; // DMA�f�[�^�ǂݎ��o�b�t�@�̏������݈ʒu��i�߂�i���Ō�ɓ��B������ŏ��ɖ߂��j
				}
				playcount_adjustcounter = (playcount_adjustcounter+leng) % PLAYCOUNT_ADJUST_VALUE;
				dmach->adrs.d = addr;
			}

			// �ǂݎ��o�C�g������dmach->leng.w�����炷�i0�ȉ��ɂȂ�����dmach->startcount�ɖ߂��j
			if (dmach->leng.w <= leng) {
				dmach->leng.w = dmach->leng.w + dmach->startcount - leng; // �߂�
				dmach->proc.extproc(DMAEXT_END);
			}else{
				dmach->leng.w -= leng;
			}

			// �ǂݎ�萔�Ǝc�萔�X�V
			lengsum += leng;
			size -= leng;
			
			// �ǂݎ�萔�J�E���g
			cs4231.totalsample += leng;
			
			// DMA�Đ��o�C�g���J�E���^(Playback DMA count register)��PI���荞�݂𔭐�������o�C�g���ɂȂ�����PI���荞�݂𔭐�������
			if(cs4231.totalsample >= playcount){
				cs4231.totalsample -= playcount;
				// ���荞�݂��L���ȏꍇ���荞�݂𔭐�������
				if ((cs4231.reg.pinctrl & IEN) && (cs4231.dmairq != 0xff)) {
					// XXX: CAL0����(0x04)��CAL0��PEN�������ɗ����(0x05)�ɑJ�ڂ���������������ς��饥� Win3.1+necpcm.drv�p�̂��̏ꂵ�̂�
					if(calpenflag != 2){
						cs4231.intflag |= INt; // ���荞�ݒ�(Interrupt Status)�r�b�g���Z�b�g
						cs4231.reg.featurestatus |= PI; // PI(Playback Interrupt)�r�b�g���Z�b�g
						pic_setirq(cs4231.dmairq); // ���荞�݂𔭐�������
					}
					calpenflag = 0;
				}
				break;
			}
		}else{
			break;
		}
	}

	return(lengsum);
}

