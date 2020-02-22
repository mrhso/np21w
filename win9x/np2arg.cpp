/**
 *	@file	np2arg.cpp
 *	@brief	�������N���X�̓���̒�`���s���܂�
 */

#include "compiler.h"
#include "np2arg.h"
#include "dosio.h"

#define	MAXARG		32				//!< �ő�����G���g����
#define	ARG_BASE	1				//!< win32 �� lpszCmdLine �̏ꍇ�̊J�n�G���g��

//! �B��̃C���X�^���X�ł�
Np2Arg Np2Arg::sm_instance;

/**
 * �R���X�g���N�^
 */
Np2Arg::Np2Arg()
{
	ZeroMemory(this, sizeof(*this));
}

/**
 * �f�X�g���N�^
 */
Np2Arg::~Np2Arg()
{
	if(m_lpArg){
		free(m_lpArg);
		m_lpArg = NULL;
	}
	if(m_lpIniFile){
		free(m_lpIniFile); // np21w ver0.86 rev8
		m_lpIniFile = NULL;
	}
}

/**
 * �p�[�X
 */
void Np2Arg::Parse()
{
	LPCTSTR lpIniFile = NULL;

	// �����ǂݏo��
	if(m_lpArg) {
		free(m_lpArg);
	}
	m_lpArg = _tcsdup(::GetCommandLine());

	LPTSTR argv[MAXARG];
	const int argc = ::milstr_getarg(m_lpArg, argv, _countof(argv));

	int nDrive = 0;

	for (int i = ARG_BASE; i < argc; i++)
	{
		LPCTSTR lpArg = argv[i];
		if ((lpArg[0] == TEXT('/')) || (lpArg[0] == TEXT('-')))
		{
			switch (_totlower(lpArg[1]))
			{
				case 'f':
					m_fFullscreen = true;
					break;

				case 'i':
					lpIniFile = &lpArg[2];
					break;
			}
		}
		else
		{
			LPCTSTR lpExt = ::file_getext(lpArg);
			if (::file_cmpname(lpExt, TEXT("ini")) == 0 || ::file_cmpname(lpExt, TEXT("npc")) == 0 || ::file_cmpname(lpExt, TEXT("npcfg")) == 0 || ::file_cmpname(lpExt, TEXT("np2cfg")) == 0 || ::file_cmpname(lpExt, TEXT("np21cfg")) == 0 || ::file_cmpname(lpExt, TEXT("np21wcfg")) == 0)
			{
				lpIniFile = lpArg;
			}
			else if (nDrive < _countof(m_lpDisk))
			{
				m_lpDisk[nDrive++] = lpArg;
			}
		}
	}
	if(m_lpIniFile){ // np21w ver0.86 rev8
		LPTSTR strbuf;
		strbuf = (LPTSTR)calloc(500, sizeof(TCHAR));
		if(!(_tcsstr(lpIniFile,_T(":"))!=NULL || (lpIniFile[0]=='\\'))){
			// �t�@�C�����݂̂̎w����ۂ������猻�݂̃f�B���N�g��������
			//getcwd(pathname, 300);
			GetCurrentDirectory(500, strbuf);
			if(strbuf[_tcslen(strbuf)-1]!='\\'){
				_tcscat(strbuf, _T("\\")); // XXX: Linux�Ƃ���������X���b�V������Ȃ��Ƒʖڂ���ˁH -> Win��p��������Ȃ�
			}
		}
		_tcscat(strbuf, lpIniFile);
		m_lpIniFile = strbuf;
	}
}

/**
 * �f�B�X�N�����N���A
 */
void Np2Arg::ClearDisk()
{
	ZeroMemory(m_lpDisk, sizeof(m_lpDisk));
}
