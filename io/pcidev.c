
// PC-9821 PCIバス

// 現状はConfiguration Mechanism #1対応でバス番号0のみ。bios1a.cにエミュレーションPCI BIOSがあります。

#include	"compiler.h"

#if defined(SUPPORT_PC9821)

#include	"cpucore.h"
#include	"pccore.h"
#include	"iocore.h"

#if defined(SUPPORT_PCI)

#include	"pci/cbusbridge.h"
#include	"pci/98graphbridge.h"

#define GETCFGREG_B(reg, ofs)			(*((UINT8*)((reg) + (ofs))))
#define GETCFGREG_W(reg, ofs)			(*((UINT16*)((reg) + (ofs))))
#define GETCFGREG_D(reg, ofs)			(*((UINT32*)((reg) + (ofs))))

#define SETCFGREG_B(reg, ofs, value)	(GETCFGREG_B(reg, ofs) = value)
#define SETCFGREG_W(reg, ofs, value)	(GETCFGREG_W(reg, ofs) = value)
#define SETCFGREG_D(reg, ofs, value)	(GETCFGREG_D(reg, ofs) = value)

#define SETCFGREG_B_MASK(reg, ofs, value, mask)	(GETCFGREG_B(reg, ofs) = (GETCFGREG_B(reg, ofs) & mask) | (value & ~mask))
#define SETCFGREG_W_MASK(reg, ofs, value, mask)	(GETCFGREG_W(reg, ofs) = (GETCFGREG_W(reg, ofs) & mask) | (value & ~mask))
#define SETCFGREG_D_MASK(reg, ofs, value, mask)	(GETCFGREG_D(reg, ofs) = (GETCFGREG_D(reg, ofs) & mask) | (value & ~mask))

UINT8 PCI_INTLINE2IRQTBL[] = {
//  INTA INTB INTC INTD
	0,   1,   2,   3, // slot #0
	1,   2,   3,   0, // slot #1 
	2,   3,   0,   1, // slot #2 
	3,   2,   1,   0, // slot #3
};

// これはなんだろう？
static void setRAM_D000(UINT8 dat){
	//UINT8 dat = *((UINT8*)(pcidev.devices[0].cfgreg8 + 0x64));
	UINT32	work;
	work = CPU_RAM_D000 & 0x03ff;
	if (dat & 0x10) {
		work |= 0x0400;
	}
	if (dat & 0x20) {
		work |= 0x0800;
	}
	if (dat & 0x80) {
		work |= 0xf000;
	}
	CPU_RAM_D000 = (UINT16)work;
}


UINT8* IOOUTCALL pci_getirqtbl(UINT port, REG8 dat) {
	return &(pcidev.devices[pcidev_cbusbridge_deviceid].cfgreg8[0x60]); // C-Bus Bridgeより
}
UINT8 IOOUTCALL pci_getslotnumber(UINT32 devNumber){
	return devNumber & 0x3;
}
UINT8 IOOUTCALL pci_getirq(UINT32 devNumber){
	int intpin = (pcidev.devices[devNumber].header.interruptpin - 1);
	if(intpin < 0 || intpin > 3){
		return 0;
	}else{
		return pcidev.devices[pcidev_cbusbridge_deviceid].cfgreg8[0x60 + PCI_INTLINE2IRQTBL[intpin + 4*pci_getslotnumber(devNumber)]]; // C-Bus Bridgeより
	}
}
UINT8 IOOUTCALL pci_getirq2(UINT8 intpin, UINT8 slot){
	if(intpin > 3){
		return 0;
	}else{
		return pcidev.devices[pcidev_cbusbridge_deviceid].cfgreg8[0x60 + PCI_INTLINE2IRQTBL[intpin + 4*slot]]; // C-Bus Bridgeより
	}
}

void pcidev_pcmc_cfgreg_w(UINT32 devNumber, UINT8 funcNumber, UINT8 cfgregOffset, UINT8 sizeinbytes, UINT32 value){

}

static void IOOUTCALL pci_o0cf8(UINT port, REG8 dat) {

	pcidev.reg_cse = dat & 0xfe;
}
static void IOOUTCALL pci_o0cf9(UINT port, REG8 dat) {
	
	pcidev.reg_trc = dat;
}
static void IOOUTCALL pci_o0cfa(UINT port, REG8 dat) {
	
	pcidev.reg_fwd = dat;
}
static void IOOUTCALL pci_o0cfb(UINT port, REG8 dat) {
	
	pcidev.reg_cms = dat;
}

