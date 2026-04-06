#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>
#include <objbase.h>
#include <vector>
#include <string>
#include <filesystem>
#include <regex>
#include <fstream>
#include "..\AE_Launcher_DxLib\AfterFXInfo.h"
#include "..\AE_Launcher_DxLib\au.h"

#define IDI_ICON1 101
namespace fs = std::filesystem;


// --- グローバル変数 ---
static ID3D11Device* g_pd3dDevice = nullptr;
static ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
static IDXGISwapChain* g_pSwapChain = nullptr;
static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;
HWND MainWindowHandle = nullptr;

std::string                     lastSelectedVersion = "";

const float BTN_SIZE = 60.0f;
const float WINDOW_PADDING = 10.0f;
const float ITEM_SPACING = 8.0f; // 追加
AfterFXInfo g_ai; // AfterFXInfoクラスのインスタンスをグローバルに作成

// --- 関数宣言 ---
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);


// *****************************************************************************************************
bool CallAfterFX(int idx) {
	if (idx >= 0 && idx < g_ai.GetAECount()) {
		SetWindowPos(MainWindowHandle, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
		ShowWindow(MainWindowHandle, SW_HIDE);
		if (g_ai.CallAfterFX(idx) == false)
		{
			ShowWindow(MainWindowHandle, SW_SHOW);
			SetWindowPos(MainWindowHandle, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
			MessageBox(NULL, L"After Effects の起動に失敗しました。", L"Error", MB_OK);
			return false;
		}
		g_ai.SaveVersion("AE_Launcher", g_ai.GetVersion(idx));
		return true;
	}
	return false;
}
// *****************************************************************************************************
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	HRESULT hrCom = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
	if (FAILED(hrCom)) {
		MessageBox(NULL, L"COM の初期化に失敗しました。", L"Error", MB_OK);
		return -1;
	}

	if (!g_ai.IsInstalled()) {
		MessageBox(NULL, L"After Effects が見つかりませんでした。", L"Error", MB_OK);
		return -1;
	}
	au au;
	if (au.FindOption("inst") >= 0) {
		au.RegisterExtension();
		MessageBox(NULL, L"拡張子割り当て(嘘)", L"完了", MB_OK);
		return 0;
	}
	else if (au.FindOption("cs") >= 0) {
		au.CreateDesktopShortcut(L"AE_Launcher");
		MessageBox(NULL, L"デスクトップにショートカットを作成しました。(嘘)", L"完了", MB_OK);
		return 0;
	}
	g_ai.SetTargetAepPath(au.FindAep());

	ImGui_ImplWin32_EnableDpiAwareness();
	lastSelectedVersion = g_ai.LoadVersion("AE_Launcher");

	int windowW, windowH;
	int aeCount = g_ai.GetAECount();
	windowW = static_cast<int>((aeCount * BTN_SIZE) + ((aeCount - 1) * ITEM_SPACING) + (WINDOW_PADDING * 2));
	windowH = static_cast<int>(BTN_SIZE + (WINDOW_PADDING * 2));

	WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr),
					   LoadIcon(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDI_ICON1)),
					   LoadCursor(nullptr, IDC_ARROW), nullptr, nullptr, L"AELauncherClass", nullptr };
	wc.hIconSm = wc.hIcon;
	::RegisterClassExW(&wc);

	DWORD winStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU;
	RECT wr = { 0, 0, windowW, windowH };
	AdjustWindowRect(&wr, winStyle, FALSE);
	HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"AE Launcher Imgui", winStyle,
		(GetSystemMetrics(SM_CXSCREEN) - (wr.right - wr.left)) / 2,
		(GetSystemMetrics(SM_CYSCREEN) - (wr.bottom - wr.top)) / 2,
		wr.right - wr.left, wr.bottom - wr.top, nullptr, nullptr, wc.hInstance, nullptr);

	if (!CreateDeviceD3D(hwnd)) return 1;

	::ShowWindow(hwnd, nCmdShow);
	::UpdateWindow(hwnd);

	SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
	MainWindowHandle = hwnd;

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	ImGui::GetIO().IniFilename = nullptr;
	
	ImFont* font = ImGui::GetIO().Fonts->AddFontFromFileTTF(
		"C:\\Windows\\Fonts\\msgothic.ttc", 24.0f, nullptr,
		ImGui::GetIO().Fonts->GetGlyphRangesJapanese());
	if (!font) {
		ImGui::GetIO().Fonts->AddFontDefault(); // フォールバック
	}


	ImGui::StyleColorsDark();
	ImGui::GetStyle().WindowPadding = ImVec2(WINDOW_PADDING, WINDOW_PADDING);
	ImGui::GetStyle().ItemSpacing = ImVec2(8.0f, 0);
	ImGui::GetStyle().WindowRounding = 0.0f;
	ImGui::GetStyle().Colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);

	ImGui_ImplWin32_Init(hwnd);
	ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

	bool done = false;
	bool firstFrame = true;
	while (!done) {
		MSG msg;
		while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
			::TranslateMessage(&msg); ::DispatchMessage(&msg);
			if (msg.message == WM_QUIT) done = true;
		}
		if (done) break;

		ImGui_ImplDX11_NewFrame(); ImGui_ImplWin32_NewFrame(); ImGui::NewFrame();
		if (firstFrame) ImGui::SetNavCursorVisible(true);

		ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
		ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
		ImGui::Begin("Launcher", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground);

		if (ImGui::BeginTable("AETable", g_ai.GetAECount())) {
			for (int i = 0; i < g_ai.GetAECount(); i++) {
				ImGui::TableNextColumn();
				if (firstFrame) {
					if ((lastSelectedVersion == "" && i == 0) || (g_ai.GetVersion(i) == lastSelectedVersion)) ImGui::SetKeyboardFocusHere();
				}
				if (ImGui::Button(g_ai.GetVersion(i).c_str(), ImVec2(BTN_SIZE, BTN_SIZE))) {
					CallAfterFX(i);
					done = true;
					break;
				}
			}
			ImGui::EndTable();
		}
		if (firstFrame) firstFrame = false;
		ImGui::End(); ImGui::Render();
		const float clear_color[4] = { 0.12f, 0.12f, 0.12f, 1.0f };
		g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
		g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color);
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
		g_pSwapChain->Present(1, 0);
	}
	ImGui_ImplDX11_Shutdown(); 
	ImGui_ImplWin32_Shutdown(); 
	ImGui::DestroyContext();
	CleanupDeviceD3D(); ::DestroyWindow(hwnd);
	CoUninitialize(); // ★ 通常終了時の終了処理を追加
	return 0;
}
// *****************************************************************************************************

