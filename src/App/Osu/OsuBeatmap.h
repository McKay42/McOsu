//================ Copyright (c) 2015, PG, All rights reserved. =================//
//
// Purpose:		beatmap loader
//
// $NoKeywords: $osubm
//===============================================================================//

#ifndef OSUBEATMAP_H
#define OSUBEATMAP_H

#include "cbase.h"

#include "OsuScore.h"
#include "OsuDatabaseBeatmap.h"

class Sound;
class ConVar;

class Osu;
class OsuVR;
class OsuSkin;
class OsuHitObject;

class OsuDatabaseBeatmap;

class OsuBackgroundStarCacheLoader;
class OsuBackgroundStarCalcHandler;

class OsuBeatmap
{
public:
	struct CLICK
	{
		long musicPos;
		int maniaColumn;
	};

public:
	OsuBeatmap(Osu *osu);
	virtual ~OsuBeatmap();

	virtual void draw(Graphics *g);
	virtual void drawInt(Graphics *g);
	virtual void drawVR(Graphics *g, Matrix4 &mvp, OsuVR *vr);
	virtual void draw3D(Graphics *g);
	virtual void draw3D2(Graphics *g);
	void drawDebug(Graphics *g);
	void drawBackground(Graphics *g);
	virtual void update();

	virtual void onKeyDown(KeyboardEvent &e);
	virtual void onKeyUp(KeyboardEvent &e);

	virtual void onModUpdate() {;}	// this should make all the necessary internal updates to hitobjects when legacy osu mods or static mods change live (but also on start)
	virtual bool isLoading();		// allows subclasses to delay the playing start, e.g. to load something

	virtual long getPVS();			// Potentially Visible Set gate time size, for optimizing draw() and update() when iterating over all hitobjects

	// callbacks called by the Osu class (osu!standard)
	void skipEmptySection();
	void keyPressed1(bool mouse);
	void keyPressed2(bool mouse);
	void keyReleased1(bool mouse);
	void keyReleased2(bool mouse);

	// songbrowser & player logic
	void select(); // loads the music of the currently selected diff and starts playing from the previewTime (e.g. clicking on a beatmap)
	void selectDifficulty2(OsuDatabaseBeatmap *difficulty2);
	void deselect(); // stops + unloads the currently loaded music and deletes all hitobjects
	bool play();
	void restart(bool quick = false);
	void pause(bool quitIfWaiting = true);
	void pausePreviewMusic(bool toggle = true);
	bool isPreviewMusicPlaying();
	void stop(bool quit = true);
	void fail();
	void cancelFailing();
	void resetScore() {resetScoreInt();}

	// loader
	void setMaxPossibleCombo(int maxPossibleCombo) {m_iMaxPossibleCombo = maxPossibleCombo;}
	void setScoreV2ComboPortionMaximum(unsigned long long scoreV2ComboPortionMaximum) {m_iScoreV2ComboPortionMaximum = scoreV2ComboPortionMaximum;}

	// music/sound
	void unloadMusic() {unloadMusicInt();}
	void setVolume(float volume);
	void setSpeed(float speed);
	void setPitch(float pitch);
	void seekPercent(double percent);
	void seekPercentPlayable(double percent);
	void seekMS(unsigned long ms);

	inline Sound *getMusic() const {return m_music;}
	unsigned long getTime() const;
	unsigned long getStartTimePlayable() const;
	unsigned long getLength() const;
	unsigned long getLengthPlayable() const;
	float getPercentFinished() const;
	float getPercentFinishedPlayable() const;

	// live statistics
	int getMostCommonBPM() const;
	float getSpeedMultiplier() const;
	inline int getNPS() const {return m_iNPS;}
	inline int getND() const {return m_iND;}
	inline int getHitObjectIndexForCurrentTime() const {return m_iCurrentHitObjectIndex;}
	inline int getNumCirclesForCurrentTime() const {return m_iCurrentNumCircles;}
	inline int getNumSlidersForCurrentTime() const {return m_iCurrentNumSliders;}
	inline int getNumSpinnersForCurrentTime() const {return m_iCurrentNumSpinners;}
	inline int getMaxPossibleCombo() const {return m_iMaxPossibleCombo;}
	inline unsigned long long getScoreV2ComboPortionMaximum() const {return m_iScoreV2ComboPortionMaximum;}
	inline double getAimStarsForUpToHitObjectIndex(int upToHitObjectIndex) const {return (m_aimStarsForNumHitObjects.size() > 0 && upToHitObjectIndex > -1 ? m_aimStarsForNumHitObjects[clamp<int>(upToHitObjectIndex, 0, m_aimStarsForNumHitObjects.size()-1)] : 0);}
	inline double getAimSliderFactorForUpToHitObjectIndex(int upToHitObjectIndex) const {return (m_aimSliderFactorForNumHitObjects.size() > 0 && upToHitObjectIndex > -1 ? m_aimSliderFactorForNumHitObjects[clamp<int>(upToHitObjectIndex, 0, m_aimSliderFactorForNumHitObjects.size()-1)] : 0);}
	inline double getSpeedStarsForUpToHitObjectIndex(int upToHitObjectIndex) const {return (m_speedStarsForNumHitObjects.size() > 0 && upToHitObjectIndex > -1 ? m_speedStarsForNumHitObjects[clamp<int>(upToHitObjectIndex, 0, m_speedStarsForNumHitObjects.size()-1)] : 0);}
	inline double getSpeedNotesForUpToHitObjectIndex(int upToHitObjectIndex) const {return (m_speedNotesForNumHitObjects.size() > 0 && upToHitObjectIndex > -1 ? m_speedNotesForNumHitObjects[clamp<int>(upToHitObjectIndex, 0, m_speedNotesForNumHitObjects.size()-1)] : 0);}
	inline const std::vector<double> &getAimStrains() const {return m_aimStrains;}
	inline const std::vector<double> &getSpeedStrains() const {return m_speedStrains;}

