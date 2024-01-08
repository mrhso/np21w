/**
 * @file	d_wab.cpp
 * @brief	Window Accelerator Board configure dialog procedure
 *
 * @author	$Author: SimK $
 * @date	$Date: 2016/03/11 $
 */

#include "compiler.h"
#include "resource.h"
#include "strres.h"
#include "dialog.h"
#include "c_combodata.h"
#include "np2class.h"
#include "dosio.h"
#include "joymng.h"
#include "np2.h"
#include "sysmng.h"
#include "misc\PropProc.h"
#include "pccore.h"
#include "iocore.h"
#include "wab/wab.h"
#include "wab/cirrus_vga_extern.h"

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef __cplusplus
}
#endif

#if defined(SUPPORT_WAB)

/**
 * @brief �E�B���h�E�A�N�Z�����[�^��{�ݒ�y�[�W
 * @param[in] hwndParent �e�E�B���h�E
 */
class CWABPage : public CPropPageProc
{
public:
	CWABPage();
	virtual ~CWABPage();

protected:
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	virtual LRESULT WindowProc(UINT nMsg, WPARAM wParam, LPARAM lParam);

private:
	UINT8 m_awitch;				//!< �A�i���O�X�C�b�`���[�h
	UINT8 m_multiwindow;		//!< �ʑ����[�h
	UINT8 m_multithread;		//!< �}���`�X���b�h���[�h
	CWndProc m_chkawitch;		//!< ANALOG SWITCH IC
	CWndProc m_chkmultiwindow;	//!< MULTI WINDOW
	CWndProc m_chkmultithread;	//!< MULTI THREAD
};

/**
 * �R���X�g���N�^
 */
CWABPage::CWABPage()
	: CPropPageProc(IDD_WAB)
{
}
/**
 * �f�X�g���N�^
 */
CWABPage::~CWABPage()
{
}

/**
 * ���̃��\�b�h�� WM_INITDIALOG �̃��b�Z�[�W�ɉ������ČĂяo����܂�
 * @retval TRUE �ŏ��̃R���g���[���ɓ��̓t�H�[�J�X��ݒ�
 * @retval FALSE ���ɐݒ��
 */
BOOL CWABPage::OnInitDialog()
{
	m_awitch = np2cfg.wabasw;
	m_multiwindow = np2wabcfg.multiwindow;
	m_multithread = np2wabcfg.multithread;

	m_chkawitch.SubclassDlgItem(IDC_WABASWITCH, this);
	if(m_awitch)
		m_chkawitch.SendMessage(BM_SETCHECK , BST_CHECKED , 0);
	else
		m_chkawitch.SendMessage(BM_SETCHECK , BST_UNCHECKED , 0);

	m_chkmultiwindow.SubclassDlgItem(IDC_WABMULTIWIN, this);
	if(m_multiwindow)
		m_chkmultiwindow.SendMessage(BM_SETCHECK , BST_CHECKED , 0);
	else
		m_chkmultiwindow.SendMessage(BM_SETCHECK , BST_UNCHECKED , 0);

	m_chkmultithread.SubclassDlgItem(IDC_WABMULTHREAD, this);
	if(m_multithread)
		m_chkmultithread.SendMessage(BM_SETCHECK , BST_CHECKED , 0);
	else
		m_chkmultithread.SendMessage(BM_SETCHECK , BST_UNCHECKED , 0);
	

	m_chkawitch.SetFocus();

	return FALSE;
}

/**
 * ���[�U�[�� OK �̃{�^�� (IDOK ID ���̃{�^��) ���N���b�N����ƌĂяo����܂�
 */
void CWABPage::OnOK()
{
	UINT update = 0;

	if (np2cfg.wabasw != m_awitch || np2wabcfg.multiwindow != m_multiwindow || np2wabcfg.multithread != m_multithread)
	{
		np2cfg.wabasw = m_awitch;
		np2wabcfg.multiwindow = m_multiwindow;
		np2wabcfg.multithread = m_multithread;
		update |= SYS_UPDATECFG;
	}
	::sysmng_update(update);
}

/**
 * ���[�U�[�����j���[�̍��ڂ�I�������Ƃ��ɁA�t���[�����[�N�ɂ���ČĂяo����܂�
 * @param[in] wParam �p�����^
 * @param[in] lParam �p�����^
 * @retval TRUE �A�v���P�[�V���������̃��b�Z�[�W����������
 */
