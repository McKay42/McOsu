//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		osu type file io helper
//
// $NoKeywords: $osudb
//===============================================================================//

#include "OsuFile.h"
#include "Engine.h"
#include "File.h"

OsuFile::OsuFile(UString filepath, bool read)
{
	m_bReady = false;
	m_iFileSize = 0;
	m_buffer = NULL;
	m_readPointer = NULL;

	// open and read everything
	m_file = new File(filepath);
	if (m_file->canRead() && read)
	{
		m_iFileSize = m_file->getFileSize();
		m_buffer = m_file->readFile();
		m_readPointer = m_buffer;
		m_bReady = true;
	}
}

OsuFile::~OsuFile()
{
	SAFE_DELETE(m_file);
}

unsigned char OsuFile::readByte()
{
	if (!m_bReady || m_readPointer > m_buffer+m_iFileSize-1) return 0;

	const unsigned char value = (unsigned char)*m_readPointer;
	m_readPointer += 1;
	return value;
}

short OsuFile::readShort()
{
	if (!m_bReady || m_readPointer > m_buffer+m_iFileSize-1) return 0;

	const short value = (short)*(short*)m_readPointer;
	m_readPointer += 2;
	return value;
}

int OsuFile::readInt()
{
	if (!m_bReady || m_readPointer > m_buffer+m_iFileSize-1) return 0;

	const int value = (int)*(int*)m_readPointer;
	m_readPointer += 4;
	return value;
}

long OsuFile::readLong()
{
	if (!m_bReady || m_readPointer > m_buffer+m_iFileSize-1) return 0;

	const long value = (long)*(long*)m_readPointer;
	m_readPointer += 8;
	return value;
}

uint64_t OsuFile::readULEB128()
{
	if (!m_bReady || m_readPointer > m_buffer+m_iFileSize-1) return 0;

	unsigned int numBytes = 0;
	uint64_t value = decodeULEB128((const unsigned char*)m_readPointer, &numBytes);
	m_readPointer += numBytes;
	return value;
}

float OsuFile::readFloat()
{
	if (!m_bReady || m_readPointer > m_buffer+m_iFileSize-1) return 0;

	const float value = (float)*(float*)m_readPointer;
	m_readPointer += 4;
	return value;
}

double OsuFile::readDouble()
{
	if (!m_bReady || m_readPointer > m_buffer+m_iFileSize-1) return 0;

	const double value = (double)*(double*)m_readPointer;
	m_readPointer += 8;
	return value;
}

bool OsuFile::readBool()
{
	return (bool)readByte();
}

UString OsuFile::readString()
{
	if (!m_bReady || m_readPointer > m_buffer+m_iFileSize-1) return "";

	UString value = "";
	const unsigned char flag = readByte();
	if (flag > 0)
	{
		uint64_t strLength = readULEB128();

		if (strLength > 0)
		{
			std::vector<unsigned char> strData;
			for (uint64_t i=0; i<strLength; i++)
			{
				strData.push_back(readByte());
			}
			strData.push_back('\0');

			const char *rawStrData = (const char *)&strData[0];
			value = UString(rawStrData);
		}
	}
	return value;
}

void OsuFile::readDateTime()
{
	if (!m_bReady || m_readPointer > m_buffer+m_iFileSize-1) return;

	m_readPointer += 8;
}

OsuFile::TIMINGPOINT OsuFile::readTimingPoint()
{
	const double bpm = readDouble();
	const double offset = readDouble();
	const bool notinherited = (bool)readByte();
	return (struct TIMINGPOINT) {bpm, offset, notinherited};
}

uint64_t OsuFile::decodeULEB128(const uint8_t *p, unsigned *n)
{
	const uint8_t *backup = p;
	uint64_t value = 0;
	unsigned shift = 0;

	do
	{
		if (p > (const unsigned char*)(m_buffer+m_iFileSize-1))
			break;

		value += uint64_t(*p & 0x7f) << shift;
		shift += 7;
	}
	while (*p++ >= 128);

	if (n)
		*n = (unsigned)(p - backup);

	return value;
}
