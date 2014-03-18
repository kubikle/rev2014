#include <windows.h>
#include <tchar.h>

#include "../resource.h"
#include "../include/options.h"
#include <Strsafe.h>

extern HWND g_hWnd;

typedef struct {
	int w,h;
} RES;

#define MAXNUMRES 4096

int nRes = 0;
RES ress[MAXNUMRES];
RES quality[8];

double aspectRatio[8];

DemoOptions	g_options;

BOOL CALLBACK OptionsDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	int j = 0;

	switch(Message)
	{
	case WM_INITDIALOG:
		char s[500];

		DEVMODE d;
		EnumDisplaySettings(NULL,0,&d);

		while(TRUE) {
			BOOL h = EnumDisplaySettings(NULL,j++,&d);
			if (!h || nRes>MAXNUMRES) break;

			/*** You can use this following line to select only certain aspect ratios ***/
			//        if ((d.dmPelsWidth * 3) / 4 != d.dmPelsHeight) continue;

			if (d.dmBitsPerPel != g_options.bpp) continue;
			if (!nRes || ress[nRes-1].w != d.dmPelsWidth || ress[nRes-1].h != d.dmPelsHeight) {
				ress[nRes].w = d.dmPelsWidth;
				ress[nRes].h = d.dmPelsHeight;
				nRes++;
				_snprintf_s(s,500,"%d * %d",d.dmPelsWidth,d.dmPelsHeight);
				SendDlgItemMessage(hwnd, IDC_RESOLUTION, CB_ADDSTRING, 0, (LPARAM)s);
			}
		}	

		SendDlgItemMessage(hwnd, IDC_ASPECTRATIO, CB_ADDSTRING, 0,   (LPARAM)TEXT("4:3"));
		SendDlgItemMessage(hwnd, IDC_ASPECTRATIO, CB_ADDSTRING, 0,   (LPARAM)TEXT("16:9"));
		SendDlgItemMessage(hwnd, IDC_ASPECTRATIO, CB_ADDSTRING, 0,   (LPARAM)TEXT("16:10"));
		SendDlgItemMessage(hwnd, IDC_ASPECTRATIO, CB_ADDSTRING, 0,   (LPARAM)TEXT("5:4"));

		SendDlgItemMessage(hwnd, IDC_ASPECTRATIO, CB_ADDSTRING, 0,   (LPARAM)TEXT("3:4"));
		SendDlgItemMessage(hwnd, IDC_ASPECTRATIO, CB_ADDSTRING, 0,   (LPARAM)TEXT("9:16"));
		SendDlgItemMessage(hwnd, IDC_ASPECTRATIO, CB_ADDSTRING, 0,   (LPARAM)TEXT("10:16"));
		SendDlgItemMessage(hwnd, IDC_ASPECTRATIO, CB_ADDSTRING, 0,   (LPARAM)TEXT("4:5"));
		aspectRatio[0] = 4.0/3.0;
		aspectRatio[1] = 16.0/9.0;
		aspectRatio[2] = 16.0/10.0;
		aspectRatio[3] = 5.0/4.0;

		aspectRatio[4] = 3.0/4.0;
		aspectRatio[5] = 9.0/16.0;
		aspectRatio[6] = 10.0/16.0;
		aspectRatio[7] = 4.0/5.0;
		
		SendDlgItemMessage(hwnd, IDC_ASPECTRATIO, CB_SETCURSEL, 0, 0);
		
		
		for(int i = 0; i < sizeof(quality)/sizeof(quality[0]); i++) {
			quality[i].w = 320 *(i+1); 
			quality[i].h = 180 *(i+1);
			_snprintf_s(s,500,"%d * %d",quality[i].w,quality[i].h);
			SendDlgItemMessage(hwnd, IDC_QUALITY, CB_ADDSTRING, 0,   (LPARAM)s);
		}
				
		SendDlgItemMessage(hwnd, IDC_QUALITY, CB_SETCURSEL, 0, 0);

		for (int i=0; i<nRes; i++) {
			if (ress[i].w==GetSystemMetrics(SM_CXSCREEN) && ress[i].h==GetSystemMetrics(SM_CYSCREEN)) {
				SendDlgItemMessage(hwnd, IDC_RESOLUTION, CB_SETCURSEL, i, 0);
			}
		}

		if (g_options.fullscreen) {
			CheckDlgButton(hwnd, IDC_FULLSCREEN, BST_CHECKED);
		}


		return TRUE;
	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDOK:
			g_options.fullscreen = IsDlgButtonChecked(hwnd, IDC_FULLSCREEN);
			g_options.dWidth = ress[SendDlgItemMessage(hwnd, IDC_RESOLUTION, CB_GETCURSEL, 0, 0)].w;
			g_options.dHeight= ress[SendDlgItemMessage(hwnd, IDC_RESOLUTION, CB_GETCURSEL, 0, 0)].h;
			g_options.aspectRatio = aspectRatio[SendDlgItemMessage(hwnd, IDC_ASPECTRATIO, CB_GETCURSEL, 0, 0)];
			g_options.iWidth = quality[SendDlgItemMessage(hwnd, IDC_QUALITY, CB_GETCURSEL, 0, 0)].w;
			g_options.iHeight= quality[SendDlgItemMessage(hwnd, IDC_QUALITY, CB_GETCURSEL, 0, 0)].h;

			EndDialog(hwnd, IDOK);
			break;
		case IDCANCEL:
			EndDialog(hwnd, IDCANCEL);
			break;
		}
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

void InitOptions() {
	int ret = DialogBox(GetModuleHandle(NULL), 
		MAKEINTRESOURCE(IDD_ABOUTBOX), g_hWnd, OptionsDlgProc);
	if(ret == IDCANCEL) {
		exit(EXIT_SUCCESS);
	}
}

void CalculateScreenSize() 
{
	double demoAspect = g_options.iHeight / g_options.iWidth;
	double realAspect = g_options.dHeight / g_options.dWidth;

	if (realAspect > demoAspect) {
		g_options.sWidth = g_options.dWidth;
		g_options.sHeight = g_options.dWidth * demoAspect;
	} else {
		g_options.sWidth = g_options.dHeight / demoAspect;
		g_options.sHeight = g_options.dHeight;
	}

	if (g_options.sWidth < g_options.dWidth) {
		g_options.offsetX = ( g_options.dWidth - g_options.sWidth ) * 0.5;
	}

	if (g_options.sHeight < g_options.dHeight) {
		g_options.offsetY = ( g_options.dHeight - g_options.sHeight ) * 0.5;
	}
}