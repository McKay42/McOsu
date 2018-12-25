//================ Copyright (c) 2015, PG, All rights reserved. =================//
//
// Purpose:		skin loader and container
//
// $NoKeywords: $osusk
//===============================================================================//

#ifndef OSUSKIN_H
#define OSUSKIN_H

#include "cbase.h"

class Image;
class Sound;
class Resource;
class ConVar;

class Osu;
class OsuSkinImage;

class OsuSkin
{
public:
	static const char *OSUSKIN_DEFAULT_SKIN_PATH;

	static ConVar *m_osu_skin_hd;

public:
	OsuSkin(Osu *osu, UString filepath, bool isDefaultSkin = false);
	virtual ~OsuSkin();

	void update();

	void load();
	void loadBeatmapOverride(UString filepath);

	// samples
	void setSampleSet(int sampleSet);
	void setSampleVolume(float volume, bool force = false);

	void playHitCircleSound(int sampleType, float pan = 0.0f);
	void playSliderTickSound(float pan = 0.0f);
	void playSpinnerSpinSound();
	void playSpinnerBonusSound();

	// drawable helpers
	inline UString getFilePath() {return m_sFilePath;}
	inline Osu *getOsu() {return m_osu;}

	// raw
	inline Image *getMissingTexture() {return m_missingTexture;}

	inline Image *getHitCircle() {return m_hitCircle;}
	inline OsuSkinImage *getHitCircleOverlay2() {return m_hitCircleOverlay2;}
	inline Image *getApproachCircle() {return m_approachCircle;}
	inline Image *getReverseArrow() {return m_reverseArrow;}
	inline OsuSkinImage *getFollowPoint2() {return m_followPoint2;}

	inline Image *getDefault0() {return m_default0;}
	inline Image *getDefault1() {return m_default1;}
	inline Image *getDefault2() {return m_default2;}
	inline Image *getDefault3() {return m_default3;}
	inline Image *getDefault4() {return m_default4;}
	inline Image *getDefault5() {return m_default5;}
	inline Image *getDefault6() {return m_default6;}
	inline Image *getDefault7() {return m_default7;}
	inline Image *getDefault8() {return m_default8;}
	inline Image *getDefault9() {return m_default9;}

	inline Image *getScore0() {return m_score0;}
	inline Image *getScore1() {return m_score1;}
	inline Image *getScore2() {return m_score2;}
	inline Image *getScore3() {return m_score3;}
	inline Image *getScore4() {return m_score4;}
	inline Image *getScore5() {return m_score5;}
	inline Image *getScore6() {return m_score6;}
	inline Image *getScore7() {return m_score7;}
	inline Image *getScore8() {return m_score8;}
	inline Image *getScore9() {return m_score9;}
	inline Image *getScoreX() {return m_scoreX;}
	inline Image *getScorePercent() {return m_scorePercent;}
	inline Image *getScoreDot() {return m_scoreDot;}

	inline OsuSkinImage *getPlaySkip() {return m_playSkip;}
	inline Image *getPlayWarningArrow() {return m_playWarningArrow;}
	inline OsuSkinImage *getPlayWarningArrow2() {return m_playWarningArrow2;}
	inline Image *getCircularmetre() {return m_circularmetre;}
	inline OsuSkinImage *getScorebarBg() {return m_scorebarBg;}
	inline OsuSkinImage *getInputoverlayBackground() {return m_inputoverlayBackground;}
	inline OsuSkinImage *getInputoverlayKey() {return m_inputoverlayKey;}

	inline OsuSkinImage *getHit0() {return m_hit0;}
	inline OsuSkinImage *getHit50() {return m_hit50;}
	inline OsuSkinImage *getHit100() {return m_hit100;}
	inline OsuSkinImage *getHit100k() {return m_hit100k;}
	inline OsuSkinImage *getHit300() {return m_hit300;}
	inline OsuSkinImage *getHit300g() {return m_hit300g;}
	inline OsuSkinImage *getHit300k() {return m_hit300k;}

