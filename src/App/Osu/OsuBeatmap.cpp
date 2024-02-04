//================ Copyright (c) 2015, PG, All rights reserved. =================//
//
// Purpose:		beatmap loader
//
// $NoKeywords: $osubm
//===============================================================================//

#include "OsuBeatmap.h"

#include "Engine.h"
#include "ResourceManager.h"
#include "Environment.h"
#include "SoundEngine.h"
#include "AnimationHandler.h"
#include "Keyboard.h"
#include "Mouse.h"
#include "Timer.h"
#include "ConVar.h"

#include "Osu.h"
#include "OsuVR.h"
#include "OsuMultiplayer.h"
#include "OsuHUD.h"
#include "OsuSkin.h"
#include "OsuSkinImage.h"
#include "OsuBackgroundImageHandler.h"
#include "OsuPauseMenu.h"
#include "OsuGameRules.h"
#include "OsuNotificationOverlay.h"
#include "OsuModSelector.h"
#include "OsuMainMenu.h"

#include "OsuDatabaseBeatmap.h"

#include "OsuBeatmapStandard.h"

#include "OsuHitObject.h"
#include "OsuCircle.h"
#include "OsuSlider.h"

#include <string.h>
#include <sstream>
#include <cctype>
#include <algorithm>

ConVar osu_pvs("osu_pvs", true, "optimizes all loops over all hitobjects by clamping the range to the Potentially Visible Set");
ConVar osu_draw_hitobjects("osu_draw_hitobjects", true);
ConVar osu_draw_beatmap_background_image("osu_draw_beatmap_background_image", true);
ConVar osu_vr_draw_desktop_playfield("osu_vr_draw_desktop_playfield", true);

ConVar osu_universal_offset("osu_universal_offset", 0.0f);
ConVar osu_universal_offset_hardcoded("osu_universal_offset_hardcoded", 0.0f);
ConVar osu_universal_offset_hardcoded_fallback_dsound("osu_universal_offset_hardcoded_fallback_dsound", -15.0f);
ConVar osu_old_beatmap_offset("osu_old_beatmap_offset", 24.0f, "offset in ms which is added to beatmap versions < 5 (default value is hardcoded 24 ms in stable)");
ConVar osu_timingpoints_offset("osu_timingpoints_offset", 5.0f, "Offset in ms which is added before determining the active timingpoint for the sample type and sample volume (hitsounds) of the current frame");
ConVar osu_interpolate_music_pos("osu_interpolate_music_pos", true, "Interpolate song position with engine time if the audio library reports the same position more than once");
ConVar osu_compensate_music_speed("osu_compensate_music_speed", true, "compensates speeds slower than 1x a little bit, by adding an offset depending on the slowness");
ConVar osu_combobreak_sound_combo("osu_combobreak_sound_combo", 20, "Only play the combobreak sound if the combo is higher than this");
ConVar osu_beatmap_preview_mods_live("osu_beatmap_preview_mods_live", false, "whether to immediately apply all currently selected mods while browsing beatmaps (e.g. speed/pitch)");
ConVar osu_beatmap_preview_music_loop("osu_beatmap_preview_music_loop", true);

ConVar osu_ar_override("osu_ar_override", -1.0f, "use this to override between AR 0 and AR 12.5+. active if value is more than or equal to 0.");
ConVar osu_ar_overridenegative("osu_ar_overridenegative", 0.0f, "use this to override below AR 0. active if value is less than 0, disabled otherwise. this override always overrides the other override.");
ConVar osu_cs_override("osu_cs_override", -1.0f, "use this to override between CS 0 and CS 12.1429. active if value is more than or equal to 0.");
ConVar osu_cs_overridenegative("osu_cs_overridenegative", 0.0f, "use this to override below CS 0. active if value is less than 0, disabled otherwise. this override always overrides the other override.");
ConVar osu_cs_cap_sanity("osu_cs_cap_sanity", true);
ConVar osu_hp_override("osu_hp_override", -1.0f);
ConVar osu_od_override("osu_od_override", -1.0f);
ConVar osu_ar_override_lock("osu_ar_override_lock", false, "always force constant AR even through speed changes");
ConVar osu_od_override_lock("osu_od_override_lock", false, "always force constant OD even through speed changes");

ConVar osu_background_dim("osu_background_dim", 0.9f);
ConVar osu_background_fade_after_load("osu_background_fade_after_load", true);
ConVar osu_background_dont_fade_during_breaks("osu_background_dont_fade_during_breaks", false);
ConVar osu_background_fade_min_duration("osu_background_fade_min_duration", 1.4f, "Only fade if the break is longer than this (in seconds)");
ConVar osu_background_fade_in_duration("osu_background_fade_in_duration", 0.85f);
ConVar osu_background_fade_out_duration("osu_background_fade_out_duration", 0.25f);
ConVar osu_background_brightness("osu_background_brightness", 0.0f, "0 to 1, if this is larger than 0 then it will replace the entire beatmap background image with a solid color (see osu_background_color_r/g/b)");
ConVar osu_background_color_r("osu_background_color_r", 255.0f, "0 to 255, only relevant if osu_background_brightness is larger than 0");
ConVar osu_background_color_g("osu_background_color_g", 255.0f, "0 to 255, only relevant if osu_background_brightness is larger than 0");
ConVar osu_background_color_b("osu_background_color_b", 255.0f, "0 to 255, only relevant if osu_background_brightness is larger than 0");
ConVar osu_hiterrorbar_misaims("osu_hiterrorbar_misaims", true);

ConVar osu_followpoints_prevfadetime("osu_followpoints_prevfadetime", 400.0f); // TODO: this shouldn't be in this class

ConVar osu_auto_and_relax_block_user_input("osu_auto_and_relax_block_user_input", true);

ConVar osu_mod_timewarp("osu_mod_timewarp", false);
ConVar osu_mod_timewarp_multiplier("osu_mod_timewarp_multiplier", 1.5f);
ConVar osu_mod_minimize("osu_mod_minimize", false);
ConVar osu_mod_minimize_multiplier("osu_mod_minimize_multiplier", 0.5f);
ConVar osu_mod_jigsaw1("osu_mod_jigsaw1", false);
ConVar osu_mod_artimewarp("osu_mod_artimewarp", false);
ConVar osu_mod_artimewarp_multiplier("osu_mod_artimewarp_multiplier", 0.5f);
ConVar osu_mod_arwobble("osu_mod_arwobble", false);
ConVar osu_mod_arwobble_strength("osu_mod_arwobble_strength", 1.0f);
ConVar osu_mod_arwobble_interval("osu_mod_arwobble_interval", 7.0f);
ConVar osu_mod_fullalternate("osu_mod_fullalternate", false);

ConVar osu_early_note_time("osu_early_note_time", 1500.0f, "Timeframe in ms at the beginning of a beatmap which triggers a starting delay for easier reading");
ConVar osu_quick_retry_time("osu_quick_retry_time", 2000.0f, "Timeframe in ms subtracted from the first hitobject when quick retrying (not regular retry)");
ConVar osu_end_delay_time("osu_end_delay_time", 750.0f, "Duration in ms which is added at the end of a beatmap after the last hitobject is finished but before the ranking screen is automatically shown");
ConVar osu_end_skip("osu_end_skip", true, "whether the beatmap jumps to the ranking screen as soon as the last hitobject plus lenience has passed");
ConVar osu_end_skip_time("osu_end_skip_time", 400.0f, "Duration in ms which is added to the endTime of the last hitobject, after which pausing the game will immediately jump to the ranking screen");
ConVar osu_skip_time("osu_skip_time", 5000.0f, "Timeframe in ms within a beatmap which allows skipping if it doesn't contain any hitobjects");
ConVar osu_fail_time("osu_fail_time", 2.25f, "Timeframe in s for the slowdown effect after failing, before the pause menu is shown");
ConVar osu_notelock_type("osu_notelock_type", 2, "which notelock algorithm to use (0 = None, 1 = McOsu, 2 = osu!stable, 3 = osu!lazer 2020)");
ConVar osu_notelock_stable_tolerance2b("osu_notelock_stable_tolerance2b", 3, "time tolerance in milliseconds to allow hitting simultaneous objects close together (e.g. circle at end of slider)");
ConVar osu_mod_suddendeath_restart("osu_mod_suddendeath_restart", false, "osu! has this set to false (i.e. you fail after missing). if set to true, then behave like SS/PF, instantly restarting the map");

ConVar osu_drain_type("osu_drain_type", 2, "which hp drain algorithm to use (0 = None, 1 = VR, 2 = osu!stable, 3 = osu!lazer 2020, 4 = osu!lazer 2018)");
ConVar osu_drain_kill("osu_drain_kill", true, "whether to kill the player upon failing");
ConVar osu_drain_kill_notification_duration("osu_drain_kill_notification_duration", 1.0f, "how long to display the \"You have failed, but you can keep playing!\" notification (0 = disabled)");

ConVar osu_drain_vr_duration("osu_drain_vr_duration", 0.35f);
ConVar osu_drain_stable_passive_fail("osu_drain_stable_passive_fail", false, "whether to fail the player instantly if health = 0, or only once a negative judgement occurs");
ConVar osu_drain_stable_break_before("osu_drain_stable_break_before", false, "drain after last hitobject before a break actually starts");
ConVar osu_drain_stable_break_before_old("osu_drain_stable_break_before_old", true, "for beatmap versions < 8, drain after last hitobject before a break actually starts");
ConVar osu_drain_stable_break_after("osu_drain_stable_break_after", false, "drain after a break before the next hitobject can be clicked");
ConVar osu_drain_lazer_passive_fail("osu_drain_lazer_passive_fail", false, "whether to fail the player instantly if health = 0, or only once a negative judgement occurs");
ConVar osu_drain_lazer_break_before("osu_drain_lazer_break_before", false, "drain after last hitobject before a break actually starts");
ConVar osu_drain_lazer_break_after("osu_drain_lazer_break_after", false, "drain after a break before the next hitobject can be clicked");
ConVar osu_drain_stable_spinner_nerf("osu_drain_stable_spinner_nerf", 0.25f, "drain gets multiplied with this while a spinner is active");
ConVar osu_drain_stable_hpbar_recovery("osu_drain_stable_hpbar_recovery", 160.0f, "hp gets set to this value when failing with ez and causing a recovery");

ConVar osu_play_hitsound_on_click_while_playing("osu_play_hitsound_on_click_while_playing", false);

ConVar osu_debug_draw_timingpoints("osu_debug_draw_timingpoints", false);

ConVar *OsuBeatmap::m_snd_speed_compensate_pitch_ref = NULL;
ConVar *OsuBeatmap::m_win_snd_fallback_dsound_ref = NULL;

ConVar *OsuBeatmap::m_osu_pvs = &osu_pvs;
ConVar *OsuBeatmap::m_osu_draw_hitobjects_ref = &osu_draw_hitobjects;
ConVar *OsuBeatmap::m_osu_vr_draw_desktop_playfield_ref = &osu_vr_draw_desktop_playfield;
ConVar *OsuBeatmap::m_osu_followpoints_prevfadetime_ref = &osu_followpoints_prevfadetime;
ConVar *OsuBeatmap::m_osu_universal_offset_ref = &osu_universal_offset;
ConVar *OsuBeatmap::m_osu_early_note_time_ref = &osu_early_note_time;
ConVar *OsuBeatmap::m_osu_fail_time_ref = &osu_fail_time;
ConVar *OsuBeatmap::m_osu_drain_type_ref = &osu_drain_type;

ConVar *OsuBeatmap::m_osu_draw_hud_ref = NULL;
ConVar *OsuBeatmap::m_osu_draw_scorebarbg_ref = NULL;
ConVar *OsuBeatmap::m_osu_hud_scorebar_hide_during_breaks_ref = NULL;
ConVar *OsuBeatmap::m_osu_drain_stable_hpbar_maximum_ref = NULL;
ConVar *OsuBeatmap::m_osu_volume_music_ref = NULL;
ConVar *OsuBeatmap::m_osu_mod_fposu_ref = NULL;
ConVar *OsuBeatmap::m_fposu_3d_ref = NULL;
ConVar *OsuBeatmap::m_fposu_draw_scorebarbg_on_top_ref = NULL;

ConVar *OsuBeatmap::m_osu_main_menu_shuffle_ref = NULL;

