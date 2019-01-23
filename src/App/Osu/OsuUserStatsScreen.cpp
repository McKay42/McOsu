//================ Copyright (c) 2019, PG, All rights reserved. =================//
//
// Purpose:		top plays list for weighted pp/acc
//
// $NoKeywords: $
//===============================================================================//

#include "OsuUserStatsScreen.h"

#include "Engine.h"
#include "ConVar.h"
#include "SoundEngine.h"
#include "ResourceManager.h"
#include "Mouse.h"

#include "CBaseUIContainer.h"
#include "CBaseUIScrollView.h"

#include "Osu.h"
#include "OsuSkin.h"
#include "OsuReplay.h"
#include "OsuOptionsMenu.h"
#include "OsuSongBrowser2.h"
#include "OsuMultiplayer.h"
#include "OsuBeatmapDifficulty.h"
#include "OsuDatabase.h"

#include "OsuUIContextMenu.h"
#include "OsuUISongBrowserScoreButton.h"
#include "OsuUISongBrowserUserButton.h"

OsuUserStatsScreen::OsuUserStatsScreen(Osu *osu) : OsuScreenBackable(osu)
{
	m_name_ref = convar->getConVarByName("name");

	// TODO: button to right of user panel in songbrowser <<< Top Ranks
	// TODO: (statistics panel with values (how many plays, average UR/offset+-/score/pp/etc.))

	m_container = new CBaseUIContainer();

	m_contextMenu = new OsuUIContextMenu(m_osu);
	m_contextMenu->setVisible(true);

	m_userButton = new OsuUISongBrowserUserButton(m_osu);
	m_userButton->addTooltipLine("Click to change [User]");
	m_userButton->setClickCallback( fastdelegate::MakeDelegate(this, &OsuUserStatsScreen::onUserClicked) );
	m_container->addBaseUIElement(m_userButton);

	m_scores = new CBaseUIScrollView();
	m_scores->setBackgroundColor(0xff222222);
	m_scores->setHorizontalScrolling(false);
	m_scores->setVerticalScrolling(true);
	m_container->addBaseUIElement(m_scores);

	// the context menu gets added last (drawn on top of everything)
	m_container->addBaseUIElement(m_contextMenu);
}

OsuUserStatsScreen::~OsuUserStatsScreen()
{
	SAFE_DELETE(m_container);
}

void OsuUserStatsScreen::draw(Graphics *g)
{
	if (!m_bVisible) return;

	/*
	const float infoScale = 0.1f;
	const float paddingScale = 0.4f;
	McFont *infoFont = m_osu->getSubTitleFont();
	g->setColor(0xff888888);
	for (int i=0; i<m_vInfoText.size(); i++)
	{
		g->pushTransform();
		{
			const float scale = (m_scores->getPos().y / infoFont->getHeight())*infoScale;
			const float height = infoFont->getHeight()*scale;
			const float padding = height*paddingScale;

			g->scale(scale, scale);
			g->translate(m_scores->getPos().x, m_scores->getPos().y/2 + height/2 - ((m_vInfoText.size()-1)*height + (m_vInfoText.size()-1)*padding)/2 + i*(height+padding));
			g->drawString(infoFont, m_vInfoText[i]);
		}
		g->popTransform();
	}
	*/

	m_container->draw(g);

	OsuScreenBackable::draw(g);
}

void OsuUserStatsScreen::update()
{
	OsuScreenBackable::update();
	if (!m_bVisible) return;

	m_container->update();

	if (m_contextMenu->isMouseInside())
		m_scores->stealFocus();

	if (m_osu->getOptionsMenu()->isMouseInside())
	{
		stealFocus();
		m_contextMenu->stealFocus();
		m_container->stealFocus();
	}

	updateLayout();
}

void OsuUserStatsScreen::setVisible(bool visible)
{
	OsuScreenBackable::setVisible(visible);

	if (m_bVisible)
		rebuildScoreButtons(m_name_ref->getString());
}

void OsuUserStatsScreen::onBack()
{
	engine->getSound()->play(m_osu->getSkin()->getMenuClick());
	m_osu->toggleSongBrowser();
}

