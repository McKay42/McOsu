//================ Copyright (c) 2015, PG, All rights reserved. =================//
//
// Purpose:		slider
//
// $NoKeywords: $slider
//===============================================================================//

#ifndef OSUSLIDER_H
#define OSUSLIDER_H

#include "OsuHitObject.h"

class OsuSliderCurve;

class VertexArrayObject;

class OsuSlider : public OsuHitObject
{
public:
	OsuSlider(char type, int repeat, float pixelLength, std::vector<Vector2> points, std::vector<int> hitSounds, std::vector<float> ticks, float sliderTime, float sliderTimeWithoutRepeats, long time, int sampleType, int comboNumber, int colorCounter, int colorOffset, OsuBeatmapStandard *beatmap);
	virtual ~OsuSlider();

	virtual void draw(Graphics *g);
	virtual void draw2(Graphics *g);
	void draw2(Graphics *g, bool drawApproachCircle, bool drawOnlyApproachCircle);
	virtual void drawVR(Graphics *g, Matrix4 &mvp, OsuVR *vr);
	virtual void drawVR2(Graphics *g, Matrix4 &mvp, OsuVR *vr);
	virtual void update(long curPos);

	virtual void updateStackPosition(float stackOffset);
	virtual int getCombo() {return 2 + std::max((m_iRepeat - 1), 0) + (std::max((m_iRepeat - 1), 0)+1)*m_ticks.size();}

	Vector2 getRawPosAt(long pos);
	Vector2 getOriginalRawPosAt(long pos);
	inline Vector2 getAutoCursorPos(long curPos) {return m_vCurPoint;}

	virtual void onClickEvent(std::vector<OsuBeatmap::CLICK> &clicks);
	virtual void onReset(long curPos);

	void rebuildVertexBuffer();

	inline bool isStartCircleFinished() const {return m_bStartFinished;}
	inline int getRepeat() const {return m_iRepeat;}
	inline std::vector<Vector2> getRawPoints() const {return m_points;}
	inline float getPixelLength() const {return m_fPixelLength;}

	// TEMP: auto cursordance
	struct SLIDERCLICK
	{
		long time;
		bool finished;
		bool successful;
		bool sliderend;
		int type;
		int tickIndex;
	};
	std::vector<SLIDERCLICK> getClicks() {return std::vector<SLIDERCLICK>(m_clicks);}

private:
	static ConVar *m_osu_playfield_mirror_horizontal_ref;
	static ConVar *m_osu_playfield_mirror_vertical_ref;
	static ConVar *m_osu_playfield_rotation_ref;
	static ConVar *m_osu_mod_fps_ref;
	static ConVar *m_osu_slider_border_size_multiplier_ref;
	static ConVar *m_epilepsy;
	static ConVar *m_osu_auto_cursordance;

	void drawStartCircle(Graphics *g, float alpha);
	void drawEndCircle(Graphics *g, float alpha, float sliderSnake = 1.0f);
	void drawBody(Graphics *g, float alpha, float from, float to);
	void drawBodyVR(Graphics *g, OsuVR *vr, Matrix4 &mvp, float alpha, float from, float to);

	void updateAnimations(long curPos);

	void onHit(OsuScore::HIT result, long delta, bool startOrEnd, float targetDelta = 0.0f, float targetAngle = 0.0f);
	void onRepeatHit(bool successful, bool sliderend);
	void onTickHit(bool successful, int tickIndex);
	void onSliderBreak();

	float getT(long pos, bool raw);

	bool isClickHeldSlider(); // special logic to disallow hold tapping

	OsuBeatmapStandard *m_beatmap;

	OsuSliderCurve *m_curve;

	char m_cType;
	int m_iRepeat;
	float m_fPixelLength;
	std::vector<Vector2> m_points;
	std::vector<int> m_hitSounds;
	float m_fSliderTime;
	float m_fSliderTimeWithoutRepeats;

	struct SLIDERTICK
	{
		float percent;
		bool finished;
	};
	std::vector<SLIDERTICK> m_ticks; // ticks (drawing)

	// TEMP: auto cursordance
	std::vector<SLIDERCLICK> m_clicks; // repeats (type 0) + ticks (type 1)

	float m_fSlidePercent;			// 0.0f - 1.0f - 0.0f - 1.0f - etc.
	float m_fActualSlidePercent;	// 0.0f - 1.0f
	float m_fSliderSnakePercent;
	float m_fReverseArrowAlpha;
	float m_fBodyAlpha;

	Vector2 m_vCurPoint;
	Vector2 m_vCurPointRaw;

	OsuScore::HIT m_startResult;
	OsuScore::HIT m_endResult;
	bool m_bStartFinished;
	float m_fStartHitAnimation;
	bool m_bEndFinished;
	float m_fEndHitAnimation;
	float m_fEndSliderBodyFadeAnimation;
	long m_iLastClickHeld;
	int m_iDownKey;
	int m_iPrevSliderSlideSoundSampleSet;
	bool m_bCursorLeft;
	bool m_bCursorInside;
	bool m_bHeldTillEnd;
	bool m_bHeldTillEndForLenienceHack;
	bool m_bHeldTillEndForLenienceHackCheck;
	float m_fFollowCircleTickAnimationScale;
	float m_fFollowCircleAnimationScale;
	float m_fFollowCircleAnimationAlpha;

	int m_iReverseArrowPos;
	int m_iCurRepeat;
	int m_iCurRepeatCounterForHitSounds;
	bool m_bInReverse;
	bool m_bHideNumberAfterFirstRepeatHit;

	// TEMP:
	float m_fSliderBreakRapeTime;

	bool m_bOnHitVRLeftControllerHapticFeedback;

	VertexArrayObject *m_vao;
	VertexArrayObject *m_vaoVR2;
};

#endif
