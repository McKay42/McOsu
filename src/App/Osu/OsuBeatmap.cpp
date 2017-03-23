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
#include "OsuGameRules.h"
#include "OsuNotificationOverlay.h"
#include "OsuBeatmapDifficulty.h"

#include "OsuHitObject.h"
#include "OsuCircle.h"
#include "OsuSlider.h"
#include "OsuSpinner.h"

ConVar *OsuBeatmap::m_osu_volume_music_ref = NULL;
ConVar *OsuBeatmap::m_osu_speed_override_ref = NULL;
ConVar *OsuBeatmap::m_osu_pitch_override_ref = NULL;

ConVar osu_vr_draw_desktop_playfield("osu_vr_draw_desktop_playfield", true);

ConVar osu_draw_reverse_order("osu_draw_reverse_order", false);

ConVar osu_draw_followpoints("osu_draw_followpoints", true);
ConVar osu_draw_playfield_border("osu_draw_playfield_border", true);
ConVar osu_draw_hitobjects("osu_draw_hitobjects", true);

ConVar osu_global_offset("osu_global_offset", 0.0f);
ConVar osu_interpolate_music_pos("osu_interpolate_music_pos", true, "Interpolate song position with engine time if the audio library reports the same position more than once");
ConVar osu_combobreak_sound_combo("osu_combobreak_sound_combo", 20, "Only play the combobreak sound if the combo is higher than this");

ConVar osu_ar_override("osu_ar_override", -1.0f);
ConVar osu_cs_override("osu_cs_override", -1.0f);
ConVar osu_hp_override("osu_hp_override", -1.0f);
ConVar osu_od_override("osu_od_override", -1.0f);

ConVar osu_auto_snapping_strength("osu_auto_snapping_strength", 1.0f, "How many iterations of quadratic interpolation to use, more = snappier, 0 = linear");
ConVar osu_auto_cursordance("osu_auto_cursordance", false);
ConVar osu_autopilot_snapping_strength("osu_autopilot_snapping_strength", 2.0f, "How many iterations of quadratic interpolation to use, more = snappier, 0 = linear");
ConVar osu_autopilot_lenience("osu_autopilot_lenience", 0.75f);

ConVar osu_followpoints_approachtime("osu_followpoints_approachtime", 800.0f);
ConVar osu_followpoints_prevfadetime("osu_followpoints_prevfadetime", 200.0f);
ConVar osu_followpoints_scale_multiplier("osu_followpoints_scale_multiplier", 1.0f);

ConVar osu_number_scale_multiplier("osu_number_scale_multiplier", 1.0f);

ConVar osu_background_dim("osu_background_dim", 0.9f);
ConVar osu_background_fade_during_breaks("osu_background_fade_during_breaks", true);
ConVar osu_background_fade_min_duration("osu_background_fade_min_duration", 1.4f, "Only fade if the break is longer than this (in seconds)");
ConVar osu_background_fadein_duration("osu_background_fadein_duration", 0.85f);
ConVar osu_background_fadeout_duration("osu_background_fadeout_duration", 0.25f);
ConVar osu_background_brightness("osu_background_brightness", 0.0f);
ConVar osu_hiterrorbar_misaims("osu_hiterrorbar_misaims", true);
ConVar osu_debug_hiterrorbar_misaims("osu_debug_hiterrorbar_misaims", false);

ConVar osu_playfield_mirror_horizontal("osu_playfield_mirror_horizontal", false);
ConVar osu_playfield_mirror_vertical("osu_playfield_mirror_vertical", false);
ConVar osu_playfield_rotation("osu_playfield_rotation", 0.0f);
ConVar osu_playfield_stretch_x("osu_playfield_stretch_x", 0.0f);
ConVar osu_playfield_stretch_y("osu_playfield_stretch_y", 0.0f);

ConVar osu_mod_wobble("osu_mod_wobble", false);
ConVar osu_mod_wobble2("osu_mod_wobble2", false);
ConVar osu_mod_wobble_strength("osu_mod_wobble_strength", 25.0f);
ConVar osu_mod_wobble_frequency("osu_mod_wobble_frequency", 1.0f);
ConVar osu_mod_wobble_rotation_speed("osu_mod_wobble_rotation_speed", 1.0f);
ConVar osu_mod_timewarp("osu_mod_timewarp", false);
ConVar osu_mod_timewarp_multiplier("osu_mod_timewarp_multiplier", 1.5f);
ConVar osu_mod_minimize("osu_mod_minimize", false);
ConVar osu_mod_minimize_multiplier("osu_mod_minimize_multiplier", 0.5f);
ConVar osu_mod_fps("osu_mod_fps", false);
ConVar osu_mod_jigsaw1("osu_mod_jigsaw1", false);
ConVar osu_mod_jigsaw2("osu_mod_jigsaw2", false);
ConVar osu_mod_jigsaw_followcircle_radius_factor("osu_mod_jigsaw_followcircle_radius_factor", 0.0f);
ConVar osu_mod_artimewarp("osu_mod_artimewarp", false);
ConVar osu_mod_artimewarp_multiplier("osu_mod_artimewarp_multiplier", 0.5f);
ConVar osu_mod_arwobble("osu_mod_arwobble", false);
ConVar osu_mod_arwobble_strength("osu_mod_arwobble_strength", 1.0f);
ConVar osu_mod_arwobble_interval("osu_mod_arwobble_interval", 7.0f);
ConVar osu_mod_shirone("osu_mod_shirone", false);
ConVar osu_mod_shirone_combo("osu_mod_shirone_combo", 20.0f);

ConVar osu_early_note_time("osu_early_note_time", 1000.0f, "Timeframe in ms at the beginning of a beatmap which triggers a starting delay for easier reading");
ConVar osu_skip_time("osu_skip_time", 5000.0f, "Timeframe in ms within a beatmap which allows skipping if it doesn't contain any hitobjects");
ConVar osu_stacking("osu_stacking", true, "Whether to use stacking calculations or not");
ConVar osu_note_blocking("osu_note_blocking", true, "Whether to use not blocking or not");

ConVar osu_debug_draw_timingpoints("osu_debug_draw_timingpoints", false);