static REG8 IOINPCALL pci_i0cf8(UINT port) {

	return pcidev.reg_cse;
}
static REG8 IOINPCALL pci_i0cf9(UINT port) {
	
	return pcidev.reg_trc;
}
static REG8 IOINPCALL pci_i0cfa(UINT port) {
	
	return pcidev.reg_fwd;
}
static REG8 IOINPCALL pci_i0cfb(UINT port) {
	
	return pcidev.reg_cms;
}

void IOOUTCALL pcidev_w8_0xcfc(UINT port, UINT8 value) {
	
	if (pcidev.reg32_caddr & 0x80000000) {
		//if((pcidev.reg32_caddr & 0x00ff0000) == 0){ 
			// Configuration Mechanism #1 Type0,1
			UINT8 busNumber = (pcidev.reg32_caddr >> 16) & 0xff;
			UINT8 idselSelect = (pcidev.reg32_caddr >> 11) & 0x1f;
			UINT8 funcNumber = (pcidev.reg32_caddr >> 8) & 0x7;
			UINT8 cfgregOffset = ((pcidev.reg32_caddr >> 0) & 0xff) + (port-0xcfc);
			if(!pcidev.enable && idselSelect!=0) return;
			if(pcidev.devices[idselSelect].enable && funcNumber==0){
				UINT32 mask = GETCFGREG_B(pcidev.devices[idselSelect].cfgreg8rom, cfgregOffset);
				SETCFGREG_B_MASK(pcidev.devices[idselSelect].cfgreg8, cfgregOffset, value, mask);
				if(pcidev.devices[idselSelect].regwfn){
					(*pcidev.devices[idselSelect].regwfn)(idselSelect, funcNumber, cfgregOffset, 1, value);
				}
			}
			if(idselSelect==0 && cfgregOffset==0x64) setRAM_D000(value);
		//}else{ 
		//	// Configuration Mechanism #1 Type1
		//	UINT8 busNumber = (pcidev.reg32_caddr >> 16) & 0xff;
		//	UINT8 devNumber = (pcidev.reg32_caddr >> 11) & 0x1f;
		//	UINT8 funcNumber = (pcidev.reg32_caddr >> 8) & 0x7;
		//	UINT8 cfgregOffset = (pcidev.reg32_caddr >> 0) & 0xff;
		//}
	}
}
void IOOUTCALL pcidev_w16_0xcfc(UINT port, UINT16 value) {
	
	if (pcidev.reg32_caddr & 0x80000000) {
		//if((pcidev.reg32_caddr & 0x00ff0000) == 0){ 
			// Configuration Mechanism #1 Type0,1
			UINT8 busNumber = (pcidev.reg32_caddr >> 16) & 0xff;
			UINT8 idselSelect = (pcidev.reg32_caddr >> 11) & 0x1f;
			UINT8 funcNumber = (pcidev.reg32_caddr >> 8) & 0x7;
			UINT8 cfgregOffset = ((pcidev.reg32_caddr >> 0) & 0xff) + (port-0xcfc);
			if(!pcidev.enable && idselSelect!=0) return;
			if(pcidev.devices[idselSelect].enable && funcNumber==0){
				UINT32 mask = GETCFGREG_W(pcidev.devices[idselSelect].cfgreg8rom, cfgregOffset);
				SETCFGREG_W_MASK(pcidev.devices[idselSelect].cfgreg8, cfgregOffset, value, mask);
				if(pcidev.devices[idselSelect].regwfn){
					(*pcidev.devices[idselSelect].regwfn)(idselSelect, funcNumber, cfgregOffset, 2, value);
				}
			}
			if(idselSelect==0 && cfgregOffset==0x64){
				//setRAM_D000((value >> 0) & 0xff);
				setRAM_D000((value >> 8) & 0xff);
			}
		//}else{ 
		//	// Configuration Mechanism #1 Type1
		//	UINT8 busNumber = (pcidev.reg32_caddr >> 16) & 0xff;
		//	UINT8 devNumber = (pcidev.reg32_caddr >> 11) & 0x1f;
		//	UINT8 funcNumber = (pcidev.reg32_caddr >> 8) & 0x7;
		//	UINT8 cfgregOffset = (pcidev.reg32_caddr >> 0) & 0xff;
		//}
	}
}
void IOOUTCALL pcidev_w32(UINT port, UINT32 value) {
	
	if(port==0xcf8){
		pcidev.reg32_caddr = value;
	}else{
		if (pcidev.reg32_caddr & 0x80000000) {
			//if((pcidev.reg32_caddr & 0x00ff0000) == 0){ 
				// Configuration Mechanism #1 Type0,1
				UINT8 busNumber = (pcidev.reg32_caddr >> 16) & 0xff;
				UINT8 idselSelect = (pcidev.reg32_caddr >> 11) & 0x1f;
				UINT8 funcNumber = (pcidev.reg32_caddr >> 8) & 0x7;
				UINT8 cfgregOffset = (pcidev.reg32_caddr >> 0) & 0xff;
				if(!pcidev.enable && idselSelect!=0) return;
				if(pcidev.devices[idselSelect].enable && funcNumber==0){
					UINT32 mask = GETCFGREG_D(pcidev.devices[idselSelect].cfgreg8rom, cfgregOffset);
					SETCFGREG_D_MASK(pcidev.devices[idselSelect].cfgreg8, cfgregOffset, value, mask);
					if(pcidev.devices[idselSelect].regwfn){
						(*pcidev.devices[idselSelect].regwfn)(idselSelect, funcNumber, cfgregOffset, 4, value);
					}
				}
				if(idselSelect==0 && cfgregOffset==0x64){
					//setRAM_D000((value >> 0) & 0xff);
					//setRAM_D000((value >> 8) & 0xff);
					//setRAM_D000((value >> 16) & 0xff);
					setRAM_D000((value >> 24) & 0xff);
				}
			//}else{ 
			//	// Configuration Mechanism #1 Type1
			//	UINT8 busNumber = (pcidev.reg32_caddr >> 16) & 0xff;
			//	UINT8 devNumber = (pcidev.reg32_caddr >> 11) & 0x1f;
			//	UINT8 funcNumber = (pcidev.reg32_caddr >> 8) & 0x7;
			//	UINT8 cfgregOffset = (pcidev.reg32_caddr >> 0) & 0xff;
			//}
		}
	}
}

