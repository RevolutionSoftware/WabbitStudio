#include "stdafx.h"

#include "dbmonitor.h"
#include "guidebug.h"
#include "calc.h"
#include "resource.h"
#include "dbcommon.h"
#include "device.h"
#include "savestate.h"

#define PORT_MIN_COL_WIDTH 40
#define PORT_ROW_SIZE 15

extern HINSTANCE g_hInst;
#define COLOR_BREAKPOINT		(RGB(230, 160, 180))
#define COLOR_SELECTION			(RGB(153, 222, 253))

// CreateListView: Creates a list-view control in report view.
// Returns the handle to the new control
// TO DO:  The calling procedure should determine whether the handle is NULL, in case 
// of an error in creation.
//
// HINST hInst: The global handle to the application instance.
// HWND  hWndParent: The handle to the control's parent window. 
//
static HWND CreateListView (const HWND hwndParent) 
{
	RECT rcClient;                       // The parent window's client area.

	GetClientRect (hwndParent, &rcClient); 

	// Create the list-view window in report view with label editing enabled.
	const HWND hWndListView = CreateWindow(WC_LISTVIEW, 
									 _T(""),
									 WS_CHILD | LVS_REPORT | LVS_SINGLESEL | LVS_NOSORTHEADER | WS_VISIBLE,
									 0, 0,
									 rcClient.right - rcClient.left,
									 rcClient.bottom - rcClient.top,
									 hwndParent,
									 (HMENU)NULL,
									 g_hInst,
									 NULL);
	
	ListView_SetExtendedListViewStyle(hWndListView, LVS_EX_HEADERDRAGDROP | LVS_EX_FULLROWSELECT);
	SetWindowTheme(hWndListView, L"explorer", NULL);

	return hWndListView;
}

// InsertListViewItems: Inserts items into a list view. 
// hWndListView:        Handle to the list-view control.
// cItems:              Number of items to insert.
// Returns TRUE if successful, and FALSE otherwise.
static BOOL InsertListViewItems(const HWND hWndListView, const int cItems)
{
	LVITEM lvI;

	// Initialize LVITEM members that are common to all items.
	lvI.pszText   = LPSTR_TEXTCALLBACK; // Sends an LVN_GETDISPINFO message.
	lvI.mask      = LVIF_TEXT;
	lvI.stateMask = 0;
	lvI.iSubItem  = 0;
	lvI.state     = 0;

	// Initialize LVITEM members that are different for each item.
	for (int index = 0; index < cItems; index++)
	{
		lvI.iItem = index;
	
		// Insert items into the list.
		if (ListView_InsertItem(hWndListView, &lvI) == -1) {
			return FALSE;
		}
	}

	return TRUE;
}

static void free_duplicate_calc_if_necessary(LPDEBUGWINDOWINFO const lpDebugInfo) {
	if (lpDebugInfo->duplicate_calc == NULL) {
		return;
	}

	calc_slot_free(lpDebugInfo->duplicate_calc);
	free(lpDebugInfo->duplicate_calc);
	lpDebugInfo->duplicate_calc = NULL;
}

static void CloseSaveEdit(LPDEBUGWINDOWINFO const lpDebugInfo) {
	LPCALC const lpCalc = lpDebugInfo->lpCalc;
	if (lpDebugInfo->hwndEditControl == NULL) {
		return;
	}

	TCHAR buf[10];
	Edit_GetText(lpDebugInfo->hwndEditControl, buf, ARRAYSIZE(buf));
	int value = GetWindowLongPtr(lpDebugInfo->hwndEditControl, GWLP_USERDATA);
	const unsigned char row_num = (unsigned char)LOWORD(value);
	//handles getting the user input and converting it to an int
	//can convert bin, hex, and dec
	value = StringToValue(buf);
	if (value == INT_MAX) {
		value = 0;
	}

	const uint8_t byte_value = value & 0xFF;
	const int port_num = lpDebugInfo->port_map[row_num];
	const BOOL output_backup = lpCalc->cpu.output;
	const unsigned char bus_backup = lpCalc->cpu.bus;
	lpCalc->cpu.bus = byte_value;
	lpCalc->cpu.output = TRUE;
	lpCalc->cpu.pio.devices[port_num].code(&lpCalc->cpu, &(lpCalc->cpu.pio.devices[port_num]));
	lpCalc->cpu.output = output_backup;
	lpCalc->cpu.bus = bus_backup;

	free_duplicate_calc_if_necessary(lpDebugInfo);
	lpDebugInfo->duplicate_calc = DuplicateCalc(lpCalc);

	DestroyWindow(lpDebugInfo->hwndEditControl);
	lpDebugInfo->hwndEditControl = NULL;
}

