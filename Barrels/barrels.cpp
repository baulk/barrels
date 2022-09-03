///
#include "pch.h"
#include "barrels.hpp"
#include "escape_argv.hpp"
#include "vsinstance.hpp"
#include <shellapi.h>
#include <ShlObj.h>
#include <filesystem>
#include <wil/resource.h>

constexpr COMDLG_FILTERSPEC filters[] = {
    // archives
    {L"Baulk Executor", L"baulk-exec.exe"},
    {L"All Files (*.*)", L"*.*"}
    //
};

std::optional<std::wstring> barrels::baulk_exec_picker(HWND hWnd) {
  wil::com_ptr<IFileOpenDialog> window;
  if (::CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_IFileOpenDialog, (void **)&window) !=
      S_OK) {
    return std::nullopt;
  }
  if (!window) {
    return std::nullopt;
  }
  if (window->SetFileTypes(2, filters) != S_OK) {
    return std::nullopt;
  }
  if (window->SetTitle(L"Seach Baulk Extended Executor") != S_OK) {
    return std::nullopt;
  }
  if (window->Show(hWnd) != S_OK) {
    return std::nullopt;
  }
  wil::com_ptr<IShellItem> item;
  if (window->GetResult(&item) != S_OK) {
    return std::nullopt;
  }
  LPWSTR pszFilePath = nullptr;
  if (item->GetDisplayName(SIGDN_DESKTOPABSOLUTEPARSING, &pszFilePath) != S_OK) {
    return std::nullopt;
  }
  auto opt = std::make_optional<std::wstring>(pszFilePath);
  CoTaskMemFree(pszFilePath);
  return opt;
}

std::wstring ExpandEnv(std::wstring_view sv) {
  auto pos = sv.find('%');
  if (pos == std::wstring_view::npos) {
    return std::wstring(sv);
  }
  // NO check
  if (sv.find(L'%', pos + 1) == std::wstring_view::npos) {
    return std::wstring(sv);
  }
  std::wstring buf;
  buf.resize(sv.size() + 256);
  auto N = ExpandEnvironmentStringsW(sv.data(), buf.data(), static_cast<DWORD>(buf.size()));
  if (static_cast<size_t>(N) > buf.size()) {
    buf.resize(N);
    N = ExpandEnvironmentStringsW(sv.data(), buf.data(), static_cast<DWORD>(buf.size()));
  }
  if (N == 0 || static_cast<size_t>(N) > buf.size()) {
    return L"";
  }
  buf.resize(N - 1);
  return buf;
}

static constexpr wchar_t RegKeyBaulk[] = L"Software\\Baulk";
static constexpr wchar_t RegKeyInstallPath[] = L"InstallPath";

auto search_baulk_install(HKEY hRootKey) -> std::optional<std::wstring> {
  wil::unique_hkey bk;
  wchar_t buffer[4096];
  DWORD dwSize = sizeof(buffer);
  DWORD type = 0;
  if (RegOpenKeyExW(hRootKey, RegKeyBaulk, 0, KEY_READ, &bk) != ERROR_SUCCESS) {
    return std::nullopt;
  }
  if (RegQueryValueExW(bk.get(), RegKeyInstallPath, nullptr, &type, reinterpret_cast<LPBYTE>(buffer), &dwSize) !=
          ERROR_SUCCESS ||
      type != REG_SZ) {
    return std::nullopt;
  }
  auto path = std::format(L"{}\\bin\\baulk-exec.exe", buffer);
  std::error_code e;
  if (!std::filesystem::exists(path, e)) {
    return std::nullopt;
  }
  return std::make_optional(std::move(path));
};

bool barrels::search_vs_instances(vs_instances_t &vsInstances) {
  barrels::Searcher s;
  return s.Initialize() && s.Search(vsInstances);
}

bool barrels::search_baulk_lists(std::vector<std::wstring> &baulks) {
  if (auto p = search_baulk_install(HKEY_CURRENT_USER); p) {
    baulks.emplace_back(std::move(*p));
  }
  if (auto p = search_baulk_install(HKEY_LOCAL_MACHINE); p) {
    baulks.emplace_back(std::move(*p));
  }
  std::error_code e;
  // store app
  if (auto p = ExpandEnv(LR"(%LOCALAPPDATA%\Microsoft\WindowsApps\baulk-exec.exe)"); std::filesystem::exists(p, e)) {
    baulks.emplace_back(std::move(p));
  }
  if (auto p = ExpandEnv(L"%SystemDrive%\\Baulk\\bin\\baulk-exec.exe"); std::filesystem::exists(p, e)) {
    baulks.emplace_back(std::move(p));
  }
  return !baulks.empty();
}

inline std::optional<std::wstring> FindWindowsTerminal() {
  auto wt = ExpandEnv(L"%LOCALAPPDATA%\\Microsoft\\WindowsApps\\wt.exe");
  if (auto a = GetFileAttributesW(wt.data()); a == INVALID_FILE_ATTRIBUTES) {
    return std::nullopt;
  }
  return std::make_optional(std::move(wt));
}

bool barrels::execute(const std::wstring_view baulkExec, const std::wstring_view vsInstanceId,
                      const std::wstring_view arch, const std::wstring_view venv, bool makeCleanEnv) {
  bela::EscapeArgv ea;
  if (auto wt = FindWindowsTerminal(); wt) {
    ea.Assign(*wt).Append(L"--title").Append(L"Windows Terminal \U0001F496 Baulk").Append(L"--");
  }
  ea.Append(baulkExec);
  auto cwd = ExpandEnv(L"%USERPROFILE%");
  ea.Append(L"--cwd").Append(cwd);
  if (!vsInstanceId.empty()) {
    ea.Append(L"--vs-instance").Append(vsInstanceId).Append(L"-A").Append(arch);
  }
  if (!venv.empty()) {
    ea.Append(L"-E").Append(venv);
  }
  if (makeCleanEnv) {
    ea.Append(L"--cleanup");
  }
  ea.Append(L"winsh");
  STARTUPINFOW si;
  SecureZeroMemory(&si, sizeof(si));
  si.cb = sizeof(si);
  PROCESS_INFORMATION pi;
  SecureZeroMemory(&pi, sizeof(pi));
  if (CreateProcessW(nullptr, ea.data(), nullptr, nullptr, FALSE, CREATE_UNICODE_ENVIRONMENT, nullptr, nullptr, &si,
                     &pi) != TRUE) {
    return false;
  }
  CloseHandle(pi.hThread);
  CloseHandle(pi.hProcess);
  return true;
}
