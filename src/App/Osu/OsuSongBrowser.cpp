//================ Copyright (c) 2015, PG, All rights reserved. =================//
//
// Purpose:		debug beatmap browser and selector
//
// $NoKeywords: $osusbdbg
//===============================================================================//

#include "OsuSongBrowser.h"

#include "Engine.h"
#include "ConVar.h"
#include "ResourceManager.h"
#include "Keyboard.h"
#include "Mouse.h"
#include "Timer.h"
#include "SoundEngine.h"
#include "AnimationHandler.h"

#include "CBaseUIContainer.h"
#include "CBaseUIImageButton.h"

#include "Osu.h"
#include "OsuHUD.h"
#include "OsuSkin.h"
#include "OsuBeatmap.h"
#include "OsuBeatmapDifficulty.h"
#include "OsuSongBrowser2.h"

#include "OsuUIBackButton.h"
#include "OsuUISelectionButton.h"

void DUMMY_OSU_REFRESH_BEATMAPS() {;}

ConVar osu_folder("osu_folder", UString("C:/Program Files (x86)/osu!/"));
ConVar osu_refresh_beatmaps("osu_refresh_beatmaps", DUMMY_OSU_REFRESH_BEATMAPS);

ConVar osu_delta("osu_delta", 0.004f);
ConVar osu_mul("osu_mul", 2.0f);

ConVar osu_songbrowser_bottombar_percent("osu_songbrowser_bottombar_percent", 0.116f);

OsuSongBrowser::OsuSongBrowser(Osu *osu) : OsuScreenBackable(osu)
{
	m_osu = osu;

	// convar callbacks
	osu_refresh_beatmaps.setCallback( MakeDelegate(this, &OsuSongBrowser::refreshBeatmaps) );

	// convar refs
	m_fps_max_ref = convar->getConVarByName("fps_max");

	// engine settings
	engine->getMouse()->addListener(this);

	// vars
	m_bHasSelectedAndIsPlaying = false;
	m_selectedBeatmap = NULL;

	m_fSongScrollPos = 0.0f;
	m_fSongScrollVelocity = 0.0f;
	m_songFont = engine->getResourceManager()->getFont("FONT_DEFAULT");
	m_fSongTextHeight = m_songFont->getHeight() + 10;
	m_fSongTextOffset = 15;

	m_bLeftMouse = false;
	m_bLeftMouseDown = false;
	m_bScrollGrab = false;
	m_bDisableClick = false;
	m_fPrevSongScrollPos = 0.0f;
	m_fSearchWaitTime = 0.0f;
	m_bF1Pressed = false;
	m_bF2Pressed = false;
	m_bShiftPressed = false;

	m_bRandomBeatmapScheduled = false;
	m_bPreviousRandomBeatmapScheduled = false;

	m_bBeatmapRefreshNeedsFileRefresh = true;
	m_iCurRefreshBeatmap = 0;
	m_iCurRefreshNumBeatmaps = 0;
	m_bBeatmapRefreshScheduled = true;

	// build bottombar
	m_bottombar = new CBaseUIContainer(0, 0, 0, 0, "");

	addBottombarNavButton()->setClickCallback( MakeDelegate(this, &OsuSongBrowser::onSelectionMods) );
	addBottombarNavButton()->setClickCallback( MakeDelegate(this, &OsuSongBrowser::onSelectionRandom) );
	///addBottombarNavButton()->setClickCallback( MakeDelegate(this, &OsuSongBrowser::onSelectionOptions) );
}

void OsuSongBrowser::onStartDebugMap()
{

	// force start a single beatmap for debugging
	m_selectedBeatmap = new OsuBeatmap(m_osu, "C:/Program Files (x86)/osu!/Songs/361159 CXu - Shoeshoeshoeshoeshoeshoeshoeshoeshoeshoe/");
	if (m_selectedBeatmap->load())
	{
		m_selectedBeatmap->select();
		m_selectedBeatmap->selectDifficulty(1);
		m_osu->onBeforePlayStart();
		if (m_selectedBeatmap->play())
		{
			m_bHasSelectedAndIsPlaying = true;
			setVisible(false);

			m_osu->onPlayStart();
		}
		else
			engine->showMessageError("Error", "Couldn't play beatmap :^(");
	}
	else
		engine->showMessageError("Error", "Couldn't load beatmap :^(");

}

