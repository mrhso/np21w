/**
 * @file	wab.c
 * @brief	Window Accelerator Board Interface
 *
 * @author	$Author: SimK $
 */

#include "compiler.h"

#if defined(SUPPORT_WAB)

#include "np2.h"
#include "resource.h"
#include "dosio.h"
#include "cpucore.h"
#include "pccore.h"
#include "iocore.h"
#include "joymng.h"
#include "mousemng.h"
#include "scrnmng.h"
#include "soundmng.h"
#include "sysmng.h"
#include "winkbd.h"
#include "winloc.h"
#include "profile.h"
#include "ini.h"
#include "dispsync.h"
#include "wab.h"
#include "bmpdata.h"
#include "wabbmpsave.h"
#if defined(_WINDOWS)
#include	<process.h>
#endif

NP2WAB		np2wab = {0};
NP2WABWND	np2wabwnd = {0};
NP2WABCFG	np2wabcfg;
	
TCHAR	g_Name[100] = _T("NP2 Window Accelerator");

static HINSTANCE ga_hInstance = NULL;
static HANDLE	ga_hThread = NULL;
static int		ga_exitThread = 0;
static int		ga_threadmode = 1;
static int		ga_lastwabwidth = 640;
static int		ga_lastwabheight = 480;
static int		ga_reqChangeWindowSize = 0;
static int		ga_reqChangeWindowSize_w = 0;
static int		ga_reqChangeWindowSize_h = 0;

static int		ga_lastscalemode = 0;
static int		ga_lastrealwidth = 0;
static int		ga_lastrealheight = 0;
static int		ga_screenupdated = 0;

#if defined(_WIN32)
static int wab_cs_initialized = 0;
static CRITICAL_SECTION wab_cs;
#endif

static BOOL wab_tryenter_criticalsection(void){
#if defined(_WIN32)
	if(!wab_cs_initialized) return TRUE;
	return TryEnterCriticalSection(&wab_cs);
#else
	return TRUE;
#endif
}
static void wab_enter_criticalsection(void){
#if defined(_WIN32)
	if(!wab_cs_initialized) return;
	EnterCriticalSection(&wab_cs);
#endif
}
static void wab_leave_criticalsection(void){
#if defined(_WIN32)
	if(!wab_cs_initialized) return;
	LeaveCriticalSection(&wab_cs);
#endif
}
static void wab_init_criticalsection(void){
#if defined(_WIN32)
	if(!wab_cs_initialized){
		memset(&wab_cs, 0, sizeof(wab_cs));
		InitializeCriticalSection(&wab_cs);
		wab_cs_initialized = 1;
	}
#endif
}
static void wab_delete_criticalsection(void){
#if defined(_WIN32)
	if(wab_cs_initialized){
		DeleteCriticalSection(&wab_cs);
		wab_cs_initialized = 0;
	}
#endif
}

/**
 * 設定
 */
static const PFTBL s_wabwndini[] =
{
	PFVAL("WindposX", PFTYPE_SINT32,	&np2wabcfg.posx),
	PFVAL("WindposY", PFTYPE_SINT32,	&np2wabcfg.posy),
	PFVAL("MULTIWND", PFTYPE_BOOL,		&np2wabcfg.multiwindow),
	PFVAL("MULTHREAD",PFTYPE_BOOL,		&np2wabcfg.multithread),
	PFVAL("HALFTONE", PFTYPE_BOOL,		&np2wabcfg.halftone),
};

/**
 * 設定読み込み
 */
void wabwin_readini()
{
	OEMCHAR szPath[MAX_PATH];

	ZeroMemory(&np2wabcfg, sizeof(np2wabcfg));
	np2wabcfg.posx = CW_USEDEFAULT;
	np2wabcfg.posy = CW_USEDEFAULT;
	np2wabcfg.multiwindow = 0;
	np2wabcfg.multithread = 1;
	np2wabcfg.halftone = 0;

	initgetfile(szPath, NELEMENTS(szPath));
	ini_read(szPath, g_Name, s_wabwndini, NELEMENTS(s_wabwndini));
}

/**
 * 設定書き込み
 */
void wabwin_writeini()
{
	if(!np2wabcfg.readonly){
		TCHAR szPath[MAX_PATH];
		initgetfile(szPath, NELEMENTS(szPath));
		ini_write(szPath, g_Name, s_wabwndini, NELEMENTS(s_wabwndini));
	}
}

