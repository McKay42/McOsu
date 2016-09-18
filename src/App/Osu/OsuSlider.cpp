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
#include "SoundEngine.h"

#include "Shader.h"
#include "VertexArrayObject.h"
#include "RenderTarget.h"

#include "Osu.h"
#include "OsuCircle.h"
#include "OsuSkin.h"
#include "OsuGameRules.h"

ConVar osu_slider_ball_tint_combo_color("osu_slider_ball_tint_combo_color", true);
ConVar osu_slider_body_color_saturation("osu_slider_body_color_saturation", 1.0f);
ConVar osu_slider_body_alpha_multiplier("osu_slider_body_alpha_multiplier", 1.0f);

ConVar osu_snaking_sliders("osu_snaking_sliders", true);
ConVar osu_mod_hd_slider_fade_percent("osu_mod_hd_slider_fade_percent", 0.6f);
ConVar osu_mod_hd_slider_fade("osu_mod_hd_slider_fade", true);
ConVar osu_mod_hd_slider_fast_fade("osu_mod_hd_slider_fast_fade", false);

ConVar osu_slider_break_epilepsy("osu_slider_break_epilepsy", false);
ConVar osu_slider_rainbow("osu_slider_rainbow", false);
ConVar osu_slider_debug("osu_slider_debug", false);
ConVar osu_slider_scorev2("osu_slider_scorev2", false);

ConVar osu_slider_draw_body("osu_slider_draw_body", true);
ConVar osu_slider_draw_endcircle("osu_slider_draw_endcircle", true);
ConVar osu_slider_shrink("osu_slider_shrink", false);
ConVar osu_slider_reverse_arrow_black_threshold("osu_slider_reverse_arrow_black_threshold", 1.0f, "Blacken reverse arrows if the average color brightness percentage is above this value"); // looks too shitty atm

ConVar *OsuSlider::m_osu_playfield_mirror_horizontal_ref = NULL;
ConVar *OsuSlider::m_osu_playfield_mirror_vertical_ref = NULL;
ConVar *OsuSlider::m_osu_playfield_rotation_ref = NULL;

Shader *OsuSliderCurve::BLEND_SHADER = NULL;
float OsuSliderCurve::CURVE_POINTS_SEPERATION = 2.5f; // bigger value = less steps, more blocky sliders
int OsuSliderCurve::UNIT_CONE_DIVIDES = 42; // can probably lower this a little bit, but not under 32
std::vector<float> OsuSliderCurve::UNIT_CONE;
VertexArrayObject *OsuSliderCurve::MASTER_CIRCLE_VAO = NULL;
float OsuSliderCurve::MASTER_CIRCLE_VAO_RADIUS = 0.0f;

OsuSlider::OsuSlider(char type, int repeat, float pixelLength, std::vector<Vector2> points, std::vector<float> ticks, float sliderTime, float sliderTimeWithoutRepeats, long time, int sampleType, int comboNumber, int colorCounter, OsuBeatmap *beatmap) : OsuHitObject(time, sampleType, comboNumber, colorCounter, beatmap)
{
	if (m_osu_playfield_mirror_horizontal_ref == NULL)
		m_osu_playfield_mirror_horizontal_ref = convar->getConVarByName("osu_playfield_mirror_horizontal");
	if (m_osu_playfield_mirror_vertical_ref == NULL)
		m_osu_playfield_mirror_vertical_ref = convar->getConVarByName("osu_playfield_mirror_vertical");
	if (m_osu_playfield_rotation_ref == NULL)
		m_osu_playfield_rotation_ref = convar->getConVarByName("osu_playfield_rotation");

	m_cType = type;
	m_iRepeat = repeat;
	m_fPixelLength = pixelLength;
	m_points = points;
	m_fSliderTime = sliderTime;
	m_fSliderTimeWithoutRepeats = sliderTimeWithoutRepeats;

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

	m_fSlidePercent = 0.0f;
	m_fActualSlidePercent = 0.0f;
	m_fHiddenAlpha = 0.0f;
	m_fHiddenSlowFadeAlpha = 0.0f;

	m_startResult = OsuScore::HIT_NULL;
	m_endResult = OsuScore::HIT_NULL;
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
	m_bInReverse = false;
	m_bHideNumberAfterFirstRepeatHit = false;

	m_fSliderBreakRapeTime = 0.0f;
}

