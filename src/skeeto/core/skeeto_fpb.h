#pragma once
// First-person body (Lux Advanced style): render-only head/arm collapse and
// camera-relative XYZ offset on a temporary Studio DrawModel bone copy.
// Does not hook BuildTransformations, write entity bone cache, change collision,
// networking, or the weapon viewmodel.
#include "skeeto_memread.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>

namespace skeeto_fpb {

inline constexpr int kMaxBones = 256; // match Visual-Effects MaxBones; anime MOD skeletons often exceed 128
inline constexpr int kBoneStride = 216;

inline constexpr int kMaxProfiles = 64;
inline constexpr int kModelKeyMax = 180;

struct ModelProfile {
	char model[kModelKeyMax]{};
	uint32_t checksum = 0;
	float forward = 0.f, right = 0.f, up = 0.f, yaw = 0.f;
};

struct Settings {
	bool enabled = false;
	bool hideHead = true;
	bool hideArms = true;
	bool thirdPersonCompat = false; // when on, skip decorate/gate outside first person
	bool hideOnLadder = true;       // default on: close FPB gate while m_MoveType == ladder
	bool dynamicAnchor = true;      // VE default: align body to live camera each Studio draw
	bool followCameraYaw = true;
	float forward = -13.f;
	float right = -1.f;
	float up = 5.f;
	float step = 1.f;
	float fallbackEyeHeight = 5.f;
	float yawTrim = 0.f;
};

inline float ClampAxis(float v) {
	if (!(v == v)) return 0.f;
	if (v < -64.f) return -64.f;
	if (v > 64.f) return 64.f;
	return v;
}
inline float ClampRange(float v, float lo, float hi, float fallback) {
	if (!(v == v)) return fallback;
	if (v < lo) return lo;
	if (v > hi) return hi;
	return v;
}
inline void Normalize(Settings& s) {
	s.forward = ClampAxis(s.forward);
	s.right = ClampAxis(s.right);
	s.up = ClampAxis(s.up);
	if (!(s.step == 0.5f || s.step == 1.f || s.step == 5.f)) s.step = 1.f;
	s.fallbackEyeHeight = ClampRange(s.fallbackEyeHeight, -8.f, 24.f, 5.f);
	s.yawTrim = ClampRange(s.yawTrim, -40.f, 40.f, 0.f);
}

struct ProfileStore {
	ModelProfile profiles[kMaxProfiles]{};
	int count = 0;
	void Clear() { count = 0; }
	void NormalizeAll() {
		if (count < 0) count = 0;
		if (count > kMaxProfiles) count = kMaxProfiles;
		for (int i = 0; i < count; ++i) {
			profiles[i].forward = ClampAxis(profiles[i].forward);
			profiles[i].right = ClampAxis(profiles[i].right);
			profiles[i].up = ClampAxis(profiles[i].up);
			profiles[i].yaw = ClampRange(profiles[i].yaw, -40.f, 40.f, 0.f);
		}
	}
};

inline bool ValidModelKey(const char* name) {
	if (!name || !name[0]) return false;
	size_t n = 0;
	for (; name[n] && n < (size_t)kModelKeyMax; ++n) {
		const unsigned char c = (unsigned char)name[n];
		if (c < 32 || c == 127) return false;
	}
	return n > 0 && n < (size_t)kModelKeyMax;
}

inline void CanonicalModel(const char* in, char* out, size_t outN) {
	if (!out || outN < 2) return;
	out[0] = 0;
	if (!in) return;
	size_t w = 0;
	for (size_t i = 0; in[i] && w + 1 < outN; ++i) {
		char c = in[i];
		if (c == '\\') c = '/';
		if (c >= 'A' && c <= 'Z') c = char(c - 'A' + 'a');
		out[w++] = c;
	}
	out[w] = 0;
	if (std::strncmp(out, "models/", 7) == 0) {
		const size_t rest = std::strlen(out + 7);
		std::memmove(out, out + 7, rest + 1);
	}
}

inline const ModelProfile* FindProfile(const ProfileStore& store, const char* key, uint32_t checksum) {
	if (!ValidModelKey(key)) return nullptr;
	for (int i = 0; i < store.count; ++i)
		if (store.profiles[i].checksum == checksum && std::strcmp(store.profiles[i].model, key) == 0)
			return &store.profiles[i];
	return nullptr;
}

inline bool StoreForModel(ProfileStore& store, Settings& s, const char* key, uint32_t checksum) {
	if (!ValidModelKey(key)) return false;
	ModelProfile* found = nullptr;
	for (int i = 0; i < store.count; ++i) {
		if (store.profiles[i].checksum == checksum && std::strcmp(store.profiles[i].model, key) == 0) {
			found = &store.profiles[i];
			break;
		}
	}
	if (!found) {
		if (store.count >= kMaxProfiles) return false;
		found = &store.profiles[store.count++];
		*found = {};
		std::strncpy(found->model, key, sizeof(found->model) - 1);
		found->checksum = checksum;
	}
	found->forward += s.forward;
	found->right += s.right;
	found->up += s.up;
	found->yaw += s.yawTrim;
	s.forward = s.right = s.up = s.yawTrim = 0.f;
	Normalize(s);
	store.NormalizeAll();
	return true;
}

inline bool EraseForModel(ProfileStore& store, const char* key, uint32_t checksum) {
	if (!ValidModelKey(key)) return false;
	int w = 0;
	for (int i = 0; i < store.count; ++i) {
		if (store.profiles[i].checksum == checksum && std::strcmp(store.profiles[i].model, key) == 0)
			continue;
		if (w != i) store.profiles[w] = store.profiles[i];
		++w;
	}
	const bool changed = w != store.count;
	store.count = w;
	return changed;
}

// Text profile file (same role as skeeto/viewmodel/profiles.ini).
inline bool FormatProfiles(const ProfileStore& store, char* out, size_t cap, size_t* written) {
	if (written) *written = 0;
	if (!out || cap < 64) return false;
	out[0] = 0;
	const char* header = "# Skeeto first-person body profiles v1\n";
	size_t used = std::strlen(header);
	if (cap <= used) return false;
	std::memcpy(out, header, used + 1);
	for (int i = 0; i < store.count; ++i) {
		const auto& p = store.profiles[i];
		if (!ValidModelKey(p.model)) { out[0] = 0; return false; }
		const int n = std::snprintf(out + used, cap - used,
			"[%s|%08x]\nforward=%.6g\nright=%.6g\nup=%.6g\nyaw=%.6g\n",
			p.model, (unsigned)p.checksum, p.forward, p.right, p.up, p.yaw);
		if (n < 0 || (size_t)n >= cap - used) { out[0] = 0; return false; }
		used += (size_t)n;
	}
	if (written) *written = used;
	return true;
}

inline bool ParseProfiles(const char* text, size_t length, ProfileStore* out) {
	if (!text || !out || length > 256 * 1024) return false;
	ProfileStore candidate{};
	bool header = false;
	int current = -1;
	unsigned fields = 0;
	size_t offset = 0;
	auto finish = [&]() -> bool {
		if (current < 0) return true;
		if ((fields & 15) != 15) return false;
		return true;
	};
	while (offset < length) {
		char line[512];
		size_t n = 0;
		while (offset < length && text[offset] != '\n') {
			const char c = text[offset++];
			if (!c || n == sizeof(line) - 1) return false;
			line[n++] = c;
		}
		if (offset < length) ++offset;
		if (n && line[n - 1] == '\r') --n;
		while (n && (line[n - 1] == ' ' || line[n - 1] == '\t')) --n;
		line[n] = 0;
		char* start = line;
		while (*start == ' ' || *start == '\t') ++start;
		if (!*start) continue;
		if (*start == '#') {
			if (!std::strcmp(start, "# Skeeto first-person body profiles v1")) {
				if (header || current >= 0) return false;
				header = true;
			}
			continue;
		}
		if (!header) return false;
		if (*start == '[') {
			if (!finish()) return false;
			const size_t size = std::strlen(start);
			if (size < 12 || start[size - 1] != ']') return false;
			start[size - 1] = 0;
			char* sep = std::strchr(start + 1, '|');
			if (!sep || std::strlen(sep + 1) != 8) return false;
			uint32_t checksum = 0;
			for (int i = 1; i <= 8; ++i) {
				const char c = sep[i];
				unsigned value = 0;
				if (c >= '0' && c <= '9') value = c - '0';
				else if (c >= 'a' && c <= 'f') value = c - 'a' + 10;
				else if (c >= 'A' && c <= 'F') value = c - 'A' + 10;
				else return false;
				checksum = (checksum << 4) | value;
			}
			*sep = 0;
			char key[kModelKeyMax]{};
			CanonicalModel(start + 1, key, sizeof(key));
			if (!ValidModelKey(key)) return false;
			current = -1;
			for (int i = 0; i < candidate.count; ++i) {
				if (candidate.profiles[i].checksum == checksum
					&& std::strcmp(candidate.profiles[i].model, key) == 0) {
					current = i;
					candidate.profiles[i] = {};
					std::strncpy(candidate.profiles[i].model, key, sizeof(candidate.profiles[i].model) - 1);
					candidate.profiles[i].checksum = checksum;
					break;
				}
			}
			if (current < 0) {
				if (candidate.count >= kMaxProfiles) return false;
				current = candidate.count++;
				candidate.profiles[current] = {};
				std::strncpy(candidate.profiles[current].model, key, sizeof(candidate.profiles[current].model) - 1);
				candidate.profiles[current].checksum = checksum;
			}
			fields = 0;
		} else {
			if (current < 0) return false;
			char* sep = std::strchr(start, '=');
			if (!sep) return false;
			char* end = sep;
			while (end > start && (end[-1] == ' ' || end[-1] == '\t')) --end;
			*end = 0;
			char* value = sep + 1;
			while (*value == ' ' || *value == '\t') ++value;
			char* parsed = nullptr;
			const float raw = std::strtof(value, &parsed);
			if (parsed == value || !std::isfinite(raw)) return false;
			while (*parsed == ' ' || *parsed == '\t') ++parsed;
			if (*parsed) return false;
			auto& p = candidate.profiles[current];
			unsigned bit = 0;
			if (!std::strcmp(start, "forward")) { p.forward = raw; bit = 1; }
			else if (!std::strcmp(start, "right")) { p.right = raw; bit = 2; }
			else if (!std::strcmp(start, "up")) { p.up = raw; bit = 4; }
			else if (!std::strcmp(start, "yaw")) { p.yaw = raw; bit = 8; }
			else return false;
			if (fields & bit) return false;
			fields |= bit;
		}
	}
	if (!header || !finish()) return false;
	candidate.NormalizeAll();
	*out = candidate;
	return true;
}

inline bool NamedEnd(const char* name, const char* stem) {
	if (!name || !stem || !stem[0]) return false;
	const size_t n = std::strlen(stem);
	const size_t len = std::strlen(name);
	if (len < n) return false;
	const char* pos = name + len - n;
	// Walk back for optional trailing digits already stripped by caller lowercasing.
	if (std::strncmp(pos, stem, n) != 0) {
		// search last occurrence
		pos = nullptr;
		for (const char* p = name; *p; ++p) {
			if (std::strncmp(p, stem, n) == 0) {
				const char* end = p + n;
				while (*end >= '0' && *end <= '9') ++end;
				if (*end == 0) pos = p;
			}
		}
		if (!pos) return false;
	} else {
		const char* end = pos + n;
		if (*end != 0) return false;
	}
	if (pos > name) {
		const char c = pos[-1];
		if (c != '_' && c != '.' && c != ':' && c != '/' && c != '-') return false;
	}
	return true;
}
inline bool HeadName(const char* n) {
	return NamedEnd(n, "head") || NamedEnd(n, "head1") || NamedEnd(n, "head_01");
}
inline bool NeckName(const char* n) { return NamedEnd(n, "neck") || NamedEnd(n, "neck1"); }
inline bool UpperArmName(const char* n) {
	return NamedEnd(n, "upperarm") || NamedEnd(n, "upper_arm") || NamedEnd(n, "upperarm1");
}
inline int Side(const char* n) {
	if (std::strstr(n, "_l_") || std::strstr(n, "left") || std::strstr(n, ".l_")) return 1;
	if (std::strstr(n, "_r_") || std::strstr(n, "right") || std::strstr(n, ".r_")) return 2;
	return 0;
}
inline bool Descendant(const int* parents, int count, int index, int root) {
	if (root < 0 || index < 0 || index >= count) return false;
	int cur = index;
	for (int n = 0; n <= count; ++n) {
		if (cur == root) return true;
		if (cur < 0 || cur >= count) return false;
		cur = parents[cur];
	}
	return false;
}

struct Plan {
	int head = -1, neck = -1, leftArm = -1, rightArm = -1;
	int leftPivot = -1, rightPivot = -1; // shoulder parents of upper arms
	uint8_t headMask[kMaxBones]{};
	uint8_t leftMask[kMaxBones]{};
	uint8_t rightMask[kMaxBones]{};
};

// Cached studiohdr decode for parts + dynamic anchor (VE StudioPose subset).
struct StudioInfo {
	void* raw = nullptr;
	int count = 0;
	uint32_t checksum = 0;
	char modelName[kModelKeyMax]{}; // canonical
	Plan plan{};
	int parents[kMaxBones]{};
	char names[kMaxBones][64]{};
	float bindLocal[kMaxBones][3]{};
	uint8_t bindValid[kMaxBones]{};
	float eyeLocal[3]{};
	bool eyeValid = false;
};

inline bool InverseBindPosition(const float m[12], float out[3]) {
	const double a = m[0], b = m[1], c = m[2], d = m[4], e = m[5], f = m[6], g = m[8], h = m[9], i = m[10];
	const double A = e * i - f * h, B = c * h - b * i, C = b * f - c * e;
	const double D = f * g - d * i, E = a * i - c * g, F = c * d - a * f;
	const double G = d * h - e * g, H = b * g - a * h, I = a * e - b * d;
	const double det = a * A + b * D + c * G;
	if (!std::isfinite(det) || std::fabs(det) < 0.1 || std::fabs(det) > 10.0) return false;
	const double tx = m[3], ty = m[7], tz = m[11];
	const double x = -(A * tx + B * ty + C * tz) / det;
	const double y = -(D * tx + E * ty + F * tz) / det;
	const double z = -(G * tx + H * ty + I * tz) / det;
	if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z)
		|| std::fabs(x) > 4096 || std::fabs(y) > 4096 || std::fabs(z) > 4096)
		return false;
	out[0] = float(x);
	out[1] = float(y);
	out[2] = float(z);
	return true;
}

