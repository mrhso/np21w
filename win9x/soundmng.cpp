/**
 * @file	soundmng.cpp
 * @brief	�T�E���h �}�l�[�W�� �N���X�̓���̒�`���s���܂�
 */

#include "compiler.h"
#include "soundmng.h"
#include "np2.h"
#if defined(SUPPORT_ROMEO)
#include "ext\externalchipmanager.h"
#endif
#if defined(MT32SOUND_DLL)
#include "ext\mt32snd.h"
#endif
#if defined(SUPPORT_ASIO)
#include "soundmng\sdasio.h"
#endif	// defined(SUPPORT_ASIO)
#include "soundmng\sddsound3.h"
#if defined(SUPPORT_WASAPI)
#include "soundmng\sdwasapi.h"
#endif	// defined(SUPPORT_WASAPI)
#include "common\parts.h"
#include "sound\sound.h"
#if defined(VERMOUTH_LIB)
#include "sound\vermouth\vermouth.h"
#endif
#include	"np2mt.h"

#if !defined(_WIN64)
#ifdef __cplusplus
extern "C"
{
#endif
/**
 * satuation
 * @param[out] dst �o�̓o�b�t�@
 * @param[in] src ���̓o�b�t�@
 * @param[in] size �T�C�Y
 */
void __fastcall satuation_s16mmx(SINT16 *dst, const SINT32 *src, UINT size);
#ifdef __cplusplus
}
#endif
#endif

#if defined(VERMOUTH_LIB)
	MIDIMOD		vermouth_module = NULL;
#endif

//! �B��̃C���X�^���X�ł�
CSoundMng CSoundMng::sm_instance;

/**
 * ������
 */
void CSoundMng::Initialize()
{
#if defined(SUPPORT_ASIO) || defined(SUPPORT_WASAPI)
	::CoInitializeEx(NULL, COINIT_MULTITHREADED);
#endif	// defined(SUPPORT_ASIO) || defined(SUPPORT_WASAPI)

	CSoundDeviceDSound3::s_mastervol_available = true;//np2oscfg.usemastervolume ? true : false;
	CSoundDeviceDSound3::Initialize();
#if defined(SUPPORT_WASAPI)
	CSoundDeviceWasapi::Initialize();
#endif	// defined(SUPPORT_WASAPI)

#if defined(SUPPORT_ASIO)
	CSoundDeviceAsio::Initialize();
#endif	// defined(SUPPORT_ASIO)

#if defined(SUPPORT_ROMEO)
	CExternalChipManager::GetInstance()->Initialize();
#endif	// defined(SUPPORT_ROMEO)

	CSoundMng::GetInstance()->InitializeSoundCriticalSection();
}

/**
 * ���
 */
void CSoundMng::Deinitialize()
{
#if defined(SUPPORT_ROMEO)
	CExternalChipManager::GetInstance()->Deinitialize();
#endif	// defined(SUPPORT_ROMEO)

#if defined(SUPPORT_WASAPI)
	CSoundDeviceWasapi::Deinitialize();
#endif	// defined(SUPPORT_WASAPI)

#if defined(SUPPORT_ASIO) || defined(SUPPORT_WASAPI)
	::CoUninitialize();
#endif	// defined(SUPPORT_ASIO) || defined(SUPPORT_WASAPI)
	
	CSoundMng::GetInstance()->FinalizeSoundCriticalSection();
}

/**
 * �R���X�g���N�^
 */
CSoundMng::CSoundMng()
	: m_pSoundDevice(NULL)
	, m_nMute(0)
	, m_sound_cs_initialized(false)
{
	SetReverse(false);
}

/**
 * �I�[�v��
 * @param[in] nType �f�o�C�X �^�C�v
 * @param[in] lpName �f�o�C�X��
 * @param[in] hWnd �E�B���h�E �n���h��
 * @retval true ����
 * @retval false ���s
 */
