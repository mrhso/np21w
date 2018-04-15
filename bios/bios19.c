#include	"compiler.h"
#include	"i286.h"
#include	"memory.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"bios.h"
#include	"biosmem.h"
#include	"rsbios.h"


static const UINT rs_speed[] = {
						// 5MHz
						0x0800, 0x0400, 0x0200, 0x0100,
						0x0080, 0x0040, 0x0020, 0x0010,
						0x0008, 0x0004, 0x0002, 0x0001,
						// 4MHz
						0x0680, 0x0340, 0x01a0, 0x00d0,
						0x0068, 0x0034, 0x001a, 0x000d};


void bios0x19(void) {

	BYTE	speed;
	BYTE	mode;
	RSBIOS	rsb;
	UINT16	doff;
	UINT16	cnt;
	UINT16	dseg;
	BYTE	flag;

	if (I286_AH < 2) {
		// 通信速度…
		mode = I286_CH | 0x02;
		speed = I286_AL;
		if (speed >= 8) {
			speed = 4;						// 1200bps
		}
		if (mem[MEMB_BIOS_FLAG] & 0x80) {	// 4MHz?
			speed += 12;
		}

#if 1	// NP2では未サポートの為　強行(汗
		mode &= ~1;
#else
		if (mode & 1) {
			if (speed < (12 + 6)) {
				speed += 2;
			}
			else {
				mode &= ~1;
			}
		}
		// シリアルリセット
		iocore_out8(0x32, 0x00);		// dummy instruction
		iocore_out8(0x32, 0x00);		// dummy instruction
		iocore_out8(0x32, 0x00);		// dummy instruction
		iocore_out8(0x32, 0x40);		// reset
		iocore_out8(0x32, mode);		// mode
		iocore_out8(0x32, I286_CL);	// cmd
#endif
		iocore_out8(0x77, 0xb6);
		iocore_out8(0x75, (BYTE)rs_speed[speed]);
		iocore_out8(0x75, (BYTE)(rs_speed[speed] >> 8));

		ZeroMemory(&rsb, sizeof(rsb));
		rsb.FLAG = (I286_AH << 4);
		rsb.CMD = I286_CL;
		sysport.c &= ~7;
		if (!(I286_CL & RCMD_IR)) {
			rsb.FLAG |= RFLAG_INIT;
			if (I286_CL & RCMD_RXE) {
				sysport.c |= 1;
				pic.pi[0].imr &= ~PIC_RS232C;
			}
		}

		rsb.STIME = I286_BH;
		if (!rsb.STIME) {
			rsb.STIME = 0x04;
		}
		rsb.RTIME = I286_BL;
		if (!rsb.RTIME) {
			rsb.RTIME = 0x40;
		}
		doff = I286_DI + sizeof(RSBIOS);
		STOREINTELWORD(rsb.HEADP, doff);
		STOREINTELWORD(rsb.PUTP, doff);
		STOREINTELWORD(rsb.GETP, doff);
		doff += I286_DX;
		STOREINTELWORD(rsb.TAILP, doff);
		cnt = I286_DX >> 3;
		STOREINTELWORD(rsb.XOFF, cnt);
		cnt += I286_DX >> 2;
		STOREINTELWORD(rsb.XON, cnt);

		// ポインタ〜
		SETBIOSMEM16(MEMW_RS_CH0_OFST, I286_DI);
		SETBIOSMEM16(MEMW_RS_CH0_SEG, I286_ES);
		i286_memstr_write(I286_ES, I286_DI, &rsb, sizeof(rsb));

		I286_AH = 0;
	}
	else if (I286_AH < 7) {
		doff = GETBIOSMEM16(MEMW_RS_CH0_OFST);
		dseg = GETBIOSMEM16(MEMW_RS_CH0_SEG);
		if ((!doff) && (!dseg)) {
			I286_AH = 1;
			return;
		}
		flag = i286_membyte_read(dseg, doff + R_FLAG);
		if (!(flag & RFLAG_INIT)) {
			I286_AH = 1;
			return;
		}
		switch(I286_AH) {
			case 0x02:
				I286_CX = i286_memword_read(dseg, doff + R_CNT);
				break;

			case 0x03:
				iocore_out8(0x30, I286_AL);
				break;

			case 0x04:
				cnt = i286_memword_read(dseg, doff + R_CNT);
				if (cnt) {
					UINT16	pos;

					// データ引き取り
					pos = i286_memword_read(dseg, doff + R_GETP);
					I286_CX = i286_memword_read(dseg, pos);

					// 次のポインタをストア
					pos += 2;
					if (pos >= i286_memword_read(dseg, doff + R_TAILP)) {
						pos = i286_memword_read(dseg, doff + R_HEADP);
					}
					i286_memword_write(dseg, doff + R_GETP, pos);

					// カウンタをデクリメント
					cnt--;
					i286_memword_write(dseg, doff + R_CNT, cnt);

					// XONを送信？
					if ((flag & RFLAG_XOFF) && 
						(cnt < i286_memword_read(dseg, doff + R_XOFF))) {
						iocore_out8(0x30, RSCODE_XON);
						flag &= ~RFLAG_XOFF;
					}
					flag &= ~RFLAG_BOVF;
					I286_AH = 0;
					i286_membyte_write(dseg, doff + R_FLAG, flag);
					return;
				}
				else {
					I286_AH = 3;
				}
				break;

			case 0x05:
				iocore_out8(0x32, I286_AL);
				if (I286_AL & RCMD_IR) {
					flag &= ~RFLAG_INIT;
					i286_membyte_write(dseg, doff + R_FLAG, flag);
					sysport.c &= ~1;
					pic.pi[0].imr |= PIC_RS232C;
				}
				else if (!(I286_AL & RCMD_RXE)) {
					sysport.c &= ~1;
					pic.pi[0].imr |= PIC_RS232C;
				}
				else {
					sysport.c |= 1;
					pic.pi[0].imr &= ~PIC_RS232C;
				}
				i286_membyte_write(dseg, doff + R_CMD, I286_AL);
				break;
			case 0x06:
				I286_CH = iocore_inp8(0x32);
				I286_CL = iocore_inp8(0x33);
				break;
		}
		I286_AH = 0;
		if (flag & RFLAG_BOVF) {
			i286_membyte_write(dseg, doff + R_FLAG,
											(BYTE)(flag & (~RFLAG_BOVF)));
			I286_AH = 2;
		}
	}
	else {
		I286_AH = 0;
	}
}

