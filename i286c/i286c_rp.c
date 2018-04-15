#include	"compiler.h"
#include	"i286.h"
#include	"i286c.h"
#include	"memory.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"i286c.mcr"


// ---------------------------------------------------------------------- ins

I286EXT i286c_rep_insb(void) {

	I286_WORKCLOCK(5);
	if (I286_CX) {
		int stp = STRING_DIR;
		do {
			REG8 dat = iocore_inp8(I286_DX);
			i286_memorywrite(I286_DI + ES_BASE, dat);
			I286_DI += stp;
			I286_WORKCLOCK(4);
		} while (--I286_CX);
	}
}

I286EXT i286c_rep_insw(void) {

	I286_WORKCLOCK(5);
	if (I286_CX) {
		int stp = STRING_DIRx2;
		do {
			REG16 dat = iocore_inp16(I286_DX);
			i286_memorywrite_w(I286_DI + ES_BASE, dat);
			I286_DI += stp;
			I286_WORKCLOCK(4);
		} while(--I286_CX);
	}
}

// ---------------------------------------------------------------------- outs

I286EXT i286c_rep_outsb(void) {

	I286_WORKCLOCK(5);
	if (I286_CX) {
		int stp = STRING_DIR;
		do {
			REG8 dat = i286_memoryread(I286_SI + DS_FIX);
			I286_SI += stp;
			iocore_out8(I286_DX, (BYTE)dat);
			I286_WORKCLOCK(4);
		} while(--I286_CX);
	}
}

I286EXT i286c_rep_outsw(void) {

	I286_WORKCLOCK(5);
	if (I286_CX) {
		int stp = STRING_DIRx2;
		do {
			REG16 dat = i286_memoryread_w(I286_SI + DS_FIX);
			I286_SI += stp;
			iocore_out16(I286_DX, (UINT16)dat);
			I286_WORKCLOCK(4);
		} while(--I286_CX);
	}
}


// ---------------------------------------------------------------------- movs

I286EXT i286c_rep_movsb(void) {

	I286_WORKCLOCK(5);
	if (I286_CX) {
		int stp = STRING_DIR;
		while(1) {
			REG8 dat = i286_memoryread(I286_SI + DS_FIX);
			i286_memorywrite(I286_DI + ES_BASE, dat);
			I286_SI += stp;
			I286_DI += stp;
			I286_WORKCLOCK(4);
			I286_CX--;
			if (!I286_CX) {
				break;
			}
			if (I286_REMCLOCK <= 0) {
				I286_IP -= i286core.s.prefix + 1;
				break;
			}
		}
	}
}

I286EXT i286c_rep_movsw(void) {

	I286_WORKCLOCK(5);
	if (I286_CX) {
		int stp = STRING_DIRx2;
		while(1) {
			REG16 dat = i286_memoryread_w(I286_SI + DS_FIX);
			i286_memorywrite_w(I286_DI + ES_BASE, dat);
			I286_SI += stp;
			I286_DI += stp;
			I286_WORKCLOCK(4);
			I286_CX--;
			if (!I286_CX) {
				break;
			}
			if (I286_REMCLOCK <= 0) {
				I286_IP -= i286core.s.prefix + 1;
				break;
			}
		}
	}
}


// ---------------------------------------------------------------------- lods

I286EXT i286c_rep_lodsb(void) {

	I286_WORKCLOCK(5);
	if (I286_CX) {
		int stp = STRING_DIR;
		while(1) {
			I286_AL = i286_memoryread(I286_SI + DS_FIX);
			I286_SI += stp;
			I286_WORKCLOCK(4);
			I286_CX--;
			if (!I286_CX) {
				break;
			}
			if (I286_REMCLOCK <= 0) {
				I286_IP -= i286core.s.prefix + 1;
				break;
			}
		}
	}
}

I286EXT i286c_rep_lodsw(void) {

	I286_WORKCLOCK(5);
	if (I286_CX) {
		int stp = STRING_DIRx2;
		while(1) {
			I286_AX = i286_memoryread_w(I286_SI + DS_FIX);
			I286_SI += stp;
			I286_WORKCLOCK(4);
		 	I286_CX--;
		 	if (!I286_CX) {
		 		break;
		 	}
			if (I286_REMCLOCK <= 0) {
				I286_IP -= i286core.s.prefix + 1;
				break;
			}
		}
	}
}


// ---------------------------------------------------------------------- stos

I286EXT i286c_rep_stosb(void) {

	I286_WORKCLOCK(4);
	if (I286_CX) {
		int stp = STRING_DIR;
		while(1) {
			i286_memorywrite(I286_DI + ES_BASE, I286_AL);
			I286_DI += stp;
			I286_WORKCLOCK(3);
			I286_CX--;
			if (!I286_CX) {
				break;
			}
			if (I286_REMCLOCK <= 0) {
				I286_IP -= i286core.s.prefix + 1;
				break;
			}
		}
	}
}