UINT8 IOOUTCALL pcidev_r8_0xcfc(UINT port) {
	
	//if((pcidev.reg32_caddr & 0x00ff0000) == 0){ 
		// Configuration Mechanism #1 Type0,1
		UINT8 busNumber = (pcidev.reg32_caddr >> 16) & 0xff;
		UINT8 idselSelect = (pcidev.reg32_caddr >> 11) & 0x1f;
		UINT8 funcNumber = (pcidev.reg32_caddr >> 8) & 0x7;
		UINT8 cfgregOffset = ((pcidev.reg32_caddr >> 0) & 0xff) + (port-0xcfc);
		if(!pcidev.enable && idselSelect!=0) return 0xff;
		if(pcidev.devices[idselSelect].enable && funcNumber==0){
			return GETCFGREG_B(pcidev.devices[idselSelect].cfgreg8, cfgregOffset);
		}
	//}else{ 
	//	// Configuration Mechanism #1 Type1
	//	UINT8 busNumber = (pcidev.reg32_caddr >> 16) & 0xff;
	//	UINT8 devNumber = (pcidev.reg32_caddr >> 11) & 0x1f;
	//	UINT8 funcNumber = (pcidev.reg32_caddr >> 8) & 0x7;
	//	UINT8 cfgregOffset = (pcidev.reg32_caddr >> 0) & 0xff;
	//}
	return 0xff;
}
UINT16 IOOUTCALL pcidev_r16_0xcfc(UINT port) {
	
	//if((pcidev.reg32_caddr & 0x00ff0000) == 0){ 
		// Configuration Mechanism #1 Type0,1
		UINT8 busNumber = (pcidev.reg32_caddr >> 16) & 0xff;
		UINT8 idselSelect = (pcidev.reg32_caddr >> 11) & 0x1f;
		UINT8 funcNumber = (pcidev.reg32_caddr >> 8) & 0x7;
		UINT8 cfgregOffset = ((pcidev.reg32_caddr >> 0) & 0xff) + (port-0xcfc);
		if(!pcidev.enable && idselSelect!=0) return 0xffff;
		if(pcidev.devices[idselSelect].enable && funcNumber==0){
			return GETCFGREG_W(pcidev.devices[idselSelect].cfgreg8, cfgregOffset);
		}
	//}else{ 
	//	// Configuration Mechanism #1 Type1
	//	UINT8 busNumber = (pcidev.reg32_caddr >> 16) & 0xff;
	//	UINT8 devNumber = (pcidev.reg32_caddr >> 11) & 0x1f;
	//	UINT8 funcNumber = (pcidev.reg32_caddr >> 8) & 0x7;
	//	UINT8 cfgregOffset = (pcidev.reg32_caddr >> 0) & 0xff;
	//}
	return 0xffff;
}
UINT32 IOOUTCALL pcidev_r32(UINT port) {
	
	if(port==0xcf8){
		return pcidev.reg32_caddr;
	}else{
		//if((pcidev.reg32_caddr & 0x00ff0000) == 0){ 
			// Configuration Mechanism #1 Type0,1
			UINT8 busNumber = (pcidev.reg32_caddr >> 16) & 0xff;
			UINT8 idselSelect = (pcidev.reg32_caddr >> 11) & 0x1f;
			UINT8 funcNumber = (pcidev.reg32_caddr >> 8) & 0x7;
			UINT8 cfgregOffset = (pcidev.reg32_caddr >> 0) & 0xff;
			if(!pcidev.enable && idselSelect!=0) return 0xffffffff;
			if(pcidev.devices[idselSelect].enable && funcNumber==0){
				return GETCFGREG_D(pcidev.devices[idselSelect].cfgreg8, cfgregOffset);
			}
		//}else{ 
		//	// Configuration Mechanism #1 Type1
		//	UINT8 busNumber = (pcidev.reg32_caddr >> 16) & 0xff;
		//	UINT8 devNumber = (pcidev.reg32_caddr >> 11) & 0x1f;
		//	UINT8 funcNumber = (pcidev.reg32_caddr >> 8) & 0x7;
		//	UINT8 cfgregOffset = (pcidev.reg32_caddr >> 0) & 0xff;
		//}
	}
	return 0xffffffff;
}

