//================ Copyright (c) 2017, PG, All rights reserved. =================//
//
// Purpose:		osu!standard circle clicking
//
// $NoKeywords: $osustd
//===============================================================================//

#include "OsuBeatmapStandard.h"

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
#include "OsuMultiplayer.h"
#include "OsuHUD.h"
#include "OsuSkin.h"
#include "OsuSkinImage.h"
#include "OsuGameRules.h"
#include "OsuDatabase.h"
#include "OsuSongBrowser2.h"
#include "OsuNotificationOverlay.h"
#include "OsuBeatmapDifficulty.h"
#include "OsuReplay.h"

#include "OsuHitObject.h"
#include "OsuCircle.h"
#include "OsuSlider.h"
#include "OsuSpinner.h"

#include "OpenGLHeaders.h"

#include <chrono>

ConVar osu_draw_followpoints("osu_draw_followpoints", true);
ConVar osu_draw_reverse_order("osu_draw_reverse_order", false);
ConVar osu_draw_playfield_border("osu_draw_playfield_border", true);

ConVar osu_stacking("osu_stacking", true, "Whether to use stacking calculations or not");

ConVar osu_auto_snapping_strength("osu_auto_snapping_strength", 1.0f, "How many iterations of quadratic interpolation to use, more = snappier, 0 = linear");
ConVar osu_auto_cursordance("osu_auto_cursordance", false);
ConVar osu_autopilot_snapping_strength("osu_autopilot_snapping_strength", 2.0f, "How many iterations of quadratic interpolation to use, more = snappier, 0 = linear");
ConVar osu_autopilot_lenience("osu_autopilot_lenience", 0.75f);

ConVar osu_followpoints_approachtime("osu_followpoints_approachtime", 800.0f);
ConVar osu_followpoints_scale_multiplier("osu_followpoints_scale_multiplier", 1.0f);

ConVar osu_number_scale_multiplier("osu_number_scale_multiplier", 1.0f);

ConVar osu_playfield_mirror_horizontal("osu_playfield_mirror_horizontal", false);
ConVar osu_playfield_mirror_vertical("osu_playfield_mirror_vertical", false);
ConVar osu_playfield_rotation("osu_playfield_rotation", 0.0f, "rotates the entire playfield by this many degrees");
ConVar osu_playfield_stretch_x("osu_playfield_stretch_x", 0.0f, "offsets/multiplies all hitobject coordinates by it (0 = default 1x playfield size, -1 = on a line, -0.5 = 0.5x playfield size, 0.5 = 1.5x playfield size)");
ConVar osu_playfield_stretch_y("osu_playfield_stretch_y", 0.0f, "offsets/multiplies all hitobject coordinates by it (0 = default 1x playfield size, -1 = on a line, -0.5 = 0.5x playfield size, 0.5 = 1.5x playfield size)");

ConVar osu_mod_wobble("osu_mod_wobble", false);
ConVar osu_mod_wobble2("osu_mod_wobble2", false);
ConVar osu_mod_wobble_strength("osu_mod_wobble_strength", 25.0f);
ConVar osu_mod_wobble_frequency("osu_mod_wobble_frequency", 1.0f);
ConVar osu_mod_wobble_rotation_speed("osu_mod_wobble_rotation_speed", 1.0f);
ConVar osu_mod_jigsaw2("osu_mod_jigsaw2", false);
ConVar osu_mod_jigsaw_followcircle_radius_factor("osu_mod_jigsaw_followcircle_radius_factor", 0.0f);
ConVar osu_mod_shirone("osu_mod_shirone", false);
ConVar osu_mod_shirone_combo("osu_mod_shirone_combo", 20.0f);
ConVar osu_mod_mafham_render_chunksize("osu_mod_mafham_render_chunksize", 15, "render this many hitobjects per frame chunk into the scene buffer (spreads rendering across many frames to minimize lag)");

ConVar osu_debug_hiterrorbar_misaims("osu_debug_hiterrorbar_misaims", false);

ConVar osu_pp_live_timeout("osu_pp_live_timeout", 1.0f, "show message that we're still calculating stars after this many seconds, on the first start of the beatmap");



class StarCacheLoader : public Resource
{
public:
	StarCacheLoader(OsuBeatmap *beatmap) : Resource()
	{
		m_beatmap = beatmap;

		m_bDead = true; // start dead! need to revive() before use
		m_iProgress = 0;

		m_bAsyncReady = false;
		m_bReady = false;
	};

	bool isDead() {return m_bDead.load();}
	void kill() {m_bDead = true; m_iProgress = 0;}
	void revive() {m_bDead = false; m_iProgress = 0;}

	int getProgress() {return m_iProgress.load();}

protected:
	virtual void init()
	{
		m_bReady = true;
	}
	virtual void initAsync()
	{
		if (m_bDead.load())
		{
			m_bAsyncReady = true;
			return;
		}

		if (m_beatmap->getSelectedDifficulty() != NULL)
			m_beatmap->getSelectedDifficulty()->rebuildStarCacheForUpToHitObjectIndex(m_beatmap, m_bDead, m_iProgress);

		m_bAsyncReady = true;
	}
	virtual void destroy() {;}

private:
	OsuBeatmap *m_beatmap;

	std::atomic<bool> m_bDead;
	std::atomic<int> m_iProgress;
};



ConVar *OsuBeatmapStandard::m_osu_draw_statistics_pp = NULL;
ConVar *OsuBeatmapStandard::m_osu_pp_live_type = NULL;

OsuBeatmapStandard::OsuBeatmapStandard(Osu *osu) : OsuBeatmap(osu)
{
	m_bIsSpinnerActive = false;

	m_fPlayfieldRotation = 0.0f;
	m_fScaleFactor = 1.0f;

	m_fXMultiplier = 1.0f;
	m_fNumberScale = 1.0f;
	m_fHitcircleOverlapScale = 1.0f;
	m_fRawHitcircleDiameter = 0.0f;
	m_fHitcircleDiameter = 0.0f;
	m_fSliderFollowCircleDiameter = 0.0f;
	m_fRawSliderFollowCircleDiameter = 0.0f;

	m_iAutoCursorDanceIndex = 0;

	m_fAimStars = 0.0f;
	m_fSpeedStars = 0.0f;
	m_starCacheLoader = new StarCacheLoader(this);
	m_fStarCacheTime = 0.0f;

	m_bWasHREnabled = false;
	m_fPrevHitCircleDiameter = 0.0f;
	m_bWasHorizontalMirrorEnabled = false;
	m_bWasVerticalMirrorEnabled = false;
	m_bWasEZEnabled = false;
	m_bWasMafhamEnabled = false;
	m_fPrevPlayfieldRotationFromConVar = 0.0f;
	m_fPrevPlayfieldStretchX = 0.0f;
	m_fPrevPlayfieldStretchY = 0.0f;
	m_fPrevHitCircleDiameterForStarCache = 1.0f;
	m_fPrevSpeedForStarCache = 1.0f;

	m_bIsVRDraw = false;

	m_bIsPreLoading = true;
	m_iPreLoadingIndex = 0;

	m_mafhamActiveRenderTarget = NULL;
	m_mafhamFinishedRenderTarget = NULL;
	m_bMafhamRenderScheduled = true;
	m_iMafhamHitObjectRenderIndex = 0;
	m_iMafhamPrevHitObjectIndex = 0;
	m_iMafhamActiveRenderHitObjectIndex = 0;
	m_iMafhamFinishedRenderHitObjectIndex = 0;
	m_bInMafhamRenderChunk = false;

	// convar refs
	if (m_osu_draw_statistics_pp == NULL)
		m_osu_draw_statistics_pp = convar->getConVarByName("osu_draw_statistics_pp");
	if (m_osu_pp_live_type == NULL)
		m_osu_pp_live_type = convar->getConVarByName("osu_pp_live_type");
}

