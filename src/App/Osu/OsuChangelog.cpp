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

#include "CBaseUIContainer.h"
#include "CBaseUIScrollView.h"
#include "CBaseUILabel.h"

#include "Osu.h"
#include "OsuSkin.h"

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

	// TODO: haptic slider feedback is hidden until SteamVR is fixed
	/*
	CHANGELOG alpha28steam;
	alpha28steam.title = "28 (Steam VR Alpha, TODOTODO)";
	alpha28steam.changes.push_back("- Added haptic feedback to sliders (Options > Virtual Reality > Slider Vibration Strength)");
	changelogs.push_back(alpha28steam);
	*/

	CHANGELOG alpha29Steam;
	alpha29Steam.title = "28.3 (Steam VR Alpha, ?)";
	alpha29Steam.changes.push_back("- Added option to draw 300s (bottom of Options, last one)");
	alpha29Steam.changes.push_back("");
	alpha29Steam.changes.push_back("- Made slider parsing & curve generation more robust against abuse (Aspire...)");
	alpha29Steam.changes.push_back("");
	alpha29Steam.changes.push_back("- Fixed experimental mod \"Minimize\" not working with CS Override as expected");
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
			title->setTextColor(0xffaaaaaa);
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
				change->setTextColor(0xffaaaaaa);
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

	m_container->update();
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
