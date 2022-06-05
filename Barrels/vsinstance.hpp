#ifndef BERRALS_VS_INSTANCE_HPP
#define BERRALS_VS_INSTANCE_HPP
#include <string>
#include <vector>
#include <format>
#include <wil/com.h>
#include <comutil.h>
#include "barrels.hpp"
#include "Setup.Configuration.h"

#ifndef VSSetupConstants
#define VSSetupConstants
/* clang-format off */
const IID IID_ISetupConfiguration = {
  0x42843719, 0xDB4C, 0x46C2,
  { 0x8E, 0x7C, 0x64, 0xF1, 0x81, 0x6E, 0xFD, 0x5B }
};
const IID IID_ISetupConfiguration2 = {
  0x26AAB78C, 0x4A60, 0x49D6,
  { 0xAF, 0x3B, 0x3C, 0x35, 0xBC, 0x93, 0x36, 0x5D }
};
const IID IID_ISetupPackageReference = {
  0xda8d8a16, 0xb2b6, 0x4487,
  { 0xa2, 0xf1, 0x59, 0x4c, 0xcc, 0xcd, 0x6b, 0xf5 }
};
const IID IID_ISetupHelper = {
  0x42b21b78, 0x6192, 0x463e,
  { 0x87, 0xbf, 0xd5, 0x77, 0x83, 0x8f, 0x1d, 0x5c }
};
const IID IID_IEnumSetupInstances = {
  0x6380BCFF, 0x41D3, 0x4B2E,
  { 0x8B, 0x2E, 0xBF, 0x8A, 0x68, 0x10, 0xC8, 0x48 }
};
const IID IID_ISetupInstance2 = {
  0x89143C9A, 0x05AF, 0x49B0,
  { 0xB7, 0x17, 0x72, 0xE2, 0x18, 0xA2, 0x18, 0x5C }
};
const IID IID_ISetupInstance = {
  0xB41463C3, 0x8866, 0x43B5,
  { 0xBC, 0x33, 0x2B, 0x06, 0x76, 0xF7, 0xF4, 0x2E }
};
const CLSID CLSID_SetupConfiguration = {
  0x177F0C4A, 0x1CD3, 0x4DE7,
  { 0xA3, 0x2C, 0x71, 0xDB, 0xBB, 0x9F, 0xA3, 0x6D }
};
/* clang-format on */
#endif

namespace bela {
class comstr {
public:
  comstr() { str = nullptr; }
  comstr(const comstr &src) {
    if (src.str != nullptr) {
      str = ::SysAllocStringByteLen((char *)str, ::SysStringByteLen(str));
    } else {
      str = ::SysAllocStringByteLen(NULL, 0);
    }
  }
  comstr &operator=(const comstr &src) {
    if (str != src.str) {
      ::SysFreeString(str);
      if (src.str != nullptr) {
        str = ::SysAllocStringByteLen((char *)str, ::SysStringByteLen(str));
      } else {
        str = ::SysAllocStringByteLen(NULL, 0);
      }
    }
    return *this;
  }
  operator BSTR() const { return str; }
  BSTR *operator&() throw() { return &str; }
  ~comstr() throw() { ::SysFreeString(str); }

private:
  BSTR str;
};

} // namespace bela

