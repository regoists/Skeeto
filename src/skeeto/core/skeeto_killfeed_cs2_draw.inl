// Included into skeeto_proxy.cpp after KillFeed helpers. CS2-style Paint path.

static void KillFeedCs2Fill(int x0, int y0, int x1, int y1, int r, int g, int b, int a) {
	if (x1 <= x0 || y1 <= y0 || a <= 0) return;
	SurfColor(r, g, b, a);
	SurfFill(x0, y0, x1, y1);
}

static void KillFeedCs2Line(int x0, int y0, int x1, int y1, unsigned rgb, int a) {
	if (a <= 0) return;
	int r, g, b, aa;
	kfcs2::FillRgb(rgb, a, &r, &g, &b, &aa);
	SurfColor(r, g, b, aa);
	SurfLine(x0, y0, x1, y1);
}

static void KillFeedCs2ComicBurst(int x, int y, int h, kfcs2::ComicEffect kind, int strength, int sc) {
	if (strength <= 0) return;
	const int size = kfcs2::ClampI(HudScalePx(7, sc), 4, h / 3);
	const int cx = x - kfcs2::ClampI(HudScalePx(12, sc), 8, 18);
	const int cy = y + h / 2;
	if (cx - size < 0) return;
	const unsigned color = kfcs2::ComicAccent(kind);
	const int a = strength;
	if (kind == kfcs2::ComicSlash) {
		KillFeedCs2Line(cx - size, cy + size, cx + size / 2, cy - size, color, a);
		KillFeedCs2Line(cx, cy + size, cx + size, cy - size / 2, color, a);
	} else if (kind == kfcs2::ComicImpact) {
		KillFeedCs2Line(cx - size, cy - size, cx, cy, color, a);
		KillFeedCs2Line(cx - size, cy + size, cx, cy, color, a);
		KillFeedCs2Line(cx, cy - size, cx + size / 2, cy - 1, color, a);
	} else if (kind == kfcs2::ComicFire) {
		KillFeedCs2Line(cx - size / 2, cy + size, cx, cy - size, color, a);
		KillFeedCs2Line(cx, cy - size, cx + size / 2, cy + size, color, a);
		const int dot = kfcs2::ClampI(HudScalePx(2, sc), 1, 3);
		int r, g, b, aa;
		kfcs2::FillRgb(0xffd18f, a, &r, &g, &b, &aa);
		KillFeedCs2Fill(cx + size, cy - size, cx + size + dot, cy - size + dot, r, g, b, aa);
	} else {
		const int gap = kfcs2::ClampI(HudScalePx(2, sc), 1, 4);
		KillFeedCs2Line(cx - size, cy, cx - gap, cy, color, a);
		KillFeedCs2Line(cx + gap, cy, cx + size, cy, color, a);
		KillFeedCs2Line(cx, cy - size, cx, cy - gap, color, a);
		KillFeedCs2Line(cx, cy + gap, cx, cy + size, color, a);
		int r, g, b, aa;
		kfcs2::FillRgb(color, a, &r, &g, &b, &aa);
		KillFeedCs2Fill(cx, cy, cx + 1, cy + 1, r, g, b, aa);
	}
}

static bool KillFeedCs2DrawMat(const char* path, int x, int y, int w, int h, unsigned tint, int a) {
	if (!path || w <= 0 || h <= 0 || a <= 0) return false;
	const int id = SurfHudMat(path);
	if (id < 0) return false;
	int r, g, b, aa;
	kfcs2::FillRgb(tint, a, &r, &g, &b, &aa);
	SurfColor(r, g, b, aa);
	return SurfDrawTexturedUv(x, y, x + w, y + h, id, 0.f, 0.f, 1.f, 1.f);
}