	// used by OsuHitObject children and OsuModSelector
	inline Osu *getOsu() const {return m_osu;}
	OsuSkin *getSkin() const; // maybe use this for beatmap skins, maybe
	inline int getRandomSeed() const {return m_iRandomSeed;}

	inline long getCurMusicPos() const {return m_iCurMusicPos;}
	inline long getCurMusicPosWithOffsets() const {return m_iCurMusicPosWithOffsets;}

	float getRawAR() const;
	float getAR() const;
	float getCS() const;
	float getHP() const;
	float getRawOD() const;
	float getOD() const;

	// health
	inline double getHealth() const {return m_fHealth;}
	inline bool hasFailed() const {return m_bFailed;}
	inline double getHPMultiplierNormal() const {return m_fHpMultiplierNormal;}
	inline double getHPMultiplierComboEnd() const {return m_fHpMultiplierComboEnd;}

	// database (legacy)
	inline OsuDatabaseBeatmap *getSelectedDifficulty2() const {return m_selectedDifficulty2;}

	// generic state
	inline bool isPlaying() const {return m_bIsPlaying;}
	inline bool isPaused() const {return m_bIsPaused;}
	inline bool isRestartScheduled() const {return m_bIsRestartScheduled;}
	inline bool isContinueScheduled() const {return m_bContinueScheduled;}
	inline bool isWaiting() const {return m_bIsWaiting;}
	inline bool isInSkippableSection() const {return m_bIsInSkippableSection;}
	inline bool isInBreak() const {return m_bInBreak;}
	inline bool shouldFlashWarningArrows() const {return m_bShouldFlashWarningArrows;}
	inline float shouldFlashSectionPass() const {return m_fShouldFlashSectionPass;}
	inline float shouldFlashSectionFail() const {return m_fShouldFlashSectionFail;}
	bool isClickHeld() const; // is any key currently being held down
	inline bool isKey1Down() const {return m_bClick1Held;}
	inline bool isKey2Down() const {return m_bClick2Held;}
	inline bool isLastKeyDownKey1() const {return m_bPrevKeyWasKey1;}

	UString getTitle() const;
	UString getArtist() const;

	inline const std::vector<OsuDatabaseBeatmap::BREAK> &getBreaks() const {return m_breaks;}
	unsigned long getBreakDurationTotal() const;
	OsuDatabaseBeatmap::BREAK getBreakForTimeRange(long startMS, long positionMS, long endMS) const;

	// OsuHitObject and other helper functions
	OsuScore::HIT addHitResult(OsuHitObject *hitObject, OsuScore::HIT hit, long delta, bool isEndOfCombo = false, bool ignoreOnHitErrorBar = false, bool hitErrorBarOnly = false, bool ignoreCombo = false, bool ignoreScore = false, bool ignoreHealth = false);
	void addSliderBreak();
	void addScorePoints(int points, bool isSpinner = false);
	void addHealth(double percent, bool isFromHitResult);
	void updateTimingPoints(long curPos);

	// ILLEGAL:
	inline const std::vector<OsuHitObject*> &getHitObjectsPointer() const {return m_hitobjects;}
	inline float getBreakBackgroundFadeAnim() const {return m_fBreakBackgroundFade;}

protected:
	static ConVar *m_snd_speed_compensate_pitch_ref;
	static ConVar *m_win_snd_fallback_dsound_ref;

	static ConVar *m_osu_pvs;
	static ConVar *m_osu_draw_hitobjects_ref;
	static ConVar *m_osu_vr_draw_desktop_playfield_ref;
	static ConVar *m_osu_followpoints_prevfadetime_ref;
	static ConVar *m_osu_universal_offset_ref;
	static ConVar *m_osu_early_note_time_ref;
	static ConVar *m_osu_fail_time_ref;
	static ConVar *m_osu_drain_type_ref;

