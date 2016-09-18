//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		beatmap browser and selector
//
// $NoKeywords: $osusb
//===============================================================================//

#include "OsuSongBrowser2.h"

#include "Engine.h"
#include "ConVar.h"
#include "ResourceManager.h"
#include "Keyboard.h"
#include "Mouse.h"
#include "Timer.h"
#include "SoundEngine.h"
#include "AnimationHandler.h"
#include "VertexArrayObject.h"

#include "CBaseUIContainer.h"
#include "CBaseUIImageButton.h"
#include "CBaseUIScrollView.h"

#include "Osu.h"
#include "OsuHUD.h"
#include "OsuSkin.h"
#include "OsuBeatmap.h"
#include "OsuBeatmapDifficulty.h"
#include "OsuModSelector.h"
#include "OsuKeyBindings.h"

#include "OsuUIBackButton.h"
#include "OsuUISelectionButton.h"
#include "OsuUISongBrowserInfoLabel.h"
#include "OsuUISongBrowserButton.h"

ConVar osu_songbrowser_topbar_left_percent("osu_songbrowser_topbar_left_percent", 0.93f);
ConVar osu_songbrowser_topbar_left_width_percent("osu_songbrowser_topbar_left_width_percent", 0.265f);
ConVar osu_songbrowser_topbar_middle_width_percent("osu_songbrowser_topbar_middle_width_percent", 0.15f);
ConVar osu_songbrowser_topbar_right_percent("osu_songbrowser_topbar_right_percent", 0.5f);

OsuSongBrowser2::OsuSongBrowser2(Osu *osu) : OsuScreenBackable(osu)
{
	m_osu = osu;

	// convar refs
	m_fps_max_ref = convar->getConVarByName("fps_max");
	m_osu_songbrowser_bottombar_percent_ref = convar->getConVarByName("osu_songbrowser_bottombar_percent");

	// engine settings
	engine->getMouse()->addListener(this);

	// vars
	m_bF1Pressed = false;
	m_bF2Pressed = false;
	m_bShiftPressed = false;
	m_bLeft = false;
	m_bRight = false;

	m_bRandomBeatmapScheduled = false;
	m_bPreviousRandomBeatmapScheduled = false;

	m_fSongSelectTopScale = 1.0f;

	// build topbar left
	m_topbarLeft = new CBaseUIContainer(0, 0, 0, 0, "");

	m_songInfo = new OsuUISongBrowserInfoLabel(m_osu, 0, 0, 0, 0, "");
	m_topbarLeft->addBaseUIElement(m_songInfo);

	// build topbar right
	m_topbarRight = new CBaseUIContainer(0, 0, 0, 0, "");

	// build bottombar
	m_bottombar = new CBaseUIContainer(0, 0, 0, 0, "");

	addBottombarNavButton()->setClickCallback( fastdelegate::MakeDelegate(this, &OsuSongBrowser2::onSelectionMods) );
	addBottombarNavButton()->setClickCallback( fastdelegate::MakeDelegate(this, &OsuSongBrowser2::onSelectionRandom) );
	///addBottombarNavButton()->setClickCallback( fastdelegate::MakeDelegate(this, &OsuSongBrowser::onSelectionOptions) );

	// build songbrowser
	m_songBrowser = new CBaseUIScrollView(0, 0, 0, 0, "");
	m_songBrowser->setDrawBackground(false);
	m_songBrowser->setDrawFrame(false);
	m_songBrowser->setHorizontalScrolling(false);
	m_songBrowser->setScrollResistance(15);

	// beatmap database
	m_importTimer = new Timer();
	m_bBeatmapRefreshNeedsFileRefresh = true;
	m_iCurRefreshBeatmap = 0;
	m_iCurRefreshNumBeatmaps = 0;
	m_bBeatmapRefreshScheduled = true;

	// behaviour
	m_bHasSelectedAndIsPlaying = false;
	m_selectedBeatmap = NULL;
	m_fPulseAnimation = 0.0f;

	// search
	m_fSearchWaitTime = 0.0f;

	updateLayout();
}

OsuSongBrowser2::~OsuSongBrowser2()
{
	SAFE_DELETE(m_topbarLeft);
	SAFE_DELETE(m_topbarRight);
	SAFE_DELETE(m_bottombar);
	SAFE_DELETE(m_songBrowser);
}

