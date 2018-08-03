//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		static renderer class, so it can be used outside of OsuSlider
//
// $NoKeywords: $sliderrender
//===============================================================================//

#include "OsuSliderRenderer.h"

#include "Engine.h"
#include "Shader.h"
#include "VertexArrayObject.h"
#include "RenderTarget.h"
#include "ResourceManager.h"
#include "ConVar.h"

#include "Osu.h"
#include "OsuVR.h"
#include "OsuSkin.h"
#include "OsuGameRules.h"

#include "OpenGLHeaders.h"
#include "OpenGLLegacyInterface.h"
#include "OpenGL3Interface.h"

Shader *OsuSliderRenderer::BLEND_SHADER = NULL;
Shader *OsuSliderRenderer::BLEND_SHADER_VR = NULL;

float OsuSliderRenderer::MESH_CENTER_HEIGHT = 0.5f; // Camera::buildMatrixOrtho2D() uses -1 to 1 for zn/zf, so don't make this too high
int OsuSliderRenderer::UNIT_CIRCLE_SUBDIVIDES = 42;
std::vector<float> OsuSliderRenderer::UNIT_CIRCLE;
VertexArrayObject *OsuSliderRenderer::UNIT_CIRCLE_VAO = NULL;
VertexArrayObject *OsuSliderRenderer::UNIT_CIRCLE_VAO_BAKED = NULL;
VertexArrayObject *OsuSliderRenderer::UNIT_CIRCLE_VAO_TRIANGLES = NULL;
float OsuSliderRenderer::UNIT_CIRCLE_VAO_RADIUS = 0.0f;

float OsuSliderRenderer::m_fBoundingBoxMinX = std::numeric_limits<float>::max();
float OsuSliderRenderer::m_fBoundingBoxMaxX = 0.0f;
float OsuSliderRenderer::m_fBoundingBoxMinY = std::numeric_limits<float>::max();
float OsuSliderRenderer::m_fBoundingBoxMaxY = 0.0f;

ConVar osu_slider_debug("osu_slider_debug", false);
ConVar osu_slider_debug_wireframe("osu_slider_debug_wireframe", false);
ConVar osu_slider_debug_draw_caps("osu_slider_debug_draw_caps", true);
ConVar osu_slider_rainbow("osu_slider_rainbow", false);
ConVar osu_slider_use_gradient_image("osu_slider_use_gradient_image", false);

ConVar osu_slider_alpha_multiplier("osu_slider_alpha_multiplier", 1.0f);
ConVar osu_slider_body_alpha_multiplier("osu_slider_body_alpha_multiplier", 1.0f);
ConVar osu_slider_body_color_saturation("osu_slider_body_color_saturation", 1.0f);
ConVar osu_slider_border_size_multiplier("osu_slider_border_size_multiplier", 1.0f);
ConVar osu_slider_border_tint_combo_color("osu_slider_border_tint_combo_color", false);
ConVar osu_slider_osu_next_style("osu_slider_osu_next_style", false);

VertexArrayObject *OsuSliderRenderer::generateVAO(Osu *osu, const std::vector<Vector2> &points, float hitcircleDiameter, Vector3 translation)
{
	engine->getResourceManager()->requestNextLoadUnmanaged();
	VertexArrayObject *vao = engine->getResourceManager()->createVertexArrayObject();

	checkUpdateVars(osu, hitcircleDiameter);

	bool isValidMesh = false;
	for (int i=0; i<points.size(); ++i)
	{
		// fuck oob sliders
		if (points[i].x < -hitcircleDiameter-OsuGameRules::OSU_COORD_WIDTH*2 || points[i].x > osu->getScreenWidth()+hitcircleDiameter+OsuGameRules::OSU_COORD_WIDTH*2 || points[i].y < -hitcircleDiameter-OsuGameRules::OSU_COORD_HEIGHT*2 || points[i].y > osu->getScreenHeight()+hitcircleDiameter+OsuGameRules::OSU_COORD_HEIGHT*2)
			continue;

		std::vector<Vector3> meshVertices = UNIT_CIRCLE_VAO_TRIANGLES->getVertices();
		std::vector<std::vector<Vector2>> meshTexCoords = UNIT_CIRCLE_VAO_TRIANGLES->getTexcoords();
		for (int v=0; v<meshVertices.size(); v++)
		{
			vao->addVertex(meshVertices[v] + Vector3(points[i].x, points[i].y, 0) + translation);
			vao->addTexcoord(meshTexCoords[0][v]);

			isValidMesh = true;
		}
	}

	if (isValidMesh)
		engine->getResourceManager()->loadResource(vao);
	else
		debugLog("OsuSliderRenderer::generateSliderVAO() ERROR: Zero triangles!\n");

	return vao;
}