	static ConVar *m_osu_draw_hud_ref;
	static ConVar *m_osu_draw_scorebarbg_ref;
	static ConVar *m_osu_hud_scorebar_hide_during_breaks_ref;
	static ConVar *m_osu_drain_stable_hpbar_maximum_ref;
	static ConVar *m_osu_volume_music_ref;
	static ConVar *m_osu_mod_fposu_ref;
	static ConVar *m_fposu_3d_ref;
	static ConVar *m_fposu_draw_scorebarbg_on_top_ref;

	static ConVar *m_osu_main_menu_shuffle_ref;

	// overridable child events
	virtual void onBeforeLoad() {;}			 // called before hitobjects are loaded
	virtual void onLoad() {;}				 // called after hitobjects have been loaded
	virtual void onPlayStart() {;}			 // called when the player starts playing (everything has been loaded, including the music)
	virtual void onBeforeStop(bool quit) {;} // called before hitobjects are unloaded (quit = don't display ranking screen)
	virtual void onStop(bool quit) {;}		 // called after hitobjects have been unloaded, but before Osu::onPlayEnd() (quit = don't display ranking screen)
	virtual void onPaused(bool first) {;}
	virtual void onUnpaused() {;}
	virtual void onRestart(bool quick) {;}

	// internal
	bool canDraw();
	bool canUpdate();

	void actualRestart();

	void handlePreviewPlay();
	void loadMusic(bool stream = true, bool prescan = false);
	void unloadMusicInt();
	void unloadObjects();

	void resetHitObjects(long curPos = 0);
	void resetScoreInt();

	void playMissSound();

	unsigned long getMusicPositionMSInterpolated();

	Osu *m_osu;

	// beatmap state
	bool m_bIsPlaying;
	bool m_bIsPaused;
	bool m_bIsWaiting;
	bool m_bIsRestartScheduled;
	bool m_bIsRestartScheduledQuick;

	bool m_bIsInSkippableSection;
	bool m_bShouldFlashWarningArrows;
	float m_fShouldFlashSectionPass;
	float m_fShouldFlashSectionFail;
	bool m_bContinueScheduled;
	unsigned long m_iContinueMusicPos;
	float m_fWaitTime;

	// database
	OsuDatabaseBeatmap *m_selectedDifficulty2;

	// sound
	Sound *m_music;
	float m_fMusicFrequencyBackup;
	long m_iCurMusicPos;
	long m_iCurMusicPosWithOffsets;
	bool m_bWasSeekFrame;
	double m_fInterpolatedMusicPos;
	double m_fLastAudioTimeAccurateSet;
	double m_fLastRealTimeForInterpolationDelta;
	int m_iResourceLoadUpdateDelayHack;
	bool m_bForceStreamPlayback;
	float m_fAfterMusicIsFinishedVirtualAudioTimeStart;
	bool m_bIsFirstMissSound;

	// health
	bool m_bFailed;
	float m_fFailAnim;
	double m_fHealth;
	float m_fHealth2;

	// drain
	double m_fDrainRate;
	double m_fHpMultiplierNormal;
	double m_fHpMultiplierComboEnd;

	// breaks
	std::vector<OsuDatabaseBeatmap::BREAK> m_breaks;
	float m_fBreakBackgroundFade;
	bool m_bInBreak;
	OsuHitObject *m_currentHitObject;
	long m_iNextHitObjectTime;
	long m_iPreviousHitObjectTime;
	long m_iPreviousSectionPassFailTime;

	// player input
	bool m_bClick1Held;
	bool m_bClick2Held;
	bool m_bClickedContinue;
	bool m_bPrevKeyWasKey1;
	int m_iAllowAnyNextKeyForFullAlternateUntilHitObjectIndex;
	std::vector<CLICK> m_clicks;
	std::vector<CLICK> m_keyUps;

	// hitobjects
	std::vector<OsuHitObject*> m_hitobjects;
	std::vector<OsuHitObject*> m_hitobjectsSortedByEndTime;
	std::vector<OsuHitObject*> m_misaimObjects;
	int m_iRandomSeed;

	// statistics
	int m_iNPS;
	int m_iND;
	int m_iCurrentHitObjectIndex;
	int m_iCurrentNumCircles;
	int m_iCurrentNumSliders;
	int m_iCurrentNumSpinners;
	int m_iMaxPossibleCombo;
	unsigned long long m_iScoreV2ComboPortionMaximum;
	std::vector<double> m_aimStarsForNumHitObjects;
	std::vector<double> m_aimSliderFactorForNumHitObjects;
	std::vector<double> m_speedStarsForNumHitObjects;
	std::vector<double> m_speedNotesForNumHitObjects;
	std::vector<double> m_aimStrains;
	std::vector<double> m_speedStrains;

	// custom
	int m_iPreviousFollowPointObjectIndex; // TODO: this shouldn't be in this class

private:
	friend class OsuBackgroundStarCacheLoader;
	friend class OsuBackgroundStarCalcHandler;
};

#endif
