#include "stdafx.h"

#include "guiwizard.h"
#include "guiskin.h"
#include "guiresource.h"
#include "guioptions.h"
#include "gui.h"
#include "fileutilities.h"
#include "linksendvar.h"
#include "registry.h"
#include "exportvar.h"
#include "osdownloadcallback.h"

#ifdef WITH_TILP
#include <libti\ticables.h>
#include <libti\tifiles.h>
#include <libti\ticalcs.h>
#endif

INT_PTR CALLBACK HelpProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);

static BOOL DownloadOS(OSDownloadCallback *callback, BOOL version);
static void finalize_calc(HWND hwnd, LPCALC lpCalc, TCHAR *buffer);

extern HINSTANCE g_hInst;
static HWND hwndWiz = NULL;
static BOOL use_bootfree = FALSE;
static CalcModel model = INVALID_MODEL;
TCHAR osPath[MAX_PATH];
TCHAR dumperPath[MAX_PATH];
static TCHAR TIConnectPath[MAX_PATH];
static BOOL error = FALSE;
static LPMAINWINDOW lpMainWindow;

LPMAINWINDOW DoWizardSheet(HWND hwndOwner) {
	lpMainWindow = NULL;

	HPROPSHEETPAGE hPropSheet[5];
	PROPSHEETPAGE psp;
	PROPSHEETHEADER psh;

	psp.dwSize = sizeof(PROPSHEETPAGE);
	psp.hInstance = g_hInst;
	psp.dwFlags = PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
	psp.lParam = 0;
	psp.pszHeaderTitle = _T("Wabbitemu ROM Selection");
	psp.pszHeaderSubTitle = _T("Setup");
	psp.pszTemplate = MAKEINTRESOURCE(IDD_SETUP_START);
	psp.pfnDlgProc = SetupStartProc;
	hPropSheet[0] = CreatePropertySheetPage(&psp);

	psp.dwSize = sizeof(PROPSHEETPAGE);
	psp.hInstance = g_hInst;
	psp.dwFlags = PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
	psp.lParam = 0;
	psp.pszHeaderTitle = _T("Calculator Type");
	psp.pszTemplate = MAKEINTRESOURCE(IDD_SETUP_TYPE);
	psp.pfnDlgProc = SetupTypeProc;
	hPropSheet[1] = CreatePropertySheetPage(&psp);

	psp.dwSize = sizeof(PROPSHEETPAGE);
	psp.hInstance = g_hInst;
	psp.dwFlags = PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
	psp.lParam = 0;
	psp.pszHeaderTitle = _T("OS Selection");
	psp.pszTemplate = MAKEINTRESOURCE(IDD_SETUP_TIOS);
	psp.pfnDlgProc = SetupOSProc;
	hPropSheet[2] = CreatePropertySheetPage(&psp);

	psp.dwSize = sizeof(PROPSHEETPAGE);
	psp.hInstance = g_hInst;
	psp.dwFlags = PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
	psp.lParam = 0;
	psp.pszHeaderTitle = _T("Send ROM Dumper");
	psp.pszTemplate = MAKEINTRESOURCE(IDD_SETUP_LOADFILE);
	psp.pfnDlgProc = SetupROMDumperProc;
	hPropSheet[3] = CreatePropertySheetPage(&psp);

	psp.dwSize = sizeof(PROPSHEETPAGE);
	psp.hInstance = g_hInst;
	psp.dwFlags = PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
	psp.lParam = 0;
	psp.pszHeaderTitle = _T("Make ROM");
	psp.pszTemplate = MAKEINTRESOURCE(IDD_SETUP_GETFILE);
	psp.pfnDlgProc = SetupMakeROMProc;
	hPropSheet[4] = CreatePropertySheetPage(&psp);

	DWORD flags;
	// TODO: check common controls version not the OS
	// MSDN says we should also set PSH_WIZARD. this however causes pszCaption
	// to require a wide string. I think it's safer to leave the flag off, than
	// to use a wide string and cast it
	flags = PSH_AEROWIZARD;

	psh.dwSize = sizeof(PROPSHEETHEADER);
	psh.hInstance = g_hInst;
	psh.dwFlags = flags | PSH_WATERMARK;// | PSH_HEADER;
	psh.hwndParent = hwndOwner;
	psh.phpage = hPropSheet;
	psh.pszCaption = _T("Wabbitemu Setup");
	psh.pszbmHeader = NULL;//MAKEINTRESOURCE("A");
	psh.pszbmWatermark = NULL;
	psh.nPages = ARRAYSIZE(hPropSheet);
	psh.nStartPage = 0;
	psh.pfnCallback = NULL;
	psh.pszIcon = NULL;
	PropertySheet(&psh);

	if (error) {
		return NULL;
	}

	return lpMainWindow;
}

INT_PTR CALLBACK SetupStartProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {
	static BOOL inited;
	static HWND hBootFree, hDumpRom, hOwnRom, hInfoText, hEditRom;
	switch (Message) {
		case WM_INITDIALOG: {
			hBootFree = GetDlgItem(hwnd, IDC_RADIO_BOOTFREE);
			hDumpRom = GetDlgItem(hwnd, IDC_RADIO_DUMP_ROM);
			hOwnRom = GetDlgItem(hwnd, IDC_RADIO_OWN_ROM);
			hInfoText = GetDlgItem(hwnd, IDC_INFO_TEXT);
			hEditRom = GetDlgItem(hwnd, IDC_EDT_ROM);
			Button_SetCheck(hOwnRom, BST_CHECKED);
			inited = FALSE;
			return FALSE;
		}
		case WM_COMMAND: {
			switch(HIWORD(wParam)) {
				case BN_CLICKED:
					switch (LOWORD(wParam)) {
						case IDC_BUTTON_BROWSE:
							TCHAR buffer[MAX_PATH];
							if (!BrowseFile(buffer, _T("Known types ( *.sav; *.rom) \0*.sav;*.rom\0Save States  (*.sav)\0*.sav\0\
										ROMs  (*.rom)\0*.rom\0All Files (*.*)\0*.*\0\0"), _T("Please select a ROM or save state"),
										_T("rom"), 0, 1)) {
								Edit_SetText(hEditRom, buffer);
							}
							break;
						case IDC_RADIO_OWN_ROM: {
							Button_Enable(GetDlgItem(hwnd, IDC_BUTTON_BROWSE), TRUE);
							Edit_Enable(hEditRom, TRUE);
							TCHAR buffer[MAX_PATH];
							Edit_GetText(hEditRom, buffer, MAX_PATH);
							if (ValidPath(buffer)) {
								PropSheet_SetWizButtons(GetParent(hwnd), PSWIZB_FINISH);
							} else {
								PropSheet_SetWizButtons(GetParent(hwnd), PSWIZB_DISABLEDFINISH);
							}
							break;
						}
						case IDC_RADIO_BOOTFREE:
						case IDC_RADIO_DUMP_ROM:
							Button_Enable(GetDlgItem(hwnd, IDC_BUTTON_BROWSE), FALSE);
							Edit_Enable(hEditRom, FALSE);
							PropSheet_SetWizButtons(GetParent(hwnd), PSWIZB_NEXT);
							break;
					}
					break;
				case EN_CHANGE: {
					TCHAR buffer[MAX_PATH];
					Edit_GetText(hEditRom, buffer, MAX_PATH);
					if (ValidPath(buffer))
						PropSheet_SetWizButtons(GetParent(hwnd), PSWIZB_FINISH);
					else
						PropSheet_SetWizButtons(GetParent(hwnd), PSWIZB_DISABLEDFINISH);
					break;
				}
			}
			return TRUE;
		}
		case WM_NOTIFY :
		{
			LPNMHDR pnmh = (LPNMHDR) lParam;
			switch(pnmh->code) {
				case PSN_SETACTIVE:
					if (inited)
						PropSheet_SetWizButtons(GetParent(hwnd), PSWIZB_NEXT);
					else {
						PropSheet_SetWizButtons(GetParent(hwnd), PSWIZB_DISABLEDFINISH);
						inited = TRUE;
					}
					break;
				case PSN_WIZNEXT:
					use_bootfree = Button_GetCheck(hBootFree) == BST_CHECKED; 
					break;
				case PSN_WIZFINISH:
					{
						TCHAR szROMPath[MAX_PATH];
						Edit_GetText(hEditRom, szROMPath, ARRAYSIZE(szROMPath));
						lpMainWindow = create_calc_frame_register_events();
						if (lpMainWindow == NULL || lpMainWindow->lpCalc == NULL) {
							MessageBox(hwnd, _T("Unable to create main window"), _T("Error"), MB_OK | MB_ICONERROR);
							SetWindowLongPtr(hwnd, DWLP_MSGRESULT, TRUE);
							return TRUE;
						}

						if (rom_load(lpMainWindow->lpCalc, szROMPath) == FALSE) {
							MessageBox(hwnd, _T("Invalid ROM file"), _T("Error"), MB_OK | MB_ICONERROR);
							SetWindowLongPtr(hwnd, DWLP_MSGRESULT, TRUE);
							return TRUE;
						}
						break;
					}
				case PSN_QUERYCANCEL:
					error = TRUE;
					break;
			}
			return TRUE;
		}
		case WM_DESTROY:
			return FALSE;
	}
	return FALSE;
}

