#ifndef	NP2_I386HAX_HAXCORE_H__
#define	NP2_I386HAX_HAXCORE_H__

#if defined(SUPPORT_IA32_HAXM)

#ifdef __cplusplus
extern "C" {
#endif
	
#if defined(_WIN32)
typedef struct {
	UINT8	available;
	UINT8	enable;
	HANDLE	hDevice;
	HANDLE	hVMDevice;
	HANDLE	hVCPUDevice;
	UINT32	vm_id;
	HAX_TUNNEL_INFO	tunnel;
	HAX_VCPU_STATE	state;
	HAX_FX_LAYOUT	fpustate;
	HAX_VCPU_STATE	default_state;
	HAX_FX_LAYOUT	default_fpustate;
	UINT8 update_regs;
	UINT8 update_segment_regs;
	UINT8 update_fpu;

	UINT8 irq_req[256];
	UINT8 irq_reqidx_cur;
	UINT8 irq_reqidx_end;

	union{
		struct {
			UINT8 A0;
			UINT8 A4;
			UINT8 A8;
			UINT8 AC;
			UINT8 B0;
			UINT8 B4;
			UINT8 B8;
			UINT8 BC;
			UINT8 C0;
			UINT8 C4;
			UINT8 C8;
			UINT8 CC;
			UINT8 D0;
			UINT8 D4;
			UINT8 D8;
			UINT8 DC;
			UINT8 E0;
			UINT8 E4;
			UINT8 E8;
			UINT8 EC;
			UINT8 F0;
			UINT8 F4;
			UINT8 reserved1; // F8 reserved
			UINT8 reserved2; // FC reserved
		} bank;
		UINT8 banks[6*4];
	} bankmem;
} NP2_HAX;
typedef struct {
	HANDLE	hThreadCPU;
	UINT8	exitflag;
	UINT8	reqsuspend;
	UINT8	suspended;
	UINT8	running;

	LARGE_INTEGER lastclock;
	LARGE_INTEGER clockpersec;
	LARGE_INTEGER clockcount;

	UINT32 lastA20en;
	UINT32 lastITFbank;
	
} NP2_HAX_THREAD;

extern	NP2_HAX	np2hax;
extern	NP2_HAX_THREAD	np2haxthread;

#endif

#ifdef __cplusplus
}
#endif

BOOL i386hax_tryenter_criticalsection(void);
void i386hax_enter_criticalsection(void);
void i386hax_leave_criticalsection(void);

UINT8 i386hax_check(void);
void i386hax_initialize(void);
void i386hax_createVM(void);
void i386hax_createVMThread(void);
void i386hax_resumeVMThread(void);
void i386hax_suspendVMThread(void);
void i386hax_disposeVMThread(void);
void i386hax_disposeVM(void);
void i386hax_deinitialize(void);
void i386hax_resetVM(void);

void i386hax_vm_exec(void);

void i386hax_vm_allocmemory(void); // ÉÅÉÇÉäóÃàÊÇìoò^
void i386hax_vm_setmemory(void); // 00000hÅ`BFFFFhÇ‹Ç≈
void i386hax_vm_setbankmemory(void); // A0000hÅ`F7FFFhÇ‹Ç≈
void i386hax_vm_setitfmemory(UINT8 isitfbank); // F8000hÅ`FFFFFhÇ‹Ç≈
void i386hax_vm_sethmemory(UINT8 a20en); // 100000hÅ`10FFFFhÇ‹Ç≈
void i386hax_vm_setextmemory(void); // 110000hà»ç~

#endif

#endif	/* !NP2_I386HAX_HAXCORE_H__ */
