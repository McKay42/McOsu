//================ Copyright (c) 2018, PG, All rights reserved. =================//
//
// Purpose:		clickable button displaying score, grade, name, acc, mods, combo
//
// $NoKeywords: $
//===============================================================================//

#include "OsuUISongBrowserScoreButton.h"

#include "Engine.h"
#include "ResourceManager.h"
#include "AnimationHandler.h"
#include "SoundEngine.h"
#include "ConVar.h"
#include "Mouse.h"

#include "Osu.h"
#include "OsuIcons.h"
#include "OsuSkin.h"
#include "OsuSkinImage.h"
#include "OsuReplay.h"
#include "OsuTooltipOverlay.h"

#include "OsuSongBrowser2.h"
#include "OsuDatabase.h"
#include "OsuBeatmap.h"
#include "OsuBeatmapDifficulty.h"

#include "OsuUIContextMenu.h"

#include <chrono>

ConVar *OsuUISongBrowserScoreButton::m_osu_scores_sort_by_pp = NULL;
UString OsuUISongBrowserScoreButton::recentScoreIconString;

OsuUISongBrowserScoreButton::OsuUISongBrowserScoreButton(Osu *osu, OsuUIContextMenu *contextMenu, float xPos, float yPos, float xSize, float ySize, UString name, STYLE style) : CBaseUIButton(xPos, yPos, xSize, ySize, name, "")
{
	m_osu = osu;
	m_contextMenu = contextMenu;
	m_style = style;

	if (m_osu_scores_sort_by_pp == NULL)
		m_osu_scores_sort_by_pp = convar->getConVarByName("osu_scores_sort_by_pp");

	if (recentScoreIconString.length() < 1)
		recentScoreIconString.insert(0, OsuIcons::ARROW_CIRCLE_UP);

	m_fIndexNumberAnim = 0.0f;
	m_bIsPulseAnim = false;

	m_bRightClick = false;
	m_bRightClickCheck = false;

	m_iScoreIndexNumber = 1;
	m_iScoreUnixTimestamp = 0;

	m_scoreGrade = OsuScore::GRADE::GRADE_D;
	//m_sScoreUsername = "McKay";
	//m_sScoreScore = "Score: 123,456,789 (123x)";
	//m_sScoreAccuracy = "98.97%";
	//m_sScoreMods = "HD,HR";

	//m_sScoreScorePPWeightedPP = "233pp";
	//m_sScoreScorePPWeightedWeight = "   weighted 95% (221pp)";
	//m_sScoreWeight = "weighted 95%";
}

OsuUISongBrowserScoreButton::~OsuUISongBrowserScoreButton()
{
	anim->deleteExistingAnimation(&m_fIndexNumberAnim);
}