OsuBeatmap::OsuBeatmap(Osu *osu)
{
	// convar refs
	if (m_snd_speed_compensate_pitch_ref == NULL)
		m_snd_speed_compensate_pitch_ref = convar->getConVarByName("snd_speed_compensate_pitch");
	if (m_win_snd_fallback_dsound_ref == NULL)
		m_win_snd_fallback_dsound_ref = convar->getConVarByName("win_snd_fallback_dsound");

	if (m_osu_draw_hud_ref == NULL)
		m_osu_draw_hud_ref = convar->getConVarByName("osu_draw_hud");
	if (m_osu_draw_scorebarbg_ref == NULL)
		m_osu_draw_scorebarbg_ref = convar->getConVarByName("osu_draw_scorebarbg");
	if (m_osu_hud_scorebar_hide_during_breaks_ref == NULL)
		m_osu_hud_scorebar_hide_during_breaks_ref = convar->getConVarByName("osu_hud_scorebar_hide_during_breaks");
	if (m_osu_drain_stable_hpbar_maximum_ref == NULL)
		m_osu_drain_stable_hpbar_maximum_ref = convar->getConVarByName("osu_drain_stable_hpbar_maximum");
	if (m_osu_volume_music_ref == NULL)
		m_osu_volume_music_ref = convar->getConVarByName("osu_volume_music");
	if (m_osu_mod_fposu_ref == NULL)
		m_osu_mod_fposu_ref = convar->getConVarByName("osu_mod_fposu");
	if (m_fposu_3d_ref == NULL)
		m_fposu_3d_ref = convar->getConVarByName("fposu_3d");
	if (m_fposu_draw_scorebarbg_on_top_ref == NULL)
		m_fposu_draw_scorebarbg_on_top_ref = convar->getConVarByName("fposu_draw_scorebarbg_on_top");

	if (m_osu_main_menu_shuffle_ref == NULL)
		m_osu_main_menu_shuffle_ref = convar->getConVarByName("osu_main_menu_shuffle");

	// vars
	m_osu = osu;

	m_bIsPlaying = false;
	m_bIsPaused = false;
	m_bIsWaiting = false;
	m_bIsRestartScheduled = false;
	m_bIsRestartScheduledQuick = false;

	m_bIsInSkippableSection = false;
	m_bShouldFlashWarningArrows = false;
	m_fShouldFlashSectionPass = 0.0f;
	m_fShouldFlashSectionFail = 0.0f;
	m_bContinueScheduled = false;
	m_iContinueMusicPos = 0;
	m_fWaitTime = 0.0f;

	m_selectedDifficulty2 = NULL;

	m_music = NULL;

	m_fMusicFrequencyBackup = 44100.0f;
	m_iCurMusicPos = 0;
	m_iCurMusicPosWithOffsets = 0;
	m_bWasSeekFrame = false;
	m_fInterpolatedMusicPos = 0.0;
	m_fLastAudioTimeAccurateSet = 0.0;
	m_fLastRealTimeForInterpolationDelta = 0.0;
	m_iResourceLoadUpdateDelayHack = 0;
	m_bForceStreamPlayback = true; // if this is set to true here, then the music will always be loaded as a stream (meaning slow disk access could cause audio stalling/stuttering)
	m_fAfterMusicIsFinishedVirtualAudioTimeStart = -1.0f;
	m_bIsFirstMissSound = true;

	m_bFailed = false;
	m_fFailAnim = 1.0f;
	m_fHealth = 1.0;
	m_fHealth2 = 1.0f;

	m_fDrainRate = 0.0;
	m_fHpMultiplierNormal = 1.0;
	m_fHpMultiplierComboEnd = 1.0;

	m_fBreakBackgroundFade = 0.0f;
	m_bInBreak = false;
	m_currentHitObject = NULL;
	m_iNextHitObjectTime = 0;
	m_iPreviousHitObjectTime = 0;
	m_iPreviousSectionPassFailTime = -1;

	m_bClick1Held = false;
	m_bClick2Held = false;
	m_bClickedContinue = false;
	m_bPrevKeyWasKey1 = false;
	m_iAllowAnyNextKeyForFullAlternateUntilHitObjectIndex = 0;

	m_iRandomSeed = 0;

	m_iNPS = 0;
	m_iND = 0;
	m_iCurrentHitObjectIndex = 0;
	m_iCurrentNumCircles = 0;
	m_iCurrentNumSliders = 0;
	m_iCurrentNumSpinners = 0;
	m_iMaxPossibleCombo = 0;
	m_iScoreV2ComboPortionMaximum = 0;

	m_iPreviousFollowPointObjectIndex = -1;
}

OsuBeatmap::~OsuBeatmap()
{
	anim->deleteExistingAnimation(&m_fBreakBackgroundFade);
	anim->deleteExistingAnimation(&m_fHealth2);
	anim->deleteExistingAnimation(&m_fFailAnim);

	unloadObjects();
}

void OsuBeatmap::draw(Graphics *g)
{
	drawInt(g);
}

void OsuBeatmap::drawInt(Graphics *g)
{
	if (!canDraw()) return;

	// draw background
	drawBackground(g);

	// draw loading circle
	if (isLoading())
		m_osu->getHUD()->drawLoadingSmall(g);
}

void OsuBeatmap::drawVR(Graphics *g, Matrix4 &mvp, OsuVR *vr)
{
	if (!canDraw()) return;

	// empty atm
}

void OsuBeatmap::draw3D(Graphics *g)
{
	if (!canDraw()) return;

	// empty atm
}

void OsuBeatmap::drawDebug(Graphics *g)
{
	if (osu_debug_draw_timingpoints.getBool())
	{
		McFont *debugFont = engine->getResourceManager()->getFont("FONT_DEFAULT");
		g->setColor(0xffffffff);
		g->pushTransform();
		g->translate(5, debugFont->getHeight() + 5 - engine->getMouse()->getPos().y);
		{
			for (const OsuDatabaseBeatmap::TIMINGPOINT &t : m_selectedDifficulty2->getTimingpoints())
			{
				g->drawString(debugFont, UString::format("%li,%f,%i,%i,%i", t.offset, t.msPerBeat, t.sampleType, t.sampleSet, t.volume));
				g->translate(0, debugFont->getHeight());
			}
		}
		g->popTransform();
	}
}

void OsuBeatmap::drawBackground(Graphics *g)
{
	if (!canDraw()) return;

	// draw beatmap background image
	{
		Image *backgroundImage = m_osu->getBackgroundImageHandler()->getLoadBackgroundImage(m_selectedDifficulty2);
		if (osu_draw_beatmap_background_image.getBool() && backgroundImage != NULL && (osu_background_dim.getFloat() < 1.0f || m_fBreakBackgroundFade > 0.0f))
		{
			const float scale = Osu::getImageScaleToFillResolution(backgroundImage, m_osu->getScreenSize());
			const Vector2 centerTrans = (m_osu->getScreenSize()/2);

			const float backgroundFadeDimMultiplier = clamp<float>(1.0f - (osu_background_dim.getFloat() - 0.3f), 0.0f, 1.0f);
			const short dim = clamp<float>((1.0f - osu_background_dim.getFloat()) + m_fBreakBackgroundFade*backgroundFadeDimMultiplier, 0.0f, 1.0f)*255.0f;

			g->setColor(COLOR(255, dim, dim, dim));
			g->pushTransform();
			{
				g->scale(scale, scale);
				g->translate((int)centerTrans.x, (int)centerTrans.y);
				g->drawImage(backgroundImage);
			}
			g->popTransform();
		}
	}

	// draw background
	if (osu_background_brightness.getFloat() > 0.0f)
	{
		const float brightness = osu_background_brightness.getFloat();
		const short red = clamp<float>(brightness * osu_background_color_r.getFloat(), 0.0f, 255.0f);
		const short green = clamp<float>(brightness * osu_background_color_g.getFloat(), 0.0f, 255.0f);
		const short blue = clamp<float>(brightness * osu_background_color_b.getFloat(), 0.0f, 255.0f);
		const short alpha = clamp<float>(1.0f - m_fBreakBackgroundFade, 0.0f, 1.0f)*255.0f;
		g->setColor(COLOR(alpha, red, green, blue));
		g->fillRect(0, 0, m_osu->getScreenWidth(), m_osu->getScreenHeight());
	}

	// draw scorebar-bg
	if (m_osu_draw_hud_ref->getBool() && m_osu_draw_scorebarbg_ref->getBool() && (!m_osu_mod_fposu_ref->getBool() || (!m_fposu_3d_ref->getBool() && !m_fposu_draw_scorebarbg_on_top_ref->getBool()))) // NOTE: special case for FPoSu
		m_osu->getHUD()->drawScorebarBg(g, m_osu_hud_scorebar_hide_during_breaks_ref->getBool() ? (1.0f - m_fBreakBackgroundFade) : 1.0f, m_osu->getHUD()->getScoreBarBreakAnim());

	if (Osu::debug->getBool())
	{
		int y = 50;

		if (m_bIsPaused)
		{
			g->setColor(0xffffffff);
			g->fillRect(50, y, 15, 50);
			g->fillRect(50 + 50 - 15, y, 15, 50);
		}

		y += 100;

		if (m_bIsWaiting)
		{
			g->setColor(0xff00ff00);
			g->fillRect(50, y, 50, 50);
		}

		y += 100;

		if (m_bIsPlaying)
		{
			g->setColor(0xff0000ff);
			g->fillRect(50, y, 50, 50);
		}

		y += 100;

		if (m_bForceStreamPlayback)
		{
			g->setColor(0xffff0000);
			g->fillRect(50, y, 50, 50);
		}

		y += 100;

		if (m_hitobjectsSortedByEndTime.size() > 0)
		{
			OsuHitObject *lastHitObject = m_hitobjectsSortedByEndTime[m_hitobjectsSortedByEndTime.size() - 1];
			if (lastHitObject->isFinished() && m_iCurMusicPos > lastHitObject->getTime() + lastHitObject->getDuration() + (long)osu_end_skip_time.getInt())
			{
				g->setColor(0xff00ffff);
				g->fillRect(50, y, 50, 50);
			}

			y += 100;
		}
	}
}

