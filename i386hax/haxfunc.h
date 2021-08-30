
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

#define HAX_VCPU_IOCTL_SET_CPUID \
        CTL_CODE(HAX_DEVICE_TYPE, 0x917, METHOD_BUFFERED, FILE_ANY_ACCESS)


// hax_capabilityinfo wstatus用
#define HAX_CAP_STATUS_WORKING     (1 << 0)
#define HAX_CAP_MEMQUOTA           (1 << 1)
#define HAX_CAP_WORKSTATUS_MASK    0x01

// hax_capabilityinfo winfo用（HAXM使用不可な場合）
#define HAX_CAP_FAILREASON_VT      (1 << 0)
#define HAX_CAP_FAILREASON_NX      (1 << 1)

// hax_capabilityinfo winfo用（HAXM使用可能な場合）
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
    UINT32 compat_version; // 互換性のあるバージョン？
    UINT32 current_version; // 現在のバージョン？
} HAX_MODULE_VERSION;

typedef struct hax_capabilityinfo {
    UINT16 wstatus; // 使用可否のフラグなど
    UINT16 winfo; // wstatusで使用可の場合：HAXMで使用できる機能のフラグ、使用不可の場合：使用不可の理由フラグ
    UINT32 win_refcount; // 謎
    UINT64 mem_quota; // 仮想マシン(VM)で使用可能なメモリの上限？
} HAX_CAPINFO;

typedef struct hax_set_memlimit {
    UINT8 enable_memlimit; // メモリ制限有効？
    UINT8 pad[7]; // 無視
    UINT64 memory_limit; // メモリ制限サイズ？
} HAX_SET_MEMLIMIT;

typedef struct hax_alloc_ram_info {
    UINT32 size; // 登録するメモリのサイズ
    UINT32 pad; // 無視
    UINT64 va; // 登録したいホストのメモリアドレス。4KB境界でないと駄目。vaはVirtual Addressの略？
} HAX_ALLOC_RAM_INFO;

typedef struct hax_ramblock_info {
    UINT64 start_va; // 登録したいホストのメモリアドレス。4KB境界でないと駄目。vaはVirtual Addressの略？
    UINT64 size; // 登録するメモリのサイズ
    UINT32 reserved; // 無視
} HAX_RAMBLOCK_INFO;

typedef struct hax_set_ram_info {
    UINT64 pa_start; // 割当先のゲスト物理アドレス(Guest Physical Address; GPA)。既に登録されている場合は上書き。4KB境界でないと駄目
    UINT32 size; // 割当するサイズ
    UINT8 flags; // 0:通常のRAM、HAX_RAM_INFO_ROM:ROM領域、HAX_RAM_INFO_INVALID:割り当て解除（vaは0にする）
    UINT8 pad[3]; // 無視
    UINT64 va; // 割当したいホストのメモリアドレス(Host Virtual Address; HVA)。先にi386haxfunc_allocRAMで登録しておかないと駄目。4KB境界でないと駄目
} HAX_SET_RAM_INFO;

typedef struct hax_tunnel_info {
    UINT64 va; // struct hax_tunnelのアドレス（キャストして使う）
    UINT64 io_va; // データやりとり用。exit_statusによって内容が変わる
    UINT16 size; // struct hax_tunnelのサイズ
    UINT16 pad[3]; // 無視
} HAX_TUNNEL_INFO;

typedef struct hax_qemu_version {
    UINT32 cur_version; // 実行中のQEMUバージョン？
    UINT32 least_version; // 最低限必要なQEMU APIバージョン？
} HAX_QEMU_VERSION;

#define HAX_IO_OUT 0
#define HAX_IO_IN  1

