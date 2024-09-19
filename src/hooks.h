#pragma once

#include "parser.h"

namespace mempatcher::hooks
{
    using patch_list = std::unordered_map<std::string, std::vector<parser::patch>>;

    auto install(HMODULE module, patch_list&& patches) -> bool;
}