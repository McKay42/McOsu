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
#include "Environment.h"
#include "ConVar.h"

#include "Osu.h"
#include "OsuVR.h"
#include "OsuSkin.h"
#include "OsuGameRules.h"

#include "OpenGLHeaders.h"
#include "OpenGLLegacyInterface.h"
#include "OpenGL3Interface.h"
#include "OpenGLES2Interface.h"

Shader *OsuSliderRenderer::BLEND_SHADER = NULL;
Shader *OsuSliderRenderer::BLEND_SHADER_VR = NULL;

float OsuSliderRenderer::MESH_CENTER_HEIGHT = 0.5f; // Camera::buildMatrixOrtho2D() uses -1 to 1 for zn/zf, so don't make this too high
int OsuSliderRenderer::UNIT_CIRCLE_SUBDIVISIONS = 0; // see osu_slider_body_unit_circle_subdivisions now
std::vector<float> OsuSliderRenderer::UNIT_CIRCLE;
VertexArrayObject *OsuSliderRenderer::UNIT_CIRCLE_VAO = NULL;
VertexArrayObject *OsuSliderRenderer::UNIT_CIRCLE_VAO_BAKED = NULL;
VertexArrayObject *OsuSliderRenderer::UNIT_CIRCLE_VAO_TRIANGLES = NULL;
float OsuSliderRenderer::UNIT_CIRCLE_VAO_RADIUS = 0.0f;

float OsuSliderRenderer::m_fBoundingBoxMinX = std::numeric_limits<float>::max();
float OsuSliderRenderer::m_fBoundingBoxMaxX = 0.0f;
float OsuSliderRenderer::m_fBoundingBoxMinY = std::numeric_limits<float>::max();
float OsuSliderRenderer::m_fBoundingBoxMaxY = 0.0f;

ConVar osu_slider_debug_draw("osu_slider_debug_draw", false, "draw hitcircle at every curve point and nothing else (no vao, no rt, no shader, nothing) (requires enabling legacy slider renderer)");
ConVar osu_slider_debug_draw_square_vao("osu_slider_debug_draw_square_vao", false, "generate square vaos and nothing else (no rt, no shader) (requires disabling legacy slider renderer)");
ConVar osu_slider_debug_wireframe("osu_slider_debug_wireframe", false, "unused");
ConVar osu_slider_debug_draw_caps("osu_slider_debug_draw_caps", true, "unused");

ConVar osu_slider_alpha_multiplier("osu_slider_alpha_multiplier", 1.0f);
ConVar osu_slider_body_alpha_multiplier("osu_slider_body_alpha_multiplier", 1.0f);
ConVar osu_slider_body_color_saturation("osu_slider_body_color_saturation", 1.0f);
ConVar osu_slider_border_size_multiplier("osu_slider_border_size_multiplier", 1.0f);
ConVar osu_slider_border_tint_combo_color("osu_slider_border_tint_combo_color", false);
ConVar osu_slider_osu_next_style("osu_slider_osu_next_style", false);
ConVar osu_slider_rainbow("osu_slider_rainbow", false);
ConVar osu_slider_use_gradient_image("osu_slider_use_gradient_image", false);

ConVar osu_slider_body_unit_circle_subdivisions("osu_slider_body_unit_circle_subdivisions", 42);