/**
 * 画面サイズ設定
 */
void np2wab_setScreenSize(int width, int height)
{
	if(np2wabwnd.multiwindow){
		// 別窓モードなら別窓サイズを更新する
		RECT rect = { 0, 0, width, height };
		np2wab.wndWidth = width;
		np2wab.wndHeight = height;
		AdjustWindowRectEx( &rect, WS_OVERLAPPEDWINDOW, FALSE, 0 );
		SetWindowPos( np2wabwnd.hWndWAB, NULL, 0, 0, rect.right-rect.left, rect.bottom-rect.top, SWP_NOMOVE|SWP_NOZORDER );
	}else{
		// 統合モードならエミュレーション領域サイズを更新する
		np2wab.wndWidth = ga_lastwabwidth = width;
		np2wab.wndHeight = ga_lastwabheight = height;
		if(np2wab.relay & 0x3){
			if(width < 32 || height < 32){
				//scrnmng_setwidth(0, 640);
				//scrnmng_setheight(0, 480);
				scrnmng_setsize(0, 0, 640, 480);
			}else{
				//scrnmng_setwidth(0, width);
				//scrnmng_setheight(0, height);
				scrnmng_setsize(0, 0, width, height);
			}
			scrnmng_updatefsres(); // フルスクリーン解像度更新
			mousemng_updateclip(); // マウスキャプチャのクリップ範囲を修正
		}
	}
	// とりあえずパレットは更新しておく
	np2wab.paletteChanged = 1;
}
/**
 * 画面サイズ設定マルチスレッド対応版（すぐに更新できない場合はnp2wab.ready=0に）
 */
void np2wab_setScreenSizeMT(int width, int height)
{
	if(!ga_threadmode){
		// マルチスレッドモードでなければ直接呼び出し
		np2wab_setScreenSize(width, height);
		ga_lastrealwidth = width;
		ga_lastrealheight = height;
	}else{
		// マルチスレッドモードなら画面サイズ変更要求を出す
		ga_reqChangeWindowSize_w = width;
		ga_reqChangeWindowSize_h = height;
		ga_reqChangeWindowSize = 1;
		np2wabwnd.ready = 0; // 更新待ち
	}
}

/**
 * ウィンドウアクセラレータ別窓を等倍サイズに戻す
 */
void np2wab_resetscreensize()
{
	if(np2wabwnd.multiwindow){
		RECT rect = {0};
		rect.right = np2wab.wndWidth = np2wab.realWidth;
		rect.bottom = np2wab.wndHeight = np2wab.realHeight;
		AdjustWindowRectEx( &rect, WS_OVERLAPPEDWINDOW, FALSE, 0 );
		SetWindowPos( np2wabwnd.hWndWAB, NULL, 0, 0, rect.right-rect.left, rect.bottom-rect.top, SWP_NOMOVE|SWP_NOZORDER );
	}
}

