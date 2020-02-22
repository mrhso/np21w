/**
 * @file	d_screen.cpp
 * @brief	�X�N���[���ݒ�_�C�A���O
 */

#include "compiler.h"
#include "resource.h"
#include "dialog.h"
#include "c_combodata.h"
#include "c_slidervalue.h"
#include "np2class.h"
#include "np2.h"
#include "scrnmng.h"
#ifdef SUPPORT_SCRN_DIRECT3D
#include "scrnmng_d3d.h"
#endif
#include "sysmng.h"
#include "misc\PropProc.h"
#include "pccore.h"
#include "iocore.h"
#include "common\strres.h"
#include "vram\scrndraw.h"
#include "vram\palettes.h"

static int resetScreen = 0;

/**
 * @brief Video �y�[�W
 */
class ScrOptVideoPage : public CPropPageProc
{
public:
	ScrOptVideoPage();
	virtual ~ScrOptVideoPage();

protected:
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	virtual LRESULT WindowProc(UINT nMsg, WPARAM wParam, LPARAM lParam);

private:
	CSliderValue m_skiplight;		//!< �X�L�b�v���C��
};

/**
 * �R���X�g���N�^
 */
ScrOptVideoPage::ScrOptVideoPage()
	: CPropPageProc(IDD_SCROPT1)
{
}

/**
 * �f�X�g���N�^
 */
ScrOptVideoPage::~ScrOptVideoPage()
{
}

/**
 * ���̃��\�b�h�� WM_INITDIALOG �̃��b�Z�[�W�ɉ������ČĂяo����܂�
 * @retval TRUE �ŏ��̃R���g���[���ɓ��̓t�H�[�J�X��ݒ�
 * @retval FALSE ���ɐݒ��
 */
BOOL ScrOptVideoPage::OnInitDialog()
{
	CheckDlgButton(IDC_LCD, (np2cfg.LCD_MODE & 1) ? BST_CHECKED : BST_UNCHECKED);
	GetDlgItem(IDC_LCDX).EnableWindow((np2cfg.LCD_MODE & 1) ? TRUE : FALSE);
	CheckDlgButton(IDC_LCDX, (np2cfg.LCD_MODE & 2) ? BST_CHECKED : BST_UNCHECKED);

	CheckDlgButton(IDC_SKIPLINE, (np2cfg.skipline) ? BST_CHECKED : BST_UNCHECKED);
	
	CheckDlgButton(IDC_SOFTWARERENDERING, (np2oscfg.emuddraw) ? BST_CHECKED : BST_UNCHECKED);

	m_skiplight.SubclassDlgItem(IDC_SKIPLIGHT, this);
	m_skiplight.SetStaticId(IDC_LIGHTSTR);
	m_skiplight.SetRange(0, 255);
	m_skiplight.SetPos(np2cfg.skiplight);
	return TRUE;
}

/**
 * ���[�U�[�� OK �̃{�^�� (IDOK ID ���̃{�^��) ���N���b�N����ƌĂяo����܂�
 */
void ScrOptVideoPage::OnOK()
{
	bool bUpdated = false;

	const UINT8 cSkipLine = (IsDlgButtonChecked(IDC_SKIPLINE) != BST_UNCHECKED) ? 1 : 0;
	if (np2cfg.skipline != cSkipLine)
	{
		np2cfg.skipline = cSkipLine;
		bUpdated = true;
	}
	const UINT8 cSkipLight = static_cast<UINT8>(m_skiplight.GetPos());
	if (np2cfg.skiplight != cSkipLight)
	{
		np2cfg.skiplight = cSkipLight;
		bUpdated = true;
	}
	if (bUpdated)
	{
		::pal_makeskiptable();
	}

	UINT8 cMode = 0;
	if (IsDlgButtonChecked(IDC_LCD) != BST_UNCHECKED)
	{
		cMode |= 1;
	}
	if (IsDlgButtonChecked(IDC_LCDX) != BST_UNCHECKED)
	{
		cMode |= 2;
	}
	if (np2cfg.LCD_MODE != cMode)
	{
		np2cfg.LCD_MODE = cMode;
		pal_makelcdpal();
		bUpdated = true;
	}
	cMode = (IsDlgButtonChecked(IDC_SOFTWARERENDERING) != BST_UNCHECKED);
	if (!!cMode == !np2oscfg.emuddraw)
	{
		np2oscfg.emuddraw = cMode;
		scrnmng_destroy();
		scrnmng_create(g_scrnmode);
		bUpdated = true;
	}
	if (bUpdated)
	{
		::scrndraw_redraw();
		::sysmng_update(SYS_UPDATECFG);
	}
}

