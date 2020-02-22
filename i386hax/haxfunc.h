
#ifndef	NP2_I386HAX_HAXFUNC_H__
#define	NP2_I386HAX_HAXFUNC_H__

#if defined(SUPPORT_IA32_HAXM)

#define HAX_DEVICE_TYPE 0x4000

#define HAX_IOCTL_VERSION			CTL_CODE(HAX_DEVICE_TYPE, 0x900, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define HAX_IOCTL_CAPABILITY		CTL_CODE(HAX_DEVICE_TYPE, 0x910, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define HAX_IOCTL_SET_MEMLIMIT		CTL_CODE(HAX_DEVICE_TYPE, 0x911, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define HAX_IOCTL_CREATE_VM			CTL_CODE(HAX_DEVICE_TYPE, 0x901, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define HAX_VM_IOCTL_VCPU_CREATE	CTL_CODE(HAX_DEVICE_TYPE, 0x902, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define HAX_VM_IOCTL_ALLOC_RAM		CTL_CODE(HAX_DEVICE_TYPE, 0x903, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define HAX_VM_IOCTL_SET_RAM		CTL_CODE(HAX_DEVICE_TYPE, 0x904, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define HAX_VM_IOCTL_VCPU_DESTROY	CTL_CODE(HAX_DEVICE_TYPE, 0x905, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define HAX_VM_IOCTL_ADD_RAMBLOCK	CTL_CODE(HAX_DEVICE_TYPE, 0x913, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define HAX_VM_IOCTL_SET_RAM2		CTL_CODE(HAX_DEVICE_TYPE, 0x914, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define HAX_VM_IOCTL_PROTECT_RAM	CTL_CODE(HAX_DEVICE_TYPE, 0x915, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define HAX_VCPU_IOCTL_RUN			CTL_CODE(HAX_DEVICE_TYPE, 0x906, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define HAX_VCPU_IOCTL_SET_MSRS		CTL_CODE(HAX_DEVICE_TYPE, 0x907, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define HAX_VCPU_IOCTL_GET_MSRS		CTL_CODE(HAX_DEVICE_TYPE, 0x908, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define HAX_VCPU_IOCTL_SET_FPU		CTL_CODE(HAX_DEVICE_TYPE, 0x909, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define HAX_VCPU_IOCTL_GET_FPU		CTL_CODE(HAX_DEVICE_TYPE, 0x90a, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define HAX_VCPU_IOCTL_SETUP_TUNNEL CTL_CODE(HAX_DEVICE_TYPE, 0x90b, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define HAX_VCPU_IOCTL_INTERRUPT	CTL_CODE(HAX_DEVICE_TYPE, 0x90c, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define HAX_VCPU_SET_REGS			CTL_CODE(HAX_DEVICE_TYPE, 0x90d, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define HAX_VCPU_GET_REGS			CTL_CODE(HAX_DEVICE_TYPE, 0x90e, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define HAX_VCPU_IOCTL_KICKOFF		CTL_CODE(HAX_DEVICE_TYPE, 0x90f, METHOD_BUFFERED, FILE_ANY_ACCESS)

/* API version 2.0 */
#define HAX_VM_IOCTL_NOTIFY_QEMU_VERSION \
        CTL_CODE(HAX_DEVICE_TYPE, 0x910, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define HAX_IOCTL_VCPU_DEBUG \
        CTL_CODE(HAX_DEVICE_TYPE, 0x916, METHOD_BUFFERED, FILE_ANY_ACCESS)


// hax_capabilityinfo wstatus�p
#define HAX_CAP_STATUS_WORKING     (1 << 0)
#define HAX_CAP_MEMQUOTA           (1 << 1)
#define HAX_CAP_WORKSTATUS_MASK    0x01

// hax_capabilityinfo winfo�p�iHAXM�g�p�s�ȏꍇ�j
#define HAX_CAP_FAILREASON_VT      (1 << 0)
#define HAX_CAP_FAILREASON_NX      (1 << 1)

// hax_capabilityinfo winfo�p�iHAXM�g�p�\�ȏꍇ�j
#define HAX_CAP_EPT                (1 << 0)
#define HAX_CAP_FASTMMIO           (1 << 1)
#define HAX_CAP_UG                 (1 << 2)
#define HAX_CAP_64BIT_RAMBLOCK     (1 << 3)
#define HAX_CAP_64BIT_SETRAM       (1 << 4)
#define HAX_CAP_TUNNEL_PAGE        (1 << 5)
#define HAX_CAP_DEBUG              (1 << 7)

#define HAX_MAX_VCPUS 16

#define HAX_RAM_INFO_ROM     0x01
#define HAX_RAM_INFO_INVALID 0x80

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32)
#pragma pack(1)
typedef struct {
    UINT32 compat_version; // �݊����̂���o�[�W�����H
    UINT32 current_version; // ���݂̃o�[�W�����H
} HAX_MODULE_VERSION;