VertexArrayObject *OsuSliderRenderer::generateVAO(Osu *osu, const std::vector<Vector2> &points, float hitcircleDiameter, Vector3 translation)
{
	engine->getResourceManager()->requestNextLoadUnmanaged();
	VertexArrayObject *vao = engine->getResourceManager()->createVertexArrayObject();

	checkUpdateVars(osu, hitcircleDiameter);

	const Vector3 xOffset = Vector3(hitcircleDiameter, 0, 0);
	const Vector3 yOffset = Vector3(0, hitcircleDiameter, 0);

	const bool debugSquareVao = osu_slider_debug_draw_square_vao.getBool();

	for (int i=0; i<points.size(); i++)
	{
		// fuck oob sliders
		if (points[i].x < -hitcircleDiameter-OsuGameRules::OSU_COORD_WIDTH*2 || points[i].x > osu->getScreenWidth()+hitcircleDiameter+OsuGameRules::OSU_COORD_WIDTH*2 || points[i].y < -hitcircleDiameter-OsuGameRules::OSU_COORD_HEIGHT*2 || points[i].y > osu->getScreenHeight()+hitcircleDiameter+OsuGameRules::OSU_COORD_HEIGHT*2)
			continue;

		if (!debugSquareVao)
		{
			const std::vector<Vector3> &meshVertices = UNIT_CIRCLE_VAO_TRIANGLES->getVertices();
			const std::vector<std::vector<Vector2>> &meshTexCoords = UNIT_CIRCLE_VAO_TRIANGLES->getTexcoords();
			for (int v=0; v<meshVertices.size(); v++)
			{
				vao->addVertex(meshVertices[v] + Vector3(points[i].x, points[i].y, 0) + translation);
				vao->addTexcoord(meshTexCoords[0][v]);
			}
		}
		else
		{
			const Vector3 topLeft = Vector3(points[i].x, points[i].y, 0) - xOffset/2.0f - yOffset/2.0f + translation;
			const Vector3 topRight = topLeft + xOffset;
			const Vector3 bottomLeft = topLeft + yOffset;
			const Vector3 bottomRight = bottomLeft + xOffset;

			vao->addVertex(topLeft);
			vao->addTexcoord(0, 0);

			vao->addVertex(bottomLeft);
			vao->addTexcoord(0, 1);

			vao->addVertex(bottomRight);
			vao->addTexcoord(1, 1);

			vao->addVertex(topLeft);
			vao->addTexcoord(0, 0);

			vao->addVertex(bottomRight);
			vao->addTexcoord(1, 1);

			vao->addVertex(topRight);
			vao->addTexcoord(1, 0);
		}
	}

	if (vao->getNumVertices() > 0)
		engine->getResourceManager()->loadResource(vao);
	else
		debugLog("OsuSliderRenderer::generateSliderVAO() ERROR: Zero triangles!\n");

	return vao;
}

void OsuSliderRenderer::draw(Graphics *g, Osu *osu, const std::vector<Vector2> &points, const std::vector<Vector2> &alwaysPoints, float hitcircleDiameter, float from, float to, Color color, float alpha, long sliderTimeForRainbow)
{
	if (osu_slider_alpha_multiplier.getFloat() <= 0.0f || alpha <= 0.0f) return;

	checkUpdateVars(osu, hitcircleDiameter);

	const int drawFromIndex = clamp<int>((int)std::round(points.size() * from), 0, points.size());
	const int drawUpToIndex = clamp<int>((int)std::round(points.size() * to), 0, points.size());

	// debug sliders
	if (osu_slider_debug_draw.getBool())
	{
		const float circleImageScale = hitcircleDiameter / (float)osu->getSkin()->getHitCircle()->getWidth();
		const float circleImageScaleInv = (1.0f / circleImageScale);

		const float width = (float)osu->getSkin()->getHitCircle()->getWidth();
		const float height = (float)osu->getSkin()->getHitCircle()->getHeight();

		const float x = (-width / 2.0f);
		const float y = (-height / 2.0f);
		const float z = -1.0f;

		g->pushTransform();
		{
			g->scale(circleImageScale, circleImageScale);

			g->setColor(color);
			g->setAlpha(alpha*osu_slider_alpha_multiplier.getFloat());
			osu->getSkin()->getHitCircle()->bind();
			{
				for (int i=drawFromIndex; i<drawUpToIndex; i++)
				{
					const Vector2 point = points[i] * circleImageScaleInv;

					static VertexArrayObject vao(Graphics::PRIMITIVE::PRIMITIVE_QUADS);
					vao.empty();
					{
						vao.addTexcoord(0, 0);
						vao.addVertex(point.x + x, point.y + y, z);

						vao.addTexcoord(0, 1);
						vao.addVertex(point.x + x, point.y + y + height, z);

						vao.addTexcoord(1, 1);
						vao.addVertex(point.x + x + width, point.y + y + height, z);

						vao.addTexcoord(1, 0);
						vao.addVertex(point.x + x + width, point.y + y, z);
					}
					g->drawVAO(&vao);
				}
			}
			osu->getSkin()->getHitCircle()->unbind();
		}
		g->popTransform();

		return; // nothing more to draw here
	}

	// reset
	resetRenderTargetBoundingBox();

	// draw entire slider into framebuffer
	g->setDepthBuffer(true);
	g->setBlending(false);
	{
		osu->getSliderFrameBuffer()->enable();
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
			{
				// draw curve mesh
				{
					drawFillSliderBodyPeppy(g, osu, points, UNIT_CIRCLE_VAO, hitcircleDiameter/2.0f, drawFromIndex, drawUpToIndex, BLEND_SHADER);

					if (alwaysPoints.size() > 0)
						drawFillSliderBodyPeppy(g, osu, alwaysPoints, UNIT_CIRCLE_VAO_BAKED, hitcircleDiameter/2.0f, 0, alwaysPoints.size(), BLEND_SHADER);
				}
			}

			if (!osu_slider_use_gradient_image.getBool())
				BLEND_SHADER->disable();
		}
		osu->getSliderFrameBuffer()->disable();
	}
	g->setBlending(true);
	g->setDepthBuffer(false);

	// now draw the slider to the screen (with alpha blending enabled again)
	const int pixelFudge = 2;
	m_fBoundingBoxMinX -= pixelFudge;
	m_fBoundingBoxMaxX += pixelFudge;
	m_fBoundingBoxMinY -= pixelFudge;
	m_fBoundingBoxMaxY += pixelFudge;

	osu->getSliderFrameBuffer()->setColor(COLORf(alpha*osu_slider_alpha_multiplier.getFloat(), 1.0f, 1.0f, 1.0f));
	osu->getSliderFrameBuffer()->drawRect(g, m_fBoundingBoxMinX, m_fBoundingBoxMinY, m_fBoundingBoxMaxX - m_fBoundingBoxMinX, m_fBoundingBoxMaxY - m_fBoundingBoxMinY);
}