BOOL CWABPage::OnCommand(WPARAM wParam, LPARAM lParam)
{
	switch (LOWORD(wParam))
	{
		case IDC_WABASWITCH:
			m_awitch = (m_chkawitch.SendMessage(BM_GETCHECK , 0 , 0) ? 1 : 0);
			return TRUE;
			
		case IDC_WABMULTIWIN:
			m_multiwindow = (m_chkmultiwindow.SendMessage(BM_GETCHECK , 0 , 0) ? 1 : 0);
			return TRUE;
			
		case IDC_WABMULTHREAD:
			m_multithread = (m_chkmultithread.SendMessage(BM_GETCHECK , 0 , 0) ? 1 : 0);
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
LRESULT CWABPage::WindowProc(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	return CDlgProc::WindowProc(nMsg, wParam, lParam);
}


#if defined(SUPPORT_CL_GD5430)

/**
 * @brief CL-GD5430 �ݒ�y�[�W
 * @param[in] hwndParent �e�E�B���h�E
 */
class CGD5430Page : public CPropPageProc
{
public:
	CGD5430Page();
	virtual ~CGD5430Page();

protected:
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	virtual LRESULT WindowProc(UINT nMsg, WPARAM wParam, LPARAM lParam);

private:
	UINT8 m_enabled;			//!< �L���t���O
	UINT16 m_type;				//!< �@��ݒ�
	UINT8 m_fakecur;			//!< ���n�[�h�E�F�A�J�[�\��
	CWndProc m_chkenabled;		//!< ENABLED
	CComboData m_cmbtype;		//!< �@��
	CWndProc m_chkfakecur;		//!< FAKE HARDWARE CURSOR
	CWndProc m_btnreset;		//!< RESET
	void SetWABType(UINT16 cValue);
	UINT16 GetWABType() const;
};

/**
 * �R���X�g���N�^
 */
CGD5430Page::CGD5430Page()
	: CPropPageProc(IDD_GD5430)
{
}
/**
 * �f�X�g���N�^
 */
CGD5430Page::~CGD5430Page()
{
}

/**
 * �@�탊�X�g
 */
static const CComboData::Entry s_type[] =
{
	{MAKEINTRESOURCE(IDS_GD5430_BE),		CIRRUS_98ID_Be},
	{MAKEINTRESOURCE(IDS_GD5430_XE),		CIRRUS_98ID_Xe},
	{MAKEINTRESOURCE(IDS_GD5430_CB),		CIRRUS_98ID_Cb},
	{MAKEINTRESOURCE(IDS_GD5430_CF),		CIRRUS_98ID_Cf},
	{MAKEINTRESOURCE(IDS_GD5430_XE10),		CIRRUS_98ID_Xe10},
	{MAKEINTRESOURCE(IDS_GD5430_CB2),		CIRRUS_98ID_Cb2},
	{MAKEINTRESOURCE(IDS_GD5430_CX2),		CIRRUS_98ID_Cx2},
#ifdef SUPPORT_PCI
	{MAKEINTRESOURCE(IDS_GD5430_PCI),		CIRRUS_98ID_PCI},
#endif
	{MAKEINTRESOURCE(IDS_GD5430_WAB),		CIRRUS_98ID_WAB},
	{MAKEINTRESOURCE(IDS_GD5430_WSN_A2F),	CIRRUS_98ID_WSN_A2F},
	{MAKEINTRESOURCE(IDS_GD5430_WSN),		CIRRUS_98ID_WSN},
	{MAKEINTRESOURCE(IDS_GD5430_GA98NBIC),	CIRRUS_98ID_GA98NBIC},
	{MAKEINTRESOURCE(IDS_GD5430_GA98NBII),	CIRRUS_98ID_GA98NBII},
	{MAKEINTRESOURCE(IDS_GD5430_GA98NBIV),	CIRRUS_98ID_GA98NBIV},
	{MAKEINTRESOURCE(IDS_GD5430_96),		CIRRUS_98ID_96},
	{MAKEINTRESOURCE(IDS_GD5430_AUTO_XE_G1_PCI),		CIRRUS_98ID_AUTO_XE_G1_PCI},
	{MAKEINTRESOURCE(IDS_GD5430_AUTO_XE_G2_PCI),		CIRRUS_98ID_AUTO_XE_G2_PCI},
	{MAKEINTRESOURCE(IDS_GD5430_AUTO_XE_G4_PCI),		CIRRUS_98ID_AUTO_XE_G4_PCI},
#ifdef SUPPORT_PCI
	{MAKEINTRESOURCE(IDS_GD5430_AUTO_XE_WA_PCI),		CIRRUS_98ID_AUTO_XE_WA_PCI},
	{MAKEINTRESOURCE(IDS_GD5430_AUTO_XE_WS_PCI),		CIRRUS_98ID_AUTO_XE_WS_PCI},
	{MAKEINTRESOURCE(IDS_GD5430_AUTO_XE_W4_PCI),		CIRRUS_98ID_AUTO_XE_W4_PCI},
#endif
	{MAKEINTRESOURCE(IDS_GD5430_AUTO_XE10_WABS),		CIRRUS_98ID_AUTO_XE10_WABS},
	{MAKEINTRESOURCE(IDS_GD5430_AUTO_XE10_WSN2),		CIRRUS_98ID_AUTO_XE10_WSN2},
	{MAKEINTRESOURCE(IDS_GD5430_AUTO_XE10_WSN4),		CIRRUS_98ID_AUTO_XE10_WSN4},
};

/**
 * ���̃��\�b�h�� WM_INITDIALOG �̃��b�Z�[�W�ɉ������ČĂяo����܂�
 * @retval TRUE �ŏ��̃R���g���[���ɓ��̓t�H�[�J�X��ݒ�
 * @retval FALSE ���ɐݒ��
 */
BOOL CGD5430Page::OnInitDialog()
{
	m_enabled = np2cfg.usegd5430;
	m_type = np2cfg.gd5430type;
	m_fakecur = np2cfg.gd5430fakecur;

	m_chkenabled.SubclassDlgItem(IDC_GD5430ENABLED, this);
	if(m_enabled)
		m_chkenabled.SendMessage(BM_SETCHECK , BST_CHECKED , 0);
	else
		m_chkenabled.SendMessage(BM_SETCHECK , BST_UNCHECKED , 0);
	
	m_cmbtype.SubclassDlgItem(IDC_GD5430TYPE, this);
	m_cmbtype.Add(s_type, _countof(s_type));
	SetWABType(m_type);
	
	m_chkfakecur.SubclassDlgItem(IDC_GD5430FAKECURSOR, this);
	if(m_fakecur)
		m_chkfakecur.SendMessage(BM_SETCHECK , BST_CHECKED , 0);
	else
		m_chkfakecur.SendMessage(BM_SETCHECK , BST_UNCHECKED , 0);
	
	m_cmbtype.SetFocus();

	return FALSE;
}

/**
 * ���[�U�[�� OK �̃{�^�� (IDOK ID ���̃{�^��) ���N���b�N����ƌĂяo����܂�
 */
void CGD5430Page::OnOK()
{
	UINT update = 0;

	if (np2cfg.usegd5430 != m_enabled
		|| np2cfg.gd5430type != m_type
		|| np2cfg.gd5430fakecur != m_fakecur)
	{
		np2cfg.usegd5430 = m_enabled;
		np2cfg.gd5430type = m_type;
		np2cfg.gd5430fakecur = m_fakecur;
		update |= SYS_UPDATECFG;
	}
	::sysmng_update(update);
}

/**
 * ���[�U�[�����j���[�̍��ڂ�I�������Ƃ��ɁA�t���[�����[�N�ɂ���ČĂяo����܂�
 * @param[in] wParam �p�����^
 * @param[in] lParam �p�����^
 * @retval TRUE �A�v���P�[�V���������̃��b�Z�[�W����������
 */
BOOL CGD5430Page::OnCommand(WPARAM wParam, LPARAM lParam)
{
	switch (LOWORD(wParam))
	{
		case IDC_GD5430ENABLED:
			m_enabled = (m_chkenabled.SendMessage(BM_GETCHECK , 0 , 0) ? 1 : 0);
			return TRUE;

		case IDC_GD5430TYPE:
			m_type = GetWABType();
			return TRUE;
			
		case IDC_GD5430FAKECURSOR:
			m_fakecur = (m_chkfakecur.SendMessage(BM_GETCHECK , 0 , 0) ? 1 : 0);
			return TRUE;

		case IDC_GD5430DEF:
			m_type = CIRRUS_98ID_Xe10;
			SetWABType(m_type);
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
LRESULT CGD5430Page::WindowProc(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	//switch (nMsg)
	//{
	//}
	return CDlgProc::WindowProc(nMsg, wParam, lParam);
}

/**
 * �@���ݒ�
 * @param[in] cValue �ݒ�
 */
void CGD5430Page::SetWABType(UINT16 cValue)
{
	m_cmbtype.SetCurItemData(cValue);
}

/**
 * �@����擾
 * @return I/O
 */
UINT16 CGD5430Page::GetWABType() const
{
	return m_cmbtype.GetCurItemData(CIRRUS_98ID_Xe10);
}

#endif


/**
 * �R���t�B�O �_�C�A���O
 * @param[in] hwndParent �e�E�B���h�E
 */
void dialog_wabopt(HWND hwndParent)
{
	CPropSheetProc prop(IDS_WABOPTION, hwndParent);
	
	CWABPage wab;
	prop.AddPage(&wab);
	
#if defined(SUPPORT_CL_GD5430)
	CGD5430Page gd5430;
	prop.AddPage(&gd5430);
#endif

	prop.m_psh.dwFlags |= PSH_NOAPPLYNOW | PSH_USEHICON | PSH_USECALLBACK;
	prop.m_psh.hIcon = LoadIcon(CWndProc::GetResourceHandle(), MAKEINTRESOURCE(IDI_ICON2));
	prop.m_psh.pfnCallback = np2class_propetysheet;
	prop.DoModal();

	InvalidateRect(hwndParent, NULL, TRUE);
}

#endif