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

	//********************//
	//  Positional Audio  //
	//********************//

	static float osuCoords2Pan(float x)
	{
		return (x / (float)OsuGameRules::OSU_COORD_WIDTH - 0.5f) * 0.8f;
	}



	//************************//
	//	Hitobject Animations  //
	//************************//

	static ConVar osu_hitobject_hittable_dim;
	static ConVar osu_hitobject_hittable_dim_start_percent;
	static ConVar osu_hitobject_hittable_dim_duration;

	static ConVar osu_hitobject_fade_in_time;

	static ConVar osu_hitobject_fade_out_time;
	static ConVar osu_hitobject_fade_out_time_speed_multiplier_min;

	static ConVar osu_circle_fade_out_scale;

	static ConVar osu_slider_followcircle_fadein_fade_time;
	static ConVar osu_slider_followcircle_fadeout_fade_time;
	static ConVar osu_slider_followcircle_fadein_scale;
	static ConVar osu_slider_followcircle_fadein_scale_time;
	static ConVar osu_slider_followcircle_fadeout_scale;
	static ConVar osu_slider_followcircle_fadeout_scale_time;
	static ConVar osu_slider_followcircle_tick_pulse_time;
	static ConVar osu_slider_followcircle_tick_pulse_scale;

	static ConVar osu_spinner_fade_out_time_multiplier;

	static float getFadeOutTime(OsuBeatmap *beatmap) // this scales the fadeout duration with the current speed multiplier
	{
		return osu_hitobject_fade_out_time.getFloat() * (1.0f / std::max(beatmap->getSpeedMultiplier(), osu_hitobject_fade_out_time_speed_multiplier_min.getFloat()));
	}

	static inline long getFadeInTime() {return (long)osu_hitobject_fade_in_time.getInt();}



	//*********************//
	//	Experimental Mods  //
	//*********************//

	static ConVar osu_mod_fps;
	static ConVar osu_mod_no50s;
	static ConVar osu_mod_no100s;
	static ConVar osu_mod_ming3012;
	static ConVar osu_mod_millhioref;
	static ConVar osu_mod_millhioref_multiplier;
	static ConVar osu_mod_mafham;
	static ConVar osu_mod_mafham_render_livesize;
	static ConVar osu_stacking_ar_override;
	static ConVar osu_mod_halfwindow;
	static ConVar osu_mod_halfwindow_allow_300s;



	//********************//
	//	Hitobject Timing  //
	//********************//

	static ConVar osu_approachtime_min;
	static ConVar osu_approachtime_mid;
	static ConVar osu_approachtime_max;

	static ConVar osu_hitwindow_300_min;
	static ConVar osu_hitwindow_300_mid;
	static ConVar osu_hitwindow_300_max;
	static ConVar osu_hitwindow_100_min;
	static ConVar osu_hitwindow_100_mid;
	static ConVar osu_hitwindow_100_max;
	static ConVar osu_hitwindow_50_min;
	static ConVar osu_hitwindow_50_mid;
	static ConVar osu_hitwindow_50_max;
	static ConVar osu_hitwindow_miss;

	// ignore all mods and overrides
	static inline float getRawMinApproachTime()
	{
		return osu_approachtime_min.getFloat();
	}
	static inline float getRawMidApproachTime()
	{
		return osu_approachtime_mid.getFloat();
	}
	static inline float getRawMaxApproachTime()
	{
		return osu_approachtime_max.getFloat();
	}

	// respect mods and overrides
	static inline float getMinApproachTime()
	{
		return getRawMinApproachTime() * (osu_mod_millhioref.getBool() ? osu_mod_millhioref_multiplier.getFloat() : 1.0f);
	}
	static inline float getMidApproachTime()
	{
		return getRawMidApproachTime() * (osu_mod_millhioref.getBool() ? osu_mod_millhioref_multiplier.getFloat() : 1.0f);
	}
	static inline float getMaxApproachTime()
	{
		return getRawMaxApproachTime() * (osu_mod_millhioref.getBool() ? osu_mod_millhioref_multiplier.getFloat() : 1.0f);
	}

	static inline float getMinHitWindow300() {return osu_hitwindow_300_min.getFloat();}
	static inline float getMidHitWindow300() {return osu_hitwindow_300_mid.getFloat();}
	static inline float getMaxHitWindow300() {return osu_hitwindow_300_max.getFloat();}

	static inline float getMinHitWindow100() {return osu_hitwindow_100_min.getFloat();}
	static inline float getMidHitWindow100() {return osu_hitwindow_100_mid.getFloat();}
	static inline float getMaxHitWindow100() {return osu_hitwindow_100_max.getFloat();}

	static inline float getMinHitWindow50() {return osu_hitwindow_50_min.getFloat();}
	static inline float getMidHitWindow50() {return osu_hitwindow_50_mid.getFloat();}
	static inline float getMaxHitWindow50() {return osu_hitwindow_50_max.getFloat();}

	// AR 5 -> 1200 ms
	static float mapDifficultyRange(float scaledDiff, float min, float mid, float max)
	{
	    if (scaledDiff > 5.0f)
	        return mid + (max - mid) * (scaledDiff - 5.0f) / 5.0f;

	    if (scaledDiff < 5.0f)
	        return mid - (mid - min) * (5.0f - scaledDiff) / 5.0f;

	    return mid;
	}

	static double mapDifficultyRangeDouble(double scaledDiff, double min, double mid, double max)
	{
	    if (scaledDiff > 5.0)
	        return mid + (max - mid) * (scaledDiff - 5.0) / 5.0;

	    if (scaledDiff < 5.0)
	        return mid - (mid - min) * (5.0 - scaledDiff) / 5.0;

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
	static float getRawApproachRateForSpeedMultiplier(float approachTime, float speedMultiplier) // ignore all mods and overrides
	{
		return mapDifficultyRangeInv(approachTime * (1.0f / speedMultiplier), getRawMinApproachTime(), getRawMidApproachTime(), getRawMaxApproachTime());
	}
	static float getApproachRateForSpeedMultiplier(OsuBeatmap *beatmap, float speedMultiplier)
	{
		return mapDifficultyRangeInv((float)getApproachTime(beatmap) * (1.0f / speedMultiplier), getMinApproachTime(), getMidApproachTime(), getMaxApproachTime());
	}
	static float getApproachRateForSpeedMultiplier(OsuBeatmap *beatmap) // respect all mods and overrides
	{
		return getApproachRateForSpeedMultiplier(beatmap, beatmap->getOsu()->getSpeedMultiplier());
	}
	static float getRawApproachRateForSpeedMultiplier(OsuBeatmap *beatmap) // ignore AR override
	{
		return mapDifficultyRangeInv((float)getRawApproachTime(beatmap) * (1.0f / beatmap->getOsu()->getSpeedMultiplier()), getMinApproachTime(), getMidApproachTime(), getMaxApproachTime());
	}
	static float getConstantApproachRateForSpeedMultiplier(OsuBeatmap *beatmap) // ignore AR override, keep AR consistent through speed changes
	{
		return mapDifficultyRangeInv((float)getRawApproachTime(beatmap) * beatmap->getOsu()->getSpeedMultiplier(), getMinApproachTime(), getMidApproachTime(), getMaxApproachTime());
	}
	static float getRawConstantApproachRateForSpeedMultiplier(float approachTime, float speedMultiplier) // ignore all mods and overrides, keep AR consistent through speed changes
	{
		return mapDifficultyRangeInv(approachTime * speedMultiplier, getRawMinApproachTime(), getRawMidApproachTime(), getRawMaxApproachTime());
	}

	// 50 ms -> OD 5
	static float getRawOverallDifficultyForSpeedMultiplier(float hitWindow300, float speedMultiplier) // ignore all mods and overrides
	{
		return mapDifficultyRangeInv(hitWindow300 * (1.0f / speedMultiplier), getMinHitWindow300(), getMidHitWindow300(), getMaxHitWindow300());
	}
	static float getOverallDifficultyForSpeedMultiplier(OsuBeatmap *beatmap, float speedMultiplier) // respect all mods and overrides
	{
		return mapDifficultyRangeInv((float)getHitWindow300(beatmap) * (1.0f / speedMultiplier), getMinHitWindow300(), getMidHitWindow300(), getMaxHitWindow300());
	}
	static float getOverallDifficultyForSpeedMultiplier(OsuBeatmap *beatmap) // respect all mods and overrides
	{
		return getOverallDifficultyForSpeedMultiplier(beatmap, beatmap->getOsu()->getSpeedMultiplier());
	}
	static float getRawOverallDifficultyForSpeedMultiplier(OsuBeatmap *beatmap) // ignore OD override
	{
		return mapDifficultyRangeInv((float)getRawHitWindow300(beatmap) * (1.0f / beatmap->getOsu()->getSpeedMultiplier()), getMinHitWindow300(), getMidHitWindow300(), getMaxHitWindow300());
	}
	static float getConstantOverallDifficultyForSpeedMultiplier(OsuBeatmap *beatmap) // ignore OD override, keep OD consistent through speed changes
	{
		return mapDifficultyRangeInv((float)getRawHitWindow300(beatmap) * beatmap->getOsu()->getSpeedMultiplier(), getMinHitWindow300(), getMidHitWindow300(), getMaxHitWindow300());
	}
	static float getRawConstantOverallDifficultyForSpeedMultiplier(float hitWindow300, float speedMultiplier) // ignore all mods and overrides, keep OD consistent through speed changes
	{
		return mapDifficultyRangeInv(hitWindow300 * speedMultiplier, getMinHitWindow300(), getMidHitWindow300(), getMaxHitWindow300());
	}

	static float getRawApproachTime(float AR) // ignore all mods and overrides
	{
		return mapDifficultyRange(AR, getRawMinApproachTime(), getRawMidApproachTime(), getRawMaxApproachTime());
	}
	static float getApproachTime(OsuBeatmap *beatmap)
	{
		return osu_mod_mafham.getBool() ? beatmap->getLength()*2 : mapDifficultyRange(beatmap->getAR(), getMinApproachTime(), getMidApproachTime(), getMaxApproachTime());
	}
	static float getRawApproachTime(OsuBeatmap *beatmap) // ignore AR override
	{
		return osu_mod_mafham.getBool() ? beatmap->getLength()*2 : mapDifficultyRange(beatmap->getRawAR(), getMinApproachTime(), getMidApproachTime(), getMaxApproachTime());
	}
	static float getApproachTimeForStacking(float AR)
	{
		return mapDifficultyRange(osu_stacking_ar_override.getFloat() < 0.0f ? AR : osu_stacking_ar_override.getFloat(), getMinApproachTime(), getMidApproachTime(), getMaxApproachTime());
	}
	static float getApproachTimeForStacking(OsuBeatmap *beatmap)
	{
		return mapDifficultyRange(osu_stacking_ar_override.getFloat() < 0.0f ? beatmap->getAR() : osu_stacking_ar_override.getFloat(), getMinApproachTime(), getMidApproachTime(), getMaxApproachTime());
	}

	static float getRawHitWindow300(float OD) // ignore all mods and overrides
	{
		return mapDifficultyRange(OD, getMinHitWindow300(), getMidHitWindow300(), getMaxHitWindow300());
	}
	static float getHitWindow300(OsuBeatmap *beatmap)
	{
		return mapDifficultyRange(beatmap->getOD(), getMinHitWindow300(), getMidHitWindow300(), getMaxHitWindow300());
	}
	static float getRawHitWindow300(OsuBeatmap *beatmap) // ignore OD override
	{
		return mapDifficultyRange(beatmap->getRawOD(), getMinHitWindow300(), getMidHitWindow300(), getMaxHitWindow300());
	}

	static float getRawHitWindow100(float OD) // ignore all mods and overrides
	{
		return mapDifficultyRange(OD, getMinHitWindow100(), getMidHitWindow100(), getMaxHitWindow100());
	}
	static float getHitWindow100(OsuBeatmap *beatmap)
	{
		return mapDifficultyRange(beatmap->getOD(), getMinHitWindow100(), getMidHitWindow100(), getMaxHitWindow100());
	}

	static float getRawHitWindow50(float OD) // ignore all mods and overrides
	{
		return mapDifficultyRange(OD, getMinHitWindow50(), getMidHitWindow50(), getMaxHitWindow50());
	}
	static float getHitWindow50(OsuBeatmap *beatmap)
	{
		return mapDifficultyRange(beatmap->getOD(), getMinHitWindow50(), getMidHitWindow50(), getMaxHitWindow50());
	}

	static inline float getHitWindowMiss(OsuBeatmap *beatmap)
	{
		return osu_hitwindow_miss.getFloat(); // opsu is using this here: (500.0f - (beatmap->getOD() * 10.0f)), while osu is just using 400 absolute ms hardcoded, not sure why
	}

	static float getSpinnerSpinsPerSecond(OsuBeatmap *beatmap) // raw spins required per second
	{
		return mapDifficultyRange(beatmap->getOD(), 3.0f, 5.0f, 7.5f);
	}
	static float getSpinnerRotationsForSpeedMultiplier(OsuBeatmap *beatmap, long spinnerDuration, float speedMultiplier)
	{
		///return (int)((float)spinnerDuration / 1000.0f * getSpinnerSpinsPerSecond(beatmap)); // actual
		return (int)((((float)spinnerDuration / 1000.0f * getSpinnerSpinsPerSecond(beatmap)) * 0.5f) * (std::min(1.0f / speedMultiplier, 1.0f))); // Mc
	}
	static float getSpinnerRotationsForSpeedMultiplier(OsuBeatmap *beatmap, long spinnerDuration) // spinner length compensated rotations // respect all mods and overrides
	{
		return getSpinnerRotationsForSpeedMultiplier(beatmap, spinnerDuration, beatmap->getOsu()->getSpeedMultiplier());
	}

	static OsuScore::HIT getHitResult(long delta, OsuBeatmap *beatmap)
	{
		if (osu_mod_halfwindow.getBool() && delta > 0 && delta <= (long)getHitWindowMiss(beatmap))
		{
			if (!osu_mod_halfwindow_allow_300s.getBool())
				return OsuScore::HIT::HIT_MISS;
			else if (delta > (long)getHitWindow300(beatmap))
				return OsuScore::HIT::HIT_MISS;
		}

		delta = std::abs(delta);

		OsuScore::HIT result = OsuScore::HIT::HIT_NULL;

		if (!osu_mod_ming3012.getBool() && !osu_mod_no100s.getBool() && !osu_mod_no50s.getBool())
		{
			if (delta <= (long)getHitWindow300(beatmap))
				result = OsuScore::HIT::HIT_300;
			else if (delta <= (long)getHitWindow100(beatmap))
				result = OsuScore::HIT::HIT_100;
			else if (delta <= (long)getHitWindow50(beatmap))
				result = OsuScore::HIT::HIT_50;
			else if (delta <= (long)getHitWindowMiss(beatmap))
				result = OsuScore::HIT::HIT_MISS;
		}
		else if (osu_mod_ming3012.getBool())
		{
			if (delta <= (long)getHitWindow300(beatmap))
				result = OsuScore::HIT::HIT_300;
			else if (delta <= (long)getHitWindow50(beatmap))
				result = OsuScore::HIT::HIT_50;
			else if (delta <= (long)getHitWindowMiss(beatmap))
				result = OsuScore::HIT::HIT_MISS;
		}
		else if (osu_mod_no100s.getBool())
		{
			if (delta <= (long)getHitWindow300(beatmap))
				result = OsuScore::HIT::HIT_300;
			else if (delta <= (long)getHitWindowMiss(beatmap))
				result = OsuScore::HIT::HIT_MISS;
		}
		else if (osu_mod_no50s.getBool())
		{
			if (delta <= (long)getHitWindow300(beatmap))
				result = OsuScore::HIT::HIT_300;
			else if (delta <= (long)getHitWindow100(beatmap))
				result = OsuScore::HIT::HIT_100;
			else if (delta <= (long)getHitWindowMiss(beatmap))
				result = OsuScore::HIT::HIT_MISS;
		}

		return result;
	}



	//*********************//
	//	Hitobject Scaling  //
	//*********************//

	static ConVar osu_slider_followcircle_size_multiplier;

	// "Builds of osu! up to 2013-05-04 had the gamefield being rounded down, which caused incorrect radius calculations
	// in widescreen cases. This ratio adjusts to allow for old replays to work post-fix, which in turn increases the lenience
	// for all plays, but by an amount so small it should only be effective in replays."
	static constexpr const float broken_gamefield_rounding_allowance = 1.00041f;

	static float getRawHitCircleScale(float CS)
	{
		return std::max(0.0f, ((1.0f - 0.7f * (CS - 5.0f) / 5.0f) / 2.0f) * broken_gamefield_rounding_allowance);
	}

	static float getRawHitCircleDiameter(float CS)
	{
		return getRawHitCircleScale(CS) * 128.0f; // gives the circle diameter in osu!pixels, goes negative above CS 12.1429
	}

	static float getHitCircleXMultiplier(Osu *osu)
	{
		return getPlayfieldSize(osu).x / OSU_COORD_WIDTH; // scales osu!pixels to the actual playfield size
	}

	static float getHitCircleDiameter(OsuBeatmap *beatmap)
	{
		return getRawHitCircleDiameter(beatmap->getCS()) * getHitCircleXMultiplier(beatmap->getOsu());
	}



	//*************//
	//	Playfield  //
	//*************//

	static ConVar osu_playfield_border_top_percent;
	static ConVar osu_playfield_border_bottom_percent;

	static const int OSU_COORD_WIDTH = 512;
	static const int OSU_COORD_HEIGHT = 384;

	static float getPlayfieldScaleFactor(Osu *osu)
	{
		const int engineScreenWidth = osu->getScreenWidth();
		const int topBorderSize = osu_playfield_border_top_percent.getFloat()*osu->getScreenHeight();
		const int bottomBorderSize = osu_playfield_border_bottom_percent.getFloat()*osu->getScreenHeight();
		const int engineScreenHeight = osu->getScreenHeight() - bottomBorderSize - topBorderSize;

		return osu->getScreenWidth()/(float)OSU_COORD_WIDTH > engineScreenHeight/(float)OSU_COORD_HEIGHT ? engineScreenHeight/(float)OSU_COORD_HEIGHT : engineScreenWidth/(float)OSU_COORD_WIDTH;
	}

	static Vector2 getPlayfieldSize(Osu *osu)
	{
		const float scaleFactor = getPlayfieldScaleFactor(osu);

		return Vector2(OSU_COORD_WIDTH*scaleFactor, OSU_COORD_HEIGHT*scaleFactor);
	}

	static Vector2 getPlayfieldOffset(Osu *osu)
	{
		const Vector2 playfieldSize = getPlayfieldSize(osu);
		const int bottomBorderSize = osu_playfield_border_bottom_percent.getFloat()*osu->getScreenHeight();
		int playfieldYOffset = (osu->getScreenHeight()/2.0f - (playfieldSize.y/2.0f)) - bottomBorderSize;

		if (osu_mod_fps.getBool())
			playfieldYOffset = 0; // first person mode doesn't need any offsets, cursor/crosshair should be centered on screen

		return Vector2((osu->getScreenWidth()-playfieldSize.x)/2.0f, (osu->getScreenHeight()-playfieldSize.y)/2.0f + playfieldYOffset);
	}

	static Vector2 getPlayfieldCenter(Osu *osu)
	{
		const float scaleFactor = getPlayfieldScaleFactor(osu);
		const Vector2 playfieldOffset = getPlayfieldOffset(osu);

		return Vector2((OSU_COORD_WIDTH/2)*scaleFactor + playfieldOffset.x, (OSU_COORD_HEIGHT/2)*scaleFactor + playfieldOffset.y);
	}
};

#endif