inline bool MatrixOk(const float m[3][4]) {
	for (int r = 0; r < 3; ++r)
		for (int c = 0; c < 4; ++c)
			if (!(m[r][c] == m[r][c])) return false;
	return true;
}
inline void Collapse(float dest[3][4], const float src[3][4], const float point[3]) {
	constexpr float tiny = 0.0001f;
	for (int r = 0; r < 3; ++r) {
		for (int c = 0; c < 3; ++c) dest[r][c] = src[r][c] * tiny;
		dest[r][3] = point[r];
	}
}

// Lowercased bone name into out (max 63 + NUL).
inline bool ReadBoneName(const uint8_t* boneBase, const uint8_t* hdrEnd, char* out, size_t outN) {
	if (!boneBase || !hdrEnd || !out || outN < 2) return false;
	int32_t disp = 0;
	if (!skmem::Copy(&disp, boneBase, 4) || disp <= 0) return false;
	const uint8_t* p = boneBase + disp;
	if (p >= hdrEnd) return false;
	size_t n = 0;
	for (; n + 1 < outN && n < 127; ++n) {
		char ch = 0;
		if (p + n >= hdrEnd || !skmem::Copy(&ch, p + n, 1)) return false;
		if (!ch) break;
		out[n] = (ch >= 'A' && ch <= 'Z') ? char(ch - 'A' + 'a') : ch;
	}
	out[n] = 0;
	return n > 0;
}

