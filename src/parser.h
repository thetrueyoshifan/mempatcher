#pragma once

#include <string>
#include <vector>
#include <expected>
#include <filesystem>

namespace mempatcher::parser
{
    enum class addr_type { absolute, rva, file };

    struct patch
    {
        addr_type type;
        std::size_t line;
        std::string target;
        std::uintptr_t address;
        std::vector<std::uint8_t> on;
        std::vector<std::uint8_t> off;

        [[nodiscard]] auto type_name() const -> std::string_view;
        [[nodiscard]] auto target_name() const -> std::string;
    };

    enum class errc: int
    {
        file_not_found = 1,
        file_open_fail,
        parse_line_empty,
        parse_insufficient_args,
        parse_too_many_args,
        parse_target_unclosed_quote,
        parse_bad_offset_address,
        parse_bad_data_length,
        parse_bad_data_bytes,
    };

    struct read_target_result
    {
        std::string name;
        std::string::size_type offset;
    };

    struct read_offset_result
    {
        addr_type type;
        std::uintptr_t address;
    };

    struct parse_error
    {
        errc ec;
        std::size_t line;
    };

    auto make_error_code(errc e) -> std::error_code;

    [[nodiscard]] auto read_target(const std::string& line) -> std::expected<read_target_result, errc>;
    [[nodiscard]] auto read_offset(const std::string& offset) -> std::expected<read_offset_result, errc>;
    [[nodiscard]] auto read_data(const std::string& bytes) -> std::expected<std::vector<std::uint8_t>, errc>;
    [[nodiscard]] auto read_line(const std::string& line) -> std::expected<patch, errc>;
    [[nodiscard]] auto read_file(const std::filesystem::path& path) -> std::expected<std::vector<patch>, parse_error>;
}

template <> struct std::is_error_code_enum<mempatcher::parser::errc>: true_type {};