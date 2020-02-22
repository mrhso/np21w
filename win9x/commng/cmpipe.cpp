/**
 * @file	cmpipe.cpp
 * @brief	���O�t���p�C�v �V���A�� �N���X�̓���̒�`���s���܂�
 */

#include "compiler.h"
#include "cmpipe.h"

#if defined(SUPPORT_NAMED_PIPE)

/**
 * �C���X�^���X�쐬
 * @param[in] nPort �|�[�g�ԍ�
 * @param[in] pipename �p�C�v�̖��O
 * @param[in] servername �T�[�o�[�̖��O
 * @return �C���X�^���X
 */
CComPipe* CComPipe::CreateInstance(LPCTSTR pipename, LPCTSTR servername)
{
	CComPipe* pSerial = new CComPipe;
	if (!pSerial->Initialize(pipename, servername))
	{
		delete pSerial;
		pSerial = NULL;
	}
	return pSerial;
}

/**
 * �R���X�g���N�^
 */
CComPipe::CComPipe()
	: CComBase(COMCONNECT_SERIAL)
	, m_hSerial(INVALID_HANDLE_VALUE)
	, m_isserver(false)
{
}

/**
 * �f�X�g���N�^
 */
CComPipe::~CComPipe()
{
	if (m_hSerial != INVALID_HANDLE_VALUE)
	{
		if(m_isserver){
			DisconnectNamedPipe(m_hSerial);
		}
		::CloseHandle(m_hSerial);
		m_hSerial = INVALID_HANDLE_VALUE;
	}
}

/**
 * ������
 * @param[in] nPort �|�[�g�ԍ�
 * @param[in] pipename �p�C�v�̖��O
 * @param[in] servername �T�[�o�[�̖��O
 * @retval true ����
 * @retval false ���s
 */
bool CComPipe::Initialize(LPCTSTR pipename, LPCTSTR servername)
{
	TCHAR szName[MAX_PATH];
	if(pipename==NULL || _tcslen(pipename) == 0){
		// No pipe name
		return false;
	}
	if(_tcslen(pipename) + (servername!=NULL ? _tcslen(servername) : 0) > MAX_PATH - 16){
		// too long pipe name
		return false;
	}
	if(_tcschr(pipename, '\\') != NULL || (servername!=NULL && _tcschr(servername, '\\') != NULL)){
		// cannot use \ in pipe name
		return false;
	}
	if(servername && _tcslen(servername)!=0){
		wsprintf(szName, TEXT("\\\\%s\\pipe\\%s"), servername, pipename);
	}else{
		wsprintf(szName, TEXT("\\\\.\\pipe\\%s"), pipename);
	}

	if(m_hSerial != INVALID_HANDLE_VALUE){
		if(m_isserver){
			DisconnectNamedPipe(m_hSerial);
		}
		::CloseHandle(m_hSerial);
		m_hSerial = INVALID_HANDLE_VALUE;
	}

	// �N���C�A���g�ڑ����g���C
	m_hSerial = CreateFile(szName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (m_hSerial == INVALID_HANDLE_VALUE) {
		// �T�[�o�[�ōăg���C
		m_hSerial = CreateNamedPipe(szName, PIPE_ACCESS_DUPLEX, PIPE_TYPE_BYTE|PIPE_NOWAIT, 1, 1024, 1024, 500, NULL);
		if (m_hSerial == INVALID_HANDLE_VALUE) {
			return false;
		}
		m_isserver = true;
		//ConnectNamedPipe(m_hSerial, NULL); // With a nonblocking-wait handle, the connect operation returns zero immediately, and the GetLastError function returns ERROR_PIPE_LISTENING. �Ƃ̂��ƂȂ̂ŏ펞0
	}

	_tcscpy(m_pipename, pipename);
	_tcscpy(m_pipeserv, servername);

	return true;
}

/**
 * �ǂݍ���
 * @param[out] pData �o�b�t�@
 * @return �T�C�Y
 */
UINT CComPipe::Read(UINT8* pData)
{
	DWORD dwReadSize;
	if (m_hSerial == INVALID_HANDLE_VALUE) {
		return 0;
	}
	if (PeekNamedPipe(m_hSerial, NULL, 0, NULL, &dwReadSize, NULL)){
		if(dwReadSize > 0){
			if (::ReadFile(m_hSerial, pData, 1, &dwReadSize, NULL))
			{
				return 1;
			}
		}
	}else{
		DWORD err = GetLastError();
		if(m_isserver){
			if(err==ERROR_BROKEN_PIPE){
				DisconnectNamedPipe(m_hSerial);
				ConnectNamedPipe(m_hSerial, NULL);
			}
		}else{
			if(err==ERROR_PIPE_NOT_CONNECTED){
				Initialize(m_pipename, m_pipeserv);
			}
		}
	}
	return 0;
}

/**
 * ��������
 * @param[out] cData �f�[�^
 * @return �T�C�Y
 */
UINT CComPipe::Write(UINT8 cData)
{
	LPTSTR lpMsgBuf;
	UINT ret;
	DWORD dwWrittenSize;
	DWORD errornum;
	if (m_hSerial == INVALID_HANDLE_VALUE) {
		return 0;
	}
	ret = (::WriteFile(m_hSerial, &cData, 1, &dwWrittenSize, NULL)) ? 1 : 0;
	if(dwWrittenSize==0) {
		return 0;
	}
	return ret;
}

/**
 * �X�e�[�^�X�𓾂�
 * @return �X�e�[�^�X
 */
UINT8 CComPipe::GetStat()
{
	return 0x20;
}

/**
 * ���b�Z�[�W
 * @param[in] nMessage ���b�Z�[�W
 * @param[in] nParam �p�����^
 * @return ���U���g �R�[�h
 */
INTPTR CComPipe::Message(UINT nMessage, INTPTR nParam)
{
	switch (nMessage)
	{
		default:
			break;
	}
	return 0;
}

#endif