void OsuUISongBrowserScoreButton::draw(Graphics *g)
{
	if (!m_bVisible) return;

	// background
	if (m_style == STYLE::SCORE_BROWSER)
	{
		g->setColor(0xff000000);
		g->setAlpha(0.59f * (0.5f + 0.5f*m_fIndexNumberAnim));
		Image *backgroundImage = m_osu->getSkin()->getMenuButtonBackground();
		g->pushTransform();
		{
			// allow overscale
			Vector2 hardcodedImageSize = Vector2(699.0f, 103.0f)*(m_osu->getSkin()->isMenuButtonBackground2x() ? 2.0f : 1.0f);
			const float scale = Osu::getImageScaleToFillResolution(hardcodedImageSize, m_vSize);

			g->scale(scale, scale);
			g->translate(m_vPos.x + hardcodedImageSize.x*scale/2.0f, m_vPos.y + hardcodedImageSize.y*scale/2.0f);
			g->drawImage(backgroundImage);
		}
		g->popTransform();
	}
	else if (m_style == STYLE::TOP_RANKS)
	{
		g->setColor(0xff666666); // from 33413c to 4e7466
		g->setAlpha(0.59f * (0.5f + 0.5f*m_fIndexNumberAnim));
		g->fillRect(m_vPos.x, m_vPos.y, m_vSize.x, m_vSize.y);
	}

	const int yPos = (int)m_vPos.y; // avoid max shimmering

	// index number
	const float indexNumberScale = 0.35f;
	const float indexNumberWidthPercent = (m_style == STYLE::TOP_RANKS ? 0.075f : 0.15f);
	McFont *indexNumberFont = m_osu->getSongBrowserFontBold();
	g->pushTransform();
	{
		UString indexNumberString = UString::format("%i", m_iScoreIndexNumber);
		const float scale = (m_vSize.y / indexNumberFont->getHeight())*indexNumberScale;

		g->scale(scale, scale);
		g->translate((int)(m_vPos.x + m_vSize.x*indexNumberWidthPercent*0.5f - indexNumberFont->getStringWidth(indexNumberString)*scale/2.0f), (int)(yPos + m_vSize.y/2.0f + indexNumberFont->getHeight()*scale/2.0f));
		g->translate(0.5f, 0.5f);
		g->setColor(0xff000000);
		g->setAlpha(1.0f - (1.0f - m_fIndexNumberAnim));
		g->drawString(indexNumberFont, indexNumberString);
		g->translate(-0.5f, -0.5f);
		g->setColor(0xffffffff);
		g->setAlpha(1.0f - (1.0f - m_fIndexNumberAnim)*(1.0f - m_fIndexNumberAnim));
		g->drawString(indexNumberFont, indexNumberString);
	}
	g->popTransform();

	// grade
	const float gradeHeightPercent = 0.7f;
	OsuSkinImage *grade = getGradeImage(m_osu, m_scoreGrade);
	int gradeWidth = 0;
	g->pushTransform();
	{
		const float scale = Osu::getImageScaleToFitResolution(grade->getSizeBaseRaw(), Vector2(m_vSize.x * (1.0f - indexNumberWidthPercent), m_vSize.y*gradeHeightPercent));
		gradeWidth = grade->getSizeBaseRaw().x*scale;

		g->setColor(0xffffffff);
		grade->drawRaw(g, Vector2((int)(m_vPos.x + m_vSize.x*indexNumberWidthPercent + gradeWidth/2.0f), (int)(m_vPos.y + m_vSize.y/2.0f)), scale);
	}
	g->popTransform();

	const float gradePaddingRight = m_vSize.y * 0.165f;

	// username | (artist + songName + diffName)
	const float usernameScale = (m_style == STYLE::TOP_RANKS ? 0.6f : 0.7f);
	McFont *usernameFont = m_osu->getSongBrowserFont();
	g->pushClipRect(McRect(m_vPos.x, m_vPos.y, m_vSize.x, m_vSize.y));
	g->pushTransform();
	{
		const float height = m_vSize.y*0.5f;
		const float paddingTopPercent = (1.0f - usernameScale)*(m_style == STYLE::TOP_RANKS ? 0.15f : 0.4f);
		const float paddingTop = height*paddingTopPercent;
		const float scale = (height / usernameFont->getHeight())*usernameScale;

		UString &string = (m_style == STYLE::TOP_RANKS ? m_sScoreTitle : m_sScoreUsername);

		g->scale(scale, scale);
		g->translate((int)(m_vPos.x + m_vSize.x*indexNumberWidthPercent + gradeWidth + gradePaddingRight), (int)(yPos + height/2.0f + usernameFont->getHeight()*scale/2.0f + paddingTop));
		g->translate(0.75f, 0.75f);
		g->setColor(0xff000000);
		g->setAlpha(0.75f);
		g->drawString(usernameFont, string);
		g->translate(-0.75f, -0.75f);
		g->setColor(0xffffffff);
		g->drawString(usernameFont, string);
	}
	g->popTransform();
	g->popClipRect();

	// score | pp | weighted 95% (pp)
	const float scoreScale = 0.5f;
	McFont *scoreFont = (m_vSize.y < 50 ? engine->getResourceManager()->getFont("FONT_DEFAULT") : usernameFont); // HACKHACK
	g->pushTransform();
	{
		const float height = m_vSize.y*0.5f;
		const float paddingBottomPercent = (1.0f - scoreScale)*(m_style == STYLE::TOP_RANKS ? 0.1f : 0.25f);
		const float paddingBottom = height*paddingBottomPercent;
		const float scale = (height / scoreFont->getHeight())*scoreScale;

		UString &string = (m_style == STYLE::TOP_RANKS ? m_sScoreScorePPWeightedPP : m_sScoreScorePP);

		g->scale(scale, scale);
		g->translate((int)(m_vPos.x + m_vSize.x*indexNumberWidthPercent + gradeWidth + gradePaddingRight), (int)(yPos + height*1.5f + scoreFont->getHeight()*scale/2.0f - paddingBottom));
		g->translate(0.75f, 0.75f);
		g->setColor(0xff000000);
		g->setAlpha(0.75f);
		g->drawString(scoreFont, (m_osu_scores_sort_by_pp->getBool() && !m_score.isLegacyScore ? string : m_sScoreScore));
		g->translate(-0.75f, -0.75f);
		g->setColor((m_style == STYLE::TOP_RANKS ? 0xffdeff87 : 0xffffffff));
		g->drawString(scoreFont, (m_osu_scores_sort_by_pp->getBool() && !m_score.isLegacyScore ? string : m_sScoreScore));

		if (m_style == STYLE::TOP_RANKS)
		{
			g->translate(scoreFont->getStringWidth(string)*scale, 0);
			g->translate(0.75f, 0.75f);
			g->setColor(0xff000000);
			g->setAlpha(0.75f);
			g->drawString(scoreFont, m_sScoreScorePPWeightedWeight);
			g->translate(-0.75f, -0.75f);
			g->setColor(0xffbbbbbb);
			g->drawString(scoreFont, m_sScoreScorePPWeightedWeight);
		}
	}
	g->popTransform();

	const float rightSideThirdHeight = m_vSize.y*0.333f;
	const float rightSidePaddingRight = (m_style == STYLE::TOP_RANKS ? 5 : m_vSize.x*0.025f);

	// mods
	const float modScale = 0.7f;
	McFont *modFont = m_osu->getSubTitleFont();
	g->pushTransform();
	{
		const float height = rightSideThirdHeight;
		const float paddingTopPercent = (1.0f - modScale)*0.45f;
		const float paddingTop = height*paddingTopPercent;
		const float scale = (height / modFont->getHeight())*modScale;

		g->scale(scale, scale);
		g->translate((int)(m_vPos.x + m_vSize.x - modFont->getStringWidth(m_sScoreMods)*scale - rightSidePaddingRight), (int)(yPos + height*0.5f + modFont->getHeight()*scale/2.0f + paddingTop));
		g->translate(0.75f, 0.75f);
		g->setColor(0xff000000);
		g->setAlpha(0.75f);
		g->drawString(modFont, m_sScoreMods);
		g->translate(-0.75f, -0.75f);
		g->setColor(0xffffffff);
		g->drawString(modFont, m_sScoreMods);
	}
	g->popTransform();

	// accuracy
	const float accScale = 0.65f;
	McFont *accFont = m_osu->getSubTitleFont();
	g->pushTransform();
	{
		const float height = rightSideThirdHeight;
		const float paddingTopPercent = (1.0f - modScale)*0.45f;
		const float paddingTop = height*paddingTopPercent;
		const float scale = (height / accFont->getHeight())*accScale;

		g->scale(scale, scale);
		g->translate((int)(m_vPos.x + m_vSize.x - accFont->getStringWidth(m_sScoreAccuracy)*scale - rightSidePaddingRight), (int)(yPos + height*1.5f + accFont->getHeight()*scale/2.0f + paddingTop));
		g->translate(0.75f, 0.75f);
		g->setColor(0xff000000);
		g->setAlpha(0.75f);
		g->drawString(accFont, m_sScoreAccuracy);
		g->translate(-0.75f, -0.75f);
		g->setColor((m_style == STYLE::TOP_RANKS ? 0xffffcc22 : 0xffffffff));
		g->drawString(accFont, m_sScoreAccuracy);
	}
	g->popTransform();

	if (m_style == STYLE::TOP_RANKS)
	{
		// weighted percent
		const float weightScale = 0.65f;
		McFont *weightFont = m_osu->getSubTitleFont();
		g->pushTransform();
		{
			const float height = rightSideThirdHeight;
			const float paddingBottomPercent = (1.0f - weightScale)*0.05f;
			const float paddingBottom = height*paddingBottomPercent;
			const float scale = (height / weightFont->getHeight())*weightScale;

			g->scale(scale, scale);
			g->translate((int)(m_vPos.x + m_vSize.x - weightFont->getStringWidth(m_sScoreWeight)*scale - rightSidePaddingRight), (int)(yPos + height*2.5f + weightFont->getHeight()*scale/2.0f - paddingBottom));
			g->translate(0.75f, 0.75f);
			g->setColor(0xff000000);
			g->setAlpha(0.75f);
			g->drawString(weightFont, m_sScoreWeight);
			g->translate(-0.75f, -0.75f);
			g->setColor(0xff999999);
			g->drawString(weightFont, m_sScoreWeight);
		}
		g->popTransform();
	}

	// recent icon + elapsed time since score
	const float upIconScale = 0.35f;
	const float timeElapsedScale = accScale;
	McFont *iconFont = m_osu->getFontIcons();
	McFont *timeFont = accFont;
	if (m_iScoreUnixTimestamp > 0)
	{
		const float iconScale = (m_vSize.y / iconFont->getHeight())*upIconScale;
		const float iconHeight = iconFont->getHeight()*iconScale;
		const float iconPaddingLeft = 2 + (m_style == STYLE::TOP_RANKS ? m_vSize.y*0.125f : 0);

		g->pushTransform();
		{
			g->scale(iconScale, iconScale);
			g->translate((int)(m_vPos.x + m_vSize.x + iconPaddingLeft), (int)(yPos + m_vSize.y/2 + iconHeight/2));
			g->translate(1, 1);
			g->setColor(0xff000000);
			g->setAlpha(0.75f);
			g->drawString(iconFont, recentScoreIconString);
			g->translate(-1, -1);
			g->setColor(0xffffffff);
			g->drawString(iconFont, recentScoreIconString);
		}
		g->popTransform();

		// elapsed time since score
		if (m_sScoreTime.length() > 0)
		{
			const float timeHeight = rightSideThirdHeight;
			const float timeScale = (timeHeight / timeFont->getHeight())*timeElapsedScale;
			const float timePaddingLeft = 8;

			g->pushTransform();
			{
				g->scale(timeScale, timeScale);
				g->translate((int)(m_vPos.x + m_vSize.x + iconPaddingLeft + iconFont->getStringWidth(recentScoreIconString)*iconScale + timePaddingLeft), (int)(yPos + m_vSize.y/2 + timeFont->getHeight()*timeScale/2));
				g->translate(0.75f, 0.75f);
				g->setColor(0xff000000);
				g->setAlpha(0.85f);
				g->drawString(timeFont, m_sScoreTime);
				g->translate(-0.75f, -0.75f);
				g->setColor(0xffffffff);
				g->drawString(timeFont, m_sScoreTime);
			}
			g->popTransform();
		}
	}

	// TODO: difference to below score in list, +12345

	if (m_style == STYLE::TOP_RANKS)
	{
		g->setColor(0xff111111);
		g->drawRect(m_vPos.x, m_vPos.y, m_vSize.x, m_vSize.y);
	}
}