	inline Image *getSliderGradient() {return m_sliderGradient;}
	inline OsuSkinImage *getSliderb() {return m_sliderb;}
	inline OsuSkinImage *getSliderFollowCircle2() {return m_sliderFollowCircle2;}
	inline Image *getSliderScorePoint() {return m_sliderScorePoint;}
	inline Image *getSliderStartCircle() {return m_sliderStartCircle;}
	inline OsuSkinImage *getSliderStartCircle2() {return m_sliderStartCircle2;}
	inline Image *getSliderStartCircleOverlay() {return m_sliderStartCircleOverlay;}
	inline OsuSkinImage *getSliderStartCircleOverlay2() {return m_sliderStartCircleOverlay2;}
	inline Image *getSliderEndCircle() {return m_sliderEndCircle;}
	inline OsuSkinImage *getSliderEndCircle2() {return m_sliderEndCircle2;}
	inline Image *getSliderEndCircleOverlay() {return m_sliderEndCircleOverlay;}
	inline OsuSkinImage *getSliderEndCircleOverlay2() {return m_sliderEndCircleOverlay2;}

	inline Image *getSpinnerBackground() {return m_spinnerBackground;}
	inline Image *getSpinnerCircle() {return m_spinnerCircle;}
	inline Image *getSpinnerApproachCircle() {return m_spinnerApproachCircle;}
	inline Image *getSpinnerBottom() {return m_spinnerBottom;}
	inline Image *getSpinnerMiddle() {return m_spinnerMiddle;}
	inline Image *getSpinnerMiddle2() {return m_spinnerMiddle2;}
	inline Image *getSpinnerTop() {return m_spinnerTop;}
	inline Image *getSpinnerSpin() {return m_spinnerSpin;}
	inline Image *getSpinnerClear() {return m_spinnerClear;}

	inline Image *getDefaultCursor() {return m_defaultCursor;}
	inline Image *getCursor() {return m_cursor;}
	inline Image *getCursorMiddle() {return m_cursorMiddle;}
	inline Image *getCursorTrail() {return m_cursorTrail;}

	inline OsuSkinImage *getSelectionModEasy() {return m_selectionModEasy;}
	inline OsuSkinImage *getSelectionModNoFail() {return m_selectionModNoFail;}
	inline OsuSkinImage *getSelectionModHalfTime() {return m_selectionModHalfTime;}
	inline OsuSkinImage *getSelectionModDayCore() {return m_selectionModDayCore;}
	inline OsuSkinImage *getSelectionModHardRock() {return m_selectionModHardRock;}
	inline OsuSkinImage *getSelectionModSuddenDeath() {return m_selectionModSuddenDeath;}
	inline OsuSkinImage *getSelectionModPerfect() {return m_selectionModPerfect;}
	inline OsuSkinImage *getSelectionModDoubleTime() {return m_selectionModDoubleTime;}
	inline OsuSkinImage *getSelectionModNightCore() {return m_selectionModNightCore;}
	inline OsuSkinImage *getSelectionModHidden() {return m_selectionModHidden;}
	inline OsuSkinImage *getSelectionModFlashlight() {return m_selectionModFlashlight;}
	inline OsuSkinImage *getSelectionModRelax() {return m_selectionModRelax;}
	inline OsuSkinImage *getSelectionModAutopilot() {return m_selectionModAutopilot;}
	inline OsuSkinImage *getSelectionModSpunOut() {return m_selectionModSpunOut;}
	inline OsuSkinImage *getSelectionModAutoplay() {return m_selectionModAutoplay;}
	inline OsuSkinImage *getSelectionModNightmare() {return m_selectionModNightmare;}
	inline OsuSkinImage *getSelectionModTarget() {return m_selectionModTarget;}
	inline OsuSkinImage *getSelectionModScorev2() {return m_selectionModScorev2;}

	inline Image *getPauseContinue() {return m_pauseContinue;}
	inline Image *getPauseRetry() {return m_pauseRetry;}
	inline Image *getPauseBack() {return m_pauseBack;}
	inline Image *getPauseOverlay() {return m_pauseOverlay;}
	inline Image *getUnpause() {return m_unpause;}

	inline Image *getButtonLeft() {return m_buttonLeft;}
	inline Image *getButtonMiddle() {return m_buttonMiddle;}
	inline Image *getButtonRight() {return m_buttonRight;}
	inline Image *getDefaultButtonLeft() {return m_defaultButtonLeft;}
	inline Image *getDefaultButtonMiddle() {return m_defaultButtonMiddle;}
	inline Image *getDefaultButtonRight() {return m_defaultButtonRight;}
	inline OsuSkinImage *getMenuBack2() {return m_menuBack;}
	inline Image *getSelectionMode() {return m_selectionMode;}
	inline Image *getSelectionModeOver() {return m_selectionModeOver;}
	inline Image *getSelectionMods() {return m_selectionMods;}
	inline Image *getSelectionModsOver() {return m_selectionModsOver;}
	inline Image *getSelectionRandom() {return m_selectionRandom;}
	inline Image *getSelectionRandomOver() {return m_selectionRandomOver;}
	inline Image *getSelectionOptions() {return m_selectionOptions;}
	inline Image *getSelectionOptionsOver() {return m_selectionOptionsOver;}

