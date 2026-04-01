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

#define IDI_ICON1 101
namespace fs = std::filesystem;

struct AEVersion {
    std::string shortName;
    std::wstring path;
};

// --- グローバル変数 ---
static ID3D11Device* g_pd3dDevice = nullptr;
static ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
static IDXGISwapChain* g_pSwapChain = nullptr;
static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;
std::vector<AEVersion>          installedAEs;
std::wstring                    targetAepPath = L"";
std::string                     lastSelectedVersion = "";

const float BTN_SIZE = 60.0f;
const float WINDOW_PADDING = 10.0f;

// --- 関数宣言 ---
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

void RegisterExtension() {
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);
    std::wstring command = L"\"" + std::wstring(exePath) + L"\" \"%1\"";
    const wchar_t* progId = L"AEP_Launcher_File";

    auto SetRegKey = [](HKEY root, const wchar_t* subkey, const wchar_t* value, const wchar_t* valueName = NULL) {
        HKEY hKey;
        if (RegCreateKeyExW(root, subkey, 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
            RegSetValueExW(hKey, valueName, 0, REG_SZ, (BYTE*)value, (DWORD)(wcslen(value) + 1) * sizeof(wchar_t));
            RegCloseKey(hKey);
            return true;
        }
        return false;
        };

    // 1. プログラムIDの登録
    SetRegKey(HKEY_CURRENT_USER, L"Software\\Classes\\AEP_Launcher_File", L"After Effects Project (Launcher)");
    SetRegKey(HKEY_CURRENT_USER, L"Software\\Classes\\AEP_Launcher_File\\shell\\open\\command", command.c_str());
    SetRegKey(HKEY_CURRENT_USER, L"Software\\Classes\\AEP_Launcher_File\\DefaultIcon", (std::wstring(exePath) + L",0").c_str());

    // 2. 拡張子の紐付け
    SetRegKey(HKEY_CURRENT_USER, L"Software\\Classes\\.aep", progId);

    // 3. Windowsの「プログラムから開く」一覧に強制的に追加するための OpenWithList
    SetRegKey(HKEY_CURRENT_USER, L"Software\\Classes\\.aep\\OpenWithList\\AE_Launcher.exe", L"");

    // 4. アプリケーションとしての登録（これをしないと一覧に出にくい）
    std::wstring appRegPath = L"Software\\Classes\\Applications\\AE_Launcher.exe";
    SetRegKey(HKEY_CURRENT_USER, (appRegPath + L"\\shell\\open\\command").c_str(), command.c_str());

    // システムに通知
    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);

    MessageBoxW(NULL, L"レジストリ登録が完了しました。\n\nもし起動しない場合は、AEPファイルを右クリックして\n「プログラムから開く」→「別のプログラムを選択」から\nこのアプリを一度だけ選んでください。", L"完了", MB_OK);
}

// --- ショートカット作成 ---
void CreateDesktopShortcut() {
    CoInitialize(NULL);
    IShellLinkW* pShellLink = NULL;
    if (SUCCEEDED(CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLinkW, (LPVOID*)&pShellLink))) {
        wchar_t exePath[MAX_PATH];
        GetModuleFileNameW(NULL, exePath, MAX_PATH);
        pShellLink->SetPath(exePath);
        pShellLink->SetDescription(L"After Effects Launcher");

        IPersistFile* pPersistFile = NULL;
        if (SUCCEEDED(pShellLink->QueryInterface(IID_IPersistFile, (LPVOID*)&pPersistFile))) {
            wchar_t desktopPath[MAX_PATH];
            SHGetFolderPathW(NULL, CSIDL_DESKTOPDIRECTORY, NULL, 0, desktopPath);
            std::wstring shortcutPath = std::wstring(desktopPath) + L"\\AE Launcher.lnk";
            pPersistFile->Save(shortcutPath.c_str(), TRUE);
            pPersistFile->Release();
        }
        pShellLink->Release();
    }
    CoUninitialize();
    MessageBoxW(NULL, L"デスクトップにショートカットを作成しました。", L"完了", MB_OK);
}

fs::path GetAppDataPath() {
    wchar_t path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, path))) {
        fs::path dir = fs::path(path) / L"AE_Launcher";
        fs::create_directories(dir);
        return dir;
    }
    return L".";
}

void SaveConfig(std::string version) {
    std::ofstream ofs(GetAppDataPath() / L"config.ini");
    ofs << version;
}

void LoadConfig() {
    std::ifstream ifs(GetAppDataPath() / L"config.ini");
    if (ifs) { ifs >> lastSelectedVersion; }
}

