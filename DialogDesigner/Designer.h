// Designer.h - 重构版
#pragma once
#include <Windows.h>
#include <Uxtheme.h>
#include <dwmapi.h>
#include <commctrl.h>
#include <commdlg.h>
#include <string>
#include <vector>
#include <iostream>
#include <windowsx.h>
#include <algorithm>
#include <map>

#pragma comment(lib,"uxtheme.lib")
#pragma comment(lib,"dwmapi.lib")
#pragma comment(lib,"comctl32.lib")
#pragma comment(lib,"comdlg32.lib")
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

using namespace std;

// 前向声明
extern HWND g_HWND, g_PropertiesHWND;
extern const int START_WIDTH, START_HEIGHT;
extern HFONT g_hGuiFont;
extern vector<LPCWSTR> listclassname;
extern void SyncControlSummaryPanel();
extern void AppendControlChangeMarker();

// ==================== 属性窗口 ====================
extern HWND g_PropertiesHWND;
vector<LPCWSTR> listdescription;
vector<LPCWSTR> listclassname;

// 属性窗口控件ID
enum PropertyControlIds {
	IDC_PROP_TITLE = 100,
	IDC_PROP_X,
	IDC_PROP_Y,
	IDC_PROP_WIDTH,
	IDC_PROP_HEIGHT,
	IDC_PROP_CLASS,
	IDC_PROP_FONT_BTN,
	IDC_PROP_UPDATE_BTN
};

// ==================== GDI资源管理 ====================
class GdiResourceManager {
private:
	std::map<HWND, HPEN> cachedPens;
	std::map<HWND, HBRUSH> cachedBrushes;
	std::map<HWND, HFONT> cachedFonts;

public:
	HPEN GetResizeHandlePen() {
		static HPEN hPen = CreatePen(PS_SOLID, 1, DarkMode::CLR_TEXT);
		return hPen;
	}

	HBRUSH GetResizeHandleBrush() {
		static HBRUSH hBrush = CreateSolidBrush(DarkMode::CLR_TEXT);
		return hBrush;
	}

	HBRUSH GetBackgroundBrush(HWND hwnd) {
		auto it = cachedBrushes.find(hwnd);
		if (it != cachedBrushes.end()) {
			return it->second;
		}
		HBRUSH brush = CreateSolidBrush(DarkMode::CLR_BG_MAIN);
		cachedBrushes[hwnd] = brush;
		return brush;
	}

	~GdiResourceManager() {
		for (auto& pair : cachedPens) DeleteObject(pair.second);
		for (auto& pair : cachedBrushes) DeleteObject(pair.second);
		for (auto& pair : cachedFonts) DeleteObject(pair.second);
	}
};

GdiResourceManager g_GdiMgr;

// ==================== 控件数据结构 ====================
struct Control {
	HWND hwnd;
	bool dragging = false, moving = false;
	WNDPROC oldproc;
	int zOrder;
	wstring className;
	wstring title;
	int x, y, width, height;
	HFONT hFont = NULL;
};

vector<Control> g_Controls;
int g_CurIndex = -1;
int g_LastIndex = -1;
int g_ControlIdCounter = 0;

// 缩放状态
bool g_IsResizing = false;
bool g_IsMoving = false;
int g_ActiveResizeHandle = -1;
POINT g_ResizeStartMouse;
RECT g_ResizeStartRect;

// ==================== 工具函数 ====================
int FindControlIndex(HWND hCtrl) {
	for (size_t i = 0; i < g_Controls.size(); i++) {
		if (g_Controls[i].hwnd == hCtrl) return (int)i;
	}
	return -1;
}

Control* GetCurrentControl() {
	if (g_CurIndex >= 0 && g_CurIndex < (int)g_Controls.size()) {
		return &g_Controls[g_CurIndex];
	}
	return nullptr;
}

