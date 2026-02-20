//================ Copyright (c) 2015, PG, All rights reserved. =================//
//
// Purpose:		slider
//
// $NoKeywords: $slider
//===============================================================================//

#include "OsuSlider.h"
#include "OsuSliderCurves.h"

#include "Engine.h"
#include "ConVar.h"
#include "Camera.h"
#include "AnimationHandler.h"
#include "ResourceManager.h"
#include "OpenVRInterface.h"
#include "OpenVRController.h"
#include "SoundEngine.h"

#include "Shader.h"
#include "VertexArrayObject.h"
#include "RenderTarget.h"

#include "Osu.h"
#include "OsuVR.h"
#include "OsuCircle.h"
#include "OsuSkin.h"
#include "OsuSkinImage.h"
#include "OsuGameRules.h"
#include "OsuSliderRenderer.h"
#include "OsuBeatmapStandard.h"
#include "OsuModFPoSu.h"

ConVar osu_slider_ball_tint_combo_color("osu_slider_ball_tint_combo_color", true, FCVAR_NONE);

ConVar osu_snaking_sliders("osu_snaking_sliders", true, FCVAR_NONE);
ConVar osu_mod_hd_slider_fade_percent("osu_mod_hd_slider_fade_percent", 1.0f, FCVAR_NONE);
ConVar osu_mod_hd_slider_fast_fade("osu_mod_hd_slider_fast_fade", false, FCVAR_NONE);

ConVar osu_slider_end_inside_check_offset("osu_slider_end_inside_check_offset", 36, FCVAR_NONE, "offset in milliseconds going backwards from the end point, at which \"being inside the slider\" is checked. (osu bullshit behavior)");
ConVar osu_slider_end_miss_breaks_combo("osu_slider_end_miss_breaks_combo", false, FCVAR_NONE, "should a missed sliderend break combo (aka cause a regular sliderbreak)");
ConVar osu_slider_break_epilepsy("osu_slider_break_epilepsy", false, FCVAR_NONE);
ConVar osu_slider_scorev2("osu_slider_scorev2", false, FCVAR_NONE);

ConVar osu_slider_draw_body("osu_slider_draw_body", true, FCVAR_NONE);
ConVar osu_slider_shrink("osu_slider_shrink", false, FCVAR_NONE);
ConVar osu_slider_snake_duration_multiplier("osu_slider_snake_duration_multiplier", 1.0f, FCVAR_NONE, "the default snaking duration is multiplied with this (max sensible value is 3, anything above that will take longer than the approachtime)");
ConVar osu_slider_reverse_arrow_black_threshold("osu_slider_reverse_arrow_black_threshold", 1.0f, FCVAR_NONE, "Blacken reverse arrows if the average color brightness percentage is above this value"); // looks too shitty atm
ConVar osu_slider_reverse_arrow_fadein_duration("osu_slider_reverse_arrow_fadein_duration", 150, FCVAR_NONE, "duration in ms of the reverse arrow fadein animation after it starts");
ConVar osu_slider_body_smoothsnake("osu_slider_body_smoothsnake", true, FCVAR_NONE, "draw 1 extra interpolated circle mesh at the start & end of every slider for extra smooth snaking/shrinking");
ConVar osu_slider_body_lazer_fadeout_style("osu_slider_body_lazer_fadeout_style", true, FCVAR_NONE, "if snaking out sliders are enabled (aka shrinking sliders), smoothly fade out the last remaining part of the body (instead of vanishing instantly)");
ConVar osu_slider_body_fade_out_time_multiplier("osu_slider_body_fade_out_time_multiplier", 1.0f, FCVAR_NONE, "multiplies osu_hitobject_fade_out_time");
ConVar osu_slider_reverse_arrow_animated("osu_slider_reverse_arrow_animated", true, FCVAR_NONE, "pulse animation on reverse arrows");
ConVar osu_slider_reverse_arrow_alpha_multiplier("osu_slider_reverse_arrow_alpha_multiplier", 1.0f, FCVAR_NONE);

ConVar *OsuSlider::m_osu_playfield_mirror_horizontal_ref = NULL;
ConVar *OsuSlider::m_osu_playfield_mirror_vertical_ref = NULL;
ConVar *OsuSlider::m_osu_playfield_rotation_ref = NULL;
ConVar *OsuSlider::m_osu_mod_fps_ref = NULL;
ConVar *OsuSlider::m_osu_mod_strict_tracking_ref = NULL;
ConVar *OsuSlider::m_osu_slider_border_size_multiplier_ref = NULL;
ConVar *OsuSlider::m_epilepsy_ref = NULL;
ConVar *OsuSlider::m_osu_auto_cursordance_ref = NULL;
ConVar *OsuSlider::m_osu_drain_type_ref = NULL;

OsuSlider::OsuSlider(char type, int repeat, float pixelLength, std::vector<Vector2> points, std::vector<int> hitSounds, std::vector<float> ticks, float sliderTime, float sliderTimeWithoutRepeats, long time, int sampleType, int comboNumber, bool isEndOfCombo, int colorCounter, int colorOffset, OsuBeatmapStandard *beatmap) : OsuHitObject(time, sampleType, comboNumber, isEndOfCombo, colorCounter, colorOffset, beatmap)
{
	if (m_osu_playfield_mirror_horizontal_ref == NULL)
		m_osu_playfield_mirror_horizontal_ref = convar->getConVarByName("osu_playfield_mirror_horizontal");
	if (m_osu_playfield_mirror_vertical_ref == NULL)
		m_osu_playfield_mirror_vertical_ref = convar->getConVarByName("osu_playfield_mirror_vertical");
	if (m_osu_playfield_rotation_ref == NULL)
		m_osu_playfield_rotation_ref = convar->getConVarByName("osu_playfield_rotation");
	if (m_osu_mod_fps_ref == NULL)
		m_osu_mod_fps_ref = convar->getConVarByName("osu_mod_fps");
	if (m_osu_mod_strict_tracking_ref == NULL)
		m_osu_mod_strict_tracking_ref = convar->getConVarByName("osu_mod_strict_tracking");
	if (m_osu_slider_border_size_multiplier_ref == NULL)
		m_osu_slider_border_size_multiplier_ref = convar->getConVarByName("osu_slider_border_size_multiplier");
	if (m_epilepsy_ref == NULL)
		m_epilepsy_ref = convar->getConVarByName("epilepsy");
	if (m_osu_auto_cursordance_ref == NULL)
		m_osu_auto_cursordance_ref = convar->getConVarByName("osu_auto_cursordance");
	if (m_osu_drain_type_ref == NULL)
		m_osu_drain_type_ref = convar->getConVarByName("osu_drain_type");

	m_cType = type;
	m_iRepeat = repeat;
	m_fPixelLength = pixelLength;
	m_points = points;
	m_hitSounds = hitSounds;
	m_fSliderTime = sliderTime;
	m_fSliderTimeWithoutRepeats = sliderTimeWithoutRepeats;

	m_beatmap = beatmap;

	// build raw ticks
	for (int i=0; i<ticks.size(); i++)
	{
		SLIDERTICK st;
		st.finished = false;
		st.percent = ticks[i];
		m_ticks.push_back(st);
	}

	// build curve
	m_curve = OsuSliderCurve::createCurve(m_cType, m_points, m_fPixelLength);

	// build repeats
	for (int i=0; i<(m_iRepeat - 1); i++)
	{
		SLIDERCLICK sc;

		sc.finished = false;
		sc.successful = false;
		sc.type = 0;
		sc.sliderend = ((i % 2) == 0); // for hit animation on repeat hit
		sc.time = m_iTime + (long)(m_fSliderTimeWithoutRepeats * (i + 1));

		m_clicks.push_back(sc);
	}

	// build ticks
	for (int i=0; i<m_iRepeat; i++)
	{
		for (int t=0; t<m_ticks.size(); t++)
		{
			// NOTE: repeat ticks are not necessarily symmetric.
			//
			// e.g. this slider: [1]=======*==[2]
			//
			// the '*' is where the tick is, let's say percent = 0.75
			// on repeat 0, the tick is at: m_iTime + 0.75*m_fSliderTimeWithoutRepeats
			// but on repeat 1, the tick is at: m_iTime + 1*m_fSliderTimeWithoutRepeats + (1.0 - 0.75)*m_fSliderTimeWithoutRepeats
			// this gives better readability at the cost of invalid rhythms: ticks are guaranteed to always be at the same position, even in repeats
			// so, depending on which repeat we are in (even or odd), we either do (percent) or (1.0 - percent)

			const float tickPercentRelativeToRepeatFromStartAbs = (((i + 1) % 2) != 0 ? m_ticks[t].percent : 1.0f - m_ticks[t].percent);

			SLIDERCLICK sc;

			sc.time = m_iTime + (long)(m_fSliderTimeWithoutRepeats * i) + (long)(tickPercentRelativeToRepeatFromStartAbs * m_fSliderTimeWithoutRepeats);
			sc.finished = false;
			sc.successful = false;
			sc.type = 1;
			sc.tickIndex = t;

			m_clicks.push_back(sc);
		}
	}

	m_iObjectDuration = (long)m_fSliderTime;
	m_iObjectDuration = m_iObjectDuration >= 0 ? m_iObjectDuration : 1; // force clamp to positive range

	m_fSlidePercent = 0.0f;
	m_fActualSlidePercent = 0.0f;
	m_fSliderSnakePercent = 0.0f;
	m_fReverseArrowAlpha = 0.0f;
	m_fBodyAlpha = 0.0f;

	m_startResult = OsuScore::HIT::HIT_NULL;
	m_endResult = OsuScore::HIT::HIT_NULL;
	m_bStartFinished = false;
	m_fStartHitAnimation = 0.0f;
	m_bEndFinished = false;
	m_fEndHitAnimation = 0.0f;
	m_fEndSliderBodyFadeAnimation = 0.0f;
	m_iStrictTrackingModLastClickHeldTime = 0;
	m_iDownKey = 0;
	m_iPrevSliderSlideSoundSampleSet = -1;
	m_bCursorLeft = true;
	m_bCursorInside = false;
	m_bHeldTillEnd = false;
	m_bHeldTillEndForLenienceHack = false;
	m_bHeldTillEndForLenienceHackCheck = false;
	m_fFollowCircleTickAnimationScale = 0.0f;
	m_fFollowCircleAnimationScale = 0.0f;
	m_fFollowCircleAnimationAlpha = 0.0f;

	m_iReverseArrowPos = 0;
	m_iCurRepeat = 0;
	m_iCurRepeatCounterForHitSounds = 0;
	m_bInReverse = false;
	m_bHideNumberAfterFirstRepeatHit = false;

	m_fSliderBreakRapeTime = 0.0f;

	m_bOnHitVRLeftControllerHapticFeedback = false;

	m_vao = NULL;
	m_vaoVR2 = NULL;
}

OsuSlider::~OsuSlider()
{
	onReset(0);

	SAFE_DELETE(m_curve);
	SAFE_DELETE(m_vao);
	SAFE_DELETE(m_vaoVR2);
}

