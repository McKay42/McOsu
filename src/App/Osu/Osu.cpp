//================ Copyright (c) 2015, PG, All rights reserved. =================//
//
// Purpose:		yet another ouendan clone, because why not
//
// $NoKeywords: $osu
//===============================================================================//

#include "Osu.h"

#include "Engine.h"
#include "Environment.h"
#include "Keyboard.h"
#include "Mouse.h"
#include "ConsoleBox.h"
#include "ResourceManager.h"
#include "AnimationHandler.h"
#include "NetworkHandler.h"
#include "SoundEngine.h"
#include "Console.h"
#include "ConVar.h"
#include "RenderTarget.h"
#include "Shader.h"

#include "CWindowManager.h"
//#include "DebugMonitor.h"

#include "OsuMainMenu.h"
#include "OsuOptionsMenu.h"
#include "OsuSongBrowser2.h"
#include "OsuModSelector.h"
#include "OsuRankingScreen.h"
#include "OsuKeyBindings.h"
#include "OsuNotificationOverlay.h"
#include "OsuTooltipOverlay.h"
#include "OsuGameRules.h"
#include "OsuPauseMenu.h"
#include "OsuBeatmap.h"
#include "OsuBeatmapDifficulty.h"
#include "OsuScore.h"
#include "OsuSkin.h"
#include "OsuHUD.h"

#include "OsuHitObject.h"
#include "OsuSliderRenderer.h"

#include "OsuBeatmapDatabase.h"

void DUMMY_OSU_LETTERBOXING(UString oldValue, UString newValue) {;}
void DUMMY_OSU_VOLUME_MUSIC_ARGS(UString oldValue, UString newValue) {;}
void DUMMY_OSU_MODS(void) {;}

ConVar osu_version("osu_version", 26.0f);
ConVar osu_debug("osu_debug", false);

ConVar osu_disable_mousebuttons("osu_disable_mousebuttons", false);
ConVar osu_disable_mousewheel("osu_disable_mousewheel", false);
ConVar osu_confine_cursor_windowed("osu_confine_cursor_windowed", false, DUMMY_OSU_LETTERBOXING);
ConVar osu_confine_cursor_fullscreen("osu_confine_cursor_fullscreen", true, DUMMY_OSU_LETTERBOXING);

ConVar osu_skin("osu_skin", "default", DUMMY_OSU_VOLUME_MUSIC_ARGS);
ConVar osu_skin_reload("osu_skin_reload", DUMMY_OSU_MODS);

ConVar osu_volume_master("osu_volume_master", 0.5f, DUMMY_OSU_VOLUME_MUSIC_ARGS);
ConVar osu_volume_music("osu_volume_music", 0.3f, DUMMY_OSU_VOLUME_MUSIC_ARGS);
ConVar osu_volume_change_interval("osu_volume_change_interval", 0.05f);

ConVar osu_speed_override("osu_speed_override", -1.0f, DUMMY_OSU_VOLUME_MUSIC_ARGS);
ConVar osu_pitch_override("osu_pitch_override", -1.0f, DUMMY_OSU_VOLUME_MUSIC_ARGS);

ConVar osu_pause_on_focus_loss("osu_pause_on_focus_loss", true);
ConVar osu_quick_retry_delay("osu_quick_retry_delay", 0.27f);

ConVar osu_mods("osu_mods", "", DUMMY_OSU_VOLUME_MUSIC_ARGS);
ConVar osu_mod_fadingcursor("osu_mod_fadingcursor", false);
ConVar osu_mod_fadingcursor_combo("osu_mod_fadingcursor_combo", 50.0f);

ConVar osu_letterboxing("osu_letterboxing", true, DUMMY_OSU_LETTERBOXING);
ConVar osu_resolution("osu_resolution", "1280x720", DUMMY_OSU_VOLUME_MUSIC_ARGS);
ConVar osu_resolution_enabled("osu_resolution_enabled", false);

ConVar osu_draw_fps("osu_draw_fps", true);
ConVar osu_hide_cursor_during_gameplay("osu_hide_cursor_during_gameplay", false);

ConVar *Osu::version = &osu_version;
ConVar *Osu::debug = &osu_debug;
Vector2 Osu::g_vInternalResolution;
Vector2 Osu::osuBaseResolution = Vector2(640.0f, 480.0f);