// VM実行中断の原因
enum exit_status {
    HAX_EXIT_IO = 1, // I/Oポートアクセス
    HAX_EXIT_MMIO, // メモリマップドIOアクセス（最近はFAST_MMIOが使われている。廃止？）
    HAX_EXIT_REALMODE, // リアルモードの場合
    HAX_EXIT_INTERRUPT, // 割り込み発生？
    HAX_EXIT_UNKNOWN, // 原因不明
    HAX_EXIT_HLT, // HLT命令が実行された
    HAX_EXIT_STATECHANGE, // シャットダウンやCPUパニックなどでリセット以外の選択肢が無いとき
    HAX_EXIT_PAUSED, // 一時停止？
    HAX_EXIT_FAST_MMIO, // メモリマップドIOアクセス（HAX_EXIT_MMIOを高速にしたものらしい）tunnelのio_vaはhax_fastmmio構造体のアドレス
    HAX_EXIT_PAGEFAULT, // ページフォールト発生？
    HAX_EXIT_DEBUG // デバッグ
};
enum run_flag {
    HAX_EXIT = 0,
    HAX_RESUME = 1
};

typedef struct hax_tunnel {
	UINT32 _exit_reason; // ???
    UINT32 pad0; // 無視
    UINT32 _exit_status; // HAX_EXIT_なんたら
    UINT32 user_event_pending; // ???
    int ready_for_interrupt_injection; // i386haxfunc_vcpu_interruptで割り込みを受け入れる準備が出来た
    int request_interrupt_window; // 割り込みしたいので準備して下さいフラグ（必要なときに自分で1に設定する）

    union {
        struct {
			// tunnel.io_vaはポートへの出力データ、あるいはポートからの入力データの書き込み先メモリアドレス
            UINT8 _direction; // 入力か出力かの判定。HAX_IO_OUTまたはHAX_IO_IN
            UINT8 _df; // _countが2以上の場合、メモリを読み取る順番。0はインクリメント、1はデクリメント（I/Oポート番号はそのまま）
            UINT16 _size; // ポートアクセス時のサイズ（バイト数）
            UINT16 _port; // 出力先ポート
            UINT16 _count; // 連続出力するデータ数
            /* Followed owned by HAXM, QEMU should not touch them */
            /* bit 1 is 1 means string io */
            UINT8 _flags; // ???
            UINT8 _pad0; // 無視
            UINT16 _pad1; // 無視
            UINT32 _pad2; // 無視
            UINT64 _vaddr; // ホスト仮想アドレス？
        } io;
        struct {
            UINT64 gla; // ゲストリニアアドレスの略？もう使われていないようなので分からない
        } mmio;
        struct {
            UINT64 gpa; // ゲスト物理アドレス？
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
    UINT64 gpa; // ゲスト物理アドレス
    union {
        UINT64 value; // 書き込み時は出力する値、読み取り時は値の格納先変数
        UINT64 gpa2;  /* since API v4 */
    };
    UINT8 size; // データバイト数（1, 2, 4 byteのいずれか）
    UINT8 direction; // READ=0、WRITE=1、READ&WRITE=2 (I/Oポートの時と何故か逆なので注意。参考: I/OポートではREAD=1, WRITE=0)
    UINT16 reg_index;  /* obsolete */
    UINT32 pad0; // 無視
    UINT64 _cr0; // CR0レジスタ？
    UINT64 _cr2; // CR2レジスタ？
    UINT64 _cr3; // CR3レジスタ？
    UINT64 _cr4; // CR4レジスタ？
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

// CPUレジスタ
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

#define HAX_MAX_CPUID_ENTRIES 0x40

typedef struct hax_cpuid_entry_t {
    UINT32 function;
    UINT32 index;
    UINT32 flags;
    UINT32 eax;
    UINT32 ebx;
    UINT32 ecx;
    UINT32 edx;
    UINT32 pad[3];
} HAX_CPUID_ENTRY;

// `HAX_CPUID` is a variable-length type. The size of `HAX_CPUID` itself is only
// 8 bytes. `entries` is just a body placeholder, which will not actually occupy
// memory. The accessible memory of `entries` is decided by the allocation from
// user space, and the array length is specified by `total`.

typedef struct hax_cpuid_t {
    UINT32 total;
    UINT32 pad;
    HAX_CPUID_ENTRY entries[HAX_MAX_CPUID_ENTRIES];
} HAX_CPUID;

#pragma pack()
#endif

#ifdef __cplusplus
}
#endif

/***
  HAXM IOCTL用関数群
 ***/

// HAXM IOCTL
UINT8 i386haxfunc_getversion(HAX_MODULE_VERSION *version); // HAXMのバージョンを取得する
UINT8 i386haxfunc_getcapability(HAX_CAPINFO *cap); // HAXMで使用可能な機能を取得する
UINT8 i386haxfunc_setmemlimit(HAX_SET_MEMLIMIT *memlimit); // HAXMのメモリ容量制限をセットする（最近の版では意味なし？）
UINT8 i386haxfunc_createVM(UINT32 *vm_id); // HAXMの仮想マシンを作成して仮想マシンIDを返す

// HAXM VM IOCTL
UINT8 i386haxfunc_notifyQEMUversion(HAX_QEMU_VERSION *hax_qemu_ver); // QEMUバージョンの通知（Fast MMIOの挙動が変わる？通知しなくても問題はなさそう）
UINT8 i386haxfunc_VCPUcreate(UINT32 vcpu_id); // HAXM仮想マシンに指定したIDの仮想CPUを作成する（CPU IDは0～15で指定）
UINT8 i386haxfunc_allocRAM(HAX_ALLOC_RAM_INFO *allocraminfo); // HAXM仮想マシンで使用するホストのメモリ範囲を登録
UINT8 i386haxfunc_addRAMblock(HAX_RAMBLOCK_INFO *ramblkinfo); // HAXM仮想マシンで使用するホストのメモリ範囲を登録
UINT8 i386haxfunc_setRAM(HAX_SET_RAM_INFO *setraminfo); // HAXM仮想マシンの物理アドレスにホストのメモリを割り当て（メモリは事前にi386haxfunc_allocRAMで登録が必要）。未割り当てのアドレスはMMIO扱いになる。

// HAXM VCPU IOCTL
UINT8 i386haxfunc_vcpu_run(); // 仮想マシン実行開始。I/Oポートアクセス・割り込み発生等で実行継続できなくなったら実行中断する
UINT8 i386haxfunc_vcpu_setup_tunnel(HAX_TUNNEL_INFO *info); // 仮想マシンとデータをやりとりするためのメモリ領域(tunnel)を確保
UINT8 i386haxfunc_vcpu_setMSRs(HAX_MSR_DATA *inbuf, HAX_MSR_DATA *outbuf); // モデル固有レジスタ（Model Specific Register; MSR）を設定
UINT8 i386haxfunc_vcpu_getMSRs(HAX_MSR_DATA *outbuf); // モデル固有レジスタを取得
UINT8 i386haxfunc_vcpu_setFPU(HAX_FX_LAYOUT *inbuf); // FPUレジスタを設定
UINT8 i386haxfunc_vcpu_getFPU(HAX_FX_LAYOUT *outbuf); // FPUレジスタを取得
UINT8 i386haxfunc_vcpu_setREGs(HAX_VCPU_STATE *inbuf); // レジスタを設定
UINT8 i386haxfunc_vcpu_getREGs(HAX_VCPU_STATE *outbuf); // レジスタを取得
UINT8 i386haxfunc_vcpu_interrupt(UINT32 vector); // 指定したベクタの割り込みを発生させる
UINT8 i386haxfunc_vcpu_kickoff(); // 謎
UINT8 i386haxfunc_vcpu_debug(HAX_DEBUG *inbuf); // デバッグの有効/無効やデバッグ方法を設定する
UINT8 i386haxfunc_vcpu_setCPUID(HAX_CPUID *inbuf); // CPUIDを設定する

#endif

#endif	/* !NP2_I386HAX_HAXFUNC_H__ */