/**
 * ���[�U�[�����j���[�̍��ڂ�I�������Ƃ��ɁA�t���[�����[�N�ɂ���ČĂяo����܂�
 * @param[in] wParam �p�����^
 * @param[in] lParam �p�����^
 * @retval TRUE �A�v���P�[�V���������̃��b�Z�[�W����������
 */
BOOL ScrOptVideoPage::OnCommand(WPARAM wParam, LPARAM lParam)
{
	if (LOWORD(wParam) == IDC_LCD)
	{
		const BOOL bEnable = (IsDlgButtonChecked(IDC_LCD) != BST_UNCHECKED) ? TRUE : FALSE;
		GetDlgItem(IDC_LCDX).EnableWindow(bEnable);
		return TRUE;
	}
	return FALSE;
}

/**
 * CWndProc �I�u�W�F�N�g�� Windows �v���V�[�W�� (WindowProc) ���p�ӂ���Ă��܂�
 * @param[in] nMsg ��������� Windows ���b�Z�[�W���w�肵�܂�
 * @param[in] wParam ���b�Z�[�W�̏����Ŏg���t������񋟂��܂��B���̃p�����[�^�̒l�̓��b�Z�[�W�Ɉˑ����܂�
 * @param[in] lParam ���b�Z�[�W�̏����Ŏg���t������񋟂��܂��B���̃p�����[�^�̒l�̓��b�Z�[�W�Ɉˑ����܂�
 * @return ���b�Z�[�W�Ɉˑ�����l��Ԃ��܂�
 */
LRESULT ScrOptVideoPage::WindowProc(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	if (nMsg == WM_HSCROLL)
	{
		switch (::GetDlgCtrlID(reinterpret_cast<HWND>(lParam)))
		{
			case IDC_SKIPLIGHT:
				m_skiplight.UpdateValue();
				break;

			default:
				break;
		}
	}
	return CDlgProc::WindowProc(nMsg, wParam, lParam);
}



// ----
/**
 * @brief Chip �y�[�W
 */
class ScrOptChipPage : public CPropPageProc
{
public:
	ScrOptChipPage();

protected:
	virtual BOOL OnInitDialog();
	virtual void OnOK();
};

/**
 * �R���X�g���N�^
 */
ScrOptChipPage::ScrOptChipPage()
	: CPropPageProc(IDD_SCROPT2)
{
}

//! GDC �`�b�v
static const UINT s_gdcchip[4] = {IDC_GRCGNON, IDC_GRCG, IDC_GRCG2, IDC_EGC};

/**
 * ���̃��\�b�h�� WM_INITDIALOG �̃��b�Z�[�W�ɉ������ČĂяo����܂�
 * @retval TRUE �ŏ��̃R���g���[���ɓ��̓t�H�[�J�X��ݒ�
 * @retval FALSE ���ɐݒ��
 */
