#include "status_thread.h"
#include "md5.h"
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <deque>
#include <iostream>
#include "logger.h"

#include <curl/curl.h>
#include <nlohmann/json.hpp>
using json = nlohmann::json;


namespace status_thread {
    static std::map<std::string, std::string> mapNameCache = {
        {"3a6b992635cb471d0d435eec3f28ee815d832f0a6666412ac6dce2e80", "air_afghan_map"},
        {"93acf3bcb73c8b04eb1426b61ffe3a3c", "avg_training_ground_tankmap"},
    };

    std::unordered_map<std::string, std::string> md5_to_name = {
    {"00b4144b0adfecf08f135ad576cce33e", "avg_syria_map"},
    {"0162b89cce6e73a7cd27629c891694b5", "avg_fulda_map"},
    {"01710143d8efe31f1df4acb295f5d3ad", "air_falklands_map"},
    {"0183b6a3a123aee544b506feb1e38f6d", "air_southeastern_cliffs_map"},
    {"03593838da81ef968cc5339f0a6970ea", "avg_sector_montmedy_snow_map"},
    {"050b5024061dc65f4ed6bfde70bee602", "avg_israel_map"},
    {"056a9468fd1bd639e6f12e867771f0b7", "avg_guadalcanal_map"},
    {"065dbb36be678c415b7a0a75b47bdf2a", "avg_sector_montmedy_map"},
    {"09c2bb89124a83d06be7e8cab9055775", "avg_netherlands_tankmap"},
    {"0a97e23ad4d81be6848f37f27cc95ea2", "malta_map"},
    {"0aae7e8a00eff4b274c019ccaf8b31ab", "avg_greece_map"},
    {"0b63e3175382834e32d165ed4dfc40b5", "arcade_canyon_snow_map"},
    {"0b8b4cd33f739633ce5663c1b791e135", "avn_england_shore_map"},
    {"0bd145fb86a6f0911daad713e4657267", "avg_abandoned_town_map"},
    {"0c21bc86d29968a2f34c179fc1508254", "avg_port_novorossiysk_map"},
    {"0e2aeb9b0f707a93bf5de004ac7b0592", "avg_red_desert_tankmap"},
    {"0ef474f4b903af8a5eb63491b8550b19", "berlin_map"},
    {"0fc7a539a2a43273dd148c60c8cf4753", "avg_karpaty_passage_tankmap"},
    {"1025d80f56e7052218207e7bbc798d5b", "avg_abandoned_factory_tankmap"},
    {"12721672295c2df4d1bf4e7ba7866c14", "avg_nuclear_incident_tankmap"},
    {"130ddf1aecd35fbfc5c629a391332311", "avn_san_francisco_map"},
    {"13108f28c821cab71bc3e6bfb97b80a1", "caribbean_islands_map"},
    {"134217777d322bb2868bceacb777829d", "arcade_ireland_map"},
    {"13ae3d26f8909b9ed8d3600cc73d4035", "avg_sweden_tankmap"},
    {"140612e861e12b6452465e770b93786b", "avg_arctic_map"},
    {"1615d579ea243bfe3f033514c7da94dc", "avn_fuego_islands_tankmap"},
    {"17f7b421a4a47dfd4dd00677054e9408", "avg_egypt_sinai_map"},
    {"1bd53f4aabaa6cb5c9d438024a123d72", "avg_karantan_tankmap"},
    {"1c1574893e33acd07ebd895406e4207c", "avn_blacksea_port_tankmap"},
    {"1c9f5478f6bcf2a92a8cff2793067a5b", "avg_syria_tankmap"},
    {"1e998b92efc3e4164dcea44e1c3b9ee9", "avg_breslau_tankmap"},
    {"1eae8e60936c97f1c0ab748860ac2014", "avg_finland_map"},
    {"1ed2b7f58ca6c4f02a7ce18e7fcd525d", "avn_finland_islands_map"},
    {"1f85741573393cf924c4c5b68c685b13", "avn_aleutian_islands_map"},
    {"1ff2bdbb3e46a1bf46b0819da08adcd4", "avg_poland_tankmap"},
    {"20787c13eca540ebffc06b698de53bd4", "kursk_map"},
    {"21328e0f728c257e34a49e38f7d29878", "hurtgen_map"},
    {"2620cbb8feb7dd2ca6bd4156948280b1", "avg_tunisia_desert_map"},
    {"28de686d23f714ccdea5ab284a114eb8", "arcade_tabletop_mountain_map"},
    {"29a8b8c1dbc82957d6c7eab823357576", "avn_mediterranean_port_map"},
    {"2a35e8117adf4ee0c9291a184922223f", "avg_vietnam_hills_tankmap"},
    {"2c03c46bc63d8b7b4f7bac555c6cc4e6", "avn_northwestern_islands_tankmap"},
    {"2f912b742369e10ff3d7173d78874e44", "avg_soviet_suburban_tankmap"},
    {"30c1fe632e8fdfc7fec03b0e3850d645", "avg_sector_montmedy_tankmap"},
    {"357e768ca0507b7bd0a02027c3db9f63", "khalkhin_gol_map"},
    {"36e2c818a61aeb5d7426ae64b7f5da1e", "avg_american_valley_map"},
    {"3779e8e4c5ddb371d7a05ee727017d12", "avn_england_shore_tankmap"},
    {"37e83779836e3154d0df664784118b76", "avn_blacksea_port_map"},
    {"3808171ada81e54d9df1df82a7d8ca8e", "avg_container_port_tankmap"},
    {"382abaa73b6214a2d54f0f627cfa510a", "avg_arctic_tankmap"},
    {"383a67cb44721ccb4fbacf823d50325c", "arcade_mediterranean_map"},
    {"384eaa356d3519286c64dfa7c0434def", "avn_peleliu_map"},
    {"39b43683923fd8ec6cf7fc7aebd1e9f3", "air_grand_canyon_map"},
    {"3a71abf2eb97998fdb9920cd0b715957", "avg_stalingrad_factory_tankmap"},
    {"3d1e35a9f022048bf1129ac3bcb67ecd", "avn_volcanic_island_tankmap"},
    {"3e2fcaa6fd69bde845eb86ccaf510047", "air_normandy_map"},
    {"3e2fcaa6fd69bde845eb86ccaf510047", "avg_normandy_map"},
    {"40a76fcc38906520503e4d857e7fae53", "britain_map"},
    {"41cd2051d75d84da96f71ddd8296594a", "water_map"},
    {"41de30cdda3e66fd99f5fc05e84135ed", "avn_new_zealand_tankmap"},
    {"420f671ac1da3f3c4a0a3f45601a95bf", "avg_iberian_castle_map"},
    {"444666456c0c3ba6467b0ec2cc5ef0d4", "air_afghan_map"},
    {"479b196499160cb1d85cc62444bec460", "arcade_phiphi_crater_rocks_map"},
    {"47df9983b077556261dfb2025f44ab5f", "air_equatorial_island_map"},
    {"4857ba05d2e5e777e7fdeed0f8da380b", "avg_soviet_suburban_snow_map"},
    {"4909fb9f7fc71ee00f724647a377bf1f", "air_denmark_map"},
    {"497ac1c829f17c0ace6b09b5d34ea330", "avg_alaska_town_tankmap"},
    {"49b15d24afdb4822b71dfaa1006711cb", "arcade_norway_fjords_map"},
    {"49de495f801476a4d991871fbdd18e9f", "avn_san_francisco_tankmap"},
    {"4a3958f9054e2806d3961446a43bc5ae", "mozdok_winter_map"},
    {"4bc33aa2288e7b14301eb1eecb2cb649", "stalingrad_w_map"},
    {"4cf4fe8b47b77c4fceb24b01c9864ebd", "avg_berlin_tankmap"},
    {"4f60912966f190ff5417f1276070cebb", "avg_eastern_europe_map"},
    {"4fc1c1d395df96df0e974724995e3826", "avn_fiji_map"},
    {"50351ca283885a7975450437d7470b49", "avn_bering_sea_map"},
    {"505c0ff368cb4c100739c0d473676bdd", "avg_mozdok_tankmap"},
    {"51c5f192b804fb25eec9137cacd3a212", "honolulu_map"},
    {"53aac54f3800751fffe34c37441a7c70", "avn_finland_islands_tankmap"},
    {"54a40f3d8d51e57862a41b9ac1354156", "avn_sunken_city_map"},
    {"551ef55eb350e96af4a79b5c11751148", "avn_ice_port_tankmap"},
    {"565647da74c6a28b00b081b9458502f8", "stalingrad_map"},
    {"571333320b9d0566f7340b9b54bbf079", "arcade_norway_plain_map"},
    {"580071d8b24f3c8ededd523f8faefa3a", "avn_japan_map"},
    {"588cb74f45092bd41d9b608ad9386496", "bulge_map"},
    {"58e66bf943fd38f6c59223d4af12a393", "avn_north_sea_tankmap"},
    {"5987ab2f0a31f361c88ea6dc3d33319a", "avg_rheinland_tankmap"},
    {"5a365a2e5e52d15a1a63a0a9bab60f0f", "guam_map"},
    {"5ab38c6a634c0f5a8dbba58bc64316e9", "avg_netherlands_map"},
    {"5c338343b2d3d92dbfafbec0f3e31242", "avg_alaska_town_map"},
    {"5c82c3af2ad118f4de0f1fba164c9546", "avn_peleliu_tankmap"},
    {"5dc95706d4f9709d2a7aed4c40d0acaf", "air_ladoga_map"},
    {"5ecc43c09b70b97aa79e38c2b2ca7f7e", "avn_ice_port_map"},
    {"5f04242ab1827894d70da6d49384b67b", "avn_south_africa_map"},
    {"6040a8e1e26edfeff763952dcf00326c", "avg_finland_tankmap"},
    {"62104b22757b822043bc18bbbc79ca68", "avg_vlaanderen_tankmap"},
    {"624a4a239a77b571995080f57221ab55", "avg_snow_alps_map"},
    {"638e3f71cd5266d662b1bfdcffb94152", "avg_lazzaro_italy_map"},
    {"64a336ba16504369be488cc14a86bdd1", "avg_vlaanderen_map"},
    {"64ce388918ba55a4d0392dd8ba0c15f6", "avg_sector_montmedy_snow_tankmap"},
    {"672274ac8cf53a5e80bc5f76ff2d6d0b", "avg_western_europe_map"},
    {"68d5917bd7e3318581eb81f06b407757", "krymsk_map"},
    {"6aff62dfc314ba777314ddac25dbb6de", "avg_africa_desert_tankmap"},
    {"6b6f642330ccc72bc66614ff6a7d82de", "air_israel_map"},
    {"6dda0356201f4e2138523f25173dcc79", "avn_volcanic_island_map"},
    {"6f091b4910cb32071af25f1c50ae935e", "avg_aral_sea_map"},
    {"70557b79691e9a266175d95b0de387cb", "avn_arabian_north_coast_tankmap"},
    {"71c520ee6382947e050a719f24c643f6", "avg_karelia_forest_a_tankmap"},
    {"73a51bf32033983ca8f791753d571efa", "avg_aral_sea_tankmap"},
    {"73da84dbc33cdc10868c5b0983e8d331", "avn_northwestern_islands_map"},
    {"75fd359c6e264f42149f7e85e42205fc", "avg_soviet_suburban_snow_tankmap"},
    {"7603590c21808de1cc190fdbc37e0c62", "sicily_map"},
    {"76a4088f48a4ebfa1257755217927199", "avg_lazzaro_italy_new_city_tankmap"},
    {"773d655d6ee90b95db02850ddbf40eee", "avg_soviet_suburban_map"},
    {"77955020c701c08976372dd170fac352", "air_kamchatka_map"},
    {"7b66c072e9740dae6c3b001e9a0f609d", "avg_sweden_map"},
    {"7c2ec3f5efc01018d189850a941a54bc", "arcade_africa_seashore_map"},
    {"7cdf6c029520b2635457df1ba78b3ae1", "avg_soviet_range_map"},
    {"7d3f2a66d3131f9c51bb570b7d400776", "guadalcanal_map"},
    {"7e576bd561728517d6e65912f1d7003f", "moscow_map"},
    {"824890f85c5110de2e88552f51e7f3af", "avg_ardennes_map"},
    {"83ed160b188bd934e32cba29cb6dbf42", "avg_port_novorossiysk_tankmap"},
    {"840b074a7c70db65675a1ee691a0aa32", "avn_fuego_islands_map"},
    {"8610b18b82cf2ecaac5c68a9472ecaf4", "avn_mediterranean_port_tankmap"},
    {"873b3914462c5d1bbab5d30621a6b34c", "ruhr_map"},
    {"877008f0b69f01cc8bd299d83c472288", "midway_map"},
    {"878163fe7a4e6c101e509d82eb87f812", "avg_tunisia_desert_tankmap"},
    {"87e0e4be01fbb92f6f965a16840502fe", "avg_japan_tankmap"},
    {"8ac9c8128f9e4ff0fad28cdad638506c", "avg_european_fortress_tankmap"},
    {"8ba25d865df04e53ff640f3140392976", "iwo_jima_map"},
    {"8ca00c2726f8c216ef24a6e912c17582", "avn_bering_sea_tankmap"},
    {"8e1d64024a7955a773398aad29349c38", "arcade_africa_canyon_map"},
    {"903ef3516d8609b984d24f7970dff242", "avg_soviet_range_tankmap"},
    {"95253a353f3f024597489e9ae8e9946d", "avg_vietnam_hills_map"},
    {"9632fabe71bc7fd150af57aed7679b06", "air_skyscraper_city_map"},
    {"9b4e2ffa11822bd530301d1302c6370b", "avg_hurtgen_map"},
    {"9bd7a1f14acddbe43157d8becf02b693", "avg_maginot_map"},
    {"9beaa42ba4c02f60a428cbdedd62c186", "avn_new_zealand_map"},
    {"9c8901c2167955a8c9c7ba94d06a30ad", "zhengzhou_map"},
    {"9d5dd1ef4449e6ac6934ce27302ceece", "avg_abandoned_town_tankmap"},
    {"9dfd7909a63779cea248107e1f109a87", "avg_normandy_tankmap"},
    {"9e7b93e3e92de1cc351096ed9f6936bd", "avg_breslau_map"},
    {"9f6ab620922c54c53f93803e917aaa31", "avg_container_port_map"},
    {"a0dc3fffe4b7510954cf9f8dbf2d2c1e", "avn_norway_islands_map"},
    {"a41debdda976632cf419be9002219e78", "avg_poland_map"},
    {"a572aa8e7417cec99b76a422f28a32c0", "air_pyrenees_map"},
    {"a574cefca25e0e7caf83fdfbdf2fe420", "avn_alps_fjord_tankmap"},
    {"a597329c7bd97a7cd32df8da24468eeb", "avn_japan_tankmap"},
    {"a8a0383729d54dc9f56df9fadd5c0e89", "avn_norway_islands_tankmap"},
    {"aa07a01da4c36b4907b8bb99604ff100", "peleliu_map"},
    {"ab2ebc7f0fb6031ac188414290fde017", "avn_africa_gulf_tankmap"},
    {"ac5af3b2c95279c070d173fc6590a61b", "saipan_map"},
    {"adc3182a806f014e078a10cbff01fe34", "air_africa_desert_map"},
    {"adc3182a806f014e078a10cbff01fe34", "avg_africa_desert_map"},
    {"af735e9a24bc8e502f15ad62483244c8", "avn_phang_nga_bay_islands_map"},
    {"afc4179322b83a3a610ba7d0c219debd", "arcade_phiphi_crater_map"},
    {"aff8ad661fe6f15972b606ca34cf8a2a", "avg_northern_valley_map"},
    {"b1705af18887b5bad14351b9300d1ee8", "avg_greece_tankmap"},
    {"b1d8f38b2b201551506191cc5c27d5d4", "avg_hurtgen_tankmap"},
    {"b2bed64077f42c2f33a59c3a76251533", "avg_karpaty_passage_map"},
    {"b52675396dcd555c50ecfc4e0d90c319", "avg_snow_alps_tankmap"},
    {"b6c30825bdb754d3eaa9ea03fb9e6a58", "air_vietnam_map"},
    {"b72b975278477fb46cdf1b2f16296098", "port_moresby_map"},
    {"b7613d5ae8ba3b1eb811c9d776eb2a7e", "arcade_norway_green_map"},
    {"b80e2cd472707bb43adac41947ef2d7f", "avg_egypt_sinai_tankmap"},
    {"ba1553ccec566a56d482f29a692928a6", "avg_volokolamsk_map"},
    {"bb9c20d37da0f256c1171f4521a2a5bb", "dover_strait_map"},
    {"bc8795b7c0d152cb4d1cd7b9f6f315f2", "avg_korea_lake_tankmap"},
    {"bf50d6a6dd6be06aaabcc04d8a2b6ada", "avg_rheinland_map"},
    {"bfa3d455139f58b5f4e1c7fbc388c400", "avg_japan_map"},
    {"c14393285bac5fd4d8a6124765880c6c", "avg_western_europe_tankmap"},
    {"c14ab450e733363b1e8b1de80265ceb5", "avg_poland_snow_tankmap"},
    {"c3250c0f0bac923e7026d22508838e0e", "avn_franz_josef_land_tankmap"},
    {"c476694b71cb433befaf7662671e551a", "avg_european_fortress_map"},
    {"c57a518a8079441a77222547ae1de255", "avn_north_sea_map"},
    {"c73ccdf9a79088d41575441aaf80037c", "avn_alps_fjord_map"},
    {"c8691b31cf35074135a7348b7d3c7e5c", "korea_map"},
    {"c8b744b4bd01701c359d4e4519936833", "wake_island_map"},
    {"c915529a9b99c26febc5651fa9d23e48", "avg_northern_india_tankmap"},
    {"c98d5bb1caa50456028c866fd8ecd14e", "norway_map"},
    {"cb2ce1700097b46c12164265e63d0ed0", "avg_abandoned_factory_map"},
    {"cbea0fc91f94500ebd6b040e2b164169", "avg_poland_snow_map"},
    {"cf10ff5db873953e05f3e5a821fa3663", "avn_arabian_north_coast_map"},
    {"d04623aebe0aa86becb5d2c771f79166", "avg_karelia_forest_a_map"},
    {"d07411a928b96b3c1fd4ec284a1d2f85", "avn_ireland_bay_map"},
    {"d1919b6bf6008e05f002e07fcdf37620", "icon"},
    {"d2dd6437dd7f2e527d82952a8b963dc4", "avn_aleutian_islands_tankmap"},
    {"d51e66f7d5a73c4abb5dcd561004f3ab", "avg_ardennes_snow_map"},
    {"d5262d10238d670e65b7c81bd201824a", "avn_coral_islands_tankmap"},
    {"d6b2723e298ab7d99e5f2c6f3d73606b", "avg_ireland_tankmap"},
    {"d787387fb1c68e7a8a14f0f2480f97c6", "air_south_eastern_city_map"},
    {"d78c668da5372787a6ba3d9a37f12c3b", "avn_phang_nga_bay_islands_tankmap"},
    {"d81c02f1de90b4bc52ae82115da56089", "arcade_rice_terraces_map"},
    {"d9f38628de0473f7223997e70f3f5b65", "avg_football_field_map"},
    {"da3426af2d571c979c78ae8c2673d61b", "avn_coral_islands_map"},
    {"da791e3cb6071bdb5fdf25feda925aff", "arcade_snow_rocks_map"},
    {"dc46b7361db3e2a52841fc8d88322dbc", "avg_iberian_castle_tankmap"},
    {"dce28fd7c797f9fa43c8ffa9f964a025", "arcade_asia_4roads_map"},
    {"dd6fe8541416cf547d57c926866112a8", "air_race_phiphi_islands_map"},
    {"de7fdd8cb441a559e1d61f88808094e4", "avn_fiji_tankmap"},
    {"dec7623f80c90b713da4be39f820b45f", "avg_fulda_tankmap"},
    {"e04b520d5ffa2aa3aac8d692ab990a5a", "avg_krymsk_tankmap"},
    {"e33605946d387b50199de0b41c231e2f", "firing_range_tankmap"},
    {"e4c3e8d5de42bf8f8a181b3cf247764c", "avg_maginot_tankmap"},
    {"e517b9c7d0384753044a6e1aabed1162", "avg_training_ground_tankmap"},
    {"e7fad10320ec940e4858182716d0a8d8", "avg_israel_tankmap"},
    {"e9f77da97af38249a2c77543a12e737b", "avn_south_africa_tankmap"},
    {"eacbc4cb4dfa80ad2285ab6e118e1211", "avg_guadalcanal_tankmap"},
    {"ebb7935e1cb083319d96a32a79e7b66e", "mozdok_map"},
    {"ed291b0ed753d9d57eaf7d6c8a29ecf3", "korsun_map"},
    {"ee789f8b8fee3fa64ec387ec9ce92db8", "avg_ardennes_snow_tankmap"},
    {"efc5c656fcc73e76fe7c651a572bc23b", "avn_africa_gulf_map"},
    {"f131de106f26a584cbd4749db3c9c823", "avg_future_city_map"},
    {"f1b4562664d578a9c6ca49438a3ac203", "avg_northern_india_map"},
    {"f221d8ccb048357bc50a525788c22632", "avn_sunken_city_tankmap"},
    {"f2d32e423db66c17b945f81cdddb3c7d", "avg_volokolamsk_tankmap"},
    {"f4253d19183260e930df9d89106e885f", "avg_american_valley_tankmap"},
    {"f4ab313031b2755ac6c1862da41d2fb5", "avg_ardennes_tankmap"},
    {"f586ef9c10a1d5defa4b763fb5a87bb7", "avg_northern_valley_tankmap"},
    {"f5e27615cd364aff7889cc3903572adf", "avn_franz_josef_land_map"},
    {"f6dfd8e95fa5f996d56b4571e7a02a16", "avg_red_desert_map"},
    {"f72923a22f82b66f0426418670368f3c", "avn_ireland_bay_tankmap"},
    {"f78f90e37d54493748241e1883be55f9", "spain_map"},
    {"f980a77f9300a15759fc07d0c71c60c7", "air_mysterious_valley_map"},
    {"fa36acda1cb9c0533b1f972552e808e1", "avg_eastern_europe_tankmap"},
    {"fb0437aee01abb3afd91c7405b40a840", "arcade_zhang_park_map"},
    {"fb1ea2704e232a98514913fadd5abd83", "avg_kursk_villages_tankmap"},
    {"fb7e0fede86ee05dd0c5ad9c2ded053e", "avg_korea_lake_map"},
    {"fcedaf50df60dd4b1754838ac275ce67", "arcade_alps_map"},
    {"ff23d1bbc96b3c68014477c797124b3e", "air_smolensk_map"},
};