I286EXT i286c_rep_stosw(void) {

	I286_WORKCLOCK(4);
	if (I286_CX) {
		int stp = STRING_DIRx2;
		while(1) {
			i286_memorywrite_w(I286_DI + ES_BASE, I286_AX);
			I286_DI += stp;
			I286_WORKCLOCK(3);
			I286_CX--;
			if (!I286_CX) {
				break;
			}
			if (I286_REMCLOCK <= 0) {
				I286_IP -= i286core.s.prefix + 1;
				break;
			}
		}
	}
}


// ---------------------------------------------------------------------- cmps

I286EXT i286c_repe_cmpsb(void) {

	I286_WORKCLOCK(5);
	if (I286_CX) {
		int stp = STRING_DIR;
		do {
			UINT res;
			UINT dst = i286_memoryread(I286_SI + DS_FIX);
			UINT src = i286_memoryread(I286_DI + ES_BASE);
			I286_SI += stp;
			I286_DI += stp;
			I286_WORKCLOCK(9);
			SUBBYTE(res, dst, src)
			I286_CX--;
		} while((I286_CX) && (I286_FLAGL & Z_FLAG));
	}
}

I286EXT i286c_repne_cmpsb(void) {

	I286_WORKCLOCK(5);
	if (I286_CX) {
		int stp = STRING_DIR;
		do {
			UINT res;
			UINT dst = i286_memoryread(I286_SI + DS_FIX);
			UINT src = i286_memoryread(I286_DI + ES_BASE);
			I286_SI += stp;
			I286_DI += stp;
			I286_WORKCLOCK(9);
			SUBBYTE(res, dst, src)
			I286_CX--;
		} while((I286_CX) && (!(I286_FLAGL & Z_FLAG)));
	}
}

I286EXT i286c_repe_cmpsw(void) {

	I286_WORKCLOCK(5);
	if (I286_CX) {
		int stp = STRING_DIRx2;
		do {
			UINT32 res;
			UINT32 dst = i286_memoryread_w(I286_SI + DS_FIX);
			UINT32 src = i286_memoryread_w(I286_DI + ES_BASE);
			I286_SI += stp;
			I286_DI += stp;
			I286_WORKCLOCK(9);
			SUBWORD(res, dst, src)
			I286_CX--;
		} while((I286_CX) && (I286_FLAGL & Z_FLAG));
	}
}

I286EXT i286c_repne_cmpsw(void) {

	I286_WORKCLOCK(5);
	if (I286_CX) {
		int stp = STRING_DIRx2;
		do {
			UINT32 res;
			UINT32 dst = i286_memoryread_w(I286_SI + DS_FIX);
			UINT32 src = i286_memoryread_w(I286_DI + ES_BASE);
			I286_SI += stp;
			I286_DI += stp;
			I286_WORKCLOCK(9);
			SUBWORD(res, dst, src)
			I286_CX--;
		} while((I286_CX) && (!(I286_FLAGL & Z_FLAG)));
	}
}


// ---------------------------------------------------------------------- scas

I286EXT i286c_repe_scasb(void) {

	I286_WORKCLOCK(5);
	if (I286_CX) {
		int stp = STRING_DIR;
		UINT dst = I286_AL;
		do {
			UINT res;
			UINT src = i286_memoryread(I286_DI + ES_BASE);
			I286_DI += stp;
			I286_WORKCLOCK(8);
			SUBBYTE(res, dst, src)
			I286_CX--;
		} while((I286_CX) && (I286_FLAGL & Z_FLAG));
	}
}

I286EXT i286c_repne_scasb(void) {

	I286_WORKCLOCK(5);
	if (I286_CX) {
		int stp = STRING_DIR;
		UINT dst = I286_AL;
		do {
			UINT res;
			UINT src = i286_memoryread(I286_DI + ES_BASE);
			I286_DI += stp;
			I286_WORKCLOCK(8);
			SUBBYTE(res, dst, src)
			I286_CX--;
		} while((I286_CX) && (!(I286_FLAGL & Z_FLAG)));
	}
}

I286EXT i286c_repe_scasw(void) {

	I286_WORKCLOCK(5);
	if (I286_CX) {
		int stp = STRING_DIRx2;
		UINT32 dst = I286_AX;
		do {
			UINT32 res;
			UINT32 src = i286_memoryread_w(I286_DI + ES_BASE);
			I286_DI += stp;
			I286_WORKCLOCK(8);
			SUBWORD(res, dst, src)
			I286_CX--;
		} while((I286_CX) && (I286_FLAGL & Z_FLAG));
	}
}

I286EXT i286c_repne_scasw(void) {

	I286_WORKCLOCK(5);
	if (I286_CX) {
		int stp = STRING_DIRx2;
		UINT32 dst = I286_AX;
		do {
			UINT32 res;
			UINT32 src = i286_memoryread_w(I286_DI + ES_BASE);
			I286_DI += stp;
			I286_WORKCLOCK(8);
			SUBWORD(res, dst, src)
			I286_CX--;
		} while((I286_CX) && (!(I286_FLAGL & Z_FLAG)));
	}
}