void OsuUISongBrowserScoreButton::update()
{
	CBaseUIButton::update();
	if (!m_bVisible) return;

	// HACKHACK: this should really be part of the UI base
	// right click detection
	if (engine->getMouse()->isRightDown())
	{
		if (!m_bRightClickCheck)
		{
			m_bRightClickCheck = true;
			m_bRightClick = isMouseInside();
		}
	}
	else
	{
		if (m_bRightClick)
		{
			if (isMouseInside())
				onRightMouseUpInside();
		}

		m_bRightClickCheck = false;
		m_bRightClick = false;
	}

	// tooltip (extra stats)
	if (isMouseInside())
	{
		if (!isContextMenuVisible())
		{
			if (m_fIndexNumberAnim > 0.0f)
			{
				m_osu->getTooltipOverlay()->begin();
				{
					for (int i=0; i<m_tooltipLines.size(); i++)
					{
						if (m_tooltipLines[i].length() > 0)
							m_osu->getTooltipOverlay()->addLine(m_tooltipLines[i]);
					}
				}
				m_osu->getTooltipOverlay()->end();
			}
		}
		else
		{
			anim->deleteExistingAnimation(&m_fIndexNumberAnim);
			m_fIndexNumberAnim = 0.0f;
		}
	}

	// update elapsed time string
	updateElapsedTimeString();

	// stuck anim reset
	if (!isMouseInside() && !anim->isAnimating(&m_fIndexNumberAnim))
		m_fIndexNumberAnim = 0.0f;
}

