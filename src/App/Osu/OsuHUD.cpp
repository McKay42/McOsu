//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		hud element drawing functions (accuracy, combo, score, etc.)
//
// $NoKeywords: $osuhud
//===============================================================================//

#include "OsuHUD.h"

#include "Engine.h"
#include "ConVar.h"
#include "ResourceManager.h"
#include "AnimationHandler.h"
#include "VertexArrayObject.h"

#include "Osu.h"
#include "OsuSkin.h"
#include "OsuBeatmap.h"
#include "OsuGameRules.h"

#include "OsuHitObject.h"
#include "OsuCircle.h"

#define NUM_FPS_AVG 35

ConVar osu_cursor_scale("osu_cursor_scale", 1.29f);
ConVar osu_cursor_expand_scale_multiplier("osu_cursor_expand_scale_multiplier", 1.3f);
ConVar osu_cursor_expand_duration("osu_cursor_expand_duration", 0.1f);
ConVar osu_cursor_trail_length("osu_cursor_trail_length", 0.17f);
ConVar osu_cursor_trail_spacing("osu_cursor_trail_spacing", 0.015f);
ConVar osu_cursor_trail_alpha("osu_cursor_trail_alpha", 1.0f);

ConVar osu_hud_scale("osu_hud_scale", 1.0f);
ConVar osu_hud_playfield_border_size("osu_hud_playfield_border_size", 5.0f);

ConVar osu_draw_cursor_trail("osu_draw_cursor_trail", true);
ConVar osu_draw_hud("osu_draw_hud", true);
ConVar osu_draw_hiterrorbar("osu_draw_hiterrorbar", true);
ConVar osu_draw_progressbar("osu_draw_progressbar", true);
ConVar osu_draw_combo("osu_draw_combo", true);
ConVar osu_draw_accuracy("osu_draw_accuracy", true);
ConVar osu_draw_statistics("osu_draw_statistics", false);
ConVar osu_draw_target_heatmap("osu_draw_target_heatmap", true);

ConVar osu_combo_anim1_duration("osu_combo_anim1_duration", 0.15f);
ConVar osu_combo_anim1_size("osu_combo_anim1_size", 0.15f);
ConVar osu_combo_anim2_duration("osu_combo_anim2_duration", 0.4f);
ConVar osu_combo_anim2_size("osu_combo_anim2_size", 0.5f);

OsuHUD::OsuHUD(Osu *osu)
{
	m_osu = osu;

	m_host_timescale_ref = convar->getConVarByName("host_timescale");
	m_osu_volume_master_ref = convar->getConVarByName("osu_volume_master");
	m_osu_mod_target_300_percent_ref = convar->getConVarByName("osu_mod_target_300_percent");
	m_osu_mod_target_100_percent_ref = convar->getConVarByName("osu_mod_target_100_percent");
	m_osu_mod_target_50_percent_ref = convar->getConVarByName("osu_mod_target_50_percent");

	m_tempFont = engine->getResourceManager()->getFont("FONT_DEFAULT");

	m_fCurFps = 60.0f;
	m_fFpsUpdate = 0.0f;
	m_fFpsFontHeight = m_tempFont->getHeight();

	m_fAccuracyXOffset = 0.0f;
	m_fAccuracyYOffset = 0.0f;

	m_fComboAnim1 = 0.0f;
	m_fComboAnim2 = 0.0f;

	m_iFpsAvgIndex = 0;
	m_fpsAvg = new float[NUM_FPS_AVG];
	for (int i=0; i<NUM_FPS_AVG; i++)
	{
		m_fpsAvg[i] = 0.0f;
	}

	m_fVolumeChangeTime = 0.0f;
	m_fVolumeChangeFade = 1.0f;
	m_fLastVolume = m_osu_volume_master_ref->getFloat();

	m_fCursorExpandAnim = 1.0f;
}

OsuHUD::~OsuHUD()
{
	delete[] m_fpsAvg;
}

void OsuHUD::draw(Graphics *g)
{
	if (osu_draw_hud.getBool())
	{
		if (m_osu->getBeatmap()->isInSkippableSection())
			drawSkip(g);

		if (osu_draw_statistics.getBool())
			drawStatistics(g, m_osu->getBeatmap()->getNPS(), m_osu->getBeatmap()->getND());

		if (osu_draw_combo.getBool())
			drawCombo(g, m_osu->getBeatmap()->getCombo());

		if (osu_draw_progressbar.getBool())
			drawProgressBar(g, m_osu->getBeatmap()->getPercentFinished(), m_osu->getBeatmap()->isWaiting());

		if (osu_draw_accuracy.getBool())
			drawAccuracy(g, m_osu->getBeatmap()->getAccuracy()*100.0f);

		if (osu_draw_hiterrorbar.getBool() && !m_osu->getBeatmap()->isSpinnerActive())
			drawHitErrorBar(g, OsuGameRules::getHitWindow300(m_osu->getBeatmap()), OsuGameRules::getHitWindow100(m_osu->getBeatmap()), OsuGameRules::getHitWindow50(m_osu->getBeatmap()));

		if (m_osu->getModTarget() && osu_draw_target_heatmap.getBool())
			drawTargetHeatmap(g, m_osu->getBeatmap()->getHitcircleDiameter());
	}

	if (m_osu->getBeatmap()->shouldFlashWarningArrows())
		drawWarningArrows(g, m_osu->getBeatmap()->getHitcircleDiameter());

	if (m_osu->getBeatmap()->isContinueScheduled())
		drawContinue(g, m_osu->getBeatmap()->getContinueCursorPoint(), m_osu->getBeatmap()->getHitcircleDiameter());
}

