//================ Copyright (c) 2015, PG & Jeffrey Han (opsu!), All rights reserved. =================//
//
// Purpose:		slider. curve classes have been taken from opsu!, albeit heavily modified
//
// $NoKeywords: $slider
//=====================================================================================================//

#include "OsuSlider.h"

#include "Engine.h"
#include "ConVar.h"
#include "AnimationHandler.h"
#include "ResourceManager.h"
#include "OpenVRInterface.h"
#include "OpenVRController.h"
#include "SoundEngine.h"

#include "Shader.h"
#include "VertexBuffer.h"
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
ConVar osu_mod_hd_slider_fade_percent("osu_mod_hd_slider_fade_percent", 0.6f);
ConVar osu_mod_hd_slider_fade("osu_mod_hd_slider_fade", true);
ConVar osu_mod_hd_slider_fast_fade("osu_mod_hd_slider_fast_fade", false);

ConVar osu_slider_break_epilepsy("osu_slider_break_epilepsy", false);
ConVar osu_slider_scorev2("osu_slider_scorev2", false);

ConVar osu_slider_draw_body("osu_slider_draw_body", true);
ConVar osu_slider_shrink("osu_slider_shrink", false);
ConVar osu_slider_reverse_arrow_black_threshold("osu_slider_reverse_arrow_black_threshold", 1.0f, "Blacken reverse arrows if the average color brightness percentage is above this value"); // looks too shitty atm

ConVar *OsuSlider::m_osu_playfield_mirror_horizontal_ref = NULL;
ConVar *OsuSlider::m_osu_playfield_mirror_vertical_ref = NULL;
ConVar *OsuSlider::m_osu_playfield_rotation_ref = NULL;
ConVar *OsuSlider::m_osu_mod_fps_ref = NULL;
ConVar *OsuSlider::m_osu_slider_border_size_multiplier_ref = NULL;
ConVar *OsuSlider::m_epilepsy = NULL;
ConVar *OsuSlider::m_osu_auto_cursordance = NULL;

float OsuSliderCurve::CURVE_POINTS_SEPERATION = 2.5f; // bigger value = less steps, more blocky sliders

OsuSlider::OsuSlider(char type, int repeat, float pixelLength, std::vector<Vector2> points, std::vector<int> hitSounds, std::vector<float> ticks, float sliderTime, float sliderTimeWithoutRepeats, long time, int sampleType, int comboNumber, int colorCounter, OsuBeatmapStandard *beatmap) : OsuHitObject(time, sampleType, comboNumber, colorCounter, beatmap)
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
	if (m_cType == SLIDER_PASSTHROUGH && m_points.size() == 3)
	{
		Vector2 nora = m_points[1] - m_points[0];
		Vector2 norb = m_points[1] - m_points[2];

		float temp = nora.x;
		nora.x = -nora.y;
		nora.y = temp;
		temp = norb.x;
		norb.x = -norb.y;
		norb.y = temp;

		if (std::abs(norb.x * nora.y - norb.y * nora.x) < 0.00001f)
			m_curve = new OsuSliderCurveLinearBezier(this, false, beatmap);  // vectors parallel, use linear bezier instead
		else
			m_curve = new OsuSliderCurveCircumscribedCircle(this, beatmap);
	}
	else if (m_cType == SLIDER_CATMULL)
		m_curve = new OsuSliderCurveCatmull(this, beatmap);
	else
		m_curve = new OsuSliderCurveLinearBezier(this, m_cType == SLIDER_LINEAR, beatmap);

	// build repeats
	for (int i=0; i<m_iRepeat-1; i++)
	{
		SLIDERCLICK sc;
		sc.finished = false;
		sc.successful = false;
		sc.type = 0;
		sc.sliderend = (i % 2) == 0; // for hit animation on repeat hit
		sc.time = m_iTime + (long)(m_fSliderTimeWithoutRepeats * (i+1));
		m_clicks.push_back(sc);
	}

	// build ticks
	for (int i=0; i<m_iRepeat; i++)
	{
		for (int t=0; t<m_ticks.size(); t++)
		{
			SLIDERCLICK sc;
			sc.time = m_iTime + (long)(m_fSliderTimeWithoutRepeats*i) + (long)(m_ticks[t].percent*m_fSliderTimeWithoutRepeats);
			sc.finished = false;
			sc.successful = false;
			sc.type = 1;
			sc.tickIndex = (i % 2) == 0 ? t : m_ticks.size()-t-1;
			m_clicks.push_back(sc);
		}
	}

	m_iObjectDuration = (long)m_fSliderTime;
	m_iObjectDuration = m_iObjectDuration >= 0 ? m_iObjectDuration : 1; // force clamp to positive range

	m_fSlidePercent = 0.0f;
	m_fActualSlidePercent = 0.0f;
	m_fHiddenAlpha = 0.0f;
	m_fHiddenSlowFadeAlpha = 0.0f;

	m_startResult = OsuScore::HIT::HIT_NULL;
	m_endResult = OsuScore::HIT::HIT_NULL;
	m_bStartFinished = false;
	m_fStartHitAnimation = 0.0f;
	m_bEndFinished = false;
	m_fEndHitAnimation = 0.0f;
	m_fEndSliderBodyFadeAnimation = 0.0f;
	m_iLastClickHeld = 0;
	m_bCursorLeft = true;
	m_bCursorInside = false;
	m_bHeldTillEnd = false;
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

	m_vb = NULL;
	m_vbVR2 = NULL;
}

OsuSlider::~OsuSlider()
{
	onReset(0);

	SAFE_DELETE(m_curve);
	SAFE_DELETE(m_vb);
	SAFE_DELETE(m_vbVR2);
}

