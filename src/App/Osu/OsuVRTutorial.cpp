//================ Copyright (c) 2017, PG, All rights reserved. =================//
//
// Purpose:		tutorial screen for controls
//
// $NoKeywords: $osuvrtut
//===============================================================================//

#include "OsuVRTutorial.h"

#include "Engine.h"
#include "ResourceManager.h"
#include "SoundEngine.h"
#include "Keyboard.h"

#include "CBaseUIContainer.h"
#include "CBaseUIScrollView.h"
#include "CBaseUIImage.h"
#include "CBaseUIImageButton.h"

#include "Osu.h"
#include "OsuSkin.h"

OsuVRTutorial::OsuVRTutorial(Osu *osu) : OsuScreenBackable(osu)
{
	m_container = new CBaseUIContainer(-1, -1, 0, 0, "");
	m_scrollView = new CBaseUIScrollView(-1, -1, 0, 0, "");
	m_scrollView->setHorizontalScrolling(false);
	m_scrollView->setScrollbarColor(0xff000000);
	m_scrollView->setDrawBackground(false);
	m_scrollView->setDrawFrame(false);
	m_scrollView->setScrollResistance(0);
	m_container->addBaseUIElement(m_scrollView);

	// load/execute VR stuff only in VR builds
	if (m_osu->isInVRMode())
	{
		engine->getResourceManager()->loadImage("mcosuvr_controls_1.png", "OSU_VR_CONTROLS_TUTORIAL_1");
		engine->getResourceManager()->loadImage("mcosuvr_controls_2.png", "OSU_VR_CONTROLS_TUTORIAL_2");
	}

	m_tutorialImage1 = new CBaseUIImage("OSU_VR_CONTROLS_TUTORIAL_1", 0, 0, 0, 0, "");
	m_scrollView->getContainer()->addBaseUIElement(m_tutorialImage1);
	m_tutorialImage2 = new CBaseUIImage("OSU_VR_CONTROLS_TUTORIAL_2", 0, 0, 0, 0, "");
	m_scrollView->getContainer()->addBaseUIElement(m_tutorialImage2);

	m_backButton->setVisible(false);
}

OsuVRTutorial::~OsuVRTutorial()
{
	SAFE_DELETE(m_container);
}

void OsuVRTutorial::draw(Graphics *g)
{
	if (!m_bVisible) return;

	m_container->draw(g);

	OsuScreenBackable::draw(g);
}

void OsuVRTutorial::update()
{
	OsuScreenBackable::update();
	if (!m_bVisible) return;

	m_container->update();

	float minPosToUnlockBackButton = m_tutorialImage1->getSize().y*0.75f;
	if (m_scrollView->getScrollPosY() < -minPosToUnlockBackButton && !m_backButton->isVisible())
		m_backButton->setVisible(true);
}

void OsuVRTutorial::onKeyDown(KeyboardEvent &e)
{
	if (!m_bVisible) return;

	if (e == KEY_ESCAPE && !m_backButton->isVisible())
		e.consume();
	else
		OsuScreenBackable::onKeyDown(e);
}

void OsuVRTutorial::setVisible(bool visible)
{
	OsuScreenBackable::setVisible(visible);

	if (m_bVisible)
		updateLayout();
}

void OsuVRTutorial::updateLayout()
{
	OsuScreenBackable::updateLayout();

	m_container->setSize(m_osu->getScreenSize() + Vector2(2,2));
	m_scrollView->setSize(m_osu->getScreenSize() + Vector2(2,2));

	// TODO: this scaling shit should be part of CBaseUIImage, clean up sometime
	Vector2 imageSize = m_tutorialImage1->getImage() != NULL ? m_tutorialImage1->getImage()->getSize() : Vector2(1,1);
	float scale = (m_osu->getScreenWidth()+1) / imageSize.x;
	m_tutorialImage1->setScale(scale, scale);
	m_tutorialImage1->setSize(imageSize*scale);

	imageSize = m_tutorialImage2->getImage() != NULL ? m_tutorialImage2->getImage()->getSize() : Vector2(1,1);
	scale = (m_osu->getScreenWidth()+1) / imageSize.x;
	m_tutorialImage2->setRelPosY(m_tutorialImage1->getSize().y - 2);
	m_tutorialImage2->setScale(scale, scale);
	m_tutorialImage2->setSize(imageSize*scale);

	m_scrollView->setScrollSizeToContent(0);
}

void OsuVRTutorial::onBack()
{
	engine->getSound()->play(m_osu->getSkin()->getMenuClick());

	m_osu->toggleVRTutorial();
}
