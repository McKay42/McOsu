//================ Copyright (c) 2015, PG, All rights reserved. =================//
//
// Purpose:		base class for all gameplay objects
//
// $NoKeywords: $hitobj
//===============================================================================//

#include "OsuHitObject.h"

#include "Engine.h"
#include "ResourceManager.h"
#include "AnimationHandler.h"
#include "ConVar.h"

#include "Osu.h"
#include "OsuSkin.h"
#include "OsuSkinImage.h"
#include "OsuGameRules.h"
#include "OsuBeatmapStandard.h"
#include "OsuBeatmapMania.h"
#include "OsuHUD.h"

ConVar osu_hitresult_draw_300s("osu_hitresult_draw_300s", false);

ConVar osu_hitresult_scale("osu_hitresult_scale", 1.0f);
ConVar osu_hitresult_duration("osu_hitresult_duration", 1.100f, "max duration of the entire hitresult in seconds (this limits all other values)");
ConVar osu_hitresult_animated("osu_hitresult_animated", true, "whether to animate hitresult scales (depending on particle<SCORE>.png, either scale wobble or smooth scale)");
ConVar osu_hitresult_fadein_duration("osu_hitresult_fadein_duration", 0.120f);
ConVar osu_hitresult_fadeout_start_time("osu_hitresult_fadeout_start_time", 0.500f);
ConVar osu_hitresult_fadeout_duration("osu_hitresult_fadeout_duration", 0.600f);

ConVar osu_hitresult_miss_fadein_scale("osu_hitresult_miss_fadein_scale", 2.0f);

ConVar osu_approach_scale_multiplier("osu_approach_scale_multiplier", 3.0f);
ConVar osu_vr_approach_type("osu_vr_approach_type", 0, "0 = linear (default), 1 = quadratic");
ConVar osu_vr_draw_approach_circles("osu_vr_draw_approach_circles", false);
ConVar osu_vr_approach_circles_on_playfield("osu_vr_approach_circles_on_playfield", false);
ConVar osu_vr_approach_circles_on_top("osu_vr_approach_circles_on_top", false);

ConVar osu_timingpoints_force("osu_timingpoints_force", true, "Forces the correct sample type and volume to be used, by getting the active timingpoint through iteration EVERY TIME a hitsound is played (performance!)");

ConVar osu_mod_hd_circle_fadein_start_percent("osu_mod_hd_circle_fadein_start_percent", 1.0f, "hiddenFadeInStartTime = circleTime - approachTime * osu_mod_hd_circle_fadein_start_percent");
ConVar osu_mod_hd_circle_fadein_end_percent("osu_mod_hd_circle_fadein_end_percent", 0.6f, "hiddenFadeInEndTime = circleTime - approachTime * osu_mod_hd_circle_fadein_end_percent");
ConVar osu_mod_hd_circle_fadeout_start_percent("osu_mod_hd_circle_fadeout_start_percent", 0.6f, "hiddenFadeOutStartTime = circleTime - approachTime * osu_mod_hd_circle_fadeout_start_percent");
ConVar osu_mod_hd_circle_fadeout_end_percent("osu_mod_hd_circle_fadeout_end_percent", 0.3f, "hiddenFadeOutEndTime = circleTime - approachTime * osu_mod_hd_circle_fadeout_end_percent");

ConVar osu_mod_target_300_percent("osu_mod_target_300_percent", 0.5f);
ConVar osu_mod_target_100_percent("osu_mod_target_100_percent", 0.7f);
ConVar osu_mod_target_50_percent("osu_mod_target_50_percent", 0.95f);

ConVar osu_relax_offset("osu_relax_offset", 0, "osu!relax always hits -12 ms too early, so set this to -12 (note the negative) if you want it to be the same");

ConVar *OsuHitObject::m_osu_approach_scale_multiplier_ref = &osu_approach_scale_multiplier;
ConVar *OsuHitObject::m_osu_timingpoints_force = &osu_timingpoints_force;
ConVar *OsuHitObject::m_osu_vr_approach_type = &osu_vr_approach_type;
ConVar *OsuHitObject::m_osu_vr_draw_approach_circles = &osu_vr_draw_approach_circles;
ConVar *OsuHitObject::m_osu_vr_approach_circles_on_playfield = &osu_vr_approach_circles_on_playfield;
ConVar *OsuHitObject::m_osu_vr_approach_circles_on_top = &osu_vr_approach_circles_on_top;
ConVar *OsuHitObject::m_osu_relax_offset_ref = &osu_relax_offset;

