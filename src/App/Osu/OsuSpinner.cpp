//================ Copyright (c) 2015, PG & Jeffrey Han (opsu!), All rights reserved. =================//
//
// Purpose:		spinner. spin logic has been taken from opsu!, I didn't have time to rewrite it yet
//
// $NoKeywords: $spin
//=====================================================================================================//

#include "OsuSpinner.h"

#include "Engine.h"
#include "ResourceManager.h"
#include "SoundEngine.h"
#include "OpenVRInterface.h"
#include "OpenVRController.h"
#include "Mouse.h"
#include "ConVar.h"

#include "Osu.h"
#include "OsuVR.h"
#include "OsuSkin.h"
#include "OsuGameRules.h"
#include "OsuBeatmapStandard.h"

OsuSpinner::OsuSpinner(int x, int y, long time, int sampleType, long endTime, OsuBeatmapStandard *beatmap) : OsuHitObject(time, sampleType, -1, -1, -1, beatmap)
{
	m_vOriginalRawPos = Vector2(x,y);
	m_vRawPos = m_vOriginalRawPos;

	m_beatmap = beatmap;

	m_iObjectDuration = endTime - time;
	m_bClickedOnce = false;
	m_fRotationsNeeded = -1.0f;

	int minVel = 12;
	int maxVel = 48;
	int minTime = 2000;
	int maxTime = 5000;
	m_iMaxStoredDeltaAngles = clamp<int>((int)( (endTime - time - minTime) * (maxVel - minVel) / (maxTime - minTime) + minVel ), minVel, maxVel);
	m_storedDeltaAngles = new float[m_iMaxStoredDeltaAngles];
	for (int i=0; i<m_iMaxStoredDeltaAngles; i++)
	{
		m_storedDeltaAngles[i] = 0.0f;
	}
	m_iDeltaAngleIndex = 0;

	m_fPercent = 0.0f;

	m_fDrawRot = 0.0f;
	m_fRotations = 0.0f;
	m_fDeltaOverflow = 0.0f;
	m_fSumDeltaAngle = 0.0f;
	m_fDeltaAngleOverflow = 0.0f;
	m_fRPM = 0.0f;
	m_fLastMouseAngle = 0.0f;
	m_fLastVRCursorAngle1 = 0.0f;
	m_fLastVRCursorAngle2 = 0.0f;
	m_fRatio = 0.0f;

	// spinners don't need misaims
	m_bMisAim = true;
}

OsuSpinner::~OsuSpinner()
{
	m_beatmap->getSkin()->stopSpinnerSpinSound();

	delete[] m_storedDeltaAngles;
	m_storedDeltaAngles = NULL;
}