BOOL ScrOptChipPage::OnInitDialog()
{
	CheckDlgButton((np2cfg.uPD72020) ? IDC_GDC72020 : IDC_GDC7220, BST_CHECKED);
	CheckDlgButton(s_gdcchip[np2cfg.grcg & 3], BST_CHECKED);
	CheckDlgButton(IDC_PC980124, (np2cfg.color16) ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(IDC_PEGC, (np2cfg.usepegcplane) ? BST_CHECKED : BST_UNCHECKED);

#if defined(SUPPORT_PC9821)
	static const UINT s_disabled[] =
	{
		IDC_GCBOX,
			IDC_GRCGNON, IDC_GRCG, IDC_GRCG2, IDC_EGC,
		IDC_PC980124
	};
	for (UINT i = 0; i < _countof(s_disabled); i++)
	{
		GetDlgItem(s_disabled[i]).EnableWindow(FALSE);
	}
	CheckDlgButton(s_gdcchip[0], BST_UNCHECKED);
	CheckDlgButton(s_gdcchip[1], BST_UNCHECKED);
	CheckDlgButton(s_gdcchip[2], BST_UNCHECKED);
	CheckDlgButton(s_gdcchip[3], BST_CHECKED);
#else
	GetDlgItem(IDC_PEGC).EnableWindow(FALSE);
	CheckDlgButton(IDC_PEGC, BST_UNCHECKED);
#endif	// defined(SUPPORT_PC9821)

	return TRUE;
}

/**
 * ���[�U�[�� OK �̃{�^�� (IDOK ID ���̃{�^��) ���N���b�N����ƌĂяo����܂�
 */
void ScrOptChipPage::OnOK()
{
	bool bUpdated = false;

	const UINT8 cMode = (IsDlgButtonChecked(IDC_GDC72020) != BST_UNCHECKED) ? 1 : 0;
	if (np2cfg.uPD72020 != cMode)
	{
		np2cfg.uPD72020 = cMode;
		bUpdated = true;
		gdc_restorekacmode();
		gdcs.grphdisp |= GDCSCRN_ALLDRAW2;
	}

	for (UINT i = 0; i < _countof(s_gdcchip); i++)
	{
		if (IsDlgButtonChecked(s_gdcchip[i]) != BST_UNCHECKED)
		{
			const UINT8 cType = static_cast<UINT8>(i);
			if (np2cfg.grcg != cType)
			{
				np2cfg.grcg = cType;
				bUpdated = true;
				gdcs.grphdisp |= GDCSCRN_ALLDRAW2;
			}
			break;
		}
	}

	const UINT8 cColor16 = (IsDlgButtonChecked(IDC_PC980124) != BST_UNCHECKED) ? 1 : 0;
	if (np2cfg.color16 != cColor16)
	{
		np2cfg.color16 = cColor16;
		bUpdated = true;
	}
	
	const UINT8 cPEGC = (IsDlgButtonChecked(IDC_PEGC) != BST_UNCHECKED) ? 1 : 0;
	if (np2cfg.usepegcplane != cPEGC)
	{
		np2cfg.usepegcplane = cPEGC;
		bUpdated = true;
	}

	if (bUpdated)
	{
		::sysmng_update(SYS_UPDATECFG);
	}
}



// ----

/**
 * @brief Timing �y�[�W
 */
class ScrOptTimingPage : public CPropPageProc
{
public:
	ScrOptTimingPage();
	virtual ~ScrOptTimingPage();

protected:
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual LRESULT WindowProc(UINT nMsg, WPARAM wParam, LPARAM lParam);

private:
	CSliderValue m_tram;	//!< TRAM
	CSliderValue m_vram;	//!< VRAM
	CSliderValue m_grcg;	//!< GRCG
	CSliderValue m_raster;	//!< ���X�^
};

/**
 * �R���X�g���N�^
 */
ScrOptTimingPage::ScrOptTimingPage()
	: CPropPageProc(IDD_SCROPT3)
{
}

/**
 * �f�X�g���N�^
 */
ScrOptTimingPage::~ScrOptTimingPage()
{
}

/**
 * ���̃��\�b�h�� WM_INITDIALOG �̃��b�Z�[�W�ɉ������ČĂяo����܂�
 * @retval TRUE �ŏ��̃R���g���[���ɓ��̓t�H�[�J�X��ݒ�
 * @retval FALSE ���ɐݒ��
 */
BOOL ScrOptTimingPage::OnInitDialog()
{
	m_tram.SubclassDlgItem(IDC_TRAMWAIT, this);
	m_tram.SetStaticId(IDC_TRAMSTR);
	m_tram.SetRange(0, 32);
	m_tram.SetPos(np2cfg.wait[0]);

	m_vram.SubclassDlgItem(IDC_VRAMWAIT, this);
	m_vram.SetStaticId(IDC_VRAMSTR);
	m_vram.SetRange(0, 32);
	m_vram.SetPos(np2cfg.wait[2]);

	m_grcg.SubclassDlgItem(IDC_GRCGWAIT, this);
	m_grcg.SetStaticId(IDC_GRCGSTR);
	m_grcg.SetRange(0, 32);
	m_grcg.SetPos(np2cfg.wait[4]);

	m_raster.SubclassDlgItem(IDC_REALPAL, this);
	m_raster.SetStaticId(IDC_REALPALSTR);
	m_raster.SetRange(-32, 32);
	m_raster.SetPos(np2cfg.realpal - 32);

	return TRUE;
}

/**
 * ���[�U�[�� OK �̃{�^�� (IDOK ID ���̃{�^��) ���N���b�N����ƌĂяo����܂�
 */
void ScrOptTimingPage::OnOK()
{
	bool bUpdated = false;

	UINT8 sWait[6];
	ZeroMemory(sWait, sizeof(sWait));
	sWait[0] = static_cast<UINT8>(m_tram.GetPos());
	sWait[2] = static_cast<UINT8>(m_vram.GetPos());
	sWait[4] = static_cast<UINT8>(m_grcg.GetPos());

	sWait[1] = (sWait[0]) ? 1 : 0;
	sWait[3] = (sWait[2]) ? 1 : 0;
	sWait[5] = (sWait[4]) ? 1 : 0;

	for (UINT i = 0; i < 6; i++)
	{
		if (np2cfg.wait[i] != sWait[i])
		{
			np2cfg.wait[i] = sWait[i];
			bUpdated = true;
		}
	}

	const UINT8 cRealPal = static_cast<UINT8>(m_raster.GetPos() + 32);
	if (np2cfg.realpal != cRealPal)
	{
		np2cfg.realpal = cRealPal;
		bUpdated = true;
	}

	if (bUpdated)
	{
		::sysmng_update(SYS_UPDATECFG);
	}
}

/**
 * CWndProc �I�u�W�F�N�g�� Windows �v���V�[�W�� (WindowProc) ���p�ӂ���Ă��܂�
 * @param[in] nMsg ��������� Windows ���b�Z�[�W���w�肵�܂�
 * @param[in] wParam ���b�Z�[�W�̏����Ŏg���t������񋟂��܂��B���̃p�����[�^�̒l�̓��b�Z�[�W�Ɉˑ����܂�
 * @param[in] lParam ���b�Z�[�W�̏����Ŏg���t������񋟂��܂��B���̃p�����[�^�̒l�̓��b�Z�[�W�Ɉˑ����܂�
 * @return ���b�Z�[�W�Ɉˑ�����l��Ԃ��܂�
 */
LRESULT ScrOptTimingPage::WindowProc(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	if (nMsg == WM_HSCROLL)
	{
		switch (::GetDlgCtrlID(reinterpret_cast<HWND>(lParam)))
		{
			case IDC_TRAMWAIT:
				m_tram.UpdateValue();
				break;

			case IDC_VRAMWAIT:
				m_vram.UpdateValue();
				break;

			case IDC_GRCGWAIT:
				m_grcg.UpdateValue();
				break;

			case IDC_REALPAL:
				m_raster.UpdateValue();
				break;

			default:
				break;
		}
	}
	return CDlgProc::WindowProc(nMsg, wParam, lParam);
}



// ----

/**
 * @brief Fullscreen �y�[�W
 */
class ScrOptFullscreenPage : public CPropPageProc
{
public:
	ScrOptFullscreenPage();
	virtual ~ScrOptFullscreenPage();

protected:
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);

private:
	CComboData m_zoom;				//!< �Y�[��
};

