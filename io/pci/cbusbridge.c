
// PC-9821 PCI-CBusブリッジ

#include	"compiler.h"

#if defined(SUPPORT_PC9821)

#include	"cpucore.h"
#include	"pccore.h"
#include	"iocore.h"

#if defined(SUPPORT_PCI)

#include	"cbusbridge.h"

#define GETCFGREG_B(reg, ofs)			(*((UINT8*)((reg) + (ofs))))
#define GETCFGREG_W(reg, ofs)			(*((UINT16*)((reg) + (ofs))))
#define GETCFGREG_D(reg, ofs)			(*((UINT32*)((reg) + (ofs))))

#define SETCFGREG_B(reg, ofs, value)	(GETCFGREG_B(reg, ofs) = value)
#define SETCFGREG_W(reg, ofs, value)	(GETCFGREG_W(reg, ofs) = value)
#define SETCFGREG_D(reg, ofs, value)	(GETCFGREG_D(reg, ofs) = value)

int pcidev_cbusbridge_deviceid = 1;

void pcidev_cbusbridge_cfgreg_w(UINT32 devNumber, UINT8 funcNumber, UINT8 cfgregOffset, UINT8 sizeinbytes, UINT32 value){

}

void pcidev_cbusbridge_reset(const NP2CFG *pConfig) {

	(void)pConfig;
}

void pcidev_cbusbridge_bind(void) {
	
	int devid = pcidev_cbusbridge_deviceid;

	// PCI-Cバスブリッジ
	ZeroMemory(pcidev.devices+devid, sizeof(_PCIDEVICE));
	pcidev.devices[devid].enable = 1;
	pcidev.devices[devid].regwfn = &pcidev_cbusbridge_cfgreg_w;
	pcidev.devices[devid].header.vendorID = 0x1033;
	pcidev.devices[devid].header.deviceID = 0x0001;
	pcidev.devices[devid].header.command = 0x010f;//0x0107;
	pcidev.devices[devid].header.status = 0x0200;//0x0400;
	pcidev.devices[devid].header.revisionID = 0x02;//0x03;
	pcidev.devices[devid].header.classcode[0] = 0x00; // レジスタレベルプログラミングインタフェース
	pcidev.devices[devid].header.classcode[1] = 0x80; // サブクラスコード
	pcidev.devices[devid].header.classcode[2] = 0x06; // ベースクラスコード
	pcidev.devices[devid].header.cachelinesize = 0;
	pcidev.devices[devid].header.latencytimer = 0x0;
	pcidev.devices[devid].header.headertype = 0;
	pcidev.devices[devid].header.BIST = 0x00;
	pcidev.devices[devid].header.interruptline = 0x00;
	pcidev.devices[devid].header.interruptpin = 0x00;
	pcidev.devices[devid].cfgreg8[0x40] = 0x00;
	pcidev.devices[devid].cfgreg8[0x41] = 0x10;
	pcidev.devices[devid].cfgreg8[0x42] = 0x00;
	pcidev.devices[devid].cfgreg8[0x43] = 0xff;
	pcidev.devices[devid].cfgreg8[0x44] = 0x00;
	pcidev.devices[devid].cfgreg8[0x45] = 0xff;
	pcidev.devices[devid].cfgreg8[0x46] = 0xff;
	pcidev.devices[devid].cfgreg8[0x47] = 0xff;
	pcidev.devices[devid].cfgreg8[0x48] = 0x3f;
	pcidev.devices[devid].cfgreg8[0x4C] = 0x3e;
	pcidev.devices[devid].cfgreg8[0x4E] = 0x03;
	//pcidev.devices[devid].cfgreg8[0x59] = 9;
	//pcidev.devices[devid].cfgreg8[0x5a] = 10;
	//pcidev.devices[devid].cfgreg8[0x5b] = 11;
	//pcidev.devices[devid].cfgreg8[0x5d] = 13;
	SETCFGREG_D(pcidev.devices[devid].cfgreg8, 0x50, 0x00EF0010);
	SETCFGREG_D(pcidev.devices[devid].cfgreg8, 0x54, 0xFFFBFFFA);
	SETCFGREG_D(pcidev.devices[devid].cfgreg8, 0x58, 0xFFFEFFFE);
	SETCFGREG_D(pcidev.devices[devid].cfgreg8, 0x5C, 0x0000FFFF);
	pcidev.devices[devid].cfgreg8[0x60] = 0x03;//0x80;//0x03;
	pcidev.devices[devid].cfgreg8[0x61] = 0x05;//0x80;//0x05;
	pcidev.devices[devid].cfgreg8[0x62] = 0x06;//0x80;//0x06;
	pcidev.devices[devid].cfgreg8[0x63] = 0x0c;//0x80;//0x0c;
	
	// ROM領域設定
	pcidev.devices[devid].headerrom.vendorID = 0xffff;
	pcidev.devices[devid].headerrom.deviceID = 0xffff;
	pcidev.devices[devid].headerrom.status = 0xffff;
	pcidev.devices[devid].headerrom.revisionID = 0xff;
	pcidev.devices[devid].headerrom.classcode[0] = 0xff;
	pcidev.devices[devid].headerrom.classcode[1] = 0xff;
	pcidev.devices[devid].headerrom.classcode[2] = 0xff;
	memset(&(pcidev.devices[devid].headerrom), 0xff, sizeof(_PCICSH));
}

#endif
#endif