Osu::Osu()
{
	srand(time(NULL));

	// convar refs
	m_osu_folder_ref = convar->getConVarByName("osu_folder");
	m_osu_draw_hud_ref = convar->getConVarByName("osu_draw_hud");
	m_osu_mod_fps_ref = convar->getConVarByName("osu_mod_fps");

	// engine settings
	env->setWindowTitle("McOsu!");
	env->setCursorVisible(false);
	engine->getConsoleBox()->setRequireShiftToActivate(true);
	engine->getSound()->setVolume(osu_volume_master.getFloat());
	engine->getMouse()->addListener(this);
	convar->getConVarByName("name")->setValue("Guest");
	convar->getConVarByName("console_overlay")->setValue(0.0f);
	convar->getConVarByName("vsync")->setValue(0.0f);
	convar->getConVarByName("fps_max")->setValue(420.0f);
	osu_resolution.setValue(UString::format("%ix%i", engine->getScreenWidth(), engine->getScreenHeight()));

	// convar callbacks
	osu_skin.setCallback( fastdelegate::MakeDelegate(this, &Osu::onSkinChange) );
	osu_skin_reload.setCallback( fastdelegate::MakeDelegate(this, &Osu::onSkinReload) );

	osu_volume_master.setCallback( fastdelegate::MakeDelegate(this, &Osu::onMasterVolumeChange) );
	osu_volume_music.setCallback( fastdelegate::MakeDelegate(this, &Osu::onMusicVolumeChange) );
	osu_speed_override.setCallback( fastdelegate::MakeDelegate(this, &Osu::onSpeedChange) );
	osu_pitch_override.setCallback( fastdelegate::MakeDelegate(this, &Osu::onPitchChange) );

	osu_mods.setCallback( fastdelegate::MakeDelegate(this, &Osu::updateModsForConVarTemplate) );

	osu_resolution.setCallback( fastdelegate::MakeDelegate(this, &Osu::onInternalResolutionChanged) );
	osu_letterboxing.setCallback( fastdelegate::MakeDelegate(this, &Osu::onLetterboxingChange) );

	osu_confine_cursor_windowed.setCallback( fastdelegate::MakeDelegate(this, &Osu::onConfineCursorWindowedChange) );
	osu_confine_cursor_fullscreen.setCallback( fastdelegate::MakeDelegate(this, &Osu::onConfineCursorFullscreenChange) );

  	// vars
	m_skin = NULL;
	m_songBrowser2 = NULL;
	m_modSelector = NULL;

	m_bF1 = false;
	m_bUIToggleCheck = false;
	m_bTab = false;
	m_bEscape = false;
	m_bKey1 = false;
	m_bKey2 = false;
	m_bSkipScheduled = false;
	m_bQuickRetryDown = false;

	m_bToggleModSelectionScheduled = false;
	m_bToggleSongBrowserScheduled = false;
	m_bToggleOptionsMenuScheduled = false;
	m_bToggleRankingScreenScheduled = false;

	m_bModAuto = false;
	m_bModAutopilot = false;
	m_bModRelax = false;
	m_bModSpunout = false;
	m_bModTarget = false;
	m_bModDT = false;
	m_bModNC = false;
	m_bModHT = false;
	m_bModHD = false;
	m_bModHR = false;
	m_bModEZ = false;
	m_bModSD = false;
	m_bModSS = false;
	m_bModNM = false;

	m_bShouldCursorBeVisible = false;

	// debug
	m_windowManager = new CWindowManager();
	/*
	DebugMonitor *dm = new DebugMonitor();
	m_windowManager->addWindow(dm);
	dm->open();
	*/

	// renderer
	g_vInternalResolution = engine->getScreenSize();
	m_frameBuffer = new RenderTarget(0, 0, getScreenWidth(), getScreenHeight());
	m_backBuffer = new RenderTarget(0, 0, getScreenWidth(), getScreenHeight());

	// load a few select subsystems very early
	m_notificationOverlay = new OsuNotificationOverlay(this);
	m_score = new OsuScore(this);

	// exec the config file (this must be right here!)
	Console::execConfigFile("osu");

	// update mod settings
	updateMods();

	// load global resources
	m_titleFont = engine->getResourceManager()->loadFont("SourceSansPro-Semibold.otf", "FONT_OSU_TITLE", 40.0f);
	m_subTitleFont = engine->getResourceManager()->loadFont("SourceSansPro-Semibold.otf", "FONT_OSU_SUBTITLE", 21.0f);
	m_songBrowserFont = engine->getResourceManager()->loadFont("SourceSansPro-Regular.otf", "FONT_OSU_SONGBROWSER", 35.0f);
	m_songBrowserFontBold = engine->getResourceManager()->loadFont("SourceSansPro-Bold.otf", "FONT_OSU_SONGBROWSER_BOLD", 30.0f);

	// load skin
	UString skinFolder = m_osu_folder_ref->getString();
	skinFolder.append("Skins/");
	skinFolder.append(osu_skin.getString());
	skinFolder.append("/");
	if (m_skin == NULL) // the skin may already be loaded by Console::execConfigFile() above
		m_skin = new OsuSkin(this, skinFolder);

	// load subsystems, add them to the screens array
	m_tooltipOverlay = new OsuTooltipOverlay(this);
	m_mainMenu = new OsuMainMenu(this);
	m_optionsMenu = new OsuOptionsMenu(this);
	m_songBrowser2 = new OsuSongBrowser2(this);
	m_modSelector = new OsuModSelector(this);
	m_rankingScreen = new OsuRankingScreen(this);
	m_pauseMenu = new OsuPauseMenu(this);
	m_hud = new OsuHUD(this);

	// the order in this vector will define in which order events are handled/consumed
	m_screens.push_back(m_notificationOverlay);
	m_screens.push_back(m_rankingScreen);
	m_screens.push_back(m_modSelector);
	m_screens.push_back(m_pauseMenu);
	m_screens.push_back(m_hud);
	m_screens.push_back(m_songBrowser2);
	m_screens.push_back(m_optionsMenu);
	m_screens.push_back(m_mainMenu);
	m_screens.push_back(m_tooltipOverlay);

	// make primary screen visible
	//m_optionsMenu->setVisible(true);
	m_mainMenu->setVisible(true);
	//m_modSelector->setVisible(true);
	//m_songBrowser2->setVisible(true);
	//m_pauseMenu->setVisible(true);
	//m_rankingScreen->setVisible(true);
}

Osu::~Osu()
{
	SAFE_DELETE(m_windowManager);

	for (int i=0; i<m_screens.size(); i++)
	{
		SAFE_DELETE(m_screens[i]);
	}

	SAFE_DELETE(m_skin);
	SAFE_DELETE(m_score);

	SAFE_DELETE(m_frameBuffer);
	SAFE_DELETE(m_backBuffer);

	SAFE_DELETE(OsuSliderRenderer::BLEND_SHADER);
}

