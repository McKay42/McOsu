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
#include "Keyboard.h"

#include "CBaseUIContainer.h"
#include "CBaseUIScrollView.h"
#include "CBaseUILabel.h"

#include "Osu.h"
#include "OsuIcons.h"
#include "OsuSkin.h"
#include "OsuReplay.h"
#include "OsuHUD.h"
#include "OsuOptionsMenu.h"
#include "OsuSongBrowser2.h"
#include "OsuModSelector.h"
#include "OsuTooltipOverlay.h"
#include "OsuMultiplayer.h"
#include "OsuDatabase.h"
#include "OsuDatabaseBeatmap.h"

#include "OsuUIContextMenu.h"
#include "OsuUISongBrowserScoreButton.h"
#include "OsuUISongBrowserUserButton.h"
#include "OsuUIUserStatsScreenLabel.h"

ConVar osu_ui_top_ranks_max("osu_ui_top_ranks_max", 200, FCVAR_NONE, "maximum number of displayed scores, to keep the ui/scrollbar manageable");



class OsuUserStatsScreenMenuButton : public CBaseUIButton
{
public:
	OsuUserStatsScreenMenuButton(float xPos = 0, float yPos = 0, float xSize = 0, float ySize = 0, UString name = "", UString text = "") : CBaseUIButton(xPos, yPos, xSize, ySize, name, text)
	{
	}

	virtual void drawText(Graphics *g)
	{
		// HACKHACK: force update string width/height to non-average line height for icon
		m_fStringWidth = m_font->getStringWidth(m_sText);
		m_fStringHeight = m_font->getStringHeight(m_sText);

		if (m_font != NULL && m_sText.length() > 0)
		{
			g->pushClipRect(McRect(m_vPos.x+1, m_vPos.y+1, m_vSize.x-1, m_vSize.y-1));
			{
				g->setColor(m_textColor);
				g->pushTransform();
				{
					g->translate((int)(m_vPos.x + m_vSize.x/2.0f - (m_fStringWidth/2.0f)), (int)(m_vPos.y + m_vSize.y/2.0f + (m_fStringHeight/2.0f)));
					g->drawString(m_font, m_sText);
				}
				g->popTransform();
			}
			g->popClipRect();
		}
	}
};

class OsuUserStatsScreenBackgroundPPRecalculator : public Resource
{
public:
	OsuUserStatsScreenBackgroundPPRecalculator(Osu *osu, UString userName, bool importLegacyScores) : Resource()
	{
		m_osu = osu;
		m_sUserName = userName;
		m_bImportLegacyScores = importLegacyScores;

		m_iNumScoresToRecalculate = 0;
		m_iNumScoresRecalculated = 0;
	}

	inline int getNumScoresToRecalculate() const {return m_iNumScoresToRecalculate.load();}
	inline int getNumScoresRecalculated() const {return m_iNumScoresRecalculated.load();}

private:
	virtual void init()
	{
		m_bReady = true;
	}