void OsuHUD::drawDummy(Graphics *g)
{
	drawPlayfieldBorder(g, OsuGameRules::getPlayfieldCenter(), OsuGameRules::getPlayfieldSize(), 0);

	drawSkip(g);

	drawWarningArrows(g);

	drawCombo(g, 420);

	drawProgressBar(g, 0.25f, false);

	drawAccuracy(g, 100.0f);

	drawHitErrorBar(g, 50, 100, 150);
}

void OsuHUD::update()
{
	// fps string update
	m_fpsAvg[m_iFpsAvgIndex] = engine->getFrameTime()/m_host_timescale_ref->getFloat();
	m_iFpsAvgIndex = (m_iFpsAvgIndex + 1) % NUM_FPS_AVG;
	if (engine->getTime() > m_fFpsUpdate)
	{
		m_fFpsUpdate = engine->getTime() + 0.35f;

		// HACKHACK: absolutely disgusting
		m_fCurFps = 0.0f;
		for (int i=0; i<NUM_FPS_AVG; i++)
		{
			if (m_fpsAvg[i] > 0.0f)
				m_fCurFps += 1.0f/m_fpsAvg[i];
		}
		m_fCurFps /= (float)NUM_FPS_AVG;
	}

	// target heatmap cleanup
	if (m_osu->getModTarget())
	{
		if (m_targets.size() > 0 && engine->getTime() > m_targets[0].time)
			m_targets.erase(m_targets.begin());
	}
}

void OsuHUD::drawCursor(Graphics *g, Vector2 pos)
{
	if (osu_draw_cursor_trail.getBool())
	{
		int i = m_cursorTrail.size()-1;
		while (i >= 0)
		{
			float alpha = clamp<float>((m_cursorTrail[i].time-engine->getTime())/osu_cursor_trail_length.getFloat(), 0.0f, 1.0f);
			if (m_cursorTrail[i].pos != pos)
				drawCursorTrailRaw(g, alpha*osu_cursor_trail_alpha.getFloat(), m_cursorTrail[i].pos);
			i--;
		}

		drawCursorTrailRaw(g, osu_cursor_trail_alpha.getFloat(), pos);
	}

	drawCursorRaw(g, pos);

	if (osu_draw_cursor_trail.getBool())
	{
		// this is a bit dirty, having array manipulation and update logic in a drawing function, but it's not too bad in this case i think.
		// necessary due to the pos variable (autopilot/auto etc.)
		if ((m_cursorTrail.size() > 0 && m_cursorTrail[m_cursorTrail.size()-1].pos != pos && engine->getTime() > m_cursorTrail[m_cursorTrail.size()-1].time-osu_cursor_trail_length.getFloat()+osu_cursor_trail_spacing.getFloat()) || m_cursorTrail.size() == 0)
		{
			CURSORTRAIL ct;
			ct.pos = pos;
			ct.time = engine->getTime() + osu_cursor_trail_length.getFloat();
			m_cursorTrail.push_back(ct);
		}

		if (m_cursorTrail.size() > 0 && engine->getTime() > m_cursorTrail[0].time)
			m_cursorTrail.erase(m_cursorTrail.begin());
	}
}

void OsuHUD::drawCursorRaw(Graphics *g, Vector2 pos)
{
	Image *cursor = m_osu->getSkin()->getCursor();
	const float scale = m_osu->getCursorScaleFactor() * (m_osu->getSkin()->isCursor2x() ? 0.5f : 1.0f);
	const float animatedScale = scale * (m_osu->getSkin()->getCursorExpand() ? m_fCursorExpandAnim : 1.0f);

	// draw cursor
	g->setColor(0xffffffff);
	g->pushTransform();
		g->scale(animatedScale*osu_cursor_scale.getFloat(), animatedScale*osu_cursor_scale.getFloat());
		if (m_osu->getSkin()->getCursorRotate())
			g->rotate(fmod(engine->getTime()*37.0f, 360.0f));
		g->translate(pos.x, pos.y);
		if (!m_osu->getSkin()->getCursorCenter())
			g->translate(cursor->getWidth()/2.0f, cursor->getHeight()/2.0f);
		g->drawImage(cursor);
	g->popTransform();

	// draw cursor middle
	if (m_osu->getSkin()->getCursorMiddle() != m_osu->getSkin()->getMissingTexture())
	{
		g->setColor(0xffffffff);
		g->pushTransform();
			g->scale(scale*osu_cursor_scale.getFloat(), scale*osu_cursor_scale.getFloat());
			g->translate(pos.x, pos.y);
			if (!m_osu->getSkin()->getCursorCenter())
				g->translate(m_osu->getSkin()->getCursorMiddle()->getWidth()/2.0f, m_osu->getSkin()->getCursorMiddle()->getHeight()/2.0f);
			g->drawImage(m_osu->getSkin()->getCursorMiddle());
		g->popTransform();
	}
}

void OsuHUD::drawCursorTrailRaw(Graphics *g, float alpha, Vector2 pos)
{
	Image *trail = m_osu->getSkin()->getCursorTrail();
	const float scale = m_osu->getCursorScaleFactor() * (m_osu->getSkin()->isCursor2x() ? 0.5f : 1.0f); // use scale from cursor, not from trail, because fuck you
	const float animatedScale = scale * (m_osu->getSkin()->getCursorExpand() ? m_fCursorExpandAnim : 1.0f);

	g->setColor(0xffffffff);
	g->setAlpha(alpha);
	g->pushTransform();
		g->scale(animatedScale*osu_cursor_scale.getFloat(), animatedScale*osu_cursor_scale.getFloat());
		g->translate(pos.x, pos.y);
		g->drawImage(trail);
	g->popTransform();
}