/**
 * ウィンドウアクセラレータ別窓WndProc
 */
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam){
	RECT		rc;
	HMENU		hSysMenu;
	HMENU		hWabMenu;
	MENUITEMINFO mii = {0};
	TCHAR szString[128];
	int scalemode;

	switch (msg) {
		case WM_CREATE:
			hSysMenu = GetSystemMenu(hWnd, FALSE);
			hWabMenu = LoadMenu(ga_hInstance, MAKEINTRESOURCE(IDR_WABSYSMENU));
			mii.cbSize = sizeof(mii);
			mii.fMask = MIIM_TYPE | MIIM_STATE | MIIM_ID | MIIM_SUBMENU | MIIM_DATA;
			mii.dwTypeData = szString;
			mii.cch = NELEMENTS(szString);
			GetMenuItemInfo(hWabMenu, 0, TRUE, &mii);
			InsertMenuItem(hSysMenu, 0, TRUE, &mii);
			mii.dwTypeData = szString;
			mii.cch = NELEMENTS(szString);
			GetMenuItemInfo(hWabMenu, 1, TRUE, &mii);
			InsertMenuItem(hSysMenu, 0, TRUE, &mii);
			CheckMenuItem(hSysMenu, IDM_WABSYSMENU_HALFTONE, MF_BYCOMMAND | (np2wabcfg.halftone ? MF_CHECKED : MF_UNCHECKED));
			DestroyMenu(hWabMenu);

			break;
			
		case WM_SYSCOMMAND:
			switch(wParam) {
				case IDM_WABSYSMENU_RESETSIZE:
					np2wab_resetscreensize();
					break;

				case IDM_WABSYSMENU_HALFTONE:
					np2wabcfg.halftone = !np2wabcfg.halftone;
					hSysMenu = GetSystemMenu(hWnd, FALSE);
					CheckMenuItem(hSysMenu, IDM_WABSYSMENU_HALFTONE, MF_BYCOMMAND | (np2wabcfg.halftone ? MF_CHECKED : MF_UNCHECKED));
					scalemode = np2wab.wndWidth!=np2wab.realWidth || np2wab.wndHeight!=np2wab.realHeight;
					if(scalemode){
						SetStretchBltMode(np2wabwnd.hDCWAB, np2wabcfg.halftone ? HALFTONE : COLORONCOLOR);
						SetBrushOrgEx(np2wabwnd.hDCWAB , 0 , 0 , NULL);
					}
					break;

				default:
					return(DefWindowProc(hWnd, msg, wParam, lParam));
			}
			break;

		case WM_MOVE:
			GetWindowRect(hWnd, &rc);
			if(np2wabwnd.multiwindow && !IsZoomed(hWnd) && !IsIconic(hWnd) && IsWindowVisible(hWnd)){
				np2wabcfg.posx = rc.left;
				np2wabcfg.posy = rc.top;
			}
			break;

		case WM_ENTERSIZEMOVE:
			break;

		case WM_MOVING:
			break;
			
		case WM_SIZE:
		case WM_SIZING:
			GetClientRect( hWnd, &rc );
			np2wab.wndWidth = rc.right - rc.left;
			np2wab.wndHeight = rc.bottom - rc.top;
			break;

		case WM_EXITSIZEMOVE:
			break;

		case WM_KEYDOWN:
			SendMessage(np2wabwnd.hWndMain, msg, wParam, lParam); // 必殺丸投げ
			break;

		case WM_KEYUP:
			SendMessage(np2wabwnd.hWndMain, msg, wParam, lParam); // 必殺丸投げ
			break;

		case WM_SYSKEYDOWN:
			SendMessage(np2wabwnd.hWndMain, msg, wParam, lParam); // 必殺丸投げ
			break;

		case WM_SYSKEYUP:
			SendMessage(np2wabwnd.hWndMain, msg, wParam, lParam); // 必殺丸投げ
			break;

		case WM_MOUSEMOVE:
			SendMessage(np2wabwnd.hWndMain, msg, wParam, lParam); // 必殺丸投げ
			break;

		case WM_LBUTTONDOWN:
			if(np2wabwnd.multiwindow){
				SendMessage(np2wabwnd.hWndMain, msg, wParam, lParam); // やはり丸投げ
			}
			break;

		case WM_LBUTTONUP:
			if(np2wabwnd.multiwindow){
				SendMessage(np2wabwnd.hWndMain, msg, wParam, lParam); // ここも丸投げ
			}
			break;

		case WM_RBUTTONDOWN:
			if(np2wabwnd.multiwindow){
				SendMessage(np2wabwnd.hWndMain, msg, wParam, lParam); // そのまま丸投げ
			}
			break;

		case WM_RBUTTONUP:
			if(np2wabwnd.multiwindow){
				SendMessage(np2wabwnd.hWndMain, msg, wParam, lParam); // なんでも丸投げ
			}
			break;

		case WM_MBUTTONDOWN:
			SetForegroundWindow(np2wabwnd.hWndMain);
			SendMessage(np2wabwnd.hWndMain, msg, wParam, lParam); // とりあえず丸投げ
			break;

		case WM_CLOSE:
			return 0;

		case WM_DESTROY:
			break;

		default:
			return(DefWindowProc(hWnd, msg, wParam, lParam));
	}
	return(0L);
}

/**
 * ウィンドウアクセラレータ画面転送
 *  別窓モード: GDI Device Independent Bitmap -> GDI Window
 *  統合モード: GDI Device Independent Bitmap -> Direct3D/DirectDraw WAB surface ( call scrnmng_blthdc() )
 */