void OsuSongBrowser2::draw(Graphics *g)
{
	if (!m_bVisible)
		return;

	// refreshing (blocks every other call in draw() below it!)
	if (m_bBeatmapRefreshScheduled)
	{
		g->setColor(0xffffffff);
		UString loadingMessage = UString::format("Loading beatmaps ... (%i %%)", (int)(((float)m_iCurRefreshBeatmap/(float)m_iCurRefreshNumBeatmaps)*100.0f));
		g->pushTransform();
		g->translate(m_osu->getScreenWidth()/2 - m_osu->getTitleFont()->getStringWidth(loadingMessage)/2, m_osu->getScreenHeight() - 15);
		g->drawString(m_osu->getTitleFont(), loadingMessage);
		g->popTransform();

		m_osu->getHUD()->drawBeatmapImportSpinner(g);
		return;
	}


	// draw background
	g->setColor(0xff000000);
	g->fillRect(0, 0, m_osu->getScreenWidth(), m_osu->getScreenHeight());

	// draw background image
	drawSelectedBeatmapBackgroundImage(g, m_osu);

	// draw song browser
	m_songBrowser->draw(g);

	// draw search text and background
	UString searchText1 = "Search: ";
	UString searchText2 = "Type to search!";
	UString combinedSearchText = searchText1;
	combinedSearchText.append(searchText2);
	McFont *searchTextFont = m_osu->getSubTitleFont();
	float searchTextScale = 0.75f;
	bool hasSearchSubTextVisible = m_sSearchString.length() > 0 && (m_fSearchWaitTime == 0.0f || m_visibleSongButtons.size() != m_songButtons.size());
	g->setColor(COLOR(m_sSearchString.length() > 0 ? 100 : 30, 0, 0, 0));
	g->fillRect(m_songBrowser->getPos().x + m_songBrowser->getSize().x*0.75f - searchTextFont->getStringWidth(combinedSearchText)*searchTextScale/2 - (searchTextFont->getHeight()*searchTextScale)*0.5f, m_songBrowser->getPos().y, m_songBrowser->getSize().x, (searchTextFont->getHeight()*searchTextScale)*(hasSearchSubTextVisible ? 4.0f : 3.0f));
	g->setColor(0xffffffff);
	g->pushTransform();
		g->translate(0, searchTextFont->getHeight()/2);
		g->scale(searchTextScale, searchTextScale);
		g->translate(m_songBrowser->getPos().x + m_songBrowser->getSize().x*0.75f - searchTextFont->getStringWidth(combinedSearchText)*searchTextScale/2, m_songBrowser->getPos().y + (searchTextFont->getHeight()*searchTextScale)*1.5f);

		// draw search text and text
		g->pushTransform();
			g->setColor(0xff00ff00);
			g->drawString(searchTextFont, searchText1);
			g->setColor(0xffffffff);
			g->translate(searchTextFont->getStringWidth(searchText1)*searchTextScale, 0);
			if (m_sSearchString.length() < 1)
				g->drawString(searchTextFont, searchText2);
			else
				g->drawString(searchTextFont, m_sSearchString);
		g->popTransform();

		// draw number of matches
		if (hasSearchSubTextVisible)
		{
			g->setColor(0xffffffff);
			g->translate(0, (searchTextFont->getHeight()*searchTextScale)*1.5f);
			g->drawString(searchTextFont, m_visibleSongButtons.size() > 0 ? UString::format("%i matches found!", m_visibleSongButtons.size()) : "No matches found. Hit ESC to reset.");
		}
	g->popTransform();

	// draw top bar
	g->setColor(0xffffffff);
	g->pushTransform();
		g->scale(m_fSongSelectTopScale, m_fSongSelectTopScale);
		g->translate((m_osu->getSkin()->getSongSelectTop()->getWidth()*m_fSongSelectTopScale)/2, (m_osu->getSkin()->getSongSelectTop()->getHeight()*m_fSongSelectTopScale)/2);
		g->drawImage(m_osu->getSkin()->getSongSelectTop());
	g->popTransform();

	m_topbarLeft->draw(g);
	//m_topbarLeft->draw_debug(g);
	m_topbarRight->draw(g);
	//m_topbarRight->draw_debug(g);

	// draw bottom bar
	float songSelectBottomScale = m_bottombar->getSize().y / m_osu->getSkin()->getSongSelectBottom()->getHeight();
	songSelectBottomScale *= 0.8f;

	g->setColor(0xff000000);
	g->fillRect(0, m_bottombar->getPos().y + 10, m_osu->getScreenWidth(), m_bottombar->getSize().y);

	g->setColor(0xffffffff);
	g->pushTransform();
		g->scale(songSelectBottomScale, songSelectBottomScale);
		g->translate(0, (int)(m_bottombar->getPos().y) + (int)((m_osu->getSkin()->getSongSelectBottom()->getHeight()*songSelectBottomScale)/2) - 1);
		m_osu->getSkin()->getSongSelectBottom()->bind();
		g->drawQuad(0, -(int)(m_bottombar->getSize().y*(1.0f/songSelectBottomScale)/2), (int)(m_osu->getScreenWidth()*(1.0f/songSelectBottomScale)), (int)(m_bottombar->getSize().y*(1.0f/songSelectBottomScale)));
	g->popTransform();

	m_bottombar->draw(g);
	//m_bottombar->draw_debug(g);
	OsuScreenBackable::draw(g);


	// no beatmaps found (osu folder is probably invalid)
	if (m_beatmaps.size() == 0 && !m_bBeatmapRefreshScheduled)
	{
		g->setColor(0xffff0000);
		UString errorMessage1 = "Invalid osu! folder (or no beatmaps found): ";
		errorMessage1.append(m_sLastOsuFolder);
		UString errorMessage2 = "Go to Options -> osu!folder";
		g->pushTransform();
		g->translate((int)(m_osu->getScreenWidth()/2 - m_osu->getSubTitleFont()->getStringWidth(errorMessage1)/2), (int)(m_osu->getScreenHeight()/2 + m_osu->getSubTitleFont()->getHeight()));
		g->drawString(m_osu->getSubTitleFont(), errorMessage1);
		g->popTransform();

		g->setColor(0xff00ff00);
		g->pushTransform();
		g->translate((int)(m_osu->getScreenWidth()/2 - m_osu->getSubTitleFont()->getStringWidth(errorMessage2)/2), (int)(m_osu->getScreenHeight()/2 + m_osu->getSubTitleFont()->getHeight()*2 + 15));
		g->drawString(m_osu->getSubTitleFont(), errorMessage2);
		g->popTransform();
	}

	// click pulse animation overlay
	if (m_fPulseAnimation > 0.0f)
	{
		Color topColor = 0x00ffffff;
		Color bottomColor = COLOR((int)(25*m_fPulseAnimation), 255, 255, 255);

		g->fillGradient(0, 0, m_osu->getScreenWidth(), m_osu->getScreenHeight(), topColor, topColor, bottomColor, bottomColor);
	}

	// debug previous random beatmap
	/*
	g->setColor(0xffffffff);
	g->pushTransform();
	g->translate(m_osu->getScreenWidth()/5, m_osu->getScreenHeight()/3);
	for (int i=0; i<m_previousRandomBeatmaps.size(); i++)
	{
		UString message = UString::format("#%i = ", i);
		message.append(m_previousRandomBeatmaps[i]->getTitle());

		g->drawString(m_osu->getSongBrowserFont(), message);
		g->translate(0, m_osu->getSongBrowserFont()->getHeight()+10);
	}
	g->popTransform();
	*/
}