void OsuHUD::drawFps(Graphics *g, McFont *font, float fps)
{
	UString fpsString = UString::format("%i fps", (int)(fps));
	UString msString = UString::format("%.1f ms", (1.0f/fps)*1000.0f);

	g->setColor(0xff000000);
	g->pushTransform();
		g->translate(Osu::getScreenWidth() - font->getStringWidth(fpsString) - 3 + 1, Osu::getScreenHeight() - 3 - m_fFpsFontHeight - 3 + 1);
		g->drawString(font, fpsString);
	g->popTransform();
	g->pushTransform();
		g->translate(Osu::getScreenWidth() - font->getStringWidth(msString) - 3 + 1, Osu::getScreenHeight() - 3 + 1);
		g->drawString(font, msString);
	g->popTransform();

	if (fps >= 120)
		g->setColor(0xffffffff);
	else if (fps >= 60)
		g->setColor(0xffffff00);
	else
		g->setColor(0xffff4444);

	g->pushTransform();
		g->translate(Osu::getScreenWidth() - font->getStringWidth(fpsString) - 3, Osu::getScreenHeight() - 3 - m_fFpsFontHeight - 3);
		g->drawString(font, fpsString);
	g->popTransform();
	g->pushTransform();
		g->translate(Osu::getScreenWidth() - font->getStringWidth(msString) - 3, Osu::getScreenHeight() - 3);
		g->drawString(font, msString);
	g->popTransform();
}

void OsuHUD::drawPlayfieldBorder(Graphics *g, Vector2 playfieldCenter, Vector2 playfieldSize, float hitcircleDiameter)
{
	const int borderSize = osu_hud_playfield_border_size.getInt();
	Vector2 playfieldBorderTopLeft = Vector2((int)(playfieldCenter.x - playfieldSize.x/2 - hitcircleDiameter/2 - borderSize), (int)(playfieldCenter.y - playfieldSize.y/2 - hitcircleDiameter/2 - borderSize));
	Vector2 playfieldBorderSize = Vector2((int)(playfieldSize.x + hitcircleDiameter), (int)(playfieldSize.y + hitcircleDiameter));

	///g->setColor(0xffffffff);
	///g->drawRect(playfieldBorderTopLeft.x + borderSize, playfieldBorderTopLeft.y + borderSize, playfieldBorderSize.x, playfieldBorderSize.y);

	const Color innerColor = 0x44ffffff;
	const Color outerColor = 0x00000000;

	// top
	{
		VertexArrayObject vao;
		vao.setType(VertexArrayObject::TYPE_QUADS);

		vao.addVertex(playfieldBorderTopLeft);
		vao.addColor(outerColor);
		vao.addVertex(playfieldBorderTopLeft + Vector2(playfieldBorderSize.x + borderSize*2, 0));
		vao.addColor(outerColor);
		vao.addVertex(playfieldBorderTopLeft + Vector2(playfieldBorderSize.x + borderSize, borderSize));
		vao.addColor(innerColor);
		vao.addVertex(playfieldBorderTopLeft + Vector2(borderSize, borderSize));
		vao.addColor(innerColor);

		g->drawVAO(&vao);
	}

	// left
	{
		VertexArrayObject vao;
		vao.setType(VertexArrayObject::TYPE_QUADS);

		vao.addVertex(playfieldBorderTopLeft);
		vao.addColor(outerColor);
		vao.addVertex(playfieldBorderTopLeft + Vector2(borderSize, borderSize));
		vao.addColor(innerColor);
		vao.addVertex(playfieldBorderTopLeft + Vector2(borderSize, playfieldBorderSize.y + borderSize));
		vao.addColor(innerColor);
		vao.addVertex(playfieldBorderTopLeft + Vector2(0, playfieldBorderSize.y + 2*borderSize));
		vao.addColor(outerColor);

		g->drawVAO(&vao);
	}

	// right
	{
		VertexArrayObject vao;
		vao.setType(VertexArrayObject::TYPE_QUADS);

		vao.addVertex(playfieldBorderTopLeft + Vector2(playfieldBorderSize.x + 2*borderSize, 0));
		vao.addColor(outerColor);
		vao.addVertex(playfieldBorderTopLeft + Vector2(playfieldBorderSize.x + 2*borderSize, playfieldBorderSize.y + 2*borderSize));
		vao.addColor(outerColor);
		vao.addVertex(playfieldBorderTopLeft + Vector2(playfieldBorderSize.x + borderSize, playfieldBorderSize.y + borderSize));
		vao.addColor(innerColor);
		vao.addVertex(playfieldBorderTopLeft + Vector2(playfieldBorderSize.x + borderSize, borderSize));
		vao.addColor(innerColor);

		g->drawVAO(&vao);
	}

	// bottom
	{
		VertexArrayObject vao;
		vao.setType(VertexArrayObject::TYPE_QUADS);

		vao.addVertex(playfieldBorderTopLeft + Vector2(borderSize, playfieldBorderSize.y + borderSize));
		vao.addColor(innerColor);
		vao.addVertex(playfieldBorderTopLeft + Vector2(playfieldBorderSize.x + borderSize, playfieldBorderSize.y + borderSize));
		vao.addColor(innerColor);
		vao.addVertex(playfieldBorderTopLeft + Vector2(playfieldBorderSize.x + 2*borderSize, playfieldBorderSize.y + 2*borderSize));
		vao.addColor(outerColor);
		vao.addVertex(playfieldBorderTopLeft + Vector2(0, playfieldBorderSize.y + 2*borderSize));
		vao.addColor(outerColor);

		g->drawVAO(&vao);
	}

	/*
	//g->setColor(0x44ffffff);
	// top
	g->fillRect(playfieldBorderTopLeft.x, playfieldBorderTopLeft.y, playfieldBorderSize.x + borderSize*2, borderSize);

	// left
	g->fillRect(playfieldBorderTopLeft.x, playfieldBorderTopLeft.y + borderSize, borderSize, playfieldBorderSize.y);

	// right
	g->fillRect(playfieldBorderTopLeft.x + playfieldBorderSize.x + borderSize, playfieldBorderTopLeft.y + borderSize, borderSize, playfieldBorderSize.y);

	// bottom
	g->fillRect(playfieldBorderTopLeft.x, playfieldBorderTopLeft.y+playfieldBorderSize.y + borderSize, playfieldBorderSize.x + borderSize*2, borderSize);
	*/
}

