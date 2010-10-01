#pragma once

#include "lynx.h"
#include "math/vec3.h"
#include "math/quaternion.h"

class CStream
{
public:
    CStream(void);
    CStream(int size);
    CStream(uint8_t* buffer, int size, int used=0);
    ~CStream(void);

    void SetBuffer(uint8_t* buffer, int size, int used=0);
    bool Resize(int newsize); // Change buffer size. Buffer wird zerstört, wenn newsize < size
    uint8_t* GetBuffer(); // pointer to raw bytes
    int GetBufferSize(); // total buffer size

    CStream GetStream(); // Stream Objekt von aktueller Position holen

    // WRITE FUNCTIONS
    void ResetWritePosition(); // Reset write pointer to start
    int GetSpaceLeft(); // Bytes left to write
    int GetBytesWritten() const; // written bytes
    void WriteAdvance(int bytes);
    void WriteDWORD(uint32_t value);
    void WriteInt32(int32_t value);
    void WriteInt16(int16_t value);
    void WriteWORD(uint16_t value);
    void WriteBYTE(uint8_t value);
    void WriteFloat(float value);
    void WriteAngle3(const vec3_t& value); // 3 Bytes
    void WritePos6(const vec3_t& value); // 6 Bytes
    void WriteFloat2(float value); // 2 Byte
    void WriteVec3(const vec3_t& value);
    void WriteQuat(const quaternion_t& value);
    void WriteBytes(const uint8_t* values, int len);
    uint16_t WriteString(const std::string& value); // Max Str len: 0xffff. return written bytes
    void WriteStream(const CStream& stream);
    
    static size_t StringSize(const std::string& value); // size in bytes the string would occupy in the stream

    // READ FUNCTIONS
    void ResetReadPosition(); // Read from the beginning
    int GetBytesToRead(); // Bytes left in stream to read
    int GetBytesRead(); // Bytes read
    void ReadAdvance(int bytes); // like "fseek(f, bytes, SEEK_CUR)"
    void ReadDWORD(uint32_t* value);
    void ReadInt32(int32_t* value);
    void ReadInt16(int16_t* value);
    void ReadWORD(uint16_t* value);
    void ReadBYTE(uint8_t* value);
    void ReadFloat(float* value);
    void ReadAngle3(vec3_t* value); // 3 Bytes
    void ReadPos6(vec3_t* value); // 6 Bytes
    void ReadFloat2(float* value); // 2 Byte Float
    void ReadVec3(vec3_t* value); // 3*4 Bytes
    void ReadQuat(quaternion_t* value);
    void ReadBytes(uint8_t* values, int len);
    void ReadString(std::string* value);

protected:  
    uint8_t* m_buffer;
    bool m_foreign;
    int m_size;
    int m_position;
    int m_used;
};

#define STREAM_SIZE_ANGLE3      3
#define STREAM_SIZE_POS6        6
#define STREAM_SIZE_VEC3        (sizeof(float)*3)
#define STREAM_SIZE_FLOAT2      2
#define STREAM_SIZE_QUAT        (sizeof(float)*4)