void OsuSlider::draw(Graphics *g)
{
	if (m_points.size() <= 0) return;

	OsuSkin *skin = m_beatmap->getSkin();

	const bool isCompletelyFinished = m_bStartFinished && m_bEndFinished && m_bFinished;

	if ((m_bVisible || (m_bStartFinished && !m_bFinished)) && !isCompletelyFinished) // extra possibility to avoid flicker between OsuHitObject::m_bVisible delay and the fadeout animation below this if block
	{
		float alpha = (osu_mod_hd_slider_fast_fade.getBool() ? m_fAlpha : m_fBodyAlpha);
		float sliderSnake = osu_snaking_sliders.getBool() ? m_fSliderSnakePercent : 1.0f;

		// shrinking sliders
		float sliderSnakeStart = 0.0f;
		if (osu_slider_shrink.getBool() && m_iReverseArrowPos == 0)
		{
			sliderSnakeStart = (m_bInReverse ? 0.0f : m_fSlidePercent);
			if (m_bInReverse)
				sliderSnake = m_fSlidePercent;
		}

		// draw slider body
		if (alpha > 0.0f && osu_slider_draw_body.getBool())
			drawBody(g, alpha, sliderSnakeStart, sliderSnake);

		// draw slider ticks
		Color tickColor = 0xffffffff;
		tickColor = COLOR(255, (int)(COLOR_GET_Ri(tickColor)*m_fHittableDimRGBColorMultiplierPercent), (int)(COLOR_GET_Gi(tickColor)*m_fHittableDimRGBColorMultiplierPercent), (int)(COLOR_GET_Bi(tickColor)*m_fHittableDimRGBColorMultiplierPercent));
		const float tickImageScale = (m_beatmap->getHitcircleDiameter() / (16.0f * (skin->isSliderScorePoint2x() ? 2.0f : 1.0f)))*0.125f;
		for (int t=0; t<m_ticks.size(); t++)
		{
			if (m_ticks[t].finished || m_ticks[t].percent > sliderSnake)
				continue;

			Vector2 pos = m_beatmap->osuCoords2Pixels(m_curve->pointAt(m_ticks[t].percent));

			g->setColor(tickColor);
			g->setAlpha(alpha);
			g->pushTransform();
			{
				g->scale(tickImageScale, tickImageScale);
				g->translate(pos.x, pos.y);
				g->drawImage(skin->getSliderScorePoint());
			}
			g->popTransform();
		}

		// draw start & end circle & reverse arrows
		if (m_points.size() > 1)
		{
			// HACKHACK: very dirty code
			bool sliderRepeatStartCircleFinished = (m_iRepeat < 2);
			bool sliderRepeatEndCircleFinished = false;
			bool endCircleIsAtActualSliderEnd = true;
			for (int i=0; i<m_clicks.size(); i++)
			{
				// repeats
				if (m_clicks[i].type == 0)
				{
					endCircleIsAtActualSliderEnd = m_clicks[i].sliderend;

					if (endCircleIsAtActualSliderEnd)
						sliderRepeatEndCircleFinished = m_clicks[i].finished;
					else
						sliderRepeatStartCircleFinished = m_clicks[i].finished;
				}
			}

			const bool ifStrictTrackingModShouldDrawEndCircle = (!m_osu_mod_strict_tracking_ref->getBool() || m_endResult != OsuScore::HIT::HIT_MISS);

			// end circle
			if ((!m_bEndFinished && m_iRepeat % 2 != 0 && ifStrictTrackingModShouldDrawEndCircle) || (!sliderRepeatEndCircleFinished && (ifStrictTrackingModShouldDrawEndCircle || (m_iRepeat > 1 && endCircleIsAtActualSliderEnd) || (m_iRepeat > 1 && std::abs(m_iRepeat - m_iCurRepeat) > 2))))
				drawEndCircle(g, alpha, sliderSnake);

			// start circle
			if (!m_bStartFinished || (!sliderRepeatStartCircleFinished && (ifStrictTrackingModShouldDrawEndCircle || (m_iRepeat > 1 && !endCircleIsAtActualSliderEnd) || (m_iRepeat > 1 && std::abs(m_iRepeat - m_iCurRepeat) > 2))) || (!m_bEndFinished && m_iRepeat % 2 == 0 && ifStrictTrackingModShouldDrawEndCircle))
				drawStartCircle(g, alpha);

			// debug
			/*
			Vector2 debugPos = m_beatmap->osuCoords2Pixels(m_curve->pointAt(0));
			g->pushTransform();
			{
				g->translate(debugPos.x, debugPos.y);
				g->setColor(0xffff0000);
				//g->drawString(engine->getResourceManager()->getFont("FONT_DEFAULT"), UString::format("%i, %i", m_cType, m_points.size()));
				//g->drawString(engine->getResourceManager()->getFont("FONT_DEFAULT"), UString::format("%i", (int)sliderRepeatStartCircleFinished));
				g->drawString(engine->getResourceManager()->getFont("FONT_DEFAULT"), UString::format("%li", m_iTime));
			}
			g->popTransform();
			*/

			// reverse arrows
			if (m_fReverseArrowAlpha > 0.0f)
			{
				// if the combo color is nearly white, blacken the reverse arrow
				Color comboColor = skin->getComboColorForCounter(m_iColorCounter, m_iColorOffset);
				Color reverseArrowColor = 0xffffffff;
				if ((COLOR_GET_Rf(comboColor) + COLOR_GET_Gf(comboColor) + COLOR_GET_Bf(comboColor))/3.0f > osu_slider_reverse_arrow_black_threshold.getFloat())
					reverseArrowColor = 0xff000000;

				reverseArrowColor = COLOR(255, (int)(COLOR_GET_Ri(reverseArrowColor)*m_fHittableDimRGBColorMultiplierPercent), (int)(COLOR_GET_Gi(reverseArrowColor)*m_fHittableDimRGBColorMultiplierPercent), (int)(COLOR_GET_Bi(reverseArrowColor)*m_fHittableDimRGBColorMultiplierPercent));

				float div = 0.30f;
				float pulse = (div - fmod(std::abs(m_beatmap->getCurMusicPos())/1000.0f, div))/div;
				pulse *= pulse; // quad in

				if (!osu_slider_reverse_arrow_animated.getBool() || m_beatmap->isInMafhamRenderChunk())
					pulse = 0.0f;

				// end
				if (m_iReverseArrowPos == 2 || m_iReverseArrowPos == 3)
				{
					/*Vector2 pos = m_beatmap->osuCoords2Pixels(m_curve->pointAt(sliderSnake));*/ // osu doesn't snake the reverse arrow
					Vector2 pos = m_beatmap->osuCoords2Pixels(m_curve->pointAt(1.0f));
					float rotation = m_curve->getEndAngle() - m_osu_playfield_rotation_ref->getFloat() - m_beatmap->getPlayfieldRotation();
					if (m_beatmap->getOsu()->getModHR())
						rotation = 360.0f - rotation;
					if (m_osu_playfield_mirror_horizontal_ref->getBool())
						rotation = 360.0f - rotation;
					if (m_osu_playfield_mirror_vertical_ref->getBool())
						rotation = 180.0f - rotation;

					const float osuCoordScaleMultiplier = m_beatmap->getHitcircleDiameter() / m_beatmap->getRawHitcircleDiameter();
					float reverseArrowImageScale = (m_beatmap->getRawHitcircleDiameter() / (128.0f * (skin->isReverseArrow2x() ? 2.0f : 1.0f))) * osuCoordScaleMultiplier;

					reverseArrowImageScale *= 1.0f + pulse*0.30f;

					g->setColor(reverseArrowColor);
					g->setAlpha(m_fReverseArrowAlpha);
					g->pushTransform();
					{
						g->rotate(rotation);
						g->scale(reverseArrowImageScale, reverseArrowImageScale);
						g->translate(pos.x, pos.y);
						g->drawImage(skin->getReverseArrow());
					}
					g->popTransform();
				}

				// start
				if (m_iReverseArrowPos == 1 || m_iReverseArrowPos == 3)
				{
					Vector2 pos = m_beatmap->osuCoords2Pixels(m_curve->pointAt(0.0f));
					float rotation = m_curve->getStartAngle() - m_osu_playfield_rotation_ref->getFloat() - m_beatmap->getPlayfieldRotation();
					if (m_beatmap->getOsu()->getModHR())
						rotation = 360.0f - rotation;
					if (m_osu_playfield_mirror_horizontal_ref->getBool())
						rotation = 360.0f - rotation;
					if (m_osu_playfield_mirror_vertical_ref->getBool())
						rotation = 180.0f - rotation;

					const float osuCoordScaleMultiplier = m_beatmap->getHitcircleDiameter() / m_beatmap->getRawHitcircleDiameter();
					float reverseArrowImageScale = (m_beatmap->getRawHitcircleDiameter() / (128.0f * (skin->isReverseArrow2x() ? 2.0f : 1.0f))) * osuCoordScaleMultiplier;

					reverseArrowImageScale *= 1.0f + pulse*0.30f;

					g->setColor(reverseArrowColor);
					g->setAlpha(m_fReverseArrowAlpha);
					g->pushTransform();
					{
						g->rotate(rotation);
						g->scale(reverseArrowImageScale, reverseArrowImageScale);
						g->translate(pos.x, pos.y);
						g->drawImage(skin->getReverseArrow());
					}
					g->popTransform();
				}
			}
		}
	}

	// slider body fade animation, draw start/end circle hit animation

	if (m_fEndSliderBodyFadeAnimation > 0.0f && m_fEndSliderBodyFadeAnimation != 1.0f && !m_beatmap->getOsu()->getModHD())
	{
		std::vector<Vector2> emptyVector;
		std::vector<Vector2> alwaysPoints;
		alwaysPoints.push_back(m_beatmap->osuCoords2Pixels(m_curve->pointAt(m_fSlidePercent)));
		if (!osu_slider_shrink.getBool())
			drawBody(g, 1.0f - m_fEndSliderBodyFadeAnimation, 0, 1);
		else if (osu_slider_body_lazer_fadeout_style.getBool())
			OsuSliderRenderer::draw(g, m_beatmap->getOsu(), emptyVector, alwaysPoints, m_beatmap->getHitcircleDiameter(), 0.0f, 0.0f, m_beatmap->getSkin()->getComboColorForCounter(m_iColorCounter, m_iColorOffset), 1.0f, 1.0f - m_fEndSliderBodyFadeAnimation, getTime());
	}

	if (m_fStartHitAnimation > 0.0f && m_fStartHitAnimation != 1.0f && !m_beatmap->getOsu()->getModHD())
	{
		float alpha = 1.0f - m_fStartHitAnimation;

		float scale = m_fStartHitAnimation;
		scale = -scale*(scale-2.0f); // quad out scale

		bool drawNumber = (skin->getVersion() > 1.0f ? false : true) && m_iCurRepeat < 1;

		g->pushTransform();
		{
			g->scale((1.0f + scale*OsuGameRules::osu_circle_fade_out_scale.getFloat()), (1.0f + scale*OsuGameRules::osu_circle_fade_out_scale.getFloat()));
			if (m_iCurRepeat < 1)
			{
				m_beatmap->getSkin()->getHitCircleOverlay2()->setAnimationTimeOffset(!m_beatmap->isInMafhamRenderChunk() ? m_iTime - m_iApproachTime : m_beatmap->getCurMusicPosWithOffsets());
				m_beatmap->getSkin()->getSliderStartCircleOverlay2()->setAnimationTimeOffset(!m_beatmap->isInMafhamRenderChunk() ? m_iTime - m_iApproachTime : m_beatmap->getCurMusicPosWithOffsets());

				OsuCircle::drawSliderStartCircle(g, m_beatmap, m_curve->pointAt(0.0f), m_iComboNumber, m_iColorCounter, m_iColorOffset, 1.0f, 1.0f, alpha, alpha, drawNumber);
			}
			else
			{
				m_beatmap->getSkin()->getHitCircleOverlay2()->setAnimationTimeOffset(!m_beatmap->isInMafhamRenderChunk() ? m_iTime : m_beatmap->getCurMusicPosWithOffsets());
				m_beatmap->getSkin()->getSliderEndCircleOverlay2()->setAnimationTimeOffset(!m_beatmap->isInMafhamRenderChunk() ? m_iTime : m_beatmap->getCurMusicPosWithOffsets());

				OsuCircle::drawSliderEndCircle(g, m_beatmap, m_curve->pointAt(0.0f), m_iComboNumber, m_iColorCounter, m_iColorOffset, 1.0f, 1.0f, alpha, alpha, drawNumber);
			}
		}
		g->popTransform();
	}

	if (m_fEndHitAnimation > 0.0f && m_fEndHitAnimation != 1.0f && !m_beatmap->getOsu()->getModHD())
	{
		float alpha = 1.0f - m_fEndHitAnimation;

		float scale = m_fEndHitAnimation;
		scale = -scale*(scale-2.0f); // quad out scale

		g->pushTransform();
		{
			g->scale((1.0f + scale*OsuGameRules::osu_circle_fade_out_scale.getFloat()), (1.0f + scale*OsuGameRules::osu_circle_fade_out_scale.getFloat()));
			{
				m_beatmap->getSkin()->getHitCircleOverlay2()->setAnimationTimeOffset(!m_beatmap->isInMafhamRenderChunk() ? m_iTime - m_iFadeInTime : m_beatmap->getCurMusicPosWithOffsets());
				m_beatmap->getSkin()->getSliderEndCircleOverlay2()->setAnimationTimeOffset(!m_beatmap->isInMafhamRenderChunk() ? m_iTime - m_iFadeInTime : m_beatmap->getCurMusicPosWithOffsets());

				OsuCircle::drawSliderEndCircle(g, m_beatmap, m_curve->pointAt(1.0f), m_iComboNumber, m_iColorCounter, m_iColorOffset, 1.0f, 1.0f, alpha, 0.0f, false);
			}
		}
		g->popTransform();
	}

	OsuHitObject::draw(g);

	// debug
	/*
	if (m_bVisible)
	{
		Vector2 screenPos = m_beatmap->osuCoords2Pixels(getRawPosAt(0));

		g->setColor(0xffffffff);
		g->pushTransform();
		g->translate(screenPos.x, screenPos.y + 50);
		g->drawString(engine->getResourceManager()->getFont("FONT_DEFAULT"), UString::format("%f", m_fSliderSnakePercent));
		g->popTransform();
	}
	*/

	// debug
	/*
	if (m_bBlocked)
	{
		const Vector2 pos = m_beatmap->osuCoords2Pixels(getRawPosAt(0));

		g->setColor(0xbbff0000);
		g->fillRect(pos.x - 20, pos.y - 20, 40, 40);
	}
	*/
}

void OsuSlider::draw2(Graphics *g)
{
	draw2(g, true, false);

	// TEMP: DEBUG:
	/*
	if (m_bVisible)
	{
		const long lenienceHackEndTime = std::max(m_iTime + m_iObjectDuration / 2, (m_iTime + m_iObjectDuration) - (long)osu_slider_end_inside_check_offset.getInt());
		Vector2 pos = m_beatmap->osuCoords2Pixels(getRawPosAt(lenienceHackEndTime));

		const int size = 30;
		g->setColor(!m_bHeldTillEndForLenienceHackCheck ? 0xff00ff00 : 0xffff0000);
		g->drawLine(pos.x - size, pos.y - size, pos.x + size, pos.y + size);
		g->drawLine(pos.x + size, pos.y - size, pos.x - size, pos.y + size);

		const long lenience300EndTime = m_iTime + m_iObjectDuration - (long)OsuGameRules::getHitWindow300(m_beatmap);
		pos = m_beatmap->osuCoords2Pixels(getRawPosAt(lenience300EndTime));
		g->setColor(0xff0000ff);
		g->drawLine(pos.x - size, pos.y - size, pos.x + size, pos.y + size);
		g->drawLine(pos.x + size, pos.y - size, pos.x - size, pos.y + size);
	}
	*/
}

void OsuSlider::draw2(Graphics *g, bool drawApproachCircle, bool drawOnlyApproachCircle)
{
	OsuHitObject::draw2(g);

	OsuSkin *skin = m_beatmap->getSkin();

	// HACKHACK: so much code duplication aaaaaaah
	if ((m_bVisible || (m_bStartFinished && !m_bFinished)) && drawApproachCircle) // extra possibility to avoid flicker between OsuHitObject::m_bVisible delay and the fadeout animation below this if block
	{
		if (m_points.size() > 1)
		{
			// HACKHACK: very dirty code
			bool sliderRepeatStartCircleFinished = m_iRepeat < 2;
			//bool sliderRepeatEndCircleFinished = false;
			for (int i=0; i<m_clicks.size(); i++)
			{
				if (m_clicks[i].type == 0)
				{
					if (m_clicks[i].sliderend)
					{
						//sliderRepeatEndCircleFinished = m_clicks[i].finished;
					}
					else
						sliderRepeatStartCircleFinished = m_clicks[i].finished;
				}
			}

			// start circle
			if (!m_bStartFinished || !sliderRepeatStartCircleFinished || (!m_bEndFinished && m_iRepeat % 2 == 0))
			{
				OsuCircle::drawApproachCircle(g, m_beatmap, m_curve->pointAt(0.0f), m_iComboNumber, m_iColorCounter, m_iColorOffset, m_fHittableDimRGBColorMultiplierPercent, m_fApproachScale, m_fAlphaForApproachCircle, m_bOverrideHDApproachCircle);
			}
		}
	}

	if (drawApproachCircle && drawOnlyApproachCircle) return;

	// draw followcircle
	// HACKHACK: this is not entirely correct (due to m_bHeldTillEnd, if held within 300 range but then released, will flash followcircle at the end)
	if ((m_bVisible && m_bCursorInside && (isClickHeldSlider() || m_beatmap->getOsu()->getModAuto() || m_beatmap->getOsu()->getModRelax())) || (m_bFinished && m_fFollowCircleAnimationAlpha > 0.0f && m_bHeldTillEnd))
	{
		Vector2 point = m_beatmap->osuCoords2Pixels(m_vCurPointRaw);

		// HACKHACK: this is shit
		float tickAnimation = (m_fFollowCircleTickAnimationScale < 0.1f ? m_fFollowCircleTickAnimationScale/0.1f : (1.0f - m_fFollowCircleTickAnimationScale)/0.9f);
		if (m_fFollowCircleTickAnimationScale < 0.1f)
		{
			tickAnimation = -tickAnimation*(tickAnimation-2.0f);
			tickAnimation = clamp<float>(tickAnimation / 0.02f, 0.0f, 1.0f);
		}
		float tickAnimationScale = 1.0f + tickAnimation*OsuGameRules::osu_slider_followcircle_tick_pulse_scale.getFloat();

		g->setColor(0xffffffff);
		g->setAlpha(m_fFollowCircleAnimationAlpha);
		skin->getSliderFollowCircle2()->setAnimationTimeOffset(m_iTime);
		skin->getSliderFollowCircle2()->drawRaw(g, point, (m_beatmap->getSliderFollowCircleDiameter() / skin->getSliderFollowCircle2()->getSizeBaseRaw().x)*tickAnimationScale*m_fFollowCircleAnimationScale*0.85f); // this is a bit strange, but seems to work perfectly with 0.85
	}

	const bool isCompletelyFinished = m_bStartFinished && m_bEndFinished && m_bFinished;

	// draw sliderb on top of everything
	if ((m_bVisible || (m_bStartFinished && !m_bFinished)) && !isCompletelyFinished) // extra possibility in the if-block to avoid flicker between OsuHitObject::m_bVisible delay and the fadeout animation below this if-block
	{
		if (m_fSlidePercent > 0.0f)
		{
			// draw sliderb
			Vector2 point = m_beatmap->osuCoords2Pixels(m_vCurPointRaw);
			Vector2 c1 = m_beatmap->osuCoords2Pixels(m_curve->pointAt(m_fSlidePercent + 0.01f <= 1.0f ? m_fSlidePercent : m_fSlidePercent - 0.01f));
			Vector2 c2 = m_beatmap->osuCoords2Pixels(m_curve->pointAt(m_fSlidePercent + 0.01f <= 1.0f ? m_fSlidePercent + 0.01f : m_fSlidePercent));
			float ballAngle = rad2deg( atan2(c2.y - c1.y, c2.x - c1.x) );
			if (skin->getSliderBallFlip())
				ballAngle += (m_iCurRepeat % 2 == 0) ? 0 : 180;

			g->setColor(skin->getAllowSliderBallTint() ? (osu_slider_ball_tint_combo_color.getBool() ? skin->getComboColorForCounter(m_iColorCounter, m_iColorOffset) : skin->getSliderBallColor()) : 0xffffffff);
			g->pushTransform();
			{
				g->rotate(ballAngle);
				skin->getSliderb()->setAnimationTimeOffset(m_iTime);
				skin->getSliderb()->drawRaw(g, point, m_beatmap->getHitcircleDiameter() / skin->getSliderb()->getSizeBaseRaw().x);
			}
			g->popTransform();
		}
	}
}

