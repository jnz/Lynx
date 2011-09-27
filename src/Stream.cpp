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

CStream::CStream(uint8_t* foreign, int size, int used)
{
    m_buffer = NULL;
    m_foreign = true;
    SetBuffer(foreign, size, used);
}

CStream::~CStream(void)
{
    if(m_buffer && !m_foreign)
        delete[] m_buffer;
}

void CStream::SetBuffer(uint8_t* buffer, int size, int used)
{
    if(m_buffer && !m_foreign)
        delete[] m_buffer;

    m_buffer = buffer;
    m_size = size;
    m_position = 0;
    m_used = used;
    m_foreign = true;
}

bool CStream::Resize(int newsize)
{
    uint8_t* newbuf;
    assert(newsize != m_size);

    newbuf = new uint8_t[newsize];
    assert(newbuf);
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

uint8_t* CStream::GetBuffer()
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

CStream CStream::GetStream()
{
    return CStream(m_buffer, m_size, m_used);
}

void CStream::WriteAdvance(int bytes)
{
    m_used += bytes;
}

void CStream::WriteDWORD(uint32_t value)
{
    assert(m_used + (int)sizeof(value) <= m_size);
    *((uint32_t*)(m_buffer+m_used)) = value;
    m_used += sizeof(value);
}

void CStream::WriteInt32(int32_t value)
{
    assert(m_used + (int)sizeof(value) <= m_size);
    *((int32_t*)(m_buffer+m_used)) = value;
    m_used += sizeof(value);
}

void CStream::WriteInt16(int16_t value)
{
    assert(m_used + (int)sizeof(value) <= m_size);
    *((int16_t*)(m_buffer+m_used)) = value;
    m_used += sizeof(value);
}

void CStream::WriteWORD(uint16_t value)
{
    assert(m_used + (int)sizeof(value) <= m_size);
    *((uint16_t*)(m_buffer+m_used)) = value;
    m_used += sizeof(value);
}

void CStream::WriteBYTE(uint8_t value)
{
    assert(m_used + (int)sizeof(value) <= m_size);
    *((uint8_t*)(m_buffer+m_used)) = value;
    m_used += sizeof(value);
}

void CStream::WriteChar(int8_t value)
{
    assert(m_used + (int)sizeof(value) <= m_size);
    *((int8_t*)(m_buffer+m_used)) = value;
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
    WriteBYTE((uint8_t)(value.x*256/360) & 255);
    WriteBYTE((uint8_t)(value.y*256/360) & 255);
    WriteBYTE((uint8_t)(value.z*256/360) & 255);
}

void CStream::WritePos6(const vec3_t& value)
{
    WriteInt16((int16_t)(value.x*8.0f));
    WriteInt16((int16_t)(value.y*8.0f));
    WriteInt16((int16_t)(value.z*8.0f));
}

void CStream::WriteFloat2(float value)
{
    WriteInt16((int16_t)(value*8.0f));
}

void CStream::WriteVec3(const vec3_t& value)
{
    WriteFloat(value.x);
    WriteFloat(value.y);
    WriteFloat(value.z);
}

void CStream::WriteQuat(const quaternion_t& value)
{
    WriteFloat(value.x);
    WriteFloat(value.y);
    WriteFloat(value.z);
    WriteFloat(value.w);
}

void CStream::WriteBytes(const uint8_t* values, int len)
{
    assert(m_used + len <= m_size);
    memcpy(m_buffer+m_used, values, len);
    m_used += len;
}

uint16_t CStream::WriteString(const std::string& value)
{
    assert(value.size() < USHRT_MAX);
    if(value.size() >= USHRT_MAX)
        return 0;
    uint16_t len = (uint16_t)value.size();
    WriteWORD(len);
    WriteBytes((uint8_t*)value.c_str(), len);
    return sizeof(uint16_t) + len;
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
    return sizeof(uint16_t) + value.size();
}

void CStream::ReadAdvance(int bytes)
{
    m_position += bytes;
}

void CStream::ReadDWORD(uint32_t* value)
{
    assert(m_position + (int)sizeof(uint32_t) <= m_used);
    *value = *((uint32_t*)(m_buffer+m_position));
    m_position += sizeof(uint32_t);
}

void CStream::ReadInt32(int32_t* value)
{
    assert(m_position + (int)sizeof(int32_t) <= m_used);
    *value = *((int32_t*)(m_buffer+m_position));
    m_position += sizeof(int32_t);
}

void CStream::ReadInt16(int16_t* value)
{
    assert(m_position + (int)sizeof(int16_t) <= m_used);
    *value = *((int16_t*)(m_buffer+m_position));
    m_position += sizeof(int16_t);
}

void CStream::ReadWORD(uint16_t* value)
{
    assert(m_position + (int)sizeof(uint16_t) <= m_used);
    *value = *((uint16_t*)(m_buffer+m_position));
    m_position += sizeof(uint16_t);
}

void CStream::ReadBYTE(uint8_t* value)
{
    assert(m_position + 1 <= m_used);
    *value = *((uint8_t*)(m_buffer+m_position));
    m_position += sizeof(uint8_t);
}

void CStream::ReadChar(int8_t* value)
{
    assert(m_position + 1 <= m_used);
    *value = *((int8_t*)(m_buffer+m_position));
    m_position += sizeof(int8_t);
}

void CStream::ReadFloat(float* value)
{
    assert(m_position + (int)sizeof(float) <= m_used);
    *value = *((float*)(m_buffer+m_position));
    m_position += sizeof(float);
}

void CStream::ReadAngle3(vec3_t* value)
{
    uint8_t bx, by, bz;
    ReadBYTE(&bx);
    ReadBYTE(&by);
    ReadBYTE(&bz);
    value->x = bx * (360.0f/256.0f);
    value->y = by * (360.0f/256.0f);
    value->z = bz * (360.0f/256.0f);
}

void CStream::ReadPos6(vec3_t* value)
{
    int16_t bx, by, bz;

    ReadInt16(&bx);
    ReadInt16(&by);
    ReadInt16(&bz);
    value->x = bx * (1.0f / 8.0f);
    value->y = by * (1.0f / 8.0f);
    value->z = bz * (1.0f / 8.0f);
}

void CStream::ReadFloat2(float* value)
{
    int16_t b;
    ReadInt16(&b);
    *value = b * (1.0f / 8.0f);
}

void CStream::ReadVec3(vec3_t* value)
{
    ReadFloat(&value->x);
    ReadFloat(&value->y);
    ReadFloat(&value->z);
}

void CStream::ReadQuat(quaternion_t* value)
{
    ReadFloat(&value->x);
    ReadFloat(&value->y);
    ReadFloat(&value->z);
    ReadFloat(&value->w);
}

void CStream::ReadBytes(uint8_t* values, int len)
{
    assert(m_position + len <= m_used);
    memcpy(values, m_buffer+m_position, len);
    m_position += len;
}

void CStream::ReadString(std::string* value)
{
    assert((uint32_t)GetBytesToRead() >= sizeof(uint16_t));
    if((uint32_t)GetBytesToRead() < sizeof(uint16_t))
        return;
    uint16_t len;
    ReadWORD(&len);
    (*value).assign((char*)(m_buffer+m_position), len);
    ReadAdvance(len);
}

