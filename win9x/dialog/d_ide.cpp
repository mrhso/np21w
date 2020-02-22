/**
 * @file	d_ide.cpp
 * @brief	IDE �ݒ�_�C�A���O
 */

#include "compiler.h"
#include "resource.h"
#include "dialog.h"
#include "c_combodata.h"
#include "np2.h"
#include "commng.h"
#include "sysmng.h"
#include "misc/DlgProc.h"
#include "pccore.h"
#include "common/strres.h"

#ifdef SUPPORT_IDEIO

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef __cplusplus
}
#endif


/**
 * @brief IDE �ݒ�_�C�A���O
 * @param[in] hwndParent �e�E�B���h�E
 */
class CIdeDlg : public CDlgProc
{
public:
	CIdeDlg(HWND hwndParent);

protected:
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	virtual LRESULT WindowProc(UINT nMsg, WPARAM wParam, LPARAM lParam);

private:
	CComboData m_cmbpm;			//!< �v���C�}�� �}�X�^
	CComboData m_cmbps;			//!< �v���C�}�� �X���[�u
	CComboData m_cmbsm;			//!< �Z�J���_�� �}�X�^
	CComboData m_cmbss;			//!< �Z�J���_�� �X���[�u
	CWndProc m_chkasynccd;		//!< Use Async CD-ROM Access
	CWndProc m_chkuseecc;		//!< Use CD-ROM EDC/ECC Emulation
	CWndProc m_chkallowtraycmd;	//!< Allow CD Tray Open/Close Command
	CWndProc m_chkidebios;		//!< Use IDE BIOS
	CWndProc m_chkautoidebios;	//!< Auto IDE BIOS
	CWndProc m_nudrwait;		//!< ���荞�݁i�������݁j�f�B���C
	CWndProc m_nudwwait;		//!< ���荞�݁i�������݁j�f�B���C
};

/**
 * ���荞�݃��X�g
 */
static const CComboData::Entry s_type[] =
{
	{MAKEINTRESOURCE(IDS_IDETYPE_NONE ),		0},
	{MAKEINTRESOURCE(IDS_IDETYPE_HDD  ),		1},
	{MAKEINTRESOURCE(IDS_IDETYPE_CDROM),		2},
};

/**
 * �R���X�g���N�^
 * @param[in] hwndParent �e�E�B���h�E
 */
CIdeDlg::CIdeDlg(HWND hwndParent)
	: CDlgProc(IDD_IDE, hwndParent)
{
}

/**
 * ���̃��\�b�h�� WM_INITDIALOG �̃��b�Z�[�W�ɉ������ČĂяo����܂�
 * @retval TRUE �ŏ��̃R���g���[���ɓ��̓t�H�[�J�X��ݒ�
 * @retval FALSE ���ɐݒ��
 */
