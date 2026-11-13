#include "../src/skeeto/core/skeeto_navigation/nav_incremental.h"
#include "../src/skeeto/core/skeeto_navigation/guidance.h"
#include <cstdio>
#include "../src/skeeto/core/skeeto_navigation/controls.h"
#include <vector>

static void Put8(std::vector<unsigned char>& out, unsigned value) {
	out.push_back((unsigned char)value);
}

static void Put16(std::vector<unsigned char>& out, unsigned value) {
	Put8(out, value);
	Put8(out, value >> 8);
}

static void Put32(std::vector<unsigned char>& out, unsigned value) {
	Put16(out, value);
	Put16(out, value >> 16);
}

static void PutFloat(std::vector<unsigned char>& out, float value) {
	unsigned bits = 0;
	memcpy(&bits, &value, sizeof(bits));
	Put32(out, bits);
}

static std::vector<unsigned char> MakeNav(unsigned connections) {
	std::vector<unsigned char> bytes;
	Put32(bytes, 0xfeedface);
	Put32(bytes, 16);
	Put32(bytes, 14);
	Put32(bytes, 0);
	Put8(bytes, 0);
	Put16(bytes, 0);
	Put8(bytes, 0);
	Put8(bytes, 0);
	Put16(bytes, 0);
	Put32(bytes, 1);
	Put32(bytes, 1);
	Put32(bytes, 0);
	for (int i = 0; i < 8; ++i) PutFloat(bytes, i == 3 || i == 4 ? 100.f : 0.f);
	for (int direction = 0; direction < 4; ++direction) {
		const unsigned count = direction == 0 ? connections : 0;
		Put32(bytes, count);
		for (unsigned i = 0; i < count; ++i) Put32(bytes, 1);
	}
	Put8(bytes, 0);  // hiding spots
	Put32(bytes, 0); // encounters
	Put16(bytes, 0); // place and ladder flags
	Put32(bytes, 0); // ladder refs, up
	Put32(bytes, 0); // ladder refs, down
	for (int i = 0; i < 24; ++i) Put8(bytes, 0);
	Put32(bytes, 0); // spawn attributes
	Put16(bytes, 0);
	Put32(bytes, 0); // visible areas
	Put32(bytes, 0); // subversion 11+ unknown area data
	Put32(bytes, 0); // ladder count
	return bytes;
}

static bool ParserRespectsEdgeBudget() {
	const unsigned budget = 32;
	auto bytes = MakeNav(2048);
	nav::Mesh mesh;
	nav::ParseJob parser;
	if (parser.Begin(mesh, bytes.data(), (unsigned)bytes.size()) != nav::ParseJob::Result::Working)
		return false;
	parser.Step(256, budget);
	const bool bounded = mesh.edgeCount <= budget;
	auto result = nav::ParseJob::Result::Working;
	for (unsigned steps = 0; steps < 1000 && result == nav::ParseJob::Result::Working; ++steps)
		result = parser.Step(256, budget);
	bool parsed = result == nav::ParseJob::Result::Ready && mesh.count == 1 && mesh.edgeCount == 2048;
	for (unsigned i = 0; parsed && i < mesh.edgeCount; ++i) parsed = mesh.edges[i] == 0;
	parser.Cancel();
	mesh.Clear();
	return bounded && parsed;
}

static bool GraphPreparationRespectsEdgeBudget() {
	const unsigned budget = 32;
	nav::Mesh mesh;
	mesh.areas = (nav::Area*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 2 * sizeof(nav::Area));
	mesh.edges = (unsigned*)HeapAlloc(GetProcessHeap(), 0, 2048 * sizeof(unsigned));
	if (!mesh.areas || !mesh.edges) {
		mesh.Clear();
		return false;
	}
	mesh.count = 2;
	mesh.edgeCount = mesh.edgeCapacity = 2048;
	mesh.areas[0] = nav::Area{ 1, 0, 0, 0, 0, 0, 100, 100, 0, 0, 0, 0, 2048 };
	mesh.areas[1] = nav::Area{ 2, 0, 0, 100, 0, 0, 200, 100, 0, 0, 0, 2048, 0 };
	for (unsigned i = 0; i < mesh.edgeCount; ++i) mesh.edges[i] = 1;
	guidance::Navigator navigator;
	if (!navigator.BeginPrepare(mesh, Vec{})) {
		navigator.Clear();
		mesh.Clear();
		return false;
	}
	navigator.PrepareStep(mesh, budget);
	const bool bounded = navigator.linkCount <= budget;
	for (unsigned steps = 0; steps < 1000 && !navigator.prepared; ++steps)
		navigator.PrepareStep(mesh, budget);
	const bool prepared = navigator.prepared && navigator.linkCount == mesh.edgeCount;
	navigator.Clear();
	mesh.Clear();
	return bounded && prepared;
}

