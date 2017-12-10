#include "stdafx.h"

#include "calc.h"
#include "guisavestate.h"
#include "gui.h"

extern HINSTANCE g_hInst;

static TCHAR save_filename[MAX_PATH];

static INT_PTR CALLBACK DlgSavestateProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	static HWND edtAuthor;
	static HWND edtComment;
	static HWND edtModel, edtRom_version;
	static HWND cmbCompress;
	static HWND chkReadonly;
	static HWND imgPreview;
	static LPCALC lpCalc;
	
	switch (uMsg) {
		case WM_INITDIALOG: {
			lpCalc = (LPCALC) lParam;
			edtAuthor = GetDlgItem(hwndDlg, IDC_EDTSAVEAUTHOR);
			edtComment = GetDlgItem(hwndDlg, IDC_EDTSAVECOMMENT);
			cmbCompress = GetDlgItem(hwndDlg, IDC_CBOSAVECOMPRESS);
			chkReadonly = GetDlgItem(hwndDlg, IDC_CHKSAVEREADONLY);
			imgPreview = GetDlgItem(hwndDlg, IDC_IMGSAVEPREVIEW);
			edtModel = GetDlgItem(hwndDlg, IDC_EDTSAVEMODEL);
			edtRom_version = GetDlgItem(hwndDlg, IDC_EDTSAVEROMVER);
			
			SendMessage(cmbCompress, CB_ADDSTRING, 0, (LPARAM) _T("None"));
			SendMessage(cmbCompress, CB_ADDSTRING, 0, (LPARAM) _T("Zlib"));
			SendMessage(cmbCompress, CB_SETCURSEL, 1, (LPARAM) 0);
			
#ifdef _UNICODE
			size_t numConv;
			TCHAR romBuffer[16];
			mbstowcs_s(&numConv, romBuffer, lpCalc->rom_version, 16);
			SendMessage(edtRom_version, WM_SETTEXT, 0, (LPARAM) romBuffer);
#else
			SendMessage(edtRom_version, WM_SETTEXT, 0, (LPARAM) lpCalc->rom_version);
#endif
			SendMessage(edtModel, WM_SETTEXT, 0, (LPARAM)calc_get_model_string(lpCalc->model));
			
			LCDBase_t *lcd = lpCalc->cpu.pio.lcd;

			HBITMAP hbmPreview = CreateBitmap(lcd->display_width, lcd->height, 1, 32, NULL);
			
			HDC hdc = CreateCompatibleDC(NULL);
			HBITMAP hbmOld = (HBITMAP) SelectObject(hdc, hbmPreview);
			

			unsigned char *image = lcd->image(lcd);
			StretchDIBits(hdc, 0, 0, lcd->display_width, lcd->height,
				0, 0, lcd->display_width, lcd->height,
				image,
				GetLCDColorPalette(lpCalc->model, lcd),
				DIB_RGB_COLORS,
				SRCCOPY);

			free(image);

			SelectObject(hdc, hbmOld);
			DeleteDC(hdc);
			SendMessage(imgPreview, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM) hbmPreview);
			
			TCHAR lpBuffer[32];
			DWORD length = sizeof(lpBuffer);
			GetUserName(lpBuffer, (LPDWORD) &length);
			SendMessage(edtAuthor, WM_SETTEXT, 0, (LPARAM) lpBuffer);
			
			SetFocus(edtAuthor);
			SendMessage(edtAuthor, EM_SETSEL, 0, (LPARAM) -1);
			return FALSE;
		}
		case WM_COMMAND:
			switch (HIWORD(wParam)) {
			case BN_CLICKED:
				switch (LOWORD(wParam)) {
					case IDC_BTNSAVEOK:
						{
							TCHAR author[MAX_SAVESTATE_AUTHOR_LENGTH];
							TCHAR comment[MAX_SAVESTATE_COMMENT_LENGTH];
							SendMessage(edtAuthor, WM_GETTEXT, MAX_SAVESTATE_AUTHOR_LENGTH, (LPARAM) author);
							SendMessage(edtComment, WM_GETTEXT, MAX_SAVESTATE_COMMENT_LENGTH, (LPARAM) comment);
							int compression = (int) SendMessage(cmbCompress, CB_GETCURSEL, 0, 0);

							SAVESTATE_t *savestate = SaveSlot(lpCalc, author, comment);
							WriteSave(save_filename, savestate, compression);
							StringCbCopy(lpCalc->rom_path, sizeof(lpCalc->rom_path), save_filename);
							FreeSave(savestate);
						}
					case IDC_BTNSAVECANCEL:
						EndDialog(hwndDlg, wParam);
						return TRUE;
				}
			}
			return FALSE;
		case WM_CLOSE:
			EndDialog(hwndDlg, 0);
			return TRUE;
		default:
			return FALSE;
	}
}

INT_PTR gui_savestate(HWND hwndParent, TCHAR *filename, LPCALC lpCalc) {
	InitCommonControls();
	StringCbCopy(save_filename, sizeof(save_filename), filename);
	return DialogBoxParam(g_hInst, MAKEINTRESOURCE(IDD_DLGSAVESTATE), hwndParent, DlgSavestateProc, (LPARAM) lpCalc);
}