typedef struct hax_capabilityinfo {
    UINT16 wstatus; // �g�p�ۂ̃t���O�Ȃ�
    UINT16 winfo; // wstatus�Ŏg�p�̏ꍇ�FHAXM�Ŏg�p�ł���@�\�̃t���O�A�g�p�s�̏ꍇ�F�g�p�s�̗��R�t���O
    UINT32 win_refcount; // ��
    UINT64 mem_quota; // ���z�}�V��(VM)�Ŏg�p�\�ȃ������̏���H
} HAX_CAPINFO;

typedef struct hax_set_memlimit {
    UINT8 enable_memlimit; // �����������L���H
    UINT8 pad[7]; // ����
    UINT64 memory_limit; // �����������T�C�Y�H
} HAX_SET_MEMLIMIT;

typedef struct hax_alloc_ram_info {
    UINT32 size; // �o�^���郁�����̃T�C�Y
    UINT32 pad; // ����
    UINT64 va; // �o�^�������z�X�g�̃������A�h���X�B4KB���E�łȂ��ƑʖځBva��Virtual Address�̗��H
} HAX_ALLOC_RAM_INFO;

typedef struct hax_ramblock_info {
    UINT64 start_va; // �o�^�������z�X�g�̃������A�h���X�B4KB���E�łȂ��ƑʖځBva��Virtual Address�̗��H
    UINT64 size; // �o�^���郁�����̃T�C�Y
    UINT32 reserved; // ����
} HAX_RAMBLOCK_INFO;

typedef struct hax_set_ram_info {
    UINT64 pa_start; // ������̃Q�X�g�����A�h���X(Guest Physical Address; GPA)�B���ɓo�^����Ă���ꍇ�͏㏑���B4KB���E�łȂ��Ƒʖ�
    UINT32 size; // ��������T�C�Y
    UINT8 flags; // 0:�ʏ��RAM�AHAX_RAM_INFO_ROM:ROM�̈�AHAX_RAM_INFO_INVALID:���蓖�ĉ����iva��0�ɂ���j
    UINT8 pad[3]; // ����
    UINT64 va; // �����������z�X�g�̃������A�h���X(Host Virtual Address; HVA)�B���i386haxfunc_allocRAM�œo�^���Ă����Ȃ��ƑʖځB4KB���E�łȂ��Ƒʖ�
} HAX_SET_RAM_INFO;

typedef struct hax_tunnel_info {
    UINT64 va; // struct hax_tunnel�̃A�h���X�i�L���X�g���Ďg���j
    UINT64 io_va; // �f�[�^���Ƃ�p�Bexit_status�ɂ���ē��e���ς��
    UINT16 size; // struct hax_tunnel�̃T�C�Y
    UINT16 pad[3]; // ����
} HAX_TUNNEL_INFO;

typedef struct hax_qemu_version {
    UINT32 cur_version; // ���s����QEMU�o�[�W�����H
    UINT32 least_version; // �Œ���K�v��QEMU API�o�[�W�����H
} HAX_QEMU_VERSION;

#define HAX_IO_OUT 0
#define HAX_IO_IN  1

// VM���s���f�̌���
enum exit_status {
    HAX_EXIT_IO = 1, // I/O�|�[�g�A�N�Z�X
    HAX_EXIT_MMIO, // �������}�b�v�hIO�A�N�Z�X�i�ŋ߂�FAST_MMIO���g���Ă���B�p�~�H�j
    HAX_EXIT_REALMODE, // ���A�����[�h�̏ꍇ
    HAX_EXIT_INTERRUPT, // ���荞�ݔ����H
    HAX_EXIT_UNKNOWN, // �����s��
    HAX_EXIT_HLT, // HLT���߂����s���ꂽ
    HAX_EXIT_STATECHANGE, // �V���b�g�_�E����CPU�p�j�b�N�ȂǂŃ��Z�b�g�ȊO�̑I�����������Ƃ�
    HAX_EXIT_PAUSED, // �ꎞ��~�H
    HAX_EXIT_FAST_MMIO, // �������}�b�v�hIO�A�N�Z�X�iHAX_EXIT_MMIO�������ɂ������̂炵���jtunnel��io_va��hax_fastmmio�\���̂̃A�h���X
    HAX_EXIT_PAGEFAULT, // �y�[�W�t�H�[���g�����H
    HAX_EXIT_DEBUG // �f�o�b�O
};
enum run_flag {
    HAX_EXIT = 0,
    HAX_RESUME = 1
};

