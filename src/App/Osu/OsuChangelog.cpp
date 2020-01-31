//================ Copyright (c) 2017, PG, All rights reserved. =================//
//
// Purpose:		changelog screen
//
// $NoKeywords: $osulog
//===============================================================================//

#include "OsuChangelog.h"

#include "Engine.h"
#include "ResourceManager.h"
#include "SoundEngine.h"
#include "Mouse.h"

#include "CBaseUIContainer.h"
#include "CBaseUIScrollView.h"
#include "CBaseUILabel.h"

#include "Osu.h"
#include "OsuSkin.h"
#include "OsuOptionsMenu.h"
#include "OsuHUD.h"

OsuChangelog::OsuChangelog(Osu *osu) : OsuScreenBackable(osu)
{
	m_container = new CBaseUIContainer(-1, -1, 0, 0, "");
	m_scrollView = new CBaseUIScrollView(-1, -1, 0, 0, "");
	m_scrollView->setVerticalScrolling(true);
	m_scrollView->setHorizontalScrolling(true);
	m_scrollView->setDrawBackground(false);
	m_scrollView->setDrawFrame(false);
	m_scrollView->setScrollResistance(0);
	m_container->addBaseUIElement(m_scrollView);

	std::vector<CHANGELOG> changelogs;

	CHANGELOG alpha313x;
	alpha313x.title = UString::format("30.13 (3) (Build Date: %s, %s)", __DATE__, __TIME__);
	alpha313x.changes.push_back("- Added \"Toggle Mod Selection Screen\" keybind (Options > Input > Keyboard > Keys - Song Select)");
	alpha313x.changes.push_back("- Added \"Random Beatmap\" keybind (Options > Input > Keyboard > Keys - Song Select)");
	alpha313x.changes.push_back("- Added ConVars (1): osu_hud_hiterrorbar_alpha, osu_hud_hiterrorbar_bar_alpha, osu_hud_hiterrorbar_centerline_alpha");
	alpha313x.changes.push_back("- Added ConVars (2): osu_hud_hiterrorbar_entry_alpha, osu_hud_hiterrorbar_entry_300/100/50/miss_r/g/b");
	alpha313x.changes.push_back("- Added ConVars (3): osu_hud_hiterrorbar_centerline_r/g/b, osu_hud_hiterrorbar_max_entries");
	alpha313x.changes.push_back("- Added ConVars (4): osu_hud_hiterrorbar_entry_hit/miss_fade_time, osu_hud_hiterrorbar_offset_percent");
	alpha313x.changes.push_back("- Added ConVars (5): osu_draw_hiterrorbar_bottom/top/left/right");
	alpha313x.changes.push_back("- Moved hiterrorbar behind hitobjects");
	alpha313x.changes.push_back("- Updated SHIFT + TAB and SHIFT scoreboard toggle behavior");
	alpha313x.changes.push_back("- Fixed kinetic tablet scrolling at very high framerates (> ~600 fps)");
	alpha313x.changes.push_back("- Fixed ranking screen layout partially for weird skins (long grade overflow)");
	alpha313x.changes.push_back("- Fixed enabling \"Ignore Beatmap Sample Volume\" not immediately updating sample volume");
	alpha313x.changes.push_back("- Fixed stale context menu in top ranks screen potentially allowing random score deletion if clicked");
	changelogs.push_back(alpha313x);

	CHANGELOG alpha313;
	alpha313.title = "30.13 (12.01.2020)";
	alpha313.changes.push_back("- Added searching by beatmap ID + beatmap set ID");
	alpha313.changes.push_back("- Added CTRL + V support to songbrowser search (paste clipboard)");
	alpha313.changes.push_back("- Added speed display to score buttons");
	alpha313.changes.push_back("- Added support for sliderslide sound");
	alpha313.changes.push_back("- Added Touch Device mod (allows simulating pp nerf)");
	alpha313.changes.push_back("- Added option \"Always enable touch device pp nerf mod\" (Options > General > Player)");
	alpha313.changes.push_back("- Added option \"Apply speed/pitch mods while browsing\" (Options > Audio > Songbrowser)");
	alpha313.changes.push_back("- Added option \"Draw Stats: 300 hitwindow\" (Options > Gameplay > HUD)");
	alpha313.changes.push_back("- Added option \"Draw Stats: Accuracy Error\" (Options > Gameplay > HUD)");
	alpha313.changes.push_back("- Added option \"Show Skip Button during Intro\" (Options > Gameplay > General)");
	alpha313.changes.push_back("- Added option \"Show Skip Button during Breaks\" (Options > Gameplay > General)");
	alpha313.changes.push_back("- Added ConVars (1): osu_followpoints_separation_multiplier, osu_songbrowser_search_delay");
	alpha313.changes.push_back("- Added ConVars (2): osu_slider_body_fade_out_time_multiplier, osu_beatmap_preview_music_loop");
	alpha313.changes.push_back("- Added ConVars (3): osu_skin_export, osu_hud_statistics_hitdelta_chunksize");
	alpha313.changes.push_back("- Windows: Added WASAPI option \"Period Size\" (Options > Audio > WASAPI) (wasapi-test beta)");
	alpha313.changes.push_back("- Allow overscaling osu_slider_body_alpha_multiplier/color_saturation, osu_cursor_scale, fposu_distance");
	alpha313.changes.push_back("- Improved engine background async loading (please report crashes)");
	alpha313.changes.push_back("- Loop music");
	alpha313.changes.push_back("- Fixed skin hit0/hit50/hit100/hit300 animation handling (keep last frame and fade)");
	alpha313.changes.push_back("- Fixed scrubbing during lead-in time breaking things");
	alpha313.changes.push_back("- Fixed right click scrolling in songbrowser stalling if cursor goes outside container");
	alpha313.changes.push_back("- Windows: Fixed Windows key not unlocking on focus loss if \"Pause on Focus Loss\" is disabled");
	changelogs.push_back(alpha313);

	CHANGELOG alpha312;
	alpha312.title = "30.12 (19.12.2019)";
	alpha312.changes.push_back("- Added button \"Random Skin\" (Options > Skin)");
	alpha312.changes.push_back("- Added option \"SHIFT + TAB toggles everything\" (Options > Gameplay > HUD)");
	alpha312.changes.push_back("- Added ConVars (1): osu_mod_random_circle/slider/spinner_offset_x/y_percent, osu_mod_hd_circle_fadein/fadeout_start/end_percent");
	alpha312.changes.push_back("- Added ConVars (2): osu_play_hitsound_on_click_while_playing, osu_alt_f4_quits_even_while_playing");
	alpha312.changes.push_back("- Added ConVars (3): osu_skin_random, osu_skin_random_elements, osu_slider_body_unit_circle_subdivisions");
	alpha312.changes.push_back("- Windows: Ignore Windows key while playing (osu_win_disable_windows_key_while_playing)");
	alpha312.changes.push_back("- Made skip button only skip if click started inside");
	alpha312.changes.push_back("- Made mod \"Jigsaw\" allow clicks during breaks and before first hitobject");
	alpha312.changes.push_back("- Made experimental mod \"Full Alternate\" allow any key for first hitobjects, and after break, and during/after spinners");
	alpha312.changes.push_back("- Improved Steam Workshop subscribed items refresh speed");
	alpha312.changes.push_back("- Fixed grade image on songbuttons ignoring score sorting setting");
	alpha312.changes.push_back("- Fixed notelock unlocking sliders too early (previously unlocked after sliderstartcircle, now unlocks after slider end)");
	alpha312.changes.push_back("- Fixed rare hitsound timingpoint offsets (accurate on slider start/end now)");
	alpha312.changes.push_back("- Fixed NaN timingpoint handling for aspire (maybe)");
	changelogs.push_back(alpha312);

	CHANGELOG alpha311;
	alpha311.title = "30.11 (07.11.2019)";
	alpha311.changes.push_back("- Improved osu collection.db loading speed");
	alpha311.changes.push_back("- Fixed new osu database format breaking loading");
	alpha311.changes.push_back("- Added upper osu database version loading limit");
	alpha311.changes.push_back("- Added \"Sort By Misses\" to score sorting options");
	alpha311.changes.push_back("- Added ConVars: osu_rich_presence_dynamic_windowtitle, osu_database_ignore_version");
	alpha311.changes.push_back("- FPoSu: Fixed disabling \"Show FPS Counter\" not working (was always shown)");
	alpha311.changes.push_back("- Fixed rare custom manual ConVars getting removed from osu.cfg");
	changelogs.push_back(alpha311);

	CHANGELOG alpha301;
	alpha301.title = "30.1 (13.09.2019)";
	alpha301.changes.push_back("- Added proper support for HiDPI displays (scaling)");
	alpha301.changes.push_back("- Added option \"UI Scale\" (Options > Graphics > UI Scaling)");
	alpha301.changes.push_back("- Added option \"DPI Scaling\" (Options > Graphics > UI Scaling)");
	alpha301.changes.push_back("- Added context menu for deleting scores in \"Top Ranks\" screen");
	alpha301.changes.push_back("- Added sorting options for local scores (sort by pp, accuracy, combo, date)");
	alpha301.changes.push_back("- FPoSu: Added option \"Vertical FOV\" (Options > FPoSu > General)");
	alpha301.changes.push_back("- Draw breaks in scrubbing timeline");
	alpha301.changes.push_back("- Made scrubbing smoother by only seeking if the cursor position actually changed");
	alpha301.changes.push_back("- Windows: Added option \"High Priority\" (Options > Graphics > Renderer)");
	alpha301.changes.push_back("- Windows: Allow windowed resolutions to overshoot window borders (offscreen)");
	alpha301.changes.push_back("- Added ConVars: osu_followpoints_connect_combos, osu_scrubbing_smooth");
	alpha301.changes.push_back("- VR: Removed LIV support (for now)");
	alpha301.changes.push_back("- Allow loading incorrect skin.ini \"[General]\" section props before section start");
	alpha301.changes.push_back("- FPoSu: Fixed rare pause menu button jitter/unclickable");
	alpha301.changes.push_back("- Windows: Fixed toggling fullscreen sometimes causing weird windowed resolutions");
	alpha301.changes.push_back("- Windows: Fixed letterboxed \"Map Absolute Raw Input to Window\" offsets not matching osu!");
	changelogs.push_back(alpha301);

	CHANGELOG alpha30;
	alpha30.title =  "30.0 (13.06.2019)";
	alpha30.changes.push_back("- Added Steam Workshop support (for skins)");
	alpha30.changes.push_back("- Added option \"Cursor ripples\" (Options > Input > Mouse)");
	alpha30.changes.push_back("- Added skinning support for menu-background and cursor-ripple");
	alpha30.changes.push_back("- Added support for using custom BeatmapDirectory even without an osu installation/database");
	alpha30.changes.push_back("- Added ConVars: osu_cursor_ripple_duration/alpha/additive/anim_start_scale/end/fadeout_delay/tint_r/g/b");
	alpha30.changes.push_back("- General engine stability improvements");
	alpha30.changes.push_back("- Fixed AR/OD lock buttons being ignored by Timewarp experimental mod");
	alpha30.changes.push_back("- Fixed custom ConVars being ignored in cfg: osu_mods, osu_speed/ar/od/cs_override");
	changelogs.push_back(alpha30);

	CHANGELOG alpha295;
	alpha295.title =  "29.5 (26.05.2019)";
	alpha295.changes.push_back("- Added lock buttons to AR/OD override sliders (force constant AR/OD even with speed multiplier)");
	alpha295.changes.push_back("- Added reset buttons to all options settings");
	alpha295.changes.push_back("- Added ConVars: osu_slider_reverse_arrow_alpha_multiplier, snd_speed_compensate_pitch");
	alpha295.changes.push_back("- Fixed BPM statistics overlay only applying speed multiplier after music is loaded");
	alpha295.changes.push_back("- Windows: Fixed random stuck cursor on engine startup if launched in background (invalid focus)");
	changelogs.push_back(alpha295);

	CHANGELOG alpha294;
	alpha294.title =  "29.4 (14.03.2019)";
	alpha294.changes.push_back("- Merged FPoSu (Options > FPoSu)");
	alpha294.changes.push_back("- FPoSu is a real 3D first person gamemode, contrary to the 2D experimental mod \"First Person\"");
	alpha294.changes.push_back("- Thanks to Colin Brook (aka SnakeModule on GitHub)");
	alpha294.changes.push_back("- FPoSu: Moved \"Playfield Edge Distance\" from Mod Selector to Options > FPoSu > \"Distance\"");
	alpha294.changes.push_back("- FPoSu: Made mouse movement handling independent from regular osu sensitivity settings");
	alpha294.changes.push_back("- FPoSu: Made backgroundcube.png skinnable");
	alpha294.changes.push_back("- FPoSu: Added tablet support (Options > FPoSu > \"Tablet/Absolute Mode\")");
	alpha294.changes.push_back("- FPoSu: Added auto/pilot support");
	alpha294.changes.push_back("- FPoSu: Added ConVars: fposu_cube_tint_r, fposu_cube_tint_g, fposu_cube_tint_b");
	alpha294.changes.push_back("- FPoSu: Added letterboxing support");
	alpha294.changes.push_back("- FPoSu: Fixed mouse position getting set while engine is in background");
	alpha294.changes.push_back("- Added support for searching in collections");
	alpha294.changes.push_back("- Added support for changing grouping/sorting while in active search");
	alpha294.changes.push_back("- Added ConVar: osu_hud_statistics_pp_decimal_places");
	alpha294.changes.push_back("- Fixed boss key not pausing music");
	alpha294.changes.push_back("- Fixed another ArithmeticException in main menu (Aspire, Acid Rain - Covetous Beaver)");
	changelogs.push_back(alpha294);

	CHANGELOG alpha293;
	alpha293.title = "29.3 (15.02.2019)";
	alpha293.changes.push_back("- NOTE: New stars/pp are accurate with an average delta of ~0.1% or ~0.003 stars, except for very few Aspire/2B maps with ~15%");
	alpha293.changes.push_back("- Updated star algorithm to respect slider curves/repeats/ticks/tails and stacking (7)");
	alpha293.changes.push_back("- Updated star algorithm (Xexxar) (6), see https://osu.ppy.sh/home/news/2019-02-05-new-changes-to-star-rating-performance-points");
	alpha293.changes.push_back("- Updated pp algorithm (5), see https://github.com/ppy/osu-performance/pull/74/");
	alpha293.changes.push_back("- Updated beatmap parser to allow bullshit sliders (e.g. Aleph-0)");
	alpha293.changes.push_back("- Updated search behaviour to additively match words separated by spaces, instead of the entire phrase");
	alpha293.changes.push_back("- Added option \"Keep Aspect Ratio\" (Options > Graphics > Layout)");
	alpha293.changes.push_back("- Added hotkey: CTRL + Click to play with auto");
	alpha293.changes.push_back("- Added hotkey: CTRL + ENTER to play with auto");
	alpha293.changes.push_back("- Added hotkey: CTRL + A to toggle auto in songbrowser");
	alpha293.changes.push_back("- Added unbind buttons to keybinds");
	alpha293.changes.push_back("- Show <artist> - <name> [<diff>] in window title while playing");
	alpha293.changes.push_back("- Draw up to 10 background stars on song diff buttons");
	alpha293.changes.push_back("- Default to 0pp in ranking screen for incomplete scores due to scrubbing");
	alpha293.changes.push_back("- Windows: Fixed touchscreen handling being broken");
	alpha293.changes.push_back("- Fixed rare override slider reset crash");
	alpha293.changes.push_back("- Fixed lv only counting top pp scores");
	alpha293.changes.push_back("- Fixed top rank score button blinking animation not resetting");
	alpha293.changes.push_back("- Fixed top left info label in songbrowser not updating 0 stars after slow calculation finishes");
	alpha293.changes.push_back("- Fixed star calculation not prioritizing active selected beatmap if background image loading is disabled");
	alpha293.changes.push_back("- Fixed potential crashes and data mangling due to race conditions for slow background image/star loading");
	alpha293.changes.push_back("- Fixed changing audio output device breaking default skin sounds");
	alpha293.changes.push_back("- Fixed search not working as expected after behaviour change");
	alpha293.changes.push_back("- Added ConVars: osu_stars_xexxar_angles_sliders, osu_stars_stacking, osu_ui_top_ranks_max");
	changelogs.push_back(alpha293);

	/*
	CHANGELOG alpha292;
	alpha292.title = "29.2 (06.01.2019)";
	alpha292.changes.push_back("- Added \"Top Ranks\"/\"Best Performance\" screen showing all weighted scores (Songbrowser > User > Top Ranks)");
	alpha292.changes.push_back("- Added option \"Include Relax/Autopilot for total weighted pp/acc\" (Options > General > Player (Name))");
	alpha292.changes.push_back("- Added option \"Show pp instead of score in scorebrowser\" (Options > General > Player (Name))");
	alpha292.changes.push_back("- Added option \"SuddenDeath restart on miss\" (Options > Gameplay > General)");
	alpha292.changes.push_back("- Added ConVars: osu_relax_offset, osu_user_draw_pp, osu_user_draw_accuracy, osu_user_draw_level, osu_user_draw_level_bar");
	alpha292.changes.push_back("- Scale top left info label in songbrowser to screen resolution");
	alpha292.changes.push_back("- McOsu scores will now show pp instead of score by default (Options > General > Player (Name))");
	alpha292.changes.push_back("- Updated pp algorithm (4), see https://github.com/ppy/osu-performance/pull/76/");
	alpha292.changes.push_back("- Updated pp algorithm (3), see https://github.com/ppy/osu-performance/pull/72/");
	alpha292.changes.push_back("- Fixed total weighted pp counting multiple scores on the same diff");
	changelogs.push_back(alpha292);

	CHANGELOG alpha291;
	alpha291.title = "29.1 (31.12.2018)";
	alpha291.changes.push_back("- Added rich presence support (Discord + Steam)");
	alpha291.changes.push_back("- Added user profile info + switcher to songbrowser (total weighted pp/acc/lv)");
	alpha291.changes.push_back("- Added option \"Rich Presence\" (Options > Online > Integration)");
	alpha291.changes.push_back("- Added option \"Automatic Cursor Size\" (Options > Skin > Skin)");
	alpha291.changes.push_back("- Added letterboxing option \"Horizontal position\" (Options > Graphics > Layout)");
	alpha291.changes.push_back("- Added letterboxing option \"Vertical position\" (Options > Graphics > Layout)");
	alpha291.changes.push_back("- Added positional audio for hitsounds");
	alpha291.changes.push_back("- Added Score V2 keybind (defaults to 'B')");
	alpha291.changes.push_back("- Added new experimental mod \"Reverse Sliders\"");
	alpha291.changes.push_back("- Added mouse sidebutton support (mouse4, mouse5)");
	alpha291.changes.push_back("- Added detail info tooltip (approach time, hit timings, etc.) when hovering over diff info label in songbrowser (CS AR etc.)");
	alpha291.changes.push_back("- Added ConVar: osu_slider_end_inside_check_offset");
	alpha291.changes.push_back("- Added ConVars: osu_sound_panning, osu_sound_panning_multiplier, osu_approachtime_min, osu_approachtime_mid, osu_approachtime_max");
	alpha291.changes.push_back("- Added ConVars: osu_songbrowser_thumbnail_fade_in_duration, osu_songbrowser_background_fade_in_duration, osu_background_fade_after_load");
	alpha291.changes.push_back("- Added key overlay");
	alpha291.changes.push_back("- Updated combo color handling to match osu!");
	alpha291.changes.push_back("- Fade in songbrowser thumbnails");
	alpha291.changes.push_back("- Fade in songbrowser background");
	alpha291.changes.push_back("- Fade out background after load");
	alpha291.changes.push_back("- Updated diff info label in songbrowser to respect mods/overrides");
	alpha291.changes.push_back("- Updated Score v1 calculation to be more accurate");
	alpha291.changes.push_back("- Improved frame pacing");
	alpha291.changes.push_back("- Fixed object count always being 0 without osu! database");
	alpha291.changes.push_back("- Fixed slider end/tail judgements being too strict and not matching osu! exactly");
	alpha291.changes.push_back("- Fixed cursortrail being too small for some skins (if cursor@2x with non-@2x cursortrail)");
	alpha291.changes.push_back("- Fixed missing 24 ms offset for beatmaps version < 5");
	alpha291.changes.push_back("- Fixed local score tooltips not applying speed multiplier to AR/OD");
	changelogs.push_back(alpha291);

	CHANGELOG alpha29;
	alpha29.title = "29 (24.07.2018)";
	alpha29.changes.push_back("- Added local scores");
	alpha29.changes.push_back("- Added osu! scores.db support (read-only)");
	alpha29.changes.push_back("- Allow options menu anywhere (CTRL + O)");
	alpha29.changes.push_back("- VR: Added cursortrails, New cursor");
	alpha29.changes.push_back("- VR: Allow 0 meter approach distance");
	alpha29.changes.push_back("- VR: Added option \"Draw VR Approach Circles\" (Options > Virtual Reality > Gameplay)");
	alpha29.changes.push_back("- VR: Added option \"Draw VR Approach Circles on top\" (Options > Virtual Reality > Gameplay)");
	alpha29.changes.push_back("- VR: Added option \"Draw VR Approach Circles on Playfield\" (Options > Virtual Reality > Gameplay)");
	alpha29.changes.push_back("- Show enabled experimental mods on ranking screen");
	alpha29.changes.push_back("- Added scorebar-bg skin element support (usually abused for playfield background)");
	alpha29.changes.push_back("- Added option \"Draw scorebar-bg\" (Options > Gameplay > Playfield)");
	alpha29.changes.push_back("- Added option \"Legacy Slider Renderer\" (Options > Graphics > Detail Settings)");
	alpha29.changes.push_back("- Added option \"Mipmaps\" (Options > Graphics > Detail Settings)");
	alpha29.changes.push_back("- Added option \"Load osu! scores.db\" (Options > General > osu!folder)");
	alpha29.changes.push_back("- Added notification during active background star calculation in songbrowser");
	alpha29.changes.push_back("- Removed CTRL + ALT hardcoded hotkeys for scrubbing timeline");
	alpha29.changes.push_back("- General engine performance and stability improvements");
	alpha29.changes.push_back("- Fixed very old beatmaps not loading hitobjects which had float coordinates");
	alpha29.changes.push_back("- (see https://github.com/ppy/osu/pull/3072)");
	alpha29.changes.push_back("- Fixed scroll jerks/jumping randomly on all scrollviews");
	alpha29.changes.push_back("- Fixed random crash on shutdown due to double delete (OsuBeatmap::m_music)");
	changelogs.push_back(alpha29);

	CHANGELOG alpha2898;
	alpha2898.title = "28.98 (03.01.2018)";
	alpha2898.changes.push_back("- Updated pp algorithm (2), see https://github.com/ppy/osu-performance/pull/47");
	alpha2898.changes.push_back("- Updated pp algorithm (1), see https://github.com/ppy/osu-performance/pull/42");
	alpha2898.changes.push_back("- Added Score v2 mod");
	alpha2898.changes.push_back("- Added search support to options menu");
	alpha2898.changes.push_back("- Added proper volume overlay HUD with individual sliders for master/effects/music");
	alpha2898.changes.push_back("- Added/Fixed ConVars: osu_slider_followcircle_size_multiplier, osu_cursor_trail_alpha, osu_hud_volume_duration, osu_hud_volume_size_multiplier");
	alpha2898.changes.push_back("- Removed number keys being hardcoded keybinds for pause menu (1,2,3)");
	alpha2898.changes.push_back("- Improved multi-monitor handling: Remember game monitor, Fullscreen to closest monitor");
	alpha2898.changes.push_back("- Don't auto minimize window on focus lost if \"Borderless Windowed Fullscreen\" is enabled");
	alpha2898.changes.push_back("- Added ConVars: monitor, minimize_on_focus_lost_if_borderless_windowed_fullscreen");
	alpha2898.changes.push_back("- Reworked Mouse/Tablet input handling");
	alpha2898.changes.push_back("- Windows: Disable IME by default");
	alpha2898.changes.push_back("- Fixed cursor jerking to bottom right corner when accidentally wiggling mouse while using tablet");
	alpha2898.changes.push_back("- Fixed letterboxing cursor behavior (clipping/confining)");
	alpha2898.changes.push_back("- Fixed desynced slider ticks (e.g. Der Wald [Maze], first three sliders)");
	alpha2898.changes.push_back("- Fixed entering search text while binding keys in options menu");
	alpha2898.changes.push_back("- Fixed smooth cursortrail not expanding with animation");
	alpha2898.changes.push_back("- Fixed sample volumes being reset when tabbing out or losing window focus");
	alpha2898.changes.push_back("- VR: Fixed reverse arrows not being animated");
	alpha2898.changes.push_back("- Linux: Fixed crash when reloading osu database beatmaps via F5 in songbrowser");
	alpha2898.changes.push_back("- Linux: Updated BASS audio library to version 2.4.13 (19/12/2017)");
	alpha2898.changes.push_back("- Linux: Added support for confine cursor (i.e. Options > Input > Mouse > Confine Cursor, works now)");
	alpha2898.changes.push_back("- Linux: Added support for key names (i.e. Options > Input > Keyboard, key names instead of numbers)");
	alpha2898.changes.push_back("- Linux: Added support for focus changes (i.e. Options > General > Window > Pause on Focus Loss, fps_max_background)");
	alpha2898.changes.push_back("- Linux: Window is no longer resizable, added support for setWindowResizable()");
	alpha2898.changes.push_back("- Linux: Disabling fullscreen no longer causes annoying oversized window");
	alpha2898.changes.push_back("");
	changelogs.push_back(alpha2898);

	CHANGELOG alpha2897;
	alpha2897.title = "28.91 to 28.97 (29.10.2017)";
	alpha2897.changes.push_back("- Star ratings in songbrowser for everyone (even without osu!.db database)");
	alpha2897.changes.push_back("- Added smooth cursortrail support");
	alpha2897.changes.push_back("- Added option \"Force Smooth CursorTrail\" (Options > Skin)");
	alpha2897.changes.push_back("- Added sliderbreak counter to statistics overlay (Options > HUD > \"Draw Stats: SliderBreaks\")");
	alpha2897.changes.push_back("- Added new experimental mod \"Mafham\"");
	alpha2897.changes.push_back("- Added a few ConVars (osu_drain_*, osu_slider_reverse_arrow_animated)");
	alpha2897.changes.push_back("- Added multiple background image drawing toggles (Options > Gameplay > General)");
	alpha2897.changes.push_back("- Added partial support for \"BeatmapDirectory\" parameter");
	alpha2897.changes.push_back("- Added VR ConVars: osu_vr_controller_offset_x, osu_vr_controller_offset_y, osu_vr_controller_offset_z");
	alpha2897.changes.push_back("- Added description tooltips to some options");
	alpha2897.changes.push_back("- Added Daycore mod");
	alpha2897.changes.push_back("- Added option \"Higher Quality Sliders\", Options > Graphics > Detail Settings");
	alpha2897.changes.push_back("- Added current key labels to key bindings (Options > Input > Keyboard)");
	alpha2897.changes.push_back("- Added skin support for pause-overlay");
	alpha2897.changes.push_back("- Added preliminary Touchscreen support for Windows 8 and higher (Windows 8.1, Windows 10, etc.)");
	alpha2897.changes.push_back("- Added option \"Use skin's sound samples\", Options > Skin > Skin");
	alpha2897.changes.push_back("- Added 2 tablet options, \"Ignore Sensitivity\" and \"Windows Ink Workaround\" for problematic tablets/drivers");
	alpha2897.changes.push_back("- VR: Added slider sliding vibrations / haptic feedback (Options > Virtual Reality > Haptic Feedback)");
	alpha2897.changes.push_back("- VR: Added layout lock checkbox (avoid accidentally messing up your layout, especially for Oculus players)");
	alpha2897.changes.push_back("- Windows: Added hacky \"Borderless Windowed Fullscreen\" mode (Options > Graphics > Layout)");
	alpha2897.changes.push_back("- Windows: Switched to osu!'s old 2009 BASS audio library dlls to fix some slightly desynced mp3s");
	alpha2897.changes.push_back("- Windows: Stream music files instead of loading them entirely (necessary for above; faster load times)");
	alpha2897.changes.push_back("- Remember selected sorting type in songbrowser");
	alpha2897.changes.push_back("- Remember volume settings if changed outside options");
	alpha2897.changes.push_back("- Minor performance improvements (ignore empty transparent skin images)");
	alpha2897.changes.push_back("- Round stars to 1 decimal place for intelligent search (e.g. \"stars=6\")");
	alpha2897.changes.push_back("- Smoother snaking sliders");
	alpha2897.changes.push_back("- Switched to osu!lazer slider body fadeout style for snaking out sliders (aka shrinking sliders)");
	alpha2897.changes.push_back("- Renamed and moved \"Shrinking Sliders\" to \"Snaking out sliders\" (Options > Graphics > Detail Settings)");
	alpha2897.changes.push_back("- The drawn playfield border now also correctly reflects osu_playfield_stretch_x/y");
	alpha2897.changes.push_back("- Lowered minimum Mouse Sensitivity slider value from 0.4 to 0.1 (Options > Input > Mouse)");
	alpha2897.changes.push_back("- Made \"play-skip\" skin element animatable");
	alpha2897.changes.push_back("- Switched slider bezier curve generator to official algorithm (fixes Aspire loading \"freezes\", mostly)");
	alpha2897.changes.push_back("- More stuff in ranking screen tooltip (stars, speed, CS/AR/OD/HP)");
	alpha2897.changes.push_back("- Round pp up from 0.5 (727.49 => 727; 727.5 => 728)");
	alpha2897.changes.push_back("- Fixed ranking screen tooltip rounding CS/AR/OD/HP >= 10 incorrectly");
	alpha2897.changes.push_back("- Fixed sample set number being used for setting the sample type (hitsounds)");
	alpha2897.changes.push_back("- Fixed invisible selection-*.png skin images with 0 or very small size scaling the *-over.png way too big (Mods + Random)");
	alpha2897.changes.push_back("- Fixed minor visual inaccuracies (approach circle fade-in duration, hidden slider body fading behaviour)");
	alpha2897.changes.push_back("- Fixed skin versions lower than 2.2 drawing thumbnails in songbrowser, even though they shouldn't");
	alpha2897.changes.push_back("- Fixed statistics overlay layout in combination with the target practice mod");
	alpha2897.changes.push_back("- Fixed osu_mod_wobble2 sliders");
	alpha2897.changes.push_back("- Fixed timingpoint sample type and sample volume inaccuracies per-hitobject (osu_timingpoints_force, osu_timingpoints_offset)");
	alpha2897.changes.push_back("- Fixed missing sliderbreaks when the slider duration is shorter than the miss timing window of the sliderstartcircle");
	alpha2897.changes.push_back("- Fixed very long beatmaps not working (mostly random marathons with big MP3s, more than ~15 minutes)");
	alpha2897.changes.push_back("- Fixed slider vertexbuffers for ConVars: osu_playfield_rotation, osu_playfield_stretch_x, osu_playfield_stretch_y");
	alpha2897.changes.push_back("- Fixed main menu crashing on some aspire timingpoints due to division by zero");
	alpha2897.changes.push_back("- Fixed missing cursor texture on \"Click on the orange cursor to continue play!\"");
	alpha2897.changes.push_back("- Fixed incorrect hitcircle number skin overlaps");
	alpha2897.changes.push_back("- Fixed strange sliderstartcircle hitboxes on some 2007 maps");
	alpha2897.changes.push_back("- Fixed stacks not being recalculated when changing CS Override while playing");
	alpha2897.changes.push_back("- Fixed sliderbody size not updating instantly when changing CS Override slider with arrow keys");
	alpha2897.changes.push_back("- VR: Fixed SteamVR SuperSampling causing too large submission textures and breaking everything");
	alpha2897.changes.push_back("- Linux: Fixed number hotkeys not working in mod selector");
	changelogs.push_back(alpha2897);

	CHANGELOG alpha289;
	alpha289.title = "28.9 (16.04.2017)";
	alpha289.changes.push_back("- Thanks to Francesco149 for letting me use his implementation of Tom94's pp algorithm! (https://github.com/Francesco149/oppai/)");
	alpha289.changes.push_back("- Added pp to ranking/results screen");
	alpha289.changes.push_back("- Added live pp counter to statistics overlay (Options > HUD > \"Draw Stats: pp\")");
	alpha289.changes.push_back("- Added new experimental mods \"Full Alternate\", \"No 50s\" and \"No 100s no 50s\" (thanks to Jason Foley for the last two, JTF195 on github)");
	alpha289.changes.push_back("- Clamped Speed/BPM override sliders to the minimum possible 0.05x multiplier (no more negative zero)");
	alpha289.changes.push_back("- Unclamped visual AR/CS/HP/OD values in mod selection screen (e.g. negative OD due to EZHT)");
	alpha289.changes.push_back("- Fixed Auto clicking circles too early directly after loading finished (at time 0)");
	alpha289.changes.push_back("- Fixed ALT+TAB and general focus loss while starting a beatmap causing it to stop with a D rank and 100% acc");
	alpha289.changes.push_back("- Fixed \"CursorCentre: 0\" skin.ini parameter not working as expected");
	alpha289.changes.push_back("- Fixed performance optimization partially breaking 2B/Aspire beatmaps (invisible sliders)");
	alpha289.changes.push_back("- Fixed accuracy calculation dividing by zero if beatmap starts with a slider");
	alpha289.changes.push_back("- Fixed letterboxing and non-native resolutions not being consistent after restarting");
	changelogs.push_back(alpha289);

	CHANGELOG alpha288;
	alpha288.title = "28.8 (07.04.2017)";
	alpha288.changes.push_back("- Major performance optimizations");
	alpha288.changes.push_back("- Switched score calculation to 64-bit. The maximum score is now 18446744073709551615");
	alpha288.changes.push_back("- Fixed spinning spinners after dying in VR");
	alpha288.changes.push_back("- Fixed Auto failing impossible spinners");
	changelogs.push_back(alpha288);

	CHANGELOG alpha287;
	alpha287.title = "28.7 (05.04.2017)";
	alpha287.changes.push_back("- Added \"OS TabletPC Support\" (Options > Input > Tablet)");
	alpha287.changes.push_back("- Fixed being able to spin spinners with relax while paused");
	changelogs.push_back(alpha287);

	CHANGELOG alpha286;
	alpha286.title = "28.5 and 28.6 (03.04.2017)";
	alpha286.changes.push_back("- Added support for animated skins");
	alpha286.changes.push_back("- Added active mods to ranking/results screen");
	alpha286.changes.push_back("- Added avg error to Unstable Rate tooltip");
	alpha286.changes.push_back("- Added support for HitCirclePrefix in skin.ini");
	alpha286.changes.push_back("- Converted hardcoded hitwindow timing values into ConVars (osu_hitwindow_...)");
	alpha286.changes.push_back("- Fixed random vertexbuffer corruptions with shrinking sliders enabled (white flashes/blocks/triangles/lines, distorted slider bodies)");
	alpha286.changes.push_back("- Fixed animated skin elements of the default skin incorrectly overriding user skins");
	changelogs.push_back(alpha286);

	CHANGELOG alpha284;
	alpha284.title = "28.4 (28.03.2017)";
	alpha284.changes.push_back("- Added preliminary primitive sorting options to songbrowser (Artist, BPM, Creator, Date Added, Difficulty, Length, Title)");
	alpha284.changes.push_back("- Slightly improved right-click absolute scrolling range in songbrowser");
	alpha284.changes.push_back("- Fixed negative slider durations causing early gameovers (Aspire...)");
	alpha284.changes.push_back("- Fixed incorrect music lengths while searching (-1 from db caused 2^32-1 due to unsigned integer conversion)");
	changelogs.push_back(alpha284);

	CHANGELOG alpha283;
	alpha283.title = "28.3 (25.03.2017)";
	alpha283.changes.push_back("- Added VR head model to spectator mode");
	alpha283.changes.push_back("- Added ConVars vr_head_rendermodel_name, vr_head_rendermodel_scale, vr_head_rendermodel_brightness");
	alpha283.changes.push_back("- Added option to draw 300s (bottom of Options, last one)");
	alpha283.changes.push_back("- Made slider parsing & curve generation more robust against abuse, long loading times are not fixed though (Aspire...)");
	alpha283.changes.push_back("- Fixed experimental mod \"Minimize\" not working with CS Override as expected");
	alpha283.changes.push_back("- Fixed seeking/scrubbing not working properly while early waiting (green progressbar)");
	changelogs.push_back(alpha283);

	CHANGELOG alpha282;
	alpha282.title = "28.2 (22.03.2017)";
	alpha282.changes.push_back("- Fixed being able to continue spinning spinners while paused");
	alpha282.changes.push_back("- Fixed random invisible songbuttons in songbrowser on ultrawide resolutions (e.g. 21:9)");
	changelogs.push_back(alpha282);

	CHANGELOG alpha27VR;
	alpha27VR.title = "27 + 28 (Steam VR Alpha, 20.03.2017)";
	alpha27VR.changes.push_back("- Added Virtual Reality Support (OpenVR/SteamVR)");
	alpha27VR.changes.push_back("- VertexBuffer'd sliders for a ~400% fps increase (worst case) in static gameplay");
	alpha27VR.changes.push_back("- Dynamic mods like Wobble or Minimize are NOT buffered, and will perform the same as before");
	alpha27VR.changes.push_back("- Added audio output device selection (Options > Audio > Devices)");
	alpha27VR.changes.push_back("- Added VR matrix reset button (resets playfield & screen position/rotation)");
	alpha27VR.changes.push_back("- Added VR health drain & failing (experimental, but better than nothing)");
	alpha27VR.changes.push_back("- Added VR controller distance warning colorization (toggleable in Options)");
	alpha27VR.changes.push_back("");
	alpha27VR.changes.push_back("- Apply online offset automatically if available (was ignored before)");
	alpha27VR.changes.push_back("- Force minimize on focus loss (~alt-tab) if in fullscreen mode");
	alpha27VR.changes.push_back("");
	alpha27VR.changes.push_back("- Fixed beatmaps not loading if they have illegal spaces at the end of their directory name");
	alpha27VR.changes.push_back("- Improved SongBrowser SongButton layout on low resolutions and rectangular aspect ratios");
	alpha27VR.changes.push_back("- Added experimental mouse_fakelag ConVar");
	alpha27VR.changes.push_back("- Added osu_hud_hiterrorbar_width_percent, ..._height_percent, ..._bar_width_scale, ..._bar_height_scale ConVars");
	changelogs.push_back(alpha27VR);

	CHANGELOG alpha27;
	alpha27.title = "27 (Alpha, 25.12.2016)";
	alpha27.changes.push_back("- Added note blocking/locking (enabled by default!)");
	alpha27.changes.push_back("- Added \"Note Blocking\" toggle (Options > Gameplay > General)");
	alpha27.changes.push_back("- Added background fadein/fadeout during Breaks");
	alpha27.changes.push_back("- Added \"FadeIn Background during Breaks\" toggle (Options > Gameplay > General)");
	alpha27.changes.push_back("- Added \"Use osu!next Slider Style\" (Options > Skin > Skin)");
	alpha27.changes.push_back("- Added \"Show Approach Circle on First Hidden Object\" (Options > Gameplay > General)");
	alpha27.changes.push_back("- Added \"Use combo color as tint for slider border\" (Options > Skin > Skin)");
	alpha27.changes.push_back("- Separated \"Slider Opacity\" and \"SliderBody Opacity\" (Options > Skin > Skin)");
	alpha27.changes.push_back("- Added \"Boss Key (Minimize)\" keybind (Options > Input > Keyboard > Keys - Universal)");
	alpha27.changes.push_back("- Added \"Scrubbing\" keybind, defaults to SHIFT (Options > Input > Keyboard > Keys - In-Game)");
	alpha27.changes.push_back("- Added \"Draw Scrubbing Timeline\" (Options > Gameplay > HUD)");
	alpha27.changes.push_back("- Added \"Quick Save\" + \"Quick Load\" keybinds, default to F6 + F7 (Options > Input > Keyboard > Keys - In-Game)");
	alpha27.changes.push_back("- Added arrow key support to override sliders (left/right with a 0.1 stepsize)");
	alpha27.changes.push_back("");
	alpha27.changes.push_back("- Updated slider startcircle & endcircle fadeout with hidden, to match recent osu changes");
	alpha27.changes.push_back("- Changed hitobject sorting to draw simultaneous objects on top of active objects (circles during sliders, 2B etc.)");
	alpha27.changes.push_back("- Made scrubbing rebindable, added minimalistic scrubbing timeline UI (also shows quicksaves)");
	alpha27.changes.push_back("- While the timeline is visible, left click scrubs, and right click sets the quicksave time (in addition to the keybind)");
	alpha27.changes.push_back("- Improved scrubbing to only seek within the playable hitobject time (instead of the whole song)");
	alpha27.changes.push_back("- Inverted ALT key behaviour for override sliders (default stepsize is now 0.1, smaller stepsize of 0.01 while ALT is held)");
	alpha27.changes.push_back("");
	alpha27.changes.push_back("- Fixed incorrect opsu circle radius scaling within some aspect ratio ranges (CS was too high/difficult by as much as x0.2)");
	alpha27.changes.push_back("- Fixed reverse arrows counting towards accuracy");
	alpha27.changes.push_back("- Fixed jerky spinner spin sound");
	changelogs.push_back(alpha27);

	CHANGELOG alpha26;
	alpha26.title = "26 (Alpha, 27.11.2016)";
	alpha26.changes.push_back("- Added partial collection.db support (read-only!)");
	alpha26.changes.push_back("- Collections will atm contain all diffs of a beatmap, even if only 1 diff has been added to the collection in osu!");
	alpha26.changes.push_back("- Partial diffs of beatmaps in collections will be tinted orange");
	alpha26.changes.push_back("- Empty collections will not be visible");
	alpha26.changes.push_back("- Searching in collections will search all beatmaps and throw you back to \"No Grouping\"");
	alpha26.changes.push_back("- Added \"By Difficulty\" sorting (displays all beatmaps, sorted by their contained highest star rating diff)");
	alpha26.changes.push_back("- Added \"Unlimited FPS\" option (Options > Graphics > Renderer)");
	alpha26.changes.push_back("- Added slider preview to options (Options > Skin > Skin)");
	alpha26.changes.push_back("- Added \"SliderBorder Size\" slider (Options > Skin > Skin)");
	alpha26.changes.push_back("- Added \"Use slidergradient.png\" option (Options > Skin > Skin)");
	alpha26.changes.push_back("");
	alpha26.changes.push_back("- Improved thumbnail loading (scheduled loads of beatmap background image paths which become invisible before the load is finished will no longer trigger a useless image load upon finishing)");
	alpha26.changes.push_back("- This means that if you scroll with a higher velocity than 1 beatmap per 0.1 seconds per window height, thumbnails will load quicker");
	alpha26.changes.push_back("");
	alpha26.changes.push_back("- Fixed Stacking once and for all");
	alpha26.changes.push_back("- Fixed slider repeats giving too many points (they were counted as full circle hits additionally, instead of just 30 points)");
	alpha26.changes.push_back("- Score calculation is still not 100% accurate (slidertick placement)");
	alpha26.changes.push_back("- Fixed only the last few seconds of a song getting replayed upon returning to the songbrowser");
	alpha26.changes.push_back("- Fixed random beatmap selection only selecting up to the 32767th available beatmap");
	alpha26.changes.push_back("- Fixed Notes Per Second not scaling correctly with the Speed Multiplier");
	alpha26.changes.push_back("");
	alpha26.changes.push_back("- Fixed slider parser (hitSound/sampleType parameter for every slider circle including repeats)");
	alpha26.changes.push_back("- Fixed key1/key2 getting released incorrectly when mixing mouse buttons and keyboard keys while playing");
	alpha26.changes.push_back("- Fixed OsuUISongBrowserButton animations getting stuck when switching tabs (Grouping, Searching, Collections)");
	alpha26.changes.push_back("- Fixed incorrect sliderball image scaling");
	alpha26.changes.push_back("");
	alpha26.changes.push_back("- Fixed mouse buttons very rarely blocking keyboard keys (with mouse buttons disabled), getting m_bMouseKey1Down/m_bMouseKey2Down stuck");
	alpha26.changes.push_back("- Moved \"Shrinking Sliders\" option from \"Bullshit\" to \"Hitobjects\" (Options > Gameplay > Hitobjects)");
	changelogs.push_back(alpha26);

	CHANGELOG alpha25;
	alpha25.title = "25 (Alpha, 20.09.2016)";
	alpha25.changes.push_back("- Changed HitWindowMiss calculation from (500 - OD*10) ms to a constant 400 ms (clicks within this delta can count as misses)");
	alpha25.changes.push_back("");
	alpha25.changes.push_back("- Added osu!.db database support (read-only! nothing is modified) (Options > osu!folder)");
	alpha25.changes.push_back("- Added partial Score + Ranking Screen (no local scores yet)");
	alpha25.changes.push_back("- Added \"stars\" and \"length\" search parameters (only available if osu!.db support is enabled)");
	alpha25.changes.push_back("- Added star display to songbrowser (only available if osu!.db support is enabled)");
	alpha25.changes.push_back("- Added multi state mod button support, added Nightcore + Sudden Death + Perfect mods");
	alpha25.changes.push_back("- Added Mod Select hotkeys (Options > Keyboard > Keys > Mod Select)");
	alpha25.changes.push_back("- Added SliderBody Opacity slider (Options > Skin > Skin)");
	alpha25.changes.push_back("- Added \"Disable Mouse Buttons\" keybind (Options > Input > Keyboard)");
	alpha25.changes.push_back("- Added \"Save Screenshot\" keybind");
	alpha25.changes.push_back("- Added \"Player Name\" textbox to options");
	alpha25.changes.push_back("- Added new experimental mod \"Random\"");
	alpha25.changes.push_back("");
	alpha25.changes.push_back("- Added osu_draw_reverse_order command for experimental purposes (EZ reading, 1 to activate)");
	alpha25.changes.push_back("- Added osu_stacking command for experimental purposes (0 to disable stacks, 1 to enable)");
	alpha25.changes.push_back("- Added osu_playfield_stretch_x/y commands for experimental purposes (0.0 = osu, 1.0 = fullscreen, -2.0 = squish etc.)");
	alpha25.changes.push_back("");
	alpha25.changes.push_back("- Hitobject fadeout durations are now scaled with the speed multiplier (DT/HT/etc.)");
	alpha25.changes.push_back("- Wobble mod is now compensated for speed. Added osu_mod_wobble_frequency and osu_mod_wobble_rotation_speed commands");
	alpha25.changes.push_back("");
	alpha25.changes.push_back("- Fixed missing ApproachCircle in options skin preview from previous version");
	alpha25.changes.push_back("- Fixed alt-tab while m_bIsWaiting causing invisible clickable pause menu");
	alpha25.changes.push_back("- Fixed \"Disable Mouse Wheel in Play Mode\" also being active while not playing");
	changelogs.push_back(alpha25);
	*/

	CHANGELOG padding;
	padding.title = "... (older changelogs are available on GitHub or Steam)";
	padding.changes.push_back("");
	padding.changes.push_back("");
	padding.changes.push_back("");
	padding.changes.push_back("");
	padding.changes.push_back("");
	changelogs.push_back(padding);

	for (int i=0; i<changelogs.size(); i++)
	{
		CHANGELOG_UI changelog;

		// title label
		changelog.title = new CBaseUILabel(0, 0, 0, 0, "", changelogs[i].title);
		if (i == 0)
			changelog.title->setTextColor(0xff00ff00);
		else
			changelog.title->setTextColor(0xff888888);

		changelog.title->setSizeToContent();
		changelog.title->setDrawBackground(false);
		changelog.title->setDrawFrame(false);

		m_scrollView->getContainer()->addBaseUIElement(changelog.title);

		// changes
		for (int c=0; c<changelogs[i].changes.size(); c++)
		{
			CBaseUILabel *change = new CBaseUILabel(0, 0, 0, 0, "", changelogs[i].changes[c]);

			if (i > 0)
				change->setTextColor(0xff888888);

			change->setSizeToContent();
			change->setDrawBackground(false);
			change->setDrawFrame(false);

			changelog.changes.push_back(change);

			m_scrollView->getContainer()->addBaseUIElement(change);
		}

		m_changelogs.push_back(changelog);
	}
}

