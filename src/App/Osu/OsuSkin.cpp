//================ Copyright (c) 2015, PG, All rights reserved. =================//
//
// Purpose:		skin loader and container
//
// $NoKeywords: $osusk
//===============================================================================//

#include "OsuSkin.h"

#include "Engine.h"
#include "Environment.h"
#include "ResourceManager.h"
#include "SoundEngine.h"
#include "SteamworksInterface.h"
#include "ConVar.h"
#include "File.h"

#include "Osu.h"
#include "OsuSkinImage.h"
#include "OsuBeatmap.h"
#include "OsuGameRules.h"
#include "OsuNotificationOverlay.h"
#include "OsuSteamWorkshop.h"

#include <string.h>

#define OSU_BITMASK_HITWHISTLE 0x2
#define OSU_BITMASK_HITFINISH 0x4
#define OSU_BITMASK_HITCLAP 0x8

ConVar osu2_sound_source_id("osu2_sound_source_id", 1, FCVAR_NONE, "which instance/player/client should play hitsounds (e.g. master top left is always 1)");

ConVar osu_volume_effects("osu_volume_effects", 1.0f, FCVAR_NONE);
ConVar osu_skin_async("osu_skin_async", true, FCVAR_NONE, "load in background without blocking");
ConVar osu_skin_hd("osu_skin_hd", true, FCVAR_NONE, "load and use @2x versions of skin images, if available");
ConVar osu_skin_mipmaps("osu_skin_mipmaps", false, FCVAR_NONE, "generate mipmaps for every skin image (only useful on lower game resolutions, requires more vram)");
ConVar osu_skin_color_index_add("osu_skin_color_index_add", 0, FCVAR_NONE);
ConVar osu_skin_animation_force("osu_skin_animation_force", false, FCVAR_NONE);
ConVar osu_skin_use_skin_hitsounds("osu_skin_use_skin_hitsounds", true, FCVAR_NONE, "If enabled: Use skin's sound samples. If disabled: Use default skin's sound samples. For hitsounds only.");
ConVar osu_skin_force_hitsound_sample_set("osu_skin_force_hitsound_sample_set", 0, FCVAR_NONE, "force a specific hitsound sample set to always be used regardless of what the beatmap says. 0 = disabled, 1 = normal, 2 = soft, 3 = drum.");
ConVar osu_skin_random("osu_skin_random", false, FCVAR_NONE, "select random skin from list on every skin load/reload");
ConVar osu_skin_random_elements("osu_skin_random_elements", false, FCVAR_NONE, "sElECt RanDOM sKIn eLemENTs FRoM ranDom SkINs");
ConVar osu_mod_fposu_sound_panning("osu_mod_fposu_sound_panning", false, FCVAR_NONE, "see osu_sound_panning");
ConVar osu_mod_fps_sound_panning("osu_mod_fps_sound_panning", false, FCVAR_NONE, "see osu_sound_panning");
ConVar osu_sound_panning("osu_sound_panning", true, FCVAR_NONE, "positional hitsound audio depending on the playfield position");
ConVar osu_sound_panning_multiplier("osu_sound_panning_multiplier", 1.0f, FCVAR_NONE, "the final panning value is multiplied with this, e.g. if you want to reduce or increase the effect strength by a percentage");

ConVar osu_ignore_beatmap_combo_colors("osu_ignore_beatmap_combo_colors", false, FCVAR_NONE);
ConVar osu_ignore_beatmap_sample_volume("osu_ignore_beatmap_sample_volume", false, FCVAR_NONE);

ConVar osu_export_skin("osu_export_skin");
ConVar osu_skin_export("osu_skin_export");

const char *OsuSkin::OSUSKIN_DEFAULT_SKIN_PATH = ""; // set dynamically below in the constructor
Image *OsuSkin::m_missingTexture = NULL;

ConVar *OsuSkin::m_osu_skin_async = &osu_skin_async;
ConVar *OsuSkin::m_osu_skin_hd = &osu_skin_hd;

ConVar *OsuSkin::m_osu_skin_ref = NULL;
ConVar *OsuSkin::m_osu_mod_fposu_ref = NULL;

OsuSkin::OsuSkin(Osu *osu, UString name, UString filepath, bool isDefaultSkin, bool isWorkshopSkin)
{
	m_osu = osu;
	m_sName = name;
	m_sFilePath = filepath;
	m_bIsDefaultSkin = isDefaultSkin;
	m_bIsWorkshopSkin = isWorkshopSkin;

	m_bReady = false;
	m_bReadyOnce = true;

	// convar refs
	if (m_osu_skin_ref == NULL)
		m_osu_skin_ref = convar->getConVarByName("osu_skin");
	if (m_osu_mod_fposu_ref == NULL)
		m_osu_mod_fposu_ref = convar->getConVarByName("osu_mod_fposu");

	if (m_missingTexture == NULL)
		m_missingTexture = engine->getResourceManager()->getImage("MISSING_TEXTURE");

	if (m_osu->isInVRMode())
		OSUSKIN_DEFAULT_SKIN_PATH = "defaultvr/";
	else
		OSUSKIN_DEFAULT_SKIN_PATH = "default/";

	// vars
	m_hitCircle = m_missingTexture;
	m_approachCircle = m_missingTexture;
	m_reverseArrow = m_missingTexture;

	m_default0 = m_missingTexture;
	m_default1 = m_missingTexture;
	m_default2 = m_missingTexture;
	m_default3 = m_missingTexture;
	m_default4 = m_missingTexture;
	m_default5 = m_missingTexture;
	m_default6 = m_missingTexture;
	m_default7 = m_missingTexture;
	m_default8 = m_missingTexture;
	m_default9 = m_missingTexture;

	m_score0 = m_missingTexture;
	m_score1 = m_missingTexture;
	m_score2 = m_missingTexture;
	m_score3 = m_missingTexture;
	m_score4 = m_missingTexture;
	m_score5 = m_missingTexture;
	m_score6 = m_missingTexture;
	m_score7 = m_missingTexture;
	m_score8 = m_missingTexture;
	m_score9 = m_missingTexture;
	m_scoreX = m_missingTexture;
	m_scorePercent = m_missingTexture;
	m_scoreDot = m_missingTexture;

	m_combo0 = m_missingTexture;
	m_combo1 = m_missingTexture;
	m_combo2 = m_missingTexture;
	m_combo3 = m_missingTexture;
	m_combo4 = m_missingTexture;
	m_combo5 = m_missingTexture;
	m_combo6 = m_missingTexture;
	m_combo7 = m_missingTexture;
	m_combo8 = m_missingTexture;
	m_combo9 = m_missingTexture;
	m_comboX = m_missingTexture;

	m_playWarningArrow = m_missingTexture;
	m_circularmetre = m_missingTexture;

	m_particle50 = m_missingTexture;
	m_particle100 = m_missingTexture;
	m_particle300 = m_missingTexture;

	m_sliderGradient = m_missingTexture;
	m_sliderScorePoint = m_missingTexture;
	m_sliderStartCircle = m_missingTexture;
	m_sliderStartCircleOverlay = m_missingTexture;
	m_sliderEndCircle = m_missingTexture;
	m_sliderEndCircleOverlay = m_missingTexture;

	m_spinnerBackground = m_missingTexture;
	m_spinnerCircle = m_missingTexture;
	m_spinnerApproachCircle = m_missingTexture;
	m_spinnerBottom = m_missingTexture;
	m_spinnerMiddle = m_missingTexture;
	m_spinnerMiddle2 = m_missingTexture;
	m_spinnerTop = m_missingTexture;
	m_spinnerSpin = m_missingTexture;
	m_spinnerClear = m_missingTexture;

	m_defaultCursor = m_missingTexture;
	m_cursor = m_missingTexture;
	m_cursorMiddle = m_missingTexture;
	m_cursorTrail = m_missingTexture;
	m_cursorRipple = m_missingTexture;

	m_pauseContinue = m_missingTexture;
	m_pauseReplay = m_missingTexture;
	m_pauseRetry = m_missingTexture;
	m_pauseBack = m_missingTexture;
	m_pauseOverlay = m_missingTexture;
	m_failBackground = m_missingTexture;
	m_unpause = m_missingTexture;

	m_buttonLeft = m_missingTexture;
	m_buttonMiddle = m_missingTexture;
	m_buttonRight = m_missingTexture;
	m_defaultButtonLeft = m_missingTexture;
	m_defaultButtonMiddle = m_missingTexture;
	m_defaultButtonRight = m_missingTexture;

	m_songSelectTop = m_missingTexture;
	m_songSelectBottom = m_missingTexture;
	m_menuButtonBackground = m_missingTexture;
	m_star = m_missingTexture;
	m_rankingPanel = m_missingTexture;
	m_rankingGraph = m_missingTexture;
	m_rankingTitle = m_missingTexture;
	m_rankingMaxCombo = m_missingTexture;
	m_rankingAccuracy = m_missingTexture;
	m_rankingA = m_missingTexture;
	m_rankingB = m_missingTexture;
	m_rankingC = m_missingTexture;
	m_rankingD = m_missingTexture;
	m_rankingS = m_missingTexture;
	m_rankingSH = m_missingTexture;
	m_rankingX = m_missingTexture;
	m_rankingXH = m_missingTexture;

	m_beatmapImportSpinner = m_missingTexture;
	m_loadingSpinner = m_missingTexture;
	m_circleEmpty = m_missingTexture;
	m_circleFull = m_missingTexture;
	m_seekTriangle = m_missingTexture;
	m_userIcon = m_missingTexture;
	m_backgroundCube = m_missingTexture;
	m_menuBackground = m_missingTexture;
	m_skybox = m_missingTexture;

	m_normalHitNormal = NULL;
	m_normalHitWhistle = NULL;
	m_normalHitFinish = NULL;
	m_normalHitClap = NULL;

	m_normalSliderTick = NULL;
	m_normalSliderSlide = NULL;
	m_normalSliderWhistle = NULL;

	m_softHitNormal = NULL;
	m_softHitWhistle = NULL;
	m_softHitFinish = NULL;
	m_softHitClap = NULL;

	m_softSliderTick = NULL;
	m_softSliderSlide = NULL;
	m_softSliderWhistle = NULL;

	m_drumHitNormal = NULL;
	m_drumHitWhistle = NULL;
	m_drumHitFinish = NULL;
	m_drumHitClap = NULL;

	m_drumSliderTick = NULL;
	m_drumSliderSlide = NULL;
	m_drumSliderWhistle = NULL;

	m_spinnerBonus = NULL;
	m_spinnerSpinSound = NULL;

	m_combobreak = NULL;
	m_failsound = NULL;
	m_applause = NULL;
	m_menuHit = NULL;
	m_menuClick = NULL;
	m_checkOn = NULL;
	m_checkOff = NULL;
	m_shutter = NULL;
	m_sectionPassSound = NULL;
	m_sectionFailSound = NULL;

	m_spinnerApproachCircleColor = 0xffffffff;
	m_sliderBorderColor = 0xffffffff;
	m_sliderBallColor = 0xffffffff; // NOTE: 0xff02aaff is a hardcoded special case for osu!'s default skin, but it does not apply to user skins

	m_songSelectActiveText = 0xff000000;
	m_songSelectInactiveText = 0xffffffff;
	m_inputOverlayText = 0xff000000;

	// scaling
	m_bCursor2x = false;
	m_bCursorTrail2x = false;
	m_bCursorRipple2x = false;
	m_bApproachCircle2x = false;
	m_bReverseArrow2x = false;
	m_bHitCircle2x = false;
	m_bIsDefault02x = false;
	m_bIsDefault12x = false;
	m_bIsScore02x = false;
	m_bIsCombo02x = false;
	m_bSpinnerApproachCircle2x = false;
	m_bSpinnerBottom2x= false;
	m_bSpinnerCircle2x= false;
	m_bSpinnerTop2x= false;
	m_bSpinnerMiddle2x= false;
	m_bSpinnerMiddle22x= false;
	m_bSliderScorePoint2x = false;
	m_bSliderStartCircle2x = false;
	m_bSliderEndCircle2x = false;

	m_bCircularmetre2x = false;

	m_bPauseContinue2x = false;

	m_bMenuButtonBackground2x = false;
	m_bStar2x = false;
	m_bRankingPanel2x = false;
	m_bRankingMaxCombo2x = false;
	m_bRankingAccuracy2x = false;
	m_bRankingA2x = false;
	m_bRankingB2x = false;
	m_bRankingC2x = false;
	m_bRankingD2x = false;
	m_bRankingS2x = false;
	m_bRankingSH2x = false;
	m_bRankingX2x = false;
	m_bRankingXH2x = false;

	// skin.ini
	m_fVersion = 1;
	m_fAnimationFramerate = 0.0f;
	m_bCursorCenter = true;
	m_bCursorRotate = true;
	m_bCursorExpand = true;

	m_bSliderBallFlip = true;
	m_bAllowSliderBallTint = false;
	m_iSliderStyle = 2;
	m_bHitCircleOverlayAboveNumber = true;
	m_bSliderTrackOverride = false;

	m_iHitCircleOverlap = 0;
	m_iScoreOverlap = 0;
	m_iComboOverlap = 0;

	// custom
	m_iSampleSet = 1;
	m_iSampleVolume = (int)(osu_volume_effects.getFloat()*100.0f);

	m_bIsRandom = osu_skin_random.getBool();
	m_bIsRandomElements = osu_skin_random_elements.getBool();

	// load all files
	load();

	// convar callbacks
	osu_volume_effects.setCallback( fastdelegate::MakeDelegate(this, &OsuSkin::onEffectVolumeChange) );
	osu_ignore_beatmap_sample_volume.setCallback( fastdelegate::MakeDelegate(this, &OsuSkin::onIgnoreBeatmapSampleVolumeChange) );
	osu_export_skin.setCallback( fastdelegate::MakeDelegate(this, &OsuSkin::onExport) );
	osu_skin_export.setCallback( fastdelegate::MakeDelegate(this, &OsuSkin::onExport) );
}