void BringToFront(int index) {
	if (index < 0 || index >= (int)g_Controls.size()) return;
	SetWindowPos(g_Controls[index].hwnd, HWND_TOP, 0, 0, 0, 0,
		SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
}

// ==================== 调整大小手柄 ====================
int HitTestResizeHandle(HWND hCtrl, int x, int y) {
	RECT rc;
	GetClientRect(hCtrl, &rc);
	const int SQUARE_SIZE = 6;
	const int HALF = SQUARE_SIZE / 2;
	const int HIT_MARGIN = 4;

	POINT positions[8] = {
		{HALF, HALF},
		{rc.right / 2, HALF},
		{rc.right - HALF, HALF},
		{rc.right - HALF, rc.bottom / 2},
		{rc.right - HALF, rc.bottom - HALF},
		{rc.right / 2, rc.bottom - HALF},
		{HALF, rc.bottom - HALF},
		{HALF, rc.bottom / 2}
	};

	for (int i = 0; i < 8; i++) {
		RECT hitArea;
		hitArea.left = positions[i].x - HALF - HIT_MARGIN;
		hitArea.top = positions[i].y - HALF - HIT_MARGIN;
		hitArea.right = positions[i].x + HALF + HIT_MARGIN;
		hitArea.bottom = positions[i].y + HALF + HIT_MARGIN;

		if (x >= hitArea.left && x <= hitArea.right &&
			y >= hitArea.top && y <= hitArea.bottom) {
			return i;
		}
	}
	return -1;
}

HCURSOR GetResizeCursor(int handle) {
	static HCURSOR cursors[8] = {
		LoadCursorW(0, IDC_SIZENWSE),
		LoadCursorW(0, IDC_SIZENS),
		LoadCursorW(0, IDC_SIZENESW),
		LoadCursorW(0, IDC_SIZEWE),
		LoadCursorW(0, IDC_SIZENWSE),
		LoadCursorW(0, IDC_SIZENS),
		LoadCursorW(0, IDC_SIZENESW),
		LoadCursorW(0, IDC_SIZEWE)
	};
	return cursors[handle];
}

void DrawResizeHandles(HWND hCtrl, HDC hdc) {
	RECT rc;
	GetClientRect(hCtrl, &rc);

	const int SQUARE_SIZE = 6;
	const int HALF = SQUARE_SIZE / 2;

	POINT positions[8] = {
		{HALF, HALF},
		{rc.right / 2, HALF},
		{rc.right - HALF, HALF},
		{rc.right - HALF, rc.bottom / 2},
		{rc.right - HALF, rc.bottom - HALF},
		{rc.right / 2, rc.bottom - HALF},
		{HALF, rc.bottom - HALF},
		{HALF, rc.bottom / 2}
	};

	HPEN hPen = g_GdiMgr.GetResizeHandlePen();
	HBRUSH hBrush = g_GdiMgr.GetResizeHandleBrush();
	HPEN oldPen = (HPEN)SelectObject(hdc, hPen);
	HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, hBrush);

	for (int i = 0; i < 8; i++) {
		RECT square;
		square.left = positions[i].x - HALF;
		square.top = positions[i].y - HALF;
		square.right = positions[i].x + HALF + (SQUARE_SIZE % 2 ? 1 : 0);
		square.bottom = positions[i].y + HALF + (SQUARE_SIZE % 2 ? 1 : 0);
		Rectangle(hdc, square.left, square.top, square.right, square.bottom);
	}

	SelectObject(hdc, oldPen);
	SelectObject(hdc, oldBrush);
	// 不要删除共享资源
}
struct FontInfo {
	std::wstring faceName;   // 字体名称，如 "Segoe UI"
	int          height;     // 逻辑高度（负值 = 字符高度）
	int          pointSize;  // 磅值（磅 = 高度 * 72 / DPI）
	int          pixelSize;  // 像素高度（abs(height)）
	int          weight;     // 字重：400=正常, 700=粗体
	bool         italic;     // 斜体
	bool         underline;  // 下划线
	bool         strikeOut;  // 删除线
	BYTE         charSet;    // 字符集
};
FontInfo GetWindowFontInfo(HWND hwnd) {
	FontInfo info;
	// 1. 获取 HFONT
	HFONT hFont = (HFONT)SendMessageW(hwnd, WM_GETFONT, 0, 0);

	//hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	// 3. 解析 LOGFONT
	LOGFONTW lf = {};
	GetObjectW(hFont, sizeof(LOGFONTW), &lf);

	// 4. 填充信息
	info.faceName = lf.lfFaceName;
	info.height = lf.lfHeight;
	info.pixelSize = abs(lf.lfHeight);
	info.weight = lf.lfWeight;
	info.italic = (lf.lfItalic != 0);
	info.underline = (lf.lfUnderline != 0);
	info.strikeOut = (lf.lfStrikeOut != 0);
	info.charSet = lf.lfCharSet;

	// 5. 计算磅值（需要屏幕 DPI）
	HDC hdc = GetDC(hwnd);
	int dpiY = GetDeviceCaps(hdc, LOGPIXELSY);
	ReleaseDC(hwnd, hdc);
	info.pointSize = -MulDiv(lf.lfHeight, 72, dpiY);  // lfHeight 为负时

	return info;
}
HFONT ChooseFontAndCreate(HWND hwndOwner, HFONT pInitialFont) {
	// 1. 准备 LOGFONT：如果给了初始字体，先读出来
	LOGFONTW lf = {};
	if (pInitialFont) {
		if (GetObjectW(pInitialFont, sizeof(LOGFONTW), &lf) == 0) {
			// 读失败就用默认值
			lf.lfHeight = -16;
			lf.lfCharSet = DEFAULT_CHARSET;
		}
	}
	else {
		// 没给初始字体，用系统默认 GUI 字体
		HFONT hSys = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
		GetObjectW(hSys, sizeof(LOGFONTW), &lf);
	}

	// 2. 准备 CHOOSEFONT 结构
	CHOOSEFONTW cf = {};
	cf.lStructSize = sizeof(CHOOSEFONTW);
	cf.hwndOwner = hwndOwner;
	cf.lpLogFont = &lf;                       // ← 对话框修改的就是这个结构
	cf.Flags = CF_INITTOLOGFONTSTRUCT     // 用 lf 作为初始字体
		| CF_SCREENFONTS             // 只列出屏幕字体
		| CF_EFFECTS                 // 启用颜色/斜体/下划线/删除线
		| CF_NOVERTFONTS;            // 不列出竖排字体（可选）
	cf.nFontType = SCREEN_FONTTYPE;

	// 3. 弹出对话框
	if (!ChooseFontW(&cf)) {
		return NULL;   // 用户取消
	}

	// 4. 用修改后的 LOGFONT 创建 HFONT
	//    如果选了 CF_EFFECTS，cf.rgbColors 里还有颜色，这里只返回字体
	HFONT hFont = CreateFontIndirectW(&lf);
	return hFont;   // 可能是 NULL（极少见）
}

