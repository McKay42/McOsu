//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		osu type file io helper
//
// $NoKeywords: $osudb
//===============================================================================//

#ifndef OSUFILE_H
#define OSUFILE_H

#include "cbase.h"

class File;

class OsuFile
{
public:
	struct TIMINGPOINT
	{
		double msPerBeat;
		double offset;
		bool notinherited;
	};

public:
	OsuFile(UString filepath, bool read = true);
	virtual ~OsuFile();

	inline bool isReady() {return m_bReady;}

	unsigned char readByte();
	short readShort();
	int readInt();
	long readLong();
	uint64_t readULEB128();
	float readFloat();
	double readDouble();
	bool readBool();
	UString readString();
	void readDateTime();
	TIMINGPOINT readTimingPoint();

private:
	uint64_t decodeULEB128(const uint8_t *p, unsigned *n = NULL);

	File *m_file;
	size_t m_iFileSize;
	const char *m_buffer;
	const char *m_readPointer;
	bool m_bReady;
};

#endif
