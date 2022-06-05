#pragma once
#include <windows.h>
#include <unknwn.h>
#include <restrictederrorinfo.h>
#include <hstring.h>

// Undefine GetCurrentTime macro to prevent
// conflict with Storyboard::GetCurrentTime
#undef GetCurrentTime

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.ApplicationModel.Activation.h>
#include <winrt/Microsoft.UI.Composition.h>
#include <winrt/Microsoft.UI.Xaml.h>
#include <winrt/Microsoft.UI.Xaml.Controls.h>
#include <winrt/Microsoft.UI.Xaml.Controls.Primitives.h>
#include <winrt/Microsoft.UI.Xaml.Data.h>
#include <winrt/Microsoft.UI.Xaml.Interop.h>
#include <winrt/Microsoft.UI.Xaml.Markup.h>
#include <winrt/Microsoft.UI.Xaml.Media.h>
#include <winrt/Microsoft.UI.Xaml.Navigation.h>
#include <winrt/Microsoft.UI.Xaml.Shapes.h>
#include <winrt/Microsoft.UI.Dispatching.h>
#include <winrt/Microsoft.UI.Composition.SystemBackdrops.h>
#include <winrt/Microsoft.UI.Interop.h>
#include <winrt/Windows.System.h>
#include <dispatcherqueue.h>
#include <microsoft.ui.xaml.window.h>
#include <wil/cppwinrt_helpers.h>

#include <vector>
#include <string>

namespace barrels {
struct vs_instance_t {
  std::wstring InstanceId;
  std::wstring DisplayName;
  std::wstring InstallLocation;
  std::wstring Version;
  std::vector<std::wstring> Packages;
  uint64_t ullVersion = 0;
  uint64_t ullMainVersion = 0;
  bool IsWin11SDKInstalled = false;
  bool IsWin10SDKInstalled = false;
  bool IsPreview = false;
  // compare
  bool operator<(const vs_instance_t &r) {
    if (ullMainVersion == r.ullMainVersion) {
      // MainVersion equal
      if (IsPreview == r.IsPreview) {
        return ullVersion < r.ullVersion;
      }
      return IsPreview;
    }
    return ullMainVersion < r.ullMainVersion;
  }
};

using vs_instances_t = std::vector<vs_instance_t>;
} // namespace barrels