//! �Y�[�� ���X�g
static const CComboData::Entry s_zoom[] =
{
	{MAKEINTRESOURCE(IDS_ZOOM_NONE),			FSCRNMOD_NORESIZE},
	{MAKEINTRESOURCE(IDS_ZOOM_FIXEDASPECT),		FSCRNMOD_ASPECTFIX8},
	{MAKEINTRESOURCE(IDS_ZOOM_ADJUSTASPECT),	FSCRNMOD_ASPECTFIX},
	{MAKEINTRESOURCE(IDS_ZOOM_FULL),			FSCRNMOD_LARGE},
	{MAKEINTRESOURCE(IDS_ZOOM_INTMULTIPLE),		FSCRNMOD_INTMULTIPLE},
};

/**
 * �R���X�g���N�^
 */
ScrOptFullscreenPage::ScrOptFullscreenPage()
	: CPropPageProc(IDD_SCROPT_FULLSCREEN)
{
}

/**
 * �f�X�g���N�^
 */
ScrOptFullscreenPage::~ScrOptFullscreenPage()
{
}

/**
 * ���̃��\�b�h�� WM_INITDIALOG �̃��b�Z�[�W�ɉ������ČĂяo����܂�
 * @retval TRUE �ŏ��̃R���g���[���ɓ��̓t�H�[�J�X��ݒ�
 * @retval FALSE ���ɐݒ��
 */