void OsuSliderRenderer::draw(Graphics *g, Osu *osu, VertexArrayObject *vao, const std::vector<Vector2> &alwaysPoints, Vector2 translation, float scale, float hitcircleDiameter, float from, float to, Color color, float alpha, long sliderTimeForRainbow)
{
	if (osu_slider_alpha_multiplier.getFloat() <= 0.0f || alpha <= 0.0f || vao == NULL) return;

	checkUpdateVars(osu, hitcircleDiameter);

	if (osu_slider_debug_draw_square_vao.getBool())
	{
		g->setColor(color);
		g->setAlpha(alpha*osu_slider_alpha_multiplier.getFloat());
		osu->getSkin()->getHitCircle()->bind();

		vao->setDrawPercent(from, to, 6); // HACKHACK: hardcoded magic number
		{
			g->pushTransform();
			{
				g->scale(scale, scale);
				g->translate(translation.x, translation.y);

				g->drawVAO(vao);
			}
			g->popTransform();
		}

		return; // nothing more to draw here
	}

	// NOTE: this would add support for aspire slider distortions, but calculating the bounding box live is a waste of performance, not worth it
	/*
	{
		m_fBoundingBoxMinX = std::numeric_limits<float>::max();
		m_fBoundingBoxMaxX = std::numeric_limits<float>::min();
		m_fBoundingBoxMinY = std::numeric_limits<float>::max();
		m_fBoundingBoxMaxY = std::numeric_limits<float>::min();

		// NOTE: to get the animated effect, would have to use from + to
		for (int i=0; i<points.size(); i++)
		{
			const float &x = points[i].x;
			const float &y = points[i].y;
			const float radius = hitcircleDiameter/2.0f;

			if (x-radius < m_fBoundingBoxMinX)
				m_fBoundingBoxMinX = x-radius;
			if (x+radius > m_fBoundingBoxMaxX)
				m_fBoundingBoxMaxX = x+radius;
			if (y-radius < m_fBoundingBoxMinY)
				m_fBoundingBoxMinY = y-radius;
			if (y+radius > m_fBoundingBoxMaxY)
				m_fBoundingBoxMaxY = y+radius;
		}

		const Vector4 tLS = (g->getProjectionMatrix() * Vector4(m_fBoundingBoxMinX, m_fBoundingBoxMinY, 0, 0) + Vector4(1, 1, 0, 0)) * 0.5f;
		const Vector4 bRS = (g->getProjectionMatrix() * Vector4(m_fBoundingBoxMaxX, m_fBoundingBoxMaxY, 0, 0) + Vector4(1, 1, 0, 0)) * 0.5f;

		float scaleToApplyAfterTranslationX = 1.0f;
		float scaleToApplyAfterTranslationY = 1.0f;

		const float sclX = (32768.0f / (float)osu->getScreenWidth());
		const float sclY = (32768.0f / (float)osu->getScreenHeight());

		if (-tLS.x + bRS.x > sclX)
			scaleToApplyAfterTranslationX = sclX / (-tLS.x + bRS.x);
		if (-tLS.y + bRS.y > sclY)
			scaleToApplyAfterTranslationY = sclY / (-tLS.y + bRS.y);
	}
	*/

	// draw entire slider into framebuffer
	g->setDepthBuffer(true);
	g->setBlending(false);
	{
		osu->getSliderFrameBuffer()->enable();
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
			{
				// draw curve mesh
				{
					vao->setDrawPercent(from, to, UNIT_CIRCLE_VAO_TRIANGLES->getVertices().size());
					g->pushTransform();
					{
						g->scale(scale, scale);
						g->translate(translation.x, translation.y);
						///g->scale(scaleToApplyAfterTranslationX, scaleToApplyAfterTranslationY); // aspire slider distortions

#ifdef MCENGINE_FEATURE_OPENGLES

						if (!osu_slider_use_gradient_image.getBool())
						{
							OpenGLES2Interface *gles2 = dynamic_cast<OpenGLES2Interface*>(g);
							if (gles2 != NULL)
							{
								gles2->forceUpdateTransform();
								Matrix4 mvp = gles2->getMVP();
								BLEND_SHADER->setUniformMatrix4fv("mvp", mvp);
							}
						}

#endif

						g->drawVAO(vao);
					}
					g->popTransform();

					if (alwaysPoints.size() > 0)
						drawFillSliderBodyPeppy(g, osu, alwaysPoints, UNIT_CIRCLE_VAO_BAKED, hitcircleDiameter/2.0f, 0, alwaysPoints.size(), BLEND_SHADER);
				}
			}

			if (!osu_slider_use_gradient_image.getBool())
				BLEND_SHADER->disable();
		}
		osu->getSliderFrameBuffer()->disable();
	}
	g->setBlending(true);
	g->setDepthBuffer(false);

	osu->getSliderFrameBuffer()->setColor(COLORf(alpha*osu_slider_alpha_multiplier.getFloat(), 1.0f, 1.0f, 1.0f));
	osu->getSliderFrameBuffer()->draw(g, 0, 0);
}