OsuSlider::~OsuSlider()
{
	onReset(0);
	SAFE_DELETE(m_curve);
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
		if (alpha > 0.0f)
			m_curve->draw(g, skin->getComboColorForCounter(m_iColorCounter), alpha, sliderSnake, sliderSnakeStart);

		// draw slider ticks
		// HACKHACK: hardcoded 0.125 multiplier, seems to be correct though (1/8)
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
			if (osu_slider_draw_endcircle.getBool() && ((!m_bEndFinished && m_iRepeat % 2 != 0) || !sliderRepeatEndCircleFinished))
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
					/*Vector2 pos = m_beatmap->osuCoords2Pixels(m_curve->pointAt(sliderSnake));*/
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
		m_curve->draw(g, m_beatmap->getSkin()->getComboColorForCounter(m_iColorCounter), 1.0f - m_fEndSliderBodyFadeAnimation);

	if (m_fStartHitAnimation > 0.0f && m_fStartHitAnimation != 1.0f && !m_beatmap->getOsu()->getModHD())
	{
		float alpha = 1.0f - m_fStartHitAnimation;
		//alpha = -alpha*(alpha-2.0f); // quad out alpha

		float scale = m_fStartHitAnimation;
		scale = -scale*(scale-2.0f); // quad out scale

		bool drawNumber = (skin->getVersion() > 1.0f ? false : true) && m_iCurRepeat < 1;

		g->pushTransform();
			g->scale((1.0f+scale*OsuGameRules::osu_circle_fade_out_scale.getFloat()), (1.0f+scale*OsuGameRules::osu_circle_fade_out_scale.getFloat()));

			// explanation for this is in drawStartCircle()
			if (skin->getSliderStartCircle() != skin->getMissingTexture() && m_iCurRepeat < 1)
				OsuCircle::drawSliderStartCircle(g, m_beatmap, m_curve->pointAt(0.0f), m_iComboNumber, m_iColorCounter, 1.0f, alpha, alpha, drawNumber);
			else if (skin->getSliderEndCircle() != skin->getMissingTexture() && m_iCurRepeat > 0)
				OsuCircle::drawSliderEndCircle(g, m_beatmap, m_curve->pointAt(0.0f), m_iComboNumber, m_iColorCounter, 1.0f, alpha, alpha, drawNumber);
			else
				OsuCircle::drawCircle(g, m_beatmap, m_curve->pointAt(0.0f), m_iComboNumber, m_iColorCounter, 1.0f, alpha, alpha, drawNumber);
		g->popTransform();
	}

	if (osu_slider_draw_endcircle.getBool() && m_fEndHitAnimation > 0.0f && m_fEndHitAnimation != 1.0f && !m_beatmap->getOsu()->getModHD())
	{
		float alpha = 1.0f - m_fEndHitAnimation;
		//alpha = -alpha*(alpha-2.0f); // quad out alpha

		float scale = m_fEndHitAnimation;
		scale = -scale*(scale-2.0f); // quad out scale

		g->pushTransform();
			g->scale((1.0f+scale*OsuGameRules::osu_circle_fade_out_scale.getFloat()), (1.0f+scale*OsuGameRules::osu_circle_fade_out_scale.getFloat()));
			if (skin->getSliderEndCircle() != skin->getMissingTexture())
				OsuCircle::drawSliderEndCircle(g, m_beatmap, m_curve->pointAt(1.0f), m_iComboNumber, m_iColorCounter, 1.0f, alpha, 0.0f, false);
			else
				OsuCircle::drawCircle(g, m_beatmap, m_curve->pointAt(1.0f), m_iComboNumber, m_iColorCounter, 1.0f, alpha, 0.0f, false);
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
	OsuSkin *skin = m_beatmap->getSkin();

	// HACKHACK: so much code duplication aaaaaaah
	if (m_bVisible || (m_bStartFinished && !m_bFinished)) // extra possibility to avoid flicker between OsuHitObject::m_bVisible delay and the fadeout animation below this if block
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
		g->pushTransform();
		g->scale(m_beatmap->getSliderFollowCircleScale()*tickAnimationScale*m_fFollowCircleAnimationScale, m_beatmap->getSliderFollowCircleScale()*tickAnimationScale*m_fFollowCircleAnimationScale);
		g->translate(point.x, point.y);
		g->drawImage(skin->getSliderFollowCircle());
		g->popTransform();
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

			float sliderbImageScale = m_beatmap->getHitcircleDiameter() / (float) skin->getSliderb()->getWidth();

			g->setColor(skin->getAllowSliderBallTint() ? (osu_slider_ball_tint_combo_color.getBool() ? skin->getComboColorForCounter(m_iColorCounter) : skin->getSliderBallColor()) : 0xffffffff);
			g->pushTransform();
			g->scale(sliderbImageScale, sliderbImageScale);
			g->rotate(ballAngle);
			g->translate(point.x, point.y);
			g->drawImage(skin->getSliderb());
			g->popTransform();
		}
	}
}