void UpdatePropertyPanel(Control* ctl) {
	if (!g_PropertiesHWND) return;
	HWND hTitle = GetDlgItem(g_PropertiesHWND, IDC_PROP_TITLE);
	HWND hX = GetDlgItem(g_PropertiesHWND, IDC_PROP_X);
	HWND hY = GetDlgItem(g_PropertiesHWND, IDC_PROP_Y);
	HWND hW = GetDlgItem(g_PropertiesHWND, IDC_PROP_WIDTH);
	HWND hH = GetDlgItem(g_PropertiesHWND, IDC_PROP_HEIGHT);
	HWND hClass = GetDlgItem(g_PropertiesHWND, IDC_PROP_CLASS);
	HWND hFontBtn = GetDlgItem(g_PropertiesHWND, IDC_PROP_FONT_BTN);
	//if (!hTitle || !hX || !hY || !hW || !hH || !hClass) return;
	wchar_t buffer[256]{};
	if (ctl) {
		// 控件信息
		SetWindowTextW(hTitle, ctl->title.c_str());
		swprintf_s(buffer, L"%d", ctl->x);
		SetWindowTextW(hX, buffer);
		swprintf_s(buffer, L"%d", ctl->y);
		SetWindowTextW(hY, buffer);
		swprintf_s(buffer, L"%d", ctl->width);
		SetWindowTextW(hW, buffer);
		swprintf_s(buffer, L"%d", ctl->height);
		SetWindowTextW(hH, buffer);

		// 查找类名显示
		wstring classDisplay = ctl->className;
		for (size_t i = 0; i < listclassname.size(); i++) {
			if (listclassname[i] == ctl->className) {
				if (i < listdescription.size()) {
					classDisplay = listdescription[i];
				}
				break;
			}
		}
		SetWindowTextW(hClass, classDisplay.c_str());

		// 更新字体按钮显示
		HFONT useFont = ctl->hFont ? ctl->hFont : g_hGuiFont;
		LOGFONTW lf{};
		if (1) {
			auto fi = GetWindowFontInfo(ctl->hwnd);
			//wprintf(L"[DEBUG] Font Info: %s, %dpx, %dpt, weight=%d, italic=%d\n",
				//fi.faceName.c_str(), fi.pixelSize, fi.pointSize, fi.weight, fi.italic);
			//MessageBoxW(g_HWND, fi.faceName.c_str(), L"Font Info", MB_OK);
			wchar_t buf[128];
			swprintf_s(buf, L"%s %dpx", fi.faceName.c_str(), fi.pointSize);
			LOGFONTW lf = {};
			lf.lfHeight = 22;						// 高度
			lf.lfWidth = 0;							// 宽度（0=自动）
			lf.lfEscapement = 0;					// 字符串倾斜角度
			lf.lfOrientation = 0;					// 字符倾斜角度
			lf.lfWeight = fi.weight;				// 字重
			lf.lfItalic = fi.italic;				// 斜体
			lf.lfUnderline = FALSE;					// 下划线
			lf.lfStrikeOut = FALSE;					// 删除线
			lf.lfCharSet = DEFAULT_CHARSET;			// ← 注意：原来是 1，看下面说明
			lf.lfOutPrecision = OUT_DEFAULT_PRECIS;
			lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
			lf.lfQuality = DEFAULT_QUALITY;
			lf.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
			wcscpy_s(lf.lfFaceName, fi.faceName.c_str());
			HFONT hFont = CreateFontIndirectW(&lf);
			wchar_t oldBtnText[128] = {};
			GetWindowTextW(hFontBtn, oldBtnText, 128);
			if (wcscmp(oldBtnText, buf) != 0) {
				SetWindowTextW(hFontBtn, buf);
			}
		}
		else {
			SetWindowTextW(hFontBtn, L"Font...");
		}

		// 启用所有编辑框
		EnableWindow(hTitle, true);
		EnableWindow(hX, true);
		EnableWindow(hY, true);
		EnableWindow(hW, true);
		EnableWindow(hH, true);
		EnableWindow(hFontBtn, true);
	}
	else {
		// 对话框信息
		RECT rect;
		GetClientRect(g_HWND, &rect);
		SetWindowTextW(hTitle, L"对话框");

		swprintf_s(buffer, L"%d", 0);
		SetWindowTextW(hX, buffer);
		SetWindowTextW(hY, buffer);

		swprintf_s(buffer, L"%d", rect.right - rect.left);
		SetWindowTextW(hW, buffer);

		swprintf_s(buffer, L"%d", rect.bottom - rect.top);
		SetWindowTextW(hH, buffer);

		SetWindowTextW(hClass, L"对话框");

		// 禁用编辑框（除了对话框大小）
		EnableWindow(hTitle, false);
		EnableWindow(hX, false);
		EnableWindow(hY, false);
		EnableWindow(hW, false);
		EnableWindow(hH, false);
		EnableWindow(hClass, false);
		EnableWindow(hFontBtn, false);
	}
	InvalidateRect(GetDlgItem(g_PropertiesHWND, IDC_PROP_FONT_BTN), 0, true);
	UpdateWindow(g_HWND);
	if (g_CurIndex >= 0 && g_CurIndex < (int)g_Controls.size()) {
		UpdateWindow(g_Controls[g_CurIndex].hwnd);
	}
}

