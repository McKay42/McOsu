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
#include "Mouse.h"
#include "ConVar.h"

#include <string.h>
#include <sstream>
#include <cctype>
#include <algorithm>

#include "Osu.h"
#include "OsuVR.h"
#include "OsuHUD.h"
#include "OsuSkin.h"
#include "OsuPauseMenu.h"
#include "OsuGameRules.h"
#include "OsuNotificationOverlay.h"
#include "OsuBeatmapDifficulty.h"

#include "OsuHitObject.h"
#include "OsuSlider.h"

ConVar osu_draw_hitobjects("osu_draw_hitobjects", true);
ConVar osu_vr_draw_desktop_playfield("osu_vr_draw_desktop_playfield", true);

ConVar osu_global_offset("osu_global_offset", 0.0f);
ConVar osu_interpolate_music_pos("osu_interpolate_music_pos", true, "Interpolate song position with engine time if the audio library reports the same position more than once");
ConVar osu_combobreak_sound_combo("osu_combobreak_sound_combo", 20, "Only play the combobreak sound if the combo is higher than this");

ConVar osu_ar_override("osu_ar_override", -1.0f);
ConVar osu_cs_override("osu_cs_override", -1.0f);
ConVar osu_hp_override("osu_hp_override", -1.0f);
ConVar osu_od_override("osu_od_override", -1.0f);

ConVar osu_background_dim("osu_background_dim", 0.9f);
ConVar osu_background_fade_during_breaks("osu_background_fade_during_breaks", true);
ConVar osu_background_fade_min_duration("osu_background_fade_min_duration", 1.4f, "Only fade if the break is longer than this (in seconds)");
ConVar osu_background_fadein_duration("osu_background_fadein_duration", 0.85f);
ConVar osu_background_fadeout_duration("osu_background_fadeout_duration", 0.25f);
ConVar osu_background_brightness("osu_background_brightness", 0.0f);
ConVar osu_hiterrorbar_misaims("osu_hiterrorbar_misaims", true);

ConVar osu_followpoints_prevfadetime("osu_followpoints_prevfadetime", 200.0f); // TODO: this shouldn't be in this class

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

ConVar osu_early_note_time("osu_early_note_time", 1000.0f, "Timeframe in ms at the beginning of a beatmap which triggers a starting delay for easier reading");
ConVar osu_skip_time("osu_skip_time", 5000.0f, "Timeframe in ms within a beatmap which allows skipping if it doesn't contain any hitobjects");
ConVar osu_fail_time("osu_fail_time", 2.25f, "Timeframe in s for the slowdown effect after failing, before the pause menu is shown");
ConVar osu_note_blocking("osu_note_blocking", true, "Whether to use not blocking or not");

ConVar osu_drain_enabled("osu_drain_enabled", false);
ConVar osu_debug_draw_timingpoints("osu_debug_draw_timingpoints", false);

ConVar *OsuBeatmap::m_osu_draw_hitobjects_ref = &osu_draw_hitobjects;
ConVar *OsuBeatmap::m_osu_vr_draw_desktop_playfield_ref = &osu_vr_draw_desktop_playfield;
ConVar *OsuBeatmap::m_osu_followpoints_prevfadetime_ref = &osu_followpoints_prevfadetime;
ConVar *OsuBeatmap::m_osu_global_offset_ref = &osu_global_offset;
ConVar *OsuBeatmap::m_osu_early_note_time_ref = &osu_early_note_time;
ConVar *OsuBeatmap::m_osu_fail_time_ref = &osu_fail_time;

ConVar *OsuBeatmap::m_osu_volume_music_ref = NULL;
ConVar *OsuBeatmap::m_osu_speed_override_ref = NULL;
ConVar *OsuBeatmap::m_osu_pitch_override_ref = NULL;

OsuBeatmap::OsuBeatmap(Osu *osu)
{
	// convar callbacks
	if (m_osu_volume_music_ref == NULL)
		m_osu_volume_music_ref = convar->getConVarByName("osu_volume_music");
	if (m_osu_speed_override_ref == NULL)
		m_osu_speed_override_ref = convar->getConVarByName("osu_speed_override");
	if (m_osu_pitch_override_ref == NULL)
		m_osu_pitch_override_ref = convar->getConVarByName("osu_pitch_override");

	// vars
	m_osu = osu;
	m_bIsPlaying = false;
	m_bIsPaused = false;
	m_bIsWaiting = false;
	m_bIsRestartScheduled = false;
	m_bIsRestartScheduledQuick = false;
	m_iContinueMusicPos = 0;

	m_bIsInSkippableSection = false;
	m_bShouldFlashWarningArrows = false;
	m_bContinueScheduled = false;
	m_iSelectedDifficulty = -1;
	m_selectedDifficulty = NULL;
	m_fWaitTime = 0.0f;

	m_music = NULL;

	m_iCurMusicPos = 0;
	m_bWasSeekFrame = false;
	m_fInterpolatedMusicPos = 0.0;
	m_fLastAudioTimeAccurateSet = 0.0;
	m_fLastRealTimeForInterpolationDelta = 0.0;

	m_bFailed = false;
	m_fFailTime = 0.0f;
	m_fHealth = 1.0f;
	m_fHealthReal = 1.0f;
	m_fBreakBackgroundFade = 0.0f;
	m_bInBreak = false;
	m_currentHitObject = NULL;
	m_iNextHitObjectTime = 0;
	m_iPreviousHitObjectTime = 0;

	m_bClick1Held = false;
	m_bClick2Held = false;

	m_iNPS = 0;
	m_iND = 0;

	m_iPreviousFollowPointObjectIndex = -1;
}

