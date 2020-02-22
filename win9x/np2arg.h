/**
 *	@file	np2arg.h
 *	@brief	�������N���X�̐錾����уC���^�[�t�F�C�X�̒�`�����܂�
 */

#pragma once

/**
 * @brief �������N���X
 */
class Np2Arg
{
public:
	static Np2Arg* GetInstance();
	static void Release();

	Np2Arg();
	~Np2Arg();
	void Parse();
	void ClearDisk();
	LPCTSTR disk(int nDrive) const;
	LPCTSTR iniFilename() const;
	bool fullscreen() const;
	void setiniFilename(LPTSTR newfile);

private:
	static Np2Arg sm_instance;		//!< �B��̃C���X�^���X�ł�

	LPCTSTR m_lpDisk[4];	//!< �f�B�X�N
	LPTSTR m_lpIniFile;	//!< �ݒ�t�@�C��
	bool m_fFullscreen;		//!< �t���X�N���[�� ���[�h
	LPTSTR m_lpArg;			//!< ���[�N
};

/**
 * �C���X�^���X�𓾂�
 * @return �C���X�^���X
 */
inline Np2Arg* Np2Arg::GetInstance()
{
	return &sm_instance;
}

/**
 * ���������
 */
inline void Np2Arg::Release()
{
	if(sm_instance.m_lpArg){
		free(sm_instance.m_lpArg);
		sm_instance.m_lpArg = NULL;
	}
	if(sm_instance.m_lpIniFile){
		free(sm_instance.m_lpIniFile); // np21w ver0.86 rev8
		sm_instance.m_lpIniFile = NULL;
	}
}

/**
 * �f�B�X�N �p�X�𓾂�
 * @param[in] nDrive �h���C�u
 * @return �f�B�X�N �p�X
 */
inline LPCTSTR Np2Arg::disk(int nDrive) const
{
	return m_lpDisk[nDrive];
}

/**
 * INI �p�X�𓾂�
 * @return INI �p�X
 */
inline LPCTSTR Np2Arg::iniFilename() const
{
	return m_lpIniFile;
}

/**
 * �t���X�N���[�����𓾂�
 * @retval true �t���X�N���[�� ���[�h
 * @retval false �E�B���h�E ���[�h
 */
inline bool Np2Arg::fullscreen() const
{
	return m_fFullscreen;
}

/**
 * INI �p�X�𓾂�
 * @return INI �p�X
 */
inline void Np2Arg::setiniFilename(LPTSTR newfile)
{
	if(m_lpIniFile){
		free(m_lpIniFile);
		m_lpIniFile = NULL;
	}
	m_lpIniFile = newfile;
}

