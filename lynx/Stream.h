#pragma once

#include "lynx.h"
#include "math/vec3.h"
#include "math/quaternion.h"

class CStream
{
public:
	CStream(void);
	CStream(int size);
	CStream(BYTE* buffer, int size, int used=0);
	~CStream(void);

	void SetBuffer(BYTE* buffer, int size, int used=0);
	bool Resize(int newsize); // Change buffer size. Buffer wird zerstört, wenn newsize < size
	BYTE* GetBuffer(); // pointer to raw bytes
	int GetBufferSize(); // total buffer size

    CStream GetStream(); // Stream Objekt von aktueller Position holen

	// WRITE FUNCTIONS
	void ResetWritePosition(); // Reset write pointer to start
	int GetSpaceLeft(); // Bytes left to write
	int GetBytesWritten() const; // written bytes
    void WriteAdvance(int bytes);
	void WriteDWORD(DWORD value);
	void WriteInt32(INT32 value);
	void WriteInt16(INT16 value);
	void WriteWORD(WORD value);
	void WriteBYTE(BYTE value);
	void WriteFloat(float value);
	void WriteAngle3(const vec3_t& value); // 3 Bytes
	void WritePos6(const vec3_t& value); // 6 Bytes
	void WriteFloat2(float value); // 2 Byte
	void WriteVec3(const vec3_t& value);
    void WriteQuat(const quaternion_t& value);
	void WriteBytes(const BYTE* values, int len);
	WORD WriteString(const std::string& value); // Max Str len: 0xffff. return written bytes
    void WriteStream(const CStream& stream);
	
	static size_t StringSize(const std::string& value); // size in bytes the string would occupy in the stream

	// READ FUNCTIONS
	void ResetReadPosition(); // Read from the beginning
	int GetBytesToRead(); // Bytes left in stream to read
	int GetBytesRead(); // Bytes read
	void ReadAdvance(int bytes); // like "fseek(f, bytes, SEEK_CUR)"
	void ReadDWORD(DWORD* value);
	void ReadInt32(INT32* value);
	void ReadInt16(INT16* value);
	void ReadWORD(WORD* value);
	void ReadBYTE(BYTE* value);
	void ReadFloat(float* value);
	void ReadAngle3(vec3_t* value); // 3 Bytes
	void ReadPos6(vec3_t* value); // 6 Bytes
	void ReadFloat2(float* value); // 2 Byte Float
	void ReadVec3(vec3_t* value); // 3*4 Bytes
    void ReadQuat(quaternion_t* value);
	void ReadBytes(BYTE* values, int len);
	void ReadString(std::string* value);

protected:	
	BYTE* m_buffer;
	bool m_foreign;
	int m_size;
	int m_position;
	int m_used;
};

#define STREAM_SIZE_ANGLE3		3
#define STREAM_SIZE_POS6		6
#define STREAM_SIZE_VEC3		(sizeof(float)*3)
#define STREAM_SIZE_FLOAT2		2
#define STREAM_SIZE_QUAT        (sizeof(float)*4)