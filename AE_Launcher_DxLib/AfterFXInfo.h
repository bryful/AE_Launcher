#pragma once
#include <windows.h>
#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;
class AERenderInfo
{
private:
	std::wstring m_InstalledPath = L"";
	std::wstring m_AfterFXPath = L"";
	std::wstring m_Version = L"";
public:
	std::wstring GetInstalledPath() const { return m_InstalledPath; }
	std::wstring GetAfterFXPath() const { return m_AfterFXPath; }
	std::wstring GetVersion() const { return m_Version; }
	AERenderInfo(const std::wstring& installPath)
		: m_InstalledPath(installPath)
	{
		if (!installPath.empty()) {
			fs::path ip = installPath;
			fs::path p2 = "AfterFX.exe";
			m_InstalledPath = ip.wstring();
			m_AfterFXPath = (ip / p2).wstring();
			fs::path pp = ip.parent_path().parent_path();
			std::wstring version = pp.filename().wstring();
			std::wstring prefix = L"Adobe After Effects ";
			if (version.find(prefix) == 0) {
				version = version.substr(prefix.size());
			}
			m_Version = version;
		}
	};
	std::wstring ToString() const {
		return L"Version: " + m_Version + L", Install Path: " + m_InstalledPath + L", AfterFX Path: " + m_AfterFXPath;
	}

};


class AfterFXInfo
{
	private:
		std::vector<AERenderInfo> installedAEs;
		std::wstring m_targetAepPath = L"";
		std::string WStringToString(const std::wstring& wstr) const {
			if (wstr.empty()) return std::string();

			// 必要なバッファサイズを計算
			int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);

			std::string strTo(size_needed, 0);
			// 実際の変換
			WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
			return strTo;
		}
		std::wstring StringToWString(const std::string& str) const {
			if (str.empty()) return std::wstring();

			// 必要なバッファサイズ（文字数）を計算
			// CP_UTF8 を指定することで UTF-8 文字列として解釈します
			int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);

			std::wstring wstrTo(size_needed, 0);
			// 実際の変換実行
			MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);

			return wstrTo;
		}
	public:
		AfterFXInfo()
		{
			GetAllAEInstallations();
		};
		bool TargetAepEmpty() const { return m_targetAepPath.empty(); }
		std::wstring GetTargetAepPath() const { return m_targetAepPath; }
		void SetTargetAepPath(const std::string& path) { m_targetAepPath = StringToWString(path); }
		void SetTargetAepPath(const std::wstring& path) { m_targetAepPath = path; }
		bool IsInstalled() const { return !installedAEs.empty(); }
		int GetAECount() const { return (int)installedAEs.size(); }
		std::vector<AERenderInfo> GetInstalledAEs() const { return installedAEs; }
		std::string GetVersion(int idx) const { 
			if (idx >= 0 && idx < installedAEs.size()) {
				return WStringToString(installedAEs[idx].GetVersion());
			}
			return ""; 
		}
		bool GetAllAEInstallations() 
		{
			bool ret = false;
			installedAEs.clear();
			HKEY hParentKey;
			std::wstring parentPath = L"SOFTWARE\\Adobe\\After Effects";

			try {
				// 親キーをオープン (64bitレジストリを明示)
				if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, parentPath.c_str(), 0, KEY_READ | KEY_WOW64_64KEY, &hParentKey) == ERROR_SUCCESS) {
					wchar_t subKeyName[256];
					DWORD subKeyNameSize;
					DWORD index = 0;

					// サブキー（"24.0" など）を順番に列挙
					while (true) {
						subKeyNameSize = sizeof(subKeyName) / sizeof(wchar_t);
						if (RegEnumKeyExW(hParentKey, index, subKeyName, &subKeyNameSize, NULL, NULL, NULL, NULL) != ERROR_SUCCESS) {
							break; // これ以上キーがない場合は終了
						}

						// 各サブキーを開いてパスを取得
						HKEY hSubKey;
						if (RegOpenKeyExW(hParentKey, subKeyName, 0, KEY_READ | KEY_WOW64_64KEY, &hSubKey) == ERROR_SUCCESS) {
							wchar_t pathBuffer[MAX_PATH];
							DWORD pathBufferSize = sizeof(pathBuffer);

							if (RegQueryValueExW(hSubKey, L"InstallPath", NULL, NULL, (LPBYTE)pathBuffer, &pathBufferSize) == ERROR_SUCCESS) {
								installedAEs.push_back( AERenderInfo( pathBuffer));
							}
							RegCloseKey(hSubKey);
						}
						index++;
					}
					RegCloseKey(hParentKey);
					if (!installedAEs.empty()) {
						ret = true; // 少なくとも1つのインストールが見つかった
					}
				}
			}
			catch (const std::exception& e) {
				std::cerr << "Error accessing registry: " << e.what() << std::endl;
				ret = false;
				installedAEs.clear();
			}
			return ret;
		}
		
		
		fs::path GetAppDataPath(std::string appName = "") {
			wchar_t path[MAX_PATH];
			if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, path))) {
				fs::path dir = fs::path(path) / appName;
				fs::create_directories(dir);
				return dir;
			}
			return L".";
		}

		void SaveVersion(std::string appName,std::string version) {
			std::string fn = appName + ".ini";
			std::ofstream ofs(GetAppDataPath(appName) / fn);
			ofs << version;
		}

		std::string LoadVersion(std::string appName) {
			std::string ret = "";
			std::string fn = appName + ".ini";
			std::ifstream ifs(GetAppDataPath(appName) / fn);
			if (ifs) {
				ifs >> ret;
			}
			return ret;
		}
		/// <summary>
		/// 指定されたインデックスのAfter Effectsを起動します。
		/// </summary>
		/// <param name="idx">起動するAfter Effectsのインストール情報のインデックス。</param>
		/// <returns>After Effectsの起動に成功した場合はtrue、失敗した場合またはインデックスが無効な場合はfalse。</returns>
		bool CallAfterFX(int idx) const {
			if (idx >= 0 && idx < installedAEs.size()) {
				std::wstring afterFXPath = installedAEs[idx].GetAfterFXPath();
				std::wstring cmdLine = L"\"" + afterFXPath + L"\"";
				if (!m_targetAepPath.empty()) {
					cmdLine += L" \"" + m_targetAepPath + L"\"";
				}

				STARTUPINFOW si = { sizeof(STARTUPINFOW) };
				si.dwFlags = STARTF_USESHOWWINDOW;
				si.wShowWindow = SW_SHOW;

				PROCESS_INFORMATION pi = { 0 };

				std::vector<wchar_t> cmdLineBuffer(cmdLine.begin(), cmdLine.end());
				cmdLineBuffer.push_back(L'\0');

				// GUIアプリケーションなのでCREATE_NEW_CONSOLEは不要
				if (CreateProcessW(
					NULL,
					cmdLineBuffer.data(),
					NULL,
					NULL,
					FALSE,
					0,  // フラグを0に変更（コンソールウィンドウを作らない）
					NULL,
					fs::path(afterFXPath).parent_path().c_str(),
					&si,
					&pi)) {

					// After Effectsが起動するまで待機（GUIアプリなので少し長めに）
					WaitForInputIdle(pi.hProcess, 5000);
					
					// プロセスのウィンドウを最前面に
					AllowSetForegroundWindow(pi.dwProcessId);
					
					CloseHandle(pi.hProcess);
					CloseHandle(pi.hThread);
					return true;
				}
			}
			return false;
		}
};