void OsuSliderRenderer::draw(Graphics *g, Osu *osu, const std::vector<Vector2> &points, const std::vector<Vector2> &alwaysPoints, float hitcircleDiameter, float from, float to, Color color, float alpha, long sliderTimeForRainbow)
{
	if (osu_slider_alpha_multiplier.getFloat() <= 0.0f || alpha <= 0.0f)
		return;

	checkUpdateVars(osu, hitcircleDiameter);

	const int drawFromIndex = clamp<int>((int)std::round(points.size() * from), 0, points.size());
	const int drawUpToIndex = clamp<int>((int)std::round(points.size() * to), 0, points.size());

	// debug sliders
	if (osu_slider_debug.getBool())
	{
		const float circleImageScale = hitcircleDiameter / (float) osu->getSkin()->getHitCircle()->getWidth();

		g->setColor(color);
		g->setAlpha(alpha);
		for (int i=drawFromIndex; i<drawUpToIndex; i++)
		{
			VertexArrayObject vao(Graphics::PRIMITIVE::PRIMITIVE_QUADS);
			Vector2 point = points[i];

			int width = osu->getSkin()->getHitCircle()->getWidth();
			int height = osu->getSkin()->getHitCircle()->getHeight();

			g->pushTransform();
				g->scale(circleImageScale, circleImageScale);
				g->translate(point.x, point.y);

				int x = -width/2;
				int y = -height/2;

				vao.addTexcoord(0, 0);
				vao.addVertex(x, y, -1.0f);

				vao.addTexcoord(0, 1);
				vao.addVertex(x, y+height, -1.0f);

				vao.addTexcoord(1, 1);
				vao.addVertex(x+width, y+height, -1.0f);

				vao.addTexcoord(1, 0);
				vao.addVertex(x+width, y, -1.0f);

				osu->getSkin()->getHitCircle()->bind();
				g->drawVAO(&vao);
				osu->getSkin()->getHitCircle()->unbind();
			g->popTransform();
		}
		return; // nothing more to draw here
	}

	// reset
	resetRenderTargetBoundingBox();

	// draw entire slider into framebuffer
	g->setDepthBuffer(true);
	g->setBlending(false);
	{
		osu->getSliderFrameBuffer()->enable();

		Color borderColor = osu_slider_border_tint_combo_color.getBool() ? color : osu->getSkin()->getSliderBorderColor();
		Color bodyColor = osu->getSkin()->isSliderTrackOverridden() ? osu->getSkin()->getSliderTrackOverride() : color;

		if (osu_slider_rainbow.getBool())
		{
			float frequency = 0.3f;
			float time = engine->getTime()*20;

			char red1	= std::sin(frequency*time + 0 + sliderTimeForRainbow) * 127 + 128;
			char green1	= std::sin(frequency*time + 2 + sliderTimeForRainbow) * 127 + 128;
			char blue1	= std::sin(frequency*time + 4 + sliderTimeForRainbow) * 127 + 128;

			char red2	= std::sin(frequency*time*1.5f + 0 + sliderTimeForRainbow) * 127 + 128;
			char green2	= std::sin(frequency*time*1.5f + 2 + sliderTimeForRainbow) * 127 + 128;
			char blue2	= std::sin(frequency*time*1.5f + 4 + sliderTimeForRainbow) * 127 + 128;

			borderColor = COLOR(255, red1, green1, blue1);
			bodyColor = COLOR(255, red2, green2, blue2);
		}

		if (!osu_slider_use_gradient_image.getBool())
		{
			BLEND_SHADER->enable();
			BLEND_SHADER->setUniform1i("style", osu_slider_osu_next_style.getBool() ? 1 : 0);
			BLEND_SHADER->setUniform1f("bodyAlphaMultiplier", osu_slider_body_alpha_multiplier.getFloat());
			BLEND_SHADER->setUniform1f("bodyColorSaturation", osu_slider_body_color_saturation.getFloat());
			BLEND_SHADER->setUniform1f("borderSizeMultiplier", osu_slider_border_size_multiplier.getFloat());
			BLEND_SHADER->setUniform3f("colBorder", COLOR_GET_Rf(borderColor), COLOR_GET_Gf(borderColor), COLOR_GET_Bf(borderColor));
			BLEND_SHADER->setUniform3f("colBody", COLOR_GET_Rf(bodyColor), COLOR_GET_Gf(bodyColor), COLOR_GET_Bf(bodyColor));
		}

		g->setColor(0xffffffff);
		osu->getSkin()->getSliderGradient()->bind();

		// draw curve mesh
		{
			drawFillSliderBodyPeppy(g, osu, points, UNIT_CIRCLE_VAO, hitcircleDiameter/2.0f, drawFromIndex, drawUpToIndex);

			if (alwaysPoints.size() > 0)
				drawFillSliderBodyPeppy(g, osu, alwaysPoints, UNIT_CIRCLE_VAO_BAKED, hitcircleDiameter/2.0f, 0, alwaysPoints.size());
		}

		if (!osu_slider_use_gradient_image.getBool())
			BLEND_SHADER->disable();

		osu->getSliderFrameBuffer()->disable();
	}
	g->setBlending(true);
	g->setDepthBuffer(false);

	// now draw the slider to the screen (with alpha blending enabled again)
	int pixelFudge = 2;
	m_fBoundingBoxMinX -= pixelFudge;
	m_fBoundingBoxMaxX += pixelFudge;
	m_fBoundingBoxMinY -= pixelFudge;
	m_fBoundingBoxMaxY += pixelFudge;

	osu->getSliderFrameBuffer()->setColor(COLORf(alpha*osu_slider_alpha_multiplier.getFloat(), 1.0f, 1.0f, 1.0f));
	osu->getSliderFrameBuffer()->drawRect(g, m_fBoundingBoxMinX, m_fBoundingBoxMinY, m_fBoundingBoxMaxX - m_fBoundingBoxMinX, m_fBoundingBoxMaxY - m_fBoundingBoxMinY);
}

