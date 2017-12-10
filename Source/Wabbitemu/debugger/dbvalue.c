#include "stdafx.h"

#include "coretypes.h"
#include "dbvalue.h"
#include "print.h"
#include "dbreg.h"
#include "dbcommon.h"
#include "guicontext.h"
#include "guidebug.h"

extern HINSTANCE g_hInst;

static LRESULT CALLBACK ValueProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);

/*
 * Create a value field with a label
 */
HWND CreateValueField(
		HWND hwndParent,
		LPDEBUGWINDOWINFO lpDebugInfo,
		TCHAR *name,
		int label_width,
		void *data,
		size_t size,
		int max_digits,
		VALUE_FORMAT format,
		int max_value)
{
	static BOOL class_registered = FALSE;
	static int ID = 400;
	static HWND hwndTip;

	if (class_registered == FALSE) {
		WNDCLASSEX wcx;
		ZeroMemory(&wcx, sizeof(wcx));

		wcx.cbSize = sizeof(wcx);
		wcx.lpfnWndProc = ValueProc;
		wcx.lpszClassName = VALUE_CLASS_NAME;
		wcx.style = CS_DBLCLKS;
		wcx.hCursor = LoadCursor(NULL, IDC_ARROW);

		RegisterClassEx(&wcx);

		// Create the tooltip
		hwndTip = CreateWindowEx(
				0,
				TOOLTIPS_CLASS,
				NULL, WS_POPUP | TTS_ALWAYSTIP,
				CW_USEDEFAULT, CW_USEDEFAULT,
				CW_USEDEFAULT, CW_USEDEFAULT,
				hwndParent, NULL, g_hInst, NULL);

		SetWindowPos(hwndTip, HWND_TOPMOST,0, 0, 0, 0,
		             SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);



		//SendMessage(hwndTip, TTM_ACTIVATE, TRUE, 0);

		class_registered = TRUE;
	}

	value_field_settings *vfs = (value_field_settings *) malloc(sizeof(*vfs));
	if (vfs == NULL) {
		return NULL;
	}

	ZeroMemory(vfs, sizeof(*vfs));

	vfs->data = data;
	vfs->cxName = label_width;
	vfs->size = size;
	vfs->format = format;
	vfs->max_digits = max_digits;
	vfs->max_value = max_value;
	vfs->hwndTip = hwndTip;
	StringCbCopy(vfs->szName, sizeof(vfs->szName), name);
	vfs->lpDebugInfo = lpDebugInfo;

	// Create the container window
	HWND hwndValue =
		CreateWindowEx(
				0,
				VALUE_CLASS_NAME,
				name,
				WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE,
				0, 0, 1, 1,
				hwndParent,
				(HMENU) ID++,
				g_hInst,
				vfs
		);

	if (hwndValue == NULL) {
		return NULL;
	}

	return hwndValue;
}

