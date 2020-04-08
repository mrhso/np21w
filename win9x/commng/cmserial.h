/**
 * @file	cmserial.h
 * @brief	�V���A�� �N���X�̐錾����уC���^�[�t�F�C�X�̒�`�����܂�
 */

#pragma once

#include "cmbase.h"

extern const UINT32 cmserial_speed[11];

/**
 * @brief commng �V���A�� �f�o�C�X �N���X
 */
class CComSerial : public CComBase
{
public:
	static CComSerial* CreateInstance(UINT nPort, UINT8 cParam, UINT32 nSpeed, UINT8 fixedspeed);

protected:
	CComSerial();
	virtual ~CComSerial();
	virtual UINT Read(UINT8* pData);
	virtual UINT Write(UINT8 cData);
	virtual UINT WriteRetry();
	virtual UINT LastWriteSuccess(); // �Ō�̏������݂��������Ă��邩�`�F�b�N
	virtual UINT8 GetStat();
	virtual INTPTR Message(UINT nMessage, INTPTR nParam);

private:
	HANDLE m_hSerial;		/*!< �V���A�� �n���h�� */

	bool m_fixedspeed;	/*!< �ʐM���x�Œ� */
	UINT8 m_lastdata; // �Ō�ɑ��낤�Ƃ����f�[�^
	UINT8 m_lastdatafail; // �f�[�^�𑗂�̂Ɏ��s���Ă�����0�ȊO
	DWORD m_lastdatatime; // �f�[�^�𑗂�̂Ɏ��s�������ԁi���܂�ɂ����s��������悤�Ȃ疳������j

	bool Initialize(UINT nPort, UINT8 cParam, UINT32 nSpeed, UINT8 fixedspeed);
};
