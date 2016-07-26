//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		difficulty & playfield behaviour
//
// $NoKeywords: $osugr
//===============================================================================//

#ifndef OSUGAMERULES_H
#define OSUGAMERULES_H

#include "Osu.h"
#include "OsuBeatmap.h"

#include "ConVar.h"

class OsuGameRules
{
public:

	//************************//
	//	Hitobject Animations  //
	//************************//

	static ConVar osu_circle_fade_out_time;
	static ConVar osu_circle_fade_out_scale;

	static ConVar osu_slider_followcircle_fadein_fade_time;
	static ConVar osu_slider_followcircle_fadeout_fade_time;
	static ConVar osu_slider_followcircle_fadein_scale;
	static ConVar osu_slider_followcircle_fadein_scale_time;
	static ConVar osu_slider_followcircle_fadeout_scale;
	static ConVar osu_slider_followcircle_fadeout_scale_time;
	static ConVar osu_slider_followcircle_tick_pulse_time;
	static ConVar osu_slider_followcircle_tick_pulse_scale;



	//*********************//
	//	Experimental Mods  //
	//*********************//

	static ConVar osu_mod_ming3012;



	//********************//
	//	Hitobject Timing  //
	//********************//

	// AR 5 -> 1200 ms
	static float mapDifficultyRange(float scaledDiff, float min, float mid, float max)
	{
	    if (scaledDiff > 5.0f)
	        return mid + (max - mid) * (scaledDiff - 5.0f) / 5.0f;

	    if (scaledDiff < 5.0f)
	        return mid - (mid - min) * (5.0f - scaledDiff) / 5.0f;

	    return mid;
	}

	// 1200 ms -> AR 5
	static float mapDifficultyRangeInv(float val, float min, float mid, float max)
	{
		if (val < mid) // > 5.0f (inverted)
			return ((val*5.0f - mid*5.0f) / (max - mid)) + 5.0f;

		if (val > mid) // < 5.0f (inverted)
			return 5.0f - ((mid*5.0f - val*5.0f) / (mid - min));

	    return 5.0f;
	}

	// 1200 ms -> AR 5
	static float getApproachRateForSpeedMultiplier(OsuBeatmap *beatmap) // respect all mods and overrides
	{
		return clamp<float>(mapDifficultyRangeInv((float)getApproachTime(beatmap) * (1.0f / beatmap->getOsu()->getSpeedMultiplier()), 1800.0f, 1200.0f, 450.0f), 0.0f, 12.5f);
	}
	static float getRawApproachRateForSpeedMultiplier(OsuBeatmap *beatmap) // ignore AR override
	{
		return clamp<float>(mapDifficultyRangeInv((float)getRawApproachTime(beatmap) * (1.0f / beatmap->getOsu()->getSpeedMultiplier()), 1800.0f, 1200.0f, 450.0f), 0.0f, 12.5f);
	}
	static float getConstantApproachRateForSpeedMultiplier(OsuBeatmap *beatmap) // ignore AR override, keep AR consistent through speed changes
	{
		return clamp<float>(mapDifficultyRangeInv((float)getRawApproachTime(beatmap) * beatmap->getOsu()->getSpeedMultiplier(), 1800.0f, 1200.0f, 450.0f), 0.0f, 12.5f);
	}

	// 50 ms -> OD 5
	static float getOverallDifficultyForSpeedMultiplier(OsuBeatmap *beatmap) // respect all mods and overrides
	{
		return clamp<float>(mapDifficultyRangeInv((float)getHitWindow300(beatmap) * (1.0f / beatmap->getOsu()->getSpeedMultiplier()), 80.0f, 50.0f, 20.0f), 0.0f, 12.5f);
	}
	static float getRawOverallDifficultyForSpeedMultiplier(OsuBeatmap *beatmap) // ignore OD override
	{
		return clamp<float>(mapDifficultyRangeInv((float)getRawHitWindow300(beatmap) * (1.0f / beatmap->getOsu()->getSpeedMultiplier()), 80.0f, 50.0f, 20.0f), 0.0f, 12.5f);
	}
	static float getConstantOverallDifficultyForSpeedMultiplier(OsuBeatmap *beatmap) // ignore OD override, keep OD consistent through speed changes
	{
		return clamp<float>(mapDifficultyRangeInv((float)getRawHitWindow300(beatmap) * beatmap->getOsu()->getSpeedMultiplier(), 80.0f, 50.0f, 20.0f), 0.0f, 12.5f);
	}

	static long getApproachTime(OsuBeatmap *beatmap)
	{
		return (long)mapDifficultyRange(beatmap->getAR(), 1800.0f, 1200.0f, 450.0f);
	}
	static long getRawApproachTime(OsuBeatmap *beatmap) // ignore AR override
	{
		return (long)mapDifficultyRange(beatmap->getRawAR(), 1800.0f, 1200.0f, 450.0f);
	}

	static long getHitWindow300(OsuBeatmap *beatmap)
	{
		return (long)mapDifficultyRange(beatmap->getOD(), 80.0f, 50.0f, 20.0f);
	}
	static long getRawHitWindow300(OsuBeatmap *beatmap) // ignore OD override
	{
		return (long)mapDifficultyRange(beatmap->getRawOD(), 80.0f, 50.0f, 20.0f);
	}

