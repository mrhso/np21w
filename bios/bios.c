/**
 * @file	bios.c
 * @brief	Implementation of BIOS
 */

#include "compiler.h"
#include "bios.h"
#include "biosmem.h"
#include "sxsibios.h"
#include "strres.h"
#include "cpucore.h"
#include "pccore.h"
#include "iocore.h"
#include "lio/lio.h"
#include "vram.h"
#include "diskimage/fddfile.h"
#include "fdd/fdd_mtr.h"
#include "fdfmt.h"
#include "dosio.h"
#include "keytable.res"
#include "itfrom.res"
#include "startup.res"
#include "biosfd80.res"
#if defined(SUPPORT_IDEIO)
#include	"fdd/sxsi.h"
#include	"cbus/ideio.h"
#endif
#if defined(SUPPORT_HRTIMER)
#include	"timemng.h"
#endif
#include	"fmboard.h"

#if defined(SUPPORT_IA32_HAXM)
#include	"i386hax/haxfunc.h"
#include	"i386hax/haxcore.h"
#endif

#define	BIOS_SIMULATE

static const char neccheck[] = "Copyright (C) 1983 by NEC Corporation";

typedef struct {
	UINT8	port;
	UINT8	data;
} IODATA;

static const IODATA iodata[] = {
			// DMA
				{0x29, 0x00}, {0x29, 0x01}, {0x29, 0x02}, {0x29, 0x03},
				{0x27, 0x00}, {0x21, 0x00}, {0x23, 0x00}, {0x25, 0x00},
				{0x1b, 0x00}, {0x11, 0x40},

			// PIT
				{0x77, 0x30}, {0x71, 0x00}, {0x71, 0x00},
				{0x77, 0x76}, {0x73, 0xcd}, {0x73, 0x04},
				{0x77, 0xb6},

			// PIC
				{0x00, 0x11}, {0x02, 0x08}, {0x02, 0x80}, {0x02, 0x1d},
				{0x08, 0x11}, {0x0a, 0x10}, {0x0a, 0x07}, {0x0a, 0x09},
				{0x02, 0x7d}, {0x0a, 0x71}};

static const UINT8 msw_default[8] =
							{0x48, 0x05, 0x04, 0x00, 0x01, 0x00, 0x00, 0x6e};

BIOSHOOKINFO	bioshookinfo;
#if defined(BIOS_IO_EMULATION)
BIOSIOEMU	biosioemu; // np21w ver0.86 rev46 BIOS I/O emulation
#endif

// BIOS�t�b�N���߂��f�t�H���g�ݒ�iNOP���� 0x90�j���珑��������
static void bios_updatehookinst(UINT8 *mem, UINT32 updatesize) {

	UINT32 i;

	if(bioshookinfo.hookinst == HOOKINST_DEFAULT) return; // ���������s�v

	// XXX: ���߂������ƌ��ď��������Ȃ��Ƃ�΂�
	for(i=0;i<updatesize-1;i++){
		if(*mem == HOOKINST_DEFAULT){
			// ���̖��߂�NOP, STI, RET, IRET���ۂ���Ώ��������i�Ԉ���ď���������̂�����j
			if(*(mem+1) == 0x90 || *(mem+1) == 0xFB || *(mem+1) == 0xC2 || *(mem+1) == 0xC3 || *(mem+1) == 0xCB || *(mem+1) == 0xCA || *(mem+1) == 0xCF || *(mem+1) == 0xE8 || *(mem+1) == 0xE9 || *(mem+1) == 0xEE){
				*mem = bioshookinfo.hookinst;
			}else if(*(mem+1) == 0x51 && *(mem+2) == 0xB9){
				*mem = bioshookinfo.hookinst;
			}else if(*(mem+1) == 0xec && *(mem+2) == 0x3c){
				*mem = bioshookinfo.hookinst;
			}else{
				*mem = 0x90;
			}
		}
		mem++;
	}
}

static void bios_itfprepare(void) {

const IODATA	*p;
const IODATA	*pterm;

	crtc_biosreset();
	gdc_biosreset();

	p = iodata;
	pterm = iodata + NELEMENTS(iodata);
	while(p < pterm) {
		iocore_out8(p->port, p->data);
		p++;
	}
}