OsuBeatmapStandard::~OsuBeatmapStandard()
{
	m_starCacheLoader->kill();
	engine->getResourceManager()->destroyResource(m_starCacheLoader);
}

void OsuBeatmapStandard::draw(Graphics *g)
{
	OsuBeatmap::draw(g);
	if (!canDraw()) return;

	if (isLoadingStarCache() && engine->getTime() > m_fStarCacheTime)
	{
		float progressPercent = 0.0f;
		if (m_hitobjects.size() > 0)
			progressPercent = (float)m_starCacheLoader->getProgress() / (float)m_hitobjects.size();

		g->setColor(0x44ffffff);
		UString loadingMessage = UString::format("Calculating stars for live pp (%i%%) ...", (int)(progressPercent*100.0f));
		UString loadingMessage2 = "(To get rid of this delay, disable [Draw Stats: pp])";
		g->pushTransform();
		g->translate((int)(m_osu->getScreenWidth()/2 - m_osu->getSubTitleFont()->getStringWidth(loadingMessage)/2), m_osu->getScreenHeight() - m_osu->getSubTitleFont()->getHeight() - 25);
		g->drawString(m_osu->getSubTitleFont(), loadingMessage);
		g->popTransform();
		g->pushTransform();
		g->translate((int)(m_osu->getScreenWidth()/2 - m_osu->getSubTitleFont()->getStringWidth(loadingMessage2)/2), m_osu->getScreenHeight() - 15);
		g->drawString(m_osu->getSubTitleFont(), loadingMessage2);
		g->popTransform();
	}
	else if (m_osu->isInMultiplayer() && m_osu->getMultiplayer()->isWaitingForPlayers())
	{
		if (!m_bIsPreLoading && !isLoadingStarCache()) // usability
		{
			g->setColor(0x44ffffff);
			UString loadingMessage = "Waiting for players ...";
			g->pushTransform();
			{
				g->translate((int)(m_osu->getScreenWidth()/2 - m_osu->getSubTitleFont()->getStringWidth(loadingMessage)/2), m_osu->getScreenHeight() - m_osu->getSubTitleFont()->getHeight() - 15);
				g->drawString(m_osu->getSubTitleFont(), loadingMessage);
			}
			g->popTransform();
		}
	}

	if (isLoading()) return; // only start drawing the rest of the playfield if everything has loaded

	// draw first person crosshair
	if (OsuGameRules::osu_mod_fps.getBool())
	{
		const int length = 15;
		Vector2 center = osuCoords2Pixels(Vector2(OsuGameRules::OSU_COORD_WIDTH/2, OsuGameRules::OSU_COORD_HEIGHT/2));
		g->setColor(0xff777777);
		g->drawLine(center.x, (int)(center.y - length), center.x, (int)(center.y + length + 1));
		g->drawLine((int)(center.x - length), center.y, (int)(center.x + length + 1), center.y);
	}

	// draw playfield border
	if (osu_draw_playfield_border.getBool() && !OsuGameRules::osu_mod_fps.getBool())
		m_osu->getHUD()->drawPlayfieldBorder(g, m_vPlayfieldCenter, m_vPlayfieldSize, m_fHitcircleDiameter);

	// allow players to not draw all hitobjects twice if in VR
	if (m_osu->isInVRMode() && !m_osu_vr_draw_desktop_playfield_ref->getBool())
		return;

	// draw followpoints
	if (osu_draw_followpoints.getBool() && !OsuGameRules::osu_mod_mafham.getBool())
		drawFollowPoints(g);

	// draw all hitobjects in reverse
	if (m_osu_draw_hitobjects_ref->getBool())
	{
		const long curPos = m_iCurMusicPosWithOffsets;
		const long pvs = getPVS();
		const bool usePVS = m_osu_pvs->getBool();

		if (!OsuGameRules::osu_mod_mafham.getBool())
		{
			if (!osu_draw_reverse_order.getBool())
			{
				for (int i=m_hitobjectsSortedByEndTime.size()-1; i>=0; i--)
				{
					// PVS optimization (reversed)
					if (usePVS)
					{
						if (m_hitobjectsSortedByEndTime[i]->isFinished() && (curPos - pvs > m_hitobjectsSortedByEndTime[i]->getTime() + m_hitobjectsSortedByEndTime[i]->getDuration())) // past objects
							break;
						if (m_hitobjectsSortedByEndTime[i]->getTime() > curPos + pvs) // future objects
							continue;
					}

					m_hitobjectsSortedByEndTime[i]->draw(g);
				}
			}
			else
			{
				for (int i=0; i<m_hitobjectsSortedByEndTime.size(); i++)
				{
					// PVS optimization
					if (usePVS)
					{
						if (m_hitobjectsSortedByEndTime[i]->isFinished() && (curPos - pvs > m_hitobjectsSortedByEndTime[i]->getTime() + m_hitobjectsSortedByEndTime[i]->getDuration())) // past objects
							continue;
						if (m_hitobjectsSortedByEndTime[i]->getTime() > curPos + pvs) // future objects
							break;
					}

					m_hitobjectsSortedByEndTime[i]->draw(g);
				}
			}
			for (int i=0; i<m_hitobjectsSortedByEndTime.size(); i++)
			{
				// PVS optimization
				if (usePVS)
				{
					if (m_hitobjectsSortedByEndTime[i]->isFinished() && (curPos - pvs > m_hitobjectsSortedByEndTime[i]->getTime() + m_hitobjectsSortedByEndTime[i]->getDuration())) // past objects
						continue;
					if (m_hitobjectsSortedByEndTime[i]->getTime() > curPos + pvs) // future objects
						break;
				}

				m_hitobjectsSortedByEndTime[i]->draw2(g);
			}
		}
		else
		{
			if (m_mafhamActiveRenderTarget == NULL)
				m_mafhamActiveRenderTarget = m_osu->getFrameBuffer();

			if (m_mafhamFinishedRenderTarget == NULL)
				m_mafhamFinishedRenderTarget = m_osu->getFrameBuffer2();

			// if we have a chunk to render into the scene buffer
			const bool shouldDrawBuffer = (m_hitobjectsSortedByEndTime.size() - m_iCurrentHitObjectIndex) > OsuGameRules::osu_mod_mafham_render_livesize.getInt();
			bool shouldRenderChunk = m_iMafhamHitObjectRenderIndex < m_hitobjectsSortedByEndTime.size() && shouldDrawBuffer;
			if (shouldRenderChunk)
			{
				m_bInMafhamRenderChunk = true;

				m_mafhamActiveRenderTarget->setClearColorOnDraw(m_iMafhamHitObjectRenderIndex == 0);
				m_mafhamActiveRenderTarget->setClearDepthOnDraw(m_iMafhamHitObjectRenderIndex == 0);

				m_mafhamActiveRenderTarget->enable();
				glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA); // HACKHACK: OpenGL hardcoded
				int chunkCounter = 0;
				for (int i=m_hitobjectsSortedByEndTime.size()-1 - m_iMafhamHitObjectRenderIndex; i>=0; i--, m_iMafhamHitObjectRenderIndex++)
				{
					chunkCounter++;
					if (chunkCounter > osu_mod_mafham_render_chunksize.getInt())
						break; // continue chunk render in next frame

					if (i <= m_iCurrentHitObjectIndex + OsuGameRules::osu_mod_mafham_render_livesize.getInt()) // skip live objects
					{
						m_iMafhamHitObjectRenderIndex = m_hitobjectsSortedByEndTime.size(); // stop chunk render
						break;
					}

					// PVS optimization (reversed)
					if (usePVS)
					{
						if (m_hitobjectsSortedByEndTime[i]->isFinished() && (curPos - pvs > m_hitobjectsSortedByEndTime[i]->getTime() + m_hitobjectsSortedByEndTime[i]->getDuration())) // past objects
						{
							m_iMafhamHitObjectRenderIndex = m_hitobjectsSortedByEndTime.size(); // stop chunk render
							break;
						}
						if (m_hitobjectsSortedByEndTime[i]->getTime() > curPos + pvs) // future objects
							continue;
					}

					m_hitobjectsSortedByEndTime[i]->draw(g);

					m_iMafhamActiveRenderHitObjectIndex = i;
				}
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // HACKHACK: OpenGL hardcoded
				m_mafhamActiveRenderTarget->disable();

				m_bInMafhamRenderChunk = false;
			}
			shouldRenderChunk = m_iMafhamHitObjectRenderIndex < m_hitobjectsSortedByEndTime.size() && shouldDrawBuffer;
			if (!shouldRenderChunk && m_bMafhamRenderScheduled)
			{
				// finished, we can now swap the active framebuffer with the one we just finished
				m_bMafhamRenderScheduled = false;

				RenderTarget *temp = m_mafhamFinishedRenderTarget;
				m_mafhamFinishedRenderTarget = m_mafhamActiveRenderTarget;
				m_mafhamActiveRenderTarget = temp;

				m_iMafhamFinishedRenderHitObjectIndex = m_iMafhamActiveRenderHitObjectIndex;
				m_iMafhamActiveRenderHitObjectIndex = m_hitobjectsSortedByEndTime.size(); // reset
			}

			// draw scene buffer
			if (shouldDrawBuffer)
			{
				glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA); // HACKHACK: OpenGL hardcoded
				m_mafhamFinishedRenderTarget->draw(g, 0, 0);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // HACKHACK: OpenGL hardcoded
			}

			// draw followpoints
			if (osu_draw_followpoints.getBool())
				drawFollowPoints(g);

			// draw live hitobjects (also, code duplication yay)
			{
				for (int i=m_hitobjectsSortedByEndTime.size()-1; i>=0; i--)
				{
					// PVS optimization (reversed)
					if (usePVS)
					{
						if (m_hitobjectsSortedByEndTime[i]->isFinished() && (curPos - pvs > m_hitobjectsSortedByEndTime[i]->getTime() + m_hitobjectsSortedByEndTime[i]->getDuration())) // past objects
							break;
						if (m_hitobjectsSortedByEndTime[i]->getTime() > curPos + pvs) // future objects
							continue;
					}

					if (i > m_iCurrentHitObjectIndex + OsuGameRules::osu_mod_mafham_render_livesize.getInt() || (i > m_iMafhamFinishedRenderHitObjectIndex-1 && shouldDrawBuffer)) // skip non-live objects
						continue;

					m_hitobjectsSortedByEndTime[i]->draw(g);
				}

				for (int i=0; i<m_hitobjectsSortedByEndTime.size(); i++)
				{
					// PVS optimization
					if (usePVS)
					{
						if (m_hitobjectsSortedByEndTime[i]->isFinished() && (curPos - pvs > m_hitobjectsSortedByEndTime[i]->getTime() + m_hitobjectsSortedByEndTime[i]->getDuration())) // past objects
							continue;
						if (m_hitobjectsSortedByEndTime[i]->getTime() > curPos + pvs) // future objects
							break;
					}

					if (i >= m_iCurrentHitObjectIndex + OsuGameRules::osu_mod_mafham_render_livesize.getInt() || (i >= m_iMafhamFinishedRenderHitObjectIndex-1 && shouldDrawBuffer)) // skip non-live objects
						break;

					m_hitobjectsSortedByEndTime[i]->draw2(g);
				}
			}
		}
	}

	if (m_bFailed)
	{
		float failTimePercentInv = 1.0f - clamp<float>((m_fFailTime - engine->getTime()) / m_osu_fail_time_ref->getFloat(), 0.0f, 1.0f); // goes from 0 to 1 over the duration of osu_fail_time
		Vector2 playfieldBorderTopLeft = Vector2((int)(m_vPlayfieldCenter.x - m_vPlayfieldSize.x/2 - m_fHitcircleDiameter/2), (int)(m_vPlayfieldCenter.y - m_vPlayfieldSize.y/2 - m_fHitcircleDiameter/2));
		Vector2 playfieldBorderSize = Vector2((int)(m_vPlayfieldSize.x + m_fHitcircleDiameter), (int)(m_vPlayfieldSize.y + m_fHitcircleDiameter));

		g->setColor(0xff000000);
		g->setAlpha(failTimePercentInv);
		g->fillRect(playfieldBorderTopLeft.x, playfieldBorderTopLeft.y, playfieldBorderSize.x, playfieldBorderSize.y);
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
}

