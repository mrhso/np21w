
enum {
	KB_CTR			= (1 << 3),
	KB_CTRMASK		= (KB_CTR - 1),

	KB_BUF			= (1 << 7),
	KB_BUFMASK		= (KB_BUF - 1)
};

typedef struct {
	UINT32	xferclock;
	UINT8	data;
	UINT8	cmd;
	UINT8	mode;
	UINT8	status;
	UINT	ctrls;
	UINT	ctrpos;
	UINT	buffers;
	UINT	bufpos;
	UINT8	ctr[KB_CTR];
	UINT8	buf[KB_BUF];
} _KEYBRD, *KEYBRD;

typedef struct {
	UINT8	result;
	UINT8	data;
	UINT8	send;
	UINT8	pad;
	UINT	pos;
	UINT	dummyinst;
	UINT	mul;
	UINT8	rawmode;
} _RS232C, *RS232C;

#if defined(SUPPORT_RS232C_FIFO)
typedef struct {
	UINT8 irqflag; // FIFO���[�h ���荞�݂̌���  0:�Ȃ�, 1:���M�\�ɂȂ���, 2:��M���ׂ��f�[�^������
	UINT8 port136;
	UINT8 port138;
	UINT8 vfast;
} _RS232CFIFO, *RS232CFIFO;
#endif



#ifdef __cplusplus
extern "C" {
#endif

void keyboard_callback(NEVENTITEM item);

void keyboard_reset(const NP2CFG *pConfig);
void keyboard_bind(void);
void keyboard_resetsignal(void);
void keyboard_ctrl(REG8 data);
void keyboard_send(REG8 data);
void keyboard_changeclock();



void rs232c_construct(void);
void rs232c_destruct(void);

void rs232c_reset(const NP2CFG *pConfig);
void rs232c_bind(void);
void rs232c_open(void);
void rs232c_callback(void);

UINT8 rs232c_stat(void);
void rs232c_midipanic(void);

#if defined(SUPPORT_RS232C_FIFO)
void rs232c_vfast_setrs232cspeed(UINT8 value);
#endif

#ifdef __cplusplus
}
#endif