inline bool LoadStudio(const void* rawHdr, int count, StudioInfo& out) {
	out = {};
	if (!rawHdr || count <= 0 || count > kMaxBones) return false;
	uint32_t words[42]{};
	if (!skmem::Copy(words, rawHdr, sizeof(words))) return false;
	if (words[0] != 0x54534449u || (words[1] != 48 && words[1] != 49)) return false;
	const uint32_t length = words[19];
	const uint32_t boneCount = words[39];
	const uint32_t boneOff = words[40];
	if (boneCount != (uint32_t)count || !boneCount || boneCount > (uint32_t)kMaxBones
		|| boneOff < sizeof(words)
		|| uint64_t(boneOff) + uint64_t(boneCount) * kBoneStride > length)
		return false;
	out.checksum = words[2];
	char rawName[64]{};
	std::memcpy(rawName, reinterpret_cast<const char*>(words) + 12, 63);
	rawName[63] = 0;
	CanonicalModel(rawName, out.modelName, sizeof(out.modelName));
	float eye[3]{};
	std::memcpy(eye, reinterpret_cast<const char*>(words) + 80, 12); // words[20..22]
	out.eyeLocal[0] = eye[0];
	out.eyeLocal[1] = eye[1];
	out.eyeLocal[2] = eye[2];
	out.eyeValid = std::isfinite(eye[0]) && std::isfinite(eye[1]) && std::isfinite(eye[2])
		&& std::fabs(eye[0]) <= 256.f && std::fabs(eye[1]) <= 256.f && std::fabs(eye[2]) <= 256.f;
	const uint8_t* base = (const uint8_t*)rawHdr;
	const uint8_t* end = base + length;
	Plan& plan = out.plan;
	for (int i = 0; i < count; ++i) {
		const uint8_t* bone = base + boneOff + i * kBoneStride;
		if (!skmem::Copy(&out.parents[i], bone + 4, 4)
			|| out.parents[i] < -1 || out.parents[i] >= count || out.parents[i] == i)
			return false;
		if (!ReadBoneName(bone, end, out.names[i], sizeof(out.names[i]))) return false;
		float poseToBone[12]{};
		if (skmem::Copy(poseToBone, bone + 96, sizeof(poseToBone))
			&& InverseBindPosition(poseToBone, out.bindLocal[i]))
			out.bindValid[i] = 1;
	}
	for (int i = 0; i < count; ++i) {
		if (!HeadName(out.names[i])) continue;
		int parent = out.parents[i];
		for (int depth = 0; parent >= 0 && depth < count; ++depth) {
			if (NeckName(out.names[parent])) {
				plan.head = i;
				plan.neck = parent;
				break;
			}
			parent = out.parents[parent];
		}
		if (plan.head >= 0) break;
	}
	for (int i = 0; i < count; ++i) {
		if (!UpperArmName(out.names[i]) || out.parents[i] < 0) continue;
		const int which = Side(out.names[i]);
		if (which == 1 && plan.leftArm < 0) {
			plan.leftArm = i;
			plan.leftPivot = out.parents[i];
		}
		if (which == 2 && plan.rightArm < 0) {
			plan.rightArm = i;
			plan.rightPivot = out.parents[i];
		}
	}
	for (int i = 0; i < count; ++i) {
		if (plan.head >= 0 && Descendant(out.parents, count, i, plan.head)) plan.headMask[i] = 1;
		if (plan.leftArm >= 0 && Descendant(out.parents, count, i, plan.leftArm)) plan.leftMask[i] = 1;
		if (plan.rightArm >= 0 && Descendant(out.parents, count, i, plan.rightArm)) plan.rightMask[i] = 1;
	}
	out.raw = (void*)rawHdr;
	out.count = count;
	return plan.head >= 0 || plan.leftArm >= 0 || plan.rightArm >= 0 || out.bindValid[0];
}

