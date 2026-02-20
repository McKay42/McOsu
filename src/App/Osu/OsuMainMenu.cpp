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
#include "SteamworksInterface.h"
#include "VertexArrayObject.h"
#include "ConVar.h"
#include "File.h"

#include "HorizonSDLEnvironment.h"

#include "Osu.h"
#include "OsuSkin.h"
#include "OsuSkinImage.h"

#include "OsuSongBrowser2.h"
#include "OsuBackgroundImageHandler.h"
#include "OsuDatabaseBeatmap.h"
#include "OsuBeatmapStandard.h"
#include "OsuGameRules.h"
#include "OsuSliderRenderer.h"
#include "OsuSliderCurves.h"

#include "OsuSlider.h"

#include "OsuMainMenu.h"
#include "OsuOptionsMenu.h"
#include "OsuHUD.h"
#include "OsuRichPresence.h"

#include "OsuUIButton.h"

#include "CBaseUIContainer.h"
#include "CBaseUIButton.h"

#include "DirectX11Interface.h"

#define MCOSU_VERSION_TEXT "Version"
#define MCOSU_BANNER_TEXT ""
UString OsuMainMenu::MCOSU_MAIN_BUTTON_TEXT = UString("McOsu");
UString OsuMainMenu::MCOSU_MAIN_BUTTON_SUBTEXT = UString("Practice Client");
#define MCOSU_MAIN_BUTTON_BACK_TEXT "by McKay"

#define MCOSU_NEWVERSION_NOTIFICATION_TRIGGER_FILE "version.txt"

static const char *s_sliderTextBeatmap = "osu file format v14\r\n"
	"\r\n"
	"[General]\r\n"
	"AudioFilename: nothing.mp3\r\n"
	"\r\n"
	"[Metadata]\r\n"
	"Title:McOsu Slider Text\r\n"
	"TitleUnicode:McOsu Slider Text\r\n"
	"Artist:McKay\r\n"
	"ArtistUnicode:McKay\r\n"
	"Creator:McKay\r\n"
	"Version:McOsu\r\n"
	"\r\n"
	"[Difficulty]\r\n"
	"HPDrainRate:6\r\n"
	"CircleSize:6\r\n"
	"OverallDifficulty:5\r\n"
	"ApproachRate:0\r\n"
	"SliderMultiplier:10\r\n"
	"SliderTickRate:1\r\n"
	"\r\n"
	"[TimingPoints]\r\n"
	"1717.04358603418,1200,4,1,0,27,1,0\r\n"
	"\r\n"
	"[HitObjects]\r\n"
	"-143,275,0,6,0,B|-143:115|-143:115|-79:179|-79:179|-20:115|-20:115|-20:275,1,498.750001415609\r\n"
	"112,197,0,2,0,P|45:236|115:262,1,183.75000052154\r\n"
	"263,111,0,2,0,P|282:272|251:111,1,520\r\n"
	"480,179,0,2,0,B|288:179|480:243|544:275|384:275,1,262.500000745058\r\n"
	"543,182,0,2,0,B|511:309|671:309|616:165,1,236.250000670552";



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



ConVar osu_toggle_preview_music("osu_toggle_preview_music");

ConVar osu_draw_menu_background("osu_draw_menu_background", true, FCVAR_NONE);
ConVar osu_draw_main_menu_button("osu_draw_main_menu_button", true, FCVAR_NONE);
ConVar osu_draw_main_menu_button_subtext("osu_draw_main_menu_button_subtext", true, FCVAR_NONE);
ConVar osu_draw_main_menu_workshop_button("osu_draw_main_menu_workshop_button", true, FCVAR_NONE);
ConVar osu_main_menu_startup_anim_duration("osu_main_menu_startup_anim_duration", 0.25f, FCVAR_NONE);
ConVar osu_main_menu_use_slider_text("osu_main_menu_use_slider_text", true, FCVAR_NONE);
ConVar osu_main_menu_slider_text_alpha("osu_main_menu_slider_text_alpha", 1.0f, FCVAR_NONE);
ConVar osu_main_menu_slider_text_scale("osu_main_menu_slider_text_scale", 1.0f, FCVAR_NONE);
ConVar osu_main_menu_slider_text_scissor("osu_main_menu_slider_text_scissor", true, FCVAR_NONE);
ConVar osu_main_menu_slider_text_feather("osu_main_menu_slider_text_feather", 0.04f, FCVAR_NONE);
ConVar osu_main_menu_slider_text_offset_x("osu_main_menu_slider_text_offset_x", 15.0f, FCVAR_NONE);
ConVar osu_main_menu_slider_text_offset_y("osu_main_menu_slider_text_offset_y", 0.0f, FCVAR_NONE);
ConVar osu_main_menu_shuffle("osu_main_menu_shuffle", false, FCVAR_NONE);
ConVar osu_main_menu_alpha("osu_main_menu_alpha", 1.0f, FCVAR_NONE);
ConVar osu_main_menu_friend("osu_main_menu_friend", true, FCVAR_NONE);

ConVar osu_main_menu_banner_always_text("osu_main_menu_banner_always_text", "", FCVAR_NONE);
ConVar osu_main_menu_banner_ifupdatedfromoldversion_text("osu_main_menu_banner_ifupdatedfromoldversion_text", "", FCVAR_NONE);
ConVar osu_main_menu_banner_ifupdatedfromoldversion_le3300_text("osu_main_menu_banner_ifupdatedfromoldversion_le3300_text", "", FCVAR_NONE);
ConVar osu_main_menu_banner_ifupdatedfromoldversion_le3303_text("osu_main_menu_banner_ifupdatedfromoldversion_le3303_text", "", FCVAR_NONE);
ConVar osu_main_menu_banner_ifupdatedfromoldversion_le3308_text("osu_main_menu_banner_ifupdatedfromoldversion_le3308_text", "", FCVAR_NONE);
ConVar osu_main_menu_banner_ifupdatedfromoldversion_le3310_text("osu_main_menu_banner_ifupdatedfromoldversion_le3310_text", "", FCVAR_NONE);
ConVar osu_main_menu_banner_ifupdatedfromoldversion_le3311_text("osu_main_menu_banner_ifupdatedfromoldversion_le3311_text", "", FCVAR_NONE);
ConVar osu_main_menu_banner_ifupdatedfromoldversion_le3312_text("osu_main_menu_banner_ifupdatedfromoldversion_le3312_text", "", FCVAR_NONE);

ConVar *OsuMainMenu::m_osu_universal_offset_ref = NULL;
ConVar *OsuMainMenu::m_osu_universal_offset_hardcoded_ref = NULL;
ConVar *OsuMainMenu::m_osu_old_beatmap_offset_ref = NULL;
ConVar *OsuMainMenu::m_win_snd_fallback_dsound_ref = NULL;
ConVar *OsuMainMenu::m_osu_universal_offset_hardcoded_fallback_dsound_ref = NULL;
ConVar *OsuMainMenu::m_osu_slider_border_feather_ref = NULL;
ConVar *OsuMainMenu::m_osu_mod_random_ref = NULL;
ConVar *OsuMainMenu::m_osu_songbrowser_background_fade_in_duration_ref = NULL;

void OsuMainMenu::openSteamWorkshopInGameOverlay(Osu *osu, bool launchInSteamIfOverlayDisabled)
{
	if (!steam->isGameOverlayEnabled())
	{
		if (engine->getTime() > 10.0f)
		{
			osu->getNotificationOverlay()->addNotification("Opening browser, please wait ...", 0xffffffff, false, 0.75f);
			openSteamWorkshopInDefaultBrowser(launchInSteamIfOverlayDisabled);
		}
		else
			osu->getNotificationOverlay()->addNotification(UString::format("Steam Overlay not ready or disabled, try again (%i/10) ...", (int)std::min(engine->getTime(), 10.0)), 0xffffff00);
	}
	else
		steam->openURLInGameOverlay("https://steamcommunity.com/app/607260/workshop/");
}

void OsuMainMenu::openSteamWorkshopInDefaultBrowser(bool launchInSteam)
{
	if (launchInSteam)
		env->openURLInDefaultBrowser("steam://url/SteamWorkshopPage/607260");
	else
		env->openURLInDefaultBrowser("https://steamcommunity.com/app/607260/workshop/");
}