bool CreateDeviceD3D(HWND hWnd) {
	DXGI_SWAP_CHAIN_DESC sd = {};
	sd.BufferCount = 2; sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; sd.OutputWindow = hWnd;
	sd.SampleDesc.Count = 1; sd.Windowed = TRUE; sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	D3D_FEATURE_LEVEL fl; const D3D_FEATURE_LEVEL flA[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
	if (S_OK != D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, flA, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &fl, &g_pd3dDeviceContext)) return false;
	CreateRenderTarget(); return true;
}
// *****************************************************************************************************
void CleanupDeviceD3D() {
	CleanupRenderTarget();
	if (g_pSwapChain) g_pSwapChain->Release();
	if (g_pd3dDeviceContext) g_pd3dDeviceContext->Release();
	if (g_pd3dDevice) g_pd3dDevice->Release();
}
// *****************************************************************************************************
void CreateRenderTarget() {
	ID3D11Texture2D* pB; 
	g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pB)); 
	g_pd3dDevice->CreateRenderTargetView(pB, nullptr, &g_mainRenderTargetView); 
	
	pB->Release(); 
}
// *****************************************************************************************************
void CleanupRenderTarget() {
	if (g_mainRenderTargetView) 
		g_mainRenderTargetView->Release(); 
}
// *****************************************************************************************************
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) return true;
	switch (msg) { case WM_SYSCOMMAND: if ((wParam & 0xfff0) == SC_KEYMENU) return 0; break; case WM_DESTROY: ::PostQuitMessage(0); return 0; }
									 return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}