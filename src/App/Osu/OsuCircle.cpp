//================ Copyright (c) 2015, PG, All rights reserved. =================//
//
// Purpose:		circle
//
// $NoKeywords: $circle
//===============================================================================//

#include "OsuCircle.h"

#include "Engine.h"
#include "ResourceManager.h"
#include "AnimationHandler.h"
#include "SoundEngine.h"
#include "OpenVRInterface.h"
#include "OpenVRController.h"
#include "ConVar.h"

#include "Osu.h"
#include "OsuVR.h"
#include "OsuSkin.h"
#include "OsuSkinImage.h"
#include "OsuGameRules.h"
#include "OsuBeatmapStandard.h"

ConVar osu_circle_color_saturation("osu_circle_color_saturation", 1.0f);
ConVar osu_circle_rainbow("osu_circle_rainbow", false);
ConVar osu_circle_number_rainbow("osu_circle_number_rainbow", false);
ConVar osu_circle_shake_duration("osu_circle_shake_duration", 0.120f);
ConVar osu_circle_shake_strength("osu_circle_shake_strength", 8.0f);
ConVar osu_approach_circle_alpha_multiplier("osu_approach_circle_alpha_multiplier", 0.9f);

ConVar osu_draw_numbers("osu_draw_numbers", true);
ConVar osu_draw_approach_circles("osu_draw_approach_circles", true);

ConVar osu_slider_draw_endcircle("osu_slider_draw_endcircle", true);

int OsuCircle::rainbowNumber = 0;
int OsuCircle::rainbowColorCounter = 0;

void OsuCircle::drawCircle(Graphics *g, OsuBeatmapStandard *beatmap, Vector2 rawPos, int number, int colorCounter, float approachScale, float alpha, float numberAlpha, bool drawNumber, bool overrideHDApproachCircle)
{
	drawCircle(g, beatmap->getSkin(), beatmap->osuCoords2Pixels(rawPos), beatmap->getHitcircleDiameter(), beatmap->getNumberScale(), beatmap->getHitcircleOverlapScale(), number, colorCounter, approachScale, alpha, numberAlpha, drawNumber, overrideHDApproachCircle);
}

void OsuCircle::drawApproachCircle(Graphics *g, OsuBeatmapStandard *beatmap, Vector2 rawPos, int number, int colorCounter, float approachScale, float alpha, bool overrideHDApproachCircle)
{
	rainbowNumber = number;
	rainbowColorCounter = colorCounter;

	Color comboColor = beatmap->getSkin()->getComboColorForCounter(colorCounter);
	comboColor = COLOR(255, (int)(COLOR_GET_Ri(comboColor)*osu_circle_color_saturation.getFloat()), (int)(COLOR_GET_Gi(comboColor)*osu_circle_color_saturation.getFloat()), (int)(COLOR_GET_Bi(comboColor)*osu_circle_color_saturation.getFloat()));

	drawApproachCircle(g, beatmap->getSkin(), beatmap->osuCoords2Pixels(rawPos), comboColor, beatmap->getHitcircleDiameter(), approachScale, alpha, beatmap->getOsu()->getModHD(), overrideHDApproachCircle);
}

void OsuCircle::drawCircle(Graphics *g, OsuSkin *skin, Vector2 pos, float hitcircleDiameter, float numberScale, float overlapScale, int number, int colorCounter, float approachScale, float alpha, float numberAlpha, bool drawNumber, bool overrideHDApproachCircle)
{
	if (alpha <= 0.0f)
		return;

	rainbowNumber = number;
	rainbowColorCounter = colorCounter;

	Color comboColor = skin->getComboColorForCounter(colorCounter);
	comboColor = COLOR(255, (int)(COLOR_GET_Ri(comboColor)*osu_circle_color_saturation.getFloat()), (int)(COLOR_GET_Gi(comboColor)*osu_circle_color_saturation.getFloat()), (int)(COLOR_GET_Bi(comboColor)*osu_circle_color_saturation.getFloat()));

	// approach circle
	///drawApproachCircle(g, skin, pos, comboColor, hitcircleDiameter, approachScale, alpha, modHD, overrideHDApproachCircle); // they are now drawn separately in draw2()

	// circle
	const float circleImageScale = hitcircleDiameter / (128.0f * (skin->isHitCircle2x() ? 2.0f : 1.0f));
	comboColor = skin->getComboColorForCounter(colorCounter);
	drawHitCircle(g, skin->getHitCircle(), pos, comboColor, circleImageScale, alpha);

	// overlay
	const float circleOverlayImageScale = hitcircleDiameter / skin->getHitCircleOverlay2()->getSizeBaseRaw().x;
	if (!skin->getHitCircleOverlayAboveNumber())
		drawHitCircleOverlay(g, skin->getHitCircleOverlay2(), pos, circleOverlayImageScale, alpha);

	// number
	if (drawNumber)
		drawHitCircleNumber(g, skin, numberScale, overlapScale, pos, number, numberAlpha);

	// overlay
	if (skin->getHitCircleOverlayAboveNumber())
		drawHitCircleOverlay(g, skin->getHitCircleOverlay2(), pos, circleOverlayImageScale, alpha);
}