OsuSkin::~OsuSkin()
{
	stopSliderSlideSound();
	stopSpinnerSpinSound();

	for (int i=0; i<m_resources.size(); i++)
	{
		if (m_resources[i] != (Resource*)m_missingTexture)
			engine->getResourceManager()->destroyResource(m_resources[i]);
	}
	m_resources.clear();

	for (int i=0; i<m_images.size(); i++)
	{
		delete m_images[i];
	}
	m_images.clear();

	m_sounds.clear();

	m_filepathsForExport.clear();
}

void OsuSkin::update()
{
	// tasks which have to be run after async loading finishes
	if (!m_bReady && isReady())
	{
		// force effect volume update
		onEffectVolumeChange("", UString::format("%f", osu_volume_effects.getFloat()));

		m_bReady = true;
	}

	// shitty check to not animate while paused with hitobjects in background
	if (m_osu->isInPlayMode() && m_osu->getSelectedBeatmap() != NULL && !m_osu->getSelectedBeatmap()->isPlaying() && !osu_skin_animation_force.getBool()) return;

	const bool useEngineTimeForAnimations = !m_osu->isInPlayMode();
	const long curMusicPos = m_osu->getSelectedBeatmap() != NULL ? m_osu->getSelectedBeatmap()->getCurMusicPosWithOffsets() : 0;
	for (int i=0; i<m_images.size(); i++)
	{
		m_images[i]->update(useEngineTimeForAnimations, curMusicPos);
	}
}

void OsuSkin::onJustBeforeReady()
{
	// prevent invalid image sizes from breaking the songbrowser UI layout
	{
		if (m_songSelectTop->getWidth() < 3 || m_songSelectTop->getHeight() < 3)
			m_songSelectTop = engine->getResourceManager()->getImage("OSU_SKIN_SONGSELECT_TOP_DEFAULT");

		if (m_songSelectBottom->getHeight() < 3 || m_songSelectBottom->getHeight() < 3)
			m_songSelectBottom = engine->getResourceManager()->getImage("OSU_SKIN_SONGSELECT_BOTTOM_DEFAULT");
	}
}

bool OsuSkin::isReady()
{
	if (m_bReady) return true;

	for (int i=0; i<m_resources.size(); i++)
	{
		if (engine->getResourceManager()->isLoadingResource(m_resources[i]))
			return false;
	}

	for (int i=0; i<m_images.size(); i++)
	{
		if (!m_images[i]->isReady())
			return false;
	}

	if (m_bReadyOnce)
	{
		m_bReadyOnce = false;

		onJustBeforeReady();
	}

	// (ready is set in update())

	return true;
}