	virtual void initAsync()
	{
		std::unordered_map<std::string, std::vector<OsuDatabase::Score>> *scores = m_osu->getSongBrowser()->getDatabase()->getScores();

		// count number of scores to recalculate for UI
		size_t numScoresToRecalculate = 0;
		for (const auto &kv : *scores)
		{
			for (size_t i=0; i<kv.second.size(); i++)
			{
				const OsuDatabase::Score &score = kv.second[i];

				if ((!score.isLegacyScore || m_bImportLegacyScores) && score.playerName == m_sUserName)
					numScoresToRecalculate++;
			}
		}
		m_iNumScoresToRecalculate = numScoresToRecalculate;

		printf("PPRecalc will recalculate %i scores ...\n", (int)numScoresToRecalculate);

		// actually recalculate them
		for (auto &kv : *scores)
		{
			for (size_t i=0; i<kv.second.size(); i++)
			{
				OsuDatabase::Score &score = kv.second[i];

				if ((!score.isLegacyScore || m_bImportLegacyScores) && score.playerName == m_sUserName)
				{
					if (m_bInterrupted.load())
						break;
					if (score.md5hash.length() < 1)
						continue;

					// NOTE: avoid importing the same score twice
					if (m_bImportLegacyScores && score.isLegacyScore)
					{
						const std::vector<OsuDatabase::Score> &otherScores = (*scores)[score.md5hash];

						bool isScoreAlreadyImported = false;
						for (size_t s=0; s<otherScores.size(); s++)
						{
							if (score.isLegacyScoreEqualToImportedLegacyScore(otherScores[s]))
							{
								isScoreAlreadyImported = true;
								break;
							}
						}

						if (isScoreAlreadyImported)
							continue;
					}

					// 1) get matching beatmap from db
					OsuDatabaseBeatmap *diff2 = m_osu->getSongBrowser()->getDatabase()->getBeatmapDifficulty(score.md5hash);
					if (diff2 == NULL)
					{
						if (Osu::debug->getBool())
							printf("PPRecalc couldn't find %s\n", score.md5hash.c_str());

						continue;
					}

					// 1.5) reload metadata for sanity (maybe osu!.db has outdated AR/CS/OD/HP or some other shit)
					if (!OsuDatabaseBeatmap::loadMetadata(diff2))
						continue;

					const OsuReplay::BEATMAP_VALUES legacyValues = OsuReplay::getBeatmapValuesForModsLegacy(score.modsLegacy, diff2->getAR(), diff2->getCS(), diff2->getOD(), diff2->getHP());
					const UString &osuFilePath = diff2->getFilePath();
					const Osu::GAMEMODE gameMode = Osu::GAMEMODE::STD;
					const float AR = (score.isLegacyScore ? legacyValues.AR : score.AR);
					const float CS = (score.isLegacyScore ? legacyValues.CS : score.CS);
					const float OD = (score.isLegacyScore ? legacyValues.OD : score.OD);
					const float HP = (score.isLegacyScore ? legacyValues.HP : score.HP);
					const float speedMultiplier = (score.isLegacyScore ? legacyValues.speedMultiplier : score.speedMultiplier);

					// 2) load hitobjects for diffcalc
					OsuDatabaseBeatmap::LOAD_DIFFOBJ_RESULT diffres = OsuDatabaseBeatmap::loadDifficultyHitObjects(osuFilePath, gameMode, AR, CS, speedMultiplier);
					if (diffres.diffobjects.size() < 1)
					{
						if (Osu::debug->getBool())
							printf("PPRecalc couldn't load %s\n", osuFilePath.toUtf8());

						continue;
					}

					// 3) calculate stars
					OsuDifficultyCalculator::DifficultyAttributes attributes{};
					OsuDifficultyCalculator::BeatmapDiffcalcData beatmapData(diff2, score, diffres.diffobjects);
					const double totalStars = OsuDifficultyCalculator::calculateDifficultyAttributes(attributes, beatmapData);

					// 4) calculate pp
					double pp = 0.0;
					int numHitObjects = 0;
					int numSpinners = 0;
					int numCircles = 0;
					int numSliders = 0;
					int maxPossibleCombo = 0;
					{
						// calculate a few values fresh from the beatmap data necessary for pp calculation
						numHitObjects = diffres.diffobjects.size();

						for (size_t h=0; h<diffres.diffobjects.size(); h++)
						{
							if (diffres.diffobjects[h].type == OsuDifficultyHitObject::TYPE::CIRCLE)
								numCircles++;
							if (diffres.diffobjects[h].type == OsuDifficultyHitObject::TYPE::SLIDER)
								numSliders++;
							if (diffres.diffobjects[h].type == OsuDifficultyHitObject::TYPE::SPINNER)
								numSpinners++;
						}

						maxPossibleCombo = diffres.maxPossibleCombo;
						if (maxPossibleCombo < 1)
							continue;

						pp = OsuDifficultyCalculator::calculatePPv2((score.isLegacyScore || score.isImportedLegacyScore), score.version, score.modsLegacy, speedMultiplier, AR, OD, attributes, numHitObjects, numCircles, numSliders, numSpinners, maxPossibleCombo, score.comboMax, score.numMisses, score.num300s, score.num100s, score.num50s, score.score);
					}

					// 5) overwrite score with new pp data (and handle imports)
					const float oldPP = score.pp;
					if (pp > 0.0f)
					{
						score.pp = pp;
						score.version = (score.version < 20251214 && (score.modsLegacy & OsuReplay::ScoreV2) && !score.isLegacyScore && !score.isImportedLegacyScore ? score.version : OsuScore::VERSION); // NOTE: special case for older ScoreV2 McOsu scores, they need to keep their old version (see OsuDifficultyCalculator) for a fix related to how osu_slider_scorev2 was handled previously

						if (m_bImportLegacyScores && score.isLegacyScore)
						{
							score.isLegacyScore = false;		// convert to McOsu (pp) score
							score.isImportedLegacyScore = true;	// but remember that this score does not have all play data
							{
								score.numSliderBreaks = 0;
								score.unstableRate = 0.0f;
								score.hitErrorAvgMin = 0.0f;
								score.hitErrorAvgMax = 0.0f;
							}
							score.starsTomTotal = totalStars;
							score.starsTomAim = attributes.AimDifficulty;
							score.starsTomSpeed = attributes.SpeedDifficulty;
							score.speedMultiplier = speedMultiplier;
							score.CS = CS;
							score.AR = AR;
							score.OD = OD;
							score.HP = HP;
							score.maxPossibleCombo = maxPossibleCombo;
							score.numHitObjects = numHitObjects;
							score.numCircles = numCircles;
						}
					}

					m_iNumScoresRecalculated++;

					if (Osu::debug->getBool())
					{
						printf("[%s] original = %f, new = %f, delta = %f\n", score.md5hash.c_str(), oldPP, score.pp, (score.pp - oldPP));
						printf("at %i/%i\n", m_iNumScoresRecalculated.load(), m_iNumScoresToRecalculate.load());
					}
				}
			}

			if (m_bInterrupted.load())
				break;
		}

		m_bAsyncReady = true;
	}