void OsuSongBrowser2::drawSelectedBeatmapBackgroundImage(Graphics *g, Osu *osu)
{
	if (osu->getSelectedBeatmap() != NULL && osu->getSelectedBeatmap()->getSelectedDifficulty() != NULL)
	{
		Image *backgroundImage = osu->getSelectedBeatmap()->getSelectedDifficulty()->backgroundImage;
		if (backgroundImage != NULL)
		{
			float scale = Osu::getImageScaleToFillResolution(backgroundImage, osu->getScreenSize());

			g->setColor(0xff999999);
			g->pushTransform();
			g->scale(scale, scale);
			g->translate(osu->getScreenWidth()/2, osu->getScreenHeight()/2);
			g->drawImage(backgroundImage);
			g->popTransform();
		}
	}
}

void OsuSongBrowser2::update()
{
	OsuScreenBackable::update();
	if (!m_bVisible) return;

	// refresh logic (blocks every other call in the update() function below it!)
	if (m_bBeatmapRefreshScheduled)
	{
		Timer t;
		t.start();

		if (m_bBeatmapRefreshNeedsFileRefresh)
		{
			m_bBeatmapRefreshNeedsFileRefresh = false;
			refreshBeatmaps();
		}

		while (m_bBeatmapRefreshScheduled && t.getElapsedTime() < 0.033f)
		{
			if (m_refreshBeatmaps.size() > 0)
			{
				UString fullBeatmapPath = m_sCurRefreshOsuSongFolder;
				fullBeatmapPath.append(m_refreshBeatmaps[m_iCurRefreshBeatmap++]);
				fullBeatmapPath.append("/");

				OsuBeatmap *bm = new OsuBeatmap(m_osu, fullBeatmapPath);

				// if the beatmap is valid, add it
				if (bm->load())
				{
					OsuUISongBrowserButton *songButton = new OsuUISongBrowserButton(bm, m_songBrowser, this, 250, 250 + m_beatmaps.size()*50, 200, 50, "");

					m_songBrowser->getContainer()->addBaseUIElement(songButton);
					m_songButtons.push_back(songButton);
					m_visibleSongButtons.push_back(songButton);
					m_beatmaps.push_back(bm);
				}
				else
				{
					if (Osu::debug->getBool())
						debugLog("Couldn't load(), deleting object.\n");
					SAFE_DELETE(bm);
				}
			}

			if (m_iCurRefreshBeatmap >= m_iCurRefreshNumBeatmaps)
			{
				m_bBeatmapRefreshScheduled = false;
				m_importTimer->update();
				debugLog("Refresh finished, added %i beatmaps in %f seconds.\n", m_beatmaps.size(), m_importTimer->getElapsedTime());
				updateLayout();
			}

			t.update();
		}
		return;
	}

	m_songBrowser->update();
	m_songBrowser->getContainer()->update_pos(); // necessary due to constant animations
	m_topbarLeft->update();
	m_topbarRight->update();
	m_bottombar->update();

	// handle right click absolute drag scrolling
	if (m_songBrowser->isMouseInside() && engine->getMouse()->isRightDown())
		m_songBrowser->scrollToY(-((engine->getMouse()->getPos().y - m_songBrowser->getPos().y)/m_songBrowser->getSize().y)*m_songBrowser->getScrollSize().y);

	// handle async random beatmap selection
	if (m_bRandomBeatmapScheduled)
	{
		m_bRandomBeatmapScheduled = false;
		selectRandomBeatmap();
	}
	if (m_bPreviousRandomBeatmapScheduled)
	{
		m_bPreviousRandomBeatmapScheduled = false;
		selectPreviousRandomBeatmap();
	}

	// if mouse is to the left, force center currently selected beatmap/diff
	if (engine->getMouse()->getPos().x < m_osu->getScreenWidth()*0.08f)
	{
		std::vector<CBaseUIElement*> *elements = m_songBrowser->getContainer()->getAllBaseUIElementsPointer();
		for (int i=0; i<elements->size(); i++)
		{
			OsuUISongBrowserButton *songButton = dynamic_cast<OsuUISongBrowserButton*>((*elements)[i]);
			if (songButton != NULL && songButton->isSelected())
			{
				scrollToSongButton(songButton);
				break;
			}
		}
	}

	// handle searching
	if (m_fSearchWaitTime != 0.0f && engine->getTime() > m_fSearchWaitTime)
	{
		m_fSearchWaitTime = 0.0f;

		// empty the container
		m_songBrowser->getContainer()->empty();

		// rebuild visible song buttons, scroll to top search result
		m_visibleSongButtons.clear();
		if (m_sSearchString.length() > 0)
		{
			// search for possible matches, add the children below the possibly visible currently selected song button (which owns them)
			for (int i=0; i<m_songButtons.size(); i++)
			{
				m_songButtons[i]->setVisible(false); // unload images

				if (searchMatcher(m_songButtons[i]->getBeatmap(), m_sSearchString))
				{
					m_visibleSongButtons.push_back(m_songButtons[i]);

					// add children again if the song button is the currently selected one
					if (m_songButtons[i]->getBeatmap() == m_selectedBeatmap)
					{
						std::vector<OsuUISongBrowserButton*> children = m_songButtons[i]->getChildren();
						for (int c=0; c<children.size(); c++)
						{
							m_songBrowser->getContainer()->addBaseUIElement(children[c]);
						}
					}
					else // add the parent
						m_songBrowser->getContainer()->addBaseUIElement(m_songButtons[i]);
				}
			}

			updateLayout();

			// scroll to top result
			if (m_visibleSongButtons.size() > 0)
				scrollToSongButton(m_visibleSongButtons[0]);
		}
		else
		{
			OsuUISongBrowserButton *selectedSongButton = NULL;

			for (int i=0; i<m_songButtons.size(); i++)
			{
				m_songButtons[i]->setVisible(false); // unload images

				// add children again if the song button is the currently selected one
				if (m_songButtons[i]->getBeatmap() == m_selectedBeatmap)
				{
					std::vector<OsuUISongBrowserButton*> children = m_songButtons[i]->getChildren();
					for (int c=0; c<children.size(); c++)
					{
						m_songBrowser->getContainer()->addBaseUIElement(children[c]);

						if (children[c]->isSelected())
							selectedSongButton = children[c];
					}
				}
				else // add the parent
					m_songBrowser->getContainer()->addBaseUIElement(m_songButtons[i]);
			}

			m_visibleSongButtons = m_songButtons;
			updateLayout();

			// scroll back to selection
			if (selectedSongButton != NULL)
				scrollToSongButton(selectedSongButton);
		}
	}
}

