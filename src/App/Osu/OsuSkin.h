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

class OsuSkin
{
public:
	OsuSkin(Osu *osu, UString filepath);
	virtual ~OsuSkin();

	void load();
	void loadBeatmapOverride(UString filepath);

	// samples
	void setSampleSet(int sampleSet);
	void setSampleVolume(float volume);

	void playHitCircleSound(int sampleType);
	Sound *getSliderTick();

	// raw
	inline Image *getMissingTexture() {return m_missingTexture;}

	inline Image *getHitCircle() {return m_hitCircle;}
	inline Image *getHitCircleOverlay() {return m_hitCircleOverlay;}
	inline Image *getApproachCircle() {return m_approachCircle;}
	inline Image *getReverseArrow() {return m_reverseArrow;}
	inline Image *getFollowPoint() {return m_followPoint;}

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

	inline Image *getPlaySkip() {return m_playSkip;}
	inline Image *getPlayWarningArrow() {return m_playWarningArrow;}
	inline Image *getCircularmetre() {return m_circularmetre;}

	inline Image *getHit0() {return m_hit0;}
	inline Image *getHit50() {return m_hit50;}
	inline Image *getHit100() {return m_hit100;}

	inline Image *getSliderGradient() {return m_sliderGradient;}
	inline Image *getSliderGradientTrack() {return m_sliderGradientTrack;}
	inline Image *getSliderGradientBody() {return m_sliderGradientBody;}
	inline Image *getSliderb() {return m_sliderb;}
	inline Image *getSliderFollowCircle() {return m_sliderFollowCircle;}
	inline Image *getSliderScorePoint() {return m_sliderScorePoint;}
	inline Image *getSliderStartCircle() {return m_sliderStartCircle;}
	inline Image *getSliderStartCircleOverlay() {return m_sliderStartCircleOverlay;}
	inline Image *getSliderEndCircle() {return m_sliderEndCircle;}
	inline Image *getSliderEndCircleOverlay() {return m_sliderEndCircleOverlay;}

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

	inline Image *getSelectionModEasy() {return m_selectionModEasy;}
	inline Image *getSelectionModNoFail() {return m_selectionModNoFail;}
	inline Image *getSelectionModHalfTime() {return m_selectionModHalfTime;}
	inline Image *getSelectionModHardRock() {return m_selectionModHardRock;}
	inline Image *getSelectionModSuddenDeath() {return m_selectionModSuddenDeath;}
	inline Image *getSelectionModPerfect() {return m_selectionModPerfect;}
	inline Image *getSelectionModDoubleTime() {return m_selectionModDoubleTime;}
	inline Image *getSelectionModNightCore() {return m_selectionModNightCore;}
	inline Image *getSelectionModHidden() {return m_selectionModHidden;}
	inline Image *getSelectionModFlashlight() {return m_selectionModFlashlight;}
	inline Image *getSelectionModRelax() {return m_selectionModRelax;}
	inline Image *getSelectionModAutopilot() {return m_selectionModAutopilot;}
	inline Image *getSelectionModSpunOut() {return m_selectionModSpunOut;}
	inline Image *getSelectionModAutoplay() {return m_selectionModAutoplay;}
	inline Image *getSelectionModNightmare() {return m_selectionModNightmare;}
	inline Image *getSelectionModTarget() {return m_selectionModTarget;}

	inline Image *getPauseContinue() {return m_pauseContinue;}
	inline Image *getPauseRetry() {return m_pauseRetry;}
	inline Image *getPauseBack() {return m_pauseBack;}
	inline Image *getUnpause() {return m_unpause;}

	inline Image *getButtonLeft() {return m_buttonLeft;}
	inline Image *getButtonMiddle() {return m_buttonMiddle;}
	inline Image *getButtonRight() {return m_buttonRight;}
	inline Image *getDefaultButtonLeft() {return m_defaultButtonLeft;}
	inline Image *getDefaultButtonMiddle() {return m_defaultButtonMiddle;}
	inline Image *getDefaultButtonRight() {return m_defaultButtonRight;}
	inline Image *getMenuBack() {return m_menuBack;}
	inline Image *getSelectionMods() {return m_selectionMods;}
	inline Image *getSelectionModsOver() {return m_selectionModsOver;}
	inline Image *getSelectionRandom() {return m_selectionRandom;}
	inline Image *getSelectionRandomOver() {return m_selectionRandomOver;}
	inline Image *getSelectionOptions() {return m_selectionOptions;}
	inline Image *getSelectionOptionsOver() {return m_selectionOptionsOver;}

	inline Image *getSongSelectTop() {return m_songSelectTop;}
	inline Image *getSongSelectBottom() {return m_songSelectBottom;}
	inline Image *getMenuButtonBackground() {return m_menuButtonBackground;}