void np2wab_drawWABWindow(HDC hdc)
{
	int scalemode = 0;
	int srcwidth = np2wab.realWidth;
	int srcheight = np2wab.realHeight;
	if(ga_lastrealwidth != srcwidth || ga_lastrealheight != srcheight){
		// 解像度が変わっていたらウィンドウサイズも変える
		if(!ga_reqChangeWindowSize){
			np2wab.paletteChanged = 1;
			np2wab_setScreenSizeMT(srcwidth, srcheight);
		}
		if(!np2wabwnd.ready) return;
	}
	if(np2wabwnd.multiwindow){ // 別窓モード判定
		scalemode = np2wab.wndWidth!=srcwidth || np2wab.wndHeight!=srcheight;
		if(ga_lastscalemode!=scalemode){ // 画面スケールが変わりました
			if(scalemode){
				// 通常はCOLORONCOLOR。HALFTONEにも設定できるけど拡大の補間が微妙･･･
				SetStretchBltMode(np2wabwnd.hDCWAB, np2wabcfg.halftone ? HALFTONE : COLORONCOLOR);
				SetBrushOrgEx(np2wabwnd.hDCWAB , 0 , 0 , NULL);
			}else{
				SetStretchBltMode(np2wabwnd.hDCWAB, BLACKONWHITE);
			}
			ga_lastscalemode = scalemode;
			np2wab.paletteChanged = 1;
		}
		if(scalemode){
			// 拡大縮小転送。とりあえず画面比は維持
			if(np2wab.wndWidth * srcheight > srcwidth * np2wab.wndHeight){
				// 横長
				int dstw = srcwidth * np2wab.wndHeight / srcheight;
				int dsth = np2wab.wndHeight;
				int mgnw = (np2wab.wndWidth - dstw);
				int shx = 0;
				if(mgnw&0x1) shx = 1;
				mgnw = mgnw>>1;
				BitBlt(np2wabwnd.hDCWAB, 0, 0, mgnw, np2wab.wndHeight, NULL, 0, 0, BLACKNESS);
				BitBlt(np2wabwnd.hDCWAB, np2wab.wndWidth-mgnw-shx, 0, mgnw+shx, np2wab.wndHeight, NULL, 0, 0, BLACKNESS);
				StretchBlt(np2wabwnd.hDCWAB, mgnw, 0, dstw, dsth, np2wabwnd.hDCBuf, 0, 0, srcwidth, srcheight, SRCCOPY);
			}else if(np2wab.wndWidth * srcheight < srcwidth * np2wab.wndHeight){
				// 縦長
				int dstw = np2wab.wndWidth;
				int dsth = srcheight * np2wab.wndWidth / srcwidth;
				int mgnh = (np2wab.wndHeight - dsth);
				int shy = 0;
				if(mgnh&0x1) shy = 1;
				mgnh = mgnh>>1;
				BitBlt(np2wabwnd.hDCWAB, 0, 0, np2wab.wndWidth, mgnh, NULL, 0, 0, BLACKNESS);
				BitBlt(np2wabwnd.hDCWAB, 0, np2wab.wndHeight-mgnh-shy, np2wab.wndWidth, mgnh+shy, NULL, 0, 0, BLACKNESS);
				StretchBlt(np2wabwnd.hDCWAB, 0, mgnh, dstw, dsth, np2wabwnd.hDCBuf, 0, 0, srcwidth, srcheight, SRCCOPY);
			}else{
				StretchBlt(np2wabwnd.hDCWAB, 0, 0, np2wab.wndWidth, np2wab.wndHeight, np2wabwnd.hDCBuf, 0, 0, srcwidth, srcheight, SRCCOPY);
			}
		}else{
			// 等倍転送
			BitBlt(np2wabwnd.hDCWAB, 0, 0, srcwidth, srcheight, np2wabwnd.hDCBuf, 0, 0, SRCCOPY);
		}
	}else{
		// DirectDrawに描かせる
		//scrnmng_blthdc(np2wabwnd.hDCBuf);
		// DirectDraw Surfaceに転送
		scrnmng_blthdc(np2wabwnd.hDCBuf);
	}
}

/**
 * 同期描画（ga_threadmodeが偽）
 */