BOOL ScrOptFullscreenPage::OnInitDialog()
{
	const UINT8 c = FSCRNCFG_fscrnmod;
	CheckDlgButton(IDC_FULLSCREEN_SAMEBPP, (c & FSCRNMOD_SAMEBPP) ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(IDC_FULLSCREEN_SAMERES, (c & FSCRNMOD_SAMERES) ? BST_CHECKED : BST_UNCHECKED);

	m_zoom.SubclassDlgItem(IDC_FULLSCREEN_ZOOM, this);
	m_zoom.Add(s_zoom, _countof(s_zoom));
	m_zoom.SetCurItemData(c & FSCRNMOD_ASPECTMASK);
	m_zoom.EnableWindow((c & FSCRNMOD_SAMERES) ? TRUE : FALSE);

	return TRUE;
}

/**
 * ���[�U�[�� OK �̃{�^�� (IDOK ID ���̃{�^��) ���N���b�N����ƌĂяo����܂�
 */
void ScrOptFullscreenPage::OnOK()
{
	UINT8 c = 0;
	if (IsDlgButtonChecked(IDC_FULLSCREEN_SAMEBPP) != BST_UNCHECKED)
	{
		c |= FSCRNMOD_SAMEBPP;
	}
	if (IsDlgButtonChecked(IDC_FULLSCREEN_SAMERES) != BST_UNCHECKED)
	{
		c |= FSCRNMOD_SAMERES;
	}
	c |= m_zoom.GetCurItemData(FSCRNCFG_fscrnmod & FSCRNMOD_ASPECTMASK);
	if (FSCRNCFG_fscrnmod != c)
	{
		if(np2oscfg.fsrescfg){
			scrnrescfg.fscrnmod = c;
			scrnrescfg.hasfscfg = 1;
			scrnres_writeini();
		}else{
			np2oscfg.fscrnmod = c;
		}
		::sysmng_update(SYS_UPDATEOSCFG);
		resetScreen = 1;
	}
}

/**
 * ���[�U�[�����j���[�̍��ڂ�I�������Ƃ��ɁA�t���[�����[�N�ɂ���ČĂяo����܂�
 * @param[in] wParam �p�����^
 * @param[in] lParam �p�����^
 * @retval TRUE �A�v���P�[�V���������̃��b�Z�[�W����������
 */
BOOL ScrOptFullscreenPage::OnCommand(WPARAM wParam, LPARAM lParam)
{
	if (LOWORD(wParam) == IDC_FULLSCREEN_SAMERES)
	{
		m_zoom.EnableWindow((IsDlgButtonChecked(IDC_FULLSCREEN_SAMERES) != BST_UNCHECKED) ? TRUE : FALSE);
		return TRUE;
	}
	return FALSE;
}



#ifdef SUPPORT_SCRN_DIRECT3D
// ----

/**
 * @brief Renderer �y�[�W
 */
class ScrOptRendererPage : public CPropPageProc
{
public:
	ScrOptRendererPage();
	virtual ~ScrOptRendererPage();

protected:
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);

private:
	CComboData m_type;				//!< ���
	CComboData m_mode;				//!< ��ԃ��[�h
};

//! ��� ���X�g
static const CComboData::Entry s_renderer_type[] =
{
	{MAKEINTRESOURCE(IDS_RENDERER_DIRECTDRAW),	DRAWTYPE_DIRECTDRAW_HW},
	{MAKEINTRESOURCE(IDS_RENDERER_DIRECT3D),	DRAWTYPE_DIRECT3D},
};
//! ��ԃ��[�h ���X�g
static const CComboData::Entry s_renderer_mode[] =
{
	{MAKEINTRESOURCE(IDS_RENDERER_IMODE_NN),	D3D_IMODE_NEAREST_NEIGHBOR},
	{MAKEINTRESOURCE(IDS_RENDERER_IMODE_LINEAR),D3D_IMODE_BILINEAR},
	{MAKEINTRESOURCE(IDS_RENDERER_IMODE_PIXEL),	D3D_IMODE_PIXEL},
	{MAKEINTRESOURCE(IDS_RENDERER_IMODE_PIXEL2),D3D_IMODE_PIXEL2},
	{MAKEINTRESOURCE(IDS_RENDERER_IMODE_PIXEL3),D3D_IMODE_PIXEL3},
};

/**
 * �R���X�g���N�^
 */
ScrOptRendererPage::ScrOptRendererPage()
	: CPropPageProc(IDD_SCROPT_RENDERER)
{
}

/**
 * �f�X�g���N�^
 */
ScrOptRendererPage::~ScrOptRendererPage()
{
}