BOOL CIdeDlg::OnInitDialog()
{
	TCHAR numbuf[31];
	m_cmbpm.SubclassDlgItem(IDC_IDE1TYPE, this);
	m_cmbpm.Add(s_type, _countof(s_type));
	m_cmbpm.SetCurItemData(np2cfg.idetype[0]);
	
	m_cmbps.SubclassDlgItem(IDC_IDE2TYPE, this);
	m_cmbps.Add(s_type, _countof(s_type));
	m_cmbps.SetCurItemData(np2cfg.idetype[1]);

	m_cmbsm.SubclassDlgItem(IDC_IDE3TYPE, this);
	m_cmbsm.Add(s_type, _countof(s_type));
	m_cmbsm.SetCurItemData(np2cfg.idetype[2]);

	m_cmbss.SubclassDlgItem(IDC_IDE4TYPE, this);
	m_cmbss.Add(s_type, _countof(s_type));
	m_cmbss.SetCurItemData(np2cfg.idetype[3]);

	
	m_chkasynccd.SubclassDlgItem(IDC_USEASYNCCD, this);
	if(np2cfg.useasynccd){
		m_chkasynccd.SendMessage(BM_SETCHECK , BST_CHECKED , 0);
	}else{
		m_chkasynccd.SendMessage(BM_SETCHECK , BST_UNCHECKED , 0);
	}
	
	m_chkuseecc.SubclassDlgItem(IDC_USECDECC, this);
	if(np2cfg.usecdecc){
		m_chkuseecc.SendMessage(BM_SETCHECK , BST_CHECKED , 0);
	}else{
		m_chkuseecc.SendMessage(BM_SETCHECK , BST_UNCHECKED , 0);
	}
	
	m_chkallowtraycmd.SubclassDlgItem(IDC_ALLOWCDTRAYOP, this);
	if(np2cfg.allowcdtraycmd){
		m_chkallowtraycmd.SendMessage(BM_SETCHECK , BST_CHECKED , 0);
	}else{
		m_chkallowtraycmd.SendMessage(BM_SETCHECK , BST_UNCHECKED , 0);
	}
	
	m_nudrwait.SubclassDlgItem(IDC_IDERWAIT, this);
	_stprintf(numbuf, _T("%d"), np2cfg.iderwait);
	m_nudrwait.SetWindowTextW(numbuf);

	m_nudwwait.SubclassDlgItem(IDC_IDEWWAIT, this);
	_stprintf(numbuf, _T("%d"), np2cfg.idewwait);
	m_nudwwait.SetWindowTextW(numbuf);
	
	m_chkautoidebios.SubclassDlgItem(IDC_AUTOIDEBIOS, this);
	if(np2cfg.autoidebios){
		m_chkautoidebios.SendMessage(BM_SETCHECK , BST_CHECKED , 0);
	}else{
		m_chkautoidebios.SendMessage(BM_SETCHECK , BST_UNCHECKED , 0);
	}
	
	m_chkidebios.SubclassDlgItem(IDC_USEIDEBIOS, this);
	if(np2cfg.idebios){
		m_chkidebios.SendMessage(BM_SETCHECK , BST_CHECKED , 0);
		m_chkautoidebios.EnableWindow(TRUE);
	}else{
		m_chkidebios.SendMessage(BM_SETCHECK , BST_UNCHECKED , 0);
		m_chkautoidebios.EnableWindow(FALSE);
	}
	
	m_cmbpm.SetFocus();

	return FALSE;
}

/**
 * ���[�U�[�� OK �̃{�^�� (IDOK ID ���̃{�^��) ���N���b�N����ƌĂяo����܂�
 */
void CIdeDlg::OnOK()
{
	UINT update = 0;
	UINT32 valtmp;
	TCHAR numbuf[31];

	if (m_cmbpm.GetCurItemData(np2cfg.idetype[0])!=np2cfg.idetype[0])
	{
		np2cfg.idetype[0] = m_cmbpm.GetCurItemData(np2cfg.idetype[0]);
		update |= SYS_UPDATECFG | SYS_UPDATEHDD;
	}
	if (m_cmbps.GetCurItemData(np2cfg.idetype[1])!=np2cfg.idetype[1])
	{
		np2cfg.idetype[1] = m_cmbps.GetCurItemData(np2cfg.idetype[1]);
		update |= SYS_UPDATECFG | SYS_UPDATEHDD;
	}
	if (m_cmbsm.GetCurItemData(np2cfg.idetype[2])!=np2cfg.idetype[2])
	{
		np2cfg.idetype[2] = m_cmbsm.GetCurItemData(np2cfg.idetype[2]);
		update |= SYS_UPDATECFG | SYS_UPDATEHDD;
	}
	if (m_cmbss.GetCurItemData(np2cfg.idetype[3])!=np2cfg.idetype[3])
	{
		np2cfg.idetype[3] = m_cmbss.GetCurItemData(np2cfg.idetype[3]);
		update |= SYS_UPDATECFG | SYS_UPDATEHDD;
	}
	if (np2cfg.useasynccd != (m_chkasynccd.SendMessage(BM_GETCHECK , 0 , 0) ? 1 : 0))
	{
		np2cfg.useasynccd = (m_chkasynccd.SendMessage(BM_GETCHECK , 0 , 0) ? 1 : 0);
		update |= SYS_UPDATECFG;
	}
	if (np2cfg.usecdecc != (m_chkuseecc.SendMessage(BM_GETCHECK , 0 , 0) ? 1 : 0))
	{
		np2cfg.usecdecc = (m_chkuseecc.SendMessage(BM_GETCHECK , 0 , 0) ? 1 : 0);
		update |= SYS_UPDATECFG;
	}
	if (np2cfg.allowcdtraycmd != (m_chkallowtraycmd.SendMessage(BM_GETCHECK , 0 , 0) ? 1 : 0))
	{
		np2cfg.allowcdtraycmd = (m_chkallowtraycmd.SendMessage(BM_GETCHECK , 0 , 0) ? 1 : 0);
		update |= SYS_UPDATECFG;
	}
	m_nudrwait.GetWindowTextW(numbuf, 30);
	valtmp = _ttol(numbuf);
	if (valtmp < 0) valtmp = 0;
	if (valtmp > 100000000) valtmp = 100000000;
	if (valtmp != np2cfg.iderwait)
	{
		np2cfg.iderwait = valtmp;
		update |= SYS_UPDATECFG;
	}
	m_nudwwait.GetWindowTextW(numbuf, 30);
	valtmp = _ttol(numbuf);
	if (valtmp < 0) valtmp = 0;
	if (valtmp > 100000000) valtmp = 100000000;
	if (valtmp != np2cfg.idewwait)
	{
		np2cfg.idewwait = valtmp;
		update |= SYS_UPDATECFG;
	}
	if (np2cfg.idebios != (m_chkidebios.SendMessage(BM_GETCHECK , 0 , 0) ? 1 : 0))
	{
		np2cfg.idebios = (m_chkidebios.SendMessage(BM_GETCHECK , 0 , 0) ? 1 : 0);
		update |= SYS_UPDATECFG;
	}
	if (np2cfg.autoidebios != (m_chkautoidebios.SendMessage(BM_GETCHECK , 0 , 0) ? 1 : 0))
	{
		np2cfg.autoidebios = (m_chkautoidebios.SendMessage(BM_GETCHECK , 0 , 0) ? 1 : 0);
		update |= SYS_UPDATECFG;
	}
	
	sysmng_update(update);

	CDlgProc::OnOK();
}