void OsuBeatmapStandard::drawVR(Graphics *g, Matrix4 &mvp, OsuVR *vr)
{
	OsuBeatmap::drawVR(g, mvp, vr);
	if (!canDraw()) return;

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

		// only start drawing the rest of the playfield if the music has loaded
		if (!isLoading())
		{
			// draw all hitobjects in reverse
			if (m_osu_draw_hitobjects_ref->getBool())
			{
				g->setDepthBuffer(false);
				vr->getShaderTexturedLegacyGeneric()->enable();
				{
					const long curPos = m_iCurMusicPosWithOffsets;
					const long pvs = getPVS();
					const bool usePVS = m_osu_pvs->getBool();

					if (!osu_draw_reverse_order.getBool())
					{
						for (int i=m_hitobjectsSortedByEndTime.size()-1; i>=0; i--)
						{
							// PVS optimization (reversed)
							if (usePVS)
							{
								if (m_hitobjectsSortedByEndTime[i]->isFinished() && (curPos - pvs > m_hitobjectsSortedByEndTime[i]->getTime() + m_hitobjectsSortedByEndTime[i]->getDuration())) // past objects
									break;
								if (m_hitobjectsSortedByEndTime[i]->getTime() > curPos + pvs) // future objects
									continue;
							}

							m_hitobjectsSortedByEndTime[i]->drawVR(g, mvp, vr);
						}
					}
					else
					{
						for (int i=0; i<m_hitobjectsSortedByEndTime.size(); i++)
						{
							// PVS optimization
							if (usePVS)
							{
								if (m_hitobjectsSortedByEndTime[i]->isFinished() && (curPos - pvs > m_hitobjectsSortedByEndTime[i]->getTime() + m_hitobjectsSortedByEndTime[i]->getDuration())) // past objects
									continue;
								if (m_hitobjectsSortedByEndTime[i]->getTime() > curPos + pvs) // future objects
									break;
							}

							m_hitobjectsSortedByEndTime[i]->drawVR(g, mvp, vr);
						}
					}
					if (OsuHitObject::m_osu_vr_draw_approach_circles->getBool() && OsuHitObject::m_osu_vr_approach_circles_on_top->getBool())
					{
						for (int i=0; i<m_hitobjectsSortedByEndTime.size(); i++)
						{
							// PVS optimization
							if (usePVS)
							{
								if (m_hitobjectsSortedByEndTime[i]->isFinished() && (curPos - pvs > m_hitobjectsSortedByEndTime[i]->getTime() + m_hitobjectsSortedByEndTime[i]->getDuration())) // past objects
									continue;
								if (m_hitobjectsSortedByEndTime[i]->getTime() > curPos + pvs) // future objects
									break;
							}

							m_hitobjectsSortedByEndTime[i]->drawVR2(g, mvp, vr);
						}
					}
				}
				vr->getShaderTexturedLegacyGeneric()->disable();
				g->setDepthBuffer(true);
			}

			if (m_bFailed)
			{
				vr->getShaderUntexturedLegacyGeneric()->enable();
				vr->getShaderUntexturedLegacyGeneric()->setUniformMatrix4fv("matrix", mvp);
				{
					Vector2 vrPlayfieldCenter = Vector2(0,0);
					Vector2 vrPlayfieldSize = Vector2(OsuGameRules::OSU_COORD_WIDTH, OsuGameRules::OSU_COORD_HEIGHT);
					float vrHitcircleDiameter = getHitcircleDiameter();

					float failTimePercentInv = 1.0f - clamp<float>((m_fFailTime - engine->getTime()) / m_osu_fail_time_ref->getFloat(), 0.0f, 1.0f); // goes from 0 to 1 over the duration of osu_fail_time
					Vector2 playfieldBorderTopLeft = Vector2((int)(vrPlayfieldCenter.x - vrPlayfieldSize.x/2 - vrHitcircleDiameter/2), (int)(vrPlayfieldCenter.y - vrPlayfieldSize.y/2 - vrHitcircleDiameter/2));
					Vector2 playfieldBorderSize = Vector2((int)(vrPlayfieldSize.x + vrHitcircleDiameter), (int)(vrPlayfieldSize.y + vrHitcircleDiameter));

					g->setColor(0xff000000);
					g->setAlpha(failTimePercentInv);
					g->pushTransform();
					g->translate(0, 0, 1.0f);
					g->fillRect(playfieldBorderTopLeft.x, playfieldBorderTopLeft.y, playfieldBorderSize.x, playfieldBorderSize.y);
					g->popTransform();
				}
				vr->getShaderUntexturedLegacyGeneric()->disable();
			}
		}
	}
	m_bIsVRDraw = false;
}