void OsuSongBrowser2::onKeyDown(KeyboardEvent &key)
{
	if (!m_bVisible) return;

	if (m_bVisible && m_bBeatmapRefreshScheduled && (key == KEY_ESCAPE || key == (KEYCODE)OsuKeyBindings::GAME_PAUSE.getInt()))
	{
		m_bBeatmapRefreshScheduled = false;
		updateLayout();
		key.consume();
		return;
	}
	if (m_bBeatmapRefreshScheduled)
		return;

	// searching text delete & escape key handling
	if (m_sSearchString.length() > 0)
	{
		switch (key.getKeyCode())
		{
		case KEY_DELETE:
		case KEY_BACKSPACE:
			if (m_sSearchString.length() > 0)
			{
				if (engine->getKeyboard()->isControlDown())
				{
					// delete everything from the current caret position to the left, until after the first non-space character (but including it)
					// TODO: use a CBaseUITextbox instead for the search box
					bool foundNonSpaceChar = false;
					while (m_sSearchString.length() > 0)
					{
						UString curChar = m_sSearchString.substr(m_sSearchString.length()-1, 1);

						if (foundNonSpaceChar && curChar.isWhitespaceOnly())
							break;

						if (!curChar.isWhitespaceOnly())
							foundNonSpaceChar = true;

						m_sSearchString.erase(m_sSearchString.length()-1, 1);
					}
				}
				else
					m_sSearchString = m_sSearchString.substr(0, m_sSearchString.length()-1);
				scheduleSearchUpdate(m_sSearchString.length() == 0);
			}
			break;
		case KEY_ESCAPE:
			m_sSearchString = "";
			scheduleSearchUpdate(true);
			break;
		}
	}
	else
	{
		if (key == KEY_ESCAPE)
			m_osu->toggleSongBrowser();
	}

	if (key == KEY_SHIFT)
		m_bShiftPressed = true;

	// function hotkeys
	if (key == KEY_F1 && !m_bF1Pressed)
	{
		m_bF1Pressed = true;
		m_bottombarNavButtons[0]->keyboardPulse();
		onSelectionMods();
	}
	if (key == KEY_F2 && !m_bF2Pressed)
	{
		m_bF2Pressed = true;
		m_bottombarNavButtons[1]->keyboardPulse();
		onSelectionRandom();
	}
	if (key == KEY_F3)
		onSelectionOptions();

	if (key == KEY_F5)
		refreshBeatmaps();

	// selection move
	if (!engine->getKeyboard()->isAltDown() && key == KEY_DOWN)
	{
		std::vector<CBaseUIElement*> *elements = m_songBrowser->getContainer()->getAllBaseUIElementsPointer();
		for (int i=0; i<elements->size(); i++)
		{
			OsuUISongBrowserButton *songButton = dynamic_cast<OsuUISongBrowserButton*>((*elements)[i]);
			if (songButton != NULL && songButton->isSelected())
			{
				int nextSelectionIndex = i+1;
				if (nextSelectionIndex > elements->size()-1)
					nextSelectionIndex = 0;

				if (nextSelectionIndex != i)
				{
					OsuUISongBrowserButton *nextSongButton = dynamic_cast<OsuUISongBrowserButton*>((*elements)[nextSelectionIndex]);
					if (!nextSongButton->isChild())
						nextSongButton->select(true, true);
					else
						nextSongButton->select(true);
				}
				break;
			}
		}
	}

	if (!engine->getKeyboard()->isAltDown() && key == KEY_UP)
	{
		std::vector<CBaseUIElement*> *elements = m_songBrowser->getContainer()->getAllBaseUIElementsPointer();
		for (int i=0; i<elements->size(); i++)
		{
			OsuUISongBrowserButton *songButton = dynamic_cast<OsuUISongBrowserButton*>((*elements)[i]);
			if (songButton != NULL && songButton->isSelected())
			{
				int nextSelectionIndex = i-1;
				if (nextSelectionIndex < 0)
					nextSelectionIndex = elements->size()-1;

				if (nextSelectionIndex != i)
				{
					OsuUISongBrowserButton *nextSongButton = dynamic_cast<OsuUISongBrowserButton*>((*elements)[nextSelectionIndex]);
					nextSongButton->select(true);
				}
				break;
			}
		}
	}

	if (key == KEY_LEFT && !m_bLeft)
	{
		m_bLeft = true;
		std::vector<CBaseUIElement*> *elements = m_songBrowser->getContainer()->getAllBaseUIElementsPointer();
		bool foundSelected = false;
		for (int i=elements->size()-1; i>=0; i--)
		{
			OsuUISongBrowserButton *songButton = dynamic_cast<OsuUISongBrowserButton*>((*elements)[i]);

			if (foundSelected && !songButton->isChild())
			{
				songButton->select(true);
				break;
			}

			if (songButton != NULL && songButton->isSelected())
				foundSelected = true;
		}
	}

	if (key == KEY_RIGHT && !m_bRight)
	{
		m_bRight = true;
		std::vector<CBaseUIElement*> *elements = m_songBrowser->getContainer()->getAllBaseUIElementsPointer();
		bool foundSelected = false;
		for (int i=0; i<elements->size(); i++)
		{
			OsuUISongBrowserButton *songButton = dynamic_cast<OsuUISongBrowserButton*>((*elements)[i]);

			if (foundSelected && !songButton->isChild())
			{
				songButton->select(true);
				break;
			}

			if (songButton != NULL && songButton->isSelected())
				foundSelected = true;
		}
	}

	// selection select
	if (key == KEY_ENTER)
	{
		std::vector<CBaseUIElement*> *elements = m_songBrowser->getContainer()->getAllBaseUIElementsPointer();
		for (int i=0; i<elements->size(); i++)
		{
			OsuUISongBrowserButton *songButton = dynamic_cast<OsuUISongBrowserButton*>((*elements)[i]);
			if (songButton != NULL && songButton->isSelected())
			{
				songButton->select(true);
				break;
			}
		}
	}

	key.consume();
}

