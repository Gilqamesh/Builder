#include <m03h5n4ecgpxtl2skl0hn35s0n_unique_id/unique_id.h>
#include <m03gagbhtft23yhjwpp881tfmc_uuid/uuid.h>
#include <m03gn97n4iusbtl7uthb01wu9m_test_framework/test_framework.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <format>
#include <functional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace api = m03h5n4ecgpxtl2skl0hn35s0n_unique_id;
namespace uuid_api = m03gagbhtft23yhjwpp881tfmc_uuid;
namespace test = m03gn97n4iusbtl7uthb01wu9m_test_framework;

int main() {
    return test::run([] {
        using api::unique_id_t;

        const std::array<std::byte, 16> bytes{
            std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{0x00},
            std::byte{0x00}, std::byte{0x2a}, std::byte{0x70}, std::byte{0x00},
            std::byte{0x80}, std::byte{0x00}, std::byte{0x00}, std::byte{0x00},
            std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{0x00}
        };
        constexpr std::string_view reference = "000000006g2dvmnfh66jop340";
        const uuid_api::uuid uuid{std::span<const std::byte>(bytes)};
        const unique_id_t unique_id(uuid);
        test::expect(std::equal_to<>(), unique_id.string(), reference);
        test::expect(std::equal_to<>(), std::format("{}", unique_id), reference);
        test::expect(std::equal_to<>(), unique_id_t(reference), unique_id);
        const auto stored_uuid = unique_id_t(reference).uuid();
        test::expect(std::identity(), std::ranges::equal(stored_uuid.bytes(), bytes));
        test::expect(std::equal_to<>(), std::format("{}", stored_uuid), std::string("00000000-002a-7000-8000-000000000000"));
        test::expect(std::identity(), unique_id != unique_id_t("000000006g2dvmnfh66jop341"));

        const unique_id_t copied_id = unique_id;
        const unique_id_t moved_id = std::move(unique_id_t(reference));
        test::expect(std::equal_to<>(), copied_id, unique_id);
        test::expect(std::equal_to<>(), moved_id, unique_id);

        const unique_id_t maximum_id("f5lxx1zz5mx97lt8x8pzguu4f");
        test::expect(std::equal_to<>(), maximum_id.string(), std::string("f5lxx1zz5mx97lt8x8pzguu4f"));
        test::expect(std::equal_to<>(), std::format("{}", maximum_id.uuid()), std::string("ffffffff-ffff-7fff-bfff-ffffffffffff"));

        for (const auto text : {
            "", "0", "00000006g2dvmnfh66jop340", "0000000006g2dvmnfh66jop340",
            "000000006g2Dvmnfh66jop340", "000000006g2dvmnfh66jop34!",
            " 00000006g2dvmnfh66jop340", "000000006g2dvmnfh66jop34 ",
            "+00000006g2dvmnfh66jop340", "-00000006g2dvmnfh66jop340",
            "00000000-002a-7000-8000-000000000000", "m000000006g2dvmnfh66jop340_slug",
            "f5lxx1zz5pnorynqglhzmsp34", "zzzzzzzzzzzzzzzzzzzzzzzzz",
            "0000000000000000000000000"
        }) {
            test::expect_throws<std::invalid_argument>([&] {
                [[maybe_unused]] const unique_id_t invalid_id(text);
            });
        }
        auto embedded_null = std::string(reference);
        embedded_null[10] = '\0';
        test::expect_throws<std::invalid_argument>([&] {
            [[maybe_unused]] const unique_id_t invalid_id(embedded_null);
        });

        // Independent encodings of the reference UUID with every non-v7 version,
        // followed by its three invalid RFC variant bit patterns.
        for (const auto text : {
            "000000006do9a62fkplw4soow", "000000006e0k83akpcjp7n11c",
            "000000006ecv60iptzhiahdds", "000000006ep63xquymfbdbpq8",
            "000000006f1h1uz039d4g622o", "000000006fdrzs757waxj0ef4",
            "000000006fq2xpfacj8qluqrk", "000000006geotjvklt4crjfgg",
            "000000006gqzrh3pqg25udrsw", "000000006h3apebuv2zyx845c",
            "000000006hflnbjzzpxs02ghs", "000000006hrwl8s54cvl2wsu8",
            "000000006i47j60a8zte5r56o", "000000006igih38fdmr78lhj4",
            "000000006istf0gki9p0bftvk", "000000006g2dtokqgnngmaups",
            "000000006g2dunm2ywx05hyww", "000000006g2dwlorzfg37w7b4"
        }) {
            test::expect_throws<std::invalid_argument>([&] {
                [[maybe_unused]] const unique_id_t invalid_id(text);
            });
        }
        test::expect_throws<std::format_error>([&] {
            [[maybe_unused]] const auto invalid_format = std::vformat("{:x}", std::make_format_args(unique_id));
        });

        // The README's round-trip example.
        const auto generated_id = unique_id_t::generate();
        const auto text = generated_id.string();
        const unique_id_t parsed_id(text);
        test::expect(std::equal_to<>(), parsed_id, generated_id);
        test::expect(std::equal_to<>(), generated_id.uuid().version(), 7U);
        test::expect(std::equal_to<>(), text.size(), unique_id_t::encoded_size);
        for (int attempt = 0; attempt < 32; ++attempt) {
            const auto next_id = unique_id_t::generate();
            test::expect(std::identity(), next_id != generated_id);
            test::expect(std::equal_to<>(), unique_id_t(next_id.string()), next_id);
            test::expect(std::equal_to<>(), unique_id_t(next_id.uuid()), next_id);
        }
    });
}