OsuMainMenu::OsuMainMenu(Osu *osu) : OsuScreen(osu)
{
	if (env->getOS() == Environment::OS::OS_HORIZON)
		MCOSU_MAIN_BUTTON_TEXT.append(" NX");
	if (m_osu->isInVRMode())
		MCOSU_MAIN_BUTTON_TEXT.append(" VR");
	if (m_osu->isInVRMode())
		MCOSU_MAIN_BUTTON_SUBTEXT.clear();

	if (m_osu_universal_offset_ref == NULL)
		m_osu_universal_offset_ref = convar->getConVarByName("osu_universal_offset");
	if (m_osu_universal_offset_hardcoded_ref == NULL)
		m_osu_universal_offset_hardcoded_ref = convar->getConVarByName("osu_universal_offset_hardcoded");
	if (m_osu_old_beatmap_offset_ref == NULL)
		m_osu_old_beatmap_offset_ref = convar->getConVarByName("osu_old_beatmap_offset");
	if (m_win_snd_fallback_dsound_ref == NULL)
		m_win_snd_fallback_dsound_ref = convar->getConVarByName("win_snd_fallback_dsound");
	if (m_osu_universal_offset_hardcoded_fallback_dsound_ref == NULL)
		m_osu_universal_offset_hardcoded_fallback_dsound_ref = convar->getConVarByName("osu_universal_offset_hardcoded_fallback_dsound");
	if (m_osu_slider_border_feather_ref == NULL)
		m_osu_slider_border_feather_ref = convar->getConVarByName("osu_slider_border_feather");
	if (m_osu_mod_random_ref == NULL)
		m_osu_mod_random_ref = convar->getConVarByName("osu_mod_random");
	if (m_osu_songbrowser_background_fade_in_duration_ref == NULL)
		m_osu_songbrowser_background_fade_in_duration_ref = convar->getConVarByName("osu_songbrowser_background_fade_in_duration");

	osu_toggle_preview_music.setCallback( fastdelegate::MakeDelegate(this, &OsuMainMenu::onPausePressed) );

	// engine settings
	engine->getMouse()->addListener(this);

	m_fSizeAddAnim = 0.0f;
	m_fCenterOffsetAnim = 0.0f;
	m_bMenuElementsVisible = false;

	m_fMainMenuAnimTime = 0.0f;
	m_fMainMenuAnimDuration = 0.0f;
	m_fMainMenuAnim = 0.0f;
	m_fMainMenuAnim1 = 0.0f;
	m_fMainMenuAnim2 = 0.0f;
	m_fMainMenuAnim3 = 0.0f;
	m_fMainMenuAnim1Target = 0.0f;
	m_fMainMenuAnim2Target = 0.0f;
	m_fMainMenuAnim3Target = 0.0f;
	m_bInMainMenuRandomAnim = false;
	m_iMainMenuRandomAnimType = 0;
	m_iMainMenuAnimBeatCounter = 0;

	m_bMainMenuAnimFriend = false;
	m_bMainMenuAnimFadeToFriendForNextAnim = false;
	m_bMainMenuAnimFriendScheduled = false;
	m_fMainMenuAnimFriendPercent = 0.0f;
	m_fMainMenuAnimFriendEyeFollowX = 0.0f;
	m_fMainMenuAnimFriendEyeFollowY = 0.0f;

	m_fShutdownScheduledTime = 0.0f;
	m_bWasCleanShutdown = false;

	m_fUpdateStatusTime = 0.0f;
	m_fUpdateButtonTextTime = 0.0f;
	m_fUpdateButtonAnim = 0.0f;
	m_fUpdateButtonAnimTime = 0.0f;
	m_bHasClickedUpdate = false;

	m_bStartupAnim = true;
	m_fStartupAnim = 0.0f;
	m_fStartupAnim2 = 0.0f;

	m_fPrevShuffleTime = 0.0f;
	m_fBackgroundFadeInTime = 0.0f;

	// check if the user has never clicked the changelog for this update
	m_bDidUserUpdateFromOlderVersion = false;
	m_bDidUserUpdateFromOlderVersionLe3300 = false;
	m_bDidUserUpdateFromOlderVersionLe3303 = false;
	m_bDidUserUpdateFromOlderVersionLe3308 = false;
	m_bDidUserUpdateFromOlderVersionLe3310 = false;
	m_bDidUserUpdateFromOlderVersionLe3311 = false;
	m_bDidUserUpdateFromOlderVersionLe3312 = false;
	{
		m_bDrawVersionNotificationArrow = false;
		if (env->fileExists(MCOSU_NEWVERSION_NOTIFICATION_TRIGGER_FILE))
		{
			File versionFile(MCOSU_NEWVERSION_NOTIFICATION_TRIGGER_FILE);
			if (versionFile.canRead())
			{
				float version = versionFile.readLine().toFloat();
				if (version < Osu::version->getFloat() - 0.0001f)
					m_bDrawVersionNotificationArrow = true;

				if (version < 33.01f - 0.0001f)
					m_bDidUserUpdateFromOlderVersionLe3300 = true;
				if (version < 33.04f - 0.0001f)
					m_bDidUserUpdateFromOlderVersionLe3303 = true;
				if (version < 33.09f - 0.0001f)
					m_bDidUserUpdateFromOlderVersionLe3308 = true;
				if (version < 33.11f - 0.0001f)
					m_bDidUserUpdateFromOlderVersionLe3310 = true;
				if (version < 33.12f - 0.0001f)
					m_bDidUserUpdateFromOlderVersionLe3311 = true;
				if (version < 33.13f - 0.0001f)
					m_bDidUserUpdateFromOlderVersionLe3312 = true;
			}
			else
				m_bDrawVersionNotificationArrow = true;
		}
		else
			m_bDrawVersionNotificationArrow = false;
	}
	m_bDidUserUpdateFromOlderVersion = m_bDrawVersionNotificationArrow; // (same logic atm)

	m_container = new CBaseUIContainer(-1, 0, m_osu->getScreenWidth(), m_osu->getScreenHeight(), "");
	m_mainButton = new OsuMainMenuMainButton(this, 0, 0, 1, 1, "", "");

	m_container->addBaseUIElement(m_mainButton);

	addMainMenuButton("Play")->setClickCallback( fastdelegate::MakeDelegate(this, &OsuMainMenu::onPlayButtonPressed) );
	//addMainMenuButton("Edit")->setClickCallback( fastdelegate::MakeDelegate(this, &OsuMainMenu::onEditButtonPressed) );
	addMainMenuButton((m_osu->isInVRMode() ? "Options" : "Options (CTRL + O)"))->setClickCallback( fastdelegate::MakeDelegate(this, &OsuMainMenu::onOptionsButtonPressed) );
	addMainMenuButton("Exit")->setClickCallback( fastdelegate::MakeDelegate(this, &OsuMainMenu::onExitButtonPressed) );

	m_pauseButton = new OsuMainMenuPauseButton(0, 0, 0, 0, "", "");
	m_pauseButton->setClickCallback( fastdelegate::MakeDelegate(this, &OsuMainMenu::onPausePressed) );
	m_container->addBaseUIElement(m_pauseButton);

	m_updateAvailableButton = new OsuUIButton(m_osu, 0, 0, 0, 0, "", Osu::debug->getBool() ? "Debug mode, update check disabled" : "Checking for updates ...");
	m_updateAvailableButton->setUseDefaultSkin();
	m_updateAvailableButton->setClickCallback( fastdelegate::MakeDelegate(this, &OsuMainMenu::onUpdatePressed) );
	m_updateAvailableButton->setColor(0x2200ff00);
	m_updateAvailableButton->setTextColor(0x22ffffff);

	m_steamWorkshopButton = new OsuUIButton(m_osu, 0, 0, 0, 0, "", "Steam Workshop");
	m_steamWorkshopButton->setUseDefaultSkin();
	m_steamWorkshopButton->setClickCallback( fastdelegate::MakeDelegate(this, &OsuMainMenu::onSteamWorkshopPressed) );
	m_steamWorkshopButton->setColor(0xff108fe8);
	m_steamWorkshopButton->setTextColor(0xffffffff);
	m_steamWorkshopButton->setVisible(osu_draw_main_menu_workshop_button.getBool());
	m_container->addBaseUIElement(m_steamWorkshopButton);

	m_githubButton = new OsuUIButton(m_osu, 0, 0, 0, 0, "", "Github");
	m_githubButton->setUseDefaultSkin();
	m_githubButton->setClickCallback( fastdelegate::MakeDelegate(this, &OsuMainMenu::onGithubPressed) );
	m_githubButton->setColor(0x2923b9ff);
	m_githubButton->setTextBrightColor(0x55172e62);
	m_githubButton->setTextDarkColor(0x11ffffff);
	m_githubButton->setAlphaAddOnHover(0.5f);
	m_githubButton->setVisible(false);
	m_container->addBaseUIElement(m_githubButton);

	m_versionButton = new CBaseUIButton(0, 0, 0, 0, "", "");
	UString versionString = MCOSU_VERSION_TEXT;
	versionString.append(" ");
	versionString.append(UString::format("%.2f", Osu::version->getFloat()));
	m_versionButton->setText(versionString);
	m_versionButton->setDrawBackground(false);
	m_versionButton->setDrawFrame(false);
	m_versionButton->setClickCallback( fastdelegate::MakeDelegate(this, &OsuMainMenu::onVersionPressed) );
	m_container->addBaseUIElement(m_versionButton);

	m_mainMenuSliderTextDatabaseBeatmap = NULL;
	m_mainMenuSliderTextBeatmapStandard = NULL;
	m_fMainMenuSliderTextRawHitCircleDiameter = 1.0f;
	if (osu_main_menu_use_slider_text.getBool())
	{
		m_mainMenuSliderTextDatabaseBeatmap = new OsuDatabaseBeatmap(m_osu, UString(s_sliderTextBeatmap), "", true);
		m_mainMenuSliderTextBeatmapStandard = new OsuBeatmapStandard(m_osu);

		// HACKHACK: temporary workaround to avoid this breaking the main menu logo text sliders (1/2)
		const bool wasModRandomEnabled = m_osu_mod_random_ref->getBool();
		if (wasModRandomEnabled)
			m_osu_mod_random_ref->setValue(0.0f);

		OsuDatabaseBeatmap::LOAD_GAMEPLAY_RESULT result = OsuDatabaseBeatmap::loadGameplay(m_mainMenuSliderTextDatabaseBeatmap, m_mainMenuSliderTextBeatmapStandard);
		if (result.errorCode == 0)
		{
			m_fMainMenuSliderTextRawHitCircleDiameter = m_mainMenuSliderTextBeatmapStandard->getRawHitcircleDiameter();
			m_mainMenuSliderTextBeatmapHitObjects = result.hitobjects;
			for (size_t i=0; i<m_mainMenuSliderTextBeatmapHitObjects.size(); i++)
			{
				OsuHitObject *hitObject = m_mainMenuSliderTextBeatmapHitObjects[i];
				OsuSlider *sliderPointer = dynamic_cast<OsuSlider*>(hitObject);
				if (sliderPointer != NULL)
					sliderPointer->rebuildVertexBuffer(true); // we are working in osu coordinate space for this (no mods, just raw curve coords)
			}
		}

		// HACKHACK: temporary workaround to avoid this breaking the main menu logo text sliders (2/2)
		if (wasModRandomEnabled)
			m_osu_mod_random_ref->setValue(1.0f);
	}
}