void OsuSliderRenderer::drawVR(Graphics *g, Osu *osu, OsuVR *vr, Matrix4 &mvp, float approachScale, const std::vector<Vector2> &points, const std::vector<Vector2> &alwaysPoints, float hitcircleDiameter, float from, float to, Color color, float alpha, long sliderTimeForRainbow)
{
	if (osu_slider_alpha_multiplier.getFloat() <= 0.0f || alpha <= 0.0f) return;

#if defined(MCENGINE_FEATURE_OPENGL)

	const bool isOpenGLRendererHack = (dynamic_cast<OpenGLLegacyInterface*>(g) != NULL || dynamic_cast<OpenGL3Interface*>(g) != NULL);

#elif defined(MCENGINE_FEATURE_OPENGLES)

	//const bool isOpenGLRendererHack = (dynamic_cast<OpenGLES2Interface*>(g) != NULL);

#endif

	checkUpdateVars(osu, hitcircleDiameter);

	const int drawFromIndex = clamp<int>((int)std::round(points.size() * from), 0, points.size());
	const int drawUpToIndex = clamp<int>((int)std::round(points.size() * to), 0, points.size());

	// draw entire slider into framebuffer
	g->setDepthBuffer(true);
	//g->setBlending(false);
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

		//if (!osu_slider_use_gradient_image.getBool())
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
		{
#if defined(MCENGINE_FEATURE_OPENGL)

			if (isOpenGLRendererHack)
				glBlendEquation(GL_MAX); // HACKHACK: OpenGL hardcoded

#endif

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

#if defined(MCENGINE_FEATURE_OPENGL)

			if (isOpenGLRendererHack)
				glBlendEquation(GL_FUNC_ADD); // HACKHACK: OpenGL hardcoded

#endif

			//if (!osu_slider_use_gradient_image.getBool())
				BLEND_SHADER_VR->disable();
		}
	}
	//g->setBlending(true);
	g->setDepthBuffer(false);

	vr->getShaderTexturedLegacyGeneric()->enable();
}