void OsuSongBrowser2::onKeyUp(KeyboardEvent &key)
{
	if (key == KEY_SHIFT)
		m_bShiftPressed = false;
	if (key == KEY_LEFT)
		m_bLeft = false;
	if (key == KEY_RIGHT)
		m_bRight = false;

	if (key == KEY_F1)
		m_bF1Pressed = false;
	if (key == KEY_F2)
		m_bF2Pressed = false;
}

void OsuSongBrowser2::onChar(KeyboardEvent &e)
{
	if (e.getCharCode() < 32 || !m_bVisible || m_bBeatmapRefreshScheduled || (engine->getKeyboard()->isControlDown() && !engine->getKeyboard()->isAltDown()))
		return;

	// handle searching
	KEYCODE charCode = e.getCharCode();
	m_sSearchString.append(UString((wchar_t*)&charCode));

	scheduleSearchUpdate();
}

void OsuSongBrowser2::onLeftChange(bool down)
{
	if (!m_bVisible) return;
}

void OsuSongBrowser2::onRightChange(bool down)
{
	if (!m_bVisible) return;
}

void OsuSongBrowser2::onResolutionChange(Vector2 newResolution)
{
	OsuScreenBackable::onResolutionChange(newResolution);
}

void OsuSongBrowser2::onPlayEnd(bool quit)
{
	m_bHasSelectedAndIsPlaying = false;
}

void OsuSongBrowser2::setVisible(bool visible)
{
	m_bVisible = visible;
	m_bShiftPressed = false; // this seems to get stuck sometimes otherwise

	if (m_bVisible)
	{
		updateLayout();

		// we have to re-select the current beatmap to start playing music again
		if (m_selectedBeatmap != NULL)
			m_selectedBeatmap->select();

		m_bHasSelectedAndIsPlaying = false; // sanity

		// try another refresh, maybe the osu!folder has changed
		if (m_beatmaps.size() == 0)
			refreshBeatmaps();
	}
}

