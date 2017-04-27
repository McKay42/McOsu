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

#include <mutex>
#include "WinMinGW.Mutex.h" // necessary due to incomplete implementation in mingw-w64

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
	};

public:
	OsuBeatmap(Osu *osu);
	virtual ~OsuBeatmap();

	virtual void draw(Graphics *g);
	virtual void drawVR(Graphics *g, Matrix4 &mvp, OsuVR *vr);
	void drawDebug(Graphics *g);
	void drawBackground(Graphics *g);
	virtual void update();

	virtual void onModUpdate() {;} // this should make all the necessary internal updates to hitobjects when legacy osu mods or static mods change live (but also on start)
	virtual bool isLoading(); // allows subclasses to delay the playing start, e.g. to load something

	virtual long getPVS(); // Potentially Visible Set gate time size, for optimizing draw() and update() when iterating over all hitobjects

	// database logic
	void setDifficulties(std::vector<OsuBeatmapDifficulty*> diffs);

	// callbacks called by the Osu class
	void skipEmptySection();
	void keyPressed1();
	void keyPressed2();
	void keyReleased1();
	void keyReleased2();

	// songbrowser logic
	void select(); // loads the music of the currently selected diff and starts playing from the previewTime (e.g. clicking on a beatmap)
	void selectDifficulty(int index, bool deleteImage = true); // DEPRECATED deleteImage parameter
	void selectDifficulty(OsuBeatmapDifficulty *difficulty, bool deleteImage = true); // DEPRECATED deleteImage parameter
	void deselect(bool deleteImages = true); // stops + unloads the currently loaded music and deletes all hitobjects // DEPRECATED deleteImages parameter
	bool play();
	void restart(bool quick = false);
	void pause(bool quitIfWaiting = true);
	void pausePreviewMusic();
	void stop(bool quit = true);
	void fail();

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
	float getSpeedMultiplier();
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

	// player
	inline float getHealth() {return m_fHealth;}
	inline bool hasFailed() {return m_bFailed;}

	// generic
	inline OsuBeatmapDifficulty *getSelectedDifficulty() {return m_selectedDifficulty;}
	inline std::vector<OsuBeatmapDifficulty*> getDifficulties() {return m_difficulties;}
	inline std::vector<OsuBeatmapDifficulty*> *getDifficultiesPointer() {return &m_difficulties;}
	inline int getNumDifficulties() {return m_difficulties.size();}

	inline bool isPlaying() {return m_bIsPlaying;}
	inline bool isPaused() {return m_bIsPaused;}
	inline bool isContinueScheduled() {return m_bContinueScheduled;}
	inline bool isWaiting() {return m_bIsWaiting;}
	inline bool isInSkippableSection() {return m_bIsInSkippableSection;}
	inline bool shouldFlashWarningArrows() {return m_bShouldFlashWarningArrows;}
	bool isClickHeld();

	UString getTitle();
	UString getArtist();

	// OsuHitObject and other helper functions
	void consumeClickEvent();
	void addHitResult(OsuScore::HIT hit, long delta, bool ignoreOnHitErrorBar = false, bool hitErrorBarOnly = false, bool ignoreCombo = false, bool ignoreScore = false);
	void addSliderBreak();
	void addScorePoints(int points);
	void addHealth(float percent);
	void playMissSound();

protected:
	static ConVar *m_osu_pvs;
	static ConVar *m_osu_draw_hitobjects_ref;
	static ConVar *m_osu_vr_draw_desktop_playfield_ref;
	static ConVar *m_osu_followpoints_prevfadetime_ref;
	static ConVar *m_osu_global_offset_ref;
	static ConVar *m_osu_early_note_time_ref;
	static ConVar *m_osu_fail_time_ref;

	static ConVar *m_osu_volume_music_ref;
	static ConVar *m_osu_speed_override_ref;
	static ConVar *m_osu_pitch_override_ref;

	// overridable child events
	virtual void onBeforeLoad() {;} // called before hitobjects are loaded
	virtual void onLoad() {;} // called after hitobjects have been loaded
	virtual void onPlayStart() {;} // called when the player starts playing (everything has been loaded, including the music)
	virtual void onBeforeStop(bool quit) {;} // called before hitobjects are unloaded (quit = don't display ranking screen)
	virtual void onStop(bool quit) {;} // called after hitobjects have been unloaded, but before Osu::onPlayEnd() (quit = don't display ranking screen)
	virtual void onPaused() {;}

	bool canDraw();
	bool canUpdate();

	void actualRestart();

	void handlePreviewPlay();
	void loadMusic(bool stream = true);
	void unloadMusic();
	void unloadDiffs();
	void unloadHitObjects();
	void resetHitObjects(long curPos = 0);
	void resetScore();

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
	int m_iSelectedDifficulty;
	float m_fWaitTime;
	bool m_bContinueScheduled;
	unsigned long m_iContinueMusicPos;

	std::vector<OsuBeatmapDifficulty*> m_difficulties;
	OsuBeatmapDifficulty *m_selectedDifficulty;
	Sound *m_music;

	// sound
	long m_iCurMusicPos;
	long m_iCurMusicPosWithOffsets;
	bool m_bWasSeekFrame;
	double m_fInterpolatedMusicPos;
	double m_fLastAudioTimeAccurateSet;
	double m_fLastRealTimeForInterpolationDelta;
	int m_iResourceLoadUpdateDelayHack;

	// gameplay & state
	bool m_bFailed;
	float m_fFailTime;
	float m_fHealth;
	float m_fHealthReal;
	float m_fBreakBackgroundFade;
	bool m_bInBreak;
	OsuHitObject *m_currentHitObject;
	long m_iNextHitObjectTime;
	long m_iPreviousHitObjectTime;

	bool m_bClick1Held;
	bool m_bClick2Held;
	bool m_bPrevKeyWasKey1;
	std::vector<CLICK> m_clicks;
	std::mutex m_clicksMutex;

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
