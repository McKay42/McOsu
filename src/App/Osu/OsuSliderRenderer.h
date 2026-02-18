//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		static renderer class, so it can be used outside of OsuSlider
//
// $NoKeywords: $sliderrender
//===============================================================================//

#ifndef OSUSLIDERRENDERER_H
#define OSUSLIDERRENDERER_H

#include "cbase.h"

class Shader;
class VertexArrayObject;

class Osu;
class OsuVR;

class OsuSliderRenderer
{
public:
	static Shader *BLEND_SHADER;
	static Shader *BLEND_SHADER_VR;

	static float UNIT_CIRCLE_VAO_DIAMETER;

public:
	static VertexArrayObject *generateVAO(Osu *osu, const std::vector<Vector2> &points, float hitcircleDiameter, Vector3 translation = Vector3(0, 0, 0), bool skipOOBPoints = true);

	static void draw(Graphics *g, Osu *osu, const std::vector<Vector2> &points, const std::vector<Vector2> &alwaysPoints, float hitcircleDiameter, float from = 0.0f, float to = 1.0f, Color undimmedColor = 0xffffffff, float colorRGBMultiplier = 1.0f, float alpha = 1.0f, long sliderTimeForRainbow = 0);
	static void draw(Graphics *g, Osu *osu, VertexArrayObject *vao, Vector4 bounds, const std::vector<Vector2> &alwaysPoints, Vector2 translation, float scale, float hitcircleDiameter, float from = 0.0f, float to = 1.0f, Color undimmedColor = 0xffffffff, float colorRGBMultiplier = 1.0f, float alpha = 1.0f, long sliderTimeForRainbow = 0, bool doEnableRenderTarget = true, bool doDisableRenderTarget = true, bool doDrawSliderFrameBufferToScreen = true);
	static void drawVR(Graphics *g, Osu *osu, OsuVR *vr, Matrix4 &mvp, float approachScale, const std::vector<Vector2> &points, const std::vector<Vector2> &alwaysPoints, float hitcircleDiameter, float from = 0.0f, float to = 1.0f, Color undimmedColor = 0xffffffff, float colorRGBMultiplier = 1.0f, float alpha = 1.0f, long sliderTimeForRainbow = 0);
	static void drawVR(Graphics *g, Osu *osu, OsuVR *vr, Matrix4 &mvp, float approachScale, VertexArrayObject *vao1, VertexArrayObject *vao2, const std::vector<Vector2> &alwaysPoints, float hitcircleDiameter, float from = 0.0f, float to = 1.0f, Color undimmedColor = 0xffffffff, float colorRGBMultiplier = 1.0f, float alpha = 1.0f, long sliderTimeForRainbow = 0);

private:
	static void drawFillSliderBodyPeppy(Graphics *g, Osu *osu, const std::vector<Vector2> &points, VertexArrayObject *circleMesh, float radius, int drawFromIndex, int drawUpToIndex, Shader *shader = NULL);
	static void drawFillSliderBodyPeppyVR(Graphics *g, Osu *osu, OsuVR *vr, Matrix4 &mvp, const std::vector<Vector2> &points, VertexArrayObject *circleMesh, float radius, int drawFromIndex, int drawUpToIndex);
	static void drawFillSliderBodyPeppyVR2(Graphics *g, OsuVR *vr, Matrix4 &mvp, const std::vector<Vector2> &points, VertexArrayObject *circleMesh, float radius, int drawFromIndex, int drawUpToIndex);

	static void checkUpdateVars(Osu *osu, float hitcircleDiameter);

	static void resetRenderTargetBoundingBox();

	// base mesh
	static float MESH_CENTER_HEIGHT;
	static int UNIT_CIRCLE_SUBDIVISIONS;
	static std::vector<float> UNIT_CIRCLE;
	static VertexArrayObject *UNIT_CIRCLE_VAO;
	static VertexArrayObject *UNIT_CIRCLE_VAO_BAKED;
	static VertexArrayObject *UNIT_CIRCLE_VAO_TRIANGLES;

	// tiny rendering optimization for RenderTarget
	static float m_fBoundingBoxMinX;
	static float m_fBoundingBoxMaxX;
	static float m_fBoundingBoxMinY;
	static float m_fBoundingBoxMaxY;
};

#endif
