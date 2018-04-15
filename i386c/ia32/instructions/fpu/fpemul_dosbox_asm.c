/*
 *  Copyright (C) 2002-2011  The DOSBox Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/*
 * Copyright (c) 2012 NONAKA Kimihiro
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * modified by SimK
 */

#include "compiler.h"

#ifdef USE_FPU
#ifdef USE_FPU_ASM

#include <float.h>
#include <math.h>
#include "ia32/cpu.h"
#include "ia32/ia32.mcr"

#include "ia32/instructions/fpu/fp.h"

#if 1
#undef	TRACEOUT
#define	TRACEOUT(s)	(void)(s)
#endif	/* 0 */

#define FPU_WORKCLOCK	6

//#define WEAK_EXCEPTIONS


#if defined (_MSC_VER)

#ifdef WEAK_EXCEPTIONS
#define clx
#else
#define clx fclex
#endif

#ifdef WEAK_EXCEPTIONS
#define FPUD_LOAD(op,szI,szA)			\
		__asm {							\
		__asm	mov		ebx, store_to	\
		__asm	shl		ebx, 4			\
		__asm	op		szI PTR FPU_STAT.p_reg[128].m1		\
		__asm	fstp	TBYTE PTR FPU_STAT.p_reg[ebx].m1	\
		}
#else
#define FPUD_LOAD(op,szI,szA)			\
		{UINT16 new_sw;					\
		__asm {							\
		__asm	mov		eax, 8			\
		__asm	shl		eax, 4			\
		__asm	mov		ebx, store_to	\
		__asm	shl		ebx, 4			\
		__asm	fclex					\
		__asm	op		szI PTR FPU_STAT.p_reg[eax].m1		\
		__asm	fnstsw	new_sw			\
		__asm	fstp	TBYTE PTR FPU_STAT.p_reg[ebx].m1	\
		}								\
		FPU_STATUSWORD=(new_sw&0xffbf)|(FPU_STATUSWORD&0x80ff);}
#endif

#ifdef WEAK_EXCEPTIONS
#define FPUD_LOAD_EA(op,szI,szA)		\
		__asm {							\
		__asm	op		szI PTR FPU_STAT.p_reg[128].m1		\
		}
#else
#define FPUD_LOAD_EA(op,szI,szA)		\
		{UINT16 new_sw;					\
		__asm {							\
		__asm	mov		eax, 8			\
		__asm	shl		eax, 4			\
		__asm	fclex					\
		__asm	op		szI PTR FPU_STAT.p_reg[eax].m1		\
		__asm	fnstsw	new_sw			\
		}								\
		FPU_STATUSWORD=(new_sw&0xffbf)|(FPU_STATUSWORD&0x80ff);}
#endif

#ifdef WEAK_EXCEPTIONS
#define FPUD_STORE(op,szI,szA)				\
		{UINT16 save_cw;						\
		__asm {								\
		__asm	fnstcw	save_cw				\
		__asm	mov		eax, FPU_STAT_TOP	\
		__asm	fldcw	FPU_CTRLWORDMASK		\
		__asm	shl		eax, 4				\
		__asm	fld		TBYTE PTR FPU_STAT.p_reg[eax].m1	\
		__asm	op		szI PTR FPU_STAT.p_reg[128].m1		\
		__asm	fldcw	save_cw				\
		}}
#else
#define FPUD_STORE(op,szI,szA)				\
		{UINT16 new_sw,save_cw;				\
		__asm {								\
		__asm	fnstcw	save_cw				\
		__asm	fldcw	FPU_CTRLWORDMASK		\
		__asm	mov		eax, FPU_STAT_TOP	\
		__asm	shl		eax, 4				\
		__asm	mov		ebx, 8				\
		__asm	shl		ebx, 4				\
		__asm	fld		TBYTE PTR FPU_STAT.p_reg[eax].m1	\
		__asm	clx							\
		__asm	op		szI PTR FPU_STAT.p_reg[ebx].m1		\
		__asm	fnstsw	new_sw				\
		__asm	fldcw	save_cw				\
		}									\
		FPU_STATUSWORD=(new_sw&exc_mask)|(FPU_STATUSWORD&0x80ff);}
#endif

// handles fsin,fcos,f2xm1,fchs,fabs
#define FPUD_TRIG(op)				\
		{UINT16 new_sw;				\
		__asm {						\
		__asm	mov		eax, FPU_STAT_TOP	\
		__asm	shl		eax, 4		\
		__asm	fld		TBYTE PTR FPU_STAT.p_reg[eax].m1	\
		__asm	clx					\
		__asm	op					\
		__asm	fnstsw	new_sw		\
		__asm	fstp	TBYTE PTR FPU_STAT.p_reg[eax].m1	\
		}							\
		FPU_STATUSWORD=(new_sw&exc_mask)|(FPU_STATUSWORD&0x80ff);}

// handles fsincos
#define FPUD_SINCOS()				\
		{UINT16 new_sw;					\
		__asm {							\
		__asm	mov		eax, FPU_STAT_TOP		\
		__asm	mov		ebx, eax		\
		__asm	dec     ebx				\
		__asm	and     ebx, 7			\
		__asm	shl		eax, 4			\
		__asm	shl		ebx, 4			\
		__asm	fld		TBYTE PTR FPU_STAT.p_reg[eax].m1	\
		__asm	clx						\
		__asm	fsincos					\
		__asm	fnstsw	new_sw			\
		__asm	mov		cx, new_sw		\
		__asm	and		ch, 0x04 		\
		__asm	jnz		argument_too_large1				\
		__asm	fstp	TBYTE PTR FPU_STAT.p_reg[ebx].m1	\
		__asm	fstp	TBYTE PTR FPU_STAT.p_reg[eax].m1	\
		__asm	jmp		end_sincos		\
		__asm	argument_too_large1:	\
		__asm	fstp	st(0)			\
		__asm	end_sincos:				\
		}												\
		FPU_STATUSWORD=(new_sw&exc_mask)|(FPU_STATUSWORD&0x80ff);		\
		if ((new_sw&0x0400)==0) FPU_PREP_PUSH();}

// handles fptan
#define FPUD_PTAN()					\
		{UINT16 new_sw;					\
		__asm {							\
		__asm	mov		eax, FPU_STAT_TOP		\
		__asm	mov		ebx, eax		\
		__asm	dec     ebx				\
		__asm	and     ebx, 7			\
		__asm	shl		eax, 4			\
		__asm	shl		ebx, 4			\
		__asm	fld		TBYTE PTR FPU_STAT.p_reg[eax].m1	\
		__asm	clx					\
		__asm	fptan					\
		__asm	fnstsw	new_sw			\
		__asm	mov		cx, new_sw		\
		__asm	and		ch, 0x04 		\
		__asm	jnz		argument_too_large2				\
		__asm	fstp	TBYTE PTR FPU_STAT.p_reg[ebx].m1	\
		__asm	fstp	TBYTE PTR FPU_STAT.p_reg[eax].m1	\
		__asm	jmp		end_ptan		\
		__asm	argument_too_large2:	\
		__asm	fstp	st(0)			\
		__asm	end_ptan:				\
		}												\
		FPU_STATUSWORD=(new_sw&exc_mask)|(FPU_STATUSWORD&0x80ff);		\
		if ((new_sw&0x0400)==0) FPU_PREP_PUSH();}

// handles fxtract
#ifdef WEAK_EXCEPTIONS
#define FPUD_XTRACT						\
		__asm {							\
		__asm	mov		eax, FPU_STAT_TOP		\
		__asm	mov		ebx, eax		\
		__asm	dec     ebx				\
		__asm	and     ebx, 7			\
		__asm	shl		eax, 4			\
		__asm	shl		ebx, 4			\
		__asm	fld		TBYTE PTR FPU_STAT.p_reg[eax].m1	\
		__asm	fxtract					\
		__asm	fstp	TBYTE PTR FPU_STAT.p_reg[ebx].m1	\
		__asm	fstp	TBYTE PTR FPU_STAT.p_reg[eax].m1	\
		}												\
		FPU_PREP_PUSH();
#else
#define FPUD_XTRACT						\
		{UINT16 new_sw;					\
		__asm {							\
		__asm	mov		eax, FPU_STAT_TOP		\
		__asm	mov		ebx, eax		\
		__asm	dec     ebx				\
		__asm	and     ebx, 7			\
		__asm	shl		eax, 4			\
		__asm	shl		ebx, 4			\
		__asm	fld		TBYTE PTR FPU_STAT.p_reg[eax].m1	\
		__asm	fclex					\
		__asm	fxtract					\
		__asm	fnstsw	new_sw			\
		__asm	fstp	TBYTE PTR FPU_STAT.p_reg[ebx].m1	\
		__asm	fstp	TBYTE PTR FPU_STAT.p_reg[eax].m1	\
		}												\
		FPU_STATUSWORD=(new_sw&0xffbf)|(FPU_STATUSWORD&0x80ff);			\
		FPU_PREP_PUSH();}
#endif

// handles fadd,fmul,fsub,fsubr
#ifdef WEAK_EXCEPTIONS
#define FPUD_ARITH1(op)						\
		{UINT16 save_cw;						\
		__asm {								\
		__asm	fnstcw	save_cw				\
		__asm	mov		eax, op1			\
		__asm	shl		eax, 4				\
		__asm	fldcw	FPU_CTRLWORDMASK		\
		__asm	mov		ebx, op2			\
		__asm	shl		ebx, 4				\
		__asm	fld		TBYTE PTR FPU_STAT.p_reg[eax].m1	\
		__asm	fld		TBYTE PTR FPU_STAT.p_reg[ebx].m1	\
		__asm	op		st(1), st(0)		\
		__asm	fstp	TBYTE PTR FPU_STAT.p_reg[eax].m1	 \
		__asm	fldcw	save_cw				\
		}}
#else
#define FPUD_ARITH1(op)						\
		{UINT16 new_sw,save_cw;				\
		__asm {								\
		__asm	fnstcw	save_cw				\
		__asm	fldcw	FPU_CTRLWORDMASK		\
		__asm	mov		eax, op1			\
		__asm	shl		eax, 4				\
		__asm	mov		ebx, op2			\
		__asm	shl		ebx, 4				\
		__asm	fld		TBYTE PTR FPU_STAT.p_reg[eax].m1	\
		__asm	fld		TBYTE PTR FPU_STAT.p_reg[ebx].m1	\
		__asm	clx							\
		__asm	op		st(1), st(0)		\
		__asm	fnstsw	new_sw				\
		__asm	fstp	TBYTE PTR FPU_STAT.p_reg[eax].m1	 \
		__asm	fldcw	save_cw				\
		}									\
		FPU_STATUSWORD=(new_sw&exc_mask)|(FPU_STATUSWORD&0x80ff);}
#endif

// handles fadd,fmul,fsub,fsubr
#ifdef WEAK_EXCEPTIONS
#define FPUD_ARITH1_EA(op)					\
		{UINT16 save_cw;						\
		__asm {								\
		__asm	fnstcw	save_cw				\
		__asm	mov		eax, op1			\
		__asm	fldcw	FPU_CTRLWORDMASK		\
		__asm	shl		eax, 4				\
		__asm	fld		TBYTE PTR FPU_STAT.p_reg[eax].m1	\
		__asm	fxch	\
		__asm	op		st(1), st(0)		\
		__asm	fstp	TBYTE PTR FPU_STAT.p_reg[eax].m1	 \
		__asm	fldcw	save_cw				\
		}}
