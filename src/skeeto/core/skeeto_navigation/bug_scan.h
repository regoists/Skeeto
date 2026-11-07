#pragma once
// Listen-host bug-point scan over the NAV file this module already parses.
// Pink quads are areas a start-area walk did not reach. They are connectivity
// suspects, not confirmed god spots. No server.dll NavMesh ABI is called.

static constexpr int kBugNearest = 48;
static constexpr float kBugDrawReach = 3200.f;

struct BugScanState {
	bool enabled = false;
	bool boxes = true;
	unsigned seenSerial = 0;
	int phase = 0;
	unsigned cursor = 0;
	unsigned start = nav::Invalid;
	unsigned startId = 0;
	int startRank = 4;
	unsigned suspects = 0;
	unsigned tally = 0;
	unsigned visited = 0;
	unsigned char* seen = nullptr;
	unsigned* queue = nullptr;
	unsigned qh = 0, qt = 0, cap = 0;
	unsigned* ladHead = nullptr;
	unsigned* ladNext = nullptr;
	unsigned* ladTo = nullptr;
	unsigned ladUsed = 0, ladSlots = 0, ladCursor = 0;
	unsigned nearest[kBugNearest]{};
	int nearestN = 0;
	unsigned buildAt[kBugNearest]{};
	float buildD[kBugNearest]{};
	int buildN = 0;
	bool nearReady = false;
	bool havePlayer = false;
	bool bestValid = false;
	float bestX = 0, bestY = 0, bestZ = 0;
	Vec player{};
	wchar_t line[96]{};
};

static BugScanState g_bug;

static void BugDrop() {
	HANDLE heap = GetProcessHeap();
	if (g_bug.seen) HeapFree(heap, 0, g_bug.seen);
	if (g_bug.queue) HeapFree(heap, 0, g_bug.queue);
	if (g_bug.ladHead) HeapFree(heap, 0, g_bug.ladHead);
	if (g_bug.ladNext) HeapFree(heap, 0, g_bug.ladNext);
	if (g_bug.ladTo) HeapFree(heap, 0, g_bug.ladTo);
	g_bug.seen = nullptr;
	g_bug.queue = g_bug.ladHead = g_bug.ladNext = g_bug.ladTo = nullptr;
	g_bug.cap = g_bug.qh = g_bug.qt = g_bug.ladUsed = g_bug.ladSlots = g_bug.ladCursor = 0;
	g_bug.nearestN = g_bug.buildN = 0;
	g_bug.nearReady = false;
	g_bug.bestValid = false;
	g_bug.visited = g_bug.suspects = g_bug.tally = 0;
}

static void BugOnMapCleared() {
	BugDrop();
	g_bug.phase = 0;
	g_bug.cursor = 0;
	g_bug.seenSerial = 0;
	g_bug.start = nav::Invalid;
	g_bug.startId = 0;
	g_bug.startRank = 4;
	g_bug.line[0] = 0;
}

static void BugConsider(unsigned area, float dist) {
	if (g_bug.buildN >= kBugNearest) {
		int worst = 0;
		for (int i = 1; i < g_bug.buildN; ++i)
			if (g_bug.buildD[i] > g_bug.buildD[worst]) worst = i;
		if (dist >= g_bug.buildD[worst]) return;
		g_bug.buildAt[worst] = area;
		g_bug.buildD[worst] = dist;
		return;
	}
	g_bug.buildAt[g_bug.buildN] = area;
	g_bug.buildD[g_bug.buildN] = dist;
	++g_bug.buildN;
}