// ==================== 控件操作 ====================
void DeleteControl(int index) {
	if (index < 0 || index >= (int)g_Controls.size()) return;
	HWND hwndToDelete = g_Controls[index].hwnd;
	// 清理每个控件的字体（如果不是共享字体）
	if (g_Controls[index].hFont && g_Controls[index].hFont != g_hGuiFont) {
		DeleteObject(g_Controls[index].hFont);
	}
	g_Controls.erase(g_Controls.begin() + index);

	if (g_CurIndex == index) {
		g_CurIndex = -1;
		UpdatePropertyPanel(nullptr);
	}
	else if (g_CurIndex > index) {
		g_CurIndex--;
	}
	SyncControlSummaryPanel();
	AppendControlChangeMarker();
	DestroyWindow(hwndToDelete);
}

// ==================== 控件子类化过程 ====================
LRESULT CALLBACK ControlProc(HWND hCtrl, UINT msg, WPARAM wParam, LPARAM lParam) {
	int ctlIndex = FindControlIndex(hCtrl);
	if (ctlIndex == -1) {
		return DefWindowProcW(hCtrl, msg, wParam, lParam);
	}
	Control* ctl = &g_Controls[ctlIndex];

	switch (msg) {
	case WM_PAINT: {
		CallWindowProcW(ctl->oldproc, hCtrl, msg, wParam, lParam);
		if (ctlIndex == g_CurIndex) {
			HDC hdc = GetDC(hCtrl);
			if (hdc) {
				DrawResizeHandles(hCtrl, hdc);
				ReleaseDC(hCtrl, hdc);
			}
		}
		return 0;
	}

	case WM_SETCURSOR: {
		if (ctlIndex == g_CurIndex) {
			POINT pt;
			GetCursorPos(&pt);
			ScreenToClient(hCtrl, &pt);

			int handle = HitTestResizeHandle(hCtrl, pt.x, pt.y);
			if (handle != -1) {
				SetCursor(GetResizeCursor(handle));
				return true;
			}
		}
		return CallWindowProcW(ctl->oldproc, hCtrl, msg, wParam, lParam);
	}

	case WM_LBUTTONDOWN: {
		if (g_CurIndex != ctlIndex) {
			int prevIndex = g_CurIndex;
			g_CurIndex = ctlIndex;
			BringToFront(ctlIndex);
			// 只重绘当前控件内容，避免擦除背景造成闪烁
			InvalidateRect(hCtrl, NULL, FALSE);
			UpdateWindow(hCtrl);
			SetFocus(hCtrl);
			if (prevIndex != -1 && prevIndex < (int)g_Controls.size()) {
				// 立即刷新上一个控件，移除其拉伸句柄
				UpdateWindow(g_Controls[prevIndex].hwnd);
				InvalidateRect(g_Controls[prevIndex].hwnd, NULL, TRUE);
			}
			UpdatePropertyPanel(ctl);
			SyncControlSummaryPanel();
		}

		int x = GET_X_LPARAM(lParam);
		int y = GET_Y_LPARAM(lParam);
		int handle = HitTestResizeHandle(hCtrl, x, y);

		if (handle != -1) {
			g_IsResizing = true;
			g_IsMoving = false;
			g_ActiveResizeHandle = handle;
			GetCursorPos(&g_ResizeStartMouse);
			GetWindowRect(hCtrl, &g_ResizeStartRect);
			SetCapture(hCtrl);
		}
		else {
			g_IsMoving = true;
			g_IsResizing = false;
			g_ActiveResizeHandle = -1;
			GetCursorPos(&g_ResizeStartMouse);
			GetWindowRect(hCtrl, &g_ResizeStartRect);
			SetCapture(hCtrl);
		}
		return 0;
	}

	case WM_MOUSEMOVE: {
		if ((g_IsResizing || g_IsMoving) && ctlIndex == g_CurIndex) {
			POINT pt;
			GetCursorPos(&pt);
			int dx = pt.x - g_ResizeStartMouse.x;
			int dy = pt.y - g_ResizeStartMouse.y;
			RECT newRect = g_ResizeStartRect;

			if (g_IsMoving) {
				newRect.left = g_ResizeStartRect.left + dx;
				newRect.top = g_ResizeStartRect.top + dy;
				newRect.right = g_ResizeStartRect.right + dx;
				newRect.bottom = g_ResizeStartRect.bottom + dy;
			}
			else if (g_IsResizing) {
				switch (g_ActiveResizeHandle) {
				case 0: newRect.left = g_ResizeStartRect.left + dx; newRect.top = g_ResizeStartRect.top + dy; break;
				case 1: newRect.top = g_ResizeStartRect.top + dy; break;
				case 2: newRect.right = g_ResizeStartRect.right + dx; newRect.top = g_ResizeStartRect.top + dy; break;
				case 3: newRect.right = g_ResizeStartRect.right + dx; break;
				case 4: newRect.right = g_ResizeStartRect.right + dx; newRect.bottom = g_ResizeStartRect.bottom + dy; break;
				case 5: newRect.bottom = g_ResizeStartRect.bottom + dy; break;
				case 6: newRect.left = g_ResizeStartRect.left + dx; newRect.bottom = g_ResizeStartRect.bottom + dy; break;
				case 7: newRect.left = g_ResizeStartRect.left + dx; break;
				}

				int minWidth = 30, minHeight = 30;
				if (newRect.right - newRect.left < minWidth) {
					if (g_ActiveResizeHandle == 0 || g_ActiveResizeHandle == 6 || g_ActiveResizeHandle == 7)
						newRect.left = newRect.right - minWidth;
					else if (g_ActiveResizeHandle == 2 || g_ActiveResizeHandle == 3 || g_ActiveResizeHandle == 4)
						newRect.right = newRect.left + minWidth;
				}
				if (newRect.bottom - newRect.top < minHeight) {
					if (g_ActiveResizeHandle == 0 || g_ActiveResizeHandle == 1 || g_ActiveResizeHandle == 2)
						newRect.top = newRect.bottom - minHeight;
					else if (g_ActiveResizeHandle == 4 || g_ActiveResizeHandle == 5 || g_ActiveResizeHandle == 6)
						newRect.bottom = newRect.top + minHeight;
				}
			}

			HWND parent = GetParent(hCtrl);
			POINT topLeft = { newRect.left, newRect.top };
			ScreenToClient(parent, &topLeft);

			SetWindowPos(hCtrl, 0,
				topLeft.x, topLeft.y,
				newRect.right - newRect.left,
				newRect.bottom - newRect.top,
				SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);

			// 更新控件数据
			ctl->x = topLeft.x;
			ctl->y = topLeft.y;
			ctl->width = newRect.right - newRect.left;
			ctl->height = newRect.bottom - newRect.top;
			UpdatePropertyPanel(ctl);

			InvalidateRect(hCtrl, NULL, FALSE);
			return 0;
		}
		break;
	}

	case WM_LBUTTONUP: {
		if (g_IsResizing || g_IsMoving) {
			RECT currentRect;
			GetWindowRect(hCtrl, &currentRect);
			bool changed = (currentRect.left != g_ResizeStartRect.left) ||
				(currentRect.top != g_ResizeStartRect.top) ||
				(currentRect.right != g_ResizeStartRect.right) ||
				(currentRect.bottom != g_ResizeStartRect.bottom);
			g_IsResizing = false;
			g_IsMoving = false;
			g_ActiveResizeHandle = -1;
			ReleaseCapture();
			if (changed) {
				AppendControlChangeMarker();
			}
			return 0;
		}
		break;
	}

	case WM_SETFOCUS: {
		if (g_CurIndex != ctlIndex) {
			int prevIndex = g_CurIndex;
			g_CurIndex = ctlIndex;
			BringToFront(ctlIndex);
			if (prevIndex != -1 && prevIndex < (int)g_Controls.size()) {
				InvalidateRect(g_Controls[prevIndex].hwnd, NULL, TRUE);
				UpdateWindow(g_Controls[prevIndex].hwnd);
			}
			InvalidateRect(hCtrl, NULL, FALSE);
			UpdateWindow(hCtrl);
			SyncControlSummaryPanel();
		}
		UpdatePropertyPanel(ctl);
		return CallWindowProcW(ctl->oldproc, hCtrl, msg, wParam, lParam);
	}
	}

	return CallWindowProcW(ctl->oldproc, hCtrl, msg, wParam, lParam);
}