void np2wab_drawframe()
{
	if(!ga_threadmode){
		if(np2wabwnd.ready && np2wabwnd.hWndWAB!=NULL && (np2wab.relay&0x3)!=0){
			// マルチスレッドじゃない場合はここで描画処理
			np2wabwnd.drawframe();
			np2wab_drawWABWindow(np2wabwnd.hDCBuf);
			if(!np2wabwnd.multiwindow){
				scrnmng_bltwab();
				scrnmng_update();
			}
		}
	}else{
		if(np2wabwnd.hWndWAB!=NULL){
			if(ga_reqChangeWindowSize){
				// 画面サイズ変更要求が来ていたら画面サイズを変える
				np2wab_setScreenSize(ga_reqChangeWindowSize_w, ga_reqChangeWindowSize_h);
				ga_lastrealwidth = ga_reqChangeWindowSize_w;
				ga_lastrealheight = ga_reqChangeWindowSize_h;
				ga_reqChangeWindowSize = 0;
				np2wabwnd.ready = 1;
			}
			if(np2wabwnd.ready && (np2wab.relay&0x3)!=0){
				if(ga_screenupdated){
					wab_enter_criticalsection();
					//int suspendcounter = 0;
					//if(ga_hThread) suspendcounter = SuspendThread(ga_hThread);
					if(!np2wabwnd.multiwindow){
						//np2wab_drawWABWindow(np2wabwnd.hDCBuf); // ga_ThreadFuncでやる
						scrnmng_bltwab();
					}
					ga_screenupdated = 0;
					if(!np2wabwnd.multiwindow){
						// XXX: ウィンドウアクセラレータ動作中は内蔵グラフィックを描画しない
						scrnmng_update();
					}
					wab_leave_criticalsection();
					//if(ga_hThread){
					//	int i;
					//	for(i=0;i<suspendcounter+1;i++){
					//		ResumeThread(ga_hThread);
					//	}
					//}
				//}else{
				//	int suspendcounter = 0;
				//	//if(ga_hThread){
				//	//	int i;
				//	//	suspendcounter = SuspendThread(ga_hThread);
				//	//	for(i=0;i<suspendcounter+1;i++){
				//	//		ResumeThread(ga_hThread);
				//	//	}
				//	//}
				}
			}
		}
	}
}
/**
 * 非同期描画（ga_threadmodeが真）
 */
unsigned int __stdcall ga_ThreadFunc(LPVOID vdParam) {
	DWORD time = GetTickCount();
	int timeleft = 0;
	while (!ga_exitThread && ga_threadmode) {
		if(!ga_screenupdated){
			wab_enter_criticalsection();
			wab_leave_criticalsection();
			if(np2wabwnd.ready && np2wabwnd.hWndWAB!=NULL && np2wabwnd.drawframe!=NULL && (np2wab.relay&0x3)!=0){
				np2wabwnd.drawframe();
				np2wab_drawWABWindow(np2wabwnd.hDCBuf); 
				// 画面転送待ち
				ga_screenupdated = 1;
				//if(!ga_exitThread) SuspendThread(ga_hThread);
			}else{
				// 描画しないのに高速でぐるぐる回しても仕方ないのでスリープ
				ga_screenupdated = 1;
				//if(!ga_exitThread) SuspendThread(ga_hThread);
			}
		}else{
			Sleep(8);
		}
	}
	ga_threadmode = 0;
	return 0;
}

/**
 * 画面出力リレー制御
 */
static void IOOUTCALL np2wab_ofac(UINT port, REG8 dat) {
	TRACEOUT(("WAB: out FACh set relay %04X d=%02X", port, dat));
	dat = dat & ~0xfc;
	if(np2wab.relaystateext != dat){
		np2wab.relaystateext = dat & 0x3;
		np2wab_setRelayState(np2wab.relaystateint|np2wab.relaystateext); // リレーはORで･･･（暫定やっつけ修正）
	}
	(void)port;
	(void)dat;
}
static REG8 IOINPCALL np2wab_ifac(UINT port) {
	//TRACEOUT(("WAB: inp FACh get relay %04X", port));
	return 0xfc | np2wab.relaystateext;
}