void OsuSlider::drawVR(Graphics *g, Matrix4 &mvp, OsuVR *vr)
{
	// HACKHACK: code duplication aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaah
	if (m_points.size() <= 0) return;

	// the approachscale for sliders will get negative (since they are hitobjects with a duration)
	float clampedApproachScalePercent = m_fApproachScale - 1.0f; // goes from <m_osu_approach_scale_multiplier_ref> to 0
	clampedApproachScalePercent = clamp<float>(clampedApproachScalePercent / m_osu_approach_scale_multiplier_ref->getFloat(), 0.0f, 1.0f); // goes from 1 to 0

	Matrix4 translation;
	translation.translate(0, 0, -clampedApproachScalePercent*vr->getApproachDistance());
	Matrix4 finalMVP = mvp * translation;

	vr->getShaderTexturedLegacyGeneric()->setUniformMatrix4fv("matrix", finalMVP);

	OsuSkin *skin = m_beatmap->getSkin();

	if (m_bVisible || (m_bStartFinished && !m_bFinished)) // extra possibility to avoid flicker between OsuHitObject::m_bVisible delay and the fadeout animation below this if block
	{
		float alpha = (osu_mod_hd_slider_fast_fade.getBool() ? m_fAlpha : m_fBodyAlpha);
		float sliderSnake = osu_snaking_sliders.getBool() ? m_fSliderSnakePercent : 1.0f;

		// shrinking sliders
		float sliderSnakeStart = 0.0f;
		if (osu_slider_shrink.getBool() && m_iReverseArrowPos == 0)
		{
			sliderSnakeStart = (m_bInReverse ? 0.0f : m_fSlidePercent);
			if (m_bInReverse)
				sliderSnake = m_fSlidePercent;
		}

		// draw slider body
		if (alpha > 0.0f && osu_slider_draw_body.getBool())
			drawBodyVR(g, vr, finalMVP, alpha, sliderSnakeStart, sliderSnake);

		// draw slider ticks
		const float tickImageScale = (m_beatmap->getHitcircleDiameter() / (16.0f * (skin->isSliderScorePoint2x() ? 2.0f : 1.0f)))*0.125f;
		for (int t=0; t<m_ticks.size(); t++)
		{
			if (m_ticks[t].finished || m_ticks[t].percent > sliderSnake)
				continue;

			Vector2 pos = m_beatmap->osuCoords2Pixels(m_curve->pointAt(m_ticks[t].percent));

			g->setColor(0xffffffff);
			g->setAlpha(alpha);
			g->pushTransform();
			{
				g->scale(tickImageScale, tickImageScale);
				g->translate(pos.x, pos.y);
				g->drawImage(skin->getSliderScorePoint());
			}
			g->popTransform();
		}

		// draw start & end circle & reverse arrows
		if (m_points.size() > 1)
		{
			// HACKHACK: very dirty code
			bool sliderRepeatStartCircleFinished = (m_iRepeat < 2);
			bool sliderRepeatEndCircleFinished = false;
			bool endCircleIsAtActualSliderEnd = true;
			for (int i=0; i<m_clicks.size(); i++)
			{
				// repeats
				if (m_clicks[i].type == 0)
				{
					endCircleIsAtActualSliderEnd = m_clicks[i].sliderend;

					if (endCircleIsAtActualSliderEnd)
						sliderRepeatEndCircleFinished = m_clicks[i].finished;
					else
						sliderRepeatStartCircleFinished = m_clicks[i].finished;
				}
			}

			const bool ifStrictTrackingModShouldDrawEndCircle = (!m_osu_mod_strict_tracking_ref->getBool() || m_endResult != OsuScore::HIT::HIT_MISS);

			// end circle
			if ((!m_bEndFinished && m_iRepeat % 2 != 0 && ifStrictTrackingModShouldDrawEndCircle) || (!sliderRepeatEndCircleFinished && (ifStrictTrackingModShouldDrawEndCircle || (m_iRepeat > 1 && endCircleIsAtActualSliderEnd) || (m_iRepeat > 1 && std::abs(m_iRepeat - m_iCurRepeat) > 2))))
				drawEndCircle(g, alpha, sliderSnake);

			// start circle
			if (!m_bStartFinished || (!sliderRepeatStartCircleFinished && (ifStrictTrackingModShouldDrawEndCircle || (m_iRepeat > 1 && !endCircleIsAtActualSliderEnd) || (m_iRepeat > 1 && std::abs(m_iRepeat - m_iCurRepeat) > 2))) || (!m_bEndFinished && m_iRepeat % 2 == 0 && ifStrictTrackingModShouldDrawEndCircle))
				drawStartCircle(g, alpha);

			// reverse arrows
			if (m_fReverseArrowAlpha > 0.0f)
			{
				// if the combo color is nearly white, blacken the reverse arrow
				Color comboColor = skin->getComboColorForCounter(m_iColorCounter, m_iColorOffset);
				Color reverseArrowColor = 0xffffffff;
				if ((COLOR_GET_Rf(comboColor) + COLOR_GET_Gf(comboColor) + COLOR_GET_Bf(comboColor))/3.0f > osu_slider_reverse_arrow_black_threshold.getFloat())
					reverseArrowColor = 0xff000000;

				float div = 0.30f;
				float pulse = (div - fmod(std::abs(m_beatmap->getCurMusicPos())/1000.0f, div))/div;
				pulse *= pulse; // quad in

				if (!osu_slider_reverse_arrow_animated.getBool() || m_beatmap->isInMafhamRenderChunk())
					pulse = 0.0f;

				// end
				if (m_iReverseArrowPos == 2 || m_iReverseArrowPos == 3)
				{
					/*Vector2 pos = m_beatmap->osuCoords2Pixels(m_curve->pointAt(sliderSnake));*/ // osu doesn't snake the reverse arrow
					Vector2 pos = m_beatmap->osuCoords2Pixels(m_curve->pointAt(1.0f));
					float rotation = m_curve->getEndAngle() - m_osu_playfield_rotation_ref->getFloat() - m_beatmap->getPlayfieldRotation();
					if (m_beatmap->getOsu()->getModHR())
						rotation = 360.0f - rotation;
					if (m_osu_playfield_mirror_horizontal_ref->getBool())
						rotation = 360.0f - rotation;
					if (m_osu_playfield_mirror_vertical_ref->getBool())
						rotation = 180.0f - rotation;

					const float osuCoordScaleMultiplier = m_beatmap->getHitcircleDiameter() / m_beatmap->getRawHitcircleDiameter();
					float reverseArrowImageScale = (m_beatmap->getRawHitcircleDiameter() / (128.0f * (skin->isReverseArrow2x() ? 2.0f : 1.0f))) * osuCoordScaleMultiplier;

					reverseArrowImageScale *= 1.0f + pulse*0.30f;

					g->setColor(reverseArrowColor);
					g->setAlpha(m_fReverseArrowAlpha);
					g->pushTransform();
					{
						g->rotate(rotation);
						g->scale(reverseArrowImageScale, reverseArrowImageScale);
						g->translate(pos.x, pos.y);
						g->drawImage(skin->getReverseArrow());
					}
					g->popTransform();
				}

				// start
				if (m_iReverseArrowPos == 1 || m_iReverseArrowPos == 3)
				{
					Vector2 pos = m_beatmap->osuCoords2Pixels(m_curve->pointAt(0.0f));
					float rotation = m_curve->getStartAngle() - m_osu_playfield_rotation_ref->getFloat() - m_beatmap->getPlayfieldRotation();
					if (m_beatmap->getOsu()->getModHR())
						rotation = 360.0f - rotation;
					if (m_osu_playfield_mirror_horizontal_ref->getBool())
						rotation = 360.0f - rotation;
					if (m_osu_playfield_mirror_vertical_ref->getBool())
						rotation = 180.0f - rotation;

					const float osuCoordScaleMultiplier = m_beatmap->getHitcircleDiameter() / m_beatmap->getRawHitcircleDiameter();
					float reverseArrowImageScale = (m_beatmap->getRawHitcircleDiameter() / (128.0f * (skin->isReverseArrow2x() ? 2.0f : 1.0f))) * osuCoordScaleMultiplier;

					reverseArrowImageScale *= 1.0f + pulse*0.30f;

					g->setColor(reverseArrowColor);
					g->setAlpha(m_fReverseArrowAlpha);
					g->pushTransform();
					{
						g->rotate(rotation);
						g->scale(reverseArrowImageScale, reverseArrowImageScale);
						g->translate(pos.x, pos.y);
						g->drawImage(skin->getReverseArrow());
					}
					g->popTransform();
				}
			}
		}
	}

	// draw slider body fade animation, start/end circle hit animation
	// NOTE: sliderbody alpha fades don't work in VR yet, due to the hacky rendering method
	/*
	if (m_fEndSliderBodyFadeAnimation > 0.0f && m_fEndSliderBodyFadeAnimation != 1.0f && !m_beatmap->getOsu()->getModHD() && !osu_slider_shrink.getBool())
	{
		drawBodyVR(g, vr, finalMVP, 1.0f - m_fEndSliderBodyFadeAnimation, 0, 1);
	}
	*/

	if (m_fStartHitAnimation > 0.0f && m_fStartHitAnimation != 1.0f && !m_beatmap->getOsu()->getModHD())
	{
		float alpha = 1.0f - m_fStartHitAnimation;

		float scale = m_fStartHitAnimation;
		scale = -scale*(scale-2.0f); // quad out scale

		bool drawNumber = (skin->getVersion() > 1.0f ? false : true) && m_iCurRepeat < 1;

		g->pushTransform();
		{
			g->scale((1.0f+scale*OsuGameRules::osu_circle_fade_out_scale.getFloat()), (1.0f+scale*OsuGameRules::osu_circle_fade_out_scale.getFloat()));
			if (m_iCurRepeat < 1)
			{
				m_beatmap->getSkin()->getHitCircleOverlay2()->setAnimationTimeOffset(!m_beatmap->isInMafhamRenderChunk() ? m_iTime - m_iApproachTime : m_beatmap->getCurMusicPosWithOffsets());
				m_beatmap->getSkin()->getSliderStartCircleOverlay2()->setAnimationTimeOffset(!m_beatmap->isInMafhamRenderChunk() ? m_iTime - m_iApproachTime : m_beatmap->getCurMusicPosWithOffsets());

				OsuCircle::drawSliderStartCircle(g, m_beatmap, m_curve->pointAt(0.0f), m_iComboNumber, m_iColorCounter, m_iColorOffset, m_fHittableDimRGBColorMultiplierPercent, 1.0f, alpha, alpha, drawNumber);
			}
			else
			{
				m_beatmap->getSkin()->getHitCircleOverlay2()->setAnimationTimeOffset(!m_beatmap->isInMafhamRenderChunk() ? m_iTime : m_beatmap->getCurMusicPosWithOffsets());
				m_beatmap->getSkin()->getSliderEndCircleOverlay2()->setAnimationTimeOffset(!m_beatmap->isInMafhamRenderChunk() ? m_iTime : m_beatmap->getCurMusicPosWithOffsets());

				OsuCircle::drawSliderEndCircle(g, m_beatmap, m_curve->pointAt(0.0f), m_iComboNumber, m_iColorCounter, m_iColorOffset, m_fHittableDimRGBColorMultiplierPercent, 1.0f, alpha, alpha, drawNumber);
			}
		}
		g->popTransform();
	}

	if (m_fEndHitAnimation > 0.0f && m_fEndHitAnimation != 1.0f && !m_beatmap->getOsu()->getModHD())
	{
		float alpha = 1.0f - m_fEndHitAnimation;

		float scale = m_fEndHitAnimation;
		scale = -scale*(scale-2.0f); // quad out scale

		g->pushTransform();
		{
			g->scale((1.0f+scale*OsuGameRules::osu_circle_fade_out_scale.getFloat()), (1.0f+scale*OsuGameRules::osu_circle_fade_out_scale.getFloat()));
			{
				m_beatmap->getSkin()->getHitCircleOverlay2()->setAnimationTimeOffset(!m_beatmap->isInMafhamRenderChunk() ? m_iTime - m_iFadeInTime : m_beatmap->getCurMusicPosWithOffsets());
				m_beatmap->getSkin()->getSliderEndCircleOverlay2()->setAnimationTimeOffset(!m_beatmap->isInMafhamRenderChunk() ? m_iTime - m_iFadeInTime : m_beatmap->getCurMusicPosWithOffsets());

				OsuCircle::drawSliderEndCircle(g, m_beatmap, m_curve->pointAt(1.0f), m_iComboNumber, m_iColorCounter, m_iColorOffset, 1.0f, 1.0f, alpha, 0.0f, false);
			}
		}
		g->popTransform();
	}

	OsuHitObject::draw(g);

	draw2(g, false, false);

	if (m_osu_vr_draw_approach_circles->getBool() && !m_osu_vr_approach_circles_on_top->getBool())
	{
		if (m_osu_vr_approach_circles_on_playfield->getBool())
			vr->getShaderTexturedLegacyGeneric()->setUniformMatrix4fv("matrix", mvp);

		draw2(g, true, true);
	}
}

void OsuSlider::drawVR2(Graphics *g, Matrix4 &mvp, OsuVR *vr)
{
	// HACKHACK: code duplication aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa
	if (m_points.size() <= 0) return;

	// the approachscale for sliders will get negative (since they are hitobjects with a duration)
	float clampedApproachScalePercent = m_fApproachScale - 1.0f; // goes from <m_osu_approach_scale_multiplier_ref> to 0
	clampedApproachScalePercent = clamp<float>(clampedApproachScalePercent / m_osu_approach_scale_multiplier_ref->getFloat(), 0.0f, 1.0f); // goes from 1 to 0

	if (m_osu_vr_approach_circles_on_playfield->getBool())
		clampedApproachScalePercent = 0.0f;

	Matrix4 translation;
	translation.translate(0, 0, -clampedApproachScalePercent*vr->getApproachDistance());
	Matrix4 finalMVP = mvp * translation;

	vr->getShaderTexturedLegacyGeneric()->setUniformMatrix4fv("matrix", finalMVP);
	draw2(g, true, true);
}