// Mesh borrows these fixture arrays; Navigator owns only its prepared graph.
struct RoutingFixture {
	nav::Area areas[6]{};
	unsigned edges[4]{1, 2, 3, 3};
	nav::Ladder ladders[2]{};
	nav::LadderLink ladderLinks[2]{{0, 3, 0, 0}, {4, 3, 1, 1}};
	nav::Mesh mesh;
	guidance::Navigator navigator;
	RoutingFixture() {
		areas[0] = {1, 0, guidance::PlayerStart | guidance::Checkpoint, 0, 0, 0, 100, 100, 0, 0, 0, 0, 1};
		areas[1] = {2, 0, 0, 100, 0, 48, 200, 100, 48, 48, 48, 1, 1};
		areas[2] = {3, 0, 0, 100, 100, 96, 200, 200, 96, 96, 96, 2, 1};
		areas[3] = {4, 0, guidance::Checkpoint, 0, 100, 128, 100, 200, 128, 128, 128, 3, 0};
		areas[4] = {5, 0, 0, 300, 0, 400, 400, 100, 400, 400, 400, 3, 0};
		areas[5] = {6, 0, 0, 600, 0, 800, 700, 100, 800, 800, 800, 3, 0};
		ladders[0] = {10, 32, {50, 100, 128}, {50, 100, 0}, 128, 0, {}, 0};
		ladders[1] = {11, 32, {350, 50, 400}, {50, 150, 128}, 420, 0, {}, 0};
		mesh.areas = areas; mesh.count = 6; mesh.edges = edges; mesh.edgeCount = mesh.edgeCapacity = 3;
		mesh.ladders = ladders; mesh.ladderCount = 2; mesh.ladderLinks = ladderLinks; mesh.ladderLinkCount = 2;
	}
	~RoutingFixture() { navigator.Clear(); }
	bool Prepare() {
		if (!navigator.BeginPrepare(mesh, {-1000, -1000, 0})) return false;
		for (unsigned i = 0; i < 500 && !navigator.prepared; ++i) navigator.PrepareStep(mesh, 1);
		if (!navigator.prepared) return false;
		navigator.Select(navigator.nodes[3].group + 1); return true;
	}
	bool Finish() {
		for (unsigned i = 0; i < 500 && navigator.building; ++i) {
			const unsigned before = navigator.tail;
			navigator.Step(mesh, 1);
			if (navigator.tail > navigator.capacity || navigator.tail > before + 1) return false;
		}
		return !navigator.building;
	}
};

static bool GroundRouteWinsAndRequiredLadderSurvives() {
	RoutingFixture f;
	if (!f.Prepare()) return false;
	auto& n = f.navigator; n.measureLength = true;
	n.Begin(f.mesh, {50, 50, 0});
	if (!f.Finish() || n.state != guidance::State::Route || n.routeLadders || n.routeUnusual || n.routeCount != 4) return false;
	if (n.route[0] != 0 || n.route[1] != 1 || n.route[2] != 2 || n.route[3] != 3 || n.routeLength <= 0) return false;
	// After the ground search has finished, moving to a ladder-only component
	// must resume search once, not permanently report "no path".
	n.Begin(f.mesh, {350, 50, 400});
	if (!f.Finish() || n.state != guidance::State::Route || n.routeLadders != 1 || n.routeCount != 2) return false;
	n.Begin(f.mesh, {50, 50, 0});
	if (n.state != guidance::State::Route || n.routeLadders || n.routeCount != 4) return false;
	n.Begin(f.mesh, {650, 50, 800});
	if (!f.Finish() || n.state != guidance::State::DirectionOnly || n.routeCount) return false;
	const unsigned generation = n.generation;
	for (unsigned i = 0; i < 20; ++i) { n.Begin(f.mesh, {650, 50, 800}); if (n.building || n.generation != generation) return false; }
	n.Select(1); n.Begin(f.mesh, {50, 50, 0});
	return f.Finish() && n.state == guidance::State::AtGoal && n.routeCount == 1 && !n.routeLadders;
}

static bool NonLadderSpecialConnectionWins() {
	RoutingFixture f;
	f.areas[0].spawn = guidance::PlayerStart; // Keep the direct edge outside checkpoint grouping.
	f.areas[0].firstEdge = 3; // Direct 128-unit rise is the existing unusual edge.
	f.mesh.edgeCount = f.mesh.edgeCapacity = 4;
	if (!f.Prepare()) return false;
	f.navigator.Begin(f.mesh, {50, 50, 0});
	return f.Finish() && f.navigator.state == guidance::State::Route &&
		f.navigator.routeLadders == 0 && f.navigator.routeUnusual == 1;
}

static bool RequiredLadderWorksFromFirstLocation() {
	RoutingFixture f;
	if (!f.Prepare()) return false;
	f.navigator.Begin(f.mesh, {350, 50, 400});
	return f.Finish() && f.navigator.state == guidance::State::Route &&
		f.navigator.routeLadders == 1 && f.navigator.routeCount == 2;
}

int main() {
	const controls::Settings defaults{};
	if (defaults.refreshFps != 10 || controls::specs[controls::RefreshIndex].maximum != 120) {
		std::fprintf(stderr, "NAV refresh default/range changed unexpectedly\n"); return 6;
	}
	if (!ParserRespectsEdgeBudget()) {
		std::fprintf(stderr, "NAV parser exceeded one-step edge budget\n");
		return 1;
	}
	if (!GraphPreparationRespectsEdgeBudget()) {
		std::fprintf(stderr, "NAV graph preparation exceeded one-step edge budget\n");
		return 2;
	}
	if (!GroundRouteWinsAndRequiredLadderSurvives()) {
		std::fprintf(stderr, "NAV must prefer ground detours, retain required ladders and resume without repeated search\n"); return 3;
	}
	if (!NonLadderSpecialConnectionWins()) {
		std::fprintf(stderr, "NAV unusual ground edges must precede ladder shortcuts\n"); return 4;
	}
	if (!RequiredLadderWorksFromFirstLocation()) {
		std::fprintf(stderr, "NAV must retain necessary ladders at the initial location\n"); return 5;
	}
	std::puts("NAV work budgets and ground-before-ladder routing passed");
	return 0;
}