OsuBeatmap::OsuBeatmap(Osu *osu, UString filepath)
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
	m_sFilePath = filepath;
	m_bIsPlaying = false;
	m_bIsPaused = false;
	m_bIsWaiting = false;
	m_bIsRestartScheduled = false;
	m_bIsRestartScheduledQuick = false;
	m_iContinueMusicPos = 0;

	m_bIsSpinnerActive = false;
	m_bIsInSkippableSection = false;
	m_bShouldFlashWarningArrows = false;
	m_bContinueScheduled = false;
	m_iSelectedDifficulty = -1;
	m_selectedDifficulty = NULL;
	m_fWaitTime = 0.0f;
	m_fBeatLength = 0.0f;
	m_fScaleFactor = 1.0f;
	m_fXMultiplier = 1.0f;
	m_fNumberScale = 1.0f;
	m_fHitcircleOverlapScale = 1.0f;
	m_fRawHitcircleDiameter = 0.0f;
	m_fHitcircleDiameter = 0.0f;
	m_fSliderFollowCircleScale = 0.0f;
	m_fSliderFollowCircleDiameter = 0.0f;
	m_fRawSliderFollowCircleDiameter = 0.0f;
	m_music = NULL;

	m_iCurMusicPos = 0;
	m_iPrevCurMusicPos = 0;
	m_bWasSeekFrame = false;
	m_fInterpolatedMusicPos = 0.0;
	m_fLastAudioTimeAccurateSet = 0.0;
	m_fLastRealTimeForInterpolationDelta = 0.0;

	m_fHealth = 1.0f;
	m_fBreakBackgroundFade = 0.0f;
	m_bInBreak = false;
	m_iNextHitObjectTime = 0;
	m_iPreviousHitObjectTime = 0;
	m_iPreviousFollowPointObjectIndex = -1;
	m_fPlayfieldRotation = 0.0f;
	m_iAutoCursorDanceIndex = 0;

	m_bClick1Held = false;
	m_bClick2Held = false;

	m_iNumMisses = 0;
	m_iNPS = 0;
	m_iND = 0;

	m_bIsPreLoading = true;
	m_iPreLoadingIndex = 0;
	m_bWasHREnabled = false;
	m_fPrevHitCircleDiameter = 0.0f;
	m_bWasHorizontalMirrorEnabled = false;
	m_bWasVerticalMirrorEnabled = false;
	m_bWasEZEnabled = false;
	m_bIsVRDraw = false;
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
	if (!m_bIsPlaying && !m_bIsPaused && !m_bContinueScheduled && !m_bIsWaiting)
		return;
	if (m_selectedDifficulty == NULL || m_music == NULL) // sanity check
		return;

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

	// draw loading circle
	if (!m_music->isAsyncReady() || m_bIsPreLoading)
		m_osu->getHUD()->drawLoadingSmall(g);

	// only start drawing the rest of the playfield if the music has loaded
	if (m_bIsWaiting && (!m_music->isAsyncReady() || m_bIsPreLoading))
		return;

	// draw first person crosshair
	if (osu_mod_fps.getBool())
	{
		const int length = 15;
		Vector2 center = osuCoords2Pixels(Vector2(OsuGameRules::OSU_COORD_WIDTH/2, OsuGameRules::OSU_COORD_HEIGHT/2));
		g->setColor(0xff777777);
		g->drawLine(center.x, (int)(center.y - length), center.x, (int)(center.y + length + 1));
		g->drawLine((int)(center.x - length), center.y, (int)(center.x + length + 1), center.y);
	}

	// draw playfield border
	if (osu_draw_playfield_border.getBool() && !osu_mod_fps.getBool())
		m_osu->getHUD()->drawPlayfieldBorder(g, m_vPlayfieldCenter, m_vPlayfieldSize, m_fHitcircleDiameter);

	// allow players to not draw all hitobjects twice if in VR
	if (m_osu->isInVRMode() && !osu_vr_draw_desktop_playfield.getBool())
		return;

	// draw followpoints
	if (osu_draw_followpoints.getBool())
		drawFollowPoints(g);

	// draw all hitobjects in reverse
	if (osu_draw_hitobjects.getBool())
	{
		if (!osu_draw_reverse_order.getBool())
		{
			for (int i=m_hitobjectsSortedByEndTime.size()-1; i>=0; i--)
			{
				m_hitobjectsSortedByEndTime[i]->draw(g);
			}
		}
		else
		{
			for (int i=0; i<m_hitobjectsSortedByEndTime.size(); i++)
			{
				m_hitobjectsSortedByEndTime[i]->draw(g);
			}
		}
		for (int i=m_hitobjectsSortedByEndTime.size()-1; i>=0; i--)
		{
			m_hitobjectsSortedByEndTime[i]->draw2(g);
		}
	}

	// debug stuff
	if (osu_debug_hiterrorbar_misaims.getBool())
	{
		for (int i=0; i<m_misaimObjects.size(); i++)
		{
			g->setColor(0xbb00ff00);
			Vector2 pos = osuCoords2Pixels(m_misaimObjects[i]->getRawPosAt(0));
			g->fillRect(pos.x - 50, pos.y - 50, 100, 100);
		}
	}
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

void OsuBeatmap::drawVR(Graphics *g, Matrix4 &mvp, OsuVR *vr)
{
	if (!m_bIsPlaying && !m_bIsPaused && !m_bContinueScheduled && !m_bIsWaiting)
		return;
	if (m_selectedDifficulty == NULL || m_music == NULL) // sanity check
		return;

	m_bIsVRDraw = true; // this flag is used by getHitcircleDiameter() and osuCoords2Pixels(), for easier backwards compatibility
	{
		updateHitobjectMetrics(); // needed for raw hitcircleDiameter

		// draw playfield border
		if (osu_draw_playfield_border.getBool())
		{
			vr->getShaderUntexturedLegacyGeneric()->enable();
			{
				vr->getShaderUntexturedLegacyGeneric()->setUniformMatrix4fv("matrix", mvp);
				g->setColor(0xffffffff);
				m_osu->getHUD()->drawPlayfieldBorder(g, Vector2(0,0), Vector2(OsuGameRules::OSU_COORD_WIDTH, OsuGameRules::OSU_COORD_HEIGHT), getHitcircleDiameter());
			}
			vr->getShaderUntexturedLegacyGeneric()->disable();
		}

		// TODO: draw beatmap background image skybox?

		// only start drawing the rest of the playfield if the music has loaded
		if (!(m_bIsWaiting && (!m_music->isAsyncReady() || m_bIsPreLoading)))
		{
			// draw all hitobjects in reverse
			if (osu_draw_hitobjects.getBool())
			{
				g->setDepthBuffer(false);
				vr->getShaderTexturedLegacyGeneric()->enable();
				{
					if (!osu_draw_reverse_order.getBool())
					{
						for (int i=m_hitobjectsSortedByEndTime.size()-1; i>=0; i--)
						{
							m_hitobjectsSortedByEndTime[i]->drawVR(g, mvp, vr);
						}
					}
					else
					{
						for (int i=0; i<m_hitobjectsSortedByEndTime.size(); i++)
						{
							m_hitobjectsSortedByEndTime[i]->drawVR(g, mvp, vr);
						}
					}
				}
				vr->getShaderTexturedLegacyGeneric()->disable();
				g->setDepthBuffer(true);
			}
		}
	}
	m_bIsVRDraw = false;
}