void OsuUISongBrowserScoreButton::highlight()
{
	m_bIsPulseAnim = true;

	const int numPulses = 10;
	const float timescale = 1.75f;
	for (int i=0; i<2*numPulses; i++)
	{
		if (i % 2 == 0)
			anim->moveQuadOut(&m_fIndexNumberAnim, 1.0f, 0.125f*timescale, ((i/2)*(0.125f + 0.15f))*timescale - 0.001f, (i == 0));
		else
			anim->moveLinear(&m_fIndexNumberAnim, 0.0f, 0.15f*timescale, (0.125f + (i/2)*(0.125f + 0.15f))*timescale - 0.001f);
	}
}

void OsuUISongBrowserScoreButton::resetHighlight()
{
	m_bIsPulseAnim = false;
	anim->deleteExistingAnimation(&m_fIndexNumberAnim);
	m_fIndexNumberAnim = 0.0f;
}

void OsuUISongBrowserScoreButton::updateElapsedTimeString()
{
	if (m_iScoreUnixTimestamp > 0)
	{
		const uint64_t curUnixTime = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
		const uint64_t delta = curUnixTime - m_iScoreUnixTimestamp;

		const uint64_t deltaInSeconds = delta;
		const uint64_t deltaInMinutes = delta / 60;
		const uint64_t deltaInHours = deltaInMinutes / 60;
		const uint64_t deltaInDays = deltaInHours / 24;
		const uint64_t deltaInYears = deltaInDays / 365;

		if (deltaInHours < 96 || m_style == STYLE::TOP_RANKS)
		{
			if (deltaInDays > 364)
				m_sScoreTime = UString::format("%iy", (int)(deltaInYears));
			else if (deltaInHours > 47)
				m_sScoreTime = UString::format("%id", (int)(deltaInDays));
			else if (deltaInHours >= 1)
				m_sScoreTime = UString::format("%ih", (int)(deltaInHours));
			else if (deltaInMinutes > 0)
				m_sScoreTime = UString::format("%im", (int)(deltaInMinutes));
			else
				m_sScoreTime = UString::format("%is", (int)(deltaInSeconds));
		}
		else
		{
			m_iScoreUnixTimestamp = 0;

			if (m_sScoreTime.length() > 0)
				m_sScoreTime.clear();
		}
	}
}

