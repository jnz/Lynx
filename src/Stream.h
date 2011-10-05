#pragma once

#include "lynx.h"
#include "math/vec3.h"
#include "math/quaternion.h"

class CStream
{
public:
    CStream(void);
    CStream(unsigned int size); // set initial buffer size
    CStream(uint8_t* buffer, unsigned int size, unsigned int used=0);
    ~CStream(void);

    void SetBuffer(uint8_t* buffer, unsigned int size, unsigned int used=0);
    bool Resize(unsigned int newsize); // Change buffer size. Buffer gets destroyed, if newsize < size
    uint8_t* GetBuffer(); // pointer to raw bytes
    unsigned int GetBufferSize() const; // total buffer size

    // Get stream object from current position.
    // This is NOT a deep copy, the stream points to the same memory region.
    CStream GetStream();

    // size in bytes the string would occupy in the stream,
    // this is larger than the character count, as the stream
    // needs to write the string length to the buffer.
    static unsigned int StringSize(const std::string& value);

    // WRITE FUNCTIONS
    void ResetWritePosition(); // Reset write pointer to start, reset overflow flag
    bool GetWriteOverflow() const; // returns true, if some write operation failed, because the buffer is full
    unsigned int GetSpaceLeft() const; // Bytes left to write
    unsigned int GetBytesWritten() const; // written bytes
    void WriteAdvance(unsigned int bytes);
    void WriteDWORD(uint32_t value);
    void WriteInt32(int32_t value);
    void WriteInt16(int16_t value);
    void WriteWORD(uint16_t value);
    void WriteBYTE(uint8_t value);
    void WriteChar(int8_t value); // note: char as in plain c, no unicode
    void WriteFloat(float value);
    void WriteVec3(const vec3_t& value);
    void WriteQuat(const quaternion_t& value);
    void WriteBytes(const uint8_t* values, unsigned int len);
    uint16_t WriteString(const std::string& value); // Max Str len: 0xffff. returns written bytes
    void WriteStream(const CStream& stream);

    // READ FUNCTIONS
    void ResetReadPosition(); // Read from the beginning
    bool GetReadOverflow() const; // returns true, if some read operation failed, because there is no data left in the buffer
    unsigned int GetBytesToRead() const; // Bytes left in stream to read
    unsigned int GetBytesRead() const; // Bytes read
    void ReadAdvance(int bytes); // like "fseek(f, bytes, SEEK_CUR)"
    void ReadDWORD(uint32_t* value);
    void ReadInt32(int32_t* value);
    void ReadInt16(int16_t* value);
    void ReadWORD(uint16_t* value);
    void ReadBYTE(uint8_t* value);
    void ReadChar(int8_t* value); // note: char as in plain c, no unicode
    void ReadFloat(float* value);
    void ReadVec3(vec3_t* value); // 3*4 Bytes
    void ReadQuat(quaternion_t* value);
    void ReadBytes(uint8_t* values, int len);
    void ReadString(std::string* value);

protected:
    uint8_t* m_buffer;
    bool m_foreign; // if the buffer is foreign, we won't free the memory
    unsigned int m_size;
    unsigned int m_position;
    unsigned int m_used;
    bool m_readoverflow; // if you try to read beyond the end of the buffer, this is set to true, until you reset the buffer
    bool m_writeoverflow; // the same for writing
};