static void bios_memclear(void) {

	ZeroMemory(mem, 0xa0000);
	ZeroMemory(mem + 0x100000, 0x10000);
	if (CPU_EXTMEM) {
		ZeroMemory(CPU_EXTMEM, CPU_EXTMEMSIZE);
	}
	bios0x18_16(0x20, 0xe1);
	ZeroMemory(mem + VRAM0_B, 0x18000);
	ZeroMemory(mem + VRAM0_E, 0x08000);
	ZeroMemory(mem + VRAM1_B, 0x18000);
	ZeroMemory(mem + VRAM1_E, 0x08000);
#if defined(SUPPORT_PC9821)
	ZeroMemory(vramex, sizeof(vramex));
#endif
}

static void bios_reinitbyswitch(void) {

	UINT8	prxcrt;
	UINT8	prxdupd;
	UINT8	biosflag;
	UINT16	extmem; // LARGE_MEM //UINT16	extmem;
	UINT8	boot;
	//FILEH	fh;
	//OEMCHAR	path[MAX_PATH];

	if (!(pccore.dipsw[2] & 0x80)) {
#if defined(CPUCORE_IA32)
		mem[MEMB_SYS_TYPE] = 0x03;		// 80386�`
#else
		mem[MEMB_SYS_TYPE] = 0x01;		// 80286
#endif
	}
	else {
		mem[MEMB_SYS_TYPE] = 0x00;		// V30
	}

	mem[MEMB_BIOS_FLAG0] = 0x01;
	prxcrt = 0x08;
	if (!(pccore.dipsw[0] & 0x01)) {			// dipsw1-1 on
		prxcrt |= 0x40;
	}
	if (gdc.display & (1 << GDCDISP_ANALOG)) {
		prxcrt |= 0x04;							// color16
	}
	if (!(pccore.dipsw[0] & 0x80)) {			// dipsw1-8 on
		prxcrt |= 0x01;
	}
	if (grcg.chip) {
		prxcrt |= 0x02;
	}
	mem[MEMB_PRXCRT] = prxcrt;

	prxdupd = 0x18;
	if (grcg.chip >= 3) {
		prxdupd |= 0x40;
	}
	if (!(pccore.dipsw[1] & 0x80)) {			// dipsw2-8 on
		prxdupd |= 0x20;
	}
	mem[MEMB_PRXDUPD] = prxdupd;

	biosflag = 0x20;
	if (pccore.cpumode & CPUMODE_8MHZ) {
		biosflag |= 0x80;
	}
	biosflag |= mem[0xa3fea] & 7;
	if (pccore.dipsw[2] & 0x80) {
		biosflag |= 0x40;
	}
	mem[MEMB_BIOS_FLAG1] = biosflag;
	extmem = pccore.extmem;
	extmem = min(extmem, 14);
	mem[MEMB_EXPMMSZ] = (UINT8)(extmem << 3);
	if (pccore.extmem >= 15) {
		//mem[0x0594] = pccore.extmem - 15;
		STOREINTELWORD((mem+0x0594), (pccore.extmem - 15));
	}
	mem[MEMB_CRT_RASTER] = 0x0f;

	// FDD initialize
	SETBIOSMEM32(MEMD_F2DD_POINTER, 0xfd801ad7);
	SETBIOSMEM32(MEMD_F2HD_POINTER, 0xfd801aaf);
	boot = mem[MEMB_MSW5] & 0xf0;
	if (boot != 0x20) {		// 1MB
		fddbios_equip(3, TRUE);
		mem[MEMB_BIOS_FLAG0] |= 0x02;
	}
	else {					// 640KB
		fddbios_equip(0, TRUE);
		mem[MEMB_BIOS_FLAG0] &= ~0x02;
	}
	mem[MEMB_F2HD_MODE] = 0xff;
	mem[MEMB_F2DD_MODE] = 0xff;

#if defined(SUPPORT_CRT31KHZ)
	mem[MEMB_CRT_BIOS] |= 0x80;
#endif
#if defined(SUPPORT_PC9821)
	mem[MEMB_CRT_BIOS] |= 0x04;		// 05/02/03
	mem[0x45c] = 0x40;
	
#if defined(SUPPORT_IDEIO)
	mem[0xF8E80+0x0010] = (sxsi_getdevtype(3)!=SXSIDEV_NC ? 0x8 : 0x0)|(sxsi_getdevtype(2)!=SXSIDEV_NC ? 0x4 : 0x0)|
						  (sxsi_getdevtype(1)!=SXSIDEV_NC ? 0x2 : 0x0)|(sxsi_getdevtype(0)!=SXSIDEV_NC ? 0x1 : 0x0);

	if(np2cfg.winntfix){
		// WinNT4.0��HDD���F������悤�ɂȂ�i������Win9x�ł�HDD�F�����s�̊����Y���ɂȂ���CD���F�����Ȃ��Ȃ�j
		mem[0x05ba] = (sxsi_getdevtype(3)==SXSIDEV_HDD ? 0x8 : 0x0)|(sxsi_getdevtype(2)==SXSIDEV_HDD ? 0x4 : 0x0)|
					  (sxsi_getdevtype(1)==SXSIDEV_HDD ? 0x2 : 0x0)|(sxsi_getdevtype(0)==SXSIDEV_HDD ? 0x1 : 0x0);
	}
	
	mem[0x45B] |= 0x80; // XXX: TEST OUT 5Fh,AL wait
#endif
	mem[0xF8E80+0x0011] = mem[0xF8E80+0x0011] & ~0x20; // 0x20�̃r�b�g��ON����Win2000�Ń}�E�X���J�N�J�N����H
	if(np2cfg.modelnum) mem[0xF8E80+0x003F] = np2cfg.modelnum; // PC-9821 Model Number
	
#endif
	
#if defined(SUPPORT_PCI)
	mem[0xF8E80+0x0004] |= 0x2c;
	//mem[0x5B7] = (0x277 >> 2); // READ_DATA port address
	mem[0x5B8] = 0x00; // No C-Bus PnP boards
#endif
	
#if defined(SUPPORT_HRTIMER)
	{
		_SYSTIME hrtimertime;
		UINT32 hrtimertimeuint;

		timemng_gettime(&hrtimertime);
		hrtimertimeuint = (((UINT32)hrtimertime.hour*60 + (UINT32)hrtimertime.minute)*60 + (UINT32)hrtimertime.second)*32 + ((UINT32)hrtimertime.milli*32)/1000;
		hrtimertimeuint |= 0x400000; // �������Ȃ���Win98�̎��v��1�������?
		STOREINTELDWORD(mem+0x04F1, hrtimertimeuint); // XXX: 04F4�ɂ�����������Ă邯�Ǎ����������Ă͖��Ȃ������Ȃ̂ť��
	}
#endif	/* defined(SUPPORT_HRTIMER) */

#if defined(SUPPORT_PC9801_119)
	mem[MEMB_BIOS_FLAG3] |= 0x40;
#endif	/* defined(SUPPORT_PC9801_119) */

	// FDC
	if (fdc.support144) {
		mem[MEMB_F144_SUP] |= fdc.equip;
	}

	// IDE initialize
	if (pccore.hddif & PCHDD_IDE) {
		mem[MEMB_SYS_TYPE] |= 0x80;		// IDE
		CPU_AX = 0x8300;
		sasibios_operate();
		//mem[0x457] = 0x97; // 10010111
	}
}