#else
#define FPUD_ARITH1_EA(op)					\
		{UINT16 new_sw,save_cw;				\
		__asm {								\
		__asm	fnstcw	save_cw				\
		__asm	fldcw	FPU_CTRLWORDMASK		\
		__asm	mov		eax, op1			\
		__asm	shl		eax, 4				\
		__asm	fld		TBYTE PTR FPU_STAT.p_reg[eax].m1	\
		__asm	fxch	\
		__asm	clx							\
		__asm	op		st(1), st(0)		\
		__asm	fnstsw	new_sw				\
		__asm	fstp	TBYTE PTR FPU_STAT.p_reg[eax].m1	 \
		__asm	fldcw	save_cw				\
		}									\
		FPU_STATUSWORD=(new_sw&exc_mask)|(FPU_STATUSWORD&0x80ff);}
#endif

// handles fsqrt,frndint
#ifdef WEAK_EXCEPTIONS
#define FPUD_ARITH2(op)						\
		{UINT16 save_cw;						\
		__asm {								\
		__asm	fnstcw	save_cw				\
		__asm	mov		eax, FPU_STAT_TOP			\
		__asm	fldcw	FPU_CTRLWORDMASK		\
		__asm	shl		eax, 4				\
		__asm	fld		TBYTE PTR FPU_STAT.p_reg[eax].m1	\
		__asm	op							\
		__asm	fstp	TBYTE PTR FPU_STAT.p_reg[eax].m1	 \
		__asm	fldcw	save_cw				\
		}}
#else
#define FPUD_ARITH2(op)						\
		{UINT16 new_sw,save_cw;				\
		__asm {								\
		__asm	fnstcw	save_cw				\
		__asm	fldcw	FPU_CTRLWORDMASK		\
		__asm	mov		eax, FPU_STAT_TOP			\
		__asm	shl		eax, 4				\
		__asm	fld		TBYTE PTR FPU_STAT.p_reg[eax].m1	\
		__asm	clx							\
		__asm	op							\
		__asm	fnstsw	new_sw				\
		__asm	fstp	TBYTE PTR FPU_STAT.p_reg[eax].m1	 \
		__asm	fldcw	save_cw				\
		}									\
		FPU_STATUSWORD=(new_sw&exc_mask)|(FPU_STATUSWORD&0x80ff);}
#endif

// handles fdiv,fdivr
#ifdef WEAK_EXCEPTIONS
#define FPUD_ARITH3(op)						\
		{UINT16 save_cw;						\
		__asm {								\
		__asm	fnstcw	save_cw				\
		__asm	mov		eax, op1			\
		__asm	shl		eax, 4				\
		__asm	fldcw	FPU_CTRLWORDMASK		\
		__asm	mov		ebx, op2			\
		__asm	shl		ebx, 4				\
		__asm	fld		TBYTE PTR FPU_STAT.p_reg[eax].m1	\
		__asm	fld		TBYTE PTR FPU_STAT.p_reg[ebx].m1	\
		__asm	op		st(1), st(0)		\
		__asm	fstp	TBYTE PTR FPU_STAT.p_reg[eax].m1	 \
		__asm	fldcw	save_cw				\
		}}
#else
#define FPUD_ARITH3(op)						\
		{UINT16 new_sw,save_cw;				\
		__asm {								\
		__asm	fnstcw	save_cw				\
		__asm	fldcw	FPU_CTRLWORDMASK		\
		__asm	mov		eax, op1			\
		__asm	shl		eax, 4				\
		__asm	mov		ebx, op2			\
		__asm	shl		ebx, 4				\
		__asm	fld		TBYTE PTR FPU_STAT.p_reg[eax].m1	\
		__asm	fld		TBYTE PTR FPU_STAT.p_reg[ebx].m1	\
		__asm	fclex						\
		__asm	op		st(1), st(0)		\
		__asm	fnstsw	new_sw				\
		__asm	fstp	TBYTE PTR FPU_STAT.p_reg[eax].m1	 \
		__asm	fldcw	save_cw				\
		}									\
		FPU_STATUSWORD=(new_sw&0xffbf)|(FPU_STATUSWORD&0x80ff);}
#endif

// handles fdiv,fdivr
#ifdef WEAK_EXCEPTIONS
#define FPUD_ARITH3_EA(op)					\
		{UINT16 save_cw;						\
		__asm {								\
		__asm	fnstcw	save_cw				\
		__asm	mov		eax, op1			\
		__asm	fldcw	FPU_CTRLWORDMASK		\
		__asm	shl		eax, 4				\
		__asm	fld		TBYTE PTR FPU_STAT.p_reg[eax].m1	\
		__asm	fxch	\
		__asm	op		st(1), st(0)		\
		__asm	fstp	TBYTE PTR FPU_STAT.p_reg[eax].m1	 \
		__asm	fldcw	save_cw				\
		}}
#else
#define FPUD_ARITH3_EA(op)					\
		{UINT16 new_sw,save_cw;				\
		__asm {								\
		__asm	fnstcw	save_cw				\
		__asm	mov		eax, op1			\
		__asm	fldcw	FPU_CTRLWORDMASK		\
		__asm	shl		eax, 4				\
		__asm	fld		TBYTE PTR FPU_STAT.p_reg[eax].m1	\
		__asm	fxch	\
		__asm	fclex						\
		__asm	op		st(1), st(0)		\
		__asm	fnstsw	new_sw				\
		__asm	fstp	TBYTE PTR FPU_STAT.p_reg[eax].m1	 \
		__asm	fldcw	save_cw				\
		}									\
		FPU_STATUSWORD=(new_sw&0xffbf)|(FPU_STATUSWORD&0x80ff);}
#endif

// handles fprem,fprem1,fscale
#define FPUD_REMINDER(op)			\
		{UINT16 new_sw;				\
		__asm {						\
		__asm	mov		eax, FPU_STAT_TOP	\
		__asm	mov		ebx, eax	\
		__asm	inc     ebx			\
		__asm	and     ebx, 7		\
		__asm	shl		ebx, 4		\
		__asm	shl		eax, 4		\
		__asm	fld		TBYTE PTR FPU_STAT.p_reg[ebx].m1	\
		__asm	fld		TBYTE PTR FPU_STAT.p_reg[eax].m1	\
		__asm	fclex				\
		__asm	op					\
		__asm	fnstsw	new_sw		\
		__asm	fstp	TBYTE PTR FPU_STAT.p_reg[eax].m1	\
		__asm	fstp	st(0)		\
		}							\
		FPU_STATUSWORD=(new_sw&0xffbf)|(FPU_STATUSWORD&0x80ff);}

// handles fcom,fucom
#define FPUD_COMPARE(op)			\
		{UINT16 new_sw;				\
		__asm {						\
		__asm	mov		ebx, op2	\
		__asm	mov		eax, op1	\
		__asm	shl		ebx, 4		\
		__asm	shl		eax, 4		\
		__asm	fld		TBYTE PTR FPU_STAT.p_reg[ebx].m1	\
		__asm	fld		TBYTE PTR FPU_STAT.p_reg[eax].m1	\
		__asm	clx					\
		__asm	op					\
		__asm	fnstsw	new_sw		\
		}							\
		FPU_STATUSWORD=(new_sw&exc_mask)|(FPU_STATUSWORD&0x80ff);}

#define FPUD_COMPARE_EA(op)			\
		{UINT16 new_sw;				\
		__asm {						\
		__asm	mov		eax, op1	\
		__asm	shl		eax, 4		\
		__asm	fld		TBYTE PTR FPU_STAT.p_reg[eax].m1	\
		__asm	clx					\
		__asm	op					\
		__asm	fnstsw	new_sw		\
		}							\
		FPU_STATUSWORD=(new_sw&exc_mask)|(FPU_STATUSWORD&0x80ff);}

// handles fxam,ftst
#define FPUD_EXAMINE(op)			\
		{UINT16 new_sw;				\
		__asm {						\
		__asm	mov		eax, FPU_STAT_TOP	\
		__asm	shl		eax, 4		\
		__asm	fld		TBYTE PTR FPU_STAT.p_reg[eax].m1	\
		__asm	clx					\
		__asm	op					\
		__asm	fnstsw	new_sw		\
		__asm	fstp	st(0)		\
		}							\
		FPU_STATUSWORD=(new_sw&exc_mask)|(FPU_STATUSWORD&0x80ff);}

// handles fpatan,fyl2xp1
#ifdef WEAK_EXCEPTIONS
#define FPUD_WITH_POP(op)			\
		__asm {						\
		__asm	mov		eax, FPU_STAT_TOP	\
		__asm	mov		ebx, eax	\
		__asm	inc     ebx			\
		__asm	and     ebx, 7		\
		__asm	shl		ebx, 4		\
		__asm	shl		eax, 4		\
		__asm	fld		TBYTE PTR FPU_STAT.p_reg[ebx].m1	\
		__asm	fld		TBYTE PTR FPU_STAT.p_reg[eax].m1	\
		__asm	op					\
		__asm	fstp	TBYTE PTR FPU_STAT.p_reg[ebx].m1	\
		}							\
		FPU_FPOP();
#else
#define FPUD_WITH_POP(op)			\
		{UINT16 new_sw;				\
		__asm {						\
		__asm	mov		eax, FPU_STAT_TOP	\
		__asm	mov		ebx, eax	\
		__asm	inc     ebx			\
		__asm	and     ebx, 7		\
		__asm	shl		ebx, 4		\
		__asm	shl		eax, 4		\
		__asm	fld		TBYTE PTR FPU_STAT.p_reg[ebx].m1	\
		__asm	fld		TBYTE PTR FPU_STAT.p_reg[eax].m1	\
		__asm	fclex				\
		__asm	op					\
		__asm	fnstsw	new_sw		\
		__asm	fstp	TBYTE PTR FPU_STAT.p_reg[ebx].m1	\
		}								\
		FPU_STATUSWORD=(new_sw&0xffbf)|(FPU_STATUSWORD&0x80ff);	\
		FPU_FPOP();}
#endif

// handles fyl2x
#ifdef WEAK_EXCEPTIONS
#define FPUD_FYL2X(op)				\
		__asm {						\
		__asm	mov		eax, FPU_STAT_TOP	\
		__asm	mov		ebx, eax	\
		__asm	inc     ebx			\
		__asm	and     ebx, 7		\
		__asm	shl		ebx, 4		\
		__asm	shl		eax, 4		\
		__asm	fld		TBYTE PTR FPU_STAT.p_reg[ebx].m1	\
		__asm	fld		TBYTE PTR FPU_STAT.p_reg[eax].m1	\
		__asm	op					\
		__asm	fstp	TBYTE PTR FPU_STAT.p_reg[ebx].m1	\
		}								\
		FPU_FPOP();
#else
#define FPUD_FYL2X(op)				\
		{UINT16 new_sw;				\
		__asm {						\
		__asm	mov		eax, FPU_STAT_TOP	\
		__asm	mov		ebx, eax	\
		__asm	inc     ebx			\
		__asm	and     ebx, 7		\
		__asm	shl		ebx, 4		\
		__asm	shl		eax, 4		\
		__asm	fld		TBYTE PTR FPU_STAT.p_reg[ebx].m1	\
		__asm	fld		TBYTE PTR FPU_STAT.p_reg[eax].m1	\
		__asm	fclex				\
		__asm	op					\
		__asm	fnstsw	new_sw		\
		__asm	fstp	TBYTE PTR FPU_STAT.p_reg[ebx].m1	\
		}								\
		FPU_STATUSWORD=(new_sw&0xffbf)|(FPU_STATUSWORD&0x80ff);	\
		FPU_FPOP();}
#endif