void OsuSlider::draw3D(Graphics *g)
{
	if (m_points.size() <= 0) return;

	OsuSkin *skin = m_beatmap->getSkin();

	const bool isCompletelyFinished = m_bStartFinished && m_bEndFinished && m_bFinished;

	if ((m_bVisible || (m_bStartFinished && !m_bFinished)) && !isCompletelyFinished) // extra possibility to avoid flicker between OsuHitObject::m_bVisible delay and the fadeout animation below this if block
	{
		float alpha = (osu_mod_hd_slider_fast_fade.getBool() ? m_fAlpha : m_fBodyAlpha);
		float sliderSnake = osu_snaking_sliders.getBool() ? m_fSliderSnakePercent : 1.0f;

		// shrinking sliders
		//float sliderSnakeStart = 0.0f;
		if (osu_slider_shrink.getBool() && m_iReverseArrowPos == 0)
		{
			//sliderSnakeStart = (m_bInReverse ? 0.0f : m_fSlidePercent);
			if (m_bInReverse)
				sliderSnake = m_fSlidePercent;
		}

		Matrix4 baseScale;
		baseScale.scale(m_beatmap->getRawHitcircleDiameter() * OsuModFPoSu::SIZEDIV3D);
		baseScale.scale(m_beatmap->getOsu()->getFPoSu()->get3DPlayfieldScale());

		// TODO: draw slider body
		/*
		if (alpha > 0.0f && osu_slider_draw_body.getBool())
			drawBody(g, alpha, sliderSnakeStart, sliderSnake);
		*/

		// draw slider ticks
		Color tickColor = 0xffffffff;
		tickColor = COLOR(255, (int)(COLOR_GET_Ri(tickColor)*m_fHittableDimRGBColorMultiplierPercent), (int)(COLOR_GET_Gi(tickColor)*m_fHittableDimRGBColorMultiplierPercent), (int)(COLOR_GET_Bi(tickColor)*m_fHittableDimRGBColorMultiplierPercent));
		for (int t=0; t<m_ticks.size(); t++)
		{
			if (m_ticks[t].finished || m_ticks[t].percent > sliderSnake)
				continue;

			Vector3 pos = m_beatmap->osuCoordsTo3D(m_curve->pointAt(m_ticks[t].percent), this);

			g->setColor(tickColor);
			g->setAlpha(alpha);
			g->pushTransform();
			{
				Matrix4 modelMatrix;
				{
					Matrix4 scale = baseScale;
					scale.scale((skin->getSliderScorePoint()->getSize().x / (16.0f * (skin->isSliderScorePoint2x() ? 2.0f : 1.0f)))*0.125f);

					Matrix4 translation;
					translation.translate(pos.x, pos.y, pos.z);

					if (m_fposu_3d_hitobjects_look_at_player_ref->getBool())
						modelMatrix = translation * Camera::buildMatrixLookAt(Vector3(0, 0, 0), pos - m_beatmap->getOsu()->getFPoSu()->getCamera()->getPos(), Vector3(0, 1, 0)).invert() * scale;
					else
						modelMatrix = translation * scale;
				}
				g->setWorldMatrixMul(modelMatrix);

				skin->getSliderScorePoint()->bind();
				{
					m_beatmap->getOsu()->getFPoSu()->getUVPlaneModel()->draw3D(g);
				}
				skin->getSliderScorePoint()->unbind();
			}
			g->popTransform();
		}

		// draw start & end circle & reverse arrows
		if (m_points.size() > 1)
		{
			// HACKHACK: very dirty code
			bool sliderRepeatStartCircleFinished = (m_iRepeat < 2);
			bool sliderRepeatEndCircleFinished = false;
			bool endCircleIsAtActualSliderEnd = true;
			for (int i=0; i<m_clicks.size(); i++)
			{
				// repeats
				if (m_clicks[i].type == 0)
				{
					endCircleIsAtActualSliderEnd = m_clicks[i].sliderend;

					if (endCircleIsAtActualSliderEnd)
						sliderRepeatEndCircleFinished = m_clicks[i].finished;
					else
						sliderRepeatStartCircleFinished = m_clicks[i].finished;
				}
			}

			const bool ifStrictTrackingModShouldDrawEndCircle = (!m_osu_mod_strict_tracking_ref->getBool() || m_endResult != OsuScore::HIT::HIT_MISS);

			// end circle
			if ((!m_bEndFinished && m_iRepeat % 2 != 0 && ifStrictTrackingModShouldDrawEndCircle) || (!sliderRepeatEndCircleFinished && (ifStrictTrackingModShouldDrawEndCircle || (m_iRepeat > 1 && endCircleIsAtActualSliderEnd) || (m_iRepeat > 1 && std::abs(m_iRepeat - m_iCurRepeat) > 2))))
				draw3DEndCircle(g, baseScale, alpha, sliderSnake);

			// start circle
			if (!m_bStartFinished || (!sliderRepeatStartCircleFinished && (ifStrictTrackingModShouldDrawEndCircle || (m_iRepeat > 1 && !endCircleIsAtActualSliderEnd) || (m_iRepeat > 1 && std::abs(m_iRepeat - m_iCurRepeat) > 2))) || (!m_bEndFinished && m_iRepeat % 2 == 0 && ifStrictTrackingModShouldDrawEndCircle))
				draw3DStartCircle(g, baseScale, alpha);

			// reverse arrows
			if (m_fReverseArrowAlpha > 0.0f)
			{
				// if the combo color is nearly white, blacken the reverse arrow
				Color comboColor = skin->getComboColorForCounter(m_iColorCounter, m_iColorOffset);
				Color reverseArrowColor = 0xffffffff;
				if ((COLOR_GET_Rf(comboColor) + COLOR_GET_Gf(comboColor) + COLOR_GET_Bf(comboColor))/3.0f > osu_slider_reverse_arrow_black_threshold.getFloat())
					reverseArrowColor = 0xff000000;

				reverseArrowColor = COLOR(255, (int)(COLOR_GET_Ri(reverseArrowColor)*m_fHittableDimRGBColorMultiplierPercent), (int)(COLOR_GET_Gi(reverseArrowColor)*m_fHittableDimRGBColorMultiplierPercent), (int)(COLOR_GET_Bi(reverseArrowColor)*m_fHittableDimRGBColorMultiplierPercent));

				float div = 0.30f;
				float pulse = (div - fmod(std::abs(m_beatmap->getCurMusicPos())/1000.0f, div))/div;
				pulse *= pulse; // quad in

				if (!osu_slider_reverse_arrow_animated.getBool() || m_beatmap->isInMafhamRenderChunk())
					pulse = 0.0f;

				// end
				if (m_iReverseArrowPos == 2 || m_iReverseArrowPos == 3)
				{
					Vector3 pos = m_beatmap->osuCoordsTo3D(m_curve->pointAt(1.0f), this);
					float rotation = m_curve->getEndAngle() - m_osu_playfield_rotation_ref->getFloat() - m_beatmap->getPlayfieldRotation();
					if (m_beatmap->getOsu()->getModHR())
						rotation = 360.0f - rotation;
					if (m_osu_playfield_mirror_horizontal_ref->getBool())
						rotation = 360.0f - rotation;
					if (m_osu_playfield_mirror_vertical_ref->getBool())
						rotation = 180.0f - rotation;

					float reverseArrowImageScale = skin->getReverseArrow()->getSize().x / (128.0f * (skin->isReverseArrow2x() ? 2.0f : 1.0f));

					reverseArrowImageScale *= 1.0f + pulse*0.30f;

					g->setColor(reverseArrowColor);
					g->setAlpha(m_fReverseArrowAlpha);
					g->pushTransform();
					{
						Matrix4 modelMatrix;
						{
							Matrix4 rotate;
							rotate.rotateZ(-rotation);

							Matrix4 scale = baseScale;
							scale.scale(reverseArrowImageScale);

							Matrix4 translation;
							translation.translate(pos.x, pos.y, pos.z);

							if (m_fposu_3d_hitobjects_look_at_player_ref->getBool())
								modelMatrix = translation * Camera::buildMatrixLookAt(Vector3(0, 0, 0), pos - m_beatmap->getOsu()->getFPoSu()->getCamera()->getPos(), Vector3(0, 1, 0)).invert() * scale * rotate;
							else
								modelMatrix = translation * scale * rotate;
						}
						g->setWorldMatrixMul(modelMatrix);

						skin->getReverseArrow()->bind();
						{
							m_beatmap->getOsu()->getFPoSu()->getUVPlaneModel()->draw3D(g);
						}
						skin->getReverseArrow()->unbind();
					}
					g->popTransform();
				}

				// start
				if (m_iReverseArrowPos == 1 || m_iReverseArrowPos == 3)
				{
					Vector3 pos = m_beatmap->osuCoordsTo3D(m_curve->pointAt(0.0f), this);
					float rotation = m_curve->getStartAngle() - m_osu_playfield_rotation_ref->getFloat() - m_beatmap->getPlayfieldRotation();
					if (m_beatmap->getOsu()->getModHR())
						rotation = 360.0f - rotation;
					if (m_osu_playfield_mirror_horizontal_ref->getBool())
						rotation = 360.0f - rotation;
					if (m_osu_playfield_mirror_vertical_ref->getBool())
						rotation = 180.0f - rotation;

					float reverseArrowImageScale = skin->getReverseArrow()->getSize().x / (128.0f * (skin->isReverseArrow2x() ? 2.0f : 1.0f));

					reverseArrowImageScale *= 1.0f + pulse*0.30f;

					g->setColor(reverseArrowColor);
					g->setAlpha(m_fReverseArrowAlpha);
					g->pushTransform();
					{
						Matrix4 modelMatrix;
						{
							Matrix4 rotate;
							rotate.rotateZ(-rotation);

							Matrix4 scale = baseScale;
							scale.scale(reverseArrowImageScale);

							Matrix4 translation;
							translation.translate(pos.x, pos.y, pos.z);

							if (m_fposu_3d_hitobjects_look_at_player_ref->getBool())
								modelMatrix = translation * Camera::buildMatrixLookAt(Vector3(0, 0, 0), pos - m_beatmap->getOsu()->getFPoSu()->getCamera()->getPos(), Vector3(0, 1, 0)).invert() * scale * rotate;
							else
								modelMatrix = translation * scale * rotate;
						}
						g->setWorldMatrixMul(modelMatrix);

						skin->getReverseArrow()->bind();
						{
							m_beatmap->getOsu()->getFPoSu()->getUVPlaneModel()->draw3D(g);
						}
						skin->getReverseArrow()->unbind();
					}
					g->popTransform();
				}
			}
		}
	}

	// slider body fade animation, draw start/end circle hit animation
	if (!m_fposu_3d_spheres_ref->getBool())
	{
		// TODO: slider body fade animation
		/*
		if (m_fEndSliderBodyFadeAnimation > 0.0f && m_fEndSliderBodyFadeAnimation != 1.0f && !m_beatmap->getOsu()->getModHD())
		{
			std::vector<Vector2> emptyVector;
			std::vector<Vector2> alwaysPoints;
			alwaysPoints.push_back(m_beatmap->osuCoords2Pixels(m_curve->pointAt(m_fSlidePercent)));
			if (!osu_slider_shrink.getBool())
				drawBody(g, 1.0f - m_fEndSliderBodyFadeAnimation, 0, 1);
			else if (osu_slider_body_lazer_fadeout_style.getBool())
				OsuSliderRenderer::draw(g, m_beatmap->getOsu(), emptyVector, alwaysPoints, m_beatmap->getHitcircleDiameter(), 0.0f, 0.0f, m_beatmap->getSkin()->getComboColorForCounter(m_iColorCounter, m_iColorOffset), 1.0f, 1.0f - m_fEndSliderBodyFadeAnimation, getTime());
		}
		*/

		if (m_fStartHitAnimation > 0.0f && m_fStartHitAnimation != 1.0f && !m_beatmap->getOsu()->getModHD())
		{
			float alpha = 1.0f - m_fStartHitAnimation;

			float scale = m_fStartHitAnimation;
			scale = -scale*(scale-2.0f); // quad out scale

			bool drawNumber = (skin->getVersion() > 1.0f ? false : true) && m_iCurRepeat < 1;

			Matrix4 baseScale;
			baseScale.scale(m_beatmap->getRawHitcircleDiameter() * OsuModFPoSu::SIZEDIV3D);
			baseScale.scale(m_beatmap->getOsu()->getFPoSu()->get3DPlayfieldScale());
			baseScale.scale((1.0f + scale*OsuGameRules::osu_circle_fade_out_scale.getFloat()));

			if (m_iCurRepeat < 1)
			{
				m_beatmap->getSkin()->getHitCircleOverlay2()->setAnimationTimeOffset(!m_beatmap->isInMafhamRenderChunk() ? m_iTime - m_iApproachTime : m_beatmap->getCurMusicPosWithOffsets());
				m_beatmap->getSkin()->getSliderStartCircleOverlay2()->setAnimationTimeOffset(!m_beatmap->isInMafhamRenderChunk() ? m_iTime - m_iApproachTime : m_beatmap->getCurMusicPosWithOffsets());

				OsuCircle::draw3DSliderStartCircle(g, m_beatmap, this, baseScale, m_curve->pointAt(0.0f), m_iComboNumber, m_iColorCounter, m_iColorOffset, 1.0f, 1.0f, alpha, alpha, drawNumber);
			}
			else
			{
				m_beatmap->getSkin()->getHitCircleOverlay2()->setAnimationTimeOffset(!m_beatmap->isInMafhamRenderChunk() ? m_iTime : m_beatmap->getCurMusicPosWithOffsets());
				m_beatmap->getSkin()->getSliderEndCircleOverlay2()->setAnimationTimeOffset(!m_beatmap->isInMafhamRenderChunk() ? m_iTime : m_beatmap->getCurMusicPosWithOffsets());

				OsuCircle::draw3DSliderEndCircle(g, m_beatmap, this, baseScale, m_curve->pointAt(0.0f), m_iComboNumber, m_iColorCounter, m_iColorOffset, 1.0f, 1.0f, alpha, alpha, drawNumber);
			}
		}

		if (m_fEndHitAnimation > 0.0f && m_fEndHitAnimation != 1.0f && !m_beatmap->getOsu()->getModHD())
		{
			float alpha = 1.0f - m_fEndHitAnimation;

			float scale = m_fEndHitAnimation;
			scale = -scale*(scale-2.0f); // quad out scale

			Matrix4 baseScale;
			baseScale.scale(m_beatmap->getRawHitcircleDiameter() * OsuModFPoSu::SIZEDIV3D);
			baseScale.scale(m_beatmap->getOsu()->getFPoSu()->get3DPlayfieldScale());
			baseScale.scale((1.0f + scale*OsuGameRules::osu_circle_fade_out_scale.getFloat()));

			m_beatmap->getSkin()->getHitCircleOverlay2()->setAnimationTimeOffset(!m_beatmap->isInMafhamRenderChunk() ? m_iTime - m_iFadeInTime : m_beatmap->getCurMusicPosWithOffsets());
			m_beatmap->getSkin()->getSliderEndCircleOverlay2()->setAnimationTimeOffset(!m_beatmap->isInMafhamRenderChunk() ? m_iTime - m_iFadeInTime : m_beatmap->getCurMusicPosWithOffsets());

			OsuCircle::draw3DSliderEndCircle(g, m_beatmap, this, baseScale, m_curve->pointAt(1.0f), m_iComboNumber, m_iColorCounter, m_iColorOffset, 1.0f, 1.0f, alpha, 0.0f, false);
		}
	}

	OsuHitObject::draw3D(g);
}