void FindAndCleanAEList() {
    installedAEs.clear();
    fs::path adobePath(L"C:\\Program Files\\Adobe");
    std::error_code ec;
    std::regex versionRegex(R"((\d{4}|CS\d+))");

    if (fs::exists(adobePath, ec) && fs::is_directory(adobePath, ec)) {
        for (auto it = fs::directory_iterator(adobePath, ec); it != fs::end(it); it.increment(ec)) {
            if (ec) break;
            const auto& entry = *it;
            if (entry.is_directory(ec)) {
                std::wstring folderNameW = entry.path().filename().wstring();
                if (folderNameW.find(L"After Effects") != std::wstring::npos) {
                    fs::path exePath = entry.path() / L"Support Files" / L"AfterFX.exe";
                    if (fs::exists(exePath, ec)) {
                        auto u8Temp = entry.path().filename().u8string();
                        std::string folderName(reinterpret_cast<const char*>(u8Temp.c_str()));
                        std::smatch match;
                        std::string shortName = "AE";
                        if (std::regex_search(folderName, match, versionRegex)) { shortName = match.str(); }
                        installedAEs.push_back({ shortName, exePath.wstring() });
                    }
                }
            }
        }
    }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    int argc;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (argv && argc > 1) {
        std::wstring arg1 = argv[1];
        if (arg1 == L"-inst") { RegisterExtension(); return 0; }
        if (arg1 == L"-cs") { CreateDesktopShortcut(); return 0; }

        std::error_code ec;
        fs::path p(argv[1]);
        targetAepPath = fs::exists(p, ec) ? fs::absolute(p, ec).wstring() : argv[1];
    }
    if (argv) LocalFree(argv);

    ImGui_ImplWin32_EnableDpiAwareness();
    FindAndCleanAEList();
    LoadConfig();

    int windowW, windowH;
    if (installedAEs.empty()) { windowW = 450; windowH = 150; }
    else {
        int aeCount = (int)installedAEs.size();
        windowW = static_cast<int>((aeCount * BTN_SIZE) + ((aeCount - 1) * 8.0f) + (WINDOW_PADDING * 2));
        windowH = static_cast<int>(BTN_SIZE + (WINDOW_PADDING * 2));
    }

    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr),
                       LoadIcon(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDI_ICON1)),
                       LoadCursor(nullptr, IDC_ARROW), nullptr, nullptr, L"AELauncherClass", nullptr };
    wc.hIconSm = wc.hIcon;
    ::RegisterClassExW(&wc);

    DWORD winStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU;
    RECT wr = { 0, 0, windowW, windowH };
    AdjustWindowRect(&wr, winStyle, FALSE);
    HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"AE Launcher", winStyle,
        (GetSystemMetrics(SM_CXSCREEN) - (wr.right - wr.left)) / 2,
        (GetSystemMetrics(SM_CYSCREEN) - (wr.bottom - wr.top)) / 2,
        wr.right - wr.left, wr.bottom - wr.top, nullptr, nullptr, wc.hInstance, nullptr);

    if (!CreateDeviceD3D(hwnd)) return 1;

    ::ShowWindow(hwnd, nCmdShow);
    ::UpdateWindow(hwnd);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::GetIO().IniFilename = nullptr;
    ImGui::GetIO().Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\msgothic.ttc", 24.0f, nullptr, ImGui::GetIO().Fonts->GetGlyphRangesJapanese());

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

        if (installedAEs.empty()) { ImGui::Text(reinterpret_cast<const char*>(u8"After Effects が見つかりません。")); }
        else {
            if (ImGui::BeginTable("AETable", (int)installedAEs.size())) {
                for (int i = 0; i < (int)installedAEs.size(); i++) {
                    ImGui::TableNextColumn();
                    if (firstFrame) {
                        if ((lastSelectedVersion == "" && i == 0) || (installedAEs[i].shortName == lastSelectedVersion)) ImGui::SetKeyboardFocusHere();
                    }
                    if (ImGui::Button(installedAEs[i].shortName.c_str(), ImVec2(BTN_SIZE, BTN_SIZE))) {
                        SaveConfig(installedAEs[i].shortName);
                        std::wstring args = targetAepPath.empty() ? L"" : (L"\"" + targetAepPath + L"\"");
                        HINSTANCE res = ShellExecuteW(NULL, L"open", installedAEs[i].path.c_str(), args.c_str(), fs::path(installedAEs[i].path).parent_path().c_str(), SW_SHOWNORMAL);
                        if ((intptr_t)res <= 32) { MessageBoxW(hwnd, L"起動に失敗しました。", L"Error", MB_ICONERROR); }
                        else {
                            for (int r = 0; r < 20; r++) {
                                Sleep(150); HWND ae = FindWindowW(L"AE_FreeWindow", NULL);
                                if (ae) { SetForegroundWindow(ae); ShowWindow(ae, SW_RESTORE); break; }
                            }
                            done = true;
                        }
                    }
                }
                ImGui::EndTable();
            }
        }
        if (firstFrame) firstFrame = false;
        ImGui::End(); ImGui::Render();
        const float clear_color[4] = { 0.12f, 0.12f, 0.12f, 1.0f };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        g_pSwapChain->Present(1, 0);
    }
    ImGui_ImplDX11_Shutdown(); ImGui_ImplWin32_Shutdown(); ImGui::DestroyContext();
    CleanupDeviceD3D(); ::DestroyWindow(hwnd);
    return 0;
}

bool CreateDeviceD3D(HWND hWnd) {
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 2; sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1; sd.Windowed = TRUE; sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    D3D_FEATURE_LEVEL fl; const D3D_FEATURE_LEVEL flA[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
    if (S_OK != D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, flA, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &fl, &g_pd3dDeviceContext)) return false;
    CreateRenderTarget(); return true;
}
void CleanupDeviceD3D() { CleanupRenderTarget(); if (g_pSwapChain) g_pSwapChain->Release(); if (g_pd3dDeviceContext) g_pd3dDeviceContext->Release(); if (g_pd3dDevice) g_pd3dDevice->Release(); }
void CreateRenderTarget() { ID3D11Texture2D* pB; g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pB)); g_pd3dDevice->CreateRenderTargetView(pB, nullptr, &g_mainRenderTargetView); pB->Release(); }
void CleanupRenderTarget() { if (g_mainRenderTargetView) g_mainRenderTargetView->Release(); }
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) return true;
    switch (msg) { case WM_SYSCOMMAND: if ((wParam & 0xfff0) == SC_KEYMENU) return 0; break; case WM_DESTROY: ::PostQuitMessage(0); return 0; }
                                     return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}