#pragma once

namespace mempatcher::util
{
    [[nodiscard]] auto narrow(const std::wstring& input) -> std::string;
    [[nodiscard]] auto get_argv() -> std::vector<std::string>;
    [[nodiscard]] auto resolve_dll_imports(std::string_view module, const std::vector<std::string>& names)
        -> std::unordered_map<std::string, std::uint8_t*>;
}