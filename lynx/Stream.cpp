#include <stdio.h>
#include <string.h>
#include <enet/enet.h>
#include "Stream.h"

#ifdef _DEBUG
#include <crtdbg.h>
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#endif

CStream::CStream(void)
{
	m_buffer = NULL;
	m_size = 0;
	m_position = 0;
	m_used = 0;
	m_foreign = false;
}

CStream::CStream(int size)
{
	m_position = 0;
	m_used = 0;
	m_foreign = false;
    m_buffer = NULL;
	if(!Resize(size))
		CStream();
}

CStream::CStream(BYTE* foreign, int size, int used)
{
	SetBuffer(foreign, size, used);
}

CStream::~CStream(void)
{
	if(m_buffer && !m_foreign)
		delete[] m_buffer;
}

void CStream::SetBuffer(BYTE* buffer, int size, int used)
{
	m_buffer = buffer;
	m_size = size;
	m_position = 0;
	m_used = used;
	m_foreign = true;
}

bool CStream::Resize(int newsize)
{
	BYTE* newbuf;
	assert(newsize != m_size);

	newbuf = new BYTE[newsize];
	if(!newbuf)
		return false;

	if(m_buffer)
	{
		if(m_used > 0 && newsize >= m_used)
			memcpy(newbuf, m_buffer, m_position);
		else
		{
			m_position = 0;
			m_used = 0;
		}

		if(!m_foreign)
			delete[] m_buffer;
	}
	m_buffer = newbuf;
	m_size = newsize;

	return true;
}

BYTE* CStream::GetBuffer()
{
	return m_buffer;
}

int CStream::GetBufferSize()
{
	return m_size;
}

void CStream::ResetWritePosition()
{
	m_used = 0;
}

int CStream::GetSpaceLeft()
{
	return m_size - m_used;
}

int CStream::GetBytesWritten() const
{
	return m_used;
}

void CStream::ResetReadPosition()
{
	m_position = 0;
}

int CStream::GetBytesToRead()
{
	return m_used - m_position;
}

int CStream::GetBytesRead()
{
	return m_position;
}

void CStream::WriteDWORD(DWORD value)
{
	assert(m_used + (int)sizeof(value) <= m_size);
	*((DWORD*)(m_buffer+m_used)) = value;
	m_used += sizeof(value);
}

void CStream::WriteInt32(INT32 value)
{
	assert(m_used + (int)sizeof(value) <= m_size);
	*((INT32*)(m_buffer+m_used)) = value;
	m_used += sizeof(value);
}

void CStream::WriteInt16(INT16 value)
{
	assert(m_used + (int)sizeof(value) <= m_size);
	*((INT16*)(m_buffer+m_used)) = value;
	m_used += sizeof(value);
}

void CStream::WriteWORD(WORD value)
{
	assert(m_used + (int)sizeof(value) <= m_size);
	*((WORD*)(m_buffer+m_used)) = value;
	m_used += sizeof(value);
}

void CStream::WriteBYTE(BYTE value)
{
	assert(m_used + (int)sizeof(value) <= m_size);
	*((BYTE*)(m_buffer+m_used)) = value;
	m_used += sizeof(value);
}

void CStream::WriteFloat(float value)
{
	assert(m_used + (int)sizeof(value) <= m_size);
	*((float*)(m_buffer+m_used)) = value;
	m_used += sizeof(value);
}

void CStream::WriteAngle3(const vec3_t& value)
{
	WriteBYTE((BYTE)(value.x*256/360) & 255);
	WriteBYTE((BYTE)(value.y*256/360) & 255);
	WriteBYTE((BYTE)(value.z*256/360) & 255);
}

void CStream::WritePos6(const vec3_t& value)
{
	WriteInt16((INT16)(value.x*8.0f));
	WriteInt16((INT16)(value.y*8.0f));
	WriteInt16((INT16)(value.z*8.0f));
}

void CStream::WriteFloat2(float value)
{
	WriteInt16((INT16)(value*8.0f));
}