static void bios_vectorset(void) {

	UINT	i;

	for (i=0; i<0x20; i++) {
		*(UINT16 *)(mem + (i*4)) = *(UINT16 *)(mem + BIOS_BASE + BIOS_TABLE + (i*2));
		SETBIOSMEM16((i * 4) + 2, BIOS_SEG);
	}
	SETBIOSMEM32(0x1e*4, 0xe8000000);
}

static void bios_screeninit(void) {

	REG8	al;

	al = 4;
	al += (pccore.dipsw[1] & 0x04) >> 1;
	al += (pccore.dipsw[1] & 0x08) >> 3;
	bios0x18_0a(al);
}

static void setbiosseed(UINT8 *ptr, UINT size, UINT seedpos) {

	UINT8	x;
	UINT8	y;
	UINT	i;

	x = 0;
	y = 0;
	for (i=0; i<size; i+=2) {
		x += ptr[i + 0];
		y += ptr[i + 1];
	}
	ptr[seedpos + 0] -= x;
	ptr[seedpos + 1] -= y;
}

void bios_initialize(void) {

	BOOL	biosrom;
	OEMCHAR	path[MAX_PATH];
	FILEH	fh;
	UINT	i, j;
	UINT32	tmp;
	UINT	pos;
	
#if defined(SUPPORT_IA32_HAXM)
	if (np2hax.enable) {
		bioshookinfo.hookinst = 0xCC;//0xF4;//;0xCC;//HOOKINST_DEFAULT; // BIOS�t�b�N�Ɏg�����߁i�f�t�H���g��NOP���߂��t�b�N�j
	}else
#endif
	{
		bioshookinfo.hookinst = HOOKINST_DEFAULT; // BIOS�t�b�N�Ɏg�����߁i�f�t�H���g��NOP���߂��t�b�N�j
	}

#if defined(BIOS_IO_EMULATION)
	// np21w ver0.86 rev46 BIOS I/O emulation
	memset(&biosioemu, 0, sizeof(biosioemu));
	biosioemu.enable = np2cfg.biosioemu;
#endif

	biosrom = FALSE;
	getbiospath(path, str_biosrom, NELEMENTS(path));
	if(np2cfg.usebios){
		fh = file_open_rb(path);
		if (fh != FILEH_INVALID) {
			biosrom = (file_read(fh, mem + 0x0e8000, 0x18000) == 0x18000);
			file_close(fh);
		}
	}
	if (biosrom) {
		TRACEOUT(("load bios.rom"));
		pccore.rom |= PCROM_BIOS;
		// PnP BIOS��ׂ�
		for (i=0; i<0x10000; i+=0x10) {
			tmp = LOADINTELDWORD(mem + 0xf0000 + i);
			if (tmp == 0x506e5024) {
				TRACEOUT(("found PnP BIOS at %.5x", 0xf0000 + i));
				mem[0xf0000 + i] = 0x6e;
				mem[0xf0002 + i] = 0x24;
				break;
			}
		}
	}
	else {
		CopyMemory(mem + 0x0e8000, nosyscode, sizeof(nosyscode));
		if ((!biosrom) && (!(pccore.model & PCMODEL_EPSON))) {
			CopyMemory(mem + 0xe8dd8, neccheck, 0x25);
			pos = LOADINTELWORD(itfrom + 2);
			CopyMemory(mem + 0xf538e, itfrom + pos, 0x27);
		}
		setbiosseed(mem + 0x0e8000, 0x10000, 0xb1f0);
	}

#if defined(SUPPORT_PC9821)
	// ideio.c�ֈړ�
	//getbiospath(path, OEMTEXT("bios9821.rom"), NELEMENTS(path));
	//fh = file_open_rb(path);
	//if (fh != FILEH_INVALID) {
	//	if (file_read(fh, mem + 0x0d8000, 0x2000) == 0x2000) {
	//		// IDE BIOS��ׂ�
	//		TRACEOUT(("load bios9821.rom"));
	//		STOREINTELWORD(mem + 0x0d8009, 0);
	//	}
	//	file_close(fh);
	//}
#if defined(BIOS_SIMULATE)
	mem[0xf8e80] = 0x98;
	mem[0xf8e81] = 0x21;
	mem[0xf8e82] = 0x1f;
	mem[0xf8e83] = 0x20;	// Model Number?
	mem[0xf8e84] = 0x2c;
	mem[0xf8e85] = 0xb0;

	// mem[0xf8eaf] = 0x21;		// <- ������ĉ��������H
#endif
#endif

#if defined(BIOS_SIMULATE)
	CopyMemory(mem + BIOS_BASE, biosfd80, sizeof(biosfd80));
	if (!biosrom) {
		lio_initialize();
	}

	for (i=0; i<8; i+=2) {
		STOREINTELWORD(mem + 0xfd800 + 0x1aaf + i, 0x1ab7);
		STOREINTELWORD(mem + 0xfd800 + 0x1ad7 + i, 0x1adf);
		STOREINTELWORD(mem + 0xfd800 + 0x2361 + i, 0x1980);
	}
	CopyMemory(mem + 0xfd800 + 0x1ab7, fdfmt2hd, sizeof(fdfmt2hd));
	CopyMemory(mem + 0xfd800 + 0x1adf, fdfmt2dd, sizeof(fdfmt2dd));
	CopyMemory(mem + 0xfd800 + 0x1980, fdfmt144, sizeof(fdfmt144));

	SETBIOSMEM16(0xfffe8, 0xcb90);
	SETBIOSMEM16(0xfffec, 0xcb90);
	mem[0xffff0] = 0xea;
	STOREINTELDWORD(mem + 0xffff1, 0xfd800000);

	CopyMemory(mem + 0x0fd800 + 0x0e00, keytable[0], 0x300);
	
	//fh = file_create_c(_T("emuitf.rom"));
	//if (fh != FILEH_INVALID) {
	//	file_write(fh, itfrom, sizeof(itfrom));
	//	file_close(fh);
	//	TRACEOUT(("write emuitf.rom"));
	//}
	CopyMemory(mem + ITF_ADRS, itfrom, sizeof(itfrom)+1);
#if defined(SUPPORT_FAST_MEMORYCHECK)
	// �����������`�F�b�N
	if(np2cfg.memcheckspeed > 1){
		// �L�������`�F�b�N��DEC r/m16 �������t�b�N(���s���̓A�h���X���ς��̂Œ���)
		STOREINTELWORD((mem + ITF_ADRS + 5886), 128 * np2cfg.memcheckspeed);
		mem[ITF_ADRS + 5924] = bioshookinfo.hookinst;
	}
#endif
	np2cfg.memchkmx = 0; // ������ (obsolete)
	if(np2cfg.memchkmx){ // �������J�E���g�ő�l�ύX
		mem[ITF_ADRS + 6057] = mem[ITF_ADRS + 6061] = (UINT8)max((int)np2cfg.memchkmx-14, 1); // XXX: �ꏊ���ߑł�
	}else{
#if defined(SUPPORT_LARGE_MEMORY)
		if(np2cfg.EXTMEM >= 256){ // ��e�ʃ������J�E���g
			// XXX: �ꏊ���ߑł�����
			for(i=ITF_ADRS + 6066; i >= ITF_ADRS + 6058; i--){
				mem[i] = mem[i-1]; // 1byte���炵
			}
			mem[ITF_ADRS + 6067] = mem[ITF_ADRS + 6068] = bioshookinfo.hookinst; // call	WAITVSYNC �� NOP��
			mem[ITF_ADRS + 6055] = 0x81; // CMP r/m16, imm8 �� CMP r/m16, imm16 �ɕς���
			STOREINTELWORD((mem + ITF_ADRS + 6057), (MEMORY_MAXSIZE-14)); // cmp�@bx, (EXTMEMORYMAX - 16)�̕�����������
			STOREINTELWORD((mem + ITF_ADRS + 6061+1), (MEMORY_MAXSIZE-14)); // mov�@bx, (EXTMEMORYMAX - 16)�̕�����������i1byte���炵���̂ŃI�t�Z�b�g���Ӂj
		}
#endif
	}
	if(np2cfg.sbeeplen || np2cfg.sbeepadj){ // �s�|�������ύX
		UINT16 beeplen = (np2cfg.sbeeplen ? np2cfg.sbeeplen : mem[ITF_ADRS + 5553]); // XXX: �ꏊ���ߑł�
		if(np2cfg.sbeepadj){ // ��������
			beeplen = beeplen * np2cfg.multiple / 10;
			if(beeplen == 0) beeplen = 1;
			if(beeplen > 255) beeplen = 255;
		}
#if defined(SUPPORT_IA32_HAXM)
		if (np2hax.enable && np2cfg.sbeepadj) {
			mem[ITF_ADRS + 5553] = 255;
		}else
#endif
		mem[ITF_ADRS + 5553] = (UINT8)beeplen; // XXX: �ꏊ���ߑł�
	}
	mem[ITF_ADRS + 0x7ff0] = 0xea;
	STOREINTELDWORD(mem + ITF_ADRS + 0x7ff1, 0xf8000000);
	if (pccore.model & PCMODEL_EPSON) {
		mem[ITF_ADRS + 0x7ff1] = 0x04;
	}
	else if ((pccore.model & PCMODELMASK) == PCMODEL_VM) {
		mem[ITF_ADRS + 0x7ff1] = 0x08;
	}

	setbiosseed(mem + 0x0f8000, 0x08000, 0x7ffe);
#else
	fh = file_open_c("itf.rom");
	if (fh != FILEH_INVALID) {
		file_read(fh, mem + ITF_ADRS, 0x8000);
		file_close(fh);
		TRACEOUT(("load itf.rom"));
	}
#endif

	CopyMemory(mem + 0x1c0000, mem + ITF_ADRS, 0x08000);
	CopyMemory(mem + 0x1e8000, mem + 0x0e8000, 0x10000);
	
#if defined(SUPPORT_PCI)
	// PCI BIOS32 Service Directory��T��
	for (i=0; i<0x10000; i+=0x4) {
		tmp = LOADINTELDWORD(mem + 0xf0000 + i);
		if (tmp == 0x5F32335F) { // "_32_"
			UINT8 checksum = 0;
			for(j=0;j<16;j++){
				checksum += mem[0xf0000 + i + j];
			}
			if(checksum==0){
				// ���������ꍇ�͂��̈ʒu���g��
				TRACEOUT(("found BIOS32 Service Directory at %.5x", 0xf0000 + i));
				pcidev.bios32svcdir = 0xf0000 + i;
				pcidev_updateBIOS32data();
				break;
			}
		}
	}
	if(i==0x10000){
		int emptyflag = 0;
		TRACEOUT(("BIOS32 Service Directory not found."));
		
		// PCI BIOS32 Service Directory�����蓖�Ă�
		// XXX: �������̕ӂȂ�󂢂Ă邾��[�Ƃ������Ƃť��
		pcidev.bios32svcdir = 0xffa00;
		for(i=0;i<0x400;i+=0x10){
			emptyflag = 1;
			// 16byte���󂢂Ă邩�`�F�b�N�i0������Ƃ����ċ󂢂Ă�Ƃ͌���Ȃ����ǥ���j
			for(j=0;j<16;j++){
				if(mem[pcidev.bios32svcdir+i+j] != 0){
					emptyflag = 0;
				}
			}
			if(emptyflag){
				// BIOS32 Service Directory��u��
				TRACEOUT(("Allocate BIOS32 Service Directory at 0x%.5x", pcidev.bios32svcdir));
				pcidev.bios32svcdir += i;
				pcidev_updateBIOS32data();
				break;
			}
		}
		if(i==0x400){
			// �󂫂��Ȃ��̂�BIOS32 Service Directory��u�������
			TRACEOUT(("Error: Cannot allocate memory for BIOS32 Service Directory."));
			pcidev.bios32svcdir = 0;
		}
	}
#endif
	
// np21w ver0.86 rev46 BIOS I/O emulation
#if defined(BIOS_IO_EMULATION)
	// �G�~�����[�V�����p�ɏ��������B�Ƃ肠����INT 18H��INT 1CH�̂ݑΉ�
	if(biosioemu.enable){
		mem[BIOS_BASE + BIOSOFST_18 + 1] = 0xee; // 0xcf(IRET) -> 0xee(OUT DX, AL)
		mem[BIOS_BASE + BIOSOFST_18 + 2] = bioshookinfo.hookinst; // 0x90(NOP) BIOS hook
		mem[BIOS_BASE + BIOSOFST_18 + 3] = 0xcf; // 0xcf(IRET)
		mem[BIOS_BASE + BIOSOFST_1c + 1] = 0xee; // 0xcf(IRET) -> 0xee(OUT DX, AL)
		mem[BIOS_BASE + BIOSOFST_1c + 2] = bioshookinfo.hookinst; // 0x90(NOP) BIOS hook
		mem[BIOS_BASE + BIOSOFST_1c + 3] = 0xcf; // 0xcf(IRET)
	}
#endif

	bios_updatehookinst(mem + 0xf8000, 0x100000 - 0xf8000);
}