	virtual void destroy() {;}

	Osu *m_osu;
	UString m_sUserName;
	bool m_bImportLegacyScores;

	std::atomic<int> m_iNumScoresToRecalculate;
	std::atomic<int> m_iNumScoresRecalculated;
};



OsuUserStatsScreen::OsuUserStatsScreen(Osu *osu) : OsuScreenBackable(osu)
{
	m_name_ref = convar->getConVarByName("name");

	m_container = new CBaseUIContainer();

	m_contextMenu = new OsuUIContextMenu(m_osu);
	m_contextMenu->setVisible(true);

	m_ppVersionInfoLabel = new OsuUIUserStatsScreenLabel(m_osu);
	m_ppVersionInfoLabel->setText(UString::format("pp Version: %i", OsuDifficultyCalculator::PP_ALGORITHM_VERSION));
	//m_ppVersionInfoLabel->setTooltipText("WARNING: McOsu's star/pp algorithm is currently lagging behind the \"official\" version.\n \nReason being that keeping up-to-date requires a LOT of changes now.\nThe next goal is rewriting the algorithm architecture to be more similar to osu!lazer,\nas that will make porting star/pp changes infinitely easier for the foreseeable future.\n \nNo promises as to when all of that will be finished.");
	m_ppVersionInfoLabel->setTooltipText("NOTE: This version number does NOT mean your scores have already been recalculated!\nNOTE: Click the gear button on the right and \"Recalculate pp\".\n \nThis version number reads as the year YYYY and then month MM and then day DD.\nThat date specifies when the last pp/star algorithm changes were done/released by peppy.\nMcOsu always uses the in-use-for-public-global-online-rankings algorithms if possible.");
	m_ppVersionInfoLabel->setTextColor(0x77888888/*0xbbbb0000*/);
	m_ppVersionInfoLabel->setDrawBackground(false);
	m_ppVersionInfoLabel->setDrawFrame(false);
	m_container->addBaseUIElement(m_ppVersionInfoLabel);

	m_userButton = new OsuUISongBrowserUserButton(m_osu);
	m_userButton->addTooltipLine("Click to change [User]");
	m_userButton->setClickCallback( fastdelegate::MakeDelegate(this, &OsuUserStatsScreen::onUserClicked) );
	m_container->addBaseUIElement(m_userButton);

	m_scores = new CBaseUIScrollView();
	m_scores->setBackgroundColor(0xff222222);
	m_scores->setHorizontalScrolling(false);
	m_scores->setVerticalScrolling(true);
	m_container->addBaseUIElement(m_scores);

	m_menuButton = new OsuUserStatsScreenMenuButton();
	m_menuButton->setFont(m_osu->getFontIcons());
	{
		UString iconString; iconString.insert(0, OsuIcons::GEAR);
		m_menuButton->setText(iconString);
	}
	m_menuButton->setClickCallback( fastdelegate::MakeDelegate(this, &OsuUserStatsScreen::onMenuClicked) );
	m_container->addBaseUIElement(m_menuButton);

	m_bRecalculatingPP = false;
	m_backgroundPPRecalculator = NULL;

	// TODO: (statistics panel with values (how many plays, average UR/offset+-/score/pp/etc.))
}

OsuUserStatsScreen::~OsuUserStatsScreen()
{
	if (m_backgroundPPRecalculator != NULL)
	{
		m_backgroundPPRecalculator->interruptLoad();
		engine->getResourceManager()->destroyResource(m_backgroundPPRecalculator);
		m_backgroundPPRecalculator = NULL;
	}

	SAFE_DELETE(m_container);
}

