#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>

// Free-camera math only. No engine calls, hooks, or dynamic storage.
// Owned by the game/Paint thread that drives OverrideView / CreateMove.
namespace skeeto_freecam {

struct Vec3 {
	float x = 0.f, y = 0.f, z = 0.f;
};

inline bool Finite(const Vec3& v) {
	return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
}

inline float Dot(const Vec3& a, const Vec3& b) {
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline float Length(const Vec3& v) {
	return std::sqrt(Dot(v, v));
}

inline Vec3 Scale(const Vec3& v, float s) {
	return { v.x * s, v.y * s, v.z * s };
}

inline Vec3 Add(const Vec3& a, const Vec3& b) {
	return { a.x + b.x, a.y + b.y, a.z + b.z };
}

inline Vec3 Sub(const Vec3& a, const Vec3& b) {
	return { a.x - b.x, a.y - b.y, a.z - b.z };
}

struct Input {
	float forward = 0.f, side = 0.f, up = 0.f;
};

struct MouseQueue {
	std::int64_t x = 0, y = 0;
	void Push(std::int64_t dx, std::int64_t dy) {
		x += dx;
		y += dy;
	}
	void Take(std::int64_t limit, float* outDx, float* outDy) {
		if (!outDx || !outDy) return;
		const std::int64_t nx = std::clamp(x, -limit, limit);
		const std::int64_t ny = std::clamp(y, -limit, limit);
		x -= nx;
		y -= ny;
		*outDx = (float)nx;
		*outDy = (float)ny;
	}
	void Drop() { x = y = 0; }
};

struct Camera {
	Vec3 origin{}, angles{}, velocity{};
	float speed = 1000.f;
	float sensitivity = 1.f;
	bool ready = false;

	void Halt() { velocity = {}; }
	void Start(const Vec3& p, const Vec3& a) {
		ready = Finite(p) && Finite(a);
		Halt();
		if (!ready) return;
		origin = p;
		angles = a;
		angles.z = 0.f;
	}
	void Look(float dx, float dy) {
		if (!ready || !std::isfinite(dx) || !std::isfinite(dy)) return;
		angles.x = std::clamp(angles.x + dy * 0.022f * sensitivity, -89.f, 89.f);
		angles.y = std::remainder(angles.y - dx * 0.022f * sensitivity, 360.f);
		angles.z = 0.f;
	}
	void Advance(const Input& in, float dt) {
		if (!ready || !std::isfinite(dt) || dt <= 0.f) return;
		dt = std::min(dt, 0.05f);
		constexpr float kRad = 0.017453292519943295f;
		const float yaw = angles.y * kRad;
		const float pitch = angles.x * kRad;
		Vec3 wish{
			std::cos(pitch) * std::cos(yaw) * in.forward + std::sin(yaw) * in.side,
			std::cos(pitch) * std::sin(yaw) * in.forward - std::cos(yaw) * in.side,
			-std::sin(pitch) * in.forward + in.up
		};
		const float len = Length(wish);
		if (!std::isfinite(len)) return;
		if (len > 1.f) wish = Scale(wish, 1.f / len);
		const bool moving = len > 0.0001f;
		const float cap = std::clamp(std::isfinite(speed) ? speed : 1000.f, 30.f, 2500.f);
		const Vec3 target = Scale(wish, cap);
		const float oldSpeed = Length(velocity);
		const bool sharp = moving && oldSpeed > 0.001f && Dot(velocity, wish) < oldSpeed * 0.7f;
		const float tau = moving ? (sharp ? 0.085f : 0.12f) : 0.065f;
		const float blend = -std::expm1(-dt / tau);
		const Vec3 previous = velocity;
		const Vec3 nextVelocity = Add(previous, Scale(Sub(target, previous), blend));
		const Vec3 displacement = Add(Scale(target, dt), Scale(Sub(previous, target), tau * blend));
		const Vec3 next = Add(origin, displacement);
		if (!Finite(nextVelocity) || !Finite(next)) {
			Halt();
			return;
		}
		velocity = nextVelocity;
		if (!moving && Length(velocity) < 0.01f) velocity = {};
		origin = next;
	}
};

inline int ClampSpeed(int v) {
	if (v < 30) return 30;
	if (v > 2500) return 2500;
	return v;
}

} // namespace skeeto_freecam
