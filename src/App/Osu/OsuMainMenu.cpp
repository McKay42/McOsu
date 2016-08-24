//================ Copyright (c) 2015, PG, All rights reserved. =================//
//
// Purpose:		main menu
//
// $NoKeywords: $osumain
//===============================================================================//

#include "OsuMainMenu.h"

#include "Engine.h"
#include "SoundEngine.h"
#include "Keyboard.h"
#include "Mouse.h"
#include "SoundEngine.h"
#include "ResourceManager.h"
#include "AnimationHandler.h"
#include "ConVar.h"

#include "Osu.h"
#include "OsuSkin.h"
#include "OsuBeatmap.h"
#include "OsuBeatmapDifficulty.h"
#include "OsuUpdateChecker.h"

#include "OsuUIButton.h"

#include "CBaseUIContainer.h"
#include "CBaseUIButton.h"

#define MCOSU_VERSION "Alpha"
#define MCOSU_BANNER "-Animated Skins are not working yet-"
#define MCOSU_MAIN_BUTTON "McOsu!"
#define MCOSU_MAIN_BUTTON_BACK "by McKay"



class OsuMainMenuMainButton : public CBaseUIButton
{
public:
	OsuMainMenuMainButton(OsuMainMenu *mainMenu, float xPos, float yPos, float xSize, float ySize, UString name, UString text);

	virtual void draw(Graphics *g);

	void onMouseInside();
	void onMouseOutside();
	void onMouseDownInside();

private:
	virtual void onClicked();

	OsuMainMenu *m_mainMenu;
};

class OsuMainMenuButton : public CBaseUIButton
{
public:
	OsuMainMenuButton(OsuMainMenu *mainMenu, float xPos, float yPos, float xSize, float ySize, UString name, UString text);

	void onMouseDownInside();

private:
	OsuMainMenu *m_mainMenu;
};

class OsuMainMenuPauseButton : public CBaseUIButton
{
public:
	OsuMainMenuPauseButton(float xPos, float yPos, float xSize, float ySize, UString name, UString text) : CBaseUIButton(xPos, yPos, xSize, ySize, name, text) {;}

	virtual void draw(Graphics *g)
	{
		int third = m_vSize.x/3;

		g->setColor(0xffffffff);
		g->fillRect(m_vPos.x, m_vPos.y, third, m_vSize.y);
		g->fillRect(m_vPos.x + 2*third, m_vPos.y, third, m_vSize.y);

		// draw hover rects
		g->setColor(m_frameColor);
		if (m_bMouseInside && m_bEnabled)
		{
			if (!m_bActive && !engine->getMouse()->isLeftDown())
				drawHoverRect(g, 3);
			else if (m_bActive)
				drawHoverRect(g, 3);
		}
		if (m_bActive && m_bEnabled)
			drawHoverRect(g, 6);
	}
};



OsuMainMenu::OsuMainMenu(Osu *osu) : OsuScreen()
{
	m_osu = osu;

	// engine settings
	engine->getMouse()->addListener(this);

	m_fSizeAddAnim = 0.0f;
	m_fCenterOffsetAnim = 0.0f;
	m_bMenuElementsVisible = false;

	m_fShutdownScheduledTime = 0.0f;
	m_fMainMenuAnimTime = 0.0f;
	m_fMainMenuAnim = 0.0f;
	m_fMainMenuAnim1 = 0.0f;
	m_fMainMenuAnim2 = 0.0f;
	m_fMainMenuAnim3 = 0.0f;
	m_fMainMenuAnim1Target = 0.0f;
	m_fMainMenuAnim2Target = 0.0f;
	m_fMainMenuAnim3Target = 0.0f;

	m_updateChecker = new OsuUpdateChecker();
	m_bUpdateStatus = false;
	m_bUpdateCheckFinished = false;
	m_fUpdateStatusTime = 0.0f;
	m_fUpdateButtonAnim = 0.0f;

	m_container = new CBaseUIContainer(0, 0, m_osu->getScreenWidth(), m_osu->getScreenHeight(), "");
	m_mainButton = new OsuMainMenuMainButton(this, 0, 0, 1, 1, "", "");

	m_container->addBaseUIElement(m_mainButton);

	addMainMenuButton("Play")->setClickCallback( MakeDelegate(this, &OsuMainMenu::onPlayButtonPressed) );
	///addMainMenuButton("Edit");
	addMainMenuButton("Options")->setClickCallback( MakeDelegate(this, &OsuMainMenu::onOptionsButtonPressed) );
	addMainMenuButton("Exit")->setClickCallback( MakeDelegate(this, &OsuMainMenu::onExitButtonPressed) );

	m_pauseButton = new OsuMainMenuPauseButton(0, 0, 0, 0, "", "");
	m_pauseButton->setClickCallback( MakeDelegate(this, &OsuMainMenu::onPausePressed) );
	m_container->addBaseUIElement(m_pauseButton);

	m_updateAvailableButton = new OsuUIButton(m_osu, 0, 0, 0, 0, "", "Checking for updates ...");
	m_updateAvailableButton->setUseDefaultSkin();
	m_updateAvailableButton->setClickCallback( MakeDelegate(this, &OsuMainMenu::onUpdatePressed) );
	m_updateAvailableButton->setColor(0x2200ff00);
	m_updateAvailableButton->setTextColor(0x22ffffff);

	m_todo.push_back("- UI Scaling");
	m_todo.push_back("- Beatmap Skin");
	m_todo.push_back("- Score & HP");
	m_todo.push_back("- Skin Anim.");
	m_todo.push_back("- Skin Prefix");
	m_todo.push_back("- Replays");
	m_todo.push_back("- Multiplayer");
	m_todo.push_back("- Note Blocking");

	m_updateChecker->checkForUpdates();
}