void OsuUserStatsScreen::draw(Graphics *g)
{
	if (!m_bVisible) return;

	if (m_bRecalculatingPP)
	{
		if (m_backgroundPPRecalculator != NULL)
		{
			const float numScoresToRecalculate = (float)m_backgroundPPRecalculator->getNumScoresToRecalculate();
			const float percentFinished = (numScoresToRecalculate > 0 ? (float)m_backgroundPPRecalculator->getNumScoresRecalculated() / numScoresToRecalculate : 0.0f);
			UString loadingMessage = UString::format("Recalculating scores ... (%i %%, %i/%i)", (int)(percentFinished*100.0f), m_backgroundPPRecalculator->getNumScoresRecalculated(), m_backgroundPPRecalculator->getNumScoresToRecalculate());

			g->setColor(0xffffffff);
			g->pushTransform();
			{
				g->translate((int)(m_osu->getScreenWidth()/2 - m_osu->getSubTitleFont()->getStringWidth(loadingMessage)/2), m_osu->getScreenHeight() - 15);
				g->drawString(m_osu->getSubTitleFont(), loadingMessage);
			}
			g->popTransform();
		}

		m_osu->getHUD()->drawBeatmapImportSpinner(g);

		OsuScreenBackable::draw(g);
		return;
	}

	m_container->draw(g);
	m_contextMenu->draw(g);

	OsuScreenBackable::draw(g);
}

void OsuUserStatsScreen::update()
{
	OsuScreenBackable::update();
	if (!m_bVisible) return;

	if (m_bRecalculatingPP)
	{
		// wait until recalc is finished
		if (m_backgroundPPRecalculator != NULL && m_backgroundPPRecalculator->isReady())
		{
			// force recalc + refresh UI
			m_osu->getSongBrowser()->getDatabase()->forceScoreUpdateOnNextCalculatePlayerStats();
			m_osu->getSongBrowser()->getDatabase()->forceScoresSaveOnNextShutdown();

			rebuildScoreButtons(m_name_ref->getString());

			m_bRecalculatingPP = false;
		}
		else
			return; // don't update rest of UI while recalcing
	}

	m_contextMenu->update();
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
	{
		rebuildScoreButtons(m_name_ref->getString());

		// detect if any old pp scores exist, and notify the user if so
		// TODO: the detection would have to know if we were able to recalculate, so very annoying for anyone with any deleted beatmap but score
		/*
		{
			OsuDatabase *db = m_osu->getSongBrowser()->getDatabase();
			const std::vector<OsuDatabase::Score*> scores = db->getPlayerPPScores(m_name_ref->getString()).ppScores;

			bool foundOldScores;
			for (size_t i=0; i<scores.size(); i++)
			{
				//debugLog("version = %i\n", scores[i]->version);
				if (scores[i]->version < OsuScore::VERSION)
				{
					foundOldScores = true;
					break;
				}
			}

			if (foundOldScores)
				m_osu->getNotificationOverlay()->addNotification("Old pp scores detected. Please recalculate as soon as convenient!");
		}
		*/
	}
	else
		m_contextMenu->setVisible2(false);
}

void OsuUserStatsScreen::onScoreContextMenu(OsuUISongBrowserScoreButton *scoreButton, int id)
{
	// NOTE: see OsuUISongBrowserScoreButton::onContextMenu()

	if (id == 2)
		rebuildScoreButtons(m_name_ref->getString());
}

void OsuUserStatsScreen::onBack()
{
	if (m_bRecalculatingPP)
	{
		if (m_backgroundPPRecalculator != NULL)
			m_backgroundPPRecalculator->interruptLoad();
		else
			m_bRecalculatingPP = false; // this should never happen
	}
	else
	{
		engine->getSound()->play(m_osu->getSkin()->getMenuClick());
		m_osu->toggleSongBrowser();
	}
}