// NP2起動時の処理
void np2wab_init(HINSTANCE hInstance, HWND hWndMain)
{
	WNDCLASSEX wcex = {0};
	HDC hdc;

	// クリティカルセクション初期化
	wab_init_criticalsection();

	//// 専用INIセクション読み取り
	//wabwin_readini();
	
	// 後々要る物を保存しておく
	ga_hInstance = hInstance;
	np2wabwnd.hWndMain = hWndMain;
	
	// ウィンドウアクセラレータ別窓を作る
	wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW | (np2wabwnd.multiwindow ? CS_DBLCLKS : 0);
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = ga_hInstance;
    wcex.hIcon = LoadIcon(ga_hInstance, MAKEINTRESOURCE(IDI_ICON1));
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
    wcex.lpszClassName = (TCHAR*)g_Name;
	if(!RegisterClassEx(&wcex)) return;
	np2wabwnd.hWndWAB = CreateWindowEx(
			0, 
			g_Name, g_Name, 
			WS_OVERLAPPEDWINDOW,
			np2wabcfg.posx, np2wabcfg.posy, 
			640, 480, 
			np2wabwnd.multiwindow ? NULL : np2wabwnd.hWndMain, 
			NULL, ga_hInstance, NULL
		);
	if(!np2wabwnd.hWndWAB) return;

	// HWNDとかHDCとかバッファ用ビットマップとかを先に作っておく
	np2wabwnd.hDCWAB = GetDC(np2wabwnd.hWndWAB);
	hdc = np2wabwnd.multiwindow ? GetDC(NULL) : np2wabwnd.hDCWAB;
	np2wabwnd.hBmpBuf = CreateCompatibleBitmap(hdc, WAB_MAX_WIDTH, WAB_MAX_HEIGHT);
	np2wabwnd.hDCBuf = CreateCompatibleDC(hdc);
	SelectObject(np2wabwnd.hDCBuf, np2wabwnd.hBmpBuf);

}
// リセット時に呼ばれる？
void np2wab_reset(const NP2CFG *pConfig)
{
	// マルチスレッドモードなら先にスレッド処理を終了させる
	if(ga_threadmode && ga_hThread){
		ga_exitThread = 1;
		//while(((int)ResumeThread(ga_hThread))>0);
		//while(WaitForSingleObject(ga_hThread, 200)==WAIT_TIMEOUT){
		//	ResumeThread(ga_hThread);
		//}
		if(WaitForSingleObject(ga_hThread, 4000)==WAIT_TIMEOUT){
			TerminateThread(ga_hThread, 0);
		}
		CloseHandle(ga_hThread);
		ga_hThread = NULL;
		ga_exitThread = 0;
	}

	// 描画を停止して設定初期化
	np2wabwnd.ready = 0;
	ga_lastscalemode = 0;
	ga_lastrealwidth = 0;
	ga_lastrealheight = 0;
	ga_screenupdated = 0;
	np2wab.lastWidth = 0;
	np2wab.lastHeight = 0;
	np2wab.realWidth = 0;
	np2wab.realHeight = 0;
	np2wab.relaystateint = 0;
	np2wab_setRelayState(np2wab.relaystateint|np2wab.relaystateext);

	// 設定値更新とか
	np2wab.wndWidth = 640;
	np2wab.wndHeight = 480;
	np2wab.fps = 60;
	ga_lastwabwidth = 640;
	ga_lastwabheight = 480;
	ga_reqChangeWindowSize = 0;
	
	// パレットを更新させる
	np2wab.paletteChanged = 1;
}
// リセット時に呼ばれる？（np2net_resetより後・iocore_attach～が使える）
void np2wab_bind(void)
{
	DWORD dwID;

	// マルチスレッドモードなら先にスレッド処理を終了させる
	if(ga_threadmode && ga_hThread){
		ga_exitThread = 1;
		//while(((int)ResumeThread(ga_hThread))>0);
		//while(WaitForSingleObject(ga_hThread, 200)==WAIT_TIMEOUT){
		//	ResumeThread(ga_hThread);
		//}
		if(WaitForSingleObject(ga_hThread, 4000)==WAIT_TIMEOUT){
			TerminateThread(ga_hThread, 0);
		}
		CloseHandle(ga_hThread);
		ga_hThread = NULL;
		ga_exitThread = 0;
	}
	
	// I/Oポートマッピング（FAChは内蔵リレー切り替え）
	iocore_attachout(0xfac, np2wab_ofac);
	iocore_attachinp(0xfac, np2wab_ifac);
	
	// 設定値更新とか
	np2wabwnd.multiwindow = np2wabcfg.multiwindow;
	ga_threadmode = np2wabcfg.multithread;
	
	//// 画面消去
	//BitBlt(np2wabwnd.hDCBuf , 0 , 0 , WAB_MAX_WIDTH , WAB_MAX_HEIGHT , NULL , 0 , 0 , BLACKNESS);
	//scrnmng_blthdc(np2wabwnd.hDCBuf);
	
	// マルチスレッドモードならスレッド開始
	if(ga_threadmode){
		ga_hThread  = (HANDLE)_beginthreadex(NULL , 0 , ga_ThreadFunc  , NULL , 0 , &dwID);
	}
	
	// パレットを更新させる
	np2wab.paletteChanged = 1;

	// 描画再開
	np2wabwnd.ready = 1;
}
void np2wab_unbind(void)
{
	iocore_detachout(0xfac);
	iocore_detachinp(0xfac);
}
// NP2終了時の処理
void np2wab_shutdown()
{
	// マルチスレッドモードなら先にスレッド処理を終了させる
	ga_exitThread = 1;
	//while(((int)ResumeThread(ga_hThread))>0);
	//if(WaitForSingleObject(ga_hThread, 1000)==WAIT_TIMEOUT){
	//	ResumeThread(ga_hThread);
	//}
	if(WaitForSingleObject(ga_hThread, 4000)==WAIT_TIMEOUT){
		TerminateThread(ga_hThread, 0); // 諦めて強制終了
	}
	CloseHandle(ga_hThread);
	ga_hThread = NULL;

	// いろいろ解放
	DeleteDC(np2wabwnd.hDCBuf);
	DeleteObject(np2wabwnd.hBmpBuf);
	ReleaseDC(np2wabwnd.hWndWAB, np2wabwnd.hDCWAB);
	np2wabwnd.hDCWAB = NULL;
	DestroyWindow(np2wabwnd.hWndWAB);
	np2wabwnd.hWndWAB = NULL;
	
	// クリティカルセクション破棄
	wab_delete_criticalsection();

	//// 専用INIセクション書き込み
	//wabwin_writeini();
}