void OsuSpinner::draw(Graphics *g)
{
	OsuHitObject::draw(g);
	const long fadeOutTimeMS = (long)(OsuGameRules::getFadeOutTime(m_beatmap) * 1000.0f * OsuGameRules::osu_spinner_fade_out_time_multiplier.getFloat());
	const long deltaEnd = m_iDelta + m_iObjectDuration;
	if ((m_bFinished || !m_bVisible) && (deltaEnd > 0 || (deltaEnd < -fadeOutTimeMS))) return;

	OsuSkin *skin = m_beatmap->getSkin();
	Vector2 center = m_beatmap->osuCoords2Pixels(m_vRawPos);

	const float alphaMultiplier = clamp<float>((deltaEnd < 0 ? 1.0f - ((float)std::abs(deltaEnd) / (float)fadeOutTimeMS) : 1.0f), 0.0f, 1.0f); // only used for fade out anim atm

	const float globalScale = 1.0f; // adjustments
	const float globalBaseSkinSize = 667; // the width of spinner-bottom.png in the default skin
	const float globalBaseSize = m_beatmap->getPlayfieldSize().y/* + m_beatmap->getHitcircleDiameter()/2*/;

	const float clampedRatio = clamp<float>(m_fRatio, 0.0f, 1.0f);
	float finishScaleRatio = clampedRatio;
	finishScaleRatio = -finishScaleRatio * (finishScaleRatio - 2);
	const float finishScale = 0.80f + finishScaleRatio*0.20f; // the spinner grows until reaching 100% during spinning, depending on how many spins are left

	if (skin->getSpinnerBackground() != skin->getMissingTexture() || skin->getVersion() < 2.0f) // old style
	{
		// draw spinner circle
		if (skin->getSpinnerCircle() != skin->getMissingTexture())
		{
			const float spinnerCircleScale = globalBaseSize / (globalBaseSkinSize * (skin->isSpinnerCircle2x() ? 2.0f : 1.0f));

			g->setColor(0xffffffff);
			g->setAlpha(m_fAlphaWithoutHidden * alphaMultiplier);
			g->pushTransform();
			{
				g->rotate(m_fDrawRot);
				g->scale(spinnerCircleScale*globalScale, spinnerCircleScale*globalScale);
				g->translate(center.x, center.y);
				g->drawImage(skin->getSpinnerCircle());
			}
			g->popTransform();
		}

		// draw approach circle
		if (!m_beatmap->getOsu()->getModHD() && m_fPercent > 0.0f)
		{
			const float spinnerApproachCircleImageScale = globalBaseSize / ((globalBaseSkinSize/2) * (skin->isSpinnerApproachCircle2x() ? 2.0f : 1.0f));

			g->setColor(skin->getSpinnerApproachCircleColor());
			g->setAlpha(m_fAlphaWithoutHidden * alphaMultiplier);
			g->pushTransform();
			{
				g->scale(spinnerApproachCircleImageScale*m_fPercent*globalScale, spinnerApproachCircleImageScale*m_fPercent*globalScale);
				g->translate(center.x, center.y);
				g->drawImage(skin->getSpinnerApproachCircle());
			}
			g->popTransform();
		}
	}
	else // new style
	{
		// bottom
		if (skin->getSpinnerBottom() != skin->getMissingTexture())
		{
			const float spinnerBottomImageScale = globalBaseSize / (globalBaseSkinSize * (skin->isSpinnerBottom2x() ? 2.0f : 1.0f));

			g->setColor(0xffffffff);
			g->setAlpha(m_fAlphaWithoutHidden * alphaMultiplier);
			g->pushTransform();
			{
				g->rotate(m_fDrawRot/7.0f);
				g->scale(spinnerBottomImageScale*finishScale*globalScale, spinnerBottomImageScale*finishScale*globalScale);
				g->translate(center.x, center.y);
				g->drawImage(skin->getSpinnerBottom());
			}
			g->popTransform();
		}

		// top
		if (skin->getSpinnerTop() != skin->getMissingTexture())
		{
			const float spinnerTopImageScale = globalBaseSize / (globalBaseSkinSize * (skin->isSpinnerTop2x() ? 2.0f : 1.0f));

			g->setColor(0xffffffff);
			g->setAlpha(m_fAlphaWithoutHidden * alphaMultiplier);
			g->pushTransform();
			{
				g->rotate(m_fDrawRot/2.0f);
				g->scale(spinnerTopImageScale*finishScale*globalScale, spinnerTopImageScale*finishScale*globalScale);
				g->translate(center.x, center.y);
				g->drawImage(skin->getSpinnerTop());
			}
			g->popTransform();
		}

		// middle
		if (skin->getSpinnerMiddle2() != skin->getMissingTexture())
		{
			const float spinnerMiddle2ImageScale = globalBaseSize / (globalBaseSkinSize * (skin->isSpinnerMiddle22x() ? 2.0f : 1.0f));

			g->setColor(0xffffffff);
			g->setAlpha(m_fAlphaWithoutHidden * alphaMultiplier);
			g->pushTransform();
			{
				g->rotate(m_fDrawRot);
				g->scale(spinnerMiddle2ImageScale*finishScale*globalScale, spinnerMiddle2ImageScale*finishScale*globalScale);
				g->translate(center.x, center.y);
				g->drawImage(skin->getSpinnerMiddle2());
			}
			g->popTransform();
		}
		if (skin->getSpinnerMiddle() != skin->getMissingTexture())
		{
			const float spinnerMiddleImageScale = globalBaseSize / (globalBaseSkinSize * (skin->isSpinnerMiddle2x() ? 2.0f : 1.0f));

			g->setColor(COLOR(255, 255, (int)(255*m_fPercent), (int)(255*m_fPercent)));
			g->setAlpha(m_fAlphaWithoutHidden * alphaMultiplier);
			g->pushTransform();
			{
				g->rotate(m_fDrawRot/2.0f); // apparently does not rotate in osu
				g->scale(spinnerMiddleImageScale*finishScale*globalScale, spinnerMiddleImageScale*finishScale*globalScale);
				g->translate(center.x, center.y);
				g->drawImage(skin->getSpinnerMiddle());
			}
			g->popTransform();
		}

		// approach circle
		if (!m_beatmap->getOsu()->getModHD() && m_fPercent > 0.0f)
		{
			const float spinnerApproachCircleImageScale = globalBaseSize / ((globalBaseSkinSize/2) * (skin->isSpinnerApproachCircle2x() ? 2.0f : 1.0f));

			g->setColor(skin->getSpinnerApproachCircleColor());
			g->setAlpha(m_fAlphaWithoutHidden * alphaMultiplier);
			g->pushTransform();
			{
				g->scale(spinnerApproachCircleImageScale*m_fPercent*globalScale, spinnerApproachCircleImageScale*m_fPercent*globalScale);
				g->translate(center.x, center.y);
				g->drawImage(skin->getSpinnerApproachCircle());
			}
			g->popTransform();
		}
	}

	// "CLEAR!"
	if (m_fRatio >= 1.0f)
	{
		const float spinnerClearImageScale = Osu::getImageScale(m_beatmap->getOsu(), skin->getSpinnerClear(), 80);

		g->setColor(0xffffffff);
		g->setAlpha(alphaMultiplier);
		g->pushTransform();
		{
			g->scale(spinnerClearImageScale, spinnerClearImageScale);
			g->translate(center.x, center.y - m_beatmap->getPlayfieldSize().y*0.25f);
			g->drawImage(skin->getSpinnerClear());
		}
		g->popTransform();
	}

	// "SPIN!"
	if (clampedRatio < 0.03f)
	{
		const float spinerSpinImageScale = Osu::getImageScale(m_beatmap->getOsu(), skin->getSpinnerSpin(), 80);

		g->setColor(0xffffffff);
		g->setAlpha(m_fAlphaWithoutHidden * alphaMultiplier);
		g->pushTransform();
		{
			g->scale(spinerSpinImageScale, spinerSpinImageScale);
			g->translate(center.x, center.y + m_beatmap->getPlayfieldSize().y*0.30f);
			g->drawImage(skin->getSpinnerSpin());
		}
		g->popTransform();
	}

	// draw RPM
	if (m_iDelta < 0)
	{
		McFont *rpmFont = engine->getResourceManager()->getFont("FONT_DEFAULT");
		const float stringWidth = rpmFont->getStringWidth("RPM: 477");
		g->setColor(0xffffffff);
		g->setAlpha(m_fAlphaWithoutHidden * m_fAlphaWithoutHidden * m_fAlphaWithoutHidden * alphaMultiplier);
		g->pushTransform();
		{
			g->translate((int)(m_beatmap->getOsu()->getScreenWidth()/2 - stringWidth/2), (int)(m_beatmap->getOsu()->getScreenHeight() - 5 + (5 + rpmFont->getHeight())*(1.0f - m_fAlphaWithoutHidden)));
			g->drawString(rpmFont, UString::format("RPM: %i", (int)(m_fRPM + 0.4f)));
		}
		g->popTransform();
	}
}