void OsuUserStatsScreen::rebuildScoreButtons(UString playerName)
{
	// since this score list can grow very large, UI elements are not cached, but rebuilt completely every time

	// hard reset (delete)
	m_scores->getContainer()->clear();
	m_scoreButtons.clear();

	OsuDatabase *db = m_osu->getSongBrowser()->getDatabase();
	std::vector<OsuDatabase::Score*> scores = db->getPlayerPPScores(playerName).ppScores;
	for (int i=scores.size()-1; i>=std::max(0, (int)scores.size() - osu_ui_top_ranks_max.getInt()); i--)
	{
		const float weight = OsuDatabase::getWeightForIndex(scores.size()-1-i);

		const OsuDatabaseBeatmap *diff = db->getBeatmapDifficulty(scores[i]->md5hash);

		UString title = "...";
		if (diff != NULL)
		{
			title = diff->getArtist();
			title.append(" - ");
			title.append(diff->getTitle());
			title.append(" [");
			title.append(diff->getDifficultyName());
			title.append("]");
		}

		OsuUISongBrowserScoreButton *button = new OsuUISongBrowserScoreButton(m_osu, m_contextMenu, 0, 0, 300, 100, UString(scores[i]->md5hash.c_str()), OsuUISongBrowserScoreButton::STYLE::TOP_RANKS);
		button->setScore(*scores[i], NULL, scores.size()-i, title, weight);
		button->setClickCallback( fastdelegate::MakeDelegate(this, &OsuUserStatsScreen::onScoreClicked) );

		m_scoreButtons.push_back(button);
		m_scores->getContainer()->addBaseUIElement(button);
	}

	m_userButton->setText(playerName);
	m_osu->getOptionsMenu()->setUsername(playerName); // NOTE: force update options textbox to avoid shutdown inconsistency
	m_userButton->updateUserStats();

	updateLayout();
}

