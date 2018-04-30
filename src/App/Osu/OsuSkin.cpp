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
#include "File.h"

#include "Osu.h"
#include "OsuSkinImage.h"
#include "OsuBeatmap.h"
#include "OsuNotificationOverlay.h"

#include <string.h>

#define OSU_BITMASK_HITWHISTLE 0x2
#define OSU_BITMASK_HITFINISH 0x4
#define OSU_BITMASK_HITCLAP 0x8

void DUMMY_OSU_VOLUME_EFFECTS_ARGS(UString oldValue, UString newValue) {;}

ConVar osu_volume_effects("osu_volume_effects", 1.0f, DUMMY_OSU_VOLUME_EFFECTS_ARGS);
ConVar osu_skin_hd("osu_skin_hd", true);
ConVar osu_skin_color_index_add("osu_skin_color_index_add", 0);
ConVar osu_skin_animation_force("osu_skin_animation_force", false);
ConVar osu_skin_use_skin_hitsounds("osu_skin_use_skin_hitsounds", true, "If enabled: Use skin's sound samples. If disabled: Use default skin's sound samples. For hitsounds only.");

ConVar osu_ignore_beatmap_combo_colors("osu_ignore_beatmap_combo_colors", false);
ConVar osu_ignore_beatmap_sample_volume("osu_ignore_beatmap_sample_volume", false);

const char *OsuSkin::OSUSKIN_DEFAULT_SKIN_PATH = ""; // set dynamically below in the constructor
Image *OsuSkin::m_missingTexture = NULL;
ConVar *OsuSkin::m_osu_skin_ref = NULL;
ConVar *OsuSkin::m_osu_skin_hd = &osu_skin_hd;

OsuSkin::OsuSkin(Osu *osu, UString filepath, bool isDefaultSkin)
{
	m_osu = osu;
	m_sFilePath = filepath;
	m_bIsDefaultSkin = isDefaultSkin;

	if (m_osu->isInVRMode())
		OSUSKIN_DEFAULT_SKIN_PATH = "defaultvr/";
	else
		OSUSKIN_DEFAULT_SKIN_PATH = "default/";

	// convar refs
	if (m_osu_skin_ref == NULL)
		m_osu_skin_ref = convar->getConVarByName("osu_skin");

	if (m_missingTexture == NULL)
		m_missingTexture = engine->getResourceManager()->getImage("MISSING_TEXTURE");

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

	m_playWarningArrow = m_missingTexture;
	m_circularmetre = m_missingTexture;

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

	m_pauseContinue = m_missingTexture;
	m_pauseRetry = m_missingTexture;
	m_pauseBack = m_missingTexture;
	m_pauseOverlay = m_missingTexture;
	m_unpause = m_missingTexture;

	m_buttonLeft = m_missingTexture;
	m_buttonMiddle = m_missingTexture;
	m_buttonRight = m_missingTexture;
	m_defaultButtonLeft = m_missingTexture;
	m_defaultButtonMiddle = m_missingTexture;
	m_defaultButtonRight = m_missingTexture;
	m_selectionMode = m_missingTexture;
	m_selectionModeOver = m_missingTexture;
	m_selectionMods = m_missingTexture;
	m_selectionModsOver = m_missingTexture;
	m_selectionRandom = m_missingTexture;
	m_selectionRandomOver = m_missingTexture;
	m_selectionOptions = m_missingTexture;
	m_selectionOptionsOver = m_missingTexture;

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
	m_failsound = NULL;
	m_applause = NULL;
	m_menuHit = NULL;
	m_menuClick = NULL;
	m_checkOn = NULL;
	m_checkOff = NULL;
	m_shutter = NULL;

	m_spinnerApproachCircleColor = 0xffffffff;
	m_sliderBorderColor = 0xffffffff;
	m_sliderBallColor = 0xff02aaff;

	m_songSelectActiveText = 0xff000000;
	m_songSelectInactiveText = 0xffffffff;

	// scaling
	m_bCursor2x = false;
	m_bApproachCircle2x = false;
	m_bReverseArrow2x = false;
	m_bHitCircle2x = false;
	m_bIsDefault02x = false;
	m_bIsDefault12x = false;
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

	load();

	// convar callbacks
	osu_volume_effects.setCallback( fastdelegate::MakeDelegate(this, &OsuSkin::onEffectVolumeChange) );

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
	m_resources.clear();

	for (int i=0; i<m_images.size(); i++)
	{
		delete m_images[i];
	}
	m_images.clear();

	m_sounds.clear();
}

