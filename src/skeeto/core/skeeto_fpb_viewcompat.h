#pragma once
// Port of L4D2-Visual-Effects view_compat + free_camera_body_policy.
#include "skeeto_fpb_dynamic.h"
#include <cstdint>
#include <optional>

namespace skeeto_fpb {
namespace viewcompat {

enum class Mode : unsigned char { Unknown, First, Third };
inline const char* Label(Mode v) {
	switch (v) {
	case Mode::First: return "first-person";
	case Mode::Third: return "third-person";
	default: return "unverified";
	}
}

struct Observation {
	Mode evidence = Mode::Unknown;
	float horizontal = -1.f;
	float eyeHeight = -1.f;
};

inline Observation Classify(const dynamic::CameraPose& camera,
	const float* playerOrigin, std::optional<bool> shoulder) {
	Observation out{};
	if (!playerOrigin || !dynamic::Valid(camera.eye)
		|| !std::isfinite(playerOrigin[0]) || !std::isfinite(playerOrigin[1]) || !std::isfinite(playerOrigin[2]))
		return out;
	const float dx = camera.eye.x - playerOrigin[0];
	const float dy = camera.eye.y - playerOrigin[1];
	out.horizontal = std::hypot(dx, dy);
	out.eyeHeight = camera.eye.z - playerOrigin[2];
	if (!std::isfinite(out.horizontal) || out.eyeHeight < -40.f || out.eyeHeight > 220.f) return out;
	if (shoulder && *shoulder) { out.evidence = Mode::Third; return out; }
	const bool eyeOk = (out.eyeHeight >= 30.f && out.eyeHeight <= 88.f);
	if (shoulder && !*shoulder) {
		if (eyeOk && out.horizontal <= 58.f) out.evidence = Mode::First;
		else if (out.horizontal >= 85.f || out.eyeHeight > 130.f) out.evidence = Mode::Third;
		return out;
	}
	if (eyeOk && out.horizontal <= 12.f) out.evidence = Mode::First;
	else if (out.horizontal >= 75.f || out.eyeHeight > 130.f) out.evidence = Mode::Third;
	return out;
}

struct CameraLatch {
	Mode committed = Mode::Unknown, candidate = Mode::Unknown;
	uint64_t since = 0, lastTick = 0, unknownSince = 0;
	uint32_t epoch = 0;
	void Reset() {
		committed = candidate = Mode::Unknown;
		since = lastTick = unknownSince = 0;
		epoch = 0;
	}
	Mode Update(Mode incoming, uint64_t tick, uint32_t currentEpoch) {
		if (epoch != currentEpoch || tick < lastTick || tick - lastTick > 1000) {
			Reset();
			epoch = currentEpoch;
		}
		lastTick = tick;
		if (incoming == Mode::Unknown) {
			candidate = Mode::Unknown;
			since = 0;
			if (!unknownSince) unknownSince = tick ? tick : 1;
			if (tick >= unknownSince && tick - unknownSince > 500) committed = Mode::Unknown;
			return committed;
		}
		unknownSince = 0;
		if (incoming == Mode::Third) {
			committed = candidate = Mode::Third;
			since = tick;
			return committed;
		}
		if (committed == Mode::First) {
			candidate = Mode::First;
			since = tick;
			return committed;
		}
		if (candidate != Mode::First) {
			candidate = Mode::First;
			since = tick;
			return committed;
		}
		if (tick >= since && tick - since >= 96) committed = Mode::First;
		return committed;
	}
};

} // namespace viewcompat

namespace freecamerabody {

enum class Mode { NormalView, WorldAnchored, NativeThirdPerson };

inline Mode Choose(bool freeCamera, bool thirdPersonCompatibility, std::optional<bool> nativeShoulder) {
	if (!freeCamera) return Mode::NormalView;
	if (thirdPersonCompatibility && nativeShoulder && *nativeShoulder)
		return Mode::NativeThirdPerson;
	return Mode::WorldAnchored;
}

inline bool PassOriginalStudio(bool freeCamera) { return freeCamera; }

struct Transition {
	bool previous = false;
	bool Update(bool freeCamera) {
		const bool changed = previous != freeCamera;
		previous = freeCamera;
		return changed;
	}
	void Reset() { previous = false; }
};

} // namespace freecamerabody
} // namespace skeeto_fpb
