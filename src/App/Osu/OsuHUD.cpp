//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		hud element drawing functions (accuracy, combo, score, etc.)
//
// $NoKeywords: $osuhud
//===============================================================================//

#include "OsuHUD.h"

#include "Engine.h"
#include "ConVar.h"
#include "Mouse.h"
#include "ResourceManager.h"
#include "AnimationHandler.h"
#include "VertexArrayObject.h"
#include "Shader.h"

#include "Osu.h"
#include "OsuVR.h"
#include "OsuSkin.h"
#include "OsuBeatmap.h"
#include "OsuBeatmapStandard.h"
#include "OsuGameRules.h"
#include "OsuScore.h"

#include "OsuHitObject.h"
#include "OsuCircle.h"

ConVar osu_cursor_alpha("osu_cursor_alpha", 1.0f);
ConVar osu_cursor_scale("osu_cursor_scale", 1.29f);
ConVar osu_cursor_expand_scale_multiplier("osu_cursor_expand_scale_multiplier", 1.3f);
ConVar osu_cursor_expand_duration("osu_cursor_expand_duration", 0.1f);
ConVar osu_cursor_trail_length("osu_cursor_trail_length", 0.17f);
ConVar osu_cursor_trail_spacing("osu_cursor_trail_spacing", 0.015f);
ConVar osu_cursor_trail_alpha("osu_cursor_trail_alpha", 1.0f);

ConVar osu_hud_scale("osu_hud_scale", 1.0f);
ConVar osu_hud_hiterrorbar_scale("osu_hud_hiterrorbar_scale", 1.0f);
ConVar osu_hud_hiterrorbar_showmisswindow("osu_hud_hiterrorbar_showmisswindow", false);
ConVar osu_hud_hiterrorbar_width_percent_with_misswindow("osu_hud_hiterrorbar_width_percent_with_misswindow", 0.4f);
ConVar osu_hud_hiterrorbar_width_percent("osu_hud_hiterrorbar_width_percent", 0.15f);
ConVar osu_hud_hiterrorbar_height_percent("osu_hud_hiterrorbar_height_percent", 0.007f);
ConVar osu_hud_hiterrorbar_bar_width_scale("osu_hud_hiterrorbar_bar_width_scale", 0.6f);
ConVar osu_hud_hiterrorbar_bar_height_scale("osu_hud_hiterrorbar_bar_height_scale", 3.4f);
ConVar osu_hud_combo_scale("osu_hud_combo_scale", 1.0f);
ConVar osu_hud_score_scale("osu_hud_score_scale", 1.0f);
ConVar osu_hud_accuracy_scale("osu_hud_accuracy_scale", 1.0f);
ConVar osu_hud_progressbar_scale("osu_hud_progressbar_scale", 1.0f);
ConVar osu_hud_playfield_border_size("osu_hud_playfield_border_size", 5.0f);
ConVar osu_hud_statistics_scale("osu_hud_statistics_scale", 1.0f);
ConVar osu_hud_statistics_offset_x("osu_hud_statistics_offset_x", 5.0f);
ConVar osu_hud_statistics_offset_y("osu_hud_statistics_offset_y", 0.0f);

ConVar osu_draw_cursor_trail("osu_draw_cursor_trail", true);
ConVar osu_draw_hud("osu_draw_hud", true);
ConVar osu_draw_hpbar("osu_draw_hpbar", false);
ConVar osu_draw_hiterrorbar("osu_draw_hiterrorbar", true);
ConVar osu_draw_progressbar("osu_draw_progressbar", true);
ConVar osu_draw_combo("osu_draw_combo", true);
ConVar osu_draw_score("osu_draw_score", true);
ConVar osu_draw_accuracy("osu_draw_accuracy", true);
ConVar osu_draw_target_heatmap("osu_draw_target_heatmap", true);
ConVar osu_draw_scrubbing_timeline("osu_draw_scrubbing_timeline", true);
ConVar osu_draw_continue("osu_draw_continue", true);

ConVar osu_draw_statistics_misses("osu_draw_statistics_misses", false);
ConVar osu_draw_statistics_bpm("osu_draw_statistics_bpm", false);
ConVar osu_draw_statistics_ar("osu_draw_statistics_ar", false);
ConVar osu_draw_statistics_cs("osu_draw_statistics_cs", false);
ConVar osu_draw_statistics_od("osu_draw_statistics_od", false);
ConVar osu_draw_statistics_nps("osu_draw_statistics_nps", false);
ConVar osu_draw_statistics_nd("osu_draw_statistics_nd", false);
ConVar osu_draw_statistics_ur("osu_draw_statistics_ur", false);
ConVar osu_draw_statistics_pp("osu_draw_statistics_pp", false);

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
	m_fCurFpsSmooth = 60.0f;
	m_fFpsUpdate = 0.0f;
	m_fFpsFontHeight = m_tempFont->getHeight();

	m_fAccuracyXOffset = 0.0f;
	m_fAccuracyYOffset = 0.0f;
	m_fScoreHeight = 0.0f;

	m_fComboAnim1 = 0.0f;
	m_fComboAnim2 = 0.0f;

	m_fVolumeChangeTime = 0.0f;
	m_fVolumeChangeFade = 1.0f;
	m_fLastVolume = m_osu_volume_master_ref->getFloat();

	m_fCursorExpandAnim = 1.0f;
}

OsuHUD::~OsuHUD()
{
}