void OsuCircle::drawCircle(Graphics *g, OsuSkin *skin, Vector2 pos, float hitcircleDiameter, Color color, float alpha)
{
	// this function is only used by the target practice heatmap

	// circle
	const float circleImageScale = hitcircleDiameter / (128.0f * (skin->isHitCircle2x() ? 2.0f : 1.0f));
	drawHitCircle(g, skin->getHitCircle(), pos, color, circleImageScale, alpha);

	// overlay
	const float circleOverlayImageScale = hitcircleDiameter / skin->getHitCircleOverlay2()->getSizeBaseRaw().x;
	drawHitCircleOverlay(g, skin->getHitCircleOverlay2(), pos, circleOverlayImageScale, alpha);
}

void OsuCircle::drawSliderStartCircle(Graphics *g, OsuBeatmapStandard *beatmap, Vector2 rawPos, int number, int colorCounter, float approachScale, float alpha, float numberAlpha, bool drawNumber, bool overrideHDApproachCircle)
{
	drawSliderStartCircle(g, beatmap->getSkin(), beatmap->osuCoords2Pixels(rawPos), beatmap->getHitcircleDiameter(), beatmap->getNumberScale(), beatmap->getHitcircleOverlapScale(), number, colorCounter, approachScale, alpha, numberAlpha, drawNumber, overrideHDApproachCircle);
}

void OsuCircle::drawSliderStartCircle(Graphics *g, OsuSkin *skin, Vector2 pos, float hitcircleDiameter, float numberScale, float hitcircleOverlapScale, int number, int colorCounter, float approachScale, float alpha, float numberAlpha, bool drawNumber, bool overrideHDApproachCircle)
{
	if (alpha <= 0.0f)
		return;

	// if no sliderstartcircle image is preset, fallback to default circle
	if (skin->getSliderStartCircle() == skin->getMissingTexture())
	{
		drawCircle(g, skin, pos, hitcircleDiameter, numberScale, hitcircleOverlapScale, number, colorCounter, approachScale, alpha, numberAlpha, drawNumber, overrideHDApproachCircle); // normal
		return;
	}

	rainbowNumber = number;
	rainbowColorCounter = colorCounter;

	const Color comboColor = skin->getComboColorForCounter(colorCounter);

	// approach circle
	///drawApproachCircle(g, skin, pos, comboColor, beatmap->getHitcircleDiameter(), approachScale, alpha, beatmap->getOsu()->getModHD(), overrideHDApproachCircle); // they are now drawn separately in draw2()

	// circle
	const float circleImageScale = hitcircleDiameter / (128.0f * (skin->isSliderStartCircle2x() ? 2.0f : 1.0f));
	drawHitCircle(g, skin->getSliderStartCircle(), pos, comboColor, circleImageScale, alpha);

	// overlay
	const float circleOverlayImageScale = hitcircleDiameter / skin->getSliderStartCircleOverlay2()->getSizeBaseRaw().x;
	if (skin->getSliderStartCircleOverlay() != skin->getMissingTexture())
	{
		if (!skin->getHitCircleOverlayAboveNumber())
			drawHitCircleOverlay(g, skin->getSliderStartCircleOverlay2(), pos, circleOverlayImageScale, alpha);
	}

	// number
	if (drawNumber)
		drawHitCircleNumber(g, skin, numberScale, hitcircleOverlapScale, pos, number, numberAlpha);

	// overlay
	if (skin->getSliderStartCircleOverlay() != skin->getMissingTexture())
	{
		if (skin->getHitCircleOverlayAboveNumber())
			drawHitCircleOverlay(g, skin->getSliderStartCircleOverlay2(), pos, circleOverlayImageScale, alpha);
	}
}

