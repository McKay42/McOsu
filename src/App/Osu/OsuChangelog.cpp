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

	CHANGELOG alpha29Steam;
	alpha29Steam.title = UString::format("29 (Steam VR Version, %s, %s)", __DATE__, __TIME__);
	alpha29Steam.changes.push_back("- Added key overlay");
	alpha29Steam.changes.push_back("- Improved frame pacing");
	alpha29Steam.changes.push_back("- Added new experimental mod \"Reverse Sliders\"");
	alpha29Steam.changes.push_back("- Added mouse sidebutton support (mouse4, mouse5)");
	alpha29Steam.changes.push_back("- Added detail info tooltip (approach time, hit timings, etc.) when hovering over diff info label in songbrowser (CS AR etc.)");
	alpha29Steam.changes.push_back("- Updated diff info label in songbrowser to respect mods/overrides");
	alpha29Steam.changes.push_back("- Fixed object count always being 0 without osu! database");
	alpha29Steam.changes.push_back("");
	alpha29Steam.changes.push_back("- Fixed slider end/tail judgements being too strict and not matching osu! exactly");
	alpha29Steam.changes.push_back("- Added ConVar: osu_slider_end_inside_check_offset");
	alpha29Steam.changes.push_back("");
	alpha29Steam.changes.push_back("- Added positional audio for hitsounds");
	alpha29Steam.changes.push_back("- Added Score V2 keybind (defaults to 'B')");
	alpha29Steam.changes.push_back("- Updated combo color handling to match osu!");
	alpha29Steam.changes.push_back("- Added ConVars: osu_sound_panning, osu_sound_panning_multiplier, osu_approachtime_min, osu_approachtime_mid, osu_approachtime_max");
	alpha29Steam.changes.push_back("- Fixed local score tooltips not applying speed multiplier to AR/OD");
	alpha29Steam.changes.push_back("");
	alpha29Steam.changes.push_back("- Added local scores");
	alpha29Steam.changes.push_back("- Added osu! scores.db support (read-only)");
	alpha29Steam.changes.push_back("- Allow options menu anywhere (CTRL + O)");
	alpha29Steam.changes.push_back("- VR: Added cursortrails, New cursor");
	alpha29Steam.changes.push_back("- VR: Allow 0 meter approach distance");
	alpha29Steam.changes.push_back("- VR: Added option \"Draw VR Approach Circles\" (Options > Virtual Reality > Gameplay)");
	alpha29Steam.changes.push_back("- VR: Added option \"Draw VR Approach Circles on top\" (Options > Virtual Reality > Gameplay)");
	alpha29Steam.changes.push_back("- VR: Added option \"Draw VR Approach Circles on Playfield\" (Options > Virtual Reality > Gameplay)");
	alpha29Steam.changes.push_back("- Show enabled experimental mods on ranking screen");
	alpha29Steam.changes.push_back("- Added scorebar-bg skin element support (usually abused for playfield background)");
	alpha29Steam.changes.push_back("- Added option \"Draw scorebar-bg\" (Options > Gameplay > Playfield)");
	alpha29Steam.changes.push_back("- Added option \"Legacy Slider Renderer\" (Options > Graphics > Detail Settings)");
	alpha29Steam.changes.push_back("- Added option \"Mipmaps\" (Options > Graphics > Detail Settings)");
	alpha29Steam.changes.push_back("- Added option \"Load osu! scores.db\" (Options > General > osu!folder)");
	alpha29Steam.changes.push_back("- Added notification during active background star calculation in songbrowser");
	alpha29Steam.changes.push_back("- Removed CTRL + ALT hardcoded hotkeys for scrubbing timeline");
	alpha29Steam.changes.push_back("- General engine performance and stability improvements");
	alpha29Steam.changes.push_back("- Fixed very old beatmaps not loading hitobjects which had float coordinates");
	alpha29Steam.changes.push_back("- (see https://github.com/ppy/osu/pull/3072)");
	alpha29Steam.changes.push_back("- Fixed scroll jerks/jumping randomly on all scrollviews");
	alpha29Steam.changes.push_back("- Fixed random crash on shutdown due to double delete (OsuBeatmap::m_music)");
	alpha29Steam.changes.push_back("");
	alpha29Steam.changes.push_back("");
	alpha29Steam.changes.push_back("");
	alpha29Steam.changes.push_back("");
	alpha29Steam.changes.push_back("");
	alpha29Steam.changes.push_back("");
	alpha29Steam.changes.push_back("- Updated pp algorithm (2), see https://github.com/ppy/osu-performance/pull/47");
	alpha29Steam.changes.push_back("- Updated pp algorithm (1), see https://github.com/ppy/osu-performance/pull/42");
	alpha29Steam.changes.push_back("- Removed number keys being hardcoded keybinds for pause menu (1,2,3)");
	alpha29Steam.changes.push_back("- Fixed smooth cursortrail not expanding with animation");
	alpha29Steam.changes.push_back("- Fixed sample volumes being reset when tabbing out or losing window focus");
	alpha29Steam.changes.push_back("- VR: Fixed reverse arrows not being animated");
	alpha29Steam.changes.push_back("");
	alpha29Steam.changes.push_back("- Improved multi-monitor handling: Remember game monitor, Fullscreen to closest monitor");
	alpha29Steam.changes.push_back("- Don't auto minimize window on focus lost if \"Borderless Windowed Fullscreen\" is enabled");
	alpha29Steam.changes.push_back("- Added ConVars: monitor, minimize_on_focus_lost_if_borderless_windowed_fullscreen");
	alpha29Steam.changes.push_back("- Windows: Disable IME by default");
	alpha29Steam.changes.push_back("- Linux: Window is no longer resizable, added support for setWindowResizable()");
	alpha29Steam.changes.push_back("- Linux: Disabling fullscreen no longer causes annoying oversized window");
	alpha29Steam.changes.push_back("");
	alpha29Steam.changes.push_back("- Reworked Mouse/Tablet input handling");
	alpha29Steam.changes.push_back("- Fixed cursor jerking to bottom right corner when accidentally wiggling mouse while using tablet");
	alpha29Steam.changes.push_back("- Fixed letterboxing cursor behavior (clipping/confining)");
	alpha29Steam.changes.push_back("- Fixed desynced slider ticks (e.g. Der Wald [Maze], first three sliders)");
	alpha29Steam.changes.push_back("- Linux: Fixed crash when reloading osu database beatmaps via F5 in songbrowser");
	alpha29Steam.changes.push_back("");
	alpha29Steam.changes.push_back("- Added Score v2 mod");
	alpha29Steam.changes.push_back("- Added search support to options menu");
	alpha29Steam.changes.push_back("- Added proper volume overlay HUD with individual sliders for master/effects/music");
	alpha29Steam.changes.push_back("- Added/Fixed ConVars: osu_slider_followcircle_size_multiplier, osu_cursor_trail_alpha,");
	alpha29Steam.changes.push_back("                                     osu_hud_volume_duration, osu_hud_volume_size_multiplier");
	alpha29Steam.changes.push_back("- Fixed entering search text while binding keys in options menu");
	alpha29Steam.changes.push_back("- Linux: Updated BASS audio library to version 2.4.13 (19/12/2017)");
	alpha29Steam.changes.push_back("- Linux: Added support for confine cursor (i.e. Options > Input > Mouse > Confine Cursor, works now)");
	alpha29Steam.changes.push_back("- Linux: Added support for key names (i.e. Options > Input > Keyboard, key names instead of numbers)");
	alpha29Steam.changes.push_back("- Linux: Added support for focus changes (i.e. Options > General > Window > Pause on Focus Loss, fps_max_background)");
	alpha29Steam.changes.push_back("");
	alpha29Steam.changes.push_back("- Added smooth cursortrail support");
	alpha29Steam.changes.push_back("- Added option \"Force Smooth CursorTrail\" (Options > Skin)");
	alpha29Steam.changes.push_back("- Added sliderbreak counter to statistics overlay (Options > HUD > \"Draw Stats: SliderBreaks\")");
	alpha29Steam.changes.push_back("- Star ratings in songbrowser for everyone (even without osu!.db database)");
	alpha29Steam.changes.push_back("- VR: Added layout lock checkbox (avoid accidentally messing up your layout, especially for Oculus players)");
	alpha29Steam.changes.push_back("- Remember selected sorting type in songbrowser");
	alpha29Steam.changes.push_back("- Remember volume settings if changed outside options");
	alpha29Steam.changes.push_back("- Linux: Fixed number hotkeys not working in mod selector");
	alpha29Steam.changes.push_back("- Minor performance improvements (ignore empty transparent skin images)");
	alpha29Steam.changes.push_back("");
	alpha29Steam.changes.push_back("- Added new experimental mod \"Mafham\"");
	alpha29Steam.changes.push_back("- Added a few ConVars (osu_drain_*, osu_slider_reverse_arrow_animated)");
	alpha29Steam.changes.push_back("- Round stars to 1 decimal place for intelligent search (e.g. \"stars=6\")");
	alpha29Steam.changes.push_back("- VR: Added slider sliding vibrations / haptic feedback (Options > Virtual Reality > Haptic Feedback)");
	alpha29Steam.changes.push_back("- Smoother snaking sliders");
	alpha29Steam.changes.push_back("- Switched to osu!lazer slider body fadeout style for snaking out sliders (aka shrinking sliders)");
	alpha29Steam.changes.push_back("- Fixed strange sliderstartcircle hitboxes on some 2007 maps");
	alpha29Steam.changes.push_back("- Fixed stacks not being recalculated when changing CS Override while playing");
	alpha29Steam.changes.push_back("- Fixed sliderbody size not updating instantly when changing CS Override slider with arrow keys");
	alpha29Steam.changes.push_back("");
	alpha29Steam.changes.push_back("- Added multiple background image drawing toggles (Options > Gameplay > General)");
	alpha29Steam.changes.push_back("- Added partial support for \"BeatmapDirectory\" parameter");
	alpha29Steam.changes.push_back("- Added VR ConVars: osu_vr_controller_offset_x, osu_vr_controller_offset_y, osu_vr_controller_offset_z");
	alpha29Steam.changes.push_back("- Added description tooltips to some options");
	alpha29Steam.changes.push_back("- Windows: Added hacky \"Borderless Windowed Fullscreen\" mode (Options > Graphics > Layout)");
	alpha29Steam.changes.push_back("- Windows: Switched to osu!'s old 2009 BASS audio library dlls to fix some slightly desynced mp3s");
	alpha29Steam.changes.push_back("- Windows: Stream music files instead of loading them entirely (necessary for above; faster load times)");
	alpha29Steam.changes.push_back("- Fixed minor visual inaccuracies (approach circle fade-in duration, hidden slider body fading behaviour)");
	alpha29Steam.changes.push_back("- Fixed skin versions lower than 2.2 drawing thumbnails in songbrowser, even though they shouldn't");
	alpha29Steam.changes.push_back("- Fixed statistics overlay layout in combination with the target practice mod");
	alpha29Steam.changes.push_back("- Fixed osu_mod_wobble2 sliders");
	alpha29Steam.changes.push_back("");
	alpha29Steam.changes.push_back("- Fixed timingpoint sample type and sample volume inaccuracies per-hitobject (osu_timingpoints_force, osu_timingpoints_offset)");
	alpha29Steam.changes.push_back("- Fixed missing sliderbreaks when the slider duration is shorter than the miss timing window of the sliderstartcircle");
	alpha29Steam.changes.push_back("");
	alpha29Steam.changes.push_back("- Added Daycore mod");
	alpha29Steam.changes.push_back("- Added option \"Higher Quality Sliders\", Options > Graphics > Detail Settings");
	alpha29Steam.changes.push_back("- Added current key labels to key bindings (Options > Input > Keyboard)");
	alpha29Steam.changes.push_back("- Added skin support for pause-overlay");
	alpha29Steam.changes.push_back("- Renamed and moved \"Shrinking Sliders\" to \"Snaking out sliders\" (Options > Graphics > Detail Settings)");
	alpha29Steam.changes.push_back("- Fixed main menu crashing on some aspire timingpoints due to division by zero");
	alpha29Steam.changes.push_back("- Fixed missing cursor texture on \"Click on the orange cursor to continue play!\"");
	alpha29Steam.changes.push_back("- Fixed incorrect hitcircle number skin overlaps");
	alpha29Steam.changes.push_back("");
	alpha29Steam.changes.push_back("- Added preliminary Touchscreen support for Windows 8 and higher (Windows 8.1, Windows 10, etc.)");
	alpha29Steam.changes.push_back("- Fixed very long beatmaps not working (mostly random marathons with big MP3s, more than ~15 minutes)");
	alpha29Steam.changes.push_back("- Fixed slider vertexbuffers for ConVars: osu_playfield_rotation, osu_playfield_stretch_x, osu_playfield_stretch_y");
	alpha29Steam.changes.push_back("- The drawn playfield border now also correctly reflects osu_playfield_stretch_x/y");
	alpha29Steam.changes.push_back("- VR: Fixed SteamVR SuperSampling causing too large submission textures and breaking everything");
	alpha29Steam.changes.push_back("");
	alpha29Steam.changes.push_back("- Lowered minimum Mouse Sensitivity slider value from 0.4 to 0.1 (Options > Input > Mouse)");
	alpha29Steam.changes.push_back("- Made \"play-skip\" skin element animatable");
	alpha29Steam.changes.push_back("- Fixed ranking screen tooltip rounding CS/AR/OD/HP >= 10 incorrectly");
	alpha29Steam.changes.push_back("");
	alpha29Steam.changes.push_back("- Switched slider bezier curve generator to official algorithm (fixes Aspire loading \"freezes\", mostly)");
	alpha29Steam.changes.push_back("- More stuff in ranking screen tooltip (stars, speed, CS/AR/OD/HP)");
	alpha29Steam.changes.push_back("- Round pp up from 0.5 (727.49 => 727; 727.5 => 728)");
	alpha29Steam.changes.push_back("");
	alpha29Steam.changes.push_back("- Added option \"Use skin's sound samples\", Options > Skin > Skin");
	alpha29Steam.changes.push_back("- Added 2 tablet options, \"Ignore Sensitivity\" and \"Windows Ink Workaround\" for problematic tablets/drivers");
	alpha29Steam.changes.push_back("- Fixed sample set number being used for setting the sample type (hitsounds)");
	alpha29Steam.changes.push_back("- Fixed invisible selection-*.png skin images with 0 or very small size scaling the *-over.png way too big (Mods + Random)");
	alpha29Steam.changes.push_back("");
	alpha29Steam.changes.push_back("- Thanks to Francesco149 for letting me use his implementation of Tom94's pp algorithm! (https://github.com/Francesco149/oppai/)");
	alpha29Steam.changes.push_back("- Added pp to ranking/results screen");
	alpha29Steam.changes.push_back("- Added live pp counter to statistics overlay (Options > HUD > \"Draw Stats: pp\")");
	alpha29Steam.changes.push_back("- Added new experimental mods \"Full Alternate\", \"No 50s\" and \"No 100s no 50s\" (thanks to Jason Foley for the last two, JTF195 on github)");
	alpha29Steam.changes.push_back("- Clamped Speed/BPM override sliders to the minimum possible 0.05x multiplier (no more negative zero)");
	alpha29Steam.changes.push_back("- Unclamped visual AR/CS/HP/OD values in mod selection screen (e.g. negative OD due to EZHT)");
	alpha29Steam.changes.push_back("- Fixed Auto clicking circles too early directly after loading finished (at time 0)");
	alpha29Steam.changes.push_back("- Fixed ALT+TAB and general focus loss while starting a beatmap causing it to stop with a D rank and 100% acc");
	alpha29Steam.changes.push_back("- Fixed \"CursorCentre: 0\" skin.ini parameter not working as expected");
	alpha29Steam.changes.push_back("");
	alpha29Steam.changes.push_back("- Fixed performance optimization partially breaking 2B/Aspire beatmaps (invisible sliders)");
	alpha29Steam.changes.push_back("- Fixed accuracy calculation dividing by zero if beatmap starts with a slider");
	alpha29Steam.changes.push_back("- Fixed letterboxing and non-native resolutions not being consistent after restarting");
	alpha29Steam.changes.push_back("");
	alpha29Steam.changes.push_back("- Major performance optimizations");
	alpha29Steam.changes.push_back("- Switched score calculation to 64-bit. The maximum score is now 18446744073709551615");
	alpha29Steam.changes.push_back("- Fixed spinning spinners after dying in VR");
	alpha29Steam.changes.push_back("- Fixed Auto failing impossible spinners");
	alpha29Steam.changes.push_back("");
	alpha29Steam.changes.push_back("- Added \"OS TabletPC Support\" (Options > Input > Tablet)");
	alpha29Steam.changes.push_back("- Fixed being able to spin spinners with relax while paused");
	alpha29Steam.changes.push_back("");
	alpha29Steam.changes.push_back("- Added support for animated skins");
	alpha29Steam.changes.push_back("- Added active mods to ranking/results screen");
	alpha29Steam.changes.push_back("- Added avg error to Unstable Rate tooltip");
	alpha29Steam.changes.push_back("- Added support for HitCirclePrefix in skin.ini");
	alpha29Steam.changes.push_back("- Converted hardcoded hitwindow timing values into ConVars (osu_hitwindow_...)");
	alpha29Steam.changes.push_back("- Fixed random vertexbuffer corruptions with shrinking sliders enabled (white flashes/blocks/triangles/lines, distorted slider bodies)");
	alpha29Steam.changes.push_back("- Fixed animated skin elements of the default skin incorrectly overriding user skins");
	alpha29Steam.changes.push_back("");
	alpha29Steam.changes.push_back("- Added preliminary primitive sorting options to songbrowser (Artist, BPM, Creator, Date Added, Difficulty, Length, Title)");
	alpha29Steam.changes.push_back("- Slightly improved right-click absolute scrolling range in songbrowser");
	alpha29Steam.changes.push_back("- Fixed negative slider durations causing early gameovers (Aspire...)");
	alpha29Steam.changes.push_back("- Fixed incorrect music lengths while searching (-1 from db caused 2^32-1 due to unsigned integer conversion)");
	alpha29Steam.changes.push_back("");
	alpha29Steam.changes.push_back("- Added VR head model to spectator mode");
	alpha29Steam.changes.push_back("- Added ConVars vr_head_rendermodel_name, vr_head_rendermodel_scale, vr_head_rendermodel_brightness");
	alpha29Steam.changes.push_back("- Added option to draw 300s (bottom of Options, last one)");
	alpha29Steam.changes.push_back("- Made slider parsing & curve generation more robust against abuse, long loading times are not fixed though (Aspire...)");
	alpha29Steam.changes.push_back("- Fixed experimental mod \"Minimize\" not working with CS Override as expected");
	alpha29Steam.changes.push_back("- Fixed seeking/scrubbing not working properly while early waiting (green progressbar)");
	alpha29Steam.changes.push_back("");
	alpha29Steam.changes.push_back("- Fixed being able to continue spinning spinners while paused");
	alpha29Steam.changes.push_back("- Fixed random invisible songbuttons in songbrowser on ultrawide resolutions (e.g. 21:9)");
	changelogs.push_back(alpha29Steam);

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

	float lastYPos = 0;
	for (int i=0; i<changelogs.size(); i++)
	{
		// title label
		CBaseUILabel *title = new CBaseUILabel(0, 0, 0, 0, "", changelogs[i].title);
		if (i == 0)
			title->setTextColor(0xff00ff00);
		else
			title->setTextColor(0xff888888);

		title->setSizeToContent();
		title->setDrawBackground(false);
		title->setDrawFrame(false);
		lastYPos += title->getSize().y;
		title->setPos(15, lastYPos);
		lastYPos += 10;

		m_scrollView->getContainer()->addBaseUIElement(title);

		// changes
		for (int c=0; c<changelogs[i].changes.size(); c++)
		{
			CBaseUILabel *change = new CBaseUILabel(0, 0, 0, 0, "", changelogs[i].changes[c]);

			if (i > 0)
				change->setTextColor(0xff888888);

			change->setSizeToContent();
			change->setDrawBackground(false);
			change->setDrawFrame(false);
			lastYPos += change->getSize().y + 7;
			change->setPos(35, lastYPos);

			m_scrollView->getContainer()->addBaseUIElement(change);
		}

		// gap to previous version
		lastYPos += 65;
	}
}

OsuChangelog::~OsuChangelog()
{
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

	m_container->setSize(m_osu->getScreenSize() + Vector2(2,2));
	m_scrollView->setSize(m_osu->getScreenSize() + Vector2(2,2));

	m_scrollView->setScrollSizeToContent(15);
}

void OsuChangelog::onBack()
{
	engine->getSound()->play(m_osu->getSkin()->getMenuClick());

	m_osu->toggleChangelog();
}
