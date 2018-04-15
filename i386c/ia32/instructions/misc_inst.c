/*
 * Copyright (c) 2002-2003 NONAKA Kimihiro
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

#include "compiler.h"
#include "ia32/cpu.h"
#include "ia32/ia32.mcr"

#include "misc_inst.h"
#include "ia32/inst_table.h"

#include "pccore.h"

void
LEA_GwM(void)
{
	UINT16 *out;
	UINT32 op, dst;

	GET_PCBYTE(op);
	if (op < 0xc0) {
		CPU_WORKCLOCK(2);
		out = reg16_b53[op];
		dst = calc_ea_dst(op);
		*out = (UINT16)dst;
		return;
	}
	EXCEPTION(UD_EXCEPTION, 0);
}

void
LEA_GdM(void)
{
	UINT32 *out;
	UINT32 op, dst;

	GET_PCBYTE(op);
	if (op < 0xc0) {
		CPU_WORKCLOCK(2);
		out = reg32_b53[op];
		dst = calc_ea_dst(op);
		*out = dst;
		return;
	}
	EXCEPTION(UD_EXCEPTION, 0);
}

void
_NOP(void)
{

	ia32_bioscall();
}

void
UD2(void)
{

	EXCEPTION(UD_EXCEPTION, 0);
}

void
XLAT(void)
{

	CPU_WORKCLOCK(5);
	CPU_INST_SEGREG_INDEX = DS_FIX;
	if (!CPU_INST_AS32) {
		CPU_AL = cpu_vmemoryread(CPU_INST_SEGREG_INDEX, CPU_BX + CPU_AL);
	} else {
		CPU_AL = cpu_vmemoryread(CPU_INST_SEGREG_INDEX, CPU_EBX + CPU_AL);
	}
}

#if defined(USE_MMX)
#define	CPU_BRAND_STRING_1	"Inte"
#define	CPU_BRAND_STRING_2	"l(R)"
#define	CPU_BRAND_STRING_3	" Pen"
#define	CPU_BRAND_STRING_4	"tium"
#define	CPU_BRAND_STRING_5	"(R) "
#define	CPU_BRAND_STRING_6	"II C"
#define	CPU_BRAND_STRING_7	"PU  "
#define	CPU_BRAND_STRING_8	"    "
#define	CPU_BRAND_STRING_SPC	"    "

//#define	CPU_BRAND_STRING_1	"Inte"
//#define	CPU_BRAND_STRING_2	"l(R)"
//#define	CPU_BRAND_STRING_3	" Pen"
//#define	CPU_BRAND_STRING_4	"tium"
//#define	CPU_BRAND_STRING_5	"(R) "
//#define	CPU_BRAND_STRING_6	"Pro "
//#define	CPU_BRAND_STRING_7	"CPU "
//#define	CPU_BRAND_STRING_8	"    "
//#define	CPU_BRAND_STRING_SPC	"    "

#else
#define	CPU_BRAND_STRING_1	"Inte"
#define	CPU_BRAND_STRING_2	"l(R)"
#define	CPU_BRAND_STRING_3	" Pen"
#define	CPU_BRAND_STRING_4	"tium"
#define	CPU_BRAND_STRING_5	"(R) "
#define	CPU_BRAND_STRING_6	"Proc"
#define	CPU_BRAND_STRING_7	"esso"
#define	CPU_BRAND_STRING_8	"r   "
#define	CPU_BRAND_STRING_SPC	"    "

#endif

void
_CPUID(void)
{
	char clkbuf[30]; // ˆÀ‘S‚Ì‚½‚ß‘½‚ß‚É¥¥¥
	UINT32 clkMHz;

	switch (CPU_EAX) {
	case 0:
		CPU_EAX = 1;
		CPU_EBX = CPU_VENDOR_1;
		CPU_EDX = CPU_VENDOR_2;
		CPU_ECX = CPU_VENDOR_3;
		break;

	case 1:
		CPU_EAX = (CPU_FAMILY << 8) | (CPU_MODEL << 4) | CPU_STEPPING;
		CPU_EBX = 0;
		CPU_ECX = 0;
		CPU_EDX = CPU_FEATURES;
		break;

	case 2:
		CPU_EAX = 0;
		CPU_EBX = 0;
		CPU_ECX = 0;
		CPU_EDX = 0;
		break;
		
#if defined(USE_FPU)
	case 0x80000000:
		CPU_EAX = 0x80000004;
		CPU_EBX = 0;
		CPU_ECX = 0;
		CPU_EDX = 0;
		break;

	case 0x80000001:
		CPU_EAX = 0;
		CPU_EBX = 0;
		CPU_ECX = 0;
		CPU_EDX = 0;
		break;
		
	case 0x80000002:
		CPU_EAX = LOADINTELDWORD(((UINT8*)CPU_BRAND_STRING_1));
		CPU_EBX = LOADINTELDWORD(((UINT8*)CPU_BRAND_STRING_2));
		CPU_ECX = LOADINTELDWORD(((UINT8*)CPU_BRAND_STRING_3));
		CPU_EDX = LOADINTELDWORD(((UINT8*)CPU_BRAND_STRING_4));
		break;
		
#if defined(USE_MMX)
	case 0x80000003:
		CPU_EAX = LOADINTELDWORD(((UINT8*)CPU_BRAND_STRING_5));
		CPU_EBX = LOADINTELDWORD(((UINT8*)CPU_BRAND_STRING_6));
		CPU_ECX = LOADINTELDWORD(((UINT8*)CPU_BRAND_STRING_7));
		clkbuf[0] = '\0';
		clkMHz = pccore.realclock/1000/1000;
		sprintf(clkbuf, "%d MHz", clkMHz);
		CPU_EDX = LOADINTELDWORD(((UINT8*)clkbuf));
		break;
		
	case 0x80000004:
		clkbuf[0] = '\0';
		clkMHz = pccore.realclock/1000/1000;
		sprintf(clkbuf, "%d MHz", clkMHz);
		CPU_EAX = LOADINTELDWORD(((UINT8*)(clkbuf+4)));
		CPU_EBX = LOADINTELDWORD(((UINT8*)(clkbuf+8)));
		CPU_ECX = LOADINTELDWORD(((UINT8*)(clkbuf+12)));
		CPU_EDX = 0;
		break;
#else
	case 0x80000003:
		CPU_EAX = LOADINTELDWORD(((UINT8*)CPU_BRAND_STRING_5));
		CPU_EBX = LOADINTELDWORD(((UINT8*)CPU_BRAND_STRING_6));
		CPU_ECX = LOADINTELDWORD(((UINT8*)CPU_BRAND_STRING_7));
		CPU_EDX = LOADINTELDWORD(((UINT8*)CPU_BRAND_STRING_8));
		break;
		
	case 0x80000004:
		clkbuf[0] = '\0';
		clkMHz = pccore.realclock/1000/1000;
		sprintf(clkbuf, "%d MHz", clkMHz);
		CPU_EAX = LOADINTELDWORD(((UINT8*)clkbuf));
		CPU_EBX = LOADINTELDWORD(((UINT8*)(clkbuf+4)));
		CPU_ECX = LOADINTELDWORD(((UINT8*)(clkbuf+8)));
		CPU_EDX = LOADINTELDWORD(((UINT8*)(clkbuf+12)));
		break;
#endif
#endif
	}
}

/* undoc 8086 */
void
SALC(void)
{

	CPU_WORKCLOCK(2);
	CPU_AL = (CPU_FLAGL & C_FLAG) ? 0xff : 0;
}

