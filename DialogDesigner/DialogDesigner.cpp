// ConsoleApplication1.cpp - 重构版
// 3Q ChatGPT
#include <Windows.h>
#include <Uxtheme.h>
#include <dwmapi.h>
#include <commctrl.h>
#include <string>
#include <vector>
#include <iostream>
#include <windowsx.h>
#include <algorithm>
#include <sstream>
#include "DarkMode.h"
#include "Logger.h"
#pragma comment(lib,"uxtheme.lib")
#pragma comment(lib,"dwmapi.lib")
#pragma comment(lib,"comctl32.lib")
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

using namespace std;

// ==================== 全局变量 ====================
HFONT g_hGuiFont = NULL;
HWND g_HWND = NULL;           // 设计器窗口
HWND g_PropertiesHWND = NULL; // 属性窗口
HWND hwndParent = NULL;

const int START_WIDTH = 400;
const int START_HEIGHT = 300;
int isDarkThemeEnabled = false;

vector<LPCWSTR> g_listdescription = { L"光标", L"文字", L"按钮", L"输入框" };
vector<LPCWSTR> g_listclassname = { L"", L"Static", L"Button", L"Edit" };
#include "Designer.h"
// ==================== 外部声明 ====================
extern vector<Control> g_Controls;
extern int g_CurIndex;
extern int g_ControlIdCounter;

enum MainWindowControlIds {
	IDC_TOOLBOX_LIST = 3,
	IDC_STATUS_BAR = 0x1234,
	IDC_CHANGE_PANEL = 0x2001,
	IDC_CHANGE_LIST = 0x2002,
	IDC_CHANGE_EDIT = 0x2003
};

enum MainMenuCommandIds {
	IDM_THEME_MICA = 0xE001,
	IDM_THEME_ACRYLIC = 0xE002,
	IDM_THEME_TABBED = 0xE003,
	IDM_THEME_DEFAULT = 0xE004,
	IDM_EDIT_DUPLICATE = 0xE101,
	IDM_EDIT_DELETE = 0xE102,
	IDM_EDIT_MOVE_UP = 0xE103,
	IDM_EDIT_MOVE_DOWN = 0xE104,
	IDM_EDIT_MOVE_LEFT = 0xE105,
	IDM_EDIT_MOVE_RIGHT = 0xE106
};

void ExecuteDeleteSelectedControl() {
	if (g_CurIndex < 0 || g_CurIndex >= (int)g_Controls.size()) return;
	DeleteControl(g_CurIndex);
	UpdatePropertyPanel(nullptr);
}

void ExecuteMoveSelectedControl(int dx, int dy) {
	if (g_CurIndex < 0 || g_CurIndex >= (int)g_Controls.size()) return;
	MoveControlWithKeyboard(g_CurIndex, dx, dy);
}

void ExecuteDuplicateSelectedControl() {
	if (g_CurIndex < 0 || g_CurIndex >= (int)g_Controls.size()) return;
	DuplicateControl(g_CurIndex);
}

HWND g_ChangePanelHwnd = NULL;
HWND g_ChangeListHwnd = NULL;
HWND g_ChangeEditHwnd = NULL;

void SyncControlSummaryPanel() {
	if (!g_ChangeListHwnd) return;
	SendMessageW(g_ChangeListHwnd, LB_RESETCONTENT, 0, 0);
	wchar_t itemText[512] = { 0 };
	for (size_t i = 0; i < g_Controls.size(); ++i) {
		swprintf_s(itemText, L"%d. [%s] %s", (int)i + 1,
			g_Controls[i].className.c_str(), g_Controls[i].title.c_str());
		SendMessageW(g_ChangeListHwnd, LB_ADDSTRING, 0, (LPARAM)itemText);
	}
	if (g_CurIndex >= 0 && g_CurIndex < (int)g_Controls.size()) {
		SendMessageW(g_ChangeListHwnd, LB_SETCURSEL, g_CurIndex, 0);
	}
}
//其实是刷新代码
void AppendControlChangeMarker() {
	if (!g_ChangeEditHwnd) return;
	CreateThread(0, 0, [](LPVOID) -> DWORD {
		int textLen = GetWindowTextLengthW(g_ChangeEditHwnd);
		SendMessageW(g_ChangeEditHwnd, EM_SETSEL, 0, textLen);
		wstringstream str;
		str << L"/**Dialog Designer**/\r\n";
		//dialog
		wchar_t title[256]{};
		GetWindowTextW(g_HWND, title, 256);
		auto sz = GetWindowSize(g_HWND, false);
		str << L"dialog," << title << L",DEF,DEF," <<
			sz.cx << L"," << sz.cy << L"\r\n";
		//ALL the controls
		for (size_t i = 0; i < g_Controls.size(); ++i) {
			auto ctl = g_Controls[i];
			auto fnt = GetWindowFontInfo(ctl.hwnd);
			str << ctl.className.c_str() << L"," << ctl.title.c_str()
				<< L"," << ctl.x << L"," << ctl.y << L"," <<
				ctl.width << L"," << ctl.height << L","
				<< fnt.faceName << L"," << fnt.height << L"," << fnt.weight << L"," << fnt.underline << L"," << fnt.italic << L"," << fnt.strikeOut << L"\r\n";
		}
		SendMessageW(g_ChangeEditHwnd, EM_REPLACESEL, FALSE, LPARAM(str.str().c_str()));
		return 0;
		}, 0, 0, 0);
	//DarkModeAPI::ApplyToAll(g_HWND);
}