// load math constants
#define FPUD_LOAD_CONST(op)		\
		FPU_PREP_PUSH();			\
		__asm {						\
		__asm	mov		eax, FPU_STAT_TOP	\
		__asm	shl		eax, 4		\
		__asm	clx					\
		__asm	op					\
		__asm	fstp	TBYTE PTR FPU_STAT.p_reg[eax].m1	\
		}							\

#else

#ifdef WEAK_EXCEPTIONS
#define clx
#else
#define clx "fclex"
#endif

#ifdef WEAK_EXCEPTIONS
#define FPUD_LOAD(op,szI,szA)				\
		__asm__ volatile (					\
			"movl		$128, %%eax		\n"	\
			"shl		$4, %0			\n"	\
			#op #szA "	(%1, %%eax)		\n"	\
			"fstpt		(%1, %0)		"	\
			:								\
			:	"r" (store_to), "r" (FPU_STAT.p_reg)	\
			:	"eax", "memory"						\
		);
#else
#define FPUD_LOAD(op,szI,szA)				\
		{UINT16 new_sw;						\
		__asm__ volatile (					\
			"movl		$8, %%eax		\n"	\
			"shl		$4, %%eax		\n"	\
			"shl		$4, %1			\n"	\
			"fclex						\n"	\
			#op #szA "	(%2, %%eax)		\n"	\
			"fnstsw		%0				\n"	\
			"fstpt		(%2, %1)		"	\
			:	"=m" (new_sw)				\
			:	"r" (store_to), "r" (FPU_STAT.p_reg)	\
			:	"eax", "memory"						\
		);									\
		FPU_STATUSWORD=(new_sw&0xffbf)|(FPU_STATUSWORD&0x80ff);}
#endif

#ifdef WEAK_EXCEPTIONS
#define FPUD_LOAD_EA(op,szI,szA)			\
		__asm__ volatile (					\
			"movl		$128, %%eax		\n"	\
			#op #szA "	(%0, %%eax)		\n"	\
			:								\
			:	"r" (FPU_STAT.p_reg)			\
			:	"eax", "memory"				\
		);
#else
#define FPUD_LOAD_EA(op,szI,szA)			\
		{UINT16 new_sw;						\
		__asm__ volatile (					\
			"movl		$8, %%eax		\n"	\
			"shl		$4, %%eax		\n"	\
			"fclex						\n"	\
			#op #szA "	(%1, %%eax)		\n"	\
			"fnstsw		%0				\n"	\
			:	"=m" (new_sw)				\
			:	"r" (FPU_STAT.p_reg)			\
			:	"eax", "memory"				\
		);									\
		FPU_STATUSWORD=(new_sw&0xffbf)|(FPU_STATUSWORD&0x80ff);}
#endif

#ifdef WEAK_EXCEPTIONS
#define FPUD_STORE(op,szI,szA)				\
		{UINT16 save_cw;						\
		__asm__ volatile (					\
			"fnstcw		%0				\n"	\
			"shll		$4, %1			\n"	\
			"fldcw		%3				\n"	\
			"movl		$128, %%eax		\n"	\
			"fldt		(%2, %1)		\n"	\
			#op #szA "	(%2, %%eax)		\n"	\
			"fldcw		%0				"	\
			:	"=m" (save_cw)				\
			:	"r" (FPU_STAT_TOP), "r" (FPU_STAT.p_reg), "m" (FPU_CTRLWORDMASK)		\
			:	"eax", "memory"						\
			);}
#else
#define FPUD_STORE(op,szI,szA)				\
		{UINT16 new_sw,save_cw;				\
		__asm__ volatile (					\
			"fnstcw		%1				\n"	\
			"fldcw		%4				\n"	\
			"shll		$4, %2			\n"	\
			"movl		$8, %%eax		\n"	\
			"shl		$4, %%eax		\n"	\
			"fldt		(%3, %2)		\n"	\
			clx" 						\n"	\
			#op #szA "	(%3, %%eax)		\n"	\
			"fnstsw		%0				\n"	\
			"fldcw		%1				"	\
			:	"=m" (new_sw), "=m" (save_cw)	\
			:	"r" (FPU_STAT_TOP), "r" (FPU_STAT.p_reg), "m" (FPU_CTRLWORDMASK)		\
			:	"eax", "memory"						\
		);										\
		FPU_STATUSWORD=(new_sw&exc_mask)|(FPU_STATUSWORD&0x80ff);}
#endif

// handles fsin,fcos,f2xm1,fchs,fabs
#define FPUD_TRIG(op)						\
		{UINT16 new_sw;						\
		__asm__ volatile (					\
			"shll		$4, %1			\n"	\
			"fldt		(%2, %1)		\n"	\
			clx" 						\n"	\
			#op" 						\n"	\
			"fnstsw		%0				\n"	\
			"fstpt		(%2, %1)		"	\
			:	"=m" (new_sw)				\
			:	"r" (FPU_STAT_TOP), "r" (FPU_STAT.p_reg)	\
			:	"memory"					\
		);									\
		FPU_STATUSWORD=(new_sw&exc_mask)|(FPU_STATUSWORD&0x80ff);}

// handles fsincos
#define FPUD_SINCOS()					\
		{UINT16 new_sw;						\
		__asm__ volatile (					\
			"movl		%1, %%eax		\n"	\
			"shll		$4, %1			\n"	\
			"decl		%%eax			\n"	\
			"andl		$7, %%eax		\n"	\
			"shll		$4, %%eax		\n"	\
			"fldt		(%2, %1)		\n"	\
			clx" 						\n"	\
			"fsincos					\n"	\
			"fnstsw		%0				\n"	\
			"fstpt		(%2, %%eax)		\n"	\
			"movw		%0, %%ax		\n"	\
			"sahf						\n"	\
			"jp			argument_too_large1		\n"	\
			"fstpt		(%2, %1)		\n"	\
			"argument_too_large1:		"	\
			:	"=m" (new_sw)				\
			:	"r" (FPU_STAT_TOP), "r" (FPU_STAT.p_reg)	\
			:	"eax", "cc", "memory"		\
		);									\
		FPU_STATUSWORD=(new_sw&exc_mask)|(FPU_STATUSWORD&0x80ff);		\
		if ((new_sw&0x0400)==0) FPU_PREP_PUSH();}

// handles fptan
#define FPUD_PTAN()						\
		{UINT16 new_sw;						\
		__asm__ volatile (					\
			"movl		%1, %%eax		\n"	\
			"shll		$4, %1			\n"	\
			"decl		%%eax			\n"	\
			"andl		$7, %%eax		\n"	\
			"shll		$4, %%eax		\n"	\
			"fldt		(%2, %1)		\n"	\
			clx" 						\n"	\
			"fptan 						\n"	\
			"fnstsw		%0				\n"	\
			"fstpt		(%2, %%eax)		\n"	\
			"movw		%0, %%ax		\n"	\
			"sahf						\n"	\
			"jp			argument_too_large2		\n"	\
			"fstpt		(%2, %1)		\n"	\
			"argument_too_large2:		"	\
			:	"=m" (new_sw)				\
			:	"r" (FPU_STAT_TOP), "r" (FPU_STAT.p_reg)	\
			:	"eax", "cc", "memory"		\
		);									\
		FPU_STATUSWORD=(new_sw&exc_mask)|(FPU_STATUSWORD&0x80ff);		\
		if ((new_sw&0x0400)==0) FPU_PREP_PUSH();}

// handles fxtract
#ifdef WEAK_EXCEPTIONS
#define FPUD_XTRACT						\
		__asm__ volatile (					\
			"movl		%0, %%eax		\n"	\
			"shll		$4, %0			\n"	\
			"decl		%%eax			\n"	\
			"andl		$7, %%eax		\n"	\
			"shll		$4, %%eax		\n"	\
			"fldt		(%1, %0)		\n"	\
			"fxtract					\n"	\
			"fstpt		(%1, %%eax)		\n"	\
			"fstpt		(%1, %0)		"	\
			:								\
			:	"r" (FPU_STAT_TOP), "r" (FPU_STAT.p_reg)	\
			:	"eax", "memory"				\
		);									\
		FPU_PREP_PUSH();
#else
#define FPUD_XTRACT						\
		{UINT16 new_sw;						\
		__asm__ volatile (					\
			"movl		%1, %%eax		\n"	\
			"shll		$4, %1			\n"	\
			"decl		%%eax			\n"	\
			"andl		$7, %%eax		\n"	\
			"shll		$4, %%eax		\n"	\
			"fldt		(%2, %1)		\n"	\
			"fclex						\n"	\
			"fxtract					\n"	\
			"fnstsw		%0				\n"	\
			"fstpt		(%2, %%eax)		\n"	\
			"fstpt		(%2, %1)		"	\
			:	"=m" (new_sw)				\
			:	"r" (FPU_STAT_TOP), "r" (FPU_STAT.p_reg)	\
			:	"eax", "memory"						\
		);									\
		FPU_STATUSWORD=(new_sw&0xffbf)|(FPU_STATUSWORD&0x80ff);		\
		FPU_PREP_PUSH();}
#endif

// handles fadd,fmul,fsub,fsubr
#ifdef WEAK_EXCEPTIONS
#define FPUD_ARITH1(op)						\
		{UINT16 save_cw;						\
		__asm__ volatile (					\
			"fnstcw		%0				\n"	\
			"fldcw		%4				\n"	\
			"shll		$4, %2			\n"	\
			"shll		$4, %1			\n"	\
			"fldt		(%3, %2)		\n"	\
			"fldt		(%3, %1)		\n"	\
			#op"						\n"	\
			"fstpt		(%3, %1)		\n"	\
			"fldcw		%0				"	\
			:	"=m" (save_cw)		\
			:	"r" (op1), "r" (op2), "r" (FPU_STAT.p_reg), "m" (FPU_CTRLWORDMASK)		\
			:	"memory"				\
			);}
#else
#define FPUD_ARITH1(op)						\
		{UINT16 new_sw,save_cw;				\
		__asm__ volatile (					\
			"fnstcw		%1				\n"	\
			"fldcw		%5				\n"	\
			"shll		$4, %3			\n"	\
			"shll		$4, %2			\n"	\
			"fldt		(%4, %3)		\n"	\
			"fldt		(%4, %2)		\n"	\
			clx" 						\n"	\
			#op"						\n"	\
			"fnstsw		%0				\n"	\
			"fstpt		(%4, %2)		\n"	\
			"fldcw		%1				"	\
			:	"=m" (new_sw), "=m" (save_cw)		\
			:	"r" (op1), "r" (op2), "r" (FPU_STAT.p_reg), "m" (FPU_CTRLWORDMASK)		\
			:	"memory"				\
		);									\
		FPU_STATUSWORD=(new_sw&exc_mask)|(FPU_STATUSWORD&0x80ff);}
#endif

// handles fadd,fmul,fsub,fsubr
#ifdef WEAK_EXCEPTIONS
#define FPUD_ARITH1_EA(op)					\
		{UINT16 save_cw;						\
		__asm__ volatile (					\
			"fnstcw		%0				\n"	\
			"fldcw		%3				\n"	\
			"shll		$4, %1			\n"	\
			"fldt		(%2, %1)		\n"	\
			#op"						\n"	\
			"fstpt		(%2, %1)		\n"	\
			"fldcw		%0				"	\
			:	"=m" (save_cw)		\
			:	"r" (op1), "r" (FPU_STAT.p_reg), "m" (FPU_CTRLWORDMASK)		\
			:	"memory"				\
			);}