static void bios_itfcall(void) {

	int		i;

	bios_itfprepare();
	bios_memclear();
	bios_vectorset();
	bios0x09_init();
	bios_reinitbyswitch();
	bios0x18_0c();

	if (!np2cfg.ITF_WORK || GetKeyState(VK_PAUSE)<0) {
		for (i=0; i<8; i++) {
			mem[MEMX_MSW + (i*4)] = msw_default[i];
		}
		CPU_FLAGL |= C_FLAG;
	}
	else {
		CPU_DX = 0x43d;
		CPU_AL = 0x10;
		mem[0x004f8] = 0xee;		// out	dx, al
		mem[0x004f9] = 0xea;		// call	far
		SETBIOSMEM16(0x004fa, 0x0000);
		SETBIOSMEM16(0x004fc, 0xffff);
		CPU_FLAGL &= ~C_FLAG;
	}
}

// np21w ver0.86 rev46 BIOS I/O emulation
#if defined(BIOS_IO_EMULATION)
// LIFO
void biosioemu_push8(UINT16 port, UINT8 data) {
	
	if(!biosioemu.enable) return;

	if(biosioemu.count < BIOSIOEMU_DATA_MAX){
		biosioemu.data[biosioemu.count].flag = BIOSIOEMU_FLAG_NONE;
		biosioemu.data[biosioemu.count].port = port;
		biosioemu.data[biosioemu.count].data = data;
		biosioemu.count++;
	}
}
// FIFO
void biosioemu_enq8(UINT16 port, UINT8 data) {
	
	if(!biosioemu.enable) return;

	if(biosioemu.count < BIOSIOEMU_DATA_MAX){
		int i;
		for(i=biosioemu.count-1;i>=0;i--){
			biosioemu.data[i+1].flag = biosioemu.data[i].flag;
			biosioemu.data[i+1].port = biosioemu.data[i].port;
			biosioemu.data[i+1].data = biosioemu.data[i].data;
		}
		biosioemu.data[0].flag = BIOSIOEMU_FLAG_NONE;
		biosioemu.data[0].port = port;
		biosioemu.data[0].data = data;
		biosioemu.count++;
	}
}
void biosioemu_begin(void) {
	
	if(!biosioemu.enable) return;

	if(biosioemu.count==0){
		// �f�[�^�������̂�I/O�|�[�g�o�͂��X�L�b�v
		biosioemu.oldEAX = 0;
		biosioemu.oldEDX = 0;
		CPU_EIP += 2;
	}else{
		int idx = biosioemu.count-1;
		// ���W�X�^�ޔ�
		biosioemu.oldEAX = CPU_EAX;
		biosioemu.oldEDX = CPU_EDX;
		// I/O�o�̓f�[�^�ݒ�
		CPU_DX = biosioemu.data[idx].port;
		CPU_AL = biosioemu.data[idx].data;
		biosioemu.count--;
	}
}
void biosioemu_proc(void) {
	
	if(!biosioemu.enable) return;

	if(biosioemu.count==0){
		// ���W�X�^�߂�
		CPU_EAX = biosioemu.oldEAX;
		CPU_EDX = biosioemu.oldEDX;
		biosioemu.oldEAX = 0;
		biosioemu.oldEDX = 0;
	}else{
		int idx = biosioemu.count-1;
		// I/O�o�̓f�[�^�ݒ�
		CPU_DX = biosioemu.data[idx].port;
		CPU_AL = biosioemu.data[idx].data;
		biosioemu.count--;
		// ���߈ʒu��߂�
		CPU_EIP -= 2;
	}
}
#endif