void OsuSongBrowser2::updateLayout()
{
	m_bottombarNavButtons[0]->setImageResourceName(m_osu->getSkin()->getSelectionMods()->getName());
	m_bottombarNavButtons[0]->setImageResourceNameOver(m_osu->getSkin()->getSelectionModsOver()->getName());
	m_bottombarNavButtons[1]->setImageResourceName(m_osu->getSkin()->getSelectionRandom()->getName());
	m_bottombarNavButtons[1]->setImageResourceNameOver(m_osu->getSkin()->getSelectionRandomOver()->getName());
	///m_bottombarNavButtons[2]->setImageResourceName(m_osu->getSkin()->getSelectionOptions()->getName());

	//************************************************************************************************************************************//

	OsuScreenBackable::updateLayout();

	const int margin = 5;

	// top bar
	m_fSongSelectTopScale = Osu::getImageScaleToFitResolution(m_osu->getSkin()->getSongSelectTop(), m_osu->getScreenSize());
	float songSelectTopHeightScaled = std::max(m_osu->getSkin()->getSongSelectTop()->getHeight()*m_fSongSelectTopScale, m_songInfo->getMinimumHeight()*1.5f + margin); // NOTE: the height is a heuristic here more or less
	m_fSongSelectTopScale = std::max(m_fSongSelectTopScale, songSelectTopHeightScaled / m_osu->getSkin()->getSongSelectTop()->getHeight());

	// topbar left
	m_topbarLeft->setSize(std::max(m_osu->getSkin()->getSongSelectTop()->getWidth()*m_fSongSelectTopScale*osu_songbrowser_topbar_left_width_percent.getFloat() + margin, m_songInfo->getMinimumWidth() + margin), std::max(m_osu->getSkin()->getSongSelectTop()->getHeight()*m_fSongSelectTopScale*osu_songbrowser_topbar_left_percent.getFloat(), m_songInfo->getMinimumHeight() + margin));
	m_songInfo->setRelPos(margin, margin);
	m_songInfo->setSize(m_topbarLeft->getSize().x - margin, std::max(m_topbarLeft->getSize().y*0.65f - margin, m_songInfo->getMinimumHeight()));

	// topbar right
	m_topbarRight->setPosX(m_topbarLeft->getPos().x + m_topbarLeft->getSize().x + margin);
	m_topbarRight->setSize(m_osu->getScreenWidth() - m_topbarRight->getPos().x, m_osu->getSkin()->getSongSelectTop()->getHeight()*m_fSongSelectTopScale*osu_songbrowser_topbar_right_percent.getFloat());

	// bottombar
	int bottomBarHeight = m_osu->getScreenHeight()*m_osu_songbrowser_bottombar_percent_ref->getFloat();

	m_bottombar->setPosY(m_osu->getScreenHeight() - bottomBarHeight);
	m_bottombar->setSize(m_osu->getScreenWidth(), bottomBarHeight);

	// nav bar
	float navBarStart = std::max(m_osu->getScreenWidth()*0.175f, m_backButton->getSize().x);
	if (m_osu->getScreenWidth()*0.175f < m_backButton->getSize().x + 25)
		navBarStart += m_backButton->getSize().x - m_osu->getScreenWidth()*0.175f;

	// bottombar cont
	for (int i=0; i<m_bottombarNavButtons.size(); i++)
	{
		m_bottombarNavButtons[i]->setSize(m_osu->getScreenWidth(), bottomBarHeight);
	}
	for (int i=0; i<m_bottombarNavButtons.size(); i++)
	{
		m_bottombarNavButtons[i]->setRelPosX(navBarStart + i*m_bottombarNavButtons[i]->getSize().x);
	}
	m_bottombar->update_pos();

	// song browser
	m_songBrowser->setPos(m_topbarRight->getPos().x, m_topbarRight->getPos().y + m_topbarRight->getSize().y + 2);
	m_songBrowser->setSize(m_osu->getScreenWidth() - m_topbarRight->getPos().x + 1, m_osu->getScreenHeight() - m_songBrowser->getPos().y - m_bottombar->getSize().y+2);

	// this rebuilds the entire songButton layout (songButtons in relation to others)
	std::vector<CBaseUIElement*> *elements = m_songBrowser->getContainer()->getAllBaseUIElementsPointer();
	int yCounter = m_songBrowser->getSize().y/2;
	bool isChild = false;
	for (int i=0; i<elements->size(); i++)
	{
		OsuUISongBrowserButton *songButton = dynamic_cast<OsuUISongBrowserButton*>((*elements)[i]);

		if (songButton != NULL)
		{
			if (songButton->isChild() || isChild)
				yCounter += songButton->getSize().y*0.1f;
			isChild = songButton->isChild();

			songButton->setRelPosY(yCounter);
			songButton->updateLayout();

			yCounter += songButton->getActualSize().y;
		}
	}
	m_songBrowser->setScrollSizeToContent(m_songBrowser->getSize().y/2);
}

void OsuSongBrowser2::scheduleSearchUpdate(bool immediately)
{
	m_fSearchWaitTime = engine->getTime() + (immediately ? 0.0f : 0.5f);
}

OsuUISelectionButton *OsuSongBrowser2::addBottombarNavButton()
{
	OsuUISelectionButton *btn = new OsuUISelectionButton("MISSING_TEXTURE", 0, 0, 0, 0, "");
	btn->setScaleToFit(true);
	m_bottombar->addBaseUIElement(btn);
	m_bottombarNavButtons.push_back(btn);
	return btn;
}

void OsuSongBrowser2::onBack()
{
	engine->getSound()->play(m_osu->getSkin()->getMenuClick());
	m_osu->toggleSongBrowser();
}

void OsuSongBrowser2::onSelectionMods()
{
	engine->getSound()->play(m_osu->getSkin()->getMenuClick());
	m_osu->toggleModSelection(m_bF1Pressed);
}

void OsuSongBrowser2::onSelectionRandom()
{
	engine->getSound()->play(m_osu->getSkin()->getMenuClick());
	if (m_bShiftPressed)
		m_bPreviousRandomBeatmapScheduled = true;
	else
		m_bRandomBeatmapScheduled = true;
}

void OsuSongBrowser2::onSelectionOptions()
{
	engine->getSound()->play(m_osu->getSkin()->getMenuClick());
}

void OsuSongBrowser2::onDifficultySelected(OsuBeatmap *beatmap, OsuBeatmapDifficulty *diff, bool fromClick, bool play)
{
	// remember it
	// if this is the last one, and we selected with a click, restart the memory
	if (fromClick && m_previousRandomBeatmaps.size() < 2)
		m_previousRandomBeatmaps.clear();
	if (beatmap != m_selectedBeatmap)
		m_previousRandomBeatmaps.push_back(beatmap);

	m_selectedBeatmap = beatmap;

	// update song info
	m_songInfo->setFromBeatmap(beatmap, diff);

	// start playing
	if (play)
	{
		m_osu->onBeforePlayStart();
		if (beatmap->play())
		{
			m_bHasSelectedAndIsPlaying = true;
			setVisible(false);

			m_osu->onPlayStart();
		}
	}

	// animate
	m_fPulseAnimation = 1.0f;
	anim->moveLinear(&m_fPulseAnimation, 0.0f, 0.55f, true);

	// notify mod selector (for BPM override slider, else we get inconsistent values)
	m_osu->getModSelector()->checkUpdateBPMSliderSlaves();
}