void pcidev_reset(const NP2CFG *pConfig) {

	int i;
	int devid = 0;
	
	ZeroMemory(&pcidev, sizeof(pcidev));

	pcidev.enable = np2cfg.usepci;

	pcidev.reg_cse = 0;
	pcidev.reg_trc = 0;
	pcidev.reg_fwd = 0;
	pcidev.reg_cms = 0x80; // Configuration Mechanism #1 Enable
	
	memset(pcidev.devices, 0xff, sizeof(_PCIDEVICE)*PCI_DEVICES_MAX);

	for(i=0;i<PCI_DEVICES_MAX;i++){
		pcidev.devices[i].enable = 0;
		pcidev.devices[i].regwfn = NULL;
	}

	if(pcidev.enable){
		// i82430LX
		ZeroMemory(pcidev.devices+devid, sizeof(_PCIDEVICE));
		pcidev.devices[devid].enable = 1;
		pcidev.devices[devid].regwfn = &pcidev_pcmc_cfgreg_w;
		pcidev.devices[devid].header.vendorID = 0x8086;
		pcidev.devices[devid].header.deviceID = 0x04A3;
		pcidev.devices[devid].header.command = 0x0006;//0x0106;//0x0006;
		pcidev.devices[devid].header.status = 0x0200;//0x2280;//0x0400;
		pcidev.devices[devid].header.revisionID = 0x03;
		pcidev.devices[devid].header.classcode[0] = 0x00; // レジスタレベルプログラミングインタフェース
		pcidev.devices[devid].header.classcode[1] = 0x00; // サブクラスコード
		pcidev.devices[devid].header.classcode[2] = 0x06; // ベースクラスコード
		pcidev.devices[devid].header.cachelinesize = 0;
		pcidev.devices[devid].header.latencytimer = 0x00;
		pcidev.devices[devid].header.headertype = 0;
		pcidev.devices[devid].header.BIST = 0x00;
		pcidev.devices[devid].header.interruptline = 0x00;
		pcidev.devices[devid].header.interruptpin = 0x00;
		pcidev.devices[devid].header.subsysID = 0x00;
		pcidev.devices[devid].header.subsysventorID = 0x00;
		pcidev.devices[devid].cfgreg8[0x50] = 0x00; // ホストCPU選択(HCS)
		pcidev.devices[devid].cfgreg8[0x51] = 0x01; // デターボ周波数制御(DFC)
		pcidev.devices[devid].cfgreg8[0x52] = 0x01; // 2次キャッシュ制御(SCC)
		pcidev.devices[devid].cfgreg8[0x53] = 0x00; // ホスト読取り／書込みバッファ制御(HBC)
		pcidev.devices[devid].cfgreg8[0x54] = 0x00; // PCI読取り／書込みバッファ制御(PBC)
		pcidev.devices[devid].cfgreg8[0x55] = 0x00; // 2次キャッシュ制御拡張レジスタ(SCCE)
		pcidev.devices[devid].cfgreg8[0x57] = 0x01; // DRAM制御
		pcidev.devices[devid].cfgreg8[0x58] = 0x00; // DRAMタイミング(DT)
		pcidev.devices[devid].cfgreg8[0x59] = 0x00; // プログラム可能属性マップ(PAM)
		pcidev.devices[devid].cfgreg8[0x5a] = 0x00; // プログラム可能属性マップ(PAM)
		pcidev.devices[devid].cfgreg8[0x5b] = 0x00; // プログラム可能属性マップ(PAM)
		pcidev.devices[devid].cfgreg8[0x5c] = 0x00; // プログラム可能属性マップ(PAM)
		pcidev.devices[devid].cfgreg8[0x5d] = 0x00; // プログラム可能属性マップ(PAM)
		pcidev.devices[devid].cfgreg8[0x5e] = 0x00; // プログラム可能属性マップ(PAM)
		pcidev.devices[devid].cfgreg8[0x5f] = 0x00; // プログラム可能属性マップ(PAM)
		pcidev.devices[devid].cfgreg8[0x60] = 0x10; // DRAMロー境界レジスタ(DRB) ROW#0 
		pcidev.devices[devid].cfgreg8[0x61] = 0x20; // ROW#0,#1
		pcidev.devices[devid].cfgreg8[0x62] = 0x20; // ROW#0～2
		pcidev.devices[devid].cfgreg8[0x63] = 0x20; // ROW#0～3
		pcidev.devices[devid].cfgreg8[0x64] = 0x20; // ????? pcidev.devices[0].cfgreg8[0x63]; // ROW#0～4
		pcidev.devices[devid].cfgreg8[0x65] = 0x20; // ????? pcidev.devices[0].cfgreg8[0x65]; // ROW#0～5
		pcidev.devices[devid].cfgreg8[0x70] = 0x00; // エラーコマンド(ERRCMD)
		pcidev.devices[devid].cfgreg8[0x71] = 0x00; // エラーステータス(ERRSTS)
		pcidev.devices[devid].cfgreg8[0x72] = 0x00; // SMRAM空間制御(SMRS)
		SETCFGREG_W(pcidev.devices[devid].cfgreg8, 0x78, 0x0000); // メモリ空間ギャップ(MSG)
		SETCFGREG_D(pcidev.devices[devid].cfgreg8, 0x7C, 0x00000000); // フレームバッファ領域(FBR)

		// ROM領域設定
		pcidev.devices[devid].headerrom.vendorID = 0xffff;
		pcidev.devices[devid].headerrom.deviceID = 0xffff;
		pcidev.devices[devid].headerrom.status = 0xffff;
		pcidev.devices[devid].headerrom.revisionID = 0xff;
		pcidev.devices[devid].headerrom.classcode[0] = 0xff;
		pcidev.devices[devid].headerrom.classcode[1] = 0xff;
		pcidev.devices[devid].headerrom.classcode[2] = 0xff;
	
		// PCI PC-9821標準デバイスreset
		pcidev_cbusbridge_reset(pConfig);
		pcidev_98graphbridge_reset(pConfig);
		
		TRACEOUT(("PCI: Peripheral Component Interconnect Enabled"));
	}

	(void)pConfig;
}

