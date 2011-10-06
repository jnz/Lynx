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
    m_writeoverflow = false;
    m_readoverflow = false;
}

CStream::CStream(unsigned int size)
{
    m_position = 0;
    m_used = 0;
    m_foreign = false;
    m_buffer = NULL;
    m_writeoverflow = false;
    m_readoverflow = false;
    if(!Resize(size))
        CStream();
}

CStream::CStream(uint8_t* foreign, unsigned int size, unsigned int used)
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

void CStream::SetBuffer(uint8_t* buffer, unsigned int size, unsigned int used)
{
    if(m_buffer && !m_foreign)
        delete[] m_buffer;

    m_buffer = buffer;
    m_size = size;
    m_position = 0;
    m_used = used;
    m_foreign = true;
    m_writeoverflow = false;
    m_readoverflow = false;
}

bool CStream::Resize(unsigned int newsize)
{
    uint8_t* newbuf;
    assert(newsize != m_size);
    if(newsize == m_size)
        return true;

    newbuf = new uint8_t[newsize + 8]; // 8 bytes for a better sleep
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
            m_writeoverflow = false;
            m_readoverflow = false;
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

unsigned int CStream::GetBufferSize() const
{
    return m_size;
}

bool CStream::GetWriteOverflow() const
{
    return m_writeoverflow;
}

void CStream::ResetWritePosition()
{
    m_used = 0;
    m_writeoverflow = false;
}

bool CStream::GetReadOverflow() const
{
    return m_readoverflow;
}

unsigned int CStream::GetSpaceLeft() const
{
    assert(m_size >= m_used);
    return m_size - m_used;
}

unsigned int CStream::GetBytesWritten() const
{
    return m_used;
}

void CStream::ResetReadPosition()
{
    m_position = 0;
}

unsigned int CStream::GetBytesToRead() const
{
    return m_used - m_position;
}

unsigned int CStream::GetBytesRead() const
{
    return m_position;
}

CStream CStream::GetStream()
{
    return CStream(m_buffer, m_size, m_used);
}

void CStream::WriteAdvance(unsigned int bytes)
{
    if(m_used + bytes > m_size)
    {
        assert(0);
        m_writeoverflow = true;
        return;
    }
    m_used += bytes;
}

void CStream::WriteDWORD(uint32_t value)
{
    if(m_used + sizeof(value) > m_size)
    {
        assert(0);
        m_writeoverflow = true;
        return;
    }
    *((uint32_t*)(m_buffer+m_used)) = value;
    m_used += sizeof(value);
}

void CStream::WriteInt32(int32_t value)
{
    if(m_used + sizeof(value) > m_size)
    {
        assert(0);
        m_writeoverflow = true;
        return;
    }
    *((int32_t*)(m_buffer+m_used)) = value;
    m_used += sizeof(value);
}

void CStream::WriteInt16(int16_t value)
{
    if(m_used + sizeof(value) > m_size)
    {
        assert(0);
        m_writeoverflow = true;
        return;
    }
    *((int16_t*)(m_buffer+m_used)) = value;
    m_used += sizeof(value);
}

void CStream::WriteWORD(uint16_t value)
{
    if(m_used + sizeof(value) > m_size)
    {
        assert(0);
        m_writeoverflow = true;
        return;
    }
    *((uint16_t*)(m_buffer+m_used)) = value;
    m_used += sizeof(value);
}

void CStream::WriteBYTE(uint8_t value)
{
    if(m_used + sizeof(value) > m_size)
    {
        assert(0);
        m_writeoverflow = true;
        return;
    }
    *((uint8_t*)(m_buffer+m_used)) = value;
    m_used += sizeof(value);
}

void CStream::WriteChar(int8_t value)
{
    if(m_used + sizeof(value) > m_size)
    {
        assert(0);
        m_writeoverflow = true;
        return;
    }
    *((int8_t*)(m_buffer+m_used)) = value;
    m_used += sizeof(value);
}

void CStream::WriteFloat(float value)
{
    if(m_used + sizeof(value) > m_size)
    {
        assert(0);
        m_writeoverflow = true;
        return;
    }
    *((float*)(m_buffer+m_used)) = value;
    m_used += sizeof(value);
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

void CStream::WriteQuatUnit(const quaternion_t& value)
{
    const quaternion_t q = value.Normalized();
    WriteFloat(value.x);
    WriteFloat(value.y);
    WriteFloat(value.z);
}

void CStream::WriteBytes(const uint8_t* values, unsigned int len)
{
    if(m_used + len > m_size)
    {
        assert(0);
        m_writeoverflow = true;
        return;
    }
    memcpy(m_buffer+m_used, values, len);
    m_used += len;
}

uint16_t CStream::WriteString(const std::string& value)
{
    if(value.size() >= USHRT_MAX)
    {
        assert(0);
        return 0;
    }
    uint16_t len = (uint16_t)value.size();
    WriteWORD(len);
    WriteBytes((uint8_t*)value.c_str(), len);
    return sizeof(uint16_t) + len;
}

void CStream::WriteStream(const CStream& stream)
{
    if(stream.GetBytesWritten() > 0)
        WriteBytes(stream.m_buffer, stream.GetBytesWritten());
}

unsigned int CStream::StringSize(const std::string& value)
{
    if(value.length() >= USHRT_MAX)
    {
        assert(0);
        return 0;
    }
    return sizeof(uint16_t) + value.size();
}

void CStream::ReadAdvance(int bytes)
{
    if(m_position + bytes > m_used)
    {
        m_readoverflow = true;
        assert(0);
        return;
    }
    m_position += bytes;
}

void CStream::ReadDWORD(uint32_t* value)
{
    if(m_position + sizeof(uint32_t) > m_used)
    {
        m_readoverflow = true;
        assert(0);
        return;
    }
    *value = *((uint32_t*)(m_buffer+m_position));
    m_position += sizeof(uint32_t);
}

void CStream::ReadInt32(int32_t* value)
{
    if(m_position + sizeof(int32_t) > m_used)
    {
        m_readoverflow = true;
        assert(0);
        return;
    }
    *value = *((int32_t*)(m_buffer+m_position));
    m_position += sizeof(int32_t);
}

void CStream::ReadInt16(int16_t* value)
{
    if(m_position + sizeof(int16_t) > m_used)
    {
        m_readoverflow = true;
        assert(0);
        return;
    }
    *value = *((int16_t*)(m_buffer+m_position));
    m_position += sizeof(int16_t);
}

void CStream::ReadWORD(uint16_t* value)
{
    if(m_position + sizeof(uint16_t) > m_used)
    {
        m_readoverflow = true;
        assert(0);
        return;
    }
    *value = *((uint16_t*)(m_buffer+m_position));
    m_position += sizeof(uint16_t);
}

void CStream::ReadBYTE(uint8_t* value)
{
    if(m_position + sizeof(uint8_t) > m_used)
    {
        m_readoverflow = true;
        assert(0);
        return;
    }
    *value = *((uint8_t*)(m_buffer+m_position));
    m_position += sizeof(uint8_t);
}

void CStream::ReadChar(int8_t* value)
{
    if(m_position + sizeof(int8_t) > m_used)
    {
        m_readoverflow = true;
        assert(0);
        return;
    }
    *value = *((int8_t*)(m_buffer+m_position));
    m_position += sizeof(int8_t);
}

void CStream::ReadFloat(float* value)
{
    if(m_position + sizeof(float) > m_used)
    {
        m_readoverflow = true;
        return;
    }
    *value = *((float*)(m_buffer+m_position));
    m_position += sizeof(float);
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

void CStream::ReadQuatUnit(quaternion_t* value)
{
    ReadFloat(&value->x);
    ReadFloat(&value->y);
    ReadFloat(&value->z);
    value->ComputeW();
}

void CStream::ReadBytes(uint8_t* values, int len)
{
    if(m_position + len > m_used)
    {
        m_readoverflow = true;
        assert(0);
        return;
    }
    memcpy(values, m_buffer+m_position, len);
    m_position += len;
}

void CStream::ReadString(std::string* value)
{
    if((uint32_t)GetBytesToRead() < sizeof(uint16_t))
    {
        m_readoverflow = true;
        assert(0); // can't be right, as a string at least has 2 bytes for the length information
        return;
    }
    uint16_t len;
    ReadWORD(&len);
    if(len > (m_used-m_position))
    {
        m_readoverflow = true;
        assert(0);
        return;
    }
    (*value).assign((char*)(m_buffer+m_position), len);
    ReadAdvance(len);
}

