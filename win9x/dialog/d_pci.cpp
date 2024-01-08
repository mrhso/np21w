/**
 * @file	d_pci.cpp
 * @brief	PCI �ݒ�_�C�A���O
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

#if defined(SUPPORT_PCI)

/**
 * @brief �E�B���h�E�A�N�Z�����[�^��{�ݒ�y�[�W
 * @param[in] hwndParent �e�E�B���h�E
 */
class CPCIPage : public CPropPageProc
{
public:
	CPCIPage();
	virtual ~CPCIPage();

protected:
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	virtual LRESULT WindowProc(UINT nMsg, WPARAM wParam, LPARAM lParam);

private:
	UINT8 m_enable;				//!< �L��
	UINT8 m_pcmctype;			//!< PCMC��ސݒ�
	UINT8 m_bios32;			//!< BIOS32�L��
	CWndProc m_chkenable;		//!< PCI ENABLE
	CComboData m_cmbpcmctype;	//!< PCMC TYPE
	CWndProc m_chkbios32;		//!< BIOS32 ENABLE
	void SetPCMCType(UINT16 cValue);
	UINT8 GetPCMCType() const;
};

/**
 * �R���X�g���N�^
 */
CPCIPage::CPCIPage()
	: CPropPageProc(IDD_PCI)
{
}
/**
 * �f�X�g���N�^
 */
CPCIPage::~CPCIPage()
{
}

/**
 * �@�탊�X�g
 */
static const CComboData::Entry s_type[] =
{
	{MAKEINTRESOURCE(IDS_PCI_PCMC_82434LX),		PCI_PCMC_82434LX},
	{MAKEINTRESOURCE(IDS_PCI_PCMC_82441FX),		PCI_PCMC_82441FX},
	{MAKEINTRESOURCE(IDS_PCI_PCMC_WILDCAT),		PCI_PCMC_WILDCAT},
};

/**
 * ���̃��\�b�h�� WM_INITDIALOG �̃��b�Z�[�W�ɉ������ČĂяo����܂�
 * @retval TRUE �ŏ��̃R���g���[���ɓ��̓t�H�[�J�X��ݒ�
 * @retval FALSE ���ɐݒ��
 */
BOOL CPCIPage::OnInitDialog()
{
	m_enable = np2cfg.usepci;
	m_pcmctype = np2cfg.pci_pcmc;
	m_bios32 = np2cfg.pci_bios32;

	m_chkenable.SubclassDlgItem(IDC_PCIENABLE, this);
	if(m_enable)
		m_chkenable.SendMessage(BM_SETCHECK , BST_CHECKED , 0);
	else
		m_chkenable.SendMessage(BM_SETCHECK , BST_UNCHECKED , 0);
	
	m_cmbpcmctype.SubclassDlgItem(IDC_PCIPCMCTYPE, this);
	m_cmbpcmctype.Add(s_type, _countof(s_type));
	SetPCMCType(m_pcmctype);
	
	m_chkbios32.SubclassDlgItem(IDC_PCIBIOS32, this);
	if(m_bios32)
		m_chkbios32.SendMessage(BM_SETCHECK , BST_CHECKED , 0);
	else
		m_chkbios32.SendMessage(BM_SETCHECK , BST_UNCHECKED , 0);

	m_chkenable.SetFocus();

	return FALSE;
}

/**
 * ���[�U�[�� OK �̃{�^�� (IDOK ID ���̃{�^��) ���N���b�N����ƌĂяo����܂�
 */
void CPCIPage::OnOK()
{
	UINT update = 0;
	
	if (np2cfg.usepci != m_enable
		|| np2cfg.pci_pcmc != m_pcmctype
		|| np2cfg.pci_bios32 != m_bios32)
	{
		np2cfg.usepci = m_enable;
		np2cfg.pci_pcmc = m_pcmctype;
		np2cfg.pci_bios32 = m_bios32;
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
BOOL CPCIPage::OnCommand(WPARAM wParam, LPARAM lParam)
{
	switch (LOWORD(wParam))
	{
		case IDC_PCIENABLE:
			m_enable = (m_chkenable.SendMessage(BM_GETCHECK , 0 , 0) ? 1 : 0);
			return TRUE;

		case IDC_PCIPCMCTYPE:
			m_pcmctype = GetPCMCType();
			return TRUE;
			
		case IDC_PCIBIOS32:
			m_bios32 = (m_chkbios32.SendMessage(BM_GETCHECK , 0 , 0) ? 1 : 0);
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
LRESULT CPCIPage::WindowProc(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	return CDlgProc::WindowProc(nMsg, wParam, lParam);
}

/**
 * �@���ݒ�
 * @param[in] cValue �ݒ�
 */
void CPCIPage::SetPCMCType(UINT16 cValue)
{
	m_cmbpcmctype.SetCurItemData(cValue);
}

/**
 * �@����擾
 * @return I/O
 */
UINT8 CPCIPage::GetPCMCType() const
{
	return m_cmbpcmctype.GetCurItemData(PCI_PCMC_82434LX);
}


/**
 * �R���t�B�O �_�C�A���O
 * @param[in] hwndParent �e�E�B���h�E
 */
void dialog_pciopt(HWND hwndParent)
{
	CPropSheetProc prop(IDS_PCIOPTION, hwndParent);
	
	CPCIPage pci;
	prop.AddPage(&pci);
	
	prop.m_psh.dwFlags |= PSH_NOAPPLYNOW | PSH_USEHICON | PSH_USECALLBACK;
	prop.m_psh.hIcon = LoadIcon(CWndProc::GetResourceHandle(), MAKEINTRESOURCE(IDI_ICON2));
	prop.m_psh.pfnCallback = np2class_propetysheet;
	prop.DoModal();

	InvalidateRect(hwndParent, NULL, TRUE);
}

#endif