INT_PTR CALLBACK SetupTypeProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {
	static BOOL inited = FALSE;
	static HWND hQuestion, hTI73, hTI82, hTI83, hTI83P, hTI83PSE, hTI84P, hTI84PSE, hTI84PCSE, hTI85, hTI86;
	switch (Message) {
		case WM_INITDIALOG:
			hQuestion = GetDlgItem(hwnd, IDC_STATIC_TYPE);
			hTI73 = GetDlgItem(hwnd, IDC_RADIO_TI73);
			hTI82 = GetDlgItem(hwnd,  IDC_RADIO_TI82);
			hTI83 = GetDlgItem(hwnd, IDC_RADIO_TI83);
			hTI83P = GetDlgItem(hwnd, IDC_RADIO_TI83P);
			hTI83PSE = GetDlgItem(hwnd, IDC_RADIO_TI83PSE);
			hTI84P = GetDlgItem(hwnd, IDC_RADIO_TI84P);
			hTI84PSE = GetDlgItem(hwnd, IDC_RADIO_TI84PSE);
			hTI84PCSE = GetDlgItem(hwnd, IDC_RADIO_TI84PCSE);
			hTI85 = GetDlgItem(hwnd, IDC_RADIO_TI85);
			hTI86 = GetDlgItem(hwnd, IDC_RADIO_TI86);

			Button_SetCheck(hTI83P, BST_CHECKED);
			PropSheet_SetWizButtons(GetParent(hwnd), PSWIZB_BACK | PSWIZB_NEXT);
			return FALSE;
		case WM_COMMAND: {
			switch(HIWORD(wParam)) {
				case BN_CLICKED:
					/*switch(LOWORD(wParam)) {
						case IDC_RADIO_TI73:
						case IDC_RADIO_TI82:
						case IDC_RADIO_TI83:
						case IDC_RADIO_TI83P:
						case IDC_RADIO_TI83PSE:
						case IDC_RADIO_TI84P:
						case IDC_RADIO_TI84PSE:
						case IDC_RADIO_TI85:
						case IDC_RADIO_TI86:

							break;
						default:*/
							PropSheet_SetWizButtons(GetParent(hwnd), PSWIZB_BACK | PSWIZB_NEXT);
							/*break;
					}*/
					break;
			}
			return TRUE;
		}
		case WM_NOTIFY :
		{
			LPNMHDR pnmh = (LPNMHDR) lParam;
			switch(pnmh->code) {
				case PSN_SETACTIVE: {
					//TODO: make all the calcs rom dumping work
					if (use_bootfree) {
						Static_SetText(hQuestion, _T("What type of calculator would you like to emulate?"));
						Button_Enable(hTI82, FALSE);
						Button_Enable(hTI83, FALSE);
						Button_Enable(hTI85, FALSE);
						Button_Enable(hTI86, FALSE);
					} else {
						Static_SetText(hQuestion, _T("What type of calculator are you going to dump?"));
						Button_Enable(hTI82, TRUE);
						Button_Enable(hTI83, TRUE);
						Button_Enable(hTI85, TRUE);
						Button_Enable(hTI86, TRUE);
					}

					PropSheet_SetWizButtons(GetParent(hwnd), PSWIZB_BACK | PSWIZB_NEXT);
					break;
				}
				case PSN_WIZNEXT: {
					if (Button_GetCheck(hTI73) == BST_CHECKED) {
						model = TI_73;
					} else if (Button_GetCheck(hTI82) == BST_CHECKED) {
						model = TI_82;
					} else if (Button_GetCheck(hTI83) == BST_CHECKED) {
						model = TI_83;
					} else if (Button_GetCheck(hTI83P) == BST_CHECKED) {
						model = TI_83P;
					} else if (Button_GetCheck(hTI83PSE) == BST_CHECKED) {
						model = TI_83PSE;
					} else if (Button_GetCheck(hTI84P) == BST_CHECKED) {
						model = TI_84P;
					} else if (Button_GetCheck(hTI84PSE) == BST_CHECKED) {
						model = TI_84PSE;
					} else if (Button_GetCheck(hTI84PCSE) == BST_CHECKED) {
						model = TI_84PCSE;
					} else if (Button_GetCheck(hTI85) == BST_CHECKED) {
						model = TI_85;
					} else if (Button_GetCheck(hTI86) == BST_CHECKED) {
						model = TI_86;
					}
					break;
				}
				case PSN_QUERYCANCEL:
					error = TRUE;
					break;
			}
			return TRUE;
		}
		case WM_DESTROY:
			return FALSE;
	}
	return FALSE;
}

