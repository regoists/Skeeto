#pragma once
// Paint-thread navigation only. No settings writes or menu action dispatch.
#include "skeeto_menu_discovery.h"
#include "skeeto_menu_discovery_descriptions.h"
#include "skeeto_menu_catalog.generated.inc"

static constexpr int kMenuDiscoveryPerPage = 5; // Three readable lines per item.
static int g_menuSearchRows = kMenuDiscoveryPerPage;
static skmenufind::Index<_countof(kMenuDiscoveryEntries)> g_menuSearch(kMenuDiscoveryEntries);
static std::atomic_uint g_menuSearchRequest{0};
static bool g_menuSearchNewRequest = false;
static unsigned g_menuDiscoveryRevision = 0;
static DWORD g_menuDiscoveryNoticeUntil = 0;
static bool g_menuDiscoveryFailed = false;

static bool MenuDiscoveryPage(int page) {
	return page == kMenuPageSearch;
}
static int MenuDiscoveryRows(int page) {
	(void)page;
	return g_menuSearchRows;
}
static int MenuDiscoveryPageCount(int page) {
	(void)page;
	const int rows = MenuDiscoveryRows(kMenuPageSearch);
	return (std::max)(1, (g_menuSearch.count + rows - 1) / rows);
}
static void MenuDiscoveryLayout() {
	if (g_menuPage != kMenuPageSearch) return;
	const int height = MenuDesignLineH();
	const int available = g_menuClientCh > 80 && g_menuScale > 0.01f
		? static_cast<int>((g_menuClientCh - 2 * kMenuMargin) / g_menuScale)
		: kMenuDesignH;
	const int header = 48 + height * 2 + 4;
	const int footer = MenuFooterReserve() + height + 20;
	const int rows = (std::max)(1, (std::min)(kMenuDiscoveryPerPage, (available - header - footer) / (3 * height + 6)));
	if (g_menuSearchRows != rows) {
		const int first = g_menuSub * g_menuSearchRows;
		g_menuSearchRows = rows;
		g_menuSub = first / rows;
		++g_menuDiscoveryRevision;
	}
}
static void MenuDiscoveryNotice(bool failed) {
	g_menuDiscoveryFailed = failed;
	g_menuDiscoveryNoticeUntil = GetTickCount() + 4000;
	MenuMarkDirty();
}
static bool MenuDiscoveryRequest(bool newSearch) {
	if (g_menuSearchRequest) { MenuDiscoveryNotice(false); return false; }
	wchar_t title[96]{}; char caption[512]{};
	swprintf_s(title, L"Skeeto - %s", UiText(UiId::FindSearchTitle, L"搜索功能"));
	WideCharToMultiByte(CP_UTF8, 0, title, -1, caption, _countof(caption), nullptr, nullptr);
	const auto ticket = MainUiServicesText(g_menuSearch.query, caption, skmenufind::kQueryLimit);
	if (!ticket) { MenuDiscoveryNotice(false); return false; }
	g_menuSearchRequest = ticket;
	g_menuSearchNewRequest = newSearch;
	if (newSearch) { g_menuPage = kMenuPageSearch; g_menuSub = 0; g_menuPagePrev = g_menuPage; }
	MenuMarkDirty();
	return true;
}
static void MenuDiscoveryRestore(const MenuReturnFrame& frame) {
	if (frame.parentPage == kMenuPageSearch && wcscmp(g_menuSearch.query, frame.discoveryQuery)) {
		g_menuSearch.Search(frame.discoveryQuery);
		++g_menuDiscoveryRevision;
	}
	g_menuDiscoveryAnchor = MenuDiscoveryPage(frame.parentPage) ? -1 : frame.discoveryAnchor;
}
static bool MenuDiscoveryOwnsFocus(HWND foreground) {
	return MainUiServicesTextOwnsWindow(g_menuSearchRequest.load(std::memory_order_relaxed), reinterpret_cast<std::uintptr_t>(foreground));
}
static void MenuDiscoveryCancelInput() {
	MainUiServicesCancelText(g_menuSearchRequest.exchange(0, std::memory_order_acq_rel));
}
static void MenuDiscoveryPump() {
	if (!g_menuSearchRequest) return;
	if (g_menuVisible && g_menuPage == kMenuPageSearch && MenuDiscoveryOwnsFocus(GetForegroundWindow())) g_menuParked = false;
	if (!g_menuVisible || g_menuPage != kMenuPageSearch) { MenuDiscoveryCancelInput(); return; }
	wchar_t text[skmenufind::kQueryLimit + 1]{};
	const auto result = MainUiServicesTakeText(g_menuSearchRequest, text, _countof(text));
	// Worker can park a main-menu view using the previous input-ready snapshot
	// just after the dialog closes. Consume a completed result before parking
	// hides the view; only Paint owns query/navigation state. A parked view
	// also retains its request if the service lock is momentarily occupied.
	if (result == MainUiTextResult::None) return;
	g_menuSearchRequest = 0;
	if (result == MainUiTextResult::Accepted) {
		g_menuSearch.Search(text);
		g_menuSub = 0;
		++g_menuDiscoveryRevision;
	} else {
		if (g_menuSearchNewRequest) {
			const bool parked = g_menuParked; g_menuParked = false;
			MenuProcessKey(0); g_menuParked = parked;
		}
		if (result == MainUiTextResult::Failed) MenuDiscoveryNotice(true);
	}
	MenuMarkDirty();
}
static void MenuDiscoveryJump(int index) {
	const auto& target = kMenuDiscoveryEntries[index].target;
	g_menuPage = target.page;
	g_menuPagePrev = target.page;
	g_menuSub = target.sub;
	g_navMenuSel = NavMenuPageFirstSetting(target.page, target.sub);
	g_radarMenuSel = RadarMenuSettingIndex(target.page, target.sub, 0);
	if (target.page == kMenuPageViewModel) g_vmSelected = ViewModelMenuPageSelection(target.sub);
	if (target.page == kMenuPageSoundVolume) g_soundVolumeMenuSel = 0;
	if (target.page == kMenuPageCareerArchive) CareerArchiveRefresh();
	g_menuDiscoveryAnchor = index;
	MenuMarkDirty();
}
static void MenuDiscoveryKey(int digit) {
	if (digit == 0) {
		if (g_menuSearchRequest) { MainUiServicesCancelText(g_menuSearchRequest); g_menuSearchRequest = 0; }
		g_menuPage = kMenuPageRoot;
		g_menuSub = 0;
		return;
	}
	if (digit == 1) { MenuDiscoveryRequest(false); return; }
	if (g_menuSearchRequest) return;
	const int row = digit - 2;
	const int result = g_menuSub * MenuDiscoveryRows(kMenuPageSearch) + row;
	if (row < 0 || row >= MenuDiscoveryRows(kMenuPageSearch) || result >= g_menuSearch.count) return;
	MenuDiscoveryJump(g_menuSearch.results[result].index);
	MenuMarkDirty();
}

