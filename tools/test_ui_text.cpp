#include "../src/skeeto/core/skeeto_ui_text.h"
#include <cassert>
#include <cwchar>
#include <cstring>

int main() {
    assert(UiParseSetting("auto") == 0);
    assert(UiParseSetting("zh") == 1);
    assert(UiParseSetting("en") == 2);
    assert(UiResolveEnglish(0, nullptr) == false);
    assert(!UiResolveEnglish(0, "russian"));
    assert(!UiResolveEnglish(0, "schinese"));
    assert(!UiResolveEnglish(0, "tchinese"));
    assert(!UiResolveEnglish(0, "english_extra"));
    assert(UiResolveEnglish(0, "english"));
    assert(UiResolveEnglish(2, "schinese"));
    const wchar_t* fallback = L"中文";
    static constexpr UiTextList<2> labels = {{
        {UiId::LanguageAuto, L"自动"}, {UiId::Count, L"original"}
    }};
    g_uiEnglish.store(false);
    assert(std::wcscmp(labels[0], L"自动") == 0);
    assert(UiText(UiId::Count, fallback) == fallback);
    assert(UiHudLanguage(1) == 1);
    assert(UiHudLanguage(2) == 1);
    assert(UiHudLanguage(0) == 0);
    g_uiEnglish.store(true);
    assert(std::wcscmp(labels[0], L"Auto") == 0);
    assert(std::wcscmp(labels[1], L"original") == 0);
    assert(UiText(UiId::Count, fallback) == fallback);
    assert(UiHudLanguage(0) == 0);
    assert(UiHudLanguage(1) == 1);
    assert(UiHudLanguage(2) == 0);
    assert(UiNormalizeHudLanguage(-1) == 1);
    assert(UiNormalizeHudLanguage(3) == 1);
    assert(UiNormalizeHudLanguage(2) == 2);
    return 0;
}
