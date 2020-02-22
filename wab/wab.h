/**
 * @file	wab.h
 * @brief	Window Accelerator Board Interface
 *
 * @author	$Author: SimK $
 */

#pragma once

// XXX: ��]�����Ă�1600x1600�ȏ�ɂȂ�Ȃ��̂ō����������Ă͂���ŏ\��
#define WAB_MAX_WIDTH	1600
#define WAB_MAX_HEIGHT	1600

#ifdef __cplusplus
extern "C" {
#endif
	
typedef struct {
	int		posx;
	int		posy;
	int		multiwindow;
	int		multithread;
	int		halftone;
	int		forcevga;
	int		readonly; // from np2oscfg
} NP2WABCFG;

typedef void NP2WAB_DrawFrame();
typedef struct {
	REG8 relay; // ��ʏo�̓����[��ԁibit0=�����E�B���h�E�A�N�Z�����[�^, bit1=RGB IN�X���[, ����ȊO�̃r�b�g��Reserved�Bbit0,1��00��98�O���t�B�b�N
	REG8 paletteChanged; // �p���b�g�v�X�V�t���O
	int realWidth; // ��ʉ𑜓x(��)
	int realHeight; // ��ʉ𑜓x(����)
	int wndWidth; // �`��̈�T�C�Y(��)
	int wndHeight; // �`��̈�T�C�Y(����)
	int fps; // ���t���b�V�����[�g�i��̍��킹�Ă���邩������Ȃ�������ǌ����_�ŉ������Ă��Ȃ��j
	int lastWidth; // �O��̃E�B���h�E�A�N�Z�����[�^�̉��𑜓x�i�f�o�C�X�č쐬����p�j
	int lastHeight; // �O��̃E�B���h�E�A�N�Z�����[�^�̉��𑜓x�i�f�o�C�X�č쐬����p�j
	
	int	relaystateint;
	int	relaystateext;

	int vramoffs;
} NP2WAB;

typedef struct {
	int multiwindow; // �ʑ����[�h
	int ready; // 0�ȊO�Ȃ�`���Ă��ǂ���
	HWND hWndMain; // ���C���E�B���h�E�̃n���h��
	HWND hWndWAB; // �E�B���h�E�A�N�Z�����[�^�ʑ��̃n���h��
	HDC hDCWAB; // �E�B���h�E�A�N�Z�����[�^�ʑ���HDC
	HBITMAP hBmpBuf; // �o�b�t�@�r�b�g�}�b�v�i��ɓ��{�j
	HDC     hDCBuf; // �o�b�t�@��HDC
	NP2WAB_DrawFrame *drawframe; // ��ʕ`��֐��BhDCBuf�ɃA�N�Z�����[�^��ʃf�[�^��]������B
} NP2WABWND;

void np2wab_init(HINSTANCE hInstance, HWND g_hWndMain);
void np2wab_reset(const NP2CFG *pConfig);
void np2wab_bind(void);
void np2wab_drawframe(void);
void np2wab_shutdown(void);

void np2wab_setRelayState(REG8 state);
void np2wab_setScreenSize(int width, int height);
void np2wab_setScreenSizeMT(int width, int height);

void wabwin_readini();
void wabwin_writeini();

extern NP2WAB		np2wab;
extern NP2WABCFG	np2wabcfg;
extern NP2WABWND	np2wabwnd;
//
//extern int		np2wab.relaystateint;
//extern int		np2wab.relaystateext;

#ifdef __cplusplus
}
#endif

