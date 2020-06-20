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

class Sound;
class ConVar;

class Osu;
class OsuVR;
class OsuSkin;
class OsuHitObject;
class OsuBeatmapDifficulty;

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
	virtual void drawVR(Graphics *g, Matrix4 &mvp, OsuVR *vr);
	void drawDebug(Graphics *g);
	void drawBackground(Graphics *g);
	virtual void update();

	virtual void onKeyDown(KeyboardEvent &e);
	virtual void onKeyUp(KeyboardEvent &e);

	virtual void onModUpdate() {;} // this should make all the necessary internal updates to hitobjects when legacy osu mods or static mods change live (but also on start)
	virtual bool isLoading(); // allows subclasses to delay the playing start, e.g. to load something

	virtual long getPVS(); // Potentially Visible Set gate time size, for optimizing draw() and update() when iterating over all hitobjects

	// database logic
	void setDifficulties(std::vector<OsuBeatmapDifficulty*> diffs);

	// callbacks called by the Osu class (osu!standard)
	void skipEmptySection();
	void keyPressed1(bool mouse);
	void keyPressed2(bool mouse);
	void keyReleased1(bool mouse);
	void keyReleased2(bool mouse);

	// songbrowser logic
	void select(); // loads the music of the currently selected diff and starts playing from the previewTime (e.g. clicking on a beatmap)
	void selectDifficulty(int index, bool deleteImage = true); // DEPRECATED deleteImage parameter
	void selectDifficulty(OsuBeatmapDifficulty *difficulty, bool deleteImage = true); // DEPRECATED deleteImage parameter
	void deselect(bool deleteImages = true); // stops + unloads the currently loaded music and deletes all hitobjects // DEPRECATED deleteImages parameter
	bool play();
	void restart(bool quick = false);
	void pause(bool quitIfWaiting = true);
	void pausePreviewMusic(bool toggle = true);
	bool isPreviewMusicPlaying();
	void stop(bool quit = true);
	void fail();
	void cancelFailing();

	// music/sound
	void setVolume(float volume);
	void setSpeed(float speed);
	void setPitch(float pitch);
	void seekPercent(double percent);
	void seekPercentPlayable(double percent);
	void seekMS(unsigned long ms);

	inline Sound *getMusic() const {return m_music;}
	unsigned long getTime();
	unsigned long getStartTimePlayable();
	unsigned long getLength();
	unsigned long getLengthPlayable();
	float getPercentFinished();
	float getPercentFinishedPlayable();

	// live statistics
	int getBPM();
	float getSpeedMultiplier() const;
	inline int getNPS() {return m_iNPS;}
	inline int getND() {return m_iND;}
	inline int getHitObjectIndexForCurrentTime() {return m_iCurrentHitObjectIndex;}
	inline int getNumCirclesForCurrentTime() {return m_iCurrentNumCircles;}

	// used by OsuHitObject children and OsuModSelector
	inline Osu *getOsu() const {return m_osu;}
	OsuSkin *getSkin(); // maybe use this for beatmap skins, maybe
	inline long getCurMusicPos() const {return m_iCurMusicPos;}
	inline long getCurMusicPosWithOffsets() const {return m_iCurMusicPosWithOffsets;}

	float getRawAR();
	float getAR();
	float getCS();
	float getHP();
	float getRawOD();
	float getOD();

	// health
	inline double getHealth() {return m_fHealth;}
	inline bool hasFailed() {return m_bFailed;}
	inline double getHPMultiplierNormal() {return m_fHpMultiplierNormal;}
	inline double getHPMultiplierComboEnd() {return m_fHpMultiplierComboEnd;}

	// generic
	inline OsuBeatmapDifficulty *getSelectedDifficulty() {return m_selectedDifficulty;}
	inline const std::vector<OsuBeatmapDifficulty*> &getDifficulties() {return m_difficulties;}

	inline bool isPlaying() {return m_bIsPlaying;}
	inline bool isPaused() {return m_bIsPaused;}
	inline bool isRestartScheduled() {return m_bIsRestartScheduled;}
	inline bool isContinueScheduled() {return m_bContinueScheduled;}
	inline bool isWaiting() {return m_bIsWaiting;}
	inline bool isInSkippableSection() {return m_bIsInSkippableSection;}
	inline bool isInBreak() {return m_bInBreak;}
	inline bool shouldFlashWarningArrows() {return m_bShouldFlashWarningArrows;}
	inline float shouldFlashSectionPass() {return m_fShouldFlashSectionPass;}
	inline float shouldFlashSectionFail() {return m_fShouldFlashSectionFail;}
	bool isClickHeld(); // is any key currently being held down
	inline bool isKey1Down() {return m_bClick1Held;}
	inline bool isKey2Down() {return m_bClick2Held;}
	inline bool isLastKeyDownKey1() {return m_bPrevKeyWasKey1;}

	UString getTitle();
	UString getArtist();

	// OsuHitObject and other helper functions
	OsuScore::HIT addHitResult(OsuHitObject *hitObject, OsuScore::HIT hit, long delta, bool isEndOfCombo = false, bool ignoreOnHitErrorBar = false, bool hitErrorBarOnly = false, bool ignoreCombo = false, bool ignoreScore = false, bool ignoreHealth = false);
	void addSliderBreak();
	void addScorePoints(int points, bool isSpinner = false);
	void addHealth(double percent, bool isFromHitResult);
	void updateTimingPoints(long curPos);

	// ILLEGAL:
	inline const std::vector<OsuHitObject*> &getHitObjectsPointer() {return m_hitobjects;}
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

	static ConVar *m_osu_draw_scorebarbg_ref;
	static ConVar *m_osu_hud_scorebar_hide_during_breaks_ref;
	static ConVar *m_osu_drain_stable_hpbar_maximum_ref;
	static ConVar *m_osu_volume_music_ref;

	// overridable child events
	virtual void onBeforeLoad() {;} // called before hitobjects are loaded
	virtual void onLoad() {;} // called after hitobjects have been loaded
	virtual void onPlayStart() {;} // called when the player starts playing (everything has been loaded, including the music)
	virtual void onBeforeStop(bool quit) {;} // called before hitobjects are unloaded (quit = don't display ranking screen)
	virtual void onStop(bool quit) {;} // called after hitobjects have been unloaded, but before Osu::onPlayEnd() (quit = don't display ranking screen)
	virtual void onPaused(bool first) {;}
	virtual void onUnpaused() {;}
	virtual void onRestart(bool quick) {;}

	// internal
	bool canDraw();
	bool canUpdate();

	void actualRestart();

	void handlePreviewPlay();
	void loadMusic(bool stream = true, bool prescan = false);
	void unloadMusic();
	void unloadDiffs();
	void unloadHitObjects();

	void resetHitObjects(long curPos = 0);
	void resetScore();

	void playMissSound();

	unsigned long getMusicPositionMSInterpolated();

	Osu *m_osu;

	// beatmap
	bool m_bIsPlaying;
	bool m_bIsPaused;
	bool m_bIsWaiting;
	bool m_bIsRestartScheduled;
	bool m_bIsRestartScheduledQuick;

	bool m_bIsInSkippableSection;
	bool m_bShouldFlashWarningArrows;
	float m_fShouldFlashSectionPass;
	float m_fShouldFlashSectionFail;
	int m_iSelectedDifficulty;
	float m_fWaitTime;
	bool m_bContinueScheduled;
	unsigned long m_iContinueMusicPos;

	std::vector<OsuBeatmapDifficulty*> m_difficulties;
	OsuBeatmapDifficulty *m_selectedDifficulty;

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
	float m_fBreakBackgroundFade;
	bool m_bInBreak;
	OsuHitObject *m_currentHitObject;
	long m_iNextHitObjectTime;
	long m_iPreviousHitObjectTime;
	long m_iPreviousSectionPassFailTime;

	// player
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

	// statistics
	int m_iNPS;
	int m_iND;
	int m_iCurrentHitObjectIndex;
	int m_iCurrentNumCircles;

	// custom
	int m_iPreviousFollowPointObjectIndex; // TODO: this shouldn't be in this class
};

#endif