OsuMainMenu::~OsuMainMenu()
{
	anim->deleteExistingAnimation(&m_fUpdateButtonAnim);

	anim->deleteExistingAnimation(&m_fMainMenuAnimFriendEyeFollowX);
	anim->deleteExistingAnimation(&m_fMainMenuAnimFriendEyeFollowY);

	anim->deleteExistingAnimation(&m_fMainMenuAnim);
	anim->deleteExistingAnimation(&m_fMainMenuAnim1);
	anim->deleteExistingAnimation(&m_fMainMenuAnim2);
	anim->deleteExistingAnimation(&m_fMainMenuAnim3);

	anim->deleteExistingAnimation(&m_fCenterOffsetAnim);
	anim->deleteExistingAnimation(&m_fStartupAnim);
	anim->deleteExistingAnimation(&m_fStartupAnim2);

	SAFE_DELETE(m_container);
	SAFE_DELETE(m_updateAvailableButton);

	// if the user didn't click on the update notification during this session, quietly remove it so it's not annoying
	if (m_bWasCleanShutdown)
		writeVersionFile();

	SAFE_DELETE(m_mainMenuSliderTextBeatmapStandard);
	SAFE_DELETE(m_mainMenuSliderTextDatabaseBeatmap);
}

void OsuMainMenu::draw(Graphics *g)
{
	if (!m_bVisible) return;

	McFont *smallFont = m_osu->getSubTitleFont();
	McFont *titleFont = m_osu->getTitleFont();

	// menu-background
	if (osu_draw_menu_background.getBool())
	{
		Image *backgroundImage = m_osu->getSkin()->getMenuBackground();
		if (backgroundImage != NULL && backgroundImage != m_osu->getSkin()->getMissingTexture() && backgroundImage->isReady())
		{
			const float scale = Osu::getImageScaleToFillResolution(backgroundImage, m_osu->getScreenSize());

			g->setColor(0xffffffff);
			g->setAlpha(m_fStartupAnim);
			g->pushTransform();
			{
				g->scale(scale, scale);
				g->translate(m_osu->getScreenWidth()/2, m_osu->getScreenHeight()/2);
				g->drawImage(backgroundImage);
			}
			g->popTransform();
		}
	}

	if (osu_main_menu_shuffle.getBool())
	{
		if (m_osu->getSelectedBeatmap() != NULL)
		{
			float alpha = 1.0f;
			if (m_osu_songbrowser_background_fade_in_duration_ref->getFloat() > 0.0f)
			{
				// handle fadein trigger after handler is finished loading
				const bool ready = m_osu->getSelectedBeatmap() != NULL
					&& m_osu->getSelectedBeatmap()->getSelectedDifficulty2() != NULL
					&& m_osu->getBackgroundImageHandler()->getLoadBackgroundImage(m_osu->getSelectedBeatmap()->getSelectedDifficulty2()) != NULL
					&& m_osu->getBackgroundImageHandler()->getLoadBackgroundImage(m_osu->getSelectedBeatmap()->getSelectedDifficulty2())->isReady();

				if (!ready)
					m_fBackgroundFadeInTime = engine->getTime();
				else if (m_fBackgroundFadeInTime > 0.0f && engine->getTime() > m_fBackgroundFadeInTime)
				{
					alpha = clamp<float>((engine->getTime() - m_fBackgroundFadeInTime)/m_osu_songbrowser_background_fade_in_duration_ref->getFloat(), 0.0f, 1.0f);
					alpha = 1.0f - (1.0f - alpha)*(1.0f - alpha);
				}
			}
			OsuSongBrowser2::drawSelectedBeatmapBackgroundImage(g, m_osu, alpha);
		}
	}

	// main button stuff
	bool haveTimingpoints = false;
	const float div = 1.25f;
	float pulse = 0.0f;
	if (m_osu->getSelectedBeatmap() != NULL && m_osu->getSelectedBeatmap()->getSelectedDifficulty2() != NULL && m_osu->getSelectedBeatmap()->getMusic() != NULL && m_osu->getSelectedBeatmap()->getMusic()->isPlaying())
	{
		haveTimingpoints = true;

		const long curMusicPos = (long)m_osu->getSelectedBeatmap()->getMusic()->getPositionMS()
			+ (long)(m_osu_universal_offset_ref->getFloat() * m_osu->getSpeedMultiplier())
			+ (long)m_osu_universal_offset_hardcoded_ref->getInt()
			+ (m_win_snd_fallback_dsound_ref->getBool() ? (long)m_osu_universal_offset_hardcoded_fallback_dsound_ref->getInt() : 0)
			- m_osu->getSelectedBeatmap()->getSelectedDifficulty2()->getLocalOffset()
			- m_osu->getSelectedBeatmap()->getSelectedDifficulty2()->getOnlineOffset()
			- (m_osu->getSelectedBeatmap()->getSelectedDifficulty2()->getVersion() < 5 ? m_osu_old_beatmap_offset_ref->getInt() : 0);

		OsuDatabaseBeatmap::TIMING_INFO t = m_osu->getSelectedBeatmap()->getSelectedDifficulty2()->getTimingInfoForTime(curMusicPos);

		if (t.beatLengthBase == 0.0f) // bah
			t.beatLengthBase = 1.0f;

		m_iMainMenuAnimBeatCounter = (curMusicPos - t.offset - (long)(std::max((long)t.beatLengthBase, (long)1)*0.5f)) / std::max((long)t.beatLengthBase, (long)1);
		pulse = (float)((curMusicPos - t.offset) % std::max((long)t.beatLengthBase, (long)1)) / t.beatLengthBase; // modulo must be >= 1
		pulse = clamp<float>(pulse, -1.0f, 1.0f);
		if (pulse < 0.0f)
			pulse = 1.0f - std::abs(pulse);
	}
	else
		pulse = (div - fmod(engine->getTime(), div))/div;

	//pulse *= pulse; // quadratic
	Vector2 size = m_vSize;
	const float pulseSub = 0.05f*pulse;
	size -= size*pulseSub;
	size += size*m_fSizeAddAnim;
	size *= m_fStartupAnim;
	McRect mainButtonRect = McRect(m_vCenter.x - size.x/2.0f - m_fCenterOffsetAnim, m_vCenter.y - size.y/2.0f, size.x, size.y);

	bool drawBanner = true;

#ifdef __SWITCH__

	drawBanner = ((HorizonSDLEnvironment*)env)->getMemAvailableMB() < 1024;

#endif

	if (drawBanner)
	{
		UString bannerText = MCOSU_BANNER_TEXT;

#ifdef MCENGINE_FEATURE_DIRECTX11

		if (dynamic_cast<DirectX11Interface*>(engine->getGraphics()) != NULL)
			bannerText = "-- DirectX11 Test - Unoptimized renderer (no batching etc.) - VR is not supported - Please report feedback on Discord/Steam Forums --";

#endif

#ifdef MCENGINE_FEATURE_BASS_WASAPI

		bannerText = UString::format(convar->getConVarByName("win_snd_wasapi_exclusive")->getBool() ?
				"-- WASAPI Exclusive Mode! win_snd_wasapi_buffer_size = %i ms --" :
				"-- WASAPI Shared Mode! win_snd_wasapi_buffer_size = %i ms --",
				(int)(std::round(convar->getConVarByName("win_snd_wasapi_buffer_size")->getFloat()*1000.0f)));

#endif

		if (osu_main_menu_banner_always_text.getString().length() > 0)
			bannerText = osu_main_menu_banner_always_text.getString();
		else if (m_bDidUserUpdateFromOlderVersion && osu_main_menu_banner_ifupdatedfromoldversion_text.getString().length() > 0)
			bannerText = osu_main_menu_banner_ifupdatedfromoldversion_text.getString();
		else if (m_bDidUserUpdateFromOlderVersionLe3300 && osu_main_menu_banner_ifupdatedfromoldversion_le3300_text.getString().length() > 0)
			bannerText = osu_main_menu_banner_ifupdatedfromoldversion_le3300_text.getString();
		else if (m_bDidUserUpdateFromOlderVersionLe3303 && osu_main_menu_banner_ifupdatedfromoldversion_le3303_text.getString().length() > 0)
			bannerText = osu_main_menu_banner_ifupdatedfromoldversion_le3303_text.getString();
		else if (m_bDidUserUpdateFromOlderVersionLe3308 && osu_main_menu_banner_ifupdatedfromoldversion_le3308_text.getString().length() > 0)
			bannerText = osu_main_menu_banner_ifupdatedfromoldversion_le3308_text.getString();
		else if (m_bDidUserUpdateFromOlderVersionLe3310 && osu_main_menu_banner_ifupdatedfromoldversion_le3310_text.getString().length() > 0)
			bannerText = osu_main_menu_banner_ifupdatedfromoldversion_le3310_text.getString();
		else if (m_bDidUserUpdateFromOlderVersionLe3311 && osu_main_menu_banner_ifupdatedfromoldversion_le3311_text.getString().length() > 0)
			bannerText = osu_main_menu_banner_ifupdatedfromoldversion_le3311_text.getString();
		else if (m_bDidUserUpdateFromOlderVersionLe3312 && osu_main_menu_banner_ifupdatedfromoldversion_le3312_text.getString().length() > 0)
			bannerText = osu_main_menu_banner_ifupdatedfromoldversion_le3312_text.getString();

		if (bannerText.length() > 0)
		{
			McFont *bannerFont = m_osu->getSubTitleFont();
			float bannerStringWidth = bannerFont->getStringWidth(bannerText);
			int bannerDiff = 20;
			int bannerMargin = 5;
			int numBanners = (int)std::round(m_osu->getScreenWidth() / (bannerStringWidth + bannerDiff)) + 2;

			g->setColor(0xffee7777);
			g->pushTransform();
			g->translate(1, 1);
			for (int i=-1; i<numBanners; i++)
			{
				g->pushTransform();
				g->translate(i*bannerStringWidth + i*bannerDiff + fmod(-engine->getTime()*30, bannerStringWidth + bannerDiff), bannerFont->getHeight() + bannerMargin);
				g->drawString(bannerFont, bannerText);
				g->popTransform();
			}
			g->popTransform();
			g->setColor(0xff555555);
			for (int i=-1; i<numBanners; i++)
			{
				g->pushTransform();
				g->translate(i*bannerStringWidth + i*bannerDiff + fmod(-engine->getTime()*30, bannerStringWidth + bannerDiff), bannerFont->getHeight() + bannerMargin);
				g->drawString(bannerFont, bannerText);
				g->popTransform();
			}
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

		const Vector2 arrowPos = Vector2(m_versionButton->getSize().x/1.75f, m_osu->getScreenHeight() - m_versionButton->getSize().y*2 - m_versionButton->getSize().y*scale);

		UString notificationText = "Changelog";
		g->setColor(0xffffffff);
		g->pushTransform();
		{
			g->translate(arrowPos.x - smallFont->getStringWidth(notificationText)/2.0f, (-offset*2)*scale + arrowPos.y - (m_osu->getSkin()->getPlayWarningArrow2()->getSizeBaseRaw().y*scale)/1.5f, 0);
			g->drawString(smallFont, notificationText);
		}
		g->popTransform();

		g->setColor(0xffffffff);
		g->pushTransform();
		{
			g->rotate(90.0f);
			g->translate(0, -offset*2, 0);
			m_osu->getSkin()->getPlayWarningArrow2()->drawRaw(g, arrowPos, scale);
		}
		g->popTransform();
	}

	// draw container
	m_container->draw(g);

	// draw update check button
	{
		if (m_osu->getUpdateHandler()->getStatus() == OsuUpdateHandler::STATUS::STATUS_SUCCESS_INSTALLATION)
		{
			g->push3DScene(McRect(m_updateAvailableButton->getPos().x, m_updateAvailableButton->getPos().y, m_updateAvailableButton->getSize().x, m_updateAvailableButton->getSize().y));
			g->rotate3DScene(m_fUpdateButtonAnim*360.0f, 0, 0);
		}
		m_updateAvailableButton->draw(g);
		if (m_osu->getUpdateHandler()->getStatus() == OsuUpdateHandler::STATUS::STATUS_SUCCESS_INSTALLATION)
			g->pop3DScene();
	}

	// pre-render all slider text into the one single sliderFrameBuffer (up here before any of the 3dscene stuff)
	if (osu_main_menu_use_slider_text.getBool() && m_mainMenuSliderTextBeatmapHitObjects.size() > 0)
	{
		static std::vector<Vector2> alwaysPoints;
		const float scale = (mainButtonRect.getWidth() / 1100.0f) * osu_main_menu_slider_text_scale.getFloat() * m_fStartupAnim;
		const Vector2 osuCoordsToCenteredAtOrigin = Vector2(-OsuGameRules::OSU_COORD_WIDTH/2 + osu_main_menu_slider_text_offset_x.getFloat(), -OsuGameRules::OSU_COORD_HEIGHT/2 + osu_main_menu_slider_text_offset_y.getFloat()) * scale;
		const Vector2 screenCenterOffset = Vector2(m_osu->getScreenWidth()/2 - m_fCenterOffsetAnim, m_osu->getScreenHeight()/2);
		const Vector2 translation = osuCoordsToCenteredAtOrigin + screenCenterOffset;
		const float from = 0.0f;
		const float to = m_fStartupAnim2;

		const float prevSliderBorderFeatherBackup = m_osu_slider_border_feather_ref->getFloat();
		m_osu_slider_border_feather_ref->setValue(osu_main_menu_slider_text_feather.getFloat()); // heuristic to avoid aliasing
		{
			const size_t numHitObjects = m_mainMenuSliderTextBeatmapHitObjects.size();
			for (size_t i=0; i<numHitObjects; i++)
			{
				OsuSlider *sliderPointer = dynamic_cast<OsuSlider*>(m_mainMenuSliderTextBeatmapHitObjects[i]);
				if (sliderPointer != NULL)
				{
					alwaysPoints.clear();
					if (to < 1.0f)
						alwaysPoints.push_back((sliderPointer->getCurve()->pointAt(to))*scale + translation); // alwaysPoints are always drawn in engine coords, so compensate by applying scale and translation manually

					const bool doEnableRenderTarget = (i == 0);
					const bool doDisableRenderTarget = (i+1 >= numHitObjects);
					const bool doDrawSliderFrameBufferToScreen = false;

					Vector4 emptyBounds;
					OsuSliderRenderer::draw(g, m_osu, sliderPointer->getVAO(), emptyBounds, alwaysPoints, translation, scale, (to < 1.0f ? m_fMainMenuSliderTextRawHitCircleDiameter*scale : OsuSliderRenderer::UNIT_CIRCLE_VAO_DIAMETER), from, to, m_osu->getSkin()->getComboColorForCounter(sliderPointer->getColorCounter(), sliderPointer->getColorOffset()), 1.0f, 1.0f, 0, doEnableRenderTarget, doDisableRenderTarget, doDrawSliderFrameBufferToScreen);
				}
			}
		}
		m_osu_slider_border_feather_ref->setValue(prevSliderBorderFeatherBackup);
	}

	// draw main button
	float inset = 0.0f;
	if ((m_fMainMenuAnim > 0.0f && m_fMainMenuAnim != 1.0f) || (haveTimingpoints && m_fMainMenuAnimFriendPercent > 0.0f))
	{
		inset = 1.0f - 0.5*m_fMainMenuAnimFriendPercent;

		g->setDepthBuffer(true);
		g->clearDepthBuffer();

		g->push3DScene(mainButtonRect);
		g->offset3DScene(0, 0, mainButtonRect.getWidth()/2);

		float friendRotation = 0.0f;
		float friendTranslationX = 0.0f;
		float friendTranslationY = 0.0f;
		if (haveTimingpoints && m_fMainMenuAnimFriendPercent > 0.0f)
		{
			float customPulse = 0.0f;
			if (pulse > 0.5f)
				customPulse = (pulse - 0.5f) / 0.5f;
			else
				customPulse = (0.5f - pulse) / 0.5f;

			customPulse = 1.0f - customPulse;

			const float anim = lerp<float>((1.0f - customPulse)*(1.0f - customPulse), (1.0f - customPulse), 0.25f);
			const float anim2 = anim * (m_iMainMenuAnimBeatCounter % 2 == 1 ? 1.0f : -1.0f);
			const float anim3 = anim;

			friendRotation = anim2*13;
			friendTranslationX = -anim2*mainButtonRect.getWidth()*0.175;
			friendTranslationY = anim3*mainButtonRect.getWidth()*0.10;

			friendRotation *= m_fMainMenuAnimFriendPercent;
			friendTranslationX *= m_fMainMenuAnimFriendPercent;
			friendTranslationY *= m_fMainMenuAnimFriendPercent;
		}

		g->translate3DScene(friendTranslationX, friendTranslationY, 0);
		g->rotate3DScene(m_fMainMenuAnim1*360.0f, m_fMainMenuAnim2*360.0f, m_fMainMenuAnim3*360.0f + friendRotation);

		//g->rotate3DScene(engine->getMouse()->getPos().y, engine->getMouse()->getPos().x, 0);

		/*
		g->offset3DScene(0, 0, 300.0f);
		g->rotate3DScene(0, m_fCenterOffsetAnim*0.13f, 0);
		*/
	}

	const Color cubeColor = COLORf(1.0f, lerp<float>(0.0f, 0.5f, m_fMainMenuAnimFriendPercent), lerp<float>(0.0f, 0.768f, m_fMainMenuAnimFriendPercent), lerp<float>(0.0f, 0.965f, m_fMainMenuAnimFriendPercent));
	const Color cubeBorderColor = COLORf(1.0f, lerp<float>(1.0f, 0.5f, m_fMainMenuAnimFriendPercent), lerp<float>(1.0f, 0.768f, m_fMainMenuAnimFriendPercent), lerp<float>(1.0f, 0.965f, m_fMainMenuAnimFriendPercent));

	if (osu_draw_main_menu_button.getBool())
	{
		// front side
		g->setColor(cubeColor);
		g->setAlpha(osu_main_menu_alpha.getFloat());
		g->pushTransform();
		{
			g->translate(0, 0, inset);
			g->fillRect(mainButtonRect.getX() + inset, mainButtonRect.getY() + inset, mainButtonRect.getWidth() - 2*inset, mainButtonRect.getHeight() - 2*inset);
		}
		g->popTransform();
		g->setColor(cubeBorderColor);
		g->setAlpha(osu_main_menu_alpha.getFloat());
		g->drawRect(mainButtonRect.getX(), mainButtonRect.getY(), mainButtonRect.getWidth(), mainButtonRect.getHeight());
		{
			// front side pulse border
			/*
			if (haveTimingpoints && !anim->isAnimating(&m_fMainMenuAnim))
			{
				const int pulseSizeMax = mainButtonRect.getWidth()*0.25f;
				const int pulseOffset = (1.0f - (1.0f - pulse)*(1.0f - pulse))*pulseSizeMax;
				g->setColor(0xffffffff);
				g->setAlpha((1.0f - pulse)*0.4f*m_fMainMenuAnimFriendPercent);
				g->drawRect(mainButtonRect.getX() - pulseOffset/2, mainButtonRect.getY() - pulseOffset/2, mainButtonRect.getWidth() + pulseOffset, mainButtonRect.getHeight() + pulseOffset);
			}
			*/
		}
	}

	// friend
	if (m_fMainMenuAnimFriendPercent > 0.0f)
	{
		// ears
		{
			const float width = mainButtonRect.getWidth() * 0.11f * 2.0f * (1.0f - pulse*0.05f);

			const float margin = width * 0.4f;

			const float offset = mainButtonRect.getWidth() * 0.02f;

			VertexArrayObject vao;
			{
				const Vector2 pos = Vector2(mainButtonRect.getX(), mainButtonRect.getY() - offset);

				Vector2 left = pos + Vector2(0, 0);
				Vector2 top = pos + Vector2(width/2, -width*std::sqrt(3.0f)/2.0f);
				Vector2 right = pos + Vector2(width, 0);

				Vector2 topRightDir = (top - right);
				{
					const float temp = topRightDir.x;
					topRightDir.x = -topRightDir.y;
					topRightDir.y = temp;
				}

				Vector2 innerLeft = left + topRightDir.normalize() * margin;

				vao.addVertex(left.x, left.y);
				vao.addVertex(top.x, top.y);
				vao.addVertex(innerLeft.x, innerLeft.y);

				Vector2 leftRightDir = (right - left);
				{
					const float temp = leftRightDir.x;
					leftRightDir.x = -leftRightDir.y;
					leftRightDir.y = temp;
				}

				Vector2 innerTop = top + leftRightDir.normalize() * margin;

				vao.addVertex(top.x, top.y);
				vao.addVertex(innerTop.x, innerTop.y);
				vao.addVertex(innerLeft.x, innerLeft.y);

				Vector2 leftTopDir = (left - top);
				{
					const float temp = leftTopDir.x;
					leftTopDir.x = -leftTopDir.y;
					leftTopDir.y = temp;
				}

				Vector2 innerRight = right + leftTopDir.normalize() * margin;

				vao.addVertex(top.x, top.y);
				vao.addVertex(innerRight.x, innerRight.y);
				vao.addVertex(innerTop.x, innerTop.y);

				vao.addVertex(top.x, top.y);
				vao.addVertex(right.x, right.y);
				vao.addVertex(innerRight.x, innerRight.y);

				vao.addVertex(left.x, left.y);
				vao.addVertex(innerLeft.x, innerLeft.y);
				vao.addVertex(innerRight.x, innerRight.y);

				vao.addVertex(left.x, left.y);
				vao.addVertex(innerRight.x, innerRight.y);
				vao.addVertex(right.x, right.y);
			}

			// left
			g->setColor(0xffc8faf1);
			g->setAlpha(m_fMainMenuAnimFriendPercent*osu_main_menu_alpha.getFloat());
			g->drawVAO(&vao);

			// right
			g->pushTransform();
			{
				g->translate(mainButtonRect.getWidth() - width, 0);
				g->drawVAO(&vao);
			}
			g->popTransform();
		}

		float headBob = 0.0f;
		{
			float customPulse = 0.0f;
			if (pulse > 0.5f)
				customPulse = (pulse - 0.5f) / 0.5f;
			else
				customPulse = (0.5f - pulse) / 0.5f;

			customPulse = 1.0f - customPulse;

			if (!haveTimingpoints)
				customPulse = 1.0f;

			headBob = (customPulse) * (customPulse);
			headBob *= m_fMainMenuAnimFriendPercent;
		}

		const float mouthEyeOffsetY = mainButtonRect.getWidth() * 0.18f + headBob*mainButtonRect.getWidth()*0.075f;

		// mouth
		{
			const float width = mainButtonRect.getWidth() * 0.10f;
			const float height = mainButtonRect.getHeight() * 0.03f * 1.75;

			const float length = width * std::sqrt(2.0f) * 2;

			const float offsetY = mainButtonRect.getHeight()/2.0f + mouthEyeOffsetY;

			g->pushTransform();
			{
				g->rotate(135);
				g->translate(mainButtonRect.getX() + length/2 + mainButtonRect.getWidth()/2 - m_fMainMenuAnimFriendEyeFollowX*mainButtonRect.getWidth()*0.5f, mainButtonRect.getY() + offsetY - m_fMainMenuAnimFriendEyeFollowY*mainButtonRect.getWidth()*0.5f);

				g->setColor(0xff000000);
				g->setAlpha(osu_main_menu_alpha.getFloat());
				g->fillRect(0, 0, width, height);
				g->fillRect(width - height/2.0f, 0, height, width);
				g->fillRect(width - height/2.0f, width - height/2.0f, width, height);
				g->fillRect(width*2 - height, width - height/2.0f, height, width + height/2);
			}
			g->popTransform();
		}

		// eyes
		{
			const float width = mainButtonRect.getWidth() * 0.22f;
			const float height = mainButtonRect.getHeight() * 0.03f * 2;

			const float offsetX = mainButtonRect.getWidth() * 0.18f;
			const float offsetY = mainButtonRect.getHeight() * 0.21f + mouthEyeOffsetY;

			const float rotation = 25.0f;

			// left
			g->pushTransform();
			{
				g->translate(-width, 0);
				g->rotate(-rotation);
				g->translate(width, 0);
				g->translate(mainButtonRect.getX() + offsetX - m_fMainMenuAnimFriendEyeFollowX*mainButtonRect.getWidth(), mainButtonRect.getY() + offsetY - m_fMainMenuAnimFriendEyeFollowY*mainButtonRect.getWidth());

				g->setColor(0xff000000);
				g->setAlpha(osu_main_menu_alpha.getFloat());
				g->fillRect(0, 0, width, height);
			}
			g->popTransform();

			// right
			g->pushTransform();
			{
				g->rotate(rotation);
				g->translate(mainButtonRect.getX() + mainButtonRect.getWidth() - offsetX - width - m_fMainMenuAnimFriendEyeFollowX*mainButtonRect.getWidth(), mainButtonRect.getY() + offsetY - m_fMainMenuAnimFriendEyeFollowY*mainButtonRect.getWidth());

				g->setColor(0xff000000);
				g->setAlpha(osu_main_menu_alpha.getFloat());
				g->fillRect(0, 0, width, height);
			}
			g->popTransform();

			// tear
			g->setColor(0xff000000);
			g->setAlpha(osu_main_menu_alpha.getFloat());
			g->fillRect(mainButtonRect.getX() + offsetX + width*0.375f - m_fMainMenuAnimFriendEyeFollowX*mainButtonRect.getWidth(), mainButtonRect.getY() + offsetY + width/2.0f - m_fMainMenuAnimFriendEyeFollowY*mainButtonRect.getWidth(), height*0.75f, width*0.375f);
		}

		// hands
		{
			const float size = mainButtonRect.getWidth()*0.2f;

			const float offset = -size*0.75f;

			float customPulse = 0.0f;
			if (pulse > 0.5f)
				customPulse = (pulse - 0.5f) / 0.5f;
			else
				customPulse = (0.5f - pulse) / 0.5f;

			customPulse = 1.0f - customPulse;

			if (!haveTimingpoints)
				customPulse = 1.0f;

			const float animLeftMultiplier = (m_iMainMenuAnimBeatCounter % 2 == 0 ? 1.0f : 0.1f);
			const float animRightMultiplier = (m_iMainMenuAnimBeatCounter % 2 == 1 ? 1.0f : 0.1f);

			const float animMoveUp = lerp<float>((1.0f - customPulse)*(1.0f - customPulse), (1.0f - customPulse), 0.35f) * m_fMainMenuAnimFriendPercent;

			const float animLeftMoveUp = animMoveUp * animLeftMultiplier;
			const float animRightMoveUp = animMoveUp * animRightMultiplier;

			const float animLeftMoveLeft = animRightMoveUp * (m_iMainMenuAnimBeatCounter % 2 == 1 ? 1.0f : 0.0f);
			const float animRightMoveRight = animLeftMoveUp * (m_iMainMenuAnimBeatCounter % 2 == 0 ? 1.0f : 0.0f);

			// left
			g->setColor(0xffd5f6fd);
			g->setAlpha(m_fMainMenuAnimFriendPercent*osu_main_menu_alpha.getFloat());
			g->pushTransform();
			{
				g->rotate(40 - (1.0f - customPulse)*10 + animLeftMoveLeft*animLeftMoveLeft*20);
				g->translate(mainButtonRect.getX() - size - offset - animLeftMoveLeft*mainButtonRect.getWidth()*-0.025f - animLeftMoveUp*mainButtonRect.getWidth()*0.25f, mainButtonRect.getY() + mainButtonRect.getHeight() - size - animLeftMoveUp*mainButtonRect.getHeight()*0.85f, -0.5f);
				g->fillRect(0, 0, size, size);
			}
			g->popTransform();

			// right
			g->pushTransform();
			{
				g->rotate(50 + (1.0f - customPulse)*10 - animRightMoveRight*animRightMoveRight*20);
				g->translate(mainButtonRect.getX() + mainButtonRect.getWidth() + size + offset + animRightMoveRight*mainButtonRect.getWidth()*-0.025f + animRightMoveUp*mainButtonRect.getWidth()*0.25f, mainButtonRect.getY() + mainButtonRect.getHeight() - size - animRightMoveUp*mainButtonRect.getHeight()*0.85f, -0.5f);
				g->fillRect(0, 0, size, size);
			}
			g->popTransform();
		}
	}

	// main text
	const float fontScale = (1.0f - pulseSub + m_fSizeAddAnim)*m_fStartupAnim;
	{
		float alpha = (1.0f - m_fMainMenuAnimFriendPercent)*(1.0f - m_fMainMenuAnimFriendPercent)*(1.0f - m_fMainMenuAnimFriendPercent);

		if (!osu_main_menu_use_slider_text.getBool() || m_mainMenuSliderTextBeatmapHitObjects.size() < 1)
		{
			g->setColor(0xffffffff);
			g->setAlpha(alpha);
			g->pushTransform();
			{
				g->scale(fontScale, fontScale);
				g->translate(m_vCenter.x - m_fCenterOffsetAnim - (titleFont->getStringWidth(MCOSU_MAIN_BUTTON_TEXT)/2.0f)*fontScale, m_vCenter.y + (titleFont->getHeight()*fontScale)/2.25f, -1.0f);
				g->drawString(titleFont, MCOSU_MAIN_BUTTON_TEXT);
			}
			g->popTransform();
		}
		else
		{
			alpha *= m_fStartupAnim*m_fStartupAnim*m_fStartupAnim*m_fStartupAnim;

			const bool doScissor = osu_main_menu_slider_text_scissor.getBool();

			m_osu->getSliderFrameBuffer()->setColor(COLORf(alpha*osu_main_menu_slider_text_alpha.getFloat(), 1.0f, 1.0f, 1.0f));
			m_osu->getSliderFrameBuffer()->drawRect(g, (doScissor ? mainButtonRect.getX() : 0) + inset, (doScissor ? mainButtonRect.getY() : 0) + inset, (doScissor ? mainButtonRect.getWidth() : m_osu->getScreenWidth()) - 2*inset, (doScissor ? mainButtonRect.getHeight() : m_osu->getScreenHeight()) - 2*inset);
		}
	}

	// subtext
	if (osu_draw_main_menu_button_subtext.getBool() && MCOSU_MAIN_BUTTON_SUBTEXT.length() > 0)
	{
		float invertedPulse = 1.0f - pulse;

		if (haveTimingpoints)
			g->setColor(COLORf(1.0f, 0.10f + 0.15f*invertedPulse, 0.10f + 0.15f*invertedPulse, 0.10f + 0.15f*invertedPulse));
		else
			g->setColor(0xff444444);

		g->setAlpha((1.0f - m_fMainMenuAnimFriendPercent)*(1.0f - m_fMainMenuAnimFriendPercent)*(1.0f - m_fMainMenuAnimFriendPercent)*(1.0f - (1.0f - m_fStartupAnim2)*(1.0f - m_fStartupAnim2)) * osu_main_menu_alpha.getFloat());

		g->pushTransform();
		{
			g->scale(fontScale, fontScale);
			g->translate(m_vCenter.x - m_fCenterOffsetAnim - (smallFont->getStringWidth(MCOSU_MAIN_BUTTON_SUBTEXT)/2.0f)*fontScale, m_vCenter.y + (mainButtonRect.getHeight()/2.0f)/2.0f + (smallFont->getHeight()*fontScale)/2.0f, -1.0f);
			g->drawString(smallFont, MCOSU_MAIN_BUTTON_SUBTEXT);
		}
		g->popTransform();
	}

	if ((m_fMainMenuAnim > 0.0f && m_fMainMenuAnim != 1.0f) || (haveTimingpoints && m_fMainMenuAnimFriendPercent > 0.0f))
	{
		// back side
		g->rotate3DScene(0, -180, 0);
		g->setColor(cubeColor);
		g->setAlpha(osu_main_menu_alpha.getFloat());
		g->pushTransform();
		{
			g->translate(0, 0, inset);
			g->fillRect(mainButtonRect.getX() + inset, mainButtonRect.getY() + inset, mainButtonRect.getWidth() - 2*inset, mainButtonRect.getHeight() - 2*inset);
		}
		g->popTransform();
		g->setColor(cubeBorderColor);
		g->setAlpha(osu_main_menu_alpha.getFloat());
		g->drawRect(mainButtonRect.getX(), mainButtonRect.getY(), mainButtonRect.getWidth(), mainButtonRect.getHeight());

		// right side
		g->offset3DScene(0, 0, mainButtonRect.getWidth()/2);
		g->rotate3DScene(0, 90, 0);
		{
			//g->setColor(0xff00ff00);
			g->setColor(cubeColor);
			g->setAlpha(osu_main_menu_alpha.getFloat());
			g->pushTransform();
			{
				g->translate(0, 0, inset);
				g->fillRect(mainButtonRect.getX() + inset, mainButtonRect.getY() + inset, mainButtonRect.getWidth() - 2*inset, mainButtonRect.getHeight() - 2*inset);
			}
			g->popTransform();

			g->setColor(cubeBorderColor);
			g->setAlpha(osu_main_menu_alpha.getFloat());
			g->drawRect(mainButtonRect.getX(), mainButtonRect.getY(), mainButtonRect.getWidth(), mainButtonRect.getHeight());
		}
		g->rotate3DScene(0, -90, 0);
		g->offset3DScene(0, 0, 0);

		// left side
		g->offset3DScene(0, 0, mainButtonRect.getWidth()/2);
		g->rotate3DScene(0, -90, 0);
		{
			//g->setColor(0xffffff00);
			g->setColor(cubeColor);
			g->setAlpha(osu_main_menu_alpha.getFloat());
			g->pushTransform();
			{
				g->translate(0, 0, inset);
				g->fillRect(mainButtonRect.getX() + inset, mainButtonRect.getY() + inset, mainButtonRect.getWidth() - 2*inset, mainButtonRect.getHeight() - 2*inset);
			}
			g->popTransform();

			g->setColor(cubeBorderColor);
			g->setAlpha(osu_main_menu_alpha.getFloat());
			g->drawRect(mainButtonRect.getX(), mainButtonRect.getY(), mainButtonRect.getWidth(), mainButtonRect.getHeight());
		}
		g->rotate3DScene(0, 90, 0);
		g->offset3DScene(0, 0, 0);

		// top side
		g->offset3DScene(0, 0, mainButtonRect.getHeight()/2);
		g->rotate3DScene(90, 0, 0);
		{
			//g->setColor(0xff00ffff);
			g->setColor(cubeColor);
			g->setAlpha(osu_main_menu_alpha.getFloat());
			g->pushTransform();
			{
				g->translate(0, 0, inset);
				g->fillRect(mainButtonRect.getX() + inset, mainButtonRect.getY() + inset, mainButtonRect.getWidth() - 2*inset, mainButtonRect.getHeight() - 2*inset);
			}
			g->popTransform();

			g->setColor(cubeBorderColor);
			g->setAlpha(osu_main_menu_alpha.getFloat());
			g->drawRect(mainButtonRect.getX(), mainButtonRect.getY(), mainButtonRect.getWidth(), mainButtonRect.getHeight());
		}
		g->rotate3DScene(-90, 0, 0);
		g->offset3DScene(0, 0, 0);

		// bottom side
		g->offset3DScene(0, 0, mainButtonRect.getHeight()/2);
		g->rotate3DScene(-90, 0, 0);
		{
			//g->setColor(0xffff0000);
			g->setColor(cubeColor);
			g->setAlpha(osu_main_menu_alpha.getFloat());
			g->pushTransform();
			{
				g->translate(0, 0, inset);
				g->fillRect(mainButtonRect.getX() + inset, mainButtonRect.getY() + inset, mainButtonRect.getWidth() - 2*inset, mainButtonRect.getHeight() - 2*inset);
			}
			g->popTransform();

			g->setColor(cubeBorderColor);
			g->setAlpha(osu_main_menu_alpha.getFloat());
			g->drawRect(mainButtonRect.getX(), mainButtonRect.getY(), mainButtonRect.getWidth(), mainButtonRect.getHeight());
		}
		g->rotate3DScene(90, 0, 0);
		g->offset3DScene(0, 0, 0);

		// back text
		g->setColor(0xffffffff);
		g->setAlpha(osu_main_menu_alpha.getFloat());
		g->setAlpha((1.0f - m_fMainMenuAnimFriendPercent)*(1.0f - m_fMainMenuAnimFriendPercent)*(1.0f - m_fMainMenuAnimFriendPercent));
		g->pushTransform();
		{
			g->scale(fontScale, fontScale);
			g->translate(m_vCenter.x - m_fCenterOffsetAnim - (titleFont->getStringWidth(MCOSU_MAIN_BUTTON_BACK_TEXT)/2.0f)*fontScale, m_vCenter.y + ((titleFont->getHeight()*fontScale)/2.25f)*4.0f, -1.0f);
			g->drawString(titleFont, MCOSU_MAIN_BUTTON_BACK_TEXT);
		}
		g->popTransform();

		g->pop3DScene();

		g->setDepthBuffer(false);
	}

	/*
	if (m_fShutdownScheduledTime != 0.0f)
	{
		g->setColor(0xff000000);
		g->setAlpha(1.0f - clamp<float>((m_fShutdownScheduledTime - engine->getTime()) / 0.3f, 0.0f, 1.0f));
		g->fillRect(0, 0, m_osu->getScreenWidth(), m_osu->getScreenHeight());
	}
	*/
}

void OsuMainMenu::update()
{
	if (!m_bVisible) return;

	if (m_steamWorkshopButton->isVisible() != osu_draw_main_menu_workshop_button.getBool())
		m_steamWorkshopButton->setVisible(osu_draw_main_menu_workshop_button.getBool());

	updateLayout();

	if (m_osu->getHUD()->isVolumeOverlayBusy())
		m_container->stealFocus();

	if (m_osu->getOptionsMenu()->isMouseInside())
		m_container->stealFocus();

	// update and focus handling
	// the main button always gets top focus
	m_mainButton->update();
	if (m_mainButton->isMouseInside())
		m_container->stealFocus();

	m_container->update();
	m_updateAvailableButton->update();

	if (m_osu->getOptionsMenu()->isMouseInside())
		m_container->stealFocus();

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
	if (m_fShutdownScheduledTime != 0.0f && (engine->getTime() > m_fShutdownScheduledTime || !anim->isAnimating(&m_fCenterOffsetAnim)))
	{
		engine->shutdown();
		m_fShutdownScheduledTime = 0.0f;
	}

	// main button autohide + anim
	if (m_bMenuElementsVisible)
	{
		m_fMainMenuAnimDuration = 15.0f;
		m_fMainMenuAnimTime = engine->getTime() + m_fMainMenuAnimDuration;
	}
	if (engine->getTime() > m_fMainMenuAnimTime)
	{
		if (m_bMainMenuAnimFriendScheduled)
			m_bMainMenuAnimFriend = true;
		if (m_bMainMenuAnimFadeToFriendForNextAnim)
			m_bMainMenuAnimFriendScheduled = true;

		m_fMainMenuAnimDuration = 10.0f + (static_cast<float>(rand())/static_cast<float>(RAND_MAX))*5.0f;
		m_fMainMenuAnimTime = engine->getTime() + m_fMainMenuAnimDuration;
		animMainButton();
	}

	if (m_bInMainMenuRandomAnim && m_iMainMenuRandomAnimType == 1 && anim->isAnimating(&m_fMainMenuAnim))
	{
		Vector2 mouseDelta = (m_mainButton->getPos() + m_mainButton->getSize()/2) - engine->getMouse()->getPos();
		mouseDelta.x = clamp<float>(mouseDelta.x, -engine->getScreenSize().x/2, engine->getScreenSize().x/2);
		mouseDelta.y = clamp<float>(mouseDelta.y, -engine->getScreenSize().y/2, engine->getScreenSize().y/2);
		mouseDelta.x /= engine->getScreenSize().x;
		mouseDelta.y /= engine->getScreenSize().y;

		const float decay = clamp<float>((1.0f - m_fMainMenuAnim - 0.075f) / 0.025f, 0.0f, 1.0f);

		const Vector2 pushAngle = Vector2(mouseDelta.y, -mouseDelta.x) * Vector2(0.15f, 0.15f) * decay;

		anim->moveQuadOut(&m_fMainMenuAnim1, pushAngle.x, 0.15f, true);

		anim->moveQuadOut(&m_fMainMenuAnim2, pushAngle.y, 0.15f, true);

		anim->moveQuadOut(&m_fMainMenuAnim3, 0.0f, 0.15f, true);
	}

	{
		m_fMainMenuAnimFriendPercent = 1.0f - clamp<float>((m_fMainMenuAnimDuration > 0.0f ? (m_fMainMenuAnimTime - engine->getTime()) / m_fMainMenuAnimDuration : 0.0f), 0.0f, 1.0f);
		m_fMainMenuAnimFriendPercent = clamp<float>((m_fMainMenuAnimFriendPercent - 0.5f)/0.5f, 0.0f, 1.0f);
		if (m_bMainMenuAnimFriend)
			m_fMainMenuAnimFriendPercent = 1.0f;
		if (!m_bMainMenuAnimFriendScheduled)
			m_fMainMenuAnimFriendPercent = 0.0f;

		Vector2 mouseDelta = (m_mainButton->getPos() + m_mainButton->getSize()/2) - engine->getMouse()->getPos();
		mouseDelta.x = clamp<float>(mouseDelta.x, -engine->getScreenSize().x/2, engine->getScreenSize().x/2);
		mouseDelta.y = clamp<float>(mouseDelta.y, -engine->getScreenSize().y/2, engine->getScreenSize().y/2);
		mouseDelta.x /= engine->getScreenSize().x;
		mouseDelta.y /= engine->getScreenSize().y;

		const Vector2 pushAngle = Vector2(mouseDelta.x, mouseDelta.y) * 0.1f;

		anim->moveLinear(&m_fMainMenuAnimFriendEyeFollowX, pushAngle.x, 0.25f, true);
		anim->moveLinear(&m_fMainMenuAnimFriendEyeFollowY, pushAngle.y, 0.25f, true);
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

	// handle shuffle while idle
	if (osu_main_menu_shuffle.getBool())
	{
		if (m_osu->getSelectedBeatmap() != NULL)
		{
			if (!m_osu->getSelectedBeatmap()->isPreviewMusicPlaying())
			{
				if (engine->getTime() > m_fPrevShuffleTime)
				{
					m_fPrevShuffleTime = engine->getTime() + 1.0f;
					m_osu->getSongBrowser()->selectRandomBeatmap(false);
				}
			}
		}
	}
}

void OsuMainMenu::onKeyDown(KeyboardEvent &e)
{
	OsuScreen::onKeyDown(e); // only used for options menu
	if (!m_bVisible || e.isConsumed()) return;

	if (osu_main_menu_shuffle.getBool())
	{
		if (!m_osu->getHUD()->isVolumeOverlayBusy() && !m_osu->getOptionsMenu()->isMouseInside())
		{
			if (e == KEY_RIGHT || e == KEY_F2)
				m_osu->getSongBrowser()->selectRandomBeatmap(false);
		}
	}

	if (e == KEY_C || e == KEY_F4)
		onPausePressed();

	if (!m_bMenuElementsVisible)
	{
		if (e == KEY_P || e == KEY_ENTER)
			m_mainButton->click();
	}
	else
	{
		if (e == KEY_P || e == KEY_ENTER)
			onPlayButtonPressed();
		if (e == KEY_O)
			onOptionsButtonPressed();
		if (e == KEY_E || e == KEY_X)
			onExitButtonPressed();

		if (e == KEY_ESCAPE)
			setMenuElementsVisible(false);
	}
}

void OsuMainMenu::onMiddleChange(bool down)
{
	if (!m_bVisible) return;

	// debug anims
	if (down && !anim->isAnimating(&m_fMainMenuAnim) && !m_bMenuElementsVisible)
	{
		if (engine->getKeyboard()->isShiftDown())
		{
			m_bMainMenuAnimFriend = true;
			m_bMainMenuAnimFriendScheduled = true;
			m_bMainMenuAnimFadeToFriendForNextAnim = true;
		}

		animMainButton();
		m_fMainMenuAnimDuration = 15.0f;
		m_fMainMenuAnimTime = engine->getTime() + m_fMainMenuAnimDuration;
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
		OsuRichPresence::onMainMenu(m_osu);

		updateLayout();

		m_fMainMenuAnimDuration = 15.0f;
		m_fMainMenuAnimTime = engine->getTime() + m_fMainMenuAnimDuration;
	}

	if (visible && m_bStartupAnim)
	{
		m_bStartupAnim = false;
		anim->moveQuadOut(&m_fStartupAnim, 1.0f, osu_main_menu_startup_anim_duration.getFloat(), (float)engine->getTimeReal());
		anim->moveQuartOut(&m_fStartupAnim2, 1.0f, osu_main_menu_startup_anim_duration.getFloat()*6.0f, (float)engine->getTimeReal() + osu_main_menu_startup_anim_duration.getFloat()*0.5f);
	}
}

void OsuMainMenu::updateLayout()
{
	const float dpiScale = Osu::getUIScale(m_osu);

	m_vCenter = m_osu->getScreenSize()/2.0f;
	const float size = Osu::getUIScale(m_osu, 324.0f);
	m_vSize = Vector2(size, size);

	m_pauseButton->setSize(30 * dpiScale, 30 * dpiScale);
	m_pauseButton->setRelPos(m_osu->getScreenWidth() - m_pauseButton->getSize().x*2 - 10 * dpiScale, m_pauseButton->getSize().y + 10 * dpiScale);

	m_updateAvailableButton->setSize(375 * dpiScale, 50 * dpiScale);
	m_updateAvailableButton->setPos(m_osu->getScreenWidth()/2 - m_updateAvailableButton->getSize().x/2, m_osu->getScreenHeight() - m_updateAvailableButton->getSize().y - 10 * dpiScale);

	m_steamWorkshopButton->onResized(); // HACKHACK: framework, setSize() does not update string metrics
	m_steamWorkshopButton->setSize(m_updateAvailableButton->getSize());
	m_steamWorkshopButton->setRelPos(m_updateAvailableButton->getPos().x, m_osu->getScreenHeight() - m_steamWorkshopButton->getSize().y - 4 * dpiScale);

	m_githubButton->setSize(100 * dpiScale, 50 * dpiScale);
	m_githubButton->setRelPos(5 * dpiScale, m_osu->getScreenHeight()/2.0f - m_githubButton->getSize().y/2.0f);

	m_versionButton->onResized(); // HACKHACK: framework, setSizeToContent() does not update string metrics
	m_versionButton->setSizeToContent(8 * dpiScale, 8 * dpiScale);
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

		m_menuElements[i]->onResized(); // HACKHACK: framework, setSize() does not update string metrics
		m_menuElements[i]->setRelPos(m_mainButton->getRelPos().x + m_mainButton->getSize().x*offsetPercent - menuElementExtraWidth*offsetPercent + menuElementExtraWidth*(1.0f - offsetPercent), curY);
		m_menuElements[i]->setSize(m_mainButton->getSize().x + menuElementExtraWidth*offsetPercent - 2.0f*menuElementExtraWidth*(1.0f - offsetPercent), menuElementHeight);
		m_menuElements[i]->setTextColor(COLORf(offsetPercent, 1.0f, 1.0f, 1.0f));
		m_menuElements[i]->setFrameColor(COLORf(offsetPercent, 1.0f, 1.0f, 1.0f));
		m_menuElements[i]->setBackgroundColor(COLORf(offsetPercent, 0.0f, 0.0f, 0.0f));
	}

	m_container->setSize(m_osu->getScreenSize() + Vector2(1,1));
	m_container->update_pos();
}

void OsuMainMenu::animMainButton()
{
	m_bInMainMenuRandomAnim = true;

	m_iMainMenuRandomAnimType = (rand() % 4) == 1 ? 1 : 0;
	if (!m_bMainMenuAnimFadeToFriendForNextAnim && osu_main_menu_friend.getBool() && env->getOS() == Environment::OS::OS_WINDOWS) // NOTE: z buffer bullshit on other platforms >:(
		m_bMainMenuAnimFadeToFriendForNextAnim = (rand() % 24) == 1;

	m_fMainMenuAnim = 0.0f;
	m_fMainMenuAnim1 = 0.0f;
	m_fMainMenuAnim2 = 0.0f;

	if (m_iMainMenuRandomAnimType == 0)
	{
		m_fMainMenuAnim3 = 1.0f;

		m_fMainMenuAnim1Target = (rand() % 2) == 1 ? 1.0f : -1.0f;
		m_fMainMenuAnim2Target = (rand() % 2) == 1 ? 1.0f : -1.0f;
		m_fMainMenuAnim3Target = (rand() % 2) == 1 ? 1.0f : -1.0f;

		const float randomDuration1 = (static_cast<float>(rand())/static_cast<float>(RAND_MAX))*3.5f;
		const float randomDuration2 = (static_cast<float>(rand())/static_cast<float>(RAND_MAX))*3.5f;
		const float randomDuration3 = (static_cast<float>(rand())/static_cast<float>(RAND_MAX))*3.5f;

		anim->moveQuadOut(&m_fMainMenuAnim, 1.0f, 1.5f + std::max(randomDuration1, std::max(randomDuration2, randomDuration3)));
		anim->moveQuadOut(&m_fMainMenuAnim1, m_fMainMenuAnim1Target, 1.5f + randomDuration1);
		anim->moveQuadOut(&m_fMainMenuAnim2, m_fMainMenuAnim2Target, 1.5f + randomDuration2);
		anim->moveQuadOut(&m_fMainMenuAnim3, m_fMainMenuAnim3Target, 1.5f + randomDuration3);
	}
	else
	{
		m_fMainMenuAnim3 = 0.0f;

		m_fMainMenuAnim1Target = 0.0f;
		m_fMainMenuAnim2Target = 0.0f;
		m_fMainMenuAnim3Target = 0.0f;

		m_fMainMenuAnim = 0.0f;
		anim->moveQuadOut(&m_fMainMenuAnim, 1.0f, 5.0f);
	}
}

void OsuMainMenu::animMainButtonBack()
{
	m_bInMainMenuRandomAnim = false;

	if (anim->isAnimating(&m_fMainMenuAnim))
	{
		anim->moveQuadOut(&m_fMainMenuAnim, 1.0f, 0.25f, true);
		anim->moveQuadOut(&m_fMainMenuAnim1, m_fMainMenuAnim1Target, 0.25f, true);
		anim->moveQuadOut(&m_fMainMenuAnim1, 0.0f, 0.0f, 0.25f);
		anim->moveQuadOut(&m_fMainMenuAnim2, m_fMainMenuAnim2Target, 0.25f, true);
		anim->moveQuadOut(&m_fMainMenuAnim2, 0.0f, 0.0f, 0.25f);
		anim->moveQuadOut(&m_fMainMenuAnim3, m_fMainMenuAnim3Target, 0.10f, true);
		anim->moveQuadOut(&m_fMainMenuAnim3, 0.0f, 0.0f, 0.1f);
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
			anim->moveQuadInOut(&m_fCenterOffsetAnim, m_vSize.x/2.0f, 0.35f, 0.0f, true);
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
			anim->moveQuadOut(&m_fCenterOffsetAnim, 0.0f, 0.5f*(m_fCenterOffsetAnim/(m_vSize.x/2.0f)) * (m_fShutdownScheduledTime != 0.0f ? 0.4f : 1.0f), 0.0f, true);
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

void OsuMainMenu::writeVersionFile()
{
	// remember, don't show the notification arrow until the version changes again
	std::ofstream versionFile(MCOSU_NEWVERSION_NOTIFICATION_TRIGGER_FILE, std::ios::out | std::ios::trunc);
	if (versionFile.good())
		versionFile << Osu::version->getFloat();
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

	if (anim->isAnimating(&m_fMainMenuAnim) && m_bInMainMenuRandomAnim)
		animMainButtonBack();
	else
	{
		m_bInMainMenuRandomAnim = false;

		Vector2 mouseDelta = (m_mainButton->getPos() + m_mainButton->getSize()/2) - engine->getMouse()->getPos();
		mouseDelta.x = clamp<float>(mouseDelta.x, -m_mainButton->getSize().x/2, m_mainButton->getSize().x/2);
		mouseDelta.y = clamp<float>(mouseDelta.y, -m_mainButton->getSize().y/2, m_mainButton->getSize().y/2);
		mouseDelta.x /= m_mainButton->getSize().x;
		mouseDelta.y /= m_mainButton->getSize().y;

		const Vector2 pushAngle = Vector2(mouseDelta.y, -mouseDelta.x) * Vector2(0.15f, 0.15f);

		m_fMainMenuAnim = 0.001f;
		anim->moveQuadOut(&m_fMainMenuAnim, 1.0f, 0.15f + 0.4f, true);

		if (!anim->isAnimating(&m_fMainMenuAnim1))
			m_fMainMenuAnim1 = 0.0f;

		anim->moveQuadOut(&m_fMainMenuAnim1, pushAngle.x, 0.15f, true);
		anim->moveQuadOut(&m_fMainMenuAnim1, 0.0f, 0.4f, 0.15f);

		if (!anim->isAnimating(&m_fMainMenuAnim2))
			m_fMainMenuAnim2 = 0.0f;

		anim->moveQuadOut(&m_fMainMenuAnim2, pushAngle.y, 0.15f, true);
		anim->moveQuadOut(&m_fMainMenuAnim2, 0.0f, 0.4f, 0.15f);

		if (!anim->isAnimating(&m_fMainMenuAnim3))
			m_fMainMenuAnim3 = 0.0f;

		anim->moveQuadOut(&m_fMainMenuAnim3, 0.0f, 0.15f, true);
	}
}

void OsuMainMenu::onPlayButtonPressed()
{
	m_bMainMenuAnimFriend = false;
	m_bMainMenuAnimFadeToFriendForNextAnim = false;
	m_bMainMenuAnimFriendScheduled = false;

	if (m_osu->getInstanceID() > 1) return;

	m_osu->toggleSongBrowser();
}

void OsuMainMenu::onEditButtonPressed()
{
	m_osu->toggleEditor();
}

void OsuMainMenu::onOptionsButtonPressed()
{
	if (!m_osu->getOptionsMenu()->isVisible())
		m_osu->toggleOptionsMenu();
}

void OsuMainMenu::onExitButtonPressed()
{
	m_fShutdownScheduledTime = engine->getTime() + 0.3f;
	m_bWasCleanShutdown = true;
	setMenuElementsVisible(false);
}

void OsuMainMenu::onPausePressed()
{
	if (m_osu->getInstanceID() > 1) return;

	osu_main_menu_shuffle.setValue(0.0f);

	if (m_osu->getSelectedBeatmap() != NULL)
		m_osu->getSelectedBeatmap()->pausePreviewMusic();
}

void OsuMainMenu::onUpdatePressed()
{
	if (m_osu->getInstanceID() > 1) return;

	if (m_osu->getUpdateHandler()->getStatus() == OsuUpdateHandler::STATUS::STATUS_SUCCESS_INSTALLATION)
		engine->restart();
	else if (m_osu->getUpdateHandler()->getStatus() == OsuUpdateHandler::STATUS::STATUS_ERROR)
		m_osu->getUpdateHandler()->checkForUpdates();
}

void OsuMainMenu::onSteamWorkshopPressed()
{
	if (m_osu->getInstanceID() > 1) return;

	if (!steam->isReady())
	{
		m_osu->getNotificationOverlay()->addNotification("Error: Steam is not running.", 0xffff0000, false, 5.0f);
		openSteamWorkshopInDefaultBrowser();
		return;
	}

	openSteamWorkshopInGameOverlay(m_osu);

	m_osu->getOptionsMenu()->openAndScrollToSkinSection();
}

void OsuMainMenu::onGithubPressed()
{
	if (m_osu->getInstanceID() > 1) return;

	if (env->getOS() == Environment::OS::OS_HORIZON)
	{
		m_osu->getNotificationOverlay()->addNotification("Go to https://github.com/McKay42/McOsu/", 0xffffffff, false, 0.75f);
		return;
	}

	m_osu->getNotificationOverlay()->addNotification("Opening browser, please wait ...", 0xffffffff, false, 0.75f);
	env->openURLInDefaultBrowser("https://github.com/McKay42/McOsu/");
}

void OsuMainMenu::onVersionPressed()
{
	if (m_osu->getInstanceID() > 1) return;

	m_bDrawVersionNotificationArrow = false;
	writeVersionFile();
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
	if (m_mainMenu->m_mainButton->isMouseInside()) return;

	engine->getSound()->play(m_mainMenu->getOsu()->getSkin()->getMenuHit());
	CBaseUIButton::onMouseDownInside();
}
