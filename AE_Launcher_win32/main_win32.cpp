#include <windows.h>
#include <commctrl.h> // サブクラス化に必要
#include <string>
#include <vector>
#include "..\AE_Launcher_DxLib\AfterFXInfo.h"

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "comctl32.lib") // サブクラス化用

const int BTN_SIZE = 64;
const int BTN_COUNT = 5;
const int MARGIN = 12;
const int SPACING = 8;
const int ID_BUTTON_START = 1000;

static int g_SelectedButton = -1;

static AfterFXInfo g_ai; // AfterFXInfoクラスのインスタンスをグローバルに作成
// --- 文字コード変換 ---
std::wstring Utf8ToWide(const std::string& utf8) {
	if (utf8.empty()) return L"";
	int size = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, NULL, 0);
	std::wstring wstr(size, 0);
	MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, &wstr[0], size);
	return wstr;
}

// --- ボタンのサブクラスプロシージャ (マウスホバー検知用) ---
LRESULT CALLBACK ButtonSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
	switch (uMsg) {
	case WM_MOUSEMOVE: {
		// マウスがボタン内に入った時の処理
		int index = (int)dwRefData;
		if (g_SelectedButton != index) {
			g_SelectedButton = index;
			// 親ウィンドウに再描画を依頼
			InvalidateRect(GetParent(hWnd), NULL, TRUE);
		}

		// マウスが外に出るのを監視開始
		TRACKMOUSEEVENT tme = { sizeof(tme) };
		tme.dwFlags = TME_LEAVE;
		tme.hwndTrack = hWnd;
		TrackMouseEvent(&tme);
		break;
	}
	case WM_MOUSELEAVE: {
		// マウスがボタンから離れたら選択を解除したい場合はここを有効にする
		// g_SelectedButton = -1;
		// InvalidateRect(GetParent(hWnd), NULL, TRUE);
		break;
	}
	}
	return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