UINT MEMCALL biosfunc(UINT32 adrs) {

	UINT16	bootseg;
	
// np21w ver0.86 rev46 BIOS I/O emulation
#if defined(BIOS_IO_EMULATION)
	UINT32	oldEIP;
#endif
	
#if defined(SUPPORT_FAST_MEMORYCHECK)
	// �����������`�F�b�N
	if (CPU_ITFBANK && adrs == 0xf9724) {
		UINT16 subvalue = LOADINTELWORD((mem + ITF_ADRS + 5886)) / 128;
		UINT16 memaddr = MEMP_READ16(CPU_EIP);
		UINT16 counter = MEMR_READ16(CPU_SS, CPU_EBP + 6);
		if(subvalue == 0) subvalue = 1;
		if(counter >= subvalue){
			counter -= subvalue;
		}else{
			counter = 0;
		}
		if(counter == 0){
			CPU_FLAG |= Z_FLAG;
		}else{
			CPU_FLAG &= ~Z_FLAG;
		}
		MEMR_WRITE16(CPU_SS, CPU_EBP + 6, counter);
		CPU_IP += 2;
		return 1;
	}
#endif

	if ((CPU_ITFBANK) && (adrs >= 0xf8000) && (adrs < 0x100000)) {
		// for epson ITF
		return(0);
	}

//	TRACEOUT(("biosfunc(%x)", adrs));
#if defined(CPUCORE_IA32) && defined(TRACE)
	if (CPU_STAT_PAGING) {
		UINT32 pde = MEMP_READ32(CPU_STAT_PDE_BASE);
		if (!(pde & CPU_PDE_PRESENT)) {
			TRACEOUT(("page0: PTE not present"));
		}
		else {
			UINT32 pte = MEMP_READ32(pde & CPU_PDE_BASEADDR_MASK);
			if (!(pte & CPU_PTE_PRESENT)) {
				TRACEOUT(("page0: not present"));
			}
			else if (pte & CPU_PTE_BASEADDR_MASK) {
				TRACEOUT(("page0: physical address != 0 (pte = %.8x)", pte));
			}
		}
	}
#endif

	switch(adrs) {
		case BIOS_BASE + BIOSOFST_ITF:		// ���Z�b�g
			bios_itfcall();
			return(1);

		case BIOS_BASE + BIOSOFST_INIT:		// �u�[�g
#if 1		// for RanceII
			bios_memclear();
#endif
			bios_vectorset();
#if 1
			bios0x09_init();
#endif
			bios_reinitbyswitch();
			bios_vectorset();
			bios_screeninit();
			if (((pccore.model & PCMODELMASK) >= PCMODEL_VX) &&
				(pccore.sound & 0x7e)) {
				if(g_nSoundID == SOUNDID_MATE_X_PCM || ((g_nSoundID == SOUNDID_PC_9801_118 || g_nSoundID == SOUNDID_PC_9801_86_118) && np2cfg.snd118irqf == np2cfg.snd118irqp) || g_nSoundID == SOUNDID_PC_9801_86_WSS || g_nSoundID == SOUNDID_WAVESTAR){
					iocore_out8(0x188, 0x27);
					iocore_out8(0x18a, 0x30);
					if(g_nSoundID == SOUNDID_PC_9801_118 || g_nSoundID == SOUNDID_PC_9801_86_118){
						iocore_out8(cs4231.port[4], 0x27);
						iocore_out8(cs4231.port[4]+2, 0x30);
					}
				}else{
					iocore_out8(0x188, 0x27);
					iocore_out8(0x18a, 0x3f);
				}
			}
			return(1);

		case BIOS_BASE + BIOSOFST_09:
			CPU_REMCLOCK -= 500;
			bios0x09();
			return(1);

		case BIOS_BASE + BIOSOFST_0c:
			CPU_REMCLOCK -= 500;
			bios0x0c();
			return(1);

		case BIOS_BASE + BIOSOFST_12:
			CPU_REMCLOCK -= 500;
			bios0x12();
			return(1);

		case BIOS_BASE + BIOSOFST_13:
			CPU_REMCLOCK -= 500;
			bios0x13();
			return(1);

		case BIOS_BASE + BIOSOFST_18:
			CPU_REMCLOCK -= 200;
#if defined(BIOS_IO_EMULATION)
			oldEIP = CPU_EIP;
#endif
			bios0x18();
#if defined(BIOS_IO_EMULATION)
			// np21w ver0.86 rev46 BIOS I/O emulation
			if(oldEIP == CPU_EIP){
				biosioemu_begin(); 
			}else{
				biosioemu.count = 0; 
			}
#endif
			return(1);
			
#if defined(BIOS_IO_EMULATION)
		case BIOS_BASE + BIOSOFST_18 + 2: // np21w ver0.86 rev46 BIOS I/O emulation
			biosioemu_proc();
			return(1);
#endif

		case BIOS_BASE + BIOSOFST_19:
			CPU_REMCLOCK -= 200;
			bios0x19();
			return(1);

		case BIOS_BASE + BIOSOFST_CMT:
			CPU_REMCLOCK -= 200;
			bios0x1a_cmt();
			return(0);											// return(1);

		case BIOS_BASE + BIOSOFST_PRT:
			CPU_REMCLOCK -= 200;
#if defined(SUPPORT_PCI)
			if(CPU_AH == 0xb1){
				bios0x1a_pci();
			}else if(CPU_AH == 0xb4){
				bios0x1a_pcipnp();
			}else
#endif
			{
				bios0x1a_prt();
			}
			return(1);

		case BIOS_BASE + BIOSOFST_1b:
			CPU_STI;
			CPU_REMCLOCK -= 200;
			bios0x1b();
			return(1);

		case BIOS_BASE + BIOSOFST_1c:
			CPU_REMCLOCK -= 200;
#if defined(BIOS_IO_EMULATION)
			oldEIP = CPU_EIP;
#endif
			bios0x1c();
#if defined(BIOS_IO_EMULATION)
			// np21w ver0.86 rev47 BIOS I/O emulation
			if(oldEIP == CPU_EIP){
				biosioemu_begin(); 
			}else{
				biosioemu.count = 0; 
			}
#endif
			return(1);
			
#if defined(BIOS_IO_EMULATION)
		case BIOS_BASE + BIOSOFST_1c + 2: // np21w ver0.86 rev47 BIOS I/O emulation
			biosioemu_proc();
			return(1);
#endif

		case BIOS_BASE + BIOSOFST_1f:
			CPU_REMCLOCK -= 200;
			bios0x1f();
			return(1);

		case BIOS_BASE + BIOSOFST_WAIT:
			CPU_STI;
			return(bios0x1b_wait());								// ver0.78

		case 0xfffe8:					// �u�[�g�X�g���b�v���[�h
			CPU_REMCLOCK -= 2000;
			bootseg = bootstrapload();
			if (bootseg) {
				CPU_STI;
				CPU_CS = bootseg;
				CPU_IP = 0x0000;
				return(1);
			}
			return(0);

		case 0xfffec:
			CPU_REMCLOCK -= 2000;
			bootstrapload();
			return(0);
	}

	if ((adrs >= 0xf9950) && (adrs <= 0x0f9990) && (!(adrs & 3))) {
		CPU_REMCLOCK -= 500;
		bios_lio((REG8)((adrs - 0xf9950) >> 2));
	}
	else if (adrs == 0xf9994) {
		if (nevent_iswork(NEVENT_GDCSLAVE)) {
			CPU_IP--;
			CPU_REMCLOCK = -1;
			return(1);
		}
	}
	
	return(0);
}

#ifdef SUPPORT_PCI
UINT MEMCALL bios32func(UINT32 adrs) {
	
	// �A�h���X��BIOS32 Entry Point�Ȃ珈��
	if (pcidev.bios32entrypoint && adrs == pcidev.bios32entrypoint) {
		CPU_REMCLOCK -= 200;
		bios0x1a_pci_part(1);
	}
	return(0);
}
#endif