void OsuSkin::load()
{
	// random skins
	{
		filepathsForRandomSkin.clear();
		if (m_bIsRandom || m_bIsRandomElements)
		{
			std::vector<UString> skinNames;

			// steam workshop items
			if (steam->isReady())
			{
				if (!m_osu->getSteamWorkshop()->isReady())
					m_osu->getSteamWorkshop()->refresh(false, false);

				if (m_osu->getSteamWorkshop()->isReady())
				{
					const std::vector<OsuSteamWorkshop::SUBSCRIBED_ITEM> &subscribedItems = m_osu->getSteamWorkshop()->getSubscribedItems();

					for (int i=0; i<subscribedItems.size(); i++)
					{
						UString randomSkinFolder = subscribedItems[i].installInfo;

						// ensure that the skinFolder ends with a slash
						if (randomSkinFolder[randomSkinFolder.length()-1] != L'/' && randomSkinFolder[randomSkinFolder.length()-1] != L'\\')
							randomSkinFolder.append("/");

						filepathsForRandomSkin.push_back(randomSkinFolder);
						skinNames.push_back(subscribedItems[i].title);
					}
				}
			}

			// regular skins
			{
				UString skinFolder = convar->getConVarByName("osu_folder")->getString();
				skinFolder.append(convar->getConVarByName("osu_folder_sub_skins")->getString());
				std::vector<UString> skinFolders = env->getFoldersInFolder(skinFolder);

				for (int i=0; i<skinFolders.size(); i++)
				{
					if (skinFolders[i] == "." || skinFolders[i] == "..") // is this universal in every file system? too lazy to check. should probably fix this in the engine and not here
						continue;

					UString randomSkinFolder = skinFolder;
					randomSkinFolder.append(skinFolders[i]);
					randomSkinFolder.append("/");

					filepathsForRandomSkin.push_back(randomSkinFolder);
					skinNames.push_back(skinFolders[i]);
				}
			}

			if (m_bIsRandom && filepathsForRandomSkin.size() > 0)
			{
				const int randomIndex = std::rand() % std::min(filepathsForRandomSkin.size(), skinNames.size());

				m_sName = skinNames[randomIndex];
				m_sFilePath = filepathsForRandomSkin[randomIndex];
			}
		}
	}

	// spinner loading has top priority in async
	randomizeFilePath();
	{
		checkLoadImage(&m_loadingSpinner, "loading-spinner", "OSU_SKIN_LOADING_SPINNER");
	}

	// and the cursor comes right after that
	randomizeFilePath();
	{
		checkLoadImage(&m_cursor, "cursor", "OSU_SKIN_CURSOR");
		checkLoadImage(&m_cursorMiddle, "cursormiddle", "OSU_SKIN_CURSORMIDDLE", true);
		checkLoadImage(&m_cursorTrail, "cursortrail", "OSU_SKIN_CURSORTRAIL");
		checkLoadImage(&m_cursorRipple, "cursor-ripple", "OSU_SKIN_CURSORRIPPLE");

		// special case: if fallback to default cursor, do load cursorMiddle
		if (m_cursor == engine->getResourceManager()->getImage("OSU_SKIN_CURSOR_DEFAULT"))
			checkLoadImage(&m_cursorMiddle, "cursormiddle", "OSU_SKIN_CURSORMIDDLE");
	}

	// skin ini
	randomizeFilePath();
	m_sSkinIniFilePath = m_sFilePath;
	UString defaultSkinIniFilePath = UString(env->getOS() == Environment::OS::OS_HORIZON ? "romfs:/materials/" : "./materials/");
	defaultSkinIniFilePath.append(OSUSKIN_DEFAULT_SKIN_PATH);
	defaultSkinIniFilePath.append("skin.ini");
	m_sSkinIniFilePath.append("skin.ini");
	bool parseSkinIni1Status = true;
	bool parseSkinIni2Status = true;
	if (!parseSkinINI(m_sSkinIniFilePath))
	{
		parseSkinIni1Status = false;
		m_sSkinIniFilePath = defaultSkinIniFilePath;
		parseSkinIni2Status = parseSkinINI(m_sSkinIniFilePath);
	}

	// default values, if none were loaded
	if (m_comboColors.size() == 0)
	{
		m_comboColors.push_back(COLOR(255, 255, 192, 0));
		m_comboColors.push_back(COLOR(255, 0, 202, 0));
		m_comboColors.push_back(COLOR(255, 18, 124, 255));
		m_comboColors.push_back(COLOR(255, 242, 24, 57));
	}

	// images
	randomizeFilePath();
	checkLoadImage(&m_hitCircle, "hitcircle", "OSU_SKIN_HITCIRCLE");
	m_hitCircleOverlay2 = createOsuSkinImage("hitcircleoverlay", Vector2(128, 128), 64);
	m_hitCircleOverlay2->setAnimationFramerate(2);

	randomizeFilePath();
	checkLoadImage(&m_approachCircle, "approachcircle", "OSU_SKIN_APPROACHCIRCLE");
	randomizeFilePath();
	checkLoadImage(&m_reverseArrow, "reversearrow", "OSU_SKIN_REVERSEARROW");

	randomizeFilePath();
	m_followPoint2 = createOsuSkinImage("followpoint", Vector2(16, 22), 64);

	randomizeFilePath();
	{
		UString hitCirclePrefix = m_sHitCirclePrefix.length() > 0 ? m_sHitCirclePrefix : "default";
		UString hitCircleStringFinal = hitCirclePrefix; hitCircleStringFinal.append("-0");
		checkLoadImage(&m_default0, hitCircleStringFinal, "OSU_SKIN_DEFAULT0");
		if (m_default0 == m_missingTexture) checkLoadImage(&m_default0, "default-0", "OSU_SKIN_DEFAULT0"); // special cases: fallback to default skin hitcircle numbers if the defined prefix doesn't point to any valid files
		hitCircleStringFinal = hitCirclePrefix; hitCircleStringFinal.append("-1");
		checkLoadImage(&m_default1, hitCircleStringFinal, "OSU_SKIN_DEFAULT1");
		if (m_default1 == m_missingTexture) checkLoadImage(&m_default1, "default-1", "OSU_SKIN_DEFAULT1");
		hitCircleStringFinal = hitCirclePrefix; hitCircleStringFinal.append("-2");
		checkLoadImage(&m_default2, hitCircleStringFinal, "OSU_SKIN_DEFAULT2");
		if (m_default2 == m_missingTexture) checkLoadImage(&m_default2, "default-2", "OSU_SKIN_DEFAULT2");
		hitCircleStringFinal = hitCirclePrefix; hitCircleStringFinal.append("-3");
		checkLoadImage(&m_default3, hitCircleStringFinal, "OSU_SKIN_DEFAULT3");
		if (m_default3 == m_missingTexture) checkLoadImage(&m_default3, "default-3", "OSU_SKIN_DEFAULT3");
		hitCircleStringFinal = hitCirclePrefix; hitCircleStringFinal.append("-4");
		checkLoadImage(&m_default4, hitCircleStringFinal, "OSU_SKIN_DEFAULT4");
		if (m_default4 == m_missingTexture) checkLoadImage(&m_default4, "default-4", "OSU_SKIN_DEFAULT4");
		hitCircleStringFinal = hitCirclePrefix; hitCircleStringFinal.append("-5");
		checkLoadImage(&m_default5, hitCircleStringFinal, "OSU_SKIN_DEFAULT5");
		if (m_default5 == m_missingTexture) checkLoadImage(&m_default5, "default-5", "OSU_SKIN_DEFAULT5");
		hitCircleStringFinal = hitCirclePrefix; hitCircleStringFinal.append("-6");
		checkLoadImage(&m_default6, hitCircleStringFinal, "OSU_SKIN_DEFAULT6");
		if (m_default6 == m_missingTexture) checkLoadImage(&m_default6, "default-6", "OSU_SKIN_DEFAULT6");
		hitCircleStringFinal = hitCirclePrefix; hitCircleStringFinal.append("-7");
		checkLoadImage(&m_default7, hitCircleStringFinal, "OSU_SKIN_DEFAULT7");
		if (m_default7 == m_missingTexture) checkLoadImage(&m_default7, "default-7", "OSU_SKIN_DEFAULT7");
		hitCircleStringFinal = hitCirclePrefix; hitCircleStringFinal.append("-8");
		checkLoadImage(&m_default8, hitCircleStringFinal, "OSU_SKIN_DEFAULT8");
		if (m_default8 == m_missingTexture) checkLoadImage(&m_default8, "default-8", "OSU_SKIN_DEFAULT8");
		hitCircleStringFinal = hitCirclePrefix; hitCircleStringFinal.append("-9");
		checkLoadImage(&m_default9, hitCircleStringFinal, "OSU_SKIN_DEFAULT9");
		if (m_default9 == m_missingTexture) checkLoadImage(&m_default9, "default-9", "OSU_SKIN_DEFAULT9");
	}

	randomizeFilePath();
	{
		UString scorePrefix = m_sScorePrefix.length() > 0 ? m_sScorePrefix : "score";
		UString scoreStringFinal = scorePrefix; scoreStringFinal.append("-0");
		checkLoadImage(&m_score0, scoreStringFinal, "OSU_SKIN_SCORE0");
		if (m_score0 == m_missingTexture) checkLoadImage(&m_score0, "score-0", "OSU_SKIN_SCORE0"); // special cases: fallback to default skin score numbers if the defined prefix doesn't point to any valid files
		scoreStringFinal = scorePrefix; scoreStringFinal.append("-1");
		checkLoadImage(&m_score1, scoreStringFinal, "OSU_SKIN_SCORE1");
		if (m_score1 == m_missingTexture) checkLoadImage(&m_score1, "score-1", "OSU_SKIN_SCORE1");
		scoreStringFinal = scorePrefix; scoreStringFinal.append("-2");
		checkLoadImage(&m_score2, scoreStringFinal, "OSU_SKIN_SCORE2");
		if (m_score2 == m_missingTexture) checkLoadImage(&m_score2, "score-2", "OSU_SKIN_SCORE2");
		scoreStringFinal = scorePrefix; scoreStringFinal.append("-3");
		checkLoadImage(&m_score3, scoreStringFinal, "OSU_SKIN_SCORE3");
		if (m_score3 == m_missingTexture) checkLoadImage(&m_score3, "score-3", "OSU_SKIN_SCORE3");
		scoreStringFinal = scorePrefix; scoreStringFinal.append("-4");
		checkLoadImage(&m_score4, scoreStringFinal, "OSU_SKIN_SCORE4");
		if (m_score4 == m_missingTexture) checkLoadImage(&m_score4, "score-4", "OSU_SKIN_SCORE4");
		scoreStringFinal = scorePrefix; scoreStringFinal.append("-5");
		checkLoadImage(&m_score5, scoreStringFinal, "OSU_SKIN_SCORE5");
		if (m_score5 == m_missingTexture) checkLoadImage(&m_score5, "score-5", "OSU_SKIN_SCORE5");
		scoreStringFinal = scorePrefix; scoreStringFinal.append("-6");
		checkLoadImage(&m_score6, scoreStringFinal, "OSU_SKIN_SCORE6");
		if (m_score6 == m_missingTexture) checkLoadImage(&m_score6, "score-6", "OSU_SKIN_SCORE6");
		scoreStringFinal = scorePrefix; scoreStringFinal.append("-7");
		checkLoadImage(&m_score7, scoreStringFinal, "OSU_SKIN_SCORE7");
		if (m_score7 == m_missingTexture) checkLoadImage(&m_score7, "score-7", "OSU_SKIN_SCORE7");
		scoreStringFinal = scorePrefix; scoreStringFinal.append("-8");
		checkLoadImage(&m_score8, scoreStringFinal, "OSU_SKIN_SCORE8");
		if (m_score8 == m_missingTexture) checkLoadImage(&m_score8, "score-8", "OSU_SKIN_SCORE8");
		scoreStringFinal = scorePrefix; scoreStringFinal.append("-9");
		checkLoadImage(&m_score9, scoreStringFinal, "OSU_SKIN_SCORE9");
		if (m_score9 == m_missingTexture) checkLoadImage(&m_score9, "score-9", "OSU_SKIN_SCORE9");



		scoreStringFinal = scorePrefix; scoreStringFinal.append("-x");
		checkLoadImage(&m_scoreX, scoreStringFinal, "OSU_SKIN_SCOREX");
		//if (m_scoreX == m_missingTexture) checkLoadImage(&m_scoreX, "score-x", "OSU_SKIN_SCOREX"); // special case: ScorePrefix'd skins don't get default fallbacks, instead missing extraneous things like the X are simply not drawn
		scoreStringFinal = scorePrefix; scoreStringFinal.append("-percent");
		checkLoadImage(&m_scorePercent, scoreStringFinal, "OSU_SKIN_SCOREPERCENT");
		//if (m_scorePercent == m_missingTexture) checkLoadImage(&m_scorePercent, "score-percent", "OSU_SKIN_SCOREPERCENT"); // special case: ScorePrefix'd skins don't get default fallbacks, instead missing extraneous things like the X are simply not drawn
		scoreStringFinal = scorePrefix; scoreStringFinal.append("-dot");
		checkLoadImage(&m_scoreDot, scoreStringFinal, "OSU_SKIN_SCOREDOT");
		//if (m_scoreDot == m_missingTexture) checkLoadImage(&m_scoreDot, "score-dot", "OSU_SKIN_SCOREDOT"); // special case: ScorePrefix'd skins don't get default fallbacks, instead missing extraneous things like the X are simply not drawn
	}

	randomizeFilePath();
	{
		UString comboPrefix = m_sComboPrefix.length() > 0 ? m_sComboPrefix : "score"; // yes, "score" is the default value for the combo prefix
		UString comboStringFinal = comboPrefix; comboStringFinal.append("-0");
		checkLoadImage(&m_combo0, comboStringFinal, "OSU_SKIN_COMBO0");
		if (m_combo0 == m_missingTexture) checkLoadImage(&m_combo0, "score-0", "OSU_SKIN_COMBO0"); // special cases: fallback to default skin combo numbers if the defined prefix doesn't point to any valid files
		comboStringFinal = comboPrefix; comboStringFinal.append("-1");
		checkLoadImage(&m_combo1, comboStringFinal, "OSU_SKIN_COMBO1");
		if (m_combo1 == m_missingTexture) checkLoadImage(&m_combo1, "score-1", "OSU_SKIN_COMBO1");
		comboStringFinal = comboPrefix; comboStringFinal.append("-2");
		checkLoadImage(&m_combo2, comboStringFinal, "OSU_SKIN_COMBO2");
		if (m_combo2 == m_missingTexture) checkLoadImage(&m_combo2, "score-2", "OSU_SKIN_COMBO2");
		comboStringFinal = comboPrefix; comboStringFinal.append("-3");
		checkLoadImage(&m_combo3, comboStringFinal, "OSU_SKIN_COMBO3");
		if (m_combo3 == m_missingTexture) checkLoadImage(&m_combo3, "score-3", "OSU_SKIN_COMBO3");
		comboStringFinal = comboPrefix; comboStringFinal.append("-4");
		checkLoadImage(&m_combo4, comboStringFinal, "OSU_SKIN_COMBO4");
		if (m_combo4 == m_missingTexture) checkLoadImage(&m_combo4, "score-4", "OSU_SKIN_COMBO4");
		comboStringFinal = comboPrefix; comboStringFinal.append("-5");
		checkLoadImage(&m_combo5, comboStringFinal, "OSU_SKIN_COMBO5");
		if (m_combo5 == m_missingTexture) checkLoadImage(&m_combo5, "score-5", "OSU_SKIN_COMBO5");
		comboStringFinal = comboPrefix; comboStringFinal.append("-6");
		checkLoadImage(&m_combo6, comboStringFinal, "OSU_SKIN_COMBO6");
		if (m_combo6 == m_missingTexture) checkLoadImage(&m_combo6, "score-6", "OSU_SKIN_COMBO6");
		comboStringFinal = comboPrefix; comboStringFinal.append("-7");
		checkLoadImage(&m_combo7, comboStringFinal, "OSU_SKIN_COMBO7");
		if (m_combo7 == m_missingTexture) checkLoadImage(&m_combo7, "score-7", "OSU_SKIN_COMBO7");
		comboStringFinal = comboPrefix; comboStringFinal.append("-8");
		checkLoadImage(&m_combo8, comboStringFinal, "OSU_SKIN_COMBO8");
		if (m_combo8 == m_missingTexture) checkLoadImage(&m_combo8, "score-8", "OSU_SKIN_COMBO8");
		comboStringFinal = comboPrefix; comboStringFinal.append("-9");
		checkLoadImage(&m_combo9, comboStringFinal, "OSU_SKIN_COMBO9");
		if (m_combo9 == m_missingTexture) checkLoadImage(&m_combo9, "score-9", "OSU_SKIN_COMBO9");



		comboStringFinal = comboPrefix; comboStringFinal.append("-x");
		checkLoadImage(&m_comboX, comboStringFinal, "OSU_SKIN_COMBOX");
		//if (m_comboX == m_missingTexture) m_comboX = m_scoreX; // special case: ComboPrefix'd skins don't get default fallbacks, instead missing extraneous things like the X are simply not drawn
	}

	randomizeFilePath();
	m_playSkip = createOsuSkinImage("play-skip", Vector2(193, 147), 94);
	randomizeFilePath();
	checkLoadImage(&m_playWarningArrow, "play-warningarrow", "OSU_SKIN_PLAYWARNINGARROW");
	m_playWarningArrow2 = createOsuSkinImage("play-warningarrow", Vector2(167, 129), 128);
	randomizeFilePath();
	checkLoadImage(&m_circularmetre, "circularmetre", "OSU_SKIN_CIRCULARMETRE");
	randomizeFilePath();
	m_scorebarBg = createOsuSkinImage("scorebar-bg", Vector2(695, 44), 27.5f);
	m_scorebarColour = createOsuSkinImage("scorebar-colour", Vector2(645, 10), 6.25f);
	m_scorebarMarker = createOsuSkinImage("scorebar-marker", Vector2(24, 24), 15.0f);
	m_scorebarKi = createOsuSkinImage("scorebar-ki", Vector2(116, 116), 72.0f);
	m_scorebarKiDanger = createOsuSkinImage("scorebar-kidanger", Vector2(116, 116), 72.0f);
	m_scorebarKiDanger2 = createOsuSkinImage("scorebar-kidanger2", Vector2(116, 116), 72.0f);
	randomizeFilePath();
	m_sectionPassImage = createOsuSkinImage("section-pass", Vector2(650, 650), 400.0f);
	randomizeFilePath();
	m_sectionFailImage = createOsuSkinImage("section-fail", Vector2(650, 650), 400.0f);
	randomizeFilePath();
	m_inputoverlayBackground = createOsuSkinImage("inputoverlay-background", Vector2(193, 55), 34.25f);
	m_inputoverlayKey = createOsuSkinImage("inputoverlay-key", Vector2(43, 46), 26.75f);

	randomizeFilePath();
	m_hit0 = createOsuSkinImage("hit0", Vector2(128, 128), 42);
	m_hit0->setAnimationFramerate(60);
	m_hit50 = createOsuSkinImage("hit50", Vector2(128, 128), 42);
	m_hit50->setAnimationFramerate(60);
	m_hit50g = createOsuSkinImage("hit50g", Vector2(128, 128), 42);
	m_hit50g->setAnimationFramerate(60);
	m_hit50k = createOsuSkinImage("hit50k", Vector2(128, 128), 42);
	m_hit50k->setAnimationFramerate(60);
	m_hit100 = createOsuSkinImage("hit100", Vector2(128, 128), 42);
	m_hit100->setAnimationFramerate(60);
	m_hit100g = createOsuSkinImage("hit100g", Vector2(128, 128), 42);
	m_hit100g->setAnimationFramerate(60);
	m_hit100k = createOsuSkinImage("hit100k", Vector2(128, 128), 42);
	m_hit100k->setAnimationFramerate(60);
	m_hit300 = createOsuSkinImage("hit300", Vector2(128, 128), 42);
	m_hit300->setAnimationFramerate(60);
	m_hit300g = createOsuSkinImage("hit300g", Vector2(128, 128), 42);
	m_hit300g->setAnimationFramerate(60);
	m_hit300k = createOsuSkinImage("hit300k", Vector2(128, 128), 42);
	m_hit300k->setAnimationFramerate(60);

	randomizeFilePath();
	checkLoadImage(&m_particle50, "particle50", "OSU_SKIN_PARTICLE50", true);
	checkLoadImage(&m_particle100, "particle100", "OSU_SKIN_PARTICLE100", true);
	checkLoadImage(&m_particle300, "particle300", "OSU_SKIN_PARTICLE300", true);

	randomizeFilePath();
	checkLoadImage(&m_sliderGradient, "slidergradient", "OSU_SKIN_SLIDERGRADIENT");
	randomizeFilePath();
	m_sliderb = createOsuSkinImage("sliderb", Vector2(128, 128), 64, false, "");
	m_sliderb->setAnimationFramerate(/*45.0f*/ 50.0f);
	randomizeFilePath();
	checkLoadImage(&m_sliderScorePoint, "sliderscorepoint", "OSU_SKIN_SLIDERSCOREPOINT");
	randomizeFilePath();
	m_sliderFollowCircle2 = createOsuSkinImage("sliderfollowcircle", Vector2(259, 259), 64);
	randomizeFilePath();
	checkLoadImage(&m_sliderStartCircle, "sliderstartcircle", "OSU_SKIN_SLIDERSTARTCIRCLE", !m_bIsDefaultSkin); // !m_bIsDefaultSkin ensures that default doesn't override user, in these special cases
	m_sliderStartCircle2 = createOsuSkinImage("sliderstartcircle", Vector2(128, 128), 64, !m_bIsDefaultSkin);
	checkLoadImage(&m_sliderStartCircleOverlay, "sliderstartcircleoverlay", "OSU_SKIN_SLIDERSTARTCIRCLEOVERLAY", !m_bIsDefaultSkin);
	m_sliderStartCircleOverlay2 = createOsuSkinImage("sliderstartcircleoverlay", Vector2(128, 128), 64, !m_bIsDefaultSkin);
	m_sliderStartCircleOverlay2->setAnimationFramerate(2);
	randomizeFilePath();
	checkLoadImage(&m_sliderEndCircle, "sliderendcircle", "OSU_SKIN_SLIDERENDCIRCLE", !m_bIsDefaultSkin);
	m_sliderEndCircle2 = createOsuSkinImage("sliderendcircle", Vector2(128, 128), 64, !m_bIsDefaultSkin);
	checkLoadImage(&m_sliderEndCircleOverlay, "sliderendcircleoverlay", "OSU_SKIN_SLIDERENDCIRCLEOVERLAY", !m_bIsDefaultSkin);
	m_sliderEndCircleOverlay2 = createOsuSkinImage("sliderendcircleoverlay", Vector2(128, 128), 64, !m_bIsDefaultSkin);
	m_sliderEndCircleOverlay2->setAnimationFramerate(2);

	randomizeFilePath();
	checkLoadImage(&m_spinnerBackground, "spinner-background", "OSU_SKIN_SPINNERBACKGROUND");
	checkLoadImage(&m_spinnerCircle, "spinner-circle", "OSU_SKIN_SPINNERCIRCLE");
	checkLoadImage(&m_spinnerApproachCircle, "spinner-approachcircle", "OSU_SKIN_SPINNERAPPROACHCIRCLE");
	checkLoadImage(&m_spinnerBottom, "spinner-bottom", "OSU_SKIN_SPINNERBOTTOM");
	checkLoadImage(&m_spinnerMiddle, "spinner-middle", "OSU_SKIN_SPINNERMIDDLE");
	checkLoadImage(&m_spinnerMiddle2, "spinner-middle2", "OSU_SKIN_SPINNERMIDDLE2");
	checkLoadImage(&m_spinnerTop, "spinner-top", "OSU_SKIN_SPINNERTOP");
	checkLoadImage(&m_spinnerSpin, "spinner-spin", "OSU_SKIN_SPINNERSPIN");
	checkLoadImage(&m_spinnerClear, "spinner-clear", "OSU_SKIN_SPINNERCLEAR");

	{
		// cursor loading was here, moved up to improve async usability
	}

	randomizeFilePath();
	m_selectionModEasy = createOsuSkinImage("selection-mod-easy", Vector2(68, 66), 38);
	m_selectionModNoFail = createOsuSkinImage("selection-mod-nofail", Vector2(68, 66), 38);
	m_selectionModHalfTime = createOsuSkinImage("selection-mod-halftime", Vector2(68, 66), 38);
	m_selectionModHardRock = createOsuSkinImage("selection-mod-hardrock", Vector2(68, 66), 38);
	m_selectionModSuddenDeath = createOsuSkinImage("selection-mod-suddendeath", Vector2(68, 66), 38);
	m_selectionModPerfect = createOsuSkinImage("selection-mod-perfect", Vector2(68, 66), 38);
	m_selectionModDoubleTime = createOsuSkinImage("selection-mod-doubletime", Vector2(68, 66), 38);
	m_selectionModNightCore = createOsuSkinImage("selection-mod-nightcore", Vector2(68, 66), 38);
	m_selectionModDayCore = createOsuSkinImage("selection-mod-daycore", Vector2(68, 66), 38);
	m_selectionModHidden = createOsuSkinImage("selection-mod-hidden", Vector2(68, 66), 38);
	m_selectionModFlashlight = createOsuSkinImage("selection-mod-flashlight", Vector2(68, 66), 38);
	m_selectionModRelax = createOsuSkinImage("selection-mod-relax", Vector2(68, 66), 38);
	m_selectionModAutopilot = createOsuSkinImage("selection-mod-relax2", Vector2(68, 66), 38);
	m_selectionModSpunOut = createOsuSkinImage("selection-mod-spunout", Vector2(68, 66), 38);
	m_selectionModAutoplay = createOsuSkinImage("selection-mod-autoplay", Vector2(68, 66), 38);
	m_selectionModNightmare = createOsuSkinImage("selection-mod-nightmare", Vector2(68, 66), 38);
	m_selectionModTarget = createOsuSkinImage("selection-mod-target", Vector2(68, 66), 38);
	m_selectionModScorev2 = createOsuSkinImage("selection-mod-scorev2", Vector2(68, 66), 38);
	m_selectionModTD = createOsuSkinImage("selection-mod-touchdevice", Vector2(68, 66), 38);
	m_selectionModCinema = createOsuSkinImage("selection-mod-cinema", Vector2(68, 66), 38);

	randomizeFilePath();
	checkLoadImage(&m_pauseContinue, "pause-continue", "OSU_SKIN_PAUSE_CONTINUE");
	checkLoadImage(&m_pauseReplay, "pause-replay", "OSU_SKIN_PAUSE_REPLAY");
	checkLoadImage(&m_pauseRetry, "pause-retry", "OSU_SKIN_PAUSE_RETRY");
	checkLoadImage(&m_pauseBack, "pause-back", "OSU_SKIN_PAUSE_BACK");
	checkLoadImage(&m_pauseOverlay, "pause-overlay", "OSU_SKIN_PAUSE_OVERLAY"); if (m_pauseOverlay == m_missingTexture) checkLoadImage(&m_pauseOverlay, "pause-overlay", "OSU_SKIN_PAUSE_OVERLAY", true, "jpg");
	checkLoadImage(&m_failBackground, "fail-background", "OSU_SKIN_FAIL_BACKGROUND"); if (m_failBackground == m_missingTexture) checkLoadImage(&m_failBackground, "fail-background", "OSU_SKIN_FAIL_BACKGROUND", true, "jpg");
	checkLoadImage(&m_unpause, "unpause", "OSU_SKIN_UNPAUSE");

	randomizeFilePath();
	checkLoadImage(&m_buttonLeft, "button-left", "OSU_SKIN_BUTTON_LEFT");
	checkLoadImage(&m_buttonMiddle, "button-middle", "OSU_SKIN_BUTTON_MIDDLE");
	checkLoadImage(&m_buttonRight, "button-right", "OSU_SKIN_BUTTON_RIGHT");
	randomizeFilePath();
	m_menuBack = createOsuSkinImage("menu-back", Vector2(225, 87), 54);
	randomizeFilePath();
	m_selectionMode = createOsuSkinImage("selection-mode", Vector2(90, 90), 38); // NOTE: should actually be Vector2(88, 90), but slightly overscale to make most skins fit better on the bottombar blue line
	m_selectionModeOver = createOsuSkinImage("selection-mode-over", Vector2(88, 90), 38);
	m_selectionMods = createOsuSkinImage("selection-mods", Vector2(74, 90), 38);
	m_selectionModsOver = createOsuSkinImage("selection-mods-over", Vector2(74, 90), 38);
	m_selectionRandom = createOsuSkinImage("selection-random", Vector2(74, 90), 38);
	m_selectionRandomOver = createOsuSkinImage("selection-random-over", Vector2(74, 90), 38);
	m_selectionOptions = createOsuSkinImage("selection-options", Vector2(74, 90), 38);
	m_selectionOptionsOver = createOsuSkinImage("selection-options-over", Vector2(74, 90), 38);

	randomizeFilePath();
	checkLoadImage(&m_songSelectTop, "songselect-top", "OSU_SKIN_SONGSELECT_TOP");
	checkLoadImage(&m_songSelectBottom, "songselect-bottom", "OSU_SKIN_SONGSELECT_BOTTOM");
	randomizeFilePath();
	checkLoadImage(&m_menuButtonBackground, "menu-button-background", "OSU_SKIN_MENU_BUTTON_BACKGROUND");
	m_menuButtonBackground2 = createOsuSkinImage("menu-button-background", Vector2(699, 103), 64.0f);
	randomizeFilePath();
	checkLoadImage(&m_star, "star", "OSU_SKIN_STAR");

	randomizeFilePath();
	checkLoadImage(&m_rankingPanel, "ranking-panel", "OSU_SKIN_RANKING_PANEL");
	checkLoadImage(&m_rankingGraph, "ranking-graph", "OSU_SKIN_RANKING_GRAPH");
	checkLoadImage(&m_rankingTitle, "ranking-title", "OSU_SKIN_RANKING_TITLE");
	checkLoadImage(&m_rankingMaxCombo, "ranking-maxcombo", "OSU_SKIN_RANKING_MAXCOMBO");
	checkLoadImage(&m_rankingAccuracy, "ranking-accuracy", "OSU_SKIN_RANKING_ACCURACY");

	checkLoadImage(&m_rankingA, "ranking-A", "OSU_SKIN_RANKING_A");
	checkLoadImage(&m_rankingB, "ranking-B", "OSU_SKIN_RANKING_B");
	checkLoadImage(&m_rankingC, "ranking-C", "OSU_SKIN_RANKING_C");
	checkLoadImage(&m_rankingD, "ranking-D", "OSU_SKIN_RANKING_D");
	checkLoadImage(&m_rankingS, "ranking-S", "OSU_SKIN_RANKING_S");
	checkLoadImage(&m_rankingSH, "ranking-SH", "OSU_SKIN_RANKING_SH");
	checkLoadImage(&m_rankingX, "ranking-X", "OSU_SKIN_RANKING_X");
	checkLoadImage(&m_rankingXH, "ranking-XH", "OSU_SKIN_RANKING_XH");

	m_rankingAsmall = createOsuSkinImage("ranking-A-small", Vector2(34, 38), 128);
	m_rankingBsmall = createOsuSkinImage("ranking-B-small", Vector2(33, 38), 128);
	m_rankingCsmall = createOsuSkinImage("ranking-C-small", Vector2(30, 38), 128);
	m_rankingDsmall = createOsuSkinImage("ranking-D-small", Vector2(33, 38), 128);
	m_rankingSsmall = createOsuSkinImage("ranking-S-small", Vector2(31, 38), 128);
	m_rankingSHsmall = createOsuSkinImage("ranking-SH-small", Vector2(31, 38), 128);
	m_rankingXsmall = createOsuSkinImage("ranking-X-small", Vector2(34, 40), 128);
	m_rankingXHsmall = createOsuSkinImage("ranking-XH-small", Vector2(34, 41), 128);

	m_rankingPerfect = createOsuSkinImage("ranking-perfect", Vector2(478, 150), 128);

	randomizeFilePath();
	checkLoadImage(&m_beatmapImportSpinner, "beatmapimport-spinner", "OSU_SKIN_BEATMAP_IMPORT_SPINNER");
	{
		// loading spinner load was here, moved up to improve async usability
	}
	checkLoadImage(&m_circleEmpty, "circle-empty", "OSU_SKIN_CIRCLE_EMPTY");
	checkLoadImage(&m_circleFull, "circle-full", "OSU_SKIN_CIRCLE_FULL");
	randomizeFilePath();
	checkLoadImage(&m_seekTriangle, "seektriangle", "OSU_SKIN_SEEKTRIANGLE");
	randomizeFilePath();
	checkLoadImage(&m_userIcon, "user-icon", "OSU_SKIN_USER_ICON");
	randomizeFilePath();
	checkLoadImage(&m_backgroundCube, "backgroundcube", "OSU_SKIN_FPOSU_BACKGROUNDCUBE", false, "png", true); // force mipmaps
	randomizeFilePath();
	checkLoadImage(&m_menuBackground, "menu-background", "OSU_SKIN_MENU_BACKGROUND", false, "jpg");
	randomizeFilePath();
	checkLoadImage(&m_skybox, "skybox", "OSU_SKIN_FPOSU_3D_SKYBOX");

	// sounds

	// samples
	checkLoadSound(&m_normalHitNormal, "normal-hitnormal", "OSU_SKIN_NORMALHITNORMAL_SND", true, true, false, 0.8f);
	checkLoadSound(&m_normalHitWhistle, "normal-hitwhistle", "OSU_SKIN_NORMALHITWHISTLE_SND", true, true, false, 0.85f);
	checkLoadSound(&m_normalHitFinish, "normal-hitfinish", "OSU_SKIN_NORMALHITFINISH_SND", true, true);
	checkLoadSound(&m_normalHitClap, "normal-hitclap", "OSU_SKIN_NORMALHITCLAP_SND", true, true, false, 0.85f);

	checkLoadSound(&m_normalSliderTick, "normal-slidertick", "OSU_SKIN_NORMALSLIDERTICK_SND", true, true);
	checkLoadSound(&m_normalSliderSlide, "normal-sliderslide", "OSU_SKIN_NORMALSLIDERSLIDE_SND", false, true, true);
	checkLoadSound(&m_normalSliderWhistle, "normal-sliderwhistle", "OSU_SKIN_NORMALSLIDERWHISTLE_SND", true, true);

	checkLoadSound(&m_softHitNormal, "soft-hitnormal", "OSU_SKIN_SOFTHITNORMAL_SND", true, true, false, 0.8f);
	checkLoadSound(&m_softHitWhistle, "soft-hitwhistle", "OSU_SKIN_SOFTHITWHISTLE_SND", true, true, false, 0.85f);
	checkLoadSound(&m_softHitFinish, "soft-hitfinish", "OSU_SKIN_SOFTHITFINISH_SND", true, true);
	checkLoadSound(&m_softHitClap, "soft-hitclap", "OSU_SKIN_SOFTHITCLAP_SND", true, true, false, 0.85f);

	checkLoadSound(&m_softSliderTick, "soft-slidertick", "OSU_SKIN_SOFTSLIDERTICK_SND", true, true);
	checkLoadSound(&m_softSliderSlide, "soft-sliderslide", "OSU_SKIN_SOFTSLIDERSLIDE_SND", false, true, true);
	checkLoadSound(&m_softSliderWhistle, "soft-sliderwhistle", "OSU_SKIN_SOFTSLIDERWHISTLE_SND", true, true);

	checkLoadSound(&m_drumHitNormal, "drum-hitnormal", "OSU_SKIN_DRUMHITNORMAL_SND", true, true, false, 0.8f);
	checkLoadSound(&m_drumHitWhistle, "drum-hitwhistle", "OSU_SKIN_DRUMHITWHISTLE_SND", true, true, false, 0.85f);
	checkLoadSound(&m_drumHitFinish, "drum-hitfinish", "OSU_SKIN_DRUMHITFINISH_SND", true, true);
	checkLoadSound(&m_drumHitClap, "drum-hitclap", "OSU_SKIN_DRUMHITCLAP_SND", true, true, false, 0.85f);

	checkLoadSound(&m_drumSliderTick, "drum-slidertick", "OSU_SKIN_DRUMSLIDERTICK_SND", true, true);
	checkLoadSound(&m_drumSliderSlide, "drum-sliderslide", "OSU_SKIN_DRUMSLIDERSLIDE_SND", false, true, true);
	checkLoadSound(&m_drumSliderWhistle, "drum-sliderwhistle", "OSU_SKIN_DRUMSLIDERWHISTLE_SND", true, true);

	checkLoadSound(&m_spinnerBonus, "spinnerbonus", "OSU_SKIN_SPINNERBONUS_SND", true, true);
	checkLoadSound(&m_spinnerSpinSound, "spinnerspin", "OSU_SKIN_SPINNERSPIN_SND", false, true, true);

	// others
	checkLoadSound(&m_combobreak, "combobreak", "OSU_SKIN_COMBOBREAK_SND", true);
	checkLoadSound(&m_failsound, "failsound", "OSU_SKIN_FAILSOUND_SND");
	checkLoadSound(&m_applause, "applause", "OSU_SKIN_APPLAUSE_SND");
	checkLoadSound(&m_menuHit, "menuhit", "OSU_SKIN_MENUHIT_SND", true);
	checkLoadSound(&m_menuClick, "menuclick", "OSU_SKIN_MENUCLICK_SND", true);
	checkLoadSound(&m_checkOn, "check-on", "OSU_SKIN_CHECKON_SND", true);
	checkLoadSound(&m_checkOff, "check-off", "OSU_SKIN_CHECKOFF_SND", true);
	checkLoadSound(&m_shutter, "shutter", "OSU_SKIN_SHUTTER_SND", true);
	checkLoadSound(&m_sectionPassSound, "sectionpass", "OSU_SKIN_SECTIONPASS_SND", true);
	checkLoadSound(&m_sectionFailSound, "sectionfail", "OSU_SKIN_SECTIONFAIL_SND", true);

	// scaling
	// HACKHACK: this is pure cancer
	if (m_cursor != NULL && m_cursor->getFilePath().find("@2x") != -1)
		m_bCursor2x = true;
	if (m_cursorTrail != NULL && m_cursorTrail->getFilePath().find("@2x") != -1)
		m_bCursorTrail2x = true;
	if (m_cursorRipple != NULL && m_cursorRipple->getFilePath().find("@2x") != -1)
		m_bCursorRipple2x = true;
	if (m_approachCircle != NULL && m_approachCircle->getFilePath().find("@2x") != -1)
		m_bApproachCircle2x = true;
	if (m_reverseArrow != NULL && m_reverseArrow->getFilePath().find("@2x") != -1)
		m_bReverseArrow2x = true;
	if (m_hitCircle != NULL && m_hitCircle->getFilePath().find("@2x") != -1)
		m_bHitCircle2x = true;
	if (m_default0 != NULL && m_default0->getFilePath().find("@2x") != -1)
		m_bIsDefault02x = true;
	if (m_default1 != NULL && m_default1->getFilePath().find("@2x") != -1)
		m_bIsDefault12x = true;
	if (m_score0 != NULL && m_score0->getFilePath().find("@2x") != -1)
		m_bIsScore02x = true;
	if (m_combo0 != NULL && m_combo0->getFilePath().find("@2x") != -1)
		m_bIsCombo02x = true;
	if (m_spinnerApproachCircle != NULL && m_spinnerApproachCircle->getFilePath().find("@2x") != -1)
		m_bSpinnerApproachCircle2x = true;
	if (m_spinnerBottom != NULL && m_spinnerBottom->getFilePath().find("@2x") != -1)
		m_bSpinnerBottom2x = true;
	if (m_spinnerCircle != NULL && m_spinnerCircle->getFilePath().find("@2x") != -1)
		m_bSpinnerCircle2x = true;
	if (m_spinnerTop != NULL && m_spinnerTop->getFilePath().find("@2x") != -1)
		m_bSpinnerTop2x = true;
	if (m_spinnerMiddle != NULL && m_spinnerMiddle->getFilePath().find("@2x") != -1)
		m_bSpinnerMiddle2x = true;
	if (m_spinnerMiddle2 != NULL && m_spinnerMiddle2->getFilePath().find("@2x") != -1)
		m_bSpinnerMiddle22x = true;
	if (m_sliderScorePoint != NULL && m_sliderScorePoint->getFilePath().find("@2x") != -1)
		m_bSliderScorePoint2x = true;
	if (m_sliderStartCircle != NULL && m_sliderStartCircle->getFilePath().find("@2x") != -1)
		m_bSliderStartCircle2x = true;
	if (m_sliderEndCircle != NULL && m_sliderEndCircle->getFilePath().find("@2x") != -1)
		m_bSliderEndCircle2x = true;

	if (m_circularmetre != NULL && m_circularmetre->getFilePath().find("@2x") != -1)
		m_bCircularmetre2x = true;

	if (m_pauseContinue != NULL && m_pauseContinue->getFilePath().find("@2x") != -1)
		m_bPauseContinue2x = true;

	if (m_menuButtonBackground != NULL && m_menuButtonBackground->getFilePath().find("@2x") != -1)
		m_bMenuButtonBackground2x = true;
	if (m_star != NULL && m_star->getFilePath().find("@2x") != -1)
		m_bStar2x = true;
	if (m_rankingPanel != NULL && m_rankingPanel->getFilePath().find("@2x") != -1)
		m_bRankingPanel2x = true;
	if (m_rankingMaxCombo != NULL && m_rankingMaxCombo->getFilePath().find("@2x") != -1)
		m_bRankingMaxCombo2x = true;
	if (m_rankingAccuracy != NULL && m_rankingAccuracy->getFilePath().find("@2x") != -1)
		m_bRankingAccuracy2x = true;
	if (m_rankingA != NULL && m_rankingA->getFilePath().find("@2x") != -1)
		m_bRankingA2x = true;
	if (m_rankingB != NULL && m_rankingB->getFilePath().find("@2x") != -1)
		m_bRankingB2x = true;
	if (m_rankingC != NULL && m_rankingC->getFilePath().find("@2x") != -1)
		m_bRankingC2x = true;
	if (m_rankingD != NULL && m_rankingD->getFilePath().find("@2x") != -1)
		m_bRankingD2x = true;
	if (m_rankingS != NULL && m_rankingS->getFilePath().find("@2x") != -1)
		m_bRankingS2x = true;
	if (m_rankingSH != NULL && m_rankingSH->getFilePath().find("@2x") != -1)
		m_bRankingSH2x = true;
	if (m_rankingX != NULL && m_rankingX->getFilePath().find("@2x") != -1)
		m_bRankingX2x = true;
	if (m_rankingXH != NULL && m_rankingXH->getFilePath().find("@2x") != -1)
		m_bRankingXH2x = true;

	// HACKHACK: all of the <>2 loads are temporary fixes until I fix the checkLoadImage() function logic

	// custom
	Image *defaultCursor = engine->getResourceManager()->getImage("OSU_SKIN_CURSOR_DEFAULT");
	Image *defaultCursor2 = m_cursor;
	if (defaultCursor != NULL)
		m_defaultCursor = defaultCursor;
	else if (defaultCursor2 != NULL)
		m_defaultCursor = defaultCursor2;

	Image *defaultButtonLeft = engine->getResourceManager()->getImage("OSU_SKIN_BUTTON_LEFT_DEFAULT");
	Image *defaultButtonLeft2 = m_buttonLeft;
	if (defaultButtonLeft != NULL)
		m_defaultButtonLeft = defaultButtonLeft;
	else if (defaultButtonLeft2 != NULL)
		m_defaultButtonLeft = defaultButtonLeft2;

	Image *defaultButtonMiddle = engine->getResourceManager()->getImage("OSU_SKIN_BUTTON_MIDDLE_DEFAULT");
	Image *defaultButtonMiddle2 = m_buttonMiddle;
	if (defaultButtonMiddle != NULL)
		m_defaultButtonMiddle = defaultButtonMiddle;
	else if (defaultButtonMiddle2 != NULL)
		m_defaultButtonMiddle = defaultButtonMiddle2;

	Image *defaultButtonRight = engine->getResourceManager()->getImage("OSU_SKIN_BUTTON_RIGHT_DEFAULT");
	Image *defaultButtonRight2 = m_buttonRight;
	if (defaultButtonRight != NULL)
		m_defaultButtonRight = defaultButtonRight;
	else if (defaultButtonRight2 != NULL)
		m_defaultButtonRight = defaultButtonRight2;

	// print some debug info
	debugLog("OsuSkin: Version %f\n", m_fVersion);
	debugLog("OsuSkin: HitCircleOverlap = %i\n", m_iHitCircleOverlap);

	// delayed error notifications due to resource loading potentially blocking engine time
	if (!parseSkinIni1Status && parseSkinIni2Status && m_osu_skin_ref->getString() != "default" && m_osu_skin_ref->getString() != "defaultvr")
		m_osu->getNotificationOverlay()->addNotification("Error: Couldn't load skin.ini!", 0xffff0000);
	else if (!parseSkinIni2Status)
		m_osu->getNotificationOverlay()->addNotification("Error: Couldn't load DEFAULT skin.ini!!!", 0xffff0000);

	// TODO: is this crashing some users?
	// HACKHACK: speed up initial game startup time by async loading the skin (if osu_skin_async 1 in underride)
	if (m_osu->getSkin() == NULL && osu_skin_async.getBool())
	{
		while (engine->getResourceManager()->isLoading())
		{
			engine->getResourceManager()->update();
			env->sleep(0);
		}
	}
}

