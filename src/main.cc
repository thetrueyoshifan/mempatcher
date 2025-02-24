#include <ranges>

#include "util.h"
#include "hooks.h"
#include "buildinfo.h"

using namespace mempatcher;

/**
 * Find input patch files from the process command line.
 * Support both '--mempatch <file>' and '--mempatch=<file>' formats.
 *
 * @param argv Command line arguments.
 * @return List of input files.
 */
auto get_input_files(const std::vector<std::string>& argv)
{
    auto result = std::vector<std::filesystem::path> {};

    for (auto i = 1; i < argv.size(); ++i)
    {
        auto path = std::filesystem::path {};

        if (argv[i].starts_with("--mempatch="))
            path = { argv[i].substr(11) };
        else if (argv[i] == "--mempatch" && ++i < argv.size())
            path = { argv[i] };

        if (path.empty())
            continue;

        if (!exists(path))
        {
            spdlog::warn("Patch file '{}' does not exist!", path.string());
            continue;
        }

        result.push_back(std::move(path));
    }

    return result;
}

/**
 * Find *.mph files in a given directory.
 *
 * @return List of input files.
 */
auto get_directory_files(const std::filesystem::path& path)
{
    auto result = std::vector<std::filesystem::path> {};

    if (!is_directory(path))
        return result;

    for (auto&& entry: std::filesystem::directory_iterator(path))
        if (entry.is_regular_file() && entry.path().extension() == ".mph")
            result.push_back(entry.path());

    return result;
}

/**
 * Get the base filename of the host module.
 *
 * @return Host module filename.
 */
auto get_host_exe() -> std::string
{
    auto name = std::wstring(MAX_PATH, L'\0');
    auto const size = GetModuleFileNameW(nullptr, name.data(), name.size());

    if (size == 0)
        return "<unknown>";

    name.resize(size);

    return std::filesystem::path { name }.filename().string();
}

/**
 * Main entrypoint: install hooks & patch any already loaded libraries.
 *
 * @param module Handle to this module.
 * @param reason Reason for calling this function.
 * @return True if the module should remain loaded, false otherwise.
 */
auto DllMain(HINSTANCE module, DWORD reason, LPVOID) -> BOOL
{
    if (reason != DLL_PROCESS_ATTACH)
        return TRUE;

    DisableThreadLibraryCalls(module);

    auto const argv = util::get_argv();
    auto files = get_input_files(argv);

    std::ranges::move(get_directory_files("autopatch"), std::back_inserter(files));

    if (files.empty())
    {
        return FALSE;
    }

    auto patches = hooks::patch_list {};

    for (auto&& file: files)
    {
        auto result = parser::read_file(file);

        if (!result)
        {
            return FALSE;
        }

        for (auto&& patch: *result)
        {
            patches.push_back(std::move(patch));
        }
    }

    // set up hooks
    if (!hooks::install(module, std::move(patches)))
        return FALSE;

    return TRUE;
}