void OsuBeatmap::update()
{
	if (!canUpdate()) return;

	if (m_bContinueScheduled)
	{
		bool isEarlyNoteContinue = (!m_bIsPaused && m_bIsWaiting); // if we paused while m_bIsWaiting (green progressbar), then we have to let the 'if (m_bIsWaiting)' block handle the sound play() call
		if (m_bClickedContinue || isEarlyNoteContinue) // originally was (m_bClick1Held || m_bClick2Held || isEarlyNoteContinue), replaced first two m_bClickedContinue
		{
			m_bClickedContinue = false;
			m_bContinueScheduled = false;
			m_bIsPaused = false;

			if (!isEarlyNoteContinue)
				engine->getSound()->play(m_music);

			m_bIsPlaying = true; // usually this should be checked with the result of the above play() call, but since we are continuing we can assume that everything works

			// for nightmare mod, to avoid a miss because of the continue click
			{
				m_clicks.clear();
				m_keyUps.clear();
			}
		}
	}

	// handle restarts
	if (m_bIsRestartScheduled)
	{
		m_bIsRestartScheduled = false;
		actualRestart();
		return;
	}

	// update current music position (this variable does not include any offsets!)
	m_iCurMusicPos = getMusicPositionMSInterpolated();
	m_iContinueMusicPos = m_music->getPositionMS();
	const bool wasSeekFrame = m_bWasSeekFrame;
	m_bWasSeekFrame = false;

	// handle timewarp
	if (osu_mod_timewarp.getBool())
	{
		if (m_hitobjects.size() > 0 && m_iCurMusicPos > m_hitobjects[0]->getTime())
		{
			const float percentFinished = ((double)(m_iCurMusicPos - m_hitobjects[0]->getTime()) / (double)(m_hitobjects[m_hitobjects.size()-1]->getTime() + m_hitobjects[m_hitobjects.size()-1]->getDuration() - m_hitobjects[0]->getTime()));
			const float speed = m_osu->getSpeedMultiplier() + percentFinished*m_osu->getSpeedMultiplier()*(osu_mod_timewarp_multiplier.getFloat()-1.0f);
			m_music->setSpeed(speed);
		}
	}

	// HACKHACK: clean this mess up
	// waiting to start (file loading, retry)
	// NOTE: this is dependent on being here AFTER m_iCurMusicPos has been set above, because it modifies it to fake a negative start (else everything would just freeze for the waiting period)
	if (m_bIsWaiting)
	{
		if (isLoading())
		{
			m_fWaitTime = engine->getTimeReal();

			// if the first hitobject starts immediately, add artificial wait time before starting the music
			if (!m_bIsRestartScheduledQuick && m_hitobjects.size() > 0)
			{
				if (m_hitobjects[0]->getTime() < (long)osu_early_note_time.getInt())
					m_fWaitTime = engine->getTimeReal() + osu_early_note_time.getFloat()/1000.0f;
			}
		}
		else
		{
			if (engine->getTimeReal() > m_fWaitTime)
			{
				if (!m_bIsPaused)
				{
					m_bIsWaiting = false;
					m_bIsPlaying = true;

					engine->getSound()->play(m_music);
					m_music->setPosition(0.0);
					m_music->setVolume(m_osu_volume_music_ref->getFloat());

					// if we are quick restarting, jump just before the first hitobject (even if there is a long waiting period at the beginning with nothing etc.)
					if (m_bIsRestartScheduledQuick && m_hitobjects.size() > 0 && m_hitobjects[0]->getTime() > (long)osu_quick_retry_time.getInt())
						m_music->setPositionMS(std::max((long)0, m_hitobjects[0]->getTime() - (long)osu_quick_retry_time.getInt()));

					m_bIsRestartScheduledQuick = false;

					onPlayStart();
				}
			}
			else
				m_iCurMusicPos = (engine->getTimeReal() - m_fWaitTime) * 1000.0f * m_osu->getSpeedMultiplier();
		}

		// ugh. force update all hitobjects while waiting (necessary because of pvs optimization)
		long curPos = m_iCurMusicPos
			+ (long)(osu_universal_offset.getFloat() * m_osu->getSpeedMultiplier())
			+ (long)osu_universal_offset_hardcoded.getInt()
			+ (m_win_snd_fallback_dsound_ref->getBool() ? (long)osu_universal_offset_hardcoded_fallback_dsound.getInt() : 0)
			- m_selectedDifficulty2->getLocalOffset()
			- m_selectedDifficulty2->getOnlineOffset()
			- (m_selectedDifficulty2->getVersion() < 5 ? osu_old_beatmap_offset.getInt() : 0);
		if (curPos > -1) // otherwise auto would already click elements that start at exactly 0 (while the map has not even started)
			curPos = -1;

		for (int i=0; i<m_hitobjects.size(); i++)
		{
			m_hitobjects[i]->update(curPos);
		}
	}

	// only continue updating hitobjects etc. if we have loaded everything
	if (isLoading()) return;

	// handle music loading fail
	if (!m_music->isReady())
	{
		m_iResourceLoadUpdateDelayHack++; // HACKHACK: async loading takes 1 additional engine update() until both isAsyncReady() and isReady() return true
		if (m_iResourceLoadUpdateDelayHack > 1 && !m_bForceStreamPlayback) // first: try loading a stream version of the music file
		{
			m_bForceStreamPlayback = true;
			unloadMusicInt();
			loadMusic(true, m_bForceStreamPlayback);

			// we are waiting for an asynchronous start of the beatmap in the next update()
			m_bIsWaiting = true;
			m_fWaitTime = engine->getTimeReal();
		}
		else if (m_iResourceLoadUpdateDelayHack > 3) // second: if that still doesn't work, stop and display an error message
		{
			m_osu->getNotificationOverlay()->addNotification("Couldn't load music file :(", 0xffff0000);
			stop(true);
		}
	}

	// detect and handle music end
	if (!m_bIsWaiting && m_music->isReady())
	{
		const bool isMusicFinished = m_music->isFinished();

		// trigger virtual audio time after music finishes
		if (!isMusicFinished)
			m_fAfterMusicIsFinishedVirtualAudioTimeStart = -1.0f;
		else if (m_fAfterMusicIsFinishedVirtualAudioTimeStart < 0.0f)
			m_fAfterMusicIsFinishedVirtualAudioTimeStart = engine->getTimeReal();

		if (isMusicFinished)
		{
			// continue with virtual audio time until the last hitobject is done (plus sanity offset given via osu_end_delay_time)
			// because some beatmaps have hitobjects going until >= the exact end of the music ffs
			// NOTE: this overwrites m_iCurMusicPos for the rest of the update loop
			m_iCurMusicPos = (long)m_music->getLengthMS() + (long)((engine->getTimeReal() - m_fAfterMusicIsFinishedVirtualAudioTimeStart)*1000.0f);
		}

		const bool hasAnyHitObjects = (m_hitobjects.size() > 0);
		const bool isTimePastLastHitObjectPlusLenience = (m_iCurMusicPos > (m_hitobjectsSortedByEndTime[m_hitobjectsSortedByEndTime.size()-1]->getTime() + m_hitobjectsSortedByEndTime[m_hitobjectsSortedByEndTime.size()-1]->getDuration() + (long)osu_end_delay_time.getInt()));
		if (!hasAnyHitObjects || (osu_end_skip.getBool() && isTimePastLastHitObjectPlusLenience) || (!osu_end_skip.getBool() && isMusicFinished))
		{
			if (!m_bFailed)
			{
				stop(false);
				return;
			}
		}
	}

	// update timing (points)
	m_iCurMusicPosWithOffsets = m_iCurMusicPos
		+ (long)(osu_universal_offset.getFloat() * m_osu->getSpeedMultiplier())
		+ (long)osu_universal_offset_hardcoded.getInt()
		+ (m_win_snd_fallback_dsound_ref->getBool() ? (long)osu_universal_offset_hardcoded_fallback_dsound.getInt() : 0)
		- m_selectedDifficulty2->getLocalOffset()
		- m_selectedDifficulty2->getOnlineOffset()
		- (m_selectedDifficulty2->getVersion() < 5 ? osu_old_beatmap_offset.getInt() : 0);
	updateTimingPoints(m_iCurMusicPosWithOffsets);

	// for performance reasons, a lot of operations are crammed into 1 loop over all hitobjects:
	// update all hitobjects,
	// handle click events,
	// also get the time of the next/previous hitobject and their indices for later,
	// and get the current hitobject,
	// also handle miss hiterrorbar slots,
	// also calculate nps and nd,
	// also handle note blocking
	m_currentHitObject = NULL;
	m_iNextHitObjectTime = 0;
	m_iPreviousHitObjectTime = 0;
	m_iPreviousFollowPointObjectIndex = 0;
	m_iNPS = 0;
	m_iND = 0;
	m_iCurrentNumCircles = 0;
	m_iCurrentNumSliders = 0;
	m_iCurrentNumSpinners = 0;
	{
		bool blockNextNotes = false;

		const long pvs = !OsuGameRules::osu_mod_mafham.getBool() ? getPVS() : (m_hitobjects.size() > 0 ? (m_hitobjects[clamp<int>(m_iCurrentHitObjectIndex + OsuGameRules::osu_mod_mafham_render_livesize.getInt() + 1, 0, m_hitobjects.size()-1)]->getTime() - m_iCurMusicPosWithOffsets + 1500) : getPVS());
		const bool usePVS = m_osu_pvs->getBool();

		const int notelockType = osu_notelock_type.getInt();
		const long tolerance2B = (long)osu_notelock_stable_tolerance2b.getInt();

		m_iCurrentHitObjectIndex = 0; // reset below here, since it's needed for mafham pvs

		for (int i=0; i<m_hitobjects.size(); i++)
		{
			// the order must be like this:
			// 0) miscellaneous stuff (minimal performance impact)
			// 1) prev + next time vars
			// 2) PVS optimization
			// 3) main hitobject update
			// 4) note blocking
			// 5) click events
			//
			// (because the hitobjects need to know about note blocking before handling the click events)

			// ************ live pp block start ************ //
			const bool isCircle = m_hitobjects[i]->isCircle();
			const bool isSlider = m_hitobjects[i]->isSlider();
			const bool isSpinner = m_hitobjects[i]->isSpinner();
			// ************ live pp block end ************** //

			// determine previous & next object time, used for auto + followpoints + warning arrows + empty section skipping
			if (m_iNextHitObjectTime == 0)
			{
				if (m_hitobjects[i]->getTime() > m_iCurMusicPosWithOffsets)
					m_iNextHitObjectTime = m_hitobjects[i]->getTime();
				else
				{
					m_currentHitObject = m_hitobjects[i];
					const long actualPrevHitObjectTime = m_hitobjects[i]->getTime() + m_hitobjects[i]->getDuration();
					m_iPreviousHitObjectTime = actualPrevHitObjectTime;

					if (m_iCurMusicPosWithOffsets > actualPrevHitObjectTime + (long)osu_followpoints_prevfadetime.getFloat())
						m_iPreviousFollowPointObjectIndex = i;
				}
			}

			// PVS optimization
			if (usePVS)
			{
				if (m_hitobjects[i]->isFinished() && (m_iCurMusicPosWithOffsets - pvs > m_hitobjects[i]->getTime() + m_hitobjects[i]->getDuration())) // past objects
				{
					// ************ live pp block start ************ //
					if (isCircle)
						m_iCurrentNumCircles++;
					if (isSlider)
						m_iCurrentNumSliders++;
					if (isSpinner)
						m_iCurrentNumSpinners++;

					m_iCurrentHitObjectIndex = i;
					// ************ live pp block end ************** //

					continue;
				}
				if (m_hitobjects[i]->getTime() > m_iCurMusicPosWithOffsets + pvs) // future objects
					break;
			}

			// ************ live pp block start ************ //
			if (m_iCurMusicPosWithOffsets >= m_hitobjects[i]->getTime() + m_hitobjects[i]->getDuration())
				m_iCurrentHitObjectIndex = i;
			// ************ live pp block end ************** //

			// main hitobject update
			m_hitobjects[i]->update(m_iCurMusicPosWithOffsets);

			// note blocking / notelock (1)
			const OsuSlider *currentSliderPointer = dynamic_cast<OsuSlider*>(m_hitobjects[i]);
			if (notelockType > 0)
			{
				m_hitobjects[i]->setBlocked(blockNextNotes);

				if (notelockType == 1) // McOsu
				{
					// (nothing, handled in (2) block)
				}
				else if (notelockType == 2) // osu!stable
				{
					if (!m_hitobjects[i]->isFinished())
					{
						blockNextNotes = true;

						// old implementation
						// sliders are "finished" after their startcircle
						/*
						{
							OsuSlider *sliderPointer = dynamic_cast<OsuSlider*>(m_hitobjects[i]);

							// sliders with finished startcircles do not block
							if (sliderPointer != NULL && sliderPointer->isStartCircleFinished())
								blockNextNotes = false;
						}
						*/

						// new implementation
						// sliders are "finished" after they end
						// extra handling for simultaneous/2b hitobjects, as these would now otherwise get blocked completely
						// NOTE: this will (same as the old implementation) still unlock some simultaneous/2b patterns too early (slider slider circle [circle]), but nobody from that niche has complained so far
						{
							const bool isSlider = (currentSliderPointer != NULL);
							const bool isSpinner = (!isSlider && !isCircle);

							if (isSlider || isSpinner)
							{
								if ((i + 1) < m_hitobjects.size())
								{
									if ((isSpinner || currentSliderPointer->isStartCircleFinished()) && (m_hitobjects[i + 1]->getTime() <= (m_hitobjects[i]->getTime() + m_hitobjects[i]->getDuration() + tolerance2B)))
										blockNextNotes = false;
								}
							}
						}
					}
				}
				else if (notelockType == 3) // osu!lazer 2020
				{
					if (!m_hitobjects[i]->isFinished())
					{
						const bool isSlider = (currentSliderPointer != NULL);
						const bool isSpinner = (!isSlider && !isCircle);

						if (!isSpinner) // spinners are completely ignored (transparent)
						{
							blockNextNotes = (m_iCurMusicPosWithOffsets <= m_hitobjects[i]->getTime());

							// sliders are "finished" after their startcircle
							{
								// sliders with finished startcircles do not block
								if (currentSliderPointer != NULL && currentSliderPointer->isStartCircleFinished())
									blockNextNotes = false;
							}
						}
					}
				}
			}
			else
				m_hitobjects[i]->setBlocked(false);

			// click events (this also handles hitsounds!)
			const bool isCurrentHitObjectASliderAndHasItsStartCircleFinishedBeforeClickEvents = (currentSliderPointer != NULL && currentSliderPointer->isStartCircleFinished());
			const bool isCurrentHitObjectFinishedBeforeClickEvents = m_hitobjects[i]->isFinished();
			{
				if (m_clicks.size() > 0)
					m_hitobjects[i]->onClickEvent(m_clicks);

				if (m_keyUps.size() > 0)
					m_hitobjects[i]->onKeyUpEvent(m_keyUps);
			}
			const bool isCurrentHitObjectFinishedAfterClickEvents = m_hitobjects[i]->isFinished();
			const bool isCurrentHitObjectASliderAndHasItsStartCircleFinishedAfterClickEvents = (currentSliderPointer != NULL && currentSliderPointer->isStartCircleFinished());

			// note blocking / notelock (2.1)
			if (!isCurrentHitObjectASliderAndHasItsStartCircleFinishedBeforeClickEvents && isCurrentHitObjectASliderAndHasItsStartCircleFinishedAfterClickEvents)
			{
				// in here if a slider had its startcircle clicked successfully in this update iteration

				if (notelockType == 2) // osu!stable
				{
					// edge case: frame perfect double tapping on overlapping sliders would incorrectly eat the second input, because the isStartCircleFinished() 2b edge case check handling happens before m_hitobjects[i]->onClickEvent(m_clicks);
					// so, we check if the currentSliderPointer got its isStartCircleFinished() within this m_hitobjects[i]->onClickEvent(m_clicks); and unlock blockNextNotes if that is the case
					// note that we still only unlock within duration + tolerance2B (same as in (1))
					if ((i + 1) < m_hitobjects.size())
					{
						if ((m_hitobjects[i + 1]->getTime() <= (m_hitobjects[i]->getTime() + m_hitobjects[i]->getDuration() + tolerance2B)))
							blockNextNotes = false;
					}
				}
			}

			// note blocking / notelock (2.2)
			if (!isCurrentHitObjectFinishedBeforeClickEvents && isCurrentHitObjectFinishedAfterClickEvents)
			{
				// in here if a hitobject has been clicked (and finished completely) successfully in this update iteration

				blockNextNotes = false;

				if (notelockType == 1) // McOsu
				{
					// auto miss all previous unfinished hitobjects, always
					// (can stop reverse iteration once we get to the first finished hitobject)

					for (int m=i-1; m>=0; m--)
					{
						if (!m_hitobjects[m]->isFinished())
						{
							const OsuSlider *sliderPointer = dynamic_cast<OsuSlider*>(m_hitobjects[m]);

							const bool isSlider = (sliderPointer != NULL);
							const bool isSpinner = (!isSlider && !isCircle);

							if (!isSpinner) // spinners are completely ignored (transparent)
							{
								if (m_hitobjects[i]->getTime() > (m_hitobjects[m]->getTime() + m_hitobjects[m]->getDuration())) // NOTE: 2b exception. only force miss if objects are not overlapping.
									m_hitobjects[m]->miss(m_iCurMusicPosWithOffsets);
							}
						}
						else
							break;
					}
				}
				else if (notelockType == 2) // osu!stable
				{
					// (nothing, handled in (1) and (2.1) blocks)
				}
				else if (notelockType == 3) // osu!lazer 2020
				{
					// auto miss all previous unfinished hitobjects if the current music time is > their time (center)
					// (can stop reverse iteration once we get to the first finished hitobject)

					for (int m=i-1; m>=0; m--)
					{
						if (!m_hitobjects[m]->isFinished())
						{
							const OsuSlider *sliderPointer = dynamic_cast<OsuSlider*>(m_hitobjects[m]);

							const bool isSlider = (sliderPointer != NULL);
							const bool isSpinner = (!isSlider && !isCircle);

							if (!isSpinner) // spinners are completely ignored (transparent)
							{
								if (m_iCurMusicPosWithOffsets > m_hitobjects[m]->getTime())
								{
									if (m_hitobjects[i]->getTime() > (m_hitobjects[m]->getTime() + m_hitobjects[m]->getDuration())) // NOTE: 2b exception. only force miss if objects are not overlapping.
										m_hitobjects[m]->miss(m_iCurMusicPosWithOffsets);
								}
							}
						}
						else
							break;
					}
				}
			}

			// ************ live pp block start ************ //
			if (isCircle && m_hitobjects[i]->isFinished())
				m_iCurrentNumCircles++;
			if (isSlider && m_hitobjects[i]->isFinished())
				m_iCurrentNumSliders++;
			if (isSpinner && m_hitobjects[i]->isFinished())
				m_iCurrentNumSpinners++;

			if (m_hitobjects[i]->isFinished())
				m_iCurrentHitObjectIndex = i;
			// ************ live pp block end ************** //

			// notes per second
			const long npsHalfGateSizeMS = (long)(500.0f * getSpeedMultiplier());
			if (m_hitobjects[i]->getTime() > m_iCurMusicPosWithOffsets-npsHalfGateSizeMS && m_hitobjects[i]->getTime() < m_iCurMusicPosWithOffsets+npsHalfGateSizeMS)
				m_iNPS++;

			// note density
			if (m_hitobjects[i]->isVisible())
				m_iND++;
		}

		// miss hiterrorbar slots
		// this gets the closest previous unfinished hitobject, as well as all following hitobjects which are in 50 range and could be clicked
		if (osu_hiterrorbar_misaims.getBool())
		{
			m_misaimObjects.clear();
			OsuHitObject *lastUnfinishedHitObject = NULL;
			const long hitWindow50 = (long)OsuGameRules::getHitWindow50(this);
			for (int i=0; i<m_hitobjects.size(); i++) // this shouldn't hurt performance too much, since no expensive operations are happening within the loop
			{
				if (!m_hitobjects[i]->isFinished())
				{
					if (m_iCurMusicPosWithOffsets >= m_hitobjects[i]->getTime())
						lastUnfinishedHitObject = m_hitobjects[i];
					else if (std::abs(m_hitobjects[i]->getTime() - m_iCurMusicPosWithOffsets) < hitWindow50)
						m_misaimObjects.push_back(m_hitobjects[i]);
					else
						break;
				}
			}
			if (lastUnfinishedHitObject != NULL && std::abs(lastUnfinishedHitObject->getTime() - m_iCurMusicPosWithOffsets) < hitWindow50)
				m_misaimObjects.insert(m_misaimObjects.begin(), lastUnfinishedHitObject);

			// now, go through the remaining clicks, and go through the unfinished hitobjects.
			// handle misaim clicks sequentially (setting the misaim flag on the hitobjects to only allow 1 entry in the hiterrorbar for misses per object)
			// clicks don't have to be consumed here, as they are deleted below anyway
			for (int c=0; c<m_clicks.size(); c++)
			{
				for (int i=0; i<m_misaimObjects.size(); i++)
				{
					if (m_misaimObjects[i]->hasMisAimed()) // only 1 slot per object!
						continue;

					m_misaimObjects[i]->misAimed();
					const long delta = (long)m_clicks[c].musicPos - (long)m_misaimObjects[i]->getTime();
					m_osu->getHUD()->addHitError(delta, false, true);

					break; // the current click has been dealt with (and the hitobject has been misaimed)
				}
			}
		}

		// all remaining clicks which have not been consumed by any hitobjects can safely be deleted
		if (m_clicks.size() > 0)
		{
			if (osu_play_hitsound_on_click_while_playing.getBool())
				m_osu->getSkin()->playHitCircleSound(0);

			// nightmare mod: extra clicks = sliderbreak
			if ((m_osu->getModNM() || osu_mod_jigsaw1.getBool()) && !m_bIsInSkippableSection && !m_bInBreak && m_iCurrentHitObjectIndex > 0)
			{
				addSliderBreak();
				addHitResult(NULL, OsuScore::HIT::HIT_MISS_SLIDERBREAK, 0, false, true, true, true, true, false); // only decrease health
			}

			m_clicks.clear();
		}
		m_keyUps.clear();
	}

	// empty section detection & skipping
	if (m_hitobjects.size() > 0)
	{
		const long legacyOffset = (m_iPreviousHitObjectTime < m_hitobjects[0]->getTime() ? 0 : 1000); // Mc
		const long nextHitObjectDelta = m_iNextHitObjectTime - (long)m_iCurMusicPosWithOffsets;
		if (nextHitObjectDelta > 0 && nextHitObjectDelta > (long)osu_skip_time.getInt() && m_iCurMusicPosWithOffsets > (m_iPreviousHitObjectTime + legacyOffset))
			m_bIsInSkippableSection = true;
		else if (!osu_end_skip.getBool() && nextHitObjectDelta < 0)
			m_bIsInSkippableSection = true;
		else
			m_bIsInSkippableSection = false;
	}

	// warning arrow logic
	if (m_hitobjects.size() > 0)
	{
		const long legacyOffset = (m_iPreviousHitObjectTime < m_hitobjects[0]->getTime() ? 0 : 1000); // Mc
		const long minGapSize = 1000;
		const long lastVisibleMin = 400;
		const long blinkDelta = 100;

		const long gapSize = m_iNextHitObjectTime - (m_iPreviousHitObjectTime + legacyOffset);
		const long nextDelta = (m_iNextHitObjectTime - m_iCurMusicPosWithOffsets);
		const bool drawWarningArrows = gapSize > minGapSize && nextDelta > 0;
		if (drawWarningArrows && ((nextDelta <= lastVisibleMin+blinkDelta*13 && nextDelta > lastVisibleMin+blinkDelta*12)
								|| (nextDelta <= lastVisibleMin+blinkDelta*11 && nextDelta > lastVisibleMin+blinkDelta*10)
								|| (nextDelta <= lastVisibleMin+blinkDelta*9 && nextDelta > lastVisibleMin+blinkDelta*8)
								|| (nextDelta <= lastVisibleMin+blinkDelta*7 && nextDelta > lastVisibleMin+blinkDelta*6)
								|| (nextDelta <= lastVisibleMin+blinkDelta*5 && nextDelta > lastVisibleMin+blinkDelta*4)
								|| (nextDelta <= lastVisibleMin+blinkDelta*3 && nextDelta > lastVisibleMin+blinkDelta*2)
								|| (nextDelta <= lastVisibleMin+blinkDelta*1 && nextDelta > lastVisibleMin)))
			m_bShouldFlashWarningArrows = true;
		else
			m_bShouldFlashWarningArrows = false;
	}

	// break time detection, and background fade during breaks
	const OsuDatabaseBeatmap::BREAK breakEvent = getBreakForTimeRange(m_iPreviousHitObjectTime, m_iCurMusicPosWithOffsets, m_iNextHitObjectTime);
	const bool isInBreak = ((int)m_iCurMusicPosWithOffsets >= breakEvent.startTime && (int)m_iCurMusicPosWithOffsets <= breakEvent.endTime);
	if (isInBreak != m_bInBreak)
	{
		m_bInBreak = !m_bInBreak;

		if (!osu_background_dont_fade_during_breaks.getBool() || m_fBreakBackgroundFade != 0.0f)
		{
			if (m_bInBreak && !osu_background_dont_fade_during_breaks.getBool())
			{
				const int breakDuration = breakEvent.endTime - breakEvent.startTime;
				if (breakDuration > (int)(osu_background_fade_min_duration.getFloat()*1000.0f))
					anim->moveLinear(&m_fBreakBackgroundFade, 1.0f, osu_background_fade_in_duration.getFloat(), true);
			}
			else
				anim->moveLinear(&m_fBreakBackgroundFade, 0.0f, osu_background_fade_out_duration.getFloat(), true);
		}
	}

	// section pass/fail logic
	if (m_hitobjects.size() > 0)
	{
		const long minGapSize = 2880;
		const long fadeStart = 1280;
		const long fadeEnd = 1480;

		const long gapSize = m_iNextHitObjectTime - m_iPreviousHitObjectTime;
		const long start = (gapSize / 2 > minGapSize ? m_iPreviousHitObjectTime + (gapSize / 2) : m_iNextHitObjectTime - minGapSize);
		const long nextDelta = m_iCurMusicPosWithOffsets - start;
		const bool inSectionPassFail = (gapSize > minGapSize && nextDelta > 0)
				&& m_iCurMusicPosWithOffsets > m_hitobjects[0]->getTime()
				&& m_iCurMusicPosWithOffsets < (m_hitobjectsSortedByEndTime[m_hitobjectsSortedByEndTime.size() - 1]->getTime() + m_hitobjectsSortedByEndTime[m_hitobjectsSortedByEndTime.size() - 1]->getDuration())
				&& !m_bFailed
				&& m_bInBreak && (breakEvent.endTime - breakEvent.startTime) > minGapSize;

		const bool passing = (m_fHealth >= 0.5);

		// draw logic
		if (passing)
		{
			if (inSectionPassFail && ((nextDelta <= fadeEnd && nextDelta >= 280)
										|| (nextDelta <= 230 && nextDelta >= 160)
										|| (nextDelta <= 100 && nextDelta >= 20)))
			{
				const float fadeAlpha = 1.0f - (float)(nextDelta - fadeStart) / (float)(fadeEnd - fadeStart);
				m_fShouldFlashSectionPass = (nextDelta > fadeStart ? fadeAlpha : 1.0f);
			}
			else
				m_fShouldFlashSectionPass = 0.0f;
		}
		else
		{
			if (inSectionPassFail && ((nextDelta <= fadeEnd && nextDelta >= 280)
										|| (nextDelta <= 230 && nextDelta >= 130)))
			{
				const float fadeAlpha = 1.0f - (float)(nextDelta - fadeStart) / (float)(fadeEnd - fadeStart);
				m_fShouldFlashSectionFail = (nextDelta > fadeStart ? fadeAlpha : 1.0f);
			}
			else
				m_fShouldFlashSectionFail = 0.0f;
		}

		// sound logic
		if (inSectionPassFail)
		{
			if (m_iPreviousSectionPassFailTime != start
				&& ((passing && nextDelta >= 20)
					|| (!passing && nextDelta >= 130)))
			{
				m_iPreviousSectionPassFailTime = start;

				if (!wasSeekFrame)
				{
					if (passing)
						engine->getSound()->play(m_osu->getSkin()->getSectionPassSound());
					else
						engine->getSound()->play(m_osu->getSkin()->getSectionFailSound());
				}
			}
		}
	}

	// hp drain & failing
	if (osu_drain_type.getInt() > 0)
	{
		const int drainType = osu_drain_type.getInt();

		// handle constant drain
		if (drainType == 2 || drainType == 3) // osu!stable + osu!lazer 2020
		{
			if (m_fDrainRate > 0.0)
			{
				if (m_bIsPlaying 					// not paused
					&& !m_bInBreak					// not in a break
					&& !m_bIsInSkippableSection)	// not in a skippable section
				{
					// special case: break drain edge cases
					bool drainAfterLastHitobjectBeforeBreakStart = false;
					bool drainBeforeFirstHitobjectAfterBreakEnd = false;

					if (drainType == 2) // osu!stable
					{
						drainAfterLastHitobjectBeforeBreakStart = (m_selectedDifficulty2->getVersion() < 8 ? osu_drain_stable_break_before_old.getBool() : osu_drain_stable_break_before.getBool());
						drainBeforeFirstHitobjectAfterBreakEnd = osu_drain_stable_break_after.getBool();
					}
					else if (drainType == 3) // osu!lazer 2020
					{
						drainAfterLastHitobjectBeforeBreakStart = osu_drain_lazer_break_before.getBool();
						drainBeforeFirstHitobjectAfterBreakEnd = osu_drain_lazer_break_after.getBool();
					}

					const bool isBetweenHitobjectsAndBreak = (int)m_iPreviousHitObjectTime <= breakEvent.startTime && (int)m_iNextHitObjectTime >= breakEvent.endTime && m_iCurMusicPosWithOffsets > m_iPreviousHitObjectTime;
					const bool isLastHitobjectBeforeBreakStart = isBetweenHitobjectsAndBreak && (int)m_iCurMusicPosWithOffsets <= breakEvent.startTime;
					const bool isFirstHitobjectAfterBreakEnd = isBetweenHitobjectsAndBreak && (int)m_iCurMusicPosWithOffsets >= breakEvent.endTime;

					if (!isBetweenHitobjectsAndBreak
						|| (drainAfterLastHitobjectBeforeBreakStart && isLastHitobjectBeforeBreakStart)
						|| (drainBeforeFirstHitobjectAfterBreakEnd && isFirstHitobjectAfterBreakEnd))
					{
						// special case: spinner nerf
						double spinnerDrainNerf = 1.0;

						if (drainType == 2) // osu!stable
						{
							OsuBeatmapStandard *standardPointer = dynamic_cast<OsuBeatmapStandard*>(this);
							if (standardPointer != NULL && standardPointer->isSpinnerActive())
								spinnerDrainNerf = (double)osu_drain_stable_spinner_nerf.getFloat();
						}

						addHealth(-m_fDrainRate * engine->getFrameTime() * (double)getSpeedMultiplier() * spinnerDrainNerf, false);
					}
				}
			}
		}

		// handle generic fail state (1) (see addHealth())
		{
			bool hasFailed = false;

			switch (drainType)
			{
			case 1: // VR
				hasFailed = m_fHealth2 < 0.001f;
				break;

			case 2: // osu!stable
				hasFailed = (m_fHealth < 0.001) && osu_drain_stable_passive_fail.getBool();
				break;

			case 3: // osu!lazer 2020
				hasFailed = (m_fHealth < 0.001) && osu_drain_lazer_passive_fail.getBool();
				break;

			default:
				hasFailed = (m_fHealth < 0.001);
				break;
			}

			if (hasFailed && !m_osu->getModNF())
				fail();
		}

		// revive in mp
		if (m_fHealth > 0.999 && m_osu->getScore()->isDead())
			m_osu->getScore()->setDead(false);

		// handle fail animation
		if (m_bFailed)
		{
			if (m_fFailAnim <= 0.0f)
			{
				if (m_music->isPlaying() || !m_osu->getPauseMenu()->isVisible())
				{
					engine->getSound()->pause(m_music);
					m_bIsPaused = true;

					m_osu->getPauseMenu()->setVisible(true);
					m_osu->updateConfineCursor();
				}
			}
			else
				m_music->setFrequency(m_fMusicFrequencyBackup*m_fFailAnim > 100 ? m_fMusicFrequencyBackup*m_fFailAnim : 100);
		}
	}
}