static void BugPublishNearest() {
	g_bug.nearestN = g_bug.buildN;
	for (int i = 0; i < g_bug.buildN; ++i) g_bug.nearest[i] = g_bug.buildAt[i];
	g_bug.suspects = g_bug.tally;
	g_bug.nearReady = true;
	g_bug.bestValid = false;
	if (g_bug.havePlayer && g_bug.buildN > 0) {
		float best = 1e30f;
		unsigned pick = g_bug.buildAt[0];
		for (int i = 0; i < g_bug.buildN; ++i) {
			const unsigned area = g_bug.buildAt[i];
			if (area >= mesh.count) continue;
			const Vec delta = guidance::Center(mesh.areas[area]) - g_bug.player;
			const float dist = delta.Dot(delta);
			if (dist < best || (dist == best && mesh.areas[area].id < mesh.areas[pick].id)) {
				best = dist;
				pick = area;
			}
		}
		if (pick < mesh.count) {
			const Vec center = guidance::Center(mesh.areas[pick]);
			g_bug.bestX = center.x;
			g_bug.bestY = center.y;
			g_bug.bestZ = center.z + 8.f;
			g_bug.bestValid = true;
		}
	}
	if (!g_bug.suspects)
		wcsncpy_s(g_bug.line, UiText(UiId::LocalBugStatusNone, L"这张图上没有找到"), _TRUNCATE);
	else
		swprintf_s(g_bug.line, UiText(UiId::LocalBugStatusReady, L"找到 %u 处"), g_bug.suspects);
}

static bool BugAlloc() {
	BugDrop();
	const unsigned n = mesh.count;
	if (!n || n > nav::MaxAreas) return false;
	HANDLE heap = GetProcessHeap();
	g_bug.seen = (unsigned char*)HeapAlloc(heap, HEAP_ZERO_MEMORY, n);
	g_bug.queue = (unsigned*)HeapAlloc(heap, 0, n * sizeof(unsigned));
	g_bug.ladSlots = mesh.ladderLinkCount * 2;
	g_bug.ladHead = (unsigned*)HeapAlloc(heap, 0, n * sizeof(unsigned));
	if (g_bug.ladSlots) {
		g_bug.ladNext = (unsigned*)HeapAlloc(heap, 0, g_bug.ladSlots * sizeof(unsigned));
		g_bug.ladTo = (unsigned*)HeapAlloc(heap, 0, g_bug.ladSlots * sizeof(unsigned));
	}
	if (!g_bug.seen || !g_bug.queue || !g_bug.ladHead || (g_bug.ladSlots && (!g_bug.ladNext || !g_bug.ladTo))) {
		BugDrop();
		return false;
	}
	for (unsigned i = 0; i < n; ++i) g_bug.ladHead[i] = nav::Invalid;
	g_bug.cap = n;
	return true;
}

static void BugPushLad(unsigned from, unsigned to) {
	if (from >= g_bug.cap || to >= g_bug.cap || from == to || g_bug.ladUsed >= g_bug.ladSlots) return;
	const unsigned slot = g_bug.ladUsed++;
	g_bug.ladTo[slot] = to;
	g_bug.ladNext[slot] = g_bug.ladHead[from];
	g_bug.ladHead[from] = slot;
}

