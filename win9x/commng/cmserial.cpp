/**
 * @file	cmserial.cpp
 * @brief	シリアル クラスの動作の定義を行います
 */

#include "compiler.h"
#include "cmserial.h"

/**
 * 速度テーブル
 */
const UINT32 cmserial_speed[11] = {110, 300, 600, 1200, 2400, 4800,
							9600, 19200, 38400, 57600, 115200};

/**
 * インスタンス作成
 * @param[in] nPort ポート番号
 * @param[in] cParam パラメタ
 * @param[in] nSpeed スピード
 * @return インスタンス
 */
CComSerial* CComSerial::CreateInstance(UINT nPort, UINT8 cParam, UINT32 nSpeed, UINT8 fixedspeed, UINT8 DSRcheck)
{
	CComSerial* pSerial = new CComSerial;
	if (!pSerial->Initialize(nPort, cParam, nSpeed, fixedspeed, DSRcheck))
	{
		delete pSerial;
		pSerial = NULL;
	}
	return pSerial;
}

/**
 * コンストラクタ
 */
CComSerial::CComSerial()
	: CComBase(COMCONNECT_SERIAL)
	, m_hSerial(INVALID_HANDLE_VALUE)
	, m_lastdata(0)
	, m_lastdatafail(0)
{
}

/**
 * デストラクタ
 */
CComSerial::~CComSerial()
{
	if (m_hSerial != INVALID_HANDLE_VALUE)
	{
		::CloseHandle(m_hSerial);
		m_hSerial = INVALID_HANDLE_VALUE;
	}
}

/**
 * 初期化
 * @param[in] nPort ポート番号
 * @param[in] cParam パラメタ
 * @param[in] nSpeed スピード
 * @retval true 成功
 * @retval false 失敗
 */