void OsuSlider::draw3D2(Graphics *g)
{
	OsuHitObject::draw3D2(g);

	OsuSkin *skin = m_beatmap->getSkin();

	// HACKHACK: so much code duplication aaaaaaah
	if (m_bVisible || (m_bStartFinished && !m_bFinished)) // extra possibility to avoid flicker between OsuHitObject::m_bVisible delay and the fadeout animation below this if block
	{
		if (m_points.size() > 1)
		{
			Matrix4 baseScale;
			baseScale.scale(m_beatmap->getRawHitcircleDiameter() * OsuModFPoSu::SIZEDIV3D);
			baseScale.scale(m_beatmap->getOsu()->getFPoSu()->get3DPlayfieldScale());

			// HACKHACK: very dirty code
			bool sliderRepeatStartCircleFinished = m_iRepeat < 2;
			//bool sliderRepeatEndCircleFinished = false;
			for (int i=0; i<m_clicks.size(); i++)
			{
				if (m_clicks[i].type == 0)
				{
					if (m_clicks[i].sliderend)
					{
						//sliderRepeatEndCircleFinished = m_clicks[i].finished;
					}
					else
						sliderRepeatStartCircleFinished = m_clicks[i].finished;
				}
			}

			// start circle
			if (!m_bStartFinished || !sliderRepeatStartCircleFinished || (!m_bEndFinished && m_iRepeat % 2 == 0))
			{
				OsuCircle::draw3DApproachCircle(g, m_beatmap, this, baseScale, m_curve->pointAt(0.0f), m_iComboNumber, m_iColorCounter, m_iColorOffset, m_fHittableDimRGBColorMultiplierPercent, m_fApproachScale, m_fAlphaForApproachCircle, m_bOverrideHDApproachCircle);
			}
		}
	}

	// draw followcircle
	// HACKHACK: this is not entirely correct (due to m_bHeldTillEnd, if held within 300 range but then released, will flash followcircle at the end)
	if ((m_bVisible && m_bCursorInside && (isClickHeldSlider() || m_beatmap->getOsu()->getModAuto() || m_beatmap->getOsu()->getModRelax())) || (m_bFinished && m_fFollowCircleAnimationAlpha > 0.0f && m_bHeldTillEnd))
	{
		Matrix4 baseScale;
		baseScale.scale(m_beatmap->getRawHitcircleDiameter() * OsuModFPoSu::SIZEDIV3D);
		baseScale.scale(m_beatmap->getOsu()->getFPoSu()->get3DPlayfieldScale());

		Vector3 point = m_beatmap->osuCoordsTo3D(m_vCurPointRaw, this);

		// HACKHACK: this is shit
		float tickAnimation = (m_fFollowCircleTickAnimationScale < 0.1f ? m_fFollowCircleTickAnimationScale/0.1f : (1.0f - m_fFollowCircleTickAnimationScale)/0.9f);
		if (m_fFollowCircleTickAnimationScale < 0.1f)
		{
			tickAnimation = -tickAnimation*(tickAnimation-2.0f);
			tickAnimation = clamp<float>(tickAnimation / 0.02f, 0.0f, 1.0f);
		}
		float tickAnimationScale = 1.0f + tickAnimation*OsuGameRules::osu_slider_followcircle_tick_pulse_scale.getFloat();

		g->setColor(0xffffffff);
		g->setAlpha(m_fFollowCircleAnimationAlpha);
		g->pushTransform();
		{
			skin->getSliderFollowCircle2()->setAnimationTimeOffset(m_iTime);

			Matrix4 modelMatrix;
			{
				Matrix4 scale = baseScale;
				scale.scale((m_beatmap->getRawSliderFollowCircleDiameter() / m_beatmap->getRawHitcircleDiameter()) * (skin->getSliderFollowCircle2()->getImageSizeForCurrentFrame().x / skin->getSliderFollowCircle2()->getSizeBaseRaw().x) * tickAnimationScale*m_fFollowCircleAnimationScale*0.85f);

				Matrix4 translation;
				translation.translate(point.x, point.y, point.z);

				if (m_fposu_3d_hitobjects_look_at_player_ref->getBool())
					modelMatrix = translation * Camera::buildMatrixLookAt(Vector3(0, 0, 0), point - m_beatmap->getOsu()->getFPoSu()->getCamera()->getPos(), Vector3(0, 1, 0)).invert() * scale;
				else
					modelMatrix = translation * scale;
			}
			g->setWorldMatrixMul(modelMatrix);

			skin->getSliderFollowCircle2()->getImageForCurrentFrame().img->bind();
			{
				m_beatmap->getOsu()->getFPoSu()->getUVPlaneModel()->draw3D(g);
			}
			skin->getSliderFollowCircle2()->getImageForCurrentFrame().img->unbind();
		}
		g->popTransform();
	}

	const bool isCompletelyFinished = m_bStartFinished && m_bEndFinished && m_bFinished;

	// draw sliderb on top of everything
	if ((m_bVisible || (m_bStartFinished && !m_bFinished)) && !isCompletelyFinished) // extra possibility in the if-block to avoid flicker between OsuHitObject::m_bVisible delay and the fadeout animation below this if-block
	{
		if (m_fSlidePercent > 0.0f)
		{
			Matrix4 baseScale;
			baseScale.scale(m_beatmap->getRawHitcircleDiameter() * OsuModFPoSu::SIZEDIV3D);
			baseScale.scale(m_beatmap->getOsu()->getFPoSu()->get3DPlayfieldScale());

			// draw sliderb
			Vector3 point = m_beatmap->osuCoordsTo3D(m_vCurPointRaw, this);
			Vector2 c1 = m_beatmap->osuCoords2Pixels(m_curve->pointAt(m_fSlidePercent + 0.01f <= 1.0f ? m_fSlidePercent : m_fSlidePercent - 0.01f));
			Vector2 c2 = m_beatmap->osuCoords2Pixels(m_curve->pointAt(m_fSlidePercent + 0.01f <= 1.0f ? m_fSlidePercent + 0.01f : m_fSlidePercent));
			float ballAngle = rad2deg( atan2(c2.y - c1.y, c2.x - c1.x) );
			if (skin->getSliderBallFlip())
				ballAngle += (m_iCurRepeat % 2 == 0) ? 0 : 180;

			Matrix4 rotate;
			rotate.rotateZ(-ballAngle);

			g->setColor(skin->getAllowSliderBallTint() ? (osu_slider_ball_tint_combo_color.getBool() ? skin->getComboColorForCounter(m_iColorCounter, m_iColorOffset) : skin->getSliderBallColor()) : 0xffffffff);
			g->pushTransform();
			{
				skin->getSliderb()->setAnimationTimeOffset(m_iTime);

				Matrix4 modelMatrix;
				{
					Matrix4 scale = baseScale;
					scale.scale(skin->getSliderb()->getImageSizeForCurrentFrame().x / skin->getSliderb()->getSizeBaseRaw().x);

					Matrix4 translation;
					translation.translate(point.x, point.y, point.z);

					if (m_fposu_3d_hitobjects_look_at_player_ref->getBool())
						modelMatrix = translation * Camera::buildMatrixLookAt(Vector3(0, 0, 0), point - m_beatmap->getOsu()->getFPoSu()->getCamera()->getPos(), Vector3(0, 1, 0)).invert() * scale * rotate;
					else
						modelMatrix = translation * scale * rotate;
				}
				g->setWorldMatrixMul(modelMatrix);

				skin->getSliderb()->getImageForCurrentFrame().img->bind();
				{
					m_beatmap->getOsu()->getFPoSu()->getUVPlaneModel()->draw3D(g);
				}
				skin->getSliderb()->getImageForCurrentFrame().img->unbind();
			}
			g->popTransform();
		}
	}
}

void OsuSlider::drawStartCircle(Graphics *g, float alpha)
{
	if (m_bStartFinished)
	{
		m_beatmap->getSkin()->getHitCircleOverlay2()->setAnimationTimeOffset(!m_beatmap->isInMafhamRenderChunk() ? m_iTime : m_beatmap->getCurMusicPosWithOffsets());
		m_beatmap->getSkin()->getSliderEndCircleOverlay2()->setAnimationTimeOffset(!m_beatmap->isInMafhamRenderChunk() ? m_iTime : m_beatmap->getCurMusicPosWithOffsets());

		OsuCircle::drawSliderEndCircle(g, m_beatmap, m_curve->pointAt(0.0f), m_iComboNumber, m_iColorCounter, m_iColorOffset, m_fHittableDimRGBColorMultiplierPercent, 1.0f, m_fAlpha, 0.0f, false, false);
	}
	else
	{
		m_beatmap->getSkin()->getHitCircleOverlay2()->setAnimationTimeOffset(!m_beatmap->isInMafhamRenderChunk() ? m_iTime - m_iApproachTime : m_beatmap->getCurMusicPosWithOffsets());
		m_beatmap->getSkin()->getSliderStartCircleOverlay2()->setAnimationTimeOffset(!m_beatmap->isInMafhamRenderChunk() ? m_iTime - m_iApproachTime : m_beatmap->getCurMusicPosWithOffsets());

		OsuCircle::drawSliderStartCircle(g, m_beatmap, m_curve->pointAt(0.0f), m_iComboNumber, m_iColorCounter, m_iColorOffset, m_fHittableDimRGBColorMultiplierPercent, m_fApproachScale, m_fAlpha, m_fAlpha, !m_bHideNumberAfterFirstRepeatHit, m_bOverrideHDApproachCircle);
	}
}

void OsuSlider::draw3DStartCircle(Graphics *g, const Matrix4 &baseScale, float alpha)
{
	if (m_bStartFinished)
	{
		m_beatmap->getSkin()->getHitCircleOverlay2()->setAnimationTimeOffset(!m_beatmap->isInMafhamRenderChunk() ? m_iTime : m_beatmap->getCurMusicPosWithOffsets());
		m_beatmap->getSkin()->getSliderEndCircleOverlay2()->setAnimationTimeOffset(!m_beatmap->isInMafhamRenderChunk() ? m_iTime : m_beatmap->getCurMusicPosWithOffsets());

		OsuCircle::draw3DSliderEndCircle(g, m_beatmap, this, baseScale, m_curve->pointAt(0.0f), m_iComboNumber, m_iColorCounter, m_iColorOffset, m_fHittableDimRGBColorMultiplierPercent, 1.0f, m_fAlpha, 0.0f, false, false);
	}
	else
	{
		m_beatmap->getSkin()->getHitCircleOverlay2()->setAnimationTimeOffset(!m_beatmap->isInMafhamRenderChunk() ? m_iTime - m_iApproachTime : m_beatmap->getCurMusicPosWithOffsets());
		m_beatmap->getSkin()->getSliderStartCircleOverlay2()->setAnimationTimeOffset(!m_beatmap->isInMafhamRenderChunk() ? m_iTime - m_iApproachTime : m_beatmap->getCurMusicPosWithOffsets());

		OsuCircle::draw3DSliderStartCircle(g, m_beatmap, this, baseScale, m_curve->pointAt(0.0f), m_iComboNumber, m_iColorCounter, m_iColorOffset, m_fHittableDimRGBColorMultiplierPercent, m_fApproachScale, m_fAlpha, m_fAlpha, !m_bHideNumberAfterFirstRepeatHit, m_bOverrideHDApproachCircle);
	}
}

void OsuSlider::drawEndCircle(Graphics *g, float alpha, float sliderSnake)
{
	m_beatmap->getSkin()->getHitCircleOverlay2()->setAnimationTimeOffset(!m_beatmap->isInMafhamRenderChunk() ? m_iTime - m_iFadeInTime : m_beatmap->getCurMusicPosWithOffsets());
	m_beatmap->getSkin()->getSliderEndCircleOverlay2()->setAnimationTimeOffset(!m_beatmap->isInMafhamRenderChunk() ? m_iTime - m_iFadeInTime : m_beatmap->getCurMusicPosWithOffsets());

	OsuCircle::drawSliderEndCircle(g, m_beatmap, m_curve->pointAt(sliderSnake), m_iComboNumber, m_iColorCounter, m_iColorOffset, m_fHittableDimRGBColorMultiplierPercent, 1.0f, m_fAlpha, 0.0f, false, false);
}

void OsuSlider::draw3DEndCircle(Graphics *g, const Matrix4 &baseScale, float alpha, float sliderSnake)
{
	m_beatmap->getSkin()->getHitCircleOverlay2()->setAnimationTimeOffset(!m_beatmap->isInMafhamRenderChunk() ? m_iTime - m_iFadeInTime : m_beatmap->getCurMusicPosWithOffsets());
	m_beatmap->getSkin()->getSliderEndCircleOverlay2()->setAnimationTimeOffset(!m_beatmap->isInMafhamRenderChunk() ? m_iTime - m_iFadeInTime : m_beatmap->getCurMusicPosWithOffsets());

	OsuCircle::draw3DSliderEndCircle(g, m_beatmap, this, baseScale, m_curve->pointAt(sliderSnake), m_iComboNumber, m_iColorCounter, m_iColorOffset, m_fHittableDimRGBColorMultiplierPercent, 1.0f, m_fAlpha, 0.0f, false, false);
}