void OsuSliderRenderer::draw(Graphics *g, Osu *osu, VertexArrayObject *vao, const std::vector<Vector2> &alwaysPoints, Vector2 translation, float scale, float hitcircleDiameter, float from, float to, Color color, float alpha, long sliderTimeForRainbow)
{
	if (osu_slider_alpha_multiplier.getFloat() <= 0.0f || alpha <= 0.0f || vao == NULL)
		return;

	checkUpdateVars(osu, hitcircleDiameter);

	// draw entire slider into framebuffer
	g->setDepthBuffer(true);
	g->setBlending(false);
	{
		osu->getSliderFrameBuffer()->enable();

		Color borderColor = osu_slider_border_tint_combo_color.getBool() ? color : osu->getSkin()->getSliderBorderColor();
		Color bodyColor = osu->getSkin()->isSliderTrackOverridden() ? osu->getSkin()->getSliderTrackOverride() : color;

		if (osu_slider_rainbow.getBool())
		{
			float frequency = 0.3f;
			float time = engine->getTime()*20;

			char red1	= std::sin(frequency*time + 0 + sliderTimeForRainbow) * 127 + 128;
			char green1	= std::sin(frequency*time + 2 + sliderTimeForRainbow) * 127 + 128;
			char blue1	= std::sin(frequency*time + 4 + sliderTimeForRainbow) * 127 + 128;

			char red2	= std::sin(frequency*time*1.5f + 0 + sliderTimeForRainbow) * 127 + 128;
			char green2	= std::sin(frequency*time*1.5f + 2 + sliderTimeForRainbow) * 127 + 128;
			char blue2	= std::sin(frequency*time*1.5f + 4 + sliderTimeForRainbow) * 127 + 128;

			borderColor = COLOR(255, red1, green1, blue1);
			bodyColor = COLOR(255, red2, green2, blue2);
		}

		if (!osu_slider_use_gradient_image.getBool())
		{
			BLEND_SHADER->enable();
			BLEND_SHADER->setUniform1i("style", osu_slider_osu_next_style.getBool() ? 1 : 0);
			BLEND_SHADER->setUniform1f("bodyAlphaMultiplier", osu_slider_body_alpha_multiplier.getFloat());
			BLEND_SHADER->setUniform1f("bodyColorSaturation", osu_slider_body_color_saturation.getFloat());
			BLEND_SHADER->setUniform1f("borderSizeMultiplier", osu_slider_border_size_multiplier.getFloat());
			BLEND_SHADER->setUniform3f("colBorder", COLOR_GET_Rf(borderColor), COLOR_GET_Gf(borderColor), COLOR_GET_Bf(borderColor));
			BLEND_SHADER->setUniform3f("colBody", COLOR_GET_Rf(bodyColor), COLOR_GET_Gf(bodyColor), COLOR_GET_Bf(bodyColor));
		}

		g->setColor(0xffffffff);
		osu->getSkin()->getSliderGradient()->bind();

		// draw curve mesh
		{
			vao->setDrawPercent(from, to, UNIT_CIRCLE_VAO_TRIANGLES->getVertices().size());
			g->pushTransform();
				g->scale(scale, scale);
				g->translate(translation.x, translation.y);
				g->drawVAO(vao);
			g->popTransform();

			if (alwaysPoints.size() > 0)
				drawFillSliderBodyPeppy(g, osu, alwaysPoints, UNIT_CIRCLE_VAO_BAKED, hitcircleDiameter/2.0f, 0, alwaysPoints.size());
		}

		if (!osu_slider_use_gradient_image.getBool())
			BLEND_SHADER->disable();

		osu->getSliderFrameBuffer()->disable();
	}
	g->setBlending(true);
	g->setDepthBuffer(false);

	osu->getSliderFrameBuffer()->setColor(COLORf(alpha*osu_slider_alpha_multiplier.getFloat(), 1.0f, 1.0f, 1.0f));
	osu->getSliderFrameBuffer()->draw(g, 0, 0);
}