ConVar *OsuHitObject::m_osu_vr_draw_desktop_playfield = NULL;

unsigned long long OsuHitObject::sortHackCounter = 0;

void OsuHitObject::drawHitResult(Graphics *g, OsuBeatmapStandard *beatmap, Vector2 rawPos, OsuScore::HIT result, float animPercentInv)
{
	drawHitResult(g, beatmap->getSkin(), beatmap->getHitcircleDiameter(), beatmap->getRawHitcircleDiameter(), rawPos, result, animPercentInv);
}

void OsuHitObject::drawHitResult(Graphics *g, OsuSkin *skin, float hitcircleDiameter, float rawHitcircleDiameter, Vector2 rawPos, OsuScore::HIT result, float animPercentInv)
{
	if (animPercentInv <= 0.0f) return;

	const float animPercent = 1.0f - animPercentInv;

	const float fadeInEndPercent = osu_hitresult_fadein_duration.getFloat() / osu_hitresult_duration.getFloat();

	g->setColor(0xffffffff);
	{
		const float fadeOutStartPercent = osu_hitresult_fadeout_start_time.getFloat() / osu_hitresult_duration.getFloat();
		const float fadeOutDurationPercent = osu_hitresult_fadeout_duration.getFloat() / osu_hitresult_duration.getFloat();

		g->setAlpha(clamp<float>(animPercent < fadeInEndPercent ? animPercent / fadeInEndPercent : 1.0f - ((animPercent - fadeOutStartPercent) / fadeOutDurationPercent), 0.0f, 1.0f));
	}
	g->pushTransform();
	{
		const float osuCoordScaleMultiplier = hitcircleDiameter / rawHitcircleDiameter;

		bool doScaleOrRotateAnim = true;
		bool hasParticle = true;
		float hitImageScale = 1.0f;
		switch (result)
		{
		case OsuScore::HIT::HIT_MISS:
			doScaleOrRotateAnim = skin->getHit0()->getNumImages() == 1;
			hitImageScale = (rawHitcircleDiameter / skin->getHit0()->getSizeBaseRaw().x) * osuCoordScaleMultiplier;
			break;

		case OsuScore::HIT::HIT_50:
			doScaleOrRotateAnim = skin->getHit50()->getNumImages() == 1;
			hasParticle = skin->getParticle50() != skin->getMissingTexture();
			hitImageScale = (rawHitcircleDiameter / skin->getHit50()->getSizeBaseRaw().x) * osuCoordScaleMultiplier;
			break;

		case OsuScore::HIT::HIT_100:
			doScaleOrRotateAnim = skin->getHit100()->getNumImages() == 1;
			hasParticle = skin->getParticle100() != skin->getMissingTexture();
			hitImageScale = (rawHitcircleDiameter / skin->getHit100()->getSizeBaseRaw().x) * osuCoordScaleMultiplier;
			break;

		case OsuScore::HIT::HIT_300:
			doScaleOrRotateAnim = skin->getHit300()->getNumImages() == 1;
			hasParticle = skin->getParticle300() != skin->getMissingTexture();
			hitImageScale = (rawHitcircleDiameter / skin->getHit300()->getSizeBaseRaw().x) * osuCoordScaleMultiplier;
			break;



		case OsuScore::HIT::HIT_100K:
			doScaleOrRotateAnim = skin->getHit100k()->getNumImages() == 1;
			hasParticle = skin->getParticle100() != skin->getMissingTexture();
			hitImageScale = (rawHitcircleDiameter / skin->getHit100k()->getSizeBaseRaw().x) * osuCoordScaleMultiplier;
			break;

		case OsuScore::HIT::HIT_300K:
			doScaleOrRotateAnim = skin->getHit300k()->getNumImages() == 1;
			hasParticle = skin->getParticle300() != skin->getMissingTexture();
			hitImageScale = (rawHitcircleDiameter / skin->getHit300k()->getSizeBaseRaw().x) * osuCoordScaleMultiplier;
			break;

		case OsuScore::HIT::HIT_300G:
			doScaleOrRotateAnim = skin->getHit300g()->getNumImages() == 1;
			hasParticle = skin->getParticle300() != skin->getMissingTexture();
			hitImageScale = (rawHitcircleDiameter / skin->getHit300g()->getSizeBaseRaw().x) * osuCoordScaleMultiplier;
			break;
		}

		// non-misses have a special scale animation (the type of which depends on hasParticle)
		float scale = 1.0f;
		if (doScaleOrRotateAnim && osu_hitresult_animated.getBool())
		{
			if (!hasParticle)
			{
				if (animPercent < fadeInEndPercent * 0.8f)
					scale = lerp<float>(0.6f, 1.1f, clamp<float>(animPercent / (fadeInEndPercent * 0.8f), 0.0f, 1.0f));
				else if (animPercent < fadeInEndPercent * 1.2f)
					scale = lerp<float>(1.1f, 0.9f, clamp<float>((animPercent - fadeInEndPercent * 0.8f) / (fadeInEndPercent * 1.2f - fadeInEndPercent * 0.8f), 0.0f, 1.0f));
				else if (animPercent < fadeInEndPercent * 1.4f)
					scale = lerp<float>(0.9f, 1.0f, clamp<float>((animPercent - fadeInEndPercent * 1.2f) / (fadeInEndPercent * 1.4f - fadeInEndPercent * 1.2f), 0.0f, 1.0f));
			}
			else
				scale = lerp<float>(0.9f, 1.05f, clamp<float>(animPercent, 0.0f, 1.0f));

			// TODO: osu draws an additive copy of the hitresult on top (?) with 0.5 alpha anim and negative timing, if the skin hasParticle. in this case only the copy does the wobble anim, while the main result just scales
		}

		switch (result)
		{
		case OsuScore::HIT::HIT_MISS:
			{
				// special case: animated misses don't move down, and skins with version <= 1 also don't move down
				Vector2 downAnim;
				if (skin->getHit0()->getNumImages() < 2 && skin->getVersion() > 1.0f)
					downAnim.y = lerp<float>(-5.0f, 40.0f, clamp<float>(animPercent*animPercent*animPercent, 0.0f, 1.0f)) * osuCoordScaleMultiplier;

				float missScale = 1.0f + clamp<float>((1.0f - (animPercent / fadeInEndPercent)), 0.0f, 1.0f) * (osu_hitresult_miss_fadein_scale.getFloat() - 1.0f);
				if (!osu_hitresult_animated.getBool())
					missScale = 1.0f;

				// TODO: rotation anim (only for all non-animated skins), rot = rng(-0.15f, 0.15f), anim1 = 120 ms to rot, anim2 = rest to rot*2, all ease in

				skin->getHit0()->drawRaw(g, rawPos + downAnim, (doScaleOrRotateAnim ? missScale : 1.0f) * hitImageScale * osu_hitresult_scale.getFloat());
			}
			break;

		case OsuScore::HIT::HIT_50:
			skin->getHit50()->drawRaw(g, rawPos, (doScaleOrRotateAnim ? scale : 1.0f) * hitImageScale * osu_hitresult_scale.getFloat());
			break;

		case OsuScore::HIT::HIT_100:
			skin->getHit100()->drawRaw(g, rawPos, (doScaleOrRotateAnim ? scale : 1.0f) * hitImageScale * osu_hitresult_scale.getFloat());
			break;

		case OsuScore::HIT::HIT_300:
			if (osu_hitresult_draw_300s.getBool())
			{
				skin->getHit300()->drawRaw(g, rawPos, (doScaleOrRotateAnim ? scale : 1.0f) * hitImageScale * osu_hitresult_scale.getFloat());
			}
			break;



		case OsuScore::HIT::HIT_100K:
			skin->getHit100k()->drawRaw(g, rawPos, (doScaleOrRotateAnim ? scale : 1.0f) * hitImageScale * osu_hitresult_scale.getFloat());
			break;

		case OsuScore::HIT::HIT_300K:
			if (osu_hitresult_draw_300s.getBool())
			{
				skin->getHit300k()->drawRaw(g, rawPos, (doScaleOrRotateAnim ? scale : 1.0f) * hitImageScale * osu_hitresult_scale.getFloat());
			}
			break;

		case OsuScore::HIT::HIT_300G:
			if (osu_hitresult_draw_300s.getBool())
			{
				skin->getHit300g()->drawRaw(g, rawPos, (doScaleOrRotateAnim ? scale : 1.0f) * hitImageScale * osu_hitresult_scale.getFloat());
			}
			break;
		}
	}
	g->popTransform();
}