DWORD ExtractBootFree(int model, TCHAR *hexFile) {
	HMODULE hModule = GetModuleHandle(NULL);
	HRSRC resource = NULL;
	switch(model) {
		case TI_73:
			resource = FindResource(hModule, MAKEINTRESOURCE(HEX_BOOT73), _T("HEX"));
			break;
		case TI_83P:
			resource = FindResource(hModule, MAKEINTRESOURCE(HEX_BOOT83P), _T("HEX"));
			break;
		case TI_83PSE:
			resource = FindResource(hModule, MAKEINTRESOURCE(HEX_BOOT83PSE), _T("HEX"));
			break;
		case TI_84P:
			resource = FindResource(hModule, MAKEINTRESOURCE(HEX_BOOT84P), _T("HEX"));
			break;
		case TI_84PSE:
			resource = FindResource(hModule, MAKEINTRESOURCE(HEX_BOOT84PSE), _T("HEX"));
			break;
		case TI_84PCSE:
			resource = FindResource(hModule, MAKEINTRESOURCE(HEX_BOOT84PCSE), _T("HEX"));
			break;
		default:
			return 1;
	}
	GetStorageString(hexFile, MAX_PATH);
	//extract and write the open source boot page
	StringCbCat(hexFile, MAX_PATH, _T("boot.hex"));
	return ExtractResource(hexFile, resource);
}

static HWND hOSStaticProgress, hOSProgressBar;
HRESULT OSDownloadCallback::OnProgress(ULONG ulProgress, ULONG ulProgressMax, ULONG, LPCWSTR) {
	if (ulProgressMax != 0) {
		SendMessage(hOSProgressBar, PBM_SETRANGE32, 0, ulProgressMax);
		SendMessage(hOSProgressBar, PBM_SETPOS, ulProgress, 0);
		InvalidateRect(GetParent(hOSProgressBar), NULL, FALSE);
		UpdateWindow(GetParent(hOSProgressBar));
	}
	return S_OK;
}

static void progress_callback(ULONG ulProgress, ULONG ulProgressMax) {
	if (ulProgressMax != 0) {
		SendMessage(hOSProgressBar, PBM_SETRANGE32, 0, ulProgressMax);
		SendMessage(hOSProgressBar, PBM_SETPOS, ulProgress, 0);
		//InvalidateRect(GetParent(hOSProgressBar), NULL, FALSE);
		//UpdateWindow(GetParent(hOSProgressBar));
	}
}

