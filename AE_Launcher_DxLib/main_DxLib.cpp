#include <dwmapi.h> // Windows APIのDWM制御用（これを追加）
#include <string>
#include "DxLib.h"
#include "resource.h"
#include "DxKM.hpp"
#include "AfterFXInfo.h"
#include "au.h"

#pragma comment(lib, "dwmapi.lib")
#define ICON_COUNT_MAX 6
#define ICON_SIZE 64

static int selectedIndex = 0;
static int icon1 = 0;
static int icon2 = 0;
static int FontHandle = 0;
static bool g_IsExitRequested = false;


DxKM g_KM; // DxKMクラスのインスタンスをグローバルに作成   
AfterFXInfo g_ai; // AfterFXInfoクラスのインスタンスをグローバルに作成

LRESULT CALLBACK MyWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
 
	switch (msg) {
	case WM_CLOSE:
		// ×ボタンが押されたら、強制終了させずにフラグだけ立てる
		g_IsExitRequested = true;
		return 0; // 0を返すとWindowsのデフォルト終了処理を阻止できる
	}
	return 0;
}
void DrawIconGrid() {
	for (int i = 0; i < g_ai.GetAECount(); i++) {
		int x = i * ICON_SIZE;
		int y = 0;
		if (i == selectedIndex) {
			// 選択されているアイコンの描画
			DrawGraph(x, y, icon2, TRUE);
		}
		else {
			// 通常のアイコンの描画
			DrawGraph(x, y, icon1, TRUE);
		}
		std::string label = g_ai.GetVersion(i);
		int ww = GetDrawStringWidthToHandle(label.c_str(), (int)label.length(), FontHandle);
		ww = x + (ICON_SIZE - ww) / 2; // アイコンの中央に配置するためのオフセット
		uint32_t color = (i == selectedIndex) ? GetColor(230, 230, 255) : GetColor(160, 160, 220);
		DrawStringToHandle(ww, 24, label.c_str(), color, FontHandle);
	}
}
bool CallAfterFX() {
	if(selectedIndex>=0 && selectedIndex < g_ai.GetAECount()) {
		HWND hWnd = GetMainWindowHandle(); // ＤＸライブラリのウィンドウハンドルを取得
		SetWindowPos(hWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
		SetWindowVisibleFlag(FALSE);
		return g_ai.CallAfterFX(selectedIndex);
	}
	return false;
}
int WINAPI WinMain(
	_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance, 
	_In_ LPSTR lpCmdLine, 
	_In_ int nCmdShow) {
	if (!g_ai.IsInstalled()) {
		MessageBox(NULL, "After Effects が見つかりませんでした。", "Error", MB_OK);
		return -1;
	}
	au au;
	if (au.FindOption("inst")>=0) {
		au.RegisterExtension();
		MessageBox(NULL, "拡張子割り当て(嘘)", "完了", MB_OK);
		return 0;
	}else if (au.FindOption("cs")>=0) {
		au.CreateDesktopShortcut(L"AE_Launcher");
		MessageBox(NULL, "デスクトップにショートカットを作成しました。(嘘)", "完了", MB_OK);
		return 0;
	}
	g_ai.SetTargetAepPath(au.FindAep());

	// --- 1. 初期化前の設定 ---
	ChangeWindowMode(TRUE);
	g_KM.SetWinSize(g_ai.GetAECount() * ICON_SIZE, ICON_SIZE);
	SetGraphMode(g_KM.winWidth(), g_KM.winHeight(), 32);
	SetWindowStyleMode(5);
	SetAlwaysRunFlag(TRUE); // バックグラウンドでも動くようにする
	SetOutApplicationLogValidFlag(FALSE);
	SetOutApplicationSystemLogValidFlag(FALSE);
	SetWindowTextDX("AE Launcher DxLib");
	SetFontSize(1);
	SetWindowVisibleFlag(FALSE);
	// --- 2. ＤＸライブラリ初期化 ---
	if (DxLib_Init() == -1) {
		MessageBox(NULL, "ＤＸライブラリ初期化初期化エラー", "Error", MB_OK);
		return -1;
	}
	SetHookWinProc(MyWndProc);
	// --- 3. 初期化後にリソースを読み込む ---
	icon1 = LoadGraphToResource(MAKEINTRESOURCE(IDB_PNG_ICON1), RT_RCDATA);
	icon2 = LoadGraphToResource(MAKEINTRESOURCE(IDB_PNG_ICON2), RT_RCDATA);

	// 読み込み成否チェック
	if (icon1 == -1 || icon2 == -1) {
		MessageBox(NULL, "画像の読み込みに失敗しました。パスを確認してください。", "Error", MB_OK);
		DxLib_End();
		return -1;
	}
	FontHandle = CreateFontToHandle(NULL, 18, 9, DX_FONTTYPE_ANTIALIASING);

	// --- 2. ウィンドウアニメーション（拡大）を無効化する処理 ---
	HWND hWnd = GetMainWindowHandle(); // ＤＸライブラリのウィンドウハンドルを取得
	BOOL disableAnimation = TRUE;
	DwmSetWindowAttribute(hWnd, DWMWA_TRANSITIONS_FORCEDISABLED, &disableAnimation, sizeof(disableAnimation));

	SetDrawScreen(DX_SCREEN_BACK);
	ClearDrawScreen();
	DrawIconGrid();
	ScreenFlip();
	SetWindowVisibleFlag(TRUE);
	SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

	g_KM.StartWait(); // 起動時のマウスクリック待ち
	// --- 4. メインループ ---
	while (ProcessMessage() == 0 && CheckHitKey(KEY_INPUT_ESCAPE) == 0 && !g_IsExitRequested) {
		g_KM.Update();
		if (g_KM.IsMouseMoved()) {
			int xx = g_KM.MoouseX() / ICON_SIZE;
			selectedIndex = xx;
		}
		else if (g_KM.IsLeftMouseClicked()) {
			if (CallAfterFX()) {
				break; // 左クリックで終了
			}
		}
		else if (g_KM.IsKeyPressed(KEY_INPUT_RETURN)) {
			break;
		}
		else if (g_KM.IsKeyPressed(KEY_INPUT_LEFT)) {
			selectedIndex = (selectedIndex - 1 + ICON_COUNT_MAX) % ICON_COUNT_MAX;
		}
		else if (g_KM.IsKeyPressed(KEY_INPUT_RIGHT)) {
			selectedIndex = (selectedIndex + 1) % ICON_COUNT_MAX;
		}
		ClearDrawScreen();
		DrawIconGrid();
		ScreenFlip();
	}
	DeleteFontToHandle(FontHandle);
	DxLib_End();
	return 0;
}