void OsuUISongBrowserScoreButton::onClicked()
{
	engine->getSound()->play(m_osu->getSkin()->getMenuHit());
	CBaseUIButton::onClicked();
}

void OsuUISongBrowserScoreButton::onMouseInside()
{
	m_bIsPulseAnim = false;

	if (!isContextMenuVisible())
		anim->moveQuadOut(&m_fIndexNumberAnim, 1.0f, 0.125f*(1.0f - m_fIndexNumberAnim), true);
}

void OsuUISongBrowserScoreButton::onMouseOutside()
{
	m_bIsPulseAnim = false;

	anim->moveLinear(&m_fIndexNumberAnim, 0.0f, 0.15f*m_fIndexNumberAnim, true);
}

void OsuUISongBrowserScoreButton::onFocusStolen()
{
	CBaseUIButton::onFocusStolen();

	m_bRightClick = false;

	if (!m_bIsPulseAnim)
	{
		anim->deleteExistingAnimation(&m_fIndexNumberAnim);
		m_fIndexNumberAnim = 0.0f;
	}
}

void OsuUISongBrowserScoreButton::onRightMouseUpInside()
{
	const Vector2 pos = engine->getMouse()->getPos();

	if (m_contextMenu != NULL && !m_score.isLegacyScore)
	{
		m_contextMenu->setPos(pos);
		m_contextMenu->setRelPos(pos);
		m_contextMenu->begin();
		{
			m_contextMenu->addButton("Delete Score");
			m_contextMenu->addButton("TODO2");
			m_contextMenu->addButton("TODO3");
		}
		m_contextMenu->end();
		m_contextMenu->setClickCallback( fastdelegate::MakeDelegate(this, &OsuUISongBrowserScoreButton::onContextMenu) );
	}
}