INT_PTR CALLBACK SetupOSProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {
	static HWND hComboOS, hBrowseOS, hEditOSPath, hRadioBrowse, hRadioDownload;
	switch (Message) {
		case WM_INITDIALOG: {
			hComboOS = GetDlgItem(hwnd, IDC_COMBO_OS);
			hBrowseOS = GetDlgItem(hwnd, IDC_BROWSE_OS);
			hEditOSPath = GetDlgItem(hwnd, IDC_EDIT_OS_PATH);
			hOSStaticProgress = GetDlgItem(hwnd, IDC_STATIC_OSPROGRESS);
			ShowWindow(hOSStaticProgress, SW_HIDE);
			hOSProgressBar = GetDlgItem(hwnd, IDC_OSPROGRESS);
			ShowWindow(hOSProgressBar, SW_HIDE);
			hRadioBrowse = GetDlgItem(hwnd, IDC_RADIO_BROWSE_OS);
			hRadioDownload = GetDlgItem(hwnd, IDC_RADIO_DOWNLOAD_OS);
			ComboBox_ResetContent(hComboOS);

			//WCHAR *wszPath = NULL;
			//SHGetKnownFolderPath(FOLDERID_Downloads, 0, NULL, &wszPath);
			//char szPath[MAX_PATH];
			//WideCharToMultiByte(CP_ACP, 0, wszPath, -1, szPath, sizeof(szPath), NULL, NULL);
			//CoTaskMemFree(wszPath);

			//SetDlgItemText(hwnd, IDC_EDITDOWNLOADPATH, szPath);
			return TRUE;
		}
		case WM_COMMAND: {
			switch(HIWORD(wParam)) {
				case BN_CLICKED: {
					switch(LOWORD(wParam)) {
						case IDC_RADIO_BROWSE_OS: {
							ComboBox_Enable(hComboOS, FALSE);
							Edit_Enable(hEditOSPath, TRUE);
							Button_Enable(hBrowseOS, TRUE);
							break;
						}
						case IDC_RADIO_DOWNLOAD_OS: {
							Edit_Enable(hEditOSPath, FALSE);
							Button_Enable(hBrowseOS, FALSE);
							ComboBox_Enable(hComboOS, TRUE);
							ComboBox_ResetContent(hComboOS);
							switch(model) {
								case TI_73:
									ComboBox_AddString(hComboOS, _T("OS 1.91"));
									ComboBox_SetCurSel(hComboOS, 0);
									break;
								case TI_83P:
								case TI_83PSE:
									ComboBox_AddString(hComboOS, _T("OS 1.19"));
									ComboBox_SetCurSel(hComboOS, 0);
									break;
								case TI_84P:
								case TI_84PSE: {
									ComboBox_AddString(hComboOS, _T("OS 2.43"));
									ComboBox_AddString(hComboOS, _T("OS 2.55 MP"));
									ComboBox_SetCurSel(hComboOS, 1);
									break;
								case TI_84PCSE:
									ComboBox_AddString(hComboOS, _T("OS 4.0"));
									ComboBox_AddString(hComboOS, _T("OS 4.2"));
									ComboBox_SetCurSel(hComboOS, 1);
									break;
								}
							}
							break;
						}
						case IDC_BROWSE_OS: {
							TCHAR defExt[32];
							int filterIndex;
							if (model == TI_73) {
								filterIndex = 2;
								StringCbCopy(defExt, sizeof(defExt), _T("73u"));
							} else if (model == TI_84PCSE) {
								filterIndex = 3;
								StringCbCopy(defExt, sizeof(defExt), _T("8cu"));
							} else {
								filterIndex = 1;
								StringCbCopy(defExt, sizeof(defExt), _T("8xu"));
							}

							TCHAR buf[512];
							if (!BrowseFile(buf,			//output
								_T("83 Plus Series OS  (*.8xu)\0*.8xu\0	73 OS  (*.73u)\0*.73u\0	84 Plus C SE OS  (*.8cu)\0*.8cu\0	All Files (*.*)\0*.*\0\0"), //filter
								_T("Open Calculator OS File"),		//title
								defExt,
								0, filterIndex))
								Edit_SetText(hEditOSPath, buf);
							break;
						}
						break;
					}
				}
			}
			return TRUE;
		}
		case WM_NOTIFY: {
			LPNMHDR pnmh = (LPNMHDR) lParam;
			switch(pnmh->code) {
				case NM_CLICK:
				case NM_RETURN: {
						PNMLINK pNMLink = (PNMLINK)lParam;
						LITEM item = pNMLink->item;
#ifdef _UNICODE
						ShellExecute(NULL, _T("open"), item.szUrl, NULL, NULL, SW_SHOWNORMAL);
#else
						TCHAR buffer[1024];
						memset(buffer, 0, ARRAYSIZE(buffer));
						int length = (int) wcslen(item.szUrl);
						WideCharToMultiByte(CP_ACP, 0, item.szUrl, length, buffer, length, NULL, NULL);
						ShellExecute(NULL, _T("open"), buffer, NULL, NULL, SW_SHOWNORMAL);
#endif
						break;
				}
				case PSN_SETACTIVE: {
					DWORD flags = PSWIZB_BACK | PSWIZB_NEXT;
					if (use_bootfree)
						flags = PSWIZB_BACK | PSWIZB_FINISH;
					PropSheet_SetWizButtons(GetParent(hwnd), flags);
					switch(model) {
						case TI_73:
						case TI_83P:
						case TI_83PSE:
						case TI_84P:
						case TI_84PSE:
						case TI_84PCSE: {
							Button_Enable(hRadioDownload, TRUE);
							Button_SetCheck(hRadioDownload, BST_CHECKED);
							Button_SetCheck(hRadioBrowse, BST_UNCHECKED);
							SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(IDC_RADIO_DOWNLOAD_OS ,BN_CLICKED), 0);
							break;
						}
						default: {
							Button_Enable(hRadioDownload, FALSE);
							ComboBox_Enable(hComboOS, FALSE);
							Button_SetCheck(hRadioDownload, BST_UNCHECKED);
							Button_SetCheck(hRadioBrowse, BST_CHECKED);
							SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(IDC_RADIO_BROWSE_OS ,BN_CLICKED), 0);
							break;
						}
					}
					break;
				}
				case PSN_WIZBACK: {
					SendMessage(hOSProgressBar, PBM_SETPOS, 0, 0);
					ShowWindow(hOSStaticProgress, SW_HIDE);
					ShowWindow(hOSProgressBar, SW_HIDE);
					break;
				}
				case PSN_WIZNEXT: {
					OSDownloadCallback callback;
					if (Button_GetCheck(hRadioDownload) == BST_CHECKED) {
						ShowWindow(hOSStaticProgress, SW_SHOW);
						ShowWindow(hOSProgressBar, SW_SHOW);
						Static_SetText(hOSStaticProgress, _T("Downloading OS..."));
						BOOL succeeded = DownloadOS(&callback, ComboBox_GetCurSel(hComboOS) == 0);
						if (!succeeded) {
							MessageBox(hwnd, _T("Unable to download file"), _T("Download failed"), MB_OK);
							SetWindowLongPtr(hwnd, DWLP_MSGRESULT, TRUE);
						}
					} else {
						Edit_GetText(hEditOSPath, osPath, MAX_PATH);
					}
					SendMessage(hOSProgressBar, PBM_SETPOS, 0, 0);
					ShowWindow(hOSStaticProgress, SW_HIDE);
					ShowWindow(hOSProgressBar, SW_HIDE);
					break;
				}
				case PSN_WIZFINISH: {
					OSDownloadCallback callback;
					TCHAR buffer[MAX_PATH];
					*buffer = '\0';
					SaveFile(buffer, _T("ROMs  (*.rom)\0*.rom\0Bins  (*.bin)\0*.bin\0All Files (*.*)\0*.*\0\0"),
								_T("Wabbitemu Export Rom"), _T("rom"), OFN_PATHMUSTEXIST, 0);
					if (Button_GetCheck(hRadioDownload) == BST_CHECKED) {
						ShowWindow(hOSStaticProgress, SW_SHOW);
						ShowWindow(hOSProgressBar, SW_SHOW);
						Static_SetText(hOSStaticProgress, _T("Downloading OS..."));
						BOOL succeeded = DownloadOS(&callback, ComboBox_GetCurSel(hComboOS) == 0);
						if (!succeeded) {
							MessageBox(hwnd, _T("Unable to download file"), _T("Download failed"), MB_OK);
							SetWindowLongPtr(hwnd, DWLP_MSGRESULT, TRUE);
							break;
						}
					} else {
						Edit_GetText(hEditOSPath, osPath, MAX_PATH);
					}

					lpMainWindow = create_calc_frame_register_events();
					if (lpMainWindow == NULL || lpMainWindow->lpCalc == NULL) {
						destroy_calc_frame(lpMainWindow);
						lpMainWindow = NULL;
						MessageBox(hwnd, _T("Unable to create main window"), _T("Error"), MB_OK | MB_ICONERROR);
						SetWindowLongPtr(hwnd, DWLP_MSGRESULT, TRUE);
						return TRUE;
					}

					LPCALC lpCalc = lpMainWindow->lpCalc;
					TCHAR hexFile[MAX_PATH];
					DWORD error = ExtractBootFree(model, hexFile);
					if (error) {
						destroy_calc_frame(lpMainWindow);
						lpMainWindow = NULL;
						MessageBox(hwnd, _T("Unable to extract boot page"), _T("Error"), MB_OK);
						SetWindowLongPtr(hwnd, DWLP_MSGRESULT, TRUE);
						break;
					}

					error = (DWORD) calc_init_model(lpCalc, model, NULL);
					if (error) {
						destroy_calc_frame(lpMainWindow);
						lpMainWindow = NULL;
						MessageBox(hwnd, _T("Unable to create new calc"), _T("Error"), MB_OK);
						SetWindowLongPtr(hwnd, DWLP_MSGRESULT, TRUE);
						break;
					}

					LoadRegistrySettings(lpMainWindow, lpCalc);
					StringCbCopy(lpCalc->rom_path, sizeof(lpCalc->rom_path), buffer);
					lpCalc->active = TRUE;
					lpCalc->model = model;
					lpCalc->cpu.pio.model = model;
					Static_SetText(hOSStaticProgress, _T("Writing Bootcode"));

					FILE *file;
					_tfopen_s(&file, hexFile, _T("rb"));
					if (file == NULL) {
						destroy_calc_frame(lpMainWindow);
						lpMainWindow = NULL;
						MessageBox(hwnd, _T("Unable to open the boot page"), _T("Error"), MB_OK);
						SetWindowLongPtr(hwnd, DWLP_MSGRESULT, TRUE);
						break;
					}

					writeboot(file, &lpCalc->mem_c, -1);
					fclose(file);
					_tremove(hexFile);

					Static_SetText(hOSStaticProgress, _T("Loading OS"));
					// if you don't want to load an OS, fine...
					if (_tcslen(osPath) > 0) {
						TIFILE_t *tifile = importvar(osPath, FALSE);
						if (tifile == NULL || tifile->type != FLASH_TYPE || tifile->flash == NULL ||
							tifile->flash->type != FLASH_TYPE_OS)
						{
							destroy_calc_frame(lpMainWindow);
							lpMainWindow = NULL;
							MessageBox(hwnd, _T("Error: OS file is corrupt or invalid"), _T("Error"), MB_OK);
							SetWindowLongPtr(hwnd, DWLP_MSGRESULT, TRUE);
							break;
						} else {
							forceload_os(&lpCalc->cpu, tifile);
							if (Button_GetCheck(hRadioDownload) == BST_CHECKED) {
								_tremove(osPath);
							}
						}
					}

					finalize_calc(hwnd, lpCalc, buffer);

					Static_SetText(hOSStaticProgress, _T("Done"));
					break;
				}
				case PSN_QUERYCANCEL:
					error = TRUE;
					break;
			}
			return TRUE;
		}
		case WM_DESTROY:
			return FALSE;
	}
	return FALSE;
}