OsuMainMenu::~OsuMainMenu()
{
	SAFE_DELETE(m_container);
	SAFE_DELETE(m_updateAvailableButton);
	SAFE_DELETE(m_updateChecker);
}

void OsuMainMenu::draw(Graphics *g)
{
	if (!m_bVisible)
		return;

	McFont *smallFont = engine->getResourceManager()->getFont("FONT_DEFAULT");
	McFont *titleFont = m_osu->getTitleFont();

	// main button stuff
	const float div = 1.25f;
	float pulse = 0.0f;
	if (m_osu->getSelectedBeatmap() != NULL && m_osu->getSelectedBeatmap()->getSelectedDifficulty() != NULL && m_osu->getSelectedBeatmap()->getMusic() != NULL && m_osu->getSelectedBeatmap()->getMusic()->isPlaying())
	{
		long curMusicPos = m_osu->getSelectedBeatmap()->getMusic()->getPositionMS();
		OsuBeatmapDifficulty::TIMING_INFO t = m_osu->getSelectedBeatmap()->getSelectedDifficulty()->getTimingInfoForTime(curMusicPos);
		pulse = (float)((curMusicPos - t.offset) % (long)t.beatLengthBase)/t.beatLengthBase;
	}
	else
		pulse = (div - fmod(engine->getTime(), div))/div;

	//pulse *= pulse; // quadratic
	Vector2 size = m_vSize;
	const float pulseSub = 0.05f*pulse;
	size -= size*pulseSub;
	size += size*m_fSizeAddAnim;
	Rect mainButtonRect = Rect(m_vCenter.x - size.x/2.0f - m_fCenterOffsetAnim, m_vCenter.y - size.y/2.0f, size.x, size.y);

	// draw banner
	/*
	float bannerStringWidth = smallFont->getStringWidth(MCOSU_BANNER);
	int bannerDiff = 20;
	int bannerMargin = 5;
	int numBanners = (int)std::round(m_osu->getScreenWidth() / (bannerStringWidth + bannerDiff)) + 2;

	g->setColor(0xff777777);
	g->pushTransform();
	g->translate(1, 1);
	for (int i=-1; i<numBanners; i++)
	{
		g->pushTransform();
		g->translate(i*bannerStringWidth + i*bannerDiff + fmod(engine->getTime()*30, bannerStringWidth + bannerDiff), smallFont->getHeight() + bannerMargin);
		g->drawString(smallFont, MCOSU_BANNER);
		g->popTransform();
	}
	g->popTransform();
	g->setColor(0xff333333);
	for (int i=-1; i<numBanners; i++)
	{
		g->pushTransform();
		g->translate(i*bannerStringWidth + i*bannerDiff + fmod(engine->getTime()*30, bannerStringWidth + bannerDiff), smallFont->getHeight() + bannerMargin);
		g->drawString(smallFont, MCOSU_BANNER);
		g->popTransform();
	}
	*/

	// draw todolist
	g->setColor(0xff777777);
	g->pushTransform();
	g->translate(7, m_osu->getScreenHeight()/2 - smallFont->getHeight()/2);
	g->drawString(smallFont, "TODO:");
	g->translate(0, 10);
	for (int i=0; i<m_todo.size(); i++)
	{
		g->translate(0, smallFont->getHeight()+5);
		g->drawString(smallFont, m_todo[i]);
	}
	g->popTransform();

	// draw container
	m_container->draw(g);

	// draw update check button
	if (m_bUpdateStatus)
	{
		g->push3DScene(Rect(m_updateAvailableButton->getPos().x, m_updateAvailableButton->getPos().y, m_updateAvailableButton->getSize().x, m_updateAvailableButton->getSize().y));
		g->rotate3DScene(m_fUpdateButtonAnim*360.0f, 0, 0);
	}
	m_updateAvailableButton->draw(g);
	if (m_bUpdateStatus)
		g->pop3DScene();

	// draw main button
	if (m_fMainMenuAnim > 0.0f && m_fMainMenuAnim != 1.0f)
	{
		g->push3DScene(mainButtonRect);
		g->rotate3DScene(m_fMainMenuAnim1*360.0f, m_fMainMenuAnim2*360.0f, m_fMainMenuAnim3*360.0f);
	}

		g->setColor(0xff000000);
		g->fillRect(mainButtonRect.getX(), mainButtonRect.getY(), mainButtonRect.getWidth(), mainButtonRect.getHeight());
		g->setColor(0xffffffff);
		g->drawRect(mainButtonRect.getX(), mainButtonRect.getY(), mainButtonRect.getWidth(), mainButtonRect.getHeight());

		float fontScale = 1.0f - pulseSub + m_fSizeAddAnim;

		g->setColor(0xffffffff);
		g->pushTransform();
		g->scale(fontScale, fontScale);
		g->translate(m_vCenter.x - m_fCenterOffsetAnim - (titleFont->getStringWidth(MCOSU_MAIN_BUTTON)/2.0f)*fontScale, m_vCenter.y + (titleFont->getHeight()*fontScale)/2.25f);
		g->drawString(titleFont, MCOSU_MAIN_BUTTON);
		g->popTransform();

	if (m_fMainMenuAnim > 0.0f && m_fMainMenuAnim != 1.0f)
	{
		if ((m_fMainMenuAnim1*360.0f > 90.0f && m_fMainMenuAnim1*360.0f < 270.0f) || (m_fMainMenuAnim2*360.0f > 90.0f && m_fMainMenuAnim2*360.0f < 270.0f)
		 || (m_fMainMenuAnim1*360.0f < -90.0f && m_fMainMenuAnim1*360.0f > -270.0f) || (m_fMainMenuAnim2*360.0f < -90.0f && m_fMainMenuAnim2*360.0f > -270.0f))
		{
			g->setColor(0xffffffff);
			g->pushTransform();
			g->scale(fontScale, fontScale);
			g->translate(m_vCenter.x - m_fCenterOffsetAnim - (titleFont->getStringWidth(MCOSU_MAIN_BUTTON_BACK)/2.0f)*fontScale, m_vCenter.y + ((titleFont->getHeight()*fontScale)/2.25f)*4.0f);
			g->drawString(titleFont, MCOSU_MAIN_BUTTON_BACK);
			g->popTransform();
		}
		g->pop3DScene();
	}

	drawVersionInfo(g);
}