void Osu::draw(Graphics *g)
{
	if (m_skin == NULL) // sanity check
	{
		g->setColor(0xffff0000);
		g->fillRect(0, 0, getScreenWidth(), getScreenHeight());
		return;
	}

	// if we are not using the native window resolution, draw into the buffer
	if (osu_resolution_enabled.getBool())
		m_backBuffer->enable();

	// draw everything in the correct order
	if (isInPlayMode()) // if we are playing a beatmap
	{
		getSelectedBeatmap()->draw(g);
		m_hud->draw(g);

		// quick retry fadeout
		if (m_fQuickRetryTime != 0.0f && m_bQuickRetryDown)
		{
			float alphaPercent = 1.0f - (m_fQuickRetryTime - engine->getTime())/osu_quick_retry_delay.getFloat();
			if (engine->getTime() > m_fQuickRetryTime)
				alphaPercent = 1.0f;

			g->setColor(COLOR((int)(255*alphaPercent), 0, 0, 0));
			g->fillRect(0, 0, getScreenWidth(), getScreenHeight());
		}

		// special cursor handling
		const bool allowDrawCursor = !osu_hide_cursor_during_gameplay.getBool() || getSelectedBeatmap()->isPaused();
		float fadingCursorAlpha = 1.0f - clamp<float>((float)m_score->getCombo()/osu_mod_fadingcursor_combo.getFloat(), 0.0f, 1.0f);
		if (m_pauseMenu->isVisible() || getSelectedBeatmap()->isContinueScheduled())
			fadingCursorAlpha = 1.0f;

		if ((m_bModAuto || m_bModAutopilot) && allowDrawCursor)
			m_hud->drawCursor(g, m_osu_mod_fps_ref->getBool() ? OsuGameRules::getPlayfieldCenter(this) : getSelectedBeatmap()->getCursorPos(), osu_mod_fadingcursor.getBool() ? fadingCursorAlpha : 1.0f);

		m_pauseMenu->draw(g);
		m_modSelector->draw(g);

		if (osu_draw_fps.getBool())
			m_hud->drawFps(g);

		m_hud->drawVolumeChange(g);

		m_windowManager->draw(g);

		if (!(m_bModAuto || m_bModAutopilot) && allowDrawCursor)
			m_hud->drawCursor(g, getSelectedBeatmap()->getCursorPos(), osu_mod_fadingcursor.getBool() ? fadingCursorAlpha : 1.0f);
	}
	else // if we are not playing
	{
		if (m_songBrowser2 != NULL)
			m_songBrowser2->draw(g);
		m_modSelector->draw(g);
		m_mainMenu->draw(g);
		m_optionsMenu->draw(g);
		m_rankingScreen->draw(g);

		if (osu_draw_fps.getBool())
			m_hud->drawFps(g);

		m_hud->drawVolumeChange(g);

		m_windowManager->draw(g);

		m_hud->drawCursor(g, engine->getMouse()->getPos());
	}

	m_tooltipOverlay->draw(g);
	m_notificationOverlay->draw(g);

	// if we are not using the native window resolution
	if (osu_resolution_enabled.getBool())
	{
		// draw a scaled version from the buffer to the screen
		m_backBuffer->disable();

		g->setBlending(false);
		if (osu_letterboxing.getBool())
			m_backBuffer->draw(g, engine->getGraphics()->getResolution().x/2 - g_vInternalResolution.x/2, engine->getGraphics()->getResolution().y/2 - g_vInternalResolution.y/2, g_vInternalResolution.x, g_vInternalResolution.y);
		else
			m_backBuffer->draw(g, 0, 0, engine->getGraphics()->getResolution().x, engine->getGraphics()->getResolution().y);
		g->setBlending(true);
	}
}