bool CSoundMng::Open(DeviceType nType, LPCTSTR lpName, HWND hWnd)
{
	EnterAllCriticalSection();

	Close();

	CSoundDeviceBase* pSoundDevice = NULL;
	switch (nType)
	{
		case kDefault:
			pSoundDevice = new CSoundDeviceDSound3;
			lpName = NULL;
			break;

		case kDSound3:
			pSoundDevice = new CSoundDeviceDSound3;
			break;

#if defined(SUPPORT_WASAPI)
		case kWasapi:
			pSoundDevice = new CSoundDeviceWasapi;
			break;
#endif	// defined(SUPPORT_WASAPI)

#if defined(SUPPORT_ASIO)
		case kAsio:
			pSoundDevice = new CSoundDeviceAsio;
			break;
#endif	// defined(SUPPORT_ASIO)
	}

	if (pSoundDevice)
	{
		if (!pSoundDevice->Open(lpName, hWnd))
		{
			delete pSoundDevice;
			pSoundDevice = NULL;
		}
	}

	if (pSoundDevice == NULL)
	{
		LeaveSoundCriticalSection();
		return false;
	}

	m_pSoundDevice = pSoundDevice;

#if defined(MT32SOUND_DLL)
	MT32Sound::GetInstance()->Initialize();
#endif
	
	LeaveAllCriticalSection();
	return true;
}

/**
 * �N���[�Y
 */
void CSoundMng::Close()
{
	EnterAllCriticalSection();
	if (m_pSoundDevice)
	{
		m_pSoundDevice->Close();
		delete m_pSoundDevice;
		m_pSoundDevice = NULL;
	}

#if defined(MT32SOUND_DLL)
	MT32Sound::GetInstance()->Deinitialize();
#endif
	LeaveAllCriticalSection();
}

/**
 * �T�E���h�L��
 * @param[in] nProc �v���V�[�W��
 */
void CSoundMng::Enable(SoundProc nProc)
{
	EnterSoundCriticalSection();
	const UINT nBit = 1 << nProc;
	if (!(m_nMute & nBit))
	{
		LeaveSoundCriticalSection();
		return;
	}
	m_nMute &= ~nBit;
	if (!m_nMute)
	{
		if (m_pSoundDevice)
		{
			m_pSoundDevice->PlayStream();
		}
#if defined(SUPPORT_ROMEO)
		CExternalChipManager::GetInstance()->Mute(false);
#endif	// defined(SUPPORT_ROMEO)
	}
	LeaveSoundCriticalSection();
}

/**
 * �T�E���h����
 * @param[in] nProc �v���V�[�W��
 */
void CSoundMng::Disable(SoundProc nProc)
{
	EnterSoundCriticalSection();
	if (!m_nMute)
	{
		if (m_pSoundDevice)
		{
			m_pSoundDevice->StopStream();
			m_pSoundDevice->StopAllPCM();
		}
#if defined(SUPPORT_ROMEO)
		CExternalChipManager::GetInstance()->Mute(true);
#endif	// defined(SUPPORT_ROMEO)
	}
	m_nMute |= (1 << nProc);
	LeaveSoundCriticalSection();
}

/**
 * �X�g���[�����쐬
 * @param[in] nSamplingRate �T���v�����O ���[�g
 * @param[in] ms �o�b�t�@��(�~���b)
 * @return �o�b�t�@��
 */
UINT CSoundMng::CreateStream(UINT nSamplingRate, UINT ms)
{
	EnterAllCriticalSection();
	if (m_pSoundDevice == NULL)
	{
		LeaveSoundCriticalSection();
		return 0;
	}

	if (ms < 40)
	{
		ms = 40;
	}
	else if (ms > 1000)
	{
		ms = 1000;
	}
	UINT nBuffer = (nSamplingRate * ms) / 2000;
	nBuffer = (nBuffer + 1) & (~1);

	nBuffer = m_pSoundDevice->CreateStream(nSamplingRate, 2, nBuffer);
	if (nBuffer == 0)
	{
		LeaveSoundCriticalSection();
		return 0;
	}
	m_pSoundDevice->SetStreamData(this);

#if defined(VERMOUTH_LIB)
	vermouth_module = midimod_create(nSamplingRate);
	midimod_loadall(vermouth_module);
#endif

#if defined(MT32SOUND_DLL)
	MT32Sound::GetInstance()->SetRate(nSamplingRate);
#endif

	LeaveAllCriticalSection();
	return nBuffer;
}

/**
 * �X�g���[����j��
 */