void OsuBeatmap::onKeyDown(KeyboardEvent &e)
{
	if (e == KEY_O && engine->getKeyboard()->isControlDown())
	{
		m_osu->toggleOptionsMenu();
		e.consume();
	}
}

void OsuBeatmap::onKeyUp(KeyboardEvent &e)
{
	// nothing
}

void OsuBeatmap::skipEmptySection()
{
	if (!m_bIsInSkippableSection) return;
	m_bIsInSkippableSection = false;

	const float offset = 2500.0f;
	float offsetMultiplier = m_osu->getSpeedMultiplier();
	{
		// only compensate if not within "normal" osu mod range (would make the game feel too different regarding time from skip until first hitobject)
		if (offsetMultiplier >= 0.74f && offsetMultiplier <= 1.51f)
			offsetMultiplier = 1.0f;

		// don't compensate speed increases at all actually
		if (offsetMultiplier > 1.0f)
			offsetMultiplier = 1.0f;

		// and cap slowdowns at sane value (~ spinner fadein start)
		if (offsetMultiplier <= 0.2f)
			offsetMultiplier = 0.2f;
	}

	const long nextHitObjectDelta = m_iNextHitObjectTime - (long)m_iCurMusicPosWithOffsets;

	if (!osu_end_skip.getBool() && nextHitObjectDelta < 0)
		m_music->setPositionMS(std::max(m_music->getLengthMS(), (unsigned long)1) - 1);
	else
		m_music->setPositionMS(std::max(m_iNextHitObjectTime - (long)(offset * offsetMultiplier), (long)0));

	engine->getSound()->play(m_osu->getSkin()->getMenuHit());
}