OsuBeatmap::~OsuBeatmap()
{
	unloadHitObjects();
	unloadMusic();

	for (int i=0; i<m_difficulties.size(); i++)
	{
		delete m_difficulties[i];
	}
	m_difficulties.clear();
}

void OsuBeatmap::setDifficulties(std::vector<OsuBeatmapDifficulty*> diffs)
{
	m_difficulties = diffs;
}

void OsuBeatmap::draw(Graphics *g)
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

void OsuBeatmap::drawDebug(Graphics *g)
{
	if (osu_debug_draw_timingpoints.getBool())
	{
		McFont *debugFont = engine->getResourceManager()->getFont("FONT_DEFAULT");
		g->setColor(0xffffffff);
		g->pushTransform();
		g->translate(5, debugFont->getHeight() + 5 - engine->getMouse()->getPos().y);
		for (int i=0; i<m_selectedDifficulty->timingpoints.size(); i++)
		{
			g->drawString(debugFont, UString::format("%li,%f,%i,%i,%i", m_selectedDifficulty->timingpoints[i].offset, m_selectedDifficulty->timingpoints[i].msPerBeat, m_selectedDifficulty->timingpoints[i].sampleType, m_selectedDifficulty->timingpoints[i].sampleSet, m_selectedDifficulty->timingpoints[i].volume));
			g->translate(0, debugFont->getHeight());
		}
		g->popTransform();
	}
}

void OsuBeatmap::drawBackground(Graphics *g)
{
	if (!canDraw()) return;

	// draw background
	if (osu_background_brightness.getFloat() > 0.0f)
	{
		const short brightness = clamp<float>(osu_background_brightness.getFloat(), 0.0f, 1.0f)*255.0f;
		g->setColor(COLOR(255, brightness, brightness, brightness));
		g->fillRect(0, 0, m_osu->getScreenWidth(), m_osu->getScreenHeight());
	}

	// draw beatmap background image
	if (m_selectedDifficulty->backgroundImage != NULL && (osu_background_dim.getFloat() < 1.0f || m_fBreakBackgroundFade > 0.0f))
	{
		const float scale = Osu::getImageScaleToFillResolution(m_selectedDifficulty->backgroundImage, m_osu->getScreenSize());
		const Vector2 centerTrans = (m_osu->getScreenSize()/2);

		const short dim = clamp<float>((1.0f - osu_background_dim.getFloat()) + m_fBreakBackgroundFade, 0.0f, 1.0f)*255.0f;

		g->setColor(COLOR(255, dim, dim, dim));
		g->pushTransform();
		g->scale(scale, scale);
		g->translate((int)centerTrans.x, (int)centerTrans.y);
		g->drawImage(m_selectedDifficulty->backgroundImage);
		g->popTransform();
	}
}