    static std::thread worker;
    static std::atomic<bool> running{false};
    static std::mutex mu;
    static std::condition_variable cv;
    static std::deque<ServerStatus> queue;
    static UpdateCallback user_cb = nullptr;
    static std::chrono::milliseconds interval = std::chrono::milliseconds(2000);

    static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp) {
        const size_t realSize = size * nmemb;
        const auto s = static_cast<std::string *>(userp);
        s->append(static_cast<char *>(contents), realSize);
        return realSize;
    }

    // Parse indicators JSON (from /indicators)
    static void parse_indicators(const std::string &buf, ServerStatus &st, bool &indicators_valid) {
        indicators_valid = false;
        if (buf.empty()) return;
        try {
            auto j = json::parse(buf);
            indicators_valid = j.value("valid", false);
            st.unit = j.value("type", "");
            st.unitCrew = j.value("crew_current", 0); //TODO: use getparty.size?
            st.unitCrewTotal = j.value("crew_total", 0);
            st.unitSpeed = j.value("speed", 0);
        } catch (const std::exception &e) {
            app_log::error(std::string("status_thread: indicators JSON parse error: ") + e.what());
        }
    }

    // Parse map_info JSON (from /map_info.json)
    static void parse_map_info(const std::string &buf, ServerStatus &st, bool &map_valid) {
        map_valid = false;
        if (buf.empty()) return;
        try {
            auto j = json::parse(buf);
            map_valid = j.value("valid", false);
            // Attempt to read map hash if provided
            if (j.contains("map_hash") && j["map_hash"].is_string()) {
                std::string hash = j["map_hash"].get<std::string>();
                auto it = mapNameCache.find(hash);
                if (it != mapNameCache.end()) {
                    st.map = it->second;
                }
            }
        } catch (const std::exception &e) {
            app_log::error(std::string("status_thread: map_info JSON parse error: ") + e.what());
        }
    }

    void worker_fn() {
        CURL *curl = curl_easy_init();
        if (!curl) {
            app_log::error("status_thread: curl_easy_init failed");
            return;
        }

        // Configure curl defaults
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, 1500L);

        const char *urls[] = {"http://127.0.0.1:8111/indicators", "http://127.0.0.1:8111/map_info.json"};
        const size_t IDX_INDICATORS = 0;
        const size_t IDX_MAPINFO = 1;
        size_t url_count = sizeof(urls) / sizeof(urls[0]);

        const char *map_img_url = "http://127.0.0.1:8111/map.img";

        while (running.load()) {
            std::string buffers[2];
            bool fetched_any = false;

            for (size_t i = 0; i < url_count; ++i) {
                buffers[i].clear();
                curl_easy_setopt(curl, CURLOPT_URL, urls[i]);
                curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
                curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffers[i]);

                CURLcode res = curl_easy_perform(curl);
                if (res == CURLE_OK) {
                    long http_code = 0;
                    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
                    if (http_code >= 200 && http_code < 300 && !buffers[i].empty()) {
                        fetched_any = true;
                    }
                }
            }

            std::string mapImgBuf;
            bool got_map_img = false;
            // fetch map image (binary)
            mapImgBuf.clear();
            curl_easy_setopt(curl, CURLOPT_URL, map_img_url);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &mapImgBuf);
            CURLcode r = curl_easy_perform(curl);
            if (r == CURLE_OK) {
                long http_code = 0;
                curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
                if (http_code >= 200 && http_code < 300 && !mapImgBuf.empty()) {
                    got_map_img = true;
                }
            }

            if (fetched_any || got_map_img) {
                ServerStatus st;
                st.online = false;
                st.map.clear();
                st.unit.clear();
                st.unitCrew = 0;
                st.unitCrewTotal = 0;
                st.unitSpeed = 0;

                bool indicators_valid = false;
                bool map_valid = false;

                parse_indicators(buffers[IDX_INDICATORS], st, indicators_valid);
                parse_map_info(buffers[IDX_MAPINFO], st, map_valid);

                // If we fetched the raw image, compute MD5 and lookup in mapNameCache
                if (got_map_img) {
                    std::string hash = md5_hex(reinterpret_cast<const uint8_t*>(mapImgBuf.data()), mapImgBuf.size());
                    auto it = mapNameCache.find(hash);
                    if (it != mapNameCache.end()) {
                        st.map = it->second;
                    } else {
                        // not found: attempt to insert placeholder (hex) to cache for later manual mapping
                        mapNameCache.emplace(hash, "unknown_map_" + hash.substr(0, 8));
                        st.map = "unknown_map_" + hash.substr(0, 8);
                    }
                }

                // map_info.json == valid:false && indicators == valid:true -> player in hangar or lobby
                // map_info.json == valid:true && indicators == valid:true -> player is in match
                st.online = indicators_valid && map_valid;

                if (indicators_valid && map_valid) {
                    st.playState = ServerStatus::PlayState::InMatch;
                } else if (indicators_valid && !map_valid) {
                    st.playState = ServerStatus::PlayState::InHangar;
                } else {
                    st.playState = ServerStatus::PlayState::Unknown;
                }

                {
                    std::lock_guard<std::mutex> lk(mu);
                    queue.push_back(st);
                    if (queue.size() > 16) queue.pop_front();
                }
                cv.notify_all();

                if (user_cb) {
                    try {
                        user_cb(st);
                    } catch (const std::exception &e) {
                        app_log::error(std::string("status_thread: user callback threw: ") + e.what());
                     }
                 }
             } else {
                // Log intermittently
                static int fail_count = 0;
                fail_count++;
                if (fail_count % 10 == 0) {
                    app_log::warn("status_thread: failed to query status endpoint");
                 }
             }

            // Sleep with early exit
            std::unique_lock<std::mutex> lk(mu);
            cv.wait_for(lk, interval, [] { return !running.load(); });
        }

        curl_easy_cleanup(curl);
    }

    void start_status_thread(UpdateCallback cb, std::chrono::milliseconds poll_interval) {
        if (running.load()) return;
        user_cb = cb;
        interval = poll_interval;
        running.store(true);
        worker = std::thread(worker_fn);
    }

    void stop_status_thread() {
        if (!running.load()) return;
        running.store(false);
        cv.notify_all();
        if (worker.joinable()) worker.join();
    }

    bool try_pop_status(ServerStatus &out) {
        std::lock_guard<std::mutex> lk(mu);
        if (queue.empty()) return false;
        out = queue.front();
        queue.pop_front();
        return true;
    }
} // namespace status_thread