void OsuCircle::drawSliderEndCircle(Graphics *g, OsuBeatmapStandard *beatmap, Vector2 rawPos, int number, int colorCounter, float approachScale, float alpha, float numberAlpha, bool drawNumber, bool overrideHDApproachCircle)
{
	drawSliderEndCircle(g, beatmap->getSkin(), beatmap->osuCoords2Pixels(rawPos), beatmap->getHitcircleDiameter(), beatmap->getNumberScale(), beatmap->getHitcircleOverlapScale(), number, colorCounter, approachScale, alpha, numberAlpha, drawNumber, overrideHDApproachCircle);
}

void OsuCircle::drawSliderEndCircle(Graphics *g, OsuSkin *skin, Vector2 pos, float hitcircleDiameter, float numberScale, float overlapScale, int number, int colorCounter, float approachScale, float alpha, float numberAlpha, bool drawNumber, bool overrideHDApproachCircle)
{
	if (alpha <= 0.0f || !osu_slider_draw_endcircle.getBool())
		return;

	// if no sliderendcircle image is preset, fallback to default circle
	if (skin->getSliderEndCircle() == skin->getMissingTexture())
	{
		drawCircle(g, skin, pos, hitcircleDiameter, numberScale, overlapScale, number, colorCounter, approachScale, alpha, numberAlpha, drawNumber, overrideHDApproachCircle);
		return;
	}

	rainbowNumber = number;
	rainbowColorCounter = colorCounter;

	const Color comboColor = skin->getComboColorForCounter(colorCounter);

	// circle
	const float circleImageScale = hitcircleDiameter / (128.0f * (skin->isSliderEndCircle2x() ? 2.0f : 1.0f));
	drawHitCircle(g, skin->getSliderEndCircle(), pos, comboColor, circleImageScale, alpha);

	// overlay
	if (skin->getSliderEndCircleOverlay() != skin->getMissingTexture())
	{
		const float circleOverlayImageScale = hitcircleDiameter / skin->getSliderEndCircleOverlay2()->getSizeBaseRaw().x;
		drawHitCircleOverlay(g, skin->getSliderEndCircleOverlay2(), pos, circleOverlayImageScale, alpha);
	}
}

void OsuCircle::drawApproachCircle(Graphics *g, OsuSkin *skin, Vector2 pos, Color comboColor, float hitcircleDiameter, float approachScale, float alpha, bool modHD, bool overrideHDApproachCircle)
{
	if ((!modHD || overrideHDApproachCircle) && osu_draw_approach_circles.getBool() && !OsuGameRules::osu_mod_mafham.getBool())
	{
		g->setColor(comboColor);

		if (osu_circle_rainbow.getBool())
		{
			float frequency = 0.3f;
			float time = engine->getTime()*20;

			char red1	= std::sin(frequency*time + 0 + rainbowNumber*rainbowColorCounter) * 127 + 128;
			char green1	= std::sin(frequency*time + 2 + rainbowNumber*rainbowColorCounter) * 127 + 128;
			char blue1	= std::sin(frequency*time + 4 + rainbowNumber*rainbowColorCounter) * 127 + 128;

			g->setColor(COLOR(255, red1, green1, blue1));
		}

		g->setAlpha(alpha*osu_approach_circle_alpha_multiplier.getFloat());
		if (approachScale > 1.0f)
		{
			float approachCircleImageScale = hitcircleDiameter / (128.0f * (skin->isApproachCircle2x() ? 2.0f : 1.0f));

			g->pushTransform();
				g->scale(approachCircleImageScale*approachScale, approachCircleImageScale*approachScale);
				g->translate(pos.x, pos.y);
				g->drawImage(skin->getApproachCircle());
			g->popTransform();
		}
	}
}