inline void CSoundMng::DestroyStream()
{
	if (m_pSoundDevice)
	{
		m_pSoundDevice->DestroyStream();
	}

#if defined(VERMOUTH_LIB)
	midimod_destroy(vermouth_module);
	vermouth_module = NULL;
#endif
#if defined(MT32SOUND_DLL)
	MT32Sound::GetInstance()->SetRate(0);
#endif
}

/**
 * �X�g���[���̃��Z�b�g
 */
inline void CSoundMng::ResetStream()
{
	EnterSoundCriticalSection();
	if (m_pSoundDevice)
	{
		m_pSoundDevice->ResetStream();
	}
	LeaveSoundCriticalSection();
}

/**
 * �X�g���[���̍Đ�
 */
inline void CSoundMng::PlayStream()
{
	EnterSoundCriticalSection();
	if (!m_nMute)
	{
		if (m_pSoundDevice)
		{
			m_pSoundDevice->PlayStream();
		}
	}
	LeaveSoundCriticalSection();
}

/**
 * �X�g���[���̒�~
 */
inline void CSoundMng::StopStream()
{
	EnterSoundCriticalSection();
	if (!m_nMute)
	{
		if (m_pSoundDevice)
		{
			m_pSoundDevice->StopStream();
		}
	}
	LeaveSoundCriticalSection();
}

/**
 * �X�g���[���𓾂�
 * @param[out] lpBuffer �o�b�t�@
 * @param[in] nBufferCount �o�b�t�@ �J�E���g
 * @return �T���v����
 */
UINT CSoundMng::Get16(SINT16* lpBuffer, UINT nBufferCount)
{
	EnterSoundCriticalSection();
	const SINT32* lpSource = ::sound_pcmlock();
	if (lpSource)
	{
		(*m_fnMix)(lpBuffer, lpSource, nBufferCount * 4);
		::sound_pcmunlock(lpSource);
		LeaveSoundCriticalSection();
		return nBufferCount;
	}
	else
	{
		LeaveSoundCriticalSection();
		return 0;
	}
}

/**
 * �p�����]��ݒ肷��
 * @param[in] bReverse ���]�t���O
 */
inline void CSoundMng::SetReverse(bool bReverse)
{
	EnterSoundCriticalSection();
	if (!bReverse)
	{
#if !defined(_WIN64)
		if (mmxflag)
		{
			m_fnMix = satuation_s16;
		}
		else {
			m_fnMix = satuation_s16mmx;
		}
#else
		m_fnMix = satuation_s16;
#endif
	}
	else
	{
		m_fnMix = satuation_s16x;
	}
	LeaveSoundCriticalSection();
}

/**
 * �}�X�^�[ ���H�����[���ݒ�
 * @param[in] nVolume ���H�����[��
 */
void CSoundMng::SetMasterVolume(int nVolume)
{
	EnterSoundCriticalSection();
	if (m_pSoundDevice)
	{
		m_pSoundDevice->SetMasterVolume(nVolume);
	}
	LeaveSoundCriticalSection();
}

/**
 * PCM �f�[�^�ǂݍ���
 * @param[in] nNum PCM �ԍ�
 * @param[in] lpFilename �t�@�C����
 */
void CSoundMng::LoadPCM(SoundPCMNumber nNum, LPCTSTR lpFilename)
{
	EnterSoundCriticalSection();
	if (m_pSoundDevice)
	{
		m_pSoundDevice->LoadPCM(nNum, lpFilename);
	}
	LeaveSoundCriticalSection();
}

/**
 * PCM �f�[�^�ēǂݍ���
 * @param[in] nNum PCM �ԍ�
 * @param[in] lpFilename �t�@�C����
 */
void CSoundMng::ReloadPCM(SoundPCMNumber nNum)
{
	EnterSoundCriticalSection();
	if (m_pSoundDevice)
	{
		m_pSoundDevice->ReloadPCM(nNum);
	}
	LeaveSoundCriticalSection();
}

/**
 * PCM ���H�����[���ݒ�
 * @param[in] nNum PCM �ԍ�
 * @param[in] nVolume ���H�����[��
 */
void CSoundMng::SetPCMVolume(SoundPCMNumber nNum, int nVolume)
{
	EnterSoundCriticalSection();
	if (m_pSoundDevice)
	{
		m_pSoundDevice->SetPCMVolume(nNum, nVolume);
	}
	LeaveSoundCriticalSection();
}