void DuplicateControl(int index) {
	if (index < 0 || index >= (int)g_Controls.size()) return;

	Control* ctl = &g_Controls[index];
	wchar_t className[256] = { 0 };
	GetClassNameW(ctl->hwnd, className, 256);

	wchar_t title[256] = { 0 };
	GetWindowTextW(ctl->hwnd, title, 256);
	wstring newTitle = title;
	if (newTitle.empty()) newTitle = ctl->title;

	RECT rect;
	GetWindowRect(ctl->hwnd, &rect);
	HWND parent = GetParent(ctl->hwnd);
	POINT topLeft = { rect.left, rect.top };
	ScreenToClient(parent, &topLeft);

	int newX = topLeft.x + 20;
	int newY = topLeft.y + 20;
	int width = rect.right - rect.left;
	int height = rect.bottom - rect.top;

	LONG style = GetWindowLongW(ctl->hwnd, GWL_STYLE);
	LONG exStyle = GetWindowLongW(ctl->hwnd, GWL_EXSTYLE);

	HWND newHwnd = CreateWindowExW(exStyle, className, newTitle.c_str(),
		style | WS_CHILD | WS_VISIBLE, newX, newY, width, height,
		parent, HMENU(g_ControlIdCounter++), 0, 0);

	if (!newHwnd) return;

	// 复制字体
	HFONT hFont = CreateFontW(22, 0, 0, 0, FW_NORMAL, 0, 0, 0, 1, 0, 0, 0, 0, L"Segoe UI");
	SendMessageW(newHwnd, WM_SETFONT, WPARAM(hFont), 1);

	Control newCtl;
	newCtl.hwnd = newHwnd;
	newCtl.oldproc = (WNDPROC)GetWindowLongPtrW(newHwnd, GWLP_WNDPROC);
	newCtl.className = className;
	newCtl.title = newTitle;
	newCtl.x = newX;
	newCtl.y = newY;
	newCtl.width = width;
	newCtl.height = height;
	newCtl.zOrder = g_ControlIdCounter;
	// 继承字体引用（如果是共享字体就共享，否则创建独立副本）
	HFONT srcFont = (HFONT)SendMessageW(ctl->hwnd, WM_GETFONT, 0, 0);
	if (srcFont && srcFont != g_hGuiFont) {
		LOGFONTW lf{};
		if (GetObjectW(srcFont, sizeof(lf), &lf)) {
			newCtl.hFont = CreateFontIndirectW(&lf);
		}
		else {
			newCtl.hFont = g_hGuiFont;
		}
	}
	else {
		newCtl.hFont = g_hGuiFont;
	}

	HFONT applyFont = newCtl.hFont ? newCtl.hFont : g_hGuiFont;
	SendMessageW(newHwnd, WM_SETFONT, WPARAM(applyFont), 1);

	SetWindowLongPtrW(newHwnd, GWLP_WNDPROC, (LONG_PTR)ControlProc);
	g_Controls.push_back(newCtl);

	int newIndex = (int)g_Controls.size() - 1;
	g_CurIndex = newIndex;
	UpdatePropertyPanel(&g_Controls[newIndex]);
	BringToFront(newIndex);
	InvalidateRect(newHwnd, NULL, FALSE);
	UpdateWindow(newHwnd);
	SyncControlSummaryPanel();
	AppendControlChangeMarker();
}