void Osu::update()
{
	m_windowManager->update();

	for (int i=0; i<m_screens.size(); i++)
	{
		m_screens[i]->update();
	}

	// main beatmap update
	if (isInPlayMode())
	{
		getSelectedBeatmap()->update();

		if (engine->getKeyboard()->isControlDown() && engine->getKeyboard()->isAltDown() && engine->getMouse()->isLeftDown())
			getSelectedBeatmap()->seekPercent(clamp<float>(engine->getMouse()->getPos().x/getScreenWidth(), 0.0f, 0.99f));

		// skip button clicking
		if (getSelectedBeatmap()->isInSkippableSection() && !getSelectedBeatmap()->isPaused())
		{
			// TODO: make this on click only, and not if held too. this can also cause earrape while scrubbing
			if (engine->getMouse()->isLeftDown() || getSelectedBeatmap()->isClickHeld())
			{
				if (m_hud->getSkipClickRect().contains(engine->getMouse()->getPos()))
					m_bSkipScheduled = true;
			}
		}

		// skipping
		if (m_bSkipScheduled)
		{
			if (getSelectedBeatmap()->isInSkippableSection() && !getSelectedBeatmap()->isPaused())
				getSelectedBeatmap()->skipEmptySection();

			m_bSkipScheduled = false;
		}

		// quick retry timer
		if (m_bQuickRetryDown && m_fQuickRetryTime != 0.0f && engine->getTime() > m_fQuickRetryTime)
		{
			m_fQuickRetryTime = 0.0f;

			getSelectedBeatmap()->restart(true);
			getSelectedBeatmap()->update();
			m_pauseMenu->setVisible(false);
		}
	}

	// asynchronous ui toggles
	if (m_bToggleModSelectionScheduled)
	{
		m_bToggleModSelectionScheduled = false;

		if (!isInPlayMode())
		{
			if (m_songBrowser2 != NULL)
				m_songBrowser2->setVisible(m_modSelector->isVisible());
		}

		m_modSelector->setVisible(!m_modSelector->isVisible());
	}
	if (m_bToggleSongBrowserScheduled)
	{
		m_bToggleSongBrowserScheduled = false;

		if (m_songBrowser2 != NULL)
			m_songBrowser2->setVisible(!m_songBrowser2->isVisible());

		m_mainMenu->setVisible(!(m_songBrowser2 != NULL && m_songBrowser2->isVisible()));
		updateConfineCursor();
	}
	if (m_bToggleOptionsMenuScheduled)
	{
		m_bToggleOptionsMenuScheduled = false;

		m_optionsMenu->setVisible(!m_optionsMenu->isVisible());
		m_mainMenu->setVisible(!m_optionsMenu->isVisible());
	}
	if (m_bToggleRankingScreenScheduled)
	{
		m_bToggleRankingScreenScheduled = false;

		m_rankingScreen->setVisible(!m_rankingScreen->isVisible());
		if (m_songBrowser2 != NULL)
			m_songBrowser2->setVisible(!m_rankingScreen->isVisible());
	}

	// handle cursor visibility if outside of internal resolution
	// TODO: not a critical bug, but the cursor gets visible way too early if sensitivity is > 1.0f, due to this using scaled/offset getMouse()->getPos()
	if (osu_resolution_enabled.getBool())
	{
		Rect internalWindow = Rect(0, 0, g_vInternalResolution.x, g_vInternalResolution.y);
		bool cursorVisible = env->isCursorVisible();
		if (!internalWindow.contains(engine->getMouse()->getPos()))
		{
			if (!cursorVisible)
				env->setCursorVisible(true);
		}
		else
		{
			if (cursorVisible != m_bShouldCursorBeVisible)
				env->setCursorVisible(m_bShouldCursorBeVisible);
		}
	}

	// handle mousewheel volume change
	if ((m_songBrowser2 != NULL && (!m_songBrowser2->isVisible() || engine->getKeyboard()->isAltDown())) && !m_optionsMenu->isVisible())
	{
		if ((!(isInPlayMode() && !m_pauseMenu->isVisible()) && !m_rankingScreen->isVisible()) || (isInPlayMode() && !osu_disable_mousewheel.getBool()) || engine->getKeyboard()->isAltDown())
		{
			int wheelDelta = engine->getMouse()->getWheelDeltaVertical();
			if (wheelDelta != 0)
			{
				if (wheelDelta > 0)
					volumeUp();
				else
					volumeDown();
			}
		}
	}
}

void Osu::updateMods()
{
	m_bModAuto = osu_mods.getString().find("auto") != -1;
	m_bModAutopilot = osu_mods.getString().find("autopilot") != -1;
	m_bModRelax = osu_mods.getString().find("relax") != -1;
	m_bModSpunout = osu_mods.getString().find("spunout") != -1;
	m_bModTarget = osu_mods.getString().find("practicetarget") != -1;
	m_bModDT = osu_mods.getString().find("dt") != -1;
	m_bModNC = osu_mods.getString().find("nc") != -1;
	m_bModHT = osu_mods.getString().find("ht") != -1;
	m_bModHD = osu_mods.getString().find("hd") != -1;
	m_bModHR = osu_mods.getString().find("hr") != -1;
	m_bModEZ = osu_mods.getString().find("ez") != -1;
	m_bModSD = osu_mods.getString().find("sd") != -1;
	m_bModSS = osu_mods.getString().find("ss") != -1;
	m_bModNM = osu_mods.getString().find("nm") != -1;

	// static overrides
	onSpeedChange("", osu_speed_override.getString());
	onPitchChange("", osu_pitch_override.getString());

	// autopilot overrides auto
	if (m_bModAutopilot)
		m_bModAuto = false;

	// handle auto/pilot cursor visibility
	if (!m_bModAuto && !m_bModAutopilot)
	{
		env->setCursorVisible(false);
		m_bShouldCursorBeVisible = false;
	}
	else if (isInPlayMode())
	{
		env->setCursorVisible(true);
		m_bShouldCursorBeVisible = true;
	}

	// notify the possibly running beatmap of mod changes, for e.g. recalculating stacks dynamically if HR is toggled
	if (getSelectedBeatmap() != NULL)
		getSelectedBeatmap()->onUpdateMods();
}

