#include "../src/skeeto/core/skeeto_navigation/minimap_ui.h"
#include <cstdio>

using namespace minimap;

static float Distance(Point p) {
	const float dx = p.x - Center, dy = p.y - Center;
	return Root(dx * dx + dy * dy);
}

static bool RootIsAccurate() {
	for (float x = 0.25f; x < 200000.f; x *= 1.7f) {
		const float r = Root(x), error = r * r - x;
		if ((error < 0 ? -error : error) > x * 1e-4f) return false;
	}
	return Root(0) == 0;
}

static bool MaskIsDisc() {
	const DiscMask& mask = Disc();
	for (int y = 0; y < Pixels; ++y)
		for (int x = 0; x < Pixels; ++x) {
			const int at = y * Pixels + x;
			const float d = Distance({x + .5f, y + .5f});
			if (d > Rim + EdgeAa * .5f + .75f && (mask.alpha[at] || mask.kind[at] != DiscMask::Outside)) return false;
			if (d < Rim - EdgeAa * .5f - .75f && mask.alpha[at] != 255) return false;
			if (d < Rim - RimFade - .75f && mask.kind[at] != DiscMask::Map) return false;
		}
	return mask.kind[0] == DiscMask::Outside && mask.kind[(Pixels / 2) * Pixels + Pixels / 2] == DiscMask::Map;
}

static bool ClampStaysInside() {
	const Point samples[] = {{-5000, 30}, {160, -900}, {9000, 9000}, {170, 160}, {Center, Center}};
	for (const Point p : samples) {
		bool clamped = false;
		const Point at = ClampShape(p, 0, TeamRadius, &clamped);
		if (Distance(at) > TeamRadius + .01f) return false;
		if (clamped != (Distance(p) > TeamRadius)) return false;
		// Square: the clamped point is a fixed point of the clamp and stays in the box.
		bool again = false; (void)again;
		const Point sq = ClampShape(p, 1, TeamRadius, &again), sq2 = ClampShape(sq, 1, TeamRadius, &again);
		if (Distance({sq2.x - sq.x + Center, sq2.y - sq.y + Center}) > .01f || sq.x < Center - TeamRadius - .01f || sq.x > Center + TeamRadius + .01f || sq.y < Center - TeamRadius - .01f || sq.y > Center + TeamRadius + .01f) return false;
	}
	return true;
}

static View MakeView(bool rotate, float yaw, int style, int shape = 0) {
	View v;
	v.shape = shape;
	v.player = {100, 200, 0};
	v.yaw = yaw;
	v.range = 1200;
	v.rotate = rotate;
	v.style = style;
	v.Prepare();
	return v;
}

static bool NorthBadgeInsideTicks() {
	for (int shape = 0; shape < 2; ++shape)
		for (int rotate = 0; rotate < 2; ++rotate)
			for (float yaw = -180; yaw <= 180; yaw += 15) {
				View v = MakeView(rotate != 0, yaw, 0, shape);
				for (float scale = 2.f / 3; scale <= 2.f; scale += .25f) {
					v.markerScale = scale;
					const Point n = NorthPoint(v);
					const float b = NorthBadge(scale) + 1;
					for (int k = 0; k < 8; ++k) {
						const float ox = k < 4 ? (k == 0 ? b : k == 1 ? -b : 0) : (k & 1 ? .707f : -.707f) * b, oy = k < 4 ? (k == 2 ? b : k == 3 ? -b : 0) : (k & 2 ? .707f : -.707f) * b;
						bool out = false;
						ClampShape({n.x + ox, n.y + oy}, shape, TickOuter, &out);
						if (out) return false;
					}
				}
			}
	return true;
}

static Raster raster;

static bool ComposeHonoursMask() {
	nav::Mesh mesh;
	const Vec teammates[] = {{100, 200 + 5000, 0}, {100 + 200, 200, 0}};
	for (int shape = 0; shape < 2; ++shape)
	for (int style = 0; style < 2; ++style)
		for (int rotate = 0; rotate < 2; ++rotate) {
			const View v = MakeView(rotate != 0, 37, style, shape);
			raster.Build(mesh, v, teammates, 2);
			const DiscMask& mask = Mask(shape);
			for (int i = 0; i < Pixels * Pixels; ++i) {
				const unsigned alpha = raster.rgba[i] >> 24;
				if (mask.kind[i] == DiscMask::Outside && raster.rgba[i]) return false;
				if (mask.kind[i] == DiscMask::Map && alpha != 255) return false;
				if (mask.kind[i] == DiscMask::Casing && alpha != mask.alpha[i]) return false;
			}
		}
	return true;
}

static bool LabelsStayInsideDisc() {
	Scene scene;
	scene.view = MakeView(true, 90, 0);
	items::Snapshot snapshot;
	const float range = scene.view.range;
	int entity = 1;
	for (float dx = -range * 1.3f; dx <= range * 1.3f; dx += range / 5)
		for (float dy = -range * 1.3f; dy <= range * 1.3f; dy += range / 5) {
			if (snapshot.count >= items::Capacity) break;
			snapshot.markers[snapshot.count++] = {{scene.view.player.x + dx, scene.view.player.y + dy, 0}, items::Kind::Medical, entity++, dx * dx + dy * dy, items::Label::Medical};
		}
	static LabelLayout layout;
	const int sizes[] = {180, 260, 480};
	for (int shape = 0; shape < 2; ++shape) {
		scene.view.shape = shape;
		for (int size : sizes) {
			layout.Clear();
			layout.Build(scene, snapshot, nullptr, size, 14);
			for (int i = 0; i < layout.count; ++i)
				if (!InsideShape(layout.labels[i].box, size, shape, TickOuter - 2)) return false;
			if (!layout.count || !layout.outside) return false;
		}
	}
	return true;
}

int main() {
	if (!RootIsAccurate()) { std::fprintf(stderr, "Root lost precision\n"); return 1; }
	if (!MaskIsDisc()) { std::fprintf(stderr, "Disc mask leaks outside the rim\n"); return 2; }
	if (!ClampStaysInside()) { std::fprintf(stderr, "Disc clamp left the radius\n"); return 3; }
	if (!NorthBadgeInsideTicks()) { std::fprintf(stderr, "North badge crosses the tick ring\n"); return 4; }
	if (!ComposeHonoursMask()) { std::fprintf(stderr, "Compose wrote outside the disc\n"); return 5; }
	if (!LabelsStayInsideDisc()) { std::fprintf(stderr, "Supply labels left the disc\n"); return 6; }
	std::puts("Minimap disc mask, clamp, compose and labels passed");
	return 0;
}
