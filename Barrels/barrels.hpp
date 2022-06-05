#ifndef BARRELS_HPP
#define BARRELS_HPP
#include "pch.h"
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace barrels {
std::optional<std::wstring> baulk_exec_picker(HWND hWnd);
bool search_vs_instances(vs_instances_t &vsInstances);
bool search_baulk_lists(std::vector<std::wstring> &baulks);
bool execute(const std::wstring_view baulkExec, const std::wstring_view vsInstanceId, const std::wstring_view arch,
             const std::wstring_view venv, bool makeCleanEnv = false);
} // namespace barrels

#endif