#else
#define FPUD_ARITH1_EA(op)					\
		{UINT16 new_sw,save_cw;				\
		__asm__ volatile (					\
			"fnstcw		%1				\n"	\
			"fldcw		%4				\n"	\
			"shll		$4, %2			\n"	\
			"fldt		(%3, %2)		\n"	\
			clx" 						\n"	\
			#op"						\n"	\
			"fnstsw		%0				\n"	\
			"fstpt		(%3, %2)		\n"	\
			"fldcw		%1				"	\
			:	"=m" (new_sw), "=m" (save_cw)		\
			:	"r" (op1), "r" (FPU_STAT.p_reg), "m" (FPU_CTRLWORDMASK)		\
			:	"memory"				\
		);									\
		FPU_STATUSWORD=(new_sw&exc_mask)|(FPU_STATUSWORD&0x80ff);}
#endif

// handles fsqrt,frndint
#ifdef WEAK_EXCEPTIONS
#define FPUD_ARITH2(op)						\
		{UINT16 save_cw;						\
		__asm__ volatile (					\
			"fnstcw		%0				\n"	\
			"fldcw		%3				\n"	\
			"shll		$4, %1			\n"	\
			"fldt		(%2, %1)		\n"	\
			#op" 						\n"	\
			"fstpt		(%2, %1)		\n"	\
			"fldcw		%0				"	\
			:	"=m" (save_cw)				\
			:	"r" (FPU_STAT_TOP), "r" (FPU_STAT.p_reg), "m" (FPU_CTRLWORDMASK)		\
			:	"memory"				\
			);}
#else
#define FPUD_ARITH2(op)						\
		{UINT16 new_sw,save_cw;				\
		__asm__ volatile (					\
			"fnstcw		%1				\n"	\
			"fldcw		%4				\n"	\
			"shll		$4, %2			\n"	\
			"fldt		(%3, %2)		\n"	\
			clx" 						\n"	\
			#op" 						\n"	\
			"fnstsw		%0				\n"	\
			"fstpt		(%3, %2)		\n"	\
			"fldcw		%1				"	\
			:	"=m" (new_sw), "=m" (save_cw)	\
			:	"r" (FPU_STAT_TOP), "r" (FPU_STAT.p_reg), "m" (FPU_CTRLWORDMASK)		\
			:	"memory"				\
		);										\
		FPU_STATUSWORD=(new_sw&exc_mask)|(FPU_STATUSWORD&0x80ff);}
#endif

// handles fdiv,fdivr
#ifdef WEAK_EXCEPTIONS
#define FPUD_ARITH3(op)						\
		{UINT16 save_cw;						\
		__asm__ volatile (					\
			"fnstcw		%0				\n"	\
			"fldcw		%4				\n"	\
			"shll		$4, %2			\n"	\
			"shll		$4, %1			\n"	\
			"fldt		(%3, %2)		\n"	\
			"fldt		(%3, %1)		\n"	\
			#op"						\n"	\
			"fstpt		(%3, %1)		\n"	\
			"fldcw		%0				"	\
			:	"=m" (save_cw)				\
			:	"r" (op1), "r" (op2), "r" (FPU_STAT.p_reg), "m" (FPU_CTRLWORDMASK)		\
			:	"memory"					\
			);}
#else
#define FPUD_ARITH3(op)						\
		{UINT16 new_sw,save_cw;				\
		__asm__ volatile (					\
			"fnstcw		%1				\n"	\
			"fldcw		%5				\n"	\
			"shll		$4, %3			\n"	\
			"shll		$4, %2			\n"	\
			"fldt		(%4, %3)		\n"	\
			"fldt		(%4, %2)		\n"	\
			"fclex						\n"	\
			#op"						\n"	\
			"fnstsw		%0				\n"	\
			"fstpt		(%4, %2)		\n"	\
			"fldcw		%1				"	\
			:	"=m" (new_sw), "=m" (save_cw)		\
			:	"r" (op1), "r" (op2), "r" (FPU_STAT.p_reg), "m" (FPU_CTRLWORDMASK)		\
			:	"memory"					\
		);									\
		FPU_STATUSWORD=(new_sw&0xffbf)|(FPU_STATUSWORD&0x80ff);}
#endif

// handles fdiv,fdivr
#ifdef WEAK_EXCEPTIONS
#define FPUD_ARITH3_EA(op)					\
		{UINT16 save_cw;						\
		__asm__ volatile (					\
			"fnstcw		%0				\n"	\
			"fldcw		%3				\n"	\
			"shll		$4, %1			\n"	\
			"fldt		(%2, %1)		\n"	\
			#op"						\n"	\
			"fstpt		(%2, %1)		\n"	\
			"fldcw		%0				"	\
			:	"=m" (save_cw)				\
			:	"r" (op1), "r" (FPU_STAT.p_reg), "m" (FPU_CTRLWORDMASK)		\
			:	"memory"					\
			);}
#else
#define FPUD_ARITH3_EA(op)					\
		{UINT16 new_sw,save_cw;				\
		__asm__ volatile (					\
			"fnstcw		%1				\n"	\
			"fldcw		%4				\n"	\
			"shll		$4, %2			\n"	\
			"fldt		(%3, %2)		\n"	\
			"fclex						\n"	\
			#op"						\n"	\
			"fnstsw		%0				\n"	\
			"fstpt		(%3, %2)		\n"	\
			"fldcw		%1				"	\
			:	"=m" (new_sw), "=m" (save_cw)		\
			:	"r" (op1), "r" (FPU_STAT.p_reg), "m" (FPU_CTRLWORDMASK)		\
			:	"memory"					\
		);									\
		FPU_STATUSWORD=(new_sw&0xffbf)|(FPU_STATUSWORD&0x80ff);}
#endif

// handles fprem,fprem1,fscale
#define FPUD_REMINDER(op)					\
		{UINT16 new_sw;						\
		__asm__ volatile (					\
			"movl		%1, %%eax		\n"	\
			"incl		%%eax			\n"	\
			"andl		$7, %%eax		\n"	\
			"shll		$4, %%eax		\n"	\
			"shll		$4, %1			\n"	\
			"fldt		(%2, %%eax)		\n"	\
			"fldt		(%2, %1)		\n"	\
			"fclex						\n"	\
			#op" 						\n"	\
			"fnstsw		%0				\n"	\
			"fstpt		(%2, %1)		\n"	\
			"fstp		%%st(0)			"	\
			:	"=m" (new_sw)				\
			:	"r" (FPU_STAT_TOP), "r" (FPU_STAT.p_reg)	\
			:	"eax", "memory"						\
		);									\
		FPU_STATUSWORD=(new_sw&0xffbf)|(FPU_STATUSWORD&0x80ff);}

// handles fcom,fucom
#define FPUD_COMPARE(op)					\
		{UINT16 new_sw;						\
		__asm__ volatile (					\
			"shll		$4, %2			\n"	\
			"shll		$4, %1			\n"	\
			"fldt		(%3, %2)		\n"	\
			"fldt		(%3, %1)		\n"	\
			clx" 						\n"	\
			#op" 						\n"	\
			"fnstsw		%0				"	\
			:	"=m" (new_sw)				\
			:	"r" (op1), "r" (op2), "r" (FPU_STAT.p_reg) 		\
			:	"memory"					\
		);									\
		FPU_STATUSWORD=(new_sw&exc_mask)|(FPU_STATUSWORD&0x80ff);}

// handles fcom,fucom
#define FPUD_COMPARE_EA(op)					\
		{UINT16 new_sw;						\
		__asm__ volatile (					\
			"shll		$4, %1			\n"	\
			"fldt		(%2, %1)		\n"	\
			clx" 						\n"	\
			#op" 						\n"	\
			"fnstsw		%0				"	\
			:	"=m" (new_sw)				\
			:	"r" (op1), "r" (FPU_STAT.p_reg) 		\
			:	"memory"					\
		);									\
		FPU_STATUSWORD=(new_sw&exc_mask)|(FPU_STATUSWORD&0x80ff);}

// handles fxam,ftst
#define FPUD_EXAMINE(op)					\
		{UINT16 new_sw;						\
		__asm__ volatile (					\
			"shll		$4, %1			\n"	\
			"fldt		(%2, %1)		\n"	\
			clx" 						\n"	\
			#op" 						\n"	\
			"fnstsw		%0				\n"	\
			"fstp		%%st(0)			"	\
			:	"=m" (new_sw)				\
			:	"r" (FPU_STAT_TOP), "r" (FPU_STAT.p_reg)	\
			:	"memory"				\
		);									\
		FPU_STATUSWORD=(new_sw&exc_mask)|(FPU_STATUSWORD&0x80ff);}

// handles fpatan,fyl2xp1
#ifdef WEAK_EXCEPTIONS
#define FPUD_WITH_POP(op)					\
		__asm__ volatile (					\
			"movl		%0, %%eax		\n"	\
			"incl		%%eax			\n"	\
			"andl		$7, %%eax		\n"	\
			"shll		$4, %%eax		\n"	\
			"shll		$4, %0			\n"	\
			"fldt		(%1, %%eax)		\n"	\
			"fldt		(%1, %0)		\n"	\
			#op" 						\n"	\
			"fstpt		(%1, %%eax)		\n"	\
			:								\
			:	"r" (FPU_STAT_TOP), "r" (FPU_STAT.p_reg)	\
			:	"eax", "memory"				\
		);									\
		FPU_FPOP();
#else
#define FPUD_WITH_POP(op)					\
		{UINT16 new_sw;						\
		__asm__ volatile (					\
			"movl		%1, %%eax		\n"	\
			"incl		%%eax			\n"	\
			"andl		$7, %%eax		\n"	\
			"shll		$4, %%eax		\n"	\
			"shll		$4, %1			\n"	\
			"fldt		(%2, %%eax)		\n"	\
			"fldt		(%2, %1)		\n"	\
			"fclex						\n"	\
			#op" 						\n"	\
			"fnstsw		%0				\n"	\
			"fstpt		(%2, %%eax)		\n"	\
			:	"=m" (new_sw)				\
			:	"r" (FPU_STAT_TOP), "r" (FPU_STAT.p_reg)	\
			:	"eax", "memory"						\
		);									\
		FPU_STATUSWORD=(new_sw&0xffbf)|(FPU_STATUSWORD&0x80ff);		\
		FPU_FPOP();}
#endif

// handles fyl2x
#ifdef WEAK_EXCEPTIONS
#define FPUD_FYL2X(op)						\
		__asm__ volatile (					\
			"movl		%0, %%eax		\n"	\
			"incl		%%eax			\n"	\
			"andl		$7, %%eax		\n"	\
			"shll		$4, %%eax		\n"	\
			"shll		$4, %0			\n"	\
			"fldt		(%1, %%eax)		\n"	\
			"fldt		(%1, %0)		\n"	\
			#op" 						\n"	\
			"fstpt		(%1, %%eax)		\n"	\
			:								\
			:	"r" (FPU_STAT_TOP), "r" (FPU_STAT.p_reg)	\
			:	"eax", "memory"				\
		);									\
		FPU_FPOP();
#else
#define FPUD_FYL2X(op)						\
		{UINT16 new_sw;						\
		__asm__ volatile (					\
			"movl		%1, %%eax		\n"	\
			"incl		%%eax			\n"	\
			"andl		$7, %%eax		\n"	\
			"shll		$4, %%eax		\n"	\
			"shll		$4, %1			\n"	\
			"fldt		(%2, %%eax)		\n"	\
			"fldt		(%2, %1)		\n"	\
			"fclex						\n"	\
			#op" 						\n"	\
			"fnstsw		%0				\n"	\
			"fstpt		(%2, %%eax)		\n"	\
			:	"=m" (new_sw)				\
			:	"r" (FPU_STAT_TOP), "r" (FPU_STAT.p_reg)	\
			:	"eax", "memory"				\
		);									\
		FPU_STATUSWORD=(new_sw&0xffbf)|(FPU_STATUSWORD&0x80ff);		\
		FPU_FPOP();}