void OsuSliderRenderer::drawVR(Graphics *g, Osu *osu, OsuVR *vr, Matrix4 &mvp, float approachScale, const std::vector<Vector2> &points, const std::vector<Vector2> &alwaysPoints, float hitcircleDiameter, float from, float to, Color color, float alpha, long sliderTimeForRainbow)
{
	if (osu_slider_alpha_multiplier.getFloat() <= 0.0f || alpha <= 0.0f)
		return;

	const bool isOpenGLRendererHack = (dynamic_cast<OpenGLLegacyInterface*>(g) != NULL || dynamic_cast<OpenGL3Interface*>(g) != NULL);

	checkUpdateVars(osu, hitcircleDiameter);

	const int drawFromIndex = clamp<int>((int)std::round(points.size() * from), 0, points.size());
	const int drawUpToIndex = clamp<int>((int)std::round(points.size() * to), 0, points.size());

	// draw entire slider into framebuffer
	g->setDepthBuffer(true);
	///g->setBlending(false);
	{
		Color borderColor = osu_slider_border_tint_combo_color.getBool() ? color : osu->getSkin()->getSliderBorderColor();
		Color bodyColor = osu->getSkin()->isSliderTrackOverridden() ? osu->getSkin()->getSliderTrackOverride() : color;

		if (osu_slider_rainbow.getBool())
		{
			float frequency = 0.3f;
			float time = engine->getTime()*20;

			char red1	= std::sin(frequency*time + 0 + sliderTimeForRainbow) * 127 + 128;
			char green1	= std::sin(frequency*time + 2 + sliderTimeForRainbow) * 127 + 128;
			char blue1	= std::sin(frequency*time + 4 + sliderTimeForRainbow) * 127 + 128;

			char red2	= std::sin(frequency*time*1.5f + 0 + sliderTimeForRainbow) * 127 + 128;
			char green2	= std::sin(frequency*time*1.5f + 2 + sliderTimeForRainbow) * 127 + 128;
			char blue2	= std::sin(frequency*time*1.5f + 4 + sliderTimeForRainbow) * 127 + 128;

			borderColor = COLOR(255, red1, green1, blue1);
			bodyColor = COLOR(255, red2, green2, blue2);
		}

		///if (!osu_slider_use_gradient_image.getBool())
		{
			BLEND_SHADER_VR->enable();
			BLEND_SHADER_VR->setUniformMatrix4fv("matrix", mvp);
			BLEND_SHADER_VR->setUniform1i("style", osu_slider_osu_next_style.getBool() ? 1 : 0);
			BLEND_SHADER_VR->setUniform1f("bodyAlphaMultiplier", osu_slider_body_alpha_multiplier.getFloat());
			BLEND_SHADER_VR->setUniform1f("bodyColorSaturation", osu_slider_body_color_saturation.getFloat());
			BLEND_SHADER_VR->setUniform1f("borderSizeMultiplier", osu_slider_border_size_multiplier.getFloat());
			BLEND_SHADER_VR->setUniform3f("colBorder", COLOR_GET_Rf(borderColor), COLOR_GET_Gf(borderColor), COLOR_GET_Bf(borderColor));
			BLEND_SHADER_VR->setUniform3f("colBody", COLOR_GET_Rf(bodyColor), COLOR_GET_Gf(bodyColor), COLOR_GET_Bf(bodyColor));
		}

		g->setColor(0xffffffff);
		osu->getSkin()->getSliderGradient()->bind();

		if (isOpenGLRendererHack)
			glBlendEquation(GL_MAX); // HACKHACK: OpenGL hardcoded

		// draw curve mesh
		{
			BLEND_SHADER_VR->setUniform1i("part", 1);
			drawFillSliderBodyPeppyVR2(g, vr, mvp, points, UNIT_CIRCLE_VAO, hitcircleDiameter/2.0f, drawFromIndex, drawUpToIndex);

			if (alwaysPoints.size() > 0)
				drawFillSliderBodyPeppyVR2(g, vr, mvp, alwaysPoints, UNIT_CIRCLE_VAO_BAKED, hitcircleDiameter/2.0f, 0, alwaysPoints.size());

			BLEND_SHADER_VR->setUniform1i("part", 0);
			drawFillSliderBodyPeppyVR(g, osu, vr, mvp, points, UNIT_CIRCLE_VAO, hitcircleDiameter/2.0f, drawFromIndex, drawUpToIndex);

			if (alwaysPoints.size() > 0)
				drawFillSliderBodyPeppyVR(g, osu, vr, mvp, alwaysPoints, UNIT_CIRCLE_VAO_BAKED, hitcircleDiameter/2.0f, 0, alwaysPoints.size());
		}

		if (isOpenGLRendererHack)
			glBlendEquation(GL_FUNC_ADD); // HACKHACK: OpenGL hardcoded

		///if (!osu_slider_use_gradient_image.getBool())
			BLEND_SHADER_VR->disable();
	}
	///g->setBlending(true);
	g->setDepthBuffer(false);

	vr->getShaderTexturedLegacyGeneric()->enable();
}

void OsuSliderRenderer::drawVR(Graphics *g, Osu *osu, OsuVR *vr, Matrix4 &mvp, float approachScale, VertexArrayObject *vao1, VertexArrayObject *vao2, const std::vector<Vector2> &alwaysPoints, float hitcircleDiameter, float from, float to, Color color, float alpha, long sliderTimeForRainbow)
{
	if (osu_slider_alpha_multiplier.getFloat() <= 0.0f || alpha <= 0.0f || vao1 == NULL || vao2 == NULL)
		return;

	const bool isOpenGLRendererHack = (dynamic_cast<OpenGLLegacyInterface*>(g) != NULL || dynamic_cast<OpenGL3Interface*>(g) != NULL);

	checkUpdateVars(osu, hitcircleDiameter);

	// draw entire slider into framebuffer
	g->setDepthBuffer(true);
	///g->setBlending(false);
	{
		Color borderColor = osu_slider_border_tint_combo_color.getBool() ? color : osu->getSkin()->getSliderBorderColor();
		Color bodyColor = osu->getSkin()->isSliderTrackOverridden() ? osu->getSkin()->getSliderTrackOverride() : color;

		if (osu_slider_rainbow.getBool())
		{
			float frequency = 0.3f;
			float time = engine->getTime()*20;

			char red1	= std::sin(frequency*time + 0 + sliderTimeForRainbow) * 127 + 128;
			char green1	= std::sin(frequency*time + 2 + sliderTimeForRainbow) * 127 + 128;
			char blue1	= std::sin(frequency*time + 4 + sliderTimeForRainbow) * 127 + 128;

			char red2	= std::sin(frequency*time*1.5f + 0 + sliderTimeForRainbow) * 127 + 128;
			char green2	= std::sin(frequency*time*1.5f + 2 + sliderTimeForRainbow) * 127 + 128;
			char blue2	= std::sin(frequency*time*1.5f + 4 + sliderTimeForRainbow) * 127 + 128;

			borderColor = COLOR(255, red1, green1, blue1);
			bodyColor = COLOR(255, red2, green2, blue2);
		}

		///if (!osu_slider_use_gradient_image.getBool())
		{
			BLEND_SHADER_VR->enable();
			BLEND_SHADER_VR->setUniformMatrix4fv("matrix", mvp);
			BLEND_SHADER_VR->setUniform1i("style", osu_slider_osu_next_style.getBool() ? 1 : 0);
			BLEND_SHADER_VR->setUniform1f("bodyAlphaMultiplier", osu_slider_body_alpha_multiplier.getFloat());
			BLEND_SHADER_VR->setUniform1f("bodyColorSaturation", osu_slider_body_color_saturation.getFloat());
			BLEND_SHADER_VR->setUniform1f("borderSizeMultiplier", osu_slider_border_size_multiplier.getFloat());
			BLEND_SHADER_VR->setUniform3f("colBorder", COLOR_GET_Rf(borderColor), COLOR_GET_Gf(borderColor), COLOR_GET_Bf(borderColor));
			BLEND_SHADER_VR->setUniform3f("colBody", COLOR_GET_Rf(bodyColor), COLOR_GET_Gf(bodyColor), COLOR_GET_Bf(bodyColor));
		}

		g->setColor(0xffffffff);
		osu->getSkin()->getSliderGradient()->bind();

		if (isOpenGLRendererHack)
			glBlendEquation(GL_MAX); // HACKHACK: OpenGL hardcoded

		// draw curve mesh
		{
			BLEND_SHADER_VR->setUniform1i("part", 1);
			vao2->setDrawPercent(from, to, UNIT_CIRCLE_VAO_TRIANGLES->getVertices().size());
			g->drawVAO(vao2);

			if (alwaysPoints.size() > 0)
				drawFillSliderBodyPeppyVR2(g, vr, mvp, alwaysPoints, UNIT_CIRCLE_VAO_BAKED, hitcircleDiameter/2.0f, 0, alwaysPoints.size());

			BLEND_SHADER_VR->setUniform1i("part", 0);
			vao1->setDrawPercent(from, to, UNIT_CIRCLE_VAO_TRIANGLES->getVertices().size());
			g->drawVAO(vao1);

			if (alwaysPoints.size() > 0)
				drawFillSliderBodyPeppyVR(g, osu, vr, mvp, alwaysPoints, UNIT_CIRCLE_VAO_BAKED, hitcircleDiameter/2.0f, 0, alwaysPoints.size());
		}

		if (isOpenGLRendererHack)
			glBlendEquation(GL_FUNC_ADD); // HACKHACK: OpenGL hardcoded

		///if (!osu_slider_use_gradient_image.getBool())
			BLEND_SHADER_VR->disable();
	}
	///g->setBlending(true);
	g->setDepthBuffer(false);

	vr->getShaderTexturedLegacyGeneric()->enable();
}

