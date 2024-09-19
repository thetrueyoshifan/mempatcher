#include "util.h"
#include "hooks.h"
#include "patch.h"

using namespace mempatcher;
using namespace mempatcher::hooks;

namespace mempatcher::hooks::detail
{
    auto module = HMODULE {};
    auto patches = patch_list {};

    auto open_file_hook = safetyhook::InlineHook {};
    auto create_section_hook = safetyhook::InlineHook {};
    auto map_view_of_section_hook = safetyhook::InlineHook {};

    auto file = HANDLE {};
    auto section = HANDLE {};
    auto path = std::filesystem::path {};
}

/**
 * First hook in the chain for capturing the file handle and path.
 */
auto NTAPI open_file_hook_fn(PHANDLE file, ACCESS_MASK access, POBJECT_ATTRIBUTES obj_attrs,
    PIO_STATUS_BLOCK io_status, ULONG share, ULONG opts) -> NTSTATUS
{
    auto const result = detail::open_file_hook.stdcall<NTSTATUS>
        (file, access, obj_attrs, io_status, share, opts);

    if (NT_SUCCESS(result))
    {
        auto const name = std::wstring_view {
            obj_attrs->ObjectName->Buffer,
            obj_attrs->ObjectName->Length / sizeof(WCHAR)
        };

        detail::file = *file;
        detail::path = name;
    }

    return result;
}

/**
 * Second hook for capturing the section handle.
 */
auto NTAPI create_section_hook_fn(PHANDLE section, ACCESS_MASK access, POBJECT_ATTRIBUTES obj_attrs,
    PLARGE_INTEGER max_size, ULONG protect, ULONG alloc_attrs, HANDLE file) -> NTSTATUS
{
    auto const result = detail::create_section_hook.stdcall<NTSTATUS>
        (section, access, obj_attrs, max_size, protect, alloc_attrs, file);

    if (NT_SUCCESS(result) && file == detail::file)
        detail::section = *section;

    return result;
}

/**
 * Final hook for applying patches.
 */
auto NTAPI map_view_of_section_hook_fn(HANDLE section, HANDLE handle, PVOID* base_addr,
    ULONG_PTR zerobits, SIZE_T commit_size, PLARGE_INTEGER section_offset, PSIZE_T view_size,
    SECTION_INHERIT inherit, ULONG alloc_type, ULONG protect) -> NTSTATUS
{
    auto const result = detail::map_view_of_section_hook.stdcall<NTSTATUS>(section, handle, base_addr,
        zerobits, commit_size, section_offset, view_size, inherit, alloc_type, protect);

    if (NT_SUCCESS(result))
    {
        if (detail::section != section)
            return result;

        auto const module = detail::path.filename().string();
        auto const address = static_cast<std::uint8_t*>(*base_addr);

        if (!detail::patches.contains(module))
            return result;

        spdlog::info("Loaded target file '{}' at address 0x{:X}",
            module, std::bit_cast<std::uintptr_t>(address));

        for (auto&& patch: detail::patches[module])
            if (!patch::apply(address, patch))
                break;

        detail::patches.erase(module);

        if (detail::patches.empty())
        {
            auto const fn = reinterpret_cast<LPTHREAD_START_ROUTINE>(FreeLibraryAndExitThread);
            spdlog::info("All patches applied, unloading from process...");
            CreateThread(nullptr, 0, fn, detail::module, 0, nullptr);
        }
    }

    return result;
}

/**
 * Installs hooks for the LoadLibrary functions.
 *
 * @param module Handle to this module.
 * @param patches List of patches to apply.
 * @return True if the module should remain loaded, false otherwise.
 */
auto hooks::install(HMODULE module, patch_list&& patches) -> bool
{
    detail::module = module;
    detail::patches = std::move(patches);

    auto const imports = util::resolve_dll_imports("ntdll.dll",
        { "NtOpenFile", "NtCreateSection", "NtMapViewOfSection" });

    if (imports.empty()) [[unlikely]]
        return false;

    detail::open_file_hook = safetyhook::create_inline
        (imports.at("NtOpenFile"), open_file_hook_fn);

    detail::create_section_hook = safetyhook::create_inline
        (imports.at("NtCreateSection"), create_section_hook_fn);

    detail::map_view_of_section_hook = safetyhook::create_inline
        (imports.at("NtMapViewOfSection"), map_view_of_section_hook_fn);

    // apply patches for any libraries that are already loaded
    for (auto&& [module, patches]: detail::patches)
    {
        if (module == "-")
        {
            for (auto&& patch: patches)
                if (!patch::apply(nullptr, patch))
                    break;

            detail::patches.erase(module);
        }
        else if (auto const base = GetModuleHandleA(module.c_str()))
        {
            spdlog::info("Target '{}' is already loaded at address 0x{:X}",
                module, std::bit_cast<std::uintptr_t>(base));

            for (auto&& patch: patches)
                if (!patch::apply(reinterpret_cast<std::uint8_t*>(base), patch))
                    break;

            detail::patches.erase(module);
        }
    }

    // if everything was applied, unload now
    if (detail::patches.empty())
    {
        spdlog::info("All patches applied, unloading from process...");
        return false;
    }

    return true;
}