void OsuSlider::drawBody(Graphics *g, float alpha, float from, float to)
{
	// smooth begin/end while snaking/shrinking
	std::vector<Vector2> alwaysPoints;
	if (osu_slider_body_smoothsnake.getBool())
	{
		if (osu_slider_shrink.getBool() && m_fSliderSnakePercent > 0.999f)
		{
			alwaysPoints.push_back(m_beatmap->osuCoords2Pixels(m_curve->pointAt(m_fSlidePercent))); // curpoint
			alwaysPoints.push_back(m_beatmap->osuCoords2Pixels(getRawPosAt(m_iTime + m_iObjectDuration + 1))); // endpoint (because setDrawPercent() causes the last circle mesh to become invisible too quickly)
		}
		if (osu_snaking_sliders.getBool() && m_fSliderSnakePercent < 1.0f)
			alwaysPoints.push_back(m_beatmap->osuCoords2Pixels(m_curve->pointAt(m_fSliderSnakePercent))); // snakeoutpoint (only while snaking out)
	}

	const Color undimmedComboColor = m_beatmap->getSkin()->getComboColorForCounter(m_iColorCounter, m_iColorOffset);

	if (m_beatmap->getOsu()->shouldFallBackToLegacySliderRenderer())
	{
		std::vector<Vector2> screenPoints = m_curve->getPoints();
		for (int p=0; p<screenPoints.size(); p++)
		{
			screenPoints[p] = m_beatmap->osuCoords2Pixels(screenPoints[p]);
		}

		// peppy sliders
		OsuSliderRenderer::draw(g, m_beatmap->getOsu(), screenPoints, alwaysPoints, m_beatmap->getHitcircleDiameter(), from, to, undimmedComboColor, m_fHittableDimRGBColorMultiplierPercent, alpha, getTime());

		// mm sliders
		///OsuSliderRenderer::drawMM(g, m_beatmap->getOsu(), screenPoints, m_beatmap->getHitcircleDiameter(), from, to, m_beatmap->getSkin()->getComboColorForCounter(m_iColorCounter, m_iColorOffset), m_fHittableDimRGBColorMultiplierPercent, alpha, getTime());
	}
	else
	{
		// vertex buffered sliders
		// as the base mesh is centered at (0, 0, 0) and in raw osu coordinates, we have to scale and translate it to make it fit the actual desktop playfield
		const float scale = OsuGameRules::getPlayfieldScaleFactor(m_beatmap->getOsu());
		Vector2 translation = OsuGameRules::getPlayfieldCenter(m_beatmap->getOsu());

		if (m_beatmap->hasFailed())
			translation = m_beatmap->osuCoords2Pixels(Vector2(OsuGameRules::OSU_COORD_WIDTH/2, OsuGameRules::OSU_COORD_HEIGHT/2));

		if (m_osu_mod_fps_ref->getBool())
			translation += m_beatmap->getFirstPersonCursorDelta();

		Vector2 minBounds = m_beatmap->legacyPixels2RawPixels(m_beatmap->osuCoords2LegacyPixels(Vector2(m_curve->getBounds().x, m_curve->getBounds().y)));
		Vector2 maxBounds = m_beatmap->legacyPixels2RawPixels(m_beatmap->osuCoords2LegacyPixels(Vector2(m_curve->getBounds().z, m_curve->getBounds().w)));

		if (minBounds.x > maxBounds.x)
			std::swap(minBounds.x, maxBounds.x);
		if (minBounds.y > maxBounds.y)
			std::swap(minBounds.y, maxBounds.y);

		minBounds += translation;
		maxBounds += translation;

		OsuSliderRenderer::draw(g, m_beatmap->getOsu(), m_vao, Vector4(minBounds.x, minBounds.y, maxBounds.x, maxBounds.y), alwaysPoints, translation, scale, m_beatmap->getHitcircleDiameter(), from, to, undimmedComboColor, m_fHittableDimRGBColorMultiplierPercent, alpha, getTime());
	}
}

void OsuSlider::drawBodyVR(Graphics *g, OsuVR *vr, Matrix4 &mvp, float alpha, float from, float to)
{
	// HACKHACK: code duplication aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa

	Color comboColor = m_beatmap->getSkin()->getComboColorForCounter(m_iColorCounter, m_iColorOffset);
	comboColor = COLOR(255, (int)(COLOR_GET_Ri(comboColor)*m_fHittableDimRGBColorMultiplierPercent), (int)(COLOR_GET_Gi(comboColor)*m_fHittableDimRGBColorMultiplierPercent), (int)(COLOR_GET_Bi(comboColor)*m_fHittableDimRGBColorMultiplierPercent));

	// smooth begin/end while snaking/shrinking
	std::vector<Vector2> alwaysPoints;
	if (osu_slider_body_smoothsnake.getBool())
	{
		if (osu_slider_shrink.getBool() && m_fSliderSnakePercent > 0.999f)
		{
			alwaysPoints.push_back(m_beatmap->osuCoords2Pixels(m_curve->pointAt(m_fSlidePercent))); // curpoint
			alwaysPoints.push_back(m_beatmap->osuCoords2Pixels(getRawPosAt(m_iTime + m_iObjectDuration + 1))); // endpoint (because setDrawPercent() causes the last circle mesh to become invisible too quickly)
		}
		if (osu_snaking_sliders.getBool() && m_fSliderSnakePercent < 1.0f)
			alwaysPoints.push_back(m_beatmap->osuCoords2Pixels(m_curve->pointAt(m_fSliderSnakePercent))); // snakeoutpoint (only while snaking out)
	}

	if (m_beatmap->getOsu()->shouldFallBackToLegacySliderRenderer())
	{
		// peppy sliders
		std::vector<Vector2> screenPoints = m_curve->getPoints();
		for (int p=0; p<screenPoints.size(); p++)
		{
			screenPoints[p] = m_beatmap->osuCoords2Pixels(screenPoints[p]);
		}
		OsuSliderRenderer::drawVR(g, m_beatmap->getOsu(), vr, mvp, m_fApproachScale, screenPoints, alwaysPoints, m_beatmap->getHitcircleDiameter(), from, to, comboColor, m_fHittableDimRGBColorMultiplierPercent, alpha, getTime());
	}
	else
	{
		// vertex buffered sliders
		OsuSliderRenderer::drawVR(g, m_beatmap->getOsu(), vr, mvp, m_fApproachScale, m_vao, m_vaoVR2, alwaysPoints, m_beatmap->getHitcircleDiameter(), from, to, comboColor, m_fHittableDimRGBColorMultiplierPercent, alpha, getTime());
	}
}

void OsuSlider::update(long curPos)
{
	OsuHitObject::update(curPos);

	if (m_fSliderBreakRapeTime != 0.0f && engine->getTime() > m_fSliderBreakRapeTime)
	{
		m_fSliderBreakRapeTime = 0.0f;
		m_epilepsy_ref->setValue(0.0f);
	}

	// stop slide sound while paused
	if (m_beatmap->isPaused() || !m_beatmap->isPlaying() || m_beatmap->hasFailed())
		m_beatmap->getSkin()->stopSliderSlideSound();

	// animations must be updated even if we are finished
	updateAnimations(curPos);

	// all further calculations are only done while we are active
	if (m_bFinished) return;

	// slider slide percent
	m_fSlidePercent = 0.0f;
	if (curPos > m_iTime)
		m_fSlidePercent = clamp<float>(clamp<long>((curPos - (m_iTime)), 0, (long)m_fSliderTime) / m_fSliderTime, 0.0f, 1.0f);

	m_fActualSlidePercent = m_fSlidePercent;

	const float sliderSnakeDuration = (1.0f / 3.0f)*m_iApproachTime * osu_slider_snake_duration_multiplier.getFloat();
	m_fSliderSnakePercent = std::min(1.0f, (curPos - (m_iTime - m_iApproachTime)) / (sliderSnakeDuration));

	const long reverseArrowFadeInStart = m_iTime - (osu_snaking_sliders.getBool() ? (m_iApproachTime - sliderSnakeDuration) : m_iApproachTime);
	const long reverseArrowFadeInEnd = reverseArrowFadeInStart + osu_slider_reverse_arrow_fadein_duration.getInt();
	m_fReverseArrowAlpha = 1.0f - clamp<float>(((float)(reverseArrowFadeInEnd - curPos) / (float)(reverseArrowFadeInEnd - reverseArrowFadeInStart)), 0.0f, 1.0f);
	m_fReverseArrowAlpha *= osu_slider_reverse_arrow_alpha_multiplier.getFloat();

	m_fBodyAlpha = m_fAlpha;
	if (m_beatmap->getOsu()->getModHD()) // hidden modifies the body alpha
	{
		m_fBodyAlpha = m_fAlphaWithoutHidden; // fade in as usual

		// fade out over the duration of the slider, starting exactly when the default fadein finishes
		const long hiddenSliderBodyFadeOutStart = std::min(m_iTime, m_iTime - m_iApproachTime + m_iFadeInTime); // min() ensures that the fade always starts at m_iTime (even if the fadeintime is longer than the approachtime)
		const long hiddenSliderBodyFadeOutEnd = m_iTime + (long)(osu_mod_hd_slider_fade_percent.getFloat()*m_fSliderTime);
		if (curPos >= hiddenSliderBodyFadeOutStart)
		{
			m_fBodyAlpha = clamp<float>(((float)(hiddenSliderBodyFadeOutEnd - curPos) / (float)(hiddenSliderBodyFadeOutEnd - hiddenSliderBodyFadeOutStart)), 0.0f, 1.0f);
			m_fBodyAlpha *= m_fBodyAlpha; // quad in body fadeout
		}
	}

	// if this slider is active, recalculate sliding/curve position and general state
	if (m_fSlidePercent > 0.0f || m_bVisible)
	{
		// handle reverse sliders
		m_bInReverse = false;
		m_bHideNumberAfterFirstRepeatHit = false;
		if (m_iRepeat > 1)
		{
			if (m_fSlidePercent > 0.0f && m_bStartFinished)
				m_bHideNumberAfterFirstRepeatHit = true;

			float part = 1.0f / (float)m_iRepeat;
			m_iCurRepeat = (int)(m_fSlidePercent*m_iRepeat);
			float baseSlidePercent = part*m_iCurRepeat;
			float partSlidePercent = (m_fSlidePercent - baseSlidePercent) / part;
			if (m_iCurRepeat % 2 == 0)
			{
				m_fSlidePercent = partSlidePercent;
				m_iReverseArrowPos = 2;
			}
			else
			{
				m_fSlidePercent = 1.0f - partSlidePercent;
				m_iReverseArrowPos = 1;
				m_bInReverse = true;
			}

			// no reverse arrow on the last repeat
			if (m_iCurRepeat == m_iRepeat-1)
				m_iReverseArrowPos = 0;

			// osu style: immediately show all coming reverse arrows (even on the circle we just started from)
			if (m_iCurRepeat < m_iRepeat-2 && m_fSlidePercent > 0.0f && m_iRepeat > 2)
				m_iReverseArrowPos = 3;
		}

		m_vCurPointRaw = m_curve->pointAt(m_fSlidePercent);
		m_vCurPoint = m_beatmap->osuCoords2Pixels(m_vCurPointRaw);
	}
	else
	{
		m_vCurPointRaw = m_curve->pointAt(0.0f);
		m_vCurPoint = m_beatmap->osuCoords2Pixels(m_vCurPointRaw);
	}

	// reset sliding key (opposite), see isClickHeldSlider()
	if (m_iDownKey > 0)
	{
		if ((m_iDownKey == 2 && !m_beatmap->isKey1Down()) || (m_iDownKey == 1 && !m_beatmap->isKey2Down())) // opposite key!
			m_iDownKey = 0;
	}

	// handle dynamic followradius
	float followRadius = m_bCursorLeft ? m_beatmap->getHitcircleDiameter() / 2.0f : m_beatmap->getSliderFollowCircleDiameter() / 2.0f;
	const bool isBeatmapCursorInside = ((m_beatmap->getCursorPos() - m_vCurPoint).length() < followRadius);
	const bool isAutoCursorInside = (m_beatmap->getOsu()->getModAuto() && (!m_osu_auto_cursordance_ref->getBool() || ((m_beatmap->getCursorPos() - m_vCurPoint).length() < followRadius)));
	m_bCursorInside = (isAutoCursorInside || isBeatmapCursorInside);
	m_bCursorLeft = !m_bCursorInside;

	if (m_beatmap->getOsu()->isInVRMode())
	{
		// if we are in vr mode, and the user has disabled the desktop playfield, then the desktop cursor should be ignored
		if (!m_osu_vr_draw_desktop_playfield->getBool())
			m_bCursorInside = isAutoCursorInside;

		// vr always uses the default (raw!) followradius (don't have to go within circle radius again after leaving the slider, would be too hard otherwise)
		followRadius = m_beatmap->getRawSliderFollowCircleDiameter()/2.0f;
		float vrCursor1Delta = (m_beatmap->getOsu()->getVR()->getCursorPos1() - m_beatmap->osuCoords2VRPixels(m_vCurPointRaw)).length();
		float vrCursor2Delta = (m_beatmap->getOsu()->getVR()->getCursorPos2() - m_beatmap->osuCoords2VRPixels(m_vCurPointRaw)).length();
		bool vrCursor1Inside = vrCursor1Delta < followRadius;
		bool vrCursor2Inside = vrCursor2Delta < followRadius;
		m_bCursorInside = m_bCursorInside || (vrCursor1Inside || vrCursor2Inside);

		// calculate which controller to do haptic feedback on (for reverse arrows and sliderendcircle)

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

	// handle slider start
	if (!m_bStartFinished)
	{
		if (m_beatmap->getOsu()->getModAuto())
		{
			if (curPos >= m_iTime)
				onHit(OsuScore::HIT::HIT_300, 0, false);
		}
		else
		{
			long delta = curPos - m_iTime;

			if (m_beatmap->getOsu()->getModRelax() || m_beatmap->getOsu()->isInVRMode())
			{
				if (curPos >= m_iTime + (long)m_osu_relax_offset_ref->getInt() && !m_beatmap->isPaused() && !m_beatmap->isContinueScheduled())
				{
					const Vector2 pos = m_beatmap->osuCoords2Pixels(m_curve->pointAt(0.0f));
					const float cursorDelta = (m_beatmap->getCursorPos() - pos).length();

					float vrCursor1Delta = 0.0f;
					float vrCursor2Delta = 0.0f;
					bool vrCursor1Inside = false;
					bool vrCursor2Inside = false;
					if (m_beatmap->getOsu()->isInVRMode())
					{
						vrCursor1Delta = (m_beatmap->getOsu()->getVR()->getCursorPos1() - m_beatmap->osuCoords2VRPixels(m_curve->pointAt(0.0f))).length();
						vrCursor2Delta = (m_beatmap->getOsu()->getVR()->getCursorPos2() - m_beatmap->osuCoords2VRPixels(m_curve->pointAt(0.0f))).length();
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
								// calculate again which controller to do haptic feedback on (since startcircle radius != followcircle radius)

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

							m_startResult = result;
							onHit(m_startResult, delta, false, targetDelta, targetAngle);
						}
					}
				}
			}

			// wait for a miss
			if (delta >= 0)
			{
				// if this is a miss after waiting
				if (delta > (long)OsuGameRules::getHitWindow50(m_beatmap))
				{
					m_startResult = OsuScore::HIT::HIT_MISS;
					onHit(m_startResult, delta, false);
				}
			}
		}
	}

	// handle slider end, repeats, ticks, and constant VR controller vibration while sliding
	if (!m_bEndFinished)
	{
		// NOTE: we have 2 timing conditions after which we start checking for strict tracking: 1) startcircle was clicked, 2) slider has started timing wise
		// it is easily possible to hit the startcircle way before the sliderball would become active, which is why the first check exists.
		// even if the sliderball has not yet started sliding, you will be punished for leaving the (still invisible) followcircle area after having clicked the startcircle, always.
		const bool isTrackingStrictTrackingMod = ((m_bStartFinished || curPos >= m_iTime) && m_osu_mod_strict_tracking_ref->getBool());

		// slider tail lenience bullshit: see https://github.com/ppy/osu/blob/master/osu.Game.Rulesets.Osu/Objects/Slider.cs#L123
		// being "inside the slider" (for the end of the slider) is NOT checked at the exact end of the slider, but somewhere random before, because fuck you
		const long lenienceHackEndTime = std::max(m_iTime + m_iObjectDuration / 2, (m_iTime + m_iObjectDuration) - (long)osu_slider_end_inside_check_offset.getInt());
		const bool isTrackingCorrectly = (isClickHeldSlider() || m_beatmap->getOsu()->getModRelax()) && m_bCursorInside;
		if (isTrackingCorrectly)
		{
			if (isTrackingStrictTrackingMod)
			{
				m_iStrictTrackingModLastClickHeldTime = curPos;
				if (m_iStrictTrackingModLastClickHeldTime == 0) // (prevent frame perfect inputs from not triggering the strict tracking miss because we use != 0 comparison to detect if tracking correctly at least once)
					m_iStrictTrackingModLastClickHeldTime = 1;
			}

			// only check it at the exact point in time ...
			if (curPos >= lenienceHackEndTime)
			{
				// ... once (like a tick)
				if (!m_bHeldTillEndForLenienceHackCheck)
				{
					m_bHeldTillEndForLenienceHackCheck = true;
					m_bHeldTillEndForLenienceHack = true; // player was correctly clicking/holding inside slider at lenienceHackEndTime
				}
			}
		}
		else
			m_bCursorLeft = true; // do not allow empty clicks outside of the circle radius to prevent the m_bCursorInside flag from resetting

		// can't be "inside the slider" after lenienceHackEndTime (even though the slider is still going, which is madness)
		if (curPos >= lenienceHackEndTime)
			m_bHeldTillEndForLenienceHackCheck = true;

		// handle strict tracking mod
		if (isTrackingStrictTrackingMod)
		{
			const bool wasTrackingCorrectlyAtLeastOnce = (m_iStrictTrackingModLastClickHeldTime != 0);
			if (wasTrackingCorrectlyAtLeastOnce && !isTrackingCorrectly)
			{
				if (!m_bHeldTillEndForLenienceHack) // if past lenience end time then don't trigger a strict tracking miss, since the slider is then already considered fully finished gameplay wise
				{
					if (m_endResult == OsuScore::HIT::HIT_NULL) // force miss the end once, if it has not already been force missed by notelock
					{
						// force miss endcircle
						{
							onSliderBreak();

							m_bHeldTillEnd = false;
							m_bHeldTillEndForLenienceHack = false;
							m_bHeldTillEndForLenienceHackCheck = true;
							m_endResult = OsuScore::HIT::HIT_MISS;

							addHitResult(m_endResult, 0, m_bIsEndOfCombo, getRawPosAt(m_iTime + m_iObjectDuration), -1.0f, 0.0f, true, true, false); // end of combo, ignore in hiterrorbar, ignore combo, subtract health
						}
					}
				}
			}
		}
	
		// handle repeats and ticks
		for (int i=0; i<m_clicks.size(); i++)
		{
			if (!m_clicks[i].finished && curPos >= m_clicks[i].time)
			{
				m_clicks[i].finished = true;
				m_clicks[i].successful = (isClickHeldSlider() && m_bCursorInside) || m_beatmap->getOsu()->getModAuto() || (m_beatmap->getOsu()->getModRelax() && m_bCursorInside);

				if (m_clicks[i].type == 0)
					onRepeatHit(m_clicks[i].successful, m_clicks[i].sliderend);
				else
					onTickHit(m_clicks[i].successful, m_clicks[i].tickIndex);
			}
		}

		// handle auto, and the last circle
		if (m_beatmap->getOsu()->getModAuto())
		{
			if (curPos >= m_iTime + m_iObjectDuration)
			{
				m_bHeldTillEnd = true;
				onHit(OsuScore::HIT::HIT_300, 0, true);
			}
		}
		else
		{
			if (curPos >= m_iTime + m_iObjectDuration)
			{
				// handle leftover startcircle
				{
					// this may happen (if the slider time is shorter than the miss window of the startcircle)
					if (m_startResult == OsuScore::HIT::HIT_NULL)
					{
						// we still want to cause a sliderbreak in this case!
						onSliderBreak();

						// special case: missing the startcircle drains HIT_MISS_SLIDERBREAK health (and not HIT_MISS health)
						m_beatmap->addHitResult(this, OsuScore::HIT::HIT_MISS_SLIDERBREAK, 0, false, true, true, true, true, false); // only decrease health

						m_startResult = OsuScore::HIT::HIT_MISS;
					}
				}

				// handle endcircle
				bool isEndResultComingFromStrictTrackingMod = false;
				if (m_endResult == OsuScore::HIT::HIT_NULL)
				{
					m_bHeldTillEnd = m_bHeldTillEndForLenienceHack;
					m_endResult = m_bHeldTillEnd ? OsuScore::HIT::HIT_300 : OsuScore::HIT::HIT_MISS;

					// handle total slider result (currently startcircle + repeats + ticks + endcircle)
					// clicks = (repeats + ticks)
					const float numMaxPossibleHits = 1 + m_clicks.size() + 1;
					float numActualHits = 0;

					if (m_startResult != OsuScore::HIT::HIT_MISS)
						numActualHits++;
					if (m_endResult != OsuScore::HIT::HIT_MISS)
						numActualHits++;

					for (int i=0; i<m_clicks.size(); i++)
					{
						if (m_clicks[i].successful)
							numActualHits++;
					}

					const float percent = numActualHits / numMaxPossibleHits;

					const bool allow300 = (osu_slider_scorev2.getBool() || m_beatmap->getOsu()->getModScorev2()) ? (m_startResult == OsuScore::HIT::HIT_300) : true;
					const bool allow100 = (osu_slider_scorev2.getBool() || m_beatmap->getOsu()->getModScorev2()) ? (m_startResult == OsuScore::HIT::HIT_300 || m_startResult == OsuScore::HIT::HIT_100) : true;

					// rewrite m_endResult as the whole slider result, then use it for the final onHit()
					if (percent >= 0.999f && allow300)
						m_endResult = OsuScore::HIT::HIT_300;
					else if (percent >= 0.5f && allow100 && !OsuGameRules::osu_mod_ming3012.getBool() && !OsuGameRules::osu_mod_no100s.getBool())
						m_endResult = OsuScore::HIT::HIT_100;
					else if (percent > 0.0f && !OsuGameRules::osu_mod_no100s.getBool() && !OsuGameRules::osu_mod_no50s.getBool())
						m_endResult = OsuScore::HIT::HIT_50;
					else
						m_endResult = OsuScore::HIT::HIT_MISS;

					//debugLog("percent = %f\n", percent);

					if (!m_bHeldTillEnd && osu_slider_end_miss_breaks_combo.getBool())
						onSliderBreak();
				}
				else
					isEndResultComingFromStrictTrackingMod = true;

				onHit(m_endResult, 0, true, 0.0f, 0.0f, isEndResultComingFromStrictTrackingMod);
			}
		}

		// handle VR controller constant sliding vibration
		if (m_beatmap->getOsu()->isInVRMode())
		{
			if (m_bStartFinished && !m_bEndFinished && m_bCursorInside
				&& !m_beatmap->isPaused() && !m_beatmap->isWaiting() && m_beatmap->isPlaying())
			{
				// clicks have priority over the constant sliding vibration, that's why this is at the bottom here AFTER the other function above have had a chance to call triggerHapticPulse()
				// while sliding the slider, vibrate the controller constantly
				if (m_bOnHitVRLeftControllerHapticFeedback)
					openvr->getLeftController()->triggerHapticPulse(m_beatmap->getOsu()->getVR()->getSliderHapticPulseStrength());
				else
					openvr->getRightController()->triggerHapticPulse(m_beatmap->getOsu()->getVR()->getSliderHapticPulseStrength());
			}
		}

		// handle sliderslide sound
		if (m_bStartFinished && !m_bEndFinished && m_bCursorInside && m_iDelta <= 0
			&& (isClickHeldSlider() || m_beatmap->getOsu()->getModAuto() || m_beatmap->getOsu()->getModRelax())
			&& !m_beatmap->isPaused() && !m_beatmap->isWaiting() && m_beatmap->isPlaying())
		{
			const Vector2 osuCoords = m_beatmap->pixels2OsuCoords(m_beatmap->osuCoords2Pixels(m_vCurPointRaw));

			m_beatmap->getSkin()->playSliderSlideSound(OsuGameRules::osuCoords2Pan(osuCoords.x));
			m_iPrevSliderSlideSoundSampleSet = m_beatmap->getSkin()->getSampleSet();
		}
		else
		{
			m_beatmap->getSkin()->stopSliderSlideSound(m_iPrevSliderSlideSoundSampleSet);
			m_iPrevSliderSlideSoundSampleSet = -1;
		}
	}
}

