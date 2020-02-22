/**
 * @file	bios.h
 * @brief	Interface of BIOS
 */

#pragma once

#include "cpumem.h"

enum {
	BIOS_SEG		= 0xfd80,
	BIOS_BASE		= (BIOS_SEG << 4),

	BIOS_TABLE		= 0x0040,

	BIOSOFST_ITF	= 0x0080,
	BIOSOFST_INIT	= 0x0084,

	BIOSOFST_09		= 0x0088,					// Keyboard
	BIOSOFST_0c		= 0x008c,					// Serial
	BIOSOFST_10		= 0x008e,					// (PC/AT VGA)

	BIOSOFST_12		= 0x0090,					// FDC
	BIOSOFST_13		= 0x0094,					// FDC

	BIOSOFST_18		= 0x0098,					// Common
	BIOSOFST_19		= 0x009c,					// RS-232C
	BIOSOFST_CMT	= 0x00a0,					// CMT
	BIOSOFST_PRT	= 0x00a4,					// Printer
	BIOSOFST_1b		= 0x00a8,					// Disk
	BIOSOFST_1c		= 0x00ac,					// Timer
	BIOSOFST_1f		= 0x00b0,					// Ext

	BIOSOFST_WAIT	= 0x00b4					// FDD waiting
	
};

#ifdef USE_CUSTOM_HOOKINST
#define HOOKINST_DEFAULT	0x90	// NOP����

typedef struct {
	UINT8	hookinst; // BIOS�t�b�N���閽�� default:NOP(0x90)
} BIOSHOOKINFO;
#endif

#if defined(BIOS_IO_EMULATION)
// np21w ver0.86 rev46 BIOS I/O emulation

// XXX: I/O�A�N�Z�X�͍ő�20�񕪂��炢����Ώ\�����Ǝv���̂Ō��ߑł�
#define BIOSIOEMU_DATA_MAX	20 

enum {
	BIOSIOEMU_FLAG_NONE	= 0x0,
	BIOSIOEMU_FLAG_MB	= 0x1, // �r�b�g�𗧂Ă��DX, AX(16bit)�܂���DX, EAX(32bit)�ɂȂ�i���ĂȂ����8bit�A�N�Z�X�j
	BIOSIOEMU_FLAG_READ	= 0x2, // �r�b�g�𗧂Ă�Ƌ�ǂ݂���
};
typedef struct {
	UINT8	flag; // �A�N�Z�X�t���O(����ł�BIOSIOEMU_FLAG_NONE�̂�)
	UINT16	port; // ���o�͐�|�[�g
	UINT32	data; // �o�̓f�[�^(����ł�8bit�l�̂ݗL��)
} BIOSIOEMU_IODATA;

typedef struct {
	UINT8	enable; // BIOS I/O �G�~�����[�V�����L��
	UINT8	count; // �o�̓f�[�^��
	UINT32	oldEAX; // EAX�ޔ�p
	UINT32	oldEDX; // EDX�ޔ�p
	BIOSIOEMU_IODATA	data[BIOSIOEMU_DATA_MAX]; // �o�͐�|�[�g�ƃ|�[�g�ɏo�͂������f�[�^�B�f�[�^����LIFO�Ȃ̂Œ���
} BIOSIOEMU;

#endif


#ifdef __cplusplus
extern "C" {
#endif

// extern	BOOL	biosrom;
	
#ifdef USE_CUSTOM_HOOKINST
extern BIOSHOOKINFO	bioshookinfo;
#endif
#if defined(BIOS_IO_EMULATION)
// np21w ver0.86 rev46 BIOS I/O emulation
extern BIOSIOEMU	biosioemu;
#endif

void bios_initialize(void);
UINT MEMCALL biosfunc(UINT32 adrs);
#ifdef SUPPORT_PCI
UINT MEMCALL bios32func(UINT32 adrs);
#endif

void bios0x09(void);
void bios0x09_init(void);

void bios0x0c(void);

void bios0x12(void);
void bios0x13(void);

void bios0x18(void);
void bios0x18_0a(REG8 mode);
void bios0x18_0c(void);
void bios0x18_10(REG8 curdel);
REG16 bios0x18_14(REG16 seg, REG16 off, REG16 code);
void bios0x18_16(REG8 chr, REG8 atr);
void bios0x18_40(void);
void bios0x18_41(void);
void bios0x18_42(REG8 mode);

void bios0x19(void);

void bios0x1a_cmt(void);
void bios0x1a_prt(void);
#if defined(SUPPORT_PCI)
void bios0x1a_pci_part(int is32bit);
void bios0x1a_pci(void);
void bios0x1a_pcipnp(void);
#endif

void bios0x1b(void);
UINT bios0x1b_wait(void);
void fddbios_equip(REG8 type, BOOL clear);

REG16 bootstrapload(void);

void bios0x1c(void);

void bios0x1f(void);

#if defined(BIOS_IO_EMULATION)
// np21w ver0.86 rev46-69 BIOS I/O emulation
void biosioemu_push8(UINT16 port, UINT8 data);
void biosioemu_push16(UINT16 port, UINT32 data);
void biosioemu_push8_read(UINT16 port);
void biosioemu_push16_read(UINT16 port);
void biosioemu_enq8(UINT16 port, UINT8 data);
void biosioemu_enq16(UINT16 port, UINT32 data);
void biosioemu_enq8_read(UINT16 port);
void biosioemu_enq16_read(UINT16 port);
#endif

#ifdef __cplusplus
}
#endif