void Osu::onKeyDown(KeyboardEvent &key)
{
	// global hotkeys

	// special hotkeys
	if (engine->getKeyboard()->isAltDown() && engine->getKeyboard()->isControlDown() && key == KEY_R)
	{
		if (OsuSliderRenderer::BLEND_SHADER != NULL)
			OsuSliderRenderer::BLEND_SHADER->recompile();
		key.consume();
	}
	else if (engine->getKeyboard()->isAltDown() && engine->getKeyboard()->isControlDown() && key == KEY_S)
	{
		onSkinReload();
		m_notificationOverlay->addNotification("Skin reloaded!");
		key.consume();
	}

	// fullscreen toggle
	if (engine->getKeyboard()->isAltDown() && key == KEY_ENTER)
	{
		engine->toggleFullscreen();
		key.consume();
	}

	// arrow keys volume (alt)
	if (engine->getKeyboard()->isAltDown() && key == (KEYCODE)OsuKeyBindings::INCREASE_VOLUME.getInt())
	{
		volumeUp();
		key.consume();
	}
	if (engine->getKeyboard()->isAltDown() && key == (KEYCODE)OsuKeyBindings::DECREASE_VOLUME.getInt())
	{
		volumeDown();
		key.consume();
	}

	// disable mouse buttons hotkey
	if (key == (KEYCODE)OsuKeyBindings::DISABLE_MOUSE_BUTTONS.getInt())
	{
		if (osu_disable_mousebuttons.getBool())
		{
			osu_disable_mousebuttons.setValue(0.0f);
			m_notificationOverlay->addNotification("Mouse buttons are enabled.");
		}
		else
		{
			osu_disable_mousebuttons.setValue(1.0f);
			m_notificationOverlay->addNotification("Mouse buttons are disabled.");
		}
	}

	// screenshots
	if (key == (KEYCODE)OsuKeyBindings::SAVE_SCREENSHOT.getInt())
		saveScreenshot();

	// local hotkeys

	// while playing
	if (isInPlayMode())
	{
		// while playing and not paused
		if (!getSelectedBeatmap()->isPaused())
		{
			if (!m_bKey1 && key == (KEYCODE)OsuKeyBindings::LEFT_CLICK.getInt())
			{
				m_bKey1 = true;
				onKey1Change(true, false);
				key.consume();
			}

			if (!m_bKey2 && key == (KEYCODE)OsuKeyBindings::RIGHT_CLICK.getInt())
			{
				m_bKey2 = true;
				onKey2Change(true, false);
				key.consume();
			}

			// handle skipping
			if (key == KEY_ENTER || key == (KEYCODE)OsuKeyBindings::SKIP_CUTSCENE.getInt())
				m_bSkipScheduled = true;

			// toggle ui
			if (key == KEY_SHIFT)
			{
				if (m_bTab && !m_bUIToggleCheck)
				{
					m_bUIToggleCheck = true;
					m_osu_draw_hud_ref->setValue(!m_osu_draw_hud_ref->getBool());

					key.consume();
				}
			}
			if (key == KEY_TAB)
			{
				m_bTab = true;
				if (engine->getKeyboard()->isShiftDown() && !m_bUIToggleCheck)
				{
					m_bUIToggleCheck = true;
					m_osu_draw_hud_ref->setValue(!m_osu_draw_hud_ref->getBool());

					key.consume();
				}
			}

			// allow live mod changing while playing
			if (!key.isConsumed() && key == KEY_F1 && !m_bF1)
			{
				m_bF1 = true;
				toggleModSelection(true);
			}
		}

		// while paused or maybe not paused

		// handle quick restart
		if (((key == (KEYCODE)OsuKeyBindings::QUICK_RETRY.getInt() || (engine->getKeyboard()->isControlDown() && !engine->getKeyboard()->isAltDown() && key == KEY_R)) && !m_bQuickRetryDown))
		{
			m_bQuickRetryDown = true;
			m_fQuickRetryTime = engine->getTime() + osu_quick_retry_delay.getFloat();
		}
	}

	// forward to all subsystem, if not already consumed
	for (int i=0; i<m_screens.size(); i++)
	{
		if (key.isConsumed())
			break;

		m_screens[i]->onKeyDown(key);
	}

	// special handling, after subsystems, if still not consumed
	if (!key.isConsumed())
	{
		// if playing
		if (isInPlayMode())
		{
			// toggle pause menu
			if ((key == (KEYCODE)OsuKeyBindings::GAME_PAUSE.getInt() || key == KEY_ESCAPE) && !m_bEscape)
			{
				m_bEscape = true;
				getSelectedBeatmap()->pause();
				m_pauseMenu->setVisible(getSelectedBeatmap()->isPaused());

				key.consume();
			}

			// arrow keys local offset
			if (key == (KEYCODE)OsuKeyBindings::INCREASE_LOCAL_OFFSET.getInt())
			{
				long offsetAdd = engine->getKeyboard()->isAltDown() ? 1 : 5;
				getSelectedBeatmap()->getSelectedDifficulty()->localoffset += offsetAdd;
				m_notificationOverlay->addNotification(UString::format("Local beatmap offset set to %ld ms", getSelectedBeatmap()->getSelectedDifficulty()->localoffset));
			}
			if (key == (KEYCODE)OsuKeyBindings::DECREASE_LOCAL_OFFSET.getInt())
			{
				long offsetAdd = -(engine->getKeyboard()->isAltDown() ? 1 : 5);
				getSelectedBeatmap()->getSelectedDifficulty()->localoffset += offsetAdd;
				m_notificationOverlay->addNotification(UString::format("Local beatmap offset set to %ld ms", getSelectedBeatmap()->getSelectedDifficulty()->localoffset));
			}
		}

		// if playing or not playing

		// arrow keys volume
		if (key == (KEYCODE)OsuKeyBindings::INCREASE_VOLUME.getInt())
			volumeUp();
		if (key == (KEYCODE)OsuKeyBindings::DECREASE_VOLUME.getInt())
			volumeDown();
	}
}

void Osu::onKeyUp(KeyboardEvent &key)
{
	// clicks
	if (key == (KEYCODE)OsuKeyBindings::LEFT_CLICK.getInt() && m_bKey1)
	{
		if (isInPlayMode())
			onKey1Change(false, false);

		m_bKey1 = false;
	}
	if (key == (KEYCODE)OsuKeyBindings::RIGHT_CLICK.getInt() && m_bKey2)
	{
		if (isInPlayMode())
			onKey2Change(false, false);

		m_bKey2 = false;
	}

	// forward to all subsystems, if not consumed
	for (int i=0; i<m_screens.size(); i++)
	{
		if (key.isConsumed())
			break;

		m_screens[i]->onKeyUp(key);
	}

	// misc hotkeys
	if (key == KEY_F1)
		m_bF1 = false;
	if (key == (KEYCODE)OsuKeyBindings::GAME_PAUSE.getInt() || key == KEY_ESCAPE)
		m_bEscape = false;
	if (key == KEY_SHIFT)
		m_bUIToggleCheck = false;
	if (key == KEY_TAB)
	{
		m_bTab = false;
		m_bUIToggleCheck = false;
	}
	if (key == (KEYCODE)OsuKeyBindings::QUICK_RETRY.getInt() || key == KEY_R)
		m_bQuickRetryDown = false;
}