static LRESULT APIENTRY EditSubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) { 
	LPDEBUGWINDOWINFO const lpDebugInfo = (LPDEBUGWINDOWINFO)GetWindowLongPtr(GetParent(GetParent(hwnd)), GWLP_USERDATA);
	switch (uMsg) {
	case WM_KEYDOWN: {
		if (wParam == VK_RETURN) {
			CloseSaveEdit(lpDebugInfo);
		} else if (wParam == VK_ESCAPE) {
			DestroyWindow(hwnd);
			lpDebugInfo->hwndEditControl = NULL;
		} else {
			return CallWindowProc(lpDebugInfo->wpOrigEditProc, hwnd, uMsg, wParam, lParam);
		}
		return TRUE;
	}
	default:
		return CallWindowProc(lpDebugInfo->wpOrigEditProc, hwnd, uMsg, wParam, lParam);
	}
} 

static void on_running_changed(LPCALC lpCalc, LPVOID lParam) {
	const HWND hListView = (HWND)lParam;
	EnableWindow(hListView, !lpCalc->running);
	UpdateWindow(hListView);
}

LRESULT CALLBACK PortMonitorProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {
	static HWND hwndListView;
	LPDEBUGWINDOWINFO lpDebugInfo = (LPDEBUGWINDOWINFO)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	LPCALC lpCalc = NULL;
	if (lpDebugInfo != NULL) {
		lpCalc = lpDebugInfo->lpCalc;
	}

	switch(Message) {
		case WM_CREATE: {
			lpDebugInfo = (LPDEBUGWINDOWINFO)((LPCREATESTRUCT)lParam)->lpCreateParams;
			lpCalc = (LPCALC) lpDebugInfo->lpCalc;
			SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR) lpDebugInfo);

			lpDebugInfo->duplicate_calc = DuplicateCalc(lpCalc);

			hwndListView = CreateListView(hwnd);
			int count = 0;
			for (int i = 0; i < 0xFF; i++) {
				if (lpCalc->cpu.pio.devices[i].active) {
					lpDebugInfo->port_map[count] = i;
					count++;
				}
			}

			LVCOLUMN listCol;
			memset(&listCol, 0, sizeof(LVCOLUMN));
			listCol.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
			listCol.pszText = _T("Port Number");
			listCol.cx = 100;
			ListView_InsertColumn(hwndListView, 0, &listCol);
			listCol.cx = 90;
			listCol.pszText = _T("Hex");
			ListView_InsertColumn(hwndListView, 1, &listCol);
			listCol.cx = 130;
			listCol.pszText = _T("Decimal");
			ListView_InsertColumn(hwndListView, 2, &listCol);
			listCol.cx = 110;
			listCol.pszText = _T("Binary");
			ListView_InsertColumn(hwndListView, 3, &listCol);
			SetWindowFont(hwndListView, lpDebugInfo->hfontSegoe, TRUE);

			InsertListViewItems(hwndListView, count);

			calc_register_event(lpCalc, ROM_RUNNING_EVENT, &on_running_changed, hwndListView);

			Debug_CreateWindow(hwnd);
			return TRUE;
		}
		case WM_SETFOCUS: {
			lpDebugInfo->hwndLastFocus = hwnd;
			return 0;
		}
		case WM_COMMAND: {
			switch (LOWORD(wParam)) {
				case DB_BREAKPOINT: {
					int port = lpDebugInfo->port_map[ListView_GetNextItem(hwndListView, -1, LVNI_SELECTED)];
					lpCalc->cpu.pio.devices[port].breakpoint = !lpCalc->cpu.pio.devices[port].breakpoint;
					break;
				}
				case IDM_PORT_EXIT: {
					SendMessage(hwnd, WM_CLOSE, 0, 0);
					break;
				}
			}
			return FALSE;
		}
		case WM_NOTIFY: {
			switch (((LPNMHDR) lParam)->code) 
			{
				case LVN_ITEMCHANGING:
					CloseSaveEdit(lpDebugInfo);
					lpDebugInfo->hwndEditControl = NULL;
					break;
				case LVN_KEYDOWN: {
					LPNMLVKEYDOWN pnkd = (LPNMLVKEYDOWN) lParam;
					if (lpDebugInfo->hwndEditControl != NULL) {
						if (pnkd->wVKey == VK_ESCAPE) {
							DestroyWindow(lpDebugInfo->hwndEditControl);
							lpDebugInfo->hwndEditControl = NULL;
						} else {
							SendMessage(lpDebugInfo->hwndEditControl, WM_KEYDOWN, pnkd->wVKey, 0);
						}
					}
					break;
				}
				case NM_SETFOCUS:
					lpDebugInfo->hwndLastFocus = hwnd;
					InvalidateRect(hwnd, NULL, TRUE);
					break;
				case NM_DBLCLK: {
					NMITEMACTIVATE *lpnmitem = (NMITEMACTIVATE *)lParam;
					int row_num = lpnmitem->iItem;
					int col_num = lpnmitem->iSubItem;
					//no editing the port num
					if (col_num == 0) {
						return FALSE;
					}

					TCHAR buf[32];
					ListView_GetItemText(hwndListView, row_num, col_num, buf, ARRAYSIZE(buf));
					RECT rc;
					LVITEMINDEX lvii;
					lvii.iItem = row_num;
					lvii.iGroup = 0;
					ListView_GetItemIndexRect(hwndListView, &lvii, col_num, LVIR_BOUNDS, &rc);
					//rc is now the rect we want to use for the edit control
					lpDebugInfo->hwndEditControl = CreateWindow(WC_EDIT, buf,
						WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT| ES_MULTILINE,
						rc.left,
						rc.top,
						rc.right - rc.left,
						rc.bottom - rc.top,
						hwndListView, 0, g_hInst, NULL);
					HWND hwndEditControl = lpDebugInfo->hwndEditControl;
					SetWindowFont(hwndEditControl, lpDebugInfo->hfontSegoe, TRUE);
					lpDebugInfo->wpOrigEditProc = (WNDPROC)SetWindowLongPtr(hwndEditControl, GWLP_WNDPROC, (LONG_PTR)EditSubclassProc);
					Edit_LimitText(hwndEditControl, 9);
					Edit_SetSel(hwndEditControl, 0, _tcslen(buf));
					SetWindowLongPtr(hwndEditControl, GWLP_USERDATA, MAKELPARAM(row_num, col_num));
					SetFocus(hwndEditControl);
					break;
				}
				case LVN_GETDISPINFO: 
				{
					const NMLVDISPINFO *plvdi = (NMLVDISPINFO *)lParam;
					const int port_num = lpDebugInfo->port_map[plvdi->item.iItem];
					const LPCALC lpDuplicateCalc = lpDebugInfo->duplicate_calc;
					if (lpDuplicateCalc == NULL && plvdi->item.iSubItem != 0) {
						StringCchPrintf(plvdi->item.pszText, 10, _T("Error"));
						break;
					}

					lpDuplicateCalc->cpu.input = TRUE;
					lpDuplicateCalc->cpu.pio.devices[port_num].code(
						&lpDuplicateCalc->cpu, &(lpDuplicateCalc->cpu.pio.devices[port_num]));
					switch (plvdi->item.iSubItem)
					{
						case 0:
							StringCchPrintf(plvdi->item.pszText, 10, _T("%02X"), port_num);
							break;
						case 1:
							StringCchPrintf(plvdi->item.pszText, 10, _T("$%02X"), lpDuplicateCalc->cpu.bus);
							break;	
						case 2:
							StringCchPrintf(plvdi->item.pszText, 10, _T("%d"), lpDuplicateCalc->cpu.bus);
							break;
						case 3:
							StringCchPrintf(plvdi->item.pszText, 10, _T("%%%s"), byte_to_binary(lpDuplicateCalc->cpu.bus));
							break;
					}

					break;
				}
				case NM_CUSTOMDRAW:
				{
					const LPNMLVCUSTOMDRAW pListDraw = (LPNMLVCUSTOMDRAW)lParam; 
					switch(pListDraw->nmcd.dwDrawStage) 
					{ 
					case CDDS_PREPAINT: {
						return CDRF_NOTIFYITEMDRAW;
					}
					case CDDS_ITEMPREPAINT: 
					case CDDS_ITEMPREPAINT | CDDS_SUBITEM: {
						const int iRow = (int)pListDraw->nmcd.dwItemSpec;
						if (lpCalc->cpu.pio.devices[lpDebugInfo->port_map[iRow]].breakpoint) {
							// pListDraw->clrText   = RGB(252, 177, 0); 
							pListDraw->clrTextBk = COLOR_BREAKPOINT;
							return CDRF_NEWFONT;
						}

						return CDRF_DODEFAULT;
					}
					default: {
						return CDRF_DODEFAULT;
					}
					}
				}
			}
			return FALSE;
		}
		case WM_SIZE: {
			RECT rc;
			GetClientRect(hwnd, &rc);
			SetWindowPos(hwndListView, NULL, rc.left, rc.top,
				rc.right - rc.left, rc.bottom - rc.top, SWP_NOZORDER);
			return FALSE;
		}
		case WM_USER: {
			switch (wParam) {
				case DB_CREATE:
					break;
				case DB_UPDATE: {
					free_duplicate_calc_if_necessary(lpDebugInfo);
					lpDebugInfo->duplicate_calc = DuplicateCalc(lpCalc);
					InvalidateRect(hwnd, NULL, FALSE);
					break;
				}
			}
			return TRUE;
		}
		case WM_DESTROY: {
			if (lpCalc != NULL) {
				calc_unregister_event(lpCalc, ROM_RUNNING_EVENT, &on_running_changed, hwndListView);
			}

			free_duplicate_calc_if_necessary(lpDebugInfo);
			return FALSE;
		}
		default:
			return DefWindowProc(hwnd, Message, wParam, lParam);
	}
}