static BOOL DownloadOS(OSDownloadCallback *callback, BOOL version)
{
	TCHAR downloaded_file[MAX_PATH];
	GetStorageString(downloaded_file, sizeof(downloaded_file));
	StringCbCat(downloaded_file, sizeof(downloaded_file), _T("OS.8xu"));
	StringCbCopy(osPath, sizeof(osPath), downloaded_file);
	TCHAR *url;
	switch (model) {
	case TI_73:
		url = _T("https://education.ti.com/~/media/32E99F6FAEB2424D8313B0DEE7B70791");
		break;
	case TI_83P:
	case TI_83PSE:
		url = _T("https://education.ti.com/~/media/EEB252CDF6A748309894C1790408D0E7");
		break;
	case TI_84P:
	case TI_84PSE:
		if (version) {
			url = _T("https://education.ti.com/~/media/A943680938CC460E8CB04554E99D665B");
		}
		else {
			url = _T("https://education.ti.com/~/media/DA0795A5F0FA45D2A9AC03BB11B8245F");
		}
		break;
	case TI_84PCSE:
		url = _T("https://education.ti.com/~/media/4D5547F48BBA4384BB85A645D7772A1A");
		break;
	default:
		assert(false);
		return FALSE;
	}

	HINTERNET hInternet = InternetOpen(_T("Mozilla/5.0 (Windows NT 10.0; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/48.0.2564.109 Safari/537.36"),
		0, NULL, NULL, 0);

	HINTERNET hFile = InternetOpenUrl(hInternet, url, NULL, 0, 0, NULL);
	if (hFile != NULL) {
		DWORD dwHeader = 0;
		DWORD dwFileLength;
		DWORD dwParamLength = sizeof(dwFileLength);
		BOOL fQueryResult = HttpQueryInfo(hFile, HTTP_QUERY_CONTENT_LENGTH, &dwFileLength, &dwParamLength, &dwHeader);

		BYTE Buffer[1024];
		DWORD dwBytesRead = -1;

		BOOL fRead = TRUE;
		HANDLE hOut = CreateFile(downloaded_file, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

		DWORD dwTotalRead = 0;
		while (fRead == TRUE && dwBytesRead != 0) {
			fRead = InternetReadFile(hFile, Buffer, sizeof(Buffer), &dwBytesRead);
			if (fRead) {
				dwTotalRead += dwBytesRead;
				DWORD dwWritten = 0;
				WriteFile(hOut, Buffer, dwBytesRead, &dwWritten, NULL);
				progress_callback(dwTotalRead, dwFileLength);
			}
		}
		CloseHandle(hOut);
		InternetCloseHandle(hFile);

		InternetCloseHandle(hInternet);
		return TRUE;
	}
	
	return FALSE;
}

DWORD WINAPI StartTIConnect(LPVOID lpParam) {
	TCHAR *path = (TCHAR *) lpParam;
	TCHAR argBuf[MAX_PATH];
	StringCbPrintf(argBuf, sizeof(argBuf), _T("\"%s\" \"%s\""), TIConnectPath, path);
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	memset(&si, 0, sizeof(si)); 
	memset(&pi, 0, sizeof(pi)); 
	si.cb = sizeof(si);
	if (!CreateProcess(NULL, argBuf,
		NULL, NULL, FALSE, CREATE_DEFAULT_ERROR_MODE, 
		NULL, NULL, &si, &pi)) {
		MessageBox(NULL, _T("Unable to start the process. Try manually sending the file."), _T("Error"), MB_OK);
		return FALSE;
	}
	// Wait until child process exits.
	WaitForSingleObject(pi.hProcess, INFINITE);
						
	CloseHandle( pi.hProcess );
	CloseHandle( pi.hThread );
	return TRUE;
}

// true if can be found. False otherwise
BOOL CanFindTIConnectProg() {
	TCHAR *env;
	size_t envLen;
	FILE *connectProg;
	SYSTEM_INFO sysInfo;
	GetSystemInfo(&sysInfo);

	ZeroMemory(TIConnectPath, sizeof(TIConnectPath));
	if (sysInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64) {
		_tdupenv_s(&env, &envLen, _T("programfiles(x86)"));
	} else {
		_tdupenv_s(&env, &envLen, _T("programfiles"));
	}
	StringCbCat(TIConnectPath, sizeof(TIConnectPath), env);
	free(env);
	// yes i know hard coding is bad :/
	StringCbCat(TIConnectPath, sizeof(TIConnectPath), _T("\\TI Education\\TI Connect\\TISendTo.exe"));
	BOOL found = _tfopen_s(&connectProg, TIConnectPath, _T("r")) == ERROR_SUCCESS;
	if (connectProg) {
		fclose(connectProg);
	}
	return found;
}

BOOL CanFindTIGraphLinkProg() {
	TCHAR *env;
	size_t envLen;
	FILE *connectProg;
	SYSTEM_INFO sysInfo;
	GetSystemInfo(&sysInfo);
	// TODO: fix path
	ZeroMemory(TIConnectPath, sizeof(TIConnectPath));
	if (sysInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64)
		_tdupenv_s(&env, &envLen, _T("programfiles(x86)"));
	else
		_tdupenv_s(&env, &envLen, _T("programfiles"));
	StringCbCat(TIConnectPath, sizeof(TIConnectPath), env);
	free(env);
	//yes i know hard coding is bad :/
	StringCbCat(TIConnectPath, sizeof(TIConnectPath), _T("\\TI Education\\TI Connect\\TISendTo.exe"));
	BOOL found =  _tfopen_s(&connectProg, TIConnectPath, _T("r")) == ERROR_SUCCESS;
	if (connectProg) {
		fclose(connectProg);
	}
	return found;
}

INT_PTR CALLBACK SetupROMDumperProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {
	static BOOL inited = FALSE;
	static HWND hButtonAuto, hButtonManual, hStaticError;
	switch (Message) {
		case WM_INITDIALOG: {
			hButtonAuto = GetDlgItem(hwnd, IDC_BUTTON_AUTO);
			hButtonManual = GetDlgItem(hwnd, IDC_BUTTON_MANUAL);
			hStaticError = GetDlgItem(hwnd, IDC_STATIC_ERROR);
			
			ExtractDumperProg();
			
			switch (model) {
				case TI_83:
				case TI_86:
				case TI_83P:
				case TI_83PSE:
				case TI_84P:
				case TI_84PSE: {
					if (CanFindTIConnectProg() == FALSE) {
						Static_SetText(hStaticError, _T("Unable to find TI Connect, please manually send the dumper program."));
						Button_Enable(hButtonAuto, FALSE);
						ShowWindow(hStaticError, SW_SHOW);
					}
					break;
				}
				case TI_82:
				case TI_85:
					if (CanFindTIGraphLinkProg() == FALSE) {
						Static_SetText(hStaticError, _T("Unable to find TI Graph Link, please manually send the dumper program."));
						Button_Enable(hButtonAuto, FALSE);
						ShowWindow(hStaticError, SW_SHOW);
					}
					break;
			}
			return FALSE;
		}
		case WM_COMMAND: {
			switch(HIWORD(wParam)) {
				case BN_CLICKED:
					if (LOWORD(wParam) == IDC_BUTTON_AUTO) {
						CreateThread(NULL, 0, StartTIConnect, &dumperPath, 0, NULL);
					} else {
						TCHAR buf[MAX_PATH], *filter, *ext;
						char ch;
						switch (model) {
						case TI_83P:
						case TI_83PSE:
						case TI_84P:
						case TI_84PSE:
						case TI_84PCSE:
							filter = _T("83 Plus Program (*.8xp)\0*.8xp\0All Files (*.*)\0*.*\0\0");
							ext = _T("8xp");
							break;
						case TI_83:
							filter = _T("83 Program (*.83p)\0*.83p\0All Files (*.*)\0*.*\0\0");
							ext = _T("83p");
							break;
						case TI_82:
							filter = _T("82 Program (*.82p)\0*.83p\0All Files (*.*)\0*.*\0\0");
							ext = _T("82p");
							break;
						case TI_85:
							filter = _T("85 String (*.85s)\0*.83p\0All Files (*.*)\0*.*\0\0");
							ext = _T("85s");
							break;
						case TI_86:
							filter = _T("86 Program (*.86p)\0*.83p\0All Files (*.*)\0*.*\0\0");
							ext = _T("86p");
							break;
						default:
							MessageBox(hwnd, _T("Invalid model selected"), _T("Error"), MB_OK);
							return 0;
						}

						if (!SaveFile(buf, filter, _T("Wabbitemu Save ROM Dumper"), ext, 0, 0)) {
							FILE *start, *end;
							_tfopen_s(&start, dumperPath, _T("rb"));
							_tfopen_s(&end, buf, _T("wb"));
							if (end == NULL || start == NULL) {
								if (end != NULL) {
									fclose(end);
								}
								if (start != NULL) {
									fclose(start);
								}
								MessageBox(hwnd, _T("Error saving file, please try again."), _T("Error"), MB_OK);
								break;
							}
							while(!feof(start)) {
								ch = (char) fgetc(start);
								fputc(ch, end);
							}
							fclose(start);
							fclose(end);
							_tremove(dumperPath);
						}
					}
					PropSheet_SetWizButtons(GetParent(hwnd), PSWIZB_NEXT | PSWIZB_BACK);
					break;
			}
			return TRUE;
		}
		case WM_NOTIFY :
		{
			LPNMHDR pnmh = (LPNMHDR) lParam;
			switch(pnmh->code) {
				case PSN_SETACTIVE:
					if (inited) {
						PropSheet_SetWizButtons(GetParent(hwnd), PSWIZB_NEXT | PSWIZB_BACK);
					}  else {
						PropSheet_SetWizButtons(GetParent(hwnd), PSWIZB_BACK);
						inited = TRUE;
					}
					break;
				case PSN_WIZNEXT:
					break;
				case PSN_QUERYCANCEL:
					error = TRUE;
					break;
			}
			return TRUE;
		}
		case WM_DESTROY:
			return FALSE;
	}
	return FALSE;
}

static BOOL is_second_bootpage(TIFILE_t *boot_var) {
	return boot_var->var->name[6] == '2';
}

DWORD write_boot_page(LPCALC lpCalc, TCHAR *file_path, int boot_page, int boot_page2) {
	BYTE(*flash)[PAGE_SIZE] = (BYTE(*)[PAGE_SIZE]) lpCalc->mem_c.flash;

	// goto the name to see which one we have (in TI var header)
	TIFILE_t *boot_var = importvar(file_path, FALSE);
	if (boot_var == NULL || boot_var->var == NULL) {
		return 1;
	}

	// app var will be 0x4002, with the first two bytes being the size
	if (boot_var->var->length != PAGE_SIZE + sizeof(uint16_t)) {
		return 2;
	}

	int current_page;
	if (is_second_bootpage(boot_var)) {
		current_page = boot_page2;
	} else {
		current_page = boot_page;
	}

	for (int i = 0; i < PAGE_SIZE; i++) {
		flash[current_page][i] = boot_var->var->data[i + 2];
	}

	boot_var = FreeTiFile(boot_var);
	return 0;
}

INT_PTR CALLBACK SetupMakeROMProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {
	static BOOL inited = FALSE;
	static HWND hStaticAppVar, hBrowseButton1, hBrowseButton2, hEditVar1, hEditVar2;
	switch (Message) {
		case WM_INITDIALOG: {
			hStaticAppVar = GetDlgItem(hwnd, IDC_STATIC_APPVAR);
			hBrowseButton1 = GetDlgItem(hwnd, IDC_BUTTON_BROWSE1);
			hBrowseButton2 = GetDlgItem(hwnd, IDC_BUTTON_BROWSE2);
			hEditVar1 = GetDlgItem(hwnd, IDC_EDT_APPVAR1);
			hEditVar2 = GetDlgItem(hwnd, IDC_EDT_APPVAR2);
			return FALSE;
		}
		case WM_COMMAND: {
			switch(HIWORD(wParam)) {
				case BN_CLICKED: {
					PropSheet_SetWizButtons(GetParent(hwnd), PSWIZB_FINISH | PSWIZB_BACK);
					TCHAR buf[MAX_PATH], ch;
					if (BrowseFile(buf, _T("83 Plus Application Variables (*.8xv)\0*.8xv\0	All Files (*.*)\0*.*\0\0"),
						_T("Wabbitemu Open ROM Dump"), _T("8xp"), 0, 1)) {
						break;
					}
					if (LOWORD(wParam) == IDC_BUTTON_BROWSE1) {
						Edit_SetText(hEditVar1, buf);
						ch = '2';
					} else {
						Edit_SetText(hEditVar2, buf);
						ch = '1';
					}
					int len = (int) _tcslen(buf);
					buf[len - 5] = ch;
					FILE *file;
					errno_t error = _tfopen_s(&file, buf, _T("r"));
					if (error) {
						break;
					}
					fclose(file);
					if (LOWORD(wParam) == IDC_BUTTON_BROWSE2) {
						Edit_SetText(hEditVar1, buf);
					} else {
						Edit_SetText(hEditVar2, buf);
					}
					break;
				}
			}
			return TRUE;
		}
		case WM_NOTIFY :
		{
			LPNMHDR pnmh = (LPNMHDR) lParam;
			switch(pnmh->code) {
				case NM_CLICK:
				case NM_RETURN: {
					PNMLINK pNMLink = (PNMLINK) lParam;
					LITEM item = pNMLink->item;
#ifdef _UNICODE
					if (_tcscmp(item.szID, _T("RunHelp"))) {
						DialogBox(NULL, MAKEINTRESOURCE(DLG_RUNHELP), hwnd, (DLGPROC) HelpProc);
					}
#else
					TCHAR buffer[1024];
					memset(buffer, 0, ARRAYSIZE(buffer));
					int length = (int) wcslen(item.szUrl);
					WideCharToMultiByte(CP_ACP, 0, item.szID, length, buffer, length, NULL, NULL);
					if (_tcscmp(buffer, _T("RunHelp"))) {
						DialogBox(NULL, MAKEINTRESOURCE(DLG_RUNHELP), hwnd, (DLGPROC) HelpProc);
					}
#endif
					break;
				}
				// The program will create two application variables D83PBE1.8xv and D84PBE2.8xv. You need to send these back to the computer, and browse them with the buttons below
				case PSN_SETACTIVE:
					if (inited) {
						PropSheet_SetWizButtons(GetParent(hwnd), PSWIZB_FINISH | PSWIZB_BACK);
					} else {
						PropSheet_SetWizButtons(GetParent(hwnd), PSWIZB_DISABLEDFINISH | PSWIZB_BACK);
						inited = TRUE;
					}
					TCHAR buf[1024];
					TCHAR name[12];
					TCHAR name2[12];
					switch(model) {
						case TI_83P:
						case TI_83PSE:
							if (model == TI_83P) {
								StringCbCopy(name, sizeof(name), _T("D83PBE1.8xv"));
							} else {
								StringCbCopy(name, sizeof(name), _T("D83PSE1.8xv"));
							}
							StringCbPrintf(buf, sizeof(buf), _T("%s %s%s"), _T("The program will create an application variable"),
								name, _T(". You need to send this back to the computer, and locate it with the button below"));
							break;
						case TI_84P:
						case TI_84PSE:
						case TI_84PCSE:
							if (model == TI_84P) {
								StringCbCopy(name, sizeof(name), _T("D84PBE1.8xv"));
								StringCbCopy(name2, sizeof(name2), _T("D84PBE2.8xv"));
							} else if (model == TI_84PSE) {
								StringCbCopy(name, sizeof(name), _T("D84PSE1.8xv"));
								StringCbCopy(name2, sizeof(name2), _T("D84PSE2.8xv"));
							} else {
								StringCbCopy(name, sizeof(name), _T("D84CSE1.8xv"));
								StringCbCopy(name2, sizeof(name2), _T("D84CSE2.8xv"));
							}

							StringCbPrintf(buf, sizeof(buf), _T("%s %s %s %s%s"), _T("The program will create two application variables"),
								name, _T("and"), name2, 
								_T(". You need to send these back to the computer, and locate them with the buttons below"));
							break;
					}
					Static_SetText(hStaticAppVar, buf);
					if (model < TI_84P) {
						ShowWindow(hBrowseButton2, SW_HIDE);
						ShowWindow(hEditVar2, SW_HIDE);
					}
					break;
				case PSN_WIZFINISH: {
					TCHAR browse[MAX_PATH];
					Edit_GetText(hEditVar1, browse, MAX_PATH);
					TCHAR buffer[MAX_PATH];
					*buffer = '\0';
					SaveFile(buffer, _T("ROMs (*.rom)\0*.rom\0Bins (*.bin)\0*.bin\0All Files (*.*)\0*.*\0\0"),
								_T("Wabbitemu Export Rom"), _T("rom"), OFN_PATHMUSTEXIST, 0);

					lpMainWindow = create_calc_frame_register_events();
					if (lpMainWindow == NULL || lpMainWindow->lpCalc == NULL) {
						destroy_calc_frame(lpMainWindow);
						lpMainWindow = NULL;
						MessageBox(hwnd, _T("Unable to create main window"), _T("Error"), MB_OK | MB_ICONERROR);
						SetWindowLongPtr(hwnd, DWLP_MSGRESULT, TRUE);
						return TRUE;
					}

					LPCALC lpCalc = lpMainWindow->lpCalc;
					DWORD error = (DWORD) calc_init_model(lpCalc, model, NULL);
					if (error) {
						destroy_calc_frame(lpMainWindow);
						MessageBox(hwnd, _T("Unable to init calc"), _T("Error"), MB_OK);
						SetWindowLongPtr(hwnd, DWLP_MSGRESULT, TRUE);
						break;
					}

					// slot stuff
					lpCalc->active = TRUE;
					lpCalc->model = model;
					lpCalc->cpu.pio.model = model;
					StringCbCopy(lpCalc->rom_path, sizeof(lpCalc->rom_path), buffer);

					// if you don't want to load an OS, fine...
					if (_tcslen(osPath) > 0) {
						TIFILE_t *tifile = importvar(osPath, FALSE);
						if (tifile == NULL || tifile->type != FLASH_TYPE || tifile->flash == NULL ||
							tifile->flash->type != FLASH_TYPE_OS) 
						{
							destroy_calc_frame(lpMainWindow);
							lpMainWindow = NULL;
							MessageBox(hwnd, _T("Error: OS file is corrupt or invalid"), _T("Error"), MB_OK);
							SetWindowLongPtr(hwnd, DWLP_MSGRESULT, TRUE);
							break;
						} else {
							forceload_os(&lpCalc->cpu, tifile);
						}
					}

					int boot_page, boot_page2;
					switch (model) {
					case TI_83P:
					case TI_83PSE:
						boot_page = lpCalc->mem_c.flash_pages - 1;
						break;
					case TI_84P:
					case TI_84PSE:
						boot_page = lpCalc->mem_c.flash_pages - 1;
						boot_page2 = lpCalc->mem_c.flash_pages - 0x11;
						break;
					case TI_84PCSE:
						boot_page = lpCalc->mem_c.flash_pages - 1;
						boot_page2 = lpCalc->mem_c.flash_pages - 3;
						break;
					}

					error = write_boot_page(lpCalc, browse, boot_page, boot_page2);
					if (error) {
						destroy_calc_frame(lpMainWindow);
						lpMainWindow = NULL;
						MessageBox(NULL, _T("Error invalid first boot page file"), _T("Error"), MB_OK);
						SetWindowLongPtr(hwnd, DWLP_MSGRESULT, TRUE);
						break;
					}

					// second boot page
					if (model >= TI_84P) {
						Edit_GetText(hEditVar2, browse, MAX_PATH);
						error = write_boot_page(lpCalc, browse, boot_page, boot_page2);
						if (error) {
							destroy_calc_frame(lpMainWindow);
							lpMainWindow = NULL;
							MessageBox(NULL, _T("Error invalid second boot page file"), _T("Error"), MB_OK);
							SetWindowLongPtr(hwnd, DWLP_MSGRESULT, TRUE);
							break;
						}
					}

					finalize_calc(hwnd, lpCalc, buffer);
					break;
				}
				case PSN_QUERYCANCEL:
					error = TRUE;
					break;
			}
			return TRUE;
		}
		case WM_DESTROY:
			return FALSE;
	}
	return FALSE;
}

INT_PTR CALLBACK HelpProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {
	static int pic_num = 0;
	static HWND hwndBitmap;
	switch (Message) {
		case WM_INITDIALOG: {
			hwndBitmap = GetDlgItem(hwnd, IDC_PIC_HELP);
			SetTimer(hwnd, 0, 1, NULL);
			return 0;
		}
		case WM_TIMER: {
			HBITMAP hbmHelp = NULL;
			if (pic_num % 2)
				hbmHelp = LoadBitmap(g_hInst, _T("HOMESCREEN"));
			else
				hbmHelp = LoadBitmap(g_hInst, _T("CATALOG"));
			SendMessage(hwndBitmap, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM) hbmHelp);
			DeleteObject(hbmHelp);
			pic_num++;
			SetTimer(hwnd, 0, 5000, NULL);
			return 0;
		}
		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDOK: {
					EndDialog(hwnd, IDOK);
					return TRUE;
				}
			}
			break;
	}
	return DefWindowProc(hwnd, Message, wParam, lParam);
}