void OsuSlider::updateAnimations(long curPos)
{
	// handle followcircle animations
	m_fFollowCircleAnimationAlpha = clamp<float>((float)((curPos - m_iTime)) / 1000.0f / clamp<float>(OsuGameRules::osu_slider_followcircle_fadein_fade_time.getFloat(), 0.0f, m_iObjectDuration/1000.0f), 0.0f, 1.0f);
	if (m_bFinished)
	{
		m_fFollowCircleAnimationAlpha = 1.0f - clamp<float>((float)((curPos - (m_iTime+m_iObjectDuration))) / 1000.0f / OsuGameRules::osu_slider_followcircle_fadeout_fade_time.getFloat(), 0.0f, 1.0f);
		m_fFollowCircleAnimationAlpha *= m_fFollowCircleAnimationAlpha; // quad in
	}

	m_fFollowCircleAnimationScale = clamp<float>((float)((curPos - m_iTime)) / 1000.0f / clamp<float>(OsuGameRules::osu_slider_followcircle_fadein_scale_time.getFloat(), 0.0f, m_iObjectDuration/1000.0f), 0.0f, 1.0f);
	if (m_bFinished)
	{
		m_fFollowCircleAnimationScale = clamp<float>((float)((curPos - (m_iTime+m_iObjectDuration))) / 1000.0f / OsuGameRules::osu_slider_followcircle_fadeout_scale_time.getFloat(), 0.0f, 1.0f);
	}
	m_fFollowCircleAnimationScale = -m_fFollowCircleAnimationScale*(m_fFollowCircleAnimationScale-2.0f); // quad out

	if (!m_bFinished)
		m_fFollowCircleAnimationScale = OsuGameRules::osu_slider_followcircle_fadein_scale.getFloat() + (1.0f - OsuGameRules::osu_slider_followcircle_fadein_scale.getFloat())*m_fFollowCircleAnimationScale;
	else
		m_fFollowCircleAnimationScale = 1.0f - (1.0f - OsuGameRules::osu_slider_followcircle_fadeout_scale.getFloat())*m_fFollowCircleAnimationScale;
}

void OsuSlider::updateStackPosition(float stackOffset)
{
	if (m_curve != NULL)
		m_curve->updateStackPosition(m_iStack * stackOffset, m_beatmap->getOsu()->getModHR());
}

void OsuSlider::miss(long curPos)
{
	if (m_bFinished) return;

	const long delta = curPos - m_iTime;

	// startcircle
	if (!m_bStartFinished)
	{
		m_startResult = OsuScore::HIT::HIT_MISS;
		onHit(m_startResult, delta, false);
	}

	// endcircle, repeats, ticks
	if (!m_bEndFinished)
	{
		// repeats, ticks
		{
			for (int i=0; i<m_clicks.size(); i++)
			{
				if (!m_clicks[i].finished)
				{
					m_clicks[i].finished = true;
					m_clicks[i].successful = false;

					if (m_clicks[i].type == 0)
						onRepeatHit(m_clicks[i].successful, m_clicks[i].sliderend);
					else
						onTickHit(m_clicks[i].successful, m_clicks[i].tickIndex);
				}
			}
		}

		// endcircle
		{
			m_bHeldTillEnd = m_bHeldTillEndForLenienceHack;

			if (!m_bHeldTillEnd && osu_slider_end_miss_breaks_combo.getBool())
				onSliderBreak();

			m_endResult = OsuScore::HIT::HIT_MISS;
			onHit(m_endResult, 0, true);
		}
	}
}

Vector2 OsuSlider::getRawPosAt(long pos)
{
	if (m_curve == NULL) return Vector2(0, 0);

	if (pos <= m_iTime)
		return m_curve->pointAt(0.0f);
	else if (pos >= m_iTime + m_fSliderTime)
	{
		if (m_iRepeat % 2 == 0)
			return m_curve->pointAt(0.0f);
		else
			return m_curve->pointAt(1.0f);
	}
	else
		return m_curve->pointAt(getT(pos, false));
}

Vector2 OsuSlider::getOriginalRawPosAt(long pos)
{
	if (m_curve == NULL) return Vector2(0, 0);

	if (pos <= m_iTime)
		return m_curve->originalPointAt(0.0f);
	else if (pos >= m_iTime + m_fSliderTime)
	{
		if (m_iRepeat % 2 == 0)
			return m_curve->originalPointAt(0.0f);
		else
			return m_curve->originalPointAt(1.0f);
	}
	else
		return m_curve->originalPointAt(getT(pos, false));
}

float OsuSlider::getT(long pos, bool raw)
{
	float t = (float)((long)pos - (long)m_iTime) / m_fSliderTimeWithoutRepeats;
	if (raw)
		return t;
	else
	{
		float floorVal = (float) std::floor(t);
		return ((int)floorVal % 2 == 0) ? t - floorVal : floorVal + 1 - t;
	}
}

void OsuSlider::onClickEvent(std::vector<OsuBeatmap::CLICK> &clicks)
{
	if (m_points.size() == 0 || m_bBlocked) return; // also handle note blocking here (doesn't need fancy shake logic, since sliders don't shake in osu!stable)

	if (!m_bStartFinished)
	{
		const Vector2 cursorPos = m_beatmap->getCursorPos();

		const Vector2 pos = m_beatmap->osuCoords2Pixels(m_curve->pointAt(0.0f));
		const float cursorDelta = (cursorPos - pos).length();

		if (cursorDelta < m_beatmap->getHitcircleDiameter()/2.0f)
		{
			const long delta = (long)clicks[0].musicPos - (long)m_iTime;

			OsuScore::HIT result = OsuGameRules::getHitResult(delta, m_beatmap);
			if (result != OsuScore::HIT::HIT_NULL)
			{
				const float targetDelta = cursorDelta / (m_beatmap->getHitcircleDiameter()/2.0f);
				const float targetAngle = rad2deg(atan2(cursorPos.y - pos.y, cursorPos.x - pos.x));

				clicks.erase(clicks.begin());
				m_startResult = result;
				onHit(m_startResult, delta, false, targetDelta, targetAngle);
			}
		}
	}
}