/**
 * ���̃��\�b�h�� WM_INITDIALOG �̃��b�Z�[�W�ɉ������ČĂяo����܂�
 * @retval TRUE �ŏ��̃R���g���[���ɓ��̓t�H�[�J�X��ݒ�
 * @retval FALSE ���ɐݒ��
 */
BOOL ScrOptRendererPage::OnInitDialog()
{
	m_type.SubclassDlgItem(IDC_RENDERER_TYPE, this);
	m_type.Add(s_renderer_type, _countof(s_renderer_type));
	m_type.SetCurItemData(np2oscfg.drawtype);
	
	m_mode.SubclassDlgItem(IDC_RENDERER_IMODE, this);
	m_mode.Add(s_renderer_mode, _countof(s_renderer_mode));
	m_mode.SetCurItemData(FSCRNCFG_d3d_imode);
	m_mode.EnableWindow(np2oscfg.drawtype==DRAWTYPE_DIRECT3D ? TRUE : FALSE);

	return TRUE;
}

/**
 * ���[�U�[�� OK �̃{�^�� (IDOK ID ���̃{�^��) ���N���b�N����ƌĂяo����܂�
 */
void ScrOptRendererPage::OnOK()
{
	UINT32 tmp;
	tmp = m_type.GetCurItemData(np2oscfg.drawtype);
	if (tmp != np2oscfg.drawtype)
	{
		if(tmp==DRAWTYPE_DIRECT3D){
			if(scrnmngD3D_check() != SUCCESS){
				MessageBox(g_hWndMain, _T("Failed to initialize Direct3D."), _T("Direct3D Error"), MB_OK|MB_ICONEXCLAMATION);
				tmp = DRAWTYPE_DIRECTDRAW_HW;
			}
		}
		np2oscfg.drawtype = tmp;
		::sysmng_update(SYS_UPDATEOSCFG);
		resetScreen = 1;
	}
	tmp = m_mode.GetCurItemData(FSCRNCFG_d3d_imode);
	if (tmp != FSCRNCFG_d3d_imode)
	{
		if(np2oscfg.fsrescfg){
			scrnrescfg.d3d_imode = tmp;
			scrnrescfg.hasfscfg = 1;
			scrnres_writeini();
		}else{
			np2oscfg.d3d_imode = tmp;
		}
		::sysmng_update(SYS_UPDATEOSCFG);
		resetScreen = 1;
	}
}

/**
 * ���[�U�[�����j���[�̍��ڂ�I�������Ƃ��ɁA�t���[�����[�N�ɂ���ČĂяo����܂�
 * @param[in] wParam �p�����^
 * @param[in] lParam �p�����^
 * @retval TRUE �A�v���P�[�V���������̃��b�Z�[�W����������
 */
BOOL ScrOptRendererPage::OnCommand(WPARAM wParam, LPARAM lParam)
{
	if (LOWORD(wParam) == IDC_RENDERER_TYPE)
	{
		m_mode.EnableWindow(m_type.GetCurItemData(np2oscfg.drawtype)==DRAWTYPE_DIRECT3D ? TRUE : FALSE);
		return TRUE;
	}
	return FALSE;
}
#endif



// ----

/**
 * �X�N���[���ݒ�
 * @param[in] hwndParent �e�E�B���h�E
 */
void dialog_scropt(HWND hwndParent)
{
	CPropSheetProc prop(IDS_SCREENOPTION, hwndParent);

	ScrOptVideoPage video;
	prop.AddPage(&video);

	ScrOptChipPage chip;
	prop.AddPage(&chip);

	ScrOptTimingPage timing;
	prop.AddPage(&timing);

	ScrOptFullscreenPage fullscreen;
	prop.AddPage(&fullscreen);
	
#ifdef SUPPORT_SCRN_DIRECT3D
	ScrOptRendererPage renderer;
	prop.AddPage(&renderer);
#endif

	prop.m_psh.dwFlags |= PSH_NOAPPLYNOW | PSH_USEHICON | PSH_USECALLBACK;
	prop.m_psh.hIcon = LoadIcon(CWndProc::GetResourceHandle(), MAKEINTRESOURCE(IDI_ICON2));
	prop.m_psh.pfnCallback = np2class_propetysheet;
	prop.DoModal();

	::InvalidateRect(hwndParent, NULL, TRUE);
	
	// �f�o�C�X�č쐬
	if(resetScreen){
		scrnmng.forcereset = 1;
		resetScreen = 0;
	}
}