void OsuHUD::drawLoadingSmall(Graphics *g)
{
	/*
	McFont *font = engine->getResourceManager()->getFont("FONT_DEFAULT");
	UString loadingText = "Loading ...";
	float stringWidth = font->getStringWidth(loadingText);

	g->setColor(0xffffffff);
	g->pushTransform();
	g->translate(-stringWidth/2, font->getHeight()/2);
	g->rotate(engine->getTime()*180, 0, 0, 1);
	g->translate(0, 0);
	g->translate(Osu::getScreenWidth()/2, Osu::getScreenHeight()/2);
	g->drawString(font, loadingText);
	g->popTransform();
	*/
	const float scale = Osu::getImageScale(m_osu->getSkin()->getBeatmapImportSpinner(), 29);

	g->setColor(0xffffffff);
	g->pushTransform();
	g->rotate(engine->getTime()*180, 0, 0, 1);
	g->scale(scale, scale);
	g->translate(Osu::getScreenWidth()/2, Osu::getScreenHeight()/2);
	g->drawImage(m_osu->getSkin()->getBeatmapImportSpinner());
	g->popTransform();
}

void OsuHUD::drawBeatmapImportSpinner(Graphics *g)
{
	const float scale = Osu::getImageScale(m_osu->getSkin()->getBeatmapImportSpinner(), 128);

	g->setColor(0xffffffff);
	g->pushTransform();
	g->rotate(engine->getTime()*180, 0, 0, 1);
	g->scale(scale, scale);
	g->translate(Osu::getScreenWidth()/2, Osu::getScreenHeight()/2);
	g->drawImage(m_osu->getSkin()->getBeatmapImportSpinner());
	g->popTransform();
}

void OsuHUD::drawBeatmapImportTop(Graphics *g)
{
	// commented because it looks ugly
	/*
	Image *beatmapImportTop = m_osu->getSkin()->getBeatmapImportTop();
	float scale = Osu::getImageScaleToFitResolution(beatmapImportTop, Osu::getScreenSize());

	g->setColor(0xffffffff);
	g->pushTransform();
	g->scale(scale, scale);
	g->translate(Osu::getScreenWidth()/2, (beatmapImportTop->getHeight()/2)*scale);
	g->drawImage(beatmapImportTop);
	g->popTransform();
	*/
}

void OsuHUD::drawVolumeChange(Graphics *g)
{
	if (engine->getTime() > m_fVolumeChangeTime) return;

	g->setColor(COLOR((char)(68*m_fVolumeChangeFade), 255, 255, 255));
	g->fillRect(0, Osu::getScreenHeight()*(1.0f - m_fLastVolume), Osu::getScreenWidth(), Osu::getScreenHeight()*m_fLastVolume);
}

void OsuHUD::drawScoreNumber(Graphics *g, int number, float scale, bool drawLeadingAndTrailingZero, int offset)
{
	// get digits
	std::vector<int> digits;
	while (number >= 10)
	{
		int curDigit = number % 10;
		number /= 10;

		digits.insert(digits.begin(), curDigit);
	}
	digits.insert(digits.begin(), number);
	if (drawLeadingAndTrailingZero && digits.size() == 1)
		digits.push_back(0);

	// draw them
	float lastWidth = m_osu->getSkin()->getScore0()->getWidth();
	for (int i=0; i<digits.size(); i++)
	{
		switch (digits[i])
		{
		case 0:
			g->translate(lastWidth*0.5f*scale, 0);
			g->drawImage(m_osu->getSkin()->getScore0());
			g->translate(lastWidth*0.5f*scale, 0);
			break;
		case 1:
			g->translate(lastWidth*0.5f*scale, 0);
			g->drawImage(m_osu->getSkin()->getScore1());
			g->translate(lastWidth*0.5f*scale, 0);
			break;
		case 2:
			g->translate(lastWidth*0.5f*scale, 0);
			g->drawImage(m_osu->getSkin()->getScore2());
			g->translate(lastWidth*0.5f*scale, 0);
			break;
		case 3:
			g->translate(lastWidth*0.5f*scale, 0);
			g->drawImage(m_osu->getSkin()->getScore3());
			g->translate(lastWidth*0.5f*scale, 0);
			break;
		case 4:
			g->translate(lastWidth*0.5f*scale, 0);
			g->drawImage(m_osu->getSkin()->getScore4());
			g->translate(lastWidth*0.5f*scale, 0);
			break;
		case 5:
			g->translate(lastWidth*0.5f*scale, 0);
			g->drawImage(m_osu->getSkin()->getScore5());
			g->translate(lastWidth*0.5f*scale, 0);
			break;
		case 6:
			g->translate(lastWidth*0.5f*scale, 0);
			g->drawImage(m_osu->getSkin()->getScore6());
			g->translate(lastWidth*0.5f*scale, 0);
			break;
		case 7:
			g->translate(lastWidth*0.5f*scale, 0);
			g->drawImage(m_osu->getSkin()->getScore7());
			g->translate(lastWidth*0.5f*scale, 0);
			break;
		case 8:
			g->translate(lastWidth*0.5f*scale, 0);
			g->drawImage(m_osu->getSkin()->getScore8());
			g->translate(lastWidth*0.5f*scale, 0);
			break;
		case 9:
			g->translate(lastWidth*0.5f*scale, 0);
			g->drawImage(m_osu->getSkin()->getScore9());
			g->translate(lastWidth*0.5f*scale, 0);
			break;
		}

		g->translate(offset, 0);
	}
}