typedef struct hax_tunnel {
	UINT32 _exit_reason; // ???
    UINT32 pad0; // ����
    UINT32 _exit_status; // HAX_EXIT_�Ȃ񂽂�
    UINT32 user_event_pending; // ???
    int ready_for_interrupt_injection; // i386haxfunc_vcpu_interrupt�Ŋ��荞�݂��󂯓���鏀�����o����
    int request_interrupt_window; // ���荞�݂������̂ŏ������ĉ������t���O�i�K�v�ȂƂ��Ɏ�����1�ɐݒ肷��j

    union {
        struct {
			// tunnel.io_va�̓|�[�g�ւ̏o�̓f�[�^�A���邢�̓|�[�g����̓��̓f�[�^�̏������ݐ惁�����A�h���X
            UINT8 _direction; // ���͂��o�͂��̔���BHAX_IO_OUT�܂���HAX_IO_IN
            UINT8 _df; // _count��2�ȏ�̏ꍇ�A��������ǂݎ�鏇�ԁB0�̓C���N�������g�A1�̓f�N�������g�iI/O�|�[�g�ԍ��͂��̂܂܁j
            UINT16 _size; // �|�[�g�A�N�Z�X���̃T�C�Y�i�o�C�g���j
            UINT16 _port; // �o�͐�|�[�g
            UINT16 _count; // �A���o�͂���f�[�^��
            /* Followed owned by HAXM, QEMU should not touch them */
            /* bit 1 is 1 means string io */
            UINT8 _flags; // ???
            UINT8 _pad0; // ����
            UINT16 _pad1; // ����
            UINT32 _pad2; // ����
            UINT64 _vaddr; // �z�X�g���z�A�h���X�H
        } io;
        struct {
            UINT64 gla; // �Q�X�g���j�A�A�h���X�̗��H�����g���Ă��Ȃ��悤�Ȃ̂ŕ�����Ȃ�
        } mmio;
        struct {
            UINT64 gpa; // �Q�X�g�����A�h���X�H
#define HAX_PAGEFAULT_ACC_R  (1 << 0)
#define HAX_PAGEFAULT_ACC_W  (1 << 1)
#define HAX_PAGEFAULT_ACC_X  (1 << 2)
#define HAX_PAGEFAULT_PERM_R (1 << 4)
#define HAX_PAGEFAULT_PERM_W (1 << 5)
#define HAX_PAGEFAULT_PERM_X (1 << 6)
            UINT32 flags;
            UINT32 reserved1;
            UINT64 reserved2;
        } pagefault;
        struct {
            UINT64 dummy;
        } state;
        struct {
            UINT64 rip;
            UINT64 dr6;
            UINT64 dr7;
        } debug;
    };
	UINT64 apic_base;
} HAX_TUNNEL;
typedef struct hax_fastmmio {
    UINT64 gpa; // �Q�X�g�����A�h���X
    union {
        UINT64 value; // �������ݎ��͏o�͂���l�A�ǂݎ�莞�͒l�̊i�[��ϐ�
        UINT64 gpa2;  /* since API v4 */
    };
    UINT8 size; // �f�[�^�o�C�g���i1, 2, 4 byte�̂����ꂩ�j
    UINT8 direction; // READ=0�AWRITE=1�AREAD&WRITE=2 (I/O�|�[�g�̎��Ɖ��̂��t�Ȃ̂Œ��ӁB�Q�l: I/O�|�[�g�ł�READ=1, WRITE=0)
    UINT16 reg_index;  /* obsolete */
    UINT32 pad0; // ����
    UINT64 _cr0; // CR0���W�X�^�H
    UINT64 _cr2; // CR2���W�X�^�H
    UINT64 _cr3; // CR3���W�X�^�H
    UINT64 _cr4; // CR4���W�X�^�H
} HAX_FASTMMIO;


#define HAX_MAX_MSR_ARRAY 0x20
typedef struct vmx_msr {
    UINT64 entry;
    UINT64 value;
} VMX_MSR;
typedef struct hax_msr_data {
    UINT16 nr_msr;
    UINT16 done;
    UINT16 pad[2];
	VMX_MSR entries[HAX_MAX_MSR_ARRAY];
} HAX_MSR_DATA;
#pragma pack()

