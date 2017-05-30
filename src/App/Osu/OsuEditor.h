/*
 * OsuEditor.h
 *
 *  Created on: 30. Mai 2017
 *      Author: Psy
 */

#ifndef OSUEDITOR_H
#define OSUEDITOR_H

#include "OsuScreenBackable.h"

class OsuEditor : public OsuScreenBackable
{
public:
	OsuEditor(Osu *osu);
	virtual ~OsuEditor();

	virtual void draw(Graphics *g);
	virtual void update();

	virtual void onResolutionChange(Vector2 newResolution);

private:
	virtual void onBack();
};

#endif