// 内蔵ディスプレイ切り替えリレー状態を設定する。stateのbit0は外部ｱｸｾﾗ(=1)/内蔵ｱｸｾﾗ(=0)切替、bit1は内蔵ｱｸｾﾗ(=1)/98ｸﾞﾗﾌ(=0)切替。他は0。
// 外部・内部の区別をしていないので事実上どちらかのビットが1ならアクセラレータ表示になる
void np2wab_setRelayState(REG8 state)
{
	// bit0,1が変化しているか確認
	if((np2wab.relay & 0x3) != (state & 0x3)){
		np2wab.relay = state & 0x3;
		if(state&0x3){
			// リレーがON
			if(!np2cfg.wabasw) soundmng_pcmplay(SOUND_RELAY1, FALSE); // カチッ
			if(np2wabwnd.multiwindow){
				// 別窓モードなら別窓を出す
				ShowWindow(np2wabwnd.hWndWAB, SW_SHOWNOACTIVATE);
				SetWindowPos(np2wabwnd.hWndWAB, HWND_TOP, np2wabcfg.posx, np2wabcfg.posy, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOSENDCHANGING | SWP_SHOWWINDOW);
			}else{
				// 統合モードなら画面を乗っ取る
				np2wab_setScreenSize(ga_lastwabwidth, ga_lastwabheight);
			}
		}else{
			// リレーがOFF
			if(!np2cfg.wabasw) soundmng_pcmplay(SOUND_RELAY1, FALSE); // カチッ
			if(np2wabwnd.multiwindow){
				// 別窓モードなら別窓を消す
				np2wab.lastWidth = 0;
				np2wab.lastHeight = 0;
				ShowWindow(np2wabwnd.hWndWAB, SW_HIDE);
			}else{
				// 統合モードなら画面を戻す
				np2wab.lastWidth = 0;
				np2wab.lastHeight = 0;
				//scrnmng_setwidth(dsync.scrnxpos, dsync.scrnxmax); // XXX: 画面幅を乗っ取る前に戻す
				//scrnmng_setheight(0, dsync.scrnymax); // XXX: 画面高さを乗っ取る前に戻す
				scrnmng_setsize(dsync.scrnxpos, 0, dsync.scrnxmax, dsync.scrnymax);// XXX: 画面サイズを乗っ取る前に戻す
				scrnmng_updatefsres(); // フルスクリーン解像度更新
				mousemng_updateclip(); // マウスキャプチャのクリップ範囲を修正
			}
		}
	}
}

