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

	m_fHitResultAnim = 0.0f;
	m_hitResult = OsuBeatmap::HIT_NULL;

	m_bVisible = false;
	m_bFinished = false;
	m_bMisAim = false;
	m_iAutopilotDelta = 0;
	m_bOverrideHDApproachCircle = false;

	m_iStack = 0;
}

void OsuHitObject::draw(Graphics *g)
{
	if (m_fHitResultAnim > 0.0f && m_fHitResultAnim < 1.0f)
		drawHitResult(g, m_beatmap, m_beatmap->osuCoords2Pixels(m_vHitResultPosRaw), m_hitResult, m_fHitResultAnim);
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
}

void OsuHitObject::addHitResult(OsuBeatmap::HIT result, long delta, Vector2 posRaw, bool ignoreOnHitErrorBar, bool ignoreCombo)
{
	m_hitResult = result;
	m_beatmap->addHitResult(result, delta, ignoreOnHitErrorBar, false, ignoreCombo);
	m_vHitResultPosRaw = posRaw;

	m_fHitResultAnim = 1.0f; // 1 frame delay, i don't even care
	anim->moveLinear(&m_fHitResultAnim, 0.0f, 1.5f, true);
}

void OsuHitObject::onReset(long curPos)
{
	m_bMisAim = false;
	m_iAutopilotDelta = 0;
}