void Osu::onChar(KeyboardEvent &e)
{
	for (int i=0; i<m_screens.size(); i++)
	{
		if (e.isConsumed())
			break;

		m_screens[i]->onChar(e);
	}
}

void Osu::onLeftChange(bool down)
{
	if (isInPlayMode() && !getSelectedBeatmap()->isPaused() && osu_disable_mousebuttons.getBool())
		return;

	if (!m_bKey1 && down)
	{
		m_bKey1 = true;
		onKey1Change(true, true);
	}
	else if (m_bKey1)
	{
		m_bKey1 = false;
		onKey1Change(false, true);
	}
}

void Osu::onRightChange(bool down)
{
	if (isInPlayMode() && !getSelectedBeatmap()->isPaused() && osu_disable_mousebuttons.getBool())
		return;

	if (!m_bKey2 && down)
	{
		m_bKey2 = true;
		onKey2Change(true, true);
	}
	else if (m_bKey2)
	{
		m_bKey2 = false;
		onKey2Change(false, true);
	}
}

void Osu::toggleModSelection(bool waitForF1KeyUp)
{
	m_bToggleModSelectionScheduled = true;
	m_modSelector->setWaitForF1KeyUp(waitForF1KeyUp);
}

void Osu::toggleSongBrowser()
{
	m_bToggleSongBrowserScheduled = true;
}

void Osu::toggleOptionsMenu()
{
	m_bToggleOptionsMenuScheduled = true;
}

void Osu::toggleRankingScreen()
{
	m_bToggleRankingScreenScheduled = true;
}

void Osu::volumeDown()
{
	float newVolume = clamp<float>(osu_volume_master.getFloat() - osu_volume_change_interval.getFloat(), 0.0f, 1.0f);
	osu_volume_master.setValue(newVolume);
	m_hud->animateVolumeChange();
}

void Osu::volumeUp()
{
	float newVolume = clamp<float>(osu_volume_master.getFloat() + osu_volume_change_interval.getFloat(), 0.0f, 1.0f);
	osu_volume_master.setValue(newVolume);
	m_hud->animateVolumeChange();
}

void Osu::saveScreenshot()
{
	engine->getSound()->play(m_skin->getShutter());
	int screenshotNumber = 0;
	while (env->fileExists(UString::format("screenshots/screenshot%i.png", screenshotNumber)))
	{
		screenshotNumber++;
	}
	std::vector<unsigned char> pixels = engine->getGraphics()->getScreenshot();
	Image::saveToImage(&pixels[0], engine->getGraphics()->getResolution().x, engine->getGraphics()->getResolution().y, UString::format("screenshots/screenshot%i.png", screenshotNumber));
}



void Osu::onBeforePlayStart()
{
	debugLog("Osu::onBeforePlayStart()\n");

	engine->getSound()->play(m_skin->getMenuHit());

	updateMods();
}

void Osu::onPlayStart()
{
	debugLog("Osu::onPlayStart()\n");

	if (m_bModAuto || m_bModAutopilot)
	{
		env->setCursorVisible(true);
		m_bShouldCursorBeVisible = true;
	}

	if (getSelectedBeatmap()->getSelectedDifficulty()->localoffset != 0)
		m_notificationOverlay->addNotification(UString::format("Using local beatmap offset (%ld ms)", getSelectedBeatmap()->getSelectedDifficulty()->localoffset));

	updateConfineCursor();
}

void Osu::onPlayEnd(bool quit)
{
	debugLog("Osu::onPlayEnd()\n");

	if (!quit)
	{
		m_rankingScreen->setScore(m_score);
		m_rankingScreen->setBeatmapInfo(getSelectedBeatmap(), getSelectedBeatmap()->getSelectedDifficulty());
		engine->getSound()->play(m_skin->getApplause());
	}

	m_mainMenu->setVisible(false);
	m_modSelector->setVisible(false);
	m_pauseMenu->setVisible(false);

	env->setCursorVisible(false);
	m_bShouldCursorBeVisible = false;

	if (m_songBrowser2 != NULL)
		m_songBrowser2->onPlayEnd(quit);

	if (quit)
		toggleSongBrowser();
	else
		toggleRankingScreen();

	updateConfineCursor();
}


OsuBeatmap *Osu::getSelectedBeatmap()
{
	if (m_songBrowser2 != NULL)
		return m_songBrowser2->getSelectedBeatmap();
	return NULL;
}

float Osu::getDifficultyMultiplier()
{
	float difficultyMultiplier = 1.0f;

	if (m_bModHR)
		difficultyMultiplier = 1.4f;
	if (m_bModEZ)
		difficultyMultiplier = 0.5f;

	return difficultyMultiplier;
}

float Osu::getCSDifficultyMultiplier()
{
	float difficultyMultiplier = 1.0f;

	if (m_bModHR)
		difficultyMultiplier = 1.3f; // different!
	if (m_bModEZ)
		difficultyMultiplier = 0.5f;

	return difficultyMultiplier;
}

float Osu::getScoreMultiplier()
{
	float multiplier = 1.0f;

	if (m_bModEZ)
		multiplier *= 0.5f;
	if (m_bModHT)
		multiplier *= 0.3f;
	if (m_bModHR)
		multiplier *= 1.06f;
	if (m_bModDT || m_bModNC)
		multiplier *= 1.12f;
	if (m_bModHD)
		multiplier *= 1.06f;
	if (m_bModSpunout)
		multiplier *= 0.9f;

	return multiplier;
}

float Osu::getRawSpeedMultiplier()
{
	float speedMultiplier = 1.0f;

	if (m_bModDT || m_bModNC || m_bModHT)
	{
		if (m_bModDT || m_bModNC)
			speedMultiplier = 1.5f;
		else
			speedMultiplier = 0.75f;
	}

	return speedMultiplier;
}

