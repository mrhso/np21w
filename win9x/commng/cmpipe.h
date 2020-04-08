/**
 * @file	cmpipe.h
 * @brief	���O�t���p�C�v �V���A�� �N���X�̐錾����уC���^�[�t�F�C�X�̒�`�����܂�
 */

#pragma once

#include "cmbase.h"

#if defined(SUPPORT_NAMED_PIPE)

/**
 * @brief commng �V���A�� �f�o�C�X �N���X
 */
class CComPipe : public CComBase
{
public:
	static CComPipe* CreateInstance(LPCTSTR pipename, LPCTSTR servername);

protected:
	CComPipe();
	virtual ~CComPipe();
	virtual UINT Read(UINT8* pData);
	virtual UINT Write(UINT8 cData);
	virtual UINT WriteRetry(); // �������ݑ��Ȃ��Ă�����ď������݂���
	virtual UINT LastWriteSuccess(); // �Ō�̏������݂��������Ă��邩�`�F�b�N
	virtual UINT8 GetStat();
	virtual INTPTR Message(UINT nMessage, INTPTR nParam);

private:
	HANDLE m_hSerial;		/*!< ���O�t���p�C�v �n���h�� */
	bool m_isserver;		/*!< �T�[�o�[���ǂ��� */
	OEMCHAR	m_pipename[MAX_PATH]; // The name of the named-pipe
	OEMCHAR	m_pipeserv[MAX_PATH]; // The server name of the named-pipe
	UINT8 m_lastdata; // �Ō�ɑ��낤�Ƃ����f�[�^
	UINT8 m_lastdatafail; // �f�[�^�𑗂�̂Ɏ��s���Ă�����0�ȊO
	DWORD m_lastdatatime; // �f�[�^�𑗂�̂Ɏ��s�������ԁi���܂�ɂ����s��������悤�Ȃ疳������j

	bool Initialize(LPCTSTR pipename, LPCTSTR servername);
};

#endif
