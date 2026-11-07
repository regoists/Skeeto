#pragma once
// Immutable menu catalog + bounded, allocation-free matching. Search runs only
// after a query/category change on Paint; it never reads game state or files.
#include <algorithm>
#include <cstddef>
#include <cstring>
#include <cwchar>

namespace skmenufind {
struct Target { int page, sub, row; };
struct Entry {
    const wchar_t *nameZh, *nameEn, *descriptionZh, *descriptionEn;
    const wchar_t *pathZh, *pathEn, *aliases;
    Target target;
    bool localOnly, experimental;
};
static constexpr int kQueryLimit = 64;
inline bool Space(wchar_t c) { return c == L' ' || c == L'\t' || c == L'\r' || c == L'\n' || c == 0x3000; }
inline wchar_t Fold(wchar_t c) { return c >= L'A' && c <= L'Z' ? c + (L'a' - L'A') : c; }
inline bool Equal(const wchar_t* a, const wchar_t* b) {
    while (*a && *b && Fold(*a) == Fold(*b)) { ++a; ++b; }
    return !*a && !*b;
}
inline bool Contains(const wchar_t* text, const wchar_t* word) {
    for (; *text; ++text) {
        const wchar_t* at = text; const wchar_t* token = word;
        while (*at && *token && Fold(*at) == Fold(*token)) { ++at; ++token; }
        if (!*token) return true;
    }
    return !*word;
}
inline int TokenRank(const Entry& entry, const wchar_t* token) {
    if (Contains(entry.nameZh, token) || Contains(entry.nameEn, token)) return 1;
    if (Contains(entry.aliases, token)) return 2;
    if (Contains(entry.descriptionZh, token) || Contains(entry.descriptionEn, token)
        || Contains(entry.pathZh, token) || Contains(entry.pathEn, token)) return 3;
    return -1;
}
struct Result { int index, rank; };
template<std::size_t N> struct Index {
    const Entry (&entries)[N];
    Result results[N]{};
    int count = 0;
    wchar_t query[kQueryLimit + 1]{};
    explicit Index(const Entry (&source)[N]) : entries(source) {}
    bool Search(const wchar_t* text) {
        if (!text) return false;
        std::size_t length = 0;
        while (length <= kQueryLimit && text[length]) ++length;
        if (length > kQueryLimit) return false;
        wchar_t normalized[kQueryLimit + 1]{}; int used = 0;
        for (std::size_t i = 0; i < length; ++i) {
            if (Space(text[i])) { if (used && normalized[used - 1] != L' ') normalized[used++] = L' '; }
            else normalized[used++] = text[i];
        }
        if (used && normalized[used - 1] == L' ') normalized[--used] = 0;
        std::memcpy(query, normalized, (used + 1) * sizeof(wchar_t));
        wchar_t tokens[kQueryLimit + 1]{}; std::memcpy(tokens, normalized, (used + 1) * sizeof(wchar_t));
        const wchar_t* words[(kQueryLimit + 1) / 2]{}; int wordCount = 0;
        for (int i = 0; i < used;) {
            words[wordCount++] = tokens + i;
            while (i < used && tokens[i] != L' ') ++i;
            if (i < used) tokens[i++] = 0;
        }
        count = 0;
        for (std::size_t i = 0; i < N; ++i) {
            const auto& entry = entries[i];
            int rank = 0;
            for (int j = 0; j < wordCount; ++j) {
                const int next = TokenRank(entry, words[j]);
                if (next < 0) { rank = -1; break; }
                rank += next;
            }
            if (rank < 0) continue;
            if (used && (Equal(entry.nameZh, normalized) || Equal(entry.nameEn, normalized))) rank = 0;
            results[count++] = { static_cast<int>(i), rank };
        }
        std::sort(results, results + count, [](const Result& a, const Result& b) {
            return a.rank != b.rank ? a.rank < b.rank : a.index < b.index;
        });
        return true;
    }
};
}
