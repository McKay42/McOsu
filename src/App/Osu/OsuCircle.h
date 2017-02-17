//================ Copyright (c) 2015, PG, All rights reserved. =================//
//
// Purpose:		circle
//
// $NoKeywords: $circle
//===============================================================================//

#ifndef OSUCIRCLE_H
#define OSUCIRCLE_H

#include "OsuHitObject.h"

class OsuCircle : public OsuHitObject
{
public:
	// main
	static void drawApproachCircle(Graphics *g, OsuBeatmap *beatmap, Vector2 rawPos, int number, int colorCounter, float approachScale, float alpha, bool overrideHDApproachCircle = false);
	static void drawCircle(Graphics *g, OsuBeatmap *beatmap, Vector2 rawPos, int number, int colorCounter, float approachScale, float alpha, float numberAlpha, bool drawNumber = true, bool overrideHDApproachCircle = false);
	static void drawCircle(Graphics *g, OsuSkin *skin, Vector2 pos, float hitcircleDiameter, float numberScale, float overlapScale, int number, int colorCounter, float approachScale, float alpha, float numberAlpha, bool drawNumber = true, bool overrideHDApproachCircle = false);
	static void drawCircle(Graphics *g, OsuSkin *skin, Vector2 pos, float hitcircleDiameter, Color color, float alpha = 1.0f);
	static void drawSliderStartCircle(Graphics *g, OsuBeatmap *beatmap, Vector2 rawPos, int number, int colorCounter, float approachScale, float alpha, float numberAlpha, bool drawNumber = true, bool overrideHDApproachCircle = false);
	static void drawSliderStartCircle(Graphics *g, OsuSkin *skin, Vector2 pos, float hitcircleDiameter, float numberScale, float hitcircleOverlapScale, int number, int colorCounter = 0, float approachScale = 1.0f, float alpha = 1.0f, float numberAlpha = 1.0f, bool drawNumber = true, bool overrideHDApproachCircle = false);
	static void drawSliderEndCircle(Graphics *g, OsuBeatmap *beatmap, Vector2 rawPos, int number, int colorCounter, float approachScale, float alpha, float numberAlpha, bool drawNumber = true, bool overrideHDApproachCircle = false);
	static void drawSliderEndCircle(Graphics *g, OsuSkin *skin, Vector2 pos, float hitcircleDiameter, float numberScale, float overlapScale, int number = 0, int colorCounter = 0, float approachScale = 1.0f, float alpha = 1.0f, float numberAlpha = 1.0f, bool drawNumber = true, bool overrideHDApproachCircle = false);

	// split helper functions
	static void drawApproachCircle(Graphics *g, OsuSkin *skin, Vector2 pos, Color comboColor, float hitcircleDiameter, float approachScale, float alpha, bool modHD, bool overrideHDApproachCircle);
	static void drawHitCircleOverlay(Graphics *g, Image *hitCircleOverlayImage, Vector2 pos, float circleOverlayImageScale, float alpha);
	static void drawHitCircle(Graphics *g, Image *hitCircleImage, Vector2 pos, Color comboColor, float circleImageScale, float alpha);
	static void drawHitCircleNumber(Graphics *g, OsuBeatmap *beatmap, Vector2 pos, int number, float numberAlpha);
	static void drawHitCircleNumber(Graphics *g, OsuSkin *skin, float numberScale, float overlapScale, Vector2 pos, int number, float numberAlpha);

public:
	OsuCircle(int x, int y, long time, int sampleType, int comboNumber, int colorCounter, OsuBeatmap *beatmap);
	virtual ~OsuCircle();

	virtual void draw(Graphics *g);
	virtual void draw2(Graphics *g);
	virtual void drawVR(Graphics *g, Matrix4 &mvp, OsuVR *vr);
	virtual void update(long curPos);

	void updateStackPosition(float stackOffset);

	Vector2 getRawPosAt(long pos) {return m_vRawPos;}
	Vector2 getOriginalRawPosAt(long pos) {return m_vOriginalRawPos;}
	Vector2 getAutoCursorPos(long curPos);

	virtual void onClickEvent(Vector2 cursorPos, std::vector<OsuBeatmap::CLICK> &clicks);
	virtual void onReset(long curPos);

private:
	// necessary due to the static draw functions
	static int rainbowNumber;
	static int rainbowColorCounter;

	void onHit(OsuScore::HIT result, long delta, float targetDelta = 0.0f, float targetAngle = 0.0f);

	Vector2 m_vRawPos;
	Vector2 m_vOriginalRawPos; // for live mod changing

	bool m_bWaiting;
	float m_fHitAnimation;
	float m_fShakeAnimation;

	bool m_bOnHitVRLeftControllerHapticFeedback;
};

#endif
