/**
 * @file	d_txt.cpp
 * @brief	txt dialog
 */

#include "compiler.h"
#include "resource.h"
#include "dialog.h"
#include "dosio.h"
#include "np2.h"
#include "sysmng.h"
#include "misc/DlgProc.h"
#include "cpucore.h"
#include "pccore.h"
#include "iocore.h"
#include "common/strres.h"

/** �t�B���^�[ */
static const UINT s_nFilter[1] =
{
	IDS_TXTFILTER
};

/**
 * �f�t�H���g �t�@�C���𓾂�
 * @param[in] lpExt �g���q
 * @param[out] lpFilename �t�@�C����
 * @param[in] cchFilename �t�@�C������
 */
static void GetDefaultFilename(LPCTSTR lpExt, LPTSTR lpFilename, UINT cchFilename)
{
	for (UINT i = 0; i < 10000; i++)
	{
		TCHAR szFilename[MAX_PATH];
		wsprintf(szFilename, TEXT("NP2_%04d.%s"), i, lpExt);

		file_cpyname(lpFilename, bmpfilefolder, cchFilename);
		file_cutname(lpFilename);
		file_catname(lpFilename, szFilename, cchFilename);

		if (file_attr(lpFilename) == -1)
		{
			break;
		}
	}
}

void convertJIStoSJIS(UINT8 buf[]) {
	unsigned char high = buf[0];
	unsigned char low = buf[1];
	high -= 0x21;
	if(high & 0x1){
		low += 0x7E;
	}else{
		low += 0x1F;
		if(low >= 0x7F){
			low++;
		}
	}
	high = high >> 1;
	if(high < 0x1F){
		high += 0x81;
	}else{
		high += 0xC1;
	}
	buf[0] = high;
	buf[1] = low;
}
void writetxt(const OEMCHAR *filename) {
	int i;
	int lpos = 0;
	int cpos = 0;
	//int kanjiMode = 0; // JIS�̂܂܂ŕۑ�����ꍇ�p
	UINT8 buf[5];
	FILEH	fh;
	if((fh = file_create(filename)) != FILEH_INVALID){
		for(i=0x0A0000;i<0x0A3FFF;i+=2){
			if(mem[i+1]){
				// �W������
				//if(!kanjiMode){
				//	buf[0] = 0x1b;
				//	buf[1] = 0x24;
				//	buf[2] = 0x40;
				//	file_write(fh, buf, 3);
				//}
				buf[0] = mem[i]+0x20;
				buf[1] = mem[i+1];
				convertJIStoSJIS(buf); // JIS -> Shift-JIS
				file_write(fh, buf, 2);
				i+=2;
				lpos+=2;
				//kanjiMode = 1;
			}else{
				// ASCII
				//if(kanjiMode){
				//	buf[0] = 0x1b;
				//	buf[1] = 0x28;
				//	buf[2] = 0x4a;
				//	file_write(fh, buf, 3);
				//}
				if(mem[i]<0x20 || (0x7F<=mem[i] && mem[i]<0xA0) || (0xE0<=mem[i] && mem[i]<0xFF)){
					// �󔒂ɕϊ�
					buf[0] = ' ';
				}else{
					buf[0] = mem[i];
				}
				file_write(fh, buf, 1);
				//kanjiMode = 0;
				lpos++;
			}
			if(lpos >= 80){
				cpos += lpos;
				lpos -= 80;
				if(cpos >= 80*25) break;
				buf[0] = '\r';
				file_write(fh, buf, 1);
				buf[0] = '\n';
				file_write(fh, buf, 1);
			}
		}
		file_close(fh);
	}
}
// XXX: �����ƓK�؂ȏꏊ�Ɉڂ��ׂ�
void dialog_getTVRAM(OEMCHAR *buffer) {
	int i;
	int lpos = 0;
	int cpos = 0;
	//int kanjiMode = 0; // JIS�̂܂܂ŕۑ�����ꍇ�p
	UINT8 buf[5];
	char *dstbuf = (char*)buffer;
	for(i=0x0A0000;i<0x0A3FFF;i+=2){
		if(mem[i+1]){
			// �W������
			//if(!kanjiMode){
			//	buf[0] = 0x1b;
			//	buf[1] = 0x24;
			//	buf[2] = 0x40;
			//	file_write(fh, buf, 3);
			//}
			buf[0] = mem[i]+0x20;
			buf[1] = mem[i+1];
			convertJIStoSJIS(buf); // JIS -> Shift-JIS
			memcpy(dstbuf, buf, 2);
			i+=2;
			lpos+=2;
			dstbuf+=2;
			//kanjiMode = 1;
		}else{
			// ASCII
			//if(kanjiMode){
			//	buf[0] = 0x1b;
			//	buf[1] = 0x28;
			//	buf[2] = 0x4a;
			//	file_write(fh, buf, 3);
			//}
			if(mem[i]<0x20 || (0x7F<=mem[i] && mem[i]<0xA0) || (0xE0<=mem[i] && mem[i]<0xFF)){
				// �󔒂ɕϊ�
				buf[0] = ' ';
			}else{
				buf[0] = mem[i];
			}
			memcpy(dstbuf, buf, 1);
			//kanjiMode = 0;
			lpos++;
			dstbuf++;
		}
		if(lpos >= 80){
			cpos += lpos;
			lpos -= 80;
			if(cpos >= 80*25) break;
			buf[0] = '\r';
			buf[1] = '\n';
			memcpy(dstbuf, buf, 2);
			dstbuf+=2;
		}
	}
	dstbuf[0] = '\0';
}

/**
 * TXT �o��
 * @param[in] hWnd �e�E�B���h�E
 */
void dialog_writetxt(HWND hWnd)
{
	std::tstring rExt(LoadTString(IDS_TXTEXT));
	std::tstring rFilter(LoadTString(s_nFilter[0]));
	std::tstring rTitle(LoadTString(IDS_TXTTITLE));

	TCHAR szPath[MAX_PATH];
	GetDefaultFilename(rExt.c_str(), szPath, _countof(szPath));

	CFileDlg dlg(FALSE, rExt.c_str(), szPath, OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY, rFilter.c_str(), hWnd);
	dlg.m_ofn.lpstrTitle = rTitle.c_str();
	dlg.m_ofn.nFilterIndex = 1;
	if (dlg.DoModal())
	{
		LPCTSTR lpFilename = dlg.GetPathName();
		LPCTSTR lpExt = file_getext(szPath);
		writetxt(lpFilename);
	}
}