// The cache touches only this page's five entries. It is invalidated by actual
// list/page/language/font/scale changes; Paint never searches or sorts a catalog.
static void MenuDiscoveryFit(wchar_t* text, int width) {
	const int length = static_cast<int>(wcslen(text));
	if (MenuTextDesignW(text) <= width) return;
	int low = 0, high = (std::min)(length, 764);
	wchar_t copy[768]{}; wcsncpy_s(copy, text, _TRUNCATE);
	while (low < high) {
		const int middle = (low + high + 1) / 2;
		text[middle] = 0; wcscat_s(text, 768, L"...");
		if (MenuTextDesignW(text) <= width) low = middle; else high = middle - 1;
		wcsncpy_s(text, 768, copy, _TRUNCATE);
	}
	// Do not split a UTF-16 surrogate pair at the visible boundary.
	if (low && text[low - 1] >= 0xd800 && text[low - 1] <= 0xdbff) --low;
	text[low] = 0; wcscat_s(text, 768, L"...");
}
static int MenuDiscoveryWidth() {
	const int available = g_menuClientCw > 80 && g_menuScale > 0.01f
		? static_cast<int>((g_menuClientCw - 2 * kMenuMargin) / g_menuScale)
		: kMenuDesignW;
	return (std::max)(80, (std::min)(kMenuDesignW, available) - 32);
}
static const wchar_t* MenuDiscoveryHeading(const wchar_t* original) {
	if (g_menuDiscoveryAnchor < 0) return original;
	const auto& entry = kMenuDiscoveryEntries[g_menuDiscoveryAnchor];
	if (entry.target.page != g_menuPage || entry.target.sub != g_menuSub) return original;
	static wchar_t heading[768]{};
	static int anchor = -1, epoch = -1, flags = -1, font = -1, width = -1; static float scale = 0; static bool english = false;
	const bool en = g_uiEnglish.load(std::memory_order_relaxed);
	const int budget = MenuDiscoveryWidth();
	if (anchor != g_menuDiscoveryAnchor || epoch != g_sktFontEpoch || flags != g_menuFontFlags || font != g_uiFont || width != budget || scale != g_menuScale || english != en) {
		anchor = g_menuDiscoveryAnchor; font = g_uiFont; epoch = g_sktFontEpoch; flags = g_menuFontFlags; width = budget; scale = g_menuScale; english = en;
		const wchar_t* format = entry.target.row ? UiText(UiId::FindLocatedRow, L"定位到：%s / 本页 [%d]")
			: UiText(UiId::FindLocatedPage, L"定位到：%s / 设置页");
		wchar_t name[768]{}, suffix[128]{};
		wcsncpy_s(name, en ? entry.nameEn : entry.nameZh, _TRUNCATE);
		if (entry.target.row) swprintf_s(suffix, format, L"", entry.target.row); else swprintf_s(suffix, format, L"");
		MenuDiscoveryFit(name, (std::max)(40, budget - MenuTextDesignW(suffix)));
		if (entry.target.row) swprintf_s(heading, format, name, entry.target.row); else swprintf_s(heading, format, name);
	}
	return heading;
}
template<class Line, class Rule, class Footer, class Pager>
static void MenuDiscoveryPaint(Line line, Rule rule, Footer footer, Pager pager,
	COLORREF title, COLORREF text, COLORREF hint, COLORREF accent) {
	MenuDiscoveryLayout();
	wchar_t buffer[768]{};
	swprintf_s(buffer, L"%s (%d/%d)", UiText(UiId::FindSearchTitle, L"搜索功能"),
		g_menuSub + 1, MenuDiscoveryPageCount(kMenuPageSearch));
	line(10, title, buffer); rule();
	int y = 48, height = MenuDesignLineH();
	{
		static wchar_t queryLine[768]{};
		static unsigned queryRev = ~0u;
		static int queryWidth = -1, queryFont = -1, queryEpoch = -1, queryFlags = -1;
		static float queryScale = 0;
		static bool queryEnglish = false;
		const bool en = g_uiEnglish.load(std::memory_order_relaxed);
		const int budget = MenuDiscoveryWidth();
		if (queryRev != g_menuDiscoveryRevision || queryWidth != budget || queryFont != g_uiFont
			|| queryEpoch != g_sktFontEpoch || queryFlags != g_menuFontFlags || queryScale != g_menuScale || queryEnglish != en) {
			queryRev = g_menuDiscoveryRevision; queryWidth = budget; queryFont = g_uiFont;
			queryEpoch = g_sktFontEpoch; queryFlags = g_menuFontFlags; queryScale = g_menuScale; queryEnglish = en;
			swprintf_s(queryLine, UiText(UiId::FindQuery, L"关键词：%s"),
				g_menuSearch.query[0] ? g_menuSearch.query : UiText(UiId::FindEmptyQuery, L"全部功能"));
			MenuDiscoveryFit(queryLine, budget);
		}
		line(y, hint, queryLine); y += height;
	}
	line(y, accent, UiText(UiId::FindEditQuery, L"[1] 修改关键词（最多 64 字）")); y += height + 4;
	if (g_menuSearchRequest) line(y, hint, UiText(UiId::FindWaiting, L"请在输入窗口确认或取消。"));
	else if (!g_menuSearch.count) line(y, hint, UiText(UiId::FindNoResults, L"没有结果。请换用功能名称或常用叫法。"));
	else {
		static wchar_t cache[kMenuDiscoveryPerPage][3][768]{};
		static unsigned revision = ~0u; static int page = -1, sub = -1, font = -1, epoch = -1, flags = -1, width = -1; static float scale = 0; static bool english = false;
		const bool en = g_uiEnglish.load(std::memory_order_relaxed);
		const int budget = MenuDiscoveryWidth();
		if (revision != g_menuDiscoveryRevision || page != g_menuPage || sub != g_menuSub || font != g_uiFont || epoch != g_sktFontEpoch || flags != g_menuFontFlags || width != budget || scale != g_menuScale || english != en) {
			revision = g_menuDiscoveryRevision; page = g_menuPage; sub = g_menuSub; font = g_uiFont; epoch = g_sktFontEpoch; flags = g_menuFontFlags; width = budget; scale = g_menuScale; english = en;
			for (int row = 0; row < kMenuDiscoveryPerPage; ++row) {
				const int at = g_menuSub * MenuDiscoveryRows(kMenuPageSearch) + row;
				cache[row][0][0] = 0;
				if (at >= g_menuSearch.count || row >= MenuDiscoveryRows(kMenuPageSearch)) continue;
				const auto& item = kMenuDiscoveryEntries[g_menuSearch.results[at].index];
				swprintf_s(cache[row][0], L"[%d]%s%s %s", row + 2,
					item.localOnly ? UiText(UiId::FindLocalTag, L" [仅本地服]") : L"",
					item.experimental ? UiText(UiId::FindExperimentalTag, L" [实验]") : L"",
					en ? item.nameEn : item.nameZh);
				wcsncpy_s(cache[row][1], en ? item.descriptionEn : item.descriptionZh, _TRUNCATE);
				swprintf_s(cache[row][2], UiText(UiId::FindPath, L"位置：%s"), en ? item.pathEn : item.pathZh);
				for (int n = 0; n < 3; ++n) MenuDiscoveryFit(cache[row][n], budget);
			}
		}
		for (int row = 0; row < kMenuDiscoveryPerPage && cache[row][0][0]; ++row) {
			line(y, text, cache[row][0]); y += height;
			line(y, hint, cache[row][1]); y += height;
			line(y, hint, cache[row][2]); y += height + 6;
		}
	}
	footer(); pager(MenuDiscoveryPageCount(kMenuPageSearch));
}