void OsuHUD::draw(Graphics *g)
{
	OsuBeatmap *beatmap = m_osu->getSelectedBeatmap();
	if (beatmap == NULL) return; // sanity check

	OsuBeatmapStandard *beatmapStd = dynamic_cast<OsuBeatmapStandard*>(beatmap);

	if (osu_draw_hud.getBool())
	{
		if (beatmap->isInSkippableSection())
			drawSkip(g);

		g->pushTransform();
			if (m_osu->getModTarget() && osu_draw_target_heatmap.getBool() && beatmapStd != NULL)
				g->translate(0, beatmapStd->getHitcircleDiameter());
			drawStatistics(g, m_osu->getScore()->getNumMisses(), beatmap->getBPM(), OsuGameRules::getApproachRateForSpeedMultiplier(beatmap, beatmap->getSpeedMultiplier()), beatmap->getCS(), OsuGameRules::getOverallDifficultyForSpeedMultiplier(beatmap, beatmap->getSpeedMultiplier()), beatmap->getNPS(), beatmap->getND(), m_osu->getScore()->getUnstableRate(), m_osu->getScore()->getPPv2());
		g->popTransform();

		if (osu_draw_hpbar.getBool())
			drawHP(g, beatmap->getHealth());

		if (osu_draw_hiterrorbar.getBool() && (beatmapStd == NULL || !beatmapStd->isSpinnerActive()) && !beatmap->isLoading())
			drawHitErrorBar(g, OsuGameRules::getHitWindow300(beatmap), OsuGameRules::getHitWindow100(beatmap), OsuGameRules::getHitWindow50(beatmap), OsuGameRules::getHitWindowMiss(beatmap));

		if (osu_draw_score.getBool())
			drawScore(g, m_osu->getScore()->getScore());

		if (osu_draw_combo.getBool())
			drawCombo(g, m_osu->getScore()->getCombo());

		if (osu_draw_progressbar.getBool())
			drawProgressBar(g, beatmap->getPercentFinishedPlayable(), beatmap->isWaiting());

		if (osu_draw_accuracy.getBool())
			drawAccuracy(g, m_osu->getScore()->getAccuracy()*100.0f);

		if (m_osu->getModTarget() && osu_draw_target_heatmap.getBool() && beatmapStd != NULL)
			drawTargetHeatmap(g, beatmapStd->getHitcircleDiameter());
	}

	if (beatmap->shouldFlashWarningArrows())
		drawWarningArrows(g, beatmapStd != NULL ? beatmapStd->getHitcircleDiameter() : 0);

	if (beatmap->isContinueScheduled() && beatmapStd != NULL && osu_draw_continue.getBool())
		drawContinue(g, beatmapStd->getContinueCursorPoint(), beatmapStd->getHitcircleDiameter());

	// TODO: put this in its own function
	if (osu_draw_scrubbing_timeline.getBool() && m_osu->isSeeking())
	{
		Vector2 cursorPos = engine->getMouse()->getPos();

		Color grey = 0xffbbbbbb;
		Color green = 0xff00ff00;

		McFont *timeFont = m_osu->getSubTitleFont();
		const float currentTimeTopTextOffset = 7;
		const float currentTimeLeftRightTextOffset = 5;
		const float startAndEndTimeTextOffset = 5;
		const unsigned long lengthFullMS = beatmap->getLength();
		const unsigned long lengthMS = beatmap->getLengthPlayable();
		const unsigned long startTimeMS = beatmap->getStartTimePlayable();
		const unsigned long endTimeMS = startTimeMS + lengthMS;
		const unsigned long currentTimeMS = beatmap->getTime();

		// timeline
		g->setColor(0xff000000);
		g->drawLine(0, cursorPos.y+1, m_osu->getScreenWidth(), cursorPos.y+1);
		g->setColor(grey);
		g->drawLine(0, cursorPos.y, m_osu->getScreenWidth(), cursorPos.y);

		// current time triangle
		Vector2 triangleTip = Vector2(m_osu->getScreenWidth()*beatmap->getPercentFinishedPlayable(), cursorPos.y);
		g->pushTransform();
			g->translate(triangleTip.x + 1, triangleTip.y - m_osu->getSkin()->getSeekTriangle()->getHeight()/2.0f + 1);
			g->setColor(0xff000000);
			g->drawImage(m_osu->getSkin()->getSeekTriangle());
			g->translate(-1, -1);
			g->setColor(green);
			g->drawImage(m_osu->getSkin()->getSeekTriangle());
		g->popTransform();

		// current time text
		UString currentTimeText = UString::format("%i:%02i", (currentTimeMS/1000) / 60, (currentTimeMS/1000) % 60);
		g->pushTransform();
			g->translate(clamp<float>(triangleTip.x - timeFont->getStringWidth(currentTimeText)/2.0f, currentTimeLeftRightTextOffset, m_osu->getScreenWidth() - timeFont->getStringWidth(currentTimeText) - currentTimeLeftRightTextOffset) + 1, triangleTip.y - m_osu->getSkin()->getSeekTriangle()->getHeight() - currentTimeTopTextOffset + 1);
			g->setColor(0xff000000);
			g->drawString(timeFont, currentTimeText);
			g->translate(-1, -1);
			g->setColor(green);
			g->drawString(timeFont, currentTimeText);
		g->popTransform();

		// start time text
		UString startTimeText = UString::format("(%i:%02i)", (startTimeMS/1000) / 60, (startTimeMS/1000) % 60);
		g->pushTransform();
			g->translate(startAndEndTimeTextOffset + 1, triangleTip.y + startAndEndTimeTextOffset + timeFont->getHeight() + 1);
			g->setColor(0xff000000);
			g->drawString(timeFont, startTimeText);
			g->translate(-1, -1);
			g->setColor(grey);
			g->drawString(timeFont, startTimeText);
		g->popTransform();

		// end time text
		UString endTimeText = UString::format("%i:%02i", (endTimeMS/1000) / 60, (endTimeMS/1000) % 60);
		g->pushTransform();
			g->translate(m_osu->getScreenWidth() - timeFont->getStringWidth(endTimeText) - startAndEndTimeTextOffset + 1, triangleTip.y + startAndEndTimeTextOffset + timeFont->getHeight() + 1);
			g->setColor(0xff000000);
			g->drawString(timeFont, endTimeText);
			g->translate(-1, -1);
			g->setColor(grey);
			g->drawString(timeFont, endTimeText);
		g->popTransform();

		// quicksave time triangle & text
		if (m_osu->getQuickSaveTime() != 0.0f)
		{
			const float quickSaveTimeToPlayablePercent = clamp<float>(((lengthFullMS*m_osu->getQuickSaveTime())) / (float)endTimeMS, 0.0f, 1.0f);
			triangleTip = Vector2(m_osu->getScreenWidth()*quickSaveTimeToPlayablePercent, cursorPos.y);
			g->pushTransform();
				g->rotate(180);
				g->translate(triangleTip.x + 1, triangleTip.y + m_osu->getSkin()->getSeekTriangle()->getHeight()/2.0f + 1);
				g->setColor(0xff000000);
				g->drawImage(m_osu->getSkin()->getSeekTriangle());
				g->translate(-1, -1);
				g->setColor(grey);
				g->drawImage(m_osu->getSkin()->getSeekTriangle());
			g->popTransform();

			// end time text
			unsigned long quickSaveTimeMS = lengthFullMS*m_osu->getQuickSaveTime();
			UString endTimeText = UString::format("%i:%02i", (quickSaveTimeMS/1000) / 60, (quickSaveTimeMS/1000) % 60);
			g->pushTransform();
				g->translate(clamp<float>(triangleTip.x - timeFont->getStringWidth(currentTimeText)/2.0f, currentTimeLeftRightTextOffset, m_osu->getScreenWidth() - timeFont->getStringWidth(currentTimeText) - currentTimeLeftRightTextOffset) + 1, triangleTip.y + m_osu->getSkin()->getSeekTriangle()->getHeight()*1.5f + currentTimeTopTextOffset + 1);
				g->setColor(0xff000000);
				g->drawString(timeFont, endTimeText);
				g->translate(-1, -1);
				g->setColor(grey);
				g->drawString(timeFont, endTimeText);
			g->popTransform();
		}
	}
}