static void finalize_calc(HWND hwnd, LPCALC lpCalc, TCHAR *buffer) {
	calc_erase_certificate(lpCalc->mem_c.flash, lpCalc->mem_c.flash_size);
	calc_reset(lpCalc);
	if (auto_turn_on) {
		calc_turn_on(lpCalc);
	}
	calc_set_running(lpCalc, TRUE);

	gui_frame_update(lpMainWindow);
	// write the output from file
	Static_SetText(hOSStaticProgress, _T("Saving File"));
	MFILE *romfile = ExportRom(buffer, lpCalc);
	if (romfile != NULL) {
		mclose(romfile);
	} else if (*buffer != '\0') {
		MessageBox(hwnd, _T("Error saving ROM"), _T("Error"), MB_OK);
	}
}

void ExtractDumperProg() {
	GetStorageString(dumperPath, sizeof(dumperPath));
	StringCbCat(dumperPath, sizeof(dumperPath), _T("\\dumper"));
	HMODULE hModule = GetModuleHandle(NULL);
	HRSRC hrDumpProg;
	switch (model) {
		case TI_83P:
		case TI_83PSE:
		case TI_84P:
		case TI_84PSE:
			hrDumpProg = FindResource(hModule, MAKEINTRESOURCE(ROM8X), _T("CALCPROG"));
			StringCbCat(dumperPath, sizeof(dumperPath), _T(".8xp"));
			break;
		case TI_84PCSE:
			hrDumpProg = FindResource(hModule, MAKEINTRESOURCE(ROM8XC), _T("CALCPROG"));
			StringCbCat(dumperPath, sizeof(dumperPath), _T(".8xp"));
			break;
		case TI_82:
			hrDumpProg = FindResource(hModule, MAKEINTRESOURCE(IDR_ROM82), _T("CALCPROG"));
			StringCbCat(dumperPath, sizeof(dumperPath), _T(".82p"));
			break;
		case TI_85:
			hrDumpProg = FindResource(hModule, MAKEINTRESOURCE(IDR_ROM85), _T("CALCPROG"));
			StringCbCat(dumperPath, sizeof(dumperPath), _T(".85s"));
			break;
		case TI_83:
			hrDumpProg = FindResource(hModule, MAKEINTRESOURCE(IDR_ROM83), _T("CALCPROG"));
			StringCbCat(dumperPath, sizeof(dumperPath), _T(".83p"));
			break;
		case TI_86:
			hrDumpProg = FindResource(hModule, MAKEINTRESOURCE(IDR_ROM86), _T("CALCPROG"));
			StringCbCat(dumperPath, sizeof(dumperPath), _T(".86p"));
			break;
		default:
			return;
	}
	ExtractResource(dumperPath, hrDumpProg);
}