void OsuBeatmap::drawFollowPoints(Graphics *g)
{
	OsuSkin *skin = m_osu->getSkin();

	const long curPos = m_iCurMusicPos + (long)osu_global_offset.getInt() - m_selectedDifficulty->localoffset - m_selectedDifficulty->onlineOffset;
	const long approachTime = std::min((long)OsuGameRules::getApproachTime(this), (long)osu_followpoints_approachtime.getFloat());

	// the followpoints are scaled by one eighth of the hitcirclediameter (not the raw diameter, but the scaled diameter)
	const float followPointImageScale = ((m_fHitcircleDiameter/8.0f) / (16.0f * (skin->isFollowPoint2x() ? 2.0f : 1.0f))) * osu_followpoints_scale_multiplier.getFloat();

	// include previous object in followpoints
	int lastObjectIndex = -1;

	for (int index=m_iPreviousFollowPointObjectIndex; index<m_hitobjects.size(); index++)
	{
		lastObjectIndex = index-1;

		// ignore future spinners
		OsuSpinner *spinnerPointer = dynamic_cast<OsuSpinner*>(m_hitobjects[index]);
		if (spinnerPointer != NULL) // if this is a spinner
		{
			lastObjectIndex = -1;
			continue;
		}

		// NOTE: "m_hitobjects[index]->getComboNumber() != 1" breaks (not literally) on new combos
		if (lastObjectIndex >= 0 && m_hitobjects[index]->getComboNumber() != 1)
		{
			// ignore previous spinners
			spinnerPointer = dynamic_cast<OsuSpinner*>(m_hitobjects[lastObjectIndex]);
			if (spinnerPointer != NULL) // if this is a spinner
			{
				lastObjectIndex = -1;
				continue;
			}

			// get time & pos of the last and current object
			const long lastObjectEndTime = m_hitobjects[lastObjectIndex]->getTime() + m_hitobjects[lastObjectIndex]->getDuration() + 1;
			const long objectStartTime = m_hitobjects[index]->getTime();
			const long timeDiff = objectStartTime - lastObjectEndTime;

			const Vector2 startPoint = osuCoords2Pixels(m_hitobjects[lastObjectIndex]->getRawPosAt(lastObjectEndTime));
			const Vector2 endPoint = osuCoords2Pixels(m_hitobjects[index]->getRawPosAt(objectStartTime));

			const float xDiff = endPoint.x - startPoint.x;
			const float yDiff = endPoint.y - startPoint.y;
			const Vector2 diff = endPoint - startPoint;
			const float dist = std::round(diff.length() * 100.0f) / 100.0f; // rounded to avoid flicker with playfield rotations

			// draw all points between the two objects
			const int followPointSeparation = Osu::getUIScale(m_osu, 32);
			for (int j=(int)(followPointSeparation * 1.5f); j<dist-followPointSeparation; j+=followPointSeparation)
			{
				const float animRatio = ((float)j / dist);

				const Vector2 animPosStart = startPoint + (animRatio - 0.1f) * diff;
				const Vector2 finalPos = startPoint + animRatio * diff;

				const long fadeInTime = (long)(lastObjectEndTime + animRatio * timeDiff) - approachTime;
				const long fadeOutTime = (long)(lastObjectEndTime + animRatio * timeDiff);

				// draw
				float alpha = 1.0f;
				float followAnimPercent = clamp<float>((float)(curPos - fadeInTime)/(float)osu_followpoints_prevfadetime.getFloat(), 0.0f, 1.0f);
				followAnimPercent = -followAnimPercent*(followAnimPercent-2); // quad out

				const float scale = 1.5f - 0.5f*followAnimPercent;
				const Vector2 followPos = animPosStart + (finalPos - animPosStart)*followAnimPercent;

				// calculate trail alpha
				if (curPos >= fadeInTime && curPos < fadeOutTime)
				{
					// future trail
					const float delta = curPos - fadeInTime;
					alpha = (float)delta / (float)approachTime;
				}
				else if (curPos >= fadeOutTime && curPos < fadeOutTime+(long)osu_followpoints_prevfadetime.getFloat())
				{
					// previous trail
					const long delta = curPos - fadeOutTime;
					alpha = 1.0f - (float)delta / (float)(osu_followpoints_prevfadetime.getFloat());
				}
				else
					alpha = 0.0f;

				// draw it
				g->setColor(0xffffffff);
				g->setAlpha(alpha);
				g->pushTransform();
					g->rotate(rad2deg(atan2(yDiff, xDiff)));
					g->scale(followPointImageScale*scale, followPointImageScale*scale);
					g->translate(followPos.x, followPos.y);
					g->drawImage(skin->getFollowPoint());
				g->popTransform();
			}
		}

		// store current index as previous index
		lastObjectIndex = index;

		// iterate up until the "nextest" element
		if (m_hitobjects[index]->getTime() >= curPos + approachTime)
			break;
	}
}

