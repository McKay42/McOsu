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
#include "ConVar.h"

#include <string.h>

#include "Osu.h"
#include "OsuNotificationOverlay.h"

#define OSUSKIN_DEFAULT_SKIN_PATH "default/"

#define OSU_BITMASK_HITWHISTLE 0x2
#define OSU_BITMASK_HITFINISH 0x4
#define OSU_BITMASK_HITCLAP 0x8

void DUMMY_OSU_VOLUME_EFFECTS_ARGS(UString oldValue, UString newValue) {;}

ConVar osu_volume_effects("osu_volume_effects", 1.0f, DUMMY_OSU_VOLUME_EFFECTS_ARGS);
ConVar osu_skin_hd("osu_skin_hd", true);
ConVar osu_skin_load_async("osu_skin_load_async", false, "VERY experimental, changing skins too quickly will crash the engine (due to known behaviour)");

ConVar osu_ignore_beatmap_combo_colors("osu_ignore_beatmap_combo_colors", false);
ConVar osu_ignore_beatmap_sample_volume("osu_ignore_beatmap_sample_volume", false);

Image *OsuSkin::m_missingTexture = NULL;
ConVar *OsuSkin::m_osu_skin_ref = NULL;

OsuSkin::OsuSkin(Osu *osu, UString filepath)
{
	m_osu = osu;
	m_sFilePath = filepath;

	// convar refs
	if (m_osu_skin_ref == NULL)
		m_osu_skin_ref = convar->getConVarByName("osu_skin");

	if (m_missingTexture == NULL)
		m_missingTexture = engine->getResourceManager()->getImage("MISSING_TEXTURE");

	// vars
	m_hitCircle = m_missingTexture;
	m_hitCircleOverlay = m_missingTexture;
	m_approachCircle = m_missingTexture;
	m_reverseArrow = m_missingTexture;
	m_followPoint = m_missingTexture;

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

	m_playSkip = m_missingTexture;
	m_playWarningArrow = m_missingTexture;
	m_circularmetre = m_missingTexture;

	m_hit0 = m_missingTexture;
	m_hit50 = m_missingTexture;
	m_hit100 = m_missingTexture;

	m_sliderGradient = m_missingTexture;
	m_sliderGradientTrack = m_missingTexture;
	m_sliderGradientBody = m_missingTexture;
	m_sliderb = m_missingTexture;
	m_sliderFollowCircle = m_missingTexture;
	m_sliderScorePoint = m_missingTexture;
	m_sliderStartCircle = m_missingTexture;
	m_sliderEndCircle = m_missingTexture;

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

	m_selectionModEasy = m_missingTexture;
	m_selectionModNoFail = m_missingTexture;
	m_selectionModHalfTime = m_missingTexture;
	m_selectionModHardRock = m_missingTexture;
	m_selectionModSuddenDeath = m_missingTexture;
	m_selectionModPerfect = m_missingTexture;
	m_selectionModDoubleTime = m_missingTexture;
	m_selectionModNightCore = m_missingTexture;
	m_selectionModHidden = m_missingTexture;
	m_selectionModFlashlight = m_missingTexture;
	m_selectionModRelax = m_missingTexture;
	m_selectionModAutopilot = m_missingTexture;
	m_selectionModSpunOut = m_missingTexture;
	m_selectionModAutoplay = m_missingTexture;
	m_selectionModNightmare = m_missingTexture;
	m_selectionModTarget = m_missingTexture;

	m_pauseContinue = m_missingTexture;
	m_pauseRetry = m_missingTexture;
	m_pauseBack = m_missingTexture;
	m_unpause = m_missingTexture;

	m_buttonLeft = m_missingTexture;
	m_buttonMiddle = m_missingTexture;
	m_buttonRight = m_missingTexture;
	m_defaultButtonLeft = m_missingTexture;
	m_defaultButtonMiddle = m_missingTexture;
	m_defaultButtonRight = m_missingTexture;
	m_menuBack = m_missingTexture;
	m_selectionMods = m_missingTexture;
	m_selectionModsOver = m_missingTexture;
	m_selectionRandom = m_missingTexture;
	m_selectionRandomOver = m_missingTexture;
	m_selectionOptions = m_missingTexture;
	m_selectionOptionsOver = m_missingTexture;

	m_songSelectTop = m_missingTexture;
	m_songSelectBottom = m_missingTexture;
	m_menuButtonBackground = m_missingTexture;

	m_beatmapImportSpinner = m_missingTexture;
	m_circleEmpty = m_missingTexture;
	m_circleFull = m_missingTexture;

	m_normalHitNormal = NULL;
	m_normalHitWhistle = NULL;
	m_normalHitFinish = NULL;
	m_normalHitClap = NULL;
	m_normalSliderTick = NULL;
	m_softHitNormal = NULL;
	m_softHitWhistle = NULL;
	m_softHitFinish = NULL;
	m_softHitClap = NULL;
	m_softSliderTick = NULL;
	m_drumHitNormal = NULL;
	m_drumHitWhistle = NULL;
	m_drumHitFinish = NULL;
	m_drumHitClap = NULL;
	m_drumSliderTick = NULL;
	m_spinnerBonus = NULL;
	m_spinnerSpinSound = NULL;

	m_combobreak = NULL;
	m_applause = NULL;
	m_menuHit = NULL;
	m_menuClick = NULL;
	m_checkOn = NULL;
	m_checkOff = NULL;

	m_spinnerApproachCircleColor = 0xffffffff;
	m_sliderBorderColor = 0xffffffff;
	m_sliderBallColor = 0xff02aaff;

	m_songSelectActiveText = 0xff000000;
	m_songSelectInactiveText = 0xffffffff;

	// scaling
	m_bCursor2x = false;
	m_bApproachCircle2x = false;
	m_bReverseArrow2x = false;
	m_bFollowPoint2x = false;
	m_bHitCircle2x = false;
	m_bHitCircleOverlay2x = false;
	m_bIsDefault12x = false;
	m_bHit02x = false;
	m_bHit502x = false;
	m_bHit1002x = false;
	m_bSpinnerBottom2x= false;
	m_bSpinnerCircle2x= false;
	m_bSpinnerTop2x= false;
	m_bSpinnerMiddle2x= false;
	m_bSpinnerMiddle22x= false;
	m_bSliderScorePoint2x = false;

	m_bCircularmetre2x = false;
	m_bPlaySkip2x = false;

	m_bPauseContinue2x = false;

	m_bMenuButtonBackground2x = false;

	// skin.ini
	m_fVersion = 1;
	m_bCursorCenter = true;
	m_bCursorRotate = true;
	m_bCursorExpand = true;

	m_bSliderBallFlip = true;
	m_bAllowSliderBallTint = false;
	m_iSliderStyle = 2;
	m_bHitCircleOverlayAboveNumber = true;
	m_bSliderTrackOverride = false;

	m_iHitCircleOverlap = 0;

	// custom
	m_iSampleSet = 1;

	load();

	// convar callbacks
	osu_volume_effects.setCallback( MakeDelegate(this, &OsuSkin::onEffectVolumeChange) );

	// force effect volume change
	onEffectVolumeChange("", UString::format("%f", osu_volume_effects.getFloat()));
}

