//================ Copyright (c) 2017, PG, All rights reserved. =================//
//
// Purpose:		difficulty & plafield behaviour
//
// $NoKeywords: $osugrm
//===============================================================================//

#ifndef OSUGAMERULESMANIA_H
#define OSUGAMERULESMANIA_H

#include "Osu.h"
#include "OsuBeatmap.h"

#include "ConVar.h"

class OsuGameRulesMania
{
public:

	//********************//
	//	Hitobject Timing  //
	//********************//

	static ConVar osu_mania_hitwindow_od_add_multiplier;
	static ConVar osu_mania_hitwindow_300r;
	static ConVar osu_mania_hitwindow_300;
	static ConVar osu_mania_hitwindow_200;
	static ConVar osu_mania_hitwindow_100;
	static ConVar osu_mania_hitwindow_50;
	static ConVar osu_mania_hitwindow_miss;

	static inline float getHitWindow300r(OsuBeatmap *beatmap)	{return osu_mania_hitwindow_300r.getFloat() + osu_mania_hitwindow_od_add_multiplier.getFloat()*(std::min(10.0f, std::max(0.0f, 10.0f - beatmap->getOD())));}
	static inline float getHitWindow300(OsuBeatmap *beatmap)	{return osu_mania_hitwindow_300.getFloat() + osu_mania_hitwindow_od_add_multiplier.getFloat()*(std::min(10.0f, std::max(0.0f, 10.0f - beatmap->getOD())));}
	static inline float getHitWindow200(OsuBeatmap *beatmap)	{return osu_mania_hitwindow_200.getFloat() + osu_mania_hitwindow_od_add_multiplier.getFloat()*(std::min(10.0f, std::max(0.0f, 10.0f - beatmap->getOD())));}
	static inline float getHitWindow100(OsuBeatmap *beatmap)	{return osu_mania_hitwindow_100.getFloat() + osu_mania_hitwindow_od_add_multiplier.getFloat()*(std::min(10.0f, std::max(0.0f, 10.0f - beatmap->getOD())));}
	static inline float getHitWindow50(OsuBeatmap *beatmap)		{return osu_mania_hitwindow_50.getFloat() + osu_mania_hitwindow_od_add_multiplier.getFloat()*(std::min(10.0f, std::max(0.0f, 10.0f - beatmap->getOD())));}
	static inline float getHitWindowMiss(OsuBeatmap *beatmap)	{return osu_mania_hitwindow_miss.getFloat() + osu_mania_hitwindow_od_add_multiplier.getFloat()*(std::min(10.0f, std::max(0.0f, 10.0f - beatmap->getOD())));}

	static OsuScore::HIT getHitResult(long delta, OsuBeatmap *beatmap)
	{
		delta = std::abs(delta);

		OsuScore::HIT result = OsuScore::HIT::HIT_NULL;

		// TODO: rainbow 300s and 200s

		if (delta <= (long)getHitWindow300(beatmap))
			result = OsuScore::HIT::HIT_300;
		else if (delta <= (long)getHitWindow100(beatmap))
			result = OsuScore::HIT::HIT_100;
		else if (delta <= (long)getHitWindow50(beatmap))
			result = OsuScore::HIT::HIT_50;
		else if (delta <= (long)getHitWindowMiss(beatmap))
			result = OsuScore::HIT::HIT_MISS;

		return result;
	}
};

#endif