void OsuBeatmapStandard::drawFollowPoints(Graphics *g)
{
	OsuSkin *skin = m_osu->getSkin();

	const long curPos = m_iCurMusicPosWithOffsets;
	const long approachTime = std::min((long)OsuGameRules::getApproachTime(this), (long)osu_followpoints_approachtime.getFloat());

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
				float followAnimPercent = clamp<float>((float)(curPos - fadeInTime)/(float)m_osu_followpoints_prevfadetime_ref->getFloat(), 0.0f, 1.0f);
				followAnimPercent = -followAnimPercent*(followAnimPercent-2); // quad out

				const float scale = 1.5f - 0.5f*followAnimPercent;
				const Vector2 followPos = animPosStart + (finalPos - animPosStart)*followAnimPercent;

				// bullshit performance optimization: only draw followpoints if within screen bounds (plus a bit of a margin)
				// there is only one beatmap where this matters currently: https://osu.ppy.sh/b/1145513
				if (followPos.x < -m_osu->getScreenWidth() || followPos.x > m_osu->getScreenWidth()*2 || followPos.y < -m_osu->getScreenHeight() || followPos.y > m_osu->getScreenHeight()*2)
					continue;

				// calculate trail alpha
				if (curPos >= fadeInTime && curPos < fadeOutTime)
				{
					// future trail
					const float delta = curPos - fadeInTime;
					alpha = (float)delta / (float)approachTime;
				}
				else if (curPos >= fadeOutTime && curPos < fadeOutTime+(long)m_osu_followpoints_prevfadetime_ref->getFloat())
				{
					// previous trail
					const long delta = curPos - fadeOutTime;
					alpha = 1.0f - (float)delta / (float)(m_osu_followpoints_prevfadetime_ref->getFloat());
				}
				else
					alpha = 0.0f;

				// draw it
				g->setColor(0xffffffff);
				g->setAlpha(alpha);
				g->pushTransform();
					g->rotate(rad2deg(atan2(yDiff, xDiff)));
					skin->getFollowPoint2()->setAnimationTimeOffset(fadeInTime - 150); // HACKHACK: hardcoded 150 is good enough, was approachTime/2.5 (osu! allows followpoints to finish before hitobject fadein (!), fuck that)

					// NOTE: getSizeBaseRaw() depends on the current animation time being set correctly beforehand! (otherwise you get incorrect scales, e.g. for animated elements with inconsistent @2x mixed in)
					// the followpoints are scaled by one eighth of the hitcirclediameter (not the raw diameter, but the scaled diameter)
					const float followPointImageScale = ((m_fHitcircleDiameter/8.0f) / skin->getFollowPoint2()->getSizeBaseRaw().x) * osu_followpoints_scale_multiplier.getFloat();

					skin->getFollowPoint2()->drawRaw(g, followPos, followPointImageScale*scale);
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

void OsuBeatmapStandard::update()
{
	if (!canUpdate())
	{
		OsuBeatmap::update();
		return;
	}

	// the order of execution in here is a bit specific, since some things do need to be updated before loading has finished
	// note that the baseclass call to update() is NOT the first call, but comes after updating the metrics and wobble

	// live update hitobject and playfield metrics
	updateHitobjectMetrics();
	updatePlayfieldMetrics();

	// wobble mod
	if (osu_mod_wobble.getBool())
	{
		const float speedMultiplierCompensation = 1.0f / getSpeedMultiplier();
		m_fPlayfieldRotation = (m_iCurMusicPos/1000.0f)*30.0f*speedMultiplierCompensation*osu_mod_wobble_rotation_speed.getFloat();
	}
	else
		m_fPlayfieldRotation = 0.0f;

	// baseclass call (does the actual hitobject updates among other things)
	OsuBeatmap::update();

	// handle preloading (only for distributed slider vertexbuffer generation atm)
	if (m_bIsPreLoading)
	{
		if (Osu::debug->getBool() && m_iPreLoadingIndex == 0)
			debugLog("OsuBeatmapStandard: Preloading slider vertexbuffers ...\n");

		double startTime = engine->getTimeReal();
		double delta = 0.0;
		while (delta < 0.010 && m_bIsPreLoading) // hardcoded VR deadline of 10 ms (11 but sanity), will temporarily bring us down to 45 fps on average (better than freezing). works fine for desktop gameplay too
		{
			if (m_iPreLoadingIndex >= m_hitobjects.size())
			{
				m_bIsPreLoading = false;
				debugLog("OsuBeatmapStandard: Preloading done.\n");
				break;
			}
			else
			{
				OsuSlider *sliderPointer = dynamic_cast<OsuSlider*>(m_hitobjects[m_iPreLoadingIndex]);
				if (sliderPointer != NULL)
					sliderPointer->rebuildVertexBuffer();
			}

			m_iPreLoadingIndex++;
			delta = engine->getTimeReal() - startTime;
		}
	}

	// notify all other players (including ourself) once we've finished loading
	if (m_osu->isInMultiplayer())
	{
		if (!isLoadingInt()) // if local loading has finished
		{
			if (m_osu->getMultiplayer()->isWaitingForClient()) // if we are still in the waiting state
				m_osu->getMultiplayer()->onClientStatusUpdate(false, false);
		}
	}

	if (isLoading()) return; // only continue if we have loaded everything

	// update auto (after having updated the hitobjects)
	if (m_osu->getModAuto() || m_osu->getModAutopilot())
		updateAutoCursorPos();

	// spinner detection (used temporarily by OsuHUD for not drawing the hiterrorbar)
	if (m_currentHitObject != NULL)
	{
		OsuSpinner *spinnerPointer = dynamic_cast<OsuSpinner*>(m_currentHitObject);
		if (spinnerPointer != NULL)
			m_bIsSpinnerActive = true;
		else
			m_bIsSpinnerActive = false;
	}

	// scene buffering logic
	if (OsuGameRules::osu_mod_mafham.getBool())
	{
		if (!m_bMafhamRenderScheduled && m_iCurrentHitObjectIndex != m_iMafhamPrevHitObjectIndex) // if we are not already rendering and the index changed
		{
			m_iMafhamPrevHitObjectIndex = m_iCurrentHitObjectIndex;
			m_iMafhamHitObjectRenderIndex = 0;
			m_bMafhamRenderScheduled = true;
		}
	}
}

void OsuBeatmapStandard::onModUpdate(bool rebuildSliderVertexBuffers)
{
	debugLog("OsuBeatmapStandard::onModUpdate() @ %f\n", engine->getTime());

	m_osu->getMultiplayer()->onServerModUpdate();

	updatePlayfieldMetrics();
	updateHitobjectMetrics();

	if (m_music != NULL)
	{
		m_music->setSpeed(m_osu->getSpeedMultiplier());
		m_music->setPitch(m_osu->getPitchMultiplier());
	}

	// recalculate slider vertexbuffers
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
		calculateStacks();

		m_bWasEZEnabled = m_osu->getModEZ();
		if (rebuildSliderVertexBuffers)
			updateSliderVertexBuffers();
	}
	if (getHitcircleDiameter() != m_fPrevHitCircleDiameter && m_hitobjects.size() > 0)
	{
		calculateStacks();

		m_fPrevHitCircleDiameter = getHitcircleDiameter();
		if (rebuildSliderVertexBuffers)
			updateSliderVertexBuffers();
	}
	if (osu_playfield_rotation.getFloat() != m_fPrevPlayfieldRotationFromConVar)
	{
		m_fPrevPlayfieldRotationFromConVar = osu_playfield_rotation.getFloat();
		if (rebuildSliderVertexBuffers)
			updateSliderVertexBuffers();
	}
	if (osu_playfield_stretch_x.getFloat() != m_fPrevPlayfieldStretchX)
	{
		calculateStacks();

		m_fPrevPlayfieldStretchX = osu_playfield_stretch_x.getFloat();
		if (rebuildSliderVertexBuffers)
			updateSliderVertexBuffers();
	}
	if (osu_playfield_stretch_y.getFloat() != m_fPrevPlayfieldStretchY)
	{
		calculateStacks();

		m_fPrevPlayfieldStretchY = osu_playfield_stretch_y.getFloat();
		if (rebuildSliderVertexBuffers)
			updateSliderVertexBuffers();
	}
	if (OsuGameRules::osu_mod_mafham.getBool() != m_bWasMafhamEnabled)
	{
		m_bWasMafhamEnabled = OsuGameRules::osu_mod_mafham.getBool();
		for (int i=0; i<m_hitobjects.size(); i++)
		{
			m_hitobjects[i]->update(m_iCurMusicPosWithOffsets);
		}
	}

	// recalculate star cache for live pp
	if (m_osu_draw_statistics_pp->getBool()) // sanity + performance/usability
	{
		bool didCSChange = false;
		if (getHitcircleDiameter() != m_fPrevHitCircleDiameterForStarCache && m_hitobjects.size() > 0)
		{
			m_fPrevHitCircleDiameterForStarCache = getHitcircleDiameter();
			didCSChange = true;
		}

		bool didSpeedChange = false;
		if (m_osu->getSpeedMultiplier() != m_fPrevSpeedForStarCache && m_hitobjects.size() > 0)
		{
			m_fPrevSpeedForStarCache = m_osu->getSpeedMultiplier(); // this is not using the beatmap function for speed on purpose, because that wouldn't work while the music is still loading
			didSpeedChange = true;
		}

		if (didCSChange || didSpeedChange)
		{
			if (m_selectedDifficulty != NULL)
			{
				updateStarCache();
				updateStars();
			}
		}
	}
}