	inline Image *getSongSelectTop() {return m_songSelectTop;}
	inline Image *getSongSelectBottom() {return m_songSelectBottom;}
	inline Image *getMenuButtonBackground() {return m_menuButtonBackground;}
	inline OsuSkinImage *getMenuButtonBackground2() {return m_menuButtonBackground2;}
	inline Image *getStar() {return m_star;}
	inline Image *getRankingPanel() {return m_rankingPanel;}
	inline Image *getRankingGraph() {return m_rankingGraph;}
	inline Image *getRankingTitle() {return m_rankingTitle;}
	inline Image *getRankingMaxCombo() {return m_rankingMaxCombo;}
	inline Image *getRankingAccuracy() {return m_rankingAccuracy;}
	inline Image *getRankingA() {return m_rankingA;}
	inline Image *getRankingB() {return m_rankingB;}
	inline Image *getRankingC() {return m_rankingC;}
	inline Image *getRankingD() {return m_rankingD;}
	inline Image *getRankingS() {return m_rankingS;}
	inline Image *getRankingSH() {return m_rankingSH;}
	inline Image *getRankingX() {return m_rankingX;}
	inline Image *getRankingXH() {return m_rankingXH;}
	inline OsuSkinImage *getRankingAsmall() {return m_rankingAsmall;}
	inline OsuSkinImage *getRankingBsmall() {return m_rankingBsmall;}
	inline OsuSkinImage *getRankingCsmall() {return m_rankingCsmall;}
	inline OsuSkinImage *getRankingDsmall() {return m_rankingDsmall;}
	inline OsuSkinImage *getRankingSsmall() {return m_rankingSsmall;}
	inline OsuSkinImage *getRankingSHsmall() {return m_rankingSHsmall;}
	inline OsuSkinImage *getRankingXsmall() {return m_rankingXsmall;}
	inline OsuSkinImage *getRankingXHsmall() {return m_rankingXHsmall;}

	inline Image *getBeatmapImportSpinner() {return m_beatmapImportSpinner;}
	inline Image *getLoadingSpinner() {return m_loadingSpinner;}
	inline Image *getCircleEmpty() {return m_circleEmpty;}
	inline Image *getCircleFull() {return m_circleFull;}
	inline Image *getSeekTriangle() {return m_seekTriangle;}

	inline Sound *getSpinnerBonus() {return m_spinnerBonus;}
	inline Sound *getSpinnerSpinSound() {return m_spinnerSpinSound;}

	inline Sound *getCombobreak() {return m_combobreak;}
	inline Sound *getFailsound() {return m_failsound;}
	inline Sound *getApplause() {return m_applause;}
	inline Sound *getMenuHit() {return m_menuHit;}
	inline Sound *getMenuClick() {return m_menuClick;}
	inline Sound *getCheckOn() {return m_checkOn;}
	inline Sound *getCheckOff() {return m_checkOff;}
	inline Sound *getShutter() {return m_shutter;}

	inline bool isCursor2x() {return m_bCursor2x;}
	inline bool isCursorTrail2x() {return m_bCursorTrail2x;}
	inline bool isApproachCircle2x() {return m_bApproachCircle2x;}
	inline bool isReverseArrow2x() {return m_bReverseArrow2x;}
	inline bool isHitCircle2x() {return m_bHitCircle2x;}
	inline bool isDefault02x() {return m_bIsDefault02x;}
	inline bool isDefault12x() {return m_bIsDefault12x;}
	inline bool isSpinnerApproachCircle2x() {return m_bSpinnerApproachCircle2x;}
	inline bool isSpinnerBottom2x() {return m_bSpinnerBottom2x;}
	inline bool isSpinnerCircle2x() {return m_bSpinnerCircle2x;}
	inline bool isSpinnerTop2x() {return m_bSpinnerTop2x;}
	inline bool isSpinnerMiddle2x() {return m_bSpinnerMiddle2x;}
	inline bool isSpinnerMiddle22x() {return m_bSpinnerMiddle22x;}
	inline bool isSliderScorePoint2x() {return m_bSliderScorePoint2x;}
	inline bool isSliderStartCircle2x() {return m_bSliderStartCircle2x;}
	inline bool isSliderEndCircle2x() {return m_bSliderEndCircle2x;}