void OsuBeatmap::update()
{
	if (!canUpdate()) return;

	if (m_bContinueScheduled)
	{
		if (m_bClick1Held || m_bClick2Held)
		{
			m_bContinueScheduled = false;

			engine->getSound()->play(m_music);
			m_bIsPlaying = true;
			m_bIsPaused = false;

			// for nightmare mod, to avoid a miss because of the continue click
			{
				std::lock_guard<std::mutex> lk(m_clicksMutex);

				m_clicks.clear();
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
	// INFO: this is dependent on being here AFTER m_iCurMusicPos has been set above, because it modifies it to fake a negative start (else everything would just freeze for the waiting period)
	if (m_bIsWaiting)
	{
		if (isLoading())
		{
			m_fWaitTime = engine->getTimeReal();

			// if the first hitobject starts immediately, add artificial wait time before starting the music
			if (!m_bIsRestartScheduledQuick && m_hitobjects.size() > 0)
			{
				if (m_hitobjects[0]->getTime() < (long)osu_early_note_time.getInt())
					m_fWaitTime = engine->getTimeReal() + osu_early_note_time.getInt()/1000.0f;
			}
		}
		else
		{
			if (engine->getTimeReal() > m_fWaitTime)
			{
				m_bIsWaiting = false;
				m_bIsPlaying = engine->getSound()->play(m_music);
				m_music->setPosition(0.0);
				m_music->setVolume(m_osu_volume_music_ref->getFloat());

				// if we are quick restarting, jump just before the first hitobject (even if there is a long waiting period at the beginning with nothing etc.)
				if (m_bIsRestartScheduledQuick && m_hitobjects.size() > 0)
					m_music->setPositionMS(m_hitobjects[0]->getTime() - 1000);
				m_bIsRestartScheduledQuick = false;

				onPlayStart();
			}
			else
				m_iCurMusicPos = (engine->getTimeReal() - m_fWaitTime)*1000.0f*m_osu->getSpeedMultiplier();
		}
	}

	// handle music end
	if (!m_bIsWaiting && !isLoading() && (m_music->isFinished() || (m_hitobjects.size() > 0 && m_hitobjects[m_hitobjects.size()-1]->getTime() + m_hitobjects[m_hitobjects.size()-1]->getDuration() + 1000 < m_iCurMusicPos)))
	{
		if (m_hitobjects.size())
		{
			debugLog("stopped because of time = %ld, duration = %ld\n", m_hitobjects[m_hitobjects.size()-1]->getTime(), m_hitobjects[m_hitobjects.size()-1]->getDuration());
		}
		stop(false);
		return;
	}

	// only continue updating hitobjects etc. if we have loaded everything
	if (isLoading()) return;

	// handle music loading fail
	if (!m_music->isReady())
	{
		m_osu->getNotificationOverlay()->addNotification("Couldn't load music file :(", 0xffff0000);
		stop(true);
	}

	// update timing (points)
	OsuBeatmapDifficulty::TIMING_INFO t = m_selectedDifficulty->getTimingInfoForTime(m_iCurMusicPos);
	m_osu->getSkin()->setSampleSet(t.sampleSet);
	m_osu->getSkin()->setSampleVolume(clamp<float>(t.volume / 100.0f, 0.0f, 1.0f));

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
	{
		std::lock_guard<std::mutex> lk(m_clicksMutex); // we need to lock this up here, else it would be possible to insert a click just before calling m_clicks.clear(), thus missing it

		long curPos = m_iCurMusicPos + (long)osu_global_offset.getInt() - m_selectedDifficulty->localoffset - m_selectedDifficulty->onlineOffset;
		bool blockNextNotes = false;
		for (int i=0; i<m_hitobjects.size(); i++)
		{
			// the order must be like this:
			// 1) main hitobject update
			// 2) note blocking
			// 3) click events
			//
			// (because the hitobjects need to know about note blocking before handling the click events)

			// main hitobject update
			m_hitobjects[i]->update(curPos);

			// note blocking
			if (osu_note_blocking.getBool())
			{
				m_hitobjects[i]->setBlocked(false);
				if (blockNextNotes)
					m_hitobjects[i]->setBlocked(true);
				if (!m_hitobjects[i]->isFinished())
				{
					blockNextNotes = true;
					if (dynamic_cast<OsuSlider*>(m_hitobjects[i]) != NULL) // sliders break the blocking chain (until the next circle)
						blockNextNotes = false;
				}
			}

			// click events
			if (m_clicks.size() > 0)
				m_hitobjects[i]->onClickEvent(m_clicks);

			// used for auto later
			if (m_iNextHitObjectTime == 0)
			{
				if (m_hitobjects[i]->getTime() > m_iCurMusicPos)
					m_iNextHitObjectTime = (long)m_hitobjects[i]->getTime();
				else
				{
					m_currentHitObject = m_hitobjects[i];
					long actualPrevHitObjectTime = m_hitobjects[i]->getTime() + m_hitobjects[i]->getDuration();
					m_iPreviousHitObjectTime = actualPrevHitObjectTime + 1000; // why is there +1000 here again? wtf

					if (m_iCurMusicPos > actualPrevHitObjectTime+(long)osu_followpoints_prevfadetime.getFloat())
						m_iPreviousFollowPointObjectIndex = i;
				}
			}

			// notes per second
			const long npsHalfGateSizeMS = (long)(500.0f * getSpeedMultiplier());
			if (m_hitobjects[i]->getTime() > m_iCurMusicPos-npsHalfGateSizeMS && m_hitobjects[i]->getTime() < m_iCurMusicPos+npsHalfGateSizeMS)
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
			for (int i=0; i<m_hitobjects.size(); i++)
			{
				if (!m_hitobjects[i]->isFinished())
				{
					if (m_iCurMusicPos >= m_hitobjects[i]->getTime())
						lastUnfinishedHitObject = m_hitobjects[i];
					else if (std::abs(m_hitobjects[i]->getTime()-m_iCurMusicPos)  < (long)OsuGameRules::getHitWindow50(this))
					{
						m_misaimObjects.push_back(m_hitobjects[i]);
					}
					else
						break;
				}
			}
			if (lastUnfinishedHitObject != NULL && std::abs(lastUnfinishedHitObject->getTime()-m_iCurMusicPos)  < (long)OsuGameRules::getHitWindow50(this))
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
					long delta = (long)m_clicks[c].musicPos - (long)m_misaimObjects[i]->getTime();
					m_osu->getHUD()->addHitError(delta, false, true);

					break; // the current click has been dealt with (and the hitobject has been misaimed)
				}
			}
		}

		// all remaining clicks which have not been consumed by any hitobjects can safely be deleted
		if (m_clicks.size() > 0)
		{
			// nightmare mod: extra klicks = sliderbreak
			if ((m_osu->getModNM() || osu_mod_jigsaw1.getBool()) && !m_bIsInSkippableSection)
				addSliderBreak();
			m_clicks.clear();
		}
	}

	// empty section detection & skipping
	long nextHitObjectDelta = m_iNextHitObjectTime - (long)m_iCurMusicPos;
	if (nextHitObjectDelta > 0 && nextHitObjectDelta > (long)osu_skip_time.getInt() && m_iCurMusicPos > m_iPreviousHitObjectTime)
		m_bIsInSkippableSection = true;
	else
		m_bIsInSkippableSection = false;

	// warning arrow detection
	// this could probably be done more elegantly, but fuck it
	long gapSize = m_iNextHitObjectTime - m_iPreviousHitObjectTime;
	long nextDelta = m_iNextHitObjectTime - m_iCurMusicPos;
	bool drawWarningArrows = gapSize > 1000 && nextDelta > 0;
	long lastVisibleMin = 400;
	long blinkDelta = 100;
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

	// break time detection, and background fade during breaks
	if (m_selectedDifficulty->isInBreak(m_iCurMusicPos) != m_bInBreak)
	{
		m_bInBreak = !m_bInBreak;

		if (osu_background_fade_during_breaks.getBool())
		{
			if (m_bInBreak)
			{
				if (m_selectedDifficulty->getBreakDuration(m_iCurMusicPos) > (unsigned long)(osu_background_fade_min_duration.getFloat()*1000.0f))
					anim->moveLinear(&m_fBreakBackgroundFade, clamp<float>(1.0f - (osu_background_dim.getFloat() - 0.3f), 0.0f, 1.0f), osu_background_fadein_duration.getFloat(), true);
			}
			else
				anim->moveLinear(&m_fBreakBackgroundFade, 0.0f, osu_background_fadeout_duration.getFloat(), true);
		}
	}

	// hp drain & failing
	if (m_fHealth < 0.01f && !m_osu->getModNF()) // less than 1 percent, meaning that if health went from 100 to 0 in integer steps you would be dead
		fail();

	if (m_bFailed)
	{
		float failTimePercent = clamp<float>((m_fFailTime - engine->getTime()) / osu_fail_time.getFloat(), 0.0f, 1.0f); // goes from 1 to 0 over the duration of osu_fail_time
		if (failTimePercent <= 0.0f)
		{
			if (m_music->isPlaying() || !m_osu->getPauseMenu()->isVisible())
			{
				engine->getSound()->pause(m_music);
				m_osu->getPauseMenu()->setVisible(true);
				m_osu->updateConfineCursor();
			}
		}
		else
			m_music->setFrequency(44100*failTimePercent > 100 ? 44100*failTimePercent : 100);
	}
}

void OsuBeatmap::skipEmptySection()
{
	if (!m_bIsInSkippableSection)
		return;

	m_music->setPositionMS(m_iNextHitObjectTime - 2500);
	m_bIsInSkippableSection = false;

	engine->getSound()->play(m_osu->getSkin()->getMenuHit());
}

void OsuBeatmap::keyPressed1()
{
	m_bClick1Held = true;

	//debugLog("async music pos = %lu, curMusicPos = %lu\n", m_music->getPositionMS(), m_iCurMusicPos);
	//long curMusicPos = getMusicPositionMSInterpolated(); // this would only be useful if we also played hitsounds async! combined with checking which musicPos is bigger
	const long curPos = m_iCurMusicPos + (long)osu_global_offset.getInt() - m_selectedDifficulty->localoffset - m_selectedDifficulty->onlineOffset;

	CLICK click;
	click.musicPos = curPos;
	click.realTime = engine->getTimeReal();

	std::lock_guard<std::mutex> lk(m_clicksMutex);
	m_clicks.push_back(click);
}

void OsuBeatmap::keyPressed2()
{
	m_bClick2Held = true;

	//debugLog("async music pos = %lu, curMusicPos = %lu\n", m_music->getPositionMS(), m_iCurMusicPos);
	//long curMusicPos = getMusicPositionMSInterpolated(); // this would only be useful if we also played hitsounds async! combined with checking which musicPos is bigger
	const long curPos = m_iCurMusicPos + (long)osu_global_offset.getInt() - m_selectedDifficulty->localoffset - m_selectedDifficulty->onlineOffset;

	CLICK click;
	click.musicPos = curPos;
	click.realTime = engine->getTimeReal();

	std::lock_guard<std::mutex> lk(m_clicksMutex);
	m_clicks.push_back(click);
}

void OsuBeatmap::keyReleased1()
{
	m_bClick1Held = false;
}

void OsuBeatmap::keyReleased2()
{
	m_bClick2Held = false;
}

void OsuBeatmap::select()
{
	// if possible, continue playing where we left off
	if (m_music != NULL && (m_music->isPlaying()))
		m_iContinueMusicPos = m_music->getPositionMS();

	selectDifficulty(m_iSelectedDifficulty == -1 ? 0 : m_iSelectedDifficulty);

	loadMusic();
	handlePreviewPlay();
}

void OsuBeatmap::selectDifficulty(int index, bool deleteImage)
{
	if (m_selectedDifficulty != NULL && index != m_iSelectedDifficulty && deleteImage)
		m_selectedDifficulty->unloadBackgroundImage();

	unloadDiffs();

	if (index > -1 && index < m_difficulties.size())
	{
		m_iSelectedDifficulty = index;
		m_selectedDifficulty = m_difficulties[index];

		// quick direct load, but may cause lag if quickly select()-ing
		// this forces the currently selected diff to load the background image immediately (and prioritized over all others since the songbuttons have a loading delay)
		m_selectedDifficulty->loadBackgroundImage();

		// need to recheck/reload the music here since every difficulty might be using a different sound file
		loadMusic();
		handlePreviewPlay();
	}
}

void OsuBeatmap::selectDifficulty(OsuBeatmapDifficulty *difficulty, bool deleteImage)
{
	for (int i=0; i<m_difficulties.size(); i++)
	{
		if (m_difficulties[i] == difficulty)
		{
			selectDifficulty(i, deleteImage);
			break;
		}
	}
}

void OsuBeatmap::deselect(bool deleteImages)
{
	m_iContinueMusicPos = 0;

	engine->getSound()->stop(m_music);
	unloadMusic();
	unloadHitObjects();
	unloadDiffs();

	for (int i=0; i<m_difficulties.size(); i++)
	{
		if (deleteImages)
			m_difficulties[i]->unloadBackgroundImage();
	}
}

bool OsuBeatmap::play()
{
	if (m_selectedDifficulty == NULL)
		return false;

	// reset everything, including deleting any previously loaded hitobjects from another diff which we might just have played
	unloadHitObjects();
	resetScore();

	onBeforeLoad();

	// actually load the difficulty (and the hitobjects)
	if (!m_selectedDifficulty->loaded)
	{
		if (!m_selectedDifficulty->loadRaw(this, &m_hitobjects))
			return false;
		else
		{
			// the drawing order is different from the playing/input order.
			// for drawing, if multiple hitobjects occupy the same time (duration) then they get drawn on top of the active hitobject
			m_hitobjectsSortedByEndTime = m_hitobjects;

			// sort hitobjects by endtime
			struct HitObjectSortComparator
			{
			    bool operator() (OsuHitObject const *a, OsuHitObject const *b) const
			    {
			        return (a->getTime() + a->getDuration()) < (b->getTime() + b->getDuration());
			    }
			};
			std::sort(m_hitobjectsSortedByEndTime.begin(), m_hitobjectsSortedByEndTime.end(), HitObjectSortComparator());
		}
	}

	onLoad();

	// try to start the music so we can check if everything works (it is actually properly started again in the next update() by m_bIsWaiting)
	unloadMusic(); // need to reload in case of speed/pitch changes (just to be sure)
	loadMusic(false);

	m_music->setEnablePitchAndSpeedShiftingHack(true);
	m_bIsPlaying = engine->getSound()->play(m_music);
	m_bIsPaused = false;
	m_bContinueScheduled = false;
	m_bInBreak = false;
	anim->deleteExistingAnimation(&m_fBreakBackgroundFade);
	m_fBreakBackgroundFade = 0.0f;

	engine->getSound()->stop(m_music);
	m_music->setPosition(0.0);
	m_iCurMusicPos = 0;

	// we are waiting for an asynchronous start of the beatmap in the next update()
	m_bIsWaiting = true;
	m_fWaitTime = engine->getTimeReal();

	// HACKHACK: using the result from engine->getSound()->play() to return here whether we were successful in starting or not should work, but for some reason I changed it
	m_bIsPlaying = true;
	return true; // was m_bIsPlaying
}

void OsuBeatmap::restart(bool quick)
{
	engine->getSound()->stop(getSkin()->getFailsound());

	if (!m_bIsWaiting)
	{
		m_bIsRestartScheduled = true;
		m_bIsRestartScheduledQuick = quick;
	}
}

void OsuBeatmap::actualRestart()
{
	// reset everything
	resetScore();
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

	// try to start the music so we can check if everything works (it is actually properly started again in the next update() by m_bIsWaiting)
	if (m_music->isPlaying())
		engine->getSound()->stop(m_music);
	m_music->setEnablePitchAndSpeedShiftingHack(true);
	m_bIsPlaying = engine->getSound()->play(m_music);
	m_bIsPaused = false;
	m_bContinueScheduled = false;
	m_bInBreak = false;
	anim->deleteExistingAnimation(&m_fBreakBackgroundFade);
	m_fBreakBackgroundFade = 0.0f;

	onModUpdate(); // sanity

	m_music->setPosition(0.0);
	m_iCurMusicPos = 0;
	engine->getSound()->stop(m_music);

	// if for some reason we can't restart, stop everything
	if (!m_bIsPlaying)
	{
		debugLog("Osu Error: Couldn't restart music ...\n");
		stop();
	}
}

void OsuBeatmap::pause(bool quitIfWaiting)
{
	if (m_selectedDifficulty == NULL)
		return;

	if (m_bIsPlaying)
	{
		// workaround (+ actual osu behaviour): if we are still m_bIsWaiting, pausing the game is the same as stopping playing
		// TODO: this also happens if we alt-tab or lose focus
		if (m_bIsWaiting && quitIfWaiting)
			stop();
		else
		{
			engine->getSound()->pause(m_music);
			m_bIsPlaying = false;
			m_bIsPaused = true;
			m_bIsWaiting = false;

			onPaused();
		}
	}
	else if (m_bIsPaused && !m_bContinueScheduled)
	{
		if (m_osu->getModAuto() || m_osu->getModAutopilot() || m_bIsInSkippableSection)
		{
			engine->getSound()->play(m_music);
			m_bIsPlaying = true;
			m_bIsPaused = false;
			m_bIsWaiting = false;
		}
		else
		{
			m_bIsPaused = false;
			m_bContinueScheduled = true;
			m_bIsWaiting = false;
		}
	}
	else
		m_bIsPaused = !m_bIsPaused;
}

void OsuBeatmap::pausePreviewMusic()
{
	if (m_music != NULL)
	{
		if (m_music->isPlaying())
			engine->getSound()->pause(m_music);
		else
			engine->getSound()->play(m_music);
	}
}

void OsuBeatmap::stop(bool quit)
{
	if (m_selectedDifficulty == NULL) return;

	if (getSkin()->getFailsound()->isPlaying())
		engine->getSound()->stop(getSkin()->getFailsound());

	m_currentHitObject = NULL;

	m_bIsPlaying = false;
	m_bIsPaused = false;
	m_bContinueScheduled = false;

	onBeforeStop();

	unloadHitObjects();

	onStop();

	m_osu->onPlayEnd(quit);
}

void OsuBeatmap::fail()
{
	if (m_bFailed) return;

	engine->getSound()->play(getSkin()->getFailsound());

	m_bFailed = true;
	m_fFailTime = engine->getTime() + osu_fail_time.getFloat(); // trigger music slowdown and delayed menu, see update()
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
	if (m_selectedDifficulty == NULL || (!m_bIsPlaying && !m_bIsPaused) || m_music == NULL || m_bFailed)
		return;

	m_bWasSeekFrame = true;
	m_fWaitTime = 0.0f;

	m_music->setPosition(percent);

	resetHitObjects(m_music->getPositionMS());
	resetScore();
}

void OsuBeatmap::seekPercentPlayable(double percent)
{
	if (m_selectedDifficulty == NULL || (!m_bIsPlaying && !m_bIsPaused) || m_music == NULL || m_bFailed)
		return;

	m_bWasSeekFrame = true;
	m_fWaitTime = 0.0f;

	double actualPlayPercent = percent;
	if (m_hitobjects.size() > 0)
		actualPlayPercent = (((double)m_hitobjects[m_hitobjects.size()-1]->getTime() + (double)m_hitobjects[m_hitobjects.size()-1]->getDuration()) * percent) / (double)m_music->getLengthMS();

	seekPercent(actualPlayPercent);
}

void OsuBeatmap::seekMS(unsigned long ms)
{
	if (m_selectedDifficulty == NULL || (!m_bIsPlaying && !m_bIsPaused) || m_music == NULL || m_bFailed)
		return;

	m_bWasSeekFrame = true;
	m_fWaitTime = 0.0f;

	m_music->setPositionMS(ms);

	resetHitObjects(m_music->getPositionMS());
	resetScore();
}

unsigned long OsuBeatmap::getTime()
{
	if (m_music != NULL && m_music->isAsyncReady())
		return m_music->getPositionMS();
	else
		return 0;
}

unsigned long OsuBeatmap::getStartTimePlayable()
{
	if (m_hitobjects.size() > 0)
		return (unsigned long)m_hitobjects[0]->getTime();
	else
		return 0;
}

unsigned long OsuBeatmap::getLength()
{
	if (m_music != NULL && m_music->isAsyncReady())
		return m_music->getLengthMS();
	else if (m_difficulties.size() > 0) // a bit shitty, but it should work fine
	{
		unsigned long longest = 0;
		for (int i=0; i<m_difficulties.size(); i++)
		{
			if (m_difficulties[i]->lengthMS > longest)
				longest = m_difficulties[i]->lengthMS;
		}
		return longest;
	}
	else
		return 0;
}

unsigned long OsuBeatmap::getLengthPlayable()
{
	if (m_hitobjects.size() > 0)
		return (unsigned long)((m_hitobjects[m_hitobjects.size()-1]->getTime() + m_hitobjects[m_hitobjects.size()-1]->getDuration()) - m_hitobjects[0]->getTime());
	else
		return getLength();
}

float OsuBeatmap::getPercentFinished()
{
	if (m_music != NULL)
		return (float)m_iCurMusicPos / (float)m_music->getLengthMS();
	else
		return 0.0f;
}

float OsuBeatmap::getPercentFinishedPlayable()
{
	if (m_bIsWaiting)
		return 1.0f - (m_fWaitTime - engine->getTimeReal())/(osu_early_note_time.getFloat()/1000.0f);

	if (m_hitobjects.size() > 0)
		return (float)m_iCurMusicPos / ((float)m_hitobjects[m_hitobjects.size()-1]->getTime() + (float)m_hitobjects[m_hitobjects.size()-1]->getDuration());
	else
		return (float)m_iCurMusicPos / (float)m_music->getLengthMS();
}

int OsuBeatmap::getBPM()
{
	if (m_selectedDifficulty != NULL && m_music != NULL)
		return (int)(m_selectedDifficulty->maxBPM*m_music->getSpeed());
	else
		return 0;
}

float OsuBeatmap::getSpeedMultiplier()
{
	if (m_music != NULL)
		return m_music->getSpeed();
	else
		return 1.0f;
}

OsuSkin *OsuBeatmap::getSkin()
{
	return m_osu->getSkin();
}

float OsuBeatmap::getRawAR()
{
	if (m_selectedDifficulty == NULL)
		return 5.0f;

	return clamp<float>(m_selectedDifficulty->AR * m_osu->getDifficultyMultiplier(), 0.0f, 10.0f);
}

float OsuBeatmap::getAR()
{
	if (m_selectedDifficulty == NULL)
		return 5.0f;

	float AR = getRawAR();
	if (osu_ar_override.getFloat() >= 0.0f)
		AR = osu_ar_override.getFloat();

	if (osu_mod_artimewarp.getBool() && m_hitobjects.size() > 0)
	{
		float percent = 1.0f - ((double)(m_iCurMusicPos - m_hitobjects[0]->getTime()) / (double)(m_hitobjects[m_hitobjects.size()-1]->getTime() + m_hitobjects[m_hitobjects.size()-1]->getDuration() - m_hitobjects[0]->getTime()))*(1.0f - osu_mod_artimewarp_multiplier.getFloat());
		AR *= percent;
	}

	if (osu_mod_arwobble.getBool())
		AR += std::sin((m_iCurMusicPos/1000.0f)*osu_mod_arwobble_interval.getFloat())*osu_mod_arwobble_strength.getFloat();

	return AR;
}

float OsuBeatmap::getCS()
{
	if (m_selectedDifficulty == NULL)
		return 5.0f;

	float CS = clamp<float>(m_selectedDifficulty->CS * m_osu->getCSDifficultyMultiplier(), 0.0f, 10.0f);

	if (osu_cs_override.getFloat() >= 0.0f)
		CS = osu_cs_override.getFloat();

	if (osu_mod_minimize.getBool() && m_hitobjects.size() > 0)
	{
		if (m_hitobjects.size() > 0)
		{
			float percent = 1.0f + ((double)(m_iCurMusicPos - m_hitobjects[0]->getTime()) / (double)(m_hitobjects[m_hitobjects.size()-1]->getTime() + m_hitobjects[m_hitobjects.size()-1]->getDuration() - m_hitobjects[0]->getTime()))*osu_mod_minimize_multiplier.getFloat();
			CS *= percent;
		}
	}

	return CS;
}

float OsuBeatmap::getHP()
{
	if (m_selectedDifficulty == NULL)
		return 5.0f;

	float HP = clamp<float>(m_selectedDifficulty->HP * m_osu->getDifficultyMultiplier(), 0.0f, 10.0f);
	if (osu_hp_override.getFloat() >= 0.0f)
		HP = osu_hp_override.getFloat();

	return HP;
}

float OsuBeatmap::getRawOD()
{
	if (m_selectedDifficulty == NULL)
		return 5.0f;

	return clamp<float>(m_selectedDifficulty->OD * m_osu->getDifficultyMultiplier(), 0.0f, 10.0f);
}

float OsuBeatmap::getOD()
{
	float OD = getRawOD();
	if (osu_od_override.getFloat() >= 0.0f)
		OD = osu_od_override.getFloat();

	return OD;
}

bool OsuBeatmap::isClickHeld()
{
	 return m_bClick1Held || m_bClick2Held || (m_osu->isInVRMode() && !m_osu->getVR()->isVirtualCursorOnScreen()); // a bit shit, but whatever
}

UString OsuBeatmap::getTitle()
{
	if (m_difficulties.size() == 0)
		return "NULL";
	else
		return m_difficulties[0]->title;
}

UString OsuBeatmap::getArtist()
{
	if (m_difficulties.size() == 0)
		return "NULL";
	else
		return m_difficulties[0]->artist;
}

void OsuBeatmap::consumeClickEvent()
{
	// NOTE: don't need a lock_guard in here because this is only called during the hitobject update(), and there is already a lock there!
	m_clicks.erase(m_clicks.begin());
}

void OsuBeatmap::addHitResult(OsuScore::HIT hit, long delta, bool ignoreOnHitErrorBar, bool hitErrorBarOnly, bool ignoreCombo, bool ignoreScore)
{
	// handle perfect & sudden death
	if (m_osu->getModSS())
	{
		if (hit != OsuScore::HIT::HIT_300 && hit != OsuScore::HIT::HIT_SLIDER10 && hit != OsuScore::HIT::HIT_SLIDER30 && !hitErrorBarOnly)
		{
			restart();
			return;
		}
	}
	else if (m_osu->getModSD())
	{
		if (hit == OsuScore::HIT::HIT_MISS)
		{
			fail();
			return;
		}
	}

	if (hit == OsuScore::HIT::HIT_MISS)
		playMissSound();

	switch (hit)
	{
	case OsuScore::HIT::HIT_300:
		addHealth(0.035f);
		break;
	case OsuScore::HIT::HIT_100:
		addHealth(-0.10f);
		break;
	case OsuScore::HIT::HIT_50:
		addHealth(-0.125f);
		break;
	case OsuScore::HIT::HIT_MISS:
		addHealth(-0.15f);
		break;
	}

	m_osu->getScore()->addHitResult(this, hit, delta, ignoreOnHitErrorBar, hitErrorBarOnly, ignoreCombo, ignoreScore);
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
		fail();
		return;
	}

	addHealth(-0.10f);

	playMissSound();

	m_osu->getScore()->addSliderBreak();
}

