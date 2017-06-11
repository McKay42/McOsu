//================ Copyright (c) 2015, PG, All rights reserved. =================//
//
// Purpose:		main menu
//
// $NoKeywords: $osumain
//===============================================================================//

#include "OsuUpdateHandler.h"

#include "Engine.h"
#include "SoundEngine.h"
#include "Keyboard.h"
#include "Mouse.h"
#include "SoundEngine.h"
#include "ResourceManager.h"
#include "AnimationHandler.h"
#include "VertexArrayObject.h"
#include "ConVar.h"
#include "File.h"

#include "Osu.h"
#include "OsuSkin.h"
#include "OsuSkinImage.h"
#include "OsuBeatmap.h"
#include "OsuBeatmapDifficulty.h"
#include "OsuMainMenu.h"
#include "OsuHUD.h"

#include "OsuUIButton.h"

#include "CBaseUIContainer.h"
#include "CBaseUIButton.h"

#define MCOSU_VERSION_TEXT "Alpha"
#define MCOSU_BANNER_TEXT "-SteamVR Bug! Random freezes on controller vibration-"
UString OsuMainMenu::MCOSU_MAIN_BUTTON_TEXT = UString("McOsu");
UString OsuMainMenu::MCOSU_MAIN_BUTTON_SUBTEXT = UString("Practice Client");
#define MCOSU_MAIN_BUTTON_BACK_TEXT "by McKay"

#define MCOSU_NEWVERSION_NOTIFICATION_TRIGGER_FILE "version.txt"



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
	OsuMainMenuPauseButton(float xPos, float yPos, float xSize, float ySize, UString name, UString text) : CBaseUIButton(xPos, yPos, xSize, ySize, name, text)
	{
		m_bIsPaused = true;
	}

	virtual void draw(Graphics *g)
	{
		int third = m_vSize.x/3;

		g->setColor(0xffffffff);

		if (!m_bIsPaused)
		{
			g->fillRect(m_vPos.x, m_vPos.y, third, m_vSize.y + 1);
			g->fillRect(m_vPos.x + 2*third, m_vPos.y, third, m_vSize.y + 1);
		}
		else
		{
			// HACKHACK: disable GL_TEXTURE_2D
			// DEPRECATED LEGACY
			g->setColor(0x00000000);
			g->drawPixel(m_vPos.x, m_vPos.y);

			g->setColor(0xffffffff);
			VertexArrayObject vao;

			const int smoothPixels = 2;

			// center triangle
			vao.addVertex(m_vPos.x, m_vPos.y + smoothPixels);
			vao.addColor(0xffffffff);
			vao.addVertex(m_vPos.x + m_vSize.x, m_vPos.y + m_vSize.y/2);
			vao.addColor(0xffffffff);
			vao.addVertex(m_vPos.x, m_vPos.y + m_vSize.y - smoothPixels);
			vao.addColor(0xffffffff);

			// top smooth
			vao.addVertex(m_vPos.x, m_vPos.y + smoothPixels);
			vao.addColor(0xffffffff);
			vao.addVertex(m_vPos.x, m_vPos.y);
			vao.addColor(0x00000000);
			vao.addVertex(m_vPos.x + m_vSize.x, m_vPos.y + m_vSize.y/2);
			vao.addColor(0xffffffff);

			vao.addVertex(m_vPos.x, m_vPos.y);
			vao.addColor(0x00000000);
			vao.addVertex(m_vPos.x + m_vSize.x, m_vPos.y + m_vSize.y/2);
			vao.addColor(0xffffffff);
			vao.addVertex(m_vPos.x + m_vSize.x, m_vPos.y + m_vSize.y/2 - smoothPixels);
			vao.addColor(0x00000000);

			// bottom smooth
			vao.addVertex(m_vPos.x, m_vPos.y + m_vSize.y - smoothPixels);
			vao.addColor(0xffffffff);
			vao.addVertex(m_vPos.x, m_vPos.y + m_vSize.y);
			vao.addColor(0x00000000);
			vao.addVertex(m_vPos.x + m_vSize.x, m_vPos.y + m_vSize.y/2);
			vao.addColor(0xffffffff);

			vao.addVertex(m_vPos.x, m_vPos.y + m_vSize.y);
			vao.addColor(0x00000000);
			vao.addVertex(m_vPos.x + m_vSize.x, m_vPos.y + m_vSize.y/2);
			vao.addColor(0xffffffff);
			vao.addVertex(m_vPos.x + m_vSize.x, m_vPos.y + m_vSize.y/2 + smoothPixels);
			vao.addColor(0x00000000);

			g->drawVAO(&vao);
		}

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

	void setPaused(bool paused) {m_bIsPaused = paused;}

private:
	bool m_bIsPaused;
};