static LRESULT CALLBACK ValueProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {
	static TEXTMETRIC tm;

	switch (Message) {
	case WM_CREATE:
	{
		value_field_settings *vfs = (value_field_settings *) ((CREATESTRUCT *)lParam)->lpCreateParams;
		SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR) vfs);

		HDC hdc = GetDC(hwnd);

		SelectObject(hdc, vfs->lpDebugInfo->hfontLucida);
		GetTextMetrics(hdc, &tm);

		ReleaseDC(hwnd, hdc);

		// Add our tool
		ZeroMemory(&vfs->toolInfo, sizeof(TOOLINFO));
		vfs->toolInfo.cbSize = sizeof(TOOLINFO);
		vfs->toolInfo.hwnd = hwnd;
		vfs->toolInfo.uFlags = TTF_SUBCLASS;// | TTF_CENTERTIP;
		vfs->toolInfo.hinst = g_hInst;
		vfs->toolInfo.uId = 1;
	    GetClientRect(hwnd, &vfs->toolInfo.rect);
		vfs->toolInfo.lpszText = vfs->szTip;
	    //SendMessage(vfs->hwndTip, TTM_ADDTOOL, 0, (LPARAM)&vfs->toolInfo);

		SendMessage(hwnd, WM_USER, DB_UPDATE, 0);
		return 0;
	}
	case WM_DESTROY: {
		value_field_settings *vfs = (value_field_settings *) GetWindowLongPtr(hwnd, GWLP_USERDATA);
		free(vfs);
		return 0;
	}
	case WM_MOUSEMOVE:
	{
		value_field_settings *vfs = (value_field_settings *) GetWindowLongPtr(hwnd, GWLP_USERDATA);
		POINT p = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};

		if (PtInRect(&vfs->hot, p)) {
			if (vfs->hot_lit == FALSE) {
				vfs->hot_lit = TRUE;
				InvalidateRect(hwnd, NULL, TRUE);
				UpdateWindow(hwnd);

				TRACKMOUSEEVENT tme;
				tme.cbSize = sizeof(tme);
				tme.dwFlags = TME_LEAVE;
				tme.hwndTrack = hwnd;
				tme.dwHoverTime = 1;
				TrackMouseEvent(&tme);
			}
			return 0;
		}
		// Fall through
	}
	case WM_MOUSELEAVE:
	{
		value_field_settings *vfs = (value_field_settings *) GetWindowLongPtr(hwnd, GWLP_USERDATA);
		if (vfs->hot_lit) {
			vfs->hot_lit = FALSE;

			InvalidateRect(hwnd, NULL, TRUE);
		}
		return 0;
	}

	case WM_KILLFOCUS:
	case WM_SETFOCUS:
	{
		InvalidateRect(hwnd, NULL, TRUE);
		UpdateWindow(hwnd);
		return 0;
	}
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc;

		hdc = BeginPaint(hwnd, &ps);
		value_field_settings *vfs = (value_field_settings *) GetWindowLongPtr(hwnd, GWLP_USERDATA);

		SelectObject(hdc, vfs->lpDebugInfo->hfontLucida);
		SetBkMode(hdc, TRANSPARENT);

		RECT rc;
		GetClientRect(hwnd, &rc);

		FillRect(hdc, &rc, GetStockBrush(WHITE_BRUSH));

		if (!vfs->editing) {
			if (vfs->selected) {
				DrawItemSelection(hdc, &vfs->hot, (hwnd == GetFocus()), FALSE, 255);
			}

			if (vfs->hot_lit) {
				DrawItemSelection(hdc, &vfs->hot, TRUE, FALSE, 130);
				//DrawSelectionRect(hdc, &vfs->hot);
			}

			if (hwnd == GetFocus()) {
				RECT sel;
				CopyRect(&sel, &vfs->hot);
				InflateRect(&sel, -1, -1);
				DrawFocusRect(hdc, &sel);
			}
		}

		if (_tcslen(vfs->szName) > 0) {
			GetClientRect(hwnd, &rc);
			SetTextColor(hdc, DBCOLOR_HILIGHT);
			rc.left += tm.tmAveCharWidth / 2;
			DrawText(hdc, vfs->szName, -1, &rc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
		}

		SetTextColor(hdc, RGB(0, 0, 0));
		CopyRect(&rc, &vfs->hot);
		rc.left += tm.tmAveCharWidth / 2;
		DrawText(hdc, vfs->szValue, -1, &rc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

		EndPaint(hwnd, &ps);
		return 0;
	}
	case WM_SIZE:
	{
		value_field_settings *vfs = (value_field_settings *) GetWindowLongPtr(hwnd, GWLP_USERDATA);
		DWORD dwWidth = tm.tmAveCharWidth * 10;
		if (_tcslen(vfs->szName) == 0)
			dwWidth = tm.tmAveCharWidth * 3;
		if (_tcslen(vfs->szName) > 4)
			dwWidth *= 2;
		dwWidth = vfs->cxName + (vfs->max_digits+1) * tm.tmAveCharWidth;
		SetWindowPos(hwnd, NULL, 0, 0, dwWidth, tm.tmHeight * 4 / 3, SWP_NOMOVE | SWP_NOZORDER);
		InvalidateRect(hwnd, NULL, TRUE);
		UpdateWindow(hwnd);
		return 0;
	}
	case WM_LBUTTONDOWN:
	{
		SendMessage(GetParent(hwnd), WM_USER, VF_DESELECT_CHILDREN, 0);
		value_field_settings *vfs = (value_field_settings *) GetWindowLongPtr(hwnd, GWLP_USERDATA);

		vfs->selected = TRUE;
		SetFocus(hwnd);
		return 0;
	}
	case WM_LBUTTONDBLCLK:
	{
		value_field_settings *vfs = (value_field_settings *) GetWindowLongPtr(hwnd, GWLP_USERDATA);
		// Create the edit window (modify the hot RECT slightly to make text line up perfectly (at 90 dpi)
		vfs->hwndVal =
		CreateWindow(_T("EDIT"), vfs->szValue,
			WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT | ES_MULTILINE,
			vfs->hot.left + 1,
			vfs->hot.top,
			vfs->hot.right - vfs->hot.left - 3,
			vfs->hot.bottom - vfs->hot.top,
			hwnd, 0, g_hInst, NULL);

		SubclassEdit(vfs->hwndVal, vfs->lpDebugInfo->hfontLucida, vfs->max_digits, vfs->format);
		vfs->editing = TRUE;
		InvalidateRect(hwnd, NULL, FALSE);
		UpdateWindow(hwnd);
		return 0;
	}
	case WM_CONTEXTMENU: {
		RECT rc;
		POINT p;
		p.x = GET_X_LPARAM(lParam);
		p.y = GET_Y_LPARAM(lParam);

		GetClientRect(hwnd, &rc);
		ScreenToClient(hwnd, &p);

		SendMessage(hwnd, WM_LBUTTONDOWN, 0, MAKELPARAM(p.x, p.y));

		HMENU hmenu = LoadMenu(g_hInst, MAKEINTRESOURCE(IDR_REGPANE_MENU));
		if (!hmenu) break;
		hmenu = GetSubMenu(hmenu, 0);

		if (!OnContextMenu(hwnd, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), hmenu)) {
			DefWindowProc(hwnd, Message, wParam, lParam);
		}

		DestroyMenu(hmenu);
		return 0;
	}
	case WM_COMMAND:
	{
		value_field_settings *vfs = (value_field_settings *) GetWindowLongPtr(hwnd, GWLP_USERDATA);
		switch (HIWORD(wParam)) {
			case EN_KILLFOCUS:
				if (GetFocus() == hwnd) break;
			case EN_SUBMIT:
				ValueSubmit(vfs->hwndVal, vfs->data, (int) vfs->size, vfs->max_value);
				vfs->editing = FALSE;
				SendMessage(GetParent(hwnd), WM_COMMAND, MAKEWPARAM(0, EN_CHANGE), (LPARAM) hwnd);
				Debug_UpdateWindow(hwnd);
				return 0;
			case EN_CANCEL:
				vfs->editing = FALSE;
				return 0;
			default:
			switch (LOWORD(wParam)) {
				case IDM_REGPANE_EDIT:
					SendMessage(hwnd, WM_LBUTTONDBLCLK, 0, 0);
					return 0;
				case IDM_REGPANE_VIEW_MEM:
					SendMessage(vfs->lpDebugInfo->hDebug, WM_COMMAND, MAKEWPARAM(DB_REGULAR_MEM_GOTO_ADDR, 0), (LPARAM) xtoi(vfs->szValue));
					return 0;
				case IDM_REGPANE_VIEW_DISASM:
					SendMessage(vfs->lpDebugInfo->hDebug, WM_COMMAND, MAKEWPARAM(DB_REGULAR_DISASM_GOTO_ADDR, 0), (LPARAM) xtoi(vfs->szValue));
					return 0;
			}
			
		}
		return 0;
	}
	case WM_USER:
	{
		value_field_settings *vfs = (value_field_settings *) GetWindowLongPtr(hwnd, GWLP_USERDATA);
		switch (wParam) {
		case DB_UPDATE:
			switch (vfs->format) {
			case HEX2:
				StringCbPrintf(vfs->szValue, sizeof(vfs->szValue), _T("%02X"), *((unsigned char *) vfs->data));
				break;
			case HEX4:
				StringCbPrintf(vfs->szValue, sizeof(vfs->szValue), _T("%04X"), *((unsigned short *) vfs->data));
				break;
			case FLOAT2:
				if (vfs->size == sizeof(float))
					StringCbPrintf(vfs->szValue, sizeof(vfs->szValue), _T("%*.2f"), vfs->max_digits, *((float *) vfs->data));
				else
					StringCbPrintf(vfs->szValue, sizeof(vfs->szValue), _T("%*.2lf"), vfs->max_digits, *((double *) vfs->data));
				break;
			case FLOAT4:
				if (vfs->size == sizeof(float))
					StringCbPrintf(vfs->szValue, sizeof(vfs->szValue), _T("%*.4f"), vfs->max_digits, *((float *) vfs->data));
				else
					StringCbPrintf(vfs->szValue, sizeof(vfs->szValue), _T("%*.4f"), vfs->max_digits, *((double *) vfs->data));
				break;
			case DEC3:
				StringCbPrintf(vfs->szValue, sizeof(vfs->szValue), _T("%*d"), vfs->max_digits, *((unsigned int *) vfs->data));
				break;
			case CHAR1:
				StringCbPrintf(vfs->szValue, sizeof(vfs->szValue), _T("%c"), *((unsigned char *) vfs->data));
				break;
			case BIN8:
				StringCbCopy(vfs->szValue, sizeof(vfs->szValue), byte_to_binary(*((unsigned int *) vfs->data)));
				break;
			case BIN16:
				StringCbCopy(vfs->szValue, sizeof(vfs->szValue), byte_to_binary(*((unsigned int *)vfs->data), TRUE));
				break;
			default:
				StringCbPrintf(vfs->szValue, sizeof(vfs->szValue), _T("%d"), *((unsigned int *) vfs->data));
				break;
			}

			GetClientRect(hwnd, &vfs->hot);
			vfs->hot.left = vfs->cxName;

			SendMessage(vfs->hwndTip, TTM_SETMAXTIPWIDTH, 0, 150);
			SetWindowFont(vfs->hwndTip, vfs->lpDebugInfo->hfontLucida, TRUE);
			SendMessage(vfs->hwndTip, TTM_SETDELAYTIME, TTDT_AUTOMATIC, MAKELONG(GetDoubleClickTime() * 5, 0));

			StringCbPrintf(vfs->szTip, sizeof(vfs->szTip), _T("%c: %3d (%s)\n%c: %3d (%s)"),
					vfs->szName[0], ((_TUCHAR *)vfs->data)[1], _T("01010100"),
					vfs->szName[1], ((_TUCHAR *)vfs->data)[0], _T("01010100"));

			vfs->toolInfo.lpszText = vfs->szTip;
			CopyRect(&vfs->toolInfo.rect, &vfs->hot);
			SendMessage(vfs->hwndTip, TTM_SETTOOLINFO, 0, (LPARAM) &vfs->toolInfo);

			InvalidateRect(hwnd, NULL, TRUE);
			UpdateWindow(hwnd);
			break;
		case VF_DESELECT:
			if (vfs->selected != FALSE || vfs->selected != FALSE) {
				vfs->selected = FALSE;
				InvalidateRect(hwnd, NULL, TRUE);
				UpdateWindow(hwnd);
			}
			break;
		default:
			break;
		}
		return 0;
	}
	default:
		return DefWindowProc(hwnd, Message, wParam, lParam);
	}
	return DefWindowProc(hwnd, Message, wParam, lParam);
}