void OsuBeatmap::addScorePoints(int points)
{
	m_osu->getScore()->addPoints(points);
}

void OsuBeatmap::addHealth(float percent)
{
	if (!osu_drain_enabled.getBool()) return;

	if (m_bFailed)
	{
		m_fHealthReal = 0.0f;
		m_fHealth = 0.0f;
		anim->deleteExistingAnimation(&m_fHealth);
		return;
	}

	float targetHealth = clamp<float>(m_fHealthReal + percent, -0.1f, 1.0f);
	m_fHealthReal = targetHealth;
	anim->moveQuadOut(&m_fHealth, targetHealth, 0.35f, true);
}

void OsuBeatmap::playMissSound()
{
	if (m_osu->getScore()->getCombo() > osu_combobreak_sound_combo.getInt())
		engine->getSound()->play(getSkin()->getCombobreak());
}

bool OsuBeatmap::isLoading()
{
	return !m_music->isAsyncReady();
}

bool OsuBeatmap::canDraw()
{
	if (!m_bIsPlaying && !m_bIsPaused && !m_bContinueScheduled && !m_bIsWaiting)
		return false;
	if (m_selectedDifficulty == NULL || m_music == NULL) // sanity check
		return false;

	return true;
}

bool OsuBeatmap::canUpdate()
{
	if (!m_bIsPlaying && !m_bIsPaused && !m_bContinueScheduled)
		return false;

	return true;
}