void OsuSliderRenderer::drawVR(Graphics *g, Osu *osu, OsuVR *vr, Matrix4 &mvp, float approachScale, VertexArrayObject *vao1, VertexArrayObject *vao2, const std::vector<Vector2> &alwaysPoints, float hitcircleDiameter, float from, float to, Color color, float alpha, long sliderTimeForRainbow)
{
	if (osu_slider_alpha_multiplier.getFloat() <= 0.0f || alpha <= 0.0f || vao1 == NULL || vao2 == NULL) return;

#if defined(MCENGINE_FEATURE_OPENGL)

	const bool isOpenGLRendererHack = (dynamic_cast<OpenGLLegacyInterface*>(g) != NULL || dynamic_cast<OpenGL3Interface*>(g) != NULL);

#elif defined(MCENGINE_FEATURE_OPENGLES)

	//const bool isOpenGLRendererHack = (dynamic_cast<OpenGLES2Interface*>(g) != NULL);

#endif

	checkUpdateVars(osu, hitcircleDiameter);

	// draw entire slider into framebuffer
	g->setDepthBuffer(true);
	//g->setBlending(false);
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

		//if (!osu_slider_use_gradient_image.getBool())
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
		{

#if defined(MCENGINE_FEATURE_OPENGL)

			if (isOpenGLRendererHack)
				glBlendEquation(GL_MAX); // HACKHACK: OpenGL hardcoded

#endif

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

#if defined(MCENGINE_FEATURE_OPENGL)

			if (isOpenGLRendererHack)
				glBlendEquation(GL_FUNC_ADD); // HACKHACK: OpenGL hardcoded

#endif

			//if (!osu_slider_use_gradient_image.getBool())
				BLEND_SHADER_VR->disable();
		}
	}
	//g->setBlending(true);
	g->setDepthBuffer(false);

	vr->getShaderTexturedLegacyGeneric()->enable();
}

void OsuSliderRenderer::drawMM(Graphics *g, Osu *osu, const std::vector<Vector2> &points, float hitcircleDiameter, float from, float to, Color color, float alpha, long sliderTimeForRainbow)
{
	if (osu_slider_alpha_multiplier.getFloat() <= 0.0f || alpha <= 0.0f) return;

	checkUpdateVars(osu, hitcircleDiameter);

	// TODO: shit
	int numPointsTotal = points.size();
	///int numPointsTotal = 0;
	///for (int i=0; i<points.size(); i++)
	///{
	///	numPointsTotal += points[i].size();
	///}

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
	///osu->getSliderFrameBuffer()->drawRect(g, m_fBoundingBoxMinX, m_fBoundingBoxMinY, m_fBoundingBoxMaxX - m_fBoundingBoxMinX, m_fBoundingBoxMaxY - m_fBoundingBoxMinY);
	osu->getSliderFrameBuffer()->draw(g, 0, 0);
}