void OsuMainMenu::drawVersionInfo(Graphics *g)
{
	UString versionString = MCOSU_VERSION;
	versionString.append(" ");
	versionString.append(Osu::version->getString());
	McFont *versionFont = engine->getResourceManager()->getFont("FONT_DEFAULT");

	g->setColor(0xffffffff);
	g->pushTransform();
	g->translate(7, m_osu->getScreenHeight() - 7);
	g->drawString(versionFont, versionString);
	g->popTransform();
}

void OsuMainMenu::update()
{
	if (!m_bVisible)
		return;

	updateLayout();

	// the main button always gets top focus
	m_mainButton->update();
	if (m_mainButton->isMouseInside())
		m_container->stealFocus();

	m_container->update();
	m_updateAvailableButton->update();

	// handle automatic menu closing
	if (m_fMainMenuButtonCloseTime != 0.0f && engine->getTime() > m_fMainMenuButtonCloseTime)
	{
		m_fMainMenuButtonCloseTime = 0.0f;
		setMenuElementsVisible(false);
	}

	// hide the buttons if the closing animation finished
	if (!anim->isAnimating(&m_fCenterOffsetAnim) && m_fCenterOffsetAnim == 0.0f)
	{
		for (int i=0; i<m_menuElements.size(); i++)
		{
			m_menuElements[i]->setVisible(false);
		}
	}

	// handle delayed shutdown
	if (m_fShutdownScheduledTime != 0.0f && engine->getTime() > m_fShutdownScheduledTime)
	{
		engine->shutdown();
		m_fShutdownScheduledTime = 0.0f;
	}

	if (m_bMenuElementsVisible)
		m_fMainMenuAnimTime = engine->getTime() + 15.0f;
	if (engine->getTime() > m_fMainMenuAnimTime)
	{
		m_fMainMenuAnimTime = engine->getTime() + 10.0f + (static_cast<float>(rand())/static_cast<float>(RAND_MAX))*5.0f;
		animMainButton();
	}

	// handle update checker
	if (!m_bUpdateCheckFinished && m_updateChecker->isReady())
	{
		m_bUpdateCheckFinished = true;
		m_bUpdateStatus = m_updateChecker->isUpdateAvailable();
		if (m_bUpdateStatus)
		{
			m_updateAvailableButton->setColor(0xff00ff00);
			m_updateAvailableButton->setTextColor(0xffffffff);
			m_updateAvailableButton->setText(">>> Update available on Github! <<<");

			m_fUpdateStatusTime = engine->getTime() + 0.5f;
		}
		else
			m_updateAvailableButton->setVisible(false);
	}
	if (m_bUpdateStatus && engine->getTime() > m_fUpdateStatusTime)
	{
		m_fUpdateStatusTime = engine->getTime() + 4.0f;
		m_fUpdateButtonAnim = 0.0f;
		anim->moveQuadInOut(&m_fUpdateButtonAnim, 1.0f, 0.5f, true);
	}
}

