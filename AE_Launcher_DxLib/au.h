#pragma once
#include <tchar.h> // __targv 用
#include <windows.h>
#include <shellapi.h> // CommandLineToArgvW 用
#include <string>
#include <vector>	
#include <algorithm>
#include <cwctype>
#include <shlobj.h>


class au
{
private:
	std::vector<std::wstring> m_CommandLineArgs;
public:
	/// <summary>
	/// 実行ファイルのパスを取得する関数
	/// </summary>
	/// <returns></returns>
	static std::wstring GetExecutablePath() {
		wchar_t path[MAX_PATH];
		GetModuleFileNameW(NULL, path, MAX_PATH);
		return std::wstring(path);
	}
	/// <summary>
	/// 実行ファイルのあるディレクトリを取得する関数
	/// </summary>
	/// <returns></returns>
	static std::wstring GetExecutableDirectory() {
		std::wstring exePath = GetExecutablePath();
		size_t pos = exePath.find_last_of(L"\\/");
		if (pos != std::wstring::npos) {
			return exePath.substr(0, pos);
		}
		return L"";
	}
	/// <summary>
	/// コマンドライン引数を取得する関数
	/// </summary>
	/// <returns></returns>
	static std::vector<std::wstring> GetCommandLineArgs() {
		int argc;
		wchar_t** argv = CommandLineToArgvW(GetCommandLineW(), &argc);
		std::vector<std::wstring> args;
		for (int i = 0; i < argc; ++i) {
			args.push_back(argv[i]);
		}
		LocalFree(argv);
		return args;
	}
	/// <summary>
	/// ワイド文字列をUTF-8エンコードされた文字列に変換します。
	/// </summary>
	/// <param name="wstr">変換するワイド文字列。</param>
	/// <returns>変換されたUTF-8エンコードされた文字列。入力文字列が空の場合は空の文字列を返します。</returns>
	static std::string WStringToString(const std::wstring& wstr) {
		if (wstr.empty()) return std::string();

		// 必要なバッファサイズを計算
		int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);