OsuHitObject::OsuHitObject(long time, int sampleType, int comboNumber, bool isEndOfCombo, int colorCounter, int colorOffset, OsuBeatmap *beatmap)
{
	m_iTime = time;
	m_iSampleType = sampleType;
	m_iComboNumber = comboNumber;
	m_bIsEndOfCombo = isEndOfCombo;
	m_iColorCounter = colorCounter;
	m_iColorOffset = colorOffset;
	m_beatmap = beatmap;

	if (m_osu_vr_draw_desktop_playfield == NULL)
		m_osu_vr_draw_desktop_playfield = convar->getConVarByName("osu_vr_draw_desktop_playfield");

	m_fAlpha = 0.0f;
	m_fAlphaWithoutHidden = 0.0f;
	m_fAlphaForApproachCircle = 0.0f;
	m_fApproachScale = 0.0f;
	m_iApproachTime = 0;
	m_iFadeInTime = 0;
	m_iObjectDuration = 0;
	m_iDelta = 0;
	m_iObjectDuration = 0;

	m_bVisible = false;
	m_bFinished = false;
	m_bBlocked = false;
	m_bMisAim = false;
	m_iAutopilotDelta = 0;
	m_bOverrideHDApproachCircle = false;

	m_iStack = 0;

	m_hitresultanim1.time = 0.0f;

	m_iSortHack = sortHackCounter++;
}