void OsuMainMenu::onKeyDown(KeyboardEvent &e)
{
	if (!m_bVisible)
		return;

	if (!m_bMenuElementsVisible)
	{
		if (e == KEY_P)
			m_mainButton->click();
	}
	else
	{
		if (e == KEY_P)
			onPlayButtonPressed();
		if (e == KEY_O)
			onOptionsButtonPressed();
		if (e == KEY_E)
			onExitButtonPressed();

		if (e == KEY_ESCAPE)
			setMenuElementsVisible(false);
	}

	if (e == KEY_U)
		m_updateAvailableButton->click();
}

void OsuMainMenu::onMiddleChange(bool down)
{
	if (down && !anim->isAnimating(&m_fMainMenuAnim) && !m_bMenuElementsVisible)
	{
		animMainButton();
		m_fMainMenuAnimTime = engine->getTime() + 15.0f;
	}
}

void OsuMainMenu::onResolutionChange(Vector2 newResolution)
{
	updateLayout();
	setMenuElementsVisible(m_bMenuElementsVisible);
}

void OsuMainMenu::setVisible(bool visible)
{
	m_bVisible = visible;

	if (!m_bVisible)
	{
		setMenuElementsVisible(false, false);
	}
	else
	{
		updateLayout();
		m_fMainMenuAnimTime = engine->getTime() + 15.0f;

		m_osu->stopRidiculouslyLongApplauseSound();
	}
}