OsuSongBrowser::~OsuSongBrowser()
{
	SAFE_DELETE(m_bottombar);
}

void OsuSongBrowser::draw(Graphics *g)
{
	if (!m_bVisible)
		return;

	// refreshing (blocks every other call in draw() below it!)
	if (m_bBeatmapRefreshScheduled)
	{
		g->setColor(0xffffffff);
		UString loadingMessage = UString::format("Loading beatmaps ... (%i %%)", (int)(((float)m_iCurRefreshBeatmap/(float)m_iCurRefreshNumBeatmaps)*100.0f));
		g->pushTransform();
		g->translate(m_osu->getScreenWidth()/2 - m_songFont->getStringWidth(loadingMessage)/2, m_osu->getScreenHeight() - 15);
		g->drawString(m_songFont, loadingMessage);
		g->popTransform();

		m_osu->getHUD()->drawBeatmapImportSpinner(g);
		return;
	}

	int browserHeight = m_osu->getScreenHeight()*(1.0f - osu_songbrowser_bottombar_percent.getFloat());

	if (m_visibleBeatmaps.size() > 0)
	{
		float maxHeight = m_visibleBeatmaps.size()*m_fSongTextHeight;

		int startIndex = (int)((m_fSongScrollPos / maxHeight) * m_visibleBeatmaps.size());
		float smoothOffset = -fmod(m_fSongScrollPos, m_fSongTextHeight);
		int numFittingSongsOnScreen = (int)((float)browserHeight/m_fSongTextHeight) + 2;

		float fontHeight = m_songFont->getHeight();
		g->enableClipRect();
			g->setClipRect(Rect(0, 0, m_osu->getScreenWidth()/2, browserHeight + 1));
			g->pushTransform();
			g->translate((int)m_fSongTextOffset, fontHeight);
				float heightAdd = 0.0f;
				for (int i=startIndex; i<startIndex+numFittingSongsOnScreen; i++)
				{
					if (i > -1 && i < m_visibleBeatmaps.size())
					{
						bool isCurrentlySelectedBeatmap = m_selectedBeatmap == m_visibleBeatmaps[i];
						Rect songRect(0, heightAdd + smoothOffset, m_osu->getScreenWidth()/2, m_fSongTextHeight-1);
						if (songRect.contains(engine->getMouse()->getPos()) || isCurrentlySelectedBeatmap)
						{
							if (isCurrentlySelectedBeatmap)
								g->setColor(COLOR((int)( std::abs(sin(engine->getTime()*3))*50 ) + 100, 0,255,0));
							else
								g->setColor(COLOR((int)( std::abs(sin(engine->getTime()*3))*50 ) + 100, 0,196,223));
							g->fillRect(-m_fSongTextOffset, -fontHeight + smoothOffset, songRect.getWidth(), songRect.getHeight()+1);

							g->setColor(0xffffffff);
						}

						// draw song text centered
						UString songText = m_visibleBeatmaps[i]->getTitle();
						songText.append(" - ");
						songText.append(m_visibleBeatmaps[i]->getArtist());
						g->pushTransform();
							g->translate(0, ((int)m_fSongTextHeight - fontHeight)/2 + (int)smoothOffset);
							g->setColor(0xffffffff);
							g->drawString(m_songFont, songText);
						g->popTransform();
					}

					g->translate(0, m_fSongTextHeight);
					heightAdd += m_fSongTextHeight;
				}
			g->popTransform();
		g->disableClipRect();

		if (m_selectedBeatmap != NULL)
		{
			g->enableClipRect();
				g->setClipRect(Rect(m_osu->getScreenWidth()/2, 0, m_osu->getScreenWidth()/2, browserHeight));
				g->pushTransform();
				g->translate(m_osu->getScreenWidth()/2 + (int)m_fSongTextOffset, m_songFont->getHeight() + (int)m_fSongTextOffset);
					float heightAdd = m_fSongTextOffset;
					for (int i=0; i<m_selectedBeatmap->getDifficulties().size(); i++)
					{
						bool isCurrentlySelectedDifficulty = m_selectedBeatmap->getSelectedDifficulty() == m_selectedBeatmap->getDifficulties()[i];
						Rect songRect(m_osu->getScreenWidth()/2 + 1, heightAdd, m_osu->getScreenWidth()/2 - 1, m_fSongTextHeight-1);
						if (songRect.contains(engine->getMouse()->getPos()) || isCurrentlySelectedDifficulty)
						{
							if (isCurrentlySelectedDifficulty)
								g->setColor(COLOR((int)( std::abs(sin(engine->getTime()*3))*50 ) + 100, 0,255,0));
							else
								g->setColor(COLOR((int)( std::abs(sin(engine->getTime()*3))*50 ) + 100, 0,196,223));
							g->fillRect(-m_fSongTextOffset, -m_songFont->getHeight(), songRect.getWidth(), songRect.getHeight()+1);

							g->setColor(0xffffffff);
						}

						// draw diff name centered
						g->pushTransform();
							g->translate(0, ((int)m_fSongTextHeight - (int)m_songFont->getHeight())/2);
							g->setColor(0xffffffff);
							g->drawString(m_songFont, m_selectedBeatmap->getDifficulties()[i]->name);
						g->popTransform();

						g->translate(0, m_fSongTextHeight);
						heightAdd += m_fSongTextHeight;
					}
				g->popTransform();
			g->disableClipRect();
		}

		g->setColor(0xffffffff);
		int width = 6;
		g->fillRect(m_osu->getScreenWidth()/2 - width/2, 0, width, browserHeight);
	}

	// draw bottom bar
	g->setColor(0xff315eed);
	g->fillRect(0, (int)(m_osu->getScreenHeight()*(1.0 - osu_songbrowser_bottombar_percent.getFloat())), m_osu->getScreenWidth(), 3);
	m_bottombar->draw(g);
	m_backButton->draw(g);

	// no beatmaps found (osu folder is probably invalid)
	if (m_beatmaps.size() == 0 && !m_bBeatmapRefreshScheduled)
	{
		g->setColor(0xffff0000);
		UString errorMessage1 = "Invalid osu! folder (or no beatmaps found): ";
		errorMessage1.append(m_sLastOsuFolder);
		UString errorMessage2 = "Go to Options -> osu!folder";
		g->pushTransform();
		g->translate((int)(m_osu->getScreenWidth()/2 - m_songFont->getStringWidth(errorMessage1)/2), (int)(m_osu->getScreenHeight()/2 + m_songFont->getHeight()));
		g->drawString(m_songFont, errorMessage1);
		g->popTransform();

		g->setColor(0xff00ff00);
		g->pushTransform();
		g->translate((int)(m_osu->getScreenWidth()/2 - m_osu->getSubTitleFont()->getStringWidth(errorMessage2)/2), (int)(m_osu->getScreenHeight()/2 + m_osu->getSubTitleFont()->getHeight()*2 + 15));
		g->drawString(m_osu->getSubTitleFont(), errorMessage2);
		g->popTransform();
	}

	// draw search text
	g->setColor(0xff00ff00);
	g->pushTransform();
	g->translate(m_osu->getScreenWidth()/2 + m_fSongTextOffset, browserHeight - m_fSongTextOffset);
	g->drawString(m_songFont, m_sSearchString);
	g->popTransform();

	// draw beatmap image
	if (m_selectedBeatmap != NULL && m_selectedBeatmap->getSelectedDifficulty() != NULL && m_selectedBeatmap->getSelectedDifficulty()->backgroundImage != NULL)
	{
		Image *img = m_selectedBeatmap->getSelectedDifficulty()->backgroundImage;
		float multiplier = 0.35f;
		float scale = Osu::getImageScaleToFitResolution(img, m_osu->getScreenSize()*multiplier);

		g->setColor(0xffffffff);
		g->pushTransform();
		g->scale(scale, scale);
		g->translate((img->getWidth()*scale)/2 + m_osu->getScreenWidth() - m_osu->getScreenWidth()*multiplier, (img->getHeight()*scale)/2 + m_osu->getScreenHeight() - m_osu->getScreenHeight()*multiplier);
		g->drawImage(m_selectedBeatmap->getSelectedDifficulty()->backgroundImage);
		g->popTransform();
	}

	// previous random beatmap debugging
	/*
	g->setColor(0xffffffff);
	g->pushTransform();
	g->translate(m_osu->getScreenWidth()/2 + 15, m_osu->getScreenHeight()/2);
	for (int i=0; i<m_previousRandomBeatmaps.size(); i++)
	{
		g->drawString(m_songFont, UString::format("#%i = %i", i, m_previousRandomBeatmaps[i]));
		g->translate(0, m_songFont->getHeight()+10);
	}
	g->popTransform();
	*/
}