void OsuSliderRenderer::drawMM(Graphics *g, Osu *osu, const std::vector<std::vector<Vector2>> &points, float hitcircleDiameter, float from, float to, Color color, float alpha, long sliderTimeForRainbow)
{
	if (osu_slider_alpha_multiplier.getFloat() <= 0.0f || alpha <= 0.0f)
		return;

	checkUpdateVars(osu, hitcircleDiameter);

	// TODO: this is unfinished as fuck (needs fixes/changes in the curve calculation in the slider class, and also the curve quad interpolation here)

	// shit
	int numPointsTotal = 0;
	for (int i=0; i<points.size(); i++)
	{
		numPointsTotal += points[i].size();
	}

	const int drawFromIndex = clamp<int>((int)std::round(numPointsTotal * from), 0, numPointsTotal);
	const int drawUpToIndex = clamp<int>((int)std::round(numPointsTotal * to), 0, numPointsTotal);

	// reset
	resetRenderTargetBoundingBox();

	g->setDepthBuffer(true);
	g->setBlending(false);
	{
		osu->getSliderFrameBuffer()->enable();

		Color borderColor = osu_slider_border_tint_combo_color.getBool() ? color : osu->getSkin()->getSliderBorderColor();
		Color bodyColor = osu->getSkin()->isSliderTrackOverridden() ? osu->getSkin()->getSliderTrackOverride() : color;

		if (osu_slider_rainbow.getBool())
		{
			float frequency = 0.3f;
			float time = engine->getTime()*20;

			char red1	= std::sin(frequency*time + 0 + sliderTimeForRainbow) * 127 + 128;
			char green1	= std::sin(frequency*time + 2 + sliderTimeForRainbow) * 127 + 128;
			char blue1	= std::sin(frequency*time + 4 + sliderTimeForRainbow) * 127 + 128;

			char red2	= std::sin(frequency*time*1.5f + 0 + sliderTimeForRainbow) * 127 + 128;
			char green2	= std::sin(frequency*time*1.5f + 2 + sliderTimeForRainbow) * 127 + 128;
			char blue2	= std::sin(frequency*time*1.5f + 4 + sliderTimeForRainbow) * 127 + 128;

			borderColor = COLOR(255, red1, green1, blue1);
			bodyColor = COLOR(255, red2, green2, blue2);
		}

		if (!osu_slider_use_gradient_image.getBool())
		{
			BLEND_SHADER->enable();
			BLEND_SHADER->setUniform1i("style", osu_slider_osu_next_style.getBool() ? 1 : 0);
			BLEND_SHADER->setUniform1f("bodyAlphaMultiplier", osu_slider_body_alpha_multiplier.getFloat());
			BLEND_SHADER->setUniform1f("bodyColorSaturation", osu_slider_body_color_saturation.getFloat());
			BLEND_SHADER->setUniform1f("borderSizeMultiplier", osu_slider_border_size_multiplier.getFloat());
			BLEND_SHADER->setUniform3f("colBorder", COLOR_GET_Rf(borderColor), COLOR_GET_Gf(borderColor), COLOR_GET_Bf(borderColor));
			BLEND_SHADER->setUniform3f("colBody", COLOR_GET_Rf(bodyColor), COLOR_GET_Gf(bodyColor), COLOR_GET_Bf(bodyColor));
		}

		g->setColor(0xffffffff);
		osu->getSkin()->getSliderGradient()->bind();

		// draw curve mesh
		{
			drawFillSliderBodyMM(g, points, hitcircleDiameter/2.0f, drawFromIndex, drawUpToIndex);
		}

		if (!osu_slider_use_gradient_image.getBool())
			BLEND_SHADER->disable();

		osu->getSliderFrameBuffer()->disable();
	}
	g->setBlending(true);
	g->setDepthBuffer(false);

	// now draw the slider to the screen (with alpha blending enabled again)
	int pixelFudge = 2;
	m_fBoundingBoxMinX -= pixelFudge;
	m_fBoundingBoxMaxX += pixelFudge;
	m_fBoundingBoxMinY -= pixelFudge;
	m_fBoundingBoxMaxY += pixelFudge;

	osu->getSliderFrameBuffer()->setColor(COLORf(alpha*osu_slider_alpha_multiplier.getFloat(), 1.0f, 1.0f, 1.0f));
	osu->getSliderFrameBuffer()->drawRect(g, m_fBoundingBoxMinX, m_fBoundingBoxMinY, m_fBoundingBoxMaxX - m_fBoundingBoxMinX, m_fBoundingBoxMaxY - m_fBoundingBoxMinY);
}

