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
ConVar osu_hitresult_duration("osu_hitresult_duration", 1.22f);
ConVar osu_hitresult_fadeout_start_percent("osu_hitresult_fadeout_start_percent", 0.4f, "start fading out after this many percent of osu_hitresult_duration have passed");

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
unsigned long long OsuHitObject::sortHackCounter = 0;

void OsuHitObject::drawHitResult(Graphics *g, OsuBeatmapStandard *beatmap, Vector2 rawPos, OsuScore::HIT result, float animPercent, float defaultAnimPercent)
{
	drawHitResult(g, beatmap->getSkin(), beatmap->getHitcircleDiameter(), beatmap->getRawHitcircleDiameter(), rawPos, result, animPercent, defaultAnimPercent);
}

void OsuHitObject::drawHitResult(Graphics *g, OsuSkin *skin, float hitcircleDiameter, float rawHitcircleDiameter, Vector2 rawPos, OsuScore::HIT result, float animPercent, float defaultAnimPercent)
{
	if (defaultAnimPercent <= 0.0f) return;

	const float osuCoordScaleMultiplier = hitcircleDiameter / rawHitcircleDiameter;

	g->setColor(0xffffffff);
	g->setAlpha(clamp<float>(defaultAnimPercent / (1.0f - osu_hitresult_fadeout_start_percent.getFloat()), 0.0f, 1.0f));
	g->pushTransform();
	{
		float hitImageScale = 1.0f;
		switch (result)
		{
		case OsuScore::HIT::HIT_MISS:
			hitImageScale = (rawHitcircleDiameter / skin->getHit0()->getSizeBaseRaw().x) * osuCoordScaleMultiplier;
			break;
		case OsuScore::HIT::HIT_50:
			hitImageScale = (rawHitcircleDiameter / skin->getHit50()->getSizeBaseRaw().x) * osuCoordScaleMultiplier;
			break;
		case OsuScore::HIT::HIT_100:
			hitImageScale = (rawHitcircleDiameter / skin->getHit100()->getSizeBaseRaw().x) * osuCoordScaleMultiplier;
			break;
		case OsuScore::HIT::HIT_300:
			hitImageScale = (rawHitcircleDiameter / skin->getHit300()->getSizeBaseRaw().x) * osuCoordScaleMultiplier;
			break;
		}

		switch (result)
		{
		case OsuScore::HIT::HIT_MISS:
			// special case: animated misses don't move down, and skins with version <= 1 also don't move down
			if (skin->getHit0()->getNumImages() < 2 && skin->getVersion() > 1.0f)
				g->translate(0, (1.0f-defaultAnimPercent)*(1.0f-defaultAnimPercent)*(1.0f-defaultAnimPercent)*skin->getHit0()->getSize().y*1.25f);

			// TODO: rotation (only for all non-animated skins), rot = rng(-0.15f, 0.15f), anim1 = 120 ms to rot, anim2 = rest to rot*2, all ease in

			/*
			g->pushTransform();
			g->translate(rawPos.x, rawPos.y);
			g->setColor(0xffffffff);
			g->drawString(engine->getResourceManager()->getFont("FONT_DEFAULT"), UString::format("animPercent = %f, frame = %i\n", animPercent, skin->getHit0()->getFrameNumber()));
			g->popTransform();
			*/

			skin->getHit0()->drawRaw(g, rawPos, hitImageScale*osu_hitresult_scale.getFloat());
			break;
		case OsuScore::HIT::HIT_50:
			skin->getHit50()->drawRaw(g, rawPos, hitImageScale*osu_hitresult_scale.getFloat());
			break;
		case OsuScore::HIT::HIT_100:
			skin->getHit100()->drawRaw(g, rawPos, hitImageScale*osu_hitresult_scale.getFloat());
			break;
		case OsuScore::HIT::HIT_300:
			if (osu_hitresult_draw_300s.getBool())
			{
				skin->getHit300()->drawRaw(g, rawPos, hitImageScale*osu_hitresult_scale.getFloat());
			}
			break;
		}
	}
	g->popTransform();
}



