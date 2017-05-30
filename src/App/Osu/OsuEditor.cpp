/*
 * OsuEditor.h
 *
 *  Created on: 30. Mai 2017
 *      Author: Psy
 */

#include "OsuEditor.h"

#include "Engine.h"
#include "ResourceManager.h"
#include "SoundEngine.h"
#include "ConVar.h"

#include "Osu.h"
#include "OsuSkin.h"

OsuEditor::OsuEditor(Osu *osu) : OsuScreenBackable(osu)
{

}

OsuEditor::~OsuEditor()
{
}

void OsuEditor::draw(Graphics *g)
{
	if (!m_bVisible) return;

	// draw back button on top of (after) everything else
	OsuScreenBackable::draw(g);
}

void OsuEditor::update()
{
	OsuScreenBackable::update();
	if (!m_bVisible) return;

	// update stuff if visible
}

void OsuEditor::onBack()
{
	engine->getSound()->play(m_osu->getSkin()->getMenuClick());

	m_osu->toggleEditor();
}

void OsuEditor::onResolutionChange(Vector2 newResolution)
{
	OsuScreenBackable::onResolutionChange(newResolution);

	debugLog("OsuEditor::onResolutionChange(%i, %i)\n", (int)newResolution.x, (int)newResolution.y);
}