void OsuSliderRenderer::drawFillSliderBodyPeppy(Graphics *g, Osu *osu, const std::vector<Vector2> &points, VertexArrayObject *circleMesh, float radius, int drawFromIndex, int drawUpToIndex)
{
	if (drawFromIndex < 0)
		drawFromIndex = 0;
	if (drawUpToIndex < 0)
		drawUpToIndex = points.size();

	g->pushTransform();

		// now, translate and draw the master vao for every curve point
		float startX = 0.0f;
		float startY = 0.0f;
		for (int i=drawFromIndex; i<drawUpToIndex; ++i)
		{
			const float x = points[i].x;
			const float y = points[i].y;

			// fuck oob sliders
			if (x < -radius*2 || x > osu->getScreenWidth()+radius*2 || y < -radius*2 || y > osu->getScreenHeight()+radius*2)
				continue;

			g->translate(x-startX, y-startY, 0);
			g->drawVAO(circleMesh);

			startX = x;
			startY = y;

			if (x-radius < m_fBoundingBoxMinX)
				m_fBoundingBoxMinX = x-radius;
			if (x+radius > m_fBoundingBoxMaxX)
				m_fBoundingBoxMaxX = x+radius;
			if (y-radius < m_fBoundingBoxMinY)
				m_fBoundingBoxMinY = y-radius;
			if (y+radius > m_fBoundingBoxMaxY)
				m_fBoundingBoxMaxY = y+radius;
		}

	g->popTransform();
}

void OsuSliderRenderer::drawFillSliderBodyPeppyVR(Graphics *g, Osu *osu, OsuVR *vr, Matrix4 &mvp, const std::vector<Vector2> &points, VertexArrayObject *circleMesh, float radius, int drawFromIndex, int drawUpToIndex)
{
	if (drawFromIndex < 0)
		drawFromIndex = 0;
	if (drawUpToIndex < 0)
		drawUpToIndex = points.size();

	g->pushTransform();

		// now, translate and draw the master vao for every curve point
		float startX = 0.0f;
		float startY = 0.0f;
		for (int i=drawFromIndex; i<drawUpToIndex; ++i)
		{
			const float x = points[i].x;
			const float y = points[i].y;

			g->translate(x-startX, y-startY, 0);
			g->drawVAO(circleMesh);

			startX = x;
			startY = y;
		}

	g->popTransform();
}

void OsuSliderRenderer::drawFillSliderBodyPeppyVR2(Graphics *g, OsuVR *vr, Matrix4 &mvp, const std::vector<Vector2> &points, VertexArrayObject *circleMesh, float radius, int drawFromIndex, int drawUpToIndex)
{
	if (drawFromIndex < 0)
		drawFromIndex = 0;
	if (drawUpToIndex < 0)
		drawUpToIndex = points.size();

	// HACKHACK: magic numbers from shader
	const float defaultBorderSize = 0.11;
	const float defaultTransitionSize = 0.011f;
	const float defaultOuterShadowSize = 0.08f;
	const float scale = 1.0f - osu_slider_border_size_multiplier.getFloat()*defaultBorderSize - defaultTransitionSize - defaultOuterShadowSize;

	// now, translate and draw the master vao for every curve point
	for (int i=drawFromIndex; i<drawUpToIndex; ++i)
	{
		const float x = points[i].x;
		const float y = points[i].y;

		g->pushTransform();
			g->scale(scale, scale, 1.0f);
			g->translate(x, y, 0.25f);
			g->drawVAO(circleMesh);
		g->popTransform();
	}
}