void OsuUISongBrowserScoreButton::onContextMenu(UString text, int id)
{
	m_osu->getSongBrowser()->onScoreContextMenu(this, text);
}

void OsuUISongBrowserScoreButton::setScore(OsuDatabase::Score score, int index, UString titleString, float weight)
{
	m_score = score;
	m_iScoreIndexNumber = index;

	const float accuracy = OsuScore::calculateAccuracy(score.num300s, score.num100s, score.num50s, score.numMisses)*100.0f;
	const bool modHidden = score.modsLegacy & OsuReplay::Mods::Hidden;
	const bool modFlashlight = score.modsLegacy & OsuReplay::Mods::Flashlight;

	// display
	m_scoreGrade = OsuScore::calculateGrade(score.num300s, score.num100s, score.num50s, score.numMisses, modHidden, modFlashlight);
	m_sScoreUsername = score.playerName;
	m_sScoreScore = UString::format("Score: %llu (%ix)", score.score, score.comboMax);
	m_sScoreScorePP = UString::format("PP: %ipp (%ix)", (int)std::round(score.pp), score.comboMax);
	m_sScoreAccuracy = UString::format("%.2f%%", accuracy);
	m_sScoreMods = getModsString(score.modsLegacy);
	if (score.experimentalModsConVars.length() > 0)
	{
		if (m_sScoreMods.length() > 0)
			m_sScoreMods.append(",");

		std::vector<UString> experimentalMods = score.experimentalModsConVars.split(";");
		for (int i=0; i<experimentalMods.size(); i++)
		{
			if (experimentalMods[i].length() > 0)
				m_sScoreMods.append("++");
		}
	}

	char dateString[64];
	memset(dateString, '\0', 64);
	std::tm *tm = std::localtime((std::time_t*)(&score.unixTimestamp));
	std::strftime(dateString, 63, "%d-%b-%y %H:%M:%S", tm);
	m_sScoreDateTime = UString(dateString);
	m_iScoreUnixTimestamp = score.unixTimestamp;

	UString achievedOn = "Achieved on ";
	achievedOn.append(m_sScoreDateTime);

	// tooltip
	m_tooltipLines.clear();
	m_tooltipLines.push_back(achievedOn);

	if (m_score.isLegacyScore)
		m_tooltipLines.push_back(UString::format("300:%i 100:%i 50:%i Miss:%i", score.num300s, score.num100s, score.num50s, score.numMisses));
	else
		m_tooltipLines.push_back(UString::format("300:%i 100:%i 50:%i Miss:%i SBreak:%i", score.num300s, score.num100s, score.num50s, score.numMisses, score.numSliderBreaks));

	m_tooltipLines.push_back(UString::format("Accuracy: %.2f%%", accuracy));

	UString tooltipMods = "Mods: ";
	if (m_sScoreMods.length() > 0)
		tooltipMods.append(m_sScoreMods);
	else
		tooltipMods.append("None");

	m_tooltipLines.push_back(tooltipMods);
	if (score.experimentalModsConVars.length() > 0)
	{
		std::vector<UString> experimentalMods = score.experimentalModsConVars.split(";");
		for (int i=0; i<experimentalMods.size(); i++)
		{
			if (experimentalMods[i].length() > 0)
			{
				UString experimentalModString = "+ ";
				experimentalModString.append(experimentalMods[i]);
				m_tooltipLines.push_back(experimentalModString);
			}
		}
	}

	if (m_style == STYLE::TOP_RANKS)
	{
		const int weightRounded = std::round(weight*100.0f);
		const int ppWeightedRounded = std::round(score.pp*weight);

		m_sScoreTitle = titleString;
		m_sScoreScorePPWeightedPP = UString::format("%ipp", (int)std::round(score.pp));
		m_sScoreScorePPWeightedWeight = UString::format("     weighted %i%% (%ipp)", weightRounded, ppWeightedRounded);
		m_sScoreWeight = UString::format("weighted %i%%", weightRounded);

		//m_tooltipLines.push_back("Difficulty:");
		m_tooltipLines.push_back(UString::format("Stars: %.2f (%.2f aim, %.2f speed)", score.starsTomTotal, score.starsTomAim, score.starsTomSpeed));
		m_tooltipLines.push_back(UString::format("Speed: %.3gx", score.speedMultiplier));
		m_tooltipLines.push_back(UString::format("CS:%.4g AR:%.4g OD:%.4g HP:%.4g", score.CS, score.AR, score.OD, score.HP));
		//m_tooltipLines.push_back("Accuracy:");
		m_tooltipLines.push_back(UString::format("Error: %.2fms - %.2fms avg", score.hitErrorAvgMin, score.hitErrorAvgMax));
		m_tooltipLines.push_back(UString::format("Unstable Rate: %.2f", score.unstableRate));
		m_tooltipLines.push_back(UString::format("Version: %i", score.version));
	}

	// custom
	updateElapsedTimeString();
}