void pcidev_bind(void) {
	
	UINT	i;

	//for (i=0x0cfc; i<0x0d00; i++) {
	//	iocore_attachout(i, pci_o04);
	//	iocore_attachinp(i, pci_i04);
	//}
	// PCI I/Oポート割り当て
	iocore_attachout(0xcf8, pci_o0cf8);
	iocore_attachout(0xcf9, pci_o0cf9);
	iocore_attachout(0xcfa, pci_o0cfa);
	iocore_attachout(0xcfb, pci_o0cfb);
	iocore_attachinp(0xcf8, pci_i0cf8);
	iocore_attachinp(0xcf9, pci_i0cf9);
	iocore_attachinp(0xcfa, pci_i0cfa);
	iocore_attachinp(0xcfb, pci_i0cfb);
	
	for (i=0; i<4; i++) {
		iocore_attachout(0xcfc+i, pcidev_w8_0xcfc);
		iocore_attachinp(0xcfc+i, pcidev_r8_0xcfc);
	}
	
	if(pcidev.enable){
		// PCI PC-9821標準デバイスbind
		pcidev_cbusbridge_bind();
		pcidev_98graphbridge_bind();
	}
}

#else

// とりあえず config #1 type0固定で…

static void pcidevset10(UINT32 addr, REG8 dat) {

	UINT32	work;

	switch(addr) {
		case 0x000064 + 3:
			pcidev.membankd0 = dat;
			work = CPU_RAM_D000 & 0x03ff;
			if (dat & 0x10) {
				work |= 0x0400;
			}
			if (dat & 0x20) {
				work |= 0x0800;
			}
			if (dat & 0x80) {
				work |= 0xf000;
			}
			CPU_RAM_D000 = (UINT16)work;
			break;
	}
}

