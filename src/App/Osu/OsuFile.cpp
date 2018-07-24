//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		osu type file io helper
//
// $NoKeywords: $osudb
//===============================================================================//

#include "OsuFile.h"
#include "Engine.h"
#include "File.h"

OsuFile::OsuFile(UString filepath, bool write)
{
	m_bWrite = write;
	m_bReady = false;
	m_iFileSize = 0;
	m_buffer = NULL;
	m_readPointer = NULL;

	// open and read everything
	m_file = new File(filepath, (write ? File::TYPE::WRITE : File::TYPE::READ));
	if (m_file->canRead() && !write)
	{
		m_iFileSize = m_file->getFileSize();
		m_buffer = m_file->readFile();
		m_readPointer = m_buffer;
		m_bReady = true;
	}
	else if (m_file->canWrite() && write)
	{
		m_writeBuffer.reserve(1024*1024); // 1 MB should be good, my db is ~700K atm
		m_bReady = true;
	}
}

OsuFile::~OsuFile()
{
	write();
	SAFE_DELETE(m_file);
}

void OsuFile::write()
{
	if (!m_bReady || !m_bWrite) return;

	if (m_writeBuffer.size() > 0)
		m_file->write((const char *)&m_writeBuffer[0], m_writeBuffer.size());

	m_bReady = false;
}

void OsuFile::writeByte(unsigned char val)
{
	m_writeBuffer.push_back(val);
	//m_file->write((const char *)&val, 1);
}

void OsuFile::writeShort(int16_t val)
{
	m_writeBuffer.push_back(((const char *)&val)[0]);
	m_writeBuffer.push_back(((const char *)&val)[1]);
	//m_file->write((const char *)&val, 2);
}

void OsuFile::writeInt(int32_t val)
{
	m_writeBuffer.push_back(((const char *)&val)[0]);
	m_writeBuffer.push_back(((const char *)&val)[1]);
	m_writeBuffer.push_back(((const char *)&val)[2]);
	m_writeBuffer.push_back(((const char *)&val)[3]);
	//m_file->write((const char *)&val, 4);
}

void OsuFile::writeLongLong(int64_t val)
{
	m_writeBuffer.push_back(((const char *)&val)[0]);
	m_writeBuffer.push_back(((const char *)&val)[1]);
	m_writeBuffer.push_back(((const char *)&val)[2]);
	m_writeBuffer.push_back(((const char *)&val)[3]);
	m_writeBuffer.push_back(((const char *)&val)[4]);
	m_writeBuffer.push_back(((const char *)&val)[5]);
	m_writeBuffer.push_back(((const char *)&val)[6]);
	m_writeBuffer.push_back(((const char *)&val)[7]);
	//m_file->write((const char *)&val, 8);
}

void OsuFile::writeULEB128(uint64_t val)
{
	std::vector<uint8_t> bytes;
	do
	{
		uint8_t byte = val & 0x7f;
		val >>= 7;

		if (val != 0)
			byte |= 0x80;

		bytes.push_back(byte);
	}
	while (val != 0);

	m_writeBuffer.insert(m_writeBuffer.end(), bytes.begin(), bytes.end());
	//m_file->write((const char *)&bytes[0], bytes.size());
}

void OsuFile::writeFloat(float val)
{
	m_writeBuffer.push_back(((const char *)&val)[0]);
	m_writeBuffer.push_back(((const char *)&val)[1]);
	m_writeBuffer.push_back(((const char *)&val)[2]);
	m_writeBuffer.push_back(((const char *)&val)[3]);
	//m_file->write((const char *)&val, 4);
}

void OsuFile::writeDouble(double val)
{
	m_writeBuffer.push_back(((const char *)&val)[0]);
	m_writeBuffer.push_back(((const char *)&val)[1]);
	m_writeBuffer.push_back(((const char *)&val)[2]);
	m_writeBuffer.push_back(((const char *)&val)[3]);
	m_writeBuffer.push_back(((const char *)&val)[4]);
	m_writeBuffer.push_back(((const char *)&val)[5]);
	m_writeBuffer.push_back(((const char *)&val)[6]);
	m_writeBuffer.push_back(((const char *)&val)[7]);
	//m_file->write((const char *)&val, 8);
}