#pragma pack(16)
typedef struct fx_layout {
    UINT16  fcw;
    UINT16  fsw;
    UINT8   ftw;
    UINT8   res1;
    UINT16  fop;
    union {
        struct {
            UINT32  fip;
            UINT16  fcs;
            UINT16  res2;
        };
        UINT64  fpu_ip;
    };
    union {
        struct {
            UINT32  fdp;
            UINT16  fds;
            UINT16  res3;
        };
        UINT64  fpu_dp;
    };
    UINT32    mxcsr;
    UINT32    mxcsr_mask;
    UINT8     st_mm[8][16];
    UINT8     mmx_1[8][16];
    UINT8     mmx_2[8][16];
    UINT8     pad[96];
} HAX_FX_LAYOUT;
#pragma pack()

#pragma pack(1)

typedef union interruptibility_state_t {
    UINT32 raw;
    struct {
        UINT32 sti_blocking   : 1;
        UINT32 movss_blocking : 1;
        UINT32 smi_blocking   : 1;
        UINT32 nmi_blocking   : 1;
        UINT32 reserved       : 28;
    };
    UINT64 pad;
} INTERRUPTIBILITY_STATE;

// Segment descriptor
typedef struct segment_desc_t {
    UINT16 selector;
    UINT16 _dummy;
    UINT32 limit;
    UINT64 base;
    union {
        struct {
            UINT32 type             : 4;
            UINT32 desc             : 1;
            UINT32 dpl              : 2;
            UINT32 present          : 1;
            UINT32                  : 4;
            UINT32 available        : 1;
            UINT32 long_mode        : 1;
            UINT32 operand_size     : 1;
            UINT32 granularity      : 1;
            UINT32 null             : 1;
            UINT32                  : 15;
        };
        UINT32 ar;
    };
    UINT32 ipad;
} SEGMENT_DESC;

// CPU���W�X�^
typedef struct vcpu_state_t {
    union {
        UINT64 _regs[16];
        struct {
            union {
                struct {
                    UINT8 _al,
                            _ah;
                };
                UINT16    _ax;
                UINT32    _eax;
                UINT64    _rax;
            };
            union {
                struct {
                    UINT8 _cl,
                            _ch;
                };
                UINT16    _cx;
                UINT32    _ecx;
                UINT64    _rcx;
            };
            union {
                struct {
                    UINT8 _dl,
                            _dh;
                };
                UINT16    _dx;
                UINT32    _edx;
                UINT64    _rdx;
            };
            union {
                struct {
                    UINT8 _bl,
                            _bh;
                };
                UINT16    _bx;
                UINT32    _ebx;
                UINT64    _rbx;
            };
            union {
                UINT16    _sp;
                UINT32    _esp;
                UINT64    _rsp;
            };
            union {
                UINT16    _bp;
                UINT32    _ebp;
                UINT64    _rbp;
            };
            union {
                UINT16    _si;
                UINT32    _esi;
                UINT64    _rsi;
            };
            union {
                UINT16    _di;
                UINT32    _edi;
                UINT64    _rdi;
            };

            UINT64 _r8;
            UINT64 _r9;
            UINT64 _r10;
            UINT64 _r11;
            UINT64 _r12;
            UINT64 _r13;
            UINT64 _r14;
            UINT64 _r15;
        };
    };

    union {
        UINT32 _eip;
        UINT64 _rip;
    };

    union {
        UINT32 _eflags;
        UINT64 _rflags;
    };

    SEGMENT_DESC _cs;
    SEGMENT_DESC _ss;
    SEGMENT_DESC _ds;
    SEGMENT_DESC _es;
    SEGMENT_DESC _fs;
    SEGMENT_DESC _gs;
    SEGMENT_DESC _ldt;
    SEGMENT_DESC _tr;

    SEGMENT_DESC _gdt;
    SEGMENT_DESC _idt;

    UINT64 _cr0;
    UINT64 _cr2;
    UINT64 _cr3;
    UINT64 _cr4;

    UINT64 _dr0;
    UINT64 _dr1;
    UINT64 _dr2;
    UINT64 _dr3;
    UINT64 _dr6;
    UINT64 _dr7;
    UINT64 _pde;

    UINT32 _efer;

    UINT32 _sysenter_cs;
    UINT64 _sysenter_eip;
    UINT64 _sysenter_esp;

    UINT32 _activity_state;
    UINT32 pad;
    INTERRUPTIBILITY_STATE _interruptibility_state;
} HAX_VCPU_STATE;

// No access (R/W/X) is allowed
#define HAX_RAM_PERM_NONE 0x0
// All accesses (R/W/X) are allowed
#define HAX_RAM_PERM_RWX  0x7
#define HAX_RAM_PERM_MASK 0x7
typedef struct hax_protect_ram_info {
    UINT64 pa_start;
    UINT64 size;
    UINT32 flags;
    UINT32 reserved;
} HAX_PROTECT_RAM_INFO;