void OsuBeatmap::keyPressed1(bool mouse)
{
	if (m_bContinueScheduled)
		m_bClickedContinue = !m_osu->getModSelector()->isMouseInside();

	if (m_osu->isInVRMode() && !m_osu_vr_draw_desktop_playfield_ref->getBool()) return;

	if (osu_mod_fullalternate.getBool() && m_bPrevKeyWasKey1)
	{
		if (m_iCurrentHitObjectIndex > m_iAllowAnyNextKeyForFullAlternateUntilHitObjectIndex)
		{
			engine->getSound()->play(getSkin()->getCombobreak());
			return;
		}
	}

	// key overlay & counter
	m_osu->getHUD()->animateInputoverlay(mouse ? 3 : 1, true);
	if (!m_bInBreak && !m_bIsInSkippableSection && m_bIsPlaying && !m_bFailed)
		m_osu->getScore()->addKeyCount(mouse ? 3 : 1);

	m_bPrevKeyWasKey1 = true;
	m_bClick1Held = true;

	//debugLog("async music pos = %lu, curMusicPos = %lu, curMusicPosWithOffsets = %lu\n", m_music->getPositionMS(), m_iCurMusicPos, m_iCurMusicPosWithOffsets);
	//long curMusicPos = getMusicPositionMSInterpolated(); // this would only be useful if we also played hitsounds async! combined with checking which musicPos is bigger

	CLICK click;
	click.musicPos = m_iCurMusicPosWithOffsets;
	click.maniaColumn = -1;

	if ((!m_osu->getModAuto() && !m_osu->getModRelax()) || !osu_auto_and_relax_block_user_input.getBool())
		m_clicks.push_back(click);
}

void OsuBeatmap::keyPressed2(bool mouse)
{
	if (m_bContinueScheduled)
		m_bClickedContinue = !m_osu->getModSelector()->isMouseInside();

	if (m_osu->isInVRMode() && !m_osu_vr_draw_desktop_playfield_ref->getBool()) return;

	if (osu_mod_fullalternate.getBool() && !m_bPrevKeyWasKey1)
	{
		if (m_iCurrentHitObjectIndex > m_iAllowAnyNextKeyForFullAlternateUntilHitObjectIndex)
		{
			engine->getSound()->play(getSkin()->getCombobreak());
			return;
		}
	}

	// key overlay & counter
	m_osu->getHUD()->animateInputoverlay(mouse ? 4 : 2, true);
	if (!m_bInBreak && !m_bIsInSkippableSection && m_bIsPlaying && !m_bFailed)
		m_osu->getScore()->addKeyCount(mouse ? 4 : 2);

	m_bPrevKeyWasKey1 = false;
	m_bClick2Held = true;

	//debugLog("async music pos = %lu, curMusicPos = %lu, curMusicPosWithOffsets = %lu\n", m_music->getPositionMS(), m_iCurMusicPos, m_iCurMusicPosWithOffsets);
	//long curMusicPos = getMusicPositionMSInterpolated(); // this would only be useful if we also played hitsounds async! combined with checking which musicPos is bigger

	CLICK click;
	click.musicPos = m_iCurMusicPosWithOffsets;
	click.maniaColumn = -1;

	if ((!m_osu->getModAuto() && !m_osu->getModRelax()) || !osu_auto_and_relax_block_user_input.getBool())
		m_clicks.push_back(click);
}

void OsuBeatmap::keyReleased1(bool mouse)
{
	if (m_osu->isInVRMode() && !m_osu_vr_draw_desktop_playfield_ref->getBool()) return;

	// key overlay
	m_osu->getHUD()->animateInputoverlay(1, false);
	m_osu->getHUD()->animateInputoverlay(3, false);

	m_bClick1Held = false;
}

void OsuBeatmap::keyReleased2(bool mouse)
{
	if (m_osu->isInVRMode() && !m_osu_vr_draw_desktop_playfield_ref->getBool()) return;

	// key overlay
	m_osu->getHUD()->animateInputoverlay(2, false);
	m_osu->getHUD()->animateInputoverlay(4, false);

	m_bClick2Held = false;
}

void OsuBeatmap::select()
{
	// if possible, continue playing where we left off
	if (m_music != NULL && (m_music->isPlaying()))
		m_iContinueMusicPos = m_music->getPositionMS();

	selectDifficulty2(m_selectedDifficulty2);

	loadMusic();
	handlePreviewPlay();
}

void OsuBeatmap::selectDifficulty2(OsuDatabaseBeatmap *difficulty2)
{
	if (difficulty2 != NULL)
	{
		m_selectedDifficulty2 = difficulty2;

		// need to recheck/reload the music here since every difficulty might be using a different sound file
		loadMusic();
		handlePreviewPlay();
	}

	if (osu_beatmap_preview_mods_live.getBool())
		onModUpdate();
}

void OsuBeatmap::deselect()
{
	m_iContinueMusicPos = 0;

	unloadObjects();
}