namespace barrels {
class Searcher {
public:
  Searcher() = default;
  Searcher(const Searcher &) = delete;
  Searcher &operator=(const Searcher &) = delete;
  bool Initialize();
  bool Search(vs_instances_t &vsInstances);

private:
  bool AddInstance(ISetupInstance2 *inst2, vs_instances_t &vsInstances);
  wil::com_ptr<ISetupConfiguration> sc;
  wil::com_ptr<ISetupConfiguration2> sc2;
  wil::com_ptr<ISetupHelper> sh;
};
inline bool Searcher::Initialize() {
  auto hr = ::CoCreateInstance(CLSID_SetupConfiguration, nullptr, CLSCTX_ALL, IID_ISetupConfiguration,
                               reinterpret_cast<void **>(&sc));
  if (FAILED(hr) || !sc) {
    return false;
  }
  hr = sc->QueryInterface(IID_ISetupConfiguration2, reinterpret_cast<void **>(&sc2));
  if (FAILED(hr) || !sc) {
    return false;
  }
  hr = sc->QueryInterface(IID_ISetupHelper, reinterpret_cast<void **>(&sh));
  if (FAILED(hr) || !sc) {
    return false;
  }
  return true;
}

inline bool Searcher::AddInstance(ISetupInstance2 *inst2, vs_instances_t &vsInstances) {
  bela::comstr bstrId;
  if (FAILED(inst2->GetInstanceId(&bstrId))) {
    return false;
  }
  vs_instance_t vsi;
  vsi.InstanceId.assign(bstrId);
  InstanceState state;
  if (FAILED(inst2->GetState(&state))) {
    return false;
  }
  bela::comstr bstrName;
  if (SUCCEEDED(inst2->GetDisplayName(GetUserDefaultLCID(), &bstrName))) {
    vsi.DisplayName.assign(bstrName);
  }
  wil::com_ptr<ISetupInstanceCatalog> catalog;
  if (SUCCEEDED(inst2->QueryInterface(__uuidof(ISetupInstanceCatalog), (void **)&catalog)) && catalog) {
    if (variant_t vt; SUCCEEDED(catalog->IsPrerelease(&vt.boolVal))) {
      vsi.IsPreview = (vt.boolVal != VARIANT_FALSE);
    }
  }
  ULONGLONG ullVersion = 0;
  bela::comstr bstrVersion;
  if (FAILED(inst2->GetInstallationVersion(&bstrVersion))) {
    return false;
  }
  vsi.Version = bstrVersion;
  if (SUCCEEDED(sh->ParseVersion(bstrVersion, &ullVersion))) {
    vsi.ullVersion = ullVersion;
    vsi.ullMainVersion = ullVersion >> 48;
  }

  // Reboot may have been required before the installation path was created.
  bela::comstr bstrInstallationPath;
  if ((eLocal & state) == eLocal) {
    if (FAILED(inst2->GetInstallationPath(&bstrInstallationPath))) {
      return false;
    }
    std::wstring_view installLocation{bstrInstallationPath};
    if (installLocation.ends_with(L"\\")) {
      installLocation.remove_suffix(1);
    }
    vsi.InstallLocation = installLocation;
  }
  if ((eRegistered & state) == eRegistered) {
    wil::com_ptr<ISetupPackageReference> product;
    if (FAILED(inst2->GetProduct(&product)) || !product) {
      return false;
    }

    LPSAFEARRAY lpsaPackages;
    if (FAILED(inst2->GetPackages(&lpsaPackages)) || lpsaPackages == NULL) {
      return false;
    }

    auto checkInstalledComponent = [&](ISetupPackageReference *package) -> bool {
      constexpr const std::wstring_view Win11SDKComponent = L"Microsoft.VisualStudio.Component.Windows11SDK";
      constexpr const std::wstring_view Win10SDKComponent = L"Microsoft.VisualStudio.Component.Windows10SDK";
      constexpr const std::wstring_view ComponentType = L"Component";
      bela::comstr bstrId;
      if (FAILED(package->GetId(&bstrId))) {
        return false;
      }
      bela::comstr bstrType;
      if (FAILED(package->GetType(&bstrType))) {
        return false;
      }

      std::wstring_view id{bstrId};
      std::wstring_view type{bstrType};
      vsi.Packages.emplace_back(id);
      // L"Microsoft.VisualStudio.Component.Windows11SDK.22000"
      if (id.starts_with(Win11SDKComponent) && type == ComponentType) {
        vsi.IsWin11SDKInstalled = true;
      }
      if (id.starts_with(Win10SDKComponent) && type == ComponentType) {
        vsi.IsWin10SDKInstalled = true;
      }
      return true;
    };

    int lower = lpsaPackages->rgsabound[0].lLbound;
    int upper = lpsaPackages->rgsabound[0].cElements + lower;

    IUnknown **ppData = (IUnknown **)lpsaPackages->pvData;
    for (int i = lower; i < upper; i++) {
      wil::com_ptr<ISetupPackageReference> package;
      if (FAILED(ppData[i]->QueryInterface(IID_ISetupPackageReference, (void **)&package)) || !package) {
        continue;
      }
      checkInstalledComponent(package.get());
    }
    SafeArrayDestroy(lpsaPackages);
  }
  vsInstances.emplace_back(std::move(vsi));
  return true;
}

inline bool Searcher::Search(vs_instances_t &vsInstances) {
  wil::com_ptr<IEnumSetupInstances> es;
  if (FAILED(sc2->EnumAllInstances(reinterpret_cast<IEnumSetupInstances **>(&es)))) {
    return false;
  }
  for (;;) {
    wil::com_ptr<ISetupInstance> inst;
    if (!SUCCEEDED(es->Next(1, reinterpret_cast<ISetupInstance **>(&inst), nullptr)) || !inst) {
      break;
    }
    wil::com_ptr<ISetupInstance2> inst2;
    if (FAILED(inst->QueryInterface(IID_ISetupInstance2, reinterpret_cast<void **>(&inst2))) || !inst2) {
      continue;
    }
    AddInstance(inst2.get(), vsInstances);
  }
  std::sort(vsInstances.begin(), vsInstances.end());
  return true;
}
} // namespace barrels

#endif