/**
 * ウィンドウアクセラレータ画面をBMPで取得
 */
BRESULT np2wab_getbmp(BMPFILE *lpbf, BMPINFO *lpbi, UINT8 **lplppal, UINT8 **lplppixels) {

	BMPDATA		bd;
	BMPFILE		bf;
	UINT		pos;
	BMPINFO		bi;
	BMPINFO		bitmp;
	UINT		align;
	//int			r;
	//int			x;
	UINT8		*dstpix;
	LPVOID      lpbits;
	UINT8       *buf;
	HBITMAP     hBmpTmp;

	// 24bit固定
	bd.width = np2wab.wndWidth;
	bd.height = np2wab.wndHeight;
	bd.bpp = 24;

	// Bitmap File
	ZeroMemory(&bf, sizeof(bf));
	bf.bfType[0] = 'B';
	bf.bfType[1] = 'M';
	pos = sizeof(BMPFILE) + sizeof(BMPINFO);
	STOREINTELDWORD(bf.bfOffBits, pos);
	CopyMemory(lpbf, &bf, sizeof(bf));

	// Bitmap Info
	bmpdata_setinfo(&bi, &bd);
	STOREINTELDWORD(bi.biClrImportant, 0);
	align = bmpdata_getalign(&bi);
	CopyMemory(lpbi, &bi, sizeof(bi));
	*lplppal = (UINT8*)malloc(0); // freeで解放されても大丈夫なように（大抵NULLが入る）

	*lplppixels = (UINT8*)malloc(bmpdata_getalign(&bi) * bd.height);
	dstpix = *lplppixels;

	// Copy Pixels
	bitmp = bi;
	STOREINTELDWORD(bitmp.biWidth, WAB_MAX_WIDTH);
	STOREINTELDWORD(bitmp.biHeight, WAB_MAX_HEIGHT);
	hBmpTmp = CreateDIBSection(NULL, (LPBITMAPINFO)&bitmp, DIB_RGB_COLORS, &lpbits, NULL, 0);
	GetDIBits(np2wabwnd.hDCBuf, np2wabwnd.hBmpBuf, 0, WAB_MAX_HEIGHT, lpbits, (LPBITMAPINFO)&bitmp, DIB_RGB_COLORS);
	buf = (UINT8*)(lpbits) + (WAB_MAX_HEIGHT - bd.height) * WAB_MAX_WIDTH*bd.bpp/8;
	do {
		CopyMemory(dstpix, buf, np2wab.wndWidth*bd.bpp/8);
		dstpix += align;
		buf += WAB_MAX_WIDTH*bd.bpp/8;
	} while(--bd.height);
	DeleteObject(hBmpTmp);

	return(SUCCESS);
}

/**
 * ウィンドウアクセラレータ画面をBMPで保存
 */
BRESULT np2wab_writebmp(const OEMCHAR *filename) {
	
	FILEH		fh;
	BMPFILE     bf;
	BMPINFO     bi;
	UINT8       *lppal;
	UINT8       *lppixels;
	int	        pixelssize;
	
	fh = file_create(filename);
	if (fh == FILEH_INVALID) {
		goto sswb_err1;
	}
	
	np2wab_getbmp(&bf, &bi, &lppal, &lppixels);
	
	// Bitmap File
	if (file_write(fh, &bf, sizeof(bf)) != sizeof(bf)) {
		goto sswb_err3;
	}

	// Bitmap Info (パレット不要)
	if (file_write(fh, &bi, sizeof(bi)) != sizeof(bi)) {
		goto sswb_err3;
	}
	
	// Pixels
	pixelssize = bmpdata_getalign(&bi) * LOADINTELDWORD(bi.biHeight);
	if (file_write(fh, lppixels, pixelssize) != pixelssize) {
		goto sswb_err3;
	}

	_MFREE(lppal);
	_MFREE(lppixels);

	file_close(fh);

	return(SUCCESS);
	
sswb_err3:
	_MFREE(lppal);
	_MFREE(lppixels);

//sswb_err2:
//	file_close(fh);
//	file_delete(filename);

sswb_err1:
	return(FAILURE);
}

#endif	/* SUPPORT_WAB */