void MoveControlWithKeyboard(int index, int dx, int dy) {
	if (index < 0 || index >= (int)g_Controls.size()) return;
	Control* ctl = &g_Controls[index];

	RECT rect;
	GetWindowRect(ctl->hwnd, &rect);
	HWND parent = GetParent(ctl->hwnd);
	POINT topLeft = { rect.left, rect.top };
	ScreenToClient(parent, &topLeft);

	int moveX = dx * 1;
	int moveY = dy * 1;

	SetWindowPos(ctl->hwnd, 0, topLeft.x + moveX, topLeft.y + moveY,
		rect.right - rect.left, rect.bottom - rect.top,
		SWP_NOZORDER | SWP_NOACTIVATE);

	ctl->x = topLeft.x + moveX;
	ctl->y = topLeft.y + moveY;
	UpdatePropertyPanel(ctl);
	AppendControlChangeMarker();
}

// ==================== 创建控件 ====================
Control CreateControlWithProc(HWND hwnd, LPCWSTR classname, LPCWSTR title,
	int x, int y, int w, int h, bool alreadyexists = false) {
	DWORD dwStyle = WS_CHILD | WS_VISIBLE, dwStyleEx = 0;

	if (wcscmp(classname, L"Edit") == 0) {
		dwStyle |= ES_AUTOHSCROLL | ES_LEFT;
		dwStyleEx |= WS_EX_CLIENTEDGE;
	}
	else if (wcscmp(classname, L"Static") == 0) {
		dwStyle |= SS_NOTIFY;
	}

	HWND tmph;
	if (alreadyexists) {
		tmph = hwnd;
	}
	else {
		tmph = CreateWindowExW(dwStyleEx, classname, title, dwStyle,
			x, y, w, h, hwnd, HMENU(g_ControlIdCounter++), 0, 0);
	}

	if (!tmph) {
		Control empty{};
		return empty;
	}

	HFONT hFont = CreateFontW(-16, 0, 0, 0, FW_NORMAL, 0, 0, 0, 1, 0, 0, 0, 0, L"Segoe UI");
	SendMessageW(tmph, WM_SETFONT, WPARAM(hFont), 1);

	Control ctl;
	ctl.hwnd = tmph;
	ctl.oldproc = (WNDPROC)GetWindowLongPtrW(tmph, GWLP_WNDPROC);
	ctl.className = classname;
	ctl.title = title;
	ctl.x = x;
	ctl.y = y;
	ctl.width = w;
	ctl.height = h;
	ctl.zOrder = g_ControlIdCounter;
	ctl.hFont = hFont;
	LOGGER.debug("Control created: hwnd=%p, class=%ls, title=%ls, x=%d, y=%d, w=%d, h=%d", tmph, classname, title, x, y, w, h);

	SetWindowLongPtrW(tmph, GWLP_WNDPROC, (LONG_PTR)ControlProc);
	g_Controls.push_back(ctl);
	g_CurIndex = (int)g_Controls.size() - 1;
	SyncControlSummaryPanel();
	AppendControlChangeMarker();
	UpdatePropertyPanel(&ctl);
	SendMessageW(g_HWND, WM_SETTINGCHANGE, 0, 0);
	return g_Controls.back();
}