static void BugStep() {
	if (!g_bug.enabled) return;
	if (!mesh.count || parser.phase) {
		g_bug.nearReady = false;
		wcsncpy_s(g_bug.line, LocalizedLoadStatus(), _TRUNCATE);
		return;
	}
	if (g_bug.seenSerial != meshSerial) {
		BugDrop();
		g_bug.phase = 1;
		g_bug.cursor = 0;
		g_bug.start = nav::Invalid;
		g_bug.startId = 0;
		g_bug.startRank = 4;
		g_bug.seenSerial = meshSerial;
		g_bug.nearReady = false;
		wcsncpy_s(g_bug.line, UiText(UiId::LocalBugStatusScan, L"正在查找"), _TRUNCATE);
	}
	int index = VCall<int(__thiscall*)(void*)>(engine, 12)(engine);
	int team = 0;
	Vec position{};
	g_bug.havePlayer = Player(index, position, team, true);
	if (g_bug.havePlayer) g_bug.player = position;

	unsigned budget = 768;
	if (g_bug.phase == 1) {
		if (g_bug.cursor > mesh.count) g_bug.cursor = 0;
		while (g_bug.cursor < mesh.count && budget) {
			const nav::Area& area = mesh.areas[g_bug.cursor];
			int rank = 4;
			if (area.spawn & guidance::PlayerStart) rank = 0;
			else if (area.spawn & guidance::Checkpoint) rank = 1;
			else if (area.spawn & guidance::Rescue) rank = 2;
			if (rank < 4 && (rank < g_bug.startRank || (rank == g_bug.startRank && area.id < g_bug.startId))) {
				g_bug.startRank = rank;
				g_bug.startId = area.id;
				g_bug.start = g_bug.cursor;
			}
			++g_bug.cursor;
			--budget;
		}
		if (g_bug.cursor < mesh.count) return;
		if (g_bug.start == nav::Invalid) {
			g_bug.phase = 9;
			wcsncpy_s(g_bug.line, UiText(UiId::LocalBugStatusNoStart, L"这张图没有可用的起点"), _TRUNCATE);
			return;
		}
		if (!BugAlloc() || g_bug.cap != mesh.count) {
			g_bug.phase = 9;
			wcsncpy_s(g_bug.line, UiText(UiId::LocalBugStatusNoNav, L"这张图没有导航数据"), _TRUNCATE);
			return;
		}
		g_bug.ladCursor = 0;
		g_bug.phase = mesh.ladderLinkCount ? 3 : 4;
		if (g_bug.phase == 4) {
			g_bug.seen[g_bug.start] = 1;
			g_bug.queue[0] = g_bug.start;
			g_bug.qh = 0;
			g_bug.qt = 1;
			g_bug.visited = 1;
		}
		return;
	}
	if (g_bug.phase == 3) {
		while (g_bug.ladCursor < mesh.ladderLinkCount && budget) {
			const nav::LadderLink& link = mesh.ladderLinks[g_bug.ladCursor++];
			BugPushLad(link.from, link.to);
			BugPushLad(link.to, link.from);
			--budget;
		}
		if (g_bug.ladCursor < mesh.ladderLinkCount) return;
		g_bug.seen[g_bug.start] = 1;
		g_bug.queue[0] = g_bug.start;
		g_bug.qh = 0;
		g_bug.qt = 1;
		g_bug.visited = 1;
		g_bug.phase = 4;
		return;
	}
	if (g_bug.phase == 4) {
		while (g_bug.qh < g_bug.qt && budget) {
			const unsigned at = g_bug.queue[g_bug.qh++];
			const nav::Area& area = mesh.areas[at];
			for (unsigned e = 0; e < area.edgeCount; ++e) {
				const unsigned to = mesh.edges[area.firstEdge + e];
				if (to >= g_bug.cap || g_bug.seen[to]) continue;
				g_bug.seen[to] = 1;
				g_bug.queue[g_bug.qt++] = to;
				++g_bug.visited;
			}
			for (unsigned slot = g_bug.ladHead[at]; slot != nav::Invalid; slot = g_bug.ladNext[slot]) {
				const unsigned to = g_bug.ladTo[slot];
				if (to >= g_bug.cap || g_bug.seen[to]) continue;
				g_bug.seen[to] = 1;
				g_bug.queue[g_bug.qt++] = to;
				++g_bug.visited;
			}
			--budget;
		}
		if (g_bug.qh < g_bug.qt) {
			wcsncpy_s(g_bug.line, UiText(UiId::LocalBugStatusScan, L"正在查找"), _TRUNCATE);
			return;
		}
		g_bug.cursor = 0;
		g_bug.buildN = 0;
		g_bug.tally = 0;
		g_bug.phase = 5;
		return;
	}
	if (g_bug.phase == 5) {
		if (!g_bug.cursor) {
			g_bug.buildN = 0;
			g_bug.tally = 0;
		}
		while (g_bug.cursor < mesh.count && budget) {
			const unsigned i = g_bug.cursor++;
			--budget;
			if (g_bug.seen[i] || !guidance::Walkable(mesh.areas[i])) continue;
			++g_bug.tally;
			if (!g_bug.havePlayer) continue;
			const Vec delta = guidance::Center(mesh.areas[i]) - g_bug.player;
			BugConsider(i, delta.Dot(delta));
		}
		if (g_bug.cursor < mesh.count) return;
		BugPublishNearest();
		g_bug.cursor = 0;
		g_bug.buildN = 0;
		g_bug.tally = 0;
	}
}

