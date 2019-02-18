#include	"compiler.h"
#include	"cpucore.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"bios/bios.h"

#if defined(SUPPORT_IA32_HAXM)

#include	"haxfunc.h"
#include	"haxcore.h"

#if defined(_WINDOWS)
#include	<process.h>
#endif

static int np2hax_cs_initialized = 0;
static CRITICAL_SECTION np2hax_cs;

NP2_HAX	np2hax = {0};
NP2_HAX_THREAD	np2haxthread = {0};

BOOL i386hax_tryenter_criticalsection(void){
	if(!np2hax_cs_initialized) return;
	return TryEnterCriticalSection(&np2hax_cs);
}
void i386hax_enter_criticalsection(void){
	if(!np2hax_cs_initialized) return;
	EnterCriticalSection(&np2hax_cs);
}
void i386hax_leave_criticalsection(void){
	if(!np2hax_cs_initialized) return;
	LeaveCriticalSection(&np2hax_cs);
}

static void make_vm_str(OEMCHAR* buf, UINT32 vm_id){
	_stprintf(buf, OEMTEXT("\\\\.\\hax_vm%02d"), vm_id);
}
static void make_vcpu_str(OEMCHAR* buf, UINT32 vm_id, UINT32 vcpu_id){
	_stprintf(buf, OEMTEXT("\\\\.\\hax_vm%02d_vcpu%02d"), vm_id, vcpu_id);
}