bool OsuBeatmap::play()
{
	if (m_selectedDifficulty2 == NULL) return false;

	// reset everything, including deleting any previously loaded hitobjects from another diff which we might just have played
	unloadObjects();
	resetScoreInt();

	onBeforeLoad();

	// actually load the difficulty (and the hitobjects)
	{
		OsuDatabaseBeatmap::LOAD_GAMEPLAY_RESULT result = OsuDatabaseBeatmap::loadGameplay(m_selectedDifficulty2, this);
		if (result.errorCode != 0)
		{
			switch (result.errorCode)
			{
			case 1:
				{
					UString errorMessage = "Error: Couldn't load beatmap metadata :(";
					debugLog("Osu Error: Couldn't load beatmap metadata %s\n", m_selectedDifficulty2->getFilePath().toUtf8());

					if (m_osu != NULL)
						m_osu->getNotificationOverlay()->addNotification(errorMessage, 0xffff0000);
				}
				break;

			case 2:
				{
					UString errorMessage = "Error: Couldn't load beatmap file :(";
					debugLog("Osu Error: Couldn't load beatmap file %s\n", m_selectedDifficulty2->getFilePath().toUtf8());

					if (m_osu != NULL)
						m_osu->getNotificationOverlay()->addNotification(errorMessage, 0xffff0000);
				}
				break;

			case 3:
				{
					UString errorMessage = "Error: No timingpoints in beatmap :(";
					debugLog("Osu Error: No timingpoints in beatmap %s\n", m_selectedDifficulty2->getFilePath().toUtf8());

					if (m_osu != NULL)
						m_osu->getNotificationOverlay()->addNotification(errorMessage, 0xffff0000);
				}
				break;

			case 4:
				{
					UString errorMessage = "Error: No hitobjects in beatmap :(";
					debugLog("Osu Error: No hitobjects in beatmap %s\n", m_selectedDifficulty2->getFilePath().toUtf8());

					if (m_osu != NULL)
						m_osu->getNotificationOverlay()->addNotification(errorMessage, 0xffff0000);
				}
				break;

			case 5:
				{
					UString errorMessage = "Error: Too many hitobjects in beatmap :(";
					debugLog("Osu Error: Too many hitobjects in beatmap %s\n", m_selectedDifficulty2->getFilePath().toUtf8());

					if (m_osu != NULL)
						m_osu->getNotificationOverlay()->addNotification(errorMessage, 0xffff0000);
				}
				break;
			}

			return false;
		}

		// move temp result data into beatmap
		m_iRandomSeed = result.randomSeed;
		m_hitobjects = std::move(result.hitobjects);
		m_breaks = std::move(result.breaks);
		m_osu->getSkin()->setBeatmapComboColors(std::move(result.combocolors)); // update combo colors in skin

		// load beatmap skin
		m_osu->getSkin()->loadBeatmapOverride(m_selectedDifficulty2->getFolder());
	}

	// the drawing order is different from the playing/input order.
	// for drawing, if multiple hitobjects occupy the exact same time (duration) then they get drawn on top of the active hitobject
	m_hitobjectsSortedByEndTime = m_hitobjects;

	// sort hitobjects by endtime
	struct HitObjectSortComparator
	{
		bool operator() (OsuHitObject const *a, OsuHitObject const *b) const
		{
			// strict weak ordering!
			if ((a->getTime() + a->getDuration()) == (b->getTime() + b->getDuration()))
				return a->getSortHack() < b->getSortHack();
			else
				return (a->getTime() + a->getDuration()) < (b->getTime() + b->getDuration());
		}
	};
	std::sort(m_hitobjectsSortedByEndTime.begin(), m_hitobjectsSortedByEndTime.end(), HitObjectSortComparator());

	onLoad();

	// load music
	unloadMusicInt(); // need to reload in case of speed/pitch changes (just to be sure)
	loadMusic(false, m_bForceStreamPlayback);

	m_music->setLoop(false);
	m_bIsPaused = false;
	m_bContinueScheduled = false;

	m_bInBreak = osu_background_fade_after_load.getBool();
	anim->deleteExistingAnimation(&m_fBreakBackgroundFade);
	m_fBreakBackgroundFade = osu_background_fade_after_load.getBool() ? 1.0f : 0.0f;
	m_iPreviousSectionPassFailTime = -1;
	m_fShouldFlashSectionPass = 0.0f;
	m_fShouldFlashSectionFail = 0.0f;

	m_music->setPosition(0.0);
	m_iCurMusicPos = 0;

	// we are waiting for an asynchronous start of the beatmap in the next update()
	m_bIsWaiting = true;
	m_fWaitTime = engine->getTimeReal();

	// NOTE: loading failures are handled dynamically in update(), so temporarily assume everything has worked in here
	m_bIsPlaying = true;
	return m_bIsPlaying;
}

void OsuBeatmap::restart(bool quick)
{
	engine->getSound()->stop(getSkin()->getFailsound());

	if (!m_bIsWaiting)
	{
		m_bIsRestartScheduled = true;
		m_bIsRestartScheduledQuick = quick;
	}
	else if (m_bIsPaused)
		pause(false);

	onRestart(quick);
}

void OsuBeatmap::actualRestart()
{
	// reset everything
	resetScoreInt();
	resetHitObjects(-1000);

	// we are waiting for an asynchronous start of the beatmap in the next update()
	m_bIsWaiting = true;
	m_fWaitTime = engine->getTimeReal();

	// if the first hitobject starts immediately, add artificial wait time before starting the music
	if (m_hitobjects.size() > 0)
	{
		if (m_hitobjects[0]->getTime() < (long)osu_early_note_time.getInt())
		{
			m_bIsWaiting = true;
			m_fWaitTime = engine->getTimeReal() + osu_early_note_time.getFloat()/1000.0f;
		}
	}

	// pause temporarily if playing
	if (m_music->isPlaying())
		engine->getSound()->pause(m_music);

	// reset/restore frequency (from potential fail before)
	m_music->setFrequency(0);

	m_music->setLoop(false);
	m_bIsPaused = false;
	m_bContinueScheduled = false;

	m_bInBreak = false;
	anim->deleteExistingAnimation(&m_fBreakBackgroundFade);
	m_fBreakBackgroundFade = 0.0f;
	m_iPreviousSectionPassFailTime = -1;
	m_fShouldFlashSectionPass = 0.0f;
	m_fShouldFlashSectionFail = 0.0f;

	onModUpdate(); // sanity

	// reset position
	m_music->setPosition(0.0);
	m_iCurMusicPos = 0;

	m_bIsPlaying = true;
}

void OsuBeatmap::pause(bool quitIfWaiting)
{
	if (m_selectedDifficulty2 == NULL) return;

	const bool isFirstPause = !m_bContinueScheduled;
	const bool forceContinueWithoutSchedule = m_osu->isInMultiplayer();

	// NOTE: this assumes that no beatmap ever goes far beyond the end of the music
	// NOTE: if pure virtual audio time is ever supported (playing without SoundEngine) then this needs to be adapted
	// fix pausing after music ends breaking beatmap state (by just not allowing it to be paused)
	if (m_fAfterMusicIsFinishedVirtualAudioTimeStart >= 0.0f)
	{
		const float delta = engine->getTimeReal() - m_fAfterMusicIsFinishedVirtualAudioTimeStart;
		if (delta < 5.0f) // WARNING: sanity limit, always allow escaping after 5 seconds of overflow time
			return;
	}

	if (m_bIsPlaying) // if we are playing, aka if this is the first time pausing
	{
		if (m_bIsWaiting && quitIfWaiting) // if we are still m_bIsWaiting, pausing the game via the escape key is the same as stopping playing
			stop();
		else
		{
			// first time pause pauses the music
			// case 1: the beatmap is already "finished", jump to the ranking screen if some small amount of time past the last objects endTime
			// case 2: in the middle somewhere, pause as usual
			OsuHitObject *lastHitObject = m_hitobjectsSortedByEndTime.size() > 0 ? m_hitobjectsSortedByEndTime[m_hitobjectsSortedByEndTime.size()-1] : NULL;
			if (lastHitObject != NULL && lastHitObject->isFinished() && (m_iCurMusicPos > lastHitObject->getTime() + lastHitObject->getDuration() + (long)osu_end_skip_time.getInt()) && osu_end_skip.getBool())
				stop(false);
			else
			{
				engine->getSound()->pause(m_music);
				m_bIsPlaying = false;
				m_bIsPaused = true;
			}
		}
	}
	else if (m_bIsPaused && !m_bContinueScheduled) // if this is the first time unpausing
	{
		if (m_osu->getModAuto() || m_osu->getModAutopilot() || m_bIsInSkippableSection || forceContinueWithoutSchedule) // under certain conditions, immediately continue the beatmap without waiting for the user to click
		{
			if (!m_bIsWaiting) // only force play() if we were not early waiting
				engine->getSound()->play(m_music);

			m_bIsPlaying = true;
			m_bIsPaused = false;
		}
		else // otherwise, schedule a continue (wait for user to click, handled in update())
		{
			// first time unpause schedules a continue
			m_bIsPaused = false;
			m_bContinueScheduled = true;
		}
	}
	else // if this is not the first time pausing/unpausing, then just toggle the pause state (the visibility of the pause menu is handled in the Osu class, a bit shit)
		m_bIsPaused = !m_bIsPaused;

	if (m_bIsPaused)
		onPaused(isFirstPause);
	else
		onUnpaused();

	// don't kill VR players while paused
	if (m_osu_drain_type_ref->getInt() == 1)
		anim->deleteExistingAnimation(&m_fHealth2);

	// if we have failed, and the user early exits to the pause menu, stop the failing animation
	if (m_bFailed)
		anim->deleteExistingAnimation(&m_fFailAnim);
}

void OsuBeatmap::pausePreviewMusic(bool toggle)
{
	if (m_music != NULL)
	{
		if (m_music->isPlaying())
			engine->getSound()->pause(m_music);
		else if (toggle)
			engine->getSound()->play(m_music);
	}
}

bool OsuBeatmap::isPreviewMusicPlaying()
{
	if (m_music != NULL)
		return m_music->isPlaying();

	return false;
}

void OsuBeatmap::stop(bool quit)
{
	if (m_selectedDifficulty2 == NULL) return;

	if (getSkin()->getFailsound()->isPlaying())
		engine->getSound()->stop(getSkin()->getFailsound());

	m_currentHitObject = NULL;

	m_bIsPlaying = false;
	m_bIsPaused = false;
	m_bContinueScheduled = false;

	onBeforeStop(quit);

	unloadObjects();

	onStop(quit);

	m_osu->onPlayEnd(quit);
}

void OsuBeatmap::fail()
{
	if (m_bFailed) return;

	if (!m_osu->isInMultiplayer() && osu_drain_kill.getBool())
	{
		engine->getSound()->play(getSkin()->getFailsound());

		m_bFailed = true;
		m_fFailAnim = 1.0f;
		anim->moveLinear(&m_fFailAnim, 0.0f, osu_fail_time.getFloat(), true); // trigger music slowdown and delayed menu, see update()
	}
	else if (!m_osu->getScore()->isDead())
	{
		anim->deleteExistingAnimation(&m_fHealth2);
		m_fHealth = 0.0;
		m_fHealth2 = 0.0f;

		if (osu_drain_kill_notification_duration.getFloat() > 0.0f)
		{
			if (!m_osu->getScore()->hasDied())
				m_osu->getNotificationOverlay()->addNotification("You have failed, but you can keep playing!", 0xffffffff, false, osu_drain_kill_notification_duration.getFloat());
		}
	}

	if (!m_osu->getScore()->isDead())
		m_osu->getScore()->setDead(true);
}

void OsuBeatmap::cancelFailing()
{
	if (!m_bFailed || m_fFailAnim <= 0.0f) return;

	m_bFailed = false;

	anim->deleteExistingAnimation(&m_fFailAnim);
	m_fFailAnim = 1.0f;

	if (m_music != NULL)
		m_music->setFrequency(0.0f);

	if (getSkin()->getFailsound()->isPlaying())
		engine->getSound()->stop(getSkin()->getFailsound());
}

void OsuBeatmap::setVolume(float volume)
{
	if (m_music != NULL)
		m_music->setVolume(volume);
}

void OsuBeatmap::setSpeed(float speed)
{
	if (m_music != NULL)
		m_music->setSpeed(speed);
}

void OsuBeatmap::setPitch(float pitch)
{
	if (m_music != NULL)
		m_music->setPitch(pitch);
}

void OsuBeatmap::seekPercent(double percent)
{
	if (m_selectedDifficulty2 == NULL || (!m_bIsPlaying && !m_bIsPaused) || m_music == NULL || m_bFailed) return;

	m_osu->getMultiplayer()->onServerPlayStateChange(OsuMultiplayer::SEEK, (unsigned long)(m_music->getLengthMS() * percent));

	m_bWasSeekFrame = true;
	m_fWaitTime = 0.0f;

	m_music->setPosition(percent);
	m_music->setVolume(m_osu_volume_music_ref->getFloat());

	resetHitObjects(m_music->getPositionMS());
	resetScoreInt();

	m_iPreviousSectionPassFailTime = -1;

	if (m_bIsWaiting)
	{
		m_bIsWaiting = false;
		m_bIsPlaying = true;
		m_bIsRestartScheduledQuick = false;

		engine->getSound()->play(m_music);

		onPlayStart();
	}
}

