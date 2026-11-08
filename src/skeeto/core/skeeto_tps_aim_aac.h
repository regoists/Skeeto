#pragma once
#include <cmath>
#include <cstdint>

// TPS-only AAC: bound added parallax changes, not the player's camera rotation.
// Owned by the existing game/Paint thread. No engine calls or dynamic storage.
// This reduces discontinuities; it does not certify compatibility with a server.
struct TpsAimAacState {
	static constexpr float kMaxStep = 0.4f;
	// Budget for the requested parallax step. Camera rotation, native fallback
	// and mandatory pitch saturation are not limits on the final usercmd delta.
	static constexpr uint32_t kRecoveryMs = 200;
	float offset[2]{}, origin[2]{};
	float remaining = 0.f;
	int command = 0;
	uint32_t at = 0, interruptedAt = 0;
	bool primed = false, interrupted = false;

	static float Wrap(float angle) {
		// All callers supply differences/sums of normalized atan2 angles.
		if (angle > 180.f) angle -= 360.f;
		if (angle < -180.f) angle += 360.f;
		return angle;
	}
	void Suspend(uint32_t now) {
		// Repeated Paint/first-person gates must not extend the same interruption.
		if (!interrupted) interruptedAt = now;
		interrupted = true; primed = false; command = 0; remaining = 0.f;
		offset[0] = offset[1] = origin[0] = origin[1] = 0.f;
	}
	bool Shape(int number, uint32_t now, const float* camera, const float* desired,
		float* result, bool* limited) {
		if (limited) *limited = false;
		if (number <= 0 || !camera || !desired || !result) return false;
		for (int i = 0; i < 3; ++i)
			if (!std::isfinite(camera[i]) || !std::isfinite(desired[i])) return false;
		if (interrupted && uint32_t(now - interruptedAt) < kRecoveryMs) return false;
		// A gap/reordered command or stale sample cannot inherit old correction.
		if (primed && number != command
			&& (int64_t(number) - command != 1 || uint32_t(now - at) > 100u)) {
			Suspend(now); return false;
		}
		if (!primed) origin[0] = origin[1] = 0.f;
		else if (number != command) {
			origin[0] = offset[0]; origin[1] = offset[1];
		}
		// A repeated command reuses its starting offset, never another step budget.
		const float wantPitch = desired[0] - camera[0];
		const float wantYaw = Wrap(desired[1] - camera[1]);
		const float dp = wantPitch - origin[0], dy = Wrap(wantYaw - origin[1]);
		const float distance = std::sqrt(dp * dp + dy * dy);
		if (!std::isfinite(distance)) return false;
		const bool clipped = distance > kMaxStep;
		const float scale = clipped ? kMaxStep / distance : 1.f;
		float pitch = camera[0] + origin[0] + dp * scale;
		if (pitch < -89.f) pitch = -89.f;
		if (pitch > 89.f) pitch = 89.f;
		result[0] = pitch;
		result[1] = Wrap(camera[1] + origin[1] + dy * scale);
		result[2] = desired[2];
		if (!std::isfinite(result[0]) || !std::isfinite(result[1])) return false;
		offset[0] = result[0] - camera[0]; offset[1] = Wrap(result[1] - camera[1]);
		const float ep = desired[0] - result[0], ey = Wrap(desired[1] - result[1]);
		remaining = std::sqrt(ep * ep + ey * ey);
		if (limited) *limited = remaining > 0.001f;
		command = number; at = now; primed = true; interrupted = false;
		return true;
	}
};