void OsuSkin::loadBeatmapOverride(UString filepath)
{
	//debugLog("OsuSkin::loadBeatmapOverride( %s )\n", filepath.toUtf8());
	// TODO: beatmap skin support
}

void OsuSkin::reloadSounds()
{
	for (int i=0; i<m_sounds.size(); i++)
	{
		m_sounds[i]->reload();
	}
}

bool OsuSkin::parseSkinINI(UString filepath)
{
	File file(filepath);
	if (!file.canRead())
	{
		debugLog("OsuSkin Error: Couldn't load %s\n", filepath.toUtf8());
		return false;
	}

	int nonEmptyLineCounter = 0;
	int curBlock = 0; // NOTE: was -1, but osu incorrectly defaults to [General] and loads properties even before the actual section start (just for this first section though)
	while (file.canRead())
	{
		UString uCurLine = file.readLine();

		if (uCurLine == "")
			continue;

		const char *curLineChar = uCurLine.toUtf8();
		std::string curLine(curLineChar);

		nonEmptyLineCounter++;

		if (curLine.find("//") > 2) // ignore comments // TODO: this is incorrect, but it works well enough
		{
			if (curLine.find("[General]") != std::string::npos)
				curBlock = 0;
			else if (curLine.find("[Colours]") != std::string::npos || curLine.find("[Colors]") != std::string::npos)
				curBlock = 1;
			else if (curLine.find("[Fonts]") != std::string::npos)
				curBlock = 2;

			switch (curBlock)
			{
			case 0: // General
				{
					int val;
					float floatVal;
					char stringBuffer[1024];

					memset(stringBuffer, '\0', 1024);
					if (sscanf(curLineChar, " Version : %1023[^\n]", stringBuffer) == 1)
					{
						UString versionString = UString(stringBuffer);
						if (versionString.find("latest") != -1 || versionString.find("User") != -1)
							m_fVersion = 2.5f; // default to latest version available
						else
							m_fVersion = versionString.toFloat();
					}

					if (sscanf(curLineChar, " CursorRotate : %i \n", &val) == 1)
						m_bCursorRotate = val > 0 ? true : false;
					if (sscanf(curLineChar, " CursorCentre : %i \n", &val) == 1)
						m_bCursorCenter = val > 0 ? true : false;
					if (sscanf(curLineChar, " CursorExpand : %i \n", &val) == 1)
						m_bCursorExpand = val > 0 ? true : false;
					if (sscanf(curLineChar, " SliderBallFlip : %i \n", &val) == 1)
						m_bSliderBallFlip = val > 0 ? true : false;
					if (sscanf(curLineChar, " AllowSliderBallTint : %i \n", &val) == 1)
						m_bAllowSliderBallTint = val > 0 ? true : false;
					if (sscanf(curLineChar, " HitCircleOverlayAboveNumber : %i \n", &val) == 1)
						m_bHitCircleOverlayAboveNumber = val > 0 ? true : false;
					if (sscanf(curLineChar, " HitCircleOverlayAboveNumer : %i \n", &val) == 1)
						m_bHitCircleOverlayAboveNumber = val > 0 ? true : false;
					if (sscanf(curLineChar, " SliderStyle : %i \n", &val) == 1)
					{
						m_iSliderStyle = val;
						if (m_iSliderStyle != 1 && m_iSliderStyle != 2)
							m_iSliderStyle = 2;
					}
					if (sscanf(curLineChar, " AnimationFramerate : %f \n", &floatVal) == 1)
						m_fAnimationFramerate = floatVal < 0 ? 0.0f : floatVal;
				}
				break;
			case 1: // Colors
				{
					int comboNum;
					int r,g,b;

					if (sscanf(curLineChar, " Combo %i : %i , %i , %i \n", &comboNum, &r, &g, &b) == 4)
						m_comboColors.push_back(COLOR(255, r, g, b));
					if (sscanf(curLineChar, " SpinnerApproachCircle : %i , %i , %i \n", &r, &g, &b) == 3)
						m_spinnerApproachCircleColor = COLOR(255, r, g, b);
					if (sscanf(curLineChar, " SliderBorder: %i , %i , %i \n", &r, &g, &b) == 3)
						m_sliderBorderColor = COLOR(255, r, g, b);
					if (sscanf(curLineChar, " SliderTrackOverride : %i , %i , %i \n", &r, &g, &b) == 3)
					{
						m_sliderTrackOverride = COLOR(255, r, g, b);
						m_bSliderTrackOverride = true;
					}
					if (sscanf(curLineChar, " SliderBall : %i , %i , %i \n", &r, &g, &b) == 3)
						m_sliderBallColor = COLOR(255, r, g, b);
					if (sscanf(curLineChar, " SongSelectActiveText : %i , %i , %i \n", &r, &g, &b) == 3)
						m_songSelectActiveText = COLOR(255, r, g, b);
					if (sscanf(curLineChar, " SongSelectInactiveText : %i , %i , %i \n", &r, &g, &b) == 3)
						m_songSelectInactiveText = COLOR(255, r, g, b);
					if (sscanf(curLineChar, " InputOverlayText : %i , %i , %i \n", &r, &g, &b) == 3)
						m_inputOverlayText = COLOR(255, r, g, b);
				}
				break;
			case 2: // Fonts
				{
					int val;
					char stringBuffer[1024];

					memset(stringBuffer, '\0', 1024);
					if (sscanf(curLineChar, " ComboPrefix : %1023[^\n]", stringBuffer) == 1)
					{
						m_sComboPrefix = UString(stringBuffer);

						for (int i=0; i<m_sComboPrefix.length(); i++)
						{
							if (m_sComboPrefix[i] == L'\\')
							{
								m_sComboPrefix.erase(i, 1);
								m_sComboPrefix.insert(i, L'/');
							}
						}
					}
					if (sscanf(curLineChar, " ComboOverlap : %i \n", &val) == 1)
						m_iComboOverlap = val;

					if (sscanf(curLineChar, " ScorePrefix : %1023[^\n]", stringBuffer) == 1)
					{
						m_sScorePrefix = UString(stringBuffer);

						for (int i=0; i<m_sScorePrefix.length(); i++)
						{
							if (m_sScorePrefix[i] == L'\\')
							{
								m_sScorePrefix.erase(i, 1);
								m_sScorePrefix.insert(i, L'/');
							}
						}
					}
					if (sscanf(curLineChar, " ScoreOverlap : %i \n", &val) == 1)
						m_iScoreOverlap = val;

					if (sscanf(curLineChar, " HitCirclePrefix : %1023[^\n]", stringBuffer) == 1)
					{
						m_sHitCirclePrefix = UString(stringBuffer);

						for (int i=0; i<m_sHitCirclePrefix.length(); i++)
						{
							if (m_sHitCirclePrefix[i] == L'\\')
							{
								m_sHitCirclePrefix.erase(i, 1);
								m_sHitCirclePrefix.insert(i, L'/');
							}
						}
					}
					if (sscanf(curLineChar, " HitCircleOverlap : %i \n", &val) == 1)
						m_iHitCircleOverlap = val;
				}
				break;
			}
		}
	}

	if (nonEmptyLineCounter < 1)
		return false;

	return true;
}

