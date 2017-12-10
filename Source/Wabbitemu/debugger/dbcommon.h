#ifndef DBCOMMON_H
#define DBCOMMON_H

#include "gui.h"
#include "calc.h"

#include "dbreg.h"

typedef enum {
	HEX2,
	HEX4,
	FLOAT2,
	FLOAT4,
	DEC3,
	DEC5,
	BIN8,
	BIN16,
	CHAR1,
} VALUE_FORMAT;

typedef enum {
	HEX,
	DEC,
	BIN,
} DISPLAY_BASE;

typedef enum {
	REGULAR,			//view paged memory
	FLASH,				//view all flash pages
	RAM,				//view all ram pages
} ViewType;

typedef struct {
	int total;
	BOOL state[32];
} ep_state;

void position_goto_dialog(HWND hGotoDialog);
int get_value(HWND hwndParent);
INT_PTR CALLBACK GotoDialogProc(HWND hwndDlg, UINT Message, WPARAM wParam, LPARAM lParam);
int ValueSubmit(HWND hwndDlg, void *loc, int size, int max_value = INT_MAX);
void DrawItemSelection(HDC hdc, RECT *r, BOOL active, COLORREF breakpoint, BYTE opacity);
const TCHAR * byte_to_binary(int x, BOOL isWord = FALSE);
int xtoi(const TCHAR *xs);
int StringToValue(TCHAR *str);

#define Debug_UpdateWindow(hwnd) SendMessage(hwnd, WM_USER, DB_UPDATE, 0)
#define Debug_CreateWindow(hwnd) SendMessage(hwnd, WM_USER, DB_CREATE, 0)
#define Debug_GotoAddr(hwnd, goto_addr) SendMessage(hwnd, WM_USER, DB_GOTO_ADDR, (LPARAM) goto_addr)

static const TCHAR* DisplayTypeString = _T("Disp_Type");

void SubclassEdit(HWND hwndEdt, HFONT hfontLucida, int edit_width, VALUE_FORMAT format);

#define EN_CANCEL 0x9999

#endif /* DBCOMMON_H */
