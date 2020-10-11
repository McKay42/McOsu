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
		bool timingChange;
	};

public:
	OsuFile(UString filepath, bool write = false);
	virtual ~OsuFile();

	inline bool isReady() const {return m_bReady;}

	void write();

	// write
	void writeByte(unsigned char val);
	void writeShort(int16_t val);
	void writeInt(int32_t val);
	void writeLongLong(int64_t val);
	void writeULEB128(uint64_t val);
	void writeFloat(float val);
	void writeDouble(double val);
	void writeBool(bool val);
	void writeString(UString &str);
	void writeStdString(std::string str);

	// read
	unsigned char readByte();
	int16_t readShort();
	int32_t readInt();
	int64_t readLongLong();
	uint64_t readULEB128();
	float readFloat();
	double readDouble();
	bool readBool();
	UString readString();
	std::string readStdString();
	void readDateTime();
	TIMINGPOINT readTimingPoint();
	void readByteArray();

private:
	uint64_t decodeULEB128(const uint8_t *p, unsigned *n = NULL);

	File *m_file;
	size_t m_iFileSize;
	const char *m_buffer;
	const char *m_readPointer;
	bool m_bReady;
	bool m_bWrite;

	std::vector<char> m_writeBuffer;
};

#endif