float Osu::getSpeedMultiplier()
{
	float speedMultiplier = getRawSpeedMultiplier();

	if (osu_speed_override.getFloat() > 0.0f)
		return osu_speed_override.getFloat();

	return speedMultiplier;
}

float Osu::getPitchMultiplier()
{
	float pitchMultiplier = 1.0f;

	if (m_bModNC)
		pitchMultiplier = 1.1166f;

	if (osu_pitch_override.getFloat() > 0.0f)
		return osu_pitch_override.getFloat();

	return pitchMultiplier;
}

bool Osu::isInPlayMode()
{
	return (m_songBrowser2 != NULL && m_songBrowser2->hasSelectedAndIsPlaying());
}



void Osu::onResolutionChanged(Vector2 newResolution)
{
	if (!osu_resolution_enabled.getBool())
		g_vInternalResolution = newResolution;
	else
	{
		// clamp internal resolution to actual resolution

		// unclamp if new size is bigger again
		if (g_vInternalResolution.x < m_vInternalResolution.x)
			g_vInternalResolution.x = m_vInternalResolution.x;
		if (g_vInternalResolution.y < m_vInternalResolution.y)
			g_vInternalResolution.y = m_vInternalResolution.y;

		if (newResolution.x < g_vInternalResolution.x)
			g_vInternalResolution.x = newResolution.x;
		if (newResolution.y < g_vInternalResolution.y)
			g_vInternalResolution.y = newResolution.y;

		// disable internal resolution on specific conditions
		if (g_vInternalResolution == engine->getScreenSize() || !env->isFullscreen())
		{
			debugLog("Internal resolution == Engine resolution || !Fullscreen, disabling resampler\n");
			osu_resolution_enabled.setValue(0.0f);
			g_vInternalResolution = engine->getScreenSize();
		}
	}

	// interfaces
	for (int i=0; i<m_screens.size(); i++)
	{
		m_screens[i]->onResolutionChange(g_vInternalResolution);
	}

	// rendertargets
	m_frameBuffer->rebuild(0, 0, g_vInternalResolution.x, g_vInternalResolution.y);
	m_backBuffer->rebuild(0, 0, g_vInternalResolution.x, g_vInternalResolution.y);

	// mouse scaling & offset
	// TODO: rethink scale logic
	if (osu_resolution_enabled.getBool())
	{
		if (osu_letterboxing.getBool())
		{
			engine->getMouse()->setOffset(-Vector2(engine->getScreenWidth()/2 - g_vInternalResolution.x/2, engine->getScreenHeight()/2 - g_vInternalResolution.y/2));
			engine->getMouse()->setScale(Vector2(1,1));
		}
		else
		{
			engine->getMouse()->setOffset(Vector2(0,0));
			engine->getMouse()->setScale(Vector2(1,1));
		}
	}
	else
	{
		engine->getMouse()->setOffset(Vector2(0,0));
		engine->getMouse()->setScale(Vector2(1,1));
	}

	// cursor clipping
	updateConfineCursor();
}

void Osu::onInternalResolutionChanged(UString oldValue, UString args)
{
	if (args.length() < 7)
		return;

	std::vector<UString> resolution = args.split("x");
	if (resolution.size() != 2)
		debugLog("Error: Invalid parameter count for command 'osu_resolution'! (Usage: e.g. \"osu_resolution 1280x720\")");
	else
	{
		int width = resolution[0].toFloat();
		int height = resolution[1].toFloat();

		if (width < 300 || height < 240)
			debugLog("Error: Invalid values for resolution for command 'osu_resolution'!");
		else
		{
			Vector2 newInternalResolution = Vector2(width, height);

			if (newInternalResolution.x > engine->getGraphics()->getResolution().x)
				newInternalResolution.x = engine->getGraphics()->getResolution().x;
			if (newInternalResolution.y > engine->getGraphics()->getResolution().y)
				newInternalResolution.y = engine->getGraphics()->getResolution().y;

			// enable and store, then force onResolutionChanged()
			osu_resolution_enabled.setValue(1.0f);
			g_vInternalResolution = newInternalResolution;
			m_vInternalResolution = newInternalResolution;
			onResolutionChanged(newInternalResolution);
		}
	}
}

void Osu::onFocusGained()
{
	// cursor clipping
	updateConfineCursor();
}

void Osu::onFocusLost()
{
	if (isInPlayMode() && !getSelectedBeatmap()->isPaused() && osu_pause_on_focus_loss.getBool())
	{
		getSelectedBeatmap()->pause(false);
		m_pauseMenu->setVisible(true);
		m_modSelector->setVisible(false);
	}

	// release cursor clip
	env->setCursorClip(false, Rect());
}

void Osu::onSkinReload()
{
	onSkinChange("", osu_skin.getString());
}

void Osu::onSkinChange(UString oldValue, UString newValue)
{
	if (newValue.length() > 1)
	{
		SAFE_DELETE(m_skin);

		UString skinFolder = m_osu_folder_ref->getString();
		skinFolder.append("Skins/");
		skinFolder.append(newValue);
		skinFolder.append("/");
		m_skin = new OsuSkin(this, skinFolder);

		// force
		onResolutionChanged(engine->getScreenSize());
	}
}

void Osu::onMasterVolumeChange(UString oldValue, UString newValue)
{
	float newVolume = newValue.toFloat();
	engine->getSound()->setVolume(newVolume);
}

void Osu::onMusicVolumeChange(UString oldValue, UString newValue)
{
	if (getSelectedBeatmap() != NULL)
		getSelectedBeatmap()->setVolume(newValue.toFloat());
}