void OsuBeatmap::seekPercentPlayable(double percent)
{
	if (m_selectedDifficulty2 == NULL || (!m_bIsPlaying && !m_bIsPaused) || m_music == NULL || m_bFailed) return;

	m_bWasSeekFrame = true;
	m_fWaitTime = 0.0f;

	double actualPlayPercent = percent;
	if (m_hitobjects.size() > 0)
		actualPlayPercent = (((double)m_hitobjects[m_hitobjects.size()-1]->getTime() + (double)m_hitobjects[m_hitobjects.size()-1]->getDuration()) * percent) / (double)m_music->getLengthMS();

	seekPercent(actualPlayPercent);
}

void OsuBeatmap::seekMS(unsigned long ms)
{
	if (m_selectedDifficulty2 == NULL || (!m_bIsPlaying && !m_bIsPaused) || m_music == NULL || m_bFailed) return;

	m_osu->getMultiplayer()->onServerPlayStateChange(OsuMultiplayer::SEEK, ms);

	m_bWasSeekFrame = true;
	m_fWaitTime = 0.0f;

	m_music->setPositionMS(ms);
	m_music->setVolume(m_osu_volume_music_ref->getFloat());

	resetHitObjects(m_music->getPositionMS());
	resetScoreInt();

	m_iPreviousSectionPassFailTime = -1;

	if (m_bIsWaiting)
	{
		m_bIsWaiting = false;
		engine->getSound()->play(m_music);
	}
}

unsigned long OsuBeatmap::getTime() const
{
	if (m_music != NULL && m_music->isAsyncReady())
		return m_music->getPositionMS();
	else
		return 0;
}

unsigned long OsuBeatmap::getStartTimePlayable() const
{
	if (m_hitobjects.size() > 0)
		return (unsigned long)m_hitobjects[0]->getTime();
	else
		return 0;
}

unsigned long OsuBeatmap::getLength() const
{
	if (m_music != NULL && m_music->isAsyncReady())
		return m_music->getLengthMS();
	else if (m_selectedDifficulty2 != NULL)
		return m_selectedDifficulty2->getLengthMS();
	else
		return 0;
}

unsigned long OsuBeatmap::getLengthPlayable() const
{
	if (m_hitobjects.size() > 0)
		return (unsigned long)((m_hitobjects[m_hitobjects.size()-1]->getTime() + m_hitobjects[m_hitobjects.size()-1]->getDuration()) - m_hitobjects[0]->getTime());
	else
		return getLength();
}

float OsuBeatmap::getPercentFinished() const
{
	if (m_music != NULL)
		return (float)m_iCurMusicPos / (float)m_music->getLengthMS();
	else
		return 0.0f;
}

float OsuBeatmap::getPercentFinishedPlayable() const
{
	if (m_bIsWaiting)
		return 1.0f - (m_fWaitTime - engine->getTimeReal())/(osu_early_note_time.getFloat()/1000.0f);

	if (m_hitobjects.size() > 0)
		return (float)m_iCurMusicPos / ((float)m_hitobjects[m_hitobjects.size()-1]->getTime() + (float)m_hitobjects[m_hitobjects.size()-1]->getDuration());
	else
		return (float)m_iCurMusicPos / (float)m_music->getLengthMS();
}

int OsuBeatmap::getMostCommonBPM() const
{
	if (m_selectedDifficulty2 != NULL)
	{
		if (m_music != NULL)
			return (int)(m_selectedDifficulty2->getMostCommonBPM() * m_music->getSpeed());
		else
			return (int)(m_selectedDifficulty2->getMostCommonBPM() * m_osu->getSpeedMultiplier());
	}
	else
		return 0;
}

float OsuBeatmap::getSpeedMultiplier() const
{
	if (m_music != NULL)
		return std::max(m_music->getSpeed(), 0.05f);
	else
		return 1.0f;
}

OsuSkin *OsuBeatmap::getSkin() const
{
	return m_osu->getSkin();
}

float OsuBeatmap::getRawAR() const
{
	if (m_selectedDifficulty2 == NULL) return 5.0f;

	return clamp<float>(m_selectedDifficulty2->getAR() * m_osu->getDifficultyMultiplier(), 0.0f, 10.0f);
}

float OsuBeatmap::getAR() const
{
	if (m_selectedDifficulty2 == NULL) return 5.0f;

	float AR = getRawAR();
	{
		if (osu_ar_override.getFloat() >= 0.0f)
			AR = osu_ar_override.getFloat();

		if (osu_ar_overridenegative.getFloat() < 0.0f)
			AR = osu_ar_overridenegative.getFloat();

		if (osu_ar_override_lock.getBool())
			AR = OsuGameRules::getRawConstantApproachRateForSpeedMultiplier(OsuGameRules::getRawApproachTime(AR), (m_music != NULL && m_bIsPlaying ? getSpeedMultiplier() : m_osu->getSpeedMultiplier()));

		if (osu_mod_artimewarp.getBool() && m_hitobjects.size() > 0)
		{
			const float percent = 1.0f - ((double)(m_iCurMusicPos - m_hitobjects[0]->getTime()) / (double)(m_hitobjects[m_hitobjects.size()-1]->getTime() + m_hitobjects[m_hitobjects.size()-1]->getDuration() - m_hitobjects[0]->getTime()))*(1.0f - osu_mod_artimewarp_multiplier.getFloat());
			AR *= percent;
		}

		if (osu_mod_arwobble.getBool())
			AR += std::sin((m_iCurMusicPos/1000.0f)*osu_mod_arwobble_interval.getFloat())*osu_mod_arwobble_strength.getFloat();
	}
	return AR;
}

float OsuBeatmap::getCS() const
{
	if (m_selectedDifficulty2 == NULL) return 5.0f;

	float CS = clamp<float>(m_selectedDifficulty2->getCS() * m_osu->getCSDifficultyMultiplier(), 0.0f, 10.0f);
	{
		if (osu_cs_override.getFloat() >= 0.0f)
			CS = osu_cs_override.getFloat();

		if (osu_cs_overridenegative.getFloat() < 0.0f)
			CS = osu_cs_overridenegative.getFloat();

		if (osu_mod_minimize.getBool() && m_hitobjects.size() > 0)
		{
			if (m_hitobjects.size() > 0)
			{
				const float percent = 1.0f + ((double)(m_iCurMusicPos - m_hitobjects[0]->getTime()) / (double)(m_hitobjects[m_hitobjects.size()-1]->getTime() + m_hitobjects[m_hitobjects.size()-1]->getDuration() - m_hitobjects[0]->getTime()))*osu_mod_minimize_multiplier.getFloat();
				CS *= percent;
			}
		}

		if (osu_cs_cap_sanity.getBool())
			CS = std::min(CS, 12.1429f);
	}
	return CS;
}

float OsuBeatmap::getHP() const
{
	if (m_selectedDifficulty2 == NULL) return 5.0f;

	float HP = clamp<float>(m_selectedDifficulty2->getHP() * m_osu->getDifficultyMultiplier(), 0.0f, 10.0f);
	if (osu_hp_override.getFloat() >= 0.0f)
		HP = osu_hp_override.getFloat();

	return HP;
}

float OsuBeatmap::getRawOD() const
{
	if (m_selectedDifficulty2 == NULL) return 5.0f;

	return clamp<float>(m_selectedDifficulty2->getOD() * m_osu->getDifficultyMultiplier(), 0.0f, 10.0f);
}

float OsuBeatmap::getOD() const
{
	float OD = getRawOD();
	if (osu_od_override.getFloat() >= 0.0f)
		OD = osu_od_override.getFloat();

	if (osu_od_override_lock.getBool())
		OD = OsuGameRules::getRawConstantOverallDifficultyForSpeedMultiplier(OsuGameRules::getRawHitWindow300(OD), (m_music != NULL && m_bIsPlaying ? getSpeedMultiplier() : m_osu->getSpeedMultiplier()));

	return OD;
}

bool OsuBeatmap::isClickHeld() const
{
	 return m_bClick1Held || m_bClick2Held || (m_osu->isInVRMode() && (!m_osu->getVR()->isVirtualCursorOnScreen() || !m_osu_vr_draw_desktop_playfield_ref->getBool())); // a bit shit, but whatever
}

UString OsuBeatmap::getTitle() const
{
	if (m_selectedDifficulty2 != NULL)
		return m_selectedDifficulty2->getTitle();
	else
		return "NULL";
}

UString OsuBeatmap::getArtist() const
{
	if (m_selectedDifficulty2 != NULL)
		return m_selectedDifficulty2->getArtist();
	else
		return "NULL";
}

unsigned long OsuBeatmap::getBreakDurationTotal() const
{
	unsigned long breakDurationTotal = 0;
	for (int i=0; i<m_breaks.size(); i++)
	{
		breakDurationTotal += (unsigned long)(m_breaks[i].endTime - m_breaks[i].startTime);
	}

	return breakDurationTotal;
}

OsuDatabaseBeatmap::BREAK OsuBeatmap::getBreakForTimeRange(long startMS, long positionMS, long endMS) const
{
	OsuDatabaseBeatmap::BREAK curBreak;

	curBreak.startTime = -1;
	curBreak.endTime = -1;

	for (int i=0; i<m_breaks.size(); i++)
	{
		if (m_breaks[i].startTime >= (int)startMS && m_breaks[i].endTime <= (int)endMS)
		{
			if ((int)positionMS >= curBreak.startTime)
				curBreak = m_breaks[i];
		}
	}

	return curBreak;
}

OsuScore::HIT OsuBeatmap::addHitResult(OsuHitObject *hitObject, OsuScore::HIT hit, long delta, bool isEndOfCombo, bool ignoreOnHitErrorBar, bool hitErrorBarOnly, bool ignoreCombo, bool ignoreScore, bool ignoreHealth)
{
	// handle perfect & sudden death
	if (m_osu->getModSS())
	{
		if (!hitErrorBarOnly
			&& hit != OsuScore::HIT::HIT_300
			&& hit != OsuScore::HIT::HIT_300G
			&& hit != OsuScore::HIT::HIT_SLIDER10
			&& hit != OsuScore::HIT::HIT_SLIDER30
			&& hit != OsuScore::HIT::HIT_SPINNERSPIN
			&& hit != OsuScore::HIT::HIT_SPINNERBONUS)
		{
			restart();
			return OsuScore::HIT::HIT_MISS;
		}
	}
	else if (m_osu->getModSD())
	{
		if (hit == OsuScore::HIT::HIT_MISS)
		{
			if (osu_mod_suddendeath_restart.getBool())
				restart();
			else
				fail();

			return OsuScore::HIT::HIT_MISS;
		}
	}

	// miss sound
	if (hit == OsuScore::HIT::HIT_MISS)
		playMissSound();

	// score
	m_osu->getScore()->addHitResult(this, hitObject, hit, delta, ignoreOnHitErrorBar, hitErrorBarOnly, ignoreCombo, ignoreScore);

	// health
	OsuScore::HIT returnedHit = OsuScore::HIT::HIT_MISS;
	if (!ignoreHealth)
	{
		addHealth(m_osu->getScore()->getHealthIncrease(this, hit), true);

		// geki/katu handling
		if (isEndOfCombo)
		{
			const int comboEndBitmask = m_osu->getScore()->getComboEndBitmask();

			if (comboEndBitmask == 0)
			{
				returnedHit = OsuScore::HIT::HIT_300G;
				addHealth(m_osu->getScore()->getHealthIncrease(this, returnedHit), true);
				m_osu->getScore()->addHitResultComboEnd(returnedHit);
			}
			else if ((comboEndBitmask & 2) == 0)
			{
				switch (hit)
				{
				case OsuScore::HIT::HIT_100:
					returnedHit = OsuScore::HIT::HIT_100K;
					addHealth(m_osu->getScore()->getHealthIncrease(this, returnedHit), true);
					m_osu->getScore()->addHitResultComboEnd(returnedHit);
					break;

				case OsuScore::HIT::HIT_300:
					returnedHit = OsuScore::HIT::HIT_300K;
					addHealth(m_osu->getScore()->getHealthIncrease(this, returnedHit), true);
					m_osu->getScore()->addHitResultComboEnd(returnedHit);
					break;
				}
			}
			else if (hit != OsuScore::HIT::HIT_MISS)
				addHealth(m_osu->getScore()->getHealthIncrease(this, OsuScore::HIT::HIT_MU), true);

			m_osu->getScore()->setComboEndBitmask(0);
		}
	}

	return returnedHit;
}

void OsuBeatmap::addSliderBreak()
{
	// handle perfect & sudden death
	if (m_osu->getModSS())
	{
		restart();
		return;
	}
	else if (m_osu->getModSD())
	{
		if (osu_mod_suddendeath_restart.getBool())
			restart();
		else
			fail();

		return;
	}

	// miss sound
	playMissSound();

	// score
	m_osu->getScore()->addSliderBreak();
}