static bool KillFeedCs2DrawMatUv(const char* path, int x, int y, int w, int h,
	float u0, float v0, float u1, float v1, unsigned tint, int a) {
	if (!path || w <= 0 || h <= 0 || a <= 0) return false;
	const int id = SurfHudMat(path);
	if (id < 0) return false;
	int r, g, b, aa;
	kfcs2::FillRgb(tint, a, &r, &g, &b, &aa);
	SurfColor(r, g, b, aa);
	return SurfDrawTexturedUv(x, y, x + w, y + h, id, u0, v0, u1, v1);
}

static bool KillFeedCs2DrawIcon(const char* weapon, int x, int y, int w, int h, unsigned tint, int finish, int alpha) {
	const kfcs2::Icon* icon = kfcs2::RenderIcon(weapon);
	kfcs2::Icon visible = kfcs2::VisibleIcon(*icon);
	int ih = h, iw = ih * visible.w / visible.h;
	if (iw > w) { iw = w; ih = iw * visible.h / visible.w; }
	const int left = x + (w - iw) / 2, top = y + (h - ih) / 2;
	bool painted = false;
	finish = kfcs2::HasFinish(icon) ? kfcs2::ClampI(finish, 0, 255) : 0;
	auto paint = [&](const kfcs2::Icon& tile, kfcs2::UV uv, unsigned color, int opacity) {
		const int a = alpha * opacity / 255;
		if (KillFeedCs2DrawMatUv(kfcs2::SheetPath(tile.sheet), left, top, iw, ih,
			uv.left, uv.top, uv.right, uv.bottom, color, a))
			painted = true;
	};
	if (finish < 255)
		paint(visible, kfcs2::IconUV(visible), tint, 255);
	if (finish)
		paint(kfcs2::FinishedIcon(icon), kfcs2::FinishUV(kfcs2::FinishedIcon(icon)), 0xffffffu, finish);
	return painted;
}

static void KillFeedCs2GoldConfetti(int x, int y, int w, int h, unsigned age, int alpha) {
	if (age >= 1650u || alpha <= 0) return;
	const unsigned colors[] = {0xffedaa, 0xb98531, 0xffd05b};
	for (int i = 0; i < 20; ++i) {
		float t = (float)age * .001f;
		float fx = (float)((i * 173 + 51) % 1000) / 1000.f - t * .13f;
		float fy = (float)((i * 283 + 93) % 1000) / 1000.f + t * .24f;
		fx -= (float)(int)fx; if (fx < 0) fx += 1.f;
		fy -= (float)(int)fy;
		const int size = kfcs2::ClampI(h / 10, 2, 7);
		const float c = cosf(t * 6.f + (float)i);
		const int pw = kfcs2::ClampI((int)(size * (c < 0 ? -c : c)), 1, size);
		const int px = x + 2 + (int)(fx * (w - size - 4));
		const int py = y + 2 + (int)(fy * (h - size - 4));
		int r, g, b, a;
		kfcs2::FillRgb(colors[i % 3], (i % 2 ? 150 : 220) * alpha / 255, &r, &g, &b, &a);
		KillFeedCs2Fill(px, py, px + pw, py + size, r, g, b, a);
	}
}

static void KillFeedCs2GoldPanel(int x, int y, int w, int h, int cut, unsigned age, int alpha) {
	kfcs2::ComicPanel(x, y, w, h, cut, 0xe3b74f, alpha, KillFeedCs2Fill);
	kfcs2::ComicPanel(x + 1, y + 1, w - 2, h - 2, kfcs2::ClampI(cut - 1, 1, 3), 0x292117, alpha * 250 / 255, KillFeedCs2Fill);
	const int inset = kfcs2::ClampI(h / 8, 2, 9);
	for (int strip = 0; strip < 8; ++strip) {
		const int top = 2 + (h - 4) * strip / 8;
		const int bottom = 2 + (h - 4) * (strip + 1) / 8;
		unsigned col = kfcs2::Mix(0x302719, 0x5b421e, strip * 20);
		int r, g, b, a;
		kfcs2::FillRgb(col, alpha * 245 / 255, &r, &g, &b, &a);
		KillFeedCs2Fill(x + inset, y + top, x + w - inset, y + bottom, r, g, b, a);
	}
	KillFeedCs2GoldConfetti(x + inset, y + 1, w - inset * 2, h - 2, age, alpha);
}