#endif

// load math constants
#define FPUD_LOAD_CONST(op)				\
		FPU_PREP_PUSH();					\
		__asm__ volatile (					\
			"shll		$4, %0			\n"	\
			clx" 						\n"	\
			#op" 						\n"	\
			"fstpt		(%1, %0)		\n"	\
			:								\
			:	"r" (FPU_STAT_TOP), "r" (FPU_STAT.p_reg)	\
			:	"memory"					\
		);

#endif

#ifdef WEAK_EXCEPTIONS
const UINT16 exc_mask=0x7f00;
#else
const UINT16 exc_mask=0xffbf;
#endif


/*
 * FPU memory access function
 */
static UINT8 MEMCALL
fpu_memoryread_b(UINT32 address)
{
	UINT16 seg;

	FPU_DATAPTR_SEG = seg = CPU_INST_SEGREG_INDEX;
	FPU_DATAPTR_OFFSET = address;
	return cpu_vmemoryread_b(seg, address);
}

static UINT16 MEMCALL
fpu_memoryread_w(UINT32 address)
{
	UINT16 seg;

	FPU_DATAPTR_SEG = seg = CPU_INST_SEGREG_INDEX;
	FPU_DATAPTR_OFFSET = address;
	return cpu_vmemoryread_w(seg, address);
}

static UINT32 MEMCALL
fpu_memoryread_d(UINT32 address)
{
	UINT16 seg;

	FPU_DATAPTR_SEG = seg = CPU_INST_SEGREG_INDEX;
	FPU_DATAPTR_OFFSET = address;
	return cpu_vmemoryread_d(seg, address);
}

static UINT64 MEMCALL
fpu_memoryread_q(UINT32 address)
{
	UINT16 seg;

	FPU_DATAPTR_SEG = seg = CPU_INST_SEGREG_INDEX;
	FPU_DATAPTR_OFFSET = address;
	return cpu_vmemoryread_q(seg, address);
}

static REG80 MEMCALL
fpu_memoryread_f(UINT32 address)
{
	UINT16 seg;

	FPU_DATAPTR_SEG = seg = CPU_INST_SEGREG_INDEX;
	FPU_DATAPTR_OFFSET = address;
	return cpu_vmemoryread_f(seg, address);
}

static void MEMCALL
fpu_memorywrite_b(UINT32 address, UINT8 value)
{
	UINT16 seg;

	FPU_DATAPTR_SEG = seg = CPU_INST_SEGREG_INDEX;
	FPU_DATAPTR_OFFSET = address;
	cpu_vmemorywrite_b(seg, address, value);
}

static void MEMCALL
fpu_memorywrite_w(UINT32 address, UINT16 value)
{
	UINT16 seg;

	FPU_DATAPTR_SEG = seg = CPU_INST_SEGREG_INDEX;
	FPU_DATAPTR_OFFSET = address;
	cpu_vmemorywrite_w(seg, address, value);
}

static void MEMCALL
fpu_memorywrite_d(UINT32 address, UINT32 value)
{
	UINT16 seg;

	FPU_DATAPTR_SEG = seg = CPU_INST_SEGREG_INDEX;
	FPU_DATAPTR_OFFSET = address;
	cpu_vmemorywrite_d(seg, address, value);
}

static void MEMCALL
fpu_memorywrite_q(UINT32 address, UINT64 value)
{
	UINT16 seg;

	FPU_DATAPTR_SEG = seg = CPU_INST_SEGREG_INDEX;
	FPU_DATAPTR_OFFSET = address;
	cpu_vmemorywrite_q(seg, address, value);
}

static void MEMCALL
fpu_memorywrite_f(UINT32 address, REG80 *value)
{
	UINT16 seg;

	FPU_DATAPTR_SEG = seg = CPU_INST_SEGREG_INDEX;
	FPU_DATAPTR_OFFSET = address;
	cpu_vmemorywrite_f(seg, address, value);
}

static const FPU_PTR zero_ptr = { 0, 0, 0 };

/*
 * FPU interface
 */
 
static INLINE UINT FPU_GET_TOP(void) {
	return (FPU_STATUSWORD & 0x3800)>>11;
}

static INLINE void FPU_SET_TOP(UINT val){
	FPU_STATUSWORD &= ~0x3800;
	FPU_STATUSWORD |= (val&7)<<11;
}


static INLINE void FPU_SET_C0(UINT C){
	FPU_STATUSWORD &= ~0x0100;
	if(C) FPU_STATUSWORD |=  0x0100;
}

static INLINE void FPU_SET_C1(UINT C){
	FPU_STATUSWORD &= ~0x0200;
	if(C) FPU_STATUSWORD |=  0x0200;
}

static INLINE void FPU_SET_C2(UINT C){
	FPU_STATUSWORD &= ~0x0400;
	if(C) FPU_STATUSWORD |=  0x0400;
}

static INLINE void FPU_SET_C3(UINT C){
	FPU_STATUSWORD &= ~0x4000;
	if(C) FPU_STATUSWORD |= 0x4000;
}

static INLINE void FPU_SET_D(UINT C){
	FPU_STATUSWORD &= ~0x0002;
	if(C) FPU_STATUSWORD |= 0x0002;
}

static INLINE void FPU_SetCW(UINT16 cword)
{
	// HACK: Bits 13-15 are not defined. Apparently, one program likes to test for
	// Cyrix EMC87 by trying to set bit 15. We want the test program to see
	// us as an Intel 287 when cputype == 286.
	//cword &= 0x7FFF;
	FPU_CTRLWORD = cword;
	FPU_CTRLWORDMASK = (UINT16)(cword | 0x3f);
	FPU_STAT.round = (FP_RND)((cword >> 10) & 3);
}

static void FPU_FLDCW(UINT32 addr)
{
	UINT16 temp = cpu_vmemoryread_w(CPU_INST_SEGREG_INDEX, addr);
	FPU_SetCW(temp);
}

static UINT16 FPU_GetTag(void)
{
	UINT i;
	
	UINT16 tag=0;
	for(i=0;i<8;i++)
		tag |= ( (FPU_STAT.tag[i]&3) <<(2*i));
	return tag;
}

static INLINE void FPU_SetTag(UINT16 tag)
{
	UINT i;
	
	for(i=0;i<8;i++)
		FPU_STAT.tag[i] = (FP_TAG)((tag >>(2*i))&3);
}

static void FPU_FCLEX(void){
	//FPU_STATUSWORD &= 0xff00;			//should clear exceptions
	FPU_STATUSWORD &= 0x7f00;			//should clear exceptions?
}

static void FPU_FNOP(void){
	return;
}

//static void FPU_PUSH(double in){
//	FPU_STAT_TOP = (FPU_STAT_TOP - 1) & 7;
//	//actually check if empty
//	FPU_STAT.tag[FPU_STAT_TOP] = TAG_Valid;
//	FPU_STAT.reg[FPU_STAT_TOP].d = in;
////	LOG(LOG_FPU,LOG_ERROR)("Pushed at %d  %g to the stack",newtop,in);
//	return;
//}

static void FPU_PREP_PUSH(void){
	FPU_STAT_TOP = (FPU_STAT_TOP - 1) & 7;
	FPU_STAT.tag[FPU_STAT_TOP] = TAG_Valid;
}

static void FPU_FPOP(void){
	FPU_STAT.tag[FPU_STAT_TOP] = TAG_Empty;
	//maybe set zero in it as well
	FPU_STAT_TOP = ((FPU_STAT_TOP+1)&7);
//	LOG(LOG_FPU,LOG_ERROR)("popped from %d  %g off the stack",top,fpu.regs[top].d);
	return;
}

static void FPU_FLD_F32(UINT32 addr,UINT store_to) {
	FPU_STAT.p_reg[8].m1 = fpu_memoryread_d(addr);
	FPUD_LOAD(fld,DWORD,s)
}
static void FPU_FLD_F32_EA(UINT32 addr) {
	FPU_STAT.p_reg[8].m1 = fpu_memoryread_d(addr);
	FPUD_LOAD_EA(fld,DWORD,s)
}

static void FPU_FLD_F64(UINT32 addr,UINT store_to) {
	FPU_STAT.p_reg[8].m1 = fpu_memoryread_d(addr);
	FPU_STAT.p_reg[8].m2 = fpu_memoryread_d(addr+4);
	FPUD_LOAD(fld,QWORD,l)
}
static void FPU_FLD_F64_EA(UINT32 addr) {
	FPU_STAT.p_reg[8].m1 = fpu_memoryread_d(addr);
	FPU_STAT.p_reg[8].m2 = fpu_memoryread_d(addr+4);
	FPUD_LOAD_EA(fld,QWORD,l)
}

static void FPU_FLD_F80(UINT32 addr) {
	FPU_STAT.p_reg[FPU_STAT_TOP].m1 = fpu_memoryread_d(addr);
	FPU_STAT.p_reg[FPU_STAT_TOP].m2 = fpu_memoryread_d(addr+4);
	FPU_STAT.p_reg[FPU_STAT_TOP].m3 = fpu_memoryread_w(addr+8);
	FPU_SET_C1(0);
}

static void FPU_FLD_I16(UINT32 addr,UINT store_to) {
	FPU_STAT.p_reg[8].m1 = (UINT32)fpu_memoryread_w(addr);
	FPUD_LOAD(fild,WORD,)
}
static void FPU_FLD_I16_EA(UINT32 addr) {
	FPU_STAT.p_reg[8].m1 = (UINT32)fpu_memoryread_w(addr);
	FPUD_LOAD_EA(fild,WORD,)
}

static void FPU_FLD_I32(UINT32 addr,UINT store_to) {
	FPU_STAT.p_reg[8].m1 = fpu_memoryread_d(addr);
	FPUD_LOAD(fild,DWORD,l)
}
static void FPU_FLD_I32_EA(UINT32 addr) {
	FPU_STAT.p_reg[8].m1 = fpu_memoryread_d(addr);
	FPUD_LOAD_EA(fild,DWORD,l)
}

static void FPU_FLD_I64(UINT32 addr,UINT store_to) {
	FPU_STAT.p_reg[8].m1 = fpu_memoryread_d(addr);
	FPU_STAT.p_reg[8].m2 = fpu_memoryread_d(addr+4);
	FPUD_LOAD(fild,QWORD,q)
}

static void FPU_FBLD(UINT32 addr,UINT store_to) 
{
	FPU_STAT.p_reg[8].m1 = fpu_memoryread_d(addr);
	FPU_STAT.p_reg[8].m2 = fpu_memoryread_d(addr+4);
	FPU_STAT.p_reg[8].m3 = fpu_memoryread_w(addr+8);
	FPUD_LOAD(fbld,TBYTE,)
}

static void FPU_FST_F32(UINT32 addr) {
	FPUD_STORE(fstp,DWORD,s)
	fpu_memorywrite_d(addr,FPU_STAT.p_reg[8].m1);
}

static void FPU_FST_F64(UINT32 addr) {
	FPUD_STORE(fstp,QWORD,l)
	fpu_memorywrite_d(addr,FPU_STAT.p_reg[8].m1);
	fpu_memorywrite_d(addr+4,FPU_STAT.p_reg[8].m2);
}

static void FPU_FST_F80(UINT32 addr) {
	fpu_memorywrite_d(addr,FPU_STAT.p_reg[FPU_STAT_TOP].m1);
	fpu_memorywrite_d(addr+4,FPU_STAT.p_reg[FPU_STAT_TOP].m2);
	fpu_memorywrite_w(addr+8,FPU_STAT.p_reg[FPU_STAT_TOP].m3);
	FPU_SET_C1(0);
}

static void FPU_FST_I16(UINT32 addr) {
	FPUD_STORE(fistp,WORD,)
	fpu_memorywrite_w(addr,(UINT16)FPU_STAT.p_reg[8].m1);
}

static void FPU_FST_I32(UINT32 addr) {
	FPUD_STORE(fistp,DWORD,l)
	fpu_memorywrite_d(addr,FPU_STAT.p_reg[8].m1);
}

static void FPU_FST_I64(UINT32 addr) {
	FPUD_STORE(fistp,QWORD,q)
	fpu_memorywrite_d(addr,FPU_STAT.p_reg[8].m1);
	fpu_memorywrite_d(addr+4,FPU_STAT.p_reg[8].m2);
}

static void FPU_FBST(UINT32 addr) 
{
	FPUD_STORE(fbstp,TBYTE,)
	fpu_memorywrite_d(addr,FPU_STAT.p_reg[8].m1);
	fpu_memorywrite_d(addr+4,FPU_STAT.p_reg[8].m2);
	fpu_memorywrite_w(addr+8,FPU_STAT.p_reg[8].m3);
}

static void FPU_FSIN(void){
	FPUD_TRIG(fsin)
	return;
}

static void FPU_FSINCOS(void){
	FPUD_SINCOS()
	return;
}

static void FPU_FCOS(void){
	FPUD_TRIG(fcos)
	return;
}

static void FPU_FSQRT(void){
	FPUD_ARITH2(fsqrt)
	return;
}
static void FPU_FPATAN(void){
	FPUD_WITH_POP(fpatan)
	return;
}
static void FPU_FPTAN(void){
	FPUD_PTAN()
	return;
}

static void FPU_FADD(UINT op1, UINT op2){
	FPUD_ARITH1(faddp)
	return;
}
static void FPU_FADD_EA(UINT op1){
	FPUD_ARITH1_EA(faddp)
	return;
}

static void FPU_FDIV(UINT op1, UINT op2){
	FPUD_ARITH3(fdivp)
	return;
}
static void FPU_FDIV_EA(UINT op1){
	FPUD_ARITH3_EA(fdivp)
	return;
}

static void FPU_FDIVR(UINT op1, UINT op2){
	FPUD_ARITH3(fdivrp)
	return;
}
static void FPU_FDIVR_EA(UINT op1){
	FPUD_ARITH3_EA(fdivrp)
	return;
}

static void FPU_FMUL(UINT op1, UINT op2){
	FPUD_ARITH1(fmulp)
	return;
}
static void FPU_FMUL_EA(UINT op1){
	FPUD_ARITH1_EA(fmulp)
	return;
}

static void FPU_FSUB(UINT op1, UINT op2){
	FPUD_ARITH1(fsubp)
	return;
}
static void FPU_FSUB_EA(UINT op1){
	FPUD_ARITH1_EA(fsubp)
	return;
}

static void FPU_FSUBR(UINT op1, UINT op2){
	FPUD_ARITH1(fsubrp)
	return;
}
static void FPU_FSUBR_EA(UINT op1){
	FPUD_ARITH1_EA(fsubrp)
	return;
}

static void FPU_FXCH(UINT stv, UINT other){
	UINT32 m1s;
	UINT32 m2s;
	UINT16 m3s;

	FP_TAG tag = FPU_STAT.tag[other];
	FPU_STAT.tag[other] = FPU_STAT.tag[stv];
	FPU_STAT.tag[stv] = tag;

	m1s = FPU_STAT.p_reg[other].m1;
	m2s = FPU_STAT.p_reg[other].m2;
	m3s = FPU_STAT.p_reg[other].m3;
	FPU_STAT.p_reg[other].m1 = FPU_STAT.p_reg[stv].m1;
	FPU_STAT.p_reg[other].m2 = FPU_STAT.p_reg[stv].m2;
	FPU_STAT.p_reg[other].m3 = FPU_STAT.p_reg[stv].m3;
	FPU_STAT.p_reg[stv].m1 = m1s;
	FPU_STAT.p_reg[stv].m2 = m2s;
	FPU_STAT.p_reg[stv].m3 = m3s;

	FPU_SET_C1(0);
}

static void FPU_FST(UINT stv, UINT other){
	FPU_STAT.tag[other] = FPU_STAT.tag[stv];

	FPU_STAT.p_reg[other].m1 = FPU_STAT.p_reg[stv].m1;
	FPU_STAT.p_reg[other].m2 = FPU_STAT.p_reg[stv].m2;
	FPU_STAT.p_reg[other].m3 = FPU_STAT.p_reg[stv].m3;

	FPU_SET_C1(0);
}

static void FPU_FCOM(UINT op1, UINT op2){
	FPUD_COMPARE(fcompp)
	return;
}
static void FPU_FCOM_EA(UINT op1){
	FPUD_COMPARE_EA(fcompp)
	return;
}

static void FPU_FUCOM(UINT op1, UINT op2){
	FPUD_COMPARE(fucompp)
}

static void FPU_FRNDINT(void){
	FPUD_ARITH2(frndint)
}

static void FPU_FPREM(void){
	FPUD_REMINDER(fprem)
}

static void FPU_FPREM1(void){
	FPUD_REMINDER(fprem1)
}

static void FPU_FXAM(void){
	FPUD_EXAMINE(fxam)
	// handle empty registers (C1 set to sign in any way!)
	if(FPU_STAT.tag[FPU_STAT_TOP] == TAG_Empty) {
		FPU_SET_C3(1);FPU_SET_C2(0);FPU_SET_C0(1);
		return;
	}
}

static void FPU_F2XM1(void){
	FPUD_TRIG(f2xm1)
	return;
}

static void FPU_FYL2X(void){
	FPUD_FYL2X(fyl2x)
	return;
}

static void FPU_FYL2XP1(void){
	FPUD_WITH_POP(fyl2xp1)
	return;
}

static void FPU_FSCALE(void){
	FPUD_REMINDER(fscale)
}

static void FPU_FSTENV(UINT32 addr)
{
	descriptor_t *sdp = &CPU_CS_DESC;	
	FPU_SET_TOP(FPU_STAT_TOP);
	
	switch ((CPU_CR0 & 1) | (SEG_IS_32BIT(sdp) ? 0x100 : 0x000))
	{
	case 0x000: case 0x001:
		fpu_memorywrite_w(addr+0,FPU_CTRLWORD);
		fpu_memorywrite_w(addr+2,FPU_STATUSWORD);
		fpu_memorywrite_w(addr+4,FPU_GetTag());
		fpu_memorywrite_w(addr+10,FPU_LASTINSTOP);
		break;
		
	case 0x100: case 0x101:
		fpu_memorywrite_d(addr+0,(UINT32)(FPU_CTRLWORD));
		fpu_memorywrite_d(addr+4,(UINT32)(FPU_STATUSWORD));
		fpu_memorywrite_d(addr+8,(UINT32)(FPU_GetTag()));	
		fpu_memorywrite_d(addr+20,FPU_LASTINSTOP);			
		break;
	}
}

static void FPU_FLDENV(UINT32 addr)
{
	descriptor_t *sdp = &CPU_CS_DESC;	
	
	switch ((CPU_CR0 & 1) | (SEG_IS_32BIT(sdp) ? 0x100 : 0x000)) {
	case 0x000: case 0x001:
		FPU_SetCW(fpu_memoryread_w(addr+0));
		FPU_STATUSWORD = fpu_memoryread_w(addr+2);
		FPU_SetTag(fpu_memoryread_w(addr+4));
		FPU_LASTINSTOP = fpu_memoryread_w(addr+10);
		break;
		
	case 0x100: case 0x101:
		FPU_SetCW((UINT16)fpu_memoryread_d(addr+0));
		FPU_STATUSWORD = (UINT16)fpu_memoryread_d(addr+4);
		FPU_SetTag((UINT16)fpu_memoryread_d(addr+8));
		FPU_LASTINSTOP = (UINT16)fpu_memoryread_d(addr+20);		
		break;
	}
	FPU_STAT_TOP = FPU_GET_TOP();
}

static void FPU_FSAVE(UINT32 addr)
{
	UINT start;
	UINT i;
	
	descriptor_t *sdp = &CPU_CS_DESC;
	
	FPU_FSTENV(addr);
	start = ((SEG_IS_32BIT(sdp))?28:14);
	for(i = 0;i < 8;i++){
		fpu_memorywrite_d(addr+start,FPU_STAT.p_reg[FPU_ST(i)].m1);
		fpu_memorywrite_d(addr+start+4,FPU_STAT.p_reg[FPU_ST(i)].m2);
		fpu_memorywrite_w(addr+start+8,FPU_STAT.p_reg[FPU_ST(i)].m3);
		start += 10;
	}
	fpu_init();
}

static void FPU_FRSTOR(UINT32 addr)
{
	UINT start;
	UINT i;
	
	descriptor_t *sdp = &CPU_CS_DESC;
	
	FPU_FLDENV(addr);
	start = ((SEG_IS_32BIT(sdp))?28:14);
	for(i = 0;i < 8;i++){
		FPU_STAT.p_reg[FPU_ST(i)].m1 = fpu_memoryread_d(addr+start);
		FPU_STAT.p_reg[FPU_ST(i)].m2 = fpu_memoryread_d(addr+start+4);
		FPU_STAT.p_reg[FPU_ST(i)].m3 = fpu_memoryread_w(addr+start+8);
		start += 10;
	}
}

static void FPU_FXTRACT(void) {
	FPUD_XTRACT
}

static void FPU_FCHS(void){
	FPUD_TRIG(fchs)
}

static void FPU_FABS(void){
	FPUD_TRIG(fabs)
}

static void FPU_FTST(void){
	FPUD_EXAMINE(ftst)
}

static void FPU_FLD1(void){
	FPUD_LOAD_CONST(fld1)
}

static void FPU_FLDL2T(void){
	FPUD_LOAD_CONST(fldl2t)
}

static void FPU_FLDL2E(void){
	FPUD_LOAD_CONST(fldl2e)
}

static void FPU_FLDPI(void){
	FPUD_LOAD_CONST(fldpi)
}

static void FPU_FLDLG2(void){
	FPUD_LOAD_CONST(fldlg2)
}

static void FPU_FLDLN2(void){
	FPUD_LOAD_CONST(fldln2)
}

static void FPU_FLDZ(void){
	FPUD_LOAD_CONST(fldz)
	FPU_STAT.tag[FPU_STAT_TOP]=TAG_Zero;
}
 
/*
 * FPU interface
 */
int fpu_updateEmuEnv(void);
void
fpu_init(void)
{
	int i;
	FPU_SetCW(0x37F);
	FPU_STATUSWORD = 0;
	FPU_STAT_TOP=FPU_GET_TOP();
	for(i=0;i<8;i++){
		FPU_STAT.tag[i] = TAG_Empty;
		FPU_STAT.p_reg[i].m1 = 0;
		FPU_STAT.p_reg[i].m2 = 0;
		FPU_STAT.p_reg[i].m3 = 0;
	}
	FPU_STAT.tag[8] = TAG_Valid; // is only used by us
}

char *
fpu_reg2str(void)
{
	return NULL;
}


/*
 * FPU instruction
 */

void
fpu_checkexception(void){
}

void
fpu_fwait(void)
{

	/* XXX: check pending FPU exception */
	fpu_checkexception();
}

static void EA_TREE(UINT op)
{
	UINT idx;

	idx = (op >> 3) & 7;
	
		switch (idx) {
		case 0:	/* FADD (単精度実数) */
			TRACEOUT(("FADD EA"));
			FPU_FADD_EA(FPU_STAT_TOP);
			break;
		case 1:	/* FMUL (単精度実数) */
			TRACEOUT(("FMUL EA"));
			FPU_FMUL_EA(FPU_STAT_TOP);
			break;
		case 2:	/* FCOM (単精度実数) */
			TRACEOUT(("FCOM EA"));
			FPU_FCOM_EA(FPU_STAT_TOP);
			break;
		case 3:	/* FCOMP (単精度実数) */
			TRACEOUT(("FCOMP EA"));
			FPU_FCOM_EA(FPU_STAT_TOP);
			FPU_FPOP();
			break;
		case 4:	/* FSUB (単精度実数) */
			TRACEOUT(("FSUB EA"));
			FPU_FSUB_EA(FPU_STAT_TOP);
			break;
		case 5:	/* FSUBR (単精度実数) */
			TRACEOUT(("FSUBR EA"));
			FPU_FSUBR_EA(FPU_STAT_TOP);
			break;
		case 6:	/* FDIV (単精度実数) */
			TRACEOUT(("FDIV EA"));
			FPU_FDIV_EA(FPU_STAT_TOP);
			break;
		case 7:	/* FDIVR (単精度実数) */
			TRACEOUT(("FDIVR EA"));
			FPU_FDIVR_EA(FPU_STAT_TOP);
			break;
		default:
			break;
		}	
}

// d8
void
ESC0(void)
{
	UINT32 op, madr;
	UINT idx, sub;
	unsigned int oldval;
	
	CPU_WORKCLOCK(FPU_WORKCLOCK);
	GET_PCBYTE(op);
	TRACEOUT(("use FPU d8 %.2x", op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	if (op >= 0xc0) {
		/* Fxxx ST(0), ST(i) */
		switch (idx) {
		case 0:	/* FADD */
			TRACEOUT(("FADD"));
			FPU_FADD(FPU_STAT_TOP,FPU_ST(sub));
			break;
		case 1:	/* FMUL */
			TRACEOUT(("FMUL"));
			FPU_FMUL(FPU_STAT_TOP,FPU_ST(sub));
			break;
		case 2:	/* FCOM */
			TRACEOUT(("FCOM"));
			FPU_FCOM(FPU_STAT_TOP,FPU_ST(sub));
			break;
		case 3:	/* FCOMP */
			TRACEOUT(("FCOMP"));
			FPU_FCOM(FPU_STAT_TOP,FPU_ST(sub));
			FPU_FPOP();
			break;
		case 4:	/* FSUB */
			TRACEOUT(("FSUB"));
			FPU_FSUB(FPU_STAT_TOP,FPU_ST(sub));
			break;
		case 5:	/* FSUBR */
			TRACEOUT(("FSUBR"));
			FPU_FSUBR(FPU_STAT_TOP,FPU_ST(sub));
			break;
		case 6:	/* FDIV */
			TRACEOUT(("FDIV"));
			FPU_FDIV(FPU_STAT_TOP,FPU_ST(sub));
			break;
		case 7:	/* FDIVR */
			TRACEOUT(("FDIVR"));
			FPU_FDIVR(FPU_STAT_TOP,FPU_ST(sub));
			break;
		}
	} else {
		madr = calc_ea_dst(op);
		FPU_FLD_F32_EA(madr);
		EA_TREE(op);
	}
	
}

// d9
void
ESC1(void)
{
	UINT32 op, madr;
	UINT idx, sub;
	unsigned int oldval;
	
	CPU_WORKCLOCK(FPU_WORKCLOCK);
	GET_PCBYTE(op);
	TRACEOUT(("use FPU d9 %.2x", op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	if (op >= 0xc0) 
	{
		switch (idx) {
		case 0: /* FLD ST(0), ST(i) */
			{
				UINT reg_from;
			
				TRACEOUT(("FLD STi"));
				reg_from = FPU_ST(sub);
				FPU_PREP_PUSH();
				FPU_FST(reg_from, FPU_STAT_TOP);
			}
			break;

		case 1:	/* FXCH ST(0), ST(i) */
			TRACEOUT(("FXCH STi"));
			FPU_FXCH(FPU_STAT_TOP,FPU_ST(sub));
			break;

		case 2: /* FNOP */
			TRACEOUT(("FNOP"));
			FPU_FNOP();
			break;

		case 3: /* FSTP STi */
			TRACEOUT(("FSTP STi"));
			FPU_FST(FPU_STAT_TOP,FPU_ST(sub));
			FPU_FPOP();
			break;
		
		case 4:
			switch (sub) {
			case 0x0:	/* FCHS */
				TRACEOUT(("FCHS"));
				FPU_FCHS();
				break;

			case 0x1:	/* FABS */
				TRACEOUT(("FABS"));
				FPU_FABS();
				break;

			case 0x2:  /* UNKNOWN */
			case 0x3:  /* ILLEGAL */
				break;

			case 0x4:	/* FTST */
				TRACEOUT(("FTST"));
				FPU_FTST();
				break;

			case 0x5:	/* FXAM */
				TRACEOUT(("FXAM"));
				FPU_FXAM();
				break;

			case 0x06:       /* FTSTP (cyrix)*/
			case 0x07:       /* UNKNOWN */
				break;
			}
			break;
			
		case 5:
			switch (sub) {
			case 0x0:	/* FLD1 */
				TRACEOUT(("FLD1"));
				FPU_FLD1();
				break;
				
			case 0x1:	/* FLDL2T */
				TRACEOUT(("FLDL2T"));
				FPU_FLDL2T();
				break;
				
			case 0x2:	/* FLDL2E */
				TRACEOUT(("FLDL2E"));
				FPU_FLDL2E();
				break;
				
			case 0x3:	/* FLDPI */
				TRACEOUT(("FLDPI"));
				FPU_FLDPI();
				break;
				
			case 0x4:	/* FLDLG2 */
				TRACEOUT(("FLDLG2"));
				FPU_FLDLG2();
				break;
				
			case 0x5:	/* FLDLN2 */
				TRACEOUT(("FLDLN2"));
				FPU_FLDLN2();
				break;
				
			case 0x6:	/* FLDZ */
				TRACEOUT(("FLDZ"));
				FPU_FLDZ();
				break;
				
			case 0x07: /* ILLEGAL */
				break;
			}
			break;

		case 6:
			switch (sub) {
			case 0x0:	/* F2XM1 */
				TRACEOUT(("F2XM1"));
				FPU_F2XM1();
				break;
				
			case 0x1:	/* FYL2X */
				TRACEOUT(("FYL2X"));
				FPU_FYL2X();
				break;
				
			case 0x2:	/* FPTAN */
				TRACEOUT(("FPTAN"));
				FPU_FPTAN();
				break;
				
			case 0x3:	/* FPATAN */
				TRACEOUT(("FPATAN"));
				FPU_FPATAN();
				break;
				
			case 0x4:	/* FXTRACT */
				TRACEOUT(("FXTRACT"));
				FPU_FXTRACT();
				break;
				
			case 0x5:	/* FPREM1 */
				TRACEOUT(("FPREM1"));
				FPU_FPREM1();
				break;
				
			case 0x6:	/* FDECSTP */
				TRACEOUT(("FDECSTP"));
				FPU_STAT_TOP = (FPU_STAT_TOP - 1) & 7;
				break;
				
			case 0x7:	/* FINCSTP */
				TRACEOUT(("FINCSTP"));
				FPU_STAT_TOP = (FPU_STAT_TOP + 1) & 7;
				break;
			}
			break;
			
		case 7:
			switch (sub) {
			case 0x0:	/* FPREM */
				TRACEOUT(("FPREM"));
				FPU_FPREM();
				break;
				
			case 0x1:	/* FYL2XP1 */
				TRACEOUT(("FYL2XP1"));
				FPU_FYL2XP1();
				break;
				
			case 0x2:	/* FSQRT */
				TRACEOUT(("FSQRT"));
				FPU_FSQRT();
				break;
				
			case 0x3:	/* FSINCOS */
				TRACEOUT(("FSINCOS"));
				FPU_FSINCOS();
				break;
				
			case 0x4:	/* FRNDINT */
				TRACEOUT(("FRNDINT"));
				FPU_FRNDINT();
				break;
				
			case 0x5:	/* FSCALE */
				TRACEOUT(("FSCALE"));
				FPU_FSCALE();
				break;
				
			case 0x6:	/* FSIN */
				TRACEOUT(("FSIN"));
				FPU_FSIN();				
				break;
				
			case 0x7:	/* FCOS */
				TRACEOUT(("FCOS"));
				FPU_FCOS();	
				break;
			}
			break;

		default:
			ia32_panic("ESC1: invalid opcode = %02x\n", op);
			break;
		}
	} else {
		madr = calc_ea_dst(op);
		switch (idx) {
		case 0:	/* FLD (単精度実数) */
			TRACEOUT(("FLD float"));
			FPU_PREP_PUSH();
			FPU_FLD_F32(madr,FPU_STAT_TOP);
			break;
			
		case 1:	/* UNKNOWN */
			break;

		case 2:	/* FST (単精度実数) */
			TRACEOUT(("FST float"));
			FPU_FST_F32(madr);			
			break;

		case 3:	/* FSTP (単精度実数) */
			TRACEOUT(("FSTP float"));
			FPU_FST_F32(madr);
			FPU_FPOP();
			break;

		case 4:	/* FLDENV */
			TRACEOUT(("FLDENV"));
			FPU_FLDENV(madr);		
			break;

		case 5:	/* FLDCW */
			TRACEOUT(("FLDCW"));
			FPU_FLDCW(madr);
			break;

		case 6:	/* FSTENV */
			TRACEOUT(("FSTENV"));
			FPU_FSTENV(madr);
			break;

		case 7:	/* FSTCW */
			TRACEOUT(("FSTCW/FNSTCW"));
			fpu_memorywrite_w(madr,FPU_CTRLWORD);
			break;

		default:
			break;
		}
	}
	
}

// da
void
ESC2(void)
{
	UINT32 op, madr;
	UINT idx, sub;
	unsigned int oldval;
	
	CPU_WORKCLOCK(FPU_WORKCLOCK);
	GET_PCBYTE(op);
	TRACEOUT(("use FPU da %.2x", op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	if (op >= 0xc0) {
		/* Fxxx ST(0), ST(i) */
		switch (idx) {
		case 5:
			switch (sub) {
			case 1: /* FUCOMPP */
				TRACEOUT(("FUCOMPP"));
				FPU_FUCOM(FPU_STAT_TOP,FPU_ST(1));
				FPU_FPOP();
				FPU_FPOP();
				break;
				
			default:
				break;
			}
			break;
			
		default:
			break;
		}
	} else {
		madr = calc_ea_dst(op);
		FPU_FLD_I32_EA(madr);
		EA_TREE(op);
	}
	
}

// db
void
ESC3(void)
{
	UINT32 op, madr;
	UINT idx, sub;
	unsigned int oldval;
	
	CPU_WORKCLOCK(FPU_WORKCLOCK);
	GET_PCBYTE(op);
	TRACEOUT(("use FPU db %.2x", op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	if (op >= 0xc0) 
	{
		/* Fxxx ST(0), ST(i) */
		switch (idx) {
		case 4:
			switch (sub) {
			case 0: /* FNENI */
			case 1: /* FNDIS */
				break;
				
			case 2: /* FCLEX */
				TRACEOUT(("FCLEX"));
				FPU_FCLEX();
				break;
				
			case 3: /* FNINIT/FINIT */
				TRACEOUT(("FNINIT/FINIT"));
				fpu_init();
				break;
				
			case 4: /* FNSETPM */
			case 5: /* FRSTPM */
				FPU_FNOP();
				break;
				
			default:
				break;
			}
			break;
		default:
			break;
		}
	} else {
		madr = calc_ea_dst(op);
		switch (idx) {
		case 0:	/* FILD (DWORD) */
			TRACEOUT(("FILD"));
			FPU_PREP_PUSH();
			FPU_FLD_I32(madr,FPU_STAT_TOP);
			break;
			
		case 1:	/* FISTTP (DWORD) */
			{
				FP_RND oldrnd = FPU_STAT.round;
				FPU_STAT.round = ROUND_Down;
				FPU_FST_I32(madr);
				FPU_STAT.round = oldrnd;
			}
			FPU_FPOP();
			break;
			
		case 2:	/* FIST (DWORD) */
			TRACEOUT(("FIST"));
			FPU_FST_I32(madr);
			break;
			
		case 3:	/* FISTP (DWORD) */
			TRACEOUT(("FISTP"));
			FPU_FST_I32(madr);
			FPU_FPOP();
			break;
			
		case 5:	/* FLD (拡張実数) */
			TRACEOUT(("FLD 80 Bits Real"));
			FPU_PREP_PUSH();
			FPU_FLD_F80(madr);
			break;
			
		case 7:	/* FSTP (拡張実数) */
			TRACEOUT(("FSTP 80 Bits Real"));
			FPU_FST_F80(madr);
			FPU_FPOP();
			break;

		default:
			break;
		}
	}
	
}

// dc
void
ESC4(void)
{
	UINT32 op, madr;
	UINT idx, sub;
	unsigned int oldval;
	//
	//if(!CPU_STAT_PM){
	//	dummy_ESC4();
	//	return;
	//}
	
	CPU_WORKCLOCK(FPU_WORKCLOCK);
	GET_PCBYTE(op);
	TRACEOUT(("use FPU dc %.2x", op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	if (op >= 0xc0) {
		/* Fxxx ST(i), ST(0) */
		switch (idx) {
		case 0:	/* FADD */
			TRACEOUT(("ESC4: FADD"));
			FPU_FADD(FPU_ST(sub),FPU_STAT_TOP);
			break;
		case 1:	/* FMUL */
			TRACEOUT(("ESC4: FMUL"));
			FPU_FMUL(FPU_ST(sub),FPU_STAT_TOP);
			break;
		case 2: /* FCOM */
			TRACEOUT(("ESC4: FCOM"));
			FPU_FCOM(FPU_STAT_TOP,FPU_ST(sub));			
			break;
		case 3: /* FCOMP */
			TRACEOUT(("ESC4: FCOMP"));
			FPU_FCOM(FPU_STAT_TOP,FPU_ST(sub));	
			FPU_FPOP();
			break;
		case 4:	/* FSUBR */
			TRACEOUT(("ESC4: FSUBR"));
			FPU_FSUBR(FPU_ST(sub),FPU_STAT_TOP);
			break;
		case 5:	/* FSUB */
			TRACEOUT(("ESC4: FSUB"));
			FPU_FSUB(FPU_ST(sub),FPU_STAT_TOP);
			break;
		case 6:	/* FDIVR */
			TRACEOUT(("ESC4: FDIVR"));
			FPU_FDIVR(FPU_ST(sub),FPU_STAT_TOP);
			break;
		case 7:	/* FDIV */
			TRACEOUT(("ESC4: FDIV"));
			FPU_FDIV(FPU_ST(sub),FPU_STAT_TOP);
			break;
		default:
			break;
		}
	} else {
		madr = calc_ea_dst(op);
		FPU_FLD_F64_EA(madr);
		EA_TREE(op);
	}
	
}

// dd
void
ESC5(void)
{
	UINT32 op, madr;
	UINT idx, sub;
	unsigned int oldval;
	//
	//if(!CPU_STAT_PM){
	//	dummy_ESC5();
	//	return;
	//}
	
	CPU_WORKCLOCK(FPU_WORKCLOCK);
	GET_PCBYTE(op);
	TRACEOUT(("use FPU dd %.2x", op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	if (op >= 0xc0) {
		/* FUCOM ST(i), ST(0) */
		/* Fxxx ST(i) */
		switch (idx) {
		case 0:	/* FFREE */
			TRACEOUT(("FFREE"));
			FPU_STAT.tag[FPU_ST(sub)]=TAG_Empty;
			break;
		case 1: /* FXCH */
			TRACEOUT(("FXCH"));
			FPU_FXCH(FPU_STAT_TOP,FPU_ST(sub));
			break;
		case 2:	/* FST */
			TRACEOUT(("FST"));
			FPU_FST(FPU_STAT_TOP,FPU_ST(sub));
			break;
		case 3:	/* FSTP */
			TRACEOUT(("FSTP"));
			FPU_FST(FPU_STAT_TOP,FPU_ST(sub));
			FPU_FPOP();
			break;
		case 4:	/* FUCOM */
			TRACEOUT(("FUCOM"));
			FPU_FUCOM(FPU_STAT_TOP,FPU_ST(sub));
			break;
		case 5:	/* FUCOMP */
			TRACEOUT(("FUCOMP"));
			FPU_FUCOM(FPU_STAT_TOP,FPU_ST(sub));
			FPU_FPOP();
			break;

		default:
			break;
		}
	} else {
		madr = calc_ea_dst(op);
		switch (idx) {
		case 0:	/* FLD (倍精度実数) */
			TRACEOUT(("FLD double real"));
			FPU_PREP_PUSH();
			FPU_FLD_F64(madr,FPU_STAT_TOP);
			break;
		case 2:	/* FST (倍精度実数) */
			TRACEOUT(("FST double real"));
			FPU_FST_F64(madr);
			break;
		case 3:	/* FSTP (倍精度実数) */
			TRACEOUT(("FSTP double real"));
			FPU_FST_F64(madr);
			FPU_FPOP();
			break;
		case 4:	/* FRSTOR */
			TRACEOUT(("FRSTOR"));
			FPU_FRSTOR(madr);
			break;
		case 6:	/* FSAVE */
			TRACEOUT(("FSAVE"));
			FPU_FSAVE(madr);
			break;

		case 7:	/* FSTSW */
			FPU_SET_TOP(FPU_STAT_TOP);
			//cpu_vmemorywrite_w(CPU_INST_SEGREG_INDEX, madr, FPU_CTRLWORD);
			cpu_vmemorywrite_w(CPU_INST_SEGREG_INDEX, madr, FPU_STATUSWORD);
			break;

		default:
			break;
		}
	}
	
}

// de
void
ESC6(void)
{
	UINT32 op, madr;
	UINT idx, sub;
	unsigned int oldval;
	
	CPU_WORKCLOCK(FPU_WORKCLOCK);
	GET_PCBYTE(op);
	TRACEOUT(("use FPU de %.2x", op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	if (op >= 0xc0) {
		/* Fxxx ST(i), ST(0) */
		switch (idx) {
		case 0:	/* FADDP */
			TRACEOUT(("FADDP"));
			FPU_FADD(FPU_ST(sub),FPU_STAT_TOP);
			break;
		case 1:	/* FMULP */
			TRACEOUT(("FMULP"));
			FPU_FMUL(FPU_ST(sub),FPU_STAT_TOP);
			break;
		case 2:	/* FCOMP5 */
			TRACEOUT(("FCOMP5"));
			FPU_FCOM(FPU_STAT_TOP,FPU_ST(sub));
			break;
		case 3: /* FCOMPP */
			TRACEOUT(("FCOMPP"));
			if(sub != 1) {
				return;
			}
			FPU_FCOM(FPU_STAT_TOP,FPU_ST(1));
			FPU_FPOP(); /* extra pop at the bottom*/
			break;			
		case 4:	/* FSUBRP */
			TRACEOUT(("FSUBRP"));
			FPU_FSUBR(FPU_ST(sub),FPU_STAT_TOP);
			break;
		case 5:	/* FSUBP */
			TRACEOUT(("FSUBP"));
			FPU_FSUB(FPU_ST(sub),FPU_STAT_TOP);
			break;
		case 6:	/* FDIVRP */
			TRACEOUT(("FDIVRP"));
			FPU_FDIVR(FPU_ST(sub),FPU_STAT_TOP);
			break;
		case 7:	/* FDIVP */
			TRACEOUT(("FDIVP"));
			FPU_FDIV(FPU_ST(sub),FPU_STAT_TOP);
			break;
			/*FALLTHROUGH*/
		default:
			break;
		}
		FPU_FPOP();
	} else {
		madr = calc_ea_dst(op);
		FPU_FLD_I16_EA(madr);
		EA_TREE(op);
	}
	
}

// df
void
ESC7(void)
{
	UINT32 op, madr;
	UINT idx, sub;
	unsigned int oldval;
	
	CPU_WORKCLOCK(FPU_WORKCLOCK);
	GET_PCBYTE(op);
	TRACEOUT(("use FPU df %.2x", op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	if (op >= 0xc0) {
		/* Fxxx ST(0), ST(i) */
		switch (idx) {
		case 0: /* FFREEP */
			TRACEOUT(("FFREEP"));
			FPU_STAT.tag[FPU_ST(sub)]=TAG_Empty;
			FPU_FPOP();
			break;
		case 1: /* FXCH */
			TRACEOUT(("FXCH"));
			FPU_FXCH(FPU_STAT_TOP,FPU_ST(sub));
			break;
		
		case 2:
		case 3: /* FSTP */
			TRACEOUT(("FSTP"));
			FPU_FST(FPU_STAT_TOP,FPU_ST(sub));
			FPU_FPOP();
			break;

		case 4:
			switch (sub)
			{
			case 0: /* FSTSW AX */
				TRACEOUT(("FSTSW AX"));
				FPU_SET_TOP(FPU_STAT_TOP);
				CPU_AX = FPU_STATUSWORD;
				break;
				
			default:
				break;
			}
			break;
			/*FALLTHROUGH*/
		default:
			break;
		}
	} else {
		madr = calc_ea_dst(op);
		switch (idx) {
		case 0:	/* FILD (WORD) */
			TRACEOUT(("FILD SINT16"));
			FPU_PREP_PUSH();
			FPU_FLD_I16(madr,FPU_STAT_TOP);
			break;
		case 2:	/* FIST (WORD) */
			TRACEOUT(("FIST SINT16"));
			FPU_FST_I16(madr);
			break;
		case 3:	/* FISTP (WORD) */
			TRACEOUT(("FISTP SINT16"));
			FPU_FST_I16(madr);
			FPU_FPOP();
			break;

		case 4:	/* FBLD (BCD) */
			TRACEOUT(("FBLD packed BCD"));
			FPU_PREP_PUSH();
			FPU_FBLD(madr,FPU_STAT_TOP);
			break;

		case 5:	/* FILD (QWORD) */
			TRACEOUT(("FILD SINT64"));
			FPU_PREP_PUSH();
			FPU_FLD_I64(madr,FPU_STAT_TOP);
			break;

		case 6:	/* FBSTP (BCD) */
			TRACEOUT(("FBSTP packed BCD"));
			FPU_FBST(madr);
			FPU_FPOP();
			break;

		case 7:	/* FISTP (QWORD) */
			TRACEOUT(("FISTP SINT64"));
			FPU_FST_I64(madr);
			FPU_FPOP();
			break;

		default:
			break;
		}
	}
	
}

#endif
#endif