void OsuSongBrowser::update()
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

		while (m_bBeatmapRefreshScheduled && t.getElapsedTime() < (1.0f/m_fps_max_ref->getFloat())*4)
		{
			if (m_refreshBeatmaps.size() > 0)
			{
				UString fullBeatmapPath = m_sCurRefreshOsuSongFolder;
				fullBeatmapPath.append(m_refreshBeatmaps[m_iCurRefreshBeatmap++]);
				fullBeatmapPath.append("/");

				OsuBeatmap *bm = new OsuBeatmap(m_osu, fullBeatmapPath);

				if (bm->load())
				{
					m_beatmaps.push_back(bm);
					m_visibleBeatmaps.push_back(bm);
				}
			}

			if (m_iCurRefreshBeatmap >= m_iCurRefreshNumBeatmaps)
			{
				m_bBeatmapRefreshScheduled = false;
				debugLog("Refresh finished, added %i beatmaps.\n", m_beatmaps.size());
			}

			t.update();
		}

		return;
	}

	m_bottombar->update();

	int browserHeight = m_osu->getScreenHeight()*(1.0f - osu_songbrowser_bottombar_percent.getFloat());

	// HACKHACK:
	float maxHeight = m_visibleBeatmaps.size()*m_fSongTextHeight;
	int numFittingSongsOnScreen = (int)((float)browserHeight/m_fSongTextHeight) + 1;
	m_fSongScrollPos = clamp<float>(m_fSongScrollPos, -(numFittingSongsOnScreen*m_fSongTextHeight)/2, maxHeight-(numFittingSongsOnScreen*m_fSongTextHeight)/2);

	// handle beatmap & difficulty selection logic
	if (m_bLeftMouseDown && !m_bLeftMouse && !m_bottombar->isBusy() && !m_backButton->isActive())
	{
		m_bLeftMouse = true;

		if (!m_bScrollGrab && engine->getMouse()->getPos().y < browserHeight && engine->getMouse()->getPos().x < m_osu->getScreenWidth()/2)
		{
			m_bScrollGrab = true;
			m_vScrollStartMousePos = engine->getMouse()->getPos();
			m_bDisableClick = false;
			anim->deleteExistingAnimation(&m_fSongScrollPos);
			m_fPrevSongScrollPos = m_fSongScrollPos;
		}
	}
	if (m_bScrollGrab) // scrolling
	{
		Vector2 scrollGrabDelta = engine->getMouse()->getPos() - m_vScrollStartMousePos;
		if (scrollGrabDelta.length() > 10)
			m_bDisableClick = true;

		m_fSongScrollPos = m_fPrevSongScrollPos - scrollGrabDelta.y;

		m_vKineticAverage += (engine->getMouse()->getPos() - m_vMouseBackup2);
		m_vKineticAverage /= 2.0f;
		m_vMouseBackup2 = engine->getMouse()->getPos();
	}
	if (!m_bLeftMouseDown && m_bLeftMouse)
	{
		if (!m_bDisableClick)
		{
			// beatmap selection
			if (m_visibleBeatmaps.size() > 0)
			{
				float maxHeight = m_visibleBeatmaps.size()*m_fSongTextHeight;

				int startIndex = (int)((m_fSongScrollPos / maxHeight) * m_visibleBeatmaps.size());
				float smoothOffset = -fmod(m_fSongScrollPos, m_fSongTextHeight);
				int numFittingSongsOnScreen = (int)((float)browserHeight/m_fSongTextHeight) + 1;

				float heightAdd = 0;
				for (int i=startIndex; i<startIndex+numFittingSongsOnScreen; i++)
				{
					if (i > -1 && i < m_visibleBeatmaps.size())
					{
						Rect songRect(0, heightAdd + smoothOffset, m_osu->getScreenWidth()/2, m_fSongTextHeight-1);
						if (songRect.contains(engine->getMouse()->getPos()))
						{
							engine->getSound()->play(m_osu->getSkin()->getMenuClick());

							selectBeatmap(m_visibleBeatmaps[i], true);
							break;
						}
					}
					heightAdd += m_fSongTextHeight;
				}
			}

			// difficulty selection
			if (m_selectedBeatmap != NULL)
			{
				float heightAdd = 0.0f;
				for (int i=0; i<m_selectedBeatmap->getDifficulties().size(); i++)
				{
					Rect songRect(m_osu->getScreenWidth()/2 + 1, m_fSongTextOffset + heightAdd, m_osu->getScreenWidth()/2 - 1, m_fSongTextHeight-1);
					if (songRect.contains(engine->getMouse()->getPos()))
					{
						engine->getSound()->play(m_osu->getSkin()->getMenuClick());

						int prevSelection = m_selectedBeatmap->getSelectedDifficultyIndex();
						if (i != prevSelection)
							m_selectedBeatmap->selectDifficulty(i);

						if (prevSelection == i)
						{
							m_osu->onBeforePlayStart();
							if (m_selectedBeatmap->play())
							{
								m_bHasSelectedAndIsPlaying = true;
								setVisible(false);

								m_osu->onPlayStart();
							}
							//else
							//	engine->showMessageError("RIP", "Couldn't play beatmap :^(");
						}

						break;
					}
					heightAdd += m_fSongTextHeight;
				}
			}
		}
		else if (m_bScrollGrab) // kinetic scrolling
		{
			float delta = convar->getConVarByName("ui_scrollview_kinetic_energy_multiplier")->getFloat()*m_vKineticAverage.y*(engine->getFrameTime() != 0.0f ? 1.0f/engine->getFrameTime() : 60.0f)/60.0f;
			delta *= osu_mul.getFloat();
			if (delta != 0.0f)
				anim->moveQuadOut(&m_fSongScrollPos, m_fSongScrollPos - delta, 0.35f, 0.0f, true);
		}

		m_bLeftMouse = false;
		m_bScrollGrab = false;
		m_bDisableClick = false;
	}

	// handle mouse wheel scrolling
	if (m_visibleBeatmaps.size() > 0)
	{
		if (!anim->isAnimating(&m_fSongScrollPos))
			m_fSongScrollVelocity = 0.0f;

		if (engine->getMouse()->getWheelDeltaVertical() != 0)
		{
			// direction swap
			if (sign<int>(engine->getMouse()->getWheelDeltaVertical()) == sign<float>(m_fSongScrollVelocity))
				m_fSongScrollVelocity = 0.0f;

			float maxHeight = m_visibleBeatmaps.size()*m_fSongTextHeight;
			int numFittingSongsOnScreen = (int)((float)m_osu->getScreenHeight()/m_fSongTextHeight);

			if (numFittingSongsOnScreen < m_visibleBeatmaps.size())
			{
				float nextScrollPos = m_fSongScrollPos;
				//nextScrollPos -= engine->getMouseWheelDelta()/2.0f;
				m_fSongScrollVelocity -= engine->getMouse()->getWheelDeltaVertical()/2.0f;
				nextScrollPos = clamp<float>(nextScrollPos+m_fSongScrollVelocity*0.5f, -numFittingSongsOnScreen*0.5f*m_fSongTextHeight, maxHeight - numFittingSongsOnScreen*0.5f*m_fSongTextHeight);

				anim->moveQuadOut(&m_fSongScrollPos, nextScrollPos, 0.3f, 0.0f, true);
			}
			else
				m_fSongScrollPos = 0.0f;
		}
	}

	// HACKHACK:
	m_fSongScrollPos = clamp<float>(m_fSongScrollPos, -(numFittingSongsOnScreen*m_fSongTextHeight)/2, maxHeight-(numFittingSongsOnScreen*m_fSongTextHeight)/2);

	// if mouse is to right, force center currently selected beatmap
	if (engine->getMouse()->getPos().x > m_osu->getScreenWidth()*0.92f && m_selectedBeatmap != NULL)
		scrollToBeatmap(m_selectedBeatmap);

	// handle searching
	if (m_fSearchWaitTime != 0.0f && engine->getTime() > m_fSearchWaitTime)
	{
		m_fSearchWaitTime = 0.0f;

		m_visibleBeatmaps.clear();
		if (m_sSearchString.length() > 0)
		{
			for (int i=0; i<m_beatmaps.size(); i++)
			{
				if (OsuSongBrowser2::searchMatcher(m_beatmaps[i], m_sSearchString))
					m_visibleBeatmaps.push_back(m_beatmaps[i]);
			}
			m_fSongScrollPos = 0;
		}
		else if (m_visibleBeatmaps.size() != m_beatmaps.size())
			m_visibleBeatmaps.insert(m_visibleBeatmaps.end(), m_beatmaps.begin(), m_beatmaps.end());
	}

	anim->moveQuadOut(&m_vPrevMousePos.y, engine->getMouse()->getPos().y, engine->getFrameTime()*3.0f, 0.0f, true);
	//m_vPrevMousePos = engine->getMousePos();

	// handle random beatmap selection
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
}

