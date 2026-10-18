#pragma once
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <commdlg.h>
#include <dwmapi.h>
#include <uxtheme.h>
#include <cstdio>
#include <cstdarg>
#include <vector>
#include <string>
#include <unordered_set>
#include <thread>
#include <Richedit.h>
#include <tlhelp32.h>
#pragma comment(lib, "Comctl32.lib")
#pragma comment(lib, "Comdlg32.lib")
#pragma comment(lib, "UxTheme.lib")
#pragma comment(lib, "Dwmapi.lib")
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
using namespace std;
typedef int(__stdcall* SetPreferredAppModeProc)(int);
SetPreferredAppModeProc SetPreferredAppMode = (SetPreferredAppModeProc)GetProcAddress(LoadLibraryExW(L"uxtheme.dll", 0, 2048), MAKEINTRESOURCEA(135));
typedef NTSTATUS(__stdcall* PRtlGetVersion)(PRTL_OSVERSIONINFOEXW);
PRtlGetVersion RtlGetVersion = (PRtlGetVersion)GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "RtlGetVersion");
RTL_OSVERSIONINFOEXW os = { sizeof(RTL_OSVERSIONINFOEXW) };
//static const LPCWSTR DARK_MODE_PROP = L"__DarkModeApplied__";
//static const LPCWSTR DARK_SUBCLASS_PROP = L"__DarkSubclassed__";
static unordered_set<HWND> g_darkModeAppliedWindows;;
static bool IsDarkModeApplied(HWND hWnd) {
	//return g_darkModeAppliedWindows.find(hWnd) != g_darkModeAppliedWindows.end();
	return 0;
}
static void SetDarkModeApplied(HWND hWnd) {
	g_darkModeAppliedWindows.insert(hWnd);
}
namespace DarkModeAPI {
	void SetFont(const wstring& fontName, const wstring& fontSize, int fontWeight = FW_NORMAL);
	void SetFont(HWND hWnd, const wstring& fontName, const wstring& fontSize, int fontWeight = FW_NORMAL);
	void Apply(HWND hCtrl, bool applyDefaultFont = true);
	bool IsPopupDialogWindow(HWND hWnd);
	void InitWindow(HWND hParent);
	void ApplyToAll(HWND hParent);
	void PaintBackground(HWND hwnd, HDC hdc);
	LRESULT HandleCtlColor(UINT msg, WPARAM wParam, LPARAM lParam);
	void CleanUp();
}

namespace DarkMode {
	enum class SystemBackdropType { Auto, None, Mica, Acrylic, Tabbed } value;
	bool UseMica = false, Subclassbtn = true;
	bool ForceDarkInInjectedProcess = false;
	COLORREF CLR_BG_MAIN = UseMica ? 0 : RGB(32, 32, 32);
	COLORREF CLR_BG_CTRL = CLR_BG_MAIN;
	COLORREF CLR_BG_CTRL_HOT = RGB(60, 60, 64);
	COLORREF CLR_BG_CTRL_DOWN = RGB(96, 96, 108);
	COLORREF CLR_BG_TAB = CLR_BG_MAIN;
	COLORREF CLR_BG_TAB_SEL = RGB(62, 62, 68);
	COLORREF CLR_BG_TAB_HOVER = RGB(58, 58, 64);
	COLORREF CLR_BG_TAB_TOUCH = RGB(90, 90, 102);
	COLORREF CLR_BG_TABPAGE = CLR_BG_MAIN;
	COLORREF CLR_BORDER = RGB(85, 85, 85);
	COLORREF CLR_BORDER_FOCUS = RGB(150, 150, 165);
	COLORREF CLR_ACCENT = RGB(0, 122, 204);
	COLORREF CLR_ACCENT_HOT = RGB(28, 151, 234);
	COLORREF CLR_TEXT = RGB(230, 230, 230);
	COLORREF CLR_TEXT_DIM = RGB(180, 180, 180);
	COLORREF CLR_BG_DISABLED = 0x383838;

	HFONT  hFont = 0;
	HBRUSH brMain = 0;
	HBRUSH brCtrl = 0;
	HBRUSH brTabPage = 0;
	HBRUSH brTabHost = 0;

	static const UINT_PTR SUBCLASS_ID_BUTTON = 101;
	static const UINT_PTR SUBCLASS_ID_CHECKBOX = 102;
	static const UINT_PTR SUBCLASS_ID_TAB = 103;
	static const UINT_PTR SUBCLASS_ID_LIST = 104;
	static const UINT_PTR SUBCLASS_ID_LISTVIEW = 105;
	static const UINT_PTR SUBCLASS_ID_HEADER = 106;
	static const UINT_PTR SUBCLASS_ID_DIALOG = 107;
	static const UINT_PTR SUBCLASS_ID_COMBO = 108;
	static const UINT_PTR SUBCLASS_ID_UPDOWN = 109;
	static const UINT_PTR SUBCLASS_ID_GROUPBOX = 110;