void OsuCircle::drawHitCircleOverlay(Graphics *g, OsuSkinImage *hitCircleOverlayImage, Vector2 pos, float circleOverlayImageScale, float alpha)
{
	g->setColor(0xffffffff);
	g->setAlpha(alpha);
	hitCircleOverlayImage->drawRaw(g, pos, circleOverlayImageScale);
	/*
	g->pushTransform();
		g->scale(circleOverlayImageScale, circleOverlayImageScale);
		g->translate(pos.x, pos.y);
		g->drawImage(hitCircleOverlayImage);
	g->popTransform();
	*/
}

void OsuCircle::drawHitCircle(Graphics *g, Image *hitCircleImage, Vector2 pos, Color comboColor, float circleImageScale, float alpha)
{
	g->setColor(comboColor);

	if (osu_circle_rainbow.getBool())
	{
		float frequency = 0.3f;
		float time = engine->getTime()*20;

		char red1	= std::sin(frequency*time + 0 + rainbowNumber*rainbowNumber*rainbowColorCounter) * 127 + 128;
		char green1	= std::sin(frequency*time + 2 + rainbowNumber*rainbowNumber*rainbowColorCounter) * 127 + 128;
		char blue1	= std::sin(frequency*time + 4 + rainbowNumber*rainbowNumber*rainbowColorCounter) * 127 + 128;

		g->setColor(COLOR(255, red1, green1, blue1));
	}

	g->setAlpha(alpha);
	g->pushTransform();
		g->scale(circleImageScale, circleImageScale);
		g->translate(pos.x, pos.y);
		g->drawImage(hitCircleImage);
	g->popTransform();
}

void OsuCircle::drawHitCircleNumber(Graphics *g, OsuBeatmapStandard *beatmap, Vector2 pos, int number, float numberAlpha)
{
	drawHitCircleNumber(g, beatmap->getSkin(), beatmap->getNumberScale(), beatmap->getHitcircleOverlapScale(), pos, number, numberAlpha);
}

void OsuCircle::drawHitCircleNumber(Graphics *g, OsuSkin *skin, float numberScale, float overlapScale, Vector2 pos, int number, float numberAlpha)
{
	if (!osu_draw_numbers.getBool())
		return;

	class DigitWidth
	{
	public:
		static float getWidth(OsuSkin *skin, int digit)
		{
			switch (digit)
			{
			case 0:
				return skin->getDefault0()->getWidth();
			case 1:
				return skin->getDefault1()->getWidth();
			case 2:
				return skin->getDefault2()->getWidth();
			case 3:
				return skin->getDefault3()->getWidth();
			case 4:
				return skin->getDefault4()->getWidth();
			case 5:
				return skin->getDefault5()->getWidth();
			case 6:
				return skin->getDefault6()->getWidth();
			case 7:
				return skin->getDefault7()->getWidth();
			case 8:
				return skin->getDefault8()->getWidth();
			case 9:
				return skin->getDefault9()->getWidth();
			}

			return skin->getDefault0()->getWidth();
		}
	};

	// generate digits
	std::vector<int> digits;
	while (number >= 10)
	{
		digits.push_back(number % 10);
		number = number / 10;
	}
	digits.push_back(number);

	int digitOffsetMultiplier = digits.size()-1;

	// set color
	g->setColor(0xffffffff);
	if (osu_circle_number_rainbow.getBool())
	{
		float frequency = 0.3f;
		float time = engine->getTime()*20;

		char red1	= std::sin(frequency*time + 0 + rainbowNumber*rainbowNumber*rainbowNumber*rainbowColorCounter) * 127 + 128;
		char green1	= std::sin(frequency*time + 2 + rainbowNumber*rainbowNumber*rainbowNumber*rainbowColorCounter) * 127 + 128;
		char blue1	= std::sin(frequency*time + 4 + rainbowNumber*rainbowNumber*rainbowNumber*rainbowColorCounter) * 127 + 128;

		g->setColor(COLOR(255, red1, green1, blue1));
	}
	g->setAlpha(numberAlpha);

	// draw digits, start at correct offset
	g->pushTransform();
	g->scale(numberScale, numberScale);
	g->translate(pos.x, pos.y);
	g->translate(-DigitWidth::getWidth(skin, (digits.size() > 0 ? digits[digits.size()-1] : 0))*digitOffsetMultiplier*numberScale*0.5f + skin->getHitCircleOverlap()*digitOffsetMultiplier*overlapScale*0.5f, 0);

	for (int i=digits.size()-1; i>=0; i--)
	{
		switch (digits[i])
		{
		case 0:
			g->drawImage(skin->getDefault0());
			break;
		case 1:
			g->drawImage(skin->getDefault1());
			break;
		case 2:
			g->drawImage(skin->getDefault2());
			break;
		case 3:
			g->drawImage(skin->getDefault3());
			break;
		case 4:
			g->drawImage(skin->getDefault4());
			break;
		case 5:
			g->drawImage(skin->getDefault5());
			break;
		case 6:
			g->drawImage(skin->getDefault6());
			break;
		case 7:
			g->drawImage(skin->getDefault7());
			break;
		case 8:
			g->drawImage(skin->getDefault8());
			break;
		case 9:
			g->drawImage(skin->getDefault9());
			break;
		}

		g->translate(DigitWidth::getWidth(skin, digits[i])*numberScale - skin->getHitCircleOverlap()*overlapScale, 0);
	}
	g->popTransform();
}



