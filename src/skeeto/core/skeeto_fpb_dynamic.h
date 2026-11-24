#pragma once
// Port of L4D2-Visual-Effects first_person_dynamic_alignment + same-frame camera pose.
// Operates only on caller-owned Studio bone copies; never writes entity caches.
#include "skeeto_fpb.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>

namespace skeeto_fpb {
namespace dynamic {

struct Vec3 { float x = 0, y = 0, z = 0; };
inline Vec3 operator+(Vec3 a, Vec3 b) { return { a.x + b.x, a.y + b.y, a.z + b.z }; }
inline Vec3 operator-(Vec3 a, Vec3 b) { return { a.x - b.x, a.y - b.y, a.z - b.z }; }
inline Vec3 operator*(Vec3 a, float s) { return { a.x * s, a.y * s, a.z * s }; }
inline float Length(Vec3 v) { return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z); }
inline Vec3 Normalize(Vec3 v) {
	const float n = Length(v);
	return n > 1e-6f ? v * (1.f / n) : Vec3{};
}
inline float Dot(Vec3 a, Vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline Vec3 Cross(Vec3 a, Vec3 b) {
	return { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x };
}
inline bool Valid(Vec3 a) {
	return std::isfinite(a.x) && std::isfinite(a.y) && std::isfinite(a.z);
}

struct CameraPose { Vec3 eye, forward, right, up; };

inline bool Inverse4(const float in[16], float out[16]) {
	double rows[4][8]{};
	for (int r = 0; r < 4; ++r)
		for (int c = 0; c < 4; ++c) {
			if (!std::isfinite(in[r * 4 + c])) return false;
			rows[r][c] = in[r * 4 + c];
			rows[r][c + 4] = (r == c) ? 1.0 : 0.0;
		}
	for (int c = 0; c < 4; ++c) {
		int pivot = c;
		for (int r = c + 1; r < 4; ++r)
			if (std::fabs(rows[r][c]) > std::fabs(rows[pivot][c])) pivot = r;
		if (std::fabs(rows[pivot][c]) < 1e-10) return false;
		for (int k = 0; k < 8; ++k) std::swap(rows[pivot][k], rows[c][k]);
		const double d = rows[c][c];
		for (int k = 0; k < 8; ++k) rows[c][k] /= d;
		for (int r = 0; r < 4; ++r) {
			if (r == c) continue;
			const double f = rows[r][c];
			for (int k = 0; k < 8; ++k) rows[r][k] -= f * rows[c][k];
		}
	}
	for (int r = 0; r < 4; ++r)
		for (int c = 0; c < 4; ++c) {
			out[r * 4 + c] = float(rows[r][c + 4]);
			if (!std::isfinite(out[r * 4 + c])) return false;
		}
	return true;
}

inline bool Transform4(const float m[16], Vec3 input, Vec3& out, bool positiveW) {
	const double w = m[12] * input.x + m[13] * input.y + m[14] * input.z + m[15];
	if (!std::isfinite(w) || std::fabs(w) < 1e-6 || (positiveW && w <= 0.001)) return false;
	out = {
		float((m[0] * input.x + m[1] * input.y + m[2] * input.z + m[3]) / w),
		float((m[4] * input.x + m[5] * input.y + m[6] * input.z + m[7]) / w),
		float((m[8] * input.x + m[9] * input.y + m[10] * input.z + m[11]) / w)
	};
	return Valid(out);
}

// Same contract as Visual-Effects dango::cameraPose (worldToView + worldToClip).
inline bool CameraFromMatrices(const float view[16], const float clip[16], CameraPose& pose) {
	if (!view || !clip) return false;
	float viewInv[16], clipInv[16];
	if (!Inverse4(view, viewInv) || !Inverse4(clip, clipInv)) return false;
	if (std::fabs(view[12]) + std::fabs(view[13]) + std::fabs(view[14]) + std::fabs(view[15] - 1.f) > 0.001f)
		return false;
	Vec3 center{}, right{}, up{}, origin{};
	if (!Transform4(viewInv, origin, pose.eye, false)
		|| !Transform4(clipInv, Vec3{ 0, 0, 0.5f }, center, false)
		|| !Transform4(clipInv, Vec3{ 0.1f, 0, 0.5f }, right, false)
		|| !Transform4(clipInv, Vec3{ 0, 0.1f, 0.5f }, up, false))
		return false;
	pose.forward = Normalize(center - pose.eye);
	pose.right = Normalize(right - center);
	pose.up = Normalize(up - center);
	if (Length(pose.forward) < 0.9f || Length(pose.right) < 0.9f || Length(pose.up) < 0.9f)
		return false;
	const float d = Dot(pose.forward, pose.right);
	const float plane = Length({ clip[12], clip[13], clip[14] });
	const float eyeW = clip[12] * pose.eye.x + clip[13] * pose.eye.y + clip[14] * pose.eye.z + clip[15];
	if (plane < 1e-5f || !std::isfinite(eyeW) || std::fabs(eyeW) / plane > 0.1f) return false;
	return std::fabs(d) < 0.05f;
}

inline bool UnitFrame(const CameraPose& pose, Vec3& front, Vec3& right) {
	front = Normalize(Vec3{ pose.forward.x, pose.forward.y, 0.f });
	right = Normalize(Vec3{ pose.right.x, pose.right.y, 0.f });
	if (Length(front) < 0.15f && Length(right) > 0.95f)
		front = Normalize(Cross(Vec3{ 0, 0, 1 }, right));
	if (Length(right) < 0.15f && Length(front) > 0.95f)
		right = Normalize(Cross(front, Vec3{ 0, 0, 1 }));
	return Length(front) > 0.9f && Length(right) > 0.9f && std::fabs(Dot(front, right)) < 0.1f;
}

inline Vec3 BodySpace(Vec3 forward, Vec3 right, float f, float r, float up) {
	return forward * f + right * r + Vec3{ 0, 0, up };
}
inline Vec3 Location(const float m[3][4]) { return { m[0][3], m[1][3], m[2][3] }; }

struct Rig {
	int anchor = -1, head = -1, neck = -1, left = -1, right = -1;
	bool knownEye = false;
	Vec3 eyeFromAnchor{};
};

inline bool Bind(const StudioInfo& studio, int index, Vec3& out) {
	if (index < 0 || index >= studio.count || !studio.bindValid[index]) return false;
	out = { studio.bindLocal[index][0], studio.bindLocal[index][1], studio.bindLocal[index][2] };
	return Valid(out);
}

inline Rig DiscoverRig(const StudioInfo& studio, float fallbackHeight) {
	Rig rig{};
	rig.neck = studio.plan.neck;
	rig.head = studio.plan.head;
	if (studio.count <= 0) return rig;
	if (rig.neck >= 0) {
		for (int n = studio.parents[rig.neck], depth = 0; n >= 0 && depth < studio.count;
			n = studio.parents[n], ++depth) {
			if (n < 0 || n >= studio.count) break;
			const char* name = studio.names[n];
			if (std::strstr(name, "spine") || std::strstr(name, "chest")) {
				rig.anchor = n;
				break;
			}
		}
		if (rig.anchor < 0) rig.anchor = rig.neck;
	}
	if (rig.anchor < 0) {
		for (int i = 0; i < studio.count; ++i) {
			const char* name = studio.names[i];
			if (std::strstr(name, "upperchest") || std::strstr(name, "spine3")
				|| std::strstr(name, "spine4")) {
				rig.anchor = i;
				break;
			}
		}
	}
	if (rig.anchor < 0) return rig;
	for (int i = 0; i < studio.count; ++i) {
		const char* n = studio.names[i];
		const int which = Side(n);
		if ((std::strstr(n, "clavicle") || std::strstr(n, "shoulder")) && which == 1) rig.left = i;
		if ((std::strstr(n, "clavicle") || std::strstr(n, "shoulder")) && which == 2) rig.right = i;
	}
	Vec3 base{}, head{};
	if (!Bind(studio, rig.anchor, base)) return rig;
	if (studio.eyeValid) {
		Vec3 eye{ studio.eyeLocal[0], studio.eyeLocal[1], studio.eyeLocal[2] };
		if (Bind(studio, rig.head, head)
			&& Length(eye - head) < 28.f && eye.z - base.z > 2.f && eye.z - base.z < 65.f) {
			rig.eyeFromAnchor = eye - base;
			rig.knownEye = true;
			return rig;
		}
	}
	if (Bind(studio, rig.head, head)) {
		rig.eyeFromAnchor = head - base + Vec3{ 0, 0, fallbackHeight };
		rig.knownEye = true;
		return rig;
	}
	Vec3 neck{};
	if (Bind(studio, rig.neck, neck)) {
		rig.eyeFromAnchor = neck - base + Vec3{ 0, 0, fallbackHeight };
		rig.knownEye = true;
		return rig;
	}
	return rig;
}

inline bool ModelBasis(const Rig& rig, const float (*bones)[3][4], int count, Vec3& front, Vec3& right) {
	if (!bones || rig.left < 0 || rig.right < 0 || rig.left >= count || rig.right >= count) return false;
	Vec3 left = Location(bones[rig.left]), r = Location(bones[rig.right]);
	Vec3 v = r - left;
	v.z = 0;
	const float width = Length(v);
	if (!std::isfinite(width) || width < 4.f || width > 70.f) return false;
	right = v * (1.f / width);
	front = Normalize(Cross(Vec3{ 0, 0, 1 }, right));
	return Length(front) > 0.9f;
}

inline float Angle(Vec3 from, Vec3 to) {
	return std::atan2(from.x * to.y - from.y * to.x, Dot(from, to));
}

struct Track {
	uint32_t epoch = 0;
	uint64_t tick = 0;
	bool active = false, bootstrapped = false;
	Vec3 lastEye{}, filtered{}, bootstrap{};
	void* raw = nullptr;
	int count = 0;
	void Reset() {
		active = false;
		bootstrapped = false;
		tick = 0;
		filtered = {};
		bootstrap = {};
	}
};

struct Result {
	bool ok = false;
	Vec3 translation{};
	float yawRadians = 0;
};

inline Result Compute(const StudioInfo& studio, const Rig& rig, const float (*bones)[3][4],
	const CameraPose& camera, const Settings& config, const ModelProfile* model,
	uint32_t epoch, uint64_t tick, Track& track) {
	Result out{};
	if (!config.dynamicAnchor || !bones || rig.anchor < 0 || rig.anchor >= studio.count || !Valid(camera.eye))
		return out;
	Vec3 cameraFront{}, cameraRight{};
	if (!UnitFrame(camera, cameraFront, cameraRight)) return out;
	const Vec3 anchor = Location(bones[rig.anchor]);
	if (!Valid(anchor)) return out;
	Vec3 bodyFront = cameraFront, bodyRight = cameraRight;
	const bool haveBody = ModelBasis(rig, bones, studio.count, bodyFront, bodyRight);
	const bool reset = !track.active || track.raw != studio.raw || track.count != studio.count
		|| track.epoch != epoch || tick < track.tick || tick - track.tick > 500
		|| Length(camera.eye - track.lastEye) > 128.f;
	if (reset) {
		track.Reset();
		track.raw = studio.raw;
		track.count = studio.count;
		track.epoch = epoch;
		track.active = true;
		track.lastEye = camera.eye;
		track.tick = tick;
	}
	float radians = 0;
	if (config.followCameraYaw && haveBody)
		radians = std::clamp(Angle(bodyFront, cameraFront), -0.78f, 0.78f);
	const float yawDeg = config.yawTrim + (model ? model->yaw : 0.f);
	radians = std::clamp(radians + yawDeg * 0.01745329252f, -0.90f, 0.90f);
	out.yawRadians = radians;
	Vec3 forward = haveBody ? bodyFront : cameraFront;
	Vec3 right = haveBody ? bodyRight : cameraRight;
	if (haveBody && radians != 0.f) {
		const float cs = std::cos(radians), sn = std::sin(radians);
		forward = { forward.x * cs - forward.y * sn, forward.x * sn + forward.y * cs, 0 };
		right = { right.x * cs - right.y * sn, right.x * sn + right.y * cs, 0 };
	}
	Vec3 relative = rig.eyeFromAnchor;
	if (!rig.knownEye) {
		if (!track.bootstrapped) {
			const Vec3 baseline = camera.eye - anchor;
			track.bootstrap = { Dot(baseline, forward), -Dot(baseline, right), baseline.z };
			track.bootstrapped = true;
		}
		relative = track.bootstrap;
	}
	const Vec3 worldEyeOffset = BodySpace(forward, right, relative.x, -relative.y, relative.z);
	const float userF = config.forward + (model ? model->forward : 0.f);
	const float userR = config.right + (model ? model->right : 0.f);
	const float userZ = config.up + (model ? model->up : 0.f);
	const Vec3 targetAnchor = camera.eye - worldEyeOffset
		+ BodySpace(cameraFront, cameraRight, userF, userR, userZ);
	const Vec3 raw = targetAnchor - anchor;
	if (!Valid(raw) || Length(raw) > 96.f) return out;
	const uint64_t dt = tick >= track.tick ? tick - track.tick : 0;
	if (reset) track.filtered = raw;
	else if (dt > 0) {
		const float alpha = 1.f - std::exp(-float(std::min<uint64_t>(dt, 100)) / 18.f);
		track.filtered.x += (raw.x - track.filtered.x) * alpha;
		track.filtered.y += (raw.y - track.filtered.y) * alpha;
	}
	track.filtered.z = raw.z;
	track.tick = tick;
	track.lastEye = camera.eye;
	out.ok = Valid(track.filtered);
	out.translation = track.filtered;
	return out;
}

inline bool Transform(float (*bones)[3][4], int count, Vec3 pivot, Vec3 translate, float yawRadians) {
	if (!bones || count <= 0 || count > kMaxBones || !Valid(pivot) || !Valid(translate)
		|| !std::isfinite(yawRadians) || std::fabs(yawRadians) > 0.91f)
		return false;
	for (int i = 0; i < count; ++i)
		if (!MatrixOk(bones[i])) return false;
	const float cs = std::cos(yawRadians), sn = std::sin(yawRadians);
	for (int i = 0; i < count; ++i) {
		float (*m)[4] = bones[i];
		for (int col = 0; col < 3; ++col) {
			const float x = m[0][col], y = m[1][col];
			m[0][col] = cs * x - sn * y;
			m[1][col] = sn * x + cs * y;
		}
		const float x = m[0][3] - pivot.x, y = m[1][3] - pivot.y;
		m[0][3] = pivot.x + cs * x - sn * y + translate.x;
		m[1][3] = pivot.y + sn * x + cs * y + translate.y;
		m[2][3] += translate.z;
	}
	return true;
}

// Manual XYZ when dynamic is off: same camera horizontal basis as VE math::offset.
inline bool ManualOffset(const CameraPose& camera, float forward, float right, float up, float out[3]) {
	Vec3 f{}, r{};
	if (!UnitFrame(camera, f, r)) return false;
	const Vec3 result = f * forward + r * right + Vec3{ 0, 0, up };
	if (!Valid(result)) return false;
	out[0] = result.x;
	out[1] = result.y;
	out[2] = result.z;
	return true;
}

} // namespace dynamic
} // namespace skeeto_fpb