void OsuSkin::onEffectVolumeChange(UString oldValue, UString newValue)
{
	float volume = newValue.toFloat();

	for (int i=0; i<m_sounds.size(); i++)
	{
		m_sounds[i]->setVolume(volume);
	}

	// restore sample volumes
	setSampleVolume(clamp<float>((float)m_iSampleVolume / 100.0f, 0.0f, 1.0f), true);
}

void OsuSkin::onIgnoreBeatmapSampleVolumeChange(UString oldValue, UString newValue)
{
	// restore sample volumes
	setSampleVolume(clamp<float>((float)m_iSampleVolume / 100.0f, 0.0f, 1.0f), true);
}

void OsuSkin::onExport(UString folderName)
{
	if (folderName.length() < 1)
	{
		m_osu->getNotificationOverlay()->addNotification("Usage: osu_skin_export MyExportedSkinName", 0xffffffff, false, 3.0f);
		return;
	}

	UString exportFolder = convar->getConVarByName("osu_folder")->getString();
	exportFolder.append(convar->getConVarByName("osu_folder_sub_skins")->getString());
	exportFolder.append(folderName);
	exportFolder.append("/");

	if (env->directoryExists(exportFolder))
	{
		m_osu->getNotificationOverlay()->addNotification("Error: Folder already exists. Use a different name.", 0xffffff00, false, 3.0f);
		return;
	}

	if (!env->createDirectory(exportFolder))
	{
		m_osu->getNotificationOverlay()->addNotification(UString::format("Error: Couldn't create folder.", exportFolder.toUtf8()), 0xffff0000, false, 3.0f);
		return;
	}

	if (!env->directoryExists(exportFolder))
	{
		m_osu->getNotificationOverlay()->addNotification(UString::format("Error: Folder does not exist.", exportFolder.toUtf8()), 0xffff0000, false, 3.0f);
		return;
	}

	struct FILE_TO_COPY
	{
		UString filePath;
		UString fileNameWithExtension;
	};

	std::vector<FILE_TO_COPY> filesToCopy;

	// skin.ini
	filesToCopy.push_back({m_sSkinIniFilePath, "skin.ini"});

	// checkLoadImage + checkLoadSound + createOsuSkinImage
	for (const UString &filepath : m_filepathsForExport)
	{
		if (filepath.length() < 3) continue;

		const int lastPathSeparatorIndex = filepath.findLast("/");
		if (lastPathSeparatorIndex > -1)
		{
			const UString fileNameWithExtension = filepath.substr(lastPathSeparatorIndex + 1);
			if (fileNameWithExtension.length() > 0)
				filesToCopy.push_back({filepath, fileNameWithExtension});
		}
	}

	for (const FILE_TO_COPY &fileToCopy : filesToCopy)
	{
		UString outputFilePath = exportFolder;
		outputFilePath.append(fileToCopy.fileNameWithExtension);

		debugLog("Copying \"%s\" to \"%s\"\n", fileToCopy.filePath.toUtf8(), outputFilePath.toUtf8());

		File inputFile(fileToCopy.filePath, File::TYPE::READ);
		File outputFile(outputFilePath, File::TYPE::WRITE);

		if (inputFile.canRead() && outputFile.canWrite())
			outputFile.write(inputFile.readFile(), inputFile.getFileSize());
		else
			debugLog("Error: Couldn't copy %s\n", fileToCopy.filePath.toUtf8());
	}

	debugLog("Done.\n");
	m_osu->getNotificationOverlay()->addNotification("Done.", 0xff00ff00, false, 2.0f);
}