/**
 * ���[�U�[�����j���[�̍��ڂ�I�������Ƃ��ɁA�t���[�����[�N�ɂ���ČĂяo����܂�
 * @param[in] wParam �p�����^
 * @param[in] lParam �p�����^
 * @retval TRUE �A�v���P�[�V���������̃��b�Z�[�W����������
 */
BOOL CIdeDlg::OnCommand(WPARAM wParam, LPARAM lParam)
{
	switch (LOWORD(wParam))
	{
		case IDC_IDE1TYPE:
		case IDC_IDE2TYPE:
		case IDC_IDE3TYPE:
		case IDC_IDE4TYPE:
			return TRUE;
		case IDC_USEIDEBIOS:
			m_chkautoidebios.EnableWindow((m_chkidebios.SendMessage(BM_GETCHECK , 0 , 0) ? TRUE : FALSE));
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
LRESULT CIdeDlg::WindowProc(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	LPNMUPDOWN lpnud;
	UINT32 nudnum;
	TCHAR numbuf[31];
	switch(wParam)
	{
		case IDC_SPINIDERWAIT:
			lpnud = (LPNMUPDOWN)lParam;
			if(lpnud->hdr.code == UDN_DELTAPOS)
			{
				m_nudrwait.GetWindowTextW(numbuf, 30);
				nudnum = _ttol(numbuf);
				if(lpnud->iDelta > 0)
				{
					if(nudnum <= 0) nudnum = 0;
					else nudnum--;
				}
				else if(lpnud->iDelta < 0)
				{
					if(nudnum >= 100000000) nudnum = 100000000;
					else nudnum++;
				}
				_stprintf(numbuf, _T("%d"), nudnum);
				m_nudrwait.SetWindowTextW(numbuf);
			}
			break;
		case IDC_SPINIDEWWAIT:
			lpnud = (LPNMUPDOWN)lParam;
			if(lpnud->hdr.code == UDN_DELTAPOS)
			{
				m_nudwwait.GetWindowTextW(numbuf, 30);
				nudnum = _ttol(numbuf);
				if(lpnud->iDelta > 0)
				{
					if(nudnum <= 0) nudnum = 0;
					else nudnum--;
				}
				else if(lpnud->iDelta < 0)
				{
					if(nudnum >= 100000000) nudnum = 100000000;
					else nudnum++;
				}
				_stprintf(numbuf, _T("%d"), nudnum);
				m_nudwwait.SetWindowTextW(numbuf);
			}
			break;
	}

	return CDlgProc::WindowProc(nMsg, wParam, lParam);
}

/**
 * �R���t�B�O �_�C�A���O
 * @param[in] hwndParent �e�E�B���h�E
 */
void dialog_ideopt(HWND hwndParent)
{
	CIdeDlg dlg(hwndParent);
	dlg.DoModal();
}

#endif