void OsuUserStatsScreen::onUserClicked(CBaseUIButton *button)
{
	engine->getSound()->play(m_osu->getSkin()->getMenuClick());

	// NOTE: code duplication (see OsuSongbrowser2.cpp)
	std::vector<UString> names = m_osu->getSongBrowser()->getDatabase()->getPlayerNamesWithScoresForUserSwitcher();
	if (names.size() > 0)
	{
		m_contextMenu->setPos(m_userButton->getPos() + Vector2(0, m_userButton->getSize().y));
		m_contextMenu->setRelPos(m_userButton->getPos() + Vector2(0, m_userButton->getSize().y));
		m_contextMenu->begin(m_userButton->getSize().x);
		m_contextMenu->addButton("Switch User:", 0)->setTextColor(0xff888888)->setTextDarkColor(0xff000000)->setTextLeft(false)->setEnabled(false);
		//m_contextMenu->addButton("", 0)->setEnabled(false);
		for (int i=0; i<names.size(); i++)
		{
			CBaseUIButton *button = m_contextMenu->addButton(names[i]);
			if (names[i] == m_name_ref->getString())
				button->setTextBrightColor(0xff00ff00);
		}
		m_contextMenu->end(false, true);
		m_contextMenu->setClickCallback( fastdelegate::MakeDelegate(this, &OsuUserStatsScreen::onUserButtonChange) );
		OsuUIContextMenu::clampToRightScreenEdge(m_contextMenu);
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

void OsuUserStatsScreen::onMenuClicked(CBaseUIButton *button)
{
	m_contextMenu->setPos(m_menuButton->getPos() + Vector2(0, m_menuButton->getSize().y));
	m_contextMenu->setRelPos(m_menuButton->getPos() + Vector2(0, m_menuButton->getSize().y));
	m_contextMenu->begin();
	{
		m_contextMenu->addButton("Recalculate pp", 1);
		CBaseUIButton *spacer = m_contextMenu->addButton("---");
		spacer->setEnabled(false);
		spacer->setTextColor(0xff888888);
		spacer->setTextDarkColor(0xff000000);
		{
			UString importText = "Import osu! Scores of \"";
			importText.append(m_name_ref->getString());
			importText.append("\"");
			m_contextMenu->addButton(importText, 2);
		}
		spacer = m_contextMenu->addButton("-");
		spacer->setEnabled(false);
		spacer->setTextColor(0xff888888);
		spacer->setTextDarkColor(0xff000000);
		{
			UString copyText = "Copy All Scores from ...";
			m_contextMenu->addButton(copyText, 3);
		}
		spacer = m_contextMenu->addButton("---");
		spacer->setEnabled(false);
		spacer->setTextColor(0xff888888);
		spacer->setTextDarkColor(0xff000000);
		{
			UString deleteText = "Delete All Scores of \"";
			deleteText.append(m_name_ref->getString());
			deleteText.append("\"");
			m_contextMenu->addButton(deleteText, 4);
		}
	}
	m_contextMenu->end(false, false);
	m_contextMenu->setClickCallback( fastdelegate::MakeDelegate(this, &OsuUserStatsScreen::onMenuSelected) );
	OsuUIContextMenu::clampToRightScreenEdge(m_contextMenu);
}

void OsuUserStatsScreen::onMenuSelected(UString text, int id)
{
	if (id == 1)
		onRecalculatePP(false);
	else if (id == 2)
		onRecalculatePPImportLegacyScoresClicked();
	else if (id == 3)
		onCopyAllScoresClicked();
	else if (id == 4)
		onDeleteAllScoresClicked();
}

void OsuUserStatsScreen::onRecalculatePPImportLegacyScoresClicked()
{
	m_contextMenu->setPos(m_menuButton->getPos() + Vector2(0, m_menuButton->getSize().y));
	m_contextMenu->setRelPos(m_menuButton->getPos() + Vector2(0, m_menuButton->getSize().y));
	m_contextMenu->begin();
	{
		{
			UString reallyText = "Really import all osu! scores of \"";
			reallyText.append(m_name_ref->getString());
			reallyText.append("\"?");
			m_contextMenu->addButton(reallyText)->setEnabled(false);
		}
		{
			UString reallyText2 = "NOTE: You can NOT mix-and-match-import.";
			CBaseUIButton *reallyText2Button = m_contextMenu->addButton(reallyText2);
			reallyText2Button->setEnabled(false);
			reallyText2Button->setTextColor(0xff555555);
			reallyText2Button->setTextDarkColor(0xff000000);
		}
		{
			UString reallyText3 = "Only scores matching the EXACT username";
			CBaseUIButton *reallyText3Button = m_contextMenu->addButton(reallyText3);
			reallyText3Button->setEnabled(false);
			reallyText3Button->setTextColor(0xff555555);
			reallyText3Button->setTextDarkColor(0xff000000);

			UString reallyText4 = "of your currently selected profile are imported.";
			CBaseUIButton *reallyText4Button = m_contextMenu->addButton(reallyText4);
			reallyText4Button->setEnabled(false);
			reallyText4Button->setTextColor(0xff555555);
			reallyText4Button->setTextDarkColor(0xff000000);
		}
		CBaseUIButton *spacer = m_contextMenu->addButton("---");
		spacer->setTextLeft(false);
		spacer->setEnabled(false);
		spacer->setTextColor(0xff888888);
		spacer->setTextDarkColor(0xff000000);
		m_contextMenu->addButton("Yes", 1)->setTextLeft(false);
		m_contextMenu->addButton("No")->setTextLeft(false);
	}
	m_contextMenu->end(false, false);
	m_contextMenu->setClickCallback( fastdelegate::MakeDelegate(this, &OsuUserStatsScreen::onRecalculatePPImportLegacyScoresConfirmed) );
	OsuUIContextMenu::clampToRightScreenEdge(m_contextMenu);
}

void OsuUserStatsScreen::onRecalculatePPImportLegacyScoresConfirmed(UString text, int id)
{
	if (id != 1) return;

	onRecalculatePP(true);
}

void OsuUserStatsScreen::onRecalculatePP(bool importLegacyScores)
{
	m_bRecalculatingPP = true;

	if (m_backgroundPPRecalculator != NULL)
	{
		m_backgroundPPRecalculator->interruptLoad();
		engine->getResourceManager()->destroyResource(m_backgroundPPRecalculator);
		m_backgroundPPRecalculator = NULL;
	}

	m_backgroundPPRecalculator = new OsuUserStatsScreenBackgroundPPRecalculator(m_osu, m_name_ref->getString(), importLegacyScores);

	// NOTE: force disable all runtime mods (including all experimental mods!), as they directly influence global OsuGameRules which are used during pp calculation
	m_osu->getModSelector()->resetMods();

	engine->getResourceManager()->requestNextLoadAsync();
	engine->getResourceManager()->loadResource(m_backgroundPPRecalculator);
}

void OsuUserStatsScreen::onCopyAllScoresClicked()
{
	std::vector<UString> names = m_osu->getSongBrowser()->getDatabase()->getPlayerNamesWithPPScores();
	{
		// remove ourself
		for (size_t i=0; i<names.size(); i++)
		{
			if (names[i] == m_name_ref->getString())
			{
				names.erase(names.begin() + i);
				i--;
			}
		}
	}

	if (names.size() < 1)
	{
		m_osu->getNotificationOverlay()->addNotification("There are no valid users/scores to copy from.");
		return;
	}

	m_contextMenu->setPos(m_menuButton->getPos() + Vector2(0, m_menuButton->getSize().y));
	m_contextMenu->setRelPos(m_menuButton->getPos() + Vector2(0, m_menuButton->getSize().y));
	m_contextMenu->begin();
	{
		m_contextMenu->addButton("Select user to copy from:", 0)->setTextColor(0xff888888)->setTextDarkColor(0xff000000)->setTextLeft(false)->setEnabled(false);
		for (int i=0; i<names.size(); i++)
		{
			m_contextMenu->addButton(names[i]);
		}
	}
	m_contextMenu->end(false, true);
	m_contextMenu->setClickCallback( fastdelegate::MakeDelegate(this, &OsuUserStatsScreen::onCopyAllScoresUserSelected) );
	OsuUIContextMenu::clampToRightScreenEdge(m_contextMenu);
}

void OsuUserStatsScreen::onCopyAllScoresUserSelected(UString text, int id)
{
	m_sCopyAllScoresFromUser = text;

	m_contextMenu->setPos(m_menuButton->getPos() + Vector2(0, m_menuButton->getSize().y));
	m_contextMenu->setRelPos(m_menuButton->getPos() + Vector2(0, m_menuButton->getSize().y));
	m_contextMenu->begin();
	{
		{
			UString reallyText = "Really copy all scores from \"";
			reallyText.append(m_sCopyAllScoresFromUser);
			reallyText.append("\" into \"");
			reallyText.append(m_name_ref->getString());
			reallyText.append("\"?");
			m_contextMenu->addButton(reallyText)->setEnabled(false);
		}
		CBaseUIButton *spacer = m_contextMenu->addButton("---");
		spacer->setTextLeft(false);
		spacer->setEnabled(false);
		spacer->setTextColor(0xff888888);
		spacer->setTextDarkColor(0xff000000);
		m_contextMenu->addButton("Yes", 1)->setTextLeft(false);
		m_contextMenu->addButton("No")->setTextLeft(false);
	}
	m_contextMenu->end(false, false);
	m_contextMenu->setClickCallback( fastdelegate::MakeDelegate(this, &OsuUserStatsScreen::onCopyAllScoresConfirmed) );
	OsuUIContextMenu::clampToRightScreenEdge(m_contextMenu);
}

void OsuUserStatsScreen::onCopyAllScoresConfirmed(UString text, int id)
{
	if (id != 1) return;
	if (m_sCopyAllScoresFromUser.length() < 1) return;

	const UString &playerNameToCopyInto = m_name_ref->getString();

	if (playerNameToCopyInto.length() < 1 || m_sCopyAllScoresFromUser == playerNameToCopyInto) return;

	debugLog("Copying all scores from \"%s\" into \"%s\"\n", m_sCopyAllScoresFromUser.toUtf8(), playerNameToCopyInto.toUtf8());

	std::unordered_map<std::string, std::vector<OsuDatabase::Score>> *scores = m_osu->getSongBrowser()->getDatabase()->getScores();

	std::vector<OsuDatabase::Score> tempScoresToCopy;
	for (auto &kv : *scores)
	{
		tempScoresToCopy.clear();

		// collect all to-be-copied scores for this beatmap into tempScoresToCopy
		for (size_t i=0; i<kv.second.size(); i++)
		{
			const OsuDatabase::Score &existingScore = kv.second[i];

			// NOTE: only ever copy McOsu scores
			if (!existingScore.isLegacyScore)
			{
				if (existingScore.playerName == m_sCopyAllScoresFromUser)
				{
					// check if this user already has this exact same score (copied previously) and don't copy if that is the case
					bool alreadyCopied = false;
					for (size_t j=0; j<kv.second.size(); j++)
					{
						const OsuDatabase::Score &alreadyCopiedScore = kv.second[j];

						if (j == i) continue;

						if (!alreadyCopiedScore.isLegacyScore)
						{
							if (alreadyCopiedScore.playerName == playerNameToCopyInto)
							{
								if (existingScore.isScoreEqualToCopiedScoreIgnoringPlayerName(alreadyCopiedScore))
								{
									alreadyCopied = true;
									break;
								}
							}
						}
					}

					if (!alreadyCopied)
						tempScoresToCopy.push_back(existingScore);
				}
			}
		}

		// and copy them into the db
		if (tempScoresToCopy.size() > 0)
		{
			if (Osu::debug->getBool())
				debugLog("Copying %i for %s\n", (int)tempScoresToCopy.size(), kv.first.c_str());

			for (size_t i=0; i<tempScoresToCopy.size(); i++)
			{
				tempScoresToCopy[i].playerName = playerNameToCopyInto; // take ownership of this copied score
				tempScoresToCopy[i].sortHack = m_osu->getSongBrowser()->getDatabase()->getAndIncrementScoreSortHackCounter();

				kv.second.push_back(tempScoresToCopy[i]); // copy into db
			}
		}
	}

	// force recalc + refresh UI
	m_osu->getSongBrowser()->getDatabase()->forceScoreUpdateOnNextCalculatePlayerStats();
	m_osu->getSongBrowser()->getDatabase()->forceScoresSaveOnNextShutdown();

	rebuildScoreButtons(playerNameToCopyInto);
}

void OsuUserStatsScreen::onDeleteAllScoresClicked()
{
	m_contextMenu->setPos(m_menuButton->getPos() + Vector2(0, m_menuButton->getSize().y));
	m_contextMenu->setRelPos(m_menuButton->getPos() + Vector2(0, m_menuButton->getSize().y));
	m_contextMenu->begin();
	{
		{
			UString reallyText = "Really delete all scores of \"";
			reallyText.append(m_name_ref->getString());
			reallyText.append("\"?");
			m_contextMenu->addButton(reallyText)->setEnabled(false);
		}
		CBaseUIButton *spacer = m_contextMenu->addButton("---");
		spacer->setTextLeft(false);
		spacer->setEnabled(false);
		spacer->setTextColor(0xff888888);
		spacer->setTextDarkColor(0xff000000);
		m_contextMenu->addButton("Yes", 1)->setTextLeft(false);
		m_contextMenu->addButton("No")->setTextLeft(false);
	}
	m_contextMenu->end(false, false);
	m_contextMenu->setClickCallback( fastdelegate::MakeDelegate(this, &OsuUserStatsScreen::onDeleteAllScoresConfirmed) );
	OsuUIContextMenu::clampToRightScreenEdge(m_contextMenu);
}

void OsuUserStatsScreen::onDeleteAllScoresConfirmed(UString text, int id)
{
	if (id != 1) return;

	const UString &playerName = m_name_ref->getString();

	debugLog("Deleting all scores for \"%s\"\n", playerName.toUtf8());

	std::unordered_map<std::string, std::vector<OsuDatabase::Score>> *scores = m_osu->getSongBrowser()->getDatabase()->getScores();

	// delete every score matching the current playerName
	for (auto &kv : *scores)
	{
		for (size_t i=0; i<kv.second.size(); i++)
		{
			const OsuDatabase::Score &score = kv.second[i];

			// NOTE: only ever delete McOsu scores
			if (!score.isLegacyScore)
			{
				if (score.playerName == playerName)
				{
					kv.second.erase(kv.second.begin() + i);
					i--;
				}
			}
		}
	}

	// force recalc + refresh UI
	m_osu->getSongBrowser()->getDatabase()->forceScoreUpdateOnNextCalculatePlayerStats();
	m_osu->getSongBrowser()->getDatabase()->forceScoresSaveOnNextShutdown();

	rebuildScoreButtons(playerName);
}

void OsuUserStatsScreen::updateLayout()
{
	OsuScreenBackable::updateLayout();

	const float dpiScale = Osu::getUIScale(m_osu);

	m_container->setSize(m_osu->getScreenSize());

	const int scoreListHeight = m_osu->getScreenHeight()*0.8f;
	m_scores->setSize(m_osu->getScreenWidth()*0.6f, scoreListHeight);
	m_scores->setPos(m_osu->getScreenWidth()/2 - m_scores->getSize().x/2, m_osu->getScreenHeight() - scoreListHeight);

	const int margin = 5 * dpiScale;
	const int padding = 5 * dpiScale;

	// NOTE: these can't really be uiScale'd, because of the fixed aspect ratio
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

	m_menuButton->setSize(userButtonHeight*0.9f, userButtonHeight*0.9f);
	m_menuButton->setPos(std::max(m_userButton->getPos().x + m_userButton->getSize().x, m_userButton->getPos().x + m_userButton->getSize().x + (m_userButton->getPos().x - m_scores->getPos().x)/2 - m_menuButton->getSize().x/2), m_userButton->getPos().y + m_userButton->getSize().y/2 - m_menuButton->getSize().y/2);

	m_ppVersionInfoLabel->onResized(); // HACKHACK: framework bug (should update string metrics on setSizeToContent())
	m_ppVersionInfoLabel->setSizeToContent(1, 10);
	{
		const Vector2 center = m_userButton->getPos() + Vector2(0, m_userButton->getSize().y/2) - Vector2((m_userButton->getPos().x - m_scores->getPos().x)/2, 0);
		const Vector2 topLeft = center - m_ppVersionInfoLabel->getSize()/2;
		const float overflow = (center.x + m_ppVersionInfoLabel->getSize().x/2) - m_userButton->getPos().x;

		m_ppVersionInfoLabel->setPos((int)(topLeft.x - std::max(overflow, 0.0f)), (int)(topLeft.y));
	}
}