void OsuHUD::drawDummy(Graphics *g)
{
	drawPlayfieldBorder(g, OsuGameRules::getPlayfieldCenter(m_osu), OsuGameRules::getPlayfieldSize(m_osu), 0);

	drawSkip(g);

	drawStatistics(g, 0, 180, 9.0f, 4.0f, 8.0f, 4, 6, 90.0f, 123);

	drawWarningArrows(g);

	if (osu_draw_combo.getBool())
		drawCombo(g, 420);

	if (osu_draw_score.getBool())
		drawScore(g, 123456789);

	if (osu_draw_progressbar.getBool())
		drawProgressBar(g, 0.25f, false);

	if (osu_draw_accuracy.getBool())
		drawAccuracy(g, 100.0f);

	if (osu_draw_hiterrorbar.getBool())
		drawHitErrorBar(g, 50, 100, 150, 400);
}

void OsuHUD::drawVR(Graphics *g, Matrix4 &mvp, OsuVR *vr)
{
	OsuBeatmapStandard *beatmap = dynamic_cast<OsuBeatmapStandard*>(m_osu->getSelectedBeatmap());
	if (beatmap == NULL) return; // sanity check

	vr->getShaderTexturedLegacyGeneric()->enable();
	vr->getShaderTexturedLegacyGeneric()->setUniformMatrix4fv("matrix", mvp);
	{
		if (osu_draw_hud.getBool())
		{
			if (beatmap->isInSkippableSection())
				drawSkip(g);

			drawStatistics(g, m_osu->getScore()->getNumMisses(), beatmap->getBPM(), OsuGameRules::getApproachRateForSpeedMultiplier(beatmap, beatmap->getSpeedMultiplier()), beatmap->getCS(), OsuGameRules::getOverallDifficultyForSpeedMultiplier(beatmap, beatmap->getSpeedMultiplier()), beatmap->getNPS(), beatmap->getND(), m_osu->getScore()->getUnstableRate(), m_osu->getScore()->getPPv2());

			vr->getShaderUntexturedLegacyGeneric()->enable();
			vr->getShaderUntexturedLegacyGeneric()->setUniformMatrix4fv("matrix", mvp);
			{
				if (osu_draw_hpbar.getBool() && !m_osu->getModNF())
					drawHP(g, beatmap->getHealth());
			}
			vr->getShaderUntexturedLegacyGeneric()->disable();
			vr->getShaderTexturedLegacyGeneric()->enable();

			if (osu_draw_score.getBool())
				drawScore(g, m_osu->getScore()->getScore());

			if (osu_draw_combo.getBool())
				drawCombo(g, m_osu->getScore()->getCombo());

			if (osu_draw_progressbar.getBool())
				drawProgressBarVR(g, mvp, vr, beatmap->getPercentFinishedPlayable(), beatmap->isWaiting());

			if (osu_draw_accuracy.getBool())
				drawAccuracy(g, m_osu->getScore()->getAccuracy()*100.0f);
		}

		if (beatmap->shouldFlashWarningArrows())
			drawWarningArrows(g, beatmap->getHitcircleDiameter());

		if (beatmap->isLoading())
			drawLoadingSmall(g);
	}
	vr->getShaderTexturedLegacyGeneric()->disable();
}

void OsuHUD::drawVRDummy(Graphics *g, Matrix4 &mvp, OsuVR *vr)
{
	vr->getShaderTexturedLegacyGeneric()->enable();
	vr->getShaderTexturedLegacyGeneric()->setUniformMatrix4fv("matrix", mvp);
	{
		drawSkip(g);

		drawStatistics(g, 0, 180, 9.0f, 4.0f, 8.0f, 4, 6, 90.0f, 123);

		vr->getShaderUntexturedLegacyGeneric()->enable();
		vr->getShaderUntexturedLegacyGeneric()->setUniformMatrix4fv("matrix", mvp);
		{
			if (osu_draw_hpbar.getBool())
				drawHP(g, 1.0f);
		}
		vr->getShaderUntexturedLegacyGeneric()->disable();
		vr->getShaderTexturedLegacyGeneric()->enable();

		if (osu_draw_score.getBool())
			drawScore(g, 123456789);

		if (osu_draw_combo.getBool())
			drawCombo(g, 1234);

		if (osu_draw_progressbar.getBool())
			drawProgressBarVR(g, mvp, vr, 0.25f, false);

		if (osu_draw_accuracy.getBool())
			drawAccuracy(g, 100.0f);

		drawWarningArrows(g);
	}
	vr->getShaderTexturedLegacyGeneric()->disable();
}

void OsuHUD::update()
{
	// fps string update
	const float smooth = std::pow(0.05, engine->getFrameTime());
	m_fCurFpsSmooth = smooth*m_fCurFpsSmooth + (1.0f - smooth)*(1.0f / engine->getFrameTime());
	if (engine->getTime() > m_fFpsUpdate || std::abs(m_fCurFpsSmooth-m_fCurFps) > 2.0f)
	{
		m_fFpsUpdate = engine->getTime() + 0.25f;
		m_fCurFps = m_fCurFpsSmooth;
	}

	// target heatmap cleanup
	if (m_osu->getModTarget())
	{
		if (m_targets.size() > 0 && engine->getTime() > m_targets[0].time)
			m_targets.erase(m_targets.begin());
	}
}