void OsuMainMenu::updateLayout()
{
	m_vCenter = m_osu->getScreenSize()/2.0f;
	float size = Osu::getUIScale(m_osu, 324.0f);
	m_vSize = Vector2(size, size);

	m_pauseButton->setSize(30, 30);
	m_pauseButton->setRelPos(m_osu->getScreenWidth() - m_pauseButton->getSize().x*2 - 10, m_pauseButton->getSize().y + 10);

	m_updateAvailableButton->setSize(375, 50);
	m_updateAvailableButton->setPos(m_osu->getScreenWidth()/2 - m_updateAvailableButton->getSize().x/2, m_osu->getScreenHeight() - m_updateAvailableButton->getSize().y - 10);

	m_mainButton->setRelPos(m_vCenter - m_vSize/2.0f - Vector2(m_fCenterOffsetAnim, 0.0f));
	m_mainButton->setSize(m_vSize);

	int numButtons = m_menuElements.size();
	int menuElementHeight = m_vSize.y / numButtons;
	int menuElementPadding = m_vSize.y*0.075f;
	menuElementHeight -= (numButtons-1)*menuElementPadding;
	int menuElementExtraWidth = m_vSize.x*0.06f;

	float offsetPercent = m_fCenterOffsetAnim / (m_vSize.x/2.0f);
	float curY = m_mainButton->getRelPos().y + (m_vSize.y - menuElementHeight*numButtons - (numButtons-1)*menuElementPadding)/2.0f;
	for (int i=0; i<m_menuElements.size(); i++)
	{
		curY += (i > 0 ? menuElementHeight+menuElementPadding : 0.0f);

		m_menuElements[i]->setRelPos(m_mainButton->getRelPos().x + m_mainButton->getSize().x*offsetPercent - menuElementExtraWidth*offsetPercent + menuElementExtraWidth*(1.0f - offsetPercent), curY);
		m_menuElements[i]->setSize(m_mainButton->getSize().x + menuElementExtraWidth*offsetPercent - 2.0f*menuElementExtraWidth*(1.0f - offsetPercent), menuElementHeight);
	}

	m_container->setSize(m_osu->getScreenSize());
	m_container->update_pos();
}

void OsuMainMenu::animMainButton()
{
	m_fMainMenuAnim = 0.0f;
	m_fMainMenuAnim1 = 0.0f;
	m_fMainMenuAnim2 = 0.0f;
	m_fMainMenuAnim3 = 1.0f;

	m_fMainMenuAnim1Target = (rand() % 2) == 1 ? 1.0f : -1.0f;
	m_fMainMenuAnim2Target = (rand() % 2) == 1 ? 1.0f : -1.0f;
	m_fMainMenuAnim3Target = (rand() % 2) == 1 ? 1.0f : -1.0f;

	anim->moveQuadOut(&m_fMainMenuAnim, 1.0f, 2.5f + 2.5f);
	anim->moveQuadOut(&m_fMainMenuAnim1, m_fMainMenuAnim1Target, 2.5f + (static_cast<float>(rand())/static_cast<float>(RAND_MAX))*2.5f);
	anim->moveQuadOut(&m_fMainMenuAnim2, m_fMainMenuAnim2Target, 2.5f + (static_cast<float>(rand())/static_cast<float>(RAND_MAX))*2.5f);
	anim->moveQuadOut(&m_fMainMenuAnim3, m_fMainMenuAnim3Target, 2.5f + (static_cast<float>(rand())/static_cast<float>(RAND_MAX))*2.5f);
}

void OsuMainMenu::animMainButtonBack()
{
	if (anim->isAnimating(&m_fMainMenuAnim))
	{
		anim->moveQuadOut(&m_fMainMenuAnim, 1.0f, 0.25f, true);
		anim->moveQuadOut(&m_fMainMenuAnim1, m_fMainMenuAnim1Target, 0.25f, true);
		anim->moveQuadOut(&m_fMainMenuAnim2, m_fMainMenuAnim2Target, 0.25f, true);
		anim->moveQuadOut(&m_fMainMenuAnim3, m_fMainMenuAnim3Target, 0.10f, true);
	}
}

void OsuMainMenu::setMenuElementsVisible(bool visible, bool animate)
{
	m_bMenuElementsVisible = visible;

	if (visible)
	{
		if (m_bMenuElementsVisible && m_vSize.x/2.0f < m_fCenterOffsetAnim) // so we don't see the ends of the menu element buttons if the window gets smaller
			m_fCenterOffsetAnim = m_vSize.x/2.0f;

		if (animate)
			anim->moveQuadInOut(&m_fCenterOffsetAnim, m_vSize.x/2.0f, 0.5f, 0.0f, true);
		else
		{
			anim->deleteExistingAnimation(&m_fCenterOffsetAnim);
			m_fCenterOffsetAnim = m_vSize.x/2.0f;
		}

		m_fMainMenuButtonCloseTime = engine->getTime() + 6.0f;

		for (int i=0; i<m_menuElements.size(); i++)
		{
			m_menuElements[i]->setVisible(true);
			m_menuElements[i]->setEnabled(true);
		}
	}
	else
	{
		if (animate)
			anim->moveQuadInOut(&m_fCenterOffsetAnim, 0.0f, 0.5f*(m_fCenterOffsetAnim/(m_vSize.x/2.0f)), 0.0f, true);
		else
		{
			anim->deleteExistingAnimation(&m_fCenterOffsetAnim);
			m_fCenterOffsetAnim = 0.0f;
		}

		m_fMainMenuButtonCloseTime = 0.0f;

		for (int i=0; i<m_menuElements.size(); i++)
		{
			m_menuElements[i]->setEnabled(false);
		}
	}
}