/*
dzcomm_init();

		comm_port* COM;
		char* ROM;        
		int Offset = 0;
		int Size;
		int LastOff = 0;
		int LastTime = 0;
		int fh;

		clrscr();
		gotoxy(1,1);
		printf("\nROM dumper v2.2");
		printf("\nby Randy Gluvna");
		printf("\nrandman@home.com");
		printf("\n----------------\n\n");    

		if(argc != 4)
		{
				printf("Syntax: romdump [filename] [size] [COM port]\n");
				return;
		}

		Size = atoi(argv[2]);

		if((ROM = malloc(Size)) == NULL)
		{
				printf("Out of memory!\a\n");
				return;
		}

		COM = comm_port_init(atoi(argv[3])-1);        

		COM->nComm = atoi(argv[3])-1;
		COM->nBaud = _9600;
		COM->control_type = NO_CONTROL;

		if(!comm_port_install_handler(COM))
		{
				printf("Error opening COM port!\a\n");
				return;
		}

		gotoxy(1,7);
		printf("Saving to %s\n",argv[1]);
		gotoxy(1,8);
		printf("File size: %d\n",Size);
		gotoxy(1,10);
		printf("Bytes received: 0\n");
		gotoxy(1,11);
		printf("Percent: 0%%\n");
		gotoxy(1,12);
		printf("CPS: 0\n");

		while(1)
		{
				if(kbhit())
				{
						if(getch() == 27)
						{
								gotoxy(1,13);                         
								return;
						}
				}

				if(LastTime != (clock()/CLK_TCK))
				{
						LastTime = (clock()/CLK_TCK);

						if((Offset == 1) && (LastOff == 0))
								Offset = 0;

						gotoxy(6,12);
						printf("%d   \n",Offset-LastOff);
						LastOff = Offset;
				}

				if(!queue_empty(COM->InBuf))
				{
						ROM[Offset++] = (char)queue_get(COM->InBuf);

						if(!(Offset % 1024))
						{
								gotoxy(17,10);
								printf("%d\n",Offset);
								gotoxy(10,11);
								printf("%d%%\n",(int)(double(Offset)/Size*100));
						}

						if(Offset == Size)
						{
								gotoxy(17,10);
								printf("%d\n",Size);
								gotoxy(10,11);
								printf("100%%\n");
								fh = open(argv[1],O_WRONLY|O_BINARY|O_CREAT,S_IWRITE);
								write(fh,ROM,Size);
								close(fh);
								gotoxy(1,14);
								printf("Done\n");
								return;
						}                        
				}
		}     
*/