void OsuSlider::onHit(OsuScore::HIT result, long delta, bool startOrEnd, float targetDelta, float targetAngle, bool isEndResultFromStrictTrackingMod)
{
	if (m_points.size() == 0) return;

	// start + end of a slider add +30 points, if successful

	//debugLog("startOrEnd = %i,    m_iCurRepeat = %i\n", (int)startOrEnd, m_iCurRepeat);

	// sound and hit animation and also sliderbreak combo drop
	{
		if (result == OsuScore::HIT::HIT_MISS)
		{
			if (!isEndResultFromStrictTrackingMod)
				onSliderBreak();
		}
		else
		{
			if (m_osu_timingpoints_force->getBool())
				m_beatmap->updateTimingPoints(m_iTime + (!startOrEnd ? 0 : m_iObjectDuration));

			const Vector2 osuCoords = m_beatmap->pixels2OsuCoords(m_beatmap->osuCoords2Pixels(m_vCurPointRaw));

			m_beatmap->getSkin()->playHitCircleSound(m_iCurRepeatCounterForHitSounds < m_hitSounds.size() ? m_hitSounds[m_iCurRepeatCounterForHitSounds] : m_iSampleType, OsuGameRules::osuCoords2Pan(osuCoords.x));

			if (!startOrEnd)
			{
				m_fStartHitAnimation = 0.001f; // quickfix for 1 frame missing images
				anim->moveQuadOut(&m_fStartHitAnimation, 1.0f, OsuGameRules::getFadeOutTime(m_beatmap), true);
			}
			else
			{
				if (m_iRepeat % 2 != 0)
				{
					m_fEndHitAnimation = 0.001f; // quickfix for 1 frame missing images
					anim->moveQuadOut(&m_fEndHitAnimation, 1.0f, OsuGameRules::getFadeOutTime(m_beatmap), true);
				}
				else
				{
					m_fStartHitAnimation = 0.001f; // quickfix for 1 frame missing images
					anim->moveQuadOut(&m_fStartHitAnimation, 1.0f, OsuGameRules::getFadeOutTime(m_beatmap), true);
				}
			}

			if (m_beatmap->getOsu()->isInVRMode())
			{
				if (m_bOnHitVRLeftControllerHapticFeedback)
					openvr->getLeftController()->triggerHapticPulse(m_beatmap->getOsu()->getVR()->getHapticPulseStrength());
				else
					openvr->getRightController()->triggerHapticPulse(m_beatmap->getOsu()->getVR()->getHapticPulseStrength());
			}
		}

		// end body fadeout
		if (startOrEnd)
		{
			m_fEndSliderBodyFadeAnimation = 0.001f; // quickfix for 1 frame missing images
			anim->moveQuadOut(&m_fEndSliderBodyFadeAnimation, 1.0f, OsuGameRules::getFadeOutTime(m_beatmap) * osu_slider_body_fade_out_time_multiplier.getFloat(), true);

			m_beatmap->getSkin()->stopSliderSlideSound();
		}
	}

	// add score, and we are finished
	if (!startOrEnd)
	{
		// startcircle

		m_bStartFinished = true;

		// remember which key this slider was started with
		if (m_beatmap->isKey2Down() && !m_beatmap->isLastKeyDownKey1())
			m_iDownKey = 2;
		else if (m_beatmap->isKey1Down() && m_beatmap->isLastKeyDownKey1())
			m_iDownKey = 1;

		if (!m_beatmap->getOsu()->getModTarget())
			m_beatmap->addHitResult(this, result, delta, false, false, true, false, true, true); // not end of combo, show in hiterrorbar, ignore for accuracy, increase combo, don't count towards score, depending on scorev2 ignore for health or not
		else
			addHitResult(result, delta, false, m_curve->pointAt(0.0f), targetDelta, targetAngle, false, false, true, false); // not end of combo, show in hiterrorbar, use for accuracy, increase combo, increase score, ignore for health, don't add object duration to result anim

		// add bonus score + health manually
		if (result != OsuScore::HIT::HIT_MISS)
		{
			OsuScore::HIT resultForHealth = OsuScore::HIT::HIT_SLIDER30;

			if (m_osu_drain_type_ref->getInt() == 1) // VR
			{
				resultForHealth = result;
			}

			m_beatmap->addHitResult(this, resultForHealth, 0, false, true, true, true, true, false); // only increase health
			m_beatmap->addScorePoints(30);
		}
		else
		{
			// special case: missing the startcircle drains HIT_MISS_SLIDERBREAK health (and not HIT_MISS health)
			m_beatmap->addHitResult(this, OsuScore::HIT::HIT_MISS_SLIDERBREAK, 0, false, true, true, true, true, false); // only decrease health
		}
	}
	else
	{
		// endcircle

		m_bStartFinished = true;
		m_bEndFinished = true;
		m_bFinished = true;

		if (!isEndResultFromStrictTrackingMod)
		{
			// special case: osu!lazer 2020 only returns 1 judgement for the whole slider, but via the startcircle. i.e. we are not allowed to drain again here in mcosu logic (because startcircle judgement is handled at the end here)
			const bool isLazer2020Drain = (m_osu_drain_type_ref->getInt() == 3); // osu!lazer 2020

			addHitResult(result, delta, m_bIsEndOfCombo, getRawPosAt(m_iTime + m_iObjectDuration), -1.0f, 0.0f, true, !m_bHeldTillEnd, isLazer2020Drain); // end of combo, ignore in hiterrorbar, depending on heldTillEnd increase combo or not, increase score, increase health depending on drain type

			// add bonus score + extra health manually
			if (m_bHeldTillEnd)
			{
				m_beatmap->addHitResult(this, OsuScore::HIT::HIT_SLIDER30, 0, false, true, true, true, true, false); // only increase health
				m_beatmap->addScorePoints(30);
			}
			else
			{
				// special case: missing the endcircle drains HIT_MISS_SLIDERBREAK health (and not HIT_MISS health)
				// NOTE: yes, this will drain twice for the end of a slider (once for the judgement of the whole slider above, and once for the endcircle here)
				m_beatmap->addHitResult(this, OsuScore::HIT::HIT_MISS_SLIDERBREAK, 0, false, true, true, true, true, false); // only decrease health
			}
		}
	}

	m_iCurRepeatCounterForHitSounds++;
}

void OsuSlider::onRepeatHit(bool successful, bool sliderend)
{
	if (m_points.size() == 0) return;

	// repeat hit of a slider adds +30 points, if successful

	// sound and hit animation
	if (!successful)
		onSliderBreak();
	else
	{
		if (m_osu_timingpoints_force->getBool())
			m_beatmap->updateTimingPoints(m_iTime + (long)((float)m_iObjectDuration*m_fActualSlidePercent));

		const Vector2 osuCoords = m_beatmap->pixels2OsuCoords(m_beatmap->osuCoords2Pixels(m_vCurPointRaw));

		m_beatmap->getSkin()->playHitCircleSound(m_iCurRepeatCounterForHitSounds < m_hitSounds.size() ? m_hitSounds[m_iCurRepeatCounterForHitSounds] : m_iSampleType, OsuGameRules::osuCoords2Pan(osuCoords.x));

		m_fFollowCircleTickAnimationScale = 0.0f;
		anim->moveLinear(&m_fFollowCircleTickAnimationScale, 1.0f, OsuGameRules::osu_slider_followcircle_tick_pulse_time.getFloat(), true);

		if (sliderend)
		{
			m_fEndHitAnimation = 0.001f; // quickfix for 1 frame missing images
			anim->moveQuadOut(&m_fEndHitAnimation, 1.0f, OsuGameRules::getFadeOutTime(m_beatmap), true);
		}
		else
		{
			m_fStartHitAnimation = 0.001f; // quickfix for 1 frame missing images
			anim->moveQuadOut(&m_fStartHitAnimation, 1.0f, OsuGameRules::getFadeOutTime(m_beatmap), true);
		}

		if (m_beatmap->getOsu()->isInVRMode())
		{
			if (m_bOnHitVRLeftControllerHapticFeedback)
				openvr->getLeftController()->triggerHapticPulse(m_beatmap->getOsu()->getVR()->getHapticPulseStrength());
			else
				openvr->getRightController()->triggerHapticPulse(m_beatmap->getOsu()->getVR()->getHapticPulseStrength());
		}
	}

	// add score
	if (!successful)
	{
		// add health manually
		// special case: missing a repeat drains HIT_MISS_SLIDERBREAK health (and not HIT_MISS health)
		m_beatmap->addHitResult(this, OsuScore::HIT::HIT_MISS_SLIDERBREAK, 0, false, true, true, true, true, false); // only decrease health
	}
	else
	{
		m_beatmap->addHitResult(this, OsuScore::HIT::HIT_SLIDER30, 0, false, true, true, false, true, false); // not end of combo, ignore in hiterrorbar, ignore for accuracy, increase combo, don't count towards score, increase health

		// add bonus score manually
		m_beatmap->addScorePoints(30);
	}

	m_iCurRepeatCounterForHitSounds++;
}

void OsuSlider::onTickHit(bool successful, int tickIndex)
{
	if (m_points.size() == 0) return;

	// tick hit of a slider adds +10 points, if successful

	// tick drawing visibility
	int numMissingTickClicks = 0;
	for (int i=0; i<m_clicks.size(); i++)
	{
		if (m_clicks[i].type == 1 && m_clicks[i].tickIndex == tickIndex && !m_clicks[i].finished)
			numMissingTickClicks++;
	}
	if (numMissingTickClicks == 0)
		m_ticks[tickIndex].finished = true;

	// sound and hit animation
	if (!successful)
		onSliderBreak();
	else
	{
		if (m_osu_timingpoints_force->getBool())
			m_beatmap->updateTimingPoints(m_iTime + (long)((float)m_iObjectDuration*m_fActualSlidePercent));

		const Vector2 osuCoords = m_beatmap->pixels2OsuCoords(m_beatmap->osuCoords2Pixels(m_vCurPointRaw));

		m_beatmap->getSkin()->playSliderTickSound(OsuGameRules::osuCoords2Pan(osuCoords.x));

		m_fFollowCircleTickAnimationScale = 0.0f;
		anim->moveLinear(&m_fFollowCircleTickAnimationScale, 1.0f, OsuGameRules::osu_slider_followcircle_tick_pulse_time.getFloat(), true);
	}

	// add score
	if (!successful)
	{
		// add health manually
		// special case: missing a tick drains HIT_MISS_SLIDERBREAK health (and not HIT_MISS health)
		m_beatmap->addHitResult(this, OsuScore::HIT::HIT_MISS_SLIDERBREAK, 0, false, true, true, true, true, false); // only decrease health
	}
	else
	{
		m_beatmap->addHitResult(this, OsuScore::HIT::HIT_SLIDER10, 0, false, true, true, false, true, false); // not end of combo, ignore in hiterrorbar, ignore for accuracy, increase combo, don't count towards score, increase health

		// add bonus score manually
		m_beatmap->addScorePoints(10);
	}
}

void OsuSlider::onSliderBreak()
{
	m_beatmap->addSliderBreak();

	if (osu_slider_break_epilepsy.getBool())
	{
		m_fSliderBreakRapeTime = engine->getTime() + 0.15f;
		m_epilepsy_ref->setValue(1.0f);
	}
}

void OsuSlider::onReset(long curPos)
{
	OsuHitObject::onReset(curPos);

	m_beatmap->getSkin()->stopSliderSlideSound();

	m_iStrictTrackingModLastClickHeldTime = 0;
	m_iDownKey = 0;
	m_iPrevSliderSlideSoundSampleSet = -1;
	m_bCursorLeft = true;
	m_bHeldTillEnd = false;
	m_bHeldTillEndForLenienceHack = false;
	m_bHeldTillEndForLenienceHackCheck = false;
	m_startResult = OsuScore::HIT::HIT_NULL;
	m_endResult = OsuScore::HIT::HIT_NULL;

	m_iCurRepeatCounterForHitSounds = 0;

	anim->deleteExistingAnimation(&m_fFollowCircleTickAnimationScale);
	anim->deleteExistingAnimation(&m_fStartHitAnimation);
	anim->deleteExistingAnimation(&m_fEndHitAnimation);
	anim->deleteExistingAnimation(&m_fEndSliderBodyFadeAnimation);

	if (m_iTime > curPos)
	{
		m_bStartFinished = false;
		m_fStartHitAnimation = 0.0f;
		m_bEndFinished = false;
		m_bFinished = false;
		m_fEndHitAnimation = 0.0f;
		m_fEndSliderBodyFadeAnimation = 0.0f;
	}
	else if (curPos < m_iTime+m_iObjectDuration)
	{
		m_bStartFinished = true;
		m_fStartHitAnimation = 1.0f;

		m_bEndFinished = false;
		m_bFinished = false;
		m_fEndHitAnimation = 0.0f;
		m_fEndSliderBodyFadeAnimation = 0.0f;
	}
	else
	{
		m_bStartFinished = true;
		m_fStartHitAnimation = 1.0f;
		m_bEndFinished = true;
		m_bFinished = true;
		m_fEndHitAnimation = 1.0f;
		m_fEndSliderBodyFadeAnimation = 1.0f;
	}

	for (int i=0; i<m_clicks.size(); i++)
	{
		if (curPos > m_clicks[i].time)
		{
			m_clicks[i].finished = true;
			m_clicks[i].successful = true;
		}
		else
		{
			m_clicks[i].finished = false;
			m_clicks[i].successful = false;
		}
	}

	for (int i=0; i<m_ticks.size(); i++)
	{
		int numMissingTickClicks = 0;
		for (int c=0; c<m_clicks.size(); c++)
		{
			if (m_clicks[c].type == 1 && m_clicks[c].tickIndex == i && !m_clicks[c].finished)
				numMissingTickClicks++;
		}
		m_ticks[i].finished = numMissingTickClicks == 0;
	}

	m_fSliderBreakRapeTime = 0.0f;
	m_epilepsy_ref->setValue(0.0f);
}

void OsuSlider::rebuildVertexBuffer(bool useRawCoords)
{
	// base mesh (background) (raw unscaled, size in raw osu coordinates centered at (0, 0, 0))
	// this mesh can be shared by both the VR draw() and the desktop draw(), although in desktop mode it needs to be scaled and translated appropriately since we are not 1:1 with the playfield
	std::vector<Vector2> osuCoordPoints = m_curve->getPoints();
	if (!useRawCoords)
	{
		for (int p=0; p<osuCoordPoints.size(); p++)
		{
			osuCoordPoints[p] = m_beatmap->osuCoords2LegacyPixels(osuCoordPoints[p]);
		}
	}
	SAFE_DELETE(m_vao);
	m_vao = OsuSliderRenderer::generateVAO(m_beatmap->getOsu(), osuCoordPoints, m_beatmap->getRawHitcircleDiameter());

	if (m_beatmap->getOsu()->isInVRMode())
	{
		// body mesh (foreground)
		// HACKHACK: magic numbers from shader
		const float defaultBorderSize = 0.11;
		const float defaultTransitionSize = 0.011f;
		const float defaultOuterShadowSize = 0.08f;
		const float scale = 1.0f - m_osu_slider_border_size_multiplier_ref->getFloat()*defaultBorderSize - defaultTransitionSize - defaultOuterShadowSize;
		SAFE_DELETE(m_vaoVR2);
		m_vaoVR2 = OsuSliderRenderer::generateVAO(m_beatmap->getOsu(), osuCoordPoints, m_beatmap->getRawHitcircleDiameter()*scale, Vector3(0, 0, 0.25f)); // push 0.25f outwards
	}
}

bool OsuSlider::isClickHeldSlider()
{
	// m_iDownKey contains the key the sliderstartcircle was clicked with (if it was clicked at all, if not then it is 0)
	// it is reset back to 0 automatically in update() once the opposite key has been released at least once
	// if m_iDownKey is less than 1, then any key being held is enough to slide (either the startcircle was missed, or the opposite key it was clicked with has been released at least once already)
	// otherwise, that specific key is the only one which counts for sliding
	const bool mouseDownAcceptable = (m_iDownKey == 1 ? m_beatmap->isKey1Down() : m_beatmap->isKey2Down());
	return (m_iDownKey < 1 ? m_beatmap->isClickHeld() : mouseDownAcceptable) || (m_beatmap->getOsu()->isInVRMode() && (!m_beatmap->getOsu()->getVR()->isVirtualCursorOnScreen() || !m_osu_vr_draw_desktop_playfield->getBool())); // a bit shit, but whatever. see OsuBeatmap::isClickHeld()
}