static void KillFeedCs2GoldCrown(int x, int y, int h, int alpha) {
	const unsigned short shape[] = {0x0040, 0x10e1, 0x19f3, 0x1fff, 0x0ffe, 0x07fc, 0x07fc, 0x07fc};
	const int cw = kfcs2::ClampI(h * 16 / 35, 13, 32), ch = kfcs2::ClampI(h * 9 / 35, 8, 18);
	constexpr float cosine = .83867f, sine = -.54464f;
	auto ink = [&](int dx, int dy) {
		float u = cosine * dx + sine * dy + cw * .5f;
		float v = -sine * dx + cosine * dy + ch - 1;
		if (u < 0 || v < 0 || u >= cw || v >= ch) return 0;
		return (shape[(int)v * 8 / ch] & (1u << ((int)u * 13 / cw))) ? (v < ch * .5f ? 2 : 1) : 0;
	};
	const int left = -(int)(cw * .5f * cosine + ch * .55f) - 2;
	const int right = (int)(cw * .5f * cosine) + 3;
	const int top = -(int)(cw * .5f * .55f + ch * cosine) - 2;
	const int bottom = (int)(cw * .5f * .55f) + 3;
	const int anchorX = x + kfcs2::ClampI(h / 12, 3, 8);
	const int anchorY = y + kfcs2::ClampI(h / 8, 4, 12);
	auto shade = [&](int dx, int dy) {
		int own = ink(dx, dy); if (own) return own;
		return ink(dx - 1, dy) || ink(dx + 1, dy) || ink(dx, dy - 1) || ink(dx, dy + 1) ? 3 : 0;
	};
	for (int yy = top; yy <= bottom; ++yy) {
		for (int xx = left; xx <= right;) {
			int value = shade(xx, yy);
			if (!value) { ++xx; continue; }
			int end = xx + 1;
			while (end <= right && shade(end, yy) == value) ++end;
			unsigned col = value == 3 ? 0x302312u : value == 2 ? 0xffedb0u : 0xe1ab3au;
			int r, g, b, a;
			kfcs2::FillRgb(col, alpha, &r, &g, &b, &a);
			KillFeedCs2Fill(anchorX + xx, anchorY + yy, anchorX + end, anchorY + yy + 1, r, g, b, a);
			xx = end;
		}
	}
}

static bool KillFeedCs2ChargeSprite(int kind, int frame, int x, int y, int w, int h, int alpha) {
	const char* path = kind == 2 ? "l4n_cs2killfeed/special/jockey_airshot_v2"
		: kind == 1 ? "l4n_cs2killfeed/special/hunter_airshot_v2"
		: "l4n_cs2killfeed/special/charge_break_gold_v2";
	kfcs2::ChargeUV uv = kind ? kfcs2::HunterAirshotTileUV(frame) : kfcs2::ChargeBreakTileUV(frame);
	return KillFeedCs2DrawMatUv(path, x, y, w, h, uv.l, uv.t, uv.r, uv.b, 0xffffffu, alpha);
}

static void KillFeedCs2ComicSprite(bool fire, int frame, int x, int y, int w, int h, int alpha) {
	kfcs2::ComicUV uv = kfcs2::ComicTileUV(fire, frame);
	KillFeedCs2DrawMatUv(fire ? "l4n_cs2killfeed/comic135/fire_atlas" : "l4n_cs2killfeed/comic135/stages",
		x, y, w, h, uv.l, uv.t, uv.r, uv.b, 0xffffffu, alpha);
}