void OsuSongBrowser2::refreshBeatmaps()
{
	if (!m_bVisible || m_bHasSelectedAndIsPlaying)
		return;

	// reset
	m_bBeatmapRefreshNeedsFileRefresh = false;
	m_selectedBeatmap = NULL;

	// delete database
	for (int i=0; i<m_beatmaps.size(); i++)
	{
		delete m_beatmaps[i];
	}
	m_songBrowser->clear();
	m_songButtons.clear();
	m_visibleSongButtons.clear();
	m_beatmaps.clear();
	m_refreshBeatmaps.clear();
	m_previousRandomBeatmaps.clear();

	// load all subfolders
	UString osuSongFolder = convar->getConVarByName("osu_folder")->getString();
	m_sLastOsuFolder = osuSongFolder;
	osuSongFolder.append("Songs/");
	m_sCurRefreshOsuSongFolder = osuSongFolder;
	m_refreshBeatmaps = env->getFoldersInFolder(osuSongFolder);

	debugLog("Building beatmap database ...\n");
	debugLog("Found %i folders in osu! song folder.\n", m_refreshBeatmaps.size());
	m_iCurRefreshNumBeatmaps = m_refreshBeatmaps.size();
	m_iCurRefreshBeatmap = 0;

	// if there are more than 0 beatmaps, schedule a refresh
	if (m_refreshBeatmaps.size() > 0)
	{
		m_bBeatmapRefreshScheduled = true;
		m_importTimer->start();
	}
}

void OsuSongBrowser2::selectSongButton(OsuUISongBrowserButton *songButton)
{
	songButton->select();
}

void OsuSongBrowser2::selectRandomBeatmap()
{
	if (m_visibleSongButtons.size() < 1)
		return;

	// remember previous
	if (m_previousRandomBeatmaps.size() == 0 && m_selectedBeatmap != NULL)
		m_previousRandomBeatmaps.push_back(m_selectedBeatmap);

	int randomIndex = rand() % m_visibleSongButtons.size();
	OsuUISongBrowserButton *songButton = m_visibleSongButtons[randomIndex];
	selectSongButton(songButton);
}

void OsuSongBrowser2::selectPreviousRandomBeatmap()
{
	if (m_previousRandomBeatmaps.size() > 0)
	{
		if (m_previousRandomBeatmaps.size() > 1 && m_previousRandomBeatmaps[m_previousRandomBeatmaps.size()-1] == m_selectedBeatmap)
			m_previousRandomBeatmaps.pop_back(); // deletes the current beatmap which may also be at the top (so we don't switch to ourself)

		// select it, if we can find it (and remove it from memory)
		OsuBeatmap *previousRandomBeatmap = m_previousRandomBeatmaps.back();
		for (int i=0; i<m_visibleSongButtons.size(); i++)
		{
			if (m_visibleSongButtons[i]->getBeatmap() == previousRandomBeatmap)
			{
				m_previousRandomBeatmaps.pop_back();
				selectSongButton(m_visibleSongButtons[i]);
				break;
			}
		}
	}
}

void OsuSongBrowser2::scrollToSongButton(OsuUISongBrowserButton *songButton)
{
	m_songBrowser->scrollToY(-songButton->getRelPos().y + m_songBrowser->getSize().y/2 - songButton->getSize().y/2);
}

