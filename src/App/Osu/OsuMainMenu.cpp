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

#include "CBaseUIContainer.h"
#include "CBaseUIButton.h"

#define MCOSU_VERSION "Alpha 20"
#define MCOSU_BANNER "-WIP-"



class OsuMainMenuMainButton : public CBaseUIButton
{
public:
	OsuMainMenuMainButton(OsuMainMenu *mainMenu, float xPos, float yPos, float xSize, float ySize, UString name, UString text);

	virtual void draw(Graphics *g);

	void onMouseInside();
	void onMouseOutside();
	void onMouseDownInside();

private:
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

	m_fSizeAddAnim = 0.0f;
	m_fCenterOffsetAnim = 0.0f;
	m_bMenuElementsVisible = false;

	m_fShutdownScheduledTime = 0.0f;

	m_container = new CBaseUIContainer(0, 0, Osu::getScreenWidth(), Osu::getScreenHeight(), "");
	m_mainButton = new OsuMainMenuMainButton(this, 0, 0, 1, 1, "", "");

	m_container->addBaseUIElement(m_mainButton);

	addMainMenuButton("Play")->setClickCallback( MakeDelegate(this, &OsuMainMenu::onPlayButtonPressed) );
	///addMainMenuButton("Edit");
	addMainMenuButton("Options")->setClickCallback( MakeDelegate(this, &OsuMainMenu::onOptionsButtonPressed) );
	addMainMenuButton("Exit")->setClickCallback( MakeDelegate(this, &OsuMainMenu::onExitButtonPressed) );

	m_pauseButton = new OsuMainMenuPauseButton(0, 0, 0, 0, "", "");
	m_pauseButton->setClickCallback( MakeDelegate(this, &OsuMainMenu::onPausePressed) );
	m_container->addBaseUIElement(m_pauseButton);

	m_todo.push_back("- UI Scaling");
	m_todo.push_back("- Beatmap Skin");
	m_todo.push_back("- Score & HP");
	m_todo.push_back("- Skin Anim.");
	m_todo.push_back("- Skin Prefix");
	m_todo.push_back("- Replays");
	m_todo.push_back("- Multiplayer");
	m_todo.push_back("- Note Blocking");
}

OsuMainMenu::~OsuMainMenu()
{
	SAFE_DELETE(m_container);
}

void OsuMainMenu::draw(Graphics *g)
{
	if (!m_bVisible)
		return;

	McFont *smallFont = engine->getResourceManager()->getFont("FONT_DEFAULT");
	McFont *titleFont = m_osu->getTitleFont();

	// draw banner
	/*
	float bannerStringWidth = smallFont->getStringWidth(MCOSU_BANNER);
	int bannerDiff = 20;
	int bannerMargin = 5;
	int numBanners = (int)roundf(Osu::getScreenWidth() / (bannerStringWidth + bannerDiff)) + 2;

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
	g->translate(7, Osu::getScreenHeight()/2 - smallFont->getHeight()/2);
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

	// draw main button text overlay (+ pulse rect anim)
	// HACKHACK: redo this properly, with an image or something
	const float div = 1.25f;
	const float pulse = (div - fmod(engine->getTime(), div))/div;
	//pulse *= pulse; // quadratic

	Vector2 size = m_vSize;
	const float pulseSub = 0.05f*pulse;

	size -= size*pulseSub;
	size += size*m_fSizeAddAnim;

	Rect rect = Rect(m_vCenter.x - size.x/2.0f - m_fCenterOffsetAnim, m_vCenter.y - size.y/2.0f, size.x, size.y);

	g->setColor(0xff000000);
	g->fillRect(rect.getX(), rect.getY(), rect.getWidth(), rect.getHeight());
	g->setColor(0xffffffff);
	g->drawRect(rect.getX(), rect.getY(), rect.getWidth(), rect.getHeight());

	g->setColor(0xffffffff);
	float fontScale = 1.0f - pulseSub + m_fSizeAddAnim;
	//fontScale *= 2;
	UString title = "McOsu!";
	g->pushTransform();
	g->scale(fontScale, fontScale);
	g->translate(m_vCenter.x - m_fCenterOffsetAnim - (titleFont->getStringWidth(title)/2.0f)*fontScale, m_vCenter.y + (titleFont->getHeight()*fontScale)/2.25f);
	g->drawString(titleFont, title);
	g->popTransform();

	drawVersionInfo(g);
}

void OsuMainMenu::drawVersionInfo(Graphics *g)
{
	McFont *versionFont = engine->getResourceManager()->getFont("FONT_DEFAULT");

	g->pushTransform();
	g->translate(7, Osu::getScreenHeight() - 7);
	g->drawString(versionFont, MCOSU_VERSION);
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

	// handle automatic menu closing
	if (m_fMainMenuButtonCloseTime != 0.0f && engine->getTime() > m_fMainMenuButtonCloseTime)
	{
		m_fMainMenuButtonCloseTime = 0.0f;
		setMenuElementsVisible(false);
	}

	if (m_fShutdownScheduledTime != 0.0f && engine->getTime() > m_fShutdownScheduledTime)
	{
		engine->shutdown();
		m_fShutdownScheduledTime = 0.0f;
	}
}

void OsuMainMenu::onKeyDown(KeyboardEvent &e)
{
	if (!m_bVisible)
		return;
}

void OsuMainMenu::onResolutionChange(Vector2 newResolution)
{
	updateLayout();
	setMenuElementsVisible(m_bMenuElementsVisible);
}

void OsuMainMenu::setVisible(bool visible)
{
	m_bVisible = visible;

	if (!visible)
		setMenuElementsVisible(false, false);
	else
		updateLayout();
}

void OsuMainMenu::updateLayout()
{
	m_vCenter = Osu::getScreenSize()/2.0f;
	float size = m_osu->getUIScale(324.0f);
	m_vSize = Vector2(size, size);

	m_pauseButton->setSize(30, 30);
	m_pauseButton->setRelPos(Osu::getScreenWidth() - m_pauseButton->getSize().x*2 - 10, m_pauseButton->getSize().y + 10);

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

	m_container->setSize(Osu::getScreenSize());
	m_container->update_pos();
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
			anim->moveQuadInOut(&m_fCenterOffsetAnim, 0.0f, 0.5f, 0.0f, true);
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
	if (m_osu->getBeatmap() != NULL)
		m_osu->getBeatmap()->pausePreviewMusic();
}



OsuMainMenuMainButton::OsuMainMenuMainButton(OsuMainMenu *mainMenu, float xPos, float yPos, float xSize, float ySize, UString name, UString text) : CBaseUIButton(xPos, yPos, xSize, ySize, name, text)
{
	m_mainMenu = mainMenu;
}

void OsuMainMenuMainButton::draw(Graphics *g)
{
	// draw nothing
	/// CBaseUIButton::draw(g);
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