// --- ボタンのカスタム描画 (変更なし) ---
void DrawMyFlatButton(LPDRAWITEMSTRUCT pdis, const std::wstring& text, bool isActive) {
	HDC hdc = pdis->hDC;
	RECT rc = pdis->rcItem;
	bool isPressed = (pdis->itemState & ODS_SELECTED);

	COLORREF bgColor = (isPressed || isActive) ? RGB(0, 80, 180) : RGB(45, 45, 48);
	HBRUSH hBrush = CreateSolidBrush(bgColor);
	FillRect(hdc, &rc, hBrush);
	DeleteObject(hBrush);

	if (isActive) {
		HBRUSH hActiveBrush = CreateSolidBrush(RGB(0, 180, 255));
		RECT rL = { rc.left, rc.top, rc.left + 3, rc.bottom }, rR = { rc.right - 3, rc.top, rc.right, rc.bottom };
		RECT rT = { rc.left, rc.top, rc.right, rc.top + 3 }, rB = { rc.left, rc.bottom - 3, rc.right, rc.bottom };
		FillRect(hdc, &rL, hActiveBrush); FillRect(hdc, &rR, hActiveBrush);
		FillRect(hdc, &rT, hActiveBrush); FillRect(hdc, &rB, hActiveBrush);
		DeleteObject(hActiveBrush);
	}
	else {
		HBRUSH hNormalBrush = CreateSolidBrush(RGB(80, 80, 80));
		FrameRect(hdc, &rc, hNormalBrush);
		DeleteObject(hNormalBrush);
	}

	SetTextColor(hdc, RGB(255, 255, 255));
	SetBkMode(hdc, TRANSPARENT);
	HFONT hFont = CreateFontW(24, 0, 0, 0, FW_MEDIUM, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Yu Gothic UI");
	HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
	DrawTextW(hdc, text.c_str(), -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	SelectObject(hdc, hOldFont);
	DeleteObject(hFont);
}

// --- メインプロシージャ ---
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	static std::vector<std::string> labels = { "TOOL", "PEN", "FILL", "ZOOM", "SET" };

	switch (uMsg) {
	case WM_CREATE:
		for (int i = 0; i < g_ai.GetAECount(); i++) {
			HWND hBtn = CreateWindowW(L"BUTTON", L"", WS_VISIBLE | WS_CHILD | BS_OWNERDRAW | WS_TABSTOP,
				MARGIN + (i * (BTN_SIZE + SPACING)), MARGIN, BTN_SIZE, BTN_SIZE,
				hwnd, (HMENU)(UINT_PTR)(ID_BUTTON_START + i), ((LPCREATESTRUCT)lParam)->hInstance, NULL);

			// ★サブクラス化を設定 (iをインデックスとして渡す)
			SetWindowSubclass(hBtn, ButtonSubclassProc, 0, (DWORD_PTR)i);
		}
		return 0;

	case WM_KEYDOWN:
	{
		bool changed = false;
		switch (wParam) {
		case VK_LEFT:
			if (g_SelectedButton <= 0) g_SelectedButton = BTN_COUNT - 1;
			else g_SelectedButton--;
			changed = true;
			break;
		case VK_RIGHT:
			if (g_SelectedButton >= BTN_COUNT - 1 || g_SelectedButton == -1) g_SelectedButton = 0;
			else g_SelectedButton++;
			changed = true;
			break;
		case VK_RETURN:
			if (g_SelectedButton != -1) {
				SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(ID_BUTTON_START + g_SelectedButton, BN_CLICKED), 0);
			}
			break;
		}
		if (changed) InvalidateRect(hwnd, NULL, TRUE);
		return 0;
	}

	case WM_COMMAND:
		if (LOWORD(wParam) >= ID_BUTTON_START && LOWORD(wParam) < ID_BUTTON_START + BTN_COUNT) {
			g_SelectedButton = LOWORD(wParam) - ID_BUTTON_START;
			InvalidateRect(hwnd, NULL, TRUE);
			UpdateWindow(hwnd);
			std::wstring msg = Utf8ToWide(labels[g_SelectedButton]) + L" モードを選択";
			MessageBoxW(hwnd, msg.c_str(), L"Event", MB_OK | MB_TOPMOST);
		}
		return 0;

	case WM_DRAWITEM:
	{
		LPDRAWITEMSTRUCT pdis = (LPDRAWITEMSTRUCT)lParam;
		if (pdis->CtlID >= ID_BUTTON_START && pdis->CtlID < ID_BUTTON_START + BTN_COUNT) {
			int idx = pdis->CtlID - ID_BUTTON_START;
			DrawMyFlatButton(pdis, Utf8ToWide(g_ai.GetVersion(idx)), (pdis->CtlID - ID_BUTTON_START == g_SelectedButton));
		}
		return TRUE;
	}

	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hwnd, &ps);
		HBRUSH hBg = CreateSolidBrush(RGB(30, 30, 30));
		FillRect(hdc, &ps.rcPaint, hBg);
		DeleteObject(hBg);
		EndPaint(hwnd, &ps);
		return 0;
	}
	case WM_ERASEBKGND: return 1;
	case WM_DESTROY: PostQuitMessage(0); return 0;
	}
	return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, PWSTR, int nCmdShow) {
	if (!g_ai.IsInstalled()) {
		MessageBoxW(NULL, L"After Effects が見つかりませんでした。", L"Error", MB_OK);
		return -1;
	}   
	const wchar_t CLASS_NAME[] = L"FlatToolApp";
	WNDCLASSW wc = { };
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInst;
	wc.lpszClassName = CLASS_NAME;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	RegisterClassW(&wc);

	int clientW = (BTN_SIZE * g_ai.GetAECount()) + (SPACING * (g_ai.GetAECount() - 1)) + (MARGIN * 2);
	int clientH = BTN_SIZE + (MARGIN * 2);
	DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU;
	RECT wr = { 0, 0, clientW, clientH };
	AdjustWindowRectEx(&wr, style, FALSE, 0);
	int windowW = wr.right - wr.left;
	int windowH = wr.bottom - wr.top;

	// 2. マウスカーソルの位置からモニターを特定する
	POINT pt;
	GetCursorPos(&pt);
	HMONITOR hMonitor = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);

	// 3. モニターの情報を取得 (タスクバーを除いた作業領域 rcWork を使用)
	MONITORINFO mi = { sizeof(mi) };
	GetMonitorInfo(hMonitor, &mi);

	// 4. 特定したモニターの作業領域内での中央座標を計算
	int screenW = mi.rcWork.right - mi.rcWork.left;
	int screenH = mi.rcWork.bottom - mi.rcWork.top;

	int x = mi.rcWork.left + (screenW - windowW) / 2;
	int y = mi.rcWork.top + (screenH - windowH) / 2;
	HWND hwnd = CreateWindowExW(0, CLASS_NAME, L"AE Launcher win32", 
		style, 
		x, y, 
		windowW, windowH, 
		NULL,
		NULL, 
		hInst,
		NULL);
	if (!hwnd) return 0;
	ShowWindow(hwnd, nCmdShow);

	MSG msg = { };
	while (GetMessageW(&msg, NULL, 0, 0) > 0) {
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}
	return 0;
}