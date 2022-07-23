#include "pch.h"
#include "MainWindow.xaml.h"
#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif
#include "resource.h"
#include "barrels.hpp"

using namespace winrt;
using namespace Microsoft::UI::Xaml;
// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

namespace winrt::Barrels::implementation {
constexpr const auto noresizewnd = (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_CLIPCHILDREN | WS_MINIMIZEBOX);
MainWindow::MainWindow() {
  InitializeComponent();
  // Retrieve the window handle (HWND) of the current WinUI 3 window.
  auto windowNative{try_as<::IWindowNative>()};
  winrt::check_bool(windowNative);
  HWND hWnd{nullptr};
  windowNative->get_WindowHandle(&hWnd);
  auto hIcon = LoadIconW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(BERRALS_APP_ICON));
  ::SendMessageW(hWnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
  ::SendMessageW(hWnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
  ::SendMessageW(hWnd, WM_SETICON, ICON_SMALL2, (LPARAM)hIcon);
  ::SetWindowTextW(hWnd, L"Barrels - Baulk environment dock");
  auto dpiX = ::GetDpiForWindow(hWnd);
  ::SetWindowPos(hWnd, nullptr, 0, 0, ::MulDiv(700, dpiX, USER_DEFAULT_SCREEN_DPI),
                 ::MulDiv(400, dpiX, USER_DEFAULT_SCREEN_DPI), SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
  ::SetWindowLongPtrW(hWnd, GWL_STYLE, noresizewnd);

  SetBackground();
  m_closedRevoker = this->Closed(winrt::auto_revoke, [&](auto &&, auto &&) {
    if (nullptr != m_micaController) {
      m_micaController.Close();
      m_micaController = nullptr;
    }

    if (nullptr != m_dispatcherQueueController) {
      m_dispatcherQueueController.ShutdownQueueAsync();
      m_dispatcherQueueController = nullptr;
    }
  });
  OnLoad();
}

int32_t MainWindow::MyProperty() { throw hresult_not_implemented(); }

void MainWindow::MyProperty(int32_t /* value */) { throw hresult_not_implemented(); }

} // namespace winrt::Barrels::implementation

#ifdef _M_X64
constexpr std::wstring_view arch_targets[] = {L"x64", L"arm64", L"x86"};
#elif defined(_M_ARM64)
constexpr std::wstring_view arch_targets[] = {L"arm64", L"x64", L"x86"};
#else
constexpr std::wstring_view arch_targets[] = {L"x86", L"x64", L"arm64"};
#endif

void winrt::Barrels::implementation::MainWindow::OnLoad() {
  std::vector<std::wstring> baulks;
  if (barrels::search_baulk_lists(baulks)) {
    for (const auto b : baulks) {
      baulkLocationBox().Items().Append(winrt::box_value(b));
    }
    baulkLocationBox().SelectedIndex(0);
  }
  // Load arch
  for (const auto a : arch_targets) {
    archTargetBox().Items().Append(winrt::box_value(a));
  }
  archTargetBox().SelectedIndex(0);
  if (barrels::search_vs_instances(vsInstances)) {
    for (const auto &i : vsInstances) {
      auto displayName = std::format(L"{}{}", i.DisplayName, i.IsPreview ? L" Preview" : L"");
      vsInstanceBox().Items().Append(winrt::box_value(displayName));
    }
    vsInstanceBox().SelectedIndex(0);
  }
}

void winrt::Barrels::implementation::MainWindow::OnOpenBaulkTerminal(
    winrt::Windows::Foundation::IInspectable const &sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const &e) {
  (void)sender;
  (void)e;
  std::wstring vsInstanceId;
  if (auto i = vsInstanceBox().SelectedIndex(); i >= 0 && static_cast<size_t>(i) < vsInstances.size()) {
    vsInstanceId = vsInstances[i].InstanceId;
  }
  auto arch = [this]() -> std::wstring {
    if (auto i = archTargetBox().SelectedIndex(); i >= 0 && static_cast<size_t>(i) < std::size(arch_targets)) {
      return std::wstring{arch_targets[i]};
    }
    return L"";
  }();
  if (!barrels::execute(baulkLocationBox().Text(), vsInstanceId, arch, envInstanceBox().Text(),
                        makeCleanupEnvBox().IsChecked().GetBoolean())) {
  }
}

void winrt::Barrels::implementation::MainWindow::OnOpenBaulkLocation(
    winrt::Windows::Foundation::IInspectable const &sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const &e) {
  (void)sender;
  (void)e;
  auto windowNative{try_as<::IWindowNative>()};
  winrt::check_bool(windowNative);
  HWND hWnd{nullptr};
  windowNative->get_WindowHandle(&hWnd);
  if (auto baulkExec = barrels::baulk_exec_picker(hWnd); baulkExec) {
    baulkLocationBox().Text(*baulkExec);
  }
}