void OsuHitObject::draw(Graphics *g)
{
	if ((m_hitresultanim1.time - osu_hitresult_duration.getFloat()) < engine->getTime())
	{
		OsuBeatmapStandard *beatmapStd = dynamic_cast<OsuBeatmapStandard*>(m_beatmap);
		const OsuBeatmapMania *beatmapMania = dynamic_cast<OsuBeatmapMania*>(m_beatmap);

		OsuSkin *skin = m_beatmap->getSkin();
		{
			const long skinAnimationTimeStartOffset = m_iTime + m_iObjectDuration + m_hitresultanim1.delta;

			skin->getHit0()->setAnimationTimeOffset(skinAnimationTimeStartOffset);
			skin->getHit0()->setAnimationFrameClampUp();
			skin->getHit50()->setAnimationTimeOffset(skinAnimationTimeStartOffset);
			skin->getHit50()->setAnimationFrameClampUp();
			skin->getHit100()->setAnimationTimeOffset(skinAnimationTimeStartOffset);
			skin->getHit100()->setAnimationFrameClampUp();
			skin->getHit100k()->setAnimationTimeOffset(skinAnimationTimeStartOffset);
			skin->getHit100k()->setAnimationFrameClampUp();
			skin->getHit300()->setAnimationTimeOffset(skinAnimationTimeStartOffset);
			skin->getHit300()->setAnimationFrameClampUp();
			skin->getHit300g()->setAnimationTimeOffset(skinAnimationTimeStartOffset);
			skin->getHit300g()->setAnimationFrameClampUp();
			skin->getHit300k()->setAnimationTimeOffset(skinAnimationTimeStartOffset);
			skin->getHit300k()->setAnimationFrameClampUp();

			const float animPercentInv = 1.0f - (((engine->getTime() - m_hitresultanim1.time) * m_beatmap->getSpeedMultiplier()) / osu_hitresult_duration.getFloat());

			if (beatmapStd != NULL)
				drawHitResult(g, beatmapStd, beatmapStd->osuCoords2Pixels(m_hitresultanim1.rawPos), m_hitresultanim1.result, animPercentInv);
			else if (beatmapMania != NULL)
				drawHitResult(g, skin, 200.0f, 150.0f, m_hitresultanim1.rawPos, m_hitresultanim1.result, animPercentInv);
		}
	}
}

