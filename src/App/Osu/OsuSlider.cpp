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

ConVar osu_slider_ball_tint_combo_color("osu_slider_ball_tint_combo_color", true);

ConVar osu_snaking_sliders("osu_snaking_sliders", true);
ConVar osu_mod_hd_slider_fade_percent("osu_mod_hd_slider_fade_percent", 1.0f);
ConVar osu_mod_hd_slider_fast_fade("osu_mod_hd_slider_fast_fade", false);

ConVar osu_slider_end_inside_check_offset("osu_slider_end_inside_check_offset", 36, "offset in milliseconds going backwards from the end point, at which \"being inside the slider\" is checked. (osu bullshit behavior)");
ConVar osu_slider_break_epilepsy("osu_slider_break_epilepsy", false);
ConVar osu_slider_scorev2("osu_slider_scorev2", false);

ConVar osu_slider_draw_body("osu_slider_draw_body", true);
ConVar osu_slider_shrink("osu_slider_shrink", false);
ConVar osu_slider_reverse_arrow_black_threshold("osu_slider_reverse_arrow_black_threshold", 1.0f, "Blacken reverse arrows if the average color brightness percentage is above this value"); // looks too shitty atm
ConVar osu_slider_body_smoothsnake("osu_slider_body_smoothsnake", true, "draw 1 extra interpolated circle mesh at the start & end of every slider for extra smooth snaking/shrinking");
ConVar osu_slider_body_lazer_fadeout_style("osu_slider_body_lazer_fadeout_style", true, "if snaking out sliders are enabled (aka shrinking sliders), smoothly fade out the last remaining part of the body (instead of vanishing instantly)");
ConVar osu_slider_body_fade_out_time_multiplier("osu_slider_body_fade_out_time_multiplier", 1.0f, "multiplies osu_hitobject_fade_out_time");
ConVar osu_slider_reverse_arrow_animated("osu_slider_reverse_arrow_animated", true, "pulse animation on reverse arrows");
ConVar osu_slider_reverse_arrow_alpha_multiplier("osu_slider_reverse_arrow_alpha_multiplier", 1.0f);

ConVar *OsuSlider::m_osu_playfield_mirror_horizontal_ref = NULL;
ConVar *OsuSlider::m_osu_playfield_mirror_vertical_ref = NULL;
ConVar *OsuSlider::m_osu_playfield_rotation_ref = NULL;
ConVar *OsuSlider::m_osu_mod_fps_ref = NULL;
ConVar *OsuSlider::m_osu_slider_border_size_multiplier_ref = NULL;
ConVar *OsuSlider::m_epilepsy = NULL;
ConVar *OsuSlider::m_osu_auto_cursordance = NULL;