bool OsuUISongBrowserScoreButton::isContextMenuVisible()
{
	return (m_contextMenu != NULL && m_contextMenu->isVisible());
}

UString OsuUISongBrowserScoreButton::getModsString(int mods)
{
	UString modsString;

	if (mods & OsuReplay::Mods::NoFail)
		modsString.append("NF,");
	if (mods & OsuReplay::Mods::Easy)
		modsString.append("EZ,");
	if (mods & OsuReplay::Mods::TouchDevice)
		modsString.append("Touch,");
	if (mods & OsuReplay::Mods::Hidden)
		modsString.append("HD,");
	if (mods & OsuReplay::Mods::HardRock)
		modsString.append("HR,");
	if (mods & OsuReplay::Mods::SuddenDeath)
		modsString.append("SD,");
	if (mods & OsuReplay::Mods::DoubleTime)
		modsString.append("DT,");
	if (mods & OsuReplay::Mods::Relax)
		modsString.append("Relax,");
	if (mods & OsuReplay::Mods::HalfTime)
		modsString.append("HT,");
	if (mods & OsuReplay::Mods::Nightcore)
		modsString.append("NC,");
	if (mods & OsuReplay::Mods::Flashlight)
		modsString.append("FL,");
	if (mods & OsuReplay::Mods::Autoplay)
		modsString.append("AT,");
	if (mods & OsuReplay::Mods::SpunOut)
		modsString.append("SO,");
	if (mods & OsuReplay::Mods::Relax2)
		modsString.append("AP,");
	if (mods & OsuReplay::Mods::Perfect)
		modsString.append("PF,");
	if (mods & OsuReplay::Mods::ScoreV2)
		modsString.append("v2,");
	if (mods & OsuReplay::Mods::Target)
		modsString.append("Target,");
	if (mods & OsuReplay::Mods::Nightmare)
		modsString.append("NM,");

	if (modsString.length() > 0)
		modsString = modsString.substr(0, modsString.length()-1);

	return modsString;
}

OsuSkinImage *OsuUISongBrowserScoreButton::getGradeImage(Osu *osu, OsuScore::GRADE grade)
{
	switch (grade)
	{
	case OsuScore::GRADE::GRADE_XH:
		return osu->getSkin()->getRankingXHsmall();
	case OsuScore::GRADE::GRADE_SH:
		return osu->getSkin()->getRankingSHsmall();
	case OsuScore::GRADE::GRADE_X:
		return osu->getSkin()->getRankingXsmall();
	case OsuScore::GRADE::GRADE_S:
		return osu->getSkin()->getRankingSsmall();
	case OsuScore::GRADE::GRADE_A:
		return osu->getSkin()->getRankingAsmall();
	case OsuScore::GRADE::GRADE_B:
		return osu->getSkin()->getRankingBsmall();
	case OsuScore::GRADE::GRADE_C:
		return osu->getSkin()->getRankingCsmall();
	default:
		return osu->getSkin()->getRankingDsmall();
	}
}