static REG8 pcidevget10(UINT32 addr) {

	switch(addr) {
		case 0x000064 + 3:
			return(pcidev.membankd0);
	}
	return(0xff);
}


// ----

static void IOOUTCALL pci_o04(UINT port, REG8 dat) {

	UINT32	addr;

	if (pcidev.base & 0x80000000) {
		addr = pcidev.base & 0x00fffffc;
		addr += port & 3;
		pcidevset10(addr, dat);
	}
}

static REG8 IOINPCALL pci_i04(UINT port) {

	UINT32	addr;

	if (pcidev.base & 0x80000000) {
		addr = pcidev.base & 0x00fffffc;
		addr += port & 3;
		return(pcidevget10(addr));
	}
	else {
		return(0xff);
	}
}

void IOOUTCALL pcidev_w32(UINT port, UINT32 value) {

	UINT32	addr;

	if (!(port & 4)) {
		pcidev.base = value;
	}
	else {
		if (pcidev.base & 0x80000000) {
			addr = pcidev.base & 0x00fffffc;
			pcidevset10(addr + 0, (UINT8)(value >> 0));
			pcidevset10(addr + 1, (UINT8)(value >> 8));
			pcidevset10(addr + 2, (UINT8)(value >> 16));
			pcidevset10(addr + 3, (UINT8)(value >> 24));
		}
	}
}

UINT32 IOOUTCALL pcidev_r32(UINT port) {

	UINT32	ret;
	UINT32	addr;

	ret = (UINT32)-1;
	if (!(port & 4)) {
		ret = pcidev.base;
	}
	else {
		if (pcidev.base & 0x80000000) {
			addr = pcidev.base & 0x00fffffc;
			ret = pcidevget10(addr + 0);
			ret |= (pcidevget10(addr + 1) << 8);
			ret |= (pcidevget10(addr + 2) << 16);
			ret |= (pcidevget10(addr + 3) << 24);
		}
	}
	return(ret);
}

void pcidev_reset(const NP2CFG *pConfig) {

	ZeroMemory(&pcidev, sizeof(pcidev));

	(void)pConfig;
}

void pcidev_bind(void) {

	UINT	i;

	for (i=0x0cfc; i<0x0d00; i++) {
		iocore_attachout(i, pci_o04);
		iocore_attachinp(i, pci_i04);
	}
}
#endif
#endif