OsuMainMenu::OsuMainMenu(Osu *osu) : OsuScreen()
{
	m_osu = osu;

	if (m_osu->isInVRMode())
		MCOSU_MAIN_BUTTON_TEXT.append(" VR");
	if (m_osu->isInVRMode())
		MCOSU_MAIN_BUTTON_SUBTEXT.clear();

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

	m_fUpdateStatusTime = 0.0f;
	m_fUpdateButtonTextTime = 0.0f;
	m_fUpdateButtonAnim = 0.0f;
	m_fUpdateButtonAnimTime = 0.0f;
	m_bHasClickedUpdate = false;

	// check if the user has never clicked the changelog for this update
	m_bDrawVersionNotificationArrow = false;
	if (env->fileExists(MCOSU_NEWVERSION_NOTIFICATION_TRIGGER_FILE))
	{
		File versionFile(MCOSU_NEWVERSION_NOTIFICATION_TRIGGER_FILE);
		if (versionFile.canRead())
		{
			float version = versionFile.readLine().toFloat();
			if (version < Osu::version->getFloat() - 0.0001f)
				m_bDrawVersionNotificationArrow = true;
		}
		else
			m_bDrawVersionNotificationArrow = true;
	}
	else
		m_bDrawVersionNotificationArrow = true;

	m_container = new CBaseUIContainer(-1, 0, m_osu->getScreenWidth(), m_osu->getScreenHeight(), "");
	m_mainButton = new OsuMainMenuMainButton(this, 0, 0, 1, 1, "", "");

	m_container->addBaseUIElement(m_mainButton);

	addMainMenuButton("Play")->setClickCallback( fastdelegate::MakeDelegate(this, &OsuMainMenu::onPlayButtonPressed) );
	//addMainMenuButton("Edit")->setClickCallback( fastdelegate::MakeDelegate(this, &OsuMainMenu::onEditButtonPressed) );
	addMainMenuButton("Options")->setClickCallback( fastdelegate::MakeDelegate(this, &OsuMainMenu::onOptionsButtonPressed) );
	addMainMenuButton("Exit")->setClickCallback( fastdelegate::MakeDelegate(this, &OsuMainMenu::onExitButtonPressed) );

	m_pauseButton = new OsuMainMenuPauseButton(0, 0, 0, 0, "", "");
	m_pauseButton->setClickCallback( fastdelegate::MakeDelegate(this, &OsuMainMenu::onPausePressed) );
	m_container->addBaseUIElement(m_pauseButton);

	m_updateAvailableButton = new OsuUIButton(m_osu, 0, 0, 0, 0, "", Osu::debug->getBool() ? "Debug mode, update check disabled" : "Checking for updates ...");
	m_updateAvailableButton->setUseDefaultSkin();
	m_updateAvailableButton->setClickCallback( fastdelegate::MakeDelegate(this, &OsuMainMenu::onUpdatePressed) );
	m_updateAvailableButton->setColor(0x2200ff00);
	m_updateAvailableButton->setTextColor(0x22ffffff);

	m_githubButton = new OsuUIButton(m_osu, 0, 0, 0, 0, "", "Github");
	m_githubButton->setUseDefaultSkin();
	m_githubButton->setClickCallback( fastdelegate::MakeDelegate(this, &OsuMainMenu::onGithubPressed) );
	m_githubButton->setColor(0x2923b9ff);
	m_githubButton->setTextBrightColor(0x55172e62);
	m_githubButton->setTextDarkColor(0x11ffffff);
	m_githubButton->setAlphaAddOnHover(0.5f);
	m_container->addBaseUIElement(m_githubButton);

	m_versionButton = new CBaseUIButton(0, 0, 0, 0, "", "");
	UString versionString = MCOSU_VERSION_TEXT;
	versionString.append(" ");
	versionString.append(Osu::version->getString());
	m_versionButton->setText(versionString);
	m_versionButton->setDrawBackground(false);
	m_versionButton->setDrawFrame(false);
	m_versionButton->setClickCallback( fastdelegate::MakeDelegate(this, &OsuMainMenu::onVersionPressed) );
	m_container->addBaseUIElement(m_versionButton);
}