	HHOOK  hCbtHook = 0;
	int isDarkThemeEnabled = true;
	bool EnableTabPreload = true;
	bool TabPreloaded = false;
}
static bool ReadAppsUseLightTheme() {
	if (DarkMode::ForceDarkInInjectedProcess) {
		return true;
	}
	DWORD val = 0;
	DWORD cb = sizeof(val);
	LSTATUS st = RegGetValueW(HKEY_CURRENT_USER,
		L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
		L"AppsUseLightTheme", RRF_RT_REG_DWORD, 0, &val, &cb);
	if (st != ERROR_SUCCESS) {
		return 1; // 默认
	}
	return val == 0; // AppsUseLightTheme==0 -> dark
}
static void ApplyThemeColors(HWND hwnd) {
	DarkMode::isDarkThemeEnabled = ReadAppsUseLightTheme();

	if (DarkMode::isDarkThemeEnabled) {
		// === 深色模式（现代深色主题） ===
		// 背景色
		DarkMode::CLR_BG_MAIN = DarkMode::UseMica ? 0 : RGB(32, 32, 32);        // 主背景
		DarkMode::CLR_BG_CTRL = DarkMode::CLR_BG_MAIN;        // 控件背景
		DarkMode::CLR_BG_CTRL_HOT = RGB(60, 60, 68);    // 控件悬停
		DarkMode::CLR_BG_CTRL_DOWN = RGB(80, 80, 92);   // 控件按下

		// Tab 控件
		DarkMode::CLR_BG_TAB = DarkMode::CLR_BG_MAIN;          // Tab 背景
		DarkMode::CLR_BG_TAB_SEL = RGB(50, 50, 58);      // Tab 选中
		DarkMode::CLR_BG_TAB_HOVER = RGB(45, 45, 52);    // Tab 悬停
		DarkMode::CLR_BG_TAB_TOUCH = RGB(72, 72, 85);    // Tab 触摸/按下
		DarkMode::CLR_BG_TABPAGE = RGB(38, 38, 42);      // TabPage 背景

		// 边框
		DarkMode::CLR_BORDER = RGB(70, 70, 78);          // 普通边框
		DarkMode::CLR_BORDER_FOCUS = RGB(130, 150, 220); // 焦点边框（亮蓝）

		// 强调色
		DarkMode::CLR_ACCENT = RGB(0, 120, 212);          // 主强调色（Win11蓝）
		DarkMode::CLR_ACCENT_HOT = RGB(40, 150, 240);     // 强调色悬停

		// 文本
		DarkMode::CLR_TEXT = RGB(235, 235, 245);          // 主文本
		DarkMode::CLR_TEXT_DIM = RGB(160, 165, 175);      // 次要文本
		DarkMode::CLR_BG_DISABLED = RGB(54, 54, 58);      // 禁用背景

		// 额外颜色（如果有需要）
		//DarkMode::CLR_BG_HEADER = RGB(38, 38, 42);        // 标题栏/头部背景
		//DarkMode::CLR_BG_ITEM = RGB(45, 45, 50);          // 列表项背景
		//DarkMode::CLR_BG_ITEM_SEL = RGB(0, 90, 160);      // 列表项选中
		//DarkMode::CLR_BG_ITEM_HOVER = RGB(55, 55, 63);    // 列表项悬停
		//DarkMode::CLR_LINE = RGB(55, 55, 60);             // 分割线
		//DarkMode::CLR_SHADOW = RGB(0, 0, 0, 80);          // 阴影（带透明度）
	}
	else {
		// === 浅色模式（清新现代风格） ===
		// 背景色 - 柔和米白/蓝灰色系
		DarkMode::CLR_BG_MAIN = RGB(246, 248, 252);       // 主背景
		DarkMode::CLR_BG_CTRL = DarkMode::CLR_BG_MAIN;       // 控件背景
		DarkMode::CLR_BG_CTRL_HOT = RGB(233, 242, 255);   // 控件悬停
		DarkMode::CLR_BG_CTRL_DOWN = RGB(214, 228, 248);  // 控件按下

		// Tab 控件
		DarkMode::CLR_BG_TAB = DarkMode::CLR_BG_MAIN;        // Tab 背景
		DarkMode::CLR_BG_TAB_SEL = RGB(232, 242, 255);    // Tab 选中（淡蓝）
		DarkMode::CLR_BG_TAB_HOVER = RGB(238, 246, 255);  // Tab 悬停
		DarkMode::CLR_BG_TAB_TOUCH = RGB(210, 228, 250);  // Tab 触摸/按下
		DarkMode::CLR_BG_TABPAGE = RGB(252, 253, 255);    // TabPage 背景（纯白微蓝）

		// 边框 - 浅灰蓝
		DarkMode::CLR_BORDER = RGB(200, 210, 225);        // 普通边框
		DarkMode::CLR_BORDER_FOCUS = RGB(0, 120, 215);    // 焦点边框（深蓝）

		// 强调色 - 现代蓝色系
		DarkMode::CLR_ACCENT = RGB(0, 110, 200);           // 主强调色
		DarkMode::CLR_ACCENT_HOT = RGB(0, 130, 230);      // 强调色悬停

		// 文本 - 深灰蓝
		DarkMode::CLR_TEXT = RGB(28, 33, 42);              // 主文本（深灰蓝）
		DarkMode::CLR_TEXT_DIM = RGB(108, 122, 142);       // 次要文本
		DarkMode::CLR_BG_DISABLED = RGB(236, 240, 246);    // 禁用背景

		// 额外颜色
		//DarkMode::CLR_BG_HEADER = RGB(242, 245, 250);      // 标题栏/头部背景
		//DarkMode::CLR_BG_ITEM = RGB(248, 250, 252);        // 列表项背景
		//DarkMode::CLR_BG_ITEM_SEL = RGB(0, 110, 200);      // 列表项选中
		//DarkMode::CLR_BG_ITEM_HOVER = RGB(235, 243, 255);  // 列表项悬停
		//DarkMode::CLR_LINE = RGB(215, 225, 235);           // 分割线
		//DarkMode::CLR_SHADOW = RGB(0, 0, 0, 30);           // 阴影（带透明度）
	}

	// 删除旧画刷并让 DarkMode API 依据新的颜色重建画刷
	if (DarkMode::brMain) { DeleteObject(DarkMode::brMain); DarkMode::brMain = 0; }
	if (DarkMode::brCtrl) { DeleteObject(DarkMode::brCtrl); DarkMode::brCtrl = 0; }
	if (DarkMode::brTabPage) { DeleteObject(DarkMode::brTabPage); DarkMode::brTabPage = 0; }
	if (DarkMode::brTabHost) { DeleteObject(DarkMode::brTabHost); DarkMode::brTabHost = 0; }

	DarkMode::brMain = CreateSolidBrush(DarkMode::CLR_BG_MAIN);
	DarkMode::brCtrl = CreateSolidBrush(DarkMode::CLR_BG_CTRL);
	DarkMode::brTabPage = CreateSolidBrush(DarkMode::CLR_BG_TABPAGE);
	DarkMode::brTabHost = CreateSolidBrush(DarkMode::CLR_BG_MAIN);

	//DarkModeAPI::InitWindow(hwnd);

	// 强制立即重绘窗口及其子控件，允许背景擦除以便及时更新背景色
	RedrawWindow(hwnd, 0, 0, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN | RDW_UPDATENOW);
}
// ============================================================================
// 1. 普通按钮独立状态与绘制
// ============================================================================
struct DarkButtonState {
	bool mouseHot = false, mouseDown = false, touchTracking = false, touchArmed = false,
		touchInsideNow = false, touchPressed = false;
	DWORD touchId = 0;
};
static void BtnFireClick(HWND hWnd) {
	HWND target = GetParent(hWnd);
	if (!target) target = GetAncestor(hWnd, GA_ROOT);
	if (!target) return;
	int cid = GetDlgCtrlID(hWnd);
	PostMessageW(target, WM_COMMAND, MAKEWPARAM(cid, BN_CLICKED), (LPARAM)hWnd);
}
static void InvalidateWindowAndChildren(HWND hWnd) {
	if (!hWnd || !IsWindow(hWnd)) return;
	RedrawWindow(hWnd, 0, 0, RDW_INVALIDATE | RDW_NOERASE | RDW_ALLCHILDREN);
}
static LRESULT CALLBACK DarkButtonSubclassProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
	auto* state = (DarkButtonState*)dwRefData;
	if (!state && msg != WM_NCDESTROY) {
		return DefSubclassProc(hWnd, msg, wParam, lParam);
	}
	switch (msg) {

	case WM_PAINT: {
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);
		RECT rc;
		GetClientRect(hWnd, &rc);

		HDC memDC = CreateCompatibleDC(hdc);
		HBITMAP memBmp = CreateCompatibleBitmap(hdc, rc.right - rc.left, rc.bottom - rc.top);
		HBITMAP oldBmp = (HBITMAP)SelectObject(memDC, memBmp);

		bool isDown = state->mouseDown || state->touchPressed, isHot = state->mouseHot,
			isDisabled = !IsWindowEnabled(hWnd);

		COLORREF bg = isDisabled ? DarkMode::CLR_BG_DISABLED : isDown ? DarkMode::CLR_BG_CTRL_DOWN : isHot ? DarkMode::CLR_BG_CTRL_HOT : DarkMode::CLR_BG_CTRL;

		HBRUSH br = CreateSolidBrush(bg);
		FillRect(memDC, &rc, br);
		DeleteObject(br);

		HPEN pen = CreatePen(PS_SOLID, 1, isDown ? DarkMode::CLR_BORDER_FOCUS : DarkMode::CLR_BORDER);
		auto op = SelectObject(memDC, pen);
		auto ob = SelectObject(memDC, GetStockObject(NULL_BRUSH));
		Rectangle(memDC, rc.left, rc.top, rc.right, rc.bottom);
		SelectObject(memDC, ob);
		SelectObject(memDC, op);
		DeleteObject(pen);

		wchar_t txt[256]{};
		GetWindowTextW(hWnd, txt, 255);
		RECT tr = rc;

		SetBkMode(memDC, TRANSPARENT);
		SetTextColor(memDC, isDisabled ? DarkMode::CLR_TEXT_DIM : DarkMode::CLR_TEXT);

		// 【优化】：支持控件各自独立的字体 (WM_GETFONT)
		HFONT hCtrlFont = (HFONT)SendMessageW(hWnd, WM_GETFONT, 0, 0);
		if (!hCtrlFont) hCtrlFont = DarkMode::hFont;
		HFONT oldFont = hCtrlFont ? (HFONT)SelectObject(memDC, hCtrlFont) : 0;
		DrawTextW(memDC, txt, -1, &tr, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
		if (oldFont) SelectObject(memDC, oldFont);

		BitBlt(hdc, 0, 0, rc.right - rc.left, rc.bottom - rc.top, memDC, 0, 0, SRCCOPY);
		SelectObject(memDC, oldBmp);
		DeleteObject(memBmp);
		DeleteDC(memDC);

		EndPaint(hWnd, &ps);
		//return 0;
		break;
	}

	case WM_MOUSEMOVE: {
		if (!state->mouseHot) {
			state->mouseHot = true;
			TRACKMOUSEEVENT tme{ sizeof(tme), TME_LEAVE, hWnd, 0 };
			TrackMouseEvent(&tme);
			InvalidateRect(hWnd, 0, false);
		}
		if (state->touchTracking) {
			POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
			RECT rc; GetClientRect(hWnd, &rc);
			bool newInside = PtInRect(&rc, pt) != 0;
			bool newPressed = state->touchArmed && newInside;
			if (state->touchInsideNow != newInside || state->touchPressed != newPressed) {
				state->touchInsideNow = newInside;
				state->touchPressed = newPressed;
				InvalidateRect(hWnd, 0, false);
			}
		}
		break;
	}

	case WM_MOUSELEAVE: {
		if (state->mouseHot || state->mouseDown) {
			state->mouseHot = false;
			state->mouseDown = false;
			InvalidateRect(hWnd, 0, false);
		}
		break;
	}

	case WM_LBUTTONDOWN:
	case WM_LBUTTONDBLCLK: {
		SetCapture(hWnd);
		state->mouseDown = true;
		InvalidateRect(hWnd, 0, false);
		break;
	}

	case WM_LBUTTONUP: {
		if (GetCapture() == hWnd) {
			ReleaseCapture();
		}
		bool wasDown = state->mouseDown;
		state->mouseDown = false;
		InvalidateRect(hWnd, 0, false);
		if (wasDown) {
			POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
			RECT rc;
			GetClientRect(hWnd, &rc);
			if (PtInRect(&rc, pt)) {
				BtnFireClick(hWnd);
			}
		}
		break;
	}

	case WM_ENABLE: {
		state->mouseDown = false;
		state->touchTracking = false;
		state->touchArmed = false;
		state->touchInsideNow = false;
		state->touchPressed = false;
		state->touchId = 0;
		if (GetCapture() == hWnd) ReleaseCapture();
		InvalidateRect(hWnd, 0, false);
		break;
	}

	case WM_TOUCH: {
		UINT count = LOWORD(wParam);
		TOUCHINPUT ti[16];
		if (count > 16) count = 16;
		if (!GetTouchInputInfo((HTOUCHINPUT)lParam, count, ti, sizeof(TOUCHINPUT))) {
			CloseTouchInputHandle((HTOUCHINPUT)lParam);
			return 0;
		}

		for (UINT i = 0; i < count; ++i) {
			POINT pt{ TOUCH_COORD_TO_PIXEL(ti[i].x), TOUCH_COORD_TO_PIXEL(ti[i].y) };
			ScreenToClient(hWnd, &pt);
			RECT rc; GetClientRect(hWnd, &rc);
			const bool inside = PtInRect(&rc, pt) != 0;
			const DWORD id = ti[i].dwID;

			if (ti[i].dwFlags & TOUCHEVENTF_DOWN) {
				if (!state->touchTracking) {
					state->touchTracking = true;
					state->touchId = id;
					state->touchArmed = inside;
					state->touchInsideNow = inside;
					state->touchPressed = inside;
					SetCapture(hWnd);
					InvalidateRect(hWnd, 0, false);
				}
				continue;
			}

			if (!state->touchTracking || id != state->touchId) continue;

			if (ti[i].dwFlags & TOUCHEVENTF_MOVE) {
				bool newInside = inside;
				bool newPressed = state->touchArmed && newInside;
				if (state->touchInsideNow != newInside || state->touchPressed != newPressed) {
					state->touchInsideNow = newInside;
					state->touchPressed = newPressed;
					InvalidateRect(hWnd, 0, false);
				}
				continue;
			}

			if (ti[i].dwFlags & TOUCHEVENTF_UP) {
				ReleaseCapture();
				bool newInside = inside;
				const bool shouldClick = state->touchArmed && newInside;
				bool needsRedraw = state->touchTracking || state->touchArmed || state->touchInsideNow || state->touchPressed;
				state->touchTracking = false;
				state->touchArmed = false;
				state->touchInsideNow = false;
				state->touchPressed = false;
				state->touchId = 0;
				if (needsRedraw) {
					printf("GetCapture()=%p\n", GetCapture());
					InvalidateRect(hWnd, 0, false);
					//ReleaseCapture();
				}
				//if (shouldClick) BtnFireClick(hWnd);
				continue;
			}
		}

		CloseTouchInputHandle((HTOUCHINPUT)lParam);
		return 0;
	}

	case WM_CANCELMODE:
	case WM_KILLFOCUS: {
		if (state->touchTracking || state->touchPressed || state->mouseDown) {
			state->touchTracking = false;
			state->touchArmed = false;
			state->touchInsideNow = false;
			state->touchPressed = false;
			state->touchId = 0;
			state->mouseDown = false;
			state->mouseHot = false;
			if (GetCapture() == hWnd) ReleaseCapture();
			InvalidateRect(hWnd, 0, false);
		}
		break;
	}

	case WM_NCDESTROY:
		delete state;
		RemoveWindowSubclass(hWnd, DarkButtonSubclassProc, uIdSubclass);
		break;
	}

	return DefSubclassProc(hWnd, msg, wParam, lParam);
}
struct DarkComboState {
	bool mouseHot = false;
};
static LRESULT CALLBACK DarkComboSubclassProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
	auto* state = (DarkComboState*)dwRefData;
	if (!state && msg != WM_NCDESTROY) return DefSubclassProc(hWnd, msg, wParam, lParam);

	switch (msg) {
	case WM_ERASEBKGND:
		return 1;

	case WM_PAINT: {
		LONG_PTR style = GetWindowLongPtrW(hWnd, GWL_STYLE);
		if ((style & CBS_DROPDOWNLIST) == 0) return DefSubclassProc(hWnd, msg, wParam, lParam);

		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);
		RECT rc{};
		GetClientRect(hWnd, &rc);

		HDC memDC = CreateCompatibleDC(hdc);
		HBITMAP memBmp = CreateCompatibleBitmap(hdc, rc.right - rc.left, rc.bottom - rc.top);
		HBITMAP oldBmp = (HBITMAP)SelectObject(memDC, memBmp);

		bool isDisabled = !IsWindowEnabled(hWnd);
		bool dropped = SendMessageW(hWnd, CB_GETDROPPEDSTATE, 0, 0) != 0;
		bool isHot = state->mouseHot;
		COLORREF bg = isDisabled ? DarkMode::CLR_BG_DISABLED : dropped ? DarkMode::CLR_BG_CTRL_DOWN : isHot ? DarkMode::CLR_BG_CTRL_HOT : DarkMode::CLR_BG_CTRL;

		HBRUSH br = CreateSolidBrush(bg);
		FillRect(memDC, &rc, br);
		DeleteObject(br);

		HPEN borderPen = CreatePen(PS_SOLID, 1, dropped ? DarkMode::CLR_BORDER_FOCUS : DarkMode::CLR_BORDER);
		auto oldPen = SelectObject(memDC, borderPen);
		auto oldBr = SelectObject(memDC, GetStockObject(NULL_BRUSH));
		Rectangle(memDC, rc.left, rc.top, rc.right, rc.bottom);
		SelectObject(memDC, oldBr);
		SelectObject(memDC, oldPen);
		DeleteObject(borderPen);

		// 箭头区域总宽度固定为 16 像素
		int arrowWidth = 18;
		RECT arrowRc = rc;
		arrowRc.left = rc.right - arrowWidth;

		HPEN splitPen = CreatePen(PS_SOLID, 1, DarkMode::CLR_BORDER);
		oldPen = SelectObject(memDC, splitPen);
		MoveToEx(memDC, arrowRc.left, rc.top + 1, 0);
		LineTo(memDC, arrowRc.left, rc.bottom - 1);
		SelectObject(memDC, oldPen);
		DeleteObject(splitPen);

		// 绘制向下箭头（居中）
		int centerX = (arrowRc.left + arrowRc.right) / 2;
		int centerY = (arrowRc.top + arrowRc.bottom) / 2;
		int arrowSize = 4;  // 箭头大小

		POINT pts[3]{
			{ centerX - arrowSize, centerY - 2 },
			{ centerX + arrowSize, centerY - 2 },
			{ centerX,              centerY + 3 }
		};

		HBRUSH triBr = CreateSolidBrush(isDisabled ? DarkMode::CLR_TEXT_DIM : DarkMode::CLR_TEXT);
		auto oldTriBr = SelectObject(memDC, triBr);
		auto oldTriPen = SelectObject(memDC, GetStockObject(NULL_PEN));
		Polygon(memDC, pts, 3);
		SelectObject(memDC, oldTriPen);
		SelectObject(memDC, oldTriBr);
		DeleteObject(triBr);

		wchar_t text[256]{};
		int sel = (int)SendMessageW(hWnd, CB_GETCURSEL, 0, 0);
		if (sel >= 0) SendMessageW(hWnd, CB_GETLBTEXT, sel, (LPARAM)text);

		RECT tr = rc;
		tr.left += 4;
		tr.right = arrowRc.left - 6;
		SetBkMode(memDC, TRANSPARENT);
		SetTextColor(memDC, isDisabled ? DarkMode::CLR_TEXT_DIM : DarkMode::CLR_TEXT);
		HFONT hCtrlFont = (HFONT)SendMessageW(hWnd, WM_GETFONT, 0, 0);
		if (!hCtrlFont) hCtrlFont = DarkMode::hFont;
		HFONT oldFont = hCtrlFont ? (HFONT)SelectObject(memDC, hCtrlFont) : 0;
		DrawTextW(memDC, text, -1, &tr, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
		if (oldFont) SelectObject(memDC, oldFont);

		BitBlt(hdc, 0, 0, rc.right - rc.left, rc.bottom - rc.top, memDC, 0, 0, SRCCOPY);
		SelectObject(memDC, oldBmp);
		DeleteObject(memBmp);
		DeleteDC(memDC);
		EndPaint(hWnd, &ps);
		return 0;
	}

	case WM_MOUSEMOVE:
		if (!state->mouseHot) {
			state->mouseHot = true;
			TRACKMOUSEEVENT tme{ sizeof(tme), TME_LEAVE, hWnd, 0 };
			TrackMouseEvent(&tme);
			InvalidateRect(hWnd, 0, false);
		}
		break;

	case WM_MOUSELEAVE:
		if (state->mouseHot) {
			state->mouseHot = false;
			InvalidateRect(hWnd, 0, false);
		}
		break;

	case WM_SETFOCUS:
	case WM_KILLFOCUS:
	case WM_CAPTURECHANGED:
		InvalidateRect(hWnd, 0, false);
		break;

	case WM_ENABLE:
		state->mouseHot = false;
		InvalidateRect(hWnd, 0, false);
		break;

	case WM_NCDESTROY:
		delete state;
		RemoveWindowSubclass(hWnd, DarkComboSubclassProc, uIdSubclass);
		break;
	}

	return DefSubclassProc(hWnd, msg, wParam, lParam);
}
struct DarkUpDownState {
	bool touchTracking = false;
	DWORD touchId = 0;
};
static LRESULT CALLBACK DarkUpDownSubclassProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
	auto* state = (DarkUpDownState*)dwRefData;
	if (!state && msg != WM_NCDESTROY) return DefSubclassProc(hWnd, msg, wParam, lParam);

	switch (msg) {
	case WM_TOUCH: {
		UINT count = LOWORD(wParam);
		TOUCHINPUT ti[8];
		if (count > 8) count = 8;
		if (!GetTouchInputInfo((HTOUCHINPUT)lParam, count, ti, sizeof(TOUCHINPUT))) {
			CloseTouchInputHandle((HTOUCHINPUT)lParam);
			return 0;
		}

		for (UINT i = 0; i < count; ++i) {
			POINT pt{ TOUCH_COORD_TO_PIXEL(ti[i].x), TOUCH_COORD_TO_PIXEL(ti[i].y) };
			ScreenToClient(hWnd, &pt);
			LPARAM lp = MAKELPARAM(pt.x, pt.y);

			if (ti[i].dwFlags & TOUCHEVENTF_DOWN) {
				if (!state->touchTracking) {
					state->touchTracking = true;
					state->touchId = ti[i].dwID;
					SetCapture(hWnd);
					SendMessageW(hWnd, WM_LBUTTONDOWN, MK_LBUTTON, lp);
				}
				continue;
			}

			if (!state->touchTracking || ti[i].dwID != state->touchId) continue;

			if (ti[i].dwFlags & TOUCHEVENTF_MOVE) {
				SendMessageW(hWnd, WM_MOUSEMOVE, MK_LBUTTON, lp);
				continue;
			}

			if (ti[i].dwFlags & TOUCHEVENTF_UP) {
				SendMessageW(hWnd, WM_LBUTTONUP, 0, lp);
				state->touchTracking = false;
				state->touchId = 0;
				if (GetCapture() == hWnd) ReleaseCapture();
				continue;
			}
		}

		CloseTouchInputHandle((HTOUCHINPUT)lParam);
		return 0;
	}

	case WM_ENABLE:
		state->touchTracking = false;
		state->touchId = 0;
		if (GetCapture() == hWnd) ReleaseCapture();
		InvalidateRect(hWnd, 0, false);
		break;

	case WM_CANCELMODE:
	case WM_KILLFOCUS:
		state->touchTracking = false;
		state->touchId = 0;
		if (GetCapture() == hWnd) ReleaseCapture();
		break;

	case WM_NCDESTROY:
		delete state;
		RemoveWindowSubclass(hWnd, DarkUpDownSubclassProc, uIdSubclass);
		break;
	}

	return DefSubclassProc(hWnd, msg, wParam, lParam);
}
struct DarkCheckBoxState {
	int checkState = BST_UNCHECKED; // 0=unchecked,1=checked,2=indeterminate
	bool threeState = false; // true for BS_3STATE / BS_AUTO3STATE
	bool mouseHot = false;
	bool mouseDown = false;
	bool touchTracking = false;
	bool touchArmed = false;
	bool touchInsideNow = false;
	bool touchPressed = false;
	DWORD touchId = 0;
};
static LRESULT CALLBACK DarkCheckBoxSubclassProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
	auto* state = (DarkCheckBoxState*)dwRefData;
	if (!state && msg != WM_NCDESTROY) {
		return DefSubclassProc(hWnd, msg, wParam, lParam);
	}

	auto ToggleCheck = [hWnd, state]() {
		if (state->threeState) {
			state->checkState = (state->checkState + 1) % 3; // cycle 0->1->2->0
		}
		else {
			state->checkState = (state->checkState == BST_CHECKED) ? BST_UNCHECKED : BST_CHECKED;
		}
		InvalidateRect(hWnd, 0, false);
		HWND target = GetParent(hWnd);
		if (!target) target = GetAncestor(hWnd, GA_ROOT);
		if (!target) return;
		int cid = GetDlgCtrlID(hWnd);
		SendMessageW(target, WM_COMMAND, MAKEWPARAM(cid, BN_CLICKED), (LPARAM)hWnd);
		};

	switch (msg) {
	case BM_GETCHECK:
		return state->checkState;

	case BM_SETCHECK: {
		int newChecked = (int)wParam;
		if (state->checkState != newChecked) {
			state->checkState = newChecked;
			InvalidateRect(hWnd, 0, false);
		}
		return 0;
	}

	case WM_ERASEBKGND:
		return 1;

	case WM_PAINT: {
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);
		RECT rc;
		GetClientRect(hWnd, &rc);

		HDC memDC = CreateCompatibleDC(hdc);
		HBITMAP memBmp = CreateCompatibleBitmap(hdc, rc.right - rc.left, rc.bottom - rc.top);
		HBITMAP oldBmp = (HBITMAP)SelectObject(memDC, memBmp);

		// 【修复】直接使用深色背景填充整个控件区域
		HBRUSH bgBrush = CreateSolidBrush(DarkMode::CLR_BG_CTRL);
		FillRect(memDC, &rc, bgBrush);
		DeleteObject(bgBrush);

		bool isDown = state->mouseDown || state->touchPressed;
		bool isHot = state->mouseHot;
		bool isDis = !IsWindowEnabled(hWnd);

		// 勾选框尺寸 (20x20)
		int boxSize = 16;
		int boxX = rc.left + 2;
		int boxY = rc.top + (rc.bottom - rc.top - boxSize) / 2;
		RECT boxRc{ boxX, boxY, boxX + boxSize, boxY + boxSize };

		// 触控/悬停效果
		COLORREF boxBg = isDis ? DarkMode::CLR_BG_DISABLED :
			(state->checkState == BST_CHECKED ? (isDown ? DarkMode::CLR_ACCENT_HOT : DarkMode::CLR_ACCENT) :
				(state->checkState == BST_INDETERMINATE ? (isDown ? DarkMode::CLR_ACCENT_HOT : DarkMode::CLR_ACCENT_HOT) :
					(isDown ? DarkMode::CLR_BG_CTRL_DOWN : isHot ? DarkMode::CLR_BG_CTRL_HOT : DarkMode::CLR_BG_CTRL)));

		HBRUSH br = CreateSolidBrush(boxBg);
		FillRect(memDC, &boxRc, br);
		DeleteObject(br);

		HPEN pen = CreatePen(PS_SOLID, 1, isDown ? DarkMode::CLR_BORDER_FOCUS : (isHot || state->checkState != BST_UNCHECKED) ? DarkMode::CLR_BORDER_FOCUS : DarkMode::CLR_BORDER);
		auto op = SelectObject(memDC, pen);
		auto ob = SelectObject(memDC, GetStockObject(NULL_BRUSH));
		Rectangle(memDC, boxRc.left, boxRc.top, boxRc.right, boxRc.bottom);
		SelectObject(memDC, ob);
		SelectObject(memDC, op);
		DeleteObject(pen);

		// 绘制勾选对号或半勾（减号表示不确定）
		if (state->checkState == BST_CHECKED) {
			HPEN checkPen = CreatePen(PS_SOLID, 2, RGB(255, 255, 255));
			auto opc = SelectObject(memDC, checkPen);

			// 根据 boxSize 计算对号坐标
			int padding = boxSize / 4;           // 4
			int centerX = boxX + boxSize / 2;
			int centerY = boxY + boxSize / 2;

			MoveToEx(memDC, boxX + padding, centerY + padding / 2, 0);
			LineTo(memDC, centerX, boxY + boxSize - padding);
			LineTo(memDC, boxX + boxSize - padding, boxY + padding);

			SelectObject(memDC, opc);
			DeleteObject(checkPen);
		}
		else if (state->checkState == BST_INDETERMINATE) {
			// 绘制中间一条减号
			COLORREF minusColor = RGB(255, 255, 255);
			int padding = boxSize / 4;
			int lineHeight = max(2, boxSize / 8);
			int y = boxY + boxSize / 2;
			RECT minusRect{ boxX + padding, y - lineHeight / 2, boxX + boxSize - padding, y + lineHeight / 2 };
			HBRUSH minusBrush = CreateSolidBrush(minusColor);
			FillRect(memDC, &minusRect, minusBrush);
			DeleteObject(minusBrush);
		}

		// 绘制文本
		wchar_t txt[256]{};
		GetWindowTextW(hWnd, txt, 255);
		RECT textRc = rc;
		textRc.left = boxX + boxSize + 4;

		SetBkMode(memDC, TRANSPARENT);
		SetTextColor(memDC, isDis ? DarkMode::CLR_TEXT_DIM : DarkMode::CLR_TEXT);

		HFONT hCtrlFont = (HFONT)SendMessageW(hWnd, WM_GETFONT, 0, 0);
		if (!hCtrlFont) hCtrlFont = DarkMode::hFont;
		HFONT oldFont = hCtrlFont ? (HFONT)SelectObject(memDC, hCtrlFont) : 0;
		DrawTextW(memDC, txt, -1, &textRc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
		if (oldFont) SelectObject(memDC, oldFont);

		BitBlt(hdc, 0, 0, rc.right - rc.left, rc.bottom - rc.top, memDC, 0, 0, SRCCOPY);
		SelectObject(memDC, oldBmp);
		DeleteObject(memBmp);
		DeleteDC(memDC);

		EndPaint(hWnd, &ps);
		return 0;
	}

	case WM_MOUSEMOVE: {
		if (!state->mouseHot) {
			state->mouseHot = true;
			TRACKMOUSEEVENT tme{ sizeof(tme), TME_LEAVE, hWnd, 0 };
			TrackMouseEvent(&tme);
			InvalidateRect(hWnd, 0, false);
		}
		break;
	}

	case WM_MOUSELEAVE: {
		if (state->mouseHot || state->mouseDown) {
			state->mouseHot = false;
			state->mouseDown = false;
			InvalidateRect(hWnd, 0, false);
		}
		break;
	}

	case WM_LBUTTONDOWN:
	case WM_LBUTTONDBLCLK: {
		SetCapture(hWnd);
		state->mouseDown = true;
		InvalidateRect(hWnd, 0, false);
		break;
	}

	case WM_LBUTTONUP: {
		if (GetCapture() == hWnd) ReleaseCapture();
		bool wasDown = state->mouseDown;
		state->mouseDown = false;
		InvalidateRect(hWnd, 0, false);
		if (wasDown) {
			POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
			RECT rc; GetClientRect(hWnd, &rc);
			if (PtInRect(&rc, pt)) {
				ToggleCheck();
			}
		}
		break;
	}

	case WM_ENABLE: {
		state->mouseDown = false;
		state->touchTracking = false;
		state->touchArmed = false;
		state->touchInsideNow = false;
		state->touchPressed = false;
		state->touchId = 0;
		if (GetCapture() == hWnd) ReleaseCapture();
		InvalidateRect(hWnd, 0, false);
		break;
	}

	case WM_TOUCH: {
		UINT count = LOWORD(wParam);
		TOUCHINPUT ti[16];
		if (count > 16) count = 16;
		if (!GetTouchInputInfo((HTOUCHINPUT)lParam, count, ti, sizeof(TOUCHINPUT))) {
			CloseTouchInputHandle((HTOUCHINPUT)lParam);
			return 0;
		}

		for (UINT i = 0; i < count; ++i) {
			POINT pt{ TOUCH_COORD_TO_PIXEL(ti[i].x), TOUCH_COORD_TO_PIXEL(ti[i].y) };
			ScreenToClient(hWnd, &pt);
			RECT rc; GetClientRect(hWnd, &rc);
			const bool inside = PtInRect(&rc, pt) != 0;
			const DWORD id = ti[i].dwID;

			if (ti[i].dwFlags & TOUCHEVENTF_DOWN) {
				if (!state->touchTracking) {
					state->touchTracking = true;
					state->touchId = id;
					state->touchArmed = true;
					state->touchInsideNow = inside;
					state->touchPressed = inside;
					SetCapture(hWnd);
					InvalidateRect(hWnd, 0, false);
				}
				continue;
			}

			if (!state->touchTracking || id != state->touchId) continue;

			if (ti[i].dwFlags & TOUCHEVENTF_MOVE) {
				bool newInside = inside;
				bool newPressed = state->touchArmed && newInside;
				if (state->touchInsideNow != newInside || state->touchPressed != newPressed) {
					state->touchInsideNow = newInside;
					state->touchPressed = newPressed;
					InvalidateRect(hWnd, 0, false);
				}
				continue;
			}

			if (ti[i].dwFlags & TOUCHEVENTF_UP) {
				state->touchInsideNow = inside;
				bool shouldToggle = IsWindowEnabled(hWnd) && state->touchArmed && inside;

				if (GetCapture() == hWnd) ReleaseCapture();

				state->touchTracking = false;
				state->touchArmed = false;
				state->touchInsideNow = false;
				state->touchPressed = false;
				state->touchId = 0;

				InvalidateRect(hWnd, 0, false);

				if (shouldToggle) {
					ToggleCheck();
				}
				continue;
			}
		}
		CloseTouchInputHandle((HTOUCHINPUT)lParam);
		return 0;
	}

	case WM_CANCELMODE:
	case WM_KILLFOCUS: {
		if (state->touchTracking || state->touchPressed || state->mouseDown) {
			state->touchTracking = false;
			state->touchArmed = false;
			state->touchInsideNow = false;
			state->touchPressed = false;
			state->touchId = 0;
			state->mouseDown = false;
			state->mouseHot = false;
			if (GetCapture() == hWnd) ReleaseCapture();
			InvalidateRect(hWnd, 0, false);
		}
		break;
	}

	case WM_NCDESTROY:
		delete state;
		RemoveWindowSubclass(hWnd, DarkCheckBoxSubclassProc, uIdSubclass);
		break;
	}

	return DefSubclassProc(hWnd, msg, wParam, lParam);
}
struct DarkRadioState {
	bool checked = false;
	bool mouseHot = false;
	bool mouseDown = false;
	bool touchTracking = false;
	bool touchArmed = false;
	bool touchInsideNow = false;
	bool touchPressed = false;
	DWORD touchId = 0;
};
static LRESULT CALLBACK DarkRadioSubclassProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
void ToggleRadioButton(HWND hWnd, DarkRadioState* state, bool notify = true) {
	if (!hWnd || !state) return;

	HWND hParent = GetParent(hWnd);
	if (hParent) {
		HWND groupStart = hWnd;
		HWND prev = GetWindow(groupStart, GW_HWNDPREV);
		while (prev) {
			groupStart = prev;
			if (GetWindowLongPtrW(prev, GWL_STYLE) & WS_GROUP) {
				break;
			}
			prev = GetWindow(prev, GW_HWNDPREV);
		}

		for (HWND hSibling = groupStart; hSibling; hSibling = GetWindow(hSibling, GW_HWNDNEXT)) {
			if (hSibling != groupStart && (GetWindowLongPtrW(hSibling, GWL_STYLE) & WS_GROUP)) {
				break;
			}
			if (hSibling == hWnd || GetParent(hSibling) != hParent) continue;

			WCHAR className[256]{};
			GetClassNameW(hSibling, className, 256);
			if (_wcsicmp(className, L"Button") != 0) continue;

			LONG_PTR style = GetWindowLongPtrW(hSibling, GWL_STYLE);
			if ((style & BS_TYPEMASK) == BS_RADIOBUTTON || (style & BS_TYPEMASK) == BS_AUTORADIOBUTTON) {
				SendMessageW(hSibling, BM_SETCHECK, BST_UNCHECKED, 0);
				DWORD_PTR refData = 0;
				if (GetWindowSubclass(hSibling, DarkRadioSubclassProc, 0, &refData)) {
					auto* siblingState = (DarkRadioState*)refData;
					if (siblingState && siblingState->checked) {
						siblingState->checked = false;
						InvalidateRect(hSibling, 0, false);
					}
				}
			}
		}
	}

	if (!state->checked) {
		state->checked = true;
		InvalidateRect(hWnd, 0, false);
	}

	if (!notify) return;
	HWND target = GetParent(hWnd);
	if (!target) target = GetAncestor(hWnd, GA_ROOT);
	if (!target) return;
	int cid = GetDlgCtrlID(hWnd);
	PostMessageW(target, WM_COMMAND, MAKEWPARAM(cid, BN_CLICKED), (LPARAM)hWnd);
}
static LRESULT CALLBACK DarkRadioSubclassProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
	auto* state = (DarkRadioState*)dwRefData;
	if (!state && msg != WM_NCDESTROY) {
		return DefSubclassProc(hWnd, msg, wParam, lParam);
	}


	switch (msg) {
	case BM_GETCHECK:
		return state->checked ? BST_CHECKED : BST_UNCHECKED;

	case BM_SETCHECK: {
		bool newChecked = (wParam == BST_CHECKED);
		if (newChecked) {
			ToggleRadioButton(hWnd, state, false);
		}
		else if (state->checked) {
			state->checked = false;
			InvalidateRect(hWnd, 0, false);
		}
		return 0;
	}

	case WM_ERASEBKGND:
		return 1;

	case WM_PAINT: {
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);
		RECT rc;
		GetClientRect(hWnd, &rc);

		HDC memDC = CreateCompatibleDC(hdc);
		HBITMAP memBmp = CreateCompatibleBitmap(hdc, rc.right - rc.left, rc.bottom - rc.top);
		HBITMAP oldBmp = (HBITMAP)SelectObject(memDC, memBmp);

		HBRUSH bgBrush = CreateSolidBrush(DarkMode::CLR_BG_CTRL);
		FillRect(memDC, &rc, bgBrush);
		DeleteObject(bgBrush);

		bool isDown = state->mouseDown || state->touchPressed;
		bool isHot = state->mouseHot;
		bool isDis = !IsWindowEnabled(hWnd);

		// 圆形单选按钮尺寸 (20x20)
		int radioSize = 20;
		int radioX = rc.left + 2;
		int radioY = rc.top + (rc.bottom - rc.top - radioSize) / 2;
		int centerX = radioX + radioSize / 2;
		int centerY = radioY + radioSize / 2;
		int radius = radioSize / 2 - 1;

		COLORREF ringFill = isDis ? DarkMode::CLR_BG_DISABLED :
			(isDown ? DarkMode::CLR_BG_CTRL_DOWN : isHot ? DarkMode::CLR_BG_CTRL_HOT : DarkMode::CLR_BG_CTRL);
		COLORREF ringBorder = isDis ? DarkMode::CLR_BORDER :
			(state->checked ? DarkMode::CLR_ACCENT : (isDown || isHot) ? DarkMode::CLR_BORDER_FOCUS : DarkMode::CLR_BORDER);

		HBRUSH br = CreateSolidBrush(ringFill);
		HPEN pen = CreatePen(PS_SOLID, 1, ringBorder);
		auto oldPen = SelectObject(memDC, pen);
		auto oldBrush = SelectObject(memDC, br);
		Ellipse(memDC, radioX, radioY, radioX + radioSize, radioY + radioSize);
		SelectObject(memDC, oldBrush);
		SelectObject(memDC, oldPen);
		DeleteObject(br);
		DeleteObject(pen);

		if (state->checked) {
			int dotRadius = 5;
			COLORREF dotColor = isDis ? DarkMode::CLR_TEXT_DIM : DarkMode::CLR_ACCENT;
			HBRUSH dotBrush = CreateSolidBrush(dotColor);
			auto oldDotBrush = SelectObject(memDC, dotBrush);
			HPEN dotPen = CreatePen(PS_SOLID, 1, dotColor);
			auto oldDotPen = SelectObject(memDC, dotPen);
			Ellipse(memDC, centerX - dotRadius, centerY - dotRadius,
				centerX + dotRadius, centerY + dotRadius);
			SelectObject(memDC, oldDotPen);
			SelectObject(memDC, oldDotBrush);
			DeleteObject(dotPen);
			DeleteObject(dotBrush);
		}

		// 绘制文本
		wchar_t txt[256]{};
		GetWindowTextW(hWnd, txt, 255);
		RECT textRc = rc;
		textRc.left = radioX + radioSize + 8;

		SetBkMode(memDC, TRANSPARENT);
		SetTextColor(memDC, isDis ? DarkMode::CLR_TEXT_DIM : DarkMode::CLR_TEXT);

		HFONT hCtrlFont = (HFONT)SendMessageW(hWnd, WM_GETFONT, 0, 0);
		if (!hCtrlFont) hCtrlFont = DarkMode::hFont;
		HFONT oldFont = hCtrlFont ? (HFONT)SelectObject(memDC, hCtrlFont) : 0;
		DrawTextW(memDC, txt, -1, &textRc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
		if (oldFont) SelectObject(memDC, oldFont);

		BitBlt(hdc, 0, 0, rc.right - rc.left, rc.bottom - rc.top, memDC, 0, 0, SRCCOPY);
		SelectObject(memDC, oldBmp);
		DeleteObject(memBmp);
		DeleteDC(memDC);

		EndPaint(hWnd, &ps);
		return 0;
	}

	case WM_MOUSEMOVE: {
		if (!state->mouseHot) {
			state->mouseHot = true;
			TRACKMOUSEEVENT tme{ sizeof(tme), TME_LEAVE, hWnd, 0 };
			TrackMouseEvent(&tme);
			InvalidateRect(hWnd, 0, false);
		}
		break;
	}

	case WM_MOUSELEAVE: {
		if (state->mouseHot || state->mouseDown) {
			state->mouseHot = false;
			state->mouseDown = false;
			InvalidateRect(hWnd, 0, false);
		}
		break;
	}

	case WM_LBUTTONDOWN:
	case WM_LBUTTONDBLCLK: {
		SetCapture(hWnd);
		state->mouseDown = true;
		InvalidateRect(hWnd, 0, false);
		break;
	}

	case WM_LBUTTONUP: {
		if (GetCapture() == hWnd) ReleaseCapture();
		bool wasDown = state->mouseDown;
		state->mouseDown = false;
		InvalidateRect(hWnd, 0, false);
		if (wasDown) {
			POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
			RECT rc; GetClientRect(hWnd, &rc);
			if (PtInRect(&rc, pt)) {
				ToggleRadioButton(hWnd, state);
			}
		}
		break;
	}

	case WM_ENABLE: {
		state->mouseDown = false;
		state->touchTracking = false;
		state->touchArmed = false;
		state->touchInsideNow = false;
		state->touchPressed = false;
		state->touchId = 0;
		if (GetCapture() == hWnd) ReleaseCapture();
		InvalidateRect(hWnd, 0, false);
		break;
	}

	case WM_TOUCH: {
		UINT count = LOWORD(wParam);
		TOUCHINPUT ti[16];
		if (count > 16) count = 16;
		if (!GetTouchInputInfo((HTOUCHINPUT)lParam, count, ti, sizeof(TOUCHINPUT))) {
			CloseTouchInputHandle((HTOUCHINPUT)lParam);
			return 0;
		}

		for (UINT i = 0; i < count; ++i) {
			POINT pt{ TOUCH_COORD_TO_PIXEL(ti[i].x), TOUCH_COORD_TO_PIXEL(ti[i].y) };
			ScreenToClient(hWnd, &pt);
			RECT rc; GetClientRect(hWnd, &rc);
			const bool inside = PtInRect(&rc, pt) != 0;
			const DWORD id = ti[i].dwID;

			if (ti[i].dwFlags & TOUCHEVENTF_DOWN) {
				if (!state->touchTracking) {
					state->touchTracking = true;
					state->touchId = id;
					state->touchArmed = true;
					state->touchInsideNow = inside;
					state->touchPressed = inside;
					SetCapture(hWnd);
					InvalidateRect(hWnd, 0, false);
				}
				continue;
			}

			if (!state->touchTracking || id != state->touchId) continue;

			if (ti[i].dwFlags & TOUCHEVENTF_MOVE) {
				bool newInside = inside;
				bool newPressed = state->touchArmed && newInside;
				if (state->touchInsideNow != newInside || state->touchPressed != newPressed) {
					state->touchInsideNow = newInside;
					state->touchPressed = newPressed;
					InvalidateRect(hWnd, 0, false);
				}
				continue;
			}

			if (ti[i].dwFlags & TOUCHEVENTF_UP) {
				state->touchInsideNow = inside;
				bool shouldToggle = IsWindowEnabled(hWnd) && state->touchArmed && inside;

				if (GetCapture() == hWnd) ReleaseCapture();

				state->touchTracking = false;
				state->touchArmed = false;
				state->touchInsideNow = false;
				state->touchPressed = false;
				state->touchId = 0;

				InvalidateRect(hWnd, 0, false);

				if (shouldToggle) {
					ToggleRadioButton(hWnd, state);
				}
				continue;
			}
		}
		CloseTouchInputHandle((HTOUCHINPUT)lParam);
		return 0;
	}

	case WM_CANCELMODE:
	case WM_KILLFOCUS: {
		if (state->touchTracking || state->touchPressed || state->mouseDown) {
			state->touchTracking = false;
			state->touchArmed = false;
			state->touchInsideNow = false;
			state->touchPressed = false;
			state->touchId = 0;
			state->mouseDown = false;
			state->mouseHot = false;
			if (GetCapture() == hWnd) ReleaseCapture();
			InvalidateRect(hWnd, 0, false);
		}
		break;
	}

	case WM_NCDESTROY:
		delete state;
		RemoveWindowSubclass(hWnd, DarkRadioSubclassProc, uIdSubclass);
		break;
	}

	return DefSubclassProc(hWnd, msg, wParam, lParam);
}
static LRESULT CALLBACK DarkGroupBoxSubclassProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR) {
	switch (msg) {
	case WM_SETTEXT:
	case WM_SETFONT:
	case WM_ENABLE:
		InvalidateRect(hWnd, 0, false);
		break;



	case WM_PAINT: {
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);
		RECT rc{};
		GetClientRect(hWnd, &rc);

		// 只在非控件区域绘制额外内容

		wchar_t txt[256]{};
		GetWindowTextW(hWnd, txt, 255);
		HFONT hCtrlFont = (HFONT)SendMessageW(hWnd, WM_GETFONT, 0, 0);
		if (!hCtrlFont) hCtrlFont = DarkMode::hFont;
		HFONT oldFont = hCtrlFont ? (HFONT)SelectObject(hdc, hCtrlFont) : 0;

		SetBkMode(hdc, TRANSPARENT);
		SetTextColor(hdc, DarkMode::CLR_TEXT);

		SIZE sz{};
		GetTextExtentPoint32W(hdc, txt, lstrlenW(txt), &sz);
		int textLeft = 10;
		int textTop = 0;
		int topY = max(1, sz.cy / 2);

		HPEN pen = CreatePen(PS_SOLID, 1, DarkMode::CLR_BORDER);
		auto oldPen = SelectObject(hdc, pen);
		MoveToEx(hdc, 1, topY, 0);
		LineTo(hdc, textLeft - 4, topY);
		MoveToEx(hdc, textLeft + sz.cx + 4, topY, 0);
		LineTo(hdc, rc.right - 2, topY);
		MoveToEx(hdc, 1, topY, 0);
		LineTo(hdc, 1, rc.bottom - 2);
		LineTo(hdc, rc.right - 2, rc.bottom - 2);
		LineTo(hdc, rc.right - 2, topY);
		SelectObject(hdc, oldPen);
		DeleteObject(pen);

		RECT textRc{ textLeft, textTop, textLeft + sz.cx + 4, textTop + sz.cy + 2 };
		DrawTextW(hdc, txt, -1, &textRc, DT_LEFT | DT_SINGLELINE | DT_VCENTER);
		if (oldFont) SelectObject(hdc, oldFont);


		EndPaint(hWnd, &ps);
		break;
	}

	case WM_NCDESTROY:
		RemoveWindowSubclass(hWnd, DarkGroupBoxSubclassProc, uIdSubclass);
		break;
	}
	return DefSubclassProc(hWnd, msg, wParam, lParam);
}
struct DarkTabState {
	bool mouseTracking = false;
	int hotIndex = -1;
	bool touchTracking = false;
	DWORD touchId = 0;
	int touchDownIndex = -1;
};
static void PreloadDarkTabRendererOnce(HWND hRefWnd) {
	if (!DarkMode::EnableTabPreload || DarkMode::TabPreloaded) return;
	DarkMode::TabPreloaded = true;

	INITCOMMONCONTROLSEX icc{ sizeof(icc), ICC_TAB_CLASSES };
	InitCommonControlsEx(&icc);

	HINSTANCE hInst = (HINSTANCE)GetWindowLongPtrW(hRefWnd ? hRefWnd : GetDesktopWindow(), GWLP_HINSTANCE);
	if (!hInst) hInst = GetModuleHandleW(nullptr);

	HWND hHost = CreateWindowExW(WS_EX_TOOLWINDOW, L"STATIC", L"",
		WS_POPUP, -32000, -32000, 220, 90,
		nullptr, nullptr, hInst, nullptr);
	if (!hHost) return;

	HWND hTab = CreateWindowExW(0, WC_TABCONTROLW, L"",
		WS_CHILD | WS_VISIBLE | TCS_TABS,
		0, 0, 220, 90,
		hHost, nullptr, hInst, nullptr);
	if (hTab) {
		TCITEMW item{};
		item.mask = TCIF_TEXT;
		item.pszText = const_cast<LPWSTR>(L"Preload");
		TabCtrl_InsertItem(hTab, 0, &item);
		SetWindowTheme(hTab, L"", L"");
		RedrawWindow(hTab, 0, 0, RDW_INVALIDATE | RDW_NOERASE | RDW_UPDATENOW);
	}

	DestroyWindow(hHost);
}
static LRESULT CALLBACK DarkTabSubclassProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
	auto* state = reinterpret_cast<DarkTabState*>(dwRefData);
	if (!state && msg != WM_NCDESTROY) return DefSubclassProc(hWnd, msg, wParam, lParam);

	switch (msg) {
	case WM_ERASEBKGND: {
		HDC hdc = (HDC)wParam;
		RECT rc{}; GetClientRect(hWnd, &rc);
		FillRect(hdc, &rc, DarkMode::brTabHost);

		RECT page = rc;
		TabCtrl_AdjustRect(hWnd, false, &page);
		FillRect(hdc, &page, DarkMode::brTabPage);

		HPEN p = CreatePen(PS_SOLID, 1, DarkMode::CLR_BORDER);
		auto op = SelectObject(hdc, p);
		auto ob = SelectObject(hdc, GetStockObject(NULL_BRUSH));
		Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);
		SelectObject(hdc, ob);
		SelectObject(hdc, op);
		DeleteObject(p);
		return 1;
	}

	case WM_PAINT: {
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);
		RECT rc{};
		GetClientRect(hWnd, &rc);

		HDC memDC = CreateCompatibleDC(hdc);
		HBITMAP memBmp = CreateCompatibleBitmap(hdc, rc.right - rc.left, rc.bottom - rc.top);
		HBITMAP oldBmp = (HBITMAP)SelectObject(memDC, memBmp);

		FillRect(memDC, &rc, DarkMode::brTabHost);
		RECT page = rc;
		TabCtrl_AdjustRect(hWnd, false, &page);
		FillRect(memDC, &page, DarkMode::brTabPage);

		HPEN borderPen = CreatePen(PS_SOLID, 1, DarkMode::CLR_BORDER);
		auto borderOldPen = SelectObject(memDC, borderPen);
		auto borderOldBrush = SelectObject(memDC, GetStockObject(NULL_BRUSH));
		Rectangle(memDC, rc.left, rc.top, rc.right, rc.bottom);
		SelectObject(memDC, borderOldBrush);
		SelectObject(memDC, borderOldPen);
		DeleteObject(borderPen);

		int count = TabCtrl_GetItemCount(hWnd);
		int curSel = TabCtrl_GetCurSel(hWnd);
		for (int i = 0; i < count; ++i) {
			RECT itemRc{};
			TabCtrl_GetItemRect(hWnd, i, &itemRc);

			bool sel = (curSel == i);
			bool tdn = (state->touchTracking && state->touchDownIndex == i);
			bool hov = (!sel && i == state->hotIndex);

			COLORREF bg = tdn ? DarkMode::CLR_BG_TAB_TOUCH :
				sel ? DarkMode::CLR_BG_TAB_SEL :
				hov ? DarkMode::CLR_BG_TAB_HOVER :
				DarkMode::CLR_BG_TAB;

			HBRUSH br = CreateSolidBrush(bg);
			FillRect(memDC, &itemRc, br);
			DeleteObject(br);

			HPEN pen = CreatePen(PS_SOLID, 1, DarkMode::CLR_BORDER);
			auto op = SelectObject(memDC, pen);
			auto ob = SelectObject(memDC, GetStockObject(NULL_BRUSH));
			Rectangle(memDC, itemRc.left, itemRc.top, itemRc.right, itemRc.bottom);
			SelectObject(memDC, ob);
			SelectObject(memDC, op);
			DeleteObject(pen);

			wchar_t text[128]{};
			TCITEMW it{};
			it.mask = TCIF_TEXT;
			it.pszText = text;
			it.cchTextMax = 127;
			TabCtrl_GetItem(hWnd, i, &it);

			SetBkMode(memDC, TRANSPARENT);
			SetTextColor(memDC, DarkMode::isDarkThemeEnabled ? DarkMode::CLR_TEXT : RGB(28, 28, 28));
			HFONT hCtrlFont = (HFONT)SendMessageW(hWnd, WM_GETFONT, 0, 0);
			if (!hCtrlFont) hCtrlFont = DarkMode::hFont;
			auto of = hCtrlFont ? SelectObject(memDC, hCtrlFont) : 0;
			DrawTextW(memDC, text, -1, &itemRc, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
			if (of) SelectObject(memDC, of);
		}

		BitBlt(hdc, 0, 0, rc.right - rc.left, rc.bottom - rc.top, memDC, 0, 0, SRCCOPY);
		SelectObject(memDC, oldBmp);
		DeleteObject(memBmp);
		DeleteDC(memDC);
		EndPaint(hWnd, &ps);
		return 0;
	}

	case WM_LBUTTONDOWN:
	case WM_LBUTTONDBLCLK: {
		POINT p{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
		TCHITTESTINFO hitInfo{ p, 0 };
		int hit = TabCtrl_HitTest(hWnd, &hitInfo);
		if (hit >= 0 && hit != TabCtrl_GetCurSel(hWnd)) {
			TabCtrl_SetCurSel(hWnd, hit);
			RedrawWindow(hWnd, 0, 0, RDW_INVALIDATE | RDW_NOERASE | RDW_UPDATENOW);
			NMHDR nm{ hWnd, (UINT_PTR)GetDlgCtrlID(hWnd), TCN_SELCHANGE };
			HWND hParent = GetParent(hWnd);
			if (hParent) {
				SendMessageW(hParent, WM_NOTIFY, (WPARAM)nm.idFrom, (LPARAM)&nm);
				RECT page{};
				GetClientRect(hWnd, &page);
				TabCtrl_AdjustRect(hWnd, false, &page);
				MapWindowPoints(hWnd, hParent, (LPPOINT)&page, 2);
				RedrawWindow(hParent, &page, 0, RDW_INVALIDATE | RDW_NOERASE);
			}
		}
		break;
	}

	case WM_MOUSEMOVE: {
		POINT p{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
		TCHITTESTINFO hitInfo{ p, 0 };
		int hit = TabCtrl_HitTest(hWnd, &hitInfo);
		int cur = TabCtrl_GetCurSel(hWnd);
		int nh = (hit >= 0 && hit != cur) ? hit : -1;
		if (nh != state->hotIndex) {
			state->hotIndex = nh;
			InvalidateRect(hWnd, 0, false);
		}
		if (!state->mouseTracking) {
			TRACKMOUSEEVENT t{ sizeof(t), TME_LEAVE, hWnd, 0 };
			if (TrackMouseEvent(&t)) state->mouseTracking = true;
		}
		break;
	}

	case WM_MOUSELEAVE: {
		state->mouseTracking = false;
		if (state->hotIndex != -1) {
			state->hotIndex = -1;
			InvalidateRect(hWnd, 0, false);
		}
		break;
	}

	case WM_ENABLE:
		state->mouseTracking = false;
		state->hotIndex = -1;
		state->touchTracking = false;
		state->touchId = 0;
		state->touchDownIndex = -1;
		InvalidateRect(hWnd, 0, false);
		break;

	case WM_TOUCH: {
		UINT n = LOWORD(wParam);
		TOUCHINPUT ti[16];
		if (n > 16) n = 16;
		if (!GetTouchInputInfo((HTOUCHINPUT)lParam, n, ti, sizeof(TOUCHINPUT))) {
			CloseTouchInputHandle((HTOUCHINPUT)lParam);
			return 0;
		}

		for (UINT i = 0; i < n; ++i) {
			POINT p{ TOUCH_COORD_TO_PIXEL(ti[i].x), TOUCH_COORD_TO_PIXEL(ti[i].y) };
			ScreenToClient(hWnd, &p);
			TCHITTESTINFO hti{ p, 0 };
			int tabIdx = TabCtrl_HitTest(hWnd, &hti);

			if (ti[i].dwFlags & TOUCHEVENTF_DOWN) {
				if (tabIdx >= 0 && !state->touchTracking) {
					state->touchTracking = true;
					state->touchId = ti[i].dwID;
					state->touchDownIndex = tabIdx;
					InvalidateRect(hWnd, 0, false);
				}
			}
			if (ti[i].dwFlags & TOUCHEVENTF_MOVE) {
				if (state->touchTracking && ti[i].dwID == state->touchId) {
					if (tabIdx != state->touchDownIndex) {
						state->touchDownIndex = tabIdx;
						InvalidateRect(hWnd, 0, false);
					}
				}
			}
			if (ti[i].dwFlags & TOUCHEVENTF_UP) {
				if (state->touchTracking && ti[i].dwID == state->touchId) {
					if (tabIdx >= 0) {
						TabCtrl_SetCurSel(hWnd, tabIdx);
						RedrawWindow(hWnd, 0, 0, RDW_INVALIDATE | RDW_NOERASE | RDW_UPDATENOW);
						NMHDR nm{ hWnd, (UINT_PTR)GetDlgCtrlID(hWnd), TCN_SELCHANGE };
						HWND hParent = GetParent(hWnd);
						if (hParent) {
							SendMessageW(hParent, WM_NOTIFY, (WPARAM)nm.idFrom, (LPARAM)&nm);
							RECT page{};
							GetClientRect(hWnd, &page);
							TabCtrl_AdjustRect(hWnd, false, &page);
							MapWindowPoints(hWnd, hParent, (LPPOINT)&page, 2);
							RedrawWindow(hParent, &page, 0, RDW_INVALIDATE | RDW_NOERASE);
						}
					}
					state->touchTracking = false;
					state->touchId = 0;
					state->touchDownIndex = -1;
					InvalidateRect(hWnd, 0, false);
				}
			}
		}

		CloseTouchInputHandle((HTOUCHINPUT)lParam);
		return 0;
	}

	case WM_NCDESTROY:
		delete state;
		RemoveWindowSubclass(hWnd, DarkTabSubclassProc, uIdSubclass);
		break;
	}

	return DefSubclassProc(hWnd, msg, wParam, lParam);
}
struct DarkHeaderState {
	int hotIndex = -1;
	bool mouseTracking = false;
};
static LRESULT CALLBACK DarkHeaderSubclassProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
	auto* state = reinterpret_cast<DarkHeaderState*>(dwRefData);
	if (!state && msg != WM_NCDESTROY) return DefSubclassProc(hWnd, msg, wParam, lParam);

	switch (msg) {
	case WM_ERASEBKGND:
		return 1;

	case WM_PAINT: {
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);
		RECT rc;
		GetClientRect(hWnd, &rc);

		HDC memDC = CreateCompatibleDC(hdc);
		HBITMAP memBmp = CreateCompatibleBitmap(hdc, rc.right - rc.left, rc.bottom - rc.top);
		HBITMAP oldBmp = (HBITMAP)SelectObject(memDC, memBmp);

		HBRUSH brBg = CreateSolidBrush(DarkMode::CLR_BG_CTRL);
		FillRect(memDC, &rc, brBg);
		DeleteObject(brBg);

		int count = Header_GetItemCount(hWnd);
		for (int i = 0; i < count; ++i) {
			RECT itemRc{};
			Header_GetItemRect(hWnd, i, &itemRc);

			bool isHot = (i == state->hotIndex);
			if (isHot) {
				HBRUSH brHot = CreateSolidBrush(DarkMode::CLR_BG_CTRL_HOT);
				FillRect(memDC, &itemRc, brHot);
				DeleteObject(brHot);
			}

			HPEN pen = CreatePen(PS_SOLID, 1, DarkMode::CLR_BORDER);
			auto oldPen = SelectObject(memDC, pen);
			MoveToEx(memDC, itemRc.right - 1, itemRc.top + 4, 0);
			LineTo(memDC, itemRc.right - 1, itemRc.bottom - 4);

			MoveToEx(memDC, itemRc.left, itemRc.bottom - 1, 0);
			LineTo(memDC, itemRc.right, itemRc.bottom - 1);
			SelectObject(memDC, oldPen);
			DeleteObject(pen);

			wchar_t text[128]{};
			HDITEMW hdi{};
			hdi.mask = HDI_TEXT;
			hdi.pszText = text;
			hdi.cchTextMax = 127;
			Header_GetItem(hWnd, i, &hdi);

			RECT textRc = itemRc;
			textRc.left += 6;
			textRc.top -= 2;
			textRc.right -= 8;

			SetBkMode(memDC, TRANSPARENT);
			SetTextColor(memDC, DarkMode::CLR_TEXT);

			HFONT hCtrlFont = (HFONT)SendMessageW(hWnd, WM_GETFONT, 0, 0);
			if (!hCtrlFont) hCtrlFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
			auto oldFont = hCtrlFont ? SelectObject(memDC, hCtrlFont) : 0;
			DrawTextW(memDC, text, -1, &textRc, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
			if (oldFont) SelectObject(memDC, oldFont);
		}

		HPEN bottomPen = CreatePen(PS_SOLID, 1, DarkMode::CLR_BORDER);
		auto oldP = SelectObject(memDC, bottomPen);
		MoveToEx(memDC, rc.left, rc.bottom - 1, 0);
		LineTo(memDC, rc.right, rc.bottom - 1);
		SelectObject(memDC, oldP);
		DeleteObject(bottomPen);

		BitBlt(hdc, 0, 0, rc.right - rc.left, rc.bottom - rc.top, memDC, 0, 0, SRCCOPY);
		SelectObject(memDC, oldBmp);
		DeleteObject(memBmp);
		DeleteDC(memDC);

		EndPaint(hWnd, &ps);
		return 0;
	}

	case WM_MOUSEMOVE: {
		POINT pt{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
		HDHITTESTINFO hdHit{};
		hdHit.pt = pt;
		int hit = (int)SendMessageW(hWnd, HDM_HITTEST, 0, (LPARAM)&hdHit);
		if (hit != state->hotIndex) {
			state->hotIndex = hit;
			InvalidateRect(hWnd, 0, false);
		}
		if (!state->mouseTracking) {
			TRACKMOUSEEVENT tme{ sizeof(tme), TME_LEAVE, hWnd, 0 };
			if (TrackMouseEvent(&tme)) state->mouseTracking = true;
		}
		break;
	}

	case WM_MOUSELEAVE:
		state->mouseTracking = false;
		if (state->hotIndex != -1) {
			state->hotIndex = -1;
			InvalidateRect(hWnd, 0, false);
		}
		break;

	case WM_ENABLE:
		state->mouseTracking = false;
		state->hotIndex = -1;
		InvalidateRect(hWnd, 0, false);
		break;

	case WM_NCDESTROY:
		delete state;
		RemoveWindowSubclass(hWnd, DarkHeaderSubclassProc, uIdSubclass);
		break;
	}

	return DefSubclassProc(hWnd, msg, wParam, lParam);
}
struct DarkListState {
	bool touchTracking = false;
	DWORD touchId = 0;
	int touchDownIndex = -1;
};
static LRESULT CALLBACK DarkListViewSubclassProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
	auto* state = reinterpret_cast<DarkListState*>(dwRefData);
	if (!state && msg != WM_NCDESTROY) return DefSubclassProc(hWnd, msg, wParam, lParam);

	switch (msg) {
	case WM_TOUCH: {
		UINT n = LOWORD(wParam);
		TOUCHINPUT ti[16];
		if (n > 16) n = 16;
		if (!GetTouchInputInfo((HTOUCHINPUT)lParam, n, ti, sizeof(TOUCHINPUT))) {
			CloseTouchInputHandle((HTOUCHINPUT)lParam);
			return 0;
		}

		for (UINT i = 0; i < n; ++i) {
			POINT p{ TOUCH_COORD_TO_PIXEL(ti[i].x), TOUCH_COORD_TO_PIXEL(ti[i].y) };
			ScreenToClient(hWnd, &p);

			LVHITTESTINFO lvhti{};
			lvhti.pt = p;
			int hitIdx = ListView_HitTest(hWnd, &lvhti);

			if (ti[i].dwFlags & TOUCHEVENTF_DOWN) {
				if (!state->touchTracking && hitIdx >= 0) {
					state->touchTracking = true;
					state->touchId = ti[i].dwID;
					state->touchDownIndex = hitIdx;

					// 【核心优化】：手指按下立即进行选择并触发选择事件
					ListView_SetItemState(hWnd, -1, 0, LVIS_SELECTED | LVIS_FOCUSED);
					ListView_SetItemState(hWnd, hitIdx, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);

					// 立即触发父窗口选择通知
					HWND hParent = GetParent(hWnd);
					if (hParent) {
						NMLISTVIEW nmlv{};
						nmlv.hdr.hwndFrom = hWnd;
						nmlv.hdr.idFrom = (UINT_PTR)GetDlgCtrlID(hWnd);
						nmlv.hdr.code = LVN_ITEMCHANGED;
						nmlv.iItem = hitIdx;
						nmlv.uNewState = LVIS_SELECTED | LVIS_FOCUSED;
						SendMessageW(hParent, WM_NOTIFY, (WPARAM)nmlv.hdr.idFrom, (LPARAM)&nmlv);
					}

					InvalidateRect(hWnd, 0, false);
				}
				continue;
			}

			if (!state->touchTracking || ti[i].dwID != state->touchId) continue;

			if (ti[i].dwFlags & TOUCHEVENTF_MOVE) {
				if (hitIdx >= 0 && hitIdx != state->touchDownIndex) {
					state->touchDownIndex = hitIdx;
					ListView_SetItemState(hWnd, -1, 0, LVIS_SELECTED | LVIS_FOCUSED);
					ListView_SetItemState(hWnd, hitIdx, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);

					HWND hParent = GetParent(hWnd);
					if (hParent) {
						NMLISTVIEW nmlv{};
						nmlv.hdr.hwndFrom = hWnd;
						nmlv.hdr.idFrom = (UINT_PTR)GetDlgCtrlID(hWnd);
						nmlv.hdr.code = LVN_ITEMCHANGED;
						nmlv.iItem = hitIdx;
						nmlv.uNewState = LVIS_SELECTED | LVIS_FOCUSED;
						SendMessageW(hParent, WM_NOTIFY, (WPARAM)nmlv.hdr.idFrom, (LPARAM)&nmlv);
					}
					InvalidateRect(hWnd, 0, false);
				}
				continue;
			}

			if (ti[i].dwFlags & TOUCHEVENTF_UP) {
				state->touchTracking = false;
				state->touchId = 0;
				state->touchDownIndex = -1;
				InvalidateRect(hWnd, 0, false);
				continue;
			}
		}

		CloseTouchInputHandle((HTOUCHINPUT)lParam);
		return 0;
	}

	case WM_ENABLE:
		state->touchTracking = false;
		state->touchId = 0;
		state->touchDownIndex = -1;
		InvalidateRect(hWnd, 0, false);
		break;

	case WM_CANCELMODE:
	case WM_KILLFOCUS:
		state->touchTracking = false;
		state->touchId = 0;
		state->touchDownIndex = -1;
		InvalidateRect(hWnd, 0, false);
		break;

	case WM_NCDESTROY:
		delete state;
		RemoveWindowSubclass(hWnd, DarkListViewSubclassProc, uIdSubclass);
		break;
	}

	return DefSubclassProc(hWnd, msg, wParam, lParam);
}
struct DarkTreeState {
	bool touchTracking = false;
	DWORD touchId = 0;
	HTREEITEM touchDownItem = NULL;
	bool mouseTracking = false;
	HTREEITEM hotItem = NULL;
	// 缓存画刷
	HBRUSH hBrushBg = NULL;
};
static LRESULT CALLBACK DarkTreeSubclassProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
	auto* state = reinterpret_cast<DarkTreeState*>(dwRefData);
	if (!state && msg != WM_NCDESTROY) return DefSubclassProc(hWnd, msg, wParam, lParam);

	switch (msg) {
	case WM_ERASEBKGND: {
		HDC hdc = (HDC)wParam;
		RECT rc;
		GetClientRect(hWnd, &rc);
		if (!state->hBrushBg) {
			state->hBrushBg = CreateSolidBrush(DarkMode::CLR_BG_CTRL);
		}
		FillRect(hdc, &rc, state->hBrushBg);
		return 1;
	}

	case WM_PAINT: {
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);
		RECT rc;
		GetClientRect(hWnd, &rc);

		if (!state->hBrushBg) {
			state->hBrushBg = CreateSolidBrush(DarkMode::CLR_BG_CTRL);
		}
		FillRect(hdc, &rc, state->hBrushBg);

		DefSubclassProc(hWnd, msg, wParam, lParam);
		EndPaint(hWnd, &ps);
		return 0;
	}

	case WM_CTLCOLORSTATIC:
	case WM_CTLCOLOREDIT: {
		HDC hdc = (HDC)wParam;
		SetTextColor(hdc, DarkMode::CLR_TEXT);
		SetBkColor(hdc, DarkMode::CLR_BG_CTRL);
		SetDCBrushColor(hdc, DarkMode::CLR_BG_CTRL);
		static HBRUSH hBrush = NULL;
		if (!hBrush) {
			hBrush = CreateSolidBrush(DarkMode::CLR_BG_CTRL);
		}
		return (LRESULT)hBrush;
	}

	case TVM_SETBKCOLOR: {
		if (DarkMode::isDarkThemeEnabled) {
			TreeView_SetBkColor(hWnd, DarkMode::CLR_BG_CTRL);
			if (state->hBrushBg) {
				DeleteObject(state->hBrushBg);
				state->hBrushBg = NULL;
			}
			InvalidateRect(hWnd, NULL, TRUE);
			return (LRESULT)DarkMode::CLR_BG_CTRL;
		}
		break;
	}

	case TVM_SETTEXTCOLOR: {
		if (DarkMode::isDarkThemeEnabled) {
			TreeView_SetTextColor(hWnd, DarkMode::CLR_TEXT);
			InvalidateRect(hWnd, NULL, TRUE);
			return (LRESULT)DarkMode::CLR_TEXT;
		}
		break;
	}

	case NM_CUSTOMDRAW: {
		LPNMTVCUSTOMDRAW pcd = (LPNMTVCUSTOMDRAW)lParam;

		switch (pcd->nmcd.dwDrawStage) {
		case CDDS_PREPAINT: {
			// 请求项目绘制通知，并告诉系统我们想控制颜色
			return CDRF_NOTIFYITEMDRAW;
		}

		case CDDS_ITEMPREPAINT: {
			HTREEITEM hItem = (HTREEITEM)pcd->nmcd.dwItemSpec;
			UINT uState = TreeView_GetItemState(hWnd, hItem, TVIS_SELECTED | TVIS_DROPHILITED);

			// 【关键修复】强制设置文字颜色为白色
			pcd->clrText = DarkMode::CLR_TEXT;

			if (uState & TVIS_SELECTED) {
				pcd->clrTextBk = DarkMode::CLR_BG_TAB_SEL;
			}
			else if (uState & TVIS_DROPHILITED) {
				pcd->clrTextBk = DarkMode::CLR_ACCENT;
			}
			else if (state->hotItem == hItem) {
				pcd->clrTextBk = DarkMode::CLR_BG_CTRL_HOT;
			}
			else {
				pcd->clrTextBk = DarkMode::CLR_BG_CTRL;
			}

			// 返回 CDRF_NEWFONT 应用颜色
			return CDRF_NEWFONT;
		}

		case CDDS_SUBITEM | CDDS_ITEMPREPAINT: {
			return CDRF_DODEFAULT;
		}
		}
		break;
	}

	case WM_MOUSEMOVE: {
		POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
		TVHITTESTINFO hti = { {pt.x, pt.y}, 0 };
		HTREEITEM hItem = TreeView_HitTest(hWnd, &hti);

		if (hItem && (hti.flags & TVHT_ONITEM)) {
			if (state->hotItem != hItem) {
				if (state->hotItem) {
					InvalidateRect(hWnd, 0, false);
				}
				state->hotItem = hItem;
				InvalidateRect(hWnd, 0, false);
			}
		}
		else {
			if (state->hotItem) {
				state->hotItem = NULL;
				InvalidateRect(hWnd, 0, false);
			}
		}

		if (!state->mouseTracking) {
			TRACKMOUSEEVENT tme{ sizeof(tme), TME_LEAVE, hWnd, 0 };
			TrackMouseEvent(&tme);
			state->mouseTracking = true;
		}
		break;
	}

	case WM_MOUSELEAVE: {
		state->mouseTracking = false;
		if (state->hotItem) {
			state->hotItem = NULL;
			InvalidateRect(hWnd, 0, false);
		}
		break;
	}

	case WM_TOUCH: {
		UINT count = LOWORD(wParam);
		TOUCHINPUT ti[16];
		if (count > 16) count = 16;
		if (!GetTouchInputInfo((HTOUCHINPUT)lParam, count, ti, sizeof(TOUCHINPUT))) {
			CloseTouchInputHandle((HTOUCHINPUT)lParam);
			return 0;
		}

		for (UINT i = 0; i < count; ++i) {
			POINT pt{ TOUCH_COORD_TO_PIXEL(ti[i].x), TOUCH_COORD_TO_PIXEL(ti[i].y) };
			ScreenToClient(hWnd, &pt);
			TVHITTESTINFO hti = { {pt.x, pt.y}, 0 };
			HTREEITEM hItem = TreeView_HitTest(hWnd, &hti);

			if (ti[i].dwFlags & TOUCHEVENTF_DOWN) {
				if (!state->touchTracking && hItem) {
					state->touchTracking = true;
					state->touchId = ti[i].dwID;
					state->touchDownItem = hItem;
					TreeView_SelectItem(hWnd, hItem);
					SetFocus(hWnd);
					HWND hParent = GetParent(hWnd);
					if (hParent) {
						NMHDR nm = { hWnd, (UINT_PTR)GetDlgCtrlID(hWnd), TVN_SELCHANGED };
						SendMessageW(hParent, WM_NOTIFY, (WPARAM)nm.idFrom, (LPARAM)&nm);
					}
					InvalidateRect(hWnd, 0, false);
				}
				continue;
			}

			if (!state->touchTracking || ti[i].dwID != state->touchId) continue;

			if (ti[i].dwFlags & TOUCHEVENTF_UP) {
				state->touchTracking = false;
				state->touchDownItem = NULL;
				state->touchId = 0;
				InvalidateRect(hWnd, 0, false);
				continue;
			}
		}

		CloseTouchInputHandle((HTOUCHINPUT)lParam);
		return 0;
	}

	case WM_ENABLE: {
		state->mouseTracking = false;
		state->hotItem = NULL;
		state->touchTracking = false;
		state->touchId = 0;
		state->touchDownItem = NULL;
		InvalidateRect(hWnd, 0, false);
		break;
	}

	case WM_CANCELMODE:
	case WM_KILLFOCUS: {
		state->touchTracking = false;
		state->touchId = 0;
		state->touchDownItem = NULL;
		state->mouseTracking = false;
		state->hotItem = NULL;
		if (GetCapture() == hWnd) ReleaseCapture();
		InvalidateRect(hWnd, 0, false);
		break;
	}

	case WM_NCDESTROY: {
		if (state->hBrushBg) {
			DeleteObject(state->hBrushBg);
			state->hBrushBg = NULL;
		}
		delete state;
		RemoveWindowSubclass(hWnd, DarkTreeSubclassProc, uIdSubclass);
		break;
	}
	}

	return DefSubclassProc(hWnd, msg, wParam, lParam);
}
namespace DarkModeAPI {
	PIMAGE_THUNK_DATA FindAddressByOrdinal(void* moduleBase, PIMAGE_THUNK_DATA impName, PIMAGE_THUNK_DATA impAddr, uint16_t ordinal)
	{
		for (; impName->u1.Ordinal; ++impName, ++impAddr)
		{
			if (IMAGE_SNAP_BY_ORDINAL(impName->u1.Ordinal) && IMAGE_ORDINAL(impName->u1.Ordinal) == ordinal)
				return impAddr;
		}
		return nullptr;
	}
	template <typename T, typename T1, typename T2>
	constexpr T RVA2VA(T1 base, T2 rva)
	{
		return reinterpret_cast<T>(reinterpret_cast<ULONG_PTR>(base) + rva);
	}
	template <typename T>
	constexpr T DataDirectoryFromModuleBase(void* moduleBase, size_t entryID)
	{
		auto dosHdr = reinterpret_cast<PIMAGE_DOS_HEADER>(moduleBase);
		auto ntHdr = RVA2VA<PIMAGE_NT_HEADERS>(moduleBase, dosHdr->e_lfanew);
		auto dataDir = ntHdr->OptionalHeader.DataDirectory;
		return RVA2VA<T>(moduleBase, dataDir[entryID].VirtualAddress);
	}
	PIMAGE_THUNK_DATA FindDelayLoadThunkInModule(void* moduleBase, const char* dllName, uint16_t ordinal) {
		auto imports = DataDirectoryFromModuleBase<PIMAGE_DELAYLOAD_DESCRIPTOR>(moduleBase, IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT);
		for (; imports->DllNameRVA; ++imports)
		{
			if (_stricmp(RVA2VA<LPCSTR>(moduleBase, imports->DllNameRVA), dllName) != 0)
				continue;

			auto impName = RVA2VA<PIMAGE_THUNK_DATA>(moduleBase, imports->ImportNameTableRVA);
			auto impAddr = RVA2VA<PIMAGE_THUNK_DATA>(moduleBase, imports->ImportAddressTableRVA);
			return FindAddressByOrdinal(moduleBase, impName, impAddr, ordinal);
		}
		return nullptr;
	}
	using OpenNcThemeDataProc = HTHEME(WINAPI*)(HWND hWnd, LPCWSTR pszClassList); // ordinal 49
	OpenNcThemeDataProc OpenNcThemeData = OpenNcThemeDataProc(GetProcAddress(GetModuleHandleW(L"uxtheme.dll"), MAKEINTRESOURCEA(49)));
	void FixDarkScrollBar() {
		HMODULE hComctl = LoadLibraryExW(L"comctl32.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
		if (hComctl)
		{
			auto addr = FindDelayLoadThunkInModule(hComctl, "uxtheme.dll", 49); // OpenNcThemeData
			if (addr)
			{
				DWORD oldProtect;
				if (VirtualProtect(addr, sizeof(IMAGE_THUNK_DATA), PAGE_READWRITE, &oldProtect))
				{
					auto MyOpenThemeData = [](HWND hWnd, LPCWSTR classList) -> HTHEME {
						if (wcscmp(classList, L"ScrollBar") == 0)
						{
							const bool enableDarkScroll = DarkMode::ForceDarkInInjectedProcess || ReadAppsUseLightTheme();
							if (enableDarkScroll) {
								hWnd = nullptr;
								classList = L"Explorer::ScrollBar";
							}
						}
						return OpenNcThemeData(hWnd, classList);
						};

					addr->u1.Function = ULONG_PTR(static_cast<OpenNcThemeDataProc>(MyOpenThemeData));
					VirtualProtect(addr, sizeof(IMAGE_THUNK_DATA), oldProtect, &oldProtect);
				}
			}
		}
	}
	LRESULT CALLBACK HandleWindow(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, WNDPROC oldproc = 0) {
		LRESULT lr = 0;
		typedef union tagUAHMENUITEMMETRICS {
			struct {
				DWORD cx;
				DWORD cy;
			} rgsizeBar[2];
			struct {
				DWORD cx;
				DWORD cy;
			} rgsizePopup[4];
		} UAHMENUITEMMETRICS;
		typedef struct tagUAHMENUPOPUPMETRICS {
			DWORD rgcx[4];
			DWORD fUpdateMaxWidths : 2;
		} UAHMENUPOPUPMETRICS;
		typedef struct tagUAHMENU {
			HMENU hmenu;
			HDC hdc;
			DWORD dwFlags;
		} UAHMENU;
		typedef struct tagUAHMENUITEM {
			int iPosition;
			UAHMENUITEMMETRICS umim;
			UAHMENUPOPUPMETRICS umpm;
		} UAHMENUITEM;
		typedef struct UAHDRAWMENUITEM {
			DRAWITEMSTRUCT dis;
			UAHMENU um;
			UAHMENUITEM umi;
		} UAHDRAWMENUITEM;
		typedef struct tagUAHMEASUREMENUITEM {
			MEASUREITEMSTRUCT mis;
			UAHMENU um;
			UAHMENUITEM umi;
		} UAHMEASUREMENUITEM;
		wchar_t wndClassName[64]{};
		GetClassNameW(hwnd, wndClassName, 63);
		const bool isPopupMenuWindow = (_wcsicmp(wndClassName, L"#32768") == 0 || _wcsicmp(wndClassName, L"PopupMenu") == 0);
		const bool canUseUahMenuBar = !isPopupMenuWindow && (GetParent(hwnd) == nullptr) && (GetMenu(hwnd) != nullptr);
		if (canUseUahMenuBar && [](HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, LRESULT* lr) -> bool {
			switch (message) {
			case 0x0091: {
				UAHMENU* udm = (UAHMENU*)lParam;
				RECT rc{};
				MENUBARINFO mbi = { sizeof(mbi) };
				GetMenuBarInfo(hWnd, OBJID_MENU, 0, &mbi);
				RECT rect;
				GetWindowRect(hWnd, &rect);
				rc = mbi.rcBar;
				OffsetRect(&rc, -rect.left, -rect.top);
				FillRect(udm->hdc, &rc, DarkMode::brMain ? DarkMode::brMain : (HBRUSH)GetStockObject(BLACK_BRUSH));
				return true;
			}
			case 0x0092: {
				UAHDRAWMENUITEM* mmi = (UAHDRAWMENUITEM*)lParam;
				HBRUSH bg = CreateSolidBrush(DarkMode::CLR_BG_MAIN);
				HBRUSH hover = CreateSolidBrush(DarkMode::CLR_BG_CTRL_HOT);
				HBRUSH select = CreateSolidBrush(DarkMode::CLR_BG_CTRL_DOWN);
				HBRUSH* bgcolor = &bg;
				wchar_t buffer[256]{};
				MENUITEMINFO mii = { sizeof(mii), MIIM_STRING };
				mii.dwTypeData = buffer;
				mii.cch = (sizeof(buffer) / 2) - 1;
				GetMenuItemInfoW(mmi->um.hmenu, mmi->umi.iPosition, TRUE, &mii);
				DWORD dwFlags = DT_CENTER | DT_SINGLELINE | DT_VCENTER;
				int textstate = 0, bgstate = 0;
				if ((mmi->dis.itemState & ODS_INACTIVE) | (mmi->dis.itemState & ODS_DEFAULT)) {
					textstate = 1;
					bgstate = 1;
				}
				if (mmi->dis.itemState & ODS_HOTLIGHT) {
					textstate = 2;
					bgstate = 2;
					bgcolor = &hover;
				}
				if (mmi->dis.itemState & ODS_SELECTED) {
					textstate = 2;
					bgstate = 2;
					bgcolor = &select;
				}
				if ((mmi->dis.itemState & ODS_GRAYED) || (mmi->dis.itemState & ODS_DISABLED)) {
					textstate = 3;
					bgstate = 3;
					bgcolor = &bg;
				}
				DTTOPTS opts = { sizeof(opts), DTT_TEXTCOLOR, textstate != 3 ? DarkMode::CLR_TEXT : DarkMode::CLR_TEXT_DIM };
				FillRect(mmi->um.hdc, &mmi->dis.rcItem, *bgcolor);
				HTHEME hTheme = OpenThemeData(hWnd, L"Menu");
				if (hTheme) {
					DrawThemeTextEx(hTheme, mmi->um.hdc, 8, 1, buffer, mii.cch, dwFlags, &mmi->dis.rcItem, &opts);
					CloseThemeData(hTheme);
				}
				DeleteObject(bg);
				DeleteObject(hover);
				DeleteObject(select);
				return true;
			}
			}
			return 0;
			} (hwnd, msg, wParam, lParam, &lr)) {
			return lr;
		}
		switch (msg) {
		case WM_MEASUREITEM: {
			MEASUREITEMSTRUCT* mis = (MEASUREITEMSTRUCT*)lParam;
			mis->itemHeight = 24; // 每个下拉项的高度
			return TRUE;
			break;
		}
		case WM_DRAWITEM: {
			DRAWITEMSTRUCT* dis = (DRAWITEMSTRUCT*)lParam;
			// 关键判断：itemID == -1 表示正在绘制 ComboBox 的“顶部显示区”
			// 其他值表示正在绘制下拉列表中的某一项
			if (dis->itemID == (UINT)-1)
			{
				// 绘制顶部显示区
				FillRect(dis->hDC, &dis->rcItem, DarkMode::brMain);

				// 获取当前选中项的文本
				int sel = (int)SendMessageW(dis->hwndItem, CB_GETCURSEL, 0, 0);
				if (sel != CB_ERR)
				{
					wchar_t buf[256] = {};
					SendMessageW(dis->hwndItem, CB_GETLBTEXT, sel, (LPARAM)buf);

					SetTextColor(dis->hDC, DarkMode::CLR_TEXT);
					SetBkMode(dis->hDC, TRANSPARENT);
					SetTextAlign(dis->hDC, TA_LEFT);

					RECT rc = dis->rcItem;
					rc.left += 4; // 左边距
					DrawTextW(dis->hDC, buf, -1, &rc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
				}
			} else {
				// 绘制下拉列表项
				wchar_t buf[256] = {};
				SendMessageW(dis->hwndItem, CB_GETLBTEXT, dis->itemID, (LPARAM)buf);

				// 判断是否选中
				bool selected = (dis->itemState & ODS_SELECTED) != 0;
				COLORREF bg = selected ? DarkMode::CLR_BG_CTRL_HOT : DarkMode::CLR_BG_MAIN;

				HBRUSH hBr = CreateSolidBrush(bg);
				FillRect(dis->hDC, &dis->rcItem, hBr);
				DeleteObject(hBr);

				SetTextColor(dis->hDC, DarkMode::CLR_TEXT);
				SetBkMode(dis->hDC, TRANSPARENT);
				SetTextAlign(dis->hDC, TA_LEFT);

				RECT rc = dis->rcItem;
				rc.left += 4;
				DrawTextW(dis->hDC, buf, -1, &rc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

				// 绘制选中虚线框（可选）
				if (dis->itemState & ODS_FOCUS)
				{
					DrawFocusRect(dis->hDC, &dis->rcItem);
				}
			}
			return TRUE;
		}
		case WM_NCHITTEST: {
			POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
			ScreenToClient(hwnd, &pt);

			// 获取菜单栏整体矩形
			MENUBARINFO mbi = { sizeof(mbi) };
			if (GetMenuBarInfo(hwnd, OBJID_MENU, 0, &mbi)) {
				RECT rcBar = mbi.rcBar;
				MapWindowPoints(HWND_DESKTOP, hwnd, (LPPOINT)&rcBar, 2);

				if (PtInRect(&rcBar, pt)) {
					// 遍历菜单项，判断是否点在某个菜单项上
					HMENU hMenu = GetMenu(hwnd);
					int count = GetMenuItemCount(hMenu);
					for (int i = 0; i < count; ++i) {
						RECT rcItem = { 0 };
						if (GetMenuItemRect(hwnd, hMenu, i, &rcItem)) {
							MapWindowPoints(HWND_DESKTOP, hwnd, (LPPOINT)&rcItem, 2);
							if (PtInRect(&rcItem, pt)) {
								return HTMENU;  // 菜单项：保持正常点击行为
							}
						}
					}
					return HTCAPTION;  // 菜单栏空白处：允许拖动
				}
			}
			break;
		}
		case WM_LBUTTONDOWN: {
			if (!(GetWindowLongPtrW(hwnd, -16) & WS_CHILD)) {
				ReleaseCapture();
				SendMessageW(hwnd, WM_SYSCOMMAND, SC_MOVE | HTCAPTION, 0);
			}
			break;
		}
		case WM_SETTINGCHANGE: {
			MARGINS margins = { -1, -1, -1, -1 };
			RECT rect;
			GetClientRect(hwnd, &rect);
			if (DarkMode::isDarkThemeEnabled) {
				if ((os.dwBuildNumber < 18362) && (os.dwBuildNumber >= 17763)) {
					SetPropW(hwnd, L"UseImmersiveDarkModeColors", HANDLE(DarkMode::isDarkThemeEnabled));
				}
				else if ((os.dwBuildNumber >= 18362) && (os.dwBuildNumber < 19041)) {
					DwmSetWindowAttribute(hwnd, 19, &DarkMode::isDarkThemeEnabled, sizeof(DarkMode::isDarkThemeEnabled));
				}
				//FillRect(GetDC(hwnd), &rect, CreateSolidBrush(DarkMode::CLR_BG_MAIN));
			}
			else {
				margins = { 0 };
			}
			if (os.dwBuildNumber >= 19041) {
				if (DarkMode::UseMica) {
					int DWMWACOLORNONE = DWMWA_COLOR_NONE;
					DwmSetWindowAttribute(hwnd, DWMWA_CAPTION_COLOR, &DWMWACOLORNONE, sizeof(DWMWACOLORNONE));
				}
				else {
					DwmSetWindowAttribute(hwnd, DWMWA_CAPTION_COLOR, &DarkMode::CLR_BG_MAIN, sizeof(DarkMode::CLR_BG_MAIN));
				}
			}
			/////////////////////////////////////////////////////////////////////////////////
			bool dark = ReadAppsUseLightTheme();
			if (dark != DarkMode::isDarkThemeEnabled) ApplyThemeColors(hwnd);
			DarkModeAPI::ApplyToAll(hwnd);
			DwmSetWindowAttribute(hwnd, 1029, &DarkMode::UseMica, sizeof(DarkMode::UseMica));

			if (DarkMode::UseMica) {
				DwmExtendFrameIntoClientArea(hwnd, &margins);
				DwmSetWindowAttribute(hwnd, DWMWA_SYSTEMBACKDROP_TYPE, &DarkMode::value, sizeof(DarkMode::value));
			}
			else {
				margins = { 0 };
				DwmExtendFrameIntoClientArea(hwnd, &margins);
			}
			DwmFlush();
			//ShowWindow(hwnd, SW_HIDE);
			//ShowWindow(hwnd, SW_SHOW);
			FixDarkScrollBar();
			//PaintBackground(hwnd,GetDC(hwnd));
			//HBRUSH hNewBrush = DarkMode::brMain;
			//HBRUSH hOld = (HBRUSH)SetClassLongPtrW(hwnd, GCLP_HBRBACKGROUND, (LONG_PTR)hNewBrush);
			//if (hOld) DeleteObject(hOld);   // 删掉旧的（如果是自己创建的）
			SendMessageW(hwnd, WM_NCACTIVATE, FALSE, 0);
			SendMessageW(hwnd, WM_NCACTIVATE, TRUE, 0);
			RedrawWindow(hwnd, 0, 0, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN | RDW_UPDATENOW);
			break;
		}
		case WM_ERASEBKGND: {
			// 仅处理背景擦除，保留原 WM_PAINT 逻辑
			HDC hdc = (HDC)wParam;
			if (hdc) {
				DarkModeAPI::PaintBackground(hwnd, hdc);
				return 0;
			}
			break;
		}
		case WM_PAINT: {
			// 保留原窗口过程的 WM_PAINT，避免吞掉默认绘制
			if (oldproc) {
				CallWindowProcW(oldproc, hwnd, msg, wParam, lParam);
			}
			return DefWindowProcW(hwnd, msg, wParam, lParam);
			break;
		}
		case WM_CTLCOLORSTATIC:
		case WM_CTLCOLOREDIT:
		case WM_CTLCOLORLISTBOX:
		case WM_CTLCOLORDLG:
		case WM_CTLCOLORBTN: {
			return DarkModeAPI::HandleCtlColor(msg, wParam, lParam);
		}
		}
		if (oldproc) {
			return CallWindowProcW(oldproc, hwnd, msg, wParam, lParam);
		}
		return DefWindowProcW(hwnd, msg, wParam, lParam);
	}

	void SetFont(const wstring& fontName, const wstring& fontSize, int fontWeight) {
		SetFont(0, fontName, fontSize, fontWeight);
	}

	void SetFont(HWND hWnd, const wstring& fontName, const wstring& fontSize, int fontWeight) {
		int size = _wtoi(fontSize.c_str());
		if (size <= 0) size = 18;
		const wchar_t* faceName = fontName.empty() ? L"Segoe UI" : fontName.c_str();

		HDC hdc = GetDC(hWnd);
		int dpiY = hdc ? GetDeviceCaps(hdc, LOGPIXELSY) : 96;
		if (hdc) ReleaseDC(hWnd, hdc);
		if (dpiY <= 0) dpiY = 96;
		int fontHeight = -MulDiv(size, dpiY, 96);

		if (DarkMode::hFont) {
			DeleteObject(DarkMode::hFont);
			DarkMode::hFont = 0;
		}

		DarkMode::hFont = CreateFontW(fontHeight, 0, 0, 0, fontWeight, false, false, false,
			DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
			DEFAULT_PITCH | FF_DONTCARE, faceName);
	}

	void Apply(HWND hCtrl, bool applyDefaultFont) {
		if (!hCtrl || !IsWindow(hCtrl)) return;
		RtlGetVersion(&os);
		wchar_t clsName[128]{};
		GetClassNameW(hCtrl, clsName, 127);
		bool keepDefaultFont = 1;
		const bool darkModeEnabled = DarkMode::isDarkThemeEnabled;
		//if (applyDefaultFont && DarkMode::hFont && !keepDefaultFont) {
		//	HFONT curFont = (HFONT)SendMessageW(hCtrl, WM_GETFONT, 0, 0);
		//	if (!curFont) {
		//		SendMessageW(hCtrl, WM_SETFONT, (WPARAM)DarkMode::hFont, true);
		//	}
		//}
		RegisterTouchWindow(hCtrl, 0);
		LONG_PTR style = GetWindowLongPtrW(hCtrl, GWL_STYLE);

		if (_wcsicmp(clsName, L"DirectUIHWND") == 0) {
			PostMessageW(hCtrl, WM_DEVMODECHANGE, 0, 0);
			InvalidateRect(GetParent(hCtrl), 0, false);
			return;
		}
		else if (_wcsicmp(clsName, L"Button") == 0) {
			LONG_PTR btnType = style & 0x0F;
			if (btnType == BS_CHECKBOX || btnType == BS_AUTOCHECKBOX || btnType == BS_3STATE || btnType == BS_AUTO3STATE) {
				DWORD_PTR existingData = 0;
				if (!GetWindowSubclass(hCtrl, DarkCheckBoxSubclassProc, DarkMode::SUBCLASS_ID_CHECKBOX, &existingData)) {
					auto* state = new DarkCheckBoxState();
					// 初始化时同步控件的勾选状态（支持三态）
					int cur = (int)SendMessageW(hCtrl, BM_GETCHECK, 0, 0);
					state->checkState = cur;
					state->threeState = (btnType == BS_3STATE || btnType == BS_AUTO3STATE);
					SetWindowSubclass(hCtrl, DarkCheckBoxSubclassProc, DarkMode::SUBCLASS_ID_CHECKBOX, (DWORD_PTR)state);
				}
				else {
					// 如果已经子类化，确保状态与当前控件状态同步（可能在创建后被程序设置过）
					auto* state = (DarkCheckBoxState*)existingData;
					if (state) {
						int cur = (int)SendMessageW(hCtrl, BM_GETCHECK, 0, 0);
						state->checkState = cur;
						state->threeState = (btnType == BS_3STATE || btnType == BS_AUTO3STATE);
					}
				}
				SetWindowTheme(hCtrl, nullptr, nullptr);
				printf("[DEBUG] Applied DarkMode to CheckBox: %p\n", hCtrl);
				InvalidateRect(hCtrl, 0, false);
			}
			else if (btnType == BS_AUTORADIOBUTTON || btnType == BS_RADIOBUTTON) {
				DWORD_PTR existingData = 0;
				if (!GetWindowSubclass(hCtrl, DarkRadioSubclassProc, 0, &existingData)) {
					auto* state = new DarkRadioState();
					state->checked = (SendMessageW(hCtrl, BM_GETCHECK, 0, 0) == BST_CHECKED);
					SetWindowSubclass(hCtrl, DarkRadioSubclassProc, 0, (DWORD_PTR)state);
					if (state->checked) {
						ToggleRadioButton(hCtrl, state, false);
					}
				}
				SetWindowTheme(hCtrl, nullptr, nullptr);
				printf("[DEBUG] Applied DarkMode to RadioButton: %p\n", hCtrl);
				InvalidateRect(hCtrl, 0, false);
				return;
			}
			else if (btnType == BS_GROUPBOX) {
				DWORD_PTR existingData = 0;
				if (!GetWindowSubclass(hCtrl, DarkGroupBoxSubclassProc, DarkMode::SUBCLASS_ID_GROUPBOX, &existingData)) {
					SetWindowSubclass(hCtrl, DarkGroupBoxSubclassProc, DarkMode::SUBCLASS_ID_GROUPBOX, 0);
				}
				//SetWindowTheme(hCtrl, nullptr, nullptr);
			}
			else {
				if (DarkMode::Subclassbtn) {
					DWORD_PTR existingData = 0;
					if (!GetWindowSubclass(hCtrl, DarkButtonSubclassProc, DarkMode::SUBCLASS_ID_BUTTON, &existingData)) {
						auto* state = new DarkButtonState();
						SetWindowSubclass(hCtrl, DarkButtonSubclassProc, DarkMode::SUBCLASS_ID_BUTTON, (DWORD_PTR)state);
					}
					SetWindowTheme(hCtrl, darkModeEnabled ? L"DarkMode_Explorer" : nullptr, 0);
				}
				SetWindowTheme(hCtrl, darkModeEnabled ? L"DarkMode_Explorer" : L"Explorer", 0);
			}
			InvalidateRect(hCtrl, 0, false);
		}
		else if (_wcsicmp(clsName, L"Edit") == 0) {
			if (darkModeEnabled) {
				SetWindowTheme(hCtrl, L"DarkMode_Explorer", 0);
				SetWindowLongPtrW(hCtrl, -20, GetWindowLongPtrW(hCtrl, -20) & ~WS_EX_CLIENTEDGE);
				SetWindowLongPtrW(hCtrl, -16, GetWindowLongPtrW(hCtrl, -16) | WS_BORDER);
			}
			else {
				SetWindowTheme(hCtrl, nullptr, nullptr);
			}
			PostMessageW(hCtrl, WM_DEVMODECHANGE, 0, 0);
			InvalidateRect(hCtrl, 0, false);
		}
		else if (_wcsicmp(clsName, WC_TREEVIEWW) == 0) {
			if (darkModeEnabled) {
				SetWindowTheme(hCtrl, L"DarkMode_Explorer", 0);
				SendMessageW(hCtrl, TVM_SETTEXTCOLOR, 0, (LPARAM)DarkMode::CLR_TEXT);
				SendMessageW(hCtrl, TVM_SETBKCOLOR, 0, (LPARAM)DarkMode::CLR_BG_CTRL);
				TreeView_SetTextColor(hCtrl, DarkMode::CLR_TEXT);
				TreeView_SetBkColor(hCtrl, DarkMode::CLR_BG_CTRL);
			}
			else {
				SetWindowTheme(hCtrl, nullptr, nullptr);
				TreeView_SetTextColor(hCtrl, GetSysColor(COLOR_WINDOWTEXT));
				TreeView_SetBkColor(hCtrl, GetSysColor(COLOR_WINDOW));
			}
			InvalidateRect(hCtrl, 0, false);
		}
		else if (_wcsicmp(clsName, MSFTEDIT_CLASS) == 0) {
			SetWindowTheme(hCtrl, darkModeEnabled ? L"DarkMode_Explorer" : nullptr, 0);
			LONG_PTR nStyle = GetWindowLongPtrW(hCtrl, GWL_STYLE);
			bool isReadOnly = (nStyle & ES_READONLY) == ES_READONLY;

			CHARFORMATW cf{};
			cf.cbSize = sizeof(CHARFORMATW);
			cf.dwMask = CFM_COLOR;
			cf.crTextColor = darkModeEnabled ? DarkMode::CLR_TEXT : GetSysColor(COLOR_WINDOWTEXT);
			SendMessageW(hCtrl, EM_SETCHARFORMAT, SCF_DEFAULT, LPARAM(&cf));

			const COLORREF clrBg = darkModeEnabled
				? (!isReadOnly ? DarkMode::CLR_BG_CTRL : DarkMode::CLR_BG_MAIN)
				: GetSysColor(COLOR_WINDOW);
			SendMessageW(hCtrl, EM_SETBKGNDCOLOR, 0, LPARAM(clrBg));
			if (darkModeEnabled) {
				SetWindowLongPtrW(hCtrl, -16, GetWindowLongPtrW(hCtrl, -16) | WS_BORDER);
				SetWindowLongPtrW(hCtrl, -20, GetWindowLongPtrW(hCtrl, -20) & ~WS_EX_CLIENTEDGE);
			}
			InvalidateRect(hCtrl, 0, false);
		}
		else if (_wcsicmp(clsName, WC_TABCONTROLW) == 0) {
			PreloadDarkTabRendererOnce(hCtrl);
			SetWindowTheme(hCtrl, L"", L"");
			DWORD_PTR existingData = 0;
			if (!GetWindowSubclass(hCtrl, DarkTabSubclassProc, DarkMode::SUBCLASS_ID_TAB, &existingData)) {
				auto* state = new DarkTabState();
				SetWindowSubclass(hCtrl, DarkTabSubclassProc, DarkMode::SUBCLASS_ID_TAB, (DWORD_PTR)state);
			}
			RedrawWindow(hCtrl, 0, 0, RDW_INVALIDATE | RDW_NOERASE | RDW_UPDATENOW);
		}
		else if (_wcsicmp(clsName, WC_HEADERW) == 0 || _wcsicmp(clsName, L"SysHeader32") == 0) {
			const wchar_t* headerTheme = nullptr;
			if (darkModeEnabled) {
				headerTheme = DarkMode::ForceDarkInInjectedProcess ? L"ItemsView" : L"DarkMode_Explorer";
			}
			SetWindowTheme(hCtrl, headerTheme, 0);
			DWORD_PTR existingData = 0;
			if (!GetWindowSubclass(hCtrl, DarkHeaderSubclassProc, DarkMode::SUBCLASS_ID_HEADER, &existingData)) {
				auto* hState = new DarkHeaderState();
				SetWindowSubclass(hCtrl, DarkHeaderSubclassProc, DarkMode::SUBCLASS_ID_HEADER, (DWORD_PTR)hState);
			}
			InvalidateRect(hCtrl, 0, false);
		}
		else if (_wcsicmp(clsName, WC_LISTVIEWW) == 0) {
			SetWindowTheme(hCtrl, darkModeEnabled ? L"DarkMode_Explorer" : nullptr, 0);
			ListView_SetBkColor(hCtrl, DarkMode::CLR_BG_CTRL);
			ListView_SetTextBkColor(hCtrl, DarkMode::CLR_BG_CTRL);
			ListView_SetTextColor(hCtrl, DarkMode::CLR_TEXT);
			HWND hHeader = ListView_GetHeader(hCtrl);
			if (hHeader) {
				DarkModeAPI::Apply(hHeader, true);
			}
			else if (DarkMode::ForceDarkInInjectedProcess) {
				HWND hList = hCtrl;
				thread([hList]() {
					for (int i = 0; i < 12; ++i) {
						if (!IsWindow(hList)) return;
						HWND hLateHeader = ListView_GetHeader(hList);
						if (hLateHeader) {
							DarkModeAPI::Apply(hLateHeader, true);
							RedrawWindow(hLateHeader, 0, 0, RDW_INVALIDATE | RDW_NOERASE | RDW_UPDATENOW);
							return;
						}
						Sleep(30);
					}
					}).detach();
			}
			DWORD_PTR existingData = 0;
			if (!GetWindowSubclass(hCtrl, DarkListViewSubclassProc, DarkMode::SUBCLASS_ID_LISTVIEW, &existingData)) {
				auto* state = new DarkListState();
				SetWindowSubclass(hCtrl, DarkListViewSubclassProc, DarkMode::SUBCLASS_ID_LISTVIEW, (DWORD_PTR)state);
			}
			ApplyToAll(hCtrl);
		}
		else if (_wcsicmp(clsName, WC_COMBOBOXW) == 0 || _wcsicmp(clsName, L"ComboBox") == 0) {
			SetWindowTheme(hCtrl, darkModeEnabled ? L"DarkMode_CFD" : nullptr, 0);
			SetWindowLongPtrW(hCtrl, -16, GetWindowLongPtrW(hCtrl, -16) | CBS_OWNERDRAWFIXED);
			DWORD_PTR existingData = 0;
			if (!GetWindowSubclass(hCtrl, DarkComboSubclassProc, DarkMode::SUBCLASS_ID_COMBO, &existingData)) {
				auto* state = new DarkComboState();
				SetWindowSubclass(hCtrl, DarkComboSubclassProc, DarkMode::SUBCLASS_ID_COMBO, (DWORD_PTR)state);
			}
		}
		else if (_wcsicmp(clsName, UPDOWN_CLASSW) == 0 || _wcsicmp(clsName, L"msctls_updown32") == 0) {
			SetWindowTheme(hCtrl, darkModeEnabled ? L"DarkMode_Explorer" : nullptr, 0);
			DWORD_PTR existingData = 0;
			if (!GetWindowSubclass(hCtrl, DarkUpDownSubclassProc, DarkMode::SUBCLASS_ID_UPDOWN, &existingData)) {
				auto* state = new DarkUpDownState();
				SetWindowSubclass(hCtrl, DarkUpDownSubclassProc, DarkMode::SUBCLASS_ID_UPDOWN, (DWORD_PTR)state);
			}
		}
		else if (_wcsicmp(clsName, L"ScrollBar") == 0 || _wcsicmp(clsName, WC_PAGESCROLLERW) == 0) {
			printf("[DEBUG] ScrollBar detected: %p\n", hCtrl);
			SetWindowTheme(hCtrl, darkModeEnabled ? L"DarkMode_Explorer" : nullptr, 0);
		}
		PostMessageW(hCtrl, WM_DEVMODECHANGE, 0, 0);
		InvalidateRect(GetParent(hCtrl), 0, false);
	}

	static void ApplyDialogSafeTheme(HWND hCtrl, bool applyDefaultFont) {
		if (!hCtrl || !IsWindow(hCtrl)) return;

		wchar_t clsName[128]{};
		GetClassNameW(hCtrl, clsName, 127);
		const bool darkModeEnabled = DarkMode::isDarkThemeEnabled;

		if (_wcsicmp(clsName, WC_COMBOBOXW) == 0 || _wcsicmp(clsName, L"ComboBox") == 0) {
			SetWindowTheme(hCtrl, darkModeEnabled ? L"DarkMode_CFD" : nullptr, 0);
		}
		else {
			SetWindowTheme(hCtrl, darkModeEnabled ? L"DarkMode_Explorer" : nullptr, 0);
		}

		//if (applyDefaultFont && DarkMode::hFont) {
		//	SendMessageW(hCtrl, WM_SETFONT, (WPARAM)DarkMode::hFont, true);
		//}
	}

	static HWND FindRootDialog(HWND hWnd) {
		while (hWnd) {
			wchar_t cls[64]{};
			GetClassNameW(hWnd, cls, 63);
			if (_wcsicmp(cls, L"#32770") == 0) return hWnd;
			hWnd = GetParent(hWnd);
		}
		return 0;
	}

	static bool DialogHasTabControl(HWND hDlg) {
		if (!hDlg || !IsWindow(hDlg)) return false;
		return FindWindowExW(hDlg, 0, WC_TABCONTROLW, 0) != 0 ||
			FindWindowExW(hDlg, 0, L"SysTabControl32", 0) != 0;
	}

	static bool IsDescendantWindow(HWND hChild, HWND hAncestor) {
		if (!hChild || !hAncestor) return false;
		HWND h = hChild;
		while (h) {
			if (h == hAncestor) return true;
			h = GetParent(h);
		}
		return false;
	}

	void InitWindow(HWND hParent) {
		//DarkMode::value = DarkMode::SystemBackdropType::Mica;
		DwmSetWindowAttribute(hParent, 20, &DarkMode::isDarkThemeEnabled, sizeof(int));
		if (os.dwBuildNumber >= 19041) {
			//DwmSetWindowAttribute(hParent, DWMWA_CAPTION_COLOR, &DarkMode::CLR_BG_MAIN, sizeof(DarkMode::CLR_BG_MAIN));
		}
		DwmFlush();
		//SetFocus(0);
		//SwitchToThisWindow(hParent, true);
		//SetFocus(hParent);
		// 在初始化时，如果不处于深色主题，则尝试在控件层面启用 Mica 采样
		ApplyThemeColors(hParent);
		// 不在这里强制覆盖 CLR_BG_MAIN，让调用者（ApplyThemeColors）决定配色
		if (!DarkMode::hFont) {
			SetFont(hParent, L"Segoe UI", L"18", FW_NORMAL);
		}
		//SetPreferredAppMode(1);
	}

	void ApplyToAll(HWND hParent) {
		InitWindow(hParent);
		//RegisterTouchWindow(hParent, 0);

		EnumChildWindows(hParent, [](HWND hChild, LPARAM) -> BOOL {
			Apply(hChild, true);
			return true;
			}, 0);
	}

	void PaintBackground(HWND hwnd, HDC hdc) {
		RECT rc{};
		GetClientRect(hwnd, &rc);
		FillRect(hdc, &rc, DarkMode::brMain);
	}

	LRESULT HandleCtlColor(UINT msg, WPARAM wParam, LPARAM lParam) {
		HDC hdc = (HDC)wParam;
		HWND hCtrl = (HWND)lParam;
		SetTextColor(hdc, DarkMode::CLR_TEXT);
		SetBkColor(hdc, DarkMode::CLR_BG_CTRL);

		wchar_t clsName[64]{};
		GetClassNameW(hCtrl, clsName, 63);

		if (_wcsicmp(clsName, L"Button") == 0) {
			LONG_PTR style = GetWindowLongPtrW(hCtrl, GWL_STYLE);
			// 检测是否为 groupbox
			if (style & BS_GROUPBOX) {
				// 【修复】让 GroupBox 使用透明背景，完全由自绘子类控制
				SetBkMode(hdc, TRANSPARENT);
				SetTextColor(hdc, DarkMode::CLR_TEXT);
				return (LRESULT)GetStockObject(NULL_BRUSH);  // 返回空画刷，不绘制背景
			}
			// 复选框保持透明
			LONG_PTR btnType = style & 0x0F;
			if (btnType == BS_CHECKBOX || btnType == BS_AUTOCHECKBOX || btnType == BS_3STATE || btnType == BS_AUTO3STATE) {
				SetBkMode(hdc, TRANSPARENT);
				return (LRESULT)GetStockObject(NULL_BRUSH);
			}
			//SetBkColor(hdc, DarkMode::CLR_BG_CTRL);
		}

		if (_wcsicmp(clsName, L"Static") == 0) {
			SetBkColor(hdc, DarkMode::CLR_BG_MAIN);
			SetBkMode(hdc, TRANSPARENT);
			return (LRESULT)DarkMode::brMain;
		}

		return (LRESULT)DarkMode::brCtrl;
	}

	void CleanUp() {
		if (DarkMode::hFont) { DeleteObject(DarkMode::hFont); DarkMode::hFont = 0; }
		if (DarkMode::brMain) { DeleteObject(DarkMode::brMain); DarkMode::brMain = 0; }
		if (DarkMode::brCtrl) { DeleteObject(DarkMode::brCtrl); DarkMode::brCtrl = 0; }
		if (DarkMode::brTabPage) { DeleteObject(DarkMode::brTabPage); DarkMode::brTabPage = 0; }
		if (DarkMode::brTabHost) { DeleteObject(DarkMode::brTabHost); DarkMode::brTabHost = 0; }
	}
}
HHOOK g_hCBT = NULL;
struct WndProcStruct {
	HWND hwnd;
	WNDPROC originalProc;
};
vector<WndProcStruct> windowProcs;
// CBT 钩子过程
BOOL __stdcall EnumWindowsProc(HWND hwnd, LPARAM lParam) {
	//if (IsDarkModeApplied(hwnd)) {
	//	return 1; // 已经应用过深色模式，跳过
	//}
	DWORD pid = 0;
	GetWindowThreadProcessId(hwnd, &pid);
	wchar_t className[256]{};
	GetClassNameW(hwnd, className, sizeof(className));
	if (pid == GetCurrentProcessId() && wcscmp(className, L"#32768") != 0) {
		bool found = false;
		for (const auto& info : windowProcs) {
			if (info.hwnd == hwnd) {
				found = true;
				break;
			}
		}
		if (!found) {
			if (!(GetWindowLongPtrW(hwnd, -16) & WS_CHILD) || wstring(className) == L"#32770") {
				printf("[DEBUG] hwnd=0x%p, class=%s\n", hwnd, wstring(className).c_str());
				DarkModeAPI::ApplyToAll(hwnd);
				SetDarkModeApplied(hwnd);
				windowProcs.push_back(WndProcStruct({ hwnd, (WNDPROC)GetWindowLongPtrW(hwnd, GWLP_WNDPROC) }));
				SetWindowLongPtrW(hwnd, GWLP_WNDPROC, (LONG_PTR)WNDPROC([](HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)->LRESULT {
					WNDPROC originalProc{};
					for (const auto& info : windowProcs) {
						if (info.hwnd == hwnd) {
							originalProc = info.originalProc;
							break;
						}
					}
					//WndProcStruct* procStruct = (WndProcStruct*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
					//if (procStruct) {
						//if (procStruct->hwnd == hwnd) {
					switch (msg) {
					}
					if (originalProc) {
						return DarkModeAPI::HandleWindow(hwnd, msg, wParam, lParam, originalProc);
					}
					else {
						return DarkModeAPI::HandleWindow(hwnd, msg, wParam, lParam);
					}
					//}
				//}
				//return DefWindowProcW(hwnd, msg, wParam, lParam);
					}));
				PostMessageW(hwnd, WM_SETTINGCHANGE, 0, 0);
				//InvalidateRect(hwnd, 0, true);
			}
		}
	}
	EnumChildWindows(hwnd, EnumWindowsProc, 0);
	return 1;
}
//LRESULT CALLBACK CBTProc(int nCode, WPARAM wParam, LPARAM lParam) {
//	CWPSTRUCT* pMsg = (CWPSTRUCT*)lParam;
//	if (pMsg->message == WM_CREATE || pMsg->message == WM_NCCREATE) {
//		HWND hwnd = (HWND)wParam;
//		// 打印窗口句柄和类名
//		WCHAR className[256];
//		GetClassNameW(hwnd, className, 256);
//		// 也可以打印到控制台
//		printf("HWND: 0x%p, class: %s\n", hwnd, className);
//		if (wstring(className)== L"#32770") {
//			DarkModeAPI::ApplyToAll(hwnd);
//			windowProcs.push_back(WndProcStruct({ hwnd, (WNDPROC)GetWindowLongPtrW(hwnd, GWLP_WNDPROC) }));
//			SetWindowLongPtrW(hwnd, GWLP_WNDPROC, (LONG_PTR)WNDPROC([](HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)->LRESULT {
//				WNDPROC originalProc{};
//				for (const auto& info : windowProcs) {
//					if (info.hwnd == hwnd) {
//						originalProc = info.originalProc;
//						break;
//					}
//				}
//				//WndProcStruct* procStruct = (WndProcStruct*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
//				//if (procStruct) {
//					//if (procStruct->hwnd == hwnd) {
//				switch (msg) {
//				}
//				if (originalProc) {
//					return DarkModeAPI::HandleWindow(hwnd, msg, wParam, lParam, originalProc);
//				}
//				else {
//					return DarkModeAPI::HandleWindow(hwnd, msg, wParam, lParam);
//				}
//				//}
//			//}
//			//return DefWindowProcW(hwnd, msg, wParam, lParam);
//				}));
//			SendMessageW(hwnd, WM_SETTINGCHANGE, 0, 0);
//		}
//	}
//
//	// 调用下一个钩子
//	return CallNextHookEx(g_hCBT, nCode, wParam, lParam);
//}
//BOOL InstallCBTHook() {
//	DWORD dwThreadId = GetCurrentThreadId();
//	g_hCBT = SetWindowsHookExW(
//		WH_CALLWNDPROC,              // 钩子类型
//		CBTProc,             // 钩子过程
//		NULL,                // 模块句柄（当前进程）
//		GetCurrentThreadId()           // 线程ID（只监控当前线程）
//	);
//	//CreateThread(0, 0, [](LPVOID) -> ULONG {
//	//	while (1) {
//	//		EnumWindows(EnumWindowsProc, 0);
//	//		Sleep(100);
//	//	}
//	//	return 0;
//	//	}, 0, 0, 0);
//	return 1;
//}
#include <map>
HWINEVENTHOOK g_hWinEventHook = NULL;
//vector<WndProcStruct> windowProcs;
map<HWND, bool> g_processedWindows;  // 避免重复处理
void ApplyDarkModeToWindow(HWND hwnd) {
	// 检查是否已经子类化
	if (g_processedWindows.find(hwnd) != g_processedWindows.end()) {
		return;
	}

	// 保存原始窗口过程
	windowProcs.push_back({ hwnd, (WNDPROC)GetWindowLongPtrW(hwnd, GWLP_WNDPROC) });

	// 子类化
	SetWindowLongPtrW(hwnd, GWLP_WNDPROC, (LONG_PTR)WNDPROC([](HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)->LRESULT {
		WNDPROC originalProc = nullptr;
		for (const auto& info : windowProcs) {
			if (info.hwnd == hWnd) {
				originalProc = info.originalProc;
				break;
			}
		}

		if (originalProc) {
			return DarkModeAPI::HandleWindow(hWnd, msg, wParam, lParam, originalProc);
		}
		return DarkModeAPI::HandleWindow(hWnd, msg, wParam, lParam);
		}));

	g_processedWindows[hwnd] = true;
	thread([hwnd]() {
		SendMessageW(hwnd, WM_SETTINGCHANGE, 0, 0);
		UpdateWindow(hwnd);
		}).detach();
}
void EnumAllChildWindows(HWND hParent) {
	EnumChildWindows(hParent, [](HWND hChild, LPARAM lParam) -> BOOL {
		WCHAR className[256] = { 0 };
		GetClassNameW(hChild, className, 256);

		if (wstring(className) == L"#32770") {
			if (g_processedWindows.find(hChild) == g_processedWindows.end()) {
				DarkModeAPI::ApplyToAll(hChild);
				ApplyDarkModeToWindow(hChild);
				EnumAllChildWindows(hChild);
			}
		}
		return TRUE;
		}, 0);
}
void CALLBACK WinEventProc(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime) {
	if (idObject != OBJID_WINDOW || idChild != CHILDID_SELF) {
		return;
	}
	DWORD dwPid = 0;
	GetWindowThreadProcessId(hwnd, &dwPid);
	if (dwPid != GetCurrentProcessId()) {
		return;
	}

	WCHAR className[256] = { 0 };
	GetClassNameW(hwnd, className, 256);

	const bool isPopupMenuWindow = (_wcsicmp(className, L"#32768") == 0 || _wcsicmp(className, L"PopupMenu") == 0);
	if (isPopupMenuWindow) {
		return;
	}

	const bool isDialogWindow = (wstring(className) == L"#32770" || wstring(className) == L"CHECKLIST_ACLUI");
	const bool isListViewWindow = (_wcsicmp(className, WC_LISTVIEWW) == 0 || _wcsicmp(className, L"SysListView32") == 0);
	const bool isHeaderWindow = (_wcsicmp(className, WC_HEADERW) == 0 || _wcsicmp(className, L"SysHeader32") == 0);
	const bool isTopLevelWindow = (GetParent(hwnd) == nullptr) && ((GetWindowLongPtrW(hwnd, GWL_STYLE) & WS_CHILD) == 0);

	if (isHeaderWindow) {
		HWND hParent = GetParent(hwnd);
		if (hParent) {
			WCHAR parentClass[256] = { 0 };
			GetClassNameW(hParent, parentClass, 256);
			if (_wcsicmp(parentClass, WC_LISTVIEWW) == 0 || _wcsicmp(parentClass, L"SysListView32") == 0) {
				DarkModeAPI::Apply(hwnd, true);
				RedrawWindow(hwnd, 0, 0, RDW_INVALIDATE | RDW_NOERASE | RDW_UPDATENOW);
				return;
			}
		}
	}

	if (isListViewWindow) {
		DarkModeAPI::Apply(hwnd, true);
		RedrawWindow(hwnd, 0, 0, RDW_INVALIDATE | RDW_NOERASE | RDW_UPDATENOW);
		return;
	}

	if (isDialogWindow || isTopLevelWindow) {
		if (g_processedWindows.find(hwnd) != g_processedWindows.end()) {
			return;
		}

		DarkModeAPI::ApplyToAll(hwnd);
		ApplyDarkModeToWindow(hwnd);
		EnumAllChildWindows(hwnd);
	}
}

// ============== 安装钩子 ==============
BOOL InstallCBTHook(bool mica = false, bool subclassbtn = true) {
	// 设置窗口创建事件钩子
	RtlGetVersion(&os);
	if (os.dwMajorVersion >= 10) {
		SetPreferredAppMode(DarkMode::ForceDarkInInjectedProcess ? 2 : 1);
	}
	DarkModeAPI::FixDarkScrollBar();
	DarkMode::UseMica = mica, DarkMode::Subclassbtn = subclassbtn;
	DarkMode::value = DarkMode::SystemBackdropType::Mica;
	ApplyThemeColors(0);
	g_hWinEventHook = SetWinEventHook(
		EVENT_OBJECT_CREATE,        // 对象创建事件
		EVENT_OBJECT_CREATE,        // 相同
		NULL,                       // 不需要DLL
		WinEventProc,               // 回调函数
		GetCurrentProcessId(),      // 只监控当前进程
		0,                          // 所有线程
		WINEVENT_OUTOFCONTEXT       // 异步回调
	);

	if (!g_hWinEventHook) {
		printf("[错误] 事件钩子安装失败: %d\n", GetLastError());
		return FALSE;
	}
	printf("[初始化] 事件钩子安装成功\n");

	// 处理已存在的窗口
	EnumWindows([](HWND hwnd, LPARAM) -> BOOL {
		DWORD dwPid = 0;
		GetWindowThreadProcessId(hwnd, &dwPid);

		if (dwPid == GetCurrentProcessId()) {
			WCHAR className[256] = { 0 };
			GetClassNameW(hwnd, className, 256);

			if (wstring(className) == L"#32770") {
				printf("[已存在] 对话框: 0x%p\n", hwnd);
				DarkModeAPI::ApplyToAll(hwnd);
				ApplyDarkModeToWindow(hwnd);
				EnumAllChildWindows(hwnd);
			}
		}
		return TRUE;
		}, 0);

	return TRUE;
}

// ============== 清理 ==============
void UninstallCBTHook() {
	if (g_hWinEventHook) {
		UnhookWinEvent(g_hWinEventHook);
		g_hWinEventHook = NULL;
	}

	// 清理子类化
	for (auto& info : windowProcs) {
		if (IsWindow(info.hwnd)) {
			SetWindowLongPtrW(info.hwnd, GWLP_WNDPROC, (LONG_PTR)info.originalProc);
		}
	}
	windowProcs.clear();
	g_processedWindows.clear();

	printf("[清理] 钩子已卸载\n");
}