		std::string strTo(size_needed, 0);
		// 実際の変換
		WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
		return strTo;
	}
	/// <summary>
	/// UTF-8エンコードされた文字列をワイド文字列に変換します。
	/// </summary>
	/// <param name="str">変換する文字列。</param>
	/// <returns>変換されたワイド文字列。入力文字列が空の場合は空のワイド文字列を返します。</returns>
	static std::wstring StringToWString(const std::string& str) {
		if (str.empty()) return std::wstring();

		// 必要なバッファサイズ（文字数）を計算
		// CP_UTF8 を指定することで UTF-8 文字列として解釈します
		int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);

		std::wstring wstrTo(size_needed, 0);
		// 実際の変換実行
		MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);

		return wstrTo;
	}
	au()
	{
		m_CommandLineArgs = GetCommandLineArgs();
	}
	int IndexOfCmd(const std::wstring& arg) const {
		for (int i = 0; i < (int)m_CommandLineArgs.size(); ++i) {
			// _wcsicmp で大文字小文字を無視して比較
			if (_wcsicmp(m_CommandLineArgs[i].c_str(), arg.c_str()) == 0) {
				return i; // 見つかった要素のインデックスを返す
			}
		}
		return -1; // 最後まで見つからなかった場合は -1 を返す
	}
	int IndexOfCmd(const std::string& argUtf8) const {
		if (argUtf8.empty()) return -1;
		// 入力の UTF-8 文字列を比較用に wstring へ変換
		std::wstring wArg = StringToWString(argUtf8);
		return IndexOfCmd(wArg);
	}
	int FindOption(const std::wstring& arg) const {
		for (int i = 0; i < (int)m_CommandLineArgs.size(); ++i) {
			// _wcsicmp で大文字小文字を無視して比較
			if (m_CommandLineArgs[i][0] == L'-' || m_CommandLineArgs[i][0] == L'/') {
				std::wstring optionName = m_CommandLineArgs[i].substr(1); // 最初の文字を除いた部分をオプション名とする
				if (_wcsicmp(optionName.c_str(), arg.c_str()) == 0) {
					return i; // 見つかった要素のインデックスを返す
				}
			}
		}
		return -1; // 最後まで見つからなかった場合は -1 を返す
	}
	/// <summary>
	/// UTF-8 エンコードされた文字列からオプションを検索します。	
	/// </summary>
	/// <param name="argUtf8">検索するオプションを表す UTF-8 エンコードされた文字列。</param>
	/// <returns>オプションが見つかった場合はそのインデックス、文字列が空の場合またはオプションが見つからない場合は -1。</returns>
	int FindOption(const std::string& argUtf8) const
	{
		if (argUtf8.empty()) return -1;
		// 入力の UTF-8 文字列を比較用に wstring へ変換
		std::wstring wArg = StringToWString(argUtf8);
		return FindOption(wArg);
	}
	/// <summary>
	/// コマンドライン引数から.aep拡張子を持つ存在するファイルのパスを検索します。
	/// </summary>
	/// <returns>見つかった.aepファイルのパスを含むベクター。コマンドライン引数が空の場合、または.aepファイルが見つからない場合は空のベクターを返します。</returns>
	std::vector<std::wstring> FindAepPath() const {
		std::vector<std::wstring> files;
		if (m_CommandLineArgs.empty()) return files;

		for(int i=0; i < (int)m_CommandLineArgs.size(); ++i) {
				std::wstring fName = m_CommandLineArgs[i];
				fs::path p(fName);
				if (fs::exists(p) && p.extension() == L".aep") {
					files.push_back(fName);
				}
		}
		return files;
	}
	/// <summary>
	/// AEPファイルを検索します。	
	/// </summary>
	/// <returns>最初に見つかったAEPファイルのパス。ファイルが見つからない場合は空文字列。</returns>
	std::wstring FindAep() const {
		auto files = FindAepPath();
		return files.empty() ? L"" : files[0];
	}
	// ***************************************************************************************
	/// <summary>
	/// .aepファイルをこのアプリに関連付けるレジストリを登録します。
	/// </summary>
	static void RegisterExtension() {
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
	// ***************************************************************************************
	// --- ショートカット作成 ---
	/// デスクトップにショートカットを作成する関数
	static void CreateDesktopShortcut(std::wstring shortcutName = L"AE_Launcher") {
		CoInitialize(NULL);
		IShellLinkW* pShellLink = NULL;
		if (SUCCEEDED(CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLinkW, (LPVOID*)&pShellLink))) {
			wchar_t exePath[MAX_PATH];
			GetModuleFileNameW(NULL, exePath, MAX_PATH);
			pShellLink->SetPath(exePath);
			pShellLink->SetDescription(shortcutName.c_str());

			IPersistFile* pPersistFile = NULL;
			if (SUCCEEDED(pShellLink->QueryInterface(IID_IPersistFile, (LPVOID*)&pPersistFile))) {
				wchar_t desktopPath[MAX_PATH];
				SHGetFolderPathW(NULL, CSIDL_DESKTOPDIRECTORY, NULL, 0, desktopPath);
				std::wstring shortcutPath = std::wstring(desktopPath) + shortcutName + std::wstring(L".LNK");
				pPersistFile->Save(shortcutPath.c_str(), TRUE);
				pPersistFile->Release();
			}
			pShellLink->Release();
		}
		CoUninitialize();
		MessageBoxW(NULL, L"デスクトップにショートカットを作成しました。", L"完了", MB_OK);
	}

	// ***************************************************************************************
	/// <summary>
	/// AppData フォルダ内にアプリ専用のディレクトリを作成し、そのパスを返す関数
	/// </summary>
	/// <param name="appName">アプリケーション名</param>
	/// <returns>作成されたディレクトリのパス</returns>
	static fs::path GetAppDataPath(std::wstring appName) {
		wchar_t path[MAX_PATH];
		if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, path))) {
			fs::path dir = fs::path(path) / appName;
			fs::create_directories(dir);
			return dir;
		}
		return L".";
	}

	
};