void OsuHUD::drawCombo(Graphics *g, int combo)
{
	g->setColor(0xffffffff);

	const int offset = 5;

	// draw back (anim)
	float animScaleMultiplier = 1.0f + m_fComboAnim2*osu_combo_anim2_size.getFloat();
	float scale = osu_hud_scale.getFloat() * m_osu->getImageScale(m_osu->getSkin()->getScore0(), 32)*animScaleMultiplier;
	if (m_fComboAnim2 > 0.01f)
	{
		g->setAlpha(m_fComboAnim2*0.65f);
		g->pushTransform();
			g->scale(scale, scale);
			g->translate(offset, Osu::getScreenHeight() - m_osu->getSkin()->getScore0()->getHeight()*scale/2.0f);
			drawScoreNumber(g, combo, scale);

			// draw 'x' at the end
			g->translate(m_osu->getSkin()->getScoreX()->getWidth()*0.5f*scale, 0);
			g->drawImage(m_osu->getSkin()->getScoreX());
		g->popTransform();
	}

	// draw front
	g->setAlpha(1.0f);
	const float animPercent = (m_fComboAnim1 < 1.0f ? m_fComboAnim1 : 2.0f - m_fComboAnim1);
	animScaleMultiplier = 1.0f + (0.5f*animPercent*animPercent)*osu_combo_anim1_size.getFloat();
	scale = osu_hud_scale.getFloat() * m_osu->getImageScale(m_osu->getSkin()->getScore0(), 32) * animScaleMultiplier;
	g->pushTransform();
		g->scale(scale, scale);
		g->translate(offset, Osu::getScreenHeight() - m_osu->getSkin()->getScore0()->getHeight()*scale/2.0f);
		drawScoreNumber(g, combo, scale);

		// draw 'x' at the end
		g->translate(m_osu->getSkin()->getScoreX()->getWidth()*0.5f*scale, 0);
		g->drawImage(m_osu->getSkin()->getScoreX());
	g->popTransform();
}

void OsuHUD::drawAccuracy(Graphics *g, float accuracy)
{
	g->setColor(0xffffffff);

	// get integer & fractional parts of the number
	const int accuracyInt = (int)accuracy;
	const int accuracyFrac = (int)((accuracy - accuracyInt)*100.0f);

	// draw it
	const int spacingOffset = 2;
	const int offset = 5;
	const float scale = osu_hud_scale.getFloat() * m_osu->getImageScale(m_osu->getSkin()->getScore0(), 13);
	g->pushTransform();

		// note that "spacingOffset*numDigits" would actually be used with (numDigits-1), but because we add a spacingOffset after the score dot we also have to add it here
		const int numDigits = (accuracyInt < 10 && accuracyInt > 0 ? 3 : (accuracyInt > 99 ? 5 : 4));
		const float xOffset = m_osu->getSkin()->getScore0()->getWidth()*scale*numDigits + m_osu->getSkin()->getScoreDot()->getWidth()*scale + m_osu->getSkin()->getScorePercent()->getWidth()*scale + spacingOffset*numDigits + 1;

		m_fAccuracyXOffset = Osu::getScreenWidth() - xOffset - offset;
		m_fAccuracyYOffset = m_osu->getSkin()->getScore0()->getHeight()*scale/2 + offset*2;
		g->scale(scale, scale);
		g->translate(m_fAccuracyXOffset, m_fAccuracyYOffset);

		drawScoreNumber(g, accuracyInt, scale, accuracy == 0.0f, spacingOffset);

		// draw dot '.' between the integer and fractional part
		g->setColor(0xffffffff);
		g->translate(m_osu->getSkin()->getScoreDot()->getWidth()*0.5f*scale, 0);
		g->drawImage(m_osu->getSkin()->getScoreDot());
		g->translate(m_osu->getSkin()->getScoreDot()->getWidth()*0.5f*scale, 0);
		g->translate(spacingOffset, 0); // extra spacing

		drawScoreNumber(g, accuracyFrac, scale, true, spacingOffset);

		// draw '%' at the end
		g->setColor(0xffffffff);
		g->translate(m_osu->getSkin()->getScorePercent()->getWidth()*0.5f*scale, 0);
		g->drawImage(m_osu->getSkin()->getScorePercent());
	g->popTransform();
}

void OsuHUD::drawSkip(Graphics *g)
{
	const float scale = osu_hud_scale.getFloat() * m_osu->getImageScale(m_osu->getSkin()->getPlaySkip(), 96);

	g->setColor(0xffffffff);
	g->pushTransform();
		g->scale(scale, scale);
		g->translate(Osu::getScreenWidth() - m_osu->getSkin()->getPlaySkip()->getWidth()*scale/2.0f, Osu::getScreenHeight() - m_osu->getSkin()->getPlaySkip()->getHeight()*scale/2.0f);
		g->drawImage(m_osu->getSkin()->getPlaySkip());
	g->popTransform();
}