// ==================== 窗口过程 ====================
LRESULT CALLBACK MainWindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	switch (msg) {
	case WM_CREATE: {
		// 创建菜单
		HMENU hMenu = CreateMenu();
		HMENU smenu = CreatePopupMenu();
		AppendMenuW(smenu, MF_STRING, IDCANCEL, L"退出(&X)");
		AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)smenu, L"文件(&F)");
		smenu = CreatePopupMenu();
		AppendMenuW(smenu, MF_STRING, IDM_EDIT_DUPLICATE, L"复制(&C)");
		AppendMenuW(smenu, MF_STRING, IDM_EDIT_DELETE, L"删除(&D)");
		AppendMenuW(smenu, MF_SEPARATOR, 0, 0);
		AppendMenuW(smenu, MF_STRING, IDM_EDIT_MOVE_UP, L"上移(&U)");
		AppendMenuW(smenu, MF_STRING, IDM_EDIT_MOVE_DOWN, L"下移(&N)");
		AppendMenuW(smenu, MF_STRING, IDM_EDIT_MOVE_LEFT, L"左移(&L)");
		AppendMenuW(smenu, MF_STRING, IDM_EDIT_MOVE_RIGHT, L"右移(&R)");
		AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)smenu, L"编辑(&E)");
		smenu = CreatePopupMenu();
		AppendMenuW(smenu, MF_STRING, IDM_THEME_MICA, L"Mica(&M)");
		AppendMenuW(smenu, MF_STRING, IDM_THEME_ACRYLIC, L"Acrylic(&A)");
		AppendMenuW(smenu, MF_STRING, IDM_THEME_TABBED, L"Tabbed(&T)");
		AppendMenuW(smenu, MF_SEPARATOR, 0, 0);
		AppendMenuW(smenu, MF_STRING, IDM_THEME_DEFAULT, L"Default(&D)");
		AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)smenu, L"主题(&T)");
		SetMenu(hwnd, hMenu);

		// 工具箱标签
		HWND tmph = CreateWindowExW(0, L"Static", L"工具箱",
			WS_CHILD | WS_VISIBLE | SS_CENTER,
			2, 1, 200, 23, hwnd, 0, 0, 0);

		HFONT hFont = CreateFontW(23, 0, 0, 0, FW_BOLD, 0, 0, 0, 1, 0, 0, 0, 0, L"微软雅黑");
		SendMessageW(tmph, WM_SETFONT, WPARAM(hFont), 1);

		// 控件列表
		tmph = CreateWindowExW(0, L"ListBox", 0,
			WS_CHILD | WS_BORDER | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY,
			0, 25, 200, 0, hwnd, HMENU(IDC_TOOLBOX_LIST), 0, 0);

		for (auto& i : g_listdescription) {
			SendMessageW(tmph, LB_ADDSTRING, 0, (LPARAM)i);
		}

		hFont = CreateFontW(23, 0, 0, 0, FW_NORMAL, 0, 0, 0, 1, 0, 0, 0, 0, L"微软雅黑");
		SendMessageW(tmph, WM_SETFONT, WPARAM(hFont), 1);

		g_ChangePanelHwnd = CreateWindowExW(0, L"Static", L"",
			WS_CHILD | WS_VISIBLE | WS_BORDER,
			240, 0, 0, 0, hwnd, HMENU(IDC_CHANGE_PANEL), 0, 0);
		SetWindowLongPtrW(g_ChangePanelHwnd, -4, LONG_PTR(WNDPROC([](HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT {

			return DarkModeAPI::HandleWindow(hwnd, msg, wparam, lparam);
			})));
		g_ChangeListHwnd = CreateWindowExW(0, L"ListBox", 0,
			WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | LBS_NOTIFY,
			0, 0, 0, 0, g_ChangePanelHwnd, HMENU(IDC_CHANGE_LIST), 0, 0);
		g_ChangeEditHwnd = CreateWindowExW(WS_EX_CLIENTEDGE, L"Edit", L"",
			WS_CHILD | WS_VISIBLE | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL | ES_READONLY,
			0, 0, 0, 0, g_ChangePanelHwnd, HMENU(IDC_CHANGE_EDIT), 0, 0);
		SendMessageW(g_ChangeListHwnd, WM_SETFONT, WPARAM(hFont), 1);
		SendMessageW(g_ChangeEditHwnd, WM_SETFONT, WPARAM(hFont), 1);
		SyncControlSummaryPanel();

		tmph = CreateWindowExW(0, L"Static", L"Made by Yeskunkun",
			WS_CHILD | WS_BORDER | WS_VISIBLE | SS_CENTER,
			0, 25, 200, 0, hwnd, HMENU(IDC_STATUS_BAR), 0, 0);
		SendMessageW(tmph, WM_SETFONT, WPARAM(hFont), 1);
		CreateThread(0, 0, [](LPVOID)->DWORD {
			SendMessageW(g_ChangePanelHwnd, WM_SETTINGCHANGE, 0, 0);
			return 0; }, 0, 0, 0);
		break;
	}

	case WM_COMMAND: {
		DWORD cmd = LOWORD(wparam);
		if (cmd == IDCANCEL) {
			PostQuitMessage(0);
		}
		if (cmd == IDM_EDIT_DUPLICATE) {
			ExecuteDuplicateSelectedControl();
		}
		else if (cmd == IDM_EDIT_DELETE) {
			ExecuteDeleteSelectedControl();
		}
		else if (cmd == IDM_EDIT_MOVE_UP) {
			ExecuteMoveSelectedControl(0, -1);
		}
		else if (cmd == IDM_EDIT_MOVE_DOWN) {
			ExecuteMoveSelectedControl(0, 1);
		}
		else if (cmd == IDM_EDIT_MOVE_LEFT) {
			ExecuteMoveSelectedControl(-1, 0);
		}
		else if (cmd == IDM_EDIT_MOVE_RIGHT) {
			ExecuteMoveSelectedControl(1, 0);
		}
		else if (cmd == IDM_THEME_MICA) {
			DarkMode::UseMica = true;
			DarkMode::value = DarkMode::SystemBackdropType::Mica;
			CreateThread(0, 0, [](LPVOID)->DWORD {
				SendMessageW(hwndParent, WM_SETTINGCHANGE, 0, 0);
				return 0; }, 0, 0, 0);
		}
		else if (cmd == IDM_THEME_ACRYLIC) {
			DarkMode::UseMica = true;
			DarkMode::value = DarkMode::SystemBackdropType::Acrylic;
			CreateThread(0, 0, [](LPVOID)->DWORD {
				SendMessageW(hwndParent, WM_SETTINGCHANGE, 0, 0);
				return 0; }, 0, 0, 0);
		}
		else if (cmd == IDM_THEME_TABBED) {
			DarkMode::UseMica = true;
			DarkMode::value = DarkMode::SystemBackdropType::Tabbed;
			CreateThread(0, 0, [](LPVOID)->DWORD {
				SendMessageW(hwndParent, WM_SETTINGCHANGE, 0, 0);
				return 0; }, 0, 0, 0);
		}
		else if (cmd == IDM_THEME_DEFAULT) {
			DarkMode::UseMica = false;
			CreateThread(0, 0, [](LPVOID)->DWORD {
				SendMessageW(hwndParent, WM_SETTINGCHANGE, 0, 0);
				return 0; }, 0, 0, 0);
		}
		if (cmd == IDC_TOOLBOX_LIST && HIWORD(wparam) == LBN_SELCHANGE) {
			int index = SendMessageW(GetDlgItem(hwnd, IDC_TOOLBOX_LIST), LB_GETCURSEL, 0, 0);
			if (index != -1) {
				wprintf(L"[DEBUG] Selected: %s\n", g_listdescription[index]);
			}
		}
		else if (cmd == IDC_CHANGE_LIST && HIWORD(wparam) == LBN_SELCHANGE) {
			int index = (int)SendMessageW(g_ChangeListHwnd, LB_GETCURSEL, 0, 0);
			if (index >= 0 && index < (int)g_Controls.size()) {
				int prev = g_CurIndex;
				if (prev != -1 && prev < (int)g_Controls.size()) {
					InvalidateRect(g_Controls[prev].hwnd, NULL, TRUE);
					UpdateWindow(g_Controls[prev].hwnd);
				}
				g_CurIndex = index;
				BringToFront(index);
				SetFocus(g_Controls[index].hwnd);
				InvalidateRect(g_Controls[index].hwnd, NULL, FALSE);
				UpdateWindow(g_Controls[index].hwnd);
				UpdatePropertyPanel(&g_Controls[index]);
				SyncControlSummaryPanel();
			}
		}
		break;
	}

	case WM_SIZE: {
		RECT rcwin;
		GetClientRect(hwnd, &rcwin);
		const int statusHeight = 24;
		const int changePanelHeight = 200;
		const int leftPanelWidth = 240;
		const int rightPanelWidth = 280;
		rcwin.bottom -= statusHeight;
		// 调整列表大小
		HWND hList = GetDlgItem(hwnd, IDC_TOOLBOX_LIST);
		if (hList) {
			SetWindowPos(hList, 0, 0, 25, leftPanelWidth, rcwin.bottom - changePanelHeight - statusHeight,
				SWP_NOZORDER | SWP_NOACTIVATE);
		}

		int centerWidth = max(0, rcwin.right);
		int designerHeight = max(0, rcwin.bottom - changePanelHeight);

		// 调整设计器窗口
		if (g_HWND) {
			SetWindowPos(g_HWND, 0, leftPanelWidth, 0,
				centerWidth - leftPanelWidth - rightPanelWidth, designerHeight,
				SWP_NOZORDER | SWP_NOACTIVATE);
			SIZE dsize = GetWindowSize(g_HWND);
			printf("[DEBUG] dsize={%d,%d}\n", dsize.cx, dsize.cy);
			AppendControlChangeMarker();
		}

		if (g_ChangePanelHwnd) {
			SetWindowPos(g_ChangePanelHwnd, 0, 0, designerHeight,
				GetWindowSize(hwndParent, false).cx - rightPanelWidth, changePanelHeight,
				SWP_NOZORDER | SWP_NOACTIVATE);
			int listWidth = centerWidth / 3;
			if (g_ChangeListHwnd) {
				SetWindowPos(g_ChangeListHwnd, 0, 0, 0, listWidth, changePanelHeight,
					SWP_NOZORDER | SWP_NOACTIVATE);
			}
			if (g_ChangeEditHwnd) {
				SetWindowPos(g_ChangeEditHwnd, 0, listWidth, 0, centerWidth - listWidth, changePanelHeight,
					SWP_NOZORDER | SWP_NOACTIVATE);
			}
		}

		// 调整属性窗口
		if (g_PropertiesHWND) {
			SetWindowPos(g_PropertiesHWND, 0,
				GetWindowSize(hwndParent, false).cx - rightPanelWidth, 0,
				rightPanelWidth, rcwin.bottom,
				SWP_NOZORDER | SWP_NOACTIVATE);
		}

		hList = GetDlgItem(hwnd, IDC_STATUS_BAR);
		if (hList) {
			SetWindowPos(hList, 0, 0, rcwin.bottom, GetWindowSize(hwndParent, false).cx, statusHeight, SWP_NOZORDER);
			InvalidateRect(hList, 0, true);
		}
		break;
	}

	case WM_DESTROY: {
		PostQuitMessage(0);
		break;
	}
	}

	return DarkModeAPI::HandleWindow(hwnd, msg, wparam, lparam);
}