void OsuSpinner::drawVR(Graphics *g, Matrix4 &mvp, OsuVR *vr)
{
	///if (m_bVisible)
	{
		float clampedApproachScalePercent = m_fApproachScale - 1.0f; // goes from <m_osu_approach_scale_multiplier_ref> to 0
		clampedApproachScalePercent = clamp<float>(clampedApproachScalePercent / m_osu_approach_scale_multiplier_ref->getFloat(), 0.0f, 1.0f); // goes from 1 to 0

		Matrix4 translation;
		translation.translate(0, 0, -clampedApproachScalePercent*vr->getApproachDistance());
		Matrix4 finalMVP = mvp * translation;

		vr->getShaderTexturedLegacyGeneric()->setUniformMatrix4fv("matrix", finalMVP);
		draw(g);
	}
}

void OsuSpinner::update(long curPos)
{
	OsuHitObject::update(curPos);

	// stop spinner sound and don't update() while paused
	if (m_beatmap->isPaused() || !m_beatmap->isPlaying() || m_beatmap->hasFailed())
	{
		m_beatmap->getSkin()->stopSpinnerSpinSound();

		return;
	}

	// if we have not been clicked yet, check if we are in the timeframe of a miss, also handle auto and relax
	if (!m_bFinished)
	{
		// handle spinner ending
		if (curPos >= m_iTime + m_iObjectDuration)
		{
			onHit();
			return;
		}

		m_fRotationsNeeded = OsuGameRules::getSpinnerRotationsForSpeedMultiplier(m_beatmap, m_iObjectDuration);

		const float fixedRate = /*(1.0f / convar->getConVarByName("fps_max")->getFloat())*/engine->getFrameTime();

		const float DELTA_UPDATE_TIME = (fixedRate * 1000.0f);
		const float AUTO_MULTIPLIER = (1.0f / 20.0f);
		//const float MAX_ANG_DIFF = ((1000.0f/60.0f) * AUTO_MULTIPLIER);

		// scale percent calculation
		long delta = (long)m_iTime - (long)curPos;
		m_fPercent = 1.0f - clamp<float>((float)delta / -(float)(m_iObjectDuration), 0.0f, 1.0f);

		// handle auto, mouse spinning movement
		float angleDiff = 0;
		if (m_beatmap->getOsu()->getModAuto() || m_beatmap->getOsu()->getModAutopilot() || m_beatmap->getOsu()->getModSpunout())
			angleDiff = engine->getFrameTime() * 1000.0f * AUTO_MULTIPLIER * m_beatmap->getOsu()->getSpeedMultiplier();
		else // user spin
		{
			Vector2 mouseDelta = engine->getMouse()->getPos() - m_beatmap->osuCoords2Pixels(m_vRawPos);
			const float currentMouseAngle = (float)std::atan2(mouseDelta.y, mouseDelta.x);
			angleDiff = (currentMouseAngle - m_fLastMouseAngle);

			if (m_beatmap->getOsu()->isInVRMode())
			{
				Vector2 vrCursorDelta1 = m_beatmap->getOsu()->getVR()->getCursorPos1() - m_beatmap->osuCoords2VRPixels(m_vRawPos);
				Vector2 vrCursorDelta2 = m_beatmap->getOsu()->getVR()->getCursorPos2() - m_beatmap->osuCoords2VRPixels(m_vRawPos);

				const float currentVRCursorAngle1 = (float)std::atan2(vrCursorDelta1.x, vrCursorDelta1.y);
				const float currentVRCursorAngle2 = (float)std::atan2(vrCursorDelta2.x, vrCursorDelta2.y);

				angleDiff -= (currentVRCursorAngle1 - m_fLastVRCursorAngle1);
				angleDiff -= (currentVRCursorAngle2 - m_fLastVRCursorAngle2);

				if (std::abs(angleDiff) > 0.001f)
				{
					m_fLastVRCursorAngle1 = currentVRCursorAngle1;
					m_fLastVRCursorAngle2 = currentVRCursorAngle2;
				}
			}

			if (std::abs(angleDiff) > 0.001f)
				m_fLastMouseAngle = currentMouseAngle;
			else
				angleDiff = 0;
		}

		// handle spinning
		// HACKHACK: rewrite this
		if (delta <= 0)
		{
			bool isSpinning = m_beatmap->isClickHeld() || m_beatmap->getOsu()->getModAuto() || m_beatmap->getOsu()->getModRelax() || m_beatmap->getOsu()->getModSpunout();

			m_fDeltaOverflow += engine->getFrameTime() * 1000.0f;

			if (angleDiff < -PI)
				angleDiff += 2*PI;
			else if (angleDiff > PI)
				angleDiff -= 2*PI;

			if (isSpinning)
				m_fDeltaAngleOverflow += angleDiff;

			while (m_fDeltaOverflow >= DELTA_UPDATE_TIME)
			{
				// spin caused by the cursor
				float deltaAngle = 0;
				if (isSpinning)
				{
					deltaAngle = m_fDeltaAngleOverflow * DELTA_UPDATE_TIME / m_fDeltaOverflow;
					m_fDeltaAngleOverflow -= deltaAngle;
					//deltaAngle = clamp<float>(deltaAngle, -MAX_ANG_DIFF, MAX_ANG_DIFF);
				}

				m_fDeltaOverflow -= DELTA_UPDATE_TIME;

				m_fSumDeltaAngle -= m_storedDeltaAngles[m_iDeltaAngleIndex];
				m_fSumDeltaAngle += deltaAngle;
				m_storedDeltaAngles[m_iDeltaAngleIndex++] = deltaAngle;
				m_iDeltaAngleIndex %= m_iMaxStoredDeltaAngles;

				float rotationAngle = m_fSumDeltaAngle / m_iMaxStoredDeltaAngles;
				//rotationAngle = clamp<float>(rotationAngle, -MAX_ANG_DIFF, MAX_ANG_DIFF);
				float rotationPerSec = rotationAngle * (1000.0f / DELTA_UPDATE_TIME) / (2.0f*PI);

				///m_fRPM = std::abs(rotationPerSec*60.0f);

				const float decay = std::pow(0.01f, (float)engine->getFrameTime());
				m_fRPM = m_fRPM * decay + (1.0 - decay) * std::abs(rotationPerSec) * 60;
				m_fRPM = std::min(m_fRPM, 477.0f);

				/*
				m_fRPM += std::abs(rotationPerSec*60.0f);
				m_fRPM /= 2.0;
				m_fRPM = std::min(m_fRPM, 477.0f);
				*/

				if (std::abs(rotationAngle) > 0.0001f)
					rotate(rotationAngle);
				///if (std::abs(rotationAngle) > 0.00001f)
				///	data.changeHealth(DELTA_UPDATE_TIME * m_beatmap->getSelectedDifficulty()->OD);
			}


			m_fRatio = m_fRotations / (m_fRotationsNeeded*360.0f);
			///debugLog("ratio = %f, rotations = %f, rotationsneeded = %f, sumdeltaangle = %f, maxStoredDeltaAngles = %i\n", m_fRatio, m_fRotations/360.0f, m_fRotationsNeeded/360.0f, m_fSumDeltaAngle, m_iMaxStoredDeltaAngles);
		}
	}
}