void OsuBeatmap::update()
{
	if (!m_bIsPlaying && !m_bIsPaused && !m_bContinueScheduled)
		return;

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

	// live update hit object and playfield metrics
	updateHitobjectMetrics();
	updatePlayfieldMetrics();

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
		if (!m_music->isAsyncReady() || m_bIsPreLoading)
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

				onUpdateMods(false);

				// if we are quick restarting, jump just before the first hitobject (even if there is a long waiting period at the beginning with nothing etc.)
				if (m_bIsRestartScheduledQuick)
					m_music->setPositionMS(m_hitobjects[0]->getTime() - 1000);
				m_bIsRestartScheduledQuick = false;
			}
			else
				m_iCurMusicPos = (engine->getTimeReal() - m_fWaitTime)*1000.0f*m_osu->getSpeedMultiplier();
		}
	}

	// handle music end
	if ((m_music->isFinished() || (m_hitobjects.size() > 0 && m_hitobjects[m_hitobjects.size()-1]->getTime() + m_hitobjects[m_hitobjects.size()-1]->getDuration() + 1000 < m_iCurMusicPos)) && !m_bIsWaiting)
	{
		stop(false);
		return;
	}

	// handle preloading (only for distributed slider vertexbuffer generation atm)
	if (m_bIsPreLoading)
	{
		if (Osu::debug->getBool() && m_iPreLoadingIndex == 0)
			debugLog("OsuBeatmap: Preloading slider vertexbuffers ...\n");

		double startTime = engine->getTimeReal();
		double delta = 0.0;
		while (delta < 0.010 && m_bIsPreLoading) // hardcoded VR deadline of 10 ms (11 but sanity), will temporarily bring us down to 45 fps on average (better than freezing). works fine for desktop gameplay too
		{
			if (m_iPreLoadingIndex >= m_hitobjects.size())
			{
				m_bIsPreLoading = false;
				debugLog("OsuBeatmap: Preloading done.\n");
				break;
			}
			else
			{
				OsuSlider *sliderPointer = dynamic_cast<OsuSlider*>(m_hitobjects[m_iPreLoadingIndex]);
				if (sliderPointer != NULL)
					sliderPointer->rebuildVertexBuffer();
			}

			m_iPreLoadingIndex++;
			delta = engine->getTimeReal()-startTime;
		}
	}

	// only continue updating hitobjects etc. if we have loaded everything
	if (!m_music->isAsyncReady() || m_bIsPreLoading)
		return;

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

	// wobble mod
	if (osu_mod_wobble.getBool())
	{
		const float speedMultiplierCompensation = 1.0f / getSpeedMultiplier();
		m_fPlayfieldRotation = (m_iCurMusicPos/1000.0f)*30.0f*speedMultiplierCompensation*osu_mod_wobble_rotation_speed.getFloat();
	}
	else
		m_fPlayfieldRotation = 0.0f;

	// for performance reasons, a lot of operations are crammed into 1 loop over all hitobjects:
	// update all hitobjects,
	// handle click events,
	// also get the time of the next/previous hitobject and their indices for later,
	// and get the current hitobject,
	// also handle miss hiterrorbar slots,
	// also calculate nps and nd,
	// also handle note blocking
	OsuHitObject *currentHitObject = NULL;
	m_iNextHitObjectTime = 0;
	m_iPreviousHitObjectTime = 0;
	m_iPreviousFollowPointObjectIndex = 0;
	m_iNPS = 0;
	m_iND = 0;
	{
		std::lock_guard<std::mutex> lk(m_clicksMutex); // we need to lock this up here, else it would be possible to insert a click just before calling m_clicks.clear(), thus missing it

		long curPos = m_iCurMusicPos + (long)osu_global_offset.getInt() - m_selectedDifficulty->localoffset - m_selectedDifficulty->onlineOffset;
		Vector2 cursorPos = getCursorPos();
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
				m_hitobjects[i]->onClickEvent(cursorPos, m_clicks);



			// used for auto later
			if (m_iNextHitObjectTime == 0)
			{
				if (m_hitobjects[i]->getTime() > m_iCurMusicPos)
					m_iNextHitObjectTime = (long)m_hitobjects[i]->getTime();
				else
				{
					currentHitObject = m_hitobjects[i];
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

	// update auto (after having updated the hitobjects)
	if (m_osu->getModAuto() || m_osu->getModAutopilot())
		updateAutoCursorPos();

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

	// spinner detection (used temporarily by OsuHUD for not drawing the hiterrorbar)
	if (currentHitObject != NULL)
	{
		OsuSpinner *spinnerPointer = dynamic_cast<OsuSpinner*>(currentHitObject);
		if (spinnerPointer != NULL)
			m_bIsSpinnerActive = true;
		else
			m_bIsSpinnerActive = false;
	}

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

	// TODO: update hp & drain

	m_iPrevCurMusicPos = m_iCurMusicPos;
}

void OsuBeatmap::skipEmptySection()
{
	if (!m_bIsInSkippableSection)
		return;

	m_music->setPositionMS(m_iNextHitObjectTime - 2500);
	m_bIsInSkippableSection = false;

	engine->getSound()->play(m_osu->getSkin()->getMenuHit());
}

void OsuBeatmap::onUpdateMods(bool rebuildSliderVertexBuffers)
{
	debugLog("OsuBeatmap::onUpdateMods()\n");

	// just for good measure
	updatePlayfieldMetrics();
	updateHitobjectMetrics();

	if (m_music != NULL)
	{
		m_music->setSpeed(m_osu->getSpeedMultiplier());
		m_music->setPitch(m_osu->getPitchMultiplier());
	}

	if (m_osu->getModHR() != m_bWasHREnabled || osu_playfield_mirror_horizontal.getBool() != m_bWasHorizontalMirrorEnabled || osu_playfield_mirror_vertical.getBool() != m_bWasVerticalMirrorEnabled)
	{
		m_bWasHREnabled = m_osu->getModHR();
		m_bWasHorizontalMirrorEnabled = osu_playfield_mirror_horizontal.getBool();
		m_bWasVerticalMirrorEnabled = osu_playfield_mirror_vertical.getBool();

		calculateStacks();
		if (rebuildSliderVertexBuffers)
			updateSliderVertexBuffers();
	}

	if (m_osu->getModEZ() != m_bWasEZEnabled)
	{
		m_bWasEZEnabled = m_osu->getModEZ();
		if (rebuildSliderVertexBuffers)
			updateSliderVertexBuffers();
	}

	if (getHitcircleDiameter() != m_fPrevHitCircleDiameter && m_hitobjects.size() > 0)
	{
		m_fPrevHitCircleDiameter = getHitcircleDiameter();
		if (rebuildSliderVertexBuffers)
			updateSliderVertexBuffers();
	}
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
		m_selectedDifficulty->loadBackgroundImage();

		// need to recheck/reload the music here since this difficulty might be using a different sound file
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

	updatePlayfieldMetrics();
	updateHitobjectMetrics();

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

	calculateStacks();
	updatePlayfieldMetrics();

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

	// HACKHACK: this should work
	m_bIsPlaying = true;
	m_bIsPreLoading = true;
	m_iPreLoadingIndex = 0;
	return true; // was m_bIsPlaying
}

void OsuBeatmap::restart(bool quick)
{
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

	updatePlayfieldMetrics();

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

	onUpdateMods();

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
			m_vContinueCursorPoint = engine->getMouse()->getPos();

			if (osu_mod_fps.getBool())
				m_vContinueCursorPoint = OsuGameRules::getPlayfieldCenter(m_osu);
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
	if (m_selectedDifficulty == NULL)
		return;

	m_bIsPlaying = false;
	m_bIsPaused = false;
	m_bContinueScheduled = false;
	unloadHitObjects();
	resetHitObjects();

	m_osu->onPlayEnd(quit);
}

void OsuBeatmap::fail()
{
	// TODO: slow down music etc.
	stop();
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
	if (m_selectedDifficulty == NULL || (!m_bIsPlaying && !m_bIsPaused) || m_music == NULL)
		return;

	m_bWasSeekFrame = true;

	m_music->setPosition(percent);

	resetHitObjects(m_music->getPositionMS());
	resetScore();
}

void OsuBeatmap::seekPercentPlayable(double percent)
{
	if (m_selectedDifficulty == NULL || (!m_bIsPlaying && !m_bIsPaused) || m_music == NULL)
		return;

	m_bWasSeekFrame = true;

	double actualPlayPercent = percent;
	if (m_hitobjects.size() > 0)
		actualPlayPercent = (((double)m_hitobjects[m_hitobjects.size()-1]->getTime() + (double)m_hitobjects[m_hitobjects.size()-1]->getDuration()) * percent) / (double)m_music->getLengthMS();

	seekPercent(actualPlayPercent);
}

void OsuBeatmap::seekMS(unsigned long ms)
{
	if (m_selectedDifficulty == NULL || (!m_bIsPlaying && !m_bIsPaused) || m_music == NULL)
		return;

	m_bWasSeekFrame = true;

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
		return m_difficulties[0]->lengthMS;
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

float OsuBeatmap::getHitcircleDiameter()
{
	// in VR, there is no resolution to which the playfield would have to get scaled up to (since the entire playfield is scaled at once as the player sees fit)
	// therefore just return the raw hitcircle diameter (osu!pixels)
	return m_bIsVRDraw ? m_fRawHitcircleDiameter : m_fHitcircleDiameter;
}

bool OsuBeatmap::isClickHeld()
{
	 return m_bClick1Held || m_bClick2Held || (m_osu->isInVRMode() && !m_osu->getVR()->isVirtualCursorOnScreen());
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

	playMissSound();

	m_osu->getScore()->addSliderBreak();
}

void OsuBeatmap::addScorePoints(int points)
{
	m_osu->getScore()->addPoints(points);
}

void OsuBeatmap::playMissSound()
{
	if (m_osu->getScore()->getCombo() > osu_combobreak_sound_combo.getInt())
		engine->getSound()->play(getSkin()->getCombobreak());
}

Vector2 OsuBeatmap::osuCoords2Pixels(Vector2 coords)
{
	if (m_bIsVRDraw)
		return osuCoords2VRPixels(coords);

	if (m_osu->getModHR() || osu_playfield_mirror_horizontal.getBool())
		coords.y = OsuGameRules::OSU_COORD_HEIGHT - coords.y;
	if (osu_playfield_mirror_vertical.getBool())
		coords.x = OsuGameRules::OSU_COORD_WIDTH - coords.x;

	// wobble
	if (osu_mod_wobble.getBool())
	{
		const float speedMultiplierCompensation = 1.0f / getSpeedMultiplier();
		coords.x += std::sin((m_iCurMusicPos/1000.0f)*5*speedMultiplierCompensation*osu_mod_wobble_frequency.getFloat())*osu_mod_wobble_strength.getFloat();
		coords.y += std::sin((m_iCurMusicPos/1000.0f)*4*speedMultiplierCompensation*osu_mod_wobble_frequency.getFloat())*osu_mod_wobble_strength.getFloat();
	}

	// wobble2
	if (osu_mod_wobble2.getBool())
	{
		const float speedMultiplierCompensation = 1.0f / getSpeedMultiplier();
		Vector2 centerDelta = coords - Vector2(OsuGameRules::OSU_COORD_WIDTH, OsuGameRules::OSU_COORD_HEIGHT)/2;
		coords.x += centerDelta.x*0.25f*std::sin((m_iCurMusicPos/1000.0f)*5*speedMultiplierCompensation*osu_mod_wobble_frequency.getFloat())*osu_mod_wobble_strength.getFloat();
		coords.y += centerDelta.y*0.25f*std::sin((m_iCurMusicPos/1000.0f)*3*speedMultiplierCompensation*osu_mod_wobble_frequency.getFloat())*osu_mod_wobble_strength.getFloat();
	}

	// rotation
	if (m_fPlayfieldRotation + osu_playfield_rotation.getFloat() != 0.0f)
	{
		coords.x -= OsuGameRules::OSU_COORD_WIDTH/2;
		coords.y -= OsuGameRules::OSU_COORD_HEIGHT/2;

		Vector3 coords3 = Vector3(coords.x, coords.y, 0);
		Matrix4 rot;
		rot.rotateZ(m_fPlayfieldRotation + osu_playfield_rotation.getFloat()); // (m_iCurMusicPos/1000.0f)*30

		coords3 = coords3 * rot;
		coords3.x += OsuGameRules::OSU_COORD_WIDTH/2;
		coords3.y += OsuGameRules::OSU_COORD_HEIGHT/2;

		coords.x = coords3.x;
		coords.y = coords3.y;
	}

	// if wobble, clamp coordinates
	if (osu_mod_wobble.getBool() || osu_mod_wobble2.getBool())
	{
		coords.x = clamp<float>(coords.x, 0.0f, OsuGameRules::OSU_COORD_WIDTH);
		coords.y = clamp<float>(coords.y, 0.0f, OsuGameRules::OSU_COORD_HEIGHT);
	}

	// scale and offset
	///coords *= m_fScaleFactor;
	///coords += m_vPlayfieldOffset; // the offset is already scaled, just add it

	const float targetScreenWidthFull = m_osu->getScreenWidth() - m_fHitcircleDiameter;
	const float targetScreenHeightFull = m_osu->getScreenHeight() - m_fHitcircleDiameter;

	// scale
	coords.x *= (1.0f - osu_playfield_stretch_x.getFloat())*m_fScaleFactor + osu_playfield_stretch_x.getFloat()*(targetScreenWidthFull / (float)OsuGameRules::OSU_COORD_WIDTH);
	coords.y *= (1.0f - osu_playfield_stretch_y.getFloat())*m_fScaleFactor + osu_playfield_stretch_y.getFloat()*(targetScreenHeightFull / (float)OsuGameRules::OSU_COORD_HEIGHT);

	// offset
	coords.x += (1.0f - osu_playfield_stretch_x.getFloat())*m_vPlayfieldOffset.x + osu_playfield_stretch_x.getFloat()*(m_fHitcircleDiameter/2.0f);
	coords.y += (1.0f - osu_playfield_stretch_y.getFloat())*m_vPlayfieldOffset.y + osu_playfield_stretch_y.getFloat()*(m_fHitcircleDiameter/2.0f);

	// first person mod, centered cursor
	if (osu_mod_fps.getBool())
	{
		// this is the worst hack possible (engine->isDrawing()), but it works
		// the problem is that this same function is called while draw()ing and update()ing
		if ((engine->isDrawing() && (m_osu->getModAuto() || m_osu->getModAutopilot())) || !(m_osu->getModAuto() || m_osu->getModAutopilot()))
			coords += getFirstPersonDelta();
	}

	return coords;
}

Vector2 OsuBeatmap::osuCoords2RawPixels(Vector2 coords)
{
	// scale and offset
	coords *= m_fScaleFactor;
	coords += m_vPlayfieldOffset; // the offset is already scaled, just add it

	return coords;
}

Vector2 OsuBeatmap::osuCoords2VRPixels(Vector2 coords)
{
	if (m_osu->getModHR() || osu_playfield_mirror_horizontal.getBool())
		coords.y = OsuGameRules::OSU_COORD_HEIGHT - coords.y;
	if (osu_playfield_mirror_vertical.getBool())
		coords.x = OsuGameRules::OSU_COORD_WIDTH - coords.x;

	// wobble
	if (osu_mod_wobble.getBool())
	{
		const float speedMultiplierCompensation = 1.0f / getSpeedMultiplier();
		coords.x += std::sin((m_iCurMusicPos/1000.0f)*5*speedMultiplierCompensation*osu_mod_wobble_frequency.getFloat())*osu_mod_wobble_strength.getFloat();
		coords.y += std::sin((m_iCurMusicPos/1000.0f)*4*speedMultiplierCompensation*osu_mod_wobble_frequency.getFloat())*osu_mod_wobble_strength.getFloat();
	}

	// wobble2
	if (osu_mod_wobble2.getBool())
	{
		const float speedMultiplierCompensation = 1.0f / getSpeedMultiplier();
		Vector2 centerDelta = coords - Vector2(OsuGameRules::OSU_COORD_WIDTH, OsuGameRules::OSU_COORD_HEIGHT)/2;
		coords.x += centerDelta.x*0.25f*std::sin((m_iCurMusicPos/1000.0f)*5*speedMultiplierCompensation*osu_mod_wobble_frequency.getFloat())*osu_mod_wobble_strength.getFloat();
		coords.y += centerDelta.y*0.25f*std::sin((m_iCurMusicPos/1000.0f)*3*speedMultiplierCompensation*osu_mod_wobble_frequency.getFloat())*osu_mod_wobble_strength.getFloat();
	}

	// rotation
	if (m_fPlayfieldRotation + osu_playfield_rotation.getFloat() != 0.0f)
	{
		coords.x -= OsuGameRules::OSU_COORD_WIDTH/2;
		coords.y -= OsuGameRules::OSU_COORD_HEIGHT/2;

		Vector3 coords3 = Vector3(coords.x, coords.y, 0);
		Matrix4 rot;
		rot.rotateZ(m_fPlayfieldRotation + osu_playfield_rotation.getFloat());

		coords3 = coords3 * rot;
		coords3.x += OsuGameRules::OSU_COORD_WIDTH/2;
		coords3.y += OsuGameRules::OSU_COORD_HEIGHT/2;

		coords.x = coords3.x;
		coords.y = coords3.y;
	}

	// if wobble, clamp coordinates
	if (osu_mod_wobble.getBool() || osu_mod_wobble2.getBool())
	{
		coords.x = clamp<float>(coords.x, 0.0f, OsuGameRules::OSU_COORD_WIDTH);
		coords.y = clamp<float>(coords.y, 0.0f, OsuGameRules::OSU_COORD_HEIGHT);
	}

	// VR center
	coords.x -= OsuGameRules::OSU_COORD_WIDTH/2;
	coords.y -= OsuGameRules::OSU_COORD_HEIGHT/2;

	// VR scale
	coords.x *= (1.0f - osu_playfield_stretch_x.getFloat()) + osu_playfield_stretch_x.getFloat();
	coords.y *= (1.0f - osu_playfield_stretch_y.getFloat()) + osu_playfield_stretch_y.getFloat();

	return coords;
}

Vector2 OsuBeatmap::osuCoords2LegacyPixels(Vector2 coords)
{
	if (m_osu->getModHR() || osu_playfield_mirror_horizontal.getBool())
		coords.y = OsuGameRules::OSU_COORD_HEIGHT - coords.y;
	if (osu_playfield_mirror_vertical.getBool())
		coords.x = OsuGameRules::OSU_COORD_WIDTH - coords.x;

	// VR center
	coords.x -= OsuGameRules::OSU_COORD_WIDTH/2;
	coords.y -= OsuGameRules::OSU_COORD_HEIGHT/2;

	// VR scale
	coords.x *= (1.0f - osu_playfield_stretch_x.getFloat()) + osu_playfield_stretch_x.getFloat();
	coords.y *= (1.0f - osu_playfield_stretch_y.getFloat()) + osu_playfield_stretch_y.getFloat();

	return coords;
}

Vector2 OsuBeatmap::getCursorPos()
{
	if (osu_mod_fps.getBool() && !m_bIsPaused)
	{
		if (m_osu->getModAuto() || m_osu->getModAutopilot())
			return m_vAutoCursorPos;
		else
			return m_vPlayfieldCenter;
	}
	else if (m_osu->getModAuto() || m_osu->getModAutopilot())
		return m_vAutoCursorPos;
	else
	{
		Vector2 pos = engine->getMouse()->getPos();
		if (osu_mod_shirone.getBool() && m_osu->getScore()->getCombo() > 0) // <3
			return pos + Vector2(std::sin((m_iCurMusicPos/20.0f)*1.15f)*((float)m_osu->getScore()->getCombo()/osu_mod_shirone_combo.getFloat()), std::cos((m_iCurMusicPos/20.0f)*1.3f)*((float)m_osu->getScore()->getCombo()/osu_mod_shirone_combo.getFloat()));
		else
			return pos;
	}
}

Vector2 OsuBeatmap::getFirstPersonDelta()
{
	return m_vPlayfieldCenter - (m_osu->getModAuto() || m_osu->getModAutopilot() ? m_vAutoCursorPos : engine->getMouse()->getPos());
}

void OsuBeatmap::handlePreviewPlay()
{
	if (m_music != NULL && (!m_music->isPlaying() || m_music->getPosition() > 0.95f) && m_selectedDifficulty != NULL)
	{
		// this is an assumption, but should be good enough for most songs
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
	m_osu->getScore()->reset();
}

void OsuBeatmap::updateAutoCursorPos()
{
	m_vAutoCursorPos = m_vPlayfieldCenter;

	if (!m_bIsPlaying && !m_bIsPaused)
		return;
	if (m_hitobjects.size() == 0)
		return;

	long prevTime = 0;
	long nextTime = m_hitobjects[0]->getTime();
	Vector2 prevPos = m_vPlayfieldCenter;
	Vector2 curPos = m_vPlayfieldCenter;
	Vector2 nextPos = m_vPlayfieldCenter;
	int nextPosIndex = 0;
	bool haveCurPos = false;

	long curMusicPos = m_iCurMusicPos + (long)osu_global_offset.getInt() - m_selectedDifficulty->localoffset - m_selectedDifficulty->onlineOffset;

	if (m_bIsWaiting)
		prevTime = -(long)osu_early_note_time.getInt();

	if (curMusicPos >= 0)
	{
		prevPos = m_hitobjects[0]->getAutoCursorPos(0);
		curPos = prevPos;
		nextPos = prevPos;
	}

	if (m_osu->getModAuto())
	{
		for (int i=0; i<m_hitobjects.size(); i++)
		{
			OsuHitObject *o = m_hitobjects[i];

			// get previous object
			if (o->getTime() <= curMusicPos)
			{
				prevTime = o->getTime() + o->getDuration();
				prevPos = o->getAutoCursorPos(curMusicPos);
				if (o->getDuration() > 0 && curMusicPos - o->getTime() <= o->getDuration())
				{
					haveCurPos = true;
					curPos = prevPos;
					break;
				}
			}

			// get next object
			if (o->getTime() > curMusicPos)
			{
				nextPosIndex = i;
				nextPos = o->getAutoCursorPos(curMusicPos);
				nextTime = o->getTime();
				break;
			}
		}
	}
	else if (m_osu->getModAutopilot())
	{
		for (int i=0; i<m_hitobjects.size(); i++)
		{
			OsuHitObject *o = m_hitobjects[i];

			// get previous object
			if (o->isFinished() || (curMusicPos > o->getTime() + o->getDuration() + (long)(OsuGameRules::getHitWindow50(this)*osu_autopilot_lenience.getFloat())))
			{
				prevTime = o->getTime() + o->getDuration() + o->getAutopilotDelta();
				prevPos = o->getAutoCursorPos(curMusicPos);
			}
			else if (!o->isFinished()) // get next object
			{
				nextPosIndex = i;
				nextPos = o->getAutoCursorPos(curMusicPos);
				nextTime = o->getTime();

				// wait for the user to click
				if (curMusicPos >= nextTime + o->getDuration())
				{
					haveCurPos = true;
					curPos = nextPos;

					//long delta = curMusicPos - (nextTime + o->getDuration());
					o->setAutopilotDelta(curMusicPos - (nextTime + o->getDuration()));
				}
				else if (o->getDuration() > 0 && curMusicPos >= nextTime) // handle objects with duration
				{
					haveCurPos = true;
					curPos = nextPos;
					o->setAutopilotDelta(0);
				}

				break;
			}
		}
	}

	if (haveCurPos) // in active hitObject
		m_vAutoCursorPos = curPos;
	else
	{
		// interpolation
		float percent = 1.0f;
		if ((nextTime == 0 && prevTime == 0) || (nextTime - prevTime) == 0)
			percent = 1.0f;
		else
			percent = (float)((long)curMusicPos - prevTime) / (float)(nextTime - prevTime);

		percent = clamp<float>(percent, 0.0f, 1.0f);

		// scaled distance (not osucoords)
		float distance = (nextPos-prevPos).length();
		if (distance > m_fHitcircleDiameter*1.05f) // snap only if not in a stream (heuristic)
		{
			int numIterations = clamp<int>(m_osu->getModAutopilot() ? osu_autopilot_snapping_strength.getInt() : osu_auto_snapping_strength.getInt(), 0, 42);
			for (int i=0; i<numIterations; i++)
			{
				percent = (-percent)*(percent-2.0f);
			}
		}
		else // in a stream
		{
			m_iAutoCursorDanceIndex = nextPosIndex;
		}

		m_vAutoCursorPos = prevPos + (nextPos - prevPos)*percent;

		if (osu_auto_cursordance.getBool())
		{
			Vector3 dir = Vector3(nextPos.x, nextPos.y, 0) - Vector3(prevPos.x, prevPos.y, 0);
			Vector3 center = dir*0.5f;
			Matrix4 worldMatrix;
			worldMatrix.translate(center);
			worldMatrix.rotate((1.0f-percent) * 180.0f * (m_iAutoCursorDanceIndex % 2 == 0 ? 1 : -1), 0, 0, 1);
			Vector3 fancyAutoCursorPos = worldMatrix*center;
			m_vAutoCursorPos = prevPos + (nextPos-prevPos)*0.5f + Vector2(fancyAutoCursorPos.x, fancyAutoCursorPos.y);
		}
	}
}

void OsuBeatmap::updatePlayfieldMetrics()
{
	m_fScaleFactor = OsuGameRules::getPlayfieldScaleFactor(m_osu);
	m_vPlayfieldSize = OsuGameRules::getPlayfieldSize(m_osu);
	m_vPlayfieldOffset = OsuGameRules::getPlayfieldOffset(m_osu);
	m_vPlayfieldCenter = OsuGameRules::getPlayfieldCenter(m_osu);
}

void OsuBeatmap::updateHitobjectMetrics()
{
	OsuSkin *skin = m_osu->getSkin();

	m_fRawHitcircleDiameter = OsuGameRules::getRawHitCircleDiameter(getCS());
	m_fXMultiplier = OsuGameRules::getHitCircleXMultiplier(m_osu);
	m_fHitcircleDiameter = OsuGameRules::getHitCircleDiameter(this);

	const float osuCoordScaleMultiplier = (getHitcircleDiameter()/m_fRawHitcircleDiameter);

	m_fNumberScale = (m_fRawHitcircleDiameter / (160.0f * (skin->isDefault12x() ? 2.0f : 1.0f))) * osuCoordScaleMultiplier * osu_number_scale_multiplier.getFloat();
	m_fHitcircleOverlapScale = (m_fRawHitcircleDiameter / (160.0f)) * osuCoordScaleMultiplier * osu_number_scale_multiplier.getFloat();

	m_fRawSliderFollowCircleDiameter = getRawHitcircleDiameter() * (m_osu->getModNM() || osu_mod_jigsaw2.getBool() ? (1.0f*(1.0f - osu_mod_jigsaw_followcircle_radius_factor.getFloat()) + osu_mod_jigsaw_followcircle_radius_factor.getFloat()*2.4f) : 2.4f);
	m_fSliderFollowCircleDiameter = getHitcircleDiameter() * (m_osu->getModNM() || osu_mod_jigsaw2.getBool() ? (1.0f*(1.0f - osu_mod_jigsaw_followcircle_radius_factor.getFloat()) + osu_mod_jigsaw_followcircle_radius_factor.getFloat()*2.4f) : 2.4f);
	m_fSliderFollowCircleScale = (m_fSliderFollowCircleDiameter / (259.0f * (skin->isSliderFollowCircle2x() ? 2.0f : 1.0f)))*0.85f; // this is a bit strange, but seems to work perfectly with 0.85
}

void OsuBeatmap::updateSliderVertexBuffers()
{
	updatePlayfieldMetrics();
	updateHitobjectMetrics();

	m_fPrevHitCircleDiameter = getHitcircleDiameter(); // to avoid useless double updates in onUpdateMods()

	debugLog("OsuBeatmap::updateSliderVertexBuffers() for %i hitobjects ...\n", m_hitobjects.size());

	for (int i=0; i<m_hitobjects.size(); i++)
	{
		OsuSlider *sliderPointer = dynamic_cast<OsuSlider*>(m_hitobjects[i]);
		if (sliderPointer != NULL)
			sliderPointer->rebuildVertexBuffer();
	}
}

void OsuBeatmap::calculateStacks()
{
	if (!osu_stacking.getBool())
		return;

	updateHitobjectMetrics(); // needed for the calculations (for m_fRawHitcircleDiameter)

	debugLog("OsuBeatmap: Calculating stacks ...\n");

	const float STACK_LENIENCE = 3.0f;
	const float STACK_OFFSET = 0.05f;

	// reset
	for (int i=0; i<m_hitobjects.size(); i++)
	{
		m_hitobjects[i]->setStack(0);
	}

	// peppy's algorithm
	// https://gist.github.com/peppy/1167470

	for (int i=m_hitobjects.size()-1; i>=0; i--)
	{
		int n = i;

		OsuHitObject *objectI = m_hitobjects[i];

		bool isSpinner = dynamic_cast<OsuSpinner*>(objectI) != NULL;

		if (objectI->getStack() != 0 || isSpinner)
			continue;

		bool isHitCircle = dynamic_cast<OsuCircle*>(objectI) != NULL;
		bool isSlider = dynamic_cast<OsuSlider*>(objectI) != NULL;

		if (isHitCircle)
		{
			while (--n >= 0)
			{
				OsuHitObject *objectN = m_hitobjects[n];

				bool isSpinnerN = dynamic_cast<OsuSpinner*>(objectN);

				if (isSpinnerN)
					continue;

				if (objectI->getTime() - (OsuGameRules::getApproachTime(this) * m_selectedDifficulty->stackLeniency) > (objectN->getTime() + objectN->getDuration()))
					break;

				Vector2 objectNEndPosition = objectN->getOriginalRawPosAt(objectN->getTime() + objectN->getDuration());
				if (objectN->getDuration() != 0 && (objectNEndPosition - objectI->getOriginalRawPosAt(objectI->getTime())).length() < STACK_LENIENCE)
				{
					int offset = objectI->getStack() - objectN->getStack() + 1;
					for (int j=n+1; j<=i; j++)
					{
						if ((objectNEndPosition - m_hitobjects[j]->getOriginalRawPosAt(m_hitobjects[j]->getTime())).length() < STACK_LENIENCE)
							m_hitobjects[j]->setStack(m_hitobjects[j]->getStack() - offset);
					}

					break;
				}

				if ((objectN->getOriginalRawPosAt(objectN->getTime()) - objectI->getOriginalRawPosAt(objectI->getTime())).length() < STACK_LENIENCE)
				{
					objectN->setStack(objectI->getStack() + 1);
					objectI = objectN;
				}
			}
		}
		else if (isSlider)
		{
			while (--n >= 0)
			{
				OsuHitObject *objectN = m_hitobjects[n];

				bool isSpinner = dynamic_cast<OsuSpinner*>(objectN) != NULL;

				if (isSpinner)
					continue;

				if (objectI->getTime() - (OsuGameRules::getApproachTime(this) * m_selectedDifficulty->stackLeniency) > objectN->getTime())
					break;

				if (((objectN->getDuration() != 0 ? objectN->getOriginalRawPosAt(objectN->getTime() + objectN->getDuration()) : objectN->getOriginalRawPosAt(objectN->getTime())) - objectI->getOriginalRawPosAt(objectI->getTime())).length() < STACK_LENIENCE)
				{
					objectN->setStack(objectI->getStack() + 1);
					objectI = objectN;
				}
			}
		}
	}

	// update hitobject positions
	float stackOffset = m_fRawHitcircleDiameter * STACK_OFFSET;
	for (int i=0; i<m_hitobjects.size(); i++)
	{
		if (m_hitobjects[i]->getStack() != 0)
			m_hitobjects[i]->updateStackPosition(stackOffset);
	}
}

unsigned long OsuBeatmap::getMusicPositionMSInterpolated()
{
	if (!osu_interpolate_music_pos.getBool())
		return m_music->getPositionMS();
	else
	{
		unsigned long returnPos = 0;
		const double curPos = (double)m_music->getPositionMS();

		// the interpolation method and magic numbers here are also (c) peppy (Dean Herbert)

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