bool OsuSongBrowser2::searchMatcher(OsuBeatmap *beatmap, UString searchString)
{
	std::vector<OsuBeatmapDifficulty*> diffs = beatmap->getDifficulties();

	// intelligent search parser
	// all strings which are not expressions get appended with spaces between, then checked with one call to findSubstringInDifficulty()
	// the rest is interpreted
	// WARNING: this code is quite shitty. the order of the operators array does matter, because find() is used to detect their presence (and '=' would then break '<=' etc.)
	// TODO: write proper parser (or beatmap database)
	enum operatorId
	{
		EQ,
		LT,
		GT,
		LE,
		GE,
		NE
	};
	const std::vector<std::pair<UString, operatorId>> operators =
	{
		std::pair<UString, operatorId>("<=",LE),
		std::pair<UString, operatorId>(">=",GE),
		std::pair<UString, operatorId>("<", LT),
		std::pair<UString, operatorId>(">", GT),
		std::pair<UString, operatorId>("!=",NE),
		std::pair<UString, operatorId>("==",EQ),
		std::pair<UString, operatorId>("=", EQ),
	};

	enum keywordId
	{
		AR,
		CS,
		OD,
		HP,
		BPM
	};
	const std::vector<std::pair<UString, keywordId>> keywords =
	{
		std::pair<UString, keywordId>("ar", AR),
		std::pair<UString, keywordId>("cs", CS),
		std::pair<UString, keywordId>("od", OD),
		std::pair<UString, keywordId>("hp", HP),
		std::pair<UString, keywordId>("bpm",BPM)
	};

	// split search string into tokens
	// parse over all difficulties
	bool expressionMatches = false; // if any diff matched all expressions
	std::vector<UString> tokens = searchString.split(" ");
	std::vector<UString> literalSearchStrings;
	for (int d=0; d<diffs.size(); d++)
	{
		bool expressionsMatch = true; // if the current search string (meaning only the expressions in this case) matches the current difficulty

		for (int i=0; i<tokens.size(); i++)
		{
			//debugLog("token[%i] = %s\n", i, tokens[i].toUtf8());
			// determine token type, interpret expression
			bool expression = false;
			for (int o=0; o<operators.size(); o++)
			{
				if (tokens[i].find(operators[o].first) != -1)
				{
					// split expression into left and right parts (only accept singular expressions, things like "0<bpm<1" will not work with this)
					//debugLog("splitting by string %s\n", operators[o].first.toUtf8());
					std::vector<UString> values = tokens[i].split(operators[o].first);
					if (values.size() == 2 && values[0].length() > 0 && values[1].length() > 0)
					{
						//debugLog("lvalue = %s, rvalue = %s\n", values[0].toUtf8(), values[1].toUtf8());
						UString lvalue = values[0];
						float rvalue = values[1].toFloat(); // this must always be a number (at least, assume it is)

						// find lvalue keyword in array (only continue if keyword exists)
						for (int k=0; k<keywords.size(); k++)
						{
							if (keywords[k].first == lvalue)
							{
								expression = true;

								// we now have a valid expression: the keyword, the operator and the value

								// solve keyword
								float compareValue = 5.0f;
								switch (keywords[k].second)
								{
								case AR:
									compareValue = diffs[d]->AR;
									break;
								case CS:
									compareValue = diffs[d]->CS;
									break;
								case OD:
									compareValue = diffs[d]->OD;
									break;
								case HP:
									compareValue = diffs[d]->HP;
									break;
								case BPM:
									compareValue = diffs[d]->maxBPM;
									break;
								}

								// solve operator
								bool matches = false;
								switch (operators[o].second)
								{
								case LE:
									if (compareValue <= rvalue)
										matches = true;
									break;
								case GE:
									if (compareValue >= rvalue)
										matches = true;
									break;
								case LT:
									if (compareValue < rvalue)
										matches = true;
									break;
								case GT:
									if (compareValue > rvalue)
										matches = true;
									break;
								case NE:
									if (compareValue != rvalue)
										matches = true;
									break;
								case EQ:
									if (compareValue == rvalue)
										matches = true;
									break;
								}

								//debugLog("comparing %f %s %f (operatorId = %i) = %i\n", compareValue, operators[o].first.toUtf8(), rvalue, (int)operators[o].second, (int)matches);

								if (!matches) // if a single expression doesn't match, then the whole diff doesn't match
									expressionsMatch = false;

								break;
							}
						}
					}

					break;
				}
			}

			// if this is not an expression, add the token to the literalSearchStrings array
			if (!expression)
			{
				// only add it if it doesn't exist yet
				// this check is only necessary due to multiple redundant parser executions (one per diff!)
				bool exists = false;
				for (int l=0; l<literalSearchStrings.size(); l++)
				{
					if (literalSearchStrings[l] == tokens[i])
					{
						exists = true;
						break;
					}
				}
				if (!exists)
				{
					UString litAdd = tokens[i].trim();
					if (litAdd.length() > 0 && !litAdd.isWhitespaceOnly())
						literalSearchStrings.push_back(litAdd);
				}
			}
		}

		if (expressionsMatch) // as soon as one difficulty matches all expressions, we are done here
		{
			expressionMatches = true;
			break;
		}
	}
	if (!expressionMatches) // if no diff matched any expression, we can already stop here
		return false;

	// build literal search string from all parts
	UString literalSearchString;
	for (int i=0; i<literalSearchStrings.size(); i++)
	{
		literalSearchString.append(literalSearchStrings[i]);
		if (i < literalSearchStrings.size()-1)
			literalSearchString.append(" ");
	}

	// if we have a valid literal string to search, do that, else just return the expression match
	if (literalSearchString.length() > 0)
	{
		//debugLog("literalSearchString = %s\n", literalSearchString.toUtf8());
		for (int i=0; i<diffs.size(); i++)
		{
			if (findSubstringInDifficulty(diffs[i], literalSearchString))
				return true;
		}
		return false; // can't be matched anymore
	}
	else
		return expressionMatches;
}

bool OsuSongBrowser2::findSubstringInDifficulty(OsuBeatmapDifficulty *diff, UString searchString)
{
	std::string stdSearchString = searchString.toUtf8();

	if (diff->title.length() > 0)
	{
		std::string difficultySongTitle = diff->title.toUtf8();
		if (Osu::findIgnoreCase(difficultySongTitle, stdSearchString))
			return true;
	}

	if (diff->artist.length() > 0)
	{
		std::string difficultySongArtist = diff->artist.toUtf8();
		if (Osu::findIgnoreCase(difficultySongArtist, stdSearchString))
			return true;
	}

	if (diff->creator.length() > 0)
	{
		std::string difficultySongCreator = diff->creator.toUtf8();
		if (Osu::findIgnoreCase(difficultySongCreator, stdSearchString))
			return true;
	}

	if (diff->name.length() > 0)
	{
		std::string difficultyName = diff->name.toUtf8();
		if (Osu::findIgnoreCase(difficultyName, stdSearchString))
			return true;
	}

	if (diff->source.length() > 0)
	{
		std::string difficultySongSource = diff->source.toUtf8();
		if (Osu::findIgnoreCase(difficultySongSource, stdSearchString))
			return true;
	}

	if (diff->tags.length() > 0)
	{
		std::string difficultySongTags = diff->tags.toUtf8();
		if (Osu::findIgnoreCase(difficultySongTags, stdSearchString))
			return true;
	}

	return false;
}