void OsuSkin::setSampleSet(int sampleSet)
{
	if (m_iSampleSet == sampleSet) return;

	///debugLog("sample set = %i\n", sampleSet);
	m_iSampleSet = sampleSet;
}

void OsuSkin::setSampleVolume(float volume, bool force)
{
	if (osu_ignore_beatmap_sample_volume.getBool() && (int)(osu_volume_effects.getFloat() * 100.0f) == m_iSampleVolume) return;

	const float newSampleVolume = (!osu_ignore_beatmap_sample_volume.getBool() ? volume : 1.0f) * osu_volume_effects.getFloat();

	if (!force && m_iSampleVolume == (int)(newSampleVolume * 100.0f)) return;

	m_iSampleVolume = (int)(newSampleVolume * 100.0f);
	///debugLog("sample volume = %f\n", sampleVolume);
	for (int i=0; i<m_soundSamples.size(); i++)
	{
		m_soundSamples[i].sound->setVolume(newSampleVolume * m_soundSamples[i].hardcodedVolumeMultiplier);
	}
}

Color OsuSkin::getComboColorForCounter(int i, int offset)
{
	i += osu_skin_color_index_add.getInt();
	i = std::max(i, 0);

	if (m_beatmapComboColors.size() > 0 && !osu_ignore_beatmap_combo_colors.getBool())
		return m_beatmapComboColors[(i + offset) % m_beatmapComboColors.size()];
	else if (m_comboColors.size() > 0)
		return m_comboColors[i % m_comboColors.size()];
	else
		return COLOR(255, 0, 255, 0);
}