	inline Image *getBeatmapImportSpinner() {return m_beatmapImportSpinner;}
	inline Image *getLoadingSpinner() {return m_loadingSpinner;}
	inline Image *getCircleEmpty() {return m_circleEmpty;}
	inline Image *getCircleFull() {return m_circleFull;}

	inline Sound *getSpinnerBonus() {return m_spinnerBonus;}
	inline Sound *getSpinnerSpinSound() {return m_spinnerSpinSound;}

	inline Sound *getCombobreak() {return m_combobreak;}
	inline Sound *getApplause() {return m_applause;}
	inline Sound *getMenuHit() {return m_menuHit;}
	inline Sound *getMenuClick() {return m_menuClick;}
	inline Sound *getCheckOn() {return m_checkOn;}
	inline Sound *getCheckOff() {return m_checkOff;}

	inline bool isCursor2x() {return m_bCursor2x;}
	inline bool isApproachCircle2x() {return m_bApproachCircle2x;}
	inline bool isReverseArrow2x() {return m_bReverseArrow2x;}
	inline bool isFollowPoint2x() {return m_bFollowPoint2x;}
	inline bool isHitCircle2x() {return m_bHitCircle2x;}
	inline bool isHitCircleOverlay2x() {return m_bHitCircleOverlay2x;}
	inline bool isDefault02x() {return m_bIsDefault02x;}
	inline bool isDefault12x() {return m_bIsDefault12x;}
	inline bool isHit02x() {return m_bHit02x;}
	inline bool isHit502x() {return m_bHit502x;}
	inline bool isHit1002x() {return m_bHit1002x;}
	inline bool isSpinnerApproachCircle2x() {return m_bSpinnerApproachCircle2x;}
	inline bool isSpinnerBottom2x() {return m_bSpinnerBottom2x;}
	inline bool isSpinnerCircle2x() {return m_bSpinnerCircle2x;}
	inline bool isSpinnerTop2x() {return m_bSpinnerTop2x;}
	inline bool isSpinnerMiddle2x() {return m_bSpinnerMiddle2x;}
	inline bool isSpinnerMiddle22x() {return m_bSpinnerMiddle22x;}
	inline bool isSliderScorePoint2x() {return m_bSliderScorePoint2x;}
	inline bool isSliderStartCircle2x() {return m_bSliderStartCircle2x;}
	inline bool isSliderStartCircleOverlay2x() {return m_bSliderStartCircleOverlay2x;}
	inline bool isSliderEndCircle2x() {return m_bSliderEndCircle2x;}
	inline bool isSliderEndCircleOverlay2x() {return m_bSliderEndCircleOverlay2x;}
	inline bool isSliderFollowCircle2x() {return m_bSliderFollowCircle2x;}

	inline bool isCircularmetre2x() {return m_bCircularmetre2x;}
	inline bool isPlaySkip2x() {return m_bPlaySkip2x;}

	inline bool isPauseContinue2x() {return m_bPauseContinue2x;}

	inline bool isMenuButtonBackground2x() {return m_bMenuButtonBackground2x;}

	// skin.ini
	inline float getVersion() {return m_fVersion;}
	Color getComboColorForCounter(int i);
	void setBeatmapComboColors(std::vector<Color> colors);
	inline Color getSpinnerApproachCircleColor() {return m_spinnerApproachCircleColor;}
	inline Color getSliderBorderColor() {return m_sliderBorderColor;}
	inline Color getSliderTrackOverride() {return m_sliderTrackOverride;}
	inline Color getSliderBallColor() {return m_sliderBallColor;}

	inline Color getSongSelectActiveText() {return m_songSelectActiveText;}
	inline Color getSongSelectInactiveText() {return m_songSelectInactiveText;}

	inline bool getCursorCenter() {return m_bCursorCenter;}
	inline bool getCursorRotate() {return m_bCursorRotate;}
	inline bool getCursorExpand() {return m_bCursorExpand;}

	inline bool getSliderBallFlip() {return m_bSliderBallFlip;}
	inline bool getAllowSliderBallTint() {return m_bAllowSliderBallTint;}
	inline int getSliderStyle() {return m_iSliderStyle;}
	inline bool getHitCircleOverlayAboveNumber() {return m_bHitCircleOverlayAboveNumber;}
	inline bool isSliderTrackOverridden() {return m_bSliderTrackOverride;}

	inline int getHitCircleOverlap() {return m_iHitCircleOverlap;}

private:
	bool parseSkinINI(UString filepath);
	bool compareFilenameWithSkinElementName(UString filename, UString skinElementName);
	void checkLoadImage(Image **addressOfPointer, UString skinElementName, UString resourceName, bool ignoreDefaultSkin = false);
	void checkLoadSound(Sound **addressOfPointer, UString skinElementName, UString resourceName, bool isOverlayable = false, bool isSample = false);

