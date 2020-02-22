#include	"compiler.h"
#include	"cpucore.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"memegc.h"
#include	"vram.h"

// PEGC �v���[�����[�h
// �֘A: vram.c, vram.h, memvga.c, memvga.h

// �ڂ������Ȃ��̂ɍ�����̂ł��Ȃ肢�������ł��B
// ���ǂ���̂ł���ΑS���̂Ăč�蒼���������ǂ���������܂���

#ifdef SUPPORT_PEGC

// 
REG16 MEMCALL pegc_memvgaplane_rd16(UINT32 address){
	
	int i,j;
	UINT16 ret = 0;
	//UINT8 bit;

	UINT32 addr; // ��f�P�ʂ̓ǂݍ��݌��A�h���X

	//UINT8 src, dst, pat1, pat2; // �\�[�X�f�[�^�A�f�B�X�e�B�l�[�V�����f�[�^�A�p�^�[���f�[�^1&2
	UINT8 ropcode = 0; // ���X�^�I�y���[�V�����ݒ� E0108h bit0�`bit7
	UINT8 ropmethod = 0; // �_�����Z�̕��@���w��i�p�^�[�����W�X�^�܂��̓J���[�p���b�g�j E0108h bit11,10
	UINT8 ropupdmode = 0; // 1�Ȃ烉�X�^�I�y���[�V�������g�p E0108h bit12
	UINT8 planemask = 0; // �v���[���������݋֎~(0=����, 1=�֎~)�@E0104h
	UINT32 pixelmask = 0; // �r�b�g�i��f�j�ւ̏������݋֎~(0=�֎~, 1=����) E010Ch
	UINT32 bitlength = 0; // �u���b�N�]���r�b�g��(�]���T�C�Y-1) E0110h
	UINT32 srcbitshift = 0; // ���[�h���̃r�b�g�V�t�g�� E0112h
	UINT8 shiftdir = 1; // �V�t�g�����i0:inc, 1:dec�jE0108h bit9
	UINT8 srccpu = 1; // ���O�ɓǂݎ����VRAM�f�[�^�ł͂Ȃ�CPU�f�[�^���g�p���邩 E0108h bit8

	// PEGC���W�X�^�ǂ݂���
	ropcode = LOADINTELWORD(vramop.mio2+PEGC_REG_PLANE_ROP) & 0xff;
	ropmethod = (LOADINTELWORD(vramop.mio2+PEGC_REG_PLANE_ROP) >> 10) & 0x3;
	ropupdmode = (LOADINTELWORD(vramop.mio2+PEGC_REG_PLANE_ROP) >> 12) & 0x1;
	planemask = vramop.mio2[PEGC_REG_PLANE_ACCESS];
	pixelmask = LOADINTELDWORD(vramop.mio2 + PEGC_REG_MASK);
	bitlength = LOADINTELDWORD(vramop.mio2 + PEGC_REG_LENGTH) & 0x0fff;
	srcbitshift = (LOADINTELWORD(vramop.mio2+PEGC_REG_SHIFT)) & 0x1f;
	shiftdir = (LOADINTELWORD(vramop.mio2+PEGC_REG_PLANE_ROP) >> 9) & 0x1;
	srccpu = (LOADINTELWORD(vramop.mio2+PEGC_REG_PLANE_ROP) >> 8) & 0x1;
	
	// ��f�P�ʂ̃A�h���X�v�Z
	addr = (address - 0xa8000) * 8;
	addr += srcbitshift;
	addr &= 0x80000-1; // ���S�̂���

	if(!srccpu){
		if(!shiftdir){
			for(i=0;i<16;i++){
				UINT32 addrtmp = (addr + i) & (0x80000-1); // �ǂݎ��ʒu
				UINT32 pixmaskpos = (1 << ((15-i+8)&0xf)); // ���݂̉�f�ɑΉ�����pixelmask�̃r�b�g�ʒu
				UINT8 data = vramex[addrtmp];

				// compare data
				if((data ^ vramop.mio2[PEGC_REG_PALETTE1]) & ~planemask){
					ret |= (1<<i);
				}

				// update last data
				pegc.lastdata[i] = data;

				// update pattern reg
				if((LOADINTELWORD(vramop.mio2+PEGC_REG_PLANE_ROP) >> 13) & 0x1){
					for(j=7;j>=0;j--){
						UINT16 regdata = LOADINTELWORD(vramop.mio2 + PEGC_REG_PATTERN + j*4);
						regdata = (regdata & ~(1 << i)) | (((data >> j) & 0x1) << i);
						STOREINTELWORD(vramop.mio2 + PEGC_REG_PATTERN + j*4, regdata);
					}
				}
			}
		}else{
			for(i=0;i<16;i++){
				UINT32 addrtmp = (addr - i) & (0x80000-1); // �ǂݎ��ʒu
				UINT32 pixmaskpos = (1 << ((i/8)*8 + (7-(i&0x7)))); // ���݂̉�f�ɑΉ�����value��pixelmask�̃r�b�g�ʒu
				UINT8 data = vramex[addrtmp];

				// compare data
				if((data ^ vramop.mio2[PEGC_REG_PALETTE1]) & ~planemask){
					ret |= (1<<i);
				}

				// update last data
				pegc.lastdata[i] = data;

				// update pattern reg
				if(LOADINTELWORD(vramop.mio2+PEGC_REG_PLANE_ROP) & 0x2000){
					for(j=7;j>=0;j--){
						UINT16 regdata = LOADINTELWORD(vramop.mio2 + PEGC_REG_PATTERN + j*4);
						regdata = (regdata & ~(1 << i)) | (((data >> j) & 0x1) << i);
						STOREINTELWORD(vramop.mio2 + PEGC_REG_PATTERN + j*4, regdata);
					}
				}
			}
		}
	}
	pegc.lastdatalen += 16;
	return ret;
}
void MEMCALL pegc_memvgaplane_wr16(UINT32 address, REG16 value){
	
	int i,j;
	UINT8 bit;

	UINT32 addr; // ��f�P�ʂ̏������ݐ�A�h���X

	UINT8 src, dst, pat1, pat2; // �\�[�X�f�[�^�A�f�B�X�e�B�l�[�V�����f�[�^�A�p�^�[���f�[�^1&2
	UINT8 ropcode = 0; // ���X�^�I�y���[�V�����ݒ� E0108h bit0�`bit7
	UINT8 ropmethod = 0; // �_�����Z�̕��@���w��i�p�^�[�����W�X�^�܂��̓J���[�p���b�g�j E0108h bit11,10
	UINT8 ropupdmode = 0; // 1�Ȃ烉�X�^�I�y���[�V�������g�p E0108h bit12
	UINT8 planemask = 0; // �v���[���������݋֎~(0=����, 1=�֎~)�@E0104h
	UINT32 pixelmask = 0; // �r�b�g�i��f�j�ւ̏������݋֎~(0=�֎~, 1=����) E010Ch
	UINT32 bitlength = 0; // �u���b�N�]���r�b�g��(�]���T�C�Y-1) E0110h
	UINT32 dstbitshift = 0; // ���C�g���̃r�b�g�V�t�g�� E0112h
	UINT8 shiftdir = 1; // �V�t�g�����i0:inc, 1:dec�jE0108h bit9
	UINT8 srccpu = 1; // ���O�ɓǂݎ����VRAM�f�[�^�ł͂Ȃ�CPU�f�[�^���g�p���邩 E0108h bit8

	// PEGC���W�X�^�ǂ݂���
	ropcode = LOADINTELWORD(vramop.mio2+PEGC_REG_PLANE_ROP) & 0xff;
	ropmethod = (LOADINTELWORD(vramop.mio2+PEGC_REG_PLANE_ROP) >> 10) & 0x3;
	ropupdmode = (LOADINTELWORD(vramop.mio2+PEGC_REG_PLANE_ROP) >> 12) & 0x1;
	planemask = vramop.mio2[PEGC_REG_PLANE_ACCESS];
	pixelmask = LOADINTELDWORD(vramop.mio2 + PEGC_REG_MASK);
	bitlength = LOADINTELDWORD(vramop.mio2 + PEGC_REG_LENGTH) & 0x0fff;
	dstbitshift = (LOADINTELWORD(vramop.mio2+PEGC_REG_SHIFT) >> 8) & 0x1f;
	shiftdir = (LOADINTELWORD(vramop.mio2+PEGC_REG_PLANE_ROP) >> 9) & 0x1;
	srccpu = (LOADINTELWORD(vramop.mio2+PEGC_REG_PLANE_ROP) >> 8) & 0x1;
	
	// ��f�P�ʂ̃A�h���X�v�Z
	addr = (address - 0xa8000) * 8;
	addr += dstbitshift;
	addr &= 0x80000-1; // ���S�̂���

	// ???
	bit = (addr & 0x40000)?2:1;
	
	//if(!srccpu && pegc.remain!=0 && pegc.lastdatalen < 16){
	//	return; // �������ݖ���
	//}
	
	if(pegc.remain == 0){
		// �f�[�^���߂�?
		pegc.remain = (LOADINTELDWORD(vramop.mio2 + PEGC_REG_LENGTH) & 0x0fff) + 1;
	}
	
	if(!shiftdir){
		// �Ƃ肠�����C���N�������^��
		// ���� �i�Ȃ񂩂��������j
		//            +----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+
		//    i       |  0 |  1 |  2 |  3 |  4 |  5 |  6 |  7 |  8 |  9 | 10 | 11 | 12 | 13 | 14 | 15 |
		//            +----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+
		// value    �� bit7 bit6 bit5 bit4 bit3 bit2 bit1 bit0 bitF bitE bitD bitC bitB bitA bit9 bit8  
		// pixelmask�� bit7 bit6 bit5 bit4 bit3 bit2 bit1 bit0 bitF bitE bitD bitC bitB bitA bit9 bit8  planemask  SRC,DST,PAT
		//                                                                                                  ��          ��
		// plane0 vramex[0]  [1]  [2]  [3]  [4]  [5]  [6]  [7]  [8]  [9] [10] [11] [12] [13] [14] [15]    bit0        bit0
		// plane1 vramex[0]  [1]  [2]  [3]  [4]  [5]  [6]  [7]  [8]  [9] [10] [11] [12] [13] [14] [15]    bit1        bit1
		// plane2 vramex[0]  [1]  [2]  [3]  [4]  [5]  [6]  [7]  [8]  [9] [10] [11] [12] [13] [14] [15]    bit2        bit2
		// plane3 vramex[0]  [1]  [2]  [3]  [4]  [5]  [6]  [7]  [8]  [9] [10] [11] [12] [13] [14] [15]    bit3        bit3
		// plane4 vramex[0]  [1]  [2]  [3]  [4]  [5]  [6]  [7]  [8]  [9] [10] [11] [12] [13] [14] [15]    bit4        bit4
		// plane5 vramex[0]  [1]  [2]  [3]  [4]  [5]  [6]  [7]  [8]  [9] [10] [11] [12] [13] [14] [15]    bit5        bit5
		// plane6 vramex[0]  [1]  [2]  [3]  [4]  [5]  [6]  [7]  [8]  [9] [10] [11] [12] [13] [14] [15]    bit6        bit6
		// plane7 vramex[0]  [1]  [2]  [3]  [4]  [5]  [6]  [7]  [8]  [9] [10] [11] [12] [13] [14] [15]    bit7        bit7

		for(i=0;i<16;i++){
			UINT32 addrtmp = (addr + i) & (0x80000-1); // �������݈ʒu
			UINT32 pixmaskpos = (1 << ((i/8)*8 + (7-(i&0x7)))); // ���݂̉�f�ɑΉ�����value��pixelmask�̃r�b�g�ʒu
			if(pixelmask & pixmaskpos){ // �������݋֎~�`�F�b�N
				// SRC�̐ݒ�
				if(srccpu){
					// CPU�f�[�^���g��
					src = (value & pixmaskpos) ? 0xff : 0x00;
				}else{
					// ���O�ɓǂݎ����VRAM�f�[�^���g��
					src = pegc.lastdata[i];
				}

				// DST�̐ݒ� ���݂�VRAM�f�[�^�擾
				dst = vramex[addrtmp];

				if(ropupdmode){
					// ROP�g�p
					vramex[addrtmp] = (vramex[addrtmp] & planemask); // �������������\��̃r�b�g��0�ɂ��Ă���
					
					// PAT�̐ݒ�
					if(ropmethod==0){
						// �p�^�[�����W�X�^���g�p
						int col = 0;
						for(j=7;j>=0;j--){
							col |= (LOADINTELWORD(vramop.mio2 + PEGC_REG_PATTERN + j*4) >> i) & 0x1;
							col <<= 1;
						}
						pat1 = pat2 = col;
					}else if(ropmethod==1){
						// �p���b�g2���g�p
						pat1 = pat2 = vramop.mio2[PEGC_REG_PALETTE2];
					}else if(ropmethod==2){
						// �p���b�g1���g�p
						pat1 = pat2 = vramop.mio2[PEGC_REG_PALETTE1];
					}else if(ropmethod==3){
						// �p���b�g1��2���g�p
						pat1 = vramop.mio2[PEGC_REG_PALETTE1];
						pat2 = vramop.mio2[PEGC_REG_PALETTE2];
					}
					// ROP���s
					if(vramop.mio2[PEGC_REG_PLANE_ROP] & (1<<7)) vramex[addrtmp] |= (src & dst & pat1) & (~planemask);
					if(vramop.mio2[PEGC_REG_PLANE_ROP] & (1<<6)) vramex[addrtmp] |= (src & dst & ~pat1) & (~planemask);
					if(vramop.mio2[PEGC_REG_PLANE_ROP] & (1<<5)) vramex[addrtmp] |= (src & ~dst & pat1) & (~planemask);
					if(vramop.mio2[PEGC_REG_PLANE_ROP] & (1<<4)) vramex[addrtmp] |= (src & ~dst & ~pat1) & (~planemask);
					if(vramop.mio2[PEGC_REG_PLANE_ROP] & (1<<3)) vramex[addrtmp] |= (~src & dst & pat2) & (~planemask);
					if(vramop.mio2[PEGC_REG_PLANE_ROP] & (1<<2)) vramex[addrtmp] |= (~src & dst & ~pat2) & (~planemask);
					if(vramop.mio2[PEGC_REG_PLANE_ROP] & (1<<1)) vramex[addrtmp] |= (~src & ~dst & pat2) & (~planemask);
					if(vramop.mio2[PEGC_REG_PLANE_ROP] & (1<<0)) vramex[addrtmp] |= (~src & ~dst & ~pat2) & (~planemask);
				}else{
					// SRC��0�ɑ΂���planemask&DST, 1�ɑ΂���~planemask|DST
					vramex[addrtmp] = 0;
					for(j=0;j<8;j++){
						if(src & (1<<j)){
							vramex[addrtmp] |= (~planemask | dst) & (1<<j);
						}else{
							vramex[addrtmp] |= (planemask & dst) & (1<<j);
						}
					}
				}
				// ???
				vramupdate[LOW15(addrtmp >> 3)] |= bit;
			}

			pegc.remain--;
			// �]���T�C�Y�`�F�b�N
			if(pegc.remain == 0){
				goto endloop; // ������
			}
		}
	}else{
		for(i=0;i<16;i++){
			UINT32 addrtmp = (addr - i) & (0x80000-1); // �������݈ʒu
			UINT32 pixmaskpos = (1 << ((i/8)*8 + (7-(i&0x7)))); // ���݂̉�f�ɑΉ�����value��pixelmask�̃r�b�g�ʒu
			if(pixelmask & pixmaskpos){ // �������݋֎~�`�F�b�N
				// SRC�̐ݒ�
				if(srccpu){
					// CPU�f�[�^���g��
					src = (value & pixmaskpos) ? 0xff : 0x00;
				}else{
					// ���O�ɓǂݎ����VRAM�f�[�^���g��
					src = pegc.lastdata[i];
				}

				// DST�̐ݒ� ���݂�VRAM�f�[�^�擾
				dst = vramex[addrtmp];

				if(ropupdmode){
					// ROP�g�p
					vramex[addrtmp] = (vramex[addrtmp] & planemask); // �������������\��̃r�b�g��0�ɂ��Ă���
					
					// PAT�̐ݒ�
					if(ropmethod==0){
						// �p�^�[�����W�X�^���g�p
						int col = 0;
						for(j=7;j>=0;j--){
							col |= (LOADINTELWORD(vramop.mio2 + PEGC_REG_PATTERN + j*4) >> i) & 0x1;
							col <<= 1;
						}
						pat1 = pat2 = col;
					}else if(ropmethod==1){
						// �p���b�g2���g�p
						pat1 = pat2 = vramop.mio2[PEGC_REG_PALETTE2];
					}else if(ropmethod==2){
						// �p���b�g1���g�p
						pat1 = pat2 = vramop.mio2[PEGC_REG_PALETTE1];
					}else if(ropmethod==3){
						// �p���b�g1��2���g�p
						pat1 = vramop.mio2[PEGC_REG_PALETTE1];
						pat2 = vramop.mio2[PEGC_REG_PALETTE2];
					}
					// ROP���s
					if(vramop.mio2[PEGC_REG_PLANE_ROP] & (1<<7)) vramex[addrtmp] |= (src & dst & pat1) & (~planemask);
					if(vramop.mio2[PEGC_REG_PLANE_ROP] & (1<<6)) vramex[addrtmp] |= (src & dst & ~pat1) & (~planemask);
					if(vramop.mio2[PEGC_REG_PLANE_ROP] & (1<<5)) vramex[addrtmp] |= (src & ~dst & pat1) & (~planemask);
					if(vramop.mio2[PEGC_REG_PLANE_ROP] & (1<<4)) vramex[addrtmp] |= (src & ~dst & ~pat1) & (~planemask);
					if(vramop.mio2[PEGC_REG_PLANE_ROP] & (1<<3)) vramex[addrtmp] |= (~src & dst & pat2) & (~planemask);
					if(vramop.mio2[PEGC_REG_PLANE_ROP] & (1<<2)) vramex[addrtmp] |= (~src & dst & ~pat2) & (~planemask);
					if(vramop.mio2[PEGC_REG_PLANE_ROP] & (1<<1)) vramex[addrtmp] |= (~src & ~dst & pat2) & (~planemask);
					if(vramop.mio2[PEGC_REG_PLANE_ROP] & (1<<0)) vramex[addrtmp] |= (~src & ~dst & ~pat2) & (~planemask);
				}else{
					// SRC��0�ɑ΂���planemask&DST, 1�ɑ΂���~planemask|DST
					vramex[addrtmp] = 0;
					for(j=0;j<8;j++){
						if(src & (1<<j)){
							vramex[addrtmp] |= (~planemask | dst) & (1<<j);
						}else{
							vramex[addrtmp] |= (planemask & dst) & (1<<j);
						}
					}
				}
				// ???
				vramupdate[LOW15(addrtmp >> 3)] |= bit;
			}

			pegc.remain--;
			// �]���T�C�Y�`�F�b�N
			if(pegc.remain == 0){
				goto endloop; // ������
			}
		}
	}
endloop:
	gdcs.grphdisp |= bit;
	pegc.lastdatalen -= 16;
}
UINT32 MEMCALL pegc_memvgaplane_rd32(UINT32 address){
	// TODO: ���
	return 0;
}
void MEMCALL pegc_memvgaplane_wr32(UINT32 address, UINT32 value){
	// TODO: ���
}

void pegc_reset(const NP2CFG *pConfig) {

	ZeroMemory(&pegc, sizeof(pegc));
	
	pegc.enable = np2cfg.usepegcplane;

	(void)pConfig;
}

void pegc_bind(void) {

}

#endif