void OsuBeatmap::handlePreviewPlay()
{
	if (m_music != NULL && (!m_music->isPlaying() || m_music->getPosition() > 0.95f) && m_selectedDifficulty != NULL)
	{
		// this is an assumption, but should be good enough for most songs
		// reset playback position when the song has nearly reached the end (when the user switches back to the results screen or the songbrowser after playing)
		if (m_music->getPosition() > 0.95f)
			m_iContinueMusicPos = 0;

		engine->getSound()->stop(m_music);
		if (engine->getSound()->play(m_music))
		{
			if (m_iContinueMusicPos != 0)
				m_music->setPositionMS(m_iContinueMusicPos);
			else
				m_music->setPositionMS(m_selectedDifficulty->previewTime);
			m_music->setVolume(m_osu_volume_music_ref->getFloat());
		}
	}
}

void OsuBeatmap::loadMusic(bool stream)
{
	// load the song (again)
	if (m_selectedDifficulty != NULL && (m_music == NULL || m_selectedDifficulty->fullSoundFilePath != m_music->getFilePath() || !m_music->isReady()))
	{
		unloadMusic();

		// if it's not a stream then we are loading the entire song into memory for playing
		if (!stream)
			engine->getResourceManager()->requestNextLoadAsync();

		m_music = engine->getResourceManager()->loadSoundAbs(m_selectedDifficulty->fullSoundFilePath, "OSU_BEATMAP_MUSIC", stream);
		m_music->setVolume(m_osu_volume_music_ref->getFloat());
	}
}