void OsuSlider::drawStartCircle(Graphics *g, float alpha)
{
	// if sliderendcircle exists, and we are past the first circle, only draw sliderendcircle on both start and end from then on
	// if sliderstartcircle exists, use it only until the end of the first circle
	if (m_beatmap->getSkin()->getSliderEndCircle() != m_beatmap->getSkin()->getMissingTexture() && m_bStartFinished)
		OsuCircle::drawSliderEndCircle(g, m_beatmap, m_curve->pointAt(0.0f), m_iComboNumber, m_iColorCounter, 1.0f, alpha, 0.0f, false, false);
	else if (m_beatmap->getSkin()->getSliderStartCircle() != m_beatmap->getSkin()->getMissingTexture() && !m_bStartFinished)
		OsuCircle::drawSliderStartCircle(g, m_beatmap, m_curve->pointAt(0.0f), m_iComboNumber, m_iColorCounter, m_fApproachScale, alpha, m_fHiddenAlpha, !m_bHideNumberAfterFirstRepeatHit, m_bOverrideHDApproachCircle);
	else
		OsuCircle::drawCircle(g, m_beatmap, m_curve->pointAt(0.0f), m_iComboNumber, m_iColorCounter, m_fApproachScale, alpha, m_fHiddenAlpha, !m_bHideNumberAfterFirstRepeatHit, m_bOverrideHDApproachCircle); // normal
}

void OsuSlider::drawEndCircle(Graphics *g, float alpha, float sliderSnake)
{
	if (m_beatmap->getSkin()->getSliderEndCircle() != m_beatmap->getSkin()->getMissingTexture())
		OsuCircle::drawSliderEndCircle(g, m_beatmap, m_curve->pointAt(sliderSnake), m_iComboNumber, m_iColorCounter, 1.0f, alpha, 0.0f, false, false);
	else
		OsuCircle::drawCircle(g, m_beatmap, m_curve->pointAt(sliderSnake), m_iComboNumber, m_iColorCounter, 1.0f, alpha, 0.0f, false, false); // normal
}

void OsuSlider::update(long curPos)
{
	OsuHitObject::update(curPos);

	//TEMP:
	if (m_fSliderBreakRapeTime != 0.0f && engine->getTime() > m_fSliderBreakRapeTime)
	{
		m_fSliderBreakRapeTime = 0.0f;
		convar->getConVarByName("epilepsy")->setValue(0.0f);
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

		// TODO: not yet correct fading, but close enough
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
	m_bCursorInside = m_beatmap->getOsu()->getModAuto() || (m_beatmap->getCursorPos() - m_vCurPoint).length() < followRadius;
	if (m_bCursorInside)
		m_bCursorLeft = false;
	else
		m_bCursorLeft = true;

	// handle slider start
	if (!m_bStartFinished)
	{
		if (m_beatmap->getOsu()->getModAuto())
		{
			if (curPos >= m_iTime)
				onHit(OsuScore::HIT_300, 0, false);
		}
		else
		{
			if (m_beatmap->getOsu()->getModRelax())
			{
				if (curPos >= m_iTime && m_bCursorInside)
				{
					const Vector2 pos = m_beatmap->osuCoords2Pixels(m_points[0]);
					const float cursorDelta = (m_beatmap->getCursorPos() - pos).length();

					if (cursorDelta < m_beatmap->getHitcircleDiameter()/2.0f)
					{
						long delta = curPos - (long)m_iTime;
						OsuScore::HIT result = OsuGameRules::getHitResult(delta, m_beatmap);

						if (result != OsuScore::HIT_NULL)
						{
							const float targetDelta = cursorDelta / (m_beatmap->getHitcircleDiameter()/2.0f);
							const float targetAngle = rad2deg(atan2(m_beatmap->getCursorPos().y - pos.y, m_beatmap->getCursorPos().x - pos.x));

							m_startResult = result;
							onHit(m_startResult, delta, false, targetDelta, targetAngle);
						}
					}
				}
			}

			// wait for a miss
			long delta = (long)curPos - (long)m_iTime;
			if (delta >= 0)
			{
				// if this is a miss after waiting
				if (delta > (long)OsuGameRules::getHitWindow50(m_beatmap))
				{
					m_startResult = OsuScore::HIT_MISS;
					onHit(m_startResult, delta, false);
				}
			}
		}
	}

	// handle slider end, repeats, ticks
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
				onHit(OsuScore::HIT_300, 0, true);
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
				if (m_endResult != OsuScore::HIT_300)
					m_endResult = OsuScore::HIT_MISS;
				else
					m_bHeldTillEnd = true;

				if ((m_beatmap->isClickHeld() || m_beatmap->getOsu()->getModRelax()) && m_bCursorInside)
					m_endResult = OsuScore::HIT_300;


				if (m_endResult == OsuScore::HIT_NULL) // this may happen
					m_endResult = OsuScore::HIT_MISS;
				if (m_startResult == OsuScore::HIT_NULL) // this may also happen
					m_startResult = OsuScore::HIT_MISS;


				// handle total slider result (currently startcircle + repeats + ticks + endcircle)
				// clicks = (repeats + ticks)
				float numMaxPossibleHits = 1 + m_clicks.size() + 1;
				float numActualHits = 0;

				if (m_startResult != OsuScore::HIT_MISS)
					numActualHits++;
				if (m_endResult != OsuScore::HIT_MISS)
					numActualHits++;

				for (int i=0; i<m_clicks.size(); i++)
				{
					if (m_clicks[i].successful)
						numActualHits++;
				}

				float percent = numActualHits / numMaxPossibleHits;

				bool allow300 = osu_slider_scorev2.getBool() ? (m_startResult == OsuScore::HIT_300) : true;
				bool allow100 = osu_slider_scorev2.getBool() ? (m_startResult == OsuScore::HIT_300 || m_startResult == OsuScore::HIT_100) : true;

				if (percent >= 0.999f && allow300)
					m_endResult = OsuScore::HIT_300;
				else if (percent >= 0.5f && allow100 && !OsuGameRules::osu_mod_ming3012.getBool())
					m_endResult = OsuScore::HIT_100;
				else if (percent > 0.0f)
					m_endResult = OsuScore::HIT_50;
				else
					m_endResult = OsuScore::HIT_MISS;

				//debugLog("percent = %f\n", percent);

				onHit(m_endResult, 0, true); // delta doesn't matter here
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
		float floorVal = (float) floor(t);
		return ((int)floorVal % 2 == 0) ? t - floorVal : floorVal + 1 - t;
	}
}