#define HAX_DEBUG_ENABLE     (1 << 0)
#define HAX_DEBUG_STEP       (1 << 1)
#define HAX_DEBUG_USE_SW_BP  (1 << 2)
#define HAX_DEBUG_USE_HW_BP  (1 << 3)

typedef struct hax_debug_t {
    UINT32 control;
    UINT32 reserved;
    UINT64 dr[8];
} HAX_DEBUG;

#pragma pack()
#endif

#ifdef __cplusplus
}
#endif

/***
  HAXM IOCTL�p�֐��Q
 ***/

// HAXM IOCTL
UINT8 i386haxfunc_getversion(HAX_MODULE_VERSION *version); // HAXM�̃o�[�W�������擾����
UINT8 i386haxfunc_getcapability(HAX_CAPINFO *cap); // HAXM�Ŏg�p�\�ȋ@�\���擾����
UINT8 i386haxfunc_setmemlimit(HAX_SET_MEMLIMIT *memlimit); // HAXM�̃������e�ʐ������Z�b�g����i�ŋ߂̔łł͈Ӗ��Ȃ��H�j
UINT8 i386haxfunc_createVM(UINT32 *vm_id); // HAXM�̉��z�}�V�����쐬���ĉ��z�}�V��ID��Ԃ�

// HAXM VM IOCTL
UINT8 i386haxfunc_notifyQEMUversion(HAX_QEMU_VERSION *hax_qemu_ver); // QEMU�o�[�W�����̒ʒm�iFast MMIO�̋������ς��H�ʒm���Ȃ��Ă����͂Ȃ������j
UINT8 i386haxfunc_VCPUcreate(UINT32 vcpu_id); // HAXM���z�}�V���Ɏw�肵��ID�̉��zCPU���쐬����iCPU ID��0�`15�Ŏw��j
UINT8 i386haxfunc_allocRAM(HAX_ALLOC_RAM_INFO *allocraminfo); // HAXM���z�}�V���Ŏg�p����z�X�g�̃������͈͂�o�^
UINT8 i386haxfunc_addRAMblock(HAX_RAMBLOCK_INFO *ramblkinfo); // HAXM���z�}�V���Ŏg�p����z�X�g�̃������͈͂�o�^
UINT8 i386haxfunc_setRAM(HAX_SET_RAM_INFO *setraminfo); // HAXM���z�}�V���̕����A�h���X�Ƀz�X�g�̃����������蓖�āi�������͎��O��i386haxfunc_allocRAM�œo�^���K�v�j�B�����蓖�ẴA�h���X��MMIO�����ɂȂ�B

// HAXM VCPU IOCTL
UINT8 i386haxfunc_vcpu_run(); // ���z�}�V�����s�J�n�BI/O�|�[�g�A�N�Z�X�E���荞�ݔ������Ŏ��s�p���ł��Ȃ��Ȃ�������s���f����
UINT8 i386haxfunc_vcpu_setup_tunnel(HAX_TUNNEL_INFO *info); // ���z�}�V���ƃf�[�^�����Ƃ肷�邽�߂̃������̈�(tunnel)���m��
UINT8 i386haxfunc_vcpu_setMSRs(HAX_MSR_DATA *inbuf, HAX_MSR_DATA *outbuf); // ���f���ŗL���W�X�^�iModel Specific Register; MSR�j��ݒ�
UINT8 i386haxfunc_vcpu_getMSRs(HAX_MSR_DATA *outbuf); // ���f���ŗL���W�X�^���擾
UINT8 i386haxfunc_vcpu_setFPU(HAX_FX_LAYOUT *inbuf); // FPU���W�X�^��ݒ�
UINT8 i386haxfunc_vcpu_getFPU(HAX_FX_LAYOUT *outbuf); // FPU���W�X�^���擾
UINT8 i386haxfunc_vcpu_setREGs(HAX_VCPU_STATE *inbuf); // ���W�X�^��ݒ�
UINT8 i386haxfunc_vcpu_getREGs(HAX_VCPU_STATE *outbuf); // ���W�X�^���擾
UINT8 i386haxfunc_vcpu_interrupt(UINT32 vector); // �w�肵���x�N�^�̊��荞�݂𔭐�������
UINT8 i386haxfunc_vcpu_kickoff(); // ��
UINT8 i386haxfunc_vcpu_debug(HAX_DEBUG *inbuf); // �f�o�b�O�̗L��/������f�o�b�O���@��ݒ肷��

#endif

#endif	/* !NP2_I386HAX_HAXFUNC_H__ */
