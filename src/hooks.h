#pragma once

#include "parser.h"

namespace mempatcher::hooks
{
    using patch_list = std::vector<parser::patch>;

    auto install(HMODULE module, patch_list&& patches) -> bool;
}