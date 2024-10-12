#include "util.h"
#include "hooks.h"
#include "patch.h"

using namespace mempatcher;
using namespace mempatcher::hooks;

namespace mempatcher::hooks::detail
{
    auto cookie = PVOID {};
    auto module = HMODULE {};
    auto patches = patch_list {};

    decltype(LdrUnregisterDllNotification)* unregister_fn {};
}

/**
 * Unregister loader notification and unload module from process.
 */
auto WINAPI unregister_and_unload(PVOID) -> DWORD
{
    auto const result = detail::unregister_fn(detail::cookie);

    if (NT_SUCCESS(result))
    {
        spdlog::info("All patches applied, unloading from process...");
        FreeLibraryAndExitThread(detail::module, EXIT_SUCCESS);
    }

    spdlog::error("Failed to unregister DLL notification callback (0x{:X})", result);
    return EXIT_FAILURE;
}

/**
 * Handle newly loaded DLLs and apply patches.
 */
auto CALLBACK dll_notification(ULONG reason, PCLDR_DLL_NOTIFICATION_DATA data, PVOID) -> void
{
    if (detail::patches.empty() || reason != LDR_DLL_NOTIFICATION_REASON_LOADED)
        return;

    auto const address = static_cast<std::uint8_t*>(data->Loaded.DllBase);
    auto const module = util::narrow({
        data->Loaded.BaseDllName->Buffer,
        data->Loaded.BaseDllName->Length / sizeof(wchar_t)
    });

    if (!detail::patches.contains(module))
        return;

    spdlog::info("Loaded target file '{}' at address 0x{:X}",
        module, std::bit_cast<std::uintptr_t>(address));

    for (auto&& patch: detail::patches[module])
        if (!patch::apply(address, patch))
            break;

    detail::patches.erase(module);

    if (!detail::patches.empty())
        return;

    CreateThread(nullptr, 0, unregister_and_unload, nullptr, 0, nullptr);
}

/**
 * Installs notifications for loader events.
 *
 * @param module Handle to this module.
 * @param patches List of patches to apply.
 * @return True if the module should remain loaded, false otherwise.
 */
auto hooks::install(HMODULE module, patch_list&& patches) -> bool
{
    detail::module = module;
    detail::patches = std::move(patches);

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

    // catch future libraries
    auto const imports = util::resolve_dll_imports("ntdll.dll",
        { "LdrRegisterDllNotification", "LdrUnregisterDllNotification" });

    if (imports.empty()) [[unlikely]]
    {
        spdlog::error("Failed to resolve required imports from 'ntdll.dll'");
        return false;
    }

    auto const register_fn = reinterpret_cast<decltype(&LdrRegisterDllNotification)>
        (imports.at("LdrRegisterDllNotification"));

    detail::unregister_fn = reinterpret_cast<decltype(&LdrUnregisterDllNotification)>
        (imports.at("LdrUnregisterDllNotification"));

    auto const result = register_fn(0, dll_notification, nullptr, &detail::cookie);

    if (result < 0)
        spdlog::error("Failed to register DLL notification callback (code: {:X})", result);

    return NT_SUCCESS(result);
}