bool OsuBeatmapStandard::isLoading()
{
	return (isLoadingInt() || (m_osu->isInMultiplayer() && m_osu->getMultiplayer()->isWaitingForPlayers()));
}

Vector2 OsuBeatmapStandard::osuCoords2Pixels(Vector2 coords)
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

	if (m_bFailed)
	{
		float failTimePercentInv = 1.0f - clamp<float>((m_fFailTime - engine->getTime()) / m_osu_fail_time_ref->getFloat(), 0.0f, 1.0f); // goes from 0 to 1 over the duration of osu_fail_time

		coords.x -= OsuGameRules::OSU_COORD_WIDTH/2;
		coords.y -= OsuGameRules::OSU_COORD_HEIGHT/2;

		Vector3 coords3 = Vector3(coords.x, coords.y, 0);
		Matrix4 rot;
		rot.rotateZ(failTimePercentInv*60.0f);

		coords3 = coords3 * rot;
		coords3.x += OsuGameRules::OSU_COORD_WIDTH/2;
		coords3.y += OsuGameRules::OSU_COORD_HEIGHT/2;

		coords.x = coords3.x + failTimePercentInv*OsuGameRules::OSU_COORD_WIDTH*0.25f;
		coords.y = coords3.y + failTimePercentInv*OsuGameRules::OSU_COORD_HEIGHT*1.25f;
	}

	// playfield stretching
	coords.x -= OsuGameRules::OSU_COORD_WIDTH/2; // center
	coords.y -= OsuGameRules::OSU_COORD_HEIGHT/2;
	coords.x *= 1.0f + osu_playfield_stretch_x.getFloat(); // stretch
	coords.y *= 1.0f + osu_playfield_stretch_y.getFloat();
	coords.x += OsuGameRules::OSU_COORD_WIDTH/2; // undo center
	coords.y += OsuGameRules::OSU_COORD_HEIGHT/2;

	// scale and offset
	coords *= m_fScaleFactor;
	coords += m_vPlayfieldOffset; // the offset is already scaled, just add it

	// first person mod, centered cursor
	if (OsuGameRules::osu_mod_fps.getBool())
	{
		// this is the worst hack possible (engine->isDrawing()), but it works
		// the problem is that this same function is called while draw()ing and update()ing
		if ((engine->isDrawing() && (m_osu->getModAuto() || m_osu->getModAutopilot())) || !(m_osu->getModAuto() || m_osu->getModAutopilot()))
			coords += getFirstPersonCursorDelta();
	}

	return coords;
}