void OsuBeatmap::addScorePoints(int points, bool isSpinner)
{
	m_osu->getScore()->addPoints(points, isSpinner);
}

void OsuBeatmap::addHealth(double percent, bool isFromHitResult)
{
	if (osu_drain_type.getInt() < 1) return;

	// never drain before first hitobject
	if (m_hitobjects.size() > 0 && m_iCurMusicPosWithOffsets < m_hitobjects[0]->getTime()) return;

	// never drain after last hitobject
	if (m_hitobjectsSortedByEndTime.size() > 0 && m_iCurMusicPosWithOffsets > (m_hitobjectsSortedByEndTime[m_hitobjectsSortedByEndTime.size() - 1]->getTime() + m_hitobjectsSortedByEndTime[m_hitobjectsSortedByEndTime.size() - 1]->getDuration())) return;

	if (m_bFailed)
	{
		anim->deleteExistingAnimation(&m_fHealth2);

		m_fHealth = 0.0;
		m_fHealth2 = 0.0f;

		return;
	}

	if (isFromHitResult && percent > 0.0)
	{
		m_osu->getHUD()->animateKiBulge();

		if (m_fHealth > 0.9)
			m_osu->getHUD()->animateKiExplode();
	}

	const int drainType = osu_drain_type.getInt();

	switch (drainType)
	{
	case 1: // VR
		{
			const float targetHealth = clamp<float>(m_fHealth2 + percent, -0.1f, 1.0f);
			m_fHealth = targetHealth;
			anim->moveQuadOut(&m_fHealth2, targetHealth, osu_drain_vr_duration.getFloat(), true);
		}
		break;

	default:
		m_fHealth = clamp<double>(m_fHealth + percent, 0.0, 1.0);
		break;
	}

	// handle generic fail state (2)
	const bool isDead = m_fHealth < 0.001;
	if (isDead && !m_osu->getModNF())
	{
		if (m_osu->getModEZ() && m_osu->getScore()->getNumEZRetries() > 0) // retries with ez
		{
			m_osu->getScore()->setNumEZRetries(m_osu->getScore()->getNumEZRetries() - 1);

			// special case: set health to 160/200 (osu!stable behavior, seems fine for all drains)
			m_fHealth = osu_drain_stable_hpbar_recovery.getFloat() / m_osu_drain_stable_hpbar_maximum_ref->getFloat();
			m_fHealth2 = (float)m_fHealth;

			anim->deleteExistingAnimation(&m_fHealth2);
		}
		else if (isFromHitResult && percent < 0.0) // judgement fail
		{
			switch (drainType)
			{
			case 2: // osu!stable
				if (!osu_drain_stable_passive_fail.getBool())
					fail();
				break;

			case 3: // osu!lazer 2020
				if (!osu_drain_lazer_passive_fail.getBool())
					fail();
				break;

			case 4: // osu!lazer 2018
				fail();
				break;
			}
		}
	}
}

void OsuBeatmap::updateTimingPoints(long curPos)
{
	if (curPos < 0) return; // aspire pls >:(

	///debugLog("updateTimingPoints( %ld )\n", curPos);

	const OsuDatabaseBeatmap::TIMING_INFO t = m_selectedDifficulty2->getTimingInfoForTime(curPos + (long)osu_timingpoints_offset.getInt());
	m_osu->getSkin()->setSampleSet(t.sampleType); // normal/soft/drum is stored in the sample type! the sample set number is for custom sets
	m_osu->getSkin()->setSampleVolume(clamp<float>(t.volume / 100.0f, 0.0f, 1.0f));
}

bool OsuBeatmap::isLoading()
{
	return (!m_music->isAsyncReady());
}

long OsuBeatmap::getPVS()
{
	// this is an approximation with generous boundaries, it doesn't need to be exact (just good enough to filter 10000 hitobjects down to a few hundred or so)
	// it will be used in both positive and negative directions (previous and future hitobjects) to speed up loops which iterate over all hitobjects
	return OsuGameRules::getApproachTime(this)
			+ OsuGameRules::getFadeInTime()
			+ (long)OsuGameRules::getHitWindowMiss(this)
			+ 1500; // sanity
}

bool OsuBeatmap::canDraw()
{
	if (!m_bIsPlaying && !m_bIsPaused && !m_bContinueScheduled && !m_bIsWaiting)
		return false;
	if (m_selectedDifficulty2 == NULL || m_music == NULL) // sanity check
		return false;

	return true;
}

bool OsuBeatmap::canUpdate()
{
	if (!m_bIsPlaying && !m_bIsPaused && !m_bContinueScheduled)
		return false;

	if (m_osu->getInstanceID() > 1)
	{
		m_music = engine->getResourceManager()->getSound("OSU_BEATMAP_MUSIC");
		if (m_music == NULL)
			return false;
	}

	return true;
}

void OsuBeatmap::handlePreviewPlay()
{
	if (m_music != NULL && (!m_music->isPlaying() || m_music->getPosition() > 0.95f) && m_selectedDifficulty2 != NULL)
	{
		// this is an assumption, but should be good enough for most songs
		// reset playback position when the song has nearly reached the end (when the user switches back to the results screen or the songbrowser after playing)
		if (m_music->getPosition() > 0.95f)
			m_iContinueMusicPos = 0;

		engine->getSound()->stop(m_music);

		if (engine->getSound()->play(m_music))
		{
			if (m_music->getFrequency() < m_fMusicFrequencyBackup) // player has died, reset frequency
				m_music->setFrequency(m_fMusicFrequencyBackup);

			if (m_osu_main_menu_shuffle_ref->getBool() && m_osu->getMainMenu()->isVisible())
				m_music->setPositionMS(0);
			else if (m_iContinueMusicPos != 0)
				m_music->setPositionMS(m_iContinueMusicPos);
			else
				m_music->setPositionMS(m_selectedDifficulty2->getPreviewTime() < 0 ? (unsigned long)(m_music->getLengthMS() * 0.40f) : m_selectedDifficulty2->getPreviewTime());

			m_music->setVolume(m_osu_volume_music_ref->getFloat());
		}
	}

	// always loop during preview
	if (m_music != NULL)
		m_music->setLoop(osu_beatmap_preview_music_loop.getBool());
}

void OsuBeatmap::loadMusic(bool stream, bool prescan)
{
	if (m_osu->getInstanceID() > 1)
	{
		m_music = engine->getResourceManager()->getSound("OSU_BEATMAP_MUSIC");
		return;
	}

	stream = stream || m_bForceStreamPlayback;
	m_iResourceLoadUpdateDelayHack = 0;

	// load the song (again)
	if (m_selectedDifficulty2 != NULL && (m_music == NULL || m_selectedDifficulty2->getFullSoundFilePath() != m_music->getFilePath() || !m_music->isReady()))
	{
		unloadMusicInt();

		// if it's not a stream then we are loading the entire song into memory for playing
		if (!stream)
			engine->getResourceManager()->requestNextLoadAsync();

		m_music = engine->getResourceManager()->loadSoundAbs(m_selectedDifficulty2->getFullSoundFilePath(), "OSU_BEATMAP_MUSIC", stream, false, false, m_bForceStreamPlayback && prescan); // m_bForceStreamPlayback = prescan necessary! otherwise big mp3s will go out of sync
		m_music->setVolume(m_osu_volume_music_ref->getFloat());
		m_fMusicFrequencyBackup = m_music->getFrequency();
	}
}

void OsuBeatmap::unloadMusicInt()
{
	if (m_osu->getInstanceID() < 2)
	{
		engine->getSound()->stop(m_music);
		engine->getResourceManager()->destroyResource(m_music);
	}

	m_music = NULL;
}

void OsuBeatmap::unloadObjects()
{
	for (int i=0; i<m_hitobjects.size(); i++)
	{
		delete m_hitobjects[i];
	}
	m_hitobjects = std::vector<OsuHitObject*>();
	m_hitobjectsSortedByEndTime = std::vector<OsuHitObject*>();
	m_misaimObjects = std::vector<OsuHitObject*>();

	m_breaks = std::vector<OsuDatabaseBeatmap::BREAK>();

	m_clicks = std::vector<CLICK>();
	m_keyUps = std::vector<CLICK>();
}

void OsuBeatmap::resetHitObjects(long curPos)
{
	for (int i=0; i<m_hitobjects.size(); i++)
	{
		m_hitobjects[i]->onReset(curPos);
		m_hitobjects[i]->update(curPos); // fgt
		m_hitobjects[i]->onReset(curPos);
	}
	m_osu->getHUD()->resetHitErrorBar();
}

void OsuBeatmap::resetScoreInt()
{
	m_fHealth = 1.0;
	m_fHealth2 = 1.0f;
	m_bFailed = false;
	m_fFailAnim = 1.0f;
	anim->deleteExistingAnimation(&m_fFailAnim);

	m_osu->getScore()->reset();

	m_bIsFirstMissSound = true;
}

void OsuBeatmap::playMissSound()
{
	if ((m_bIsFirstMissSound && m_osu->getScore()->getCombo() > 0) || m_osu->getScore()->getCombo() > osu_combobreak_sound_combo.getInt())
	{
		m_bIsFirstMissSound = false;
		engine->getSound()->play(getSkin()->getCombobreak());
	}
}

unsigned long OsuBeatmap::getMusicPositionMSInterpolated()
{
	if (!osu_interpolate_music_pos.getBool() || (m_bFailed && m_osu->isInVRMode()) || isLoading())
		return m_music->getPositionMS();
	else
	{
#ifdef MCENGINE_FEATURE_SDL_MIXER

		const double interpolationMultiplier = 2.0;

#else

		const double interpolationMultiplier = 1.0;

#endif

		// TODO: fix snapping at beginning for maps with instant start

		unsigned long returnPos = 0;
		const double curPos = (double)m_music->getPositionMS();
		const float speed = m_music->getSpeed();

		// not reinventing the wheel, the interpolation magic numbers here are (c) peppy

		const double realTime = engine->getTimeReal();
		const double interpolationDelta = (realTime - m_fLastRealTimeForInterpolationDelta) * 1000.0 * speed;
		const double interpolationDeltaLimit = ((realTime - m_fLastAudioTimeAccurateSet)*1000.0 < 1500 || speed < 1.0f ? 11 : 33) * interpolationMultiplier;

		if (m_music->isPlaying() && !m_bWasSeekFrame)
		{
			double newInterpolatedPos = m_fInterpolatedMusicPos + interpolationDelta;
			double delta = newInterpolatedPos - curPos;

			//debugLog("delta = %ld\n", (long)delta);

			// approach and recalculate delta
			newInterpolatedPos -= delta / 8.0 / interpolationMultiplier;
			delta = newInterpolatedPos - curPos;

			if (std::abs(delta) > interpolationDeltaLimit*2) // we're fucked, snap back to curPos
			{
				m_fInterpolatedMusicPos = (double)curPos;
			}
			else if (delta < -interpolationDeltaLimit) // undershot
			{
				m_fInterpolatedMusicPos += interpolationDelta * 2;
				m_fLastAudioTimeAccurateSet = realTime;
			}
			else if (delta < interpolationDeltaLimit) // normal
			{
				m_fInterpolatedMusicPos = newInterpolatedPos;
			}
			else // overshot
			{
				m_fInterpolatedMusicPos += interpolationDelta / 2;
				m_fLastAudioTimeAccurateSet = realTime;
			}

			// calculate final return value
			returnPos = (unsigned long)std::round(m_fInterpolatedMusicPos);
			if (speed < 1.0f && osu_compensate_music_speed.getBool() && m_snd_speed_compensate_pitch_ref->getBool())
				returnPos += (unsigned long)(((1.0f - speed) / 0.75f) * 5); // osu (new)
				///returnPos += (unsigned long)((1.0f / speed) * 9); // Mc (old)
		}
		else // no interpolation
		{
			returnPos = curPos;
			m_fInterpolatedMusicPos = (unsigned long)returnPos;
			m_fLastAudioTimeAccurateSet = realTime;
		}

		m_fLastRealTimeForInterpolationDelta = realTime; // this is more accurate than engine->getFrameTime() for the delta calculation, since it correctly handles all possible delays inbetween

		//debugLog("returning %lu \n", returnPos);
		//debugLog("delta = %lu\n", (long)returnPos - m_iCurMusicPos);
		//debugLog("raw delta = %ld\n", (long)returnPos - (long)curPos);

		return returnPos;
	}
}
