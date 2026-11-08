// ============================================================================
// Skeeto UI text: immutable DLL-owned strings; no engine calls or allocation.
// Language is published by the game thread; readers may run on other threads.
// ============================================================================
#pragma once
#include <atomic>
#include <cstddef>
#include <cstring>

inline std::atomic_bool g_uiEnglish{false};

inline int UiParseSetting(const char* value) {
    if (!value) return 0;
    if (std::strcmp(value, "zh") == 0 || std::strcmp(value, "chinese") == 0) return 1;
    if (std::strcmp(value, "en") == 0 || std::strcmp(value, "english") == 0) return 2;
    return 0;
}
inline const char* UiSettingName(int setting) {
    return setting == 1 ? "zh" : setting == 2 ? "en" : "auto";
}
inline bool UiResolveEnglish(int setting, const char* language) {
    return setting == 2 || (setting == 0 && language && std::strcmp(language, "english") == 0);
}
inline int UiNormalizeHudLanguage(int value) {
    return value >= 0 && value <= 2 ? value : 1;
}
inline int UiHudLanguage(int setting) {
    if (setting == 0) return 0;
    if (setting == 1) return 1;
    return g_uiEnglish.load(std::memory_order_relaxed) ? 0 : 1;
}

enum class UiId : std::size_t {
#define SKEETO_UI(id, text) id,
#include "skeeto_ui_en.inc"
#undef SKEETO_UI
    Count
};
inline const wchar_t* UiText(UiId id, const wchar_t* chinese) {
    if (!g_uiEnglish.load(std::memory_order_relaxed)) return chinese;
    static constexpr const wchar_t* english[] = {
#define SKEETO_UI(id, text) text,
#include "skeeto_ui_en.inc"
#undef SKEETO_UI
        nullptr
    };
    const std::size_t index = static_cast<std::size_t>(id);
    return index < static_cast<std::size_t>(UiId::Count) && english[index] && english[index][0]
        ? english[index] : chinese;
}

struct UiTextEntry { UiId id; const wchar_t* chinese; };
template<std::size_t N> struct UiTextList {
    UiTextEntry entries[N];
    constexpr std::size_t size() const { return N; }
    const wchar_t* operator[](std::size_t index) const {
        return UiText(entries[index].id, entries[index].chinese);
    }
};