void OsuSlider::onClickEvent(Vector2 cursorPos, std::vector<OsuBeatmap::CLICK> &clicks)
{
	if (m_points.size() == 0)
		return;

	if (!m_bStartFinished)
	{
		const Vector2 pos = m_beatmap->osuCoords2Pixels(m_points[0]);
		const float cursorDelta = (cursorPos - pos).length();

		if (cursorDelta < m_beatmap->getHitcircleDiameter()/2.0f)
		{
			const long delta = (long)clicks[0].musicPos - (long)m_iTime;

			OsuScore::HIT result = OsuGameRules::getHitResult(delta, m_beatmap);
			if (result != OsuScore::HIT_NULL)
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
	if (result == OsuScore::HIT_MISS)
		onSliderBreak();
	else
	{
		m_beatmap->getSkin()->playHitCircleSound(m_iSampleType);

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

		m_beatmap->getSkin()->playHitCircleSound(m_iSampleType);
		m_beatmap->addHitResult(OsuScore::HIT_300, 0, true);

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

		// add score
		m_beatmap->addScorePoints(30);
	}
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
		engine->getSound()->play(m_beatmap->getSkin()->getSliderTick());
		m_beatmap->addHitResult(OsuScore::HIT_SLIDER30, 0, true);

		// add score
		m_beatmap->addScorePoints(10);
	}
}

void OsuSlider::onSliderBreak()
{
	m_beatmap->addSliderBreak();

	//TEMP:
	if (osu_slider_break_epilepsy.getBool())
	{
		m_fSliderBreakRapeTime = engine->getTime() + 0.15f;
		convar->getConVarByName("epilepsy")->setValue(1.0f);
	}
}

void OsuSlider::onReset(long curPos)
{
	OsuHitObject::onReset(curPos);

	m_iLastClickHeld = 0;
	m_bCursorLeft = true;
	m_bHeldTillEnd = false;
	m_startResult = OsuScore::HIT_NULL;
	m_endResult = OsuScore::HIT_NULL;

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
	convar->getConVarByName("epilepsy")->setValue(0.0f);
}



//**********//
//	Curves	//
//**********//

OsuSliderCurve::OsuSliderCurve(OsuSlider *parent, OsuBeatmap *beatmap)
{
	// static globals
	if (BLEND_SHADER == NULL)
	{
		// build shader
		BLEND_SHADER = new Shader("slider.vsh", "slider.fsh");

		// build unit cone
		{
			// tip of the cone
			// texture coordinates
			UNIT_CONE.push_back(1.0f);
			UNIT_CONE.push_back(0.0f);

			// position
			UNIT_CONE.push_back(0.0f);
			UNIT_CONE.push_back(0.0f);
			UNIT_CONE.push_back(0.0f);

			for (int j=0; j<UNIT_CONE_DIVIDES; ++j)
			{
				float phase = j * (float) PI * 2.0f / UNIT_CONE_DIVIDES;

				// texture coordinates
				UNIT_CONE.push_back(0.0f);
				UNIT_CONE.push_back(0.0f);

				// positon
				UNIT_CONE.push_back((float)std::sin(phase));
				UNIT_CONE.push_back((float)std::cos(phase));
				UNIT_CONE.push_back(-1.0f);
			}

			// texture coordinates
			UNIT_CONE.push_back(0.0f);
			UNIT_CONE.push_back(0.0f);

			// positon
			UNIT_CONE.push_back((float)std::sin(0.0f));
			UNIT_CONE.push_back((float)std::cos(0.0f));
			UNIT_CONE.push_back(-1.0f);
		}
	}
	if (MASTER_CIRCLE_VAO == NULL)
	{
		MASTER_CIRCLE_VAO = new VertexArrayObject();
		MASTER_CIRCLE_VAO->setType(VertexArrayObject::TYPE_TRIANGLE_FAN);
	}



	m_slider = parent;
	m_beatmap = beatmap;

	m_points = m_slider->getRawPoints();

	m_fStartAngle = 0.0f;
	m_fEndAngle = 0.0f;

	// rendering optimization
	m_fBoundingBoxMinX = 0.0f;
	m_fBoundingBoxMaxX = 0.0f;
	m_fBoundingBoxMinY = 0.0f;
	m_fBoundingBoxMaxY = 0.0f;
}

OsuSliderCurve::~OsuSliderCurve()
{
}

void OsuSliderCurve::draw(Graphics *g, Color color, float alpha)
{
	draw(g, color, alpha, 1.0f);
}

void OsuSliderCurve::draw(Graphics *g, Color color, float alpha, float t, float t2)
{
	if (!osu_slider_draw_body.getBool())
		return;
	if (osu_slider_body_alpha_multiplier.getFloat() <= 0.0f || alpha <= 0.0f)
		return;

	OsuSkin *skin = m_beatmap->getSkin();

	t = clamp<float>(t, 0.0f, 1.0f);
	int drawFrom = clamp<int>((int)std::round(m_curvePoints.size() * t2), 0, m_curvePoints.size());
	int drawUpTo = clamp<int>((int)std::round(m_curvePoints.size() * t), 0, m_curvePoints.size());

	// debug sliders
	float circleImageScale = m_beatmap->getHitcircleDiameter() / (float) skin->getHitCircle()->getWidth();
	if (osu_slider_debug.getBool())
	{
		g->setColor(color);
		g->setAlpha(alpha);
		for (int i=drawFrom; i<drawUpTo; i++)
		{
			VertexArrayObject vao;
			vao.setType(VertexArrayObject::TYPE_QUADS);
			Vector2 point = m_beatmap->osuCoords2Pixels(m_curvePoints[i]);

			int width = skin->getHitCircle()->getWidth();
			int height = skin->getHitCircle()->getHeight();

			g->pushTransform();
				g->scale(circleImageScale, circleImageScale);
				g->translate(point.x, point.y);

				int x = -width/2;
				int y = -height/2;

				vao.addTexcoord(0, 0);
				vao.addVertex(x, y, -1.0f);

				vao.addTexcoord(0, 1);
				vao.addVertex(x, y+height, -1.0f);

				vao.addTexcoord(1, 1);
				vao.addVertex(x+width, y+height, -1.0f);

				vao.addTexcoord(1, 0);
				vao.addVertex(x+width, y, -1.0f);

				skin->getHitCircle()->bind();
				g->drawVAO(&vao);
				skin->getHitCircle()->unbind();
			g->popTransform();
		}
		return; // nothing more to draw here
	}

	// rendering optimization reset
	m_fBoundingBoxMinX = std::numeric_limits<float>::max();
	m_fBoundingBoxMaxX = 0.0f;
	m_fBoundingBoxMinY = std::numeric_limits<float>::max();
	m_fBoundingBoxMaxY = 0.0f;

	// draw entire slider into framebuffer
	g->setDepthBuffer(true);
	g->setBlending(false);
	{
		m_beatmap->getOsu()->getFrameBuffer()->enable();

		Color borderColor = skin->getSliderBorderColor();
		Color bodyColor = m_beatmap->getOsu()->getSkin()->isSliderTrackOverridden() ? skin->getSliderTrackOverride() : color;

		if (osu_slider_rainbow.getBool())
		{
			float frequency = 0.3f;
			float time = engine->getTime()*20;

			char red1	= std::sin(frequency*time + 0 + m_slider->getTime()) * 127 + 128;
			char green1	= std::sin(frequency*time + 2 + m_slider->getTime()) * 127 + 128;
			char blue1	= std::sin(frequency*time + 4 + m_slider->getTime()) * 127 + 128;

			char red2	= std::sin(frequency*time*1.5f + 0 + m_slider->getTime()) * 127 + 128;
			char green2	= std::sin(frequency*time*1.5f + 2 + m_slider->getTime()) * 127 + 128;
			char blue2	= std::sin(frequency*time*1.5f + 4 + m_slider->getTime()) * 127 + 128;

			borderColor = COLOR(255, red1, green1, blue1);
			bodyColor = COLOR(255, red2, green2, blue2);
		}

		///float amplitudeMultiplier = 1.0f - (1.0f - m_beatmap->getAmplitude())*(1.0f - m_beatmap->getAmplitude());

		BLEND_SHADER->enable();
		BLEND_SHADER->setUniform1f("bodyColorSaturation", osu_slider_body_color_saturation.getFloat());
		BLEND_SHADER->setUniform3f("col_border", COLOR_GET_Rf(borderColor), COLOR_GET_Gf(borderColor), COLOR_GET_Bf(borderColor));
		BLEND_SHADER->setUniform3f("col_body", COLOR_GET_Rf(bodyColor)/* + COLOR_GET_Rf(color)*amplitudeMultiplier*/, COLOR_GET_Gf(bodyColor)/* + COLOR_GET_Gf(color)*amplitudeMultiplier*/, COLOR_GET_Bf(bodyColor)/* + COLOR_GET_Bf(color)*amplitudeMultiplier*/);

		g->setColor(0xffffffff);
		g->setAlpha(alpha*osu_slider_body_alpha_multiplier.getFloat());
		skin->getSliderGradient()->bind();
		drawFillSliderBody2(g, drawUpTo, drawFrom);

		BLEND_SHADER->disable();

		m_beatmap->getOsu()->getFrameBuffer()->disable();
	}
	g->setBlending(true);
	g->setDepthBuffer(false);

	// now draw the slider to the screen (with alpha blending enabled again)
	int pixelFudge = 2;
	m_fBoundingBoxMinX -= pixelFudge;
	m_fBoundingBoxMaxX += pixelFudge;
	m_fBoundingBoxMinY -= pixelFudge;
	m_fBoundingBoxMaxY += pixelFudge;
	m_beatmap->getOsu()->getFrameBuffer()->drawRect(g, m_fBoundingBoxMinX, m_fBoundingBoxMinY, m_fBoundingBoxMaxX - m_fBoundingBoxMinX, m_fBoundingBoxMaxY - m_fBoundingBoxMinY);
	///m_beatmap->getOsu()->getFrameBuffer()->draw(g, 0, 0);

	//g->setColor(0xffffffff);
	//g->drawRect(m_fBoundingBoxMinX, m_fBoundingBoxMinY, m_fBoundingBoxMaxX - m_fBoundingBoxMinX, m_fBoundingBoxMaxY - m_fBoundingBoxMinY);
}

void OsuSliderCurve::drawFillSliderBody(Graphics *g, int drawUpTo)
{
	// draw slider body half 1
	{
		VertexArrayObject vao;
		Vector2 prevVertex1;
		Vector2 prevVertex2;

		for (int i=0; i<drawUpTo; i++)
		{
			Vector2 current = m_beatmap->osuCoords2Pixels(m_curvePoints[i]);
			float angle;

			if (i == 0)
			{
				angle = (PI - deg2rad(getStartAngle())) + PI;
				if (m_beatmap->getOsu()->getModHR())
					angle = -angle;
			}
			else if (i == m_curvePoints.size()-1)
			{
				angle = PI - deg2rad(getEndAngle());
				if (m_beatmap->getOsu()->getModHR())
					angle = -angle;
			}
			else if (i > 0 && i < m_curvePoints.size()-1)
			{
				Vector2 last = m_beatmap->osuCoords2Pixels(m_curvePoints[i-1]);
				Vector2 next = m_beatmap->osuCoords2Pixels(m_curvePoints[i+1]);

				Vector2 normal1 = (last - current).normalize();
				Vector2 normal2 = (current - next).normalize();
				Vector2 normal = (normal1 + normal2).normalize();

				angle = atan2(normal.x, normal.y)+PI*0.5;
			}

			float offset = m_beatmap->getHitcircleDiameter()*0.5f;

			Vector2 dir = Vector2(std::sin(angle), std::cos(angle))*offset;

			Vector2 pos1 = current + dir; // outer
			Vector2 pos2 = current; // inner

			if (i > 0)
			{
				vao.addTexcoord(0, 0);
				vao.addVertex(prevVertex1);
				vao.addTexcoord(1, 0);
				vao.addVertex(prevVertex2);
				vao.addTexcoord(0, 0);
				vao.addVertex(pos1);

				vao.addTexcoord(1, 0);
				vao.addVertex(prevVertex2);
				vao.addTexcoord(0, 0);
				vao.addVertex(pos1);
				vao.addTexcoord(1, 0);
				vao.addVertex(pos2);
			}

			// save
			prevVertex1 = pos1;
			prevVertex2 = pos2;
		}

		// draw it
		g->drawVAO(&vao);
	}

	// draw slider body half 2
	{
		VertexArrayObject vao;
		Vector2 prevVertex1;
		Vector2 prevVertex2;

		for (int i=0; i<drawUpTo; i++)
		{
			Vector2 current = m_beatmap->osuCoords2Pixels(m_curvePoints[i]);
			float angle;

			if (i == 0)
			{
				angle = (PI - deg2rad(getStartAngle())) + PI;
				if (m_beatmap->getOsu()->getModHR())
					angle = -angle;
			}
			else if (i == m_curvePoints.size()-1)
			{
				angle = PI - deg2rad(getEndAngle());
				if (m_beatmap->getOsu()->getModHR())
					angle = -angle;
			}
			else if (i > 0 && i < m_curvePoints.size()-1)
			{
				Vector2 last = m_beatmap->osuCoords2Pixels(m_curvePoints[i-1]);
				Vector2 next = m_beatmap->osuCoords2Pixels(m_curvePoints[i+1]);

				Vector2 normal1 = (last - current).normalize();
				Vector2 normal2 = (current - next).normalize();
				Vector2 normal = (normal1 + normal2).normalize();

				angle = atan2(normal.x, normal.y)+PI*0.5;
			}
			float offset = m_beatmap->getHitcircleDiameter()*0.5f;

			Vector2 dir = Vector2(std::sin(angle), std::cos(angle))*offset;

			Vector2 pos1 = current; // outer
			Vector2 pos2 = current - dir; // inner

			if (i > 0)
			{
				vao.addTexcoord(1, 0);
				vao.addVertex(prevVertex1);
				vao.addTexcoord(0, 0);
				vao.addVertex(prevVertex2);
				vao.addTexcoord(1, 0);
				vao.addVertex(pos1);

				vao.addTexcoord(0, 0);
				vao.addVertex(prevVertex2);
				vao.addTexcoord(1, 0);
				vao.addVertex(pos1);
				vao.addTexcoord(0, 0);
				vao.addVertex(pos2);
			}

			// save
			prevVertex1 = pos1;
			prevVertex2 = pos2;
		}

		// draw it
		g->drawVAO(&vao);
	}
}

void OsuSliderCurve::drawFillSliderBody2(Graphics *g, int drawUpTo, int drawFrom)
{
	// generate master circle vao (centered) if the size changed
	float radius = m_beatmap->getHitcircleDiameter()/2.0f;
	if (radius != MASTER_CIRCLE_VAO_RADIUS)
	{
		MASTER_CIRCLE_VAO_RADIUS = radius;
		MASTER_CIRCLE_VAO->clear();
		for (int i=0; i<UNIT_CONE.size()/5; i++)
		{
			MASTER_CIRCLE_VAO->addVertex(Vector3((radius * UNIT_CONE[i * 5 + 2]), (radius * UNIT_CONE[i * 5 + 3]), UNIT_CONE[i * 5 + 4]));
			MASTER_CIRCLE_VAO->addTexcoord(Vector2(UNIT_CONE[i * 5 + 0], UNIT_CONE[i * 5 + 1]));
		}
	}

	g->pushTransform();

	// now, translate and draw the master vao for every curve step
	float startX = 0.0f;
	float startY = 0.0f;
	for (int i=drawFrom; i<drawUpTo; ++i)
	{
		float x = m_beatmap->osuCoords2Pixels(m_curvePoints[i]).x;
		float y = m_beatmap->osuCoords2Pixels(m_curvePoints[i]).y;

		g->translate(x-startX, y-startY, 0);
		g->drawVAO(MASTER_CIRCLE_VAO);

		startX = x;
		startY = y;

		if (x-radius < m_fBoundingBoxMinX)
			m_fBoundingBoxMinX = x-radius;
		if (x+radius > m_fBoundingBoxMaxX)
			m_fBoundingBoxMaxX = x+radius;
		if (y-radius < m_fBoundingBoxMinY)
			m_fBoundingBoxMinY = y-radius;
		if (y+radius > m_fBoundingBoxMaxY)
			m_fBoundingBoxMaxY = y+radius;
	}

	g->popTransform();
}

void OsuSliderCurve::drawFillCircle(Graphics *g, Vector2 center)
{
	float radius = (m_beatmap->getHitcircleDiameter()/2.0f)*0.9f;
	float num_segments = 15*8;

	float theta = 2 * PI / float(num_segments);
	float c = cosf(theta); // precalculate the sine and cosine
	float s = sinf(theta);
	float t;
	float x = 0;
	float y = -radius; // we start at the top

	{
		VertexArrayObject vao;
		Vector2 prevVertex;
		for (int i=0; i<num_segments+1; i++)
		{
			// build current vertex
			Vector2 curVertex = Vector2(x + center.x, y + center.y);

			// add vertex, triangle strip style (counter clockwise)
			if (i > 0)
			{
				vao.addVertex(curVertex);
				vao.addVertex(prevVertex);
				vao.addVertex(center);
			}

			// apply the rotation matrix
			t = x;
			x = c * x - s * y;
			y = s * t + c * y;

			// save
			prevVertex = curVertex;
		}

		// draw it
		g->drawVAO(&vao);
	}
}

void OsuSliderCurve::updateStackPosition(float stackMulStackOffset)
{
	for (int i=0; i<m_originalCurvePoints.size() && i<m_curvePoints.size(); i++)
	{
		m_curvePoints[i] = m_originalCurvePoints[i] - Vector2(stackMulStackOffset, stackMulStackOffset * (m_beatmap->getOsu()->getModHR() ? -1.0f : 1.0f));
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




OsuSliderCurveTypeBezier2::OsuSliderCurveTypeBezier2(std::vector<Vector2> points)
{
	m_points = points;

	// approximate by finding the length of all points
	// (which should be the max possible length of the curve)
	float approxlength = 0;
	for (int i=0; i<points.size()-1; i++)
	{
		approxlength += (points[i] - points[i + 1]).length();
	}

	init(approxlength);
}

Vector2 OsuSliderCurveTypeBezier2::pointAt(float t)
{
	Vector2 c;
	int n = m_points.size() - 1;

	for (int i=0; i<=n; i++)
	{
		double b = bernstein(i, n, t);
		c.x += m_points[i].x * b;
		c.y += m_points[i].y * b;
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

OsuSliderCurveTypeCentripetalCatmullRom::OsuSliderCurveTypeCentripetalCatmullRom(std::vector<Vector2> points)
{
	if (points.size() != 4)
	{
		debugLog("OsuSliderCurveTypeCentripetalCatmullRom() Error: points.size() != 4!!!\n");
		return;
	}

	m_points = points;
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

	Vector2 A1 = m_points[0]*((m_time[1] - t) / (m_time[1] - m_time[0]))
		  +(m_points[1]*((t - m_time[0]) / (m_time[1] - m_time[0])));
	Vector2 A2 = m_points[1]*((m_time[2] - t) / (m_time[2] - m_time[1]))
		  +(m_points[2]*((t - m_time[1]) / (m_time[2] - m_time[1])));
	Vector2 A3 = m_points[2]*((m_time[3] - t) / (m_time[3] - m_time[2]))
		  +(m_points[3]*((t - m_time[2]) / (m_time[3] - m_time[2])));

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

	m_originalCurvePoints = m_curvePoints; // backup
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

	// length of Curve should equal pixel length (in 640x480)
	float pixelLength = m_slider->getPixelLength();

	// for each distance, try to get in between the two points that are between it
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
				curCurveIndex++;
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

		// interpolate the point between the two closest distances
		m_curvePoints.push_back(Vector2(0,0));
		if (distanceAt - lastDistanceAt > 1)
		{
			float t = (prefDistance - lastDistanceAt) / (distanceAt - lastDistanceAt);
			m_curvePoints[i] = Vector2(lerp<float>(lastCurve.x, thisCurve.x, t), lerp<float>(lastCurve.y, thisCurve.y, t));
		}
		else
			m_curvePoints[i] = thisCurve;
	}

	// calculate start and end angles for possible repeats
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

	m_originalCurvePoints = m_curvePoints; // backup
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