// ==================== 窗口尺寸工具 ====================
bool ResizeWindow(HWND hwnd, int w, int h, bool full = false) {
	RECT rect = { 0, 0, w, h };
	if (!full) {
		AdjustWindowRectEx(&rect, GetWindowLongPtrW(hwnd, GWL_STYLE),
			GetMenu(hwnd) != NULL, GetWindowLongPtrW(hwnd, GWL_EXSTYLE));
	}
	return SetWindowPos(hwnd, HWND_TOP, 0, 0,
		rect.right - rect.left, rect.bottom - rect.top,
		SWP_NOMOVE | SWP_NOZORDER);
}

bool SetWindowSize(HWND hwnd, int w, int h, bool full = false) {
	RECT rect = { 0, 0, w, h };
	if (!full) {
		AdjustWindowRectEx(&rect, GetWindowLongPtrW(hwnd, GWL_STYLE),
			GetMenu(hwnd) != NULL, GetWindowLongPtrW(hwnd, GWL_EXSTYLE));
	}
	return SetWindowPos(hwnd, HWND_TOP, 0, 0, rect.right - rect.left, rect.bottom - rect.top, SWP_NOMOVE | SWP_NOZORDER);
}

SIZE GetWindowSize(HWND hwnd, bool full = false) {
	RECT rect;
	GetWindowRect(hwnd, &rect);
	int windowWidth = rect.right - rect.left;
	int windowHeight = rect.bottom - rect.top;

	rect = { 0, 0, windowWidth, windowHeight };
	if (!full) {
		AdjustWindowRectEx(&rect, GetWindowLongPtrW(hwnd, GWL_STYLE),
			GetMenu(hwnd) != NULL, GetWindowLongPtrW(hwnd, GWL_EXSTYLE));
	}
	SIZE ans{};
	ans.cx = windowWidth - ((rect.right - rect.left) - windowWidth);
	ans.cy = windowHeight - ((rect.bottom - rect.top) - windowHeight);
	return ans;
}