void OsuSkin::setBeatmapComboColors(std::vector<Color> colors)
{
	m_beatmapComboColors = colors;
}

void OsuSkin::playHitCircleSound(int sampleType, float pan)
{
	if (m_iSampleVolume <= 0 || (m_osu->getInstanceID() > 0 && m_osu->getInstanceID() != osu2_sound_source_id.getInt())) return;

	if (!osu_sound_panning.getBool() || (m_osu_mod_fposu_ref->getBool() && !osu_mod_fposu_sound_panning.getBool()) || (OsuGameRules::osu_mod_fps.getBool() && !osu_mod_fps_sound_panning.getBool()))
		pan = 0.0f;
	else
		pan *= osu_sound_panning_multiplier.getFloat();

	int actualSampleSet = m_iSampleSet;
	if (osu_skin_force_hitsound_sample_set.getInt() > 0)
		actualSampleSet = osu_skin_force_hitsound_sample_set.getInt();

	switch (actualSampleSet)
	{
	case 3:
		engine->getSound()->play(m_drumHitNormal, pan);

		if (sampleType & OSU_BITMASK_HITWHISTLE)
			engine->getSound()->play(m_drumHitWhistle, pan);
		if (sampleType & OSU_BITMASK_HITFINISH)
			engine->getSound()->play(m_drumHitFinish, pan);
		if (sampleType & OSU_BITMASK_HITCLAP)
			engine->getSound()->play(m_drumHitClap, pan);
		break;
	case 2:
		engine->getSound()->play(m_softHitNormal, pan);

		if (sampleType & OSU_BITMASK_HITWHISTLE)
			engine->getSound()->play(m_softHitWhistle, pan);
		if (sampleType & OSU_BITMASK_HITFINISH)
			engine->getSound()->play(m_softHitFinish, pan);
		if (sampleType & OSU_BITMASK_HITCLAP)
			engine->getSound()->play(m_softHitClap, pan);
		break;
	default:
		engine->getSound()->play(m_normalHitNormal, pan);

		if (sampleType & OSU_BITMASK_HITWHISTLE)
			engine->getSound()->play(m_normalHitWhistle, pan);
		if (sampleType & OSU_BITMASK_HITFINISH)
			engine->getSound()->play(m_normalHitFinish, pan);
		if (sampleType & OSU_BITMASK_HITCLAP)
			engine->getSound()->play(m_normalHitClap, pan);
		break;
	}
}

void OsuSkin::playSliderTickSound(float pan)
{
	if (m_iSampleVolume <= 0 || (m_osu->getInstanceID() > 0 && m_osu->getInstanceID() != osu2_sound_source_id.getInt())) return;

	if (!osu_sound_panning.getBool() || (m_osu_mod_fposu_ref->getBool() && !osu_mod_fposu_sound_panning.getBool()) || (OsuGameRules::osu_mod_fps.getBool() && !osu_mod_fps_sound_panning.getBool()))
		pan = 0.0f;
	else
		pan *= osu_sound_panning_multiplier.getFloat();

	switch (m_iSampleSet)
	{
	case 3:
		engine->getSound()->play(m_drumSliderTick, pan);
		break;
	case 2:
		engine->getSound()->play(m_softSliderTick, pan);
		break;
	default:
		engine->getSound()->play(m_normalSliderTick, pan);
		break;
	}
}

void OsuSkin::playSliderSlideSound(float pan)
{
	if ((m_osu->getInstanceID() > 0 && m_osu->getInstanceID() != osu2_sound_source_id.getInt())) return;

	if (!osu_sound_panning.getBool() || (m_osu_mod_fposu_ref->getBool() && !osu_mod_fposu_sound_panning.getBool()) || (OsuGameRules::osu_mod_fps.getBool() && !osu_mod_fps_sound_panning.getBool()))
		pan = 0.0f;
	else
		pan *= osu_sound_panning_multiplier.getFloat();

	switch (m_iSampleSet)
	{
	case 3:
		if (m_softSliderSlide->isPlaying())
			engine->getSound()->stop(m_softSliderSlide);
		if (m_normalSliderSlide->isPlaying())
			engine->getSound()->stop(m_normalSliderSlide);

		if (!m_drumSliderSlide->isPlaying())
			engine->getSound()->play(m_drumSliderSlide, pan);
		else
			m_drumSliderSlide->setPan(pan);
		break;
	case 2:
		if (m_drumSliderSlide->isPlaying())
			engine->getSound()->stop(m_drumSliderSlide);
		if (m_normalSliderSlide->isPlaying())
			engine->getSound()->stop(m_normalSliderSlide);

		if (!m_softSliderSlide->isPlaying())
			engine->getSound()->play(m_softSliderSlide, pan);
		else
			m_softSliderSlide->setPan(pan);
		break;
	default:
		if (m_softSliderSlide->isPlaying())
			engine->getSound()->stop(m_softSliderSlide);
		if (m_drumSliderSlide->isPlaying())
			engine->getSound()->stop(m_drumSliderSlide);

		if (!m_normalSliderSlide->isPlaying())
			engine->getSound()->play(m_normalSliderSlide, pan);
		else
			m_normalSliderSlide->setPan(pan);
		break;
	}
}

void OsuSkin::playSpinnerSpinSound()
{
	if ((m_osu->getInstanceID() > 0 && m_osu->getInstanceID() != osu2_sound_source_id.getInt())) return;

	if (!m_spinnerSpinSound->isPlaying())
		engine->getSound()->play(m_spinnerSpinSound);
}

void OsuSkin::playSpinnerBonusSound()
{
	if (m_iSampleVolume <= 0 || (m_osu->getInstanceID() > 0 && m_osu->getInstanceID() != osu2_sound_source_id.getInt())) return;

	engine->getSound()->play(m_spinnerBonus);
}

void OsuSkin::stopSliderSlideSound(int sampleSet)
{
	if ((sampleSet == -2 || sampleSet == 3) && m_drumSliderSlide != NULL && m_drumSliderSlide->isPlaying())
		engine->getSound()->stop(m_drumSliderSlide);
	if ((sampleSet == -2 || sampleSet == 2) && m_softSliderSlide != NULL && m_softSliderSlide->isPlaying())
		engine->getSound()->stop(m_softSliderSlide);
	if ((sampleSet == -2 || sampleSet == 1 || sampleSet == 0) && m_normalSliderSlide != NULL && m_normalSliderSlide->isPlaying())
		engine->getSound()->stop(m_normalSliderSlide);
}

void OsuSkin::stopSpinnerSpinSound()
{
	if (m_spinnerSpinSound != NULL && m_spinnerSpinSound->isPlaying())
		engine->getSound()->stop(m_spinnerSpinSound);
}