OsuChangelog::~OsuChangelog()
{
	m_changelogs.clear();

	SAFE_DELETE(m_container);
}

void OsuChangelog::draw(Graphics *g)
{
	if (!m_bVisible) return;

	m_container->draw(g);

	OsuScreenBackable::draw(g);
}

void OsuChangelog::update()
{
	OsuScreenBackable::update();
	if (!m_bVisible) return;

	// HACKHACK:
	if (m_osu->getHUD()->isVolumeOverlayBusy() || m_osu->getOptionsMenu()->isMouseInside())
		engine->getMouse()->resetWheelDelta();

	m_container->update();

	if (m_osu->getHUD()->isVolumeOverlayBusy() || m_osu->getOptionsMenu()->isMouseInside())
	{
		stealFocus();
		m_container->stealFocus();
	}
}

void OsuChangelog::setVisible(bool visible)
{
	OsuScreenBackable::setVisible(visible);

	if (m_bVisible)
		updateLayout();
}

void OsuChangelog::updateLayout()
{
	OsuScreenBackable::updateLayout();

	const float dpiScale = Osu::getUIScale();

	m_container->setSize(m_osu->getScreenSize() + Vector2(2, 2));
	m_scrollView->setSize(m_osu->getScreenSize() + Vector2(2, 2));

	float yCounter = 0;
	for (const CHANGELOG_UI &changelog : m_changelogs)
	{
		changelog.title->onResized(); // HACKHACK: framework, setSizeToContent() does not update string metrics
		changelog.title->setSizeToContent();

		yCounter += changelog.title->getSize().y;
		changelog.title->setRelPos(15 * dpiScale, yCounter);
		yCounter += 10 * dpiScale;

		for (CBaseUILabel *change : changelog.changes)
		{
			change->onResized(); // HACKHACK: framework, setSizeToContent() does not update string metrics
			change->setSizeToContent();
			yCounter += change->getSize().y + 7 * dpiScale;
			change->setRelPos(35 * dpiScale, yCounter);
		}

		// gap to previous version
		yCounter += 65 * dpiScale;
	}

	m_scrollView->setScrollSizeToContent(15 * dpiScale);
}

void OsuChangelog::onBack()
{
	engine->getSound()->play(m_osu->getSkin()->getMenuClick());

	m_osu->toggleChangelog();
}