void OsuSongBrowser::onKeyDown(KeyboardEvent &key)
{
	if (m_bVisible && m_bBeatmapRefreshScheduled && key == KEY_ESCAPE)
	{
		m_bBeatmapRefreshScheduled = false;
		return;
	}

	if (!m_bVisible || m_bBeatmapRefreshScheduled)
		return;

	// searching
	if (m_sSearchString.length() > 0)
	{
		switch (key.getKeyCode())
		{
		case KEY_DELETE:
		case KEY_BACKSPACE:
			if (m_sSearchString.length() > 0)
			{
				m_sSearchString = m_sSearchString.substr(0, m_sSearchString.length()-1);
				scheduleSearchUpdate();
			}
			break;
		case KEY_ESCAPE:
			m_sSearchString = "";
			scheduleSearchUpdate();
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

	key.consume();
}

void OsuSongBrowser::onKeyUp(KeyboardEvent &key)
{
	if (key == KEY_SHIFT)
		m_bShiftPressed = false;

	if (key == KEY_F1)
		m_bF1Pressed = false;
	if (key == KEY_F2)
		m_bF2Pressed = false;
}

void OsuSongBrowser::onChar(KeyboardEvent &e)
{
	if (e.getCharCode() < 32 || !m_bVisible || m_bBeatmapRefreshScheduled)
		return;

	KEYCODE charCode = e.getCharCode();
	m_sSearchString.append(UString((wchar_t*)&charCode));

	scheduleSearchUpdate();
}

void OsuSongBrowser::onLeftChange(bool down)
{
	if (!m_bVisible) return;

	m_bLeftMouseDown = down;
}

void OsuSongBrowser::onRightChange(bool down)
{
	if (!m_bVisible) return;
}

void OsuSongBrowser::onResolutionChange(Vector2 newResolution)
{
	OsuScreenBackable::onResolutionChange(newResolution);
}

void OsuSongBrowser::selectBeatmap(OsuBeatmap *selectedBeatmap, bool isFromClick)
{
	// remember it
	// if this is the last one, and we selected with a click, restart the memory
	if (isFromClick && m_previousRandomBeatmaps.size() == 1)
		m_previousRandomBeatmaps.clear();

	if (selectedBeatmap == m_selectedBeatmap)
		return;

	for (int i=0; i<m_beatmaps.size(); i++)
	{
		if (m_beatmaps[i] == selectedBeatmap)
		{
			m_previousRandomBeatmaps.push_back(i);
			break;
		}
	}

	if (m_selectedBeatmap != NULL)
		m_selectedBeatmap->deselect();
	m_selectedBeatmap = selectedBeatmap;
	m_selectedBeatmap->select();
}

void OsuSongBrowser::scheduleSearchUpdate()
{
	m_fSearchWaitTime = engine->getTime() + 0.5f;
}

void OsuSongBrowser::selectRandomBeatmap()
{
	if (m_visibleBeatmaps.size() == 0)
		return;

	int randomIndex = rand() % m_visibleBeatmaps.size();
	selectBeatmap(m_visibleBeatmaps[randomIndex]);
	scrollToBeatmap(m_visibleBeatmaps[randomIndex]);
}

void OsuSongBrowser::selectPreviousRandomBeatmap()
{
	if (m_previousRandomBeatmaps.size() > 0)
	{
		if (m_previousRandomBeatmaps.size() > 1 && m_beatmaps[m_previousRandomBeatmaps[m_previousRandomBeatmaps.size()-1]] == m_selectedBeatmap)
			m_previousRandomBeatmaps.pop_back(); // deletes the current beatmap which may also be at the top (so we don't switch to ourselves)

		// select it
		int previousRandomIndex = m_previousRandomBeatmaps.back();
		m_previousRandomBeatmaps.pop_back();
		selectBeatmap(m_beatmaps[previousRandomIndex]);
		scrollToBeatmap(m_beatmaps[previousRandomIndex]);
	}
}

void OsuSongBrowser::scrollToBeatmap(OsuBeatmap *beatmap)
{
	int visibleIndex = -1;
	for (int i=0; i<m_visibleBeatmaps.size(); i++)
	{
		if (m_visibleBeatmaps[i] == beatmap)
		{
			visibleIndex = i;
			break;
		}
	}

	if (visibleIndex != -1)
	{
		anim->moveQuadOut(&m_fSongScrollPos, (m_fSongTextHeight*(visibleIndex+1)) - m_osu->getScreenHeight()/2, 0.20f, 0.0f, true);
		//m_fSongScrollPos = (m_fSongTextHeight*(visibleIndex+1)) - m_osu->getScreenHeight()/2;
	}
}

void OsuSongBrowser::refreshBeatmaps()
{
	if (!m_bVisible || m_bHasSelectedAndIsPlaying)
		return;

	m_bBeatmapRefreshNeedsFileRefresh = false;
	m_fSongScrollPos = 0.0f;
	m_selectedBeatmap = NULL;

	for (int i=0; i<m_beatmaps.size(); i++)
	{
		delete m_beatmaps[i];
	}
	m_beatmaps.clear();
	m_visibleBeatmaps.clear();
	m_refreshBeatmaps.clear();
	m_previousRandomBeatmaps.clear();

	UString osuSongFolder = osu_folder.getString();
	m_sLastOsuFolder = osuSongFolder;
	osuSongFolder.append("Songs/");
	m_sCurRefreshOsuSongFolder = osuSongFolder;
	//osuSongFolder.append("*.*");

	m_refreshBeatmaps = env->getFoldersInFolder(osuSongFolder);
	debugLog("Building beatmap database ...\n");
	debugLog("Found %i folders in osu! song folder.\n", m_refreshBeatmaps.size());
	m_iCurRefreshNumBeatmaps = m_refreshBeatmaps.size();
	m_iCurRefreshBeatmap = 0;

	/*
	for (int i=0; i<m_refreshBeatmaps.size(); i++)
	{
		debugLog("#%i = %s\n", i, m_refreshBeatmaps[i].toUtf8());
	}
	*/

	if (m_refreshBeatmaps.size() > 0)
		m_bBeatmapRefreshScheduled = true;
}

void OsuSongBrowser::setVisible(bool visible)
{
	m_bVisible = visible;

	// HACKHACK: sometimes this seems to get stuck
	m_bShiftPressed = false;

	if (visible)
	{
		updateLayout();

		// we have to re-select the current beatmap to start playing the music again
		if (m_bHasSelectedAndIsPlaying)
			m_selectedBeatmap->select();

		m_bHasSelectedAndIsPlaying = false;

		// try another refresh, maybe the osu!folder has changed
		if (m_beatmaps.size() == 0)
			refreshBeatmaps();
	}
}

void OsuSongBrowser::updateLayout()
{
	m_bottombarNavButtons[0]->setImageResourceName(m_osu->getSkin()->getSelectionMods()->getName());
	m_bottombarNavButtons[0]->setImageResourceNameOver(m_osu->getSkin()->getSelectionModsOver()->getName());
	m_bottombarNavButtons[1]->setImageResourceName(m_osu->getSkin()->getSelectionRandom()->getName());
	m_bottombarNavButtons[1]->setImageResourceNameOver(m_osu->getSkin()->getSelectionRandomOver()->getName());
	///m_bottombarNavButtons[2]->setImageResourceName(m_osu->getSkin()->getSelectionOptions()->getName());

	//************************************************************************************************************************************//

	OsuScreenBackable::updateLayout();

	int height = m_osu->getScreenHeight()*osu_songbrowser_bottombar_percent.getFloat();

	m_bottombar->setPosY(m_osu->getScreenHeight()*(1.0 - osu_songbrowser_bottombar_percent.getFloat()));
	m_bottombar->setSize(m_osu->getScreenWidth(), height);

	// nav bar
	float navBarStart = std::max(m_osu->getScreenWidth()*0.175f, m_backButton->getSize().x);
	if (m_osu->getScreenWidth()*0.175f < m_backButton->getSize().x + 25)
		navBarStart += m_backButton->getSize().x - m_osu->getScreenWidth()*0.175f;

	for (int i=0; i<m_bottombarNavButtons.size(); i++)
	{
		m_bottombarNavButtons[i]->setSize(m_osu->getScreenWidth(), height);
	}
	for (int i=0; i<m_bottombarNavButtons.size(); i++)
	{
		m_bottombarNavButtons[i]->setRelPosX(navBarStart + i*m_bottombarNavButtons[i]->getSize().x);
	}

	m_bottombar->update_pos();
}

OsuUISelectionButton *OsuSongBrowser::addBottombarNavButton()
{
	OsuUISelectionButton *btn = new OsuUISelectionButton("MISSING_TEXTURE", 0, 0, 0, 0, "");
	btn->setScaleToFit(true);
	m_bottombar->addBaseUIElement(btn);
	m_bottombarNavButtons.push_back(btn);
	return btn;
}

void OsuSongBrowser::onBack()
{
	engine->getSound()->play(m_osu->getSkin()->getMenuClick());
	m_osu->toggleSongBrowser();
}

void OsuSongBrowser::onSelectionMods()
{
	engine->getSound()->play(m_osu->getSkin()->getMenuClick());
	m_osu->toggleModSelection(m_bF1Pressed);
}

void OsuSongBrowser::onSelectionRandom()
{
	engine->getSound()->play(m_osu->getSkin()->getMenuClick());
	if (m_bShiftPressed)
		m_bPreviousRandomBeatmapScheduled = true;
	else
		m_bRandomBeatmapScheduled = true;
}

void OsuSongBrowser::onSelectionOptions()
{
	engine->getSound()->play(m_osu->getSkin()->getMenuClick());
}