Vector2 OsuBeatmapStandard::osuCoords2RawPixels(Vector2 coords)
{
	// scale and offset
	coords *= m_fScaleFactor;
	coords += m_vPlayfieldOffset; // the offset is already scaled, just add it

	return coords;
}

Vector2 OsuBeatmapStandard::osuCoords2VRPixels(Vector2 coords)
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

	if (m_bFailed)
	{
		float failTimePercentInv = 1.0f - clamp<float>((m_fFailTime - engine->getTime()) / m_osu_fail_time_ref->getFloat(), 0.0f, 1.0f); // goes from 0 to 1 over the duration of osu_fail_time

		coords.x -= OsuGameRules::OSU_COORD_WIDTH/2;
		coords.y -= OsuGameRules::OSU_COORD_HEIGHT/2;

		Vector3 coords3 = Vector3(coords.x, coords.y, 0);
		Matrix4 rot;
		rot.rotateZ(failTimePercentInv*60.0f);

		coords3 = coords3 * rot;
		coords3.x += OsuGameRules::OSU_COORD_WIDTH/2;
		coords3.y += OsuGameRules::OSU_COORD_HEIGHT/2;

		coords.x = coords3.x + failTimePercentInv*OsuGameRules::OSU_COORD_WIDTH*0.25f;
		coords.y = coords3.y + failTimePercentInv*OsuGameRules::OSU_COORD_HEIGHT*1.25f;
	}

	// VR center
	coords.x -= OsuGameRules::OSU_COORD_WIDTH/2;
	coords.y -= OsuGameRules::OSU_COORD_HEIGHT/2;

	// VR scale
	coords.x *= 1.0f + osu_playfield_stretch_x.getFloat();
	coords.y *= 1.0f + osu_playfield_stretch_y.getFloat();

	return coords;
}

Vector2 OsuBeatmapStandard::osuCoords2LegacyPixels(Vector2 coords)
{
	if (m_osu->getModHR() || osu_playfield_mirror_horizontal.getBool())
		coords.y = OsuGameRules::OSU_COORD_HEIGHT - coords.y;
	if (osu_playfield_mirror_vertical.getBool())
		coords.x = OsuGameRules::OSU_COORD_WIDTH - coords.x;

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

	// VR center
	coords.x -= OsuGameRules::OSU_COORD_WIDTH/2;
	coords.y -= OsuGameRules::OSU_COORD_HEIGHT/2;

	// VR scale
	coords.x *= 1.0f + osu_playfield_stretch_x.getFloat();
	coords.y *= 1.0f + osu_playfield_stretch_y.getFloat();

	return coords;
}

