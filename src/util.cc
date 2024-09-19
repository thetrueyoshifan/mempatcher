#include "util.h"

using namespace mempatcher;

/**
 * Converts a wide string to a UTF-8 string.
 *
 * @param input The wide string to convert.
 * @return Converted UTF-8 string.
 */
auto util::narrow(const std::wstring& input) -> std::string
{
    auto const size = WideCharToMultiByte(CP_UTF8, 0, input.c_str(),
        static_cast<std::int32_t>(input.size()), nullptr, 0, nullptr, nullptr);

    if (!size)
        return {};

    auto result = std::string(size, '\0');

    WideCharToMultiByte(CP_UTF8, 0, input.c_str(), static_cast<std::int32_t>(input.size()),
        result.data(), static_cast<std::int32_t>(result.size()), nullptr, nullptr);

    return result;
}

/**
 * Get the command line arguments passed to the process.
 *
 * @return A vector of command line arguments.
 */
auto util::get_argv() -> std::vector<std::string>
{
    auto argc = std::int32_t {};
    auto const argv = CommandLineToArgvW(GetCommandLineW(), &argc);

    auto result = std::vector<std::string> {};

    for (auto i = 0; i < argc; ++i)
        result.push_back(narrow(argv[i]));

    LocalFree(argv);

    return result;
}

/**
 * Find addresses to exported methods from a loaded module.
 *
 * @param module The name of the module to resolve the functions from.
 * @param names The names of the functions to resolve.
 * @return A map of function names to their addresses.
 */
auto util::resolve_dll_imports(std::string_view module, const std::vector<std::string>& names)
    -> std::unordered_map<std::string, std::uint8_t*>
{
    auto const handle = GetModuleHandleA(module.data());

    if (!handle)
    {
        spdlog::error("Failed to get module handle for '{}'", module);
        return {};
    }

    auto result = std::unordered_map<std::string, std::uint8_t*> {};

    for (auto&& name: names)
    {
        result[name] = reinterpret_cast<std::uint8_t*>
            (GetProcAddress(handle, name.c_str()));

        if (!result[name])
        {
            spdlog::error("Failed to resolve address for '{}' in '{}'", module, name);
            return {}; // return nothing instead of an incomplete result
        }
    }

    return result;
}