void OsuFile::writeBool(bool val)
{
	writeByte(val ? 1 : 0);
}

void OsuFile::writeString(UString &str)
{
	const bool flag = (str.length() > 0);
	writeByte(flag ? 1 : 0);
	if (flag)
	{
		writeULEB128(str.length());
		for (int i=0; i<str.length(); i++)
		{
			m_writeBuffer.push_back(str.toUtf8()[i]);
		}
		//m_file->write(str.toUtf8(), str.length());
	}
}

void OsuFile::writeStdString(std::string str)
{
	const bool flag = (str.length() > 0);
	writeByte(flag ? 1 : 0);
	if (flag)
	{
		writeULEB128(str.length());
		for (int i=0; i<str.length(); i++)
		{
			m_writeBuffer.push_back(str[i]);
		}
		//m_file->write(str.c_str(), str.length());
	}
}



unsigned char OsuFile::readByte()
{
	if (!m_bReady || m_readPointer > (m_buffer + m_iFileSize - 1)) return 0;

	const unsigned char value = (unsigned char)*m_readPointer;
	m_readPointer += 1;
	return value;
}

int16_t OsuFile::readShort()
{
	if (!m_bReady || m_readPointer > (m_buffer + m_iFileSize - 2)) return 0;

	const int16_t value = (int16_t)*(int16_t*)m_readPointer;
	m_readPointer += 2;
	return value;
}

int32_t OsuFile::readInt()
{
	if (!m_bReady || m_readPointer > (m_buffer + m_iFileSize - 4)) return 0;

	const int32_t value = (int32_t)*(int32_t*)m_readPointer;
	m_readPointer += 4;
	return value;
}

int64_t OsuFile::readLongLong()
{
	if (!m_bReady || m_readPointer > (m_buffer + m_iFileSize - 8)) return 0;

	const int64_t value = (int64_t)*(int64_t*)m_readPointer;
	m_readPointer += 8;
	return value;
}

uint64_t OsuFile::readULEB128()
{
	if (!m_bReady || m_readPointer > (m_buffer + m_iFileSize - 1)) return 0;

	unsigned int numBytes = 0;
	uint64_t value = decodeULEB128((const unsigned char*)m_readPointer, &numBytes);
	m_readPointer += numBytes;
	return value;
}

float OsuFile::readFloat()
{
	if (!m_bReady || m_readPointer > (m_buffer + m_iFileSize - 4)) return 0;

	const float value = (float)*(float*)m_readPointer;
	m_readPointer += 4;
	return value;
}

double OsuFile::readDouble()
{
	if (!m_bReady || m_readPointer > (m_buffer + m_iFileSize - 8)) return 0;

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

std::string OsuFile::readStdString()
{
	std::string value;
	value.reserve(32);

	const unsigned char flag = readByte();
	if (flag > 0)
	{
		uint64_t strLength = readULEB128();

		if (strLength > 0)
		{
			for (uint64_t i=0; i<strLength; i++)
			{
				value += readByte();
			}
		}
	}
	return value;
}

void OsuFile::readDateTime()
{
	if (!m_bReady || m_readPointer > (m_buffer + m_iFileSize - 8)) return;

	m_readPointer += 8;
}

OsuFile::TIMINGPOINT OsuFile::readTimingPoint()
{
	const double bpm = readDouble();
	const double offset = readDouble();
	const bool notinherited = (bool)readByte();
	return (struct TIMINGPOINT) {bpm, offset, notinherited};
}

void OsuFile::readByteArray()
{
	const int numBytes = readInt();
	for (int i=0; i<numBytes; i++)
	{
		readByte();
	}
}

uint64_t OsuFile::decodeULEB128(const uint8_t *p, unsigned *n)
{
	const uint8_t *backup = p;
	uint64_t value = 0;
	unsigned shift = 0;

	do
	{
		if (p > (const unsigned char*)(m_buffer + m_iFileSize - 1))
			break;

		value += uint64_t(*p & 0x7f) << shift;
		shift += 7;
	}
	while (*p++ >= 128);

	if (n)
		*n = (unsigned)(p - backup);

	return value;
}