void OsuHUD::drawWarningArrow(Graphics *g, Vector2 pos, bool flipVertically, bool originLeft)
{
	const float scale = osu_hud_scale.getFloat() * m_osu->getImageScale(m_osu->getSkin()->getPlayWarningArrow(), 78);

	g->pushTransform();
		g->scale(flipVertically ? -scale : scale, scale);
		g->translate(pos.x + (flipVertically ? (-m_osu->getSkin()->getPlayWarningArrow()->getWidth()*scale/2.0f) : (m_osu->getSkin()->getPlayWarningArrow()->getWidth()*scale/2.0f)) * (originLeft ? 1.0f : -1.0f), pos.y);
		g->drawImage(m_osu->getSkin()->getPlayWarningArrow());
	g->popTransform();
}

void OsuHUD::drawWarningArrows(Graphics *g, float hitcircleDiameter)
{
	const float divider = 18.0f;
	const float part = OsuGameRules::getPlayfieldSize().y * (1.0f / divider);

	g->setColor(0xffffffff);
	drawWarningArrow(g, Vector2(m_osu->getUIScale(28), OsuGameRules::getPlayfieldCenter().y - OsuGameRules::getPlayfieldSize().y/2 + part*2), false);
	drawWarningArrow(g, Vector2(m_osu->getUIScale(28), OsuGameRules::getPlayfieldCenter().y - OsuGameRules::getPlayfieldSize().y/2 + part*2 + part*13), false);

	drawWarningArrow(g, Vector2(Osu::getScreenWidth() - m_osu->getUIScale(28), OsuGameRules::getPlayfieldCenter().y - OsuGameRules::getPlayfieldSize().y/2 + part*2), true);
	drawWarningArrow(g, Vector2(Osu::getScreenWidth() - m_osu->getUIScale(28), OsuGameRules::getPlayfieldCenter().y - OsuGameRules::getPlayfieldSize().y/2 + part*2 + part*13), true);
}

void OsuHUD::drawContinue(Graphics *g, Vector2 cursor, float hitcircleDiameter)
{
	Image *unpause = m_osu->getSkin()->getUnpause();
	const float unpauseScale = Osu::getImageScale(unpause, 80);

	Image *cursorImage = m_osu->getSkin()->getDefaultCursor();
	const float cursorScale = Osu::getImageScaleToFitResolution(cursorImage, Vector2(hitcircleDiameter, hitcircleDiameter));

	// bleh
	if (cursor.x < cursorImage->getWidth() || cursor.y < cursorImage->getHeight() || cursor.x > Osu::getScreenWidth()-cursorImage->getWidth() || cursor.y > Osu::getScreenHeight()-cursorImage->getHeight())
		cursor = Osu::getScreenSize()/2;

	// base
	g->setColor(COLOR(255, 255, 153, 51));
	g->pushTransform();
	g->scale(cursorScale, cursorScale);
	g->translate(cursor.x, cursor.y);
	g->drawImage(m_osu->getSkin()->getDefaultCursor());
	g->popTransform();

	// pulse animation
	float cursorAnimPulsePercent = clamp<float>(fmod(engine->getTime(), 1.35f), 0.0f, 1.0f);
	g->setColor(COLOR((short)(255.0f*(1.0f-cursorAnimPulsePercent)), 255, 153, 51));
	g->pushTransform();
	g->scale(cursorScale*(1.0f + cursorAnimPulsePercent), cursorScale*(1.0f + cursorAnimPulsePercent));
	g->translate(cursor.x, cursor.y);
	g->drawImage(m_osu->getSkin()->getDefaultCursor());
	g->popTransform();

	// unpause click message
	g->setColor(0xffffffff);
	g->pushTransform();
	g->scale(unpauseScale, unpauseScale);
	g->translate(cursor.x + 20 + (unpause->getWidth()/2)*unpauseScale, cursor.y + 20 + (unpause->getHeight()/2)*unpauseScale);
	g->drawImage(unpause);
	g->popTransform();
}

void OsuHUD::drawHitErrorBar(Graphics *g, float hitWindow300, float hitWindow100, float hitWindow50)
{
	const int brightnessSub = 50;
	const Color color300 = COLOR(255, 0, 255-brightnessSub, 255-brightnessSub);
	const Color color100 = COLOR(255, 0, 255-brightnessSub, 0);
	const Color color50 = COLOR(255, 255-brightnessSub, 165-brightnessSub, 0);

	const Vector2 size = Vector2(Osu::getScreenWidth()*0.15f, Osu::getScreenHeight()*0.007f);
	const Vector2 center = Vector2(Osu::getScreenWidth()/2.0f, Osu::getScreenHeight()*0.985f);

	const float entryHeight = size.y*3.4f;
	const float entryWidth = size.y*0.6f;

	const float totalHitWindowLength = hitWindow50;
	const float percent100 = hitWindow100 / totalHitWindowLength;
	const float percent300 = hitWindow300 / totalHitWindowLength;

	// draw background bar with color indicators for 300s, 100s and 50s
	g->setColor(color50);
	g->fillRect(center.x - size.x/2.0f, center.y - size.y/2.0f, size.x, size.y);
	if (!OsuGameRules::osu_mod_ming3012.getBool())
	{
		g->setColor(color100);
		g->fillRect(center.x - size.x*percent100/2.0f, center.y - size.y/2.0f, size.x*percent100, size.y);
	}
	g->setColor(color300);
	g->fillRect(center.x - size.x*percent300/2.0f, center.y - size.y/2.0f, size.x*percent300, size.y);

	// draw hit errors
	for (int i=m_hiterrors.size()-1; i>=0; i--)
	{
		float percent = clamp<float>((float)m_hiterrors[i].delta / (float)totalHitWindowLength, -5.0f, 5.0f);
		float alpha = clamp<float>((m_hiterrors[i].time - engine->getTime()) / (m_hiterrors[i].miss || m_hiterrors[i].misaim ? 4.0f : 6.0f), 0.0f, 1.0f);
		alpha *= alpha;

		if (m_hiterrors[i].miss || m_hiterrors[i].misaim)
			g->setColor(0xffff0000);
		else
		{
			Color barColor = std::abs(percent) <= percent300 ? color300 : (std::abs(percent) <= percent100 && !OsuGameRules::osu_mod_ming3012.getBool() ? color100 : color50);
			g->setColor(barColor);
		}

		g->setAlpha(alpha*0.75f);

		float missHeightMultiplier = 1.0f;
		if (m_hiterrors[i].miss)
			missHeightMultiplier = 1.5f;
		if (m_hiterrors[i].misaim)
			missHeightMultiplier = 4.0f;

		g->fillRect(center.x - (entryWidth/2.0f) + percent*(size.x/2.0f), center.y - (entryHeight*missHeightMultiplier)/2.0f, entryWidth, (entryHeight*missHeightMultiplier));
	}

	// white center line
	g->setColor(0xffffffff);
	g->fillRect(center.x - entryWidth/2.0f, center.y - entryHeight/2.0f, entryWidth, entryHeight);
}

