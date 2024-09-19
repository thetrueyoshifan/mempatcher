#pragma once

#include "parser.h"

namespace mempatcher::patch
{
    [[nodiscard]] auto apply(std::uint8_t* base, const parser::patch& patch) -> bool;
}