void OsuSliderRenderer::drawFillSliderBodyMM(Graphics *g, const std::vector<std::vector<Vector2>> &points, float radius, int drawFromIndex, int drawUpToIndex)
{
	// draw all segments
	int snakeIndexCounter = 0;
	Vector2 prevEnd = (points.size() > 0 ? (points[0].size() > 0 ? (points[0][0]) : Vector2()) : Vector2()); // optimization: only draw startcap if previous endcap is different
	for (int s=0; s<points.size(); s++)
	{
		VertexArrayObject vao(Graphics::PRIMITIVE::PRIMITIVE_TRIANGLES);
		Vector2 prevVertexLeft;
		Vector2 prevVertexCenter;
		Vector2 prevVertexRight;

		// generate body mesh
		bool draw = snakeIndexCounter == 0; // always draw the first segment
		Vector2 start = (points[s].size() > 0 ? points[s][0] : Vector2()); // even if the snakeIndexCounter is 0, we still need a correct value for the startcap
		Vector2 end = start;
		for (int i=0; i<points[s].size(); i++)
		{
			snakeIndexCounter++; // shared snaking counter

			const Vector2 current = points[s][i];
			float angle;

			// calculate angle
			Vector2 diff;
			{
				const int prevIndex = clamp<int>(i-1, 0, points[s].size()-1);
				const int nextIndex = clamp<int>(i+1, 0, points[s].size()-1);

				diff = ((points[s][prevIndex] - current) + (current - points[s][nextIndex])).normalize();
				angle = std::atan2(diff.x, diff.y) + PI*0.5;
			}

			const float offset = radius; // offset from the center of the curve to the edge (~radius)

			Vector2 dir = Vector2(std::sin(angle), std::cos(angle));
			Vector2 edge = dir * offset;

			Vector2 vertexLeft = current - edge;
			Vector2 vertexCenter = current;
			Vector2 vertexRight = current + edge;

			// handle snaking in both directions
			bool addMesh = true;
			if (snakeIndexCounter > drawUpToIndex) // "snaking" upper limit
				break; // we're done here
			else if (snakeIndexCounter-1 <= drawFromIndex) // "shrinking" lower limit
			{
				draw = false; // don't draw anything until we reach > drawFromIndex
				addMesh = false; // however, iterate as usual, but don't add vertices until we reach > drawFromIndex
				start = current; // move startcap with shrink
			}
			else
				draw = true;

			// mesh
			if (i > 0 && addMesh)
			{
				// type is triangles, build a rectangle out of 6 vertices

				//
				// 1   3   5     // vertex<>
				// *---*---*
				// |  /|  /|
				// | / | / |     // the line 3-4 is the center of the slider (with a raised z-coordinate for blending)
				// |/  |/  |
				// *---*---*
				// 2   4   6     // prevVertex<>
				//

				const float crackFudge = 1.0f; // one pixel should be enough
				diff = diff * crackFudge; // make sure that no cracks exist between segments (equal points don't guarantee a perfect surface along the triangle edges)

				vao.addTexcoord(0, 0);
				vao.addVertex(vertexLeft.x - diff.x, vertexLeft.y - diff.y); // 1
				vao.addTexcoord(0, 0);
				vao.addVertex(prevVertexLeft.x + diff.x, prevVertexLeft.y + diff.y); // 2
				vao.addTexcoord(1, 0);
				vao.addVertex(vertexCenter.x - diff.x, vertexCenter.y - diff.y, MESH_CENTER_HEIGHT); // 3

				vao.addTexcoord(1, 0);
				vao.addVertex(vertexCenter.x - diff.x, vertexCenter.y - diff.y, MESH_CENTER_HEIGHT); // 3
				vao.addTexcoord(0, 0);
				vao.addVertex(prevVertexLeft.x + diff.x, prevVertexLeft.y + diff.y); // 2
				vao.addTexcoord(1, 0);
				vao.addVertex(prevVertexCenter.x + diff.x, prevVertexCenter.y + diff.y, MESH_CENTER_HEIGHT); // 4

				vao.addTexcoord(1, 0);
				vao.addVertex(vertexCenter.x - diff.x, vertexCenter.y - diff.y, MESH_CENTER_HEIGHT); // 3
				vao.addTexcoord(1, 0);
				vao.addVertex(prevVertexCenter.x + diff.x, prevVertexCenter.y + diff.y, MESH_CENTER_HEIGHT); // 4
				vao.addTexcoord(0, 0);
				vao.addVertex(vertexRight.x - diff.x, vertexRight.y - diff.y); // 5

				vao.addTexcoord(0, 0);
				vao.addVertex(vertexRight.x - diff.x, vertexRight.y - diff.y); // 5
				vao.addTexcoord(1, 0);
				vao.addVertex(prevVertexCenter.x + diff.x, prevVertexCenter.y + diff.y, MESH_CENTER_HEIGHT); // 4
				vao.addTexcoord(0, 0);
				vao.addVertex(prevVertexRight.x + diff.x, prevVertexRight.y + diff.y); // 6
			}

			end = current; // move endcap with snake

			// save
			prevVertexLeft = vertexLeft;
			prevVertexCenter = vertexCenter;
			prevVertexRight = vertexRight;

			// optimization
			if (current.x-radius < m_fBoundingBoxMinX)
				m_fBoundingBoxMinX = current.x-radius;
			if (current.x+radius > m_fBoundingBoxMaxX)
				m_fBoundingBoxMaxX = current.x+radius;
			if (current.y-radius < m_fBoundingBoxMinY)
				m_fBoundingBoxMinY = current.y-radius;
			if (current.y+radius > m_fBoundingBoxMaxY)
				m_fBoundingBoxMaxY = current.y+radius;
		}

		// draw it
		if (points.size() > 0 && draw)
		{
			if (osu_slider_debug_wireframe.getBool())
				g->setWireframe(true);

			// draw startcap & endcap
			if (osu_slider_debug_draw_caps.getBool())
			{
				if (prevEnd != start || s == 0) // only draw startcap if previous endcap is different, or if this is the first segment
				{
					g->pushTransform();
						g->translate(start.x, start.y);
						g->drawVAO(UNIT_CIRCLE_VAO);
					g->popTransform();
				}
				prevEnd = end;
				if (end != start)
				{
					g->pushTransform();
						g->translate(end.x, end.y);
						g->drawVAO(UNIT_CIRCLE_VAO);
					g->popTransform();
				}
			}

			// draw body
			g->drawVAO(&vao);

			if (osu_slider_debug_wireframe.getBool())
				g->setWireframe(false);
		}
	}
}

