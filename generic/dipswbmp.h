
#ifdef __cplusplus
extern "C"{
#endif


// ���ꂼ�� 4bit BMP���Ԃ� (������������s�Ȃ�����)

UINT8 *dipswbmp_get9861(const UINT8 *s, const UINT8 *j);

UINT8 *dipswbmp_getsnd26(UINT8 cfg);
UINT8 *dipswbmp_getsnd86(UINT8 cfg);
UINT8 *dipswbmp_getsndspb(UINT8 cfg, UINT8 vrc);
UINT8 *dipswbmp_getmpu(UINT8 cfg);
UINT8 *dipswbmp_getsmpu(UINT8 cfg);
UINT8 *dipswbmp_getsnd118(UINT16 snd118io, UINT8 snd118dma, UINT8 snd118irqf, UINT8 snd118irqp, UINT8 snd118irqm, UINT8 snd118rom);

#ifdef __cplusplus
}
#endif