void OsuHUD::drawProgressBar(Graphics *g, float percent, bool waiting)
{
	const float num_segments = 15*8;
	const int offset = 20;
	const float radius = osu_hud_scale.getFloat() * m_osu->getUIScale(10.5f);
	const float circularMetreScale = ((2*radius)/m_osu->getSkin()->getCircularmetre()->getWidth()) * 1.3f; // HACKHACK: the skin image is NOT border to border; hardcoded 1.3 multiplier
	const float actualCircularMetreScale = ((2*radius)/m_osu->getSkin()->getCircularmetre()->getWidth());
	Vector2 center = Vector2(m_fAccuracyXOffset - radius - offset, m_fAccuracyYOffset);

	// clamp to top edge of screen
	if (center.y - (m_osu->getSkin()->getCircularmetre()->getHeight()*actualCircularMetreScale)/2.0f < 0)
		center.y += std::abs(center.y - (m_osu->getSkin()->getCircularmetre()->getHeight()*actualCircularMetreScale)/2.0f);

	const float theta = 2 * PI / float(num_segments);
	const float s = sinf(theta); // precalculate the sine and cosine
	const float c = cosf(theta);
	float t;
	float x = 0;
	float y = -radius; // we start at the top

	if (waiting)
		g->setColor(0xaa00f200);
	else
		g->setColor(0xaaf2f2f2);

	{
		VertexArrayObject vao;
		Vector2 prevVertex;
		for (int i=0; i<num_segments+1; i++)
		{
			float curPercent = (i*(360.0f / num_segments)) / 360.0f;
			if (curPercent > percent)
				break;

			// build current vertex
			Vector2 curVertex = Vector2(x + center.x, y + center.y);

			// add vertex, triangle strip style (counter clockwise)
			if (i > 0)
			{
				vao.addVertex(curVertex);
				vao.addVertex(prevVertex);
				vao.addVertex(center);
			}

			// apply the rotation
			t = x;
			x = c * x - s * y;
			y = s * t + c * y;

			// save
			prevVertex = curVertex;
		}

		// draw it
		g->setAntialiasing(true);
		g->drawVAO(&vao);
		g->setAntialiasing(false);
	}

	// draw circularmetre
	g->setColor(0xffffffff);
	g->pushTransform();
		g->scale(circularMetreScale, circularMetreScale);
		g->translate(center.x, center.y);
		g->drawImage(m_osu->getSkin()->getCircularmetre());
	g->popTransform();
}

void OsuHUD::drawStatistics(Graphics *g, int nps, int nd)
{
	g->pushTransform();
		drawNPS(g, nps);
		g->translate(0, m_osu->getTitleFont()->getHeight() + 10);
		drawND(g, nd);
	g->popTransform();
}

void OsuHUD::drawNPS(Graphics *g, int nps)
{
	UString npsString = UString::format("NPS: %i", nps);

	g->pushTransform();
		g->setColor(0xff000000);
		g->translate(4, m_osu->getTitleFont()->getHeight()+4);
		g->drawString(m_osu->getTitleFont(), npsString);

		g->setColor(0xffffffff);
		g->translate(-1, -1);
		g->drawString(m_osu->getTitleFont(), npsString);
	g->popTransform();
}

void OsuHUD::drawND(Graphics *g, int nd)
{
	UString ndString = UString::format("ND: %i", nd);

	g->pushTransform();
		g->setColor(0xff000000);
		g->translate(4, m_osu->getTitleFont()->getHeight()+4);
		g->drawString(m_osu->getTitleFont(), ndString);

		g->setColor(0xffffffff);
		g->translate(-1, -1);
		g->drawString(m_osu->getTitleFont(), ndString);
	g->popTransform();
}