	void onEffectVolumeChange(UString oldValue, UString newValue);

	static ConVar *m_osu_skin_ref;

	Osu *m_osu;
	UString m_sFilePath;
	std::vector<Resource*> m_resources;
	std::vector<Sound*> m_sounds;
	std::vector<Sound*> m_soundSamples;

	static Image *m_missingTexture;

	Image *m_hitCircle;
	Image *m_hitCircleOverlay;
	Image *m_approachCircle;
	Image *m_reverseArrow;
	Image *m_followPoint;

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

	Image *m_playSkip;
	Image *m_playWarningArrow;
	Image *m_circularmetre;

	Image *m_hit0;
	Image *m_hit50;
	Image *m_hit100;

	Image *m_sliderGradient;
	Image *m_sliderGradientTrack;
	Image *m_sliderGradientBody;
	Image *m_sliderb;
	Image *m_sliderFollowCircle;
	Image *m_sliderScorePoint;
	Image *m_sliderStartCircle;
	Image *m_sliderStartCircleOverlay;
	Image *m_sliderEndCircle;
	Image *m_sliderEndCircleOverlay;

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

	Image *m_selectionModEasy;
	Image *m_selectionModNoFail;
	Image *m_selectionModHalfTime;
	Image *m_selectionModHardRock;
	Image *m_selectionModSuddenDeath;
	Image *m_selectionModPerfect;
	Image *m_selectionModDoubleTime;
	Image *m_selectionModNightCore;
	Image *m_selectionModHidden;
	Image *m_selectionModFlashlight;
	Image *m_selectionModRelax;
	Image *m_selectionModAutopilot;
	Image *m_selectionModSpunOut;
	Image *m_selectionModAutoplay;
	Image *m_selectionModNightmare;
	Image *m_selectionModTarget;

	Image *m_pauseContinue;
	Image *m_pauseRetry;
	Image *m_pauseBack;
	Image *m_unpause;

	Image *m_buttonLeft;
	Image *m_buttonMiddle;
	Image *m_buttonRight;
	Image *m_defaultButtonLeft;
	Image *m_defaultButtonMiddle;
	Image *m_defaultButtonRight;
	Image *m_menuBack;
	Image *m_selectionMods;
	Image *m_selectionModsOver;
	Image *m_selectionRandom;
	Image *m_selectionRandomOver;
	Image *m_selectionOptions;
	Image *m_selectionOptionsOver;

	Image *m_songSelectTop;
	Image *m_songSelectBottom;
	Image *m_menuButtonBackground;

	Image *m_beatmapImportSpinner;
	Image *m_loadingSpinner;
	Image *m_circleEmpty;
	Image *m_circleFull;

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
	Sound *m_applause;
	Sound *m_menuHit;
	Sound *m_menuClick;
	Sound *m_checkOn;
	Sound *m_checkOff;

	std::vector<Color> m_comboColors;
	std::vector<Color> m_beatmapComboColors;
	Color m_spinnerApproachCircleColor;
	Color m_sliderBorderColor;
	Color m_sliderTrackOverride;
	Color m_sliderBallColor;

	Color m_songSelectInactiveText;
	Color m_songSelectActiveText;


	// scaling
	bool m_bCursor2x;
	bool m_bApproachCircle2x;
	bool m_bReverseArrow2x;
	bool m_bFollowPoint2x;
	bool m_bHitCircle2x;
	bool m_bHitCircleOverlay2x;
	bool m_bIsDefault02x;
	bool m_bIsDefault12x;
	bool m_bHit02x;
	bool m_bHit502x;
	bool m_bHit1002x;
	bool m_bSpinnerApproachCircle2x;
	bool m_bSpinnerBottom2x;
	bool m_bSpinnerCircle2x;
	bool m_bSpinnerTop2x;
	bool m_bSpinnerMiddle2x;
	bool m_bSpinnerMiddle22x;
	bool m_bSliderScorePoint2x;
	bool m_bSliderStartCircle2x;
	bool m_bSliderStartCircleOverlay2x;
	bool m_bSliderEndCircle2x;
	bool m_bSliderEndCircleOverlay2x;
	bool m_bSliderFollowCircle2x;

	bool m_bCircularmetre2x;
	bool m_bPlaySkip2x;

	bool m_bPauseContinue2x;

	bool m_bMenuButtonBackground2x;

	// skin.ini
	float m_fVersion;
	bool m_bCursorCenter;
	bool m_bCursorRotate;
	bool m_bCursorExpand;

	bool m_bSliderBallFlip;
	bool m_bAllowSliderBallTint;
	int m_iSliderStyle;
	bool m_bHitCircleOverlayAboveNumber;
	bool m_bSliderTrackOverride;

	int m_iHitCircleOverlap;

	// custom
	int m_iSampleSet;
	int m_iSampleVolume;
};

#endif