void OsuHUD::drawCursor(Graphics *g, Vector2 pos, float alphaMultiplier)
{
	if (osu_draw_cursor_trail.getBool())
	{
		int i = m_cursorTrail.size()-1;
		while (i >= 0)
		{
			float alpha = clamp<float>(((m_cursorTrail[i].time-engine->getTime())/osu_cursor_trail_length.getFloat())*alphaMultiplier, 0.0f, 1.0f);
			if (m_cursorTrail[i].pos != pos)
				drawCursorTrailRaw(g, alpha*osu_cursor_trail_alpha.getFloat(), m_cursorTrail[i].pos);
			i--;
		}

		drawCursorTrailRaw(g, osu_cursor_trail_alpha.getFloat()*alphaMultiplier, pos);
	}

	drawCursorRaw(g, pos, alphaMultiplier);

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

void OsuHUD::drawCursorRaw(Graphics *g, Vector2 pos, float alphaMultiplier)
{
	Image *cursor = m_osu->getSkin()->getCursor();
	const float scale = getCursorScaleFactor() * (m_osu->getSkin()->isCursor2x() ? 0.5f : 1.0f);
	const float animatedScale = scale * (m_osu->getSkin()->getCursorExpand() ? m_fCursorExpandAnim : 1.0f);

	// draw cursor
	g->setColor(0xffffffff);
	g->setAlpha(osu_cursor_alpha.getFloat()*alphaMultiplier);
	g->pushTransform();
		g->scale(animatedScale*osu_cursor_scale.getFloat(), animatedScale*osu_cursor_scale.getFloat());
		if (!m_osu->getSkin()->getCursorCenter())
			g->translate((cursor->getWidth()/2.0f)*animatedScale*osu_cursor_scale.getFloat(), (cursor->getHeight()/2.0f)*animatedScale*osu_cursor_scale.getFloat());
		if (m_osu->getSkin()->getCursorRotate())
			g->rotate(fmod(engine->getTime()*37.0f, 360.0f));
		g->translate(pos.x, pos.y);
		g->drawImage(cursor);
	g->popTransform();

	// draw cursor middle
	if (m_osu->getSkin()->getCursorMiddle() != m_osu->getSkin()->getMissingTexture())
	{
		g->setColor(0xffffffff);
		g->setAlpha(osu_cursor_alpha.getFloat()*alphaMultiplier);
		g->pushTransform();
			g->scale(scale*osu_cursor_scale.getFloat(), scale*osu_cursor_scale.getFloat());
			g->translate(pos.x, pos.y, 0.05f);
			if (!m_osu->getSkin()->getCursorCenter())
				g->translate((m_osu->getSkin()->getCursorMiddle()->getWidth()/2.0f)*scale*osu_cursor_scale.getFloat(), (m_osu->getSkin()->getCursorMiddle()->getHeight()/2.0f)*scale*osu_cursor_scale.getFloat());
			g->drawImage(m_osu->getSkin()->getCursorMiddle());
		g->popTransform();
	}
}

void OsuHUD::drawCursorTrailRaw(Graphics *g, float alpha, Vector2 pos)
{
	Image *trail = m_osu->getSkin()->getCursorTrail();
	const float scale = getCursorScaleFactor() * (m_osu->getSkin()->isCursor2x() ? 0.5f : 1.0f); // use scale from cursor, not from trail, because fuck you
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
	fps = std::round(fps);
	UString fpsString = UString::format("%i fps", (int)(fps));
	UString msString = UString::format("%.1f ms", (1.0f/fps)*1000.0f);

	g->setColor(0xff000000);
	g->pushTransform();
		g->translate(m_osu->getScreenWidth() - font->getStringWidth(fpsString) - 3 + 1, m_osu->getScreenHeight() - 3 - m_fFpsFontHeight - 3 + 1);
		g->drawString(font, fpsString);
	g->popTransform();
	g->pushTransform();
		g->translate(m_osu->getScreenWidth() - font->getStringWidth(msString) - 3 + 1, m_osu->getScreenHeight() - 3 + 1);
		g->drawString(font, msString);
	g->popTransform();


	if (fps >= 200 || (m_osu->isInVRMode() && fps >= 80))
		g->setColor(0xffffffff);
	else if (fps >= 120 || (m_osu->isInVRMode() && fps >= 60))
		g->setColor(0xffdddd00);
	else
	{
		const float pulse = std::abs(std::sin(engine->getTime()*4));
		g->setColor(COLORf(1.0f, 1.0f, 0.26f*pulse, 0.26f*pulse));
	}

	g->pushTransform();
		g->translate(m_osu->getScreenWidth() - font->getStringWidth(fpsString) - 3, m_osu->getScreenHeight() - 3 - m_fFpsFontHeight - 3);
		g->drawString(font, fpsString);
	g->popTransform();
	g->pushTransform();
		g->translate(m_osu->getScreenWidth() - font->getStringWidth(msString) - 3, m_osu->getScreenHeight() - 3);
		g->drawString(font, msString);
	g->popTransform();
}

void OsuHUD::drawPlayfieldBorder(Graphics *g, Vector2 playfieldCenter, Vector2 playfieldSize, float hitcircleDiameter)
{
	drawPlayfieldBorder(g, playfieldCenter, playfieldSize, hitcircleDiameter, osu_hud_playfield_border_size.getInt());
}

void OsuHUD::drawPlayfieldBorder(Graphics *g, Vector2 playfieldCenter, Vector2 playfieldSize, float hitcircleDiameter, float borderSize)
{
	if (borderSize == 0.0f)
		return;

	Vector2 playfieldBorderTopLeft = Vector2((int)(playfieldCenter.x - playfieldSize.x/2 - hitcircleDiameter/2 - borderSize), (int)(playfieldCenter.y - playfieldSize.y/2 - hitcircleDiameter/2 - borderSize));
	Vector2 playfieldBorderSize = Vector2((int)(playfieldSize.x + hitcircleDiameter), (int)(playfieldSize.y + hitcircleDiameter));

	// HACKHACK: force glDisable(GL_TEXTURE_2D) by drawPixel(), to avoid invisible border on maps without a background image
	g->setColor(0x00000000);
	g->drawPixel(-999,-999);

	const Color innerColor = 0x44ffffff;
	const Color outerColor = 0x00000000;

	g->pushTransform();
	g->translate(0, 0, 0.2f);

	// top
	{
		VertexArrayObject vao(Graphics::PRIMITIVE::PRIMITIVE_QUADS);

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
		VertexArrayObject vao(Graphics::PRIMITIVE::PRIMITIVE_QUADS);

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
		VertexArrayObject vao(Graphics::PRIMITIVE::PRIMITIVE_QUADS);

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
		VertexArrayObject vao(Graphics::PRIMITIVE::PRIMITIVE_QUADS);

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

	g->popTransform();

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
	g->translate(m_osu->getScreenWidth()/2, m_osu->getScreenHeight()/2);
	g->drawString(font, loadingText);
	g->popTransform();
	*/
	const float scale = Osu::getImageScale(m_osu, m_osu->getSkin()->getLoadingSpinner(), 29);

	g->setColor(0xffffffff);
	g->pushTransform();
	g->rotate(engine->getTime()*180, 0, 0, 1);
	g->scale(scale, scale);
	g->translate(m_osu->getScreenWidth()/2, m_osu->getScreenHeight()/2);
	g->drawImage(m_osu->getSkin()->getLoadingSpinner());
	g->popTransform();
}

void OsuHUD::drawBeatmapImportSpinner(Graphics *g)
{
	const float scale = Osu::getImageScale(m_osu, m_osu->getSkin()->getBeatmapImportSpinner(), 100);

	g->setColor(0xffffffff);
	g->pushTransform();
	g->rotate(engine->getTime()*180, 0, 0, 1);
	g->scale(scale, scale);
	g->translate(m_osu->getScreenWidth()/2, m_osu->getScreenHeight()/2);
	g->drawImage(m_osu->getSkin()->getBeatmapImportSpinner());
	g->popTransform();
}

void OsuHUD::drawVolumeChange(Graphics *g)
{
	if (engine->getTime() > m_fVolumeChangeTime) return;

	g->setColor(COLOR((char)(68*m_fVolumeChangeFade), 255, 255, 255));
	g->fillRect(0, m_osu->getScreenHeight()*(1.0f - m_fLastVolume), m_osu->getScreenWidth(), m_osu->getScreenHeight()*m_fLastVolume);
}

void OsuHUD::drawScoreNumber(Graphics *g, unsigned long long number, float scale, bool drawLeadingZeroes, int offset)
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
	if (digits.size() == 1)
	{
		if (drawLeadingZeroes)
			digits.insert(digits.begin(), 0);
	}

	// draw them
	// NOTE: just using the width here is incorrect, but it is the quickest solution instead of painstakingly reverse-engineering how osu does it
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

void OsuHUD::drawComboSimple(Graphics *g, int combo, float scale)
{
	g->pushTransform();
		drawScoreNumber(g, combo, scale);

		// draw 'x' at the end
		g->translate(m_osu->getSkin()->getScoreX()->getWidth()*0.5f*scale, 0);
		g->drawImage(m_osu->getSkin()->getScoreX());
	g->popTransform();
}

void OsuHUD::drawCombo(Graphics *g, int combo)
{
	g->setColor(0xffffffff);

	const int offset = 5;

	// draw back (anim)
	float animScaleMultiplier = 1.0f + m_fComboAnim2*osu_combo_anim2_size.getFloat();
	float scale = m_osu->getImageScale(m_osu, m_osu->getSkin()->getScore0(), 32)*animScaleMultiplier * osu_hud_scale.getFloat() * osu_hud_combo_scale.getFloat();
	if (m_fComboAnim2 > 0.01f)
	{
		g->setAlpha(m_fComboAnim2*0.65f);
		g->pushTransform();
			g->scale(scale, scale);
			g->translate(offset, m_osu->getScreenHeight() - m_osu->getSkin()->getScore0()->getHeight()*scale/2.0f, m_osu->isInVRMode() ? 0.15f : 0.0f);
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
	scale = m_osu->getImageScale(m_osu, m_osu->getSkin()->getScore0(), 32) * animScaleMultiplier * osu_hud_scale.getFloat() * osu_hud_combo_scale.getFloat();
	g->pushTransform();
		g->scale(scale, scale);
		g->translate(offset, m_osu->getScreenHeight() - m_osu->getSkin()->getScore0()->getHeight()*scale/2.0f, m_osu->isInVRMode() ? 0.45f : 0.0f);
		drawScoreNumber(g, combo, scale);

		// draw 'x' at the end
		g->translate(m_osu->getSkin()->getScoreX()->getWidth()*0.5f*scale, 0);
		g->drawImage(m_osu->getSkin()->getScoreX());
	g->popTransform();
}

void OsuHUD::drawScore(Graphics *g, unsigned long long score)
{
	g->setColor(0xffffffff);

	int numDigits = 1;
	unsigned long long scoreCopy = score;
	while (scoreCopy >= 10)
	{
		scoreCopy /= 10;
		numDigits++;
	}

	const float scale = m_osu->getImageScale(m_osu, m_osu->getSkin()->getScore0(), 13*1.5f) * osu_hud_scale.getFloat() * osu_hud_score_scale.getFloat();
	m_fScoreHeight = m_osu->getSkin()->getScore0()->getHeight()*scale;
	const int offset = 2;
	g->pushTransform();
		g->scale(scale, scale);
		g->translate(m_osu->getScreenWidth() - m_osu->getSkin()->getScore0()->getWidth()*scale*numDigits - offset*(numDigits-1), m_osu->getSkin()->getScore0()->getHeight()*scale/2);
		drawScoreNumber(g, score, scale, false, offset);
	g->popTransform();
}

void OsuHUD::drawHP(Graphics *g, float health)
{
	if (health <= 0.0f) return;

	float fadeStartPercent = 0.40f;
	float fadeFinishPercent = 0.25f;
	float greenBlueFactor = 1.0f;
	if (health < fadeStartPercent)
	{
		if (health > fadeFinishPercent)
			greenBlueFactor = (health - fadeFinishPercent) / std::abs(fadeStartPercent - fadeFinishPercent);
		else
			greenBlueFactor = 0.0f;
	}
	g->setColor(COLORf(1.0f, 1.0f, greenBlueFactor, greenBlueFactor));
	g->fillRect(0, 0, m_osu->getScreenWidth()*0.5f*health, m_osu->getScreenHeight()*0.015f);
	/*
	g->pushTransform();
		g->translate(100, 100);
		g->drawString(m_tempFont, UString::format("HP: %i", (int)(health*100.0f)));
		if (health < 0.01f)
		{
			g->translate(0, m_tempFont->getHeight());
			g->drawString(m_tempFont, "RIP RIP RIP RIP RIP RIP RIP RIP RIP RIP RIP RIP RIP RIP RIP RIP RIP RIP RIP RIP RIP RIP RIP RIP RIP RIP RIP RIP RIP RIP RIP RIP RIP RIP RIP RIP RIP RIP");
		}
	g->popTransform();
	*/
}

void OsuHUD::drawAccuracySimple(Graphics *g, float accuracy, float scale)
{
	// get integer & fractional parts of the number
	// see drawAccuracy() for explanation
	const int accuracyInt = (int)accuracy;
	const int accuracyFrac = clamp<int>(((int)(std::round((accuracy - accuracyInt)*1000.0f))) / 10, 0, 99);

	// draw it
	const int spacingOffset = 2;
	g->pushTransform();
		drawScoreNumber(g, accuracyInt, scale, true, spacingOffset);

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

void OsuHUD::drawAccuracy(Graphics *g, float accuracy)
{
	g->setColor(0xffffffff);

	// get integer & fractional parts of the number
	const int accuracyInt = (int)accuracy;
	const int accuracyFrac = clamp<int>(((int)(std::round((accuracy - accuracyInt)*1000.0f))) / 10, 0, 99);
	// what we want: 99.015 -> 99.01, 99.019 -> 99.01, 99.02 -> 99.02
	// if we only multiply with 100, floating point inaccuracies would cause things like 99.02 -> 99.01
	// to solve this for a known number of decimal digits, just increase the multiplier, round, and integer divide later to compensate

	// draw it
	const int spacingOffset = 2;
	const int offset = 5;
	const float scale = m_osu->getImageScale(m_osu, m_osu->getSkin()->getScore0(), 13) * osu_hud_scale.getFloat() * osu_hud_accuracy_scale.getFloat();
	g->pushTransform();

		// note that "spacingOffset*numDigits" would actually be used with (numDigits-1), but because we add a spacingOffset after the score dot we also have to add it here
		const int numDigits = (accuracyInt > 99 ? 5 : 4);
		const float xOffset = m_osu->getSkin()->getScore0()->getWidth()*scale*numDigits + m_osu->getSkin()->getScoreDot()->getWidth()*scale + m_osu->getSkin()->getScorePercent()->getWidth()*scale + spacingOffset*numDigits + 1;

		m_fAccuracyXOffset = m_osu->getScreenWidth() - xOffset - offset;
		m_fAccuracyYOffset = (osu_draw_score.getBool() ? m_fScoreHeight : 0.0f) + m_osu->getSkin()->getScore0()->getHeight()*scale/2 + offset*2;

		g->scale(scale, scale);
		g->translate(m_fAccuracyXOffset, m_fAccuracyYOffset);

		drawScoreNumber(g, accuracyInt, scale, true, spacingOffset);

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
	const float scale = osu_hud_scale.getFloat() * m_osu->getImageScale(m_osu, m_osu->getSkin()->getPlaySkip(), 96);

	g->setColor(0xffffffff);
	g->pushTransform();
		g->scale(scale, scale);
		g->translate(m_osu->getScreenWidth() - m_osu->getSkin()->getPlaySkip()->getWidth()*scale/2.0f, m_osu->getScreenHeight() - m_osu->getSkin()->getPlaySkip()->getHeight()*scale/2.0f, m_osu->isInVRMode() ? 0.5f : 0.0f);
		g->drawImage(m_osu->getSkin()->getPlaySkip());
	g->popTransform();
}

void OsuHUD::drawWarningArrow(Graphics *g, Vector2 pos, bool flipVertically, bool originLeft)
{
	const float scale = osu_hud_scale.getFloat() * m_osu->getImageScale(m_osu, m_osu->getSkin()->getPlayWarningArrow(), 78);

	g->pushTransform();
		g->scale(flipVertically ? -scale : scale, scale);
		g->translate(pos.x + (flipVertically ? (-m_osu->getSkin()->getPlayWarningArrow()->getWidth()*scale/2.0f) : (m_osu->getSkin()->getPlayWarningArrow()->getWidth()*scale/2.0f)) * (originLeft ? 1.0f : -1.0f), pos.y, m_osu->isInVRMode() ? 0.75f : 0.0f);
		g->drawImage(m_osu->getSkin()->getPlayWarningArrow());
	g->popTransform();
}

void OsuHUD::drawWarningArrows(Graphics *g, float hitcircleDiameter)
{
	const float divider = 18.0f;
	const float part = OsuGameRules::getPlayfieldSize(m_osu).y * (1.0f / divider);

	g->setColor(0xffffffff);
	drawWarningArrow(g, Vector2(m_osu->getUIScale(m_osu, 28), OsuGameRules::getPlayfieldCenter(m_osu).y - OsuGameRules::getPlayfieldSize(m_osu).y/2 + part*2), false);
	drawWarningArrow(g, Vector2(m_osu->getUIScale(m_osu, 28), OsuGameRules::getPlayfieldCenter(m_osu).y - OsuGameRules::getPlayfieldSize(m_osu).y/2 + part*2 + part*13), false);

	drawWarningArrow(g, Vector2(m_osu->getScreenWidth() - m_osu->getUIScale(m_osu, 28), OsuGameRules::getPlayfieldCenter(m_osu).y - OsuGameRules::getPlayfieldSize(m_osu).y/2 + part*2), true);
	drawWarningArrow(g, Vector2(m_osu->getScreenWidth() - m_osu->getUIScale(m_osu, 28), OsuGameRules::getPlayfieldCenter(m_osu).y - OsuGameRules::getPlayfieldSize(m_osu).y/2 + part*2 + part*13), true);
}

void OsuHUD::drawContinue(Graphics *g, Vector2 cursor, float hitcircleDiameter)
{
	Image *unpause = m_osu->getSkin()->getUnpause();
	const float unpauseScale = Osu::getImageScale(m_osu, unpause, 80);

	Image *cursorImage = m_osu->getSkin()->getDefaultCursor();
	const float cursorScale = Osu::getImageScaleToFitResolution(cursorImage, Vector2(hitcircleDiameter, hitcircleDiameter));

	// bleh
	if (cursor.x < cursorImage->getWidth() || cursor.y < cursorImage->getHeight() || cursor.x > m_osu->getScreenWidth()-cursorImage->getWidth() || cursor.y > m_osu->getScreenHeight()-cursorImage->getHeight())
		cursor = m_osu->getScreenSize()/2;

	// base
	g->setColor(COLOR(255, 255, 153, 51));
	g->pushTransform();
	g->scale(cursorScale, cursorScale);
	g->translate(cursor.x, cursor.y);
	g->drawImage(cursorImage);
	g->popTransform();

	// pulse animation
	float cursorAnimPulsePercent = clamp<float>(fmod(engine->getTime(), 1.35f), 0.0f, 1.0f);
	g->setColor(COLOR((short)(255.0f*(1.0f-cursorAnimPulsePercent)), 255, 153, 51));
	g->pushTransform();
	g->scale(cursorScale*(1.0f + cursorAnimPulsePercent), cursorScale*(1.0f + cursorAnimPulsePercent));
	g->translate(cursor.x, cursor.y);
	g->drawImage(cursorImage);
	g->popTransform();

	// unpause click message
	g->setColor(0xffffffff);
	g->pushTransform();
	g->scale(unpauseScale, unpauseScale);
	g->translate(cursor.x + 20 + (unpause->getWidth()/2)*unpauseScale, cursor.y + 20 + (unpause->getHeight()/2)*unpauseScale);
	g->drawImage(unpause);
	g->popTransform();
}

void OsuHUD::drawHitErrorBar(Graphics *g, float hitWindow300, float hitWindow100, float hitWindow50, float hitWindowMiss)
{
	const int brightnessSub = 50;
	const Color color300 = COLOR(255, 0, 255-brightnessSub, 255-brightnessSub);
	const Color color100 = COLOR(255, 0, 255-brightnessSub, 0);
	const Color color50 = COLOR(255, 255-brightnessSub, 165-brightnessSub, 0);
	const Color colorMiss = COLOR(255, 255-brightnessSub, 0, 0);

	Vector2 size = Vector2(m_osu->getScreenWidth()*osu_hud_hiterrorbar_width_percent.getFloat(), m_osu->getScreenHeight()*osu_hud_hiterrorbar_height_percent.getFloat())*osu_hud_scale.getFloat()*osu_hud_hiterrorbar_scale.getFloat();
	if (osu_hud_hiterrorbar_showmisswindow.getBool())
	{
		size = Vector2(m_osu->getScreenWidth()*osu_hud_hiterrorbar_width_percent_with_misswindow.getFloat(), m_osu->getScreenHeight()*osu_hud_hiterrorbar_height_percent.getFloat())*osu_hud_scale.getFloat()*osu_hud_hiterrorbar_scale.getFloat();
	}
	const Vector2 center = Vector2(m_osu->getScreenWidth()/2.0f, m_osu->getScreenHeight() - m_osu->getScreenHeight()*2.15f*osu_hud_hiterrorbar_height_percent.getFloat()*osu_hud_scale.getFloat()*osu_hud_hiterrorbar_scale.getFloat());

	const float entryHeight = size.y*osu_hud_hiterrorbar_bar_height_scale.getFloat();
	const float entryWidth = size.y*osu_hud_hiterrorbar_bar_width_scale.getFloat();

	float totalHitWindowLength  = hitWindow50;
	if (osu_hud_hiterrorbar_showmisswindow.getBool())
	{
		totalHitWindowLength  = hitWindowMiss; //400
	}
	const float percent50 = hitWindow50 / totalHitWindowLength;
	const float percent100 = hitWindow100 / totalHitWindowLength;
	const float percent300 = hitWindow300 / totalHitWindowLength;

	// draw background bar with color indicators for 300s, 100s and 50s
	if (osu_hud_hiterrorbar_showmisswindow.getBool())
	{
		g->setColor(colorMiss);
		g->fillRect(center.x - size.x/2.0f, center.y - size.y/2.0f, size.x, size.y);
	}
	if (!OsuGameRules::osu_mod_no100s.getBool() && !OsuGameRules::osu_mod_no50s.getBool())
	{
		g->setColor(color50);
		g->fillRect(center.x - size.x*percent50/2.0f, center.y - size.y/2.0f, size.x*percent50, size.y);
	}
	if (!OsuGameRules::osu_mod_ming3012.getBool() && !OsuGameRules::osu_mod_no100s.getBool())
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
			g->setColor(colorMiss);
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
	if (!osu_draw_accuracy.getBool())
		m_fAccuracyXOffset = m_osu->getScreenWidth();

	const float num_segments = 15*8;
	const int offset = 20;
	const float radius = m_osu->getUIScale(m_osu, 10.5f) * osu_hud_scale.getFloat() * osu_hud_progressbar_scale.getFloat();
	const float circularMetreScale = ((2*radius)/m_osu->getSkin()->getCircularmetre()->getWidth()) * 1.3f; // hardcoded 1.3 multiplier?!
	const float actualCircularMetreScale = ((2*radius)/m_osu->getSkin()->getCircularmetre()->getWidth());
	Vector2 center = Vector2(m_fAccuracyXOffset - radius - offset, m_fAccuracyYOffset);

	// clamp to top edge of screen
	if (center.y - (m_osu->getSkin()->getCircularmetre()->getHeight()*actualCircularMetreScale + 5)/2.0f < 0)
		center.y += std::abs(center.y - (m_osu->getSkin()->getCircularmetre()->getHeight()*actualCircularMetreScale + 5)/2.0f);

	// clamp to bottom edge of score numbers
	if (osu_draw_score.getBool() && center.y-radius < m_fScoreHeight)
		center.y = m_fScoreHeight + radius;

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
		g->translate(center.x, center.y, 0.65f);
		g->drawImage(m_osu->getSkin()->getCircularmetre());
	g->popTransform();
}

void OsuHUD::drawProgressBarVR(Graphics *g, Matrix4 &mvp, OsuVR *vr, float percent, bool waiting)
{
	if (!osu_draw_accuracy.getBool())
		m_fAccuracyXOffset = m_osu->getScreenWidth();

	const float num_segments = 15*8;
	const int offset = 20;
	const float radius = m_osu->getUIScale(m_osu, 10.5f) * osu_hud_scale.getFloat() * osu_hud_progressbar_scale.getFloat();
	const float circularMetreScale = ((2*radius)/m_osu->getSkin()->getCircularmetre()->getWidth()) * 1.3f; // hardcoded 1.3 multiplier?!
	const float actualCircularMetreScale = ((2*radius)/m_osu->getSkin()->getCircularmetre()->getWidth());
	Vector2 center = Vector2(m_fAccuracyXOffset - radius - offset, m_fAccuracyYOffset);

	// clamp to top edge of screen
	if (center.y - (m_osu->getSkin()->getCircularmetre()->getHeight()*actualCircularMetreScale + 5)/2.0f < 0)
		center.y += std::abs(center.y - (m_osu->getSkin()->getCircularmetre()->getHeight()*actualCircularMetreScale + 5)/2.0f);

	// clamp to bottom edge of score numbers
	if (osu_draw_score.getBool() && center.y-radius < m_fScoreHeight)
		center.y = m_fScoreHeight + radius;

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
		vr->getShaderUntexturedLegacyGeneric()->enable();
		vr->getShaderUntexturedLegacyGeneric()->setUniformMatrix4fv("matrix", mvp);
		{
			g->drawVAO(&vao);
		}
		vr->getShaderUntexturedLegacyGeneric()->disable();
		g->setAntialiasing(false);
	}

	// draw circularmetre
	vr->getShaderTexturedLegacyGeneric()->enable();
	vr->getShaderTexturedLegacyGeneric()->setUniformMatrix4fv("matrix", mvp);
	{
		g->setColor(0xffffffff);
		g->pushTransform();
			g->scale(circularMetreScale, circularMetreScale);
			g->translate(center.x, center.y, m_osu->isInVRMode() ? 0.65f : 0.0f);
			g->drawImage(m_osu->getSkin()->getCircularmetre());
		g->popTransform();
	}
}

void OsuHUD::drawStatistics(Graphics *g, int misses, int bpm, float ar, float cs, float od, int nps, int nd, int ur, int pp)
{
	g->pushTransform();
		g->scale(osu_hud_statistics_scale.getFloat()*osu_hud_scale.getFloat(), osu_hud_statistics_scale.getFloat()*osu_hud_scale.getFloat());
		g->translate(osu_hud_statistics_offset_x.getInt(), (int)((m_osu->getTitleFont()->getHeight())*osu_hud_scale.getFloat()*osu_hud_statistics_scale.getFloat()) + osu_hud_statistics_offset_y.getInt());

		const int yDelta = (int)((m_osu->getTitleFont()->getHeight() + 10)*osu_hud_scale.getFloat()*osu_hud_statistics_scale.getFloat());
		if (osu_draw_statistics_pp.getBool())
		{
			drawStatisticText(g, UString::format("%ipp", pp));
			g->translate(0, yDelta);
		}
		if (osu_draw_statistics_misses.getBool())
		{
			drawStatisticText(g, UString::format("Miss: %i", misses));
			g->translate(0, yDelta);
		}
		if (osu_draw_statistics_bpm.getBool())
		{
			drawStatisticText(g, UString::format("BPM: %i", bpm));
			g->translate(0, yDelta);
		}
		if (osu_draw_statistics_ar.getBool())
		{
			ar = std::round(ar * 100.0f) / 100.0f;
			drawStatisticText(g, UString::format("AR: %g", ar));
			g->translate(0, yDelta);
		}
		if (osu_draw_statistics_cs.getBool())
		{
			cs = std::round(cs * 100.0f) / 100.0f;
			drawStatisticText(g, UString::format("CS: %g", cs));
			g->translate(0, yDelta);
		}
		if (osu_draw_statistics_od.getBool())
		{
			od = std::round(od * 100.0f) / 100.0f;
			drawStatisticText(g, UString::format("OD: %g", od));
			g->translate(0, yDelta);
		}
		if (osu_draw_statistics_nps.getBool())
		{
			drawStatisticText(g, UString::format("NPS: %i", nps));
			g->translate(0, yDelta);
		}
		if (osu_draw_statistics_nd.getBool())
		{
			drawStatisticText(g, UString::format("ND: %i", nd));
			g->translate(0, yDelta);
		}
		if (osu_draw_statistics_ur.getBool())
		{
			drawStatisticText(g, UString::format("UR: %i", ur));
			g->translate(0, yDelta);
		}
	g->popTransform();
}

void OsuHUD::drawStatisticText(Graphics *g, const UString text)
{
	if (text.length() < 1)
		return;

	g->pushTransform();
		g->setColor(0xff000000);
		g->translate(0, 0, 0.25f);
		g->drawString(m_osu->getTitleFont(), text);

		g->setColor(0xffffffff);
		g->translate(-1, -1, 0.325f);
		g->drawString(m_osu->getTitleFont(), text);
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
		const float cs = std::cos(theta);
		const float sn = std::sin(theta);

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

float OsuHUD::getCursorScaleFactor()
{
	// FUCK OSU hardcoded piece of shit code
	const float spriteRes = 768;
	return (float)m_osu->getScreenHeight() / spriteRes;
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

void OsuHUD::resetHitErrorBar()
{
	m_hiterrors.clear();
}

Rect OsuHUD::getSkipClickRect()
{
	float skipScale = osu_hud_scale.getFloat() * Osu::getImageScale(m_osu, m_osu->getSkin()->getPlaySkip(), 120);
	return Rect(m_osu->getScreenWidth() - m_osu->getSkin()->getPlaySkip()->getWidth()*skipScale, m_osu->getScreenHeight() - m_osu->getSkin()->getPlaySkip()->getHeight()*skipScale, m_osu->getSkin()->getPlaySkip()->getWidth()*skipScale, m_osu->getSkin()->getPlaySkip()->getHeight()*skipScale);
}
