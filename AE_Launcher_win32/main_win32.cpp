#include <windows.h>
#include <commctrl.h> // サブクラス化に必要
#include <string>
#include <vector>
#include <shellapi.h>
#include "..\AE_Launcher_DxLib\AfterFXInfo.h"
#include "..\AE_Launcher_DxLib\au.h"
#include "resource.h"
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "comctl32.lib") // サブクラス化用
#pragma comment(lib, "shell32.lib")

const int BTN_SIZE = 60;
const int MARGIN = 10;
const int SPACING = 8;
const int ID_BUTTON_START = 1000;

//const float BTN_SIZE = 60.0f;
//const float WINDOW_PADDING = 10.0f;
//const float ITEM_SPACING = 8.0f; // 追加

static int g_SelectedButton = -1;

static AfterFXInfo g_ai; // AfterFXInfoクラスのインスタンスをグローバルに作成
static std::string                     lastSelectedVersion = "";
// --- 文字コード変換 ---
std::wstring Utf8ToWide(const std::string& utf8) {
	if (utf8.empty()) return L"";
	int size = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, NULL, 0);
	std::wstring wstr(size, 0);
	MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, &wstr[0], size);
	return wstr;
}
// *****************************************************************************************************
bool CallAfterFX(HWND hwnd, int idx) {
	if (idx >= 0 && idx < g_ai.GetAECount()) {
		SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
		ShowWindow(hwnd, SW_HIDE);
		if (g_ai.CallAfterFX(idx) == false)
		{
			ShowWindow(hwnd, SW_SHOW);
			SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
			SetForegroundWindow(hwnd); // ウィンドウを前面に出す
			MessageBox(NULL, L"After Effects の起動に失敗しました。", L"Error", MB_OK);
			return false;
		}
		g_ai.SaveVersion("AE_Launcher", g_ai.GetVersion(idx));
		return true;
	}
	return false;
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
			HWND hParent = GetParent(hWnd);
			RedrawWindow(hParent, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
			//InvalidateRect(GetParent(hWnd), NULL, TRUE);
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
	case WM_SETCURSOR:
		SetCursor(LoadCursor(NULL, IDC_ARROW));
		return TRUE; // ボタン自身の標準的なカーソル変更を拒否
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
void UpdateFocusByMousePos(HWND hwnd) {
	POINT pt;
	if (GetCursorPos(&pt)) {
		ScreenToClient(hwnd, &pt);
		HWND hChild = ChildWindowFromPoint(hwnd, pt);

		// 自分（親ウィンドウ）以外の子ウィンドウ（ボタン）がヒットした場合
		if (hChild && hChild != hwnd) {
			int btnId = GetDlgCtrlID(hChild);
			if (btnId >= ID_BUTTON_START && btnId < ID_BUTTON_START + g_ai.GetAECount()) {
				int index = btnId - ID_BUTTON_START;
				if (g_SelectedButton != index) {
					g_SelectedButton = index; // ここで値を更新

					// ★ ここで強制的に描き直させます
					// RDW_UPDATENOW が「今すぐ描け」という命令です
					RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
				}
				return;
			}
		}
	}
}
// --- メインプロシージャ ---
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	switch (uMsg) {
	case WM_CREATE:
	{
		for (int i = 0; i < g_ai.GetAECount(); i++) {
			HWND hBtn = CreateWindowW(L"BUTTON", L"", WS_VISIBLE | WS_CHILD | BS_OWNERDRAW | WS_TABSTOP,
				MARGIN + (i * (BTN_SIZE + SPACING)), MARGIN, BTN_SIZE, BTN_SIZE,
				hwnd, (HMENU)(UINT_PTR)(ID_BUTTON_START + i), ((LPCREATESTRUCT)lParam)->hInstance, NULL);

			// ★サブクラス化を設定 (iをインデックスとして渡す)
			SetWindowSubclass(hBtn, ButtonSubclassProc, 0, (DWORD_PTR)i);
		}
		DragAcceptFiles(hwnd, TRUE);
		return 0;
	}
	
	case WM_MOUSEMOVE:
	case WM_NCMOUSEMOVE: // タイトルバー上の動きも監視
	{
		// マウスが動いている間、OSが勝手にリングを出そうとするのを上書きし続ける
		SetCursor(LoadCursor(NULL, IDC_ARROW));

		// 通常の移動処理へ（DefWindowProcに流してもOK）
		break;
	}
	case WM_KEYDOWN:
	{
		bool changed = false;
		switch (wParam) {
		case VK_LEFT:
			if (g_SelectedButton <= 0) g_SelectedButton = g_ai.GetAECount() - 1;
			else g_SelectedButton--;
			changed = true;
			break;
		case VK_RIGHT:
			if (g_SelectedButton >= g_ai.GetAECount() - 1 || g_SelectedButton == -1) g_SelectedButton = 0;
			else g_SelectedButton++;
			changed = true;
			break;
		case VK_RETURN:
			if (g_SelectedButton >= 0 && g_SelectedButton < g_ai.GetAECount()) {
				CallAfterFX(hwnd, g_SelectedButton);
				DestroyWindow(hwnd);
				//SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(ID_BUTTON_START + g_SelectedButton, BN_CLICKED), 0);
			}
			break;
		}
		if (changed) InvalidateRect(hwnd, NULL, TRUE);
		return 0;
	}

	case WM_COMMAND:
		if (LOWORD(wParam) >= ID_BUTTON_START && LOWORD(wParam) < ID_BUTTON_START + g_ai.GetAECount()) {
			g_SelectedButton = LOWORD(wParam) - ID_BUTTON_START;
			if (g_SelectedButton >= 0 && g_SelectedButton < g_ai.GetAECount()) {
				CallAfterFX(hwnd, g_SelectedButton);
				DestroyWindow(hwnd);
				//SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(ID_BUTTON_START + g_SelectedButton, BN_CLICKED), 0);
			}
			//InvalidateRect(hwnd, NULL, TRUE);
			//UpdateWindow(hwnd);
			//std::wstring msg = Utf8ToWide(g_ai.GetVersion(g_SelectedButton)) + L" モードを選択";
			//MessageBoxW(hwnd, msg.c_str(), L"Event", MB_OK | MB_TOPMOST);
		}
		return 0;

	case WM_DRAWITEM:
	{
		LPDRAWITEMSTRUCT pdis = (LPDRAWITEMSTRUCT)lParam;
		int idx = pdis->CtlID - ID_BUTTON_START;
		if (idx>=0 && idx < g_ai.GetAECount()) {
			DrawMyFlatButton(pdis, Utf8ToWide(g_ai.GetVersion(idx)), (idx == g_SelectedButton));
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
	case WM_SETCURSOR:
	{
		if (LOWORD(lParam) == HTCLIENT) {
			UpdateFocusByMousePos(hwnd);
		}
		// DefWindowProcW に流すと起動直後の IDC_APPSTARTING が復元されるため
		// すべての領域で IDC_ARROW を強制設定する
		SetCursor(LoadCursor(NULL, IDC_ARROW));
		return 1;
	}
	case WM_DROPFILES:
	{
		HDROP hDrop = (HDROP)wParam;
		POINT pt;
		bool finFlag = false;
		// 1. ドロップされた位置（ウィンドウ相対座標）を取得
		DragQueryPoint(hDrop, &pt);

		// 2. その座標にある子ウィンドウ（ボタン）のハンドルを取得
		HWND hChild = ChildWindowFromPoint(hwnd, pt);

		if (hChild != NULL) {
			// 3. ボタンのIDを取得して、どのボタンか判定
			int btnId = GetDlgCtrlID(hChild);

			if (btnId >= ID_BUTTON_START && btnId < ID_BUTTON_START + g_ai.GetAECount()) {
				int index = btnId - ID_BUTTON_START;
				g_SelectedButton = index;
				RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
				// ファイルパスの取得（最初の1つだけ例）
				UINT pathLen = DragQueryFileW(hDrop, 0, NULL, 0);
				std::wstring filePath(pathLen, L'\0');
				DragQueryFileW(hDrop, 0, &filePath[0], pathLen + 1);

				g_ai.SetTargetAepPath(filePath);
				// ボタンごとの個別処理
				//std::wstring msg = L"ボタン [" + std::to_wstring(index + 1) + L"] に\n" + filePath + L"\nをドロップしました。";
				//MessageBoxW(hwnd, msg.c_str(), L"Drop Event", MB_OK | MB_TOPMOST);
				if (g_SelectedButton >= 0 && g_SelectedButton < g_ai.GetAECount()) {
					CallAfterFX(hwnd, g_SelectedButton);
					finFlag = true;
					//SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(ID_BUTTON_START + g_SelectedButton, BN_CLICKED), 0);
				}
			}
		}

		DragFinish(hDrop);
		if (finFlag)
			DestroyWindow(hwnd);
		return 0;
	}
	case WM_CONTEXTMENU:
	{
		// 1. メニューハンドルの作成
		HMENU hMenu = CreatePopupMenu();

		// 2. 項目の追加 (IDは適宜定義してください)
		AppendMenuW(hMenu, MF_STRING, 101, L"RegisterExtension");
		AppendMenuW(hMenu, MF_STRING, 102, L"CreateDesktopShortcut開く");
		AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL); // 区切り線
		AppendMenuW(hMenu, MF_STRING, 103, L"Quit");

		// 3. 表示位置の取得 (wParamは右クリックされたウィンドウのハンドル)
		// LOWORD(lParam), HIWORD(lParam) にはスクリーン座標が入っています
		int x = LOWORD(lParam);
		int y = HIWORD(lParam);

		// 4. メニューを表示
		// TPM_RETURNCMD を付けると、選択されたIDが戻り値として返ってきます
		BOOL id = TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD, x, y, 0, hwnd, NULL);

		// 5. 選択された項目に応じた処理
		if (id == 101) {
			au::RegisterExtension();
		}
		if (id == 102) {
			au::CreateDesktopShortcut(L"AE_Launcher");
		}
		if (id == 103) PostQuitMessage(0);

		// 6. メニューの破棄（必須）
		DestroyMenu(hMenu);
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
	au au;
	if (au.FindOption("inst") >= 0) {
		au.RegisterExtension();
		MessageBox(NULL, L"拡張子割り当て", L"完了", MB_OK);
		return 0;
	}
	else if (au.FindOption("cs") >= 0) {
		au.CreateDesktopShortcut(L"AE_Launcher");
		MessageBox(NULL, L"デスクトップにショートカットを作成しました。", L"完了", MB_OK);
		return 0;
	}
	g_ai.SetTargetAepPath(au.FindAep());
	lastSelectedVersion = g_ai.LoadVersion("AE_Launcher");
	g_SelectedButton = g_ai.IndexOfVersion(lastSelectedVersion);
	const wchar_t CLASS_NAME[] = L"FlatToolApp";
	WNDCLASSW wc = { };
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInst;
	wc.lpszClassName = CLASS_NAME;
	wc.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON1));
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	//wc.hCursor = NULL;
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

	SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

	ShowWindow(hwnd, nCmdShow);
	// WM_NULL をキューに積んで GetMessage を即時 return させる。
	// これにより Windows のスタートアップカーソルフィードバック (IDC_APPSTARTING) が
	// ユーザーがウィンドウにマウスを入れる前に終了する。
	PostMessage(hwnd, WM_NULL, 0, 0);
	MSG msg = { };
	while (GetMessageW(&msg, NULL, 0, 0) > 0) {
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}
	return 0;
}