void OsuUserStatsScreen::rebuildScoreButtons(UString playerName)
{
	// since this score list can grow very large, UI elements are not cached, but rebuilt completely every time

	// hard reset (delete)
	m_scores->getContainer()->clear();
	m_scoreButtons.clear();

	// TODO: optimize db accesses by caching a hashmap from md5hash -> OsuBeatmap*, currently it just does a loop over all diffs of all beatmaps (for every score here)
	OsuDatabase *db = m_osu->getSongBrowser()->getDatabase();
	std::vector<OsuDatabase::Score*> scores = db->getPlayerPPScores(playerName).ppScores;
	for (int i=scores.size()-1; i>=0; i--)
	{
		const float weight = OsuDatabase::getWeightForIndex(scores.size()-1-i);

		OsuBeatmapDifficulty *diff = db->getBeatmapDifficulty(scores[i]->md5hash);

		UString title = "...";
		if (diff != NULL)
		{
			title = diff->artist;
			title.append(" - ");
			title.append(diff->title);
			title.append(" [");
			title.append(diff->name);
			title.append("]");
		}

		OsuUISongBrowserScoreButton *button = new OsuUISongBrowserScoreButton(m_osu, NULL, 0, 0, 300, 100, "", OsuUISongBrowserScoreButton::STYLE::TOP_RANKS);
		button->setScore(*scores[i], scores.size()-i, title, weight);
		button->setClickCallback( fastdelegate::MakeDelegate(this, &OsuUserStatsScreen::onScoreClicked) );

		m_scoreButtons.push_back(button);
		m_scores->getContainer()->addBaseUIElement(button);
	}

	m_userButton->setText(playerName);
	m_osu->getOptionsMenu()->setUsername(playerName); // force update textbox to avoid shutdown inconsistency
	m_userButton->updateUserStats();

	updateLayout();
}

void OsuUserStatsScreen::onUserClicked(CBaseUIButton *button)
{
	engine->getSound()->play(m_osu->getSkin()->getMenuClick());

	// HACKHACK: code duplication (see OsuSongbrowser2.cpp)
	std::vector<UString> names = m_osu->getSongBrowser()->getDatabase()->getPlayerNamesWithPPScores();
	if (names.size() > 0)
	{
		m_contextMenu->setPos(m_userButton->getPos() + Vector2(0, m_userButton->getSize().y));
		m_contextMenu->setRelPos(m_userButton->getPos() + Vector2(0, m_userButton->getSize().y));
		m_contextMenu->begin(m_userButton->getSize().x);
		m_contextMenu->addButton("Switch User", 0)->setTextColor(0xff888888)->setTextLeft(false)->setEnabled(false);
		//m_contextMenu->addButton("", 0)->setEnabled(false);
		for (int i=0; i<names.size(); i++)
		{
			CBaseUIButton *button = m_contextMenu->addButton(names[i]);
			if (names[i] == m_name_ref->getString())
				button->setTextBrightColor(0xff00ff00);
		}
		m_contextMenu->end();
		m_contextMenu->setClickCallback( fastdelegate::MakeDelegate(this, &OsuUserStatsScreen::onUserButtonChange) );
	}
}

void OsuUserStatsScreen::onUserButtonChange(UString text, int id)
{
	if (id == 0) return;

	if (text != m_name_ref->getString())
	{
		m_name_ref->setValue(text);
		rebuildScoreButtons(m_name_ref->getString());
	}
}

void OsuUserStatsScreen::onScoreClicked(CBaseUIButton *button)
{
	m_osu->toggleSongBrowser();
	m_osu->getMultiplayer()->setBeatmap(((OsuUISongBrowserScoreButton*)button)->getScore().md5hash);
	m_osu->getSongBrowser()->highlightScore(((OsuUISongBrowserScoreButton*)button)->getScore().unixTimestamp);
}

void OsuUserStatsScreen::updateLayout()
{
	OsuScreenBackable::updateLayout();

	m_container->setSize(m_osu->getScreenSize());

	const int scoreListHeight = m_osu->getScreenHeight()*0.8f;
	m_scores->setSize(m_osu->getScreenWidth()*0.6f, scoreListHeight);
	m_scores->setPos(m_osu->getScreenWidth()/2 - m_scores->getSize().x/2, m_osu->getScreenHeight() - scoreListHeight);

	const int margin = 5;
	const int padding = 5;

	const int scoreButtonWidth = m_scores->getSize().x*0.92f - 2*margin;
	const int scoreButtonHeight = scoreButtonWidth*0.065f;
	for (int i=0; i<m_scoreButtons.size(); i++)
	{
		m_scoreButtons[i]->setSize(scoreButtonWidth - 2, scoreButtonHeight);
		m_scoreButtons[i]->setRelPos(margin, margin + i*(scoreButtonHeight + padding));
	}
	m_scores->getContainer()->update_pos();

	m_scores->setScrollSizeToContent();

	const int userButtonHeight = m_scores->getPos().y*0.6f;
	m_userButton->setSize(userButtonHeight*3.5f, userButtonHeight);
	m_userButton->setPos(m_osu->getScreenWidth()/2 - m_userButton->getSize().x/2, m_scores->getPos().y/2 - m_userButton->getSize().y/2);
}