void OsuHUD::drawTargetHeatmap(Graphics *g, float hitcircleDiameter)
{
	const Vector2 center = Vector2((int)(hitcircleDiameter/2.0f + 5.0f), (int)(hitcircleDiameter/2.0f + 5.0f));

	const int brightnessSub = 0;
	const Color color300 = COLOR(255, 0, 255-brightnessSub, 255-brightnessSub);
	const Color color100 = COLOR(255, 0, 255-brightnessSub, 0);
	const Color color50 = COLOR(255, 255-brightnessSub, 165-brightnessSub, 0);
	const Color colorMiss = COLOR(255, 255-brightnessSub, 0, 0);

	OsuCircle::drawCircle(g, m_osu->getSkin(), center, hitcircleDiameter, COLOR(255, 50, 50, 50));

	const int size = hitcircleDiameter*0.075f;
	for (int i=0; i<m_targets.size(); i++)
	{
		const float delta = m_targets[i].delta;

		const float overlap = 0.15f;
		Color color;
		if (delta < m_osu_mod_target_300_percent_ref->getFloat()-overlap)
			color = color300;
		else if (delta < m_osu_mod_target_300_percent_ref->getFloat()+overlap)
		{
			const float factor300 = (m_osu_mod_target_300_percent_ref->getFloat() + overlap - delta) / (2.0f*overlap);
			const float factor100 = 1.0f - factor300;
			color = COLORf(1.0f, COLOR_GET_Rf(color300)*factor300 + COLOR_GET_Rf(color100)*factor100, COLOR_GET_Gf(color300)*factor300 + COLOR_GET_Gf(color100)*factor100, COLOR_GET_Bf(color300)*factor300 + COLOR_GET_Bf(color100)*factor100);
		}
		else if (delta < m_osu_mod_target_100_percent_ref->getFloat()-overlap)
			color = color100;
		else if (delta < m_osu_mod_target_100_percent_ref->getFloat()+overlap)
		{
			const float factor100 = (m_osu_mod_target_100_percent_ref->getFloat() + overlap - delta) / (2.0f*overlap);
			const float factor50 = 1.0f - factor100;
			color = COLORf(1.0f, COLOR_GET_Rf(color100)*factor100 + COLOR_GET_Rf(color50)*factor50, COLOR_GET_Gf(color100)*factor100 + COLOR_GET_Gf(color50)*factor50, COLOR_GET_Bf(color100)*factor100 + COLOR_GET_Bf(color50)*factor50);
		}
		else if (delta < m_osu_mod_target_50_percent_ref->getFloat())
			color = color50;
		else
			color = colorMiss;

		g->setColor(color);
		g->setAlpha(clamp<float>((m_targets[i].time - engine->getTime())/3.5f, 0.0f, 1.0f));

		const float theta = deg2rad(m_targets[i].angle);
		const float cs = cos(theta);
		const float sn = sin(theta);

		Vector2 up = Vector2(-1, 0);
		Vector2 offset;
		offset.x = up.x * cs - up.y * sn;
		offset.y = up.x * sn + up.y * cs;
		offset.normalize();
		offset *= (delta*(hitcircleDiameter/2.0f));

		//g->fillRect(center.x-size/2 - offset.x, center.y-size/2 - offset.y, size, size);

		const float imageScale = m_osu->getImageScaleToFitResolution(m_osu->getSkin()->getCircleFull(), Vector2(size, size));
		g->pushTransform();
		g->scale(imageScale, imageScale);
		g->translate(center.x - offset.x, center.y - offset.y);
		g->drawImage(m_osu->getSkin()->getCircleFull());
		g->popTransform();
	}
}

void OsuHUD::animateCombo()
{
	m_fComboAnim1 = 0.0f;
	m_fComboAnim2 = 1.0f;

	anim->moveLinear(&m_fComboAnim1, 2.0f, osu_combo_anim1_duration.getFloat(), true);
	anim->moveQuadOut(&m_fComboAnim2, 0.0f, osu_combo_anim2_duration.getFloat(), 0.0f, true);
}

void OsuHUD::addHitError(long delta, bool miss, bool misaim)
{
	HITERROR h;
	h.delta = delta;
	h.time = engine->getTime() + (miss || misaim ? 4.0f : 6.0f);
	h.miss = miss;
	h.misaim = misaim;

	m_hiterrors.push_back(h);

	for (int i=0; i<m_hiterrors.size(); i++)
	{
		if (engine->getTime() > m_hiterrors[i].time)
		{
			m_hiterrors.erase(m_hiterrors.begin()+i);
			i--;
		}
	}
}

void OsuHUD::addTarget(float delta, float angle)
{
	TARGET t;
	t.time = engine->getTime() + 3.5f;
	t.delta = delta;
	t.angle = angle;

	m_targets.push_back(t);
}

void OsuHUD::animateVolumeChange()
{
	m_fVolumeChangeTime = engine->getTime() + 0.65f;
	m_fVolumeChangeFade = 1.0f;
	anim->moveQuadOut(&m_fVolumeChangeFade, 0.0f, 0.25f, 0.4f, true);
	anim->moveQuadOut(&m_fLastVolume, m_osu_volume_master_ref->getFloat(), 0.15f, 0.0f, true);
}

void OsuHUD::animateCursorExpand()
{
	m_fCursorExpandAnim = 1.0f;
	anim->moveQuadOut(&m_fCursorExpandAnim, osu_cursor_expand_scale_multiplier.getFloat(), osu_cursor_expand_duration.getFloat(), 0.0f, true);
}

void OsuHUD::animateCursorShrink()
{
	anim->moveQuadOut(&m_fCursorExpandAnim, 1.0f, osu_cursor_expand_duration.getFloat(), 0.0f, true);
}

Rect OsuHUD::getSkipClickRect()
{
	float skipScale = osu_hud_scale.getFloat() * Osu::getImageScale(m_osu->getSkin()->getPlaySkip(), 120);
	return Rect(Osu::getScreenWidth() - m_osu->getSkin()->getPlaySkip()->getWidth()*skipScale, Osu::getScreenHeight() - m_osu->getSkin()->getPlaySkip()->getHeight()*skipScale, m_osu->getSkin()->getPlaySkip()->getWidth()*skipScale, m_osu->getSkin()->getPlaySkip()->getHeight()*skipScale);
}