void OsuBeatmap::unloadMusic()
{
	engine->getSound()->stop(m_music);
	engine->getResourceManager()->destroyResource(m_music);
	m_music = NULL;
}

void OsuBeatmap::unloadDiffs()
{
	for (int i=0; i<m_difficulties.size(); i++)
	{
		m_difficulties[i]->unload();
	}
}

void OsuBeatmap::unloadHitObjects()
{
	for (int i=0; i<m_hitobjects.size(); i++)
	{
		delete m_hitobjects[i];
	}
	m_hitobjects = std::vector<OsuHitObject*>();
	m_hitobjectsSortedByEndTime = std::vector<OsuHitObject*>();
	m_misaimObjects = std::vector<OsuHitObject*>();
}

void OsuBeatmap::resetHitObjects(long curPos)
{
	for (int i=0; i<m_hitobjects.size(); i++)
	{
		m_hitobjects[i]->onReset(curPos);
	}
	m_osu->getHUD()->resetHitErrorBar();
}

void OsuBeatmap::resetScore()
{
	anim->deleteExistingAnimation(&m_fHealth);
	m_fHealth = 1.0f;
	m_fHealthReal = 1.0f;
	m_bFailed = false;

	m_osu->getScore()->reset();
}

unsigned long OsuBeatmap::getMusicPositionMSInterpolated()
{
	if (!osu_interpolate_music_pos.getBool() || m_bFailed)
		return m_music->getPositionMS();
	else
	{
		unsigned long returnPos = 0;
		const double curPos = (double)m_music->getPositionMS();

		// not reinventing the wheel, the interpolation magic numbers here are (c) peppy

		const double realTime = engine->getTimeReal();
		const double interpolationDelta = (realTime - m_fLastRealTimeForInterpolationDelta) * 1000.0 * m_osu->getSpeedMultiplier();
		const double interpolationDeltaLimit = ((realTime - m_fLastAudioTimeAccurateSet)*1000.0 < 1500 || m_osu->getSpeedMultiplier() < 1.0f ? 11 : 33);

		if (m_music->isPlaying() && !m_bWasSeekFrame)
		{
			double newInterpolatedPos = m_fInterpolatedMusicPos + interpolationDelta;
			double delta = newInterpolatedPos - curPos;

			// approach and recalculate delta
			newInterpolatedPos -= delta / 8.0;
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
		}
		else // no interpolation
		{
			m_bWasSeekFrame = false;

			returnPos = curPos;
			m_fInterpolatedMusicPos = (unsigned long)returnPos;
			m_fLastAudioTimeAccurateSet = realTime;
		}

		m_fLastRealTimeForInterpolationDelta = realTime; // this is more accurate than engine->getFrameTime() for the delta calculation, since it correctly handles all possible delays inbetween

		//debugLog("returning %lu \n", returnPos);
		//debugLog("delta = %lu\n", (long)returnPos - m_iCurMusicPos);

		return returnPos;
	}
}