Vector2 OsuBeatmapStandard::getCursorPos()
{
	if (OsuGameRules::osu_mod_fps.getBool() && !m_bIsPaused)
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

Vector2 OsuBeatmapStandard::getFirstPersonCursorDelta()
{
	return m_vPlayfieldCenter - (m_osu->getModAuto() || m_osu->getModAutopilot() ? m_vAutoCursorPos : engine->getMouse()->getPos());
}

float OsuBeatmapStandard::getHitcircleDiameter()
{
	// in VR, there is no resolution to which the playfield would have to get scaled up to (since the entire playfield is scaled at once as the player sees fit)
	// therefore just return the raw hitcircle diameter (osu!pixels)
	return m_bIsVRDraw ? m_fRawHitcircleDiameter : m_fHitcircleDiameter;
}

void OsuBeatmapStandard::onBeforeLoad()
{
	debugLog("OsuBeatmapStandard::onBeforeLoad()\n");

	m_osu->getMultiplayer()->onServerPlayStateChange(OsuMultiplayer::STATE::START, 0, false, this);

	// some hitobjects already need this information to be up-to-date before their constructor is called
	updatePlayfieldMetrics();
	updateHitobjectMetrics();

	m_bIsPreLoading = false;
}

void OsuBeatmapStandard::onLoad()
{
	debugLog("OsuBeatmapStandard::onLoad()\n");

	// after the hitobjects have been loaded we can calculate the stacks
	calculateStacks();

	// start preloading (delays the play start until it's set to false, see isLoading())
	m_bIsPreLoading = true;
	m_iPreLoadingIndex = 0;

	// build stars
	m_fStarCacheTime = engine->getTime() + osu_pp_live_timeout.getFloat(); // first time delay only. subsequent updates should immediately show the loading spinner
	updateStarCache();
	updateStars();
}

void OsuBeatmapStandard::onPlayStart()
{
	debugLog("OsuBeatmapStandard::onPlayStart()\n");

	onModUpdate(false); // if there are calculations in there that need the hitobjects to be loaded
}

void OsuBeatmapStandard::onBeforeStop(bool quit)
{
	debugLog("OsuBeatmapStandard::onBeforeStop()\n");

	// kill any running star cache loader
	stopStarCacheLoader();

	if (!quit) // if the ranking screen is going to be shown
	{
		// calculate final pp
		debugLog("OsuBeatmapStandard::onBeforeStop() calculating pp ...\n");
		double aim = 0.0;
		double speed = 0.0;
		const double totalStars = m_selectedDifficulty->calculateStarDiff(this, &aim, &speed);
		m_fAimStars = (float)aim;
		m_fSpeedStars = (float)speed;

		const int numHitObjects = m_hitobjects.size();
		const int numCircles = m_selectedDifficulty->hitcircles.size();
		const int maxPossibleCombo = m_selectedDifficulty->getMaxCombo();
		const int highestCombo = m_osu->getScore()->getComboMax();
		const int numMisses = m_osu->getScore()->getNumMisses();
		const int num300s = m_osu->getScore()->getNum300s();
		const int num100s = m_osu->getScore()->getNum100s();
		const int num50s = m_osu->getScore()->getNum50s();
		const float pp = m_selectedDifficulty->calculatePPv2(m_osu, this, aim, speed, numHitObjects, numCircles, maxPossibleCombo, highestCombo, numMisses, num300s, num100s, num50s);
		m_osu->getScore()->setStarsTomTotal(totalStars);
		m_osu->getScore()->setStarsTomAim(m_fAimStars);
		m_osu->getScore()->setStarsTomSpeed(m_fSpeedStars);
		m_osu->getScore()->setPPv2(pp);
		debugLog("OsuBeatmapStandard::onBeforeStop() done.\n");

		// save local score, but only under certain conditions
		const bool isComplete = (num300s + num100s + num50s + numMisses >= numHitObjects);
		const bool isZero = (m_osu->getScore()->getScore() < 1);
		const bool isUnranked = (m_osu->getModAuto() || (m_osu->getModAutopilot() && m_osu->getModRelax()));
		int scoreIndex = -1;
		if (isComplete && !isZero && !isUnranked && !m_osu->getScore()->hasDied())
		{
			const int scoreVersion = 20180722;

			debugLog("OsuBeatmapStandard::onBeforeStop() saving score ...\n");
			{
				OsuDatabase::Score score;

				score.isLegacyScore = false;
				score.version = scoreVersion;
				score.unixTimestamp = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();

				// default
				score.playerName = convar->getConVarByName("name")->getString();

				score.num300s = m_osu->getScore()->getNum300s();
				score.num100s = m_osu->getScore()->getNum100s();
				score.num50s = m_osu->getScore()->getNum50s();
				score.numGekis = m_osu->getScore()->getNum300gs();
				score.numKatus = m_osu->getScore()->getNum100ks();
				score.numMisses = m_osu->getScore()->getNumMisses();
				score.score = m_osu->getScore()->getScore();
				score.comboMax = m_osu->getScore()->getComboMax();
				score.modsLegacy = 0;
				score.modsLegacy |= (m_osu->getModAuto() ? OsuReplay::Mods::Autoplay : 0);
				score.modsLegacy |= (m_osu->getModAutopilot() ? OsuReplay::Mods::Relax2 : 0);
				score.modsLegacy |= (m_osu->getModRelax() ? OsuReplay::Mods::Relax : 0);
				score.modsLegacy |= (m_osu->getModSpunout() ? OsuReplay::Mods::SpunOut : 0);
				score.modsLegacy |= (m_osu->getModTarget() ? OsuReplay::Mods::Target : 0);
				score.modsLegacy |= (m_osu->getModScorev2() ? OsuReplay::Mods::ScoreV2 : 0);
				score.modsLegacy |= (m_osu->getModDT() ? OsuReplay::Mods::DoubleTime : 0);
				score.modsLegacy |= (m_osu->getModNC() ? OsuReplay::Mods::Nightcore : 0);
				score.modsLegacy |= (m_osu->getModNF() ? OsuReplay::Mods::NoFail : 0);
				score.modsLegacy |= (m_osu->getModHT() ? OsuReplay::Mods::HalfTime : 0);
				score.modsLegacy |= (m_osu->getModDC() ? OsuReplay::Mods::HalfTime : 0);
				score.modsLegacy |= (m_osu->getModHD() ? OsuReplay::Mods::Hidden : 0);
				score.modsLegacy |= (m_osu->getModHR() ? OsuReplay::Mods::HardRock : 0);
				score.modsLegacy |= (m_osu->getModEZ() ? OsuReplay::Mods::Easy : 0);
				score.modsLegacy |= (m_osu->getModSD() ? OsuReplay::Mods::SuddenDeath : 0);
				score.modsLegacy |= (m_osu->getModSS() ? OsuReplay::Mods::Perfect : 0);
				score.modsLegacy |= (m_osu->getModNM() ? OsuReplay::Mods::Nightmare : 0);

				// custom
				score.numSliderBreaks = m_osu->getScore()->getNumSliderBreaks();
				score.pp = pp;
				score.unstableRate = m_osu->getScore()->getUnstableRate();
				score.hitErrorAvgMin = m_osu->getScore()->getHitErrorAvgMin();
				score.hitErrorAvgMax = m_osu->getScore()->getHitErrorAvgMax();
				score.starsTomTotal = totalStars;
				score.starsTomAim = aim;
				score.starsTomSpeed = speed;
				score.speedMultiplier = m_osu->getSpeedMultiplier();
				score.CS = getCS();
				score.AR = getAR();
				score.OD = getOD();
				score.HP = getHP();
				std::vector<ConVar*> allExperimentalMods = m_osu->getExperimentalMods();
				for (int i=0; i<allExperimentalMods.size(); i++)
				{
					if (allExperimentalMods[i]->getBool())
					{
						score.experimentalModsConVars.append(allExperimentalMods[i]->getName());
						score.experimentalModsConVars.append(";");
					}
				}

				// save it
				scoreIndex = m_osu->getSongBrowser()->getDatabase()->addScore(m_selectedDifficulty->md5hash, score);
				if (scoreIndex == -1)
					m_osu->getNotificationOverlay()->addNotification(UString::format("Failed saving score! md5hash.length() = %i", m_selectedDifficulty->md5hash.length()), 0xffff0000, false, 3.0f);
			}
			debugLog("OsuBeatmapStandard::onBeforeStop() done.\n");
		}
		m_osu->getScore()->setIndex(scoreIndex);
	}
}

void OsuBeatmapStandard::onStop(bool quit)
{
	debugLog("OsuBeatmapStandard::onStop()\n");

	if (quit)
		m_osu->getMultiplayer()->onServerPlayStateChange(OsuMultiplayer::STATE::STOP);
}

void OsuBeatmapStandard::onPaused(bool first)
{
	debugLog("OsuBeatmapStandard::onPaused()\n");

	m_osu->getMultiplayer()->onServerPlayStateChange(OsuMultiplayer::STATE::PAUSE);

	if (first)
	{
		m_vContinueCursorPoint = engine->getMouse()->getPos();

		if (OsuGameRules::osu_mod_fps.getBool())
			m_vContinueCursorPoint = OsuGameRules::getPlayfieldCenter(m_osu);
	}
}

void OsuBeatmapStandard::onUnpaused()
{
	debugLog("OsuBeatmapStandard::onUnpaused()\n");

	m_osu->getMultiplayer()->onServerPlayStateChange(OsuMultiplayer::STATE::UNPAUSE);
}

void OsuBeatmapStandard::onRestart(bool quick)
{
	debugLog("OsuBeatmapStandard::onRestart()\n");

	m_osu->getMultiplayer()->onServerPlayStateChange(OsuMultiplayer::STATE::RESTART, 0, quick);
}

void OsuBeatmapStandard::updateAutoCursorPos()
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

	long curMusicPos = m_iCurMusicPosWithOffsets;

	if (m_bIsWaiting)
		prevTime = -(long)m_osu_early_note_time_ref->getInt();

	if (curMusicPos >= 0)
	{
		prevPos = m_hitobjects[0]->getAutoCursorPos(0);
		curPos = prevPos;
		nextPos = prevPos;
	}

	if (m_osu->getModAuto())
	{
		bool autoDanceOverride = false;
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
					if (osu_auto_cursordance.getBool())
					{
						OsuSlider *sliderPointer = dynamic_cast<OsuSlider*>(o);
						if (sliderPointer != NULL)
						{
							std::vector<OsuSlider::SLIDERCLICK> clicks = sliderPointer->getClicks();

							// start
							prevTime = o->getTime();
							prevPos = osuCoords2Pixels(o->getRawPosAt(prevTime));

							long biggestPrevious = 0;
							long smallestNext = std::numeric_limits<long>::max();
							bool allFinished = true;
							long endTime = 0;

							// middle clicks
							for (int c=0; c<clicks.size(); c++)
							{
								// get previous click
								if (clicks[c].time <= curMusicPos && clicks[c].time > biggestPrevious)
								{
									biggestPrevious = clicks[c].time;
									prevTime = clicks[c].time;
									prevPos = osuCoords2Pixels(o->getRawPosAt(prevTime));
								}

								// get next click
								if (clicks[c].time > curMusicPos && clicks[c].time < smallestNext)
								{
									smallestNext = clicks[c].time;
									nextTime = clicks[c].time;
									nextPos = osuCoords2Pixels(o->getRawPosAt(nextTime));
								}

								// end hack
								if (!clicks[c].finished)
									allFinished = false;
								else if (clicks[c].time > endTime)
									endTime = clicks[c].time;
							}

							// end
							if (allFinished)
							{
								// hack for slider without middle clicks
								if (endTime == 0)
									endTime = o->getTime();

								prevTime = endTime;
								prevPos = osuCoords2Pixels(o->getRawPosAt(prevTime));
								nextTime = o->getTime() + o->getDuration();
								nextPos = osuCoords2Pixels(o->getRawPosAt(nextTime));
							}

							haveCurPos = false;
							autoDanceOverride = true;
							break;
						}
					}

					haveCurPos = true;
					curPos = prevPos;
					break;
				}
			}

			// get next object
			if (o->getTime() > curMusicPos)
			{
				nextPosIndex = i;
				if (!autoDanceOverride)
				{
					nextPos = o->getAutoCursorPos(curMusicPos);
					nextTime = o->getTime();
				}
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

		if (osu_auto_cursordance.getBool() && !m_osu->getModAutopilot())
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

void OsuBeatmapStandard::updatePlayfieldMetrics()
{
	m_fScaleFactor = OsuGameRules::getPlayfieldScaleFactor(m_osu);
	m_vPlayfieldSize = OsuGameRules::getPlayfieldSize(m_osu);
	m_vPlayfieldOffset = OsuGameRules::getPlayfieldOffset(m_osu);
	m_vPlayfieldCenter = OsuGameRules::getPlayfieldCenter(m_osu);
}

void OsuBeatmapStandard::updateHitobjectMetrics()
{
	OsuSkin *skin = m_osu->getSkin();

	m_fRawHitcircleDiameter = OsuGameRules::getRawHitCircleDiameter(getCS());
	m_fXMultiplier = OsuGameRules::getHitCircleXMultiplier(m_osu);
	m_fHitcircleDiameter = OsuGameRules::getHitCircleDiameter(this);

	const float osuCoordScaleMultiplier = (getHitcircleDiameter() / m_fRawHitcircleDiameter);

	m_fNumberScale = (m_fRawHitcircleDiameter / (160.0f * (skin->isDefault12x() ? 2.0f : 1.0f))) * osuCoordScaleMultiplier * osu_number_scale_multiplier.getFloat();
	m_fHitcircleOverlapScale = (m_fRawHitcircleDiameter / (160.0f)) * osuCoordScaleMultiplier * osu_number_scale_multiplier.getFloat();

	m_fRawSliderFollowCircleDiameter = getRawHitcircleDiameter() * (m_osu->getModNM() || osu_mod_jigsaw2.getBool() ? (1.0f*(1.0f - osu_mod_jigsaw_followcircle_radius_factor.getFloat()) + osu_mod_jigsaw_followcircle_radius_factor.getFloat()*OsuGameRules::osu_slider_followcircle_size_multiplier.getFloat()) : OsuGameRules::osu_slider_followcircle_size_multiplier.getFloat());
	m_fSliderFollowCircleDiameter = getHitcircleDiameter() * (m_osu->getModNM() || osu_mod_jigsaw2.getBool() ? (1.0f*(1.0f - osu_mod_jigsaw_followcircle_radius_factor.getFloat()) + osu_mod_jigsaw_followcircle_radius_factor.getFloat()*OsuGameRules::osu_slider_followcircle_size_multiplier.getFloat()) : OsuGameRules::osu_slider_followcircle_size_multiplier.getFloat());
}

void OsuBeatmapStandard::updateSliderVertexBuffers()
{
	updatePlayfieldMetrics();
	updateHitobjectMetrics();

	m_bWasEZEnabled = m_osu->getModEZ(); // to avoid useless double updates in onModUpdate()
	m_fPrevHitCircleDiameter = getHitcircleDiameter(); // same here
	m_fPrevPlayfieldRotationFromConVar = osu_playfield_rotation.getFloat(); // same here
	m_fPrevPlayfieldStretchX = osu_playfield_stretch_x.getFloat(); // same here
	m_fPrevPlayfieldStretchY = osu_playfield_stretch_y.getFloat(); // same here

	debugLog("OsuBeatmapStandard::updateSliderVertexBuffers() for %i hitobjects ...\n", m_hitobjects.size());

	for (int i=0; i<m_hitobjects.size(); i++)
	{
		OsuSlider *sliderPointer = dynamic_cast<OsuSlider*>(m_hitobjects[i]);
		if (sliderPointer != NULL)
			sliderPointer->rebuildVertexBuffer();
	}
}

void OsuBeatmapStandard::calculateStacks()
{
	if (!osu_stacking.getBool()) return;

	updateHitobjectMetrics();

	debugLog("OsuBeatmapStandard: Calculating stacks ...\n");

	const float STACK_LENIENCE = 3.0f;
	const float STACK_OFFSET = 0.05f;

	// reset
	for (int i=0; i<m_hitobjects.size(); i++)
	{
		m_hitobjects[i]->setStack(0);
	}

	// peppy's algorithm
	// https://gist.github.com/peppy/1167470

	const float approachTime = OsuGameRules::getApproachTimeForStacking(this);
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

				if (objectI->getTime() - (approachTime * m_selectedDifficulty->stackLeniency) > (objectN->getTime() + objectN->getDuration()))
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

				if (objectI->getTime() - (approachTime * m_selectedDifficulty->stackLeniency) > objectN->getTime())
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

void OsuBeatmapStandard::updateStarCache()
{
	if (m_osu_draw_statistics_pp->getBool() && m_osu_pp_live_type->getInt() == 2)
	{
		// so we don't get a useless double load inside onModUpdate()
		m_fPrevHitCircleDiameterForStarCache = getHitcircleDiameter();
		m_fPrevSpeedForStarCache = m_osu->getSpeedMultiplier();

		// kill any running loader, so we get to a clean state
		stopStarCacheLoader();
		engine->getResourceManager()->destroyResource(m_starCacheLoader);

		// create new loader
		m_starCacheLoader = new StarCacheLoader(this);
		m_starCacheLoader->revive(); // activate it
		engine->getResourceManager()->requestNextLoadAsync();
		engine->getResourceManager()->loadResource(m_starCacheLoader);
	}
}

void OsuBeatmapStandard::updateStars()
{
	if (m_osu_draw_statistics_pp->getBool() && m_osu_pp_live_type->getInt() != 2)
	{
		double aim = 0.0;
		double speed = 0.0;
		m_selectedDifficulty->calculateStarDiff(this, &aim, &speed);
		m_fAimStars = (float)aim;
		m_fSpeedStars = (float)speed;
	}
}

void OsuBeatmapStandard::stopStarCacheLoader()
{
	if (!m_starCacheLoader->isDead())
	{
		m_starCacheLoader->kill();
		double startTime = engine->getTimeReal();
		while (!m_starCacheLoader->isAsyncReady()) // stall main thread until it's killed (this should be very quick, around max 1 ms, as the kill flag is checked in every iteration)
		{
			if (engine->getTimeReal() - startTime > 1)
				break;
		}
	}
}

bool OsuBeatmapStandard::isLoadingStarCache()
{
	return m_osu_draw_statistics_pp->getBool() && m_osu_pp_live_type->getInt() == 2 && !m_starCacheLoader->isReady();
}

bool OsuBeatmapStandard::isLoadingInt()
{
	return OsuBeatmap::isLoading() || m_bIsPreLoading || isLoadingStarCache();
}