	inline bool isCircularmetre2x() {return m_bCircularmetre2x;}

	inline bool isPauseContinue2x() {return m_bPauseContinue2x;}

	inline bool isMenuButtonBackground2x() {return m_bMenuButtonBackground2x;}
	inline bool isStar2x() {return m_bStar2x;}
	inline bool isRankingPanel2x() {return m_bRankingPanel2x;}
	inline bool isRankingMaxCombo2x() {return m_bRankingMaxCombo2x;}
	inline bool isRankingAccuracy2x() {return m_bRankingAccuracy2x;}
	inline bool isRankingA2x() {return m_bRankingA2x;}
	inline bool isRankingB2x() {return m_bRankingB2x;}
	inline bool isRankingC2x() {return m_bRankingC2x;}
	inline bool isRankingD2x() {return m_bRankingD2x;}
	inline bool isRankingS2x() {return m_bRankingS2x;}
	inline bool isRankingSH2x() {return m_bRankingSH2x;}
	inline bool isRankingX2x() {return m_bRankingX2x;}
	inline bool isRankingXH2x() {return m_bRankingXH2x;}

	// skin.ini
	inline float getVersion() {return m_fVersion;}
	inline float getAnimationFramerate() {return m_fAnimationFramerate;}
	Color getComboColorForCounter(int i, int offset);
	void setBeatmapComboColors(std::vector<Color> colors);
	inline Color getSpinnerApproachCircleColor() {return m_spinnerApproachCircleColor;}
	inline Color getSliderBorderColor() {return m_sliderBorderColor;}
	inline Color getSliderTrackOverride() {return m_sliderTrackOverride;}
	inline Color getSliderBallColor() {return m_sliderBallColor;}

	inline Color getSongSelectActiveText() {return m_songSelectActiveText;}
	inline Color getSongSelectInactiveText() {return m_songSelectInactiveText;}

	inline Color getInputOverlayText() {return m_inputOverlayText;}

	inline bool getCursorCenter() {return m_bCursorCenter;}
	inline bool getCursorRotate() {return m_bCursorRotate;}
	inline bool getCursorExpand() {return m_bCursorExpand;}

	inline bool getSliderBallFlip() {return m_bSliderBallFlip;}
	inline bool getAllowSliderBallTint() {return m_bAllowSliderBallTint;}
	inline int getSliderStyle() {return m_iSliderStyle;}
	inline bool getHitCircleOverlayAboveNumber() {return m_bHitCircleOverlayAboveNumber;}
	inline bool isSliderTrackOverridden() {return m_bSliderTrackOverride;}

	inline UString getComboPrefix() {return m_sComboPrefix;}
	inline int getComboOverlap() {return m_iComboOverlap;}

	inline UString getScorePrefix() {return m_sScorePrefix;}
	inline int getScoreOverlap() {return m_iScoreOverlap;}

	inline UString getHitCirclePrefix() {return m_sHitCirclePrefix;}
	inline int getHitCircleOverlap() {return m_iHitCircleOverlap;}

	// custom
	inline bool useSmoothCursorTrail() {return m_cursorMiddle != m_missingTexture;}
	inline bool isDefaultSkin() {return m_bIsDefaultSkin;}

private:
	static ConVar *m_osu_skin_ref;

	struct SOUND_SAMPLE
	{
		Sound *sound;
		float hardcodedVolumeMultiplier; // some samples in osu have hardcoded multipliers which can not be modified (i.e. you can NEVER reach 100% volume with them)
	};

	bool parseSkinINI(UString filepath);

	bool compareFilenameWithSkinElementName(UString filename, UString skinElementName);

	OsuSkinImage *createOsuSkinImage(UString skinElementName, Vector2 baseSizeForScaling2x, float osuSize, bool ignoreDefaultSkin = false, UString animationSeparator = "-");
	void checkLoadImage(Image **addressOfPointer, UString skinElementName, UString resourceName, bool ignoreDefaultSkin = false, UString fileExtension = "png", bool forceLoadMipmaps = false);
	void checkLoadSound(Sound **addressOfPointer, UString skinElementName, UString resourceName, bool isOverlayable = false, bool isSample = false, bool loop = false, float hardcodedVolumeMultiplier = -1.0f);

	void onEffectVolumeChange(UString oldValue, UString newValue);

	Osu *m_osu;
	bool m_bIsDefaultSkin;
	UString m_sFilePath;
	std::vector<Resource*> m_resources;
	std::vector<Sound*> m_sounds;
	std::vector<SOUND_SAMPLE> m_soundSamples;