inline bool Discover(const void* rawHdr, int count, Plan& plan) {
	StudioInfo info{};
	if (!LoadStudio(rawHdr, count, info)) return false;
	plan = info.plan;
	return plan.head >= 0 || plan.leftArm >= 0 || plan.rightArm >= 0;
}

inline void ApplyParts(float (*bones)[3][4], int count, const Plan& plan, bool hideHead, bool hideArms) {
	if (!bones || count <= 0 || count > kMaxBones) return;
	for (int i = 0; i < count; ++i)
		if (!MatrixOk(bones[i])) return;
	auto branch = [&](const uint8_t* mask, int pivot) {
		if (pivot < 0 || pivot >= count) return;
		const float point[3] = { bones[pivot][0][3], bones[pivot][1][3], bones[pivot][2][3] };
		for (int i = 0; i < count; ++i) {
			if (!mask[i] || i == pivot) continue;
			float tmp[3][4];
			Collapse(tmp, bones[i], point);
			std::memcpy(bones[i], tmp, sizeof(tmp));
		}
	};
	if (hideHead && plan.head >= 0 && plan.neck >= 0)
		branch(plan.headMask, plan.neck);
	if (hideArms) {
		if (plan.leftArm >= 0) branch(plan.leftMask, plan.leftPivot);
		if (plan.rightArm >= 0) branch(plan.rightMask, plan.rightPivot);
	}
}

// Horizontal yaw basis from eye angles (degrees).
inline bool OffsetFromAngles(float pitch, float yaw, float forward, float right, float up, float out[3]) {
	(void)pitch;
	const float yawRad = yaw * (3.14159265f / 180.f);
	const float cy = std::cos(yawRad), sy = std::sin(yawRad);
	// Source forward on XY: (cy*cos? Source: forward = (cos(yaw), sin(yaw)) with pitch separate.
	const float fx = cy, fy = sy;
	const float rx = sy, ry = -cy; // right = cross(up, forward) approx on XY
	out[0] = fx * forward + rx * right;
	out[1] = fy * forward + ry * right;
	out[2] = up;
	return std::isfinite(out[0]) && std::isfinite(out[1]) && std::isfinite(out[2]);
}

inline void ApplyOffset(float (*bones)[3][4], int count, const float delta[3]) {
	if (!bones || !delta || count <= 0 || count > kMaxBones) return;
	for (int i = 0; i < count; ++i) {
		bones[i][0][3] += delta[0];
		bones[i][1][3] += delta[1];
		bones[i][2][3] += delta[2];
	}
}

} // namespace skeeto_fpb