bool CComSerial::Initialize(UINT nPort, UINT8 cParam, UINT32 nSpeed, UINT8 fixedspeed, UINT8 DSRcheck)
{
	TCHAR szName[16];
	wsprintf(szName, TEXT("COM%u"), nPort);
	m_hSerial = CreateFile(szName, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, NULL);
	if (m_hSerial == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	m_fixedspeed = !!fixedspeed;

	PurgeComm(m_hSerial, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);

	DCB dcb;
	::GetCommState(m_hSerial, &dcb);
	for (UINT i = 0; i < NELEMENTS(cmserial_speed); i++)
	{
		if (cmserial_speed[i] >= nSpeed)
		{
			dcb.BaudRate = cmserial_speed[i];
			break;
		}
	}
	dcb.fDtrControl = DTR_CONTROL_ENABLE;
	dcb.fRtsControl = RTS_CONTROL_ENABLE;
	dcb.ByteSize = (UINT8)(((cParam >> 2) & 3) + 5);
	switch (cParam & 0x30)
	{
		case 0x10:
			dcb.Parity = ODDPARITY;
			break;

		case 0x30:
			dcb.Parity = EVENPARITY;
			break;

		default:
			dcb.Parity = NOPARITY;
			break;
	}
	switch (cParam & 0xc0)
	{
		case 0x80:
			dcb.StopBits = ONE5STOPBITS;
			break;

		case 0xc0:
			dcb.StopBits = TWOSTOPBITS;
			break;

		default:
			dcb.StopBits = ONESTOPBIT;
			break;
	}
	dcb.fOutX = FALSE;
	dcb.fInX = FALSE;
	dcb.fOutxCtsFlow = FALSE;
	dcb.fOutxDsrFlow = FALSE;
	dcb.fDsrSensitivity = (DSRcheck ? TRUE : FALSE);
	::SetCommState(m_hSerial, &dcb);
	return true;
}

/**
 * 読み込み
 * @param[out] pData バッファ
 * @return サイズ
 */
UINT CComSerial::Read(UINT8* pData)
{
	DWORD err;
	COMSTAT ct;
	::ClearCommError(m_hSerial, &err, &ct);
	if (ct.cbInQue)
	{
		DWORD dwReadSize;
		if (::ReadFile(m_hSerial, pData, 1, &dwReadSize, NULL))
		{
			return 1;
		}
	}
	return 0;
}

/**
 * 書き込み
 * @param[out] cData データ
 * @return サイズ
 */
UINT CComSerial::Write(UINT8 cData)
{
	UINT ret;
	DWORD dwWrittenSize;
	if (m_hSerial == INVALID_HANDLE_VALUE) {
		m_lastdatafail = 1;
		return 0;
	}
	ret = (::WriteFile(m_hSerial, &cData, 1, &dwWrittenSize, NULL)) ? 1 : 0;
	if(dwWrittenSize==0) {
		if(m_lastdatafail && GetTickCount() - m_lastdatatime > 3000){
			return 1; // 3秒間バッファデータが減りそうにないならあきらめる
		}
		m_lastdatafail = 1;
		m_lastdata = cData;
		m_lastdatatime = GetTickCount();
		return 0;
	}else{
		m_lastdatafail = 0;
		m_lastdata = 0;
		m_lastdatatime = 0;
	}
	return ret;
}

/**
 * 書き込みリトライ
 * @return サイズ
 */
UINT CComSerial::WriteRetry()
{
	UINT ret;
	DWORD dwWrittenSize;
	if(m_lastdatafail){
		if (GetTickCount() - m_lastdatatime > 3000) return 1; // 3秒間バッファデータが減りそうにないならあきらめる
		if (m_hSerial == INVALID_HANDLE_VALUE) {
			return 0;
		}
		ret = (::WriteFile(m_hSerial, &m_lastdata, 1, &dwWrittenSize, NULL)) ? 1 : 0;
		if(dwWrittenSize==0) {
			return 0;
		}
		m_lastdatafail = 0;
		m_lastdata = 0;
		m_lastdatatime = 0;
		return ret;
	}
	return 1;
}

/**
 * 最後の書き込みが成功しているかチェック
 * @return サイズ
 */
UINT CComSerial::LastWriteSuccess()
{
	if(m_lastdatafail && GetTickCount() - m_lastdatatime > 3000){
		return 1; // 3秒間バッファデータが減りそうにないならあきらめる
	}
	if(m_lastdatafail){
		return 0;
	}
	return 1;
}

/**
 * ステータスを得る
 * bit 7: ~CI (RI, RING)
 * bit 6: ~CS (CTS)
 * bit 5: ~CD (DCD, RLSD)
 * bit 4: reserved
 * bit 3: reserved
 * bit 2: reserved
 * bit 1: reserved
 * bit 0: ~DSR (DR)
 * @return ステータス 
 */
UINT8 CComSerial::GetStat()
{
	UINT8 ret = 0;
	DWORD modemStat;
	if(::GetCommModemStatus(m_hSerial, &modemStat)){
		if(!(modemStat & MS_DSR_ON)){
			ret |= 0x01;
		}
		if(!(modemStat & MS_CTS_ON)){
			ret |= 0x40;
		}
		if(!(modemStat & MS_RING_ON)){
			ret |= 0x80;
		}
		if(!(modemStat & MS_RLSD_ON)){
			ret |= 0x20;
		}
		return ret;
	}else{
		DWORD err;
		COMSTAT ct;
		::ClearCommError(m_hSerial, &err, &ct);
		if (ct.fDsrHold)
		{
			ret |= 0x01;
		}
		if (ct.fCtsHold)
		{
			ret |= 0x40;
		}
		if (ct.fRlsdHold)
		{
			ret |= 0x20;
		}
		return ret;
	}
}

/**
 * メッセージ
 * @param[in] nMessage メッセージ
 * @param[in] nParam パラメタ
 * @return リザルト コード
 */
INTPTR CComSerial::Message(UINT nMessage, INTPTR nParam)
{
	switch (nMessage)
	{
		case COMMSG_CHANGESPEED:
			if(!m_fixedspeed){
				int newspeed = *(reinterpret_cast<int*>(nParam));
				for (UINT i = 0; i < NELEMENTS(cmserial_speed); i++)
				{
					if (cmserial_speed[i] >= newspeed)
					{
						DCB dcb;
						::GetCommState(m_hSerial, &dcb);
						if(cmserial_speed[i] != dcb.BaudRate){
							dcb.BaudRate = cmserial_speed[i];
							::SetCommState(m_hSerial, &dcb);
						}
						break;
					}
				}
			}
			break;
			
		case COMMSG_CHANGEMODE:
			if(!m_fixedspeed){
				bool changed = false;
				UINT8 newmode = *(reinterpret_cast<UINT8*>(nParam)); // I/O 32h モードセットのデータ
				BYTE stopbits_value[] = {ONESTOPBIT, ONESTOPBIT, ONE5STOPBITS, TWOSTOPBITS};
				BYTE parity_value[] = {NOPARITY, ODDPARITY, NOPARITY, EVENPARITY};
				BYTE bytesize_value[] = {5, 6, 7, 8};
				DCB dcb;
				::GetCommState(m_hSerial, &dcb);
				if(dcb.StopBits != stopbits_value[(newmode >> 6) & 0x3]){
					dcb.StopBits = stopbits_value[(newmode >> 6) & 0x3];
					changed = true;
				}
				if(dcb.Parity != parity_value[(newmode >> 4) & 0x3]){
					dcb.Parity = parity_value[(newmode >> 4) & 0x3];
					changed = true;
				}
				if(dcb.ByteSize != bytesize_value[(newmode >> 2) & 0x3]){
					dcb.ByteSize = bytesize_value[(newmode >> 2) & 0x3];
					changed = true;
				}
				if(changed){
					::PurgeComm(m_hSerial, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);
					::SetCommState(m_hSerial, &dcb);
				}
			}
			break;

		case COMMSG_SETCOMMAND:
			{
				UINT8 cmd = *(reinterpret_cast<UINT8*>(nParam)); // I/O 32h コマンドセットのデータ
				::PurgeComm(m_hSerial, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);
				if(cmd & 0x20){ // RTS
					::EscapeCommFunction(m_hSerial, SETRTS);
				}else{
					::EscapeCommFunction(m_hSerial, CLRRTS);
				}
				if(cmd & 0x02){ // DTR
					::EscapeCommFunction(m_hSerial, SETDTR);
				}else{
					::EscapeCommFunction(m_hSerial, CLRDTR);
				}
			}
			break;

		case COMMSG_PURGE:
			{
				::PurgeComm(m_hSerial, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);
			}
			break;
			

		case COMMSG_SETFLAG:
			{
				COMFLAG flag = reinterpret_cast<COMFLAG>(nParam);
				if ((flag) && (flag->size == sizeof(_COMFLAG)))
				{
					return 1;
				}
			}
			break;

		case COMMSG_GETFLAG:
			{
				// dummy data
				COMFLAG flag = (COMFLAG)_MALLOC(sizeof(_COMFLAG), "RS232C FLAG");
				if (flag)
				{
					flag->size = sizeof(_COMFLAG);
					flag->sig = COMSIG_COM1;
					flag->ver = 0;
					flag->param = 0;
					return reinterpret_cast<INTPTR>(flag);
				}
			}
			break;

		default:
			break;
	}
	return 0;
}