	std::vector<OsuSkinImage*> m_images;

	static Image *m_missingTexture;

	Image *m_hitCircle;
	OsuSkinImage *m_hitCircleOverlay2;
	Image *m_approachCircle;
	Image *m_reverseArrow;
	OsuSkinImage *m_followPoint2;

	Image *m_default0;
	Image *m_default1;
	Image *m_default2;
	Image *m_default3;
	Image *m_default4;
	Image *m_default5;
	Image *m_default6;
	Image *m_default7;
	Image *m_default8;
	Image *m_default9;

	Image *m_score0;
	Image *m_score1;
	Image *m_score2;
	Image *m_score3;
	Image *m_score4;
	Image *m_score5;
	Image *m_score6;
	Image *m_score7;
	Image *m_score8;
	Image *m_score9;
	Image *m_scoreX;
	Image *m_scorePercent;
	Image *m_scoreDot;

	OsuSkinImage *m_playSkip;
	Image *m_playWarningArrow;
	OsuSkinImage *m_playWarningArrow2;
	Image *m_circularmetre;
	OsuSkinImage *m_scorebarBg;
	OsuSkinImage *m_inputoverlayBackground;
	OsuSkinImage *m_inputoverlayKey;

	OsuSkinImage *m_hit0;
	OsuSkinImage *m_hit50;
	OsuSkinImage *m_hit100;
	OsuSkinImage *m_hit100k;
	OsuSkinImage *m_hit300;
	OsuSkinImage *m_hit300g;
	OsuSkinImage *m_hit300k;

	Image *m_sliderGradient;
	OsuSkinImage *m_sliderb;
	OsuSkinImage *m_sliderFollowCircle2;
	Image *m_sliderScorePoint;
	Image *m_sliderStartCircle;
	OsuSkinImage *m_sliderStartCircle2;
	Image *m_sliderStartCircleOverlay;
	OsuSkinImage *m_sliderStartCircleOverlay2;
	Image *m_sliderEndCircle;
	OsuSkinImage *m_sliderEndCircle2;
	Image *m_sliderEndCircleOverlay;
	OsuSkinImage *m_sliderEndCircleOverlay2;

	Image *m_spinnerBackground;
	Image *m_spinnerCircle;
	Image *m_spinnerApproachCircle;
	Image *m_spinnerBottom;
	Image *m_spinnerMiddle;
	Image *m_spinnerMiddle2;
	Image *m_spinnerTop;
	Image *m_spinnerSpin;
	Image *m_spinnerClear;

	Image *m_defaultCursor;
	Image *m_cursor;
	Image *m_cursorMiddle;
	Image *m_cursorTrail;

	OsuSkinImage *m_selectionModEasy;
	OsuSkinImage *m_selectionModNoFail;
	OsuSkinImage *m_selectionModHalfTime;
	OsuSkinImage *m_selectionModDayCore;
	OsuSkinImage *m_selectionModHardRock;
	OsuSkinImage *m_selectionModSuddenDeath;
	OsuSkinImage *m_selectionModPerfect;
	OsuSkinImage *m_selectionModDoubleTime;
	OsuSkinImage *m_selectionModNightCore;
	OsuSkinImage *m_selectionModHidden;
	OsuSkinImage *m_selectionModFlashlight;
	OsuSkinImage *m_selectionModRelax;
	OsuSkinImage *m_selectionModAutopilot;
	OsuSkinImage *m_selectionModSpunOut;
	OsuSkinImage *m_selectionModAutoplay;
	OsuSkinImage *m_selectionModNightmare;
	OsuSkinImage *m_selectionModTarget;
	OsuSkinImage *m_selectionModScorev2;

	Image *m_pauseContinue;
	Image *m_pauseRetry;
	Image *m_pauseBack;
	Image *m_pauseOverlay;
	Image *m_unpause;

	Image *m_buttonLeft;
	Image *m_buttonMiddle;
	Image *m_buttonRight;
	Image *m_defaultButtonLeft;
	Image *m_defaultButtonMiddle;
	Image *m_defaultButtonRight;
	OsuSkinImage *m_menuBack;
	Image *m_selectionMode;
	Image *m_selectionModeOver;
	Image *m_selectionMods;
	Image *m_selectionModsOver;
	Image *m_selectionRandom;
	Image *m_selectionRandomOver;
	Image *m_selectionOptions;
	Image *m_selectionOptionsOver;