OsuCircle::OsuCircle(int x, int y, long time, int sampleType, int comboNumber, int colorCounter, OsuBeatmapStandard *beatmap) : OsuHitObject(time, sampleType, comboNumber, colorCounter, beatmap)
{
	m_vOriginalRawPos = Vector2(x,y);
	m_vRawPos = m_vOriginalRawPos;

	m_beatmap = beatmap;

	m_bWaiting = false;
	m_fHitAnimation = 0.0f;
	m_fShakeAnimation = 0.0f;

	m_bOnHitVRLeftControllerHapticFeedback = false;
}

OsuCircle::~OsuCircle()
{
	onReset(0);
}

void OsuCircle::draw(Graphics *g)
{
	OsuHitObject::draw(g);

	// draw hit animation
	if (m_fHitAnimation > 0.0f && m_fHitAnimation != 1.0f && !m_beatmap->getOsu()->getModHD())
	{
		float alpha = 1.0f - m_fHitAnimation;

		float scale = m_fHitAnimation;
		scale = -scale*(scale-2.0f); // quad out scale

		const bool drawNumber = m_beatmap->getSkin()->getVersion() > 1.0f ? false : true;

		g->pushTransform();
			g->scale((1.0f+scale*OsuGameRules::osu_circle_fade_out_scale.getFloat()), (1.0f+scale*OsuGameRules::osu_circle_fade_out_scale.getFloat()));
			{
				m_beatmap->getSkin()->getHitCircleOverlay2()->setAnimationTimeOffset(!m_beatmap->isInMafhamRenderChunk() ? m_iTime - m_iApproachTime : m_beatmap->getCurMusicPosWithOffsets());
				drawCircle(g, m_beatmap, m_vRawPos, m_iComboNumber, m_iColorCounter, 1.0f, alpha, alpha, drawNumber);
			}
		g->popTransform();
	}

	if (m_bFinished || (!m_bVisible && !m_bWaiting)) // special case needed for when we are past this objects time, but still within not-miss range, because we still need to draw the object
		return;

	// draw circle
	const bool hd = m_beatmap->getOsu()->getModHD();
	Vector2 shakeCorrectedPos = m_vRawPos;
	if (engine->getTime() < m_fShakeAnimation && !m_beatmap->isInMafhamRenderChunk()) // handle note blocking shaking
	{
		float smooth = 1.0f - ((m_fShakeAnimation - engine->getTime()) / osu_circle_shake_duration.getFloat()); // goes from 0 to 1
		if (smooth < 0.5f)
			smooth = smooth / 0.5f;
		else
			smooth = (1.0f - smooth) / 0.5f;
		// (now smooth goes from 0 to 1 to 0 linearly)
		smooth = -smooth*(smooth-2); // quad out
		smooth = -smooth*(smooth-2); // quad out twice
		shakeCorrectedPos.x += std::sin(engine->getTime()*120) * smooth * osu_circle_shake_strength.getFloat();
	}
	m_beatmap->getSkin()->getHitCircleOverlay2()->setAnimationTimeOffset(!m_beatmap->isInMafhamRenderChunk() ? m_iTime - m_iApproachTime : m_beatmap->getCurMusicPosWithOffsets());
	drawCircle(g, m_beatmap, shakeCorrectedPos, m_iComboNumber, m_iColorCounter, m_bWaiting && !hd ? 1.0f : m_fApproachScale, m_bWaiting && !hd ? 1.0f : m_fAlpha, m_bWaiting && !hd ? 1.0f : m_fAlpha, true, m_bOverrideHDApproachCircle);
}

