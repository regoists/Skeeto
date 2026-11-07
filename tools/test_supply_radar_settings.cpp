#include <cstdio>
#include <cstring>
#include "../src/skeeto/core/skeeto_navigation/sdk.h"
#include "../src/skeeto/core/skeeto_navigation/supply_radar/settings.h"
#define CHECK(x) do { if (!(x)) { std::printf("FAIL line %d: %s\n", __LINE__, #x); return 1; } } while (0)
int main() {
    using namespace supplyradar;
    options::Reset();
    CHECK(options::Count == 190 && options::ItemBase == 15 && options::LayoutBase == 50);
    CHECK(options::values[options::Enable] == 0); // the radar master switch defaults off
    CHECK(options::AnyEnabled());
    CHECK(options::Supported(24) && options::Supported(25) && options::Supported(32) && options::Supported(33) && options::Supported(34));
    CHECK(!options::Supported(10) && !options::Supported(22) && !options::Supported(30) && !options::Supported(35));
    // New world supplies start disabled; everything else supported starts enabled.
    CHECK(!options::EnabledId(32) && !options::EnabledId(33) && options::EnabledId(34) && options::EnabledId(8) && options::EnabledId(9));
    CHECK(options::values[options::Range] == 40 && options::values[options::Rows] == 8 && options::values[options::Columns] == 2);
    options::Set(options::Range, 999); CHECK(options::values[options::Range] == 120);
    options::Set(options::Range, 1); CHECK(options::values[options::Range] == 5);
    options::Set(options::Medical, 0); CHECK(!options::EnabledId(1)); CHECK(options::EnabledId(5));
    options::Set(options::ItemBase + 5, 0); CHECK(!options::EnabledId(5));
    options::Set(options::ItemBase + 32, 1); CHECK(options::EnabledId(32)); // ammo-style items need no category switch
    options::Set(options::ItemBase + 10, 1); CHECK(options::values[options::ItemBase + 10] == 0); // unsupported labels stay off
    options::Set(options::LayoutBase + 4 + 3, 999); CHECK(options::values[options::LayoutBase + 7] == 200);
    options::Set(options::LayoutBase + 4 + 3, 1); CHECK(options::values[options::LayoutBase + 7] == 50);
    options::Set(options::LayoutBase + 4 + 1, 99999); CHECK(options::values[options::LayoutBase + 5] == 10000);
    options::Set(options::DigitFont, 9); CHECK(options::DigitStyle() == 2);
    options::Set(-1, 1); options::Set(options::Count, 1); // out of range writes are ignored
    char keys[options::Count][80]{};
    for (int i = 0; i < options::Count; ++i) {
        CHECK(options::Key(i)); std::strcpy(keys[i], options::Key(i));
        CHECK(std::strncmp(keys[i], "supply_radar_", 13) == 0);
        for (int j = 0; j < i; ++j) CHECK(std::strcmp(keys[i], keys[j]) != 0);
        const int v = options::values[i]; options::Set(i, v); CHECK(options::values[i] == v);
    }
    CHECK(options::Key(-1) == nullptr && options::Key(options::Count) == nullptr);
    std::puts("PASS supply radar defaults, category isolation, bounds and unique setting keys"); return 0;
}