	static long getHitWindow100(OsuBeatmap *beatmap)
	{
		return (long)mapDifficultyRange(beatmap->getOD(), 140.0f, 100.0f, 60.0f);
	}

	static long getHitWindow50(OsuBeatmap *beatmap)
	{
		return (long)mapDifficultyRange(beatmap->getOD(), 200.0f, 150.0f, 100.0f);
	}

	static inline long getHitWindowMiss(OsuBeatmap *beatmap)
	{
		return (long)(500.0f - (beatmap->getOD() * 10.0f));
	}

	static float getSpinnerSpins(OsuBeatmap *beatmap)
	{
		return mapDifficultyRange(beatmap->getOD(), 3.0f, 5.0f, 7.5f);
	}

	static OsuBeatmap::HIT getHitResult(long delta, OsuBeatmap *beatmap)
	{
		delta = std::abs(delta);

		OsuBeatmap::HIT result = OsuBeatmap::HIT_NULL;

		if (!osu_mod_ming3012.getBool())
		{
			if (delta <= getHitWindow300(beatmap))
				result = OsuBeatmap::HIT_300;
			else if (delta <= getHitWindow100(beatmap))
				result = OsuBeatmap::HIT_100;
			else if (delta <= getHitWindow50(beatmap))
				result = OsuBeatmap::HIT_50;
			else if (delta <= getHitWindowMiss(beatmap))
				result = OsuBeatmap::HIT_MISS;
		}
		else
		{
			if (delta <= getHitWindow300(beatmap))
				result = OsuBeatmap::HIT_300;
			else if (delta <= getHitWindow50(beatmap))
				result = OsuBeatmap::HIT_50;
			else if (delta <= getHitWindowMiss(beatmap))
				result = OsuBeatmap::HIT_MISS;
		}

		return result;
	}



	//*********************//
	//	Hitobject Scaling  //
	//*********************//

	static float getRawHitCircleDiameter(OsuBeatmap *beatmap)
	{
		return 108.848f - ((beatmap->getCS()) * 8.9646f);
	}

	// note: these calculations are correct, even though it may not seem so due to magic numbers in osu_playfield_border_bottom_percent and osu_playfield_border_top_percent
	static float getHitCircleXMultiplier()
	{
		int swidth = Osu::getScreenWidth();
		int sheight = Osu::getScreenHeight();

		if (swidth * 3 > sheight * 4)
			swidth = sheight * 4 / 3;
		else
			sheight = swidth * 3 / 4;

		const float xMultiplier = swidth / 640.0f;
		//const float yMultiplier = sheight / 480.0f;

		return xMultiplier;
	}

	static float getHitCircleDiameter(OsuBeatmap *beatmap)
	{
		return getRawHitCircleDiameter(beatmap) * getHitCircleXMultiplier();
	}



	//*************//
	//	Playfield  //
	//*************//

	static ConVar osu_playfield_border_top_percent;		// why peppy WHYYYYYYYYYYYYYYYY, is it so hard to make the playfield have a normal constant size and offset ffs
	static ConVar osu_playfield_border_bottom_percent;	// i know this is due to the hp bar, but changing the size and position of the entire playfield just for the fucking hp bar is bullshit

	static const int OSU_COORD_WIDTH = 512;
	static const int OSU_COORD_HEIGHT = 384;

	static float getPlayfieldScaleFactor()
	{
		const int engineScreenWidth = Osu::getScreenWidth();
		const int topBorderSize = osu_playfield_border_top_percent.getFloat()*Osu::getScreenHeight();
		const int bottomBorderSize = osu_playfield_border_bottom_percent.getFloat()*Osu::getScreenHeight();
		const int engineScreenHeight = Osu::getScreenHeight() - bottomBorderSize - topBorderSize;

		return Osu::getScreenWidth()/(float)OSU_COORD_WIDTH > engineScreenHeight/(float)OSU_COORD_HEIGHT ? engineScreenHeight / (float)OSU_COORD_HEIGHT : engineScreenWidth / (float)OSU_COORD_WIDTH;
	}

	static Vector2 getPlayfieldSize()
	{
		const float scaleFactor = getPlayfieldScaleFactor();

		return Vector2(OSU_COORD_WIDTH*scaleFactor, OSU_COORD_HEIGHT*scaleFactor);
	}

	static Vector2 getPlayfieldOffset()
	{
		const Vector2 playfieldSize = getPlayfieldSize();
		const int bottomBorderSize = osu_playfield_border_bottom_percent.getFloat()*Osu::getScreenHeight();
		const int playfieldYOffset = (Osu::getScreenHeight()/2.0f - (playfieldSize.y/2.0f)) - bottomBorderSize;

		return Vector2((Osu::getScreenWidth()-playfieldSize.x)/2.0f, (Osu::getScreenHeight()-playfieldSize.y)/2.0f + playfieldYOffset);
	}

	static Vector2 getPlayfieldCenter()
	{
		const float scaleFactor = getPlayfieldScaleFactor();
		const Vector2 playfieldOffset = getPlayfieldOffset();

		return Vector2((OSU_COORD_WIDTH/2)*scaleFactor + playfieldOffset.x, (OSU_COORD_HEIGHT/2)*scaleFactor + playfieldOffset.y);
	}
};

#endif