UINT8 i386hax_check(void) {
	
#if defined(_WIN32)
	HAX_MODULE_VERSION haxver = {0};
	HAX_CAPINFO haxcap = {0};

	np2hax.hDevice = CreateFile(OEMTEXT("\\\\.\\HAX"), 
		GENERIC_READ|GENERIC_WRITE, 0, NULL, 
		CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if (np2hax.hDevice == INVALID_HANDLE_VALUE) {
		TRACEOUT(("HAXM: Failed to initialize the HAX device."));
        msgbox("HAXM check", "Failed to initialize the HAX device.");
		np2hax.available = 0;
		np2hax.hDevice = NULL;
        return FAILURE;
	}
	
	if(i386haxfunc_getversion(&haxver)==SUCCESS){
		char buf[256] = {0};
		TRACEOUT(("HAXM: HAX getversion: compatible version %d, current version %d", haxver.compat_version, haxver.current_version));
		//sprintf(buf, "HAXM: HAX getversion: compatible version %d, current version %d", haxver.compat_version, haxver.current_version);
		//msgbox("HAXM check", buf);
		//msgbox("HAXM init", "HAX getversion succeeded.");
	}else{
		TRACEOUT(("HAXM: HAX getversion failed."));
		msgbox("HAXM check", "HAX getversion failed.");
		goto initfailure;
	}
	
	if(i386haxfunc_getcapability(&haxcap)==SUCCESS){
		char buf[256] = {0};
		TRACEOUT(("HAXM: HAX getcapability: wstatus 0x%.4x, winfo 0x%.4x, win_refcount %d, mem_quota %d", haxcap.wstatus, haxcap.winfo, haxcap.win_refcount, haxcap.mem_quota));
		//sprintf(buf, "HAXM: HAX getcapability: wstatus 0x%.4x, winfo 0x%.4x, win_refcount %d, mem_quota %d", haxcap.wstatus, haxcap.winfo, haxcap.win_refcount, haxcap.mem_quota);
		//msgbox("HAXM check", buf);
		if(!(haxcap.wstatus & HAX_CAP_WORKSTATUS_MASK)){
			if(haxcap.winfo & HAX_CAP_FAILREASON_VT){
				msgbox("HAXM check", "Error: Intel VT-x is not supported by the host CPU.");
			}else{
				msgbox("HAXM check", "Error: Intel Execute Disable Bit is not supported by the host CPU.");
			}
			goto initfailure;
		}
		if(!(haxcap.winfo & HAX_CAP_FASTMMIO)){
			msgbox("HAXM check", "Error: Fast MMIO is not supported. Please update HAXM.");
			goto initfailure;
		}
		if(!(haxcap.winfo & HAX_CAP_DEBUG)){
			msgbox("HAXM check", "Error: Debugging function is not supported. Please update HAXM.");
			goto initfailure;
		}
		//msgbox("HAXM init", "HAX getversion succeeded.");
	}else{
		TRACEOUT(("HAXM: HAX getcapability failed."));
		msgbox("HAXM check", "HAX getcapability failed.");
		goto initfailure;
	}
	
	CloseHandle(np2hax.hDevice);
	np2hax.hDevice = NULL;

	TRACEOUT(("HAXM: HAX available."));
    //msgbox("HAXM check", "HAX available.");

	np2hax.available = 1;

	return SUCCESS;

initfailure:
	CloseHandle(np2hax.hDevice);
	np2hax.available = 0;
	np2hax.enable = 0;
	np2hax.hDevice = NULL;
	return FAILURE;
#else
	return 1;
#endif
}

void i386hax_initialize(void) {
	
#if defined(_WIN32)
	if(!np2hax.available) return;
	if(!np2hax.enable) return;

	i386hax_deinitialize();
	
	if(!np2hax_cs_initialized){
		memset(&np2hax_cs, 0, sizeof(np2hax_cs));
		InitializeCriticalSection(&np2hax_cs);
		np2hax_cs_initialized = 1;
	}

	np2hax.hDevice = CreateFile(OEMTEXT("\\\\.\\HAX"), 
		GENERIC_READ|GENERIC_WRITE, 0, NULL, 
		CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if (np2hax.hDevice == INVALID_HANDLE_VALUE) {
		TRACEOUT(("HAXM: Failed to initialize the HAX device."));
        return;
	}

	TRACEOUT(("HAXM: HAX initialized."));
    //msgbox("HAXM init", "HAX initialized.");
	
	return;
	
error1:
	CloseHandle(np2hax.hDevice);
	np2hax.hDevice = NULL;
	return;
#endif
}

void i386hax_createVM(void) {
	
#if defined(_WIN32)
	OEMCHAR vm_str[64] = {0};
	OEMCHAR vcpu_str[64] = {0};
	HAX_DEBUG hax_dbg = {0};
	HAX_QEMU_VERSION hax_qemu_ver = {0};

	if(!np2hax.available) return;
	if(!np2hax.enable) return;
	if(!np2hax.hDevice) return;

	i386hax_disposeVM();
	
	if(i386haxfunc_createVM(&np2hax.vm_id)==FAILURE){
		TRACEOUT(("HAXM: HAX create VM failed."));
		msgbox("HAXM VM", "HAX create VM failed.");
		return;
	}

	make_vm_str(vm_str, np2hax.vm_id);
	
	np2hax.hVMDevice = CreateFile(vm_str, 
		GENERIC_READ|GENERIC_WRITE, 0, NULL, 
		CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if (np2hax.hVMDevice == INVALID_HANDLE_VALUE) {
		TRACEOUT(("HAXM: Failed to initialize the HAX VM device."));
		return;
	}

	hax_qemu_ver.cur_version = 0x0;
	if(i386haxfunc_notifyQEMUversion(&hax_qemu_ver)==FAILURE){
		TRACEOUT(("HAXM: HAX notify QEMU version failed."));
		msgbox("HAXM VM", "HAX notify QEMU version failed.");
		goto error2;
	}
	
	if(i386haxfunc_VCPUcreate(0)==FAILURE){
		TRACEOUT(("HAXM: HAX create VCPU failed."));
		msgbox("HAXM VM", "HAX create VCPU failed.");
		goto error2;
	}
	
	make_vcpu_str(vcpu_str, np2hax.vm_id, 0);
	
	np2hax.hVCPUDevice = CreateFile(vcpu_str, 
		GENERIC_READ|GENERIC_WRITE, 0, NULL, 
		CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if (np2hax.hVCPUDevice == INVALID_HANDLE_VALUE) {
		TRACEOUT(("HAXM: Failed to initialize the HAX VCPU device."));
		goto error2;
	}
	
	if(i386haxfunc_vcpu_setup_tunnel(&np2hax.tunnel)==FAILURE){
		TRACEOUT(("HAXM: HAX VCPU setup tunnel failed."));
		msgbox("HAXM VM", "HAX VCPU setup tunnel failed.");
		goto error2;
	}
	
	hax_dbg.control = HAX_DEBUG_ENABLE|HAX_DEBUG_USE_SW_BP;//|HAX_DEBUG_USE_HW_BP|HAX_DEBUG_STEP;
	if(i386haxfunc_vcpu_debug(&hax_dbg)==FAILURE){
		TRACEOUT(("HAXM: HAX VCPU debug setting failed."));
		msgbox("HAXM VM", "HAX VCPU debug setting failed.");
		goto error2;
	}
	
	TRACEOUT(("HAXM: HAX VM initialized."));
    //msgbox("HAXM VM", "HAX VM initialized.");
	
	memset(&np2hax.bankmem, 0, sizeof(np2hax.bankmem));
	np2hax.bankmem.bank.A0 = 1;
	np2hax.bankmem.bank.A4 = 1;
	np2hax.bankmem.bank.A8 = 1;
	np2hax.bankmem.bank.AC = 1;
	np2hax.bankmem.bank.B0 = 1;
	np2hax.bankmem.bank.B4 = 1;
	np2hax.bankmem.bank.B8 = 1;
	np2hax.bankmem.bank.BC = 1;
	np2hax.bankmem.bank.C0 = 1;
	np2hax.bankmem.bank.C4 = 1;
	np2hax.bankmem.bank.C8 = 1;
	np2hax.bankmem.bank.CC = 1;
	np2hax.bankmem.bank.D0 = 1;
	np2hax.bankmem.bank.D4 = 1;
	//np2hax.bankmem.bank.D8 = 1;
	//np2hax.bankmem.bank.DC = 1;
	np2hax.bankmem.bank.E0 = 1;
	np2hax.bankmem.bank.E4 = 1;
	//np2hax.bankmem.bank.E8 = 1;
	//np2hax.bankmem.bank.EC = 1;
	//np2hax.bankmem.bank.F0 = 1;
	//np2hax.bankmem.bank.F4 = 1;
	
	//i386haxfunc_vcpu_getREGs(&np2hax.state);
	//i386haxfunc_vcpu_getFPU(&np2hax.fpustate);
	//memcpy(&np2hax.default_state, &np2hax.state, sizeof(np2hax.state));
	//memcpy(&np2hax.default_fpustate, &np2hax.fpustate, sizeof(np2hax.fpustate));
	////memset(&np2hax.state, 0, sizeof(np2hax.state));
	////memset(&np2hax.fpustate, 0, sizeof(np2hax.fpustate));
	//np2hax.update_regs = np2hax.update_fpu = 1;

	return;
	
error3:
	CloseHandle(np2hax.hVCPUDevice);
	np2hax.hVCPUDevice = NULL;
error2:
	CloseHandle(np2hax.hVMDevice);
	np2hax.hVMDevice = NULL;
error1:
	return;
#endif
}

void i386hax_resetVM(void) {
	
	np2hax.irq_reqidx_cur = np2hax.irq_reqidx_end = 0;
	
	np2haxthread.lastA20en = (CPU_ADRSMASK != 0x000fffff);
	np2haxthread.lastITFbank = CPU_ITFBANK;
	
	//memcpy(&np2hax.state, &np2hax.default_state, sizeof(np2hax.state));
	//memcpy(&np2hax.fpustate, &np2hax.default_fpustate, sizeof(np2hax.fpustate));

	np2hax.update_regs = np2hax.update_fpu = 1;
	np2hax.update_segment_regs = 1;
}

/*
 * bios call interface
 */
static int
ia32hax_bioscall(void)
{
	UINT32 adrs;
	int ret = 0;

	if (!CPU_STAT_PM || CPU_STAT_VM86) {
#if 1
		adrs = CPU_PREV_EIP + (CPU_CS << 4);
#else
		adrs = CPU_PREV_EIP + CPU_STAT_CS_BASE;
#endif
		if ((adrs >= 0xf8000) && (adrs < 0x100000)) {
			if (biosfunc(adrs)) {
				/* Nothing to do */
				ret = 1;
			}
			LOAD_SEGREG(CPU_ES_INDEX, CPU_ES);
			LOAD_SEGREG(CPU_CS_INDEX, CPU_CS);
			LOAD_SEGREG(CPU_SS_INDEX, CPU_SS);
			LOAD_SEGREG(CPU_DS_INDEX, CPU_DS);
		}
	}
	return ret;
}

static void
ia32hax_copyregHAXtoNP2(void)
{
	CPU_EAX = np2hax.state._eax;
	CPU_EBX = np2hax.state._ebx;
	CPU_ECX = np2hax.state._ecx;
	CPU_EDX = np2hax.state._edx;

	CPU_ESI = np2hax.state._esi;
	CPU_EDI = np2hax.state._edi;

	CPU_CS = np2hax.state._cs.selector;
	CS_BASE = np2hax.state._cs.base;
	CPU_DS = np2hax.state._ds.selector;
	DS_BASE = np2hax.state._ds.base;
	CPU_ES = np2hax.state._es.selector;
	ES_BASE = np2hax.state._es.base;
	CPU_SS = np2hax.state._ss.selector;
	SS_BASE = np2hax.state._ss.base;
	CPU_FS = np2hax.state._fs.selector;
	FS_BASE = np2hax.state._fs.base;
	CPU_GS = np2hax.state._gs.selector;
	GS_BASE = np2hax.state._gs.base;

	CPU_CS_DESC.u.seg.segbase = np2hax.state._cs.base;//(UINT32)np2hax.state._cs.selector << 4;
	CPU_CS_DESC.u.seg.limit = np2hax.state._cs.limit;
	CPU_CS_DESC.type = np2hax.state._cs.type;
	CPU_CS_DESC.s = np2hax.state._cs.desc;
	CPU_CS_DESC.dpl = np2hax.state._cs.dpl;
	CPU_CS_DESC.p = np2hax.state._cs.present;
	CPU_CS_DESC.valid = np2hax.state._cs.available;
	CPU_CS_DESC.d = np2hax.state._cs.operand_size;
	CPU_CS_DESC.u.seg.g = np2hax.state._cs.granularity;
	
	CPU_DS_DESC.u.seg.segbase = np2hax.state._ds.base;//(UINT32)np2hax.state._ds.selector << 4;
	CPU_DS_DESC.u.seg.limit = np2hax.state._ds.limit;
	CPU_DS_DESC.type = np2hax.state._ds.type;
	CPU_DS_DESC.s = np2hax.state._ds.desc;
	CPU_DS_DESC.dpl = np2hax.state._ds.dpl;
	CPU_DS_DESC.p = np2hax.state._ds.present;
	CPU_DS_DESC.valid = np2hax.state._ds.available;
	CPU_DS_DESC.d = np2hax.state._ds.operand_size;
	CPU_DS_DESC.u.seg.g = np2hax.state._ds.granularity;
	
	CPU_ES_DESC.u.seg.segbase = np2hax.state._es.base;//(UINT32)np2hax.state._es.selector << 4;
	CPU_ES_DESC.u.seg.limit = np2hax.state._es.limit;
	CPU_ES_DESC.type = np2hax.state._es.type;
	CPU_ES_DESC.s = np2hax.state._es.desc;
	CPU_ES_DESC.dpl = np2hax.state._es.dpl;
	CPU_ES_DESC.p = np2hax.state._es.present;
	CPU_ES_DESC.valid = np2hax.state._es.available;
	CPU_ES_DESC.d = np2hax.state._es.operand_size;
	CPU_ES_DESC.u.seg.g = np2hax.state._es.granularity;
	
	CPU_SS_DESC.u.seg.segbase = np2hax.state._ss.base;//(UINT32)np2hax.state._ss.selector << 4;
	CPU_SS_DESC.u.seg.limit = np2hax.state._ss.limit;
	CPU_SS_DESC.type = np2hax.state._ss.type;
	CPU_SS_DESC.s = np2hax.state._ss.desc;
	CPU_SS_DESC.dpl = np2hax.state._ss.dpl;
	CPU_SS_DESC.p = np2hax.state._ss.present;
	CPU_SS_DESC.valid = np2hax.state._ss.available;
	CPU_SS_DESC.d = np2hax.state._ss.operand_size;
	CPU_SS_DESC.u.seg.g = np2hax.state._ss.granularity;
	
	CPU_FS_DESC.u.seg.segbase = np2hax.state._fs.base;//(UINT32)np2hax.state._fs.selector << 4;
	CPU_FS_DESC.u.seg.limit = np2hax.state._fs.limit;
	CPU_FS_DESC.type = np2hax.state._fs.type;
	CPU_FS_DESC.s = np2hax.state._fs.desc;
	CPU_FS_DESC.dpl = np2hax.state._fs.dpl;
	CPU_FS_DESC.p = np2hax.state._fs.present;
	CPU_FS_DESC.valid = np2hax.state._fs.available;
	CPU_FS_DESC.d = np2hax.state._fs.operand_size;
	CPU_FS_DESC.u.seg.g = np2hax.state._fs.granularity;
	
	CPU_GS_DESC.u.seg.segbase = np2hax.state._gs.base;//(UINT32)np2hax.state._gs.selector << 4;
	CPU_GS_DESC.u.seg.limit = np2hax.state._gs.limit;
	CPU_GS_DESC.type = np2hax.state._gs.type;
	CPU_GS_DESC.s = np2hax.state._gs.desc;
	CPU_GS_DESC.dpl = np2hax.state._gs.dpl;
	CPU_GS_DESC.p = np2hax.state._gs.present;
	CPU_GS_DESC.valid = np2hax.state._gs.available;
	CPU_GS_DESC.d = np2hax.state._gs.operand_size;
	CPU_GS_DESC.u.seg.g = np2hax.state._gs.granularity;
	
	CPU_EBP = np2hax.state._ebp;
	CPU_ESP = np2hax.state._esp;
	CPU_EIP = np2hax.state._eip;
	CPU_PREV_EIP = np2hax.state._eip; // XXX: ‚ ‚ñ‚Ü‚è—Ç‚­‚È‚¢
	
	CPU_EFLAG = np2hax.state._eflags;
	
	CPU_CR0 = np2hax.state._cr0;
	//CPU_CR1 = np2hax.state._cr1;
	CPU_CR2 = np2hax.state._cr2;
	CPU_CR3 = np2hax.state._cr3;
	CPU_CR4 = np2hax.state._cr4;
	
	CPU_GDTR_BASE = np2hax.state._gdt.base;
	CPU_GDTR_LIMIT = np2hax.state._gdt.limit;
	CPU_IDTR_BASE = np2hax.state._idt.base;
	CPU_IDTR_LIMIT = np2hax.state._idt.limit;
	CPU_LDTR = np2hax.state._ldt.selector;
	CPU_LDTR_BASE = np2hax.state._ldt.base;
	CPU_LDTR_LIMIT = np2hax.state._ldt.limit;
	CPU_TR = np2hax.state._tr.selector;
	CPU_TR_BASE = np2hax.state._tr.base;
	CPU_TR_LIMIT = np2hax.state._tr.limit;

	CPU_DR(0) = np2hax.state._dr0;
	CPU_DR(1) = np2hax.state._dr1;
	CPU_DR(2) = np2hax.state._dr2;
	CPU_DR(3) = np2hax.state._dr3;
	//CPU_DR(4) = np2hax.state._dr4;
	//CPU_DR(5) = np2hax.state._dr5;
	CPU_DR(6) = np2hax.state._dr6;
	CPU_DR(7) = np2hax.state._dr7;
	
	CPU_STAT_PDE_BASE = np2hax.state._pde;
	CPU_STAT_PM = (np2hax.state._cr0 & 0x1)!=0;
	CPU_STAT_VM86 = (np2hax.state._eflags & VM_FLAG)!=0;
}
static void
ia32hax_copyregNP2toHAX(void)
{
	np2hax.state._eax = CPU_EAX;
	np2hax.state._ebx = CPU_EBX;
	np2hax.state._ecx = CPU_ECX;
	np2hax.state._edx = CPU_EDX;
	
	np2hax.state._esi = CPU_ESI;
	np2hax.state._edi = CPU_EDI;
	
	if(1||np2hax.update_segment_regs){
		np2hax.state._cs.selector = CPU_CS;
		np2hax.state._ds.selector = CPU_DS;
		np2hax.state._es.selector = CPU_ES;
		np2hax.state._ss.selector = CPU_SS;
		np2hax.state._fs.selector = CPU_FS;
		np2hax.state._gs.selector = CPU_GS;
		np2hax.state._cs.base = CS_BASE;
		np2hax.state._ds.base = DS_BASE;
		np2hax.state._es.base = ES_BASE;
		np2hax.state._ss.base = SS_BASE;
		np2hax.state._fs.base = FS_BASE;
		np2hax.state._gs.base = GS_BASE;

		np2hax.state._cs.base = CPU_CS_DESC.u.seg.segbase;
		np2hax.state._cs.limit = CPU_CS_DESC.u.seg.limit;
		//np2hax.state._cs.type = CPU_CS_DESC.type;
		np2hax.state._cs.desc = CPU_CS_DESC.s;
		np2hax.state._cs.dpl = CPU_CS_DESC.dpl;
		np2hax.state._cs.present = CPU_CS_DESC.p;
		np2hax.state._cs.available = CPU_CS_DESC.valid;
		np2hax.state._cs.operand_size = CPU_CS_DESC.d;
		np2hax.state._cs.granularity = CPU_CS_DESC.u.seg.g;
	
		np2hax.state._ds.base = CPU_DS_DESC.u.seg.segbase;
		np2hax.state._ds.limit = CPU_DS_DESC.u.seg.limit;
		//np2hax.state._ds.type = CPU_DS_DESC.type;
		np2hax.state._ds.desc = CPU_DS_DESC.s;
		np2hax.state._ds.dpl = CPU_DS_DESC.dpl;
		np2hax.state._ds.present = CPU_DS_DESC.p;
		np2hax.state._ds.available = CPU_DS_DESC.valid;
		np2hax.state._ds.operand_size = CPU_DS_DESC.d;
		np2hax.state._ds.granularity = CPU_DS_DESC.u.seg.g;
	
		np2hax.state._es.base = CPU_ES_DESC.u.seg.segbase;
		np2hax.state._es.limit = CPU_ES_DESC.u.seg.limit;
		//np2hax.state._es.type = CPU_ES_DESC.type;
		np2hax.state._es.desc = CPU_ES_DESC.s;
		np2hax.state._es.dpl = CPU_ES_DESC.dpl;
		np2hax.state._es.present = CPU_ES_DESC.p;
		np2hax.state._es.available = CPU_ES_DESC.valid;
		np2hax.state._es.operand_size = CPU_ES_DESC.d;
		np2hax.state._es.granularity = CPU_ES_DESC.u.seg.g;
	
		np2hax.state._ss.base = CPU_SS_DESC.u.seg.segbase;
		np2hax.state._ss.limit = CPU_SS_DESC.u.seg.limit;
		//np2hax.state._ss.type = CPU_SS_DESC.type;
		np2hax.state._ss.desc = CPU_SS_DESC.s;
		np2hax.state._ss.dpl = CPU_SS_DESC.dpl;
		np2hax.state._ss.present = CPU_SS_DESC.p;
		np2hax.state._ss.available = CPU_SS_DESC.valid;
		np2hax.state._ss.operand_size = CPU_SS_DESC.d;
		np2hax.state._ss.granularity = CPU_SS_DESC.u.seg.g;
	
		np2hax.state._fs.base = CPU_FS_DESC.u.seg.segbase;
		np2hax.state._fs.limit = CPU_FS_DESC.u.seg.limit;
		//np2hax.state._fs.type = CPU_FS_DESC.type;
		np2hax.state._fs.desc = CPU_FS_DESC.s;
		np2hax.state._fs.dpl = CPU_FS_DESC.dpl;
		np2hax.state._fs.present = CPU_FS_DESC.p;
		np2hax.state._fs.available = CPU_FS_DESC.valid;
		np2hax.state._fs.operand_size = CPU_FS_DESC.d;
		np2hax.state._fs.granularity = CPU_FS_DESC.u.seg.g;
	
		np2hax.state._gs.base = CPU_GS_DESC.u.seg.segbase;
		np2hax.state._gs.limit = CPU_GS_DESC.u.seg.limit;
		//np2hax.state._gs.type = CPU_GS_DESC.type;
		np2hax.state._gs.desc = CPU_GS_DESC.s;
		np2hax.state._gs.dpl = CPU_GS_DESC.dpl;
		np2hax.state._gs.present = CPU_GS_DESC.p;
		np2hax.state._gs.available = CPU_GS_DESC.valid;
		np2hax.state._gs.operand_size = CPU_GS_DESC.d;
		np2hax.state._gs.granularity = CPU_GS_DESC.u.seg.g;

		np2hax.update_segment_regs = 0;
	}
	
	np2hax.state._ebp = CPU_EBP;
	np2hax.state._esp = CPU_ESP;
	np2hax.state._eip = CPU_EIP;
	
	np2hax.state._eflags = CPU_EFLAG;
	
	np2hax.state._cr0 = CPU_CR0;
	//np2hax.state._cr1 = CPU_CR1;
	np2hax.state._cr2 = CPU_CR2;
	np2hax.state._cr3 = CPU_CR3;
	np2hax.state._cr4 = CPU_CR4;
	
	np2hax.state._gdt.base = CPU_GDTR_BASE;
	np2hax.state._gdt.limit = CPU_GDTR_LIMIT;
	np2hax.state._idt.base = CPU_IDTR_BASE;
	np2hax.state._idt.limit = CPU_IDTR_LIMIT;
	np2hax.state._ldt.selector = CPU_LDTR;
	np2hax.state._ldt.base = CPU_LDTR_BASE;
	np2hax.state._ldt.limit = CPU_LDTR_LIMIT;
	np2hax.state._tr.selector = CPU_TR;
	np2hax.state._tr.base = CPU_TR_BASE;
	np2hax.state._tr.limit = CPU_TR_LIMIT;

	//np2hax.state._dr0 = CPU_DR(0);
	//np2hax.state._dr1 = CPU_DR(1);
	//np2hax.state._dr2 = CPU_DR(2);
	//np2hax.state._dr3 = CPU_DR(3);
	////np2hax.state._dr4 = CPU_DR(4);
	////np2hax.state._dr5 = CPU_DR(5);
	//np2hax.state._dr6 = CPU_DR(6);
	//np2hax.state._dr7 = CPU_DR(7);
	
	np2hax.state._pde = CPU_STAT_PDE_BASE;
}

void i386hax_vm_exec(void) {
	HAX_TUNNEL *tunnel;
	UINT8 *iobuf;
	UINT32 exitstatus;
	UINT32 addr;

	np2haxthread.running = 1;

	tunnel = (HAX_TUNNEL*)np2hax.tunnel.va;
	iobuf = (UINT8*)np2hax.tunnel.io_va;
		
	ia32hax_copyregNP2toHAX();
	
	if(np2hax.update_regs) i386haxfunc_vcpu_setREGs(&np2hax.state);
	if(np2hax.update_fpu) i386haxfunc_vcpu_setFPU(&np2hax.fpustate);
	
	if(np2hax.irq_reqidx_cur != np2hax.irq_reqidx_end){
		if (tunnel->ready_for_interrupt_injection) {
			//if (CPU_EFLAG & I_FLAG) {
				i386haxfunc_vcpu_interrupt(np2hax.irq_req[np2hax.irq_reqidx_cur]);
				np2hax.irq_reqidx_cur++;
			//}else{
			//	printf("I FLAG?\n");
			//}
		}else{
			//printf("IRQ?\n");
		}
	}
	if(np2hax.irq_reqidx_cur != np2hax.irq_reqidx_end){
		tunnel->request_interrupt_window = 1;
	}else{
		tunnel->request_interrupt_window = 0;
	}

coutinue_cpu:
	i386haxfunc_vcpu_run();
	exitstatus = tunnel->_exit_status;
	switch (exitstatus) {
	case HAX_EXIT_IO:
		//printf("HAX_EXIT_IO\n");
		switch(tunnel->io._direction){
		case HAX_IO_OUT:
			if(tunnel->io._df==0){
				int i;
				UINT8 *bufptr = iobuf;
				for(i=0;i<tunnel->io._count;i++){
					if(tunnel->io._size==1){
						iocore_out8(tunnel->io._port, *bufptr);
					}else if(tunnel->io._size==2){
						iocore_out16(tunnel->io._port, LOADINTELWORD(bufptr));
					}else{ // tunnel->io._size==4
						iocore_out32(tunnel->io._port, LOADINTELDWORD(bufptr));
					}
					bufptr += tunnel->io._size;
				}
			}else{
				int i;
				UINT8 *bufptr = iobuf;
				bufptr += tunnel->io._size * (tunnel->io._count-1);
				for(i=tunnel->io._count-1;i>=0;i--){
					if(tunnel->io._size==1){
						iocore_out8(tunnel->io._port, *bufptr);
					}else if(tunnel->io._size==2){
						iocore_out16(tunnel->io._port, LOADINTELWORD(bufptr));
					}else{ // tunnel->io._size==4
						iocore_out32(tunnel->io._port, LOADINTELDWORD(bufptr));
					}
					bufptr -= tunnel->io._size;
				}
			}
			if(np2haxthread.lastA20en != (CPU_ADRSMASK != 0x000fffff)){
				i386hax_vm_sethmemory(CPU_ADRSMASK != 0x000fffff);
			}
			if(np2haxthread.lastITFbank != CPU_ITFBANK){
				i386hax_vm_setitfmemory(CPU_ITFBANK);
			}
			break;
		case HAX_IO_IN:
			if(tunnel->io._df==0){
				int i;
				UINT8 *bufptr = iobuf;
				for(i=0;i<tunnel->io._count;i++){
					if(tunnel->io._size==1){
						*bufptr = iocore_inp8(tunnel->io._port);
					}else if(tunnel->io._size==2){
						STOREINTELWORD(bufptr, iocore_inp16(tunnel->io._port));
					}else{ // tunnel->io._size==4
						STOREINTELDWORD(bufptr, iocore_inp32(tunnel->io._port));
					}
					bufptr += tunnel->io._size;
				}
			}else{
				int i;
				UINT8 *bufptr = iobuf;
				bufptr += tunnel->io._size * (tunnel->io._count-1);
				for(i=tunnel->io._count-1;i>=0;i--){
					if(tunnel->io._size==1){
						*bufptr = iocore_inp8(tunnel->io._port);
					}else if(tunnel->io._size==2){
						STOREINTELWORD(bufptr, iocore_inp16(tunnel->io._port));
					}else{ // tunnel->io._size==4
						STOREINTELDWORD(bufptr, iocore_inp32(tunnel->io._port));
					}
					bufptr -= tunnel->io._size;
				}
			}
			break;
		}
		np2haxthread.clockcount = GetTickCounter_Clock();
		if (!((np2haxthread.clockcount.QuadPart - np2haxthread.lastclock.QuadPart) * pccore.realclock * 4 / 5 / np2haxthread.clockpersec.QuadPart < CPU_BASECLOCK)) {
			break;
		}
		if(tunnel->io._port < 0x60 || 
			(0x430 <= tunnel->io._port && tunnel->io._port <= 0x64e) || 
			(0x7FD9 <= tunnel->io._port && tunnel->io._port <= 0x7FDF) || 
			(0x7FD9 <= tunnel->io._port && tunnel->io._port <= 0x7FDF)){
			goto coutinue_cpu;
		}
		//np2hax.update_regs = 1;
		break;
	case HAX_EXIT_FAST_MMIO:
		//printf("HAX_EXIT_FAST_MMIO\n");
		{
			HAX_FASTMMIO *mmio = (HAX_FASTMMIO*)np2hax.tunnel.io_va;
				
			switch(mmio->direction){
			case 0:
				if(mmio->size==1){
					mmio->value = memp_read8(mmio->gpa);
				}else if(mmio->size==2){
					mmio->value = memp_read16(mmio->gpa);
				}else{ // mmio->size==4
					mmio->value = memp_read32(mmio->gpa);
				}
				break;
			case 1:
				if(mmio->size==1){
					memp_write8(mmio->gpa, (UINT8)mmio->value);
				}else if(mmio->size==2){
					memp_write16(mmio->gpa, (UINT16)mmio->value);
				}else{ // mmio->size==4
					memp_write32(mmio->gpa, (UINT32)mmio->value);
				}
				break;
			default:
				if(mmio->size==1){
					memp_write8(mmio->gpa2, memp_read8(mmio->gpa));
				}else if(mmio->size==2){
					memp_write16(mmio->gpa2, memp_read16(mmio->gpa));
				}else{ // mmio->size==4
					memp_write32(mmio->gpa2, memp_read32(mmio->gpa));
				}
				break;
			}
		}
		np2haxthread.clockcount = GetTickCounter_Clock();
		if (!((np2haxthread.clockcount.QuadPart - np2haxthread.lastclock.QuadPart) * pccore.realclock * 4 / 5 / np2haxthread.clockpersec.QuadPart < CPU_BASECLOCK)) {
			break;
		}
		goto coutinue_cpu;
		break;
	//case HAX_EXIT_INTERRUPT:
	//	//printf("HAX_EXIT_INTERRUPT\n");
	//	np2haxthread.clockcount = GetTickCounter_Clock();
	//	if (!((np2haxthread.clockcount.QuadPart - np2haxthread.lastclock.QuadPart) * pccore.realclock / np2haxthread.clockpersec.QuadPart < CPU_BASECLOCK)) {
	//		break;
	//	}
	//	goto coutinue_cpu;
	//case HAX_EXIT_PAUSED:
	//	//printf("HAX_EXIT_PAUSED\n");
	//	np2haxthread.clockcount = GetTickCounter_Clock();
	//	if (!((np2haxthread.clockcount.QuadPart - np2haxthread.lastclock.QuadPart) * pccore.realclock / np2haxthread.clockpersec.QuadPart < CPU_BASECLOCK)) {
	//		break;
	//	}
	//	goto coutinue_cpu;
	}

	i386haxfunc_vcpu_getREGs(&np2hax.state);
	i386haxfunc_vcpu_getFPU(&np2hax.fpustate);
	np2hax.update_regs = np2hax.update_fpu = 0;
	
	ia32hax_copyregHAXtoNP2();

	
	np2haxthread.lastA20en = (CPU_ADRSMASK != 0x000fffff);
	np2haxthread.lastITFbank = CPU_ITFBANK;
	
	switch (exitstatus) {
	//case HAX_EXIT_IO:
	//	//printf("HAX_EXIT_IO\n");
	//	switch(tunnel->io._direction){
	//	case HAX_IO_OUT:
	//		if(tunnel->io._df==0){
	//			int i;
	//			UINT8 *bufptr = iobuf;
	//			for(i=0;i<tunnel->io._count;i++){
	//				if(tunnel->io._size==1){
	//					iocore_out8(tunnel->io._port, *bufptr);
	//				}else if(tunnel->io._size==2){
	//					iocore_out16(tunnel->io._port, LOADINTELWORD(bufptr));
	//				}else{ // tunnel->io._size==4
	//					iocore_out32(tunnel->io._port, LOADINTELDWORD(bufptr));
	//				}
	//				bufptr += tunnel->io._size;
	//			}
	//		}else{
	//			int i;
	//			UINT8 *bufptr = iobuf;
	//			bufptr += tunnel->io._size * (tunnel->io._count-1);
	//			for(i=tunnel->io._count-1;i>=0;i--){
	//				if(tunnel->io._size==1){
	//					iocore_out8(tunnel->io._port, *bufptr);
	//				}else if(tunnel->io._size==2){
	//					iocore_out16(tunnel->io._port, LOADINTELWORD(bufptr));
	//				}else{ // tunnel->io._size==4
	//					iocore_out32(tunnel->io._port, LOADINTELDWORD(bufptr));
	//				}
	//				bufptr -= tunnel->io._size;
	//			}
	//		}
	//		break;
	//	case HAX_IO_IN:
	//		if(tunnel->io._df==0){
	//			int i;
	//			UINT8 *bufptr = iobuf;
	//			for(i=0;i<tunnel->io._count;i++){
	//				if(tunnel->io._size==1){
	//					*bufptr = iocore_inp8(tunnel->io._port);
	//				}else if(tunnel->io._size==2){
	//					STOREINTELWORD(bufptr, iocore_inp16(tunnel->io._port));
	//				}else{ // tunnel->io._size==4
	//					STOREINTELDWORD(bufptr, iocore_inp32(tunnel->io._port));
	//				}
	//				bufptr += tunnel->io._size;
	//			}
	//		}else{
	//			int i;
	//			UINT8 *bufptr = iobuf;
	//			bufptr += tunnel->io._size * (tunnel->io._count-1);
	//			for(i=tunnel->io._count-1;i>=0;i--){
	//				if(tunnel->io._size==1){
	//					*bufptr = iocore_inp8(tunnel->io._port);
	//				}else if(tunnel->io._size==2){
	//					STOREINTELWORD(bufptr, iocore_inp16(tunnel->io._port));
	//				}else{ // tunnel->io._size==4
	//					STOREINTELDWORD(bufptr, iocore_inp32(tunnel->io._port));
	//				}
	//				bufptr -= tunnel->io._size;
	//			}
	//		}
	//		break;
	//	}
	//	//np2hax.update_regs = 1;
	//	break;
	case HAX_EXIT_MMIO:
		//printf("HAX_EXIT_MMIO\n");
		break;
	case HAX_EXIT_REALMODE:
		//printf("HAX_EXIT_REALMODE\n");
		break;
	//case HAX_EXIT_INTERRUPT:
	//	//printf("HAX_EXIT_INTERRUPT\n");
	//	break;
	case HAX_EXIT_UNKNOWN:
		//printf("HAX_EXIT_UNKNOWN\n");
		break;
	case HAX_EXIT_HLT:
		//printf("HAX_EXIT_HLT\n");
		break;
	case HAX_EXIT_STATECHANGE:
		//printf("HAX_EXIT_STATECHANGE\n");
		{
			int i;
			UINT32 memdumpa[0x100];
			UINT8 memdump[0x100];
			UINT32 baseaddr;
			addr = CPU_EIP + (CPU_CS << 4);
			baseaddr = addr & ~0xff;
			for(i=0;i<0x100;i++){
				memdumpa[i] = baseaddr + i;
				memdump[i] = memp_read8(memdumpa[i]);
			}
		}
		break;
	//case HAX_EXIT_PAUSED:
	//	//printf("HAX_EXIT_PAUSED\n");
	//	break;
	//case HAX_EXIT_FAST_MMIO:
	//	//printf("HAX_EXIT_FAST_MMIO\n");
	//	{
	//		HAX_FASTMMIO *mmio = (HAX_FASTMMIO*)np2hax.tunnel.io_va;
	//			
	//		switch(mmio->direction){
	//		case 0:
	//			if(mmio->size==1){
	//				mmio->value = memp_read8(mmio->gpa);
	//			}else if(mmio->size==2){
	//				mmio->value = memp_read16(mmio->gpa);
	//			}else{ // mmio->size==4
	//				mmio->value = memp_read32(mmio->gpa);
	//			}
	//			break;
	//		case 1:
	//			if(mmio->size==1){
	//				memp_write8(mmio->gpa, (UINT8)mmio->value);
	//			}else if(mmio->size==2){
	//				memp_write16(mmio->gpa, (UINT16)mmio->value);
	//			}else{ // mmio->size==4
	//				memp_write32(mmio->gpa, (UINT32)mmio->value);
	//			}
	//			break;
	//		default:
	//			if(mmio->size==1){
	//				memp_write8(mmio->gpa2, memp_read8(mmio->gpa));
	//			}else if(mmio->size==2){
	//				memp_write16(mmio->gpa2, memp_read16(mmio->gpa));
	//			}else{ // mmio->size==4
	//				memp_write32(mmio->gpa2, memp_read32(mmio->gpa));
	//			}
	//			break;
	//		}
	//	}
	//	//np2hax.update_regs = 1;
	//	break;
	case HAX_EXIT_PAGEFAULT:
		//printf("HAX_EXIT_PAGEFAULT\n");
		break;
	case HAX_EXIT_DEBUG:
		//printf("HAX_EXIT_DEBUG\n");
		{
			addr = CPU_EIP + (CPU_CS << 4);
			if(memp_read8(addr)==bioshookinfo.hookinst){
				CPU_EIP++;
				if(ia32hax_bioscall()){
				}
				np2hax.update_regs = 1;
				np2hax.update_segment_regs = 1;
			}else{
				printf("Unknown hook\n");
			}
		}
		break;
	default:
		break;
	}
	
	if(np2haxthread.lastA20en != (CPU_ADRSMASK != 0x000fffff)){
		i386hax_vm_sethmemory(CPU_ADRSMASK != 0x000fffff);
	}
	if(np2haxthread.lastITFbank != CPU_ITFBANK){
		i386hax_vm_setitfmemory(CPU_ITFBANK);
	}
	
	np2haxthread.running = 0;
}

static unsigned int __stdcall i386hax_ThreadFuncCPU(LPVOID vdParam) {

	HAX_TUNNEL *tunnel;
	UINT8 *iobuf;
	UINT32 exitstatus;
	UINT32 addr;
	
	UINT32 dbgon = 0;
	UINT32 flag1 = 0;
	//UINT8 lastop = 0;
	//UINT32 lastaddr = 0;
	
	UINT32 workaround_enable = 0;
	UINT32 workaround_bankidx = 0;

	UINT32 lastA20en;
	UINT32 lastITFbank;
	UINT32 suspendnext = 0;
	UINT32 keepCriticalSection = 0;

	tunnel = (HAX_TUNNEL*)np2hax.tunnel.va;
	iobuf = (UINT8*)np2hax.tunnel.io_va;
	
	while (!np2haxthread.exitflag) {
		np2haxthread.running = 1;
		
		ia32hax_copyregNP2toHAX();

		if(np2hax.update_regs) i386haxfunc_vcpu_setREGs(&np2hax.state);
		if(np2hax.update_fpu) i386haxfunc_vcpu_setFPU(&np2hax.fpustate);
		np2hax.update_regs = np2hax.update_fpu = 0;
		
		//if(!keepCriticalSection){
		//	i386hax_enter_criticalsection();
		//}
			//if(np2hax.irq_reqidx_cur != np2hax.irq_reqidx_end){
			//	if (tunnel->ready_for_interrupt_injection && !(CPU_FLAG & I_FLAG)) {
			//		i386haxfunc_vcpu_interrupt(np2hax.irq_req[np2hax.irq_reqidx_cur]);
			//		np2hax.irq_reqidx_cur++;
			//	}else{
			//		printf("IRQ?\n");
			//	}
			//}
			if(np2hax.irq_reqidx_cur != np2hax.irq_reqidx_end){
				if (tunnel->ready_for_interrupt_injection) {
					//if (CPU_EFLAG & I_FLAG) {
						i386haxfunc_vcpu_interrupt(np2hax.irq_req[np2hax.irq_reqidx_cur]);
						np2hax.irq_reqidx_cur++;
					//}else{
					//	printf("I FLAG?\n");
					//}
				}else{
					//printf("IRQ?\n");
				}
			}
			if(np2hax.irq_reqidx_cur != np2hax.irq_reqidx_end){
				tunnel->request_interrupt_window = 1;
			}else{
				tunnel->request_interrupt_window = 0;
			}
		//if(!keepCriticalSection){
		//	i386hax_leave_criticalsection();
		//}
continue_cpu:
		i386haxfunc_vcpu_run();
		exitstatus = tunnel->_exit_status;
		//if(!keepCriticalSection){
		//	switch (exitstatus) {
		//	case HAX_EXIT_REALMODE:
		//	case HAX_EXIT_INTERRUPT:
		//	case HAX_EXIT_PAGEFAULT:
		//		//printf("HAX_EXIT_PAGEFAULT\n");
		//		ia32hax_copyregHAXtoNP2();
		//		i386hax_leave_criticalsection();
		//		i386hax_enter_criticalsection();
		//		ia32hax_copyregNP2toHAX();
		//		goto continue_cpu;
		//	}
		//}

		i386haxfunc_vcpu_getREGs(&np2hax.state);
		i386haxfunc_vcpu_getFPU(&np2hax.fpustate);
		np2hax.update_regs = np2hax.update_fpu = 0;
		
		if(!keepCriticalSection){
			i386hax_enter_criticalsection();
		}
		keepCriticalSection = 0;

		ia32hax_copyregHAXtoNP2();

		lastA20en = (CPU_ADRSMASK != 0x000fffff);
		lastITFbank = CPU_ITFBANK;
		
		if(0x1609 <= CPU_EIP && CPU_EIP <= 0x16A8){
			//HAX_DEBUG hax_dbg = {0};
			//hax_dbg.control = HAX_DEBUG_ENABLE|HAX_DEBUG_USE_SW_BP|HAX_DEBUG_USE_HW_BP|HAX_DEBUG_STEP;
			//if(i386haxfunc_vcpu_debug(&hax_dbg)==FAILURE){
			//}
			if(np2hax.bankmem.bank.A0!=0 || np2hax.bankmem.bank.A4!=0 || np2hax.bankmem.bank.A8!=0 || np2hax.bankmem.bank.AC!=0){
				//np2hax.bankmem.bank.A0 = 0;
				//np2hax.bankmem.bank.A4 = 0;
				//np2hax.bankmem.bank.A8 = 1;
				//np2hax.bankmem.bank.AC = 0;
				//i386hax_vm_setbankmemory();
				//i386haxfunc_vcpu_kickoff();
			}
		}
		switch (exitstatus) {
		case HAX_EXIT_IO:
			//i386hax_enter_criticalsection();
			//printf("HAX_EXIT_IO\n");
			switch(tunnel->io._direction){
			case HAX_IO_OUT:
				if(tunnel->io._df==0){
					int i;
					UINT8 *bufptr = iobuf;
					for(i=0;i<tunnel->io._count;i++){
						if(tunnel->io._size==1){
							iocore_out8(tunnel->io._port, *bufptr);
						}else if(tunnel->io._size==2){
							iocore_out16(tunnel->io._port, LOADINTELWORD(bufptr));
						}else{ // tunnel->io._size==4
							iocore_out32(tunnel->io._port, LOADINTELDWORD(bufptr));
						}
						bufptr += tunnel->io._size;
					}
				}else{
					int i;
					UINT8 *bufptr = iobuf;
					bufptr += tunnel->io._size * (tunnel->io._count-1);
					for(i=tunnel->io._count-1;i>=0;i--){
						if(tunnel->io._size==1){
							iocore_out8(tunnel->io._port, *bufptr);
						}else if(tunnel->io._size==2){
							iocore_out16(tunnel->io._port, LOADINTELWORD(bufptr));
						}else{ // tunnel->io._size==4
							iocore_out32(tunnel->io._port, LOADINTELDWORD(bufptr));
						}
						bufptr -= tunnel->io._size;
					}
				}
				break;
			case HAX_IO_IN:
				if(tunnel->io._df==0){
					int i;
					UINT8 *bufptr = iobuf;
					for(i=0;i<tunnel->io._count;i++){
						if(tunnel->io._size==1){
							*bufptr = iocore_inp8(tunnel->io._port);
						}else if(tunnel->io._size==2){
							STOREINTELWORD(bufptr, iocore_inp16(tunnel->io._port));
						}else{ // tunnel->io._size==4
							STOREINTELDWORD(bufptr, iocore_inp32(tunnel->io._port));
						}
						bufptr += tunnel->io._size;
					}
				}else{
					int i;
					UINT8 *bufptr = iobuf;
					bufptr += tunnel->io._size * (tunnel->io._count-1);
					for(i=tunnel->io._count-1;i>=0;i--){
						if(tunnel->io._size==1){
							*bufptr = iocore_inp8(tunnel->io._port);
						}else if(tunnel->io._size==2){
							STOREINTELWORD(bufptr, iocore_inp16(tunnel->io._port));
						}else{ // tunnel->io._size==4
							STOREINTELDWORD(bufptr, iocore_inp32(tunnel->io._port));
						}
						bufptr -= tunnel->io._size;
					}
				}
				break;
			}
			//np2hax.update_regs = 1;
			//i386hax_leave_criticalsection();
			keepCriticalSection = 1;
			//suspendnext = 1;
			//np2haxthread.reqsuspend = 1;
			//np2hax.update_regs = 1;
			break;
		case HAX_EXIT_MMIO:
			//printf("HAX_EXIT_MMIO\n");
			break;
		case HAX_EXIT_REALMODE:
			//printf("HAX_EXIT_REALMODE\n");
			break;
		case HAX_EXIT_INTERRUPT:
			//printf("HAX_EXIT_INTERRUPT\n");
			//{
			//	int i;
			//	for(i=0;i<16;i++){
			//		pic_resetirq(i);
			//	}
			//}
			break;
		case HAX_EXIT_UNKNOWN:
			//printf("HAX_EXIT_UNKNOWN\n");
			break;
		case HAX_EXIT_HLT:
			//printf("HAX_EXIT_HLT\n");
			addr = CPU_EIP + (CPU_CS << 4) - 1;
			if(memp_read8(addr)==bioshookinfo.hookinst){
				CPU_EIP--;
				CPU_PREV_EIP--;
				if(ia32hax_bioscall()){
					i386haxfunc_vcpu_interrupt(2);
				}
				CPU_EIP++;
				CPU_PREV_EIP++;
				np2hax.update_regs = 1;
			}
			break;
		case HAX_EXIT_STATECHANGE:
			//printf("HAX_EXIT_STATECHANGE\n");
			{
				int i;
				UINT32 memdumpa[0x100];
				UINT8 memdump[0x100];
				UINT32 baseaddr;
				addr = CPU_EIP + (CPU_CS << 4);
				baseaddr = addr & ~0xff;
				for(i=0;i<0x100;i++){
					memdumpa[i] = baseaddr + i;
					memdump[i] = memp_read8(memdumpa[i]);
				}
				//printf("memdump");
			}
			//np2hax.enable = 0;
			break;
		case HAX_EXIT_PAUSED:
			//printf("HAX_EXIT_PAUSED\n");
			break;
		case HAX_EXIT_FAST_MMIO:
			//i386hax_enter_criticalsection();
			{
				HAX_FASTMMIO *mmio = (HAX_FASTMMIO*)np2hax.tunnel.io_va;
				
				switch(mmio->direction){
				case 0:
					if(mmio->size==1){
						mmio->value = memp_read8(mmio->gpa);
						//if(mmio->gpa==0xa3fea && CPU_EIP==0x173f){
						//	HAX_DEBUG hax_dbg = {0};
						//	hax_dbg.control = HAX_DEBUG_ENABLE|HAX_DEBUG_USE_SW_BP|HAX_DEBUG_USE_HW_BP|HAX_DEBUG_STEP;
						//	if(i386haxfunc_vcpu_debug(&hax_dbg)==FAILURE){
						//	}
						//	{
						//		int i;
						//		UINT32 memdumpa[0x100];
						//		UINT8 memdump[0x100];
						//		UINT32 baseaddr;
						//		addr = CPU_EIP + (CPU_CS << 4);
						//		baseaddr = addr & ~0xff;
						//		for(i=0;i<0x100;i++){
						//			memdumpa[i] = baseaddr + i;
						//			memdump[i] = memp_read8(memdumpa[i]);
						//		}
						//		printf("memdump");
						//	}
						//	dbgon = 1;
						//}
					}else if(mmio->size==2){
						mmio->value = memp_read16(mmio->gpa);
					}else{ // mmio->size==4
						mmio->value = memp_read32(mmio->gpa);
					}
					break;
				case 1:
					if(mmio->size==1){
						memp_write8(mmio->gpa, (UINT8)mmio->value);
					}else if(mmio->size==2){
						memp_write16(mmio->gpa, (UINT16)mmio->value);
					}else{ // mmio->size==4
						memp_write32(mmio->gpa, (UINT32)mmio->value);
					}
					break;
				default:
					if(mmio->size==1){
						memp_write8(mmio->gpa2, memp_read8(mmio->gpa));
					}else if(mmio->size==2){
						memp_write16(mmio->gpa2, memp_read16(mmio->gpa));
					}else{ // mmio->size==4
						memp_write32(mmio->gpa2, memp_read32(mmio->gpa));
					}
					break;
				}
				//printf("HAX_EXIT_FAST_MMIO\n");
			}
			//np2hax.update_regs = 1;
			//keepCriticalSection = 1;
			//i386hax_leave_criticalsection();
			break;
		case HAX_EXIT_PAGEFAULT:
			//printf("HAX_EXIT_PAGEFAULT\n");
			break;
		case HAX_EXIT_DEBUG:
			//i386hax_enter_criticalsection();
			{
				addr = CPU_EIP + (CPU_CS << 4);
				if(memp_read8(addr)==bioshookinfo.hookinst){
					//CPU_EIP-=1;
					CPU_EIP++;
					if(ia32hax_bioscall()){
						//if(CPU_PREV_EIP==0xa8){
						//	printf("memdump");
						//}
						//if(CPU_PREV_EIP==0xa9){
						//	HAX_DEBUG hax_dbg = {0};
						//	hax_dbg.control = HAX_DEBUG_ENABLE|HAX_DEBUG_USE_SW_BP|HAX_DEBUG_USE_HW_BP|HAX_DEBUG_STEP;
						//	if(i386haxfunc_vcpu_debug(&hax_dbg)==FAILURE){
						//	}
						//	{
						//		int i;
						//		UINT32 memdumpa[0x100];
						//		UINT8 memdump[0x100];
						//		UINT32 baseaddr;
						//		addr = CPU_EIP + (CPU_CS << 4);
						//		baseaddr = addr & ~0xff;
						//		for(i=0;i<0x100;i++){
						//			memdumpa[i] = baseaddr + i;
						//			memdump[i] = memp_read8(memdumpa[i]);
						//		}
						//		printf("memdump");
						//	}
						//	dbgon = 1;
						//}
					}else{
						TRACEOUT(("Not hook?: EIP:%08xh, addr:%08xh, data:%02xh", CPU_EIP, CPU_EIP + (CPU_CS << 4), memp_read8(CPU_EIP + (CPU_CS << 4))));
					}
					//np2hax.state._eip = np2hax.state._eip ^ 0x1;
					//np2hax.state._sp= np2hax.state._sp ^ 0x1;
					//i386haxfunc_vcpu_setREGs(&np2hax.state);
					np2hax.update_regs = 1;
				}
				if(dbgon){
					TRACEOUT(("EIP:%08xh, addr:%08xh, data:%02xh", CPU_EIP, CPU_EIP + (CPU_CS << 4), memp_read8(CPU_EIP + (CPU_CS << 4))));
				}
				//if(dbgon){
				//	TRACEOUT(("EIP:%08xh, addr:%08xh, data:%02xh", CPU_EIP, CPU_EIP + (CPU_CS << 4), memp_read8(CPU_EIP + (CPU_CS << 4))));
				//	if(CPU_EIP==0x16fa){
				//		HAX_DEBUG hax_dbg = {0};
				//		hax_dbg.control = HAX_DEBUG_ENABLE|HAX_DEBUG_USE_SW_BP;
				//		i386haxfunc_vcpu_debug(&hax_dbg);
				//		CPU_EBP = 0x167b;
				//		CPU_ECX = 0x4e1;
				//		printf("break!!!");
				//		np2hax.bankmem.bank.A0 = 0;
				//		np2hax.bankmem.bank.A4 = 0;
				//		np2hax.bankmem.bank.A8 = 0;
				//		np2hax.bankmem.bank.AC = 0;
				//		i386hax_vm_setbankmemory();
				//	}
				//}
				//{
				//	int i;
				//	UINT32 memdumpa[0x100];
				//	UINT8 memdump[0x100];
				//	UINT32 baseaddr;
				//	addr = CPU_EIP + (CPU_CS << 4);
				//	baseaddr = addr & ~0xff;
				//	for(i=0;i<0x100;i++){
				//		memdumpa[i] = baseaddr + i;
				//		memdump[i] = memp_read8(memdumpa[i]);
				//	}
				//	printf("memdump");
				//}
			}
			//printf("HAX_EXIT_DEBUG\n");
			//i386hax_leave_criticalsection();
			break;
		}
		//if(flag1==0 && CPU_EBP==0x167b){
		//	HAX_DEBUG hax_dbg = {0};
		//	hax_dbg.control = HAX_DEBUG_ENABLE|HAX_DEBUG_USE_SW_BP|HAX_DEBUG_USE_HW_BP|HAX_DEBUG_STEP;
		//	if(i386haxfunc_vcpu_debug(&hax_dbg)==FAILURE){
		//	}
		//	flag1 = 1;
		//}else if(flag1==1 && (CPU_EBP!=0x167b || CPU_EIP==0x173f)){
		//	{
		//		int i;
		//		UINT32 memdumpa[0x100];
		//		UINT8 memdump[0x100];
		//		UINT32 baseaddr;
		//		addr = CPU_EIP + (CPU_CS << 4);
		//		baseaddr = addr & ~0xff;
		//		for(i=0;i<0x100;i++){
		//			memdumpa[i] = baseaddr + i;
		//			memdump[i] = memp_read8(memdumpa[i]);
		//		}
		//		printf("memdump");
		//	}
		//	dbgon = 1;
		//}
		
		if(0x00b4==CPU_PREV_EIP && 0xfd80==CPU_CS){
			//printf("debug!!!");
			//{
			//	int i;
			//	UINT32 memdumpa[0x100];
			//	UINT8 memdump[0x100];
			//	UINT32 baseaddr;
			//	addr = CPU_EIP + (CPU_CS << 4);
			//	baseaddr = addr & ~0xff;
			//	for(i=0;i<0x100;i++){
			//		memdumpa[i] = baseaddr + i;
			//		memdump[i] = memp_read8(memdumpa[i]);
			//	}
			//	printf("memdump");
			//}
		}

		//lastop = memp_read8(CPU_EIP + (CPU_CS << 4));
		//lastaddr =CPU_EIP + (CPU_CS << 4);

		np2haxthread.running = 0;
		//if(!np2hax.enable){
		//	np2haxthread.reqsuspend = 1;
		//}
		if(!keepCriticalSection){
			i386hax_leave_criticalsection();
		}
		//if(np2haxthread.reqsuspend && !suspendnext){
		//	np2haxthread.suspended = 1;
		//	np2haxthread.reqsuspend = 0;
		//	SuspendThread(np2haxthread.hThreadCPU);
		//}else if(suspendnext>0){
		//	suspendnext--;
		//}
		
		if(lastA20en != (CPU_ADRSMASK != 0x000fffff)){
			i386hax_vm_sethmemory(CPU_ADRSMASK != 0x000fffff);
		}
		if(lastITFbank != CPU_ITFBANK){
			i386hax_vm_setitfmemory(CPU_ITFBANK);
			//if(lastITFbank){
			//	HAX_DEBUG hax_dbg = {0};
			//	hax_dbg.control = HAX_DEBUG_ENABLE|HAX_DEBUG_USE_SW_BP|HAX_DEBUG_USE_HW_BP|HAX_DEBUG_STEP;
			//	i386haxfunc_vcpu_debug(&hax_dbg);
			//	dbgon = 1;
			//}
		}

	}

	if(keepCriticalSection){
		i386hax_leave_criticalsection();
	}

	return 0;
}

void i386hax_createVMThread(void) {
	
#if defined(_WIN32)
	unsigned int dwID;

	if(np2haxthread.hThreadCPU){
		i386hax_disposeVMThread();
	}

	memset(&np2haxthread, 0, sizeof(np2haxthread));

	np2haxthread.exitflag = 0;
	np2haxthread.hThreadCPU = (HANDLE)_beginthreadex(NULL, 0, i386hax_ThreadFuncCPU, NULL, CREATE_SUSPENDED, &dwID);
	
	i386haxfunc_vcpu_getREGs(&np2hax.state);
	i386haxfunc_vcpu_getFPU(&np2hax.fpustate);
	memcpy(&np2hax.default_state, &np2hax.state, sizeof(np2hax.state));
	memcpy(&np2hax.default_fpustate, &np2hax.fpustate, sizeof(np2hax.fpustate));
	//memset(&np2hax.state, 0, sizeof(np2hax.state));
	//memset(&np2hax.fpustate, 0, sizeof(np2hax.fpustate));
	np2hax.update_regs = np2hax.update_fpu = 1;
#endif
}
void i386hax_resumeVMThread(void) {
	
#if defined(_WIN32)
	np2haxthread.suspended = 0;
	np2haxthread.reqsuspend = 0;
	ResumeThread(np2haxthread.hThreadCPU);
#endif
}
void i386hax_suspendVMThread(void) {
	
#if defined(_WIN32)
	// TODO: ‘Ò‚Â
	//HAX_DEBUG hax_dbg = {0};
	//hax_dbg.control = HAX_DEBUG_ENABLE|HAX_DEBUG_USE_SW_BP|HAX_DEBUG_USE_HW_BP|HAX_DEBUG_STEP;
	//i386haxfunc_vcpu_debug(&hax_dbg);
	//while(np2haxthread.running){
	//	np2haxthread.reqsuspend = 1;
	//}
#endif
}
void i386hax_disposeVMThread(void) {
	
#if defined(_WIN32)
	if(np2haxthread.hThreadCPU){
		np2haxthread.exitflag = 1;
		ResumeThread(np2haxthread.hThreadCPU);
		while(WaitForSingleObject(np2haxthread.hThreadCPU, 500)==WAIT_TIMEOUT){
			ResumeThread(np2haxthread.hThreadCPU);
		}
		np2haxthread.hThreadCPU = 0;
	}
#endif
}

void i386hax_disposeVM(void) {
	
#if defined(_WIN32)
	if(!np2hax.hDevice) return;
	if(!np2hax.hVCPUDevice) return;
	if(!np2hax.hVMDevice) return;

	i386hax_disposeVMThread();

	CloseHandle(np2hax.hVCPUDevice);
	np2hax.hVCPUDevice = NULL;
	CloseHandle(np2hax.hVMDevice);
	np2hax.hVMDevice = NULL;
	
	TRACEOUT(("HAXM: HAX VM disposed."));
    //msgbox("HAXM deinit", "HAX VM disposed.");
#endif
}

void i386hax_deinitialize(void) {
	
#if defined(_WIN32)
	if(!np2hax.hDevice) return;

	i386hax_disposeVM();
	
	CloseHandle(np2hax.hVCPUDevice);
	np2hax.hVCPUDevice = NULL;
	CloseHandle(np2hax.hVMDevice);
	np2hax.hVMDevice = NULL;
	CloseHandle(np2hax.hDevice);
	np2hax.hDevice = NULL;
	
	if(np2hax_cs_initialized){
		DeleteCriticalSection(&np2hax_cs);
		np2hax_cs_initialized = 0;
	}

	TRACEOUT(("HAXM: HAX deinitialized."));
    //msgbox("HAXM deinit", "HAX deinitialized.");

#endif
}

void i386hax_vm_allocmemory(void) {
	
#if defined(_WIN32)
	HAX_ALLOC_RAM_INFO info = {0};

	if(!np2hax.hVMDevice) return;
	
	info.size = 0x200000;
	info.va = (UINT64)mem;
	if(i386haxfunc_allocRAM(&info)==FAILURE){
		TRACEOUT(("HAXM: HAX VM alloc memory failed."));
		msgbox("HAXM VM", "HAX VM alloc memory failed.");
		np2hax.enable = 0;
		i386hax_disposeVM();
		return;
	}
	
	info.size = CPU_EXTMEMSIZE + 4096;
	info.va = (UINT64)CPU_EXTMEM;
	if(i386haxfunc_allocRAM(&info)==FAILURE){
		TRACEOUT(("HAXM: HAX VM alloc memory failed."));
		msgbox("HAXM VM", "HAX VM alloc memory failed.");
		np2hax.enable = 0;
		i386hax_disposeVM();
		return;
	}
	
	TRACEOUT(("HAXM: HAX VM alloc memory."));
    //msgbox("HAXM VM", "HAX VM alloc memory.");
#endif
}
void i386hax_vm_setmemory(void) {
	
#if defined(_WIN32)
	HAX_SET_RAM_INFO info = {0};

	if(!np2hax.enable) return;
	if(!np2hax.hVMDevice) return;
	
	info.pa_start = 0;
	info.size = 0xA0000;
	info.flags = 0;
	info.va = (UINT64)mem;
	if(i386haxfunc_setRAM(&info)==FAILURE){
		TRACEOUT(("HAXM: HAX VM set memory failed."));
		msgbox("HAXM VM", "HAX VM set memory failed.");
		np2hax.enable = 0;
		return;
	}
	
	//info.pa_start = 0xFFF00000;
	//info.size = 0xA0000;
	//info.flags = 0;
	//info.va = (UINT64)mem;
	//if(i386haxfunc_setRAM(&info)==FAILURE){
	//	TRACEOUT(("HAXM: HAX VM set memory failed."));
	//	msgbox("HAXM VM", "HAX VM set memory failed.");
	//	np2hax.enable = 0;
	//	return;
	//}
	
	TRACEOUT(("HAXM: HAX VM set memory."));
    //msgbox("HAXM VM", "HAX VM set memory.");
#endif
}
void i386hax_vm_setbankmemory() {
	
#if defined(_WIN32)
	HAX_SET_RAM_INFO info = {0};
	UINT32 bankaddr = 0xA0000;
	int i;

	if(!np2hax.hVMDevice) 
	if(!np2hax.enable) return;
	
	for(i=0;i<6*4-2;i++){
		info.pa_start = bankaddr;
		info.size = 0x4000;
		info.flags = HAX_RAM_INFO_INVALID;
		info.va = 0;
		i386haxfunc_setRAM(&info);
		if(!np2hax.bankmem.banks[i]){
			info.flags = 0;
			info.va = (UINT64)(mem + info.pa_start);
			if(i386haxfunc_setRAM(&info)==FAILURE){
				TRACEOUT(("HAXM: HAX VM set memory failed."));
				msgbox("HAXM VM", "HAX VM set memory failed.");
				np2hax.enable = 0;
				return;
			}
		}
		bankaddr += 0x4000;
	}
	
	//bankaddr = 0xFFFA0000;
	//for(i=0;i<6*4-2;i++){
	//	info.pa_start = bankaddr;
	//	info.size = 0x4000;
	//	info.flags = HAX_RAM_INFO_INVALID;
	//	info.va = 0;
	//	i386haxfunc_setRAM(&info);
	//	info.flags = 0;
	//	info.va = (UINT64)(mem + info.pa_start);
	//	if(i386haxfunc_setRAM(&info)==FAILURE){
	//		TRACEOUT(("HAXM: HAX VM set memory failed."));
	//		msgbox("HAXM VM", "HAX VM set memory failed.");
	//		np2hax.enable = 0;
	//		return;
	//	}
	//	bankaddr += 0x4000;
	//}

	TRACEOUT(("HAXM: HAX VM set bank memory."));
    //msgbox("HAXM VM", "HAX VM set bank memory.");
#endif
}
void i386hax_vm_setitfmemory(UINT8 isitfbank) {
	
#if defined(_WIN32)
	HAX_SET_RAM_INFO info = {0};

	if(!np2hax.hVMDevice) 
	if(!np2hax.enable) return;
	
	info.pa_start = 0xF8000;
	info.size = 0x8000;
	info.flags = HAX_RAM_INFO_INVALID;
	if(i386haxfunc_setRAM(&info)==FAILURE){
	}
	if(isitfbank){
		info.flags = HAX_RAM_INFO_ROM;
		info.va = (UINT64)(mem + ITF_ADRS);
	}else{
		info.flags = 0;
		info.va = (UINT64)(mem + info.pa_start);
	}
	if(i386haxfunc_setRAM(&info)==FAILURE){
		TRACEOUT(("HAXM: HAX VM set ITF bank memory failed."));
		msgbox("HAXM VM", "HAX VM set ITF bank memory failed.");
		np2hax.enable = 0;
		return;
	}
	
	//info.pa_start = 0xFFFF8000;
	//info.size = 0x8000;
	//info.flags = HAX_RAM_INFO_INVALID;
	//if(i386haxfunc_setRAM(&info)==FAILURE){
	//}
	//if(isitfbank){
	//	info.flags = HAX_RAM_INFO_ROM;
	//	info.va = (UINT64)(mem + ITF_ADRS);
	//}else{
	//	info.flags = 0;
	//	info.va = (UINT64)(mem + info.pa_start);
	//}
	//if(i386haxfunc_setRAM(&info)==FAILURE){
	//	TRACEOUT(("HAXM: HAX VM set ITF bank memory failed."));
	//	msgbox("HAXM VM", "HAX VM set ITF bank memory failed.");
	//	np2hax.enable = 0;
	//	return;
	//}

	if(isitfbank){
		TRACEOUT(("HAXM: HAX VM set ITF bank memory. (ITF)"));
		//msgbox("HAXM VM", "HAX VM set ITF bank memory. (ITF)");
	}else{
		TRACEOUT(("HAXM: HAX VM set ITF bank memory. (OFF)"));
		//msgbox("HAXM VM", "HAX VM set ITF bank memory. (OFF)");
	}
#endif
}
void i386hax_vm_sethmemory(UINT8 a20en) {
	
#if defined(_WIN32)
	HAX_SET_RAM_INFO info = {0};
	
	if(!np2hax.enable) return;
	if(!np2hax.hVMDevice) return;
	
	info.pa_start = 0x100000;
	info.size = 0x10000;
	info.flags = HAX_RAM_INFO_INVALID;
	if(i386haxfunc_setRAM(&info)==FAILURE){
	}

	if(a20en){
		info.flags = 0;
		info.va = (UINT64)(mem + info.pa_start);
	}else{
		info.flags = 0;
		info.va = (UINT64)(mem);
	}
	if(i386haxfunc_setRAM(&info)==FAILURE){
		TRACEOUT(("HAXM: HAX VM set high memory failed."));
		msgbox("HAXM VM", "HAX VM set high memory failed.");
		np2hax.enable = 0;
		return;
	}
	
	if(a20en){
		TRACEOUT(("HAXM: HAX VM set high memory. (A20 ON)"));
		//msgbox("HAXM VM", "HAX VM set high memory. (A20 ON)");
	}else{
		TRACEOUT(("HAXM: HAX VM set ITF bank memory. (A20 OFF)"));
		//msgbox("HAXM VM", "HAX VM set ITF bank memory. (A20 OFF)");
	}
#endif
}
void i386hax_vm_setextmemory(void) {
	
#if defined(_WIN32)
	HAX_SET_RAM_INFO info = {0};
	
	if(!np2hax.enable) return;
	if(!np2hax.hVMDevice) return;
	
	info.pa_start = 0x110000;
	info.size = CPU_EXTLIMIT16 - 0x110000;
	info.flags = 0;
	info.va = (UINT64)(CPU_EXTMEMBASE + info.pa_start);
	if(i386haxfunc_setRAM(&info)==FAILURE){
		TRACEOUT(("HAXM: HAX VM set ext16 memory failed."));
		msgbox("HAXM VM", "HAX VM set ext16 memory failed.");
		np2hax.enable = 0;
		return;
	}

	if(CPU_EXTLIMIT > 0x01000000){
		info.pa_start = 0x01000000;
		info.size = CPU_EXTLIMIT - 0x01000000;
		info.flags = 0;
		info.va = (UINT64)(CPU_EXTMEMBASE + info.pa_start);
		if(i386haxfunc_setRAM(&info)==FAILURE){
			TRACEOUT(("HAXM: HAX VM set ext memory failed."));
			msgbox("HAXM VM", "HAX VM set ext memory failed.");
			np2hax.enable = 0;
			return;
		}
	}
	
	TRACEOUT(("HAXM: HAX VM set ext memory."));
    //msgbox("HAXM VM", "HAX VM set ext memory.");
#endif
}

#endif