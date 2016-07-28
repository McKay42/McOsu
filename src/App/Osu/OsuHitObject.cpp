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
#include "OsuGameRules.h"
#include "OsuHUD.h"

ConVar osu_mod_target_300_percent("osu_mod_target_300_percent", 0.5f);
ConVar osu_mod_target_100_percent("osu_mod_target_100_percent", 0.7f);
ConVar osu_mod_target_50_percent("osu_mod_target_50_percent", 0.95f);

void OsuHitObject::drawHitResult(Graphics *g, OsuBeatmap *beatmap, Vector2 rawPos, OsuBeatmap::HIT result, float animPercent)
{
	drawHitResult(g, beatmap->getSkin(), beatmap->getHitcircleDiameter(), beatmap->getRawHitcircleDiameter(), rawPos, result, animPercent);
}

void OsuHitObject::drawHitResult(Graphics *g, OsuSkin *skin, float hitcircleDiameter, float rawHitcircleDiameter, Vector2 rawPos, OsuBeatmap::HIT result, float animPercent)
{
	const float osuCoordScaleMultiplier = hitcircleDiameter / rawHitcircleDiameter;

	g->setColor(0xffffffff);
	g->setAlpha(animPercent);
	g->pushTransform();
		float hitImageScale = 1.0f;
		switch (result)
		{
		case OsuBeatmap::HIT_MISS:
			hitImageScale = (rawHitcircleDiameter / (128.0f * (skin->isHit02x() ? 2.0f : 1.0f))) * osuCoordScaleMultiplier;
			break;
		case OsuBeatmap::HIT_50:
			hitImageScale = (rawHitcircleDiameter / (128.0f * (skin->isHit502x() ? 2.0f : 1.0f))) * osuCoordScaleMultiplier;
			break;
		case OsuBeatmap::HIT_100:
			hitImageScale = (rawHitcircleDiameter / (128.0f * (skin->isHit1002x() ? 2.0f : 1.0f))) * osuCoordScaleMultiplier;
			break;
		}
		g->scale(hitImageScale, hitImageScale);
		g->translate(rawPos.x, rawPos.y);
		switch (result)
		{
		case OsuBeatmap::HIT_MISS:
			g->translate(0, (1.0f-animPercent)*(1.0f-animPercent)*skin->getHit0()->getHeight()*1.25f);
			g->drawImage(skin->getHit0());
			break;
		case OsuBeatmap::HIT_50:
			g->drawImage(skin->getHit50());
			break;
		case OsuBeatmap::HIT_100:
			g->drawImage(skin->getHit100());
			break;
		}
	g->popTransform();
}



OsuHitObject::OsuHitObject(long time, int sampleType, int comboNumber, int colorCounter, OsuBeatmap *beatmap)
{
	m_iTime = time;
	m_iSampleType = sampleType;
	m_iComboNumber = comboNumber;
	m_iColorCounter = colorCounter;
	m_beatmap = beatmap;

	m_fAlpha = 0.0f;
	m_fApproachScale = 0.0f;
	m_fFadeInScale = 0.0f;
	m_iObjectTime = 0;
	m_iObjectDuration = 0;
	m_iDelta = 0;
	m_iFadeInTime = 0;
	m_iObjectTime = 0;
	m_iObjectDuration = 0;

	m_iHiddenDecayTime = 0;
	m_iHiddenTimeDiff = 0;

	m_bVisible = false;
	m_bFinished = false;
	m_bMisAim = false;
	m_iAutopilotDelta = 0;
	m_bOverrideHDApproachCircle = false;

	m_iStack = 0;
}

void OsuHitObject::draw(Graphics *g)
{
	for (int i=0; i<m_hitResults.size(); i++)
	{
		drawHitResult(g, m_beatmap, m_beatmap->osuCoords2Pixels(m_hitResults[i].rawPos), m_hitResults[i].result, clamp<float>(((m_hitResults[i].anim-engine->getTime()) / 1.5f), 0.0f, 1.0f));
	}
}

void OsuHitObject::update(long curPos)
{
	long approachTime = OsuGameRules::getApproachTime(m_beatmap);
	m_iHiddenDecayTime = (long) ((float)approachTime / 3.6f);
	m_iHiddenTimeDiff = (long) ((float)approachTime / 3.3f);
	m_iFadeInTime = std::min(400, (int) ((float)approachTime / 1.75f));

	m_iObjectTime = approachTime + m_iFadeInTime + (m_beatmap->getOsu()->getModHD() ? m_iHiddenTimeDiff : 0);
	m_iDelta = m_iTime - curPos;

	if (curPos >= m_iTime - m_iObjectTime && curPos < m_iTime+m_iObjectDuration ) // 1 ms fudge by using >=, shouldn't really be a problem
	{
		// this calculates the default alpha and approach circle scale for playing without mods
		float scale = (float)m_iDelta / (float)approachTime;
		m_fApproachScale = 1 + scale * 3;

		m_fFadeInScale = ((float)m_iDelta - (float)approachTime + (float)m_iFadeInTime) / (float)m_iFadeInTime;
		m_fAlpha = clamp<float>(1.0f - m_fFadeInScale, 0.0f, 1.0f);

		m_bVisible = true;
	}
	else
		m_bVisible = false;

	if (m_hitResults.size() > 0 && engine->getTime() > m_hitResults[0].anim)
		m_hitResults.erase(m_hitResults.begin());
}

void OsuHitObject::addHitResult(OsuBeatmap::HIT result, long delta, Vector2 posRaw, float targetDelta, float targetAngle, bool ignoreOnHitErrorBar, bool ignoreCombo)
{
	if (m_beatmap->getOsu()->getModTarget() && result != OsuBeatmap::HIT_MISS && targetDelta >= 0.0f)
	{
		if (targetDelta < osu_mod_target_300_percent.getFloat() && (result == OsuBeatmap::HIT_300 || result == OsuBeatmap::HIT_100))
			result = OsuBeatmap::HIT_300;
		else if (targetDelta < osu_mod_target_100_percent.getFloat())
			result = OsuBeatmap::HIT_100;
		else if (targetDelta < osu_mod_target_50_percent.getFloat())
			result = OsuBeatmap::HIT_50;
		else
			result = OsuBeatmap::HIT_MISS;

		m_beatmap->getOsu()->getHUD()->addTarget(targetDelta, targetAngle);
	}

	m_beatmap->addHitResult(result, delta, ignoreOnHitErrorBar, false, ignoreCombo);

	HITRESULTANIM hitresult;
	hitresult.result = result;
	hitresult.rawPos = posRaw;
	hitresult.anim = engine->getTime() + 1.5f;
	m_hitResults.push_back(hitresult);
}

void OsuHitObject::onReset(long curPos)
{
	m_bMisAim = false;
	m_iAutopilotDelta = 0;
}