OsuMainMenuButton *OsuMainMenu::addMainMenuButton(UString text)
{
	OsuMainMenuButton *button = new OsuMainMenuButton(this, m_vSize.x, 0, 1, 1, "", text);
	button->setFont(m_osu->getSubTitleFont());
	button->setVisible(false);

	m_menuElements.push_back(button);
	m_container->addBaseUIElement(button);
	return button;
}

void OsuMainMenu::onMainMenuButtonPressed()
{
	engine->getSound()->play(m_osu->getSkin()->getMenuHit());

	// if the menu is already visible, this counts as pressing the play button
	if (m_bMenuElementsVisible)
		onPlayButtonPressed();
	else
		setMenuElementsVisible(true);

	animMainButtonBack();
}

void OsuMainMenu::onPlayButtonPressed()
{
	m_osu->toggleSongBrowser();
}

void OsuMainMenu::onOptionsButtonPressed()
{
	m_osu->toggleOptionsMenu();
}

void OsuMainMenu::onExitButtonPressed()
{
	m_fShutdownScheduledTime = engine->getTime() + 0.5f;
	setMenuElementsVisible(false);
}

void OsuMainMenu::onPausePressed()
{
	if (m_osu->getSelectedBeatmap() != NULL)
		m_osu->getSelectedBeatmap()->pausePreviewMusic();
}

void OsuMainMenu::onUpdatePressed()
{
	if (m_bUpdateCheckFinished && m_bUpdateStatus)
	{
		env->openURLInDefaultBrowser(OsuUpdateChecker::GITHUB_RELEASE_DOWNLOAD_URL);
		env->minimize();
	}
}



OsuMainMenuMainButton::OsuMainMenuMainButton(OsuMainMenu *mainMenu, float xPos, float yPos, float xSize, float ySize, UString name, UString text) : CBaseUIButton(xPos, yPos, xSize, ySize, name, text)
{
	m_mainMenu = mainMenu;
}

void OsuMainMenuMainButton::draw(Graphics *g)
{
	// draw nothing
	///CBaseUIButton::draw(g);
}

void OsuMainMenuMainButton::onMouseDownInside()
{
	anim->moveQuadInOut(&m_mainMenu->m_fSizeAddAnim, 0.0f, 0.06f, 0.0f, false);
	anim->moveQuadInOut(&m_mainMenu->m_fSizeAddAnim, 0.12f, 0.06f, 0.07f, false);

	m_mainMenu->onMainMenuButtonPressed();

	CBaseUIButton::onMouseDownInside();
}

void OsuMainMenuMainButton::onMouseInside()
{
	anim->moveQuadInOut(&m_mainMenu->m_fSizeAddAnim, 0.12f, 0.15f, 0.0f, true);

	CBaseUIButton::onMouseInside();
}

void OsuMainMenuMainButton::onMouseOutside()
{
	anim->moveQuadInOut(&m_mainMenu->m_fSizeAddAnim, 0.0f, 0.15f, 0.0f, true);

	CBaseUIButton::onMouseOutside();
}

void OsuMainMenuMainButton::onClicked()
{
	onMouseDownInside();
}



OsuMainMenuButton::OsuMainMenuButton(OsuMainMenu *mainMenu, float xPos, float yPos, float xSize, float ySize, UString name, UString text) : CBaseUIButton(xPos, yPos, xSize, ySize, name, text)
{
	m_mainMenu = mainMenu;
}

void OsuMainMenuButton::onMouseDownInside()
{
	if (m_mainMenu->m_mainButton->isMouseInside())
		return;

	engine->getSound()->play(m_mainMenu->getOsu()->getSkin()->getMenuHit());
	CBaseUIButton::onMouseDownInside();
}