// ==================== 创建设计器窗口 ====================
void CreateDesignerWindow() {
	WNDCLASSEXW wc = { sizeof(WNDCLASSEXW) };
	wc.lpfnWndProc = [](HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT {
		switch (msg) {
		case WM_ERASEBKGND: {
			RECT rect;
			GetClientRect(hwnd, &rect);
			HBRUSH hBrush = CreateSolidBrush(isDarkThemeEnabled ? RGB(32, 32, 32) : RGB(240, 240, 240));
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
				// 点击空白区域 - 取消选中
				int index = SendMessageW(GetDlgItem(hwndParent, 3), LB_GETCURSEL, 0, 0);
				if (index > 0 && index < (int)listclassname.size()) {
					CreateControlWithProc(hwnd, listclassname[index], L"Example", x, y, 100, 50);
				}
				else {
					g_CurIndex = -1;
					UpdatePropertyPanel(nullptr);
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
		return DefWindowProcW(hwnd, msg, wparam, lparam);
		};

	wc.hInstance = GetModuleHandleW(0);
	wc.hCursor = LoadCursorW(0, IDC_ARROW);
	wc.lpszClassName = L"DialogDesigner";
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	RegisterClassExW(&wc);

	g_HWND = CreateWindowExW(WS_EX_CONTROLPARENT, L"DialogDesigner", L"Form1",
		WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_CHILD,
		240, 0, 0, 0, hwndParent, 0, 0, 0);

	ResizeWindow(g_HWND, START_WIDTH, START_HEIGHT);
}
// ==================== 创建属性窗口 ====================
void CreatePropertiesPanelWindow() {
	WNDCLASSEXW wc = { sizeof(WNDCLASSEXW) };
	wc.lpfnWndProc = [](HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT {
		switch (msg) {
		case WM_CREATE: {
			// 标题
			CreateWindowExW(0, L"Static", L"属性", WS_CHILD | WS_VISIBLE | SS_CENTER,
				0, 0, 200, 25, hwnd, 0, 0, 0);

			int y = 30;
			int labelW = 50;
			int editW = 130;
			int spacing = 28;

			// 类名（只读）
			CreateWindowExW(0, L"Static", L"类型:", WS_CHILD | WS_VISIBLE | SS_RIGHT,
				0, y, labelW, 22, hwnd, 0, 0, 0);
			CreateWindowExW(WS_EX_CLIENTEDGE, L"Edit", L"",
				WS_CHILD | WS_VISIBLE | ES_READONLY,
				labelW + 5, y, editW, 22, hwnd, HMENU(IDC_PROP_CLASS), 0, 0);

			y += spacing;

			// 标题
			CreateWindowExW(0, L"Static", L"标题:", WS_CHILD | WS_VISIBLE | SS_RIGHT,
				0, y, labelW, 22, hwnd, 0, 0, 0);
			CreateWindowExW(WS_EX_CLIENTEDGE, L"Edit", L"",
				WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
				labelW + 5, y, editW, 22, hwnd, HMENU(IDC_PROP_TITLE), 0, 0);

			y += spacing;

			// X
			CreateWindowExW(0, L"Static", L"X:", WS_CHILD | WS_VISIBLE | SS_RIGHT,
				0, y, labelW, 22, hwnd, 0, 0, 0);
			CreateWindowExW(WS_EX_CLIENTEDGE, L"Edit", L"0",
				WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_NUMBER,
				labelW + 5, y, editW / 2 - 5, 22, hwnd, HMENU(IDC_PROP_X), 0, 0);

			// Y
			CreateWindowExW(0, L"Static", L"Y:", WS_CHILD | WS_VISIBLE | SS_RIGHT,
				labelW + 5 + editW / 2, y, labelW / 2, 22, hwnd, 0, 0, 0);
			CreateWindowExW(WS_EX_CLIENTEDGE, L"Edit", L"0",
				WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_NUMBER,
				labelW + 5 + editW / 2 + labelW / 2, y, editW / 2 - 5, 22, hwnd, HMENU(IDC_PROP_Y), 0, 0);

			y += spacing;

			// 宽度
			CreateWindowExW(0, L"Static", L"宽度:", WS_CHILD | WS_VISIBLE | SS_RIGHT,
				0, y, labelW, 22, hwnd, 0, 0, 0);
			CreateWindowExW(WS_EX_CLIENTEDGE, L"Edit", L"100",
				WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_NUMBER,
				labelW + 5, y, editW / 2 - 5, 22, hwnd, HMENU(IDC_PROP_WIDTH), 0, 0);

			// 高度
			CreateWindowExW(0, L"Static", L"高度:", WS_CHILD | WS_VISIBLE | SS_RIGHT,
				labelW + 5 + editW / 2, y, labelW / 2, 22, hwnd, 0, 0, 0);
			CreateWindowExW(WS_EX_CLIENTEDGE, L"Edit", L"50",
				WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_NUMBER,
				labelW + 5 + editW / 2 + labelW / 2, y, editW / 2 - 5, 22, hwnd, HMENU(IDC_PROP_HEIGHT), 0, 0);

			y += spacing + 10;

			// 应用按钮
			CreateWindowExW(0, L"Button", L"应用更改", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
				(200 - 80) / 2, y, 80, 28, hwnd, HMENU(IDC_PROP_UPDATE_BTN), 0, 0);

			// 设置字体
			HFONT hFont = CreateFontW(18, 0, 0, 0, FW_NORMAL, 0, 0, 0, 1, 0, 0, 0, 0, L"Segoe UI");
			EnumChildWindows(hwnd, [](HWND hWnd, LPARAM lParam) -> BOOL {
				SendMessageW(hWnd, WM_SETFONT, lParam, 1);
				return true;
				}, (LPARAM)hFont);

			// 初始更新
			UpdatePropertyPanel(nullptr);
			break;
		}
		case WM_COMMAND: {
			printf("[DEBUG] WM_COMMAND: wparam=0x%X, lparam=0x%X\n", wparam, lparam);
			switch (LOWORD(wparam)) {
			case IDC_PROP_FONT_BTN: {
				HFONT hFont = ChooseFontAndCreate(hwnd, g_CurIndex != -1 ? g_Controls[g_CurIndex].hFont : g_hGuiFont);
				if (hFont) {
					SendMessageW(g_Controls[g_CurIndex].hwnd, WM_SETFONT, WPARAM(hFont), 1);
				}
				break;
			}
			case IDC_PROP_UPDATE_BTN: {
				// 如果需要，可以在此处理应用更改（当前逻辑由外部管理）
				break;
			}
			}
			break;
		}
		case WM_ERASEBKGND: {
			RECT rect;
			GetClientRect(hwnd, &rect);
			HBRUSH hBrush = CreateSolidBrush(isDarkThemeEnabled ? RGB(32, 32, 32) : RGB(240, 240, 240));
			FillRect((HDC)wparam, &rect, hBrush);
			DeleteObject(hBrush);
			return 1;
		}

		case WM_DESTROY: {
			PostQuitMessage(0);
			break;
		}
		}
		return DefWindowProcW(hwnd, msg, wparam, lparam);
		};

	wc.hInstance = GetModuleHandleW(0);
	wc.hCursor = LoadCursorW(0, IDC_ARROW);
	wc.lpszClassName = L"PropertiesPanel";
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	RegisterClassExW(&wc);

	g_PropertiesHWND = CreateWindowExW(WS_EX_CONTROLPARENT, L"PropertiesPanel", L"属性窗口", WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_CHILD, 0, 0, 250, 400, hwndParent, HMENU(0xFFFA), 0, 0);
}