OsuMainMenu::~OsuMainMenu()
{
	SAFE_DELETE(m_container);
	SAFE_DELETE(m_updateAvailableButton);
}

void OsuMainMenu::draw(Graphics *g)
{
	if (!m_bVisible)
		return;

	McFont *smallFont = m_osu->getSubTitleFont() /*engine->getResourceManager()->getFont("FONT_DEFAULT")*/;
	McFont *titleFont = m_osu->getTitleFont();

	// main button stuff
	bool haveTimingpoints = false;
	const float div = 1.25f;
	float pulse = 0.0f;
	if (m_osu->getSelectedBeatmap() != NULL && m_osu->getSelectedBeatmap()->getSelectedDifficulty() != NULL && m_osu->getSelectedBeatmap()->getMusic() != NULL && m_osu->getSelectedBeatmap()->getMusic()->isPlaying())
	{
		haveTimingpoints = true;
		long curMusicPos = m_osu->getSelectedBeatmap()->getMusic()->getPositionMS();
		OsuBeatmapDifficulty::TIMING_INFO t = m_osu->getSelectedBeatmap()->getSelectedDifficulty()->getTimingInfoForTime(curMusicPos);
		if (t.beatLengthBase == 0.0f) // bah
			t.beatLengthBase = 1.0f;
		pulse = (float)((curMusicPos - t.offset) % (long)t.beatLengthBase) / t.beatLengthBase;
	}
	else
		pulse = (div - fmod(engine->getTime(), div))/div;

	//pulse *= pulse; // quadratic
	Vector2 size = m_vSize;
	const float pulseSub = 0.05f*pulse;
	size -= size*pulseSub;
	size += size*m_fSizeAddAnim;
	McRect mainButtonRect = McRect(m_vCenter.x - size.x/2.0f - m_fCenterOffsetAnim, m_vCenter.y - size.y/2.0f, size.x, size.y);

	// draw banner
	if (m_osu->isInVRMode())
	{
		McFont *bannerFont = m_osu->getSubTitleFont();
		float bannerStringWidth = bannerFont->getStringWidth(MCOSU_BANNER_TEXT);
		int bannerDiff = 20;
		int bannerMargin = 5;
		int numBanners = (int)std::round(m_osu->getScreenWidth() / (bannerStringWidth + bannerDiff)) + 2;

		g->setColor(0xffee7777);
		g->pushTransform();
		g->translate(1, 1);
		for (int i=-1; i<numBanners; i++)
		{
			g->pushTransform();
			g->translate(i*bannerStringWidth + i*bannerDiff + fmod(engine->getTime()*30, bannerStringWidth + bannerDiff), bannerFont->getHeight() + bannerMargin);
			g->drawString(bannerFont, MCOSU_BANNER_TEXT);
			g->popTransform();
		}
		g->popTransform();
		g->setColor(0xff555555);
		for (int i=-1; i<numBanners; i++)
		{
			g->pushTransform();
			g->translate(i*bannerStringWidth + i*bannerDiff + fmod(engine->getTime()*30, bannerStringWidth + bannerDiff), bannerFont->getHeight() + bannerMargin);
			g->drawString(bannerFont, MCOSU_BANNER_TEXT);
			g->popTransform();
		}
	}

	// draw notification arrow for changelog (version button)
	if (m_bDrawVersionNotificationArrow)
	{
		float animation = fmod((float)(engine->getTime())*3.2f, 2.0f);
		if (animation > 1.0f)
			animation = 2.0f - animation;
		animation =  -animation*(animation-2); // quad out
		float offset = m_osu->getUIScale(m_osu, 45.0f*animation);

		const float scale = m_versionButton->getSize().x / m_osu->getSkin()->getPlayWarningArrow2()->getSizeBaseRaw().x;

		g->setColor(0xffffffff);
		g->pushTransform();
		g->rotate(90.0f);
		g->translate(0, -offset*2, 0);
		m_osu->getSkin()->getPlayWarningArrow2()->drawRaw(g, Vector2(m_versionButton->getSize().x/2, m_osu->getScreenHeight() - m_versionButton->getSize().y*2 - m_versionButton->getSize().y*scale), scale);
		g->popTransform();
	}

	// draw container
	m_container->draw(g);

	// draw update check button
	if (m_osu->getUpdateHandler()->getStatus() == OsuUpdateHandler::STATUS::STATUS_SUCCESS_INSTALLATION)
	{
		g->push3DScene(McRect(m_updateAvailableButton->getPos().x, m_updateAvailableButton->getPos().y, m_updateAvailableButton->getSize().x, m_updateAvailableButton->getSize().y));
		g->rotate3DScene(m_fUpdateButtonAnim*360.0f, 0, 0);
	}
	m_updateAvailableButton->draw(g);
	if (m_osu->getUpdateHandler()->getStatus() == OsuUpdateHandler::STATUS::STATUS_SUCCESS_INSTALLATION)
		g->pop3DScene();

	// draw main button
	if (m_fMainMenuAnim > 0.0f && m_fMainMenuAnim != 1.0f)
	{
		g->push3DScene(mainButtonRect);
		g->rotate3DScene(m_fMainMenuAnim1*360.0f, m_fMainMenuAnim2*360.0f, m_fMainMenuAnim3*360.0f);

		/*
		g->offset3DScene(0, 0, 300.0f);
		g->rotate3DScene(0, m_fCenterOffsetAnim*0.13f, 0);
		*/
	}

	/*
	g->setDepthBuffer(true);
	g->clearDepthBuffer();
	g->setCulling(true);
	*/

	// front side
	g->setColor(0xff000000);
	g->pushTransform();
	g->translate(0, 0, -0.1f);
	g->fillRect(mainButtonRect.getX(), mainButtonRect.getY(), mainButtonRect.getWidth(), mainButtonRect.getHeight());
	g->popTransform();
	g->setColor(0xffffffff);
	g->drawRect(mainButtonRect.getX(), mainButtonRect.getY(), mainButtonRect.getWidth(), mainButtonRect.getHeight());

	/*
	// right side
	g->offset3DScene(0, 0, mainButtonRect.getWidth()/2);
	g->rotate3DScene(0, 90, 0);
	{
		g->setColor(0xff00ff00);
		g->pushTransform();
		g->translate(0, 0, -0.1f);
		g->fillRect(mainButtonRect.getX(), mainButtonRect.getY(), mainButtonRect.getWidth(), mainButtonRect.getHeight());
		g->popTransform();
		//g->setColor(0xffffffff);
		//g->drawRect(mainButtonRect.getX(), mainButtonRect.getY(), mainButtonRect.getWidth(), mainButtonRect.getHeight());
	}
	g->rotate3DScene(0, -90, 0);
	g->offset3DScene(0, 0, 0);

	// left side
	g->offset3DScene(0, 0, mainButtonRect.getWidth()/2);
	g->rotate3DScene(0, -90, 0);
	{
		g->setColor(0xffffff00);
		g->pushTransform();
		g->translate(0, 0, -0.1f);
		g->fillRect(mainButtonRect.getX(), mainButtonRect.getY(), mainButtonRect.getWidth(), mainButtonRect.getHeight());
		g->popTransform();
		//g->setColor(0xffffffff);
		//g->drawRect(mainButtonRect.getX(), mainButtonRect.getY(), mainButtonRect.getWidth(), mainButtonRect.getHeight());
	}
	g->rotate3DScene(0, 90, 0);
	g->offset3DScene(0, 0, 0);

	// top side
	g->offset3DScene(0, 0, mainButtonRect.getHeight()/2);
	g->rotate3DScene(90, 0, 0);
	{
		g->setColor(0xff00ffff);
		g->pushTransform();
		g->translate(0, 0, -0.1f);
		g->fillRect(mainButtonRect.getX(), mainButtonRect.getY(), mainButtonRect.getWidth(), mainButtonRect.getHeight());
		g->popTransform();
		//g->setColor(0xffffffff);
		//g->drawRect(mainButtonRect.getX(), mainButtonRect.getY(), mainButtonRect.getWidth(), mainButtonRect.getHeight());
	}
	g->rotate3DScene(-90, 0, 0);
	g->offset3DScene(0, 0, 0);

	// bottom side
	g->offset3DScene(0, 0, mainButtonRect.getHeight()/2);
	g->rotate3DScene(-90, 0, 0);
	{
		g->setColor(0xffff0000);
		g->pushTransform();
		g->translate(0, 0, -0.1f);
		g->fillRect(mainButtonRect.getX(), mainButtonRect.getY(), mainButtonRect.getWidth(), mainButtonRect.getHeight());
		g->popTransform();
		//g->setColor(0xffffffff);
		//g->drawRect(mainButtonRect.getX(), mainButtonRect.getY(), mainButtonRect.getWidth(), mainButtonRect.getHeight());
	}
	g->rotate3DScene(90, 0, 0);
	g->offset3DScene(0, 0, 0);
	*/

	float fontScale = 1.0f - pulseSub + m_fSizeAddAnim;

	g->setColor(0xffffffff);
	g->pushTransform();
	g->scale(fontScale, fontScale);
	g->translate(m_vCenter.x - m_fCenterOffsetAnim - (titleFont->getStringWidth(MCOSU_MAIN_BUTTON_TEXT)/2.0f)*fontScale, m_vCenter.y + (titleFont->getHeight()*fontScale)/2.25f);
	g->drawString(titleFont, MCOSU_MAIN_BUTTON_TEXT);
	g->popTransform();

	if ((m_fMainMenuAnim1*360.0f > 90.0f && m_fMainMenuAnim1*360.0f < 270.0f) || (m_fMainMenuAnim2*360.0f > 90.0f && m_fMainMenuAnim2*360.0f < 270.0f)
	 || (m_fMainMenuAnim1*360.0f < -90.0f && m_fMainMenuAnim1*360.0f > -270.0f) || (m_fMainMenuAnim2*360.0f < -90.0f && m_fMainMenuAnim2*360.0f > -270.0f))
	{
		// huehuehuehuehue
	}
	else
	{
		if (MCOSU_MAIN_BUTTON_SUBTEXT.length() > 0)
		{
			float invertedPulse = 1.0f - pulse;

			if (haveTimingpoints)
				g->setColor(COLORf(1.0f, 0.10f + 0.15f*invertedPulse, 0.10f + 0.15f*invertedPulse, 0.10f + 0.15f*invertedPulse));
			else
				g->setColor(0xff444444);

			g->pushTransform();
			g->scale(fontScale, fontScale);
			g->translate(m_vCenter.x - m_fCenterOffsetAnim - (smallFont->getStringWidth(MCOSU_MAIN_BUTTON_SUBTEXT)/2.0f)*fontScale, m_vCenter.y + (mainButtonRect.getHeight()/2.0f)/2.0f + (smallFont->getHeight()*fontScale)/2.0f);
			g->drawString(smallFont, MCOSU_MAIN_BUTTON_SUBTEXT);
			g->popTransform();
		}
	}

	if (m_fMainMenuAnim > 0.0f && m_fMainMenuAnim != 1.0f)
	{
		if ((m_fMainMenuAnim1*360.0f > 90.0f && m_fMainMenuAnim1*360.0f < 270.0f) || (m_fMainMenuAnim2*360.0f > 90.0f && m_fMainMenuAnim2*360.0f < 270.0f)
		 || (m_fMainMenuAnim1*360.0f < -90.0f && m_fMainMenuAnim1*360.0f > -270.0f) || (m_fMainMenuAnim2*360.0f < -90.0f && m_fMainMenuAnim2*360.0f > -270.0f))
		{
			g->setColor(0xffffffff);
			g->pushTransform();
			g->scale(fontScale, fontScale);
			g->translate(m_vCenter.x - m_fCenterOffsetAnim - (titleFont->getStringWidth(MCOSU_MAIN_BUTTON_BACK_TEXT)/2.0f)*fontScale, m_vCenter.y + ((titleFont->getHeight()*fontScale)/2.25f)*4.0f);
			g->drawString(titleFont, MCOSU_MAIN_BUTTON_BACK_TEXT);
			g->popTransform();
		}
		g->pop3DScene();
	}

	/*
	g->setCulling(false);
	g->setDepthBuffer(false);
	*/
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

	// handle update checker and status text
	switch (m_osu->getUpdateHandler()->getStatus())
	{
	case OsuUpdateHandler::STATUS::STATUS_UP_TO_DATE:
		if (m_updateAvailableButton->isVisible())
		{
			m_updateAvailableButton->setText("");
			m_updateAvailableButton->setVisible(false);
		}
		break;
	case OsuUpdateHandler::STATUS::STATUS_CHECKING_FOR_UPDATE:
		m_updateAvailableButton->setText("Checking for updates ...");
		break;
	case OsuUpdateHandler::STATUS::STATUS_DOWNLOADING_UPDATE:
		m_updateAvailableButton->setText("Downloading ...");
		break;
	case OsuUpdateHandler::STATUS::STATUS_INSTALLING_UPDATE:
		m_updateAvailableButton->setText("Installing ...");
		break;
	case OsuUpdateHandler::STATUS::STATUS_SUCCESS_INSTALLATION:
		if (engine->getTime() > m_fUpdateButtonTextTime && anim->isAnimating(&m_fUpdateButtonAnim) && m_fUpdateButtonAnim > 0.175f)
		{
			m_fUpdateButtonTextTime = m_fUpdateButtonAnimTime;

			m_updateAvailableButton->setColor(0xff00ff00);
			m_updateAvailableButton->setTextColor(0xffffffff);

			if (m_updateAvailableButton->getText().find("ready") != -1)
				m_updateAvailableButton->setText("Click here to restart now!");
			else
				m_updateAvailableButton->setText("A new version of McOsu is ready!");
		}
		break;
	case OsuUpdateHandler::STATUS::STATUS_ERROR:
		m_updateAvailableButton->setText("Update Error! Click to retry ...");
		break;
	}

	if (m_osu->getUpdateHandler()->getStatus() == OsuUpdateHandler::STATUS::STATUS_SUCCESS_INSTALLATION && engine->getTime() > m_fUpdateButtonAnimTime)
	{
		m_fUpdateButtonAnimTime = engine->getTime() + 3.0f;
		m_fUpdateButtonAnim = 0.0f;
		anim->moveQuadInOut(&m_fUpdateButtonAnim, 1.0f, 0.5f, true);
	}

	// handle pause button pause detection
	if (m_osu->getSelectedBeatmap() != NULL)
		m_pauseButton->setPaused(!m_osu->getSelectedBeatmap()->isPreviewMusicPlaying());
	else
		m_pauseButton->setPaused(true);
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
	if (!m_bVisible) return;

	// debug anims
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

	m_githubButton->setSize(100, 50);
	m_githubButton->setRelPos(5, m_osu->getScreenHeight()/2.0f - m_githubButton->getSize().y/2.0f);

	m_versionButton->setSizeToContent(8, 8);
	m_versionButton->setRelPos(-1, m_osu->getScreenSize().y - m_versionButton->getSize().y);

	m_mainButton->setRelPos(m_vCenter - m_vSize/2.0f - Vector2(m_fCenterOffsetAnim, 0.0f));
	m_mainButton->setSize(m_vSize);

	int numButtons = m_menuElements.size();
	int menuElementHeight = m_vSize.y / numButtons;
	int menuElementPadding = numButtons > 3 ? m_vSize.y*0.04f : m_vSize.y*0.075f;
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

	m_container->setSize(m_osu->getScreenSize() + Vector2(1,1));
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

void OsuMainMenu::onEditButtonPressed()
{
	m_osu->toggleEditor();
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
	if (m_osu->getUpdateHandler()->getStatus() == OsuUpdateHandler::STATUS::STATUS_SUCCESS_INSTALLATION)
		engine->restart();
	else if (m_osu->getUpdateHandler()->getStatus() == OsuUpdateHandler::STATUS::STATUS_ERROR)
		m_osu->getUpdateHandler()->checkForUpdates();
}

void OsuMainMenu::onGithubPressed()
{
	env->openURLInDefaultBrowser("https://github.com/McKay42/McOsu/");
}

void OsuMainMenu::onVersionPressed()
{
	m_bDrawVersionNotificationArrow = false;

	// remember, don't show the notification arrow until the version changes again
	std::ofstream versionFile(MCOSU_NEWVERSION_NOTIFICATION_TRIGGER_FILE);
	if (versionFile.good())
	{
		versionFile << Osu::version->getFloat();
		versionFile.close();
	}

	m_osu->toggleChangelog();
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
