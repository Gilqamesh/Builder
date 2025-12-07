#include <iostream>
#include <thread>
#include <riot.h.in>

int main() {
    riot_api riot("RGAPI-7875e7bc-061f-4c55-850f-be865a19b81e");

    const std::string& game_name = "Hart";
    const std::string& tag_line = "IHART";
    riot.get_puuid_async(
        game_name,
        tag_line,
        [&](const std::string& resulting_puuid) {
            riot.get_challenges_by_puuid_async(
                riot_api::region_t::REGION_EUW,
                resulting_puuid,
                [&](const nlohmann::json& resulting_challenges_info_for_puuid) {
                    std::cout << "Asynchronously received challenges info for PUUID " << resulting_puuid << ":\n";
                    std::cout << resulting_challenges_info_for_puuid.dump(4) << std::endl;
                },
                []() {
                    std::cerr << "Failed to asynchronously fetch challenges info." << std::endl;
                }
            );
        },
        []() {
            std::cerr << "Failed to asynchronously fetch PUUID." << std::endl;
        }
    );

    std::this_thread::sleep_for(std::chrono::seconds(5));

    return 0;
}