void OsuSliderRenderer::checkUpdateVars(Osu *osu, float hitcircleDiameter)
{
	// static globals

	// build shaders and circle mesh
	if (BLEND_SHADER == NULL) // only do this once
	{
		// build shaders
		BLEND_SHADER = engine->getResourceManager()->loadShader("slider.vsh", "slider.fsh", "slider");
		if (osu->isInVRMode())
			BLEND_SHADER_VR = engine->getResourceManager()->loadShader("sliderVR.vsh", "sliderVR.fsh", "sliderVR");

		// build unit cone
		{
			// tip of the cone
			// texture coordinates
			UNIT_CIRCLE.push_back(1.0f);
			UNIT_CIRCLE.push_back(0.0f);

			// position
			UNIT_CIRCLE.push_back(0.0f);
			UNIT_CIRCLE.push_back(0.0f);
			UNIT_CIRCLE.push_back(MESH_CENTER_HEIGHT);

			for (int j=0; j<UNIT_CIRCLE_SUBDIVIDES; ++j)
			{
				float phase = j * (float) PI * 2.0f / UNIT_CIRCLE_SUBDIVIDES;

				// texture coordinates
				UNIT_CIRCLE.push_back(0.0f);
				UNIT_CIRCLE.push_back(0.0f);

				// positon
				UNIT_CIRCLE.push_back((float)std::sin(phase));
				UNIT_CIRCLE.push_back((float)std::cos(phase));
				UNIT_CIRCLE.push_back(0.0f);
			}

			// texture coordinates
			UNIT_CIRCLE.push_back(0.0f);
			UNIT_CIRCLE.push_back(0.0f);

			// positon
			UNIT_CIRCLE.push_back((float)std::sin(0.0f));
			UNIT_CIRCLE.push_back((float)std::cos(0.0f));
			UNIT_CIRCLE.push_back(0.0f);
		}
	}

	// build vaos
	if (UNIT_CIRCLE_VAO == NULL)
		UNIT_CIRCLE_VAO = new VertexArrayObject(Graphics::PRIMITIVE::PRIMITIVE_TRIANGLE_FAN);
	if (UNIT_CIRCLE_VAO_BAKED == NULL)
		UNIT_CIRCLE_VAO_BAKED = engine->getResourceManager()->createVertexArrayObject(Graphics::PRIMITIVE::PRIMITIVE_TRIANGLE_FAN);
	if (UNIT_CIRCLE_VAO_TRIANGLES == NULL)
		UNIT_CIRCLE_VAO_TRIANGLES = new VertexArrayObject(Graphics::PRIMITIVE::PRIMITIVE_TRIANGLES);

	// (re-)generate master circle mesh (centered) if the size changed
	// TODO: this will destroy performance for mods like Minimize (since the radius is different every frame), maybe just scale during draw instead of rebuild?
	float radius = hitcircleDiameter/2.0f;
	if (radius != UNIT_CIRCLE_VAO_RADIUS)
	{
		UNIT_CIRCLE_VAO_BAKED->release();

		// triangle fan
		UNIT_CIRCLE_VAO_RADIUS = radius;
		UNIT_CIRCLE_VAO->clear();
		for (int i=0; i<UNIT_CIRCLE.size()/5; i++)
		{
			Vector3 vertexPos = Vector3((radius * UNIT_CIRCLE[i * 5 + 2]), (radius * UNIT_CIRCLE[i * 5 + 3]), UNIT_CIRCLE[i * 5 + 4]);
			Vector2 vertexTexcoord = Vector2(UNIT_CIRCLE[i * 5 + 0], UNIT_CIRCLE[i * 5 + 1]);

			UNIT_CIRCLE_VAO->addVertex(vertexPos);
			UNIT_CIRCLE_VAO->addTexcoord(vertexTexcoord);

			UNIT_CIRCLE_VAO_BAKED->addVertex(vertexPos);
			UNIT_CIRCLE_VAO_BAKED->addTexcoord(vertexTexcoord);
		}

		engine->getResourceManager()->loadResource(UNIT_CIRCLE_VAO_BAKED);

		// pure triangles (needed for VertexArrayObject, because we can't merge multiple triangle fan meshes into one VertexArrayObject)
		UNIT_CIRCLE_VAO_TRIANGLES->clear();
		Vector3 startVertex = Vector3((radius * UNIT_CIRCLE[0 * 5 + 2]), (radius * UNIT_CIRCLE[0 * 5 + 3]), UNIT_CIRCLE[0 * 5 + 4]);
		Vector2 startUV = Vector2(UNIT_CIRCLE[0 * 5 + 0], UNIT_CIRCLE[0 * 5 + 1]);
		for (int i=1; i<UNIT_CIRCLE.size()/5 - 1; i++)
		{
			// center
			UNIT_CIRCLE_VAO_TRIANGLES->addVertex(startVertex);
			UNIT_CIRCLE_VAO_TRIANGLES->addTexcoord(startUV);

			// pizza slice edge 1
			UNIT_CIRCLE_VAO_TRIANGLES->addVertex(Vector3((radius * UNIT_CIRCLE[i * 5 + 2]), (radius * UNIT_CIRCLE[i * 5 + 3]), UNIT_CIRCLE[i * 5 + 4]));
			UNIT_CIRCLE_VAO_TRIANGLES->addTexcoord(Vector2(UNIT_CIRCLE[i * 5 + 0], UNIT_CIRCLE[i * 5 + 1]));

			// pizza slice edge 2
			UNIT_CIRCLE_VAO_TRIANGLES->addVertex(Vector3((radius * UNIT_CIRCLE[(i+1) * 5 + 2]), (radius * UNIT_CIRCLE[(i+1) * 5 + 3]), UNIT_CIRCLE[(i+1) * 5 + 4]));
			UNIT_CIRCLE_VAO_TRIANGLES->addTexcoord(Vector2(UNIT_CIRCLE[(i+1) * 5 + 0], UNIT_CIRCLE[(i+1) * 5 + 1]));
		}
	}
}

void OsuSliderRenderer::resetRenderTargetBoundingBox()
{
	OsuSliderRenderer::m_fBoundingBoxMinX = std::numeric_limits<float>::max();
	OsuSliderRenderer::m_fBoundingBoxMaxX = 0.0f;
	OsuSliderRenderer::m_fBoundingBoxMinY = std::numeric_limits<float>::max();
	OsuSliderRenderer::m_fBoundingBoxMaxY = 0.0f;
}