void OsuSkin::randomizeFilePath()
{
	if (m_bIsRandomElements && filepathsForRandomSkin.size() > 0)
		m_sFilePath = filepathsForRandomSkin[rand() % filepathsForRandomSkin.size()];
}

OsuSkinImage *OsuSkin::createOsuSkinImage(UString skinElementName, Vector2 baseSizeForScaling2x, float osuSize, bool ignoreDefaultSkin, UString animationSeparator)
{
	OsuSkinImage *skinImage = new OsuSkinImage(this, skinElementName, baseSizeForScaling2x, osuSize, animationSeparator, ignoreDefaultSkin);
	m_images.push_back(skinImage);

	const std::vector<UString> &filepathsForExport = skinImage->getFilepathsForExport();
	m_filepathsForExport.insert(m_filepathsForExport.end(), filepathsForExport.begin(), filepathsForExport.end());

	return skinImage;
}

void OsuSkin::checkLoadImage(Image **addressOfPointer, UString skinElementName, UString resourceName, bool ignoreDefaultSkin, UString fileExtension, bool forceLoadMipmaps, bool forceUseDefaultSkin)
{
	if (*addressOfPointer != m_missingTexture) return; // we are already loaded

	// NOTE: only the default skin is loaded with a resource name (it must never be unloaded by other instances), and it is NOT added to the resources vector

	UString defaultFilePath1 = UString(env->getOS() == Environment::OS::OS_HORIZON ? "romfs:/materials/" : "./materials/");
	defaultFilePath1.append(OSUSKIN_DEFAULT_SKIN_PATH);
	defaultFilePath1.append(skinElementName);
	defaultFilePath1.append("@2x.");
	defaultFilePath1.append(fileExtension);

	UString defaultFilePath2 = UString(env->getOS() == Environment::OS::OS_HORIZON ? "romfs:/materials/" : "./materials/");
	defaultFilePath2.append(OSUSKIN_DEFAULT_SKIN_PATH);
	defaultFilePath2.append(skinElementName);
	defaultFilePath2.append(".");
	defaultFilePath2.append(fileExtension);

	UString filepath1 = m_sFilePath;
	filepath1.append(skinElementName);
	filepath1.append("@2x.");
	filepath1.append(fileExtension);

	UString filepath2 = m_sFilePath;
	filepath2.append(skinElementName);
	filepath2.append(".");
	filepath2.append(fileExtension);

	const bool existsDefaultFilePath1 = env->fileExists(defaultFilePath1);
	const bool existsDefaultFilePath2 = env->fileExists(defaultFilePath2);
	const bool existsFilepath1 = (!forceUseDefaultSkin && env->fileExists(filepath1));
	const bool existsFilepath2 = (!forceUseDefaultSkin && env->fileExists(filepath2));

	// check if an @2x version of this image exists
	if (osu_skin_hd.getBool())
	{
		// load default skin

		if (!ignoreDefaultSkin || forceUseDefaultSkin)
		{
			if (existsDefaultFilePath1)
			{
				UString defaultResourceName = resourceName;
				defaultResourceName.append("_DEFAULT"); // so we don't load the default skin twice

				if (osu_skin_async.getBool())
					engine->getResourceManager()->requestNextLoadAsync();

				*addressOfPointer = engine->getResourceManager()->loadImageAbs(defaultFilePath1, defaultResourceName, osu_skin_mipmaps.getBool() || forceLoadMipmaps);
				///m_resources.push_back(*addressOfPointer); // DEBUG: also reload default skin
			}
			else // fallback to @1x
			{
				if (existsDefaultFilePath2)
				{
					UString defaultResourceName = resourceName;
					defaultResourceName.append("_DEFAULT"); // so we don't load the default skin twice

					if (osu_skin_async.getBool())
						engine->getResourceManager()->requestNextLoadAsync();

					*addressOfPointer = engine->getResourceManager()->loadImageAbs(defaultFilePath2, defaultResourceName, osu_skin_mipmaps.getBool() || forceLoadMipmaps);
					///m_resources.push_back(*addressOfPointer); // DEBUG: also reload default skin
				}
			}
		}

		// load user skin

		if (existsFilepath1 && !forceUseDefaultSkin)
		{
			if (osu_skin_async.getBool())
				engine->getResourceManager()->requestNextLoadAsync();

			*addressOfPointer = engine->getResourceManager()->loadImageAbs(filepath1, "", osu_skin_mipmaps.getBool() || forceLoadMipmaps);
			m_resources.push_back(*addressOfPointer);

			// export
			{
				if (existsFilepath1)
					m_filepathsForExport.push_back(filepath1);

				if (existsFilepath2)
					m_filepathsForExport.push_back(filepath2);

				if (!existsFilepath1 && !existsFilepath2)
				{
					if (existsDefaultFilePath1)
						m_filepathsForExport.push_back(defaultFilePath1);

					if (existsDefaultFilePath2)
						m_filepathsForExport.push_back(defaultFilePath2);
				}
			}

			return; // nothing more to do here
		}
	}

	// else load normal @1x version

	// load default skin

	if (!ignoreDefaultSkin || forceUseDefaultSkin)
	{
		if (existsDefaultFilePath2)
		{
			UString defaultResourceName = resourceName;
			defaultResourceName.append("_DEFAULT"); // so we don't load the default skin twice

			if (osu_skin_async.getBool())
				engine->getResourceManager()->requestNextLoadAsync();

			*addressOfPointer = engine->getResourceManager()->loadImageAbs(defaultFilePath2, defaultResourceName, osu_skin_mipmaps.getBool() || forceLoadMipmaps);
			///m_resources.push_back(*addressOfPointer); // DEBUG: also reload default skin
		}
	}

	// load user skin

	if (existsFilepath2 && !forceUseDefaultSkin)
	{
		if (osu_skin_async.getBool())
			engine->getResourceManager()->requestNextLoadAsync();

		*addressOfPointer = engine->getResourceManager()->loadImageAbs(filepath2, "", osu_skin_mipmaps.getBool() || forceLoadMipmaps);
		m_resources.push_back(*addressOfPointer);
	}

	// export
	{
		if (!forceUseDefaultSkin)
		{
			if (existsFilepath1)
				m_filepathsForExport.push_back(filepath1);

			if (existsFilepath2)
				m_filepathsForExport.push_back(filepath2);
		}

		if ((!existsFilepath1 && !existsFilepath2) || forceUseDefaultSkin)
		{
			if (existsDefaultFilePath1)
				m_filepathsForExport.push_back(defaultFilePath1);

			if (existsDefaultFilePath2)
				m_filepathsForExport.push_back(defaultFilePath2);
		}
	}
}

void OsuSkin::checkLoadSound(Sound **addressOfPointer, UString skinElementName, UString resourceName, bool isOverlayable, bool isSample, bool loop, float hardcodedVolumeMultiplier)
{
	if (*addressOfPointer != NULL) return; // we are already loaded

	// NOTE: only the default skin is loaded with a resource name (it must never be unloaded by other instances), and it is NOT added to the resources vector

	// random skin support
	randomizeFilePath();

	// load default skin

	UString defaultpath1 = UString(env->getOS() == Environment::OS::OS_HORIZON ? "romfs:/materials/" : "./materials/");
	{
		defaultpath1.append(OSUSKIN_DEFAULT_SKIN_PATH);
		defaultpath1.append(skinElementName);
		defaultpath1.append(".wav");
	}

	UString defaultpath2 = UString(env->getOS() == Environment::OS::OS_HORIZON ? "romfs:/materials/" : "./materials/");
	{
		defaultpath2.append(OSUSKIN_DEFAULT_SKIN_PATH);
		defaultpath2.append(skinElementName);
		defaultpath2.append(".mp3");
	}

	UString defaultpath3 = UString(env->getOS() == Environment::OS::OS_HORIZON ? "romfs:/materials/" : "./materials/");
	{
		defaultpath3.append(OSUSKIN_DEFAULT_SKIN_PATH);
		defaultpath3.append(skinElementName);
		defaultpath3.append(".ogg");
	}

	UString defaultResourceName = resourceName;
	defaultResourceName.append("_DEFAULT");
	if (env->fileExists(defaultpath1))
	{
		if (osu_skin_async.getBool())
			engine->getResourceManager()->requestNextLoadAsync();

		*addressOfPointer = engine->getResourceManager()->loadSoundAbs(defaultpath1, defaultResourceName, false, false, loop);
	}
	else if (env->fileExists(defaultpath2))
	{
		if (osu_skin_async.getBool())
			engine->getResourceManager()->requestNextLoadAsync();

		*addressOfPointer = engine->getResourceManager()->loadSoundAbs(defaultpath2, defaultResourceName, false, false, loop);
	}
	else if (env->fileExists(defaultpath3))
	{
		if (osu_skin_async.getBool())
			engine->getResourceManager()->requestNextLoadAsync();

		*addressOfPointer = engine->getResourceManager()->loadSoundAbs(defaultpath3, defaultResourceName, false, false, loop);
	}

	// load user skin

	bool isDefaultSkin = true;
	if (!isSample || osu_skin_use_skin_hitsounds.getBool())
	{
		// check if mp3 or wav exist
		UString filepath1 = m_sFilePath;
		filepath1.append(skinElementName);
		filepath1.append(".wav");

		UString filepath2 = m_sFilePath;
		filepath2.append(skinElementName);
		filepath2.append(".mp3");

		UString filepath3 = m_sFilePath;
		filepath3.append(skinElementName);
		filepath3.append(".ogg");

		// load it
		if (env->fileExists(filepath1))
		{
			if (osu_skin_async.getBool())
				engine->getResourceManager()->requestNextLoadAsync();

			*addressOfPointer = engine->getResourceManager()->loadSoundAbs(filepath1, "", false, false, loop);
			isDefaultSkin = false;
		}
		else if (env->fileExists(filepath2))
		{
			if (osu_skin_async.getBool())
				engine->getResourceManager()->requestNextLoadAsync();

			*addressOfPointer = engine->getResourceManager()->loadSoundAbs(filepath2, "", false, false, loop);
			isDefaultSkin = false;
		}
		else if (env->fileExists(filepath3))
		{
			if (osu_skin_async.getBool())
				engine->getResourceManager()->requestNextLoadAsync();

			*addressOfPointer = engine->getResourceManager()->loadSoundAbs(filepath3, "", false, false, loop);
			isDefaultSkin = false;
		}
	}

	// force reload default skin sound anyway if the custom skin does not include it (e.g. audio device change)
	if (isDefaultSkin && *addressOfPointer != NULL)
		(*addressOfPointer)->reload();

	if ((*addressOfPointer) != NULL)
	{
		if (isOverlayable)
			(*addressOfPointer)->setOverlayable(true);

		if (!isDefaultSkin)
			m_resources.push_back(*addressOfPointer);

		m_sounds.push_back(*addressOfPointer);

		if (isSample)
		{
			SOUND_SAMPLE sample;
			sample.sound = *addressOfPointer;
			sample.hardcodedVolumeMultiplier = (hardcodedVolumeMultiplier >= 0.0f ? hardcodedVolumeMultiplier : 1.0f);
			m_soundSamples.push_back(sample);
		}

		// export
		m_filepathsForExport.push_back((*addressOfPointer)->getFilePath());
	}
	else
		debugLog("OsuSkin Warning: NULL sound %s!\n", skinElementName.toUtf8());
}

bool OsuSkin::compareFilenameWithSkinElementName(UString filename, UString skinElementName)
{
	if (filename.length() == 0 || skinElementName.length() == 0) return false;

	return filename.substr(0, filename.findLast(".")) == skinElementName;
}