static void BugPaint() {
	if (!g_bug.enabled || !g_bug.boxes || uiShown || !g_bug.nearReady || !mesh.count || g_bug.seenSerial != meshSerial) return;
	int width = 0, height = 0;
	VCall<void(__thiscall*)(void*, int&, int&)>(engine, 5)(engine, width, height);
	if (width < 640 || height < 400 || width > 16384 || height > 16384) return;
	const auto& matrix = VCall<const Matrix&(__thiscall*)(void*)>(engine, 37)(engine);
	worldnav::Camera camera;
	if (!Readable(&matrix, sizeof matrix) || !camera.Set(matrix, width, height)) return;
	static unsigned font = 0;
	static int fontSize = 0;
	const int pixels = 16;
	if (!font) font = VCall<unsigned(__thiscall*)(void*)>(surface, 63)(surface);
	if (font && fontSize != pixels && VCall<bool(__thiscall*)(void*, unsigned, const char*, int, int, int, int, int, int, int)>(surface, 64)(surface, font, "Microsoft YaHei", pixels, 500, 0, 0, 0x90, 0, 0xffff))
		fontSize = pixels;
	auto line = [&](worldnav::Point a, worldnav::Point b) {
		VCall<void(__thiscall*)(void*, int, int, int, int)>(surface, 11)(surface, 255, 105, 180, 220);
		VCall<void(__thiscall*)(void*, int, int, int, int)>(surface, 15)(surface, int(a.x), int(a.y), int(b.x), int(b.y));
	};
	const float reach = kBugDrawReach * kBugDrawReach;
	for (int i = 0; i < g_bug.nearestN; ++i) {
		const unsigned index = g_bug.nearest[i];
		if (index >= mesh.count) continue;
		const nav::Area& area = mesh.areas[index];
		const Vec center = guidance::Center(area);
		const Vec delta = center - g_bug.player;
		if (g_bug.havePlayer && delta.Dot(delta) > reach) continue;
		const Vec corner[4] = {
			{area.x1, area.y1, nav::Height(area, area.x1, area.y1) + 6.f},
			{area.x2, area.y1, nav::Height(area, area.x2, area.y1) + 6.f},
			{area.x2, area.y2, nav::Height(area, area.x2, area.y2) + 6.f},
			{area.x1, area.y2, nav::Height(area, area.x1, area.y2) + 6.f},
		};
		worldnav::Point screen[4];
		bool ok[4];
		for (int c = 0; c < 4; ++c) ok[c] = camera.Project(corner[c], screen[c]);
		for (int c = 0; c < 4; ++c) {
			const int n = (c + 1) & 3;
			if (ok[c] && ok[n]) line(screen[c], screen[n]);
		}
	}
	if (font && g_bug.line[0]) {
		VCall<void(__thiscall*)(void*, unsigned)>(surface, 17)(surface, font);
		VCall<void(__thiscall*)(void*, int, int, int, int)>(surface, 19)(surface, 255, 150, 190, 230);
		VCall<void(__thiscall*)(void*, int, int)>(surface, 20)(surface, 16, height - 42);
		int n = 0;
		while (g_bug.line[n] && n < 90) ++n;
		VCall<void(__thiscall*)(void*, const WCHAR*, int, int)>(surface, 22)(surface, g_bug.line, n, 0);
	}
}

static bool BugNearestPoint(float* x, float* y, float* z) {
	if (!x || !y || !z || !g_bug.bestValid) return false;
	*x = g_bug.bestX;
	*y = g_bug.bestY;
	*z = g_bug.bestZ;
	return true;
}