void OsuSliderRenderer::drawFillSliderBodyPeppy(Graphics *g, Osu *osu, const std::vector<Vector2> &points, VertexArrayObject *circleMesh, float radius, int drawFromIndex, int drawUpToIndex, Shader *shader)
{
	if (drawFromIndex < 0)
		drawFromIndex = 0;
	if (drawUpToIndex < 0)
		drawUpToIndex = points.size();

#ifdef MCENGINE_FEATURE_OPENGLES

	OpenGLES2Interface *gles2 = dynamic_cast<OpenGLES2Interface*>(g);

#endif

	g->pushTransform();
	{
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

#ifdef MCENGINE_FEATURE_OPENGLES

			if (shader != NULL && gles2 != NULL)
			{
				gles2->forceUpdateTransform();
				Matrix4 mvp = gles2->getMVP();
				shader->setUniformMatrix4fv("mvp", mvp);
			}

#endif

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
	{
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
		{
			g->scale(scale, scale, 1.0f);
			g->translate(x, y, 0.25f);
			g->drawVAO(circleMesh);
		}
		g->popTransform();
	}
}

void OsuSliderRenderer::drawFillSliderBodyMM(Graphics *g, const std::vector<Vector2> &points, float radius, int drawFromIndex, int drawUpToIndex)
{
	// modified version of https://github.com/ppy/osu-framework/blob/master/osu.Framework/Graphics/Lines/Path_DrawNode.cs

	// TODO: remaining problems
	// 1) how to handle snaking? very very annoying to do via draw call bounds (due to inconsistent mesh topology)
	// 2) check performance for baked vao, maybe as a compatibility option for slower pcs
	// 3) recalculating every frame is not an option, way too slow
	// 4) since we already have smoothsnake begin+end circles, could precalculate a list of draw call bounds indices for snaking and shrinking respectively
	// 5) unbaked performance is way worse than legacy sliders, so it will only be used in baked form

	VertexArrayObject vao(Graphics::PRIMITIVE::PRIMITIVE_TRIANGLES);

	struct Helper
	{
		static inline Vector2 pointOnCircle(float angle) {return Vector2(std::sin(angle), -std::cos(angle));}

		static void addLineCap(Vector2 origin, float theta, float thetaDiff, float radius, VertexArrayObject &vao)
		{
			const float step = PI / 32.0f; // MAX_RES

			const float dir = sign<float>(thetaDiff);
			thetaDiff = dir * thetaDiff;

			const int amountPoints = (int)std::ceil(thetaDiff / step);

			if (amountPoints > 0)
			{
				if (dir < 0)
					theta += PI;

				Vector2 current = origin + pointOnCircle(theta) * radius;

				for (int p=1; p<=amountPoints; p++)
				{
					// center
					vao.addTexcoord(1, 0);
					vao.addVertex(origin.x, origin.y, MESH_CENTER_HEIGHT);

					// first outer point
					vao.addTexcoord(0, 0);
					vao.addVertex(current.x, current.y);

					const float angularOffset = std::min(p * step, thetaDiff);
					current = origin + pointOnCircle(theta + dir * angularOffset) * radius;

					// second outer point
					vao.addTexcoord(0, 0);
					vao.addVertex(current.x, current.y);
				}
			}
		}
	};

	float prevLineTheta = 0.0f;

	for (int i=1; i<points.size(); i++)
	{
		const Vector2 lineStartPoint = points[i - 1];
		const Vector2 lineEndPoint = points[i];

		const Vector2 lineDirection = (lineEndPoint - lineStartPoint);
		const Vector2 lineDirectionNormalized = Vector2(lineDirection.x, lineDirection.y).normalize();
		const Vector2 lineOrthogonalDirection = Vector2(-lineDirectionNormalized.y, lineDirectionNormalized.x);

		const float lineTheta = std::atan2(lineEndPoint.y - lineStartPoint.y, lineEndPoint.x - lineStartPoint.x);

		// start cap
		if (i == 1)
			Helper::addLineCap(lineStartPoint, lineTheta + PI, PI, radius, vao);

		// body
		{
			const Vector2 ortho = lineOrthogonalDirection;

			const Vector2 screenLineLeftStartPoint = lineStartPoint + ortho * radius;
			const Vector2 screenLineLeftEndPoint = lineEndPoint + ortho * radius;

			const Vector2 screenLineRightStartPoint = lineStartPoint - ortho * radius;
			const Vector2 screenLineRightEndPoint = lineEndPoint - ortho * radius;

			const Vector2 screenLineStartPoint = lineStartPoint;
			const Vector2 screenLineEndPoint = lineEndPoint;

			// type is triangles, build a rectangle out of 6 vertices

			//
			// 1   3   5
			// *---*---*
			// |  /|  /|
			// | / | / |     // the line 3-4 is the center of the slider (with a raised z-coordinate for blending)
			// |/  |/  |
			// *---*---*
			// 2   4   6
			//

			vao.addTexcoord(0, 0);
			vao.addVertex(screenLineLeftEndPoint.x, screenLineLeftEndPoint.y); // 1
			vao.addTexcoord(0, 0);
			vao.addVertex(screenLineLeftStartPoint.x, screenLineLeftStartPoint.y); // 2
			vao.addTexcoord(1, 0);
			vao.addVertex(screenLineEndPoint.x, screenLineEndPoint.y, MESH_CENTER_HEIGHT); // 3

			vao.addTexcoord(1, 0);
			vao.addVertex(screenLineEndPoint.x, screenLineEndPoint.y, MESH_CENTER_HEIGHT); // 3
			vao.addTexcoord(0, 0);
			vao.addVertex(screenLineLeftStartPoint.x, screenLineLeftStartPoint.y); // 2
			vao.addTexcoord(1, 0);
			vao.addVertex(screenLineStartPoint.x, screenLineStartPoint.y, MESH_CENTER_HEIGHT); // 4

			vao.addTexcoord(1, 0);
			vao.addVertex(screenLineEndPoint.x, screenLineEndPoint.y, MESH_CENTER_HEIGHT); // 3
			vao.addTexcoord(1, 0);
			vao.addVertex(screenLineStartPoint.x, screenLineStartPoint.y, MESH_CENTER_HEIGHT); // 4
			vao.addTexcoord(0, 0);
			vao.addVertex(screenLineRightEndPoint.x, screenLineRightEndPoint.y); // 5

			vao.addTexcoord(0, 0);
			vao.addVertex(screenLineRightEndPoint.x, screenLineRightEndPoint.y); // 5
			vao.addTexcoord(1, 0);
			vao.addVertex(screenLineStartPoint.x, screenLineStartPoint.y, MESH_CENTER_HEIGHT); // 4
			vao.addTexcoord(0, 0);
			vao.addVertex(screenLineRightStartPoint.x, screenLineRightStartPoint.y); // 6
		}

		// TODO: fix non-rolled-over theta causing full circle to be built at perfect angle points at e.g. reach out at 0:48
		// these would also break snaking, so we have to handle that special case and not generate any meshes there

		// wedges
		if (i > 1)
			Helper::addLineCap(lineStartPoint, prevLineTheta, lineTheta - prevLineTheta, radius, vao);

		// end cap
		if (i == (points.size() - 1))
			Helper::addLineCap(lineEndPoint, lineTheta, PI, radius, vao);

		prevLineTheta = lineTheta;
	}

	// draw it
	if (vao.getNumVertices() > 0)
	{
		if (osu_slider_debug_wireframe.getBool())
			g->setWireframe(true);

		// draw body
		g->drawVAO(&vao);

		if (osu_slider_debug_wireframe.getBool())
			g->setWireframe(false);
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
	}

	const int subdivisions = osu_slider_body_unit_circle_subdivisions.getInt();
	if (subdivisions != UNIT_CIRCLE_SUBDIVISIONS)
	{
		UNIT_CIRCLE_SUBDIVISIONS = subdivisions;

		// build unit cone
		{
			UNIT_CIRCLE.clear();

			// tip of the cone
			// texture coordinates
			UNIT_CIRCLE.push_back(1.0f);
			UNIT_CIRCLE.push_back(0.0f);

			// position
			UNIT_CIRCLE.push_back(0.0f);
			UNIT_CIRCLE.push_back(0.0f);
			UNIT_CIRCLE.push_back(MESH_CENTER_HEIGHT);

			for (int j=0; j<subdivisions; ++j)
			{
				float phase = j * (float) PI * 2.0f / subdivisions;

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
	// dynamic mods like minimize or wobble have to use the legacy renderer anyway, since the slider shape may change every frame
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