// ==================== 主函数 ====================
int main() {
	DarkMode::value = DarkMode::SystemBackdropType::Mica;
	InstallCBTHook(true, 1);
	// 注册主窗口类
	WNDCLASSEXW wc = { sizeof(WNDCLASSEXW) };
	wc.lpfnWndProc = MainWindowProc;
	wc.hInstance = GetModuleHandleW(0);
	wc.hCursor = LoadCursorW(0, IDC_ARROW);
	wc.lpszClassName = L"WindowClass";
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	RegisterClassExW(&wc);
	// 注册设计器窗口类
	wc.lpszClassName = L"DialogDesigner";
	wc.lpfnWndProc = [](HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT {
		switch (msg) {
		case WM_SYSCOMMAND: {
			return 1;
		}
		case WM_ERASEBKGND: {
			RECT rect;
			GetClientRect(hwnd, &rect);
			HBRUSH hBrush = CreateSolidBrush(DarkMode::CLR_BG_MAIN);
			FillRect((HDC)wparam, &rect, hBrush);
			DeleteObject(hBrush);
			return 1;
		}
		case WM_LBUTTONDOWN: {
			int x = GET_X_LPARAM(lparam);
			int y = GET_Y_LPARAM(lparam);
			POINT pt = { x, y };
			ClientToScreen(hwnd, &pt);
			HWND hChild = ChildWindowFromPoint(hwnd, pt);
			if (hChild == 0 || hChild == hwnd) {
				HWND hList = GetDlgItem(hwndParent, 3);
				int index = SendMessageW(hList, LB_GETCURSEL, 0, 0);
				if (index > 0 && index < (int)g_listclassname.size()) {
					// 检查是否已有控件在该位置
					bool found = false;
					for (auto& ctl : g_Controls) {
						RECT rc;
						GetWindowRect(ctl.hwnd, &rc);
						if (x >= rc.left && x <= rc.right && y >= rc.top && y <= rc.bottom) {
							found = true;
							break;
						}
					}
					if (!found) {
						Control tmph = CreateControlWithProc(hwnd, g_listclassname[index], L"Example", x, y, 100, 50);
					}
				}
				else {
					g_CurIndex = -1;
					InvalidateRect(hwnd, 0, true);
					SetFocus(hwnd);
				}
			}
			break;
		}
		case WM_DESTROY: {
			PostQuitMessage(0);
			break;
		}
		}
		return DarkModeAPI::HandleWindow(hwnd, msg, wparam, lparam);
		};
	RegisterClassExW(&wc);
	// 注册属性窗口类
	wc.lpszClassName = L"PropertiesPanel";
	wc.lpfnWndProc = [](HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT {
		switch (msg) {
		case WM_CREATE: {
			HWND tmph = CreateWindowExW(0, L"Static", L"属性",
				WS_CHILD | WS_VISIBLE | SS_CENTER,
				2, 1, 280, 23, hwnd, 0, 0, 0);
			SendMessageW(tmph, WM_SETFONT, WPARAM(CreateFontA(20, 0, 0, 0, FW_BOLD, 0, 0, 0, 1, 0, 0, 0, 0, "微软雅黑")), 1);
			HFONT hFont = CreateFontW(23, 0, 0, 0, FW_BOLD, 0, 0, 0, 1, 0, 0, 0, 0, L"微软雅黑");
			SendMessageW(tmph, WM_SETFONT, WPARAM(hFont), 1);
			auto createcontent = [&](LPCWSTR cont, int id) {
				int flag = 0;
				if (id >= 101 && id <= 104) {
					flag = ES_NUMBER;
				}
				tmph = CreateWindowExW(0, L"Static", cont, WS_CHILD | WS_VISIBLE | SS_LEFT, 3, 30 + 30 * (id - 100), 116, 26, hwnd, HMENU(0), 0, 0);
				SendMessageW(tmph, WM_SETFONT, WPARAM(CreateFontA(25, 0, 0, 0, FW_NORMAL, 0, 0, 0, 1, 0, 0, 0, 0, "微软雅黑")), 1);
				if (id == IDC_PROP_FONT_BTN) {
					tmph = CreateWindowExW(0, L"Button", L"", WS_CHILD | WS_VISIBLE | BS_CENTER | BS_PUSHBOX, 120, 30 + 30 * (id - 100), 154, 28, hwnd,
						HMENU(IDC_PROP_FONT_BTN), 0, 0);
					SendMessageW(tmph, WM_SETFONT, WPARAM(CreateFontA(22, 0, 0, 0, FW_NORMAL, 0, 0, 0, 1, 0, 0, 0, 0, "微软雅黑")), 1);
				}
				else {
					tmph = CreateWindowExW(0, L"Edit", L"", flag | WS_CHILD | WS_VISIBLE | ES_LEFT | ES_AUTOHSCROLL | WS_BORDER, 120, 30 + 30 * (id - 100), 154, 28, hwnd,
						HMENU(id), 0, 0);
					SendMessageW(tmph, WM_SETFONT, WPARAM(CreateFontA(22, 0, 0, 0, FW_NORMAL, 0, 0, 0, 1, 0, 0, 0, 0, "微软雅黑")), 1);
				}
				};
			createcontent(L"标题", IDC_PROP_TITLE);
			createcontent(L"X", IDC_PROP_X);
			createcontent(L"Y", IDC_PROP_Y);
			createcontent(L"宽度", IDC_PROP_WIDTH);
			createcontent(L"高度", IDC_PROP_HEIGHT);
			createcontent(L"类名", IDC_PROP_CLASS);
			createcontent(L"字体", IDC_PROP_FONT_BTN);
			UpdatePropertyPanel(nullptr);
			break;
		}
		case WM_COMMAND: {
			if (HIWORD(wparam) == EN_CHANGE) {
				if (g_CurIndex < 0 || g_CurIndex >= (int)g_Controls.size()) {
					// 如果没有选中控件，只处理对话框大小
					if (LOWORD(wparam) == IDC_PROP_WIDTH || LOWORD(wparam) == IDC_PROP_HEIGHT) {
						// 延迟应用，避免频繁调整
						SetTimer(hwnd, 9999, 200, NULL);
					}
					break;
				}

				Control* ctl = &g_Controls[g_CurIndex];
				if (!ctl) break;

				wchar_t buf[32];
				int newValue;
				//printf("[DEBUG] Property change: %d\n", LOWORD(wparam));
				switch (LOWORD(wparam)) {
				case IDC_PROP_TITLE: {
					wchar_t titleBuf[256];
					GetDlgItemTextW(hwnd, IDC_PROP_TITLE, titleBuf, 256);
					if (wcslen(titleBuf) > 0) {
						ctl->title = titleBuf;
						SetWindowTextW(ctl->hwnd, titleBuf);
					}
					break;
				}

				case IDC_PROP_X: {
					GetDlgItemTextW(hwnd, IDC_PROP_X, buf, 32);
					newValue = _wtoi(buf);
					if (newValue >= 0) {
						ctl->x = newValue;
						SetWindowPos(ctl->hwnd, 0, newValue, ctl->y, 0, 0,
							SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
					}
					break;
				}

				case IDC_PROP_Y: {
					GetDlgItemTextW(hwnd, IDC_PROP_Y, buf, 32);
					newValue = _wtoi(buf);
					if (newValue >= 0) {
						ctl->y = newValue;
						SetWindowPos(ctl->hwnd, 0, ctl->x, newValue, 0, 0,
							SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
					}
					break;
				}

				case IDC_PROP_WIDTH: {
					GetDlgItemTextW(hwnd, IDC_PROP_WIDTH, buf, 32);
					newValue = _wtoi(buf);
					if (newValue > 10) {
						ctl->width = newValue;
						SetWindowPos(ctl->hwnd, 0, 0, 0, newValue, ctl->height,
							SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
						InvalidateRect(ctl->hwnd, 0, true);
					}
					break;
				}

				case IDC_PROP_HEIGHT: {
					GetDlgItemTextW(hwnd, IDC_PROP_HEIGHT, buf, 32);
					newValue = _wtoi(buf);
					if (newValue > 10) {
						ctl->height = newValue;
						SetWindowPos(ctl->hwnd, 0, 0, 0, ctl->width, newValue,
							SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
						InvalidateRect(ctl->hwnd, 0, true);
					}
					break;
				}
				}
			}
			else {
				switch (LOWORD(wparam)) {
				case IDC_PROP_FONT_BTN: {
					if (g_CurIndex < g_Controls.size()) {
						Control* ctl = &g_Controls[g_CurIndex];
						HFONT hOldFont = HFONT(SendMessageW(ctl->hwnd, WM_GETFONT, 0, 0));
						HFONT hFont = ChooseFontAndCreate(hwnd, hOldFont);
						if (hFont) {
							if (ctl->hFont && ctl->hFont != g_hGuiFont && ctl->hFont != hOldFont) {
								DeleteObject(ctl->hFont);
							}
							ctl->hFont = hFont;
							SendMessageW(ctl->hwnd, WM_SETFONT, WPARAM(hFont), 1);
							AppendControlChangeMarker();
						}
						UpdatePropertyPanel(ctl);
					}
					break;
				}
				}
			}
			break;
		}
		case WM_TIMER: {
			if (wparam == 9999) {
				KillTimer(hwnd, 9999);
				// 应用对话框大小变化
				if (g_CurIndex < 0 || g_CurIndex >= (int)g_Controls.size()) {
					HWND hW = GetDlgItem(hwnd, IDC_PROP_WIDTH);
					HWND hH = GetDlgItem(hwnd, IDC_PROP_HEIGHT);
					if (hW && hH) {
						wchar_t bufW[32], bufH[32];
						GetWindowTextW(hW, bufW, 32);
						GetWindowTextW(hH, bufH, 32);
						int newW = _wtoi(bufW);
						int newH = _wtoi(bufH);
						if (newW > 100 && newH > 100) {
							ResizeWindow(g_HWND, newW, newH);
						}
					}
				}
			}
			break;
		}
		case WM_ERASEBKGND: {
			RECT rect;
			GetClientRect(hwnd, &rect);
			HBRUSH hBrush = CreateSolidBrush(DarkMode::CLR_BG_MAIN);
			FillRect((HDC)wparam, &rect, hBrush);
			DeleteObject(hBrush);
			return 1;
		}

		case WM_DESTROY: {
			PostQuitMessage(0);
			break;
		}
		}
		return DarkModeAPI::HandleWindow(hwnd, msg, wparam, lparam);
		};
	RegisterClassExW(&wc);

	// 创建主窗口
	hwndParent = CreateWindowExW(WS_EX_CONTROLPARENT, L"WindowClass",
		L"Dialog Designer by Yeskunkun",
		WS_OVERLAPPEDWINDOW | 0,
		CW_USEDEFAULT, CW_USEDEFAULT, 0, 0,
		0, 0, wc.hInstance, 0);

	if (!hwndParent) return 0;

	// 创建设计器窗口
	g_HWND = CreateWindowExW(WS_EX_CONTROLPARENT, L"DialogDesigner", L"Form1",
		WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_CHILD,
		240, 0, START_WIDTH, START_HEIGHT,
		hwndParent, 0, wc.hInstance, 0);
	SendMessageW(g_HWND, WM_SETTINGCHANGE, 0, 0);
	// 创建属性窗口
	g_PropertiesHWND = CreateWindowExW(WS_EX_CONTROLPARENT, L"PropertiesPanel", L"属性窗口",
		0 | WS_VISIBLE | WS_CHILD,
		0, 0, 250, 400,
		hwndParent, HMENU(0xFFFA), wc.hInstance, 0);
	RECT rect = { 0, 0, START_WIDTH, START_HEIGHT };
	AdjustWindowRectEx(&rect, GetWindowLongPtrW(g_HWND, -16), false, GetWindowLongPtrW(g_HWND, -20));
	SetWindowSize(hwndParent, 240 + 280 + rect.right - rect.left, rect.bottom - rect.top + 24 + 200, false);
	// 触发布局更新
	SendMessageW(hwndParent, WM_SETTINGCHANGE, 0, 0);
	ShowWindow(hwndParent, true);
	// 消息循环
	MSG msg;
	while (GetMessageW(&msg, 0, 0, 0)) {
		if (msg.message == WM_KEYDOWN) {
			// 如果焦点位于编辑框，跳过全局快捷键处理
			HWND hFocus = GetFocus();
			wchar_t cls[64] = {};
			if (hFocus) GetClassNameW(hFocus, cls, 64);
			if (wcscmp(cls, L"Edit") == 0) {
				TranslateMessage(&msg);
				DispatchMessageW(&msg);
				continue;
			}

			// Ctrl+C 复制
			if ((GetKeyState(VK_CONTROL) & 0x8000) && msg.wParam == 'C') {
				ExecuteDuplicateSelectedControl();
				continue;
			}

			// 控件移动 / 删除
			switch (msg.wParam) {
			case VK_UP:    ExecuteMoveSelectedControl(0, -1); break;
			case VK_DOWN:  ExecuteMoveSelectedControl(0, 1); break;
			case VK_LEFT:  ExecuteMoveSelectedControl(-1, 0); break;
			case VK_RIGHT: ExecuteMoveSelectedControl(1, 0); break;
			case VK_DELETE:
				ExecuteDeleteSelectedControl();
				break;
			}
		}

		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}

	return msg.wParam;
}