#ifdef WITH_TILP
#pragma comment(lib, "ticalcs2.lib")

static void print_lc_error(int errnum)
{
  char *msg;

  ticables_error_get(errnum, &msg);
  fprintf(stderr, _T("Link cable error (code %i)...\n<<%s>>\n"), errnum, msg);

  free(msg);
}

/*
  Dump the ROM (get a ROM image)
*/
int calc_rom_dump(CalcHandle *calc_handle)
{
	int ret, err;
	TCHAR tmp_filename[MAX_PATH];

	// Transfer ROM dumper
	err = ticalcs_calc_dump_rom_1(calc_handle);
	if(err)
		return err;

	// Get data from dumper
	GetStorageString(tmp_filename, sizeof(tmp_filename));
	StringCbCat(tmp_filename, sizeof(tmp_filename), _T("\\temp.rom"));

	err = ticalcs_calc_dump_rom_2(calc_handle, ROMSIZE_AUTO, tmp_filename);

	return err;
}


int DumpRomLibTi() {
	CableHandle *cable;
	CalcHandle *calc;
	int err;

	// init libs
	ticables_library_init();
	ticalcs_library_init();
	
	// set cable
	cable = ticables_handle_new(CABLE_BLK, PORT_2);
	if(cable == NULL)
		return -1;
	
	// set calc
	calc = ticalcs_handle_new(CALC_TI83);
	if(calc == NULL)
		return -1;

	// attach cable to calc (and open cable)
	err = ticalcs_cable_attach(calc, cable);
	err = ticalcs_calc_isready(calc);
	if(err)
		print_lc_error(err);
	
	calc_rom_dump(calc);

	// detach cable (made by handle_del, too)
	err = ticalcs_cable_detach(calc);

	// remove calc & cable
	ticalcs_handle_del(calc);
	ticables_handle_del(cable);
	return ERROR_SUCCESS;
}
#endif