	Image *m_songSelectTop;
	Image *m_songSelectBottom;
	Image *m_menuButtonBackground;
	OsuSkinImage *m_menuButtonBackground2;
	Image *m_star;
	Image *m_rankingPanel;
	Image *m_rankingGraph;
	Image *m_rankingTitle;
	Image *m_rankingMaxCombo;
	Image *m_rankingAccuracy;
	Image *m_rankingA;
	Image *m_rankingB;
	Image *m_rankingC;
	Image *m_rankingD;
	Image *m_rankingS;
	Image *m_rankingSH;
	Image *m_rankingX;
	Image *m_rankingXH;
	OsuSkinImage *m_rankingAsmall;
	OsuSkinImage *m_rankingBsmall;
	OsuSkinImage *m_rankingCsmall;
	OsuSkinImage *m_rankingDsmall;
	OsuSkinImage *m_rankingSsmall;
	OsuSkinImage *m_rankingSHsmall;
	OsuSkinImage *m_rankingXsmall;
	OsuSkinImage *m_rankingXHsmall;

	Image *m_beatmapImportSpinner;
	Image *m_loadingSpinner;
	Image *m_circleEmpty;
	Image *m_circleFull;
	Image *m_seekTriangle;

	Sound *m_normalHitNormal;
	Sound *m_normalHitWhistle;
	Sound *m_normalHitFinish;
	Sound *m_normalHitClap;
	Sound *m_normalSliderTick;
	Sound *m_softHitNormal;
	Sound *m_softHitWhistle;
	Sound *m_softHitFinish;
	Sound *m_softHitClap;
	Sound *m_softSliderTick;
	Sound *m_drumHitNormal;
	Sound *m_drumHitWhistle;
	Sound *m_drumHitFinish;
	Sound *m_drumHitClap;
	Sound *m_drumSliderTick;
	Sound *m_spinnerBonus;
	Sound *m_spinnerSpinSound;

	Sound *m_combobreak;
	Sound *m_failsound;
	Sound *m_applause;
	Sound *m_menuHit;
	Sound *m_menuClick;
	Sound *m_checkOn;
	Sound *m_checkOff;
	Sound *m_shutter;

	std::vector<Color> m_comboColors;
	std::vector<Color> m_beatmapComboColors;
	Color m_spinnerApproachCircleColor;
	Color m_sliderBorderColor;
	Color m_sliderTrackOverride;
	Color m_sliderBallColor;

	Color m_songSelectInactiveText;
	Color m_songSelectActiveText;

	Color m_inputOverlayText;


	// scaling
	bool m_bCursor2x;
	bool m_bCursorTrail2x;
	bool m_bApproachCircle2x;
	bool m_bReverseArrow2x;
	bool m_bHitCircle2x;
	bool m_bIsDefault02x;
	bool m_bIsDefault12x;
	bool m_bSpinnerApproachCircle2x;
	bool m_bSpinnerBottom2x;
	bool m_bSpinnerCircle2x;
	bool m_bSpinnerTop2x;
	bool m_bSpinnerMiddle2x;
	bool m_bSpinnerMiddle22x;
	bool m_bSliderScorePoint2x;
	bool m_bSliderStartCircle2x;
	bool m_bSliderEndCircle2x;

	bool m_bCircularmetre2x;

	bool m_bPauseContinue2x;

	bool m_bMenuButtonBackground2x;
	bool m_bStar2x;
	bool m_bRankingPanel2x;
	bool m_bRankingMaxCombo2x;
	bool m_bRankingAccuracy2x;
	bool m_bRankingA2x;
	bool m_bRankingB2x;
	bool m_bRankingC2x;
	bool m_bRankingD2x;
	bool m_bRankingS2x;
	bool m_bRankingSH2x;
	bool m_bRankingX2x;
	bool m_bRankingXH2x;

	// skin.ini
	float m_fVersion;
	float m_fAnimationFramerate;
	bool m_bCursorCenter;
	bool m_bCursorRotate;
	bool m_bCursorExpand;

	bool m_bSliderBallFlip;
	bool m_bAllowSliderBallTint;
	int m_iSliderStyle;
	bool m_bHitCircleOverlayAboveNumber;
	bool m_bSliderTrackOverride;

	UString m_sComboPrefix;
	int m_iComboOverlap;

	UString m_sScorePrefix;
	int m_iScoreOverlap;

	UString m_sHitCirclePrefix;
	int m_iHitCircleOverlap;

	// custom
	int m_iSampleSet;
	int m_iSampleVolume;
};

#endif