void CStream::WriteVec3(const vec3_t& value)
{
	WriteFloat(value.v[0]);
	WriteFloat(value.v[1]);
	WriteFloat(value.v[2]);
}

void CStream::WriteBytes(const BYTE* values, int len)
{
	assert(m_used + len <= m_size);
	memcpy(m_buffer+m_used, values, len);
	m_used += len;
}

WORD CStream::WriteString(const std::string& value)
{
	assert(value.size() < USHRT_MAX);
	if(value.size() >= USHRT_MAX)
		return 0;
	WORD len = (WORD)value.size();
	WriteWORD(len);
	WriteBytes((BYTE*)value.c_str(), len);
	return sizeof(WORD) + len;
}

void CStream::WriteStream(const CStream& stream)
{
    //assert(stream.GetBytesWritten() > 0);
    WriteBytes(stream.m_buffer, stream.GetBytesWritten());
}

size_t CStream::StringSize(const std::string& value)
{
	assert(value.size() < USHRT_MAX);
	if(value.size() >= USHRT_MAX)
		return 0;
	return sizeof(WORD) + value.size();
}

void CStream::ReadAdvance(int bytes)
{
	m_position += bytes;
}

void CStream::ReadDWORD(DWORD* value)
{
	assert(m_position + (int)sizeof(DWORD) <= m_used);
	*value = *((DWORD*)(m_buffer+m_position));
	m_position += sizeof(DWORD);
}

void CStream::ReadInt32(INT32* value)
{
	assert(m_position + (int)sizeof(INT32) <= m_used);
	*value = *((INT32*)(m_buffer+m_position));
	m_position += sizeof(INT32);
}

void CStream::ReadInt16(INT16* value)
{
	assert(m_position + (int)sizeof(INT16) <= m_used);
	*value = *((INT16*)(m_buffer+m_position));
	m_position += sizeof(INT16);
}

void CStream::ReadWORD(WORD* value)
{
	assert(m_position + (int)sizeof(WORD) <= m_used);
	*value = *((WORD*)(m_buffer+m_position));
	m_position += sizeof(WORD);
}

void CStream::ReadBYTE(BYTE* value)
{ 
	assert(m_position + 1 <= m_used);
	*value = *((BYTE*)(m_buffer+m_position));
	m_position += sizeof(BYTE);
}

void CStream::ReadFloat(float* value)
{
	assert(m_position + (int)sizeof(float) <= m_used);
	*value = *((float*)(m_buffer+m_position));
	m_position += sizeof(float);
}

void CStream::ReadAngle3(vec3_t* value)
{
	BYTE bx, by, bz;
	ReadBYTE(&bx);
	ReadBYTE(&by);
	ReadBYTE(&bz);
	value->x = bx * (360.0f/256.0f);
	value->y = by * (360.0f/256.0f);
	value->z = bz * (360.0f/256.0f);
}

void CStream::ReadPos6(vec3_t* value)
{
	INT16 bx, by, bz;

	ReadInt16(&bx);
	ReadInt16(&by);
	ReadInt16(&bz);
	value->x = bx * (1.0f / 8.0f);
	value->y = by * (1.0f / 8.0f);
	value->z = bz * (1.0f / 8.0f);
}

void CStream::ReadFloat2(float* value)
{
	INT16 b;
	ReadInt16(&b);
	*value = b * (1.0f / 8.0f);
}

void CStream::ReadVec3(vec3_t* value)
{
	for(int i=0;i<3;i++)
		ReadFloat(&value->v[i]);
}

void CStream::ReadBytes(BYTE* values, int len)
{
	assert(m_position + len <= m_used);
	memcpy(values, m_buffer+m_position, len);
	m_position += len;
}

void CStream::ReadString(std::string* value)
{
	assert(GetBytesToRead() >= sizeof(WORD)+1);
	if(GetBytesToRead() < sizeof(WORD)+1)
		return;
	WORD len;
	ReadWORD(&len);
	(*value).assign((char*)(m_buffer+m_position), len);
	ReadAdvance(len);
}