void OsuSlider::draw(Graphics *g)
{
	if (m_points.size() <= 0) return;

	OsuSkin *skin = m_beatmap->getSkin();

	if (m_bVisible || (m_bStartFinished && !m_bFinished)) // extra possibility to avoid flicker between OsuHitObject::m_bVisible delay and the fadeout animation below this if block
	{
		float alpha = !osu_mod_hd_slider_fade.getBool() ? m_fAlpha : (osu_mod_hd_slider_fast_fade.getBool() || m_beatmap->getOsu()->getModNM() ? m_fHiddenAlpha : m_fHiddenSlowFadeAlpha);
		float sliderSnake = osu_snaking_sliders.getBool() ? clamp<float>(alpha*2.0f, m_fAlpha, 1.0f) : 1.0f;

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
		float tickImageScale = (m_beatmap->getHitcircleDiameter() / (16.0f * (skin->isSliderScorePoint2x() ? 2.0f : 1.0f)))*0.125f;
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
			if (m_fAlpha > 0.0f)
			{
				// if the combo color is nearly white, blacken the reverse arrow
				Color comboColor = skin->getComboColorForCounter(m_iColorCounter);
				Color reverseArrowColor = 0xffffffff;
				if ((COLOR_GET_Rf(comboColor) + COLOR_GET_Gf(comboColor) + COLOR_GET_Bf(comboColor))/3.0f > osu_slider_reverse_arrow_black_threshold.getFloat())
					reverseArrowColor = 0xff000000;

				float div = 0.30f;
				float pulse = (div - fmod(m_beatmap->getCurMusicPos()/1000.0f, div))/div;
				pulse *= pulse; // quadratic

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
					g->setAlpha((sliderSnake > 0.99f ? 1.0f : 0.0f)*m_fAlpha); // reverse arrows ignore hidden alpha
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
					g->setAlpha(m_fAlpha); // reverse arrows ignore hidden alpha
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

	// draw start/end circle hit animation, slider body fade animation, followcircle
	if (m_fEndSliderBodyFadeAnimation > 0.0f && m_fEndSliderBodyFadeAnimation != 1.0f && !m_beatmap->getOsu()->getModHD() && !osu_slider_shrink.getBool())
		drawBody(g, 1.0f - m_fEndSliderBodyFadeAnimation, 0, 1);

	if (m_fStartHitAnimation > 0.0f && m_fStartHitAnimation != 1.0f && !m_beatmap->getOsu()->getModHD())
	{
		float alpha = 1.0f - m_fStartHitAnimation;
		//alpha = -alpha*(alpha-2.0f); // quad out alpha

		float scale = m_fStartHitAnimation;
		scale = -scale*(scale-2.0f); // quad out scale

		bool drawNumber = (skin->getVersion() > 1.0f ? false : true) && m_iCurRepeat < 1;

		g->pushTransform();
			g->scale((1.0f+scale*OsuGameRules::osu_circle_fade_out_scale.getFloat()), (1.0f+scale*OsuGameRules::osu_circle_fade_out_scale.getFloat()));
			if (m_iCurRepeat < 1)
			{
				m_beatmap->getSkin()->getHitCircleOverlay2()->setAnimationTimeOffset(m_iTime - m_iApproachTime);
				m_beatmap->getSkin()->getSliderStartCircleOverlay2()->setAnimationTimeOffset(m_iTime - m_iApproachTime);

				OsuCircle::drawSliderStartCircle(g, m_beatmap, m_curve->pointAt(0.0f), m_iComboNumber, m_iColorCounter, 1.0f, alpha, alpha, drawNumber);
			}
			else
			{
				m_beatmap->getSkin()->getHitCircleOverlay2()->setAnimationTimeOffset(m_iTime);
				m_beatmap->getSkin()->getSliderEndCircleOverlay2()->setAnimationTimeOffset(m_iTime);

				OsuCircle::drawSliderEndCircle(g, m_beatmap, m_curve->pointAt(0.0f), m_iComboNumber, m_iColorCounter, 1.0f, alpha, alpha, drawNumber);
			}
		g->popTransform();
	}

	if (m_fEndHitAnimation > 0.0f && m_fEndHitAnimation != 1.0f && !m_beatmap->getOsu()->getModHD())
	{
		float alpha = 1.0f - m_fEndHitAnimation;
		//alpha = -alpha*(alpha-2.0f); // quad out alpha

		float scale = m_fEndHitAnimation;
		scale = -scale*(scale-2.0f); // quad out scale

		g->pushTransform();
			g->scale((1.0f+scale*OsuGameRules::osu_circle_fade_out_scale.getFloat()), (1.0f+scale*OsuGameRules::osu_circle_fade_out_scale.getFloat()));
			{
				m_beatmap->getSkin()->getHitCircleOverlay2()->setAnimationTimeOffset(m_iTime - m_iFadeInTime);
				m_beatmap->getSkin()->getSliderEndCircleOverlay2()->setAnimationTimeOffset(m_iTime - m_iFadeInTime);

				OsuCircle::drawSliderEndCircle(g, m_beatmap, m_curve->pointAt(1.0f), m_iComboNumber, m_iColorCounter, 1.0f, alpha, 0.0f, false);
			}
		g->popTransform();
	}

	OsuHitObject::draw(g);

	// debug
	/*
	if (m_bVisible)
	{
		Vector2 screenPos = m_beatmap->osuCoords2Pixels(getRawPosAt(0));

		g->setColor(0xff0000ff);
		g->pushTransform();
		g->translate(screenPos.x, screenPos.y);
		g->drawString(engine->getResourceManager()->getFont("FONT_DEFAULT"), UString::format("%li", m_iTime));
		g->popTransform();
	}
	*/
}

void OsuSlider::draw2(Graphics *g)
{
	draw2(g, true);
}

void OsuSlider::draw2(Graphics *g, bool drawApproachCircle)
{
	OsuSkin *skin = m_beatmap->getSkin();

	// HACKHACK: so much code duplication aaaaaaah
	if ((m_bVisible || (m_bStartFinished && !m_bFinished)) && drawApproachCircle) // extra possibility to avoid flicker between OsuHitObject::m_bVisible delay and the fadeout animation below this if block
	{
		float alpha = !osu_mod_hd_slider_fade.getBool() ? m_fAlpha : (osu_mod_hd_slider_fast_fade.getBool() || m_beatmap->getOsu()->getModNM() ? m_fHiddenAlpha : m_fHiddenSlowFadeAlpha);

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
				OsuCircle::drawApproachCircle(g, m_beatmap, m_curve->pointAt(0.0f), m_iComboNumber, m_iColorCounter, m_fApproachScale, alpha, m_bOverrideHDApproachCircle);
			}
		}
	}

	// draw followcircle
	// HACKHACK: this is not entirely correct (due to m_bHeldTillEnd, if held within 300 range but then released, will flash followcircle at the end)
	if ((m_bVisible && m_bCursorInside && (m_beatmap->isClickHeld() || m_beatmap->getOsu()->getModAuto() || m_beatmap->getOsu()->getModRelax())) || (m_bFinished && m_fFollowCircleAnimationAlpha > 0.0f && m_bHeldTillEnd))
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

			g->setColor(skin->getAllowSliderBallTint() ? (osu_slider_ball_tint_combo_color.getBool() ? skin->getComboColorForCounter(m_iColorCounter) : skin->getSliderBallColor()) : 0xffffffff);
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
		float alpha = !osu_mod_hd_slider_fade.getBool() ? m_fAlpha : (osu_mod_hd_slider_fast_fade.getBool() || m_beatmap->getOsu()->getModNM() ? m_fHiddenAlpha : m_fHiddenSlowFadeAlpha);
		float sliderSnake = osu_snaking_sliders.getBool() ? clamp<float>(alpha*2.0f, m_fAlpha, 1.0f) : 1.0f;

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
		float tickImageScale = (m_beatmap->getHitcircleDiameter() / (16.0f * (skin->isSliderScorePoint2x() ? 2.0f : 1.0f)))*0.125f;
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
			if (m_fAlpha > 0.0f)
			{
				// if the combo color is nearly white, blacken the reverse arrow
				Color comboColor = skin->getComboColorForCounter(m_iColorCounter);
				Color reverseArrowColor = 0xffffffff;
				if ((COLOR_GET_Rf(comboColor) + COLOR_GET_Gf(comboColor) + COLOR_GET_Bf(comboColor))/3.0f > osu_slider_reverse_arrow_black_threshold.getFloat())
					reverseArrowColor = 0xff000000;

				float div = 0.30f;
				float pulse = (div - fmod(m_beatmap->getCurMusicPos()/1000.0f, div))/div;
				pulse *= pulse; // quadratic

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
					g->setAlpha((sliderSnake > 0.99f ? 1.0f : 0.0f)*m_fAlpha); // reverse arrows ignore hidden alpha
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
					g->setAlpha(m_fAlpha); // reverse arrows ignore hidden alpha
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

	// draw start/end circle hit animation, slider body fade animation, followcircle
	/*
	if (m_fEndSliderBodyFadeAnimation > 0.0f && m_fEndSliderBodyFadeAnimation != 1.0f && !m_beatmap->getOsu()->getModHD() && !osu_slider_shrink.getBool())
	{
		drawBodyVR(g, vr, finalMVP, 1.0f - m_fEndSliderBodyFadeAnimation, 0, 1);
	}
	*/

	if (m_fStartHitAnimation > 0.0f && m_fStartHitAnimation != 1.0f && !m_beatmap->getOsu()->getModHD())
	{
		float alpha = 1.0f - m_fStartHitAnimation;
		//alpha = -alpha*(alpha-2.0f); // quad out alpha

		float scale = m_fStartHitAnimation;
		scale = -scale*(scale-2.0f); // quad out scale

		bool drawNumber = (skin->getVersion() > 1.0f ? false : true) && m_iCurRepeat < 1;

		g->pushTransform();
			g->scale((1.0f+scale*OsuGameRules::osu_circle_fade_out_scale.getFloat()), (1.0f+scale*OsuGameRules::osu_circle_fade_out_scale.getFloat()));
			if (m_iCurRepeat < 1)
			{
				m_beatmap->getSkin()->getHitCircleOverlay2()->setAnimationTimeOffset(m_iTime - m_iApproachTime);
				m_beatmap->getSkin()->getSliderStartCircleOverlay2()->setAnimationTimeOffset(m_iTime - m_iApproachTime);

				OsuCircle::drawSliderStartCircle(g, m_beatmap, m_curve->pointAt(0.0f), m_iComboNumber, m_iColorCounter, 1.0f, alpha, alpha, drawNumber);
			}
			else
			{
				m_beatmap->getSkin()->getHitCircleOverlay2()->setAnimationTimeOffset(m_iTime);
				m_beatmap->getSkin()->getSliderEndCircleOverlay2()->setAnimationTimeOffset(m_iTime);

				OsuCircle::drawSliderEndCircle(g, m_beatmap, m_curve->pointAt(0.0f), m_iComboNumber, m_iColorCounter, 1.0f, alpha, alpha, drawNumber);
			}
		g->popTransform();
	}

	if (m_fEndHitAnimation > 0.0f && m_fEndHitAnimation != 1.0f && !m_beatmap->getOsu()->getModHD())
	{
		float alpha = 1.0f - m_fEndHitAnimation;
		//alpha = -alpha*(alpha-2.0f); // quad out alpha

		float scale = m_fEndHitAnimation;
		scale = -scale*(scale-2.0f); // quad out scale

		g->pushTransform();
			g->scale((1.0f+scale*OsuGameRules::osu_circle_fade_out_scale.getFloat()), (1.0f+scale*OsuGameRules::osu_circle_fade_out_scale.getFloat()));
			{
				m_beatmap->getSkin()->getHitCircleOverlay2()->setAnimationTimeOffset(m_iTime - m_iFadeInTime);
				m_beatmap->getSkin()->getSliderEndCircleOverlay2()->setAnimationTimeOffset(m_iTime - m_iFadeInTime);

				OsuCircle::drawSliderEndCircle(g, m_beatmap, m_curve->pointAt(1.0f), m_iComboNumber, m_iColorCounter, 1.0f, alpha, 0.0f, false);
			}
		g->popTransform();
	}

	OsuHitObject::draw(g);

	draw2(g, false);
}

void OsuSlider::drawStartCircle(Graphics *g, float alpha)
{
	if (m_bStartFinished)
	{
		m_beatmap->getSkin()->getHitCircleOverlay2()->setAnimationTimeOffset(m_iTime);
		m_beatmap->getSkin()->getSliderEndCircleOverlay2()->setAnimationTimeOffset(m_iTime);

		OsuCircle::drawSliderEndCircle(g, m_beatmap, m_curve->pointAt(0.0f), m_iComboNumber, m_iColorCounter, 1.0f, m_fHiddenAlpha, 0.0f, false, false);
	}
	else
	{
		m_beatmap->getSkin()->getHitCircleOverlay2()->setAnimationTimeOffset(m_iTime - m_iApproachTime);
		m_beatmap->getSkin()->getSliderStartCircleOverlay2()->setAnimationTimeOffset(m_iTime - m_iApproachTime);

		OsuCircle::drawSliderStartCircle(g, m_beatmap, m_curve->pointAt(0.0f), m_iComboNumber, m_iColorCounter, m_fApproachScale, m_fHiddenAlpha, m_fHiddenAlpha, !m_bHideNumberAfterFirstRepeatHit, m_bOverrideHDApproachCircle);
	}
}

void OsuSlider::drawEndCircle(Graphics *g, float alpha, float sliderSnake)
{
	m_beatmap->getSkin()->getHitCircleOverlay2()->setAnimationTimeOffset(m_iTime - m_iFadeInTime);
	m_beatmap->getSkin()->getSliderEndCircleOverlay2()->setAnimationTimeOffset(m_iTime - m_iFadeInTime);

	OsuCircle::drawSliderEndCircle(g, m_beatmap, m_curve->pointAt(sliderSnake), m_iComboNumber, m_iColorCounter, 1.0f, m_fHiddenAlpha, 0.0f, false, false);
}

void OsuSlider::drawBody(Graphics *g, float alpha, float from, float to)
{
	if (m_beatmap->getOsu()->shouldFallBackToLegacySliderRenderer())
	{
		// peppy sliders
		std::vector<Vector2> screenPoints = m_curve->getPoints();
		for (int p=0; p<screenPoints.size(); p++)
		{
			screenPoints[p] = m_beatmap->osuCoords2Pixels(screenPoints[p]);
		}
		OsuSliderRenderer::draw(g, m_beatmap->getOsu(), screenPoints, m_beatmap->getHitcircleDiameter(), from, to, m_beatmap->getSkin()->getComboColorForCounter(m_iColorCounter), alpha, getTime());

		// mm sliders
		/*
		std::vector<std::vector<Vector2>> screenSegmentPoints = m_curve->getPointSegments();
		for (int s=0; s<screenSegmentPoints.size(); s++)
		{
			for (int p=0; p<screenSegmentPoints[s].size(); p++)
			{
				screenSegmentPoints[s][p] = m_beatmap->osuCoords2Pixels(screenSegmentPoints[s][p]);
			}
		}
		OsuSliderRenderer::drawMM(g, m_beatmap->getOsu(), screenSegmentPoints, m_beatmap->getHitcircleDiameter(), sliderSnakeStart, sliderSnake, skin->getComboColorForCounter(m_iColorCounter), alpha, getTime());
		*/
	}
	else
	{
		// vertex buffered sliders
		// as the base mesh is centered at (0, 0, 0) and in raw osu coordinates, we have to scale and translate it to make it fit the actual desktop playfield
		const float scale = OsuGameRules::getPlayfieldScaleFactor(m_beatmap->getOsu());
		Vector2 translation = OsuGameRules::getPlayfieldCenter(m_beatmap->getOsu());
		if (m_osu_mod_fps_ref->getBool())
			translation += m_beatmap->getFirstPersonCursorDelta();
		OsuSliderRenderer::draw(g, m_beatmap->getOsu(), m_vb, translation, scale, m_beatmap->getHitcircleDiameter(), from, to, m_beatmap->getSkin()->getComboColorForCounter(m_iColorCounter), alpha, getTime());
	}
}

void OsuSlider::drawBodyVR(Graphics *g, OsuVR *vr, Matrix4 &mvp, float alpha, float from, float to)
{
	if (m_beatmap->getOsu()->shouldFallBackToLegacySliderRenderer())
	{
		// peppy sliders
		std::vector<Vector2> screenPoints = m_curve->getPoints();
		for (int p=0; p<screenPoints.size(); p++)
		{
			screenPoints[p] = m_beatmap->osuCoords2Pixels(screenPoints[p]);
		}
		OsuSliderRenderer::drawVR(g, m_beatmap->getOsu(), vr, mvp, m_fApproachScale, screenPoints, m_beatmap->getHitcircleDiameter(), from, to, m_beatmap->getSkin()->getComboColorForCounter(m_iColorCounter), alpha, getTime());
	}
	else
	{
		// vertex buffered sliders
		OsuSliderRenderer::drawVR(g, m_beatmap->getOsu(), vr, mvp, m_fApproachScale, m_vb, m_vbVR2, m_beatmap->getHitcircleDiameter(), from, to, m_beatmap->getSkin()->getComboColorForCounter(m_iColorCounter), alpha, getTime());
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

	// hidden modifies the alpha
	m_fHiddenAlpha = m_fAlpha;
	m_fHiddenSlowFadeAlpha = m_fAlpha;
	if (m_beatmap->getOsu()->getModHD())
	{
		if (m_iDelta < m_iHiddenTimeDiff + m_iHiddenDecayTime) // fadeout
		{
			m_fHiddenAlpha = clamp<float>((m_iDelta - m_iHiddenTimeDiff) / (float) m_iHiddenDecayTime, 0.0f, 1.0f);
			m_fAlpha = 1.0f; // this is needed to ensure that sliders don't un-snake a tiny bit because of the overlap between the normal fadeintime and the hidden fadeouttime
							 // note that m_fAlpha is also used for the reverse arrow alpha, which would also flicker because of this
		}
		else if (m_iDelta < m_iHiddenTimeDiff + m_iHiddenDecayTime + m_iFadeInTime*0.8f) // fadein
			m_fHiddenAlpha = clamp<float>(1.0f - (float)(m_iDelta - m_iHiddenTimeDiff - m_iHiddenDecayTime)/(float)(m_iFadeInTime*0.8f), 0.0f, 1.0f);
		else
			m_fHiddenAlpha = 0.0f;

		m_fHiddenSlowFadeAlpha = m_fHiddenAlpha;

		// TODO: not yet correct fading, but close enough, actually not really
		if (m_iDelta < m_iHiddenTimeDiff + m_iHiddenDecayTime)
			m_fHiddenSlowFadeAlpha = 1.0f - clamp<float>((float)(curPos - (m_iTime - m_iHiddenTimeDiff - m_iHiddenDecayTime)) / (m_fSliderTime*osu_mod_hd_slider_fade_percent.getFloat()+m_iHiddenTimeDiff+m_iHiddenDecayTime), 0.0f, 1.0f);
	}

	// when playing hidden, delta may get negative early enough to show the number again before the slider starts, this fixes that
	if (m_beatmap->getOsu()->getModHD() && m_iDelta <= 0)
		m_fHiddenAlpha = 0.0f;

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
	else if (m_points.size() > 0)
	{
		m_vCurPointRaw = m_points[0];
		m_vCurPoint = m_beatmap->osuCoords2Pixels(m_vCurPointRaw);
	}

	// handle dynamic followradius
	float followRadius = m_beatmap->getSliderFollowCircleDiameter()/2.0f;
	if (m_bCursorLeft) // need to go within the circle radius to be valid again
		followRadius = m_beatmap->getHitcircleDiameter()/2.0f;
	m_bCursorInside = (m_beatmap->getOsu()->getModAuto() && (!m_osu_auto_cursordance->getBool() || ((m_beatmap->getCursorPos() - m_vCurPoint).length() < followRadius))) || ((m_beatmap->getCursorPos() - m_vCurPoint).length() < followRadius);
	if (m_bCursorInside)
		m_bCursorLeft = false;
	else
		m_bCursorLeft = true;

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
				if (curPos >= m_iTime) // TODO: there was an && m_bCursorInside there, why?
				{
					const Vector2 pos = m_beatmap->osuCoords2Pixels(m_points[0]);
					const float cursorDelta = (m_beatmap->getCursorPos() - pos).length();

					float vrCursor1Delta = 0.0f;
					float vrCursor2Delta = 0.0f;
					bool vrCursor1Inside = false;
					bool vrCursor2Inside = false;
					if (m_beatmap->getOsu()->isInVRMode())
					{
						vrCursor1Delta = (m_beatmap->getOsu()->getVR()->getCursorPos1() - m_beatmap->osuCoords2VRPixels(m_points[0])).length();
						vrCursor2Delta = (m_beatmap->getOsu()->getVR()->getCursorPos2() - m_beatmap->osuCoords2VRPixels(m_points[0])).length();
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
		if ((m_beatmap->isClickHeld() || m_beatmap->getOsu()->getModRelax()) && m_bCursorInside)
			m_iLastClickHeld = curPos;
		else
			m_bCursorLeft = true; // do not allow empty clicks outside of the circle radius to prevent the m_bCursorInside flag from resetting
	
		// handle repeats and ticks
		for (int i=0; i<m_clicks.size(); i++)
		{
			if (!m_clicks[i].finished && curPos >= m_clicks[i].time)
			{
				m_clicks[i].finished = true;
				m_clicks[i].successful = (m_beatmap->isClickHeld() && m_bCursorInside) || m_beatmap->getOsu()->getModAuto() || (m_beatmap->getOsu()->getModRelax() && m_bCursorInside);

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
				long holdDelta = (long)m_iLastClickHeld - (long)curPos;
				m_endResult = OsuGameRules::getHitResult(holdDelta, m_beatmap);

				// slider lenience: only allow HIT_300 delta
				if (m_endResult != OsuScore::HIT::HIT_300)
					m_endResult = OsuScore::HIT::HIT_MISS;
				else
					m_bHeldTillEnd = true;

				if ((m_beatmap->isClickHeld() || m_beatmap->getOsu()->getModRelax()) && m_bCursorInside)
					m_endResult = OsuScore::HIT::HIT_300;


				if (m_endResult == OsuScore::HIT::HIT_NULL) // this may happen
					m_endResult = OsuScore::HIT::HIT_MISS;
				if (m_startResult == OsuScore::HIT::HIT_NULL) // this may also happen
					m_startResult = OsuScore::HIT::HIT_MISS;


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

				bool allow300 = osu_slider_scorev2.getBool() ? (m_startResult == OsuScore::HIT::HIT_300) : true;
				bool allow100 = osu_slider_scorev2.getBool() ? (m_startResult == OsuScore::HIT::HIT_300 || m_startResult == OsuScore::HIT::HIT_100) : true;

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
		m_curve->updateStackPosition(m_iStack * stackOffset);
}

Vector2 OsuSlider::getRawPosAt(long pos)
{
	if (m_curve == NULL)
		return Vector2(0, 0);

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
	if (m_curve == NULL)
		return Vector2(0, 0);

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
	if (m_points.size() == 0 || m_bBlocked) // also handle note blocking here (doesn't need fancy shake logic, since sliders don't shake)
		return;

	if (!m_bStartFinished)
	{
		const Vector2 cursorPos = m_beatmap->getCursorPos();

		const Vector2 pos = m_beatmap->osuCoords2Pixels(m_points[0]);
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
	if (m_points.size() == 0)
		return;

	// start + end of a slider add +30 points, if successful

	//debugLog("startOrEnd = %i,    m_iCurRepeat = %i\n", (int)startOrEnd, m_iCurRepeat);

	// sound and hit animation
	if (result == OsuScore::HIT::HIT_MISS)
		onSliderBreak();
	else
	{
		m_beatmap->getSkin()->playHitCircleSound(m_iCurRepeatCounterForHitSounds < m_hitSounds.size() ? m_hitSounds[m_iCurRepeatCounterForHitSounds] : m_iSampleType);

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
		anim->moveQuadOut(&m_fEndSliderBodyFadeAnimation, 1.0f, OsuGameRules::getFadeOutTime(m_beatmap), true);
	}

	m_iCurRepeatCounterForHitSounds++;
}

void OsuSlider::onRepeatHit(bool successful, bool sliderend)
{
	if (m_points.size() == 0)
		return;

	// repeat hit of a slider adds +30 points, if successful

	// sound and hit animation
	if (!successful)
		onSliderBreak();
	else
	{
		m_fFollowCircleTickAnimationScale = 0.0f;
		anim->moveLinear(&m_fFollowCircleTickAnimationScale, 1.0f, OsuGameRules::osu_slider_followcircle_tick_pulse_time.getFloat(), true);

		m_beatmap->getSkin()->playHitCircleSound(m_iCurRepeatCounterForHitSounds < m_hitSounds.size() ? m_hitSounds[m_iCurRepeatCounterForHitSounds] : m_iSampleType);
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
	if (m_points.size() == 0)
		return;

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
		m_fFollowCircleTickAnimationScale = 0.0f;
		anim->moveLinear(&m_fFollowCircleTickAnimationScale, 1.0f, OsuGameRules::osu_slider_followcircle_tick_pulse_time.getFloat(), true);
		m_beatmap->getSkin()->playSliderTickSound();
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
	m_bCursorLeft = true;
	m_bHeldTillEnd = false;
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
	SAFE_DELETE(m_vb);
	m_vb = OsuSliderRenderer::generateSliderVertexBuffer(m_beatmap->getOsu(), osuCoordPoints, m_beatmap->getRawHitcircleDiameter());

	if (m_beatmap->getOsu()->isInVRMode())
	{
		// body mesh (foreground)
		// HACKHACK: magic numbers from shader
		const float defaultBorderSize = 0.11;
		const float defaultTransitionSize = 0.011f;
		const float defaultOuterShadowSize = 0.08f;
		const float scale = 1.0f - m_osu_slider_border_size_multiplier_ref->getFloat()*defaultBorderSize - defaultTransitionSize - defaultOuterShadowSize;
		SAFE_DELETE(m_vbVR2);
		m_vbVR2 = OsuSliderRenderer::generateSliderVertexBuffer(m_beatmap->getOsu(), osuCoordPoints, m_beatmap->getRawHitcircleDiameter()*scale, Vector3(0, 0, 0.25f)); // push 0.25f outwards
	}
}



//**********//
//	Curves	//
//**********//

OsuSliderCurve::OsuSliderCurve(OsuSlider *parent, OsuBeatmap *beatmap)
{
	m_slider = parent;
	m_beatmap = beatmap;

	m_points = m_slider->getRawPoints();

	m_fStartAngle = 0.0f;
	m_fEndAngle = 0.0f;
}

void OsuSliderCurve::updateStackPosition(float stackMulStackOffset)
{
	for (int i=0; i<m_originalCurvePoints.size() && i<m_curvePoints.size(); i++)
	{
		m_curvePoints[i] = m_originalCurvePoints[i] - Vector2(stackMulStackOffset, stackMulStackOffset * (m_beatmap->getOsu()->getModHR() ? -1.0f : 1.0f));
	}

	for (int s=0; s<m_originalCurvePointSegments.size() && s<m_curvePointSegments.size(); s++)
	{
		for (int p=0; p<m_originalCurvePointSegments[s].size() && p<m_curvePointSegments[s].size(); p++)
		{
			m_curvePointSegments[s][p] = m_originalCurvePointSegments[s][p] - Vector2(stackMulStackOffset, stackMulStackOffset * (m_beatmap->getOsu()->getModHR() ? -1.0f : 1.0f));
		}
	}
}



OsuSliderCurveType::OsuSliderCurveType()
{
	m_iNCurve = 0;
	m_fTotalDistance = 0.0f;
}

void OsuSliderCurveType::init(float approxlength)
{
	// subdivide the curve
	m_iNCurve = (int)(approxlength / 4.0f) + 2;
	for (int i=0; i<m_iNCurve; i++)
	{
		m_points.push_back( pointAt(i / (float) (m_iNCurve - 1) ) );
	}

	// find the distance of each point from the previous point (needed for some curve types)
	m_fTotalDistance = 0.0f;
	for (int i=0; i<m_iNCurve; i++)
	{
		float curDist = (i == 0) ? 0 : (m_points[i] - m_points[i-1]).length();

		m_curveDistances.push_back(curDist);
		m_fTotalDistance += curDist;
	}
}

void OsuSliderCurveType::initCustom(std::vector<Vector2> points)
{
	m_points = points;
	m_iNCurve = points.size();

	// find the distance of each point from the previous point (needed for some curve types)
	m_fTotalDistance = 0.0f;
	for (int i=0; i<m_iNCurve; i++)
	{
		float curDist = (i == 0) ? 0 : (m_points[i] - m_points[i-1]).length();

		m_curveDistances.push_back(curDist);
		m_fTotalDistance += curDist;
	}
}



OsuSliderCurveTypeBezier2::OsuSliderCurveTypeBezier2(const std::vector<Vector2> &points) : OsuSliderCurveType()
{
	m_points2 = points;

	// approximate by finding the length of all points
	// (which should be the max possible length of the curve)
	float approxlength = 0;
	for (int i=0; i<points.size()-1; i++)
	{
		approxlength += (points[i] - points[i + 1]).length();
	}
	init(approxlength);

	/*
	BezierApproximator b(points);
	initCustom(b.createBezier());
	*/
}

Vector2 OsuSliderCurveTypeBezier2::pointAt(float t)
{
	Vector2 c;
	int n = m_points2.size() - 1;

	for (int i=0; i<=n; i++)
	{
		double b = bernstein(i, n, t);
		c.x += m_points2[i].x * b;
		c.y += m_points2[i].y * b;
	}
	return c;
}

long OsuSliderCurveTypeBezier2::binomialCoefficient(int n, int k)
{
	if (k < 0 || k > n)
		return 0;
	if (k == 0 || k == n)
		return 1;
	k = std::min(k, n - k);  // take advantage of symmetry
	long c = 1;
	for (int i = 0; i < k; i++)
		c = c * (n - i) / (i + 1);
	return c;
}

double OsuSliderCurveTypeBezier2::bernstein(int i, int n, float t)
{
	return binomialCoefficient(n, i) * std::pow(t, i) * std::pow(1 - t, n - i);
}

OsuSliderCurveTypeCentripetalCatmullRom::OsuSliderCurveTypeCentripetalCatmullRom(const std::vector<Vector2> &points) : OsuSliderCurveType()
{
	if (points.size() != 4)
	{
		debugLog("OsuSliderCurveTypeCentripetalCatmullRom() Error: points.size() != 4!!!\n");
		return;
	}

	m_points2 = points;
	m_time[0] = 0.0f;
	float approxLength = 0;

	for (int i=1; i<4; i++)
	{
		float len = 0;

		if (i > 0)
			len = (points[i] - points[i - 1]).length();

		if (len <= 0)
			len += 0.0001f;

		approxLength += len;

		// m_time[i] = (float) std::sqrt(len) + time[i-1];// ^(0.5)
		m_time[i] = i;
	}

	init(approxLength / 2.0f);
}

Vector2 OsuSliderCurveTypeCentripetalCatmullRom::pointAt(float t)
{
	t = t * (m_time[2] - m_time[1]) + m_time[1];

	Vector2 A1 = m_points2[0]*((m_time[1] - t) / (m_time[1] - m_time[0]))
		  +(m_points2[1]*((t - m_time[0]) / (m_time[1] - m_time[0])));
	Vector2 A2 = m_points2[1]*((m_time[2] - t) / (m_time[2] - m_time[1]))
		  +(m_points2[2]*((t - m_time[1]) / (m_time[2] - m_time[1])));
	Vector2 A3 = m_points2[2]*((m_time[3] - t) / (m_time[3] - m_time[2]))
		  +(m_points2[3]*((t - m_time[2]) / (m_time[3] - m_time[2])));

	Vector2 B1 = A1*((m_time[2] - t) / (m_time[2] - m_time[0]))
		  +(A2*((t - m_time[0]) / (m_time[2] - m_time[0])));
	Vector2 B2 = A2*((m_time[3] - t) / (m_time[3] - m_time[1]))
		  +(A3*((t - m_time[1]) / (m_time[3] - m_time[1])));

	Vector2 C = B1*((m_time[2] - t) / (m_time[2] - m_time[1]))
		 +(B2*((t - m_time[1]) / (m_time[2] - m_time[1])));

	return C;
}



OsuSliderCurveLinearBezier::OsuSliderCurveLinearBezier(OsuSlider *parent, bool line, OsuBeatmap *beatmap) : OsuSliderCurveEqualDistanceMulti(parent, beatmap)
{
	std::vector<OsuSliderCurveType*> beziers;

	// Beziers: splits points into different Beziers if has the same points (red points)
	// a b c - c d - d e f g
	// Lines: generate a new curve for each sequential pair
	// ab  bc  cd  de  ef  fg
	int controlPoints = m_points.size();
	std::vector<Vector2> points;  // temporary list of points to separate different Bezier curves
	Vector2 lastPoi(-1, -1);
	for (int i=0; i<controlPoints; i++)
	{
		Vector2 tpoi = m_points[i];
		if (line)
		{
			if (lastPoi != Vector2(-1,-1))
			{
				points.push_back(tpoi);
				beziers.push_back(new OsuSliderCurveTypeBezier2(points));
				points.clear();
			}
		}
		else if (lastPoi != Vector2(-1,-1) && tpoi == lastPoi)
		{
			if (points.size() >= 2)
				beziers.push_back(new OsuSliderCurveTypeBezier2(points));

			points.clear();
		}
		points.push_back(tpoi);
		lastPoi = tpoi;
	}
	if (line || points.size() < 2)
	{
		// trying to continue Bezier with less than 2 points
		// probably ending on a red point, just ignore it
	}
	else
	{
		beziers.push_back(new OsuSliderCurveTypeBezier2(points));
		points.clear();
	}

	init(beziers);

	for (int i=0; i<beziers.size(); i++)
	{
		delete beziers[i];
	}
}

OsuSliderCurveCatmull::OsuSliderCurveCatmull(OsuSlider *parent, OsuBeatmap *beatmap) : OsuSliderCurveEqualDistanceMulti(parent, beatmap)
{
	std::vector<OsuSliderCurveType*> catmulls;
	int ncontrolPoints = m_points.size();
	std::vector<Vector2> points; // temporary list of points to separate different curves

	// repeat the first and last points as controls points
	// only if the first/last two points are different
	// aabb
	// aabc abcc
	// aabc abcd bcdd
	if (m_points[0].x != m_points[1].x || m_points[0].y != m_points[1].y)
		points.push_back(m_points[0]);

	for (int i=0; i<ncontrolPoints; i++)
	{
		points.push_back(m_points[i]);
		if (points.size() >= 4)
		{
			catmulls.push_back(new OsuSliderCurveTypeCentripetalCatmullRom(points));
			points.erase(points.begin());
		}
	}

	if (m_points[ncontrolPoints - 1].x != m_points[ncontrolPoints - 2].x || m_points[ncontrolPoints - 1].y != m_points[ncontrolPoints - 2].y)
		points.push_back(m_points[ncontrolPoints - 1]);

	if (points.size() >= 4)
		catmulls.push_back(new OsuSliderCurveTypeCentripetalCatmullRom(points));

	init(catmulls);

	for (int i=0; i<catmulls.size(); i++)
	{
		delete catmulls[i];
	}
}

OsuSliderCurveCircumscribedCircle::OsuSliderCurveCircumscribedCircle(OsuSlider *parent, OsuBeatmap *beatmap) : OsuSliderCurve(parent, beatmap)
{
	// construct the three points
	Vector2 start = parent->getRawPoints()[0];
	Vector2 mid = parent->getRawPoints()[1];
	Vector2 end = parent->getRawPoints()[2];

	// find the circle center
	Vector2 mida = start + (mid-start)*0.5f;
	Vector2 midb = end + (mid-end)*0.5f;
	Vector2 nora = mid - start;
	float temp = nora.x;
	nora.x = -nora.y;
	nora.y = temp;
	Vector2 norb = mid - end;
	temp = norb.x;
	norb.x = -norb.y;
	norb.y = temp;

	m_vOriginalCircleCenter = intersect(mida, nora, midb, norb);
	m_vCircleCenter = m_vOriginalCircleCenter;

	// find the angles relative to the circle center
	Vector2 startAngPoint = start - m_vCircleCenter;
	Vector2 midAngPoint   = mid - m_vCircleCenter;
	Vector2 endAngPoint   = end - m_vCircleCenter;

	m_fCalculationStartAngle = (float) atan2(startAngPoint.y, startAngPoint.x);
	float midAng   = (float) atan2(midAngPoint.y, midAngPoint.x);
	m_fCalculationEndAngle   = (float) atan2(endAngPoint.y, endAngPoint.x);

	// find the angles that pass through midAng
	if (!isIn(m_fCalculationStartAngle, midAng, m_fCalculationEndAngle))
	{
		if (std::abs(m_fCalculationStartAngle + 2*PI - m_fCalculationEndAngle) < 2*PI && isIn(m_fCalculationStartAngle + (2*PI), midAng, m_fCalculationEndAngle))
			m_fCalculationStartAngle += 2*PI;
		else if (std::abs(m_fCalculationStartAngle - (m_fCalculationEndAngle + 2*PI)) < 2*PI && isIn(m_fCalculationStartAngle, midAng, m_fCalculationEndAngle + (2*PI)))
			m_fCalculationEndAngle += 2*PI;
		else if (std::abs(m_fCalculationStartAngle - 2*PI - m_fCalculationEndAngle) < 2*PI && isIn(m_fCalculationStartAngle - (2*PI), midAng, m_fCalculationEndAngle))
			m_fCalculationStartAngle -= 2*PI;
		else if (std::abs(m_fCalculationStartAngle - (m_fCalculationEndAngle - 2*PI)) < 2*PI && isIn(m_fCalculationStartAngle, midAng, m_fCalculationEndAngle - (2*PI)))
			m_fCalculationEndAngle -= 2*PI;
		else
		{
			debugLog("OsuSliderCurveCircumscribedCircle() Error: Cannot find angles between midAng (%.3f %.3f %.3f).", m_fCalculationStartAngle, midAng, m_fCalculationEndAngle);
			return;
		}
	}

	// find an angle with an arc length of pixelLength along this circle
	m_fRadius = startAngPoint.length();
	float pixelLength = m_slider->getPixelLength();
	float arcAng = pixelLength / m_fRadius;  // len = theta * r / theta = len / r

	// now use it for our new end angle
	m_fCalculationEndAngle = (m_fCalculationEndAngle > m_fCalculationStartAngle) ? m_fCalculationStartAngle + arcAng : m_fCalculationStartAngle - arcAng;

	// finds the angles to draw for repeats
	m_fEndAngle   = (float) ((m_fCalculationEndAngle   + (m_fCalculationStartAngle > m_fCalculationEndAngle ? PI/2.0f : -PI/2.0f)) * 180 / PI);
	m_fStartAngle = (float) ((m_fCalculationStartAngle + (m_fCalculationStartAngle > m_fCalculationEndAngle ? -PI/2.0f : PI/2.0f)) * 180 / PI);

	// calculate points
	float step = m_slider->getPixelLength() / CURVE_POINTS_SEPERATION;
	int intStep = (int)std::round(step)+1; // must guarantee an int range of 0 to step
	for (int i=0; i<(int)intStep+1; i++)
	{
		float t = clamp<float>(i/step, 0.0f, 1.0f);
		m_curvePoints.push_back(pointAt(t));

		if (t >= 1.0f)
			break;
	}

	// add the segment (no special logic here for SliderCurveCircumscribedCircle, just add the entire vector)
	m_curvePointSegments.push_back(m_curvePoints);

	// backup (for dynamic updateStackPosition() recalculation)
	m_originalCurvePoints = m_curvePoints;
	m_originalCurvePointSegments = m_curvePointSegments;
}

void OsuSliderCurveCircumscribedCircle::updateStackPosition(float stackMulStackOffset)
{
	OsuSliderCurve::updateStackPosition(stackMulStackOffset);

	m_vCircleCenter = m_vOriginalCircleCenter - Vector2(stackMulStackOffset, stackMulStackOffset * (m_beatmap->getOsu()->getModHR() ? -1.0f : 1.0f));
}

Vector2 OsuSliderCurveCircumscribedCircle::pointAt(float t)
{
	float ang = lerp(m_fCalculationStartAngle, m_fCalculationEndAngle, t);

	return Vector2(std::cos(ang) * m_fRadius + m_vCircleCenter.x,
				   std::sin(ang) * m_fRadius + m_vCircleCenter.y);
}

Vector2 OsuSliderCurveCircumscribedCircle::originalPointAt(float t)
{
	float ang = lerp(m_fCalculationStartAngle, m_fCalculationEndAngle, t);

	return Vector2(std::cos(ang) * m_fRadius + m_vOriginalCircleCenter.x,
				   std::sin(ang) * m_fRadius + m_vOriginalCircleCenter.y);
}

Vector2 OsuSliderCurveCircumscribedCircle::intersect(Vector2 a, Vector2 ta, Vector2 b, Vector2 tb)
{
	float des = tb.x * ta.y - tb.y * ta.x;
	if (std::abs(des) < 0.0001f)
	{
		debugLog("OsuSliderCurveCircumscribedCircle::intersect() Error: Vectors are parallel!!!\n");
		return Vector2(0,0);
	}

	float u = ((b.y - a.y) * ta.x + (a.x - b.x) * ta.y) / des;
	return b + Vector2(tb.x * u, tb.y * u);
}

bool OsuSliderCurveCircumscribedCircle::isIn(float a, float b, float c)
{
	return (b > a && b < c) || (b < a && b > c);
}



OsuSliderCurveEqualDistanceMulti::OsuSliderCurveEqualDistanceMulti(OsuSlider *parent, OsuBeatmap *beatmap) : OsuSliderCurve(parent, beatmap)
{
	m_iNCurve = (int) (m_slider->getPixelLength() / CURVE_POINTS_SEPERATION);
}

void OsuSliderCurveEqualDistanceMulti::init(std::vector<OsuSliderCurveType*> curvesList)
{
	if (curvesList.size() == 0)
	{
		debugLog("OsuSliderCurveEqualDistanceMulti::init() Error: curvesList.size() == 0!!!\n");
		return;
	}

	float distanceAt = 0;
	int curPoint = 0;
	int curCurveIndex = 0;
	OsuSliderCurveType *curCurve = curvesList[curCurveIndex];
	Vector2 lastCurve = curCurve->getCurvePoints()[0];
	float lastDistanceAt = 0;

	// length of the curve should be equal to the pixel length
	float pixelLength = m_slider->getPixelLength();

	// for each distance, try to get in between the two points that are between it
	Vector2 lastCurvePointForNextSegmentStart;
	std::vector<Vector2> curCurvePoints;
	for (int i=0; i<m_iNCurve+1; i++)
	{
		int prefDistance = (int) ((i * pixelLength) / m_iNCurve);
		while (distanceAt < prefDistance)
		{
			lastDistanceAt = distanceAt;
			lastCurve = curCurve->getCurvePoints()[curPoint];
			curPoint++;

			if (curPoint >= curCurve->getCurvesCount())
			{
				// jump to next curve
				curCurveIndex++;

				// when jumping to the next curve, add the current segment to the list of segments
				if (curCurvePoints.size() > 0)
				{
					m_curvePointSegments.push_back(curCurvePoints);
					curCurvePoints.clear();

					// prepare the next segment by setting/adding the starting point to the exact end point of the previous segment
					// this also enables an optimization, namely that startcaps only have to be drawn [for every segment] if the startpoint != endpoint in the loop
					if (m_curvePoints.size() > 0)
						curCurvePoints.push_back(lastCurvePointForNextSegmentStart);
				}

				if (curCurveIndex < curvesList.size())
				{
					curCurve = curvesList[curCurveIndex];
					curPoint = 0;
				}
				else
				{
					curPoint = curCurve->getCurvesCount() - 1;
					if (lastDistanceAt == distanceAt)
					{
						// out of points even though the preferred distance hasn't been reached
						break;
					}
				}
			}
			distanceAt += curCurve->getCurveDistances()[curPoint];
		}
		Vector2 thisCurve = curCurve->getCurvePoints()[curPoint];

		// TODO: there's some major fuckery going on with the indices here, from the raw opsu curve code translation. fix that sometime

		// interpolate the point between the two closest distances
		m_curvePoints.push_back(Vector2(0,0));
		curCurvePoints.push_back(Vector2(0,0));
		if (distanceAt - lastDistanceAt > 1)
		{
			float t = (prefDistance - lastDistanceAt) / (distanceAt - lastDistanceAt);
			m_curvePoints[i] = Vector2(lerp<float>(lastCurve.x, thisCurve.x, t), lerp<float>(lastCurve.y, thisCurve.y, t));
		}
		else
			m_curvePoints[i] = thisCurve;

		// add the point to the current segment (this is not using the lerp'd point! would cause mm mesh artifacts if it did)
		lastCurvePointForNextSegmentStart = thisCurve;
		curCurvePoints[curCurvePoints.size()-1] = thisCurve;
	}

	// if we only had one segment, no jump to any next curve has occurred (and therefore no insertion of the segment into the vector)
	// manually add the lone segment here
	if (curCurvePoints.size() > 0)
		m_curvePointSegments.push_back(curCurvePoints);

	// sanity check
	if (m_curvePoints.size() == 0)
	{
		debugLog("OsuSliderCurveEqualDistanceMulti::init() Error: m_curvePoints.size() == 0!!! @ %ld\n", m_slider->getTime());
		return;
	}

	// make sure that the uninterpolated segment points are exactly as long as the pixelLength
	// this is necessary because we can't use the lerp'd points for the segments
	float segmentedLength = 0.0f;
	for (int s=0; s<m_curvePointSegments.size(); s++)
	{
		for (int p=0; p<m_curvePointSegments[s].size(); p++)
		{
			segmentedLength += (p == 0) ? 0 : (m_curvePointSegments[s][p] - m_curvePointSegments[s][p-1]).length();
		}
	}
	// TODO: this is still incorrect. sliders are sometimes too long or start too late
	if (segmentedLength > pixelLength && m_curvePointSegments.size() > 1 && m_curvePointSegments[0].size() > 1)
	{
		float excess = segmentedLength - pixelLength;
		while (excess > 0)
		{
			for (int s=m_curvePointSegments.size()-1; s>=0; s--)
			{
				for (int p=m_curvePointSegments[s].size()-1; p>=0; p--)
				{
					const float curLength = (p == 0) ? 0 : (m_curvePointSegments[s][p] - m_curvePointSegments[s][p-1]).length();
					if (curLength >= excess)
					{
						Vector2 segmentVector = (m_curvePointSegments[s][p] - m_curvePointSegments[s][p-1]).normalize();
						m_curvePointSegments[s][p] = m_curvePointSegments[s][p] - segmentVector*excess;
						excess = 0.0f;
						break;
					}
					else
					{
						m_curvePointSegments[s].erase(m_curvePointSegments[s].begin() + p);
						excess -= curLength;
					}
				}
			}
		}
	}

	// calculate start and end angles for possible repeats (good enough and cheaper than calculating it live every frame)
	Vector2 c1 = m_curvePoints[0];
	int cnt = 1;
	Vector2 c2 = m_curvePoints[cnt++];
	while (cnt <= m_iNCurve && (c2-c1).length() < 1)
	{
		c2 = m_curvePoints[cnt++];
	}
	m_fStartAngle = (float) (atan2(c2.y - c1.y, c2.x - c1.x) * 180 / PI);

	c1 = m_curvePoints[m_iNCurve];
	cnt = m_iNCurve - 1;
	c2 = m_curvePoints[cnt--];
	while (cnt >= 0 && (c2-c1).length() < 1)
	{
		c2 = m_curvePoints[cnt--];
	}
	m_fEndAngle = (float) (atan2(c2.y - c1.y, c2.x - c1.x) * 180 / PI);

	// backup (for dynamic updateStackPosition() recalculation)
	m_originalCurvePoints = m_curvePoints;
	m_originalCurvePointSegments = m_curvePointSegments;
}

Vector2 OsuSliderCurveEqualDistanceMulti::pointAt(float t)
{
	if (m_curvePoints.size() < 1) // this might happen
		return Vector2(0,0);

	float indexF = t * m_iNCurve;
	int index = (int) indexF;
	if (index >= m_iNCurve)
		return m_curvePoints[m_iNCurve];
	else
	{
		Vector2 poi = m_curvePoints[index];
		Vector2 poi2 = m_curvePoints[index + 1];
		float t2 = indexF - index;
		return Vector2(lerp(poi.x, poi2.x, t2), lerp(poi.y, poi2.y, t2));
	}
}

Vector2 OsuSliderCurveEqualDistanceMulti::originalPointAt(float t)
{
	if (m_originalCurvePoints.size() < 1) // this might happen
		return Vector2(0,0);

	float indexF = t * m_iNCurve;
	int index = (int) indexF;
	if (index >= m_iNCurve)
		return m_originalCurvePoints[m_iNCurve];
	else
	{
		Vector2 poi = m_originalCurvePoints[index];
		Vector2 poi2 = m_originalCurvePoints[index + 1];
		float t2 = indexF - index;
		return Vector2(lerp(poi.x, poi2.x, t2), lerp(poi.y, poi2.y, t2));
	}
}



/*
float BezierApproximator::TOLERANCE = 0.25f;
float BezierApproximator::TOLERANCE_SQ = 0.25f * 0.25f;

BezierApproximator::BezierApproximator(std::vector<Vector2> controlPoints)
{
	m_controlPoints = controlPoints;
	m_iCount = m_controlPoints.size();

	m_subdivisionBuffer1.resize(m_iCount);
	m_subdivisionBuffer2.resize(m_iCount*2 - 1);
}

bool BezierApproximator::isFlatEnough(std::vector<Vector2> controlPoints)
{
    for (int i=1; i<controlPoints.size() - 1; i++)
    {
        if (std::pow((controlPoints[i - 1] - 2 * controlPoints[i] + controlPoints[i + 1]).length(), 2.0f) > TOLERANCE_SQ * 4)
            return false;
    }

    return true;
}

void BezierApproximator::subdivide(std::vector<Vector2> &controlPoints, std::vector<Vector2> &l, std::vector<Vector2> &r)
{
	std::vector<Vector2> &midpoints = m_subdivisionBuffer1;

    for (int i=0; i<m_iCount; ++i)
    {
        midpoints[i] = controlPoints[i];
    }

    for (int i=0; i<m_iCount; i++)
    {
        l[i] = midpoints[0];
        r[m_iCount - i - 1] = midpoints[m_iCount - i - 1];

        for (int j=0; j<m_iCount-i-1; j++)
        {
            midpoints[j] = (midpoints[j] + midpoints[j + 1]) / 2;
        }
    }
}

void BezierApproximator::approximate(std::vector<Vector2> &controlPoints, std::vector<Vector2> &output)
{
    std::vector<Vector2> &l = m_subdivisionBuffer2;
    std::vector<Vector2> &r = m_subdivisionBuffer1;

    subdivide(controlPoints, l, r);

    for (int i=0; i<m_iCount-1; ++i)
        l[m_iCount + i] = r[i + 1];

    output.push_back(controlPoints[0]);
    for (int i=1; i<m_iCount-1; ++i)
    {
        int index = 2 * i;
        Vector2 p = 0.25f * (l[index - 1] + 2 * l[index] + l[index + 1]);
        output.push_back(p);
    }
}

std::vector<Vector2> BezierApproximator::createBezier()
{
	std::vector<Vector2> output;

	if (m_iCount == 0)
		return output;

	std::stack<std::vector<Vector2>> toFlatten;
	std::stack<std::vector<Vector2>> freeBuffers;

	toFlatten.push(m_controlPoints);

	std::vector<Vector2> &leftChild = m_subdivisionBuffer2;

	while (toFlatten.size() > 0)
	{
		std::vector<Vector2> parent = toFlatten.top();
		toFlatten.pop();

		if (isFlatEnough(parent))
		{
			approximate(parent, output);
			freeBuffers.push(parent);
			continue;
		}

		std::vector<Vector2> rightChild;
		if (freeBuffers.size() > 0)
		{
			rightChild = freeBuffers.top();
			freeBuffers.pop();
		}
		else
			rightChild.resize(m_iCount);
        subdivide(parent, leftChild, rightChild);

        for (int i=0; i<m_iCount; ++i)
        {
            parent[i] = leftChild[i];
        }

        toFlatten.push(rightChild);
        toFlatten.push(parent);
	}

	output.push_back(m_controlPoints[m_iCount - 1]);
	return output;
}
*/