void OsuSpinner::onClickEvent(std::vector<OsuBeatmap::CLICK> &clicks)
{
	if (m_bFinished) return;

	// needed for nightmare mod
	if (m_bVisible && !m_bClickedOnce)
	{
		m_bClickedOnce = true;
		m_beatmap->consumeClickEvent();
	}
}

void OsuSpinner::onReset(long curPos)
{
	OsuHitObject::onReset(curPos);

	m_beatmap->getSkin()->stopSpinnerSpinSound();

	m_bClickedOnce = false;

	m_fRPM = 0.0f;
	m_fDrawRot = 0.0f;
	m_fRotations = 0.0f;
	m_fDeltaOverflow = 0.0f;
	m_fSumDeltaAngle = 0.0f;
	m_iDeltaAngleIndex = 0;
	m_fDeltaAngleOverflow = 0.0f;
	m_fRatio = 0.0f;

	// spinners don't need misaims
	m_bMisAim = true;

	for (int i=0; i<m_iMaxStoredDeltaAngles; i++)
	{
		m_storedDeltaAngles[i] = 0.0f;
	}

	if (curPos > m_iTime + m_iObjectDuration)
		m_bFinished = true;
	else
		m_bFinished = false;
}

void OsuSpinner::onHit()
{
	///debugLog("ratio = %f\n", m_fRatio);
	//m_bDrawRPM = false;

	// calculate hit result
	OsuScore::HIT result = OsuScore::HIT::HIT_NULL;
	if (m_fRatio >= 1.0f || m_beatmap->getOsu()->getModAuto())
		result = OsuScore::HIT::HIT_300;
	else if (m_fRatio >= 0.9f && !OsuGameRules::osu_mod_ming3012.getBool() && !OsuGameRules::osu_mod_no100s.getBool())
		result = OsuScore::HIT::HIT_100;
	else if (m_fRatio >= 0.75f && !OsuGameRules::osu_mod_no100s.getBool() && !OsuGameRules::osu_mod_no50s.getBool() )
		result = OsuScore::HIT::HIT_50;
	else
		result = OsuScore::HIT::HIT_MISS;

	// sound
	if (result != OsuScore::HIT::HIT_MISS)
	{
		if (m_osu_timingpoints_force->getBool())
			m_beatmap->updateTimingPoints(m_iTime + m_iObjectDuration);

		const Vector2 osuCoords = m_beatmap->pixels2OsuCoords(m_beatmap->osuCoords2Pixels(m_vRawPos));

		m_beatmap->getSkin()->playHitCircleSound(m_iSampleType, OsuGameRules::osuCoords2Pan(osuCoords.x));
	}

	// add it, and we are finished
	addHitResult(result, 0, m_vRawPos, -1.0f);
	m_bFinished = true;

	m_beatmap->getSkin()->stopSpinnerSpinSound();
}