/**
 * PCM �Đ�
 * @param[in] nNum PCM �ԍ�
 * @param[in] bLoop ���[�v �t���O
 * @retval true ����
 * @retval false ���s
 */
inline bool CSoundMng::PlayPCM(SoundPCMNumber nNum, BOOL bLoop)
{
	EnterSoundCriticalSection();
	if (!m_nMute)
	{
		if (m_pSoundDevice)
		{
			LeaveSoundCriticalSection();
			return m_pSoundDevice->PlayPCM(nNum, bLoop);
		}
	}
	LeaveSoundCriticalSection();
	return false;
}

/**
 * PCM ��~
 * @param[in] nNum PCM �ԍ�
 */
inline void CSoundMng::StopPCM(SoundPCMNumber nNum)
{
	EnterSoundCriticalSection();
	if (m_pSoundDevice)
	{
		m_pSoundDevice->StopPCM(nNum);
	}
	LeaveSoundCriticalSection();
}

// �N���e�B�J���Z�N�V�����֘A
void CSoundMng::InitializeSoundCriticalSection()
{
	if(!m_sound_cs_initialized){
		InitializeCriticalSection(&m_sound_cs);
		m_sound_cs_initialized = true;
	}
}
void CSoundMng::FinalizeSoundCriticalSection()
{
	if(m_sound_cs_initialized){
		DeleteCriticalSection(&m_sound_cs);
		m_sound_cs_initialized = false;
	}
}
void CSoundMng::EnterSoundCriticalSection()
{
	if(m_sound_cs_initialized){
		EnterCriticalSection(&m_sound_cs);
	}
}
void CSoundMng::LeaveSoundCriticalSection()
{
	if(m_sound_cs_initialized){
		LeaveCriticalSection(&m_sound_cs);
	}
}
void CSoundMng::EnterAllCriticalSection()
{
	np2_multithread_EnterCriticalSection();
	EnterSoundCriticalSection();
}
void CSoundMng::LeaveAllCriticalSection()
{
	LeaveSoundCriticalSection();
	np2_multithread_LeaveCriticalSection();
}

// ---- C ���b�p�[

/**
 * �X�g���[���쐬
 * @param[in] rate �T���v�����O ���[�g
 * @param[in] ms �o�b�t�@��(�~���b)
 * @return �o�b�t�@ �T�C�Y
 */
UINT soundmng_create(UINT rate, UINT ms)
{
	UINT result;
	result = CSoundMng::GetInstance()->CreateStream(rate, ms);
	return result;
}

/**
 * �X�g���[���j��
 */
void soundmng_destroy(void)
{
	CSoundMng::GetInstance()->DestroyStream();
}

/**
 * �X�g���[�� ���Z�b�g
 */
void soundmng_reset(void)
{
	CSoundMng::GetInstance()->ResetStream();
}

/**
 * �X�g���[���J�n
 */
void soundmng_play(void)
{
	CSoundMng::GetInstance()->PlayStream();
}

/**
 * �X�g���[����~
 */
void soundmng_stop(void)
{
	CSoundMng::GetInstance()->StopStream();
}

/**
 * �X�g���[�� �p�����]�ݒ�
 * @param[in] bReverse ���]
 */
void soundmng_setreverse(BOOL bReverse)
{
	CSoundMng::GetInstance()->SetReverse((bReverse) ? true : false);
}

/**
 * �{�����[���ݒ�
 * @param[in] nVolume �{�����[��(�ŏ� 0 �` 100 �ő�)
 */
void soundmng_setvolume(int nVolume)
{
	CSoundMng::GetInstance()->SetMasterVolume(nVolume);
}

/**
 * PCM �Đ�
 * @param[in] nNum PCM �ԍ�
 * @param[in] bLoop ���[�v
 * @retval SUCCESS ����
 * @retval FAILURE ���s
 */
BRESULT soundmng_pcmplay(enum SoundPCMNumber nNum, BOOL bLoop)
{
	BRESULT result;
	result = (CSoundMng::GetInstance()->PlayPCM(nNum, bLoop)) ? SUCCESS : FAILURE;
	return result;
}

/**
 * PCM ��~
 * @param[in] nNum PCM �ԍ�
 */
void soundmng_pcmstop(enum SoundPCMNumber nNum)
{
	CSoundMng::GetInstance()->StopPCM(nNum);
}


