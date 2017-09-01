//================ Copyright (c) 2017, PG, All rights reserved. =================//
//
// Purpose:		a simple VR button with an image/icon on top
//
// $NoKeywords: $osuvribt
//===============================================================================//

#include "OsuVRUIImageButton.h"

#include "Engine.h"
#include "ResourceManager.h"
#include "AnimationHandler.h"

#include "OsuVR.h"

OsuVRUIImageButton::OsuVRUIImageButton(OsuVR *vr, float x, float y, float width, float height, UString imageResourceName) : OsuVRUIButton(vr, x, y, width, height)
{
	m_sImageResourceName = imageResourceName;
	updateImageResource();

	m_fAnimation = 0.0f;
}

void OsuVRUIImageButton::drawVR(Graphics *g, Matrix4 &mvp)
{
	OsuVRUIButton::drawVR(g, mvp);
	if (!m_bIsVisible) return;
	if (m_image == NULL) return;

	// icon mesh
	VertexArrayObject vao(Graphics::PRIMITIVE::PRIMITIVE_QUADS);

	const Color color = m_bIsCursorInside ? COLORf(1.0f, 0.4f + 0.6f*m_fAnimation, 0.4f + 0.6f*m_fAnimation, 0.4f + 0.6f*m_fAnimation) : COLORf(1.0f, 0.4f, 0.4f, 0.4f);

	const float animationDistance = m_fAnimation * 0.02f;

	vao.addVertex(0, 0, animationDistance);
	vao.addTexcoord(0, 0);
	vao.addColor(color);
	vao.addVertex(m_vSize.x, 0, animationDistance);
	vao.addTexcoord(1, 0);
	vao.addColor(color);
	vao.addVertex(m_vSize.x, -m_vSize.y, animationDistance);
	vao.addTexcoord(1, 1);
	vao.addColor(color);
	vao.addVertex(0, -m_vSize.y, animationDistance);
	vao.addTexcoord(0, 1);
	vao.addColor(color);

	// position
	Matrix4 translation;
	translation.translate(m_vPos.x, m_vPos.y, 0);
	Matrix4 finalMVP = mvp * translation;

	// draw icon
	m_vr->getShaderTexturedGeneric()->enable();
	m_vr->getShaderTexturedGeneric()->setUniformMatrix4fv("matrix", finalMVP);
	{
		if (m_image != NULL)
			m_image->bind();

		g->drawVAO(&vao);

		if (m_image != NULL)
			m_image->unbind();
	}
	m_vr->getShaderTexturedGeneric()->disable();
}

void OsuVRUIImageButton::update(Vector2 cursorPos)
{
	OsuVRUIButton::update(cursorPos);
	if (!m_bIsVisible) return;

	if (m_image == NULL)
		updateImageResource();
}

void OsuVRUIImageButton::onCursorInside()
{
	anim->moveLinear(&m_fAnimation, 1.0f, 0.1f, true);
}

void OsuVRUIImageButton::onCursorOutside()
{
	anim->moveLinear(&m_fAnimation, 0.0f, 0.1f, true);
}

void OsuVRUIImageButton::updateImageResource()
{
	m_image = engine->getResourceManager()->getImage(m_sImageResourceName);
}