static void KillFeedCs2TextureCard(int x, int y, int w, int h, int theme, int alpha) {
	const int extent = kfcs2::ClampI(h * 6 / 50, 1, 12);
	const int tw = w + extent, th = h + extent;
	const int left = kfcs2::ClampI(h * 20 / 50, 1, tw / 4);
	const int right = kfcs2::ClampI(h * 64 / 50, 1, tw / 3);
	const int a = alpha * theme / 255;
	KillFeedCs2DrawMat("l4n_cs2killfeed/comic134/card_left", x, y, left, th, 0xffffffu, a);
	KillFeedCs2DrawMat("l4n_cs2killfeed/comic134/card_middle", x + left, y, tw - left - right, th, 0xffffffu, a);
	KillFeedCs2DrawMat("l4n_cs2killfeed/comic134/card_right", x + tw - right, y, right, th, 0xffffffu, a);
}

static void KillFeedCs2StreakTag(int x, int y, int h, int theme, DWORD now, const KillFeedStreak& streak,
	unsigned long font, int alpha) {
	const DWORD age = now - streak.lastKill;
	const bool penta = streak.count == 5 && age < 700;
	const int a = alpha * theme / 255;
	if (streak.count >= 10) {
		const bool headline = streak.count == 10 && age < 1450;
		const int bw = h * (headline ? 160 : 100) / 50, bh = h * (headline ? 110 : 52) / 50;
		const int bx = x - bw - kfcs2::ClampI(HudScalePx(6, 100), 3, 12);
		const int by = y + (h - bh) / 2;
		if (bx >= 0 && by >= 0)
			KillFeedCs2ComicSprite(false, headline ? 7 : 6, bx, by, bw, bh, a);
		return;
	}
	if (penta) {
		const int bw = h * 116 / 50, bh = h * 72 / 50, bx = x - bw, by = y - h * 12 / 50;
		if (bx >= 0 && by >= 0) {
			const int fade = age < 400 ? a : a * (int)(700 - age) / 300;
			KillFeedCs2DrawMat("l4n_cs2killfeed/comic134/speed", bx, y, bw,
				kfcs2::ClampI(h * 60 / 50, 1, 200), 0xffffffu, fade);
			KillFeedCs2DrawMat("l4n_cs2killfeed/comic134/penta", bx, by, bw, bh, 0xffffffu, a);
		}
		return;
	}
	wchar_t label[8]{};
	if (penta) {
		wcsncpy_s(label, L"PENTA!", _TRUNCATE);
	} else {
		int n = 0, value = streak.count;
		wchar_t digits[4]{};
		do { digits[n++] = (wchar_t)(L'0' + value % 10); value /= 10; } while (value && n < 3);
		label[0] = L'x';
		for (int j = 0; j < n; ++j) label[j + 1] = digits[n - j - 1];
		label[n + 1] = 0;
	}
	const int tagWidth = SurfTextWFont(font, label);
	const int pad = kfcs2::ClampI(HudScalePx(7, 100), 3, 14);
	const int bw = tagWidth + 2 * pad;
	const int bh = SurfTextWFont(font, L"Ag") > 0
		? (HudScalePx(18, 100) + kfcs2::ClampI(HudScalePx(8, 100), 4, 16))
		: (HudScalePx(22, 100));
	int bx = x - bw - kfcs2::ClampI(HudScalePx(16, 100), 8, 32);
	const int by = y + (h - bh) / 2;
	if (age < 180) {
		const int remaining = (int)(180 - age);
		bx -= kfcs2::ClampI(HudScalePx(8, 100), 4, 16) * remaining * remaining / (180 * 180);
	}
	if (bx < 3 || by < 2) return;
	KillFeedCs2DrawMat("l4n_cs2killfeed/comic134/counter", bx - 2, by - 2, bw + 4, bh + 4, 0xffffffu, a);
	SurfTextAt(font, bx + pad, by + (bh - HudScalePx(17, 100)) / 2, RGB(244, 232, 202), label);
}

static int KillFeedCs2Alpha(const KillFeedRow& row, DWORD now) {
	const DWORD age = now - row.born;
	const DWORD hold = (row.localAtk || row.localVic) ? kKillFeedCs2HoldLocalMs : kKillFeedCs2HoldOtherMs;
	if (age >= hold + kKillFeedCs2OutMs) return 0;
	if (age <= hold) return 255;
	return (int)((hold + kKillFeedCs2OutMs - age) * 255 / kKillFeedCs2OutMs);
}