void OsuSpinner::rotate(float rad)
{
	m_fDrawRot += rad2deg(rad);

	rad = std::abs(rad);
	const float newRotations = m_fRotations + rad2deg(rad);

	// added one whole rotation...
	if (std::floor(newRotations/360.0f) > m_fRotations/360.0f)
	{
		// TODO seems to give 1100 points per spin but also an extra 100 for some spinners
		if ((int)(newRotations/360.0f) > (int)(m_fRotationsNeeded)+1)
		{
			// extra rotations and bonus sound
			m_beatmap->addScorePoints(1000, true);
			m_beatmap->getSkin()->playSpinnerBonusSound();
		}
		m_beatmap->addScorePoints(100, true);
	}

	// spinner sound
	{
		m_beatmap->getSkin()->playSpinnerSpinSound();

		const float frequency = 20000.0f + (int)(clamp<float>(m_fRatio, 0.0f, 2.5f)*40000.0f);
		m_beatmap->getSkin()->getSpinnerSpinSound()->setFrequency(frequency);
	}

	m_fRotations = newRotations;
}

Vector2 OsuSpinner::getAutoCursorPos(long curPos)
{
	// calculate point
	long delta = 0;
	if (curPos <= m_iTime)
		delta = 0;
	else if (curPos >= m_iTime + m_iObjectDuration)
		delta = m_iObjectDuration;
	else
		delta = curPos - m_iTime;

	Vector2 actualPos = m_beatmap->osuCoords2Pixels(m_vRawPos);
	const float AUTO_MULTIPLIER = (1.0f / 20.0f);
	float multiplier = (m_beatmap->getOsu()->getModAuto() || m_beatmap->getOsu()->getModAutopilot()) ? AUTO_MULTIPLIER : 1.0f;
	float angle = (delta * multiplier) - PI/2.0f;
	float r = m_beatmap->getPlayfieldSize().y / 10.0f;
	return Vector2((float) (actualPos.x + r * std::cos(angle)), (float) (actualPos.y + r * std::sin(angle)));
}
