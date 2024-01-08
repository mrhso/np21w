/**
 * @file	extrom.cpp
 * @brief	EXTROM ���\�[�X �N���X�̓���̒�`���s���܂�
 */

#include "compiler.h"
#include "ExtRom.h"
#include "np2.h"
#include "WndProc.h"
#include "dosio.h"
#include "pccore.h"


//! ���\�[�X��
static const TCHAR s_szExtRom[] = TEXT("EXTROM");

/**
 * �R���X�g���N�^
 */
CExtRom::CExtRom()
	: m_hGlobal(NULL)
	, m_lpRes(NULL)
	, m_nSize(0)
	, m_nPointer(0)
{
}

/**
 * �f�X�g���N�^
 */
CExtRom::~CExtRom()
{
	Close();
}

/**
 * �I�[�v��
 * @param[in] lpFilename �t�@�C����
 * @retval true ����
 * @retval false ���s
 */
bool CExtRom::Open(LPCTSTR lpFilename)
{
	Close();

	HINSTANCE hInstance = CWndProc::FindResourceHandle(lpFilename, s_szExtRom);
	HRSRC hRsrc = ::FindResource(hInstance, lpFilename, s_szExtRom);
	if (hRsrc == NULL)
	{
		return false;
	}

	m_hGlobal = ::LoadResource(hInstance, hRsrc);
	m_lpRes = ::LockResource(m_hGlobal);
	m_nSize = ::SizeofResource(hInstance, hRsrc);
	m_nPointer = 0;
	return true;
}

/**
 * �I�[�v��
 * @param[in] lpFilename �t�@�C����
 * @param[in] extlen �t�@�C�����̌�뉽�������g���q�������邩
 * @retval true ����
 * @retval false ���s
 */
bool CExtRom::Open(LPCTSTR lpFilename, DWORD extlen)
{
	TCHAR tmpfilesname[MAX_PATH];
	TCHAR tmpfilesname2[MAX_PATH];
	FILEH	fh;

	Close();
	
	_tcscpy(tmpfilesname, lpFilename);
	int fnamelen = (int)_tcslen(tmpfilesname);
	for (int i=0; i<(int)extlen+1; i++)
	{
		tmpfilesname[fnamelen+1-i] = tmpfilesname[fnamelen+1-i-1];
	}
	tmpfilesname[fnamelen-extlen] = '.';
	getbiospath(tmpfilesname2, tmpfilesname, NELEMENTS(tmpfilesname2));
	fh = file_open_rb(tmpfilesname2);
	if (fh != FILEH_INVALID)
	{
		m_nSize = (UINT)file_getsize(fh);
		m_lpRes = malloc(m_nSize);
		file_read(fh, m_lpRes, m_nSize);
		m_hGlobal = NULL;
		m_nPointer = 0;
		m_isfile = 1;
		file_close(fh);
	}
	else
	{
		HINSTANCE hInstance = CWndProc::FindResourceHandle(lpFilename, s_szExtRom);
		HRSRC hRsrc = ::FindResource(hInstance, lpFilename, s_szExtRom);
		if (hRsrc == NULL)
		{
			return false;
		}

		m_hGlobal = ::LoadResource(hInstance, hRsrc);
		m_lpRes = ::LockResource(m_hGlobal);
		m_nSize = ::SizeofResource(hInstance, hRsrc);
		m_nPointer = 0;
	}

	return true;
}

/**
 * �I�[�v��
 * @param[in] lpFilename �t�@�C����
 * @param[in] lpExt �O���t�@�C���̏ꍇ�̊g���q
 * @retval true ����
 * @retval false ���s
 */
bool CExtRom::Open(LPCTSTR lpFilename, LPCTSTR lpExt)
{
	TCHAR tmpfilesname[MAX_PATH];
	TCHAR tmpfilesname2[MAX_PATH];
	FILEH	fh;

	Close();
	
	_tcscpy(tmpfilesname, lpFilename);
	_tcscat(tmpfilesname, lpExt);
	getbiospath(tmpfilesname2, tmpfilesname, NELEMENTS(tmpfilesname2));
	fh = file_open_rb(tmpfilesname2);
	if (fh != FILEH_INVALID)
	{
		m_nSize = (UINT)file_getsize(fh);
		m_lpRes = malloc(m_nSize);
		file_read(fh, m_lpRes, m_nSize);
		m_hGlobal = NULL;
		m_nPointer = 0;
		m_isfile = 1;
		file_close(fh);
	}
	else
	{
		HINSTANCE hInstance = CWndProc::FindResourceHandle(lpFilename, s_szExtRom);
		HRSRC hRsrc = ::FindResource(hInstance, lpFilename, s_szExtRom);
		if (hRsrc == NULL)
		{
			return false;
		}

		m_hGlobal = ::LoadResource(hInstance, hRsrc);
		m_lpRes = ::LockResource(m_hGlobal);
		m_nSize = ::SizeofResource(hInstance, hRsrc);
		m_nPointer = 0;
	}

	return true;
}

/**
 * �N���[�Y
 */
void CExtRom::Close()
{
	if (m_isfile)
	{
		if (m_lpRes)
		{
			free(m_lpRes);
		}
		m_isfile = 0;
	}
	else
	{
		if (m_hGlobal)
		{
			::FreeResource(m_hGlobal);
			m_hGlobal = NULL;
		}
	}
	m_lpRes = NULL;
	m_nSize = 0;
	m_nPointer = 0;
}

/**
 * �ǂݍ���
 * @param[out] lpBuffer �o�b�t�@
 * @param[out] cbBuffer �o�b�t�@��
 * @return �T�C�Y
 */
UINT CExtRom::Read(LPVOID lpBuffer, UINT cbBuffer)
{
	UINT nLength = m_nSize - m_nPointer;
	nLength = min(nLength, cbBuffer);
	if (nLength)
	{
		if (lpBuffer)
		{
			CopyMemory(lpBuffer, static_cast<char*>(m_lpRes) + m_nPointer, nLength);
		}
		m_nPointer += nLength;
	}
	return nLength;
}

/**
 * �V�[�N
 * @param[in] lDistanceToMove �t�@�C���|�C���^�̈ړ��o�C�g��
 * @param[in] dwMoveMethod �t�@�C���|�C���^���ړ����邽�߂̊J�n�_�i��_�j���w�肵�܂�
 * @return ���݂̈ʒu
 */
LONG CExtRom::Seek(LONG lDistanceToMove, DWORD dwMoveMethod)
{
	switch (dwMoveMethod)
	{
		case FILE_BEGIN:
		default:
			break;

		case FILE_CURRENT:
			lDistanceToMove += m_nPointer;
			break;

		case FILE_END:
			lDistanceToMove += m_nSize;
			break;
	}

	if (lDistanceToMove < 0)
	{
		lDistanceToMove = 0;
	}
	else if (static_cast<UINT>(lDistanceToMove) > m_nSize)
	{
		lDistanceToMove = m_nSize;
	}
	m_nPointer = lDistanceToMove;
	return lDistanceToMove;
}