OsuSkin::~OsuSkin()
{
	for (int i=0; i<m_resources.size(); i++)
	{
		if (m_resources[i] != (Resource*)m_missingTexture)
			engine->getResourceManager()->destroyResource(m_resources[i]);
	}

	m_sounds.clear();
}

void OsuSkin::load()
{
	// skin ini
	UString skinIniFilePath = m_sFilePath;
	UString defaultSkinIniFilePath = UString("./materials/");
	defaultSkinIniFilePath.append(OSUSKIN_DEFAULT_SKIN_PATH);
	defaultSkinIniFilePath.append("skin.ini");
	skinIniFilePath.append("skin.ini");
	bool parseSkinIni1Status = true;
	bool parseSkinIni2Status = true;
	if (!parseSkinINI(skinIniFilePath))
	{
		parseSkinIni1Status = false;
		parseSkinIni2Status = parseSkinINI(defaultSkinIniFilePath);
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
	checkLoadImage(&m_hitCircle, "hitcircle", "OSU_SKIN_HITCIRCLE");
	checkLoadImage(&m_hitCircleOverlay, "hitcircleoverlay", "OSU_SKIN_HITCIRCLEOVERLAY");

	checkLoadImage(&m_approachCircle, "approachcircle", "OSU_SKIN_APPROACHCIRCLE");
	checkLoadImage(&m_reverseArrow, "reversearrow", "OSU_SKIN_REVERSEARROW");

	checkLoadImage(&m_followPoint, "followpoint-3", "OSU_SKIN_FOLLOWPOINT"); checkLoadImage(&m_followPoint, "followpoint", "OSU_SKIN_FOLLOWPOINT");

	checkLoadImage(&m_default0, "default-0", "OSU_SKIN_DEFAULT0");
	checkLoadImage(&m_default1, "default-1", "OSU_SKIN_DEFAULT1");
	checkLoadImage(&m_default2, "default-2", "OSU_SKIN_DEFAULT2");
	checkLoadImage(&m_default3, "default-3", "OSU_SKIN_DEFAULT3");
	checkLoadImage(&m_default4, "default-4", "OSU_SKIN_DEFAULT4");
	checkLoadImage(&m_default5, "default-5", "OSU_SKIN_DEFAULT5");
	checkLoadImage(&m_default6, "default-6", "OSU_SKIN_DEFAULT6");
	checkLoadImage(&m_default7, "default-7", "OSU_SKIN_DEFAULT7");
	checkLoadImage(&m_default8, "default-8", "OSU_SKIN_DEFAULT8");
	checkLoadImage(&m_default9, "default-9", "OSU_SKIN_DEFAULT9");

	checkLoadImage(&m_score0, "score-0", "OSU_SKIN_SCORE0");
	checkLoadImage(&m_score1, "score-1", "OSU_SKIN_SCORE1");
	checkLoadImage(&m_score2, "score-2", "OSU_SKIN_SCORE2");
	checkLoadImage(&m_score3, "score-3", "OSU_SKIN_SCORE3");
	checkLoadImage(&m_score4, "score-4", "OSU_SKIN_SCORE4");
	checkLoadImage(&m_score5, "score-5", "OSU_SKIN_SCORE5");
	checkLoadImage(&m_score6, "score-6", "OSU_SKIN_SCORE6");
	checkLoadImage(&m_score7, "score-7", "OSU_SKIN_SCORE7");
	checkLoadImage(&m_score8, "score-8", "OSU_SKIN_SCORE8");
	checkLoadImage(&m_score9, "score-9", "OSU_SKIN_SCORE9");

	checkLoadImage(&m_scoreX, "score-x", "OSU_SKIN_SCOREX");
	checkLoadImage(&m_scorePercent, "score-percent", "OSU_SKIN_SCOREPERCENT");
	checkLoadImage(&m_scoreDot, "score-dot", "OSU_SKIN_SCOREDOT");

	checkLoadImage(&m_playSkip, "play-skip", "OSU_SKIN_PLAYSKIP");
	checkLoadImage(&m_playWarningArrow, "play-warningarrow", "OSU_SKIN_PLAYWARNINGARROW");
	checkLoadImage(&m_circularmetre, "circularmetre", "OSU_SKIN_CIRCULARMETRE");

	checkLoadImage(&m_hit0, "hit0", "OSU_SKIN_HIT0");
	checkLoadImage(&m_hit50, "hit50", "OSU_SKIN_HIT50");
	checkLoadImage(&m_hit100, "hit100", "OSU_SKIN_HIT100");

	checkLoadImage(&m_sliderGradient, "slidergradient", "OSU_SKIN_SLIDERGRADIENT");
	checkLoadImage(&m_sliderGradientTrack, "slidergradienttrack2", "OSU_SKIN_SLIDERGRADIENTTRACK");
	checkLoadImage(&m_sliderGradientBody, "slidergradientbody", "OSU_SKIN_SLIDERGRADIENTBODY");
	checkLoadImage(&m_sliderb, "sliderb", "OSU_SKIN_SLIDERB"); checkLoadImage(&m_sliderb, "sliderb0", "OSU_SKIN_SLIDERB");
	checkLoadImage(&m_sliderScorePoint, "sliderscorepoint", "OSU_SKIN_SLIDERSCOREPOINT");
	checkLoadImage(&m_sliderFollowCircle, "sliderfollowcircle", "OSU_SKIN_SLIDERFOLLOWCIRCLE");
	checkLoadImage(&m_sliderStartCircle, "sliderstartcircle", "OSU_SKIN_SLIDERSTARTCIRCLE");
	checkLoadImage(&m_sliderEndCircle, "sliderendcircle", "OSU_SKIN_SLIDERENDCIRCLE");

	checkLoadImage(&m_spinnerBackground, "spinner-background", "OSU_SKIN_SPINNERBACKGROUND");
	checkLoadImage(&m_spinnerCircle, "spinner-circle", "OSU_SKIN_SPINNERCIRCLE");
	checkLoadImage(&m_spinnerApproachCircle, "spinner-approachcircle", "OSU_SKIN_SPINNERAPPROACHCIRCLE");
	checkLoadImage(&m_spinnerBottom, "spinner-bottom", "OSU_SKIN_SPINNERBOTTOM");
	checkLoadImage(&m_spinnerMiddle, "spinner-middle", "OSU_SKIN_SPINNERMIDDLE");
	checkLoadImage(&m_spinnerMiddle2, "spinner-middle2", "OSU_SKIN_SPINNERMIDDLE2");
	checkLoadImage(&m_spinnerTop, "spinner-top", "OSU_SKIN_SPINNERTOP");
	checkLoadImage(&m_spinnerSpin, "spinner-spin", "OSU_SKIN_SPINNERSPIN");
	checkLoadImage(&m_spinnerClear, "spinner-clear", "OSU_SKIN_SPINNERCLEAR");

	checkLoadImage(&m_cursor, "cursor", "OSU_SKIN_CURSOR");
	checkLoadImage(&m_cursorMiddle, "cursormiddle", "OSU_SKIN_CURSORMIDDLE", true);
	checkLoadImage(&m_cursorTrail, "cursortrail", "OSU_SKIN_CURSORTRAIL");

	// special case: if fallback to default cursor, do load cursorMiddle
	if (m_cursor == engine->getResourceManager()->getImage("OSU_SKIN_CURSOR_DEFAULT"))
		checkLoadImage(&m_cursorMiddle, "cursormiddle", "OSU_SKIN_CURSORMIDDLE");

	checkLoadImage(&m_selectionModEasy, "selection-mod-easy", "OSU_SKIN_SELECTION_MOD_EASY");
	checkLoadImage(&m_selectionModNoFail, "selection-mod-nofail", "OSU_SKIN_SELECTION_MOD_NOFAIL");
	checkLoadImage(&m_selectionModHalfTime, "selection-mod-halftime", "OSU_SKIN_SELECTION_MOD_HALFTIME");
	checkLoadImage(&m_selectionModHardRock, "selection-mod-hardrock", "OSU_SKIN_SELECTION_MOD_HARDROCK");
	checkLoadImage(&m_selectionModSuddenDeath, "selection-mod-suddendeath", "OSU_SKIN_SELECTION_MOD_SUDDENDEATH");
	checkLoadImage(&m_selectionModPerfect, "selection-mod-perfect", "OSU_SKIN_SELECTION_MOD_PERFECT");
	checkLoadImage(&m_selectionModDoubleTime, "selection-mod-doubletime", "OSU_SKIN_SELECTION_MOD_DOUBLETIME");
	checkLoadImage(&m_selectionModNightCore, "selection-mod-nightcore", "OSU_SKIN_SELECTION_MOD_NIGHTCORE");
	checkLoadImage(&m_selectionModHidden, "selection-mod-hidden", "OSU_SKIN_SELECTION_MOD_HIDDEN");
	checkLoadImage(&m_selectionModFlashlight, "selection-mod-flashlight", "OSU_SKIN_SELECTION_MOD_FLASHLIGHT");
	checkLoadImage(&m_selectionModRelax, "selection-mod-relax", "OSU_SKIN_SELECTION_MOD_RELAX");
	checkLoadImage(&m_selectionModAutopilot, "selection-mod-relax2", "OSU_SKIN_SELECTION_MOD_RELAX2");
	checkLoadImage(&m_selectionModSpunOut, "selection-mod-spunout", "OSU_SKIN_SELECTION_MOD_SPUNOUT");
	checkLoadImage(&m_selectionModAutoplay, "selection-mod-autoplay", "OSU_SKIN_SELECTION_MOD_AUTOPLAY");
	checkLoadImage(&m_selectionModNightmare, "selection-mod-nightmare", "OSU_SKIN_SELECTION_MOD_NIGHTMARE");
	checkLoadImage(&m_selectionModTarget, "selection-mod-target", "OSU_SKIN_SELECTION_MOD_TARGET");

	checkLoadImage(&m_pauseContinue, "pause-continue", "OSU_SKIN_PAUSE_CONTINUE");
	checkLoadImage(&m_pauseRetry, "pause-retry", "OSU_SKIN_PAUSE_RETRY");
	checkLoadImage(&m_pauseBack, "pause-back", "OSU_SKIN_PAUSE_BACK");
	checkLoadImage(&m_unpause, "unpause", "OSU_SKIN_UNPAUSE");

	checkLoadImage(&m_buttonLeft, "button-left", "OSU_SKIN_BUTTON_LEFT");
	checkLoadImage(&m_buttonMiddle, "button-middle", "OSU_SKIN_BUTTON_MIDDLE");
	checkLoadImage(&m_buttonRight, "button-right", "OSU_SKIN_BUTTON_RIGHT");
	checkLoadImage(&m_menuBack, "menu-back-0", "OSU_SKIN_MENU_BACK"); checkLoadImage(&m_menuBack, "menu-back", "OSU_SKIN_MENU_BACK");
	checkLoadImage(&m_selectionMods, "selection-mods", "OSU_SKIN_SELECTION_MODS");
	checkLoadImage(&m_selectionModsOver, "selection-mods-over", "OSU_SKIN_SELECTION_MODS_OVER");
	checkLoadImage(&m_selectionRandom, "selection-random", "OSU_SKIN_SELECTION_RANDOM");
	checkLoadImage(&m_selectionRandomOver, "selection-random-over", "OSU_SKIN_SELECTION_RANDOM_OVER");
	checkLoadImage(&m_selectionOptions, "selection-options", "OSU_SKIN_SELECTION_OPTIONS");
	checkLoadImage(&m_selectionOptionsOver, "selection-selectoptions-over", "OSU_SKIN_SELECTION_OPTIONS_OVER");

	checkLoadImage(&m_songSelectTop, "songselect-top", "OSU_SKIN_SONGSELECT_TOP");
	checkLoadImage(&m_songSelectBottom, "songselect-bottom", "OSU_SKIN_SONGSELECT_BOTTOM");
	checkLoadImage(&m_menuButtonBackground, "menu-button-background", "OSU_SKIN_MENU_BUTTON_BACKGROUND");

	checkLoadImage(&m_beatmapImportSpinner, "beatmapimport-spinner", "OSU_SKIN_BEATMAP_IMPORT_SPINNER");
	checkLoadImage(&m_circleEmpty, "circle-empty", "OSU_SKIN_CIRCLE_EMPTY");
	checkLoadImage(&m_circleFull, "circle-full", "OSU_SKIN_CIRCLE_FULL");

	// sounds

	// samples
	checkLoadSound(&m_normalHitNormal, "normal-hitnormal", "OSU_SKIN_NORMALHITNORMAL_SND", true, true);
	checkLoadSound(&m_normalHitWhistle, "normal-hitwhistle", "OSU_SKIN_NORMALHITWHISTLE_SND", true, true);
	checkLoadSound(&m_normalHitFinish, "normal-hitfinish", "OSU_SKIN_NORMALHITFINISH_SND", true, true);
	checkLoadSound(&m_normalHitClap, "normal-hitclap", "OSU_SKIN_NORMALHITCLAP_SND", true, true);
	checkLoadSound(&m_normalSliderTick, "normal-slidertick", "OSU_SKIN_NORMALSLIDERTICK_SND", true, true);
	checkLoadSound(&m_softHitNormal, "soft-hitnormal", "OSU_SKIN_SOFTHITNORMAL_SND", true, true);
	checkLoadSound(&m_softHitWhistle, "soft-hitwhistle", "OSU_SKIN_SOFTHITWHISTLE_SND", true, true);
	checkLoadSound(&m_softHitFinish, "soft-hitfinish", "OSU_SKIN_SOFTHITFINISH_SND", true, true);
	checkLoadSound(&m_softHitClap, "soft-hitclap", "OSU_SKIN_SOFTHITCLAP_SND", true, true);
	checkLoadSound(&m_softSliderTick, "soft-slidertick", "OSU_SKIN_SOFTSLIDERTICK_SND", true, true);
	checkLoadSound(&m_drumHitNormal, "drum-hitnormal", "OSU_SKIN_DRUMHITNORMAL_SND", true, true);
	checkLoadSound(&m_drumHitWhistle, "drum-hitwhistle", "OSU_SKIN_DRUMHITWHISTLE_SND", true, true);
	checkLoadSound(&m_drumHitFinish, "drum-hitfinish", "OSU_SKIN_DRUMHITFINISH_SND", true, true);
	checkLoadSound(&m_drumHitClap, "drum-hitclap", "OSU_SKIN_DRUMHITCLAP_SND", true, true);
	checkLoadSound(&m_drumSliderTick, "drum-slidertick", "OSU_SKIN_DRUMSLIDERTICK_SND", true, true);
	checkLoadSound(&m_spinnerBonus, "spinnerbonus", "OSU_SKIN_SPINNERBONUS_SND", true, true);
	checkLoadSound(&m_spinnerSpinSound, "spinnerspin", "OSU_SKIN_SPINNERSPIN_SND", false, true);

	// others
	checkLoadSound(&m_combobreak, "combobreak", "OSU_SKIN_COMBOBREAK_SND");
	checkLoadSound(&m_applause, "applause", "OSU_SKIN_APPLAUSE_SND");
	checkLoadSound(&m_menuHit, "menuhit", "OSU_SKIN_MENUHIT_SND", true);
	checkLoadSound(&m_menuClick, "menuclick", "OSU_SKIN_MENUCLICK_SND", true);
	checkLoadSound(&m_checkOn, "check-on", "OSU_SKIN_CHECKON_SND", true);
	checkLoadSound(&m_checkOff, "check-off", "OSU_SKIN_CHECKOFF_SND", true);

	// scaling
	// HACKHACK: this is pure cancer
	if (m_cursor != NULL && m_cursor->getFilePath().find("@2x") != -1)
		m_bCursor2x = true;
	if (m_approachCircle != NULL && m_approachCircle->getFilePath().find("@2x") != -1)
		m_bApproachCircle2x = true;
	if (m_reverseArrow != NULL && m_reverseArrow->getFilePath().find("@2x") != -1)
		m_bReverseArrow2x = true;
	if (m_followPoint != NULL && m_followPoint->getFilePath().find("@2x") != -1)
		m_bFollowPoint2x = true;
	if (m_hitCircle != NULL && m_hitCircle->getFilePath().find("@2x") != -1)
		m_bHitCircle2x = true;
	if (m_hitCircleOverlay != NULL && m_hitCircleOverlay->getFilePath().find("@2x") != -1)
		m_bHitCircleOverlay2x = true;
	if (m_default1 != NULL && m_default1->getFilePath().find("@2x") != -1)
		m_bIsDefault12x = true;
	if (m_hit0 != NULL && m_hit0->getFilePath().find("@2x") != -1)
		m_bHit02x = true;
	if (m_hit50 != NULL && m_hit50->getFilePath().find("@2x") != -1)
		m_bHit502x = true;
	if (m_hit100 != NULL && m_hit100->getFilePath().find("@2x") != -1)
		m_bHit1002x = true;
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

	if (m_circularmetre != NULL && m_circularmetre->getFilePath().find("@2x") != -1)
		m_bCircularmetre2x = true;
	if (m_playSkip != NULL && m_playSkip->getFilePath().find("@2x") != -1)
		m_bPlaySkip2x = true;

	if (m_pauseContinue != NULL && m_pauseContinue->getFilePath().find("@2x") != -1)
		m_bPauseContinue2x = true;

	if (m_menuButtonBackground != NULL && m_menuButtonBackground->getFilePath().find("@2x") != -1)
		m_bMenuButtonBackground2x = true;

	// custom
	Image *defaultCursor = engine->getResourceManager()->getImage("OSU_SKIN_CURSOR_DEFAULT");
	if (defaultCursor != NULL)
		m_defaultCursor = defaultCursor;

	Image *defaultButtonLeft = engine->getResourceManager()->getImage("OSU_SKIN_BUTTON_LEFT_DEFAULT");
	if (defaultButtonLeft != NULL)
		m_defaultButtonLeft = defaultButtonLeft;
	Image *defaultButtonMiddle = engine->getResourceManager()->getImage("OSU_SKIN_BUTTON_MIDDLE_DEFAULT");
	if (defaultButtonMiddle != NULL)
		m_defaultButtonMiddle = defaultButtonMiddle;
	Image *defaultButtonRight = engine->getResourceManager()->getImage("OSU_SKIN_BUTTON_RIGHT_DEFAULT");
	if (defaultButtonRight != NULL)
		m_defaultButtonRight = defaultButtonRight;

	// print some debug info
	debugLog("OsuSkin: Version %f\n", m_fVersion);
	debugLog("OsuSkin: HitCircleOverlap = %i\n", m_iHitCircleOverlap);

	// delayed error notifications due to resource loading potentially blocking engine time
	if (!parseSkinIni1Status && parseSkinIni2Status && m_osu_skin_ref->getString() != "default")
		m_osu->getNotificationOverlay()->addNotification("OsuSkin Error: Couldn't load skin.ini!", 0xffff0000);
	else if (!parseSkinIni2Status)
		m_osu->getNotificationOverlay()->addNotification("OsuSkin Error: Couldn't load DEFAULT skin.ini!!!", 0xffff0000);
}

void OsuSkin::loadBeatmapOverride(UString filepath)
{
	debugLog("OsuSkin::loadBeatmapOverride( %s )\n", filepath.toUtf8());
	// TODO:
}

bool OsuSkin::parseSkinINI(UString filepath)
{
	std::ifstream file(filepath.toUtf8());
	if (!file.good())
	{
		debugLog("OsuSkin Error: Couldn't load %s\n", filepath.toUtf8());
		return false;
	}

	int curBlock = -1;
	std::string curLine;
	while (std::getline(file, curLine))
	{
		const char *curLineChar = curLine.c_str();

		if (curLine.find("//") == std::string::npos) // ignore comments
		{
			if (curLine.find("[General]") != std::string::npos)
				curBlock = 0;
			else if (curLine.find("[Colours]") != std::string::npos)
				curBlock = 1;
			else if (curLine.find("[Fonts]") != std::string::npos)
				curBlock = 2;

			switch (curBlock)
			{
			case 0: // General
				{
					int val;
					char stringBuffer[1024];

					memset(stringBuffer, '\0', 1024);
					if (sscanf(curLineChar, "Version:%1023[^\n]", stringBuffer) == 1)
					{
						UString versionString = UString(stringBuffer);
						if (versionString.find("latest") != -1 || versionString.find("User") != -1)
							m_fVersion = 2.0f;
						else
							m_fVersion = versionString.toFloat();
					}

					if (sscanf(curLineChar, "CursorRotate: %i\n", &val) == 1)
						m_bCursorRotate = val > 0 ? true : false;
					if (sscanf(curLineChar, "CursorCentre: %i\n", &val) == 1)
						m_bCursorCenter = val > 0 ? true : false;
					if (sscanf(curLineChar, "CursorExpand: %i\n", &val) == 1)
						m_bCursorExpand = val > 0 ? true : false;
					if (sscanf(curLineChar, "SliderBallFlip: %i\n", &val) == 1)
						m_bSliderBallFlip = val > 0 ? true : false;
					if (sscanf(curLineChar, "AllowSliderBallTint: %i\n", &val) == 1)
						m_bAllowSliderBallTint = val > 0 ? true : false;
					if (sscanf(curLineChar, "HitCircleOverlayAboveNumber: %i\n", &val) == 1)
						m_bHitCircleOverlayAboveNumber = val > 0 ? true : false;
					if (sscanf(curLineChar, "HitCircleOverlayAboveNumer: %i\n", &val) == 1)
						m_bHitCircleOverlayAboveNumber = val > 0 ? true : false;
					if (sscanf(curLineChar, "SliderStyle: %i\n", &val) == 1)
					{
						m_iSliderStyle = val;
						if (m_iSliderStyle != 1 && m_iSliderStyle != 2)
							m_iSliderStyle = 2;
					}
				}
				break;
			case 1: // Colors
				{
					int comboNum;
					int r,g,b;
					if (sscanf(curLineChar, "Combo%i: %i,%i,%i\n", &comboNum, &r, &g, &b) == 4)
						m_comboColors.push_back(COLOR(255, r, g, b));
					if (sscanf(curLineChar, "SpinnerApproachCircle: %i,%i,%i\n", &r, &g, &b) == 3)
						m_spinnerApproachCircleColor = COLOR(255, r, g, b);
					if (sscanf(curLineChar, "SliderBorder: %i,%i,%i\n", &r, &g, &b) == 3)
						m_sliderBorderColor = COLOR(255, r, g, b);
					if (sscanf(curLineChar, "SliderTrackOverride: %i,%i,%i\n", &r, &g, &b) == 3)
					{
						m_sliderTrackOverride = COLOR(255, r, g, b);
						m_bSliderTrackOverride = true;
					}
					if (sscanf(curLineChar, "SliderBall: %i,%i,%i\n", &r, &g, &b) == 3)
						m_sliderBallColor = COLOR(255, r, g, b);
					if (sscanf(curLineChar, "SongSelectActiveText: %i,%i,%i\n", &r, &g, &b) == 3)
						m_songSelectActiveText = COLOR(255, r, g, b);
					if (sscanf(curLineChar, "SongSelectInactiveText: %i,%i,%i\n", &r, &g, &b) == 3)
						m_songSelectInactiveText = COLOR(255, r, g, b);
				}
				break;
			case 2: // Fonts
				{
					int val;

					if (sscanf(curLineChar, "HitCircleOverlap: %i\n", &val) == 1)
						m_iHitCircleOverlap = val;
				}
				break;
			}
		}
	}

	return true;
}

void OsuSkin::onEffectVolumeChange(UString oldValue, UString newValue)
{
	float volume = newValue.toFloat();
	m_iSampleVolume = (int)(volume*100.0f);

	for (int i=0; i<m_sounds.size(); i++)
	{
		m_sounds[i]->setVolume(volume);
	}
}

void OsuSkin::setSampleSet(int sampleSet)
{
	if (m_iSampleSet == sampleSet)
		return;

	///debugLog("sample set = %i\n", sampleSet);
	m_iSampleSet = sampleSet;
}

void OsuSkin::setSampleVolume(float volume)
{
	if (osu_ignore_beatmap_sample_volume.getBool()) return;

	float sampleVolume = volume*osu_volume_effects.getFloat();
	if (m_iSampleVolume == (int)(sampleVolume*100.0f))
		return;

	m_iSampleVolume = (int)(sampleVolume*100.0f);
	///debugLog("sample volume = %f\n", sampleVolume);
	for (int i=0; i<m_soundSamples.size(); i++)
	{
		m_soundSamples[i]->setVolume(sampleVolume);
	}
}

Color OsuSkin::getComboColorForCounter(int i)
{
	if (m_beatmapComboColors.size() > 0 && !osu_ignore_beatmap_combo_colors.getBool())
		return m_beatmapComboColors[i % m_beatmapComboColors.size()];
	else if (m_comboColors.size() > 0)
		return m_comboColors[i % m_comboColors.size()];
	else
		return COLOR(255, 255, 255, 255);
}

void OsuSkin::setBeatmapComboColors(std::vector<Color> colors)
{
	m_beatmapComboColors = colors;
}

void OsuSkin::playHitCircleSound(int sampleType)
{
	switch (m_iSampleSet)
	{
	case 3:
		engine->getSound()->play(m_drumHitNormal);

		if (sampleType & OSU_BITMASK_HITWHISTLE)
			engine->getSound()->play(m_drumHitWhistle);
		if (sampleType & OSU_BITMASK_HITFINISH)
			engine->getSound()->play(m_drumHitFinish);
		if (sampleType & OSU_BITMASK_HITCLAP)
			engine->getSound()->play(m_drumHitClap);
		break;
	case 2:
		engine->getSound()->play(m_softHitNormal);

		if (sampleType & OSU_BITMASK_HITWHISTLE)
			engine->getSound()->play(m_softHitWhistle);
		if (sampleType & OSU_BITMASK_HITFINISH)
			engine->getSound()->play(m_softHitFinish);
		if (sampleType & OSU_BITMASK_HITCLAP)
			engine->getSound()->play(m_softHitClap);
		break;
	default:
		engine->getSound()->play(m_normalHitNormal);

		if (sampleType & OSU_BITMASK_HITWHISTLE)
			engine->getSound()->play(m_normalHitWhistle);
		if (sampleType & OSU_BITMASK_HITFINISH)
			engine->getSound()->play(m_normalHitFinish);
		if (sampleType & OSU_BITMASK_HITCLAP)
			engine->getSound()->play(m_normalHitClap);
		break;
	}
}

Sound *OsuSkin::getSliderTick()
{
	switch (m_iSampleSet)
	{
	case 3:
		return m_drumSliderTick;
	case 2:
		return m_softSliderTick;
	default:
		return m_normalSliderTick;
	}
}


void OsuSkin::checkLoadImage(Image **addressOfPointer, UString skinElementName, UString resourceName, bool ignoreDefaultSkin)
{
	// we are already loaded
	if (*addressOfPointer != m_missingTexture)
		return;

	// check if an @2x version of this image exists
	if (osu_skin_hd.getBool())
	{
		// load default first
		if (!ignoreDefaultSkin)
		{
			UString defaultFilePath = UString("./materials/");
			defaultFilePath.append(OSUSKIN_DEFAULT_SKIN_PATH);
			defaultFilePath.append(skinElementName);
			defaultFilePath.append("@2x.png");
			if (env->fileExists(defaultFilePath))
			{
				UString defaultResourceName = resourceName;
				defaultResourceName.append("_DEFAULT"); // so we don't load the default skin twice
				if (osu_skin_load_async.getBool())
					engine->getResourceManager()->requestNextLoadAsync();
				*addressOfPointer = engine->getResourceManager()->loadImageAbs(defaultFilePath, defaultResourceName);
				///m_resources.push_back(*addressOfPointer); // HACKHACK: also reload default skin
			}
		}

		// and now try to load the actual specified skin
		UString filepath1 = m_sFilePath;
		filepath1.append(skinElementName);
		filepath1.append("@2x.png");

		if (env->fileExists(filepath1))
		{
			if (osu_skin_load_async.getBool())
				engine->getResourceManager()->requestNextLoadAsync();
			*addressOfPointer = engine->getResourceManager()->loadImageAbs(filepath1, resourceName);
			m_resources.push_back(*addressOfPointer);

			return; // nothing more to do here
		}
	}

	// else load the normal version

	// load default first
	if (!ignoreDefaultSkin)
	{
		UString defaultFilePath = UString("./materials/");
		defaultFilePath.append(OSUSKIN_DEFAULT_SKIN_PATH);
		defaultFilePath.append(skinElementName);
		defaultFilePath.append(".png");
		if (env->fileExists(defaultFilePath))
		{
			UString defaultResourceName = resourceName;
			defaultResourceName.append("_DEFAULT"); // so we don't load the default skin twice
			if (osu_skin_load_async.getBool())
				engine->getResourceManager()->requestNextLoadAsync();
			*addressOfPointer = engine->getResourceManager()->loadImageAbs(defaultFilePath, defaultResourceName);
			///m_resources.push_back(*addressOfPointer); // HACKHACK: also reload default skin
		}
	}

	// and then the actual specified skin
	UString filepath2 = m_sFilePath;
	filepath2.append(skinElementName);
	filepath2.append(".png");

	if (env->fileExists(filepath2))
	{
		if (osu_skin_load_async.getBool())
			engine->getResourceManager()->requestNextLoadAsync();
		*addressOfPointer = engine->getResourceManager()->loadImageAbs(filepath2, resourceName);
		m_resources.push_back(*addressOfPointer);
	}
}

void OsuSkin::checkLoadSound(Sound **addressOfPointer, UString skinElementName, UString resourceName, bool isOverlayable, bool isSample)
{
	// we are already loaded
	if (*addressOfPointer != NULL)
		return;

	// load default
	UString defaultpath1 = UString("./materials/");
	defaultpath1.append(OSUSKIN_DEFAULT_SKIN_PATH);
	defaultpath1.append(skinElementName);
	defaultpath1.append(".wav");

	UString defaultpath2 = UString("./materials/");
	defaultpath2.append(OSUSKIN_DEFAULT_SKIN_PATH);
	defaultpath2.append(skinElementName);
	defaultpath2.append(".mp3");

	UString defaultResourceName = resourceName;
	defaultResourceName.append("_DEFAULT"); // so we don't load the default skin twice
	if (env->fileExists(defaultpath1))
	{
		if (osu_skin_load_async.getBool())
			engine->getResourceManager()->requestNextLoadAsync();
		*addressOfPointer = engine->getResourceManager()->loadSoundAbs(defaultpath1, defaultResourceName);
	}
	else if (env->fileExists(defaultpath2))
	{
		if (osu_skin_load_async.getBool())
			engine->getResourceManager()->requestNextLoadAsync();
		*addressOfPointer = engine->getResourceManager()->loadSoundAbs(defaultpath2, defaultResourceName);
	}

	// and then the actual specified skin

	// check if mp3 or wav exist
	UString filepath1 = m_sFilePath;
	filepath1.append(skinElementName);
	filepath1.append(".wav");

	UString filepath2 = m_sFilePath;
	filepath2.append(skinElementName);
	filepath2.append(".mp3");

	// load it
	if (env->fileExists(filepath1))
	{
		if (osu_skin_load_async.getBool())
			engine->getResourceManager()->requestNextLoadAsync();
		*addressOfPointer = engine->getResourceManager()->loadSoundAbs(filepath1, resourceName);
	}
	else if (env->fileExists(filepath2))
	{
		if (osu_skin_load_async.getBool())
			engine->getResourceManager()->requestNextLoadAsync();
		*addressOfPointer = engine->getResourceManager()->loadSoundAbs(filepath2, resourceName);
	}

	if ((*addressOfPointer) != NULL)
	{
		m_resources.push_back(*addressOfPointer);
		m_sounds.push_back(*addressOfPointer);
		if (isSample)
			m_soundSamples.push_back(*addressOfPointer);
		if (isOverlayable)
			(*addressOfPointer)->setOverlayable(true);
	}
	else
		debugLog("OsuSkin Warning: NULL sound %s\n", skinElementName.toUtf8());
}

bool OsuSkin::compareFilenameWithSkinElementName(UString filename, UString skinElementName)
{
	if (filename.length() == 0 || skinElementName.length() == 0)
		return false;

	return filename.substr(0, filename.findLast(".")) == skinElementName;
}

