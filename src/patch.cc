#include <span>

#include "patch.h"

using namespace mempatcher;

/**
 * Converts a file offset to a relative virtual address.
 *
 * @param base Pointer to the image base address.
 * @param offset File offset to convert.
 * @return Relative virtual address, or nullptr if the offset was invalid.
 */
auto file2rva(std::uint8_t* base, std::uintptr_t offset) -> std::uint8_t*
{
    auto const dos = reinterpret_cast<PIMAGE_DOS_HEADER>(base);

    if (dos->e_magic != IMAGE_DOS_SIGNATURE)
        return nullptr;

    auto const nt = reinterpret_cast<PIMAGE_NT_HEADERS>(base + dos->e_lfanew);

    if (nt->Signature != IMAGE_NT_SIGNATURE)
        return nullptr;

    auto const sections = IMAGE_FIRST_SECTION(nt);

    if (offset < sections->PointerToRawData)
        return base + offset;

    for (auto i = 0; i < nt->FileHeader.NumberOfSections; ++i)
    {
        auto const section = sections + i;

        if (offset >= section->PointerToRawData && offset < section->PointerToRawData + section->SizeOfRawData)
            return base + section->VirtualAddress + (offset - section->PointerToRawData);
    }

    return nullptr;
}

/**
 * Resolve patch address to a location in memory.
 *
 * @param base Pointer to the image base address.
 * @param patch Patch containing the address.
 * @return Final address for the patch, or nullptr if an error occurred.
 */
auto resolve_address(std::uint8_t* base, const parser::patch& patch) -> std::uint8_t*
{
    if (patch.type == parser::addr_type::absolute)
        return reinterpret_cast<std::uint8_t*>(patch.address);

    if (patch.type == parser::addr_type::rva)
        return base + patch.address;

    if (patch.type == parser::addr_type::file)
    {
        auto const result = file2rva(base, patch.address);

        if (!result)
        {
            return nullptr;
        }

        return result;
    }

    return nullptr;
}

/**
 * Compare data in memory to the expected data from patch.
 *
 * @param target Pointer to the memory location.
 * @param patch Patch containing the expected data.
 * @return True if the data matches, false otherwise.
 */
auto compare_data(std::uint8_t* target, const parser::patch& patch)
{
    if (std::memcmp(target, patch.off.data(), patch.off.size()) == 0)
        return true;

    if (!patch.on.empty() && std::memcmp(target, patch.on.data(), patch.on.size()) == 0)
    {
        return true;
    }

    return false;
}

/**
 * Compare patch data while handling SEH exceptions.
 *
 * @param target Pointer to the memory location.
 * @param patch Patch containing the expected data.
 * @return True if the data matches, false otherwise.
 */
auto try_compare_data(std::uint8_t* target, const parser::patch& patch)
{
    if (patch.off.empty())
        return true;

    __try
        { return compare_data(target, patch); }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

/**
 * Apply patched bytes to the specified address.
 *
 * @param target Pointer to the memory location.
 * @param patch Patch containing the data to write.
 * @return True if the data was successfully written, false otherwise.
 */
auto write_data(std::uint8_t* target, const parser::patch& patch)
{
    auto old = DWORD {};

    if (!VirtualProtect(target, patch.on.size(), PAGE_EXECUTE_READWRITE, &old))
    {
        return false;
    }

    std::memcpy(target, patch.on.data(), patch.on.size());

    if (!VirtualProtect(target, patch.on.size(), old, &old))
    {
        return false;
    }

    return true;
}

/**
 * Applies a patch to the specified address.
 *
 * @param base Pointer to the image base address.
 * @param patch Patch to apply.
 * @return True if the patch was successfully applied, false otherwise.
 */
auto patch::apply(std::uint8_t* base, const parser::patch& patch) -> bool
{
    auto const address = resolve_address(base, patch);

    if (!address)
        return false;

    if (!try_compare_data(address, patch))
        return false;

    if (patch.on.empty())
    {
        return true;
    }

    return write_data(address, patch);
}