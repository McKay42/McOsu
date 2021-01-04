//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		replay handler & parser
//
// $NoKeywords: $osr
//===============================================================================//

#include "OsuReplay.h"

OsuReplay::OsuReplay()
{
}

OsuReplay::~OsuReplay()
{
}

OsuReplay::BEATMAP_VALUES OsuReplay::getBeatmapValuesForModsLegacy(int modsLegacy, float legacyAR, float legacyCS, float legacyOD, float legacyHP)
{
	BEATMAP_VALUES v;

	// HACKHACK: code duplication, see Osu::getRawSpeedMultiplier()
	v.speedMultiplier = 1.0f;
	{
		if (modsLegacy & OsuReplay::Mods::HalfTime)
			v.speedMultiplier = 0.75f;
		if ((modsLegacy & OsuReplay::Mods::DoubleTime) || (modsLegacy & OsuReplay::Mods::Nightcore))
			v.speedMultiplier = 1.5f;
	}

	// HACKHACK: code duplication, see Osu::getDifficultyMultiplier()
	v.difficultyMultiplier = 1.0f;
	{
		if (modsLegacy & OsuReplay::Mods::HardRock)
			v.difficultyMultiplier = 1.4f;
		if (modsLegacy & OsuReplay::Mods::Easy)
			v.difficultyMultiplier = 0.5f;
	}

	// HACKHACK: code duplication, see Osu::getCSDifficultyMultiplier()
	v.csDifficultyMultiplier = 1.0f;
	{
		if (modsLegacy & OsuReplay::Mods::HardRock)
			v.csDifficultyMultiplier = 1.3f; // different!
		if (modsLegacy & OsuReplay::Mods::Easy)
			v.csDifficultyMultiplier = 0.5f;
	}

	// apply legacy mods to legacy beatmap values
	v.AR = clamp<float>(legacyAR * v.difficultyMultiplier, 0.0f, 10.0f);
	v.CS = clamp<float>(legacyCS * v.csDifficultyMultiplier, 0.0f, 10.0f);
	v.OD = clamp<float>(legacyOD * v.difficultyMultiplier, 0.0f, 10.0f);
	v.HP = clamp<float>(legacyHP * v.difficultyMultiplier, 0.0f, 10.0f);

	return v;
}
