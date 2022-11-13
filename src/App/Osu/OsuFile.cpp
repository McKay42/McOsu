//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		osu type file io helper
//
// $NoKeywords: $osudb
//===============================================================================//

#include "OsuFile.h"

#include "Engine.h"
#include "File.h"
#include "MD5.h"

constexpr const uint64_t OsuFile::MAX_STRING_LENGTH;

OsuFile::OsuFile(UString filepath, bool write, bool writeBufferOnly)
{
	m_bWrite = write;
	m_bReady = false;
	m_file = NULL;
	m_iFileSize = 0;
	m_buffer = NULL;
	m_readPointer = NULL;

	if (!writeBufferOnly)
	{
		m_file = new File(filepath, (write ? File::TYPE::WRITE : File::TYPE::READ));
		if (m_file->canRead() && !write)
		{
			m_iFileSize = m_file->getFileSize();
			m_buffer = m_file->readFile();
			m_readPointer = m_buffer;
			m_bReady = (m_buffer != NULL);
		}
		else if (m_file->canWrite() && write)
		{
			m_writeBuffer.reserve(1024*1024);
			m_bReady = true;
		}
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
}

void OsuFile::writeShort(int16_t val)
{
	m_writeBuffer.push_back(((const char *)&val)[0]);
	m_writeBuffer.push_back(((const char *)&val)[1]);
}

void OsuFile::writeInt(int32_t val)
{
	m_writeBuffer.push_back(((const char *)&val)[0]);
	m_writeBuffer.push_back(((const char *)&val)[1]);
	m_writeBuffer.push_back(((const char *)&val)[2]);
	m_writeBuffer.push_back(((const char *)&val)[3]);
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
}

void OsuFile::writeFloat(float val)
{
	m_writeBuffer.push_back(((const char *)&val)[0]);
	m_writeBuffer.push_back(((const char *)&val)[1]);
	m_writeBuffer.push_back(((const char *)&val)[2]);
	m_writeBuffer.push_back(((const char *)&val)[3]);
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
}

void OsuFile::writeBool(bool val)
{
	writeByte(val ? 1 : 0);
}

void OsuFile::writeString(UString &str)
{
	const bool flag = (str.lengthUtf8() > 0);
	writeByte(flag ? 0x0b : 0);
	if (flag)
	{
		writeULEB128(str.lengthUtf8());
		for (int i=0; i<str.lengthUtf8(); i++)
		{
			m_writeBuffer.push_back(str.toUtf8()[i]);
		}
	}
}

void OsuFile::writeStdString(std::string str)
{
	const bool flag = (str.length() > 0);
	writeByte(flag ? 0x0b : 0);
	if (flag)
	{
		writeULEB128(str.length());
		for (int i=0; i<str.length(); i++)
		{
			m_writeBuffer.push_back(str[i]);
		}
	}
}



unsigned char OsuFile::readByte()
{
	if (!m_bReady || m_readPointer >= (m_buffer + m_iFileSize)) return 0;

	const unsigned char value = (unsigned char)*m_readPointer;
	m_readPointer += 1;

	return value;
}

int16_t OsuFile::readShort()
{
	if (!m_bReady || (m_readPointer + 2) >= (m_buffer + m_iFileSize)) return 0;

	const int16_t value = (int16_t)*(int16_t*)m_readPointer;
	m_readPointer += 2;

	return value;
}

int32_t OsuFile::readInt()
{
	if (!m_bReady || (m_readPointer + 4) >= (m_buffer + m_iFileSize)) return 0;

	const int32_t value = (int32_t)*(int32_t*)m_readPointer;
	m_readPointer += 4;

	return value;
}

int64_t OsuFile::readLongLong()
{
	if (!m_bReady || (m_readPointer + 8) >= (m_buffer + m_iFileSize)) return 0;

	const int64_t value = (int64_t)*(int64_t*)m_readPointer;
	m_readPointer += 8;

	return value;
}

uint64_t OsuFile::readULEB128()
{
	if (!m_bReady || m_readPointer >= (m_buffer + m_iFileSize)) return 0;

	unsigned int numBytes = 0;
	const uint64_t value = decodeULEB128((const unsigned char*)m_readPointer, &numBytes);
	m_readPointer += numBytes;

	return value;
}

float OsuFile::readFloat()
{
	if (!m_bReady || (m_readPointer + 4) >= (m_buffer + m_iFileSize)) return 0;

	const float value = (float)*(float*)m_readPointer;
	m_readPointer += 4;

	return value;
}

double OsuFile::readDouble()
{
	if (!m_bReady || (m_readPointer + 8) >= (m_buffer + m_iFileSize)) return 0;

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
	UString value;

	const unsigned char flag = readByte();
	if (flag > 0)
	{
		const uint64_t strLength = readULEB128();

		if (strLength > 0)
		{
			std::vector<unsigned char> strData;
			strData.reserve(std::min(strLength + 1, MAX_STRING_LENGTH));
			for (uint64_t i=0; i<strLength; i++)
			{
				const unsigned char character = readByte();
				if (i < std::min(strLength, MAX_STRING_LENGTH - 1))
					strData.push_back(character);
			}
			strData.push_back('\0');

			if (strData.size() > 0)
			{
				const char *rawStrData = (const char*)&strData[0];
				value = UString(rawStrData, strData.size() - 1);
			}
		}
	}

	return value;
}

std::string OsuFile::readStdString()
{
	std::string value;

	const unsigned char flag = readByte();
	if (flag > 0)
	{
		const uint64_t strLength = readULEB128();

		if (strLength > 0)
		{
			value.reserve(std::min(strLength, MAX_STRING_LENGTH));

			for (uint64_t i=0; i<strLength; i++)
			{
				const unsigned char character = readByte();
				if (i < std::min(strLength, MAX_STRING_LENGTH))
					value += character;
			}
		}
	}

	return value;
}

void OsuFile::readDateTime()
{
	if (!m_bReady || (m_readPointer + 8) >= (m_buffer + m_iFileSize)) return;

	m_readPointer += 8;
}

OsuFile::TIMINGPOINT OsuFile::readTimingPoint()
{
	const double bpm = readDouble();
	const double offset = readDouble();
	const bool timingChange = (bool)readByte();
	return (struct TIMINGPOINT) {bpm, offset, timingChange};
}

void OsuFile::readByteArray()
{
	const int numBytes = readInt();
	for (int i=0; i<numBytes; i++)
	{
		readByte();
	}
}

uint64_t OsuFile::decodeULEB128(const uint8_t *p, unsigned int *n)
{
	const uint8_t *backup = p;
	uint64_t value = 0;
	unsigned shift = 0;

	do
	{
		if (p >= (const uint8_t*)(m_buffer + m_iFileSize))
			break;

		value += uint64_t(*p & 0x7f) << shift;
		shift += 7;
	}
	while (*p++ >= 128);

	if (n != NULL)
		*n = (unsigned int)(p - backup);

	return value;
}



std::string OsuFile::md5(const unsigned char *data, size_t numBytes)
{
	if (data == NULL || numBytes < 1) return "";

	const char *hexDigits = "0123456789abcdef";

	std::string md5Hash;
	{
		MD5 hasher;
		{
			hasher.update(data, numBytes);
			hasher.finalize();
		}
		const unsigned char *rawMD5Hash = hasher.getDigest();

		for (int i=0; i<16; i++)
		{
			const size_t index1 = (rawMD5Hash[i] >> 4) & 0xf;	// md5hash[i] / 16
			const size_t index2 = (rawMD5Hash[i] & 0xf);		// md5hash[i] % 16

			if (index1 > 15 || index2 > 15)
				continue;

			md5Hash += hexDigits[index1];
			md5Hash += hexDigits[index2];
		}
	}
	return md5Hash;
}