static const KillFeedStreak* KillFeedCs2StreakFor(const KillFeedRow& row) {
	if (row.atkSlot < 1 || row.atkSlot > 64 || !row.streakId) return nullptr;
	const KillFeedStreak& s = g_kfStreaks[row.atkSlot];
	return s.uid == row.atkUid && s.id == row.streakId ? &s : nullptr;
}

static void KillFeedPaintCs2(KillFeedRow* show, int n, bool preview, int sw, int sh, int sc) {
	const int fontPx = HudScalePx(17, sc);
	if (!g_killFeedFontId || g_killFeedFontPx != fontPx) {
		g_killFeedFontId = SurfMakeYahei(fontPx, 500, kFontFlagAntiAlias | kFontFlagDropShadow);
		g_killFeedFontPx = fontPx;
	}
	const unsigned long font = g_killFeedFontId;
	if (!font || n <= 0) return;

	const int rowHeight = kfcs2::ClampI(HudScalePx(35, sc), 24, 96);
	const int gap = kfcs2::ClampI(HudScalePx(4, sc), 2, 12);
	const int goldHeight = kfcs2::ClampI(HudScalePx(44, sc), rowHeight, 120);
	const int width = kfcs2::ClampI(HudScalePx(470, sc), 220, sw - 16);
	int goldCount = 0;
	for (int i = 0; i < n; ++i)
		if (show[i].chargeBreak || show[i].hunterAirshot || show[i].jockeyAirshot) ++goldCount;
	const int height = rowHeight * n + gap * (n > 0 ? n - 1 : 0) + (goldHeight - rowHeight) * goldCount;

	if (!g_killFeedAbs) {
		g_killFeedOffX = sw - width - HudScalePx(10, sc);
		g_killFeedOffY = HudScalePx(72, sc);
		if (g_killFeedOffX < 0) g_killFeedOffX = 0;
		g_killFeedAbs = true;
	}
	int ax = 0, ay = 0;
	HudPlaceAbs(sw, sh, width, height, g_killFeedOffX, g_killFeedOffY, &ax, &ay);

	const DWORD now = GetTickCount();
	int cursorY = ay;
	for (int i = 0; i < n; ++i) {
		KillFeedRow& row = show[i];
		const int alpha = preview ? 255 : KillFeedCs2Alpha(row, now);
		if (alpha < 8) continue;

		const bool gold = row.chargeBreak || row.hunterAirshot || row.jockeyAirshot;
		const bool airshot = row.hunterAirshot || row.jockeyAirshot;
		const unsigned chargeAge = now - row.chargeBorn;
		const bool katana = kfcs2::Eq(row.wepKey, "katana");
		const int h = gold ? goldHeight : rowHeight;
		const int pad = kfcs2::ClampI(HudScalePx(12, sc), 6, 34);
		const int space = kfcs2::ClampI(HudScalePx(4, sc), 2, 12);
		const int iconH = kfcs2::ClampI(HudScalePx(katana ? 27 : 24, sc), 12, rowHeight - 4);
		const int badgeH = kfcs2::ClampI(HudScalePx(20, sc), 12, iconH);
		int iconW = kfcs2::IconWidth(row.wepKey, iconH, kfcs2::ClampI(HudScalePx(katana ? 72 : 64, sc), 28, 170));
		if (gold) iconW = kfcs2::ClampI(HudScalePx(84, sc), 48, 152);
		const int badge = row.hs ? badgeH + space : 0;
		const int printMargin = gold ? kfcs2::ClampI(HudScalePx(3, sc), 2, 6) : h * 22 / 35;

		wchar_t attacker[64]{}, victim[64]{};
		wcsncpy_s(attacker, row.atk, _TRUNCATE);
		if (gold) {
			wcsncpy_s(victim,
				row.jockeyAirshot ? L"JOCKEY AIRSHOT"
				: row.hunterAirshot ? L"HUNTER AIRSHOT" : L"CHARGER BREAK", _TRUNCATE);
		} else {
			wcsncpy_s(victim, KillFeedVicLabel(row.vicKind, row.vic), _TRUNCATE);
		}
		const int aw = attacker[0] ? SurfTextWFont(font, attacker) : 0;
		const int vw = victim[0] ? SurfTextWFont(font, victim) : 0;
		int w = 2 * pad + 2 * space + aw + (gold ? kfcs2::ClampI(vw, 1, 220) : vw) + iconW + badge + printMargin;
		if (w > width) w = width;
		const int x = g_killFeedSide ? ax : (ax + width - w);
		float targetY = (float)(cursorY - ay);
		if (preview) row.y = targetY;
		else {
			for (int k = 0; k < g_killFeedN; ++k) {
				if (g_killFeed[k].seq == row.seq && row.seq) {
					g_killFeed[k].y += (targetY - g_killFeed[k].y) * 0.38f;
					row.y = g_killFeed[k].y;
					break;
				}
			}
		}
		const int y = ay + (int)row.y;

		const DWORD age = preview ? 400 : (now - row.born);
		kfcs2::ComicEffect kind = kfcs2::ComicType(row.wepKey, row.hs, row.atkTeam);
		int strength = kind != kfcs2::ComicNone && age < 400 ? (age < 160 ? 255 : (int)((400 - age) * 255 / 240)) : 0;
		int wordStrength = kind != kfcs2::ComicNone && age < 800 ? (age < 440 ? 255 : (int)((800 - age) * 255 / 360)) : 0;
		int flash = kind == kfcs2::ComicSlash && age < 220 ? (int)((220 - age) * 255 / 220) : 0;
		const KillFeedStreak* streak = preview ? nullptr : KillFeedCs2StreakFor(row);
		int theme = streak ? KillFeedStreakAmount(*streak, now) : 0;
		bool newestStreakRow = theme && streak && row.streakOrdinal == streak->count;
		if (newestStreakRow) {
			for (int j = i + 1; j < n; ++j)
				if (show[j].streakId == row.streakId && show[j].atkUid == row.atkUid)
					newestStreakRow = false;
		}
		if (theme || gold) { strength = 0; wordStrength = 0; flash = 0; }

		unsigned bg = row.localVic ? 0x331b20u : 0x121b23u;
		int panelAlpha = row.localVic ? 210 : row.localAtk ? 231 : 175;
		if (strength && kind == kfcs2::ComicFire) {
			bg = kfcs2::Mix(bg, 0x3d2825, strength);
			panelAlpha += (225 - panelAlpha) * strength / 255;
		}
		if (strength && kind == kfcs2::ComicHeadshot) {
			bg = kfcs2::Mix(bg, 0x332630, strength);
			panelAlpha += (225 - panelAlpha) * strength / 255;
		}
		if (strength && kind == kfcs2::ComicImpact)
			bg = kfcs2::Mix(bg, 0x27343d, strength);
		if (flash) {
			bg = kfcs2::Mix(bg, 0xefedd2, flash);
			panelAlpha += (247 - panelAlpha) * flash / 255;
		}
		const int cut = kfcs2::ClampI(HudScalePx(4, sc), 2, 4);
		const int pa = panelAlpha * alpha / 255;
		if (gold) KillFeedCs2GoldPanel(x, y, w, h, cut, chargeAge, alpha);
		else if (theme) {
			if (theme < 255)
				kfcs2::ComicPanel(x, y, w, h, cut, bg, pa * (255 - theme) / 255, KillFeedCs2Fill);
			if (row.streakOrdinal >= 6) {
				const int extent = kfcs2::ClampI(h * 6 / 50, 1, 12);
				const int tw = w + extent, th = h + extent;
				const int ta = alpha * theme / 255;
				if (row.streakOrdinal >= 10)
					KillFeedCs2ComicSprite(true, kfcs2::ComicFireFrame(now, row.born), x, y, tw, th, ta);
				else
					KillFeedCs2ComicSprite(false, row.streakOrdinal - 5, x, y, tw, th, ta);
			} else {
				KillFeedCs2TextureCard(x, y, w, h, theme, alpha);
			}
		} else {
			kfcs2::ComicPanel(x, y, w, h, cut, bg, pa, KillFeedCs2Fill);
		}
		if (row.localAtk && !theme && !gold) {
			const int bar = kfcs2::ClampI(HudScalePx(2, sc), 1, 4);
			KillFeedCs2Fill(x + cut / 2, y + cut + 1, x + cut / 2 + bar, y + h - cut - 1, 200, 82, 80, 220 * alpha / 255);
		}
		unsigned rule = theme ? 0xfffbeau : kind != kfcs2::ComicNone && strength ? kfcs2::ComicAccent(kind) : 0x93a2abu;
		if (!theme && !gold) {
			KillFeedCs2Line(x + cut + kfcs2::ClampI(HudScalePx(6, sc), 3, 12), y + 1,
				x + w - cut - kfcs2::ClampI(HudScalePx(6, sc), 3, 12), y + 1, rule, strength ? strength * alpha / 255 : 100 * alpha / 255);
		}

		const int ty = y + (h - fontPx) / 2;
		unsigned attackerColor = kfcs2::TeamColor(row.atkTeam);
		unsigned victimColor = kfcs2::TeamColor(row.vicTeam);
		if (row.vicKind != kKfVicPlayer) {
			int vr = 210, vg = 214, vb = 226;
			KillFeedVicRgb(row.vicKind, &vr, &vg, &vb);
			victimColor = ((unsigned)vr << 16) | ((unsigned)vg << 8) | (unsigned)vb;
		}
		if (flash) {
			attackerColor = kfcs2::Mix(attackerColor, 0x243b49, flash);
			victimColor = kfcs2::Mix(victimColor, 0x67433b, flash);
		}
		if (theme) {
			attackerColor = kfcs2::Mix(attackerColor, 0x245069, theme);
			victimColor = kfcs2::Mix(victimColor, 0x9d3527, theme);
		}
		if (theme && row.streakOrdinal >= 10) {
			attackerColor = kfcs2::Mix(kfcs2::TeamColor(row.atkTeam), 0xf4e8ca, theme);
			victimColor = kfcs2::Mix(kfcs2::TeamColor(row.vicTeam), 0xf4e8ca, theme);
		}
		if (gold) { attackerColor = 0xffdc78; victimColor = 0xf6d89a; }

		auto textRgb = [&](unsigned c) {
			return RGB((c >> 16) & 255, (c >> 8) & 255, c & 255);
		};
		if (attacker[0] && !row.envOnly)
			SurfTextAt(font, x + pad, ty, textRgb(attackerColor), attacker);

		const int ix = x + pad + aw + space;
		unsigned iconTint = flash ? kfcs2::Mix(0xffffff, 0x20323c, flash)
			: kind == kfcs2::ComicFire && strength ? kfcs2::Mix(0xffffff, 0xffd1a1, strength) : 0xffffffu;
		if (theme) iconTint = kfcs2::Mix(iconTint, 0x131c22, theme);
		if (theme && row.streakOrdinal >= 10) iconTint = kfcs2::Mix(0xffffff, 0xf4e8ca, theme);
		if (gold) iconTint = 0xffe4a2;

		bool specialSprite = false;
		if (gold && chargeAge < 1650u) {
			// Do not reuse parameter name `sh` (screen height) — that clamped
			// spriteTop into [0, spriteH] and drew the atlas above the row.
			int sprH = kfcs2::ClampI(h - 2, 12, 100);
			int sprW = airshot ? sprH * 313 / 267 : sprH * 252 / 140;
			if (sprW > iconW) {
				sprW = iconW;
				sprH = airshot ? sprW * 267 / 313 : sprW * 140 / 252;
			}
			int frame = airshot ? kfcs2::HunterAirshotFrame(chargeAge) : kfcs2::ClampI((int)(chargeAge / 75u), 0, 15);
			if (row.jockeyAirshot) frame = kfcs2::ClampI(frame, 0, 13);
			const int chargeWidth = sprH * 252 / 140;
			const int spriteLeft = ix + kfcs2::ClampI((iconW - chargeWidth) / 2, 0, iconW > sprW ? iconW - sprW : 0);
			const int spriteTop = kfcs2::ClampI(
				y + (h - sprH) / 2 - (airshot ? sprH * 18 / 100 : 0),
				0, sh > sprH ? sh - sprH : 0);
			specialSprite = KillFeedCs2ChargeSprite(row.jockeyAirshot ? 2 : row.hunterAirshot ? 1 : 0,
				frame, spriteLeft, spriteTop, sprW, sprH, alpha);
		}
		if (!specialSprite && !KillFeedCs2DrawIcon(row.wepKey[0] ? row.wepKey : KillFeedPreviewWepKey(row.wep),
			ix, y + (h - iconH) / 2, iconW, iconH, iconTint, theme, alpha)) {
			const int cx = ix + iconW / 2, cy = y + h / 2, d = kfcs2::ClampI(rowHeight / 7, 3, 9);
			unsigned fallback = theme ? kfcs2::Mix(0xf4f4f4, 0x131c22, theme) : flash ? 0x20323cu : 0xf4f4f4u;
			if (theme && row.streakOrdinal >= 10) fallback = kfcs2::Mix(0xf4f4f4, 0xf4e8ca, theme);
			int r, g, b, a;
			kfcs2::FillRgb(fallback, alpha, &r, &g, &b, &a);
			KillFeedCs2Fill(cx - d, cy - d, cx + d, cy - d + 2, r, g, b, a);
			KillFeedCs2Fill(cx - d, cy - d + 2, cx - d + 2, cy + d, r, g, b, a);
			KillFeedCs2Fill(cx + d - 2, cy - d + 2, cx + d, cy + d, r, g, b, a);
			KillFeedCs2Fill(cx - d + 1, cy + 2, cx + d - 1, cy + 4, r, g, b, a);
		}
		if (row.hs) {
			const int bx = ix + iconW + space + kfcs2::ClampI(HudScalePx(7, sc), 1, 14);
			const int by = y + (h - badgeH) / 2;
			if (!KillFeedCs2DrawMat("l4n_cs2killfeed/killfeed/headshot", bx, by, badgeH, badgeH, iconTint, alpha))
				KillFeedDrawHs(bx + badgeH / 2, by + badgeH / 2, alpha, sc);
		}
		const int targetShift = kfcs2::ClampI(HudScalePx(10, sc), 1, pad + printMargin - 2);
		const int vx = gold ? (x + w - pad - printMargin - vw) : (ix + iconW + badge + space);
		SurfTextAt(font, vx + targetShift, ty, textRgb(gold ? 0xffdc78u : victimColor), victim);
		if (gold) KillFeedCs2GoldCrown(x, y, kfcs2::ClampI(HudScalePx(62, sc), rowHeight, 100), alpha);
		if (newestStreakRow && streak)
			KillFeedCs2StreakTag(x, y, h, theme, now, *streak, font, alpha);
		if (strength && kind != kfcs2::ComicNone)
			KillFeedCs2ComicBurst(x, y, h, kind, strength * alpha / 255, sc);
		if (wordStrength && kind != kfcs2::ComicNone) {
			const wchar_t* word = kfcs2::ComicWord(kind);
			const int tagWidth = SurfTextWFont(font, word);
			const int tagGap = kfcs2::ClampI(HudScalePx(28, sc), 18, 50);
			if (x >= tagWidth + tagGap + kfcs2::ClampI(HudScalePx(8, sc), 4, 16)) {
				unsigned accent = kfcs2::ComicAccent(kind);
				SurfTextAt(font, x - tagGap - tagWidth, ty, textRgb(accent), word);
			}
		}
		cursorY = y + h + gap;
	}
}