void OsuCircle::draw2(Graphics *g)
{
	if (m_bFinished || (!m_bVisible && !m_bWaiting)) // special case needed for when we are past this objects time, but still within not-miss range, because we still need to draw the object
		return;

	// draw approach circle
	const bool hd = m_beatmap->getOsu()->getModHD();
	drawApproachCircle(g, m_beatmap, m_vRawPos, m_iComboNumber, m_iColorCounter, m_bWaiting && !hd ? 1.0f : m_fApproachScale, m_bWaiting && !hd ? 1.0f : m_fAlphaForApproachCircle, m_bOverrideHDApproachCircle);
}

void OsuCircle::drawVR(Graphics *g, Matrix4 &mvp, OsuVR *vr)
{
	// TODO: performance! if nothing of the circle is visible, then we don't have to calculate anything
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

void OsuCircle::update(long curPos)
{
	OsuHitObject::update(curPos);

	// if we have not been clicked yet, check if we are in the timeframe of a miss, also handle auto and relax
	if (!m_bFinished)
	{
		if (m_beatmap->getOsu()->getModAuto())
		{
			if (curPos >= m_iTime)
				onHit(OsuScore::HIT::HIT_300, 0);
		}
		else
		{
			const long delta = curPos - m_iTime;

			if (m_beatmap->getOsu()->getModRelax() || m_beatmap->getOsu()->isInVRMode())
			{
				if (curPos >= m_iTime)
				{
					const Vector2 pos = m_beatmap->osuCoords2Pixels(m_vRawPos);
					const float cursorDelta = (m_beatmap->getCursorPos() - pos).length();

					float vrCursor1Delta = 0.0f;
					float vrCursor2Delta = 0.0f;
					bool vrCursor1Inside = false;
					bool vrCursor2Inside = false;
					if (m_beatmap->getOsu()->isInVRMode())
					{
						vrCursor1Delta = (m_beatmap->getOsu()->getVR()->getCursorPos1() - m_beatmap->osuCoords2VRPixels(m_vRawPos)).length();
						vrCursor2Delta = (m_beatmap->getOsu()->getVR()->getCursorPos2() - m_beatmap->osuCoords2VRPixels(m_vRawPos)).length();
						vrCursor1Inside = vrCursor1Delta < ((m_beatmap->getRawHitcircleDiameter()/2.0f) * m_beatmap->getOsu()->getVR()->getCircleHitboxScale());
						vrCursor2Inside = vrCursor2Delta < ((m_beatmap->getRawHitcircleDiameter()/2.0f) * m_beatmap->getOsu()->getVR()->getCircleHitboxScale());
					}

					if ((cursorDelta < m_beatmap->getHitcircleDiameter()/2.0f && m_beatmap->getOsu()->getModRelax()) || (vrCursor1Inside || vrCursor2Inside))
					{
						OsuScore::HIT result = OsuGameRules::getHitResult(delta, m_beatmap);

						if (result != OsuScore::HIT::HIT_NULL)
						{
							const float targetDelta = cursorDelta / (m_beatmap->getHitcircleDiameter()/2.0f);
							const float targetAngle = rad2deg(atan2(m_beatmap->getCursorPos().y - pos.y, m_beatmap->getCursorPos().x - pos.x));

							if (m_beatmap->getOsu()->isInVRMode())
							{
								// distance to circle
								if (vrCursor1Delta < vrCursor2Delta)
									m_bOnHitVRLeftControllerHapticFeedback = true;
								else
									m_bOnHitVRLeftControllerHapticFeedback = false;

								// distance to playfield, if both cursors were valid (overrides distance to circle for haptic feedback)
								if (vrCursor1Inside && vrCursor2Inside)
								{
									if (m_beatmap->getOsu()->getVR()->getCursorDist1() < m_beatmap->getOsu()->getVR()->getCursorDist2())
										m_bOnHitVRLeftControllerHapticFeedback = true;
									else
										m_bOnHitVRLeftControllerHapticFeedback = false;
								}
							}

							onHit(result, delta, targetDelta, targetAngle);
						}
					}
				}
			}

			if (delta >= 0)
			{
				m_bWaiting = true;

				// if this is a miss after waiting
				if (delta > (long)OsuGameRules::getHitWindow50(m_beatmap))
					onHit(OsuScore::HIT::HIT_MISS, delta);
			}
			else
				m_bWaiting = false;
		}
	}
}

void OsuCircle::updateStackPosition(float stackOffset)
{
	m_vRawPos = m_vOriginalRawPos - Vector2(m_iStack * stackOffset, m_iStack * stackOffset * (m_beatmap->getOsu()->getModHR() ? -1.0f : 1.0f));
}

void OsuCircle::onClickEvent(std::vector<OsuBeatmap::CLICK> &clicks)
{
	if (m_bFinished)
		return;

	const Vector2 cursorPos = m_beatmap->getCursorPos();

	const Vector2 pos = m_beatmap->osuCoords2Pixels(m_vRawPos);
	const float cursorDelta = (cursorPos - pos).length();

	if (cursorDelta < m_beatmap->getHitcircleDiameter()/2.0f)
	{
		// note blocking & shake
		if (m_bBlocked)
		{
			m_fShakeAnimation = engine->getTime() + osu_circle_shake_duration.getFloat();
			return; // ignore click event completely
		}

		const long delta = (long)clicks[0].musicPos - (long)m_iTime;

		OsuScore::HIT result = OsuGameRules::getHitResult(delta, m_beatmap);
		if (result != OsuScore::HIT::HIT_NULL)
		{
			const float targetDelta = cursorDelta / (m_beatmap->getHitcircleDiameter()/2.0f);
			const float targetAngle = rad2deg(atan2(cursorPos.y - pos.y, cursorPos.x - pos.x));

			m_beatmap->consumeClickEvent();
			onHit(result, delta, targetDelta, targetAngle);
		}
	}
}

void OsuCircle::onHit(OsuScore::HIT result, long delta, float targetDelta, float targetAngle)
{
	// sound and hit animation
	if (result != OsuScore::HIT::HIT_MISS)
	{
		if (m_osu_timingpoints_force->getBool())
			m_beatmap->updateTimingPoints(m_iTime);

		m_beatmap->getSkin()->playHitCircleSound(m_iSampleType);

		m_fHitAnimation = 0.001f; // quickfix for 1 frame missing images
		anim->moveQuadOut(&m_fHitAnimation, 1.0f, OsuGameRules::getFadeOutTime(m_beatmap), true);

		if (m_beatmap->getOsu()->isInVRMode())
		{
			if (m_bOnHitVRLeftControllerHapticFeedback)
				openvr->getLeftController()->triggerHapticPulse(m_beatmap->getOsu()->getVR()->getHapticPulseStrength());
			else
				openvr->getRightController()->triggerHapticPulse(m_beatmap->getOsu()->getVR()->getHapticPulseStrength());
		}
	}

	// add it, and we are finished
	addHitResult(result, delta, m_vRawPos, targetDelta, targetAngle);
	m_bFinished = true;
}

void OsuCircle::onReset(long curPos)
{
	OsuHitObject::onReset(curPos);

	m_bWaiting = false;
	m_fShakeAnimation = 0.0f;

	anim->deleteExistingAnimation(&m_fHitAnimation);

	if (m_iTime > curPos)
	{
		m_bFinished = false;
		m_fHitAnimation = 0.0f;
	}
	else
	{
		m_bFinished = true;
		m_fHitAnimation = 1.0f;
	}
}

Vector2 OsuCircle::getAutoCursorPos(long curPos)
{
	return m_beatmap->osuCoords2Pixels(m_vRawPos);
}