OsuSlider::OsuSlider(char type, int repeat, float pixelLength, std::vector<Vector2> points, std::vector<int> hitSounds, std::vector<float> ticks, float sliderTime, float sliderTimeWithoutRepeats, long time, int sampleType, int comboNumber, int colorCounter, int colorOffset, OsuBeatmapStandard *beatmap) : OsuHitObject(time, sampleType, comboNumber, colorCounter, colorOffset, beatmap)
{
	if (m_osu_playfield_mirror_horizontal_ref == NULL)
		m_osu_playfield_mirror_horizontal_ref = convar->getConVarByName("osu_playfield_mirror_horizontal");
	if (m_osu_playfield_mirror_vertical_ref == NULL)
		m_osu_playfield_mirror_vertical_ref = convar->getConVarByName("osu_playfield_mirror_vertical");
	if (m_osu_playfield_rotation_ref == NULL)
		m_osu_playfield_rotation_ref = convar->getConVarByName("osu_playfield_rotation");
	if (m_osu_mod_fps_ref == NULL)
		m_osu_mod_fps_ref = convar->getConVarByName("osu_mod_fps");
	if (m_osu_slider_border_size_multiplier_ref == NULL)
		m_osu_slider_border_size_multiplier_ref = convar->getConVarByName("osu_slider_border_size_multiplier");
	if (m_epilepsy == NULL)
		m_epilepsy = convar->getConVarByName("epilepsy");
	if (m_osu_auto_cursordance == NULL)
		m_osu_auto_cursordance = convar->getConVarByName("osu_auto_cursordance");

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
		sc.time = m_iTime + (long)(m_fSliderTimeWithoutRepeats * (i+1));

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

			const float tickPercentRelativeToRepeatFromStartAbs = (((i+1) % 2) != 0 ? m_ticks[t].percent : 1.0f - m_ticks[t].percent);

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
	m_iLastClickHeld = 0;
	m_iDownKey = 0;
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
			drawBody(g, alpha, sliderSnakeStart, sliderSnake);

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
			g->scale(tickImageScale, tickImageScale);
			g->translate(pos.x, pos.y);
			g->drawImage(skin->getSliderScorePoint());
			g->popTransform();
		}

		// draw start & end circle & reverse arrows
		if (m_points.size() > 1)
		{
			// HACKHACK: very dirty code
			bool sliderRepeatStartCircleFinished = m_iRepeat < 2;
			bool sliderRepeatEndCircleFinished = false;
			for (int i=0; i<m_clicks.size(); i++)
			{
				if (m_clicks[i].type == 0)
				{
					if (m_clicks[i].sliderend)
						sliderRepeatEndCircleFinished = m_clicks[i].finished;
					else
						sliderRepeatStartCircleFinished = m_clicks[i].finished;
				}
			}

			// end circle
			if (((!m_bEndFinished && m_iRepeat % 2 != 0) || !sliderRepeatEndCircleFinished))
				drawEndCircle(g, alpha, sliderSnake);

			// start circle
			if (!m_bStartFinished || !sliderRepeatStartCircleFinished || (!m_bEndFinished && m_iRepeat % 2 == 0))
				drawStartCircle(g, alpha);

			// debug
			/*
			Vector2 debugPos = m_beatmap->osuCoords2Pixels(m_curve->pointAt(0));
			g->pushTransform();
			g->translate(debugPos.x, debugPos.y);
			g->setColor(0xffff0000);
			//g->drawString(engine->getResourceManager()->getFont("FONT_DEFAULT"), UString::format("%i, %i", m_cType, m_points.size()));
			//g->drawString(engine->getResourceManager()->getFont("FONT_DEFAULT"), UString::format("%i", (int)sliderRepeatStartCircleFinished));
			g->drawString(engine->getResourceManager()->getFont("FONT_DEFAULT"), UString::format("%li", m_iTime));
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
					if (m_beatmap->getOsu()->getModHR() || m_osu_playfield_mirror_horizontal_ref->getBool())
						rotation = 360.0f - rotation;
					if (m_osu_playfield_mirror_vertical_ref->getBool())
						rotation = 180.0f - rotation;

					const float osuCoordScaleMultiplier = m_beatmap->getHitcircleDiameter() / m_beatmap->getRawHitcircleDiameter();
					float reverseArrowImageScale = (m_beatmap->getRawHitcircleDiameter() / (128.0f * (skin->isReverseArrow2x() ? 2.0f : 1.0f))) * osuCoordScaleMultiplier;

					reverseArrowImageScale *= 1.0f + pulse*0.30f;

					g->setColor(reverseArrowColor);
					g->setAlpha(m_fReverseArrowAlpha);
					g->pushTransform();
					g->rotate(rotation);
					g->scale(reverseArrowImageScale, reverseArrowImageScale);
					g->translate(pos.x, pos.y);
					g->drawImage(skin->getReverseArrow());
					g->popTransform();
				}

				// start
				if (m_iReverseArrowPos == 1 || m_iReverseArrowPos == 3)
				{
					Vector2 pos = m_beatmap->osuCoords2Pixels(m_curve->pointAt(0.0f));
					float rotation = m_curve->getStartAngle() - m_osu_playfield_rotation_ref->getFloat() - m_beatmap->getPlayfieldRotation();
					if (m_beatmap->getOsu()->getModHR() || m_osu_playfield_mirror_horizontal_ref->getBool())
						rotation = 360.0f - rotation;
					if (m_osu_playfield_mirror_vertical_ref->getBool())
						rotation = 180.0f - rotation;

					const float osuCoordScaleMultiplier = m_beatmap->getHitcircleDiameter() / m_beatmap->getRawHitcircleDiameter();
					float reverseArrowImageScale = (m_beatmap->getRawHitcircleDiameter() / (128.0f * (skin->isReverseArrow2x() ? 2.0f : 1.0f))) * osuCoordScaleMultiplier;

					reverseArrowImageScale *= 1.0f + pulse*0.30f;

					g->setColor(reverseArrowColor);
					g->setAlpha(m_fReverseArrowAlpha);
					g->pushTransform();
					g->rotate(rotation);
					g->scale(reverseArrowImageScale, reverseArrowImageScale);
					g->translate(pos.x, pos.y);
					g->drawImage(skin->getReverseArrow());
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
			OsuSliderRenderer::draw(g, m_beatmap->getOsu(), emptyVector, alwaysPoints, m_beatmap->getHitcircleDiameter(), 0.0f, 0.0f, m_beatmap->getSkin()->getComboColorForCounter(m_iColorCounter, m_iColorOffset), 1.0f - m_fEndSliderBodyFadeAnimation, getTime());
	}

	if (m_fStartHitAnimation > 0.0f && m_fStartHitAnimation != 1.0f && !m_beatmap->getOsu()->getModHD())
	{
		float alpha = 1.0f - m_fStartHitAnimation;

		float scale = m_fStartHitAnimation;
		scale = -scale*(scale-2.0f); // quad out scale

		bool drawNumber = (skin->getVersion() > 1.0f ? false : true) && m_iCurRepeat < 1;

		g->pushTransform();
			g->scale((1.0f+scale*OsuGameRules::osu_circle_fade_out_scale.getFloat()), (1.0f+scale*OsuGameRules::osu_circle_fade_out_scale.getFloat()));
			if (m_iCurRepeat < 1)
			{
				m_beatmap->getSkin()->getHitCircleOverlay2()->setAnimationTimeOffset(!m_beatmap->isInMafhamRenderChunk() ? m_iTime - m_iApproachTime : m_beatmap->getCurMusicPosWithOffsets());
				m_beatmap->getSkin()->getSliderStartCircleOverlay2()->setAnimationTimeOffset(!m_beatmap->isInMafhamRenderChunk() ? m_iTime - m_iApproachTime : m_beatmap->getCurMusicPosWithOffsets());

				OsuCircle::drawSliderStartCircle(g, m_beatmap, m_curve->pointAt(0.0f), m_iComboNumber, m_iColorCounter, m_iColorOffset, 1.0f, alpha, alpha, drawNumber);
			}
			else
			{
				m_beatmap->getSkin()->getHitCircleOverlay2()->setAnimationTimeOffset(!m_beatmap->isInMafhamRenderChunk() ? m_iTime : m_beatmap->getCurMusicPosWithOffsets());
				m_beatmap->getSkin()->getSliderEndCircleOverlay2()->setAnimationTimeOffset(!m_beatmap->isInMafhamRenderChunk() ? m_iTime : m_beatmap->getCurMusicPosWithOffsets());

				OsuCircle::drawSliderEndCircle(g, m_beatmap, m_curve->pointAt(0.0f), m_iComboNumber, m_iColorCounter, m_iColorOffset, 1.0f, alpha, alpha, drawNumber);
			}
		g->popTransform();
	}

	if (m_fEndHitAnimation > 0.0f && m_fEndHitAnimation != 1.0f && !m_beatmap->getOsu()->getModHD())
	{
		float alpha = 1.0f - m_fEndHitAnimation;

		float scale = m_fEndHitAnimation;
		scale = -scale*(scale-2.0f); // quad out scale

		g->pushTransform();
			g->scale((1.0f+scale*OsuGameRules::osu_circle_fade_out_scale.getFloat()), (1.0f+scale*OsuGameRules::osu_circle_fade_out_scale.getFloat()));
			{
				m_beatmap->getSkin()->getHitCircleOverlay2()->setAnimationTimeOffset(!m_beatmap->isInMafhamRenderChunk() ? m_iTime - m_iFadeInTime : m_beatmap->getCurMusicPosWithOffsets());
				m_beatmap->getSkin()->getSliderEndCircleOverlay2()->setAnimationTimeOffset(!m_beatmap->isInMafhamRenderChunk() ? m_iTime - m_iFadeInTime : m_beatmap->getCurMusicPosWithOffsets());

				OsuCircle::drawSliderEndCircle(g, m_beatmap, m_curve->pointAt(1.0f), m_iComboNumber, m_iColorCounter, m_iColorOffset, 1.0f, alpha, 0.0f, false);
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
	OsuSkin *skin = m_beatmap->getSkin();

	// HACKHACK: so much code duplication aaaaaaah
	if ((m_bVisible || (m_bStartFinished && !m_bFinished)) && drawApproachCircle) // extra possibility to avoid flicker between OsuHitObject::m_bVisible delay and the fadeout animation below this if block
	{
		if (m_points.size() > 1)
		{
			// HACKHACK: very dirty code
			bool sliderRepeatStartCircleFinished = m_iRepeat < 2;
			bool sliderRepeatEndCircleFinished = false;
			for (int i=0; i<m_clicks.size(); i++)
			{
				if (m_clicks[i].type == 0)
				{
					if (m_clicks[i].sliderend)
						sliderRepeatEndCircleFinished = m_clicks[i].finished;
					else
						sliderRepeatStartCircleFinished = m_clicks[i].finished;
				}
			}

			// start circle
			if (!m_bStartFinished || !sliderRepeatStartCircleFinished || (!m_bEndFinished && m_iRepeat % 2 == 0))
			{
				OsuCircle::drawApproachCircle(g, m_beatmap, m_curve->pointAt(0.0f), m_iComboNumber, m_iColorCounter, m_iColorOffset, m_fApproachScale, m_fAlphaForApproachCircle, m_bOverrideHDApproachCircle);
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

	// draw sliderb on top of everything
	if (m_bVisible || (m_bStartFinished && !m_bFinished)) // extra possibility in the if-block to avoid flicker between OsuHitObject::m_bVisible delay and the fadeout animation below this if-block
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
			g->rotate(ballAngle);
			skin->getSliderb()->setAnimationTimeOffset(m_iTime);
			skin->getSliderb()->drawRaw(g, point, m_beatmap->getHitcircleDiameter() / skin->getSliderb()->getSizeBaseRaw().x);
			g->popTransform();
		}
	}
}

void OsuSlider::drawVR(Graphics *g, Matrix4 &mvp, OsuVR *vr)
{
	// HACKHACK: code duplication aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa
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
			g->scale(tickImageScale, tickImageScale);
			g->translate(pos.x, pos.y);
			g->drawImage(skin->getSliderScorePoint());
			g->popTransform();
		}

		// draw start & end circle & reverse arrows
		if (m_points.size() > 1)
		{
			// HACKHACK: very dirty code
			bool sliderRepeatStartCircleFinished = m_iRepeat < 2;
			bool sliderRepeatEndCircleFinished = false;
			for (int i=0; i<m_clicks.size(); i++)
			{
				if (m_clicks[i].type == 0)
				{
					if (m_clicks[i].sliderend)
						sliderRepeatEndCircleFinished = m_clicks[i].finished;
					else
						sliderRepeatStartCircleFinished = m_clicks[i].finished;
				}
			}

			// end circle
			if (((!m_bEndFinished && m_iRepeat % 2 != 0) || !sliderRepeatEndCircleFinished))
				drawEndCircle(g, alpha, sliderSnake);

			// start circle
			if (!m_bStartFinished || !sliderRepeatStartCircleFinished || (!m_bEndFinished && m_iRepeat % 2 == 0))
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
					if (m_beatmap->getOsu()->getModHR() || m_osu_playfield_mirror_horizontal_ref->getBool())
						rotation = 360.0f - rotation;
					if (m_osu_playfield_mirror_vertical_ref->getBool())
						rotation = 180.0f - rotation;

					const float osuCoordScaleMultiplier = m_beatmap->getHitcircleDiameter() / m_beatmap->getRawHitcircleDiameter();
					float reverseArrowImageScale = (m_beatmap->getRawHitcircleDiameter() / (128.0f * (skin->isReverseArrow2x() ? 2.0f : 1.0f))) * osuCoordScaleMultiplier;


					reverseArrowImageScale *= 1.0f + pulse*0.30f;

					g->setColor(reverseArrowColor);
					g->setAlpha(m_fReverseArrowAlpha);
					g->pushTransform();
					g->rotate(rotation);
					g->scale(reverseArrowImageScale, reverseArrowImageScale);
					g->translate(pos.x, pos.y);
					g->drawImage(skin->getReverseArrow());
					g->popTransform();
				}

				// start
				if (m_iReverseArrowPos == 1 || m_iReverseArrowPos == 3)
				{
					Vector2 pos = m_beatmap->osuCoords2Pixels(m_curve->pointAt(0.0f));
					float rotation = m_curve->getStartAngle() - m_osu_playfield_rotation_ref->getFloat() - m_beatmap->getPlayfieldRotation();
					if (m_beatmap->getOsu()->getModHR() || m_osu_playfield_mirror_horizontal_ref->getBool())
						rotation = 360.0f - rotation;
					if (m_osu_playfield_mirror_vertical_ref->getBool())
						rotation = 180.0f - rotation;

					const float osuCoordScaleMultiplier = m_beatmap->getHitcircleDiameter() / m_beatmap->getRawHitcircleDiameter();
					float reverseArrowImageScale = (m_beatmap->getRawHitcircleDiameter() / (128.0f * (skin->isReverseArrow2x() ? 2.0f : 1.0f))) * osuCoordScaleMultiplier;

					reverseArrowImageScale *= 1.0f + pulse*0.30f;

					g->setColor(reverseArrowColor);
					g->setAlpha(m_fReverseArrowAlpha);
					g->pushTransform();
					g->rotate(rotation);
					g->scale(reverseArrowImageScale, reverseArrowImageScale);
					g->translate(pos.x, pos.y);
					g->drawImage(skin->getReverseArrow());
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
			g->scale((1.0f+scale*OsuGameRules::osu_circle_fade_out_scale.getFloat()), (1.0f+scale*OsuGameRules::osu_circle_fade_out_scale.getFloat()));
			if (m_iCurRepeat < 1)
			{
				m_beatmap->getSkin()->getHitCircleOverlay2()->setAnimationTimeOffset(!m_beatmap->isInMafhamRenderChunk() ? m_iTime - m_iApproachTime : m_beatmap->getCurMusicPosWithOffsets());
				m_beatmap->getSkin()->getSliderStartCircleOverlay2()->setAnimationTimeOffset(!m_beatmap->isInMafhamRenderChunk() ? m_iTime - m_iApproachTime : m_beatmap->getCurMusicPosWithOffsets());

				OsuCircle::drawSliderStartCircle(g, m_beatmap, m_curve->pointAt(0.0f), m_iComboNumber, m_iColorCounter, m_iColorOffset, 1.0f, alpha, alpha, drawNumber);
			}
			else
			{
				m_beatmap->getSkin()->getHitCircleOverlay2()->setAnimationTimeOffset(!m_beatmap->isInMafhamRenderChunk() ? m_iTime : m_beatmap->getCurMusicPosWithOffsets());
				m_beatmap->getSkin()->getSliderEndCircleOverlay2()->setAnimationTimeOffset(!m_beatmap->isInMafhamRenderChunk() ? m_iTime : m_beatmap->getCurMusicPosWithOffsets());

				OsuCircle::drawSliderEndCircle(g, m_beatmap, m_curve->pointAt(0.0f), m_iComboNumber, m_iColorCounter, m_iColorOffset, 1.0f, alpha, alpha, drawNumber);
			}
		g->popTransform();
	}

	if (m_fEndHitAnimation > 0.0f && m_fEndHitAnimation != 1.0f && !m_beatmap->getOsu()->getModHD())
	{
		float alpha = 1.0f - m_fEndHitAnimation;

		float scale = m_fEndHitAnimation;
		scale = -scale*(scale-2.0f); // quad out scale

		g->pushTransform();
			g->scale((1.0f+scale*OsuGameRules::osu_circle_fade_out_scale.getFloat()), (1.0f+scale*OsuGameRules::osu_circle_fade_out_scale.getFloat()));
			{
				m_beatmap->getSkin()->getHitCircleOverlay2()->setAnimationTimeOffset(!m_beatmap->isInMafhamRenderChunk() ? m_iTime - m_iFadeInTime : m_beatmap->getCurMusicPosWithOffsets());
				m_beatmap->getSkin()->getSliderEndCircleOverlay2()->setAnimationTimeOffset(!m_beatmap->isInMafhamRenderChunk() ? m_iTime - m_iFadeInTime : m_beatmap->getCurMusicPosWithOffsets());

				OsuCircle::drawSliderEndCircle(g, m_beatmap, m_curve->pointAt(1.0f), m_iComboNumber, m_iColorCounter, m_iColorOffset, 1.0f, alpha, 0.0f, false);
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

void OsuSlider::drawStartCircle(Graphics *g, float alpha)
{
	if (m_bStartFinished)
	{
		m_beatmap->getSkin()->getHitCircleOverlay2()->setAnimationTimeOffset(!m_beatmap->isInMafhamRenderChunk() ? m_iTime : m_beatmap->getCurMusicPosWithOffsets());
		m_beatmap->getSkin()->getSliderEndCircleOverlay2()->setAnimationTimeOffset(!m_beatmap->isInMafhamRenderChunk() ? m_iTime : m_beatmap->getCurMusicPosWithOffsets());

		OsuCircle::drawSliderEndCircle(g, m_beatmap, m_curve->pointAt(0.0f), m_iComboNumber, m_iColorCounter, m_iColorOffset, 1.0f, m_fAlpha, 0.0f, false, false);
	}
	else
	{
		m_beatmap->getSkin()->getHitCircleOverlay2()->setAnimationTimeOffset(!m_beatmap->isInMafhamRenderChunk() ? m_iTime - m_iApproachTime : m_beatmap->getCurMusicPosWithOffsets());
		m_beatmap->getSkin()->getSliderStartCircleOverlay2()->setAnimationTimeOffset(!m_beatmap->isInMafhamRenderChunk() ? m_iTime - m_iApproachTime : m_beatmap->getCurMusicPosWithOffsets());

		OsuCircle::drawSliderStartCircle(g, m_beatmap, m_curve->pointAt(0.0f), m_iComboNumber, m_iColorCounter, m_iColorOffset, m_fApproachScale, m_fAlpha, m_fAlpha, !m_bHideNumberAfterFirstRepeatHit, m_bOverrideHDApproachCircle);
	}
}

void OsuSlider::drawEndCircle(Graphics *g, float alpha, float sliderSnake)
{
	m_beatmap->getSkin()->getHitCircleOverlay2()->setAnimationTimeOffset(!m_beatmap->isInMafhamRenderChunk() ? m_iTime - m_iFadeInTime : m_beatmap->getCurMusicPosWithOffsets());
	m_beatmap->getSkin()->getSliderEndCircleOverlay2()->setAnimationTimeOffset(!m_beatmap->isInMafhamRenderChunk() ? m_iTime - m_iFadeInTime : m_beatmap->getCurMusicPosWithOffsets());

	OsuCircle::drawSliderEndCircle(g, m_beatmap, m_curve->pointAt(sliderSnake), m_iComboNumber, m_iColorCounter, m_iColorOffset, 1.0f, m_fAlpha, 0.0f, false, false);
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

	if (m_beatmap->getOsu()->shouldFallBackToLegacySliderRenderer())
	{
		std::vector<Vector2> screenPoints = m_curve->getPoints();
		for (int p=0; p<screenPoints.size(); p++)
		{
			screenPoints[p] = m_beatmap->osuCoords2Pixels(screenPoints[p]);
		}

		// peppy sliders
		OsuSliderRenderer::draw(g, m_beatmap->getOsu(), screenPoints, alwaysPoints, m_beatmap->getHitcircleDiameter(), from, to, m_beatmap->getSkin()->getComboColorForCounter(m_iColorCounter, m_iColorOffset), alpha, getTime());

		// mm sliders
		///OsuSliderRenderer::drawMM(g, m_beatmap->getOsu(), screenPoints, m_beatmap->getHitcircleDiameter(), from, to, m_beatmap->getSkin()->getComboColorForCounter(m_iColorCounter, m_iColorOffset), alpha, getTime());
	}
	else
	{
		// vertex buffered sliders
		// as the base mesh is centered at (0, 0, 0) and in raw osu coordinates, we have to scale and translate it to make it fit the actual desktop playfield
		const float scale = OsuGameRules::getPlayfieldScaleFactor(m_beatmap->getOsu());
		Vector2 translation = OsuGameRules::getPlayfieldCenter(m_beatmap->getOsu());
		if (m_osu_mod_fps_ref->getBool())
			translation += m_beatmap->getFirstPersonCursorDelta();

		OsuSliderRenderer::draw(g, m_beatmap->getOsu(), m_vao, alwaysPoints, translation, scale, m_beatmap->getHitcircleDiameter(), from, to, m_beatmap->getSkin()->getComboColorForCounter(m_iColorCounter, m_iColorOffset), alpha, getTime());
	}
}

void OsuSlider::drawBodyVR(Graphics *g, OsuVR *vr, Matrix4 &mvp, float alpha, float from, float to)
{
	// HACKHACK: code duplication aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa

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
		OsuSliderRenderer::drawVR(g, m_beatmap->getOsu(), vr, mvp, m_fApproachScale, screenPoints, alwaysPoints, m_beatmap->getHitcircleDiameter(), from, to, m_beatmap->getSkin()->getComboColorForCounter(m_iColorCounter, m_iColorOffset), alpha, getTime());
	}
	else
	{
		// vertex buffered sliders
		OsuSliderRenderer::drawVR(g, m_beatmap->getOsu(), vr, mvp, m_fApproachScale, m_vao, m_vaoVR2, alwaysPoints, m_beatmap->getHitcircleDiameter(), from, to, m_beatmap->getSkin()->getComboColorForCounter(m_iColorCounter, m_iColorOffset), alpha, getTime());
	}
}

void OsuSlider::update(long curPos)
{
	OsuHitObject::update(curPos);

	// TEMP:
	if (m_fSliderBreakRapeTime != 0.0f && engine->getTime() > m_fSliderBreakRapeTime)
	{
		m_fSliderBreakRapeTime = 0.0f;
		m_epilepsy->setValue(0.0f);
	}

	// animations must be updated even if we are finished
	updateAnimations(curPos);

	// all further calculations are only done while we are active
	if (m_bFinished) return;

	// slider slide percent
	m_fSlidePercent = 0.0f;
	if (curPos > m_iTime)
		m_fSlidePercent = clamp<float>(clamp<long>((curPos - (m_iTime)), 0, (long)m_fSliderTime) / m_fSliderTime, 0.0f, 1.0f);

	m_fActualSlidePercent = m_fSlidePercent;

	m_fSliderSnakePercent = std::min(1.0f, (curPos - (m_iTime - m_iApproachTime)) / (m_iApproachTime / 3.0f));

	const long reverseArrowFadeInStart = m_iTime - (osu_snaking_sliders.getBool() ? (2.0f / 3.0f)*m_iApproachTime : m_iApproachTime);
	const long reverseArrowFadeInEnd = reverseArrowFadeInStart + 150;
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

	// if this slider is active
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
	m_bCursorInside = (m_beatmap->getOsu()->getModAuto() && (!m_osu_auto_cursordance->getBool() || ((m_beatmap->getCursorPos() - m_vCurPoint).length() < followRadius))) || ((m_beatmap->getCursorPos() - m_vCurPoint).length() < followRadius);
	m_bCursorLeft = !m_bCursorInside;

	if (m_beatmap->getOsu()->isInVRMode())
	{
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
				if (curPos >= m_iTime + (long)m_osu_relax_offset_ref->getInt()) // TODO: there was an && m_bCursorInside there, why?
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
		// slider tail lenience bullshit: see https://github.com/ppy/osu/blob/master/osu.Game.Rulesets.Osu/Objects/Slider.cs#L123
		// being "inside the slider" (for the end of the slider) is NOT checked at the exact end of the slider, but somewhere random before, because fuck you
		const long lenienceHackEndTime = std::max(m_iTime + m_iObjectDuration / 2, (m_iTime + m_iObjectDuration) - (long)osu_slider_end_inside_check_offset.getInt());
		if ((isClickHeldSlider() || m_beatmap->getOsu()->getModRelax()) && m_bCursorInside)
		{
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
				// handle endcircle

				m_bHeldTillEnd = m_bHeldTillEndForLenienceHack;
				m_endResult = m_bHeldTillEnd ? OsuScore::HIT::HIT_300 : OsuScore::HIT::HIT_MISS;


				if (m_startResult == OsuScore::HIT::HIT_NULL) // this may happen (if the slider time is shorter than the miss window of the startcircle)
				{
					// we still want to cause a sliderbreak in this case!
					onSliderBreak();
					m_startResult = OsuScore::HIT::HIT_MISS;
				}


				// handle total slider result (currently startcircle + repeats + ticks + endcircle)
				// clicks = (repeats + ticks)
				float numMaxPossibleHits = 1 + m_clicks.size() + 1;
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

				float percent = numActualHits / numMaxPossibleHits;

				bool allow300 = (osu_slider_scorev2.getBool() || m_beatmap->getOsu()->getModScorev2()) ? (m_startResult == OsuScore::HIT::HIT_300) : true;
				bool allow100 = (osu_slider_scorev2.getBool() || m_beatmap->getOsu()->getModScorev2()) ? (m_startResult == OsuScore::HIT::HIT_300 || m_startResult == OsuScore::HIT::HIT_100) : true;

				if (percent >= 0.999f && allow300)
					m_endResult = OsuScore::HIT::HIT_300;
				else if (percent >= 0.5f && allow100 && !OsuGameRules::osu_mod_ming3012.getBool() && !OsuGameRules::osu_mod_no100s.getBool())
					m_endResult = OsuScore::HIT::HIT_100;
				else if (percent > 0.0f && !OsuGameRules::osu_mod_no100s.getBool() && !OsuGameRules::osu_mod_no50s.getBool())
					m_endResult = OsuScore::HIT::HIT_50;
				else
					m_endResult = OsuScore::HIT::HIT_MISS;

				//debugLog("percent = %f\n", percent);

				onHit(m_endResult, 0, true); // delta doesn't matter here
			}
		}

		// handle constant sliding vibration
		if (m_beatmap->getOsu()->isInVRMode())
		{
			// clicks have priority over the constant sliding vibration, that's why this is at the bottom here AFTER the other function above have had a chance to call triggerHapticPulse()
			// while sliding the slider, vibrate the controller constantly
			if (m_bStartFinished && !m_bEndFinished && m_bCursorInside && !m_beatmap->isPaused() && !m_beatmap->isWaiting() && m_beatmap->isPlaying())
			{
				if (m_bOnHitVRLeftControllerHapticFeedback)
					openvr->getLeftController()->triggerHapticPulse(m_beatmap->getOsu()->getVR()->getSliderHapticPulseStrength());
				else
					openvr->getRightController()->triggerHapticPulse(m_beatmap->getOsu()->getVR()->getSliderHapticPulseStrength());
			}
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
	if (m_points.size() == 0 || m_bBlocked) return; // also handle note blocking here (doesn't need fancy shake logic, since sliders don't shake)

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

				m_beatmap->consumeClickEvent();
				m_startResult = result;
				onHit(m_startResult, delta, false, targetDelta, targetAngle);
			}
		}
	}
}

void OsuSlider::onHit(OsuScore::HIT result, long delta, bool startOrEnd, float targetDelta, float targetAngle)
{
	if (m_points.size() == 0) return;

	// start + end of a slider add +30 points, if successful

	//debugLog("startOrEnd = %i,    m_iCurRepeat = %i\n", (int)startOrEnd, m_iCurRepeat);

	// sound and hit animation
	if (result == OsuScore::HIT::HIT_MISS)
		onSliderBreak();
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

		// add score
		m_beatmap->addScorePoints(30);
	}

	// add it, and we are finished
	if (!startOrEnd)
	{
		m_bStartFinished = true;

		// remember which key this slider was started with
		if (m_beatmap->isKey2Down() && !m_beatmap->isLastKeyDownKey1())
			m_iDownKey = 2;
		else if (m_beatmap->isKey1Down() && m_beatmap->isLastKeyDownKey1())
			m_iDownKey = 1;

		if (!m_beatmap->getOsu()->getModTarget())
			m_beatmap->addHitResult(result, delta, false, true);
		else
			addHitResult(result, delta, m_curve->pointAt(0.0f), targetDelta, targetAngle, false);
	}
	else
	{
		addHitResult(result, delta, getRawPosAt(m_iTime + m_iObjectDuration), -1.0f, 0.0f, true, !m_bHeldTillEnd);
		m_bStartFinished = true;
		m_bEndFinished = true;
		m_bFinished = true;

		m_fEndSliderBodyFadeAnimation = 0.001f; // quickfix for 1 frame missing images
		anim->moveQuadOut(&m_fEndSliderBodyFadeAnimation, 1.0f, OsuGameRules::getFadeOutTime(m_beatmap) * osu_slider_body_fade_out_time_multiplier.getFloat(), true);
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

		m_beatmap->addHitResult(OsuScore::HIT::HIT_300, 0, true, true, false, true); // ignore in hiterrorbar, ignore for accuracy, increase combo, but don't count towards score!

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

		// add score
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

		m_beatmap->addHitResult(OsuScore::HIT::HIT_SLIDER30, 0, true);

		// add score
		m_beatmap->addScorePoints(10);
	}
}

void OsuSlider::onSliderBreak()
{
	m_beatmap->addSliderBreak();

	// TEMP:
	if (osu_slider_break_epilepsy.getBool())
	{
		m_fSliderBreakRapeTime = engine->getTime() + 0.15f;
		m_epilepsy->setValue(1.0f);
	}
}

void OsuSlider::onReset(long curPos)
{
	OsuHitObject::onReset(curPos);

	m_iLastClickHeld = 0;
	m_iDownKey = 0;
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
	m_epilepsy->setValue(0.0f);
}

void OsuSlider::rebuildVertexBuffer()
{
	// base mesh (background) (raw unscaled, size in raw osu coordinates centered at (0, 0, 0))
	// this mesh can be shared by both the VR draw() and the desktop draw(), although in desktop mode it needs to be scaled and translated appropriately since we are not 1:1 with the playfield
	std::vector<Vector2> osuCoordPoints = m_curve->getPoints();
	for (int p=0; p<osuCoordPoints.size(); p++)
	{
		osuCoordPoints[p] = m_beatmap->osuCoords2LegacyPixels(osuCoordPoints[p]);
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
	return (m_iDownKey < 1 ? m_beatmap->isClickHeld() : mouseDownAcceptable) || (m_beatmap->getOsu()->isInVRMode() && !m_beatmap->getOsu()->getVR()->isVirtualCursorOnScreen()); // a bit shit, but whatever. see OsuBeatmap::isClickHeld()
}