void OsuHitObject::update(long curPos)
{
	m_fAlphaForApproachCircle = 0.0f;

	m_iApproachTime = (long)OsuGameRules::getApproachTime(m_beatmap);
	m_iFadeInTime = OsuGameRules::getFadeInTime();

	m_iDelta = m_iTime - curPos;

	if (curPos >= (m_iTime - m_iApproachTime) && curPos < (m_iTime + m_iObjectDuration) ) // 1 ms fudge by using >=, shouldn't really be a problem
	{
		// approach circle scale
		const float scale = clamp<float>((float)m_iDelta / (float)m_iApproachTime, 0.0f, 1.0f);
		m_fApproachScale = 1 + scale * osu_approach_scale_multiplier.getFloat();

		// hitobject body fadein
		const long fadeInStart = m_iTime - m_iApproachTime;
		const long fadeInEnd = std::min(m_iTime, m_iTime - m_iApproachTime + m_iFadeInTime); // min() ensures that the fade always finishes at m_iTime (even if the fadeintime is longer than the approachtime)
		m_fAlpha = m_fAlphaWithoutHidden = clamp<float>(1.0f - ((float)(fadeInEnd - curPos) / (float)(fadeInEnd - fadeInStart)), 0.0f, 1.0f);

		if (m_beatmap->getOsu()->getModHD())
		{
			// hidden hitobject body fadein
			const long hiddenFadeInStart = m_iTime - (long)(m_iApproachTime * osu_mod_hd_circle_fadein_start_percent.getFloat());
			const long hiddenFadeInEnd = m_iTime - (long)(m_iApproachTime * osu_mod_hd_circle_fadein_end_percent.getFloat());
			m_fAlpha = clamp<float>(1.0f - ((float)(hiddenFadeInEnd - curPos) / (float)(hiddenFadeInEnd - hiddenFadeInStart)), 0.0f, 1.0f);

			// hidden hitobject body fadeout
			const long hiddenFadeOutStart = m_iTime - (long)(m_iApproachTime * osu_mod_hd_circle_fadeout_start_percent.getFloat());
			const long hiddenFadeOutEnd = m_iTime - (long)(m_iApproachTime * osu_mod_hd_circle_fadeout_end_percent.getFloat());
			if (curPos >= hiddenFadeOutStart)
				m_fAlpha = clamp<float>(((float)(hiddenFadeOutEnd - curPos) / (float)(hiddenFadeOutEnd - hiddenFadeOutStart)), 0.0f, 1.0f);
		}

		// approach circle fadein (doubled fadeintime)
		const long approachCircleFadeStart = m_iTime - m_iApproachTime;
		const long approachCircleFadeEnd = std::min(m_iTime, m_iTime - m_iApproachTime + 2*m_iFadeInTime); // min() ensures that the fade always finishes at m_iTime (even if the fadeintime is longer than the approachtime)
		m_fAlphaForApproachCircle = clamp<float>(1.0f - ((float)(approachCircleFadeEnd - curPos) / (float)(approachCircleFadeEnd - approachCircleFadeStart)), 0.0f, 1.0f);

		m_bVisible = true;
	}
	else
	{
		m_fApproachScale = 1.0f;
		m_bVisible = false;
	}
}

void OsuHitObject::addHitResult(OsuScore::HIT result, long delta, bool isEndOfCombo, Vector2 posRaw, float targetDelta, float targetAngle, bool ignoreOnHitErrorBar, bool ignoreCombo, bool ignoreHealth)
{
	if (m_beatmap->getOsu()->getModTarget() && result != OsuScore::HIT::HIT_MISS && targetDelta >= 0.0f)
	{
		if (targetDelta < osu_mod_target_300_percent.getFloat() && (result == OsuScore::HIT::HIT_300 || result == OsuScore::HIT::HIT_100))
			result = OsuScore::HIT::HIT_300;
		else if (targetDelta < osu_mod_target_100_percent.getFloat())
			result = OsuScore::HIT::HIT_100;
		else if (targetDelta < osu_mod_target_50_percent.getFloat())
			result = OsuScore::HIT::HIT_50;
		else
			result = OsuScore::HIT::HIT_MISS;

		m_beatmap->getOsu()->getHUD()->addTarget(targetDelta, targetAngle);
	}

	const OsuScore::HIT returnedHit = m_beatmap->addHitResult(this, result, delta, isEndOfCombo, ignoreOnHitErrorBar, false, ignoreCombo, false, ignoreHealth);

	HITRESULTANIM hitresultanim;
	{
		hitresultanim.result = (returnedHit != OsuScore::HIT::HIT_MISS ? returnedHit : result);
		hitresultanim.rawPos = posRaw;
		hitresultanim.delta = delta;
		hitresultanim.time = engine->getTime();
	}
	m_hitresultanim1 = hitresultanim;

	// NOTE: in the future, can easily support 2 results for e.g. slider headcircle scorev2 stuff (2 results should be enough for any hitobject)
}

void OsuHitObject::onReset(long curPos)
{
	m_bMisAim = false;
	m_iAutopilotDelta = 0;

	m_hitresultanim1.time = 0.0f;
}