void OsuSkin::update()
{
	// shitty check to not animate while paused with hitobjects in background
	if (m_osu->isInPlayMode() && m_osu->getSelectedBeatmap() != NULL && !m_osu->getSelectedBeatmap()->isPlaying() && !osu_skin_animation_force.getBool())
		return;

	const bool useEngineTimeForAnimations = !m_osu->isInPlayMode();
	const long curMusicPos = m_osu->getSelectedBeatmap() != NULL ? m_osu->getSelectedBeatmap()->getCurMusicPosWithOffsets() : 0;
	for (int i=0; i<m_images.size(); i++)
	{
		m_images[i]->update(useEngineTimeForAnimations, curMusicPos);
	}
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
	m_hitCircleOverlay2 = createOsuSkinImage("hitcircleoverlay", Vector2(128, 128), 64);
	m_hitCircleOverlay2->setAnimationFramerate(2);

	checkLoadImage(&m_approachCircle, "approachcircle", "OSU_SKIN_APPROACHCIRCLE");
	checkLoadImage(&m_reverseArrow, "reversearrow", "OSU_SKIN_REVERSEARROW");

	m_followPoint2 = createOsuSkinImage("followpoint", Vector2(16, 22), 64);

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

	m_playSkip = createOsuSkinImage("play-skip", Vector2(193, 147), 94);
	checkLoadImage(&m_playWarningArrow, "play-warningarrow", "OSU_SKIN_PLAYWARNINGARROW");
	m_playWarningArrow2 = createOsuSkinImage("play-warningarrow", Vector2(167, 129), 128);
	checkLoadImage(&m_circularmetre, "circularmetre", "OSU_SKIN_CIRCULARMETRE");

	m_hit0 = createOsuSkinImage("hit0", Vector2(128, 128), 42);
	m_hit0->setAnimationFramerate(60);
	m_hit50 = createOsuSkinImage("hit50", Vector2(128, 128), 42);
	m_hit50->setAnimationFramerate(60);
	m_hit100 = createOsuSkinImage("hit100", Vector2(128, 128), 42);
	m_hit100->setAnimationFramerate(60);
	m_hit100k = createOsuSkinImage("hit100k", Vector2(128, 128), 42);
	m_hit100k->setAnimationFramerate(60);
	m_hit300 = createOsuSkinImage("hit300", Vector2(128, 128), 42);
	m_hit300->setAnimationFramerate(60);
	m_hit300g = createOsuSkinImage("hit300g", Vector2(128, 128), 42);
	m_hit300g->setAnimationFramerate(60);
	m_hit300k = createOsuSkinImage("hit300k", Vector2(128, 128), 42);
	m_hit300k->setAnimationFramerate(60);

	checkLoadImage(&m_sliderGradient, "slidergradient", "OSU_SKIN_SLIDERGRADIENT");
	m_sliderb = createOsuSkinImage("sliderb", Vector2(128, 128), 64, false, "");
	m_sliderb->setAnimationFramerate(/*45.0f*/ 50.0f);
	checkLoadImage(&m_sliderScorePoint, "sliderscorepoint", "OSU_SKIN_SLIDERSCOREPOINT");
	m_sliderFollowCircle2 = createOsuSkinImage("sliderfollowcircle", Vector2(259, 259), 64);
	checkLoadImage(&m_sliderStartCircle, "sliderstartcircle", "OSU_SKIN_SLIDERSTARTCIRCLE", !m_bIsDefaultSkin); // !m_bIsDefaultSkin ensures that default doesn't override user, in these special cases
	m_sliderStartCircle2 = createOsuSkinImage("sliderstartcircle", Vector2(128, 128), 64, !m_bIsDefaultSkin);
	checkLoadImage(&m_sliderStartCircleOverlay, "sliderstartcircleoverlay", "OSU_SKIN_SLIDERSTARTCIRCLEOVERLAY", !m_bIsDefaultSkin);
	m_sliderStartCircleOverlay2 = createOsuSkinImage("sliderstartcircleoverlay", Vector2(128, 128), 64, !m_bIsDefaultSkin);
	m_sliderStartCircleOverlay2->setAnimationFramerate(2);
	checkLoadImage(&m_sliderEndCircle, "sliderendcircle", "OSU_SKIN_SLIDERENDCIRCLE", !m_bIsDefaultSkin);
	m_sliderEndCircle2 = createOsuSkinImage("sliderendcircle", Vector2(128, 128), 64, !m_bIsDefaultSkin);
	checkLoadImage(&m_sliderEndCircleOverlay, "sliderendcircleoverlay", "OSU_SKIN_SLIDERENDCIRCLEOVERLAY", !m_bIsDefaultSkin);
	m_sliderEndCircleOverlay2 = createOsuSkinImage("sliderendcircleoverlay", Vector2(128, 128), 64, !m_bIsDefaultSkin);
	m_sliderEndCircleOverlay2->setAnimationFramerate(2);

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

	checkLoadImage(&m_pauseContinue, "pause-continue", "OSU_SKIN_PAUSE_CONTINUE");
	checkLoadImage(&m_pauseRetry, "pause-retry", "OSU_SKIN_PAUSE_RETRY");
	checkLoadImage(&m_pauseBack, "pause-back", "OSU_SKIN_PAUSE_BACK");
	checkLoadImage(&m_pauseOverlay, "pause-overlay", "OSU_SKIN_PAUSE_OVERLAY"); if (m_pauseOverlay == m_missingTexture) checkLoadImage(&m_pauseOverlay, "pause-overlay", "OSU_SKIN_PAUSE_OVERLAY", true, "jpg");
	checkLoadImage(&m_unpause, "unpause", "OSU_SKIN_UNPAUSE");

	checkLoadImage(&m_buttonLeft, "button-left", "OSU_SKIN_BUTTON_LEFT");
	checkLoadImage(&m_buttonMiddle, "button-middle", "OSU_SKIN_BUTTON_MIDDLE");
	checkLoadImage(&m_buttonRight, "button-right", "OSU_SKIN_BUTTON_RIGHT");
	m_menuBack = createOsuSkinImage("menu-back", Vector2(225, 87), 54);
	checkLoadImage(&m_selectionMode, "selection-mode", "OSU_SKIN_SELECTION_MODE");
	checkLoadImage(&m_selectionModeOver, "selection-mode-over", "OSU_SKIN_SELECTION_MODE_OVER");
	checkLoadImage(&m_selectionMods, "selection-mods", "OSU_SKIN_SELECTION_MODS");
	checkLoadImage(&m_selectionModsOver, "selection-mods-over", "OSU_SKIN_SELECTION_MODS_OVER");
	checkLoadImage(&m_selectionRandom, "selection-random", "OSU_SKIN_SELECTION_RANDOM");
	checkLoadImage(&m_selectionRandomOver, "selection-random-over", "OSU_SKIN_SELECTION_RANDOM_OVER");
	checkLoadImage(&m_selectionOptions, "selection-options", "OSU_SKIN_SELECTION_OPTIONS");
	checkLoadImage(&m_selectionOptionsOver, "selection-selectoptions-over", "OSU_SKIN_SELECTION_OPTIONS_OVER");

	checkLoadImage(&m_songSelectTop, "songselect-top", "OSU_SKIN_SONGSELECT_TOP");
	checkLoadImage(&m_songSelectBottom, "songselect-bottom", "OSU_SKIN_SONGSELECT_BOTTOM");
	checkLoadImage(&m_menuButtonBackground, "menu-button-background", "OSU_SKIN_MENU_BUTTON_BACKGROUND");
	checkLoadImage(&m_star, "star", "OSU_SKIN_STAR");
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

	checkLoadImage(&m_beatmapImportSpinner, "beatmapimport-spinner", "OSU_SKIN_BEATMAP_IMPORT_SPINNER");
	checkLoadImage(&m_loadingSpinner, "loading-spinner", "OSU_SKIN_LOADING_SPINNER");
	checkLoadImage(&m_circleEmpty, "circle-empty", "OSU_SKIN_CIRCLE_EMPTY");
	checkLoadImage(&m_circleFull, "circle-full", "OSU_SKIN_CIRCLE_FULL");
	checkLoadImage(&m_seekTriangle, "seektriangle", "OSU_SKIN_SEEKTRIANGLE");

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

	// scaling
	// HACKHACK: this is pure cancer
	if (m_cursor != NULL && m_cursor->getFilePath().find("@2x") != -1)
		m_bCursor2x = true;
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
	Image *defaultCursor2 = engine->getResourceManager()->getImage("OSU_SKIN_CURSOR_DEFAULT");
	if (defaultCursor != NULL)
		m_defaultCursor = defaultCursor;
	else if (defaultCursor2 != NULL)
		m_defaultCursor = defaultCursor2;
	else
		m_defaultCursor = m_cursor;

	Image *defaultButtonLeft = engine->getResourceManager()->getImage("OSU_SKIN_BUTTON_LEFT_DEFAULT");
	Image *defaultButtonLeft2 = engine->getResourceManager()->getImage("OSU_SKIN_BUTTON_LEFT");
	if (defaultButtonLeft != NULL)
		m_defaultButtonLeft = defaultButtonLeft;
	else if (defaultButtonLeft2 != NULL)
		m_defaultButtonLeft = defaultButtonLeft2;

	Image *defaultButtonMiddle = engine->getResourceManager()->getImage("OSU_SKIN_BUTTON_MIDDLE_DEFAULT");
	Image *defaultButtonMiddle2 = engine->getResourceManager()->getImage("OSU_SKIN_BUTTON_MIDDLE");
	if (defaultButtonMiddle != NULL)
		m_defaultButtonMiddle = defaultButtonMiddle;
	else if (defaultButtonMiddle2 != NULL)
		m_defaultButtonMiddle = defaultButtonMiddle2;

	Image *defaultButtonRight = engine->getResourceManager()->getImage("OSU_SKIN_BUTTON_RIGHT_DEFAULT");
	Image *defaultButtonRight2 = engine->getResourceManager()->getImage("OSU_SKIN_BUTTON_RIGHT");
	if (defaultButtonRight != NULL)
		m_defaultButtonRight = defaultButtonRight;
	else if (defaultButtonRight2 != NULL)
		m_defaultButtonRight = defaultButtonRight2;

	// print some debug info
	debugLog("OsuSkin: Version %f\n", m_fVersion);
	debugLog("OsuSkin: HitCircleOverlap = %i\n", m_iHitCircleOverlap);

	// delayed error notifications due to resource loading potentially blocking engine time
	if (!parseSkinIni1Status && parseSkinIni2Status && m_osu_skin_ref->getString() != "default" && m_osu_skin_ref->getString() != "defaultvr")
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
	File file(filepath);
	if (!file.canRead())
	{
		debugLog("OsuSkin Error: Couldn't load %s\n", filepath.toUtf8());
		return false;
	}

	int curBlock = -1;
	while (file.canRead())
	{
		UString uCurLine = file.readLine();
		const char *curLineChar = uCurLine.toUtf8();
		std::string curLine(curLineChar);

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
				}
				break;
			case 2: // Fonts
				{
					int val;
					char stringBuffer[1024];

					memset(stringBuffer, '\0', 1024);
					if (sscanf(curLineChar, " ComboPrefix : %1023[^\n]", stringBuffer) == 1)
						m_sComboPrefix = UString(stringBuffer);
					if (sscanf(curLineChar, " ComboOverlap : %i \n", &val) == 1)
						m_iComboOverlap = val;

					if (sscanf(curLineChar, " ScorePrefix : %1023[^\n]", stringBuffer) == 1)
						m_sScorePrefix = UString(stringBuffer);
					if (sscanf(curLineChar, " ScoreOverlap : %i \n", &val) == 1)
						m_iScoreOverlap = val;

					if (sscanf(curLineChar, " HitCirclePrefix : %1023[^\n]", stringBuffer) == 1)
						m_sHitCirclePrefix = UString(stringBuffer);
					if (sscanf(curLineChar, " HitCircleOverlap : %i \n", &val) == 1)
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

	for (int i=0; i<m_sounds.size(); i++)
	{
		m_sounds[i]->setVolume(volume);
	}

	// restore sample volumes
	setSampleVolume(clamp<float>((float)m_iSampleVolume / 100.0f, 0.0f, 1.0f), true);
}

void OsuSkin::setSampleSet(int sampleSet)
{
	if (m_iSampleSet == sampleSet)
		return;

	///debugLog("sample set = %i\n", sampleSet);
	m_iSampleSet = sampleSet;
}

void OsuSkin::setSampleVolume(float volume, bool force)
{
	if (osu_ignore_beatmap_sample_volume.getBool()) return;

	float sampleVolume = volume*osu_volume_effects.getFloat();
	if (!force && m_iSampleVolume == (int)(sampleVolume*100.0f))
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
	i += osu_skin_color_index_add.getInt();
	if (m_beatmapComboColors.size() > 0 && !osu_ignore_beatmap_combo_colors.getBool())
		return m_beatmapComboColors[i % m_beatmapComboColors.size()];
	else if (m_comboColors.size() > 0)
		return m_comboColors[i % m_comboColors.size()];
	else
		return COLOR(255, 0, 255, 0);
}

void OsuSkin::setBeatmapComboColors(std::vector<Color> colors)
{
	m_beatmapComboColors = colors;
}

void OsuSkin::playHitCircleSound(int sampleType)
{
	if (m_iSampleVolume <= 0)
		return;

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

void OsuSkin::playSliderTickSound()
{
	if (m_iSampleVolume <= 0)
		return;

	switch (m_iSampleSet)
	{
	case 3:
		engine->getSound()->play(m_drumSliderTick);
		break;
	case 2:
		engine->getSound()->play(m_softSliderTick);
		break;
	default:
		engine->getSound()->play(m_normalSliderTick);
		break;
	}
}

OsuSkinImage *OsuSkin::createOsuSkinImage(UString skinElementName, Vector2 baseSizeForScaling2x, float osuSize, bool ignoreDefaultSkin, UString animationSeparator)
{
	OsuSkinImage *skinImage = new OsuSkinImage(this, skinElementName, baseSizeForScaling2x, osuSize, animationSeparator, ignoreDefaultSkin);
	m_images.push_back(skinImage);
	return skinImage;
}

void OsuSkin::checkLoadImage(Image **addressOfPointer, UString skinElementName, UString resourceName, bool ignoreDefaultSkin, UString fileExtension)
{
	if (*addressOfPointer != m_missingTexture) return; // we are already loaded

	// check if an @2x version of this image exists
	if (osu_skin_hd.getBool())
	{
		// load default first
		if (!ignoreDefaultSkin)
		{
			UString defaultFilePath = UString("./materials/");
			defaultFilePath.append(OSUSKIN_DEFAULT_SKIN_PATH);
			defaultFilePath.append(skinElementName);
			defaultFilePath.append("@2x.");
			defaultFilePath.append(fileExtension);
			if (env->fileExists(defaultFilePath))
			{
				UString defaultResourceName = resourceName;
				defaultResourceName.append("_DEFAULT"); // so we don't load the default skin twice
				*addressOfPointer = engine->getResourceManager()->loadImageAbs(defaultFilePath, defaultResourceName);
				///m_resources.push_back(*addressOfPointer); // HACKHACK: also reload default skin
			}
		}

		// and now try to load the actual specified skin
		UString filepath1 = m_sFilePath;
		filepath1.append(skinElementName);
		filepath1.append("@2x.");
		filepath1.append(fileExtension);

		if (env->fileExists(filepath1))
		{
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
		defaultFilePath.append(".");
		defaultFilePath.append(fileExtension);
		if (env->fileExists(defaultFilePath))
		{
			UString defaultResourceName = resourceName;
			defaultResourceName.append("_DEFAULT"); // so we don't load the default skin twice
			*addressOfPointer = engine->getResourceManager()->loadImageAbs(defaultFilePath, defaultResourceName);
			///m_resources.push_back(*addressOfPointer); // HACKHACK: also reload default skin
		}
	}

	// and then the actual specified skin
	UString filepath2 = m_sFilePath;
	filepath2.append(skinElementName);
	filepath2.append(".");
	filepath2.append(fileExtension);

	if (env->fileExists(filepath2))
	{
		*addressOfPointer = engine->getResourceManager()->loadImageAbs(filepath2, resourceName);
		m_resources.push_back(*addressOfPointer);
	}
}

void OsuSkin::checkLoadSound(Sound **addressOfPointer, UString skinElementName, UString resourceName, bool isOverlayable, bool isSample, bool loop)
{
	if (*addressOfPointer != NULL) return; // we are already loaded

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
		*addressOfPointer = engine->getResourceManager()->loadSoundAbs(defaultpath1, defaultResourceName, false, false, loop);
	else if (env->fileExists(defaultpath2))
		*addressOfPointer = engine->getResourceManager()->loadSoundAbs(defaultpath2, defaultResourceName, false, false, loop);

	// and then the actual specified skin

	if (!isSample || osu_skin_use_skin_hitsounds.getBool())
	{
		// check if mp3 or wav exist
		UString filepath1 = m_sFilePath;
		filepath1.append(skinElementName);
		filepath1.append(".wav");

		UString filepath2 = m_sFilePath;
		filepath2.append(skinElementName);
		filepath2.append(".mp3");

		// load it
		if (env->fileExists(filepath1))
			*addressOfPointer = engine->getResourceManager()->loadSoundAbs(filepath1, resourceName, false, false, loop);
		else if (env->fileExists(filepath2))
			*addressOfPointer = engine->getResourceManager()->loadSoundAbs(filepath2, resourceName, false, false, loop);
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

