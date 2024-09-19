#include <share.h>
#include <fstream>
#include <catch2/catch_test_macros.hpp>

#include "../src/parser.h"

using namespace mempatcher;

TEST_CASE("Non-existent path fails to open", "[parse-mph]")
{
    auto const path = std::filesystem::path {};
    REQUIRE(parser::read_file(path).error().ec == parser::errc::file_not_found);
}

TEST_CASE("Unreadable file fails to open", "[parse-mph]")
{
    auto file = _fsopen("lock.tmp", "w", _SH_DENYRW);

    REQUIRE(parser::read_file("lock.tmp").error().ec == parser::errc::file_open_fail);

    fclose(file);
    std::filesystem::remove("lock.tmp");
}

TEST_CASE("Empty file returns no patches", "[parse-mph]")
{
    {
        auto file = std::ofstream { "empty.tmp" };
        REQUIRE(parser::read_file("empty.tmp")->empty());
    }

    std::filesystem::remove("empty.tmp");
}

TEST_CASE("Empty line returns error", "[parse-mph]")
{
    REQUIRE(parser::read_line("").error() == parser::errc::parse_line_empty);
    REQUIRE(parser::read_line("   ").error() == parser::errc::parse_line_empty);
}

TEST_CASE("Insufficient arguments return error", "[parse-mph]")
{
    REQUIRE(parser::read_line("target.exe").error() == parser::errc::parse_insufficient_args);
    REQUIRE(parser::read_line("\"target.exe\"").error() == parser::errc::parse_insufficient_args);
    REQUIRE(parser::read_line("target.exe       ").error() == parser::errc::parse_insufficient_args);
    REQUIRE(parser::read_line("target.exe 123456").error() == parser::errc::parse_insufficient_args);
    REQUIRE(parser::read_line("target.exe 123456 -").error() == parser::errc::parse_insufficient_args);
}

TEST_CASE("Too many arguments return error", "[parse-mph]")
{
    REQUIRE(parser::read_line("target.exe 123456 11 22 33").error() == parser::errc::parse_too_many_args);
    REQUIRE(parser::read_line("target.exe 123456 11 22 33 44").error() == parser::errc::parse_too_many_args);
}

TEST_CASE("Unclosed quote in target returns error", "[parse-mph]")
{
    REQUIRE(parser::read_line("\"target.exe").error() == parser::errc::parse_target_unclosed_quote);
}

TEST_CASE("Invalid offset address returns error", "[parse-mph]")
{
    REQUIRE(parser::read_line("target.exe huh 11 22").error() == parser::errc::parse_bad_offset_address);
    REQUIRE(parser::read_line("target.exe f+wha 11 22").error() == parser::errc::parse_bad_offset_address);
}

TEST_CASE("Data length non-divisible by 2 returns error", "[parse-mph]")
{
    REQUIRE(parser::read_line("target.exe 123456 11 22222").error() == parser::errc::parse_bad_data_length);
    REQUIRE(parser::read_line("target.exe 123456 11111 22").error() == parser::errc::parse_bad_data_length);
}

TEST_CASE("Invalid data bytes return error", "[parse-mph]")
{
    REQUIRE(parser::read_line("target.exe 123456 00 ZZ").error() == parser::errc::parse_bad_data_bytes);
    REQUIRE(parser::read_line("target.exe 123456 @1 ]2").error() == parser::errc::parse_bad_data_bytes);
}

TEST_CASE("Valid patches parse successfully", "[parse-mph]")
{
    REQUIRE(parser::read_line("\"spaced target.exe\" ABCDEF 11 22").has_value());
    REQUIRE(parser::read_line("\"spaced target.exe\" ABCDEF - 22").has_value());
    REQUIRE(parser::read_line("\"spaced target.exe\" ABCDEF 123456").has_value());

    {
        auto file = std::ofstream { "valid.mph" };
        file << "# A comment followed by some empty lines\n\n\n";
        file << "- 99999999 00 11\n";
        file << "target.dll 123456 11 22\n";
        file << "\"cool lib.dll\" ABCDEF 11223344556677889900AABBCCDDEEFF\n";
        file << "s0m3th1ng3ls3.exe f+DEADBEEF - AAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\n";
        file << "s0m3th1ng3ls3.exe F+1337CAFE - BBBBBBBBBBBBBBBBBBBBBBBBBBBBBB\n";
    }

    REQUIRE(parser::read_file("valid.mph")->size() == 5);
    std::filesystem::remove("valid.mph");
}

TEST_CASE("Invalid patch in file has line number", "[parse-mph]")
{
    {
        auto file = std::ofstream { "valid.mph" };
        file << "# Some empty lines to pad the line number\n\n\n\n\n";
        file << "badLineThatShouldError";
    }

    REQUIRE(parser::read_file("valid.mph").error().line == 6);
    std::filesystem::remove("valid.mph");
}