/* undoc 286 */
void
LOADALL286(void)
{

	ia32_panic("LOADALL286: not implemented yet.");
}

/* undoc 386 */
void
LOADALL(void)
{

	ia32_panic("LOADALL: not implemented yet.");
}

void
OpSize(void)
{

	CPU_INST_OP32 = !CPU_STATSAVE.cpu_inst_default.op_32;
}

void
AddrSize(void)
{

	CPU_INST_AS32 = !CPU_STATSAVE.cpu_inst_default.as_32;
}

void
_2byte_ESC16(void)
{
	UINT32 op;

	GET_PCBYTE(op);
	(*insttable_2byte[0][op])();
}

void
_2byte_ESC32(void)
{
	UINT32 op;

	GET_PCBYTE(op);
	(*insttable_2byte[1][op])();
}

void
Prefix_ES(void)
{

	CPU_INST_SEGUSE = 1;
	CPU_INST_SEGREG_INDEX = CPU_ES_INDEX;
}

void
Prefix_CS(void)
{

	CPU_INST_SEGUSE = 1;
	CPU_INST_SEGREG_INDEX = CPU_CS_INDEX;
}

void
Prefix_SS(void)
{

	CPU_INST_SEGUSE = 1;
	CPU_INST_SEGREG_INDEX = CPU_SS_INDEX;
}

void
Prefix_DS(void)
{

	CPU_INST_SEGUSE = 1;
	CPU_INST_SEGREG_INDEX = CPU_DS_INDEX;
}

void
Prefix_FS(void)
{

	CPU_INST_SEGUSE = 1;
	CPU_INST_SEGREG_INDEX = CPU_FS_INDEX;
}

void
Prefix_GS(void)
{

	CPU_INST_SEGUSE = 1;
	CPU_INST_SEGREG_INDEX = CPU_GS_INDEX;
}