OsuHitObject::OsuHitObject(long time, int sampleType, int comboNumber, int colorCounter, int colorOffset, OsuBeatmap *beatmap)
{
	m_iTime = time;
	m_iSampleType = sampleType;
	m_iComboNumber = comboNumber;
	m_iColorCounter = colorCounter;
	m_iColorOffset = colorOffset;
	m_beatmap = beatmap;

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

	m_iSortHack = sortHackCounter++;
}

void OsuHitObject::draw(Graphics *g)
{
	if (m_hitResults.size() > 0)
	{
		OsuBeatmapStandard *beatmapStd = dynamic_cast<OsuBeatmapStandard*>(m_beatmap);
		OsuBeatmapMania *beatmapMania = dynamic_cast<OsuBeatmapMania*>(m_beatmap);

		OsuSkin *skin = m_beatmap->getSkin();
		for (int i=0; i<m_hitResults.size(); i++)
		{
			const long offset = m_iTime + m_iObjectDuration + m_hitResults[i].delta;

			skin->getHit0()->setAnimationTimeOffset(offset);
			skin->getHit0()->setAnimationFrameClampUp();
			skin->getHit50()->setAnimationTimeOffset(offset);
			skin->getHit50()->setAnimationFrameClampUp();
			skin->getHit100()->setAnimationTimeOffset(offset);
			skin->getHit100()->setAnimationFrameClampUp();
			skin->getHit100k()->setAnimationTimeOffset(offset);
			skin->getHit100k()->setAnimationFrameClampUp();
			skin->getHit300()->setAnimationTimeOffset(offset);
			skin->getHit300()->setAnimationFrameClampUp();
			skin->getHit300g()->setAnimationTimeOffset(offset);
			skin->getHit300g()->setAnimationFrameClampUp();
			skin->getHit300k()->setAnimationTimeOffset(offset);
			skin->getHit300k()->setAnimationFrameClampUp();

			if (beatmapStd != NULL)
				drawHitResult(g, beatmapStd, beatmapStd->osuCoords2Pixels(m_hitResults[i].rawPos), m_hitResults[i].result, clamp<float>(((m_hitResults[i].anim - engine->getTime()) / m_hitResults[i].duration), 0.0f, 1.0f), ((m_hitResults[i].defaultanim - engine->getTime()) / m_hitResults[i].defaultduration));
			else if (beatmapMania != NULL)
				drawHitResult(g, skin, 200.0f, 150.0f, m_hitResults[i].rawPos, m_hitResults[i].result, clamp<float>(((m_hitResults[i].anim - engine->getTime()) / m_hitResults[i].duration), 0.0f, 1.0f), ((m_hitResults[i].defaultanim - engine->getTime()) / m_hitResults[i].defaultduration));
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

	if (m_hitResults.size() > 0 && engine->getTime() > m_hitResults[0].anim)
		m_hitResults.erase(m_hitResults.begin());
}

void OsuHitObject::addHitResult(OsuScore::HIT result, long delta, Vector2 posRaw, float targetDelta, float targetAngle, bool ignoreOnHitErrorBar, bool ignoreCombo)
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

	m_beatmap->addHitResult(result, delta, ignoreOnHitErrorBar, false, ignoreCombo);

	HITRESULTANIM hitresult;
	hitresult.result = result;
	hitresult.rawPos = posRaw;
	hitresult.delta = delta;
	hitresult.defaultduration = osu_hitresult_duration.getFloat();
	hitresult.defaultanim = engine->getTime() + hitresult.defaultduration;
	hitresult.duration = hitresult.defaultduration;
	hitresult.anim = engine->getTime() + hitresult.duration;

	m_hitResults.push_back(hitresult);
}

void OsuHitObject::onReset(long curPos)
{
	m_bMisAim = false;
	m_iAutopilotDelta = 0;

	m_hitResults.clear();
}
