//================ Copyright (c) 2015, PG, All rights reserved. =================//
//
// Purpose:		base class for all gameplay objects
//
// $NoKeywords: $hitobj
//===============================================================================//

#ifndef OSUHITOBJECT_H
#define OSUHITOBJECT_H

#include "OsuBeatmap.h"

class ConVar;

class OsuBeatmapStandard;

class OsuHitObject
{
public:
	static void drawHitResult(Graphics *g, OsuBeatmapStandard *beatmap, Vector2 rawPos, OsuScore::HIT result, float animPercent, float defaultAnimPercent);
	static void drawHitResult(Graphics *g, OsuSkin *skin, float hitcircleDiameter, float rawHitcircleDiameter, Vector2 rawPos, OsuScore::HIT result, float animPercent, float defaultAnimPercent);

	static ConVar *m_osu_approach_scale_multiplier_ref;
	static ConVar *m_osu_timingpoints_force;
	static ConVar *m_osu_vr_approach_type;
	static ConVar *m_osu_vr_draw_approach_circles;
	static ConVar *m_osu_vr_approach_circles_on_playfield;
	static ConVar *m_osu_vr_approach_circles_on_top;

public:
	OsuHitObject(long time, int sampleType, int comboNumber, int colorCounter, int colorOffset, OsuBeatmap *beatmap);
	virtual ~OsuHitObject() {;}

	virtual void draw(Graphics *g);
	virtual void draw2(Graphics *g){;}
	virtual void drawVR(Graphics *g, Matrix4 &mvp, OsuVR *vr){;}
	virtual void drawVR2(Graphics *g, Matrix4 &mvp, OsuVR *vr){;}
	virtual void update(long curPos);

	virtual void updateStackPosition(float stackOffset) = 0;
	virtual int getCombo() {return 1;} // how much combo this hitobject is "worth"
	virtual bool isCircle() {return false;} // HACKHACK:
	void addHitResult(OsuScore::HIT result, long delta, Vector2 posRaw, float targetDelta = 0.0f, float targetAngle = 0.0f, bool ignoreOnHitErrorBar = false, bool ignoreCombo = false);
	void misAimed() {m_bMisAim = true;}

	void setStack(int stack) {m_iStack = stack;}
	void setForceDrawApproachCircle(bool firstNote) {m_bOverrideHDApproachCircle = firstNote;}
	void setAutopilotDelta(long delta) {m_iAutopilotDelta = delta;}
	void setBlocked(bool blocked) {m_bBlocked = blocked;}

	virtual Vector2 getRawPosAt(long pos) = 0; // with stack calculation modifications
	virtual Vector2 getOriginalRawPosAt(long pos) = 0; // without stack calculations
	virtual Vector2 getAutoCursorPos(long curPos) = 0;

	inline long getTime() const {return m_iTime;}
	inline long getDuration() const {return m_iObjectDuration;}
	inline int getStack() const {return m_iStack;}
	inline int getComboNumber() const {return m_iComboNumber;}
	inline long getAutopilotDelta() const {return m_iAutopilotDelta;}
	inline unsigned long long getSortHack() const {return m_iSortHack;}

	inline bool isVisible() const {return m_bVisible;}
	inline bool isFinished() const {return m_bFinished;}
	inline bool isBlocked() const {return m_bBlocked;}
	inline bool hasMisAimed() const {return m_bMisAim;}

	virtual void onClickEvent(std::vector<OsuBeatmap::CLICK> &clicks) {;}
	virtual void onKeyUpEvent(std::vector<OsuBeatmap::CLICK> &keyUps) {;}
	virtual void onReset(long curPos);

protected:
	OsuBeatmap *m_beatmap;

	bool m_bVisible;
	bool m_bFinished;

	long m_iTime; // the time at which this object must be clicked
	int m_iSampleType;
	int m_iComboNumber;
	int m_iColorCounter;
	int m_iColorOffset;

	float m_fAlpha;
	float m_fAlphaWithoutHidden;
	float m_fAlphaForApproachCircle;
	float m_fApproachScale;
	long m_iDelta; // this must be signed
	long m_iApproachTime;
	long m_iFadeInTime;		// extra time added before the approachTime to let the object smoothly become visible
	long m_iObjectDuration; // how long this object takes to click (circle = 0, slider = sliderTime, spinner = spinnerTime etc.), the object will stay visible this long extra after m_iTime;
							// it should be set by the actual object inheriting from this class

	int m_iStack;

	bool m_bBlocked;
	bool m_bOverrideHDApproachCircle;
	bool m_bMisAim;
	long m_iAutopilotDelta;

private:
	static unsigned long long sortHackCounter;

	struct HITRESULTANIM
	{
		float anim;
		float duration;
		float defaultanim;
		float defaultduration;
		Vector2 rawPos;
		OsuScore::HIT result;
		long delta;
	};

	std::vector<HITRESULTANIM> m_hitResults;

	unsigned long long m_iSortHack;
};

#endif