void Osu::onSpeedChange(UString oldValue, UString newValue)
{
	if (getSelectedBeatmap() != NULL)
	{
		float speed = newValue.toFloat();
		getSelectedBeatmap()->setSpeed(speed > 0.0f ? speed : getSpeedMultiplier());
	}
}

void Osu::onPitchChange(UString oldValue, UString newValue)
{
	if (getSelectedBeatmap() != NULL)
	{
		float pitch = newValue.toFloat();
		getSelectedBeatmap()->setPitch(pitch > 0.0f ? pitch : getPitchMultiplier());
	}
}

void Osu::onLetterboxingChange(UString oldValue, UString newValue)
{
	if (osu_resolution_enabled.getBool())
	{
		bool oldVal = oldValue.toFloat() > 0.0f;
		bool newVal = newValue.toFloat() > 0.0f;

		if (oldVal != newVal)
			Osu::onResolutionChanged(g_vInternalResolution);
	}
}

void Osu::updateConfineCursor()
{
	if ((osu_confine_cursor_fullscreen.getBool() && env->isFullscreen()) || (osu_confine_cursor_windowed.getBool() && !env->isFullscreen()) || (isInPlayMode() && !getSelectedBeatmap()->isPaused() && !getModAuto() && !getModAutopilot()))
		env->setCursorClip(true, Rect());
	else
		env->setCursorClip(false, Rect());
}

void Osu::onConfineCursorWindowedChange(UString oldValue, UString newValue)
{
	updateConfineCursor();
}

void Osu::onConfineCursorFullscreenChange(UString oldValue, UString newValue)
{
	updateConfineCursor();
}

void Osu::onKey1Change(bool pressed, bool mouse)
{
	if (isInPlayMode() && !getSelectedBeatmap()->isPaused())
	{
		if (!(mouse && osu_disable_mousebuttons.getBool()))
		{
			if (pressed)
				getSelectedBeatmap()->keyPressed1();
			else
				getSelectedBeatmap()->keyReleased1();
		}
	}

	bool doAnimate = !(isInPlayMode() && !getSelectedBeatmap()->isPaused() && mouse && osu_disable_mousebuttons.getBool());

	if (doAnimate)
	{
		if (pressed)
			m_hud->animateCursorExpand();
		else if (!pressed && !m_bKey2)
			m_hud->animateCursorShrink();
	}
}

void Osu::onKey2Change(bool pressed, bool mouse)
{
	if (isInPlayMode() && !getSelectedBeatmap()->isPaused())
	{
		if (!(mouse && osu_disable_mousebuttons.getBool()))
		{
			if (pressed)
				getSelectedBeatmap()->keyPressed2();
			else
				getSelectedBeatmap()->keyReleased2();
		}
	}

	bool doAnimate = !(isInPlayMode() && !getSelectedBeatmap()->isPaused() && mouse && osu_disable_mousebuttons.getBool());

	if (doAnimate)
	{
		if (pressed)
			m_hud->animateCursorExpand();
		else if (!pressed && !m_bKey1)
			m_hud->animateCursorShrink();
	}
}



float Osu::getImageScaleToFitResolution(Vector2 size, Vector2 resolution)
{
	return resolution.x/size.x > resolution.y/size.y ? resolution.y/size.y : resolution.x/size.x;
}

float Osu::getImageScaleToFitResolution(Image *img, Vector2 resolution)
{
	return getImageScaleToFitResolution(Vector2(img->getWidth(), img->getHeight()), resolution);
}

float Osu::getImageScaleToFillResolution(Vector2 size, Vector2 resolution)
{
	return resolution.x/size.x < resolution.y/size.y ? resolution.y/size.y : resolution.x/size.x;
}

float Osu::getImageScaleToFillResolution(Image *img, Vector2 resolution)
{
	return getImageScaleToFillResolution(Vector2(img->getWidth(), img->getHeight()), resolution);
}

float Osu::getImageScale(Osu *osu, Vector2 size, float osuSize)
{
	int swidth = osu->getScreenWidth();
	int sheight = osu->getScreenHeight();

	if (swidth * 3 > sheight * 4)
		swidth = sheight * 4 / 3;
	else
		sheight = swidth * 3 / 4;

	const float xMultiplier = swidth / osuBaseResolution.x;
	const float yMultiplier = sheight / osuBaseResolution.y;

	const float xDiameter = osuSize*xMultiplier;
	const float yDiameter = osuSize*yMultiplier;

	return xDiameter/size.x > yDiameter/size.y ? xDiameter/size.x : yDiameter/size.y;
}

float Osu::getImageScale(Osu *osu, Image *img, float osuSize)
{
	return getImageScale(osu, Vector2(img->getWidth(), img->getHeight()), osuSize);
}

float Osu::getUIScale(Osu *osu, float osuResolutionRatio)
{
	int swidth = osu->getScreenWidth();
	int sheight = osu->getScreenHeight();

	if (swidth * 3 > sheight * 4)
		swidth = sheight * 4 / 3;
	else
		sheight = swidth * 3 / 4;

	const float xMultiplier = swidth / osuBaseResolution.x;
	const float yMultiplier = sheight / osuBaseResolution.y;

	const float xDiameter = osuResolutionRatio*xMultiplier;
	const float yDiameter = osuResolutionRatio*yMultiplier;

	return xDiameter > yDiameter ? xDiameter : yDiameter;
}

bool Osu::findIgnoreCase(const std::string &haystack, const std::string &needle)
{
	auto it = std::search(
	    haystack.begin(), haystack.end(),
		needle.begin(),   needle.end(),
	    [](char ch1, char ch2)
		{return std::tolower(ch1) == std::tolower(ch2);}
	);

	return (it != haystack.end());
}
