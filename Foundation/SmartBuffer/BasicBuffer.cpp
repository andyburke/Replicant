#include "BasicBuffer.h"

#include "Platform/Exception.h"
#include "Foundation/Log.h"

#include "Foundation/Math/Vector3.h"
#include "Foundation/Math/Vector4.h"
#include "Foundation/Math/Half.h"

using namespace Helium;

//
// To turn on debug tracking/logging capability, set this to "1".
//  It may run slightly slower, and will use a ton of memory.
//

#define BASIC_BUFFER_DEBUG_INFO 0

#if BASIC_BUFFER_DEBUG_INFO

#define ADD_DEBUG_INFO(blockType, blockSize)          \
{                                                     \
    va_list args;                                       \
    va_start(args, dbgStr);                             \
    AddDebugInfo(blockType, blockSize, dbgStr, args, 0);\
}

#define ADD_DEBUG_INFO_SKIP(blockType, blockSize)     \
{                                                     \
    va_list args;                                       \
    va_start(args, dbgStr);                             \
    AddDebugInfo(blockType, blockSize, dbgStr, args, 1);\
}

static tchar g_DebugString[2048];

#else

#define ADD_DEBUG_INFO(blockType, blockSize)
#define ADD_DEBUG_INFO_SKIP(blockType, blockSize)

#endif

bool BasicBuffer::IsDebugInfoEnabled()
{
#if BASIC_BUFFER_DEBUG_INFO
    return true;
#else
    return false;
#endif
}

void BasicBuffer::AddDebugInfo(BasicBufferDebugInfo::BlockType blockType, unsigned int blockSize, const tchar *dbgStr, va_list argptr, tchar ignore_next)
{
#if BASIC_BUFFER_DEBUG_INFO
    BasicBufferDebugInfo debug_info;
    tchar* dbg_str;

    static tchar s_ignore_next = 0;

    if ( s_ignore_next )
    {
        s_ignore_next += ignore_next - 1;  // - 1 because we are ignoring current...

        return;
    }
    else
    {
        s_ignore_next += ignore_next;
    }

    if ( dbgStr )
    {
        static tchar prtbuf[2048];
        vsprintf(prtbuf, dbgStr, argptr);
        dbg_str = prtbuf;
    }
    else
    {
        dbg_str = "(NONE)";
    }

    debug_info.m_BlockType = blockType;
    debug_info.m_DebugString = dbg_str;
    debug_info.m_BlockSize = blockSize;
    debug_info.m_FileSize = GetSize();

    m_DebugInfo.push_back(debug_info);
#endif
}

void BasicBuffer::DumpDebugInfo(FILE* file)
{
    if ( file )
    {
        fprintf(file, "ByteOrder   : %s\n", m_ByteOrder==ByteOrders::LittleEndian?"PC":"PS3");

        for (BasicBufferDebugInfoVector::iterator i=m_DebugInfo.begin(); i!=m_DebugInfo.end(); i++)
        {
            tchar* blockTypeStr = NULL;

            switch(i->m_BlockType)
            {
            case BasicBufferDebugInfo::BLOCK_TYPE_BUFFER:  blockTypeStr = TXT( "Buffer" ); break;
            case BasicBufferDebugInfo::BLOCK_TYPE_I8:      blockTypeStr = TXT( "i8" ); break;
            case BasicBufferDebugInfo::BLOCK_TYPE_U8:      blockTypeStr = TXT( "u8" ); break;
            case BasicBufferDebugInfo::BLOCK_TYPE_I16:     blockTypeStr = TXT( "i16" ); break;
            case BasicBufferDebugInfo::BLOCK_TYPE_U16:     blockTypeStr = TXT( "u16" ); break;
            case BasicBufferDebugInfo::BLOCK_TYPE_I32:     blockTypeStr = TXT( "i32" ); break;
            case BasicBufferDebugInfo::BLOCK_TYPE_U32:     blockTypeStr = TXT( "u32" ); break;
            case BasicBufferDebugInfo::BLOCK_TYPE_I64:     blockTypeStr = TXT( "i64" ); break;
            case BasicBufferDebugInfo::BLOCK_TYPE_U64:     blockTypeStr = TXT( "u64" ); break;
            case BasicBufferDebugInfo::BLOCK_TYPE_F32:     blockTypeStr = TXT( "f32" ); break;
            case BasicBufferDebugInfo::BLOCK_TYPE_F64:     blockTypeStr = TXT( "f64" ); break;
            case BasicBufferDebugInfo::BLOCK_TYPE_RESERVE: blockTypeStr = TXT( "Reserve" ); break;
            case BasicBufferDebugInfo::BLOCK_TYPE_POINTER: blockTypeStr = TXT( "Pointer" ); break;
            case BasicBufferDebugInfo::BLOCK_TYPE_OFFSET:  blockTypeStr = TXT( "Offset" ); break;
            default:                                       blockTypeStr = TXT( "Unknown" ); break;
            }

            fprintf(file, "Block Label   : %s\n", i->m_DebugString.c_str());
            fprintf(file, "Block Type    : %s, Size: %d\n", blockTypeStr, i->m_BlockSize);
            fprintf(file, "Block BufferLocation: 0x%08X\n", i->m_FileSize);
            fprintf(file, "Contents      : ");

            u8* data = m_Data + i->m_FileSize;

            switch(i->m_BlockType)
            {
            case BasicBufferDebugInfo::BLOCK_TYPE_I8:      fprintf(file, "%02X\n",*((i8*)data)); break;
            case BasicBufferDebugInfo::BLOCK_TYPE_U8:      fprintf(file, "%02X\n",*((u8*)data)); break;
            case BasicBufferDebugInfo::BLOCK_TYPE_I16:     fprintf(file, "%04X\n",ConvertEndian( *((i16*)data),IsPlatformBigEndian())); break;
            case BasicBufferDebugInfo::BLOCK_TYPE_U16:     fprintf(file, "%04X\n",ConvertEndian( *((u16*)data),IsPlatformBigEndian())); break;
            case BasicBufferDebugInfo::BLOCK_TYPE_I32:     fprintf(file, "%08X\n",ConvertEndian( *((i32*)data),IsPlatformBigEndian())); break;
            case BasicBufferDebugInfo::BLOCK_TYPE_U32:     fprintf(file, "%08X\n",ConvertEndian( *((u32*)data),IsPlatformBigEndian())); break;
            case BasicBufferDebugInfo::BLOCK_TYPE_I64:     fprintf(file, "0x%I64x\n",ConvertEndian( *((u64*)data),IsPlatformBigEndian())); break;
            case BasicBufferDebugInfo::BLOCK_TYPE_U64:     fprintf(file, "0x%I64x\n",ConvertEndian( *((u64*)data),IsPlatformBigEndian())); break;
            case BasicBufferDebugInfo::BLOCK_TYPE_F32:     fprintf(file, "%.3f\n",ConvertEndian( *((f32*)data),IsPlatformBigEndian())); break;
            case BasicBufferDebugInfo::BLOCK_TYPE_F64:     fprintf(file, "%.3f\n",ConvertEndian( *((f64*)data),IsPlatformBigEndian())); break;
            case BasicBufferDebugInfo::BLOCK_TYPE_OFFSET:  fprintf(file, "0x%08x\n", *((u32*)data)); break;
            case BasicBufferDebugInfo::BLOCK_TYPE_POINTER: if (GetPlatformPtrSize() == 8) fprintf(file, "0x%I64x\n", *((u64*)data)); else fprintf(file, "0x%08x\n", *((u32*)data)); break;
            case BasicBufferDebugInfo::BLOCK_TYPE_RESERVE:
                {
                    bool handled = false;

                    switch (i->m_BlockSize) 
                    {
                    case 1: handled = true; fprintf(file, "%d\n", *((u8*)data)); break;
                    case 2: handled = true; fprintf(file, "%d\n", *((u16*)data)); break;
                    case 4: handled = true; fprintf(file, "%d\n", *((u32*)data)); break;
                    }

                    if ( handled )
                    {
                        break;
                    }
                }

            case BasicBufferDebugInfo::BLOCK_TYPE_BUFFER:
                {
                    unsigned int num_blocks = i->m_BlockSize;
                    unsigned int j;

                    // limit the length of this output..
                    if (num_blocks > 1024)
                        num_blocks = 1024;

                    // just output hex...
                    for (j=0; j<num_blocks; j++)
                    {
                        fprintf(file, "0x%02X ", data[j]);
                    }

                    fprintf(file, "\n");
                    fprintf(file, "              : ");

                    // output string...
                    for (j=0; j<num_blocks; j++)
                    {
                        if ( data[j] >= 32 && data[j] <= 126 )
                        {
                            fprintf(file, "%c", data[j]);
                        }
                        else
                        {
                            fprintf(file, "." );
                        }
                    }

                    fprintf(file, "\n");
                }
                break;

            default:
                {
                    fprintf(file, "PARADOX ERR\n");
                    break;
                }
            }

            fprintf(file, "\n");
        }
    }
}

u32 BasicBuffer::AddBuffer( const u8* buffer, u32 size, const tchar* dbgStr, ... )
{
    ADD_DEBUG_INFO(BasicBufferDebugInfo::BLOCK_TYPE_BUFFER, size);

    if ( (size + m_Size) > m_Capacity )
    {
        GrowBy( size );
    }

    memcpy( m_Data + m_Size, buffer, size );
    m_Size += size;

    return ( m_Size - size );
}

u32 BasicBuffer::AddBuffer( const SmartBufferPtr& buffer, bool add_fixups )
{
    // platform types have to match
    HELIUM_ASSERT(buffer->GetByteOrder() == m_ByteOrder);

    // first bring in all the data from the incoming buffer
    u32 return_val = AddBuffer( buffer->GetData(), buffer->GetSize() );

    if (add_fixups)
    {
        // inherit all the incoming / outgoing fixups from this buffer with out new offset
        InheritFixups( buffer, return_val );
    }

    return return_val;
}

u32 BasicBuffer::AddFile( const tstring& filename )
{
    return AddFile( filename.c_str() );
}

u32 BasicBuffer::AddFile( const tchar* filename )
{
    FILE *pfile = _tfopen( filename, TXT( "rb" ) );
    if ( pfile == NULL )
    {
        throw Helium::Exception( TXT( "Could not open file '%s' to add to membuf '%s'." ), filename, m_Name.c_str() );
    }

    fseek(pfile,0,SEEK_END);
    long filesize = ftell(pfile);
    fseek(pfile,0,SEEK_SET);

    if(filesize == -1)
    {
        throw Helium::Exception( TXT( "Could not get file size for file '%s' to add to membuf '%s'." ), filename, m_Name.c_str() );
    }

    if ( (filesize + m_Size) > m_Capacity )
        GrowBy(filesize);

    size_t file_read = fread( m_Data + m_Size, 1, filesize, pfile );
    if(file_read != (size_t)filesize)
    {
        Log::Warning( TXT( "Could not read entire file '%s' to add to membuf '%s'.\n" ), filename, m_Name.c_str() );
    }

    fclose(pfile);

    m_Size += (u32)file_read;
    return m_Size - (u32)file_read;
}

u32 BasicBuffer::AddI8( i8 val, const tchar* dbgStr, ... )
{
    ADD_DEBUG_INFO_SKIP(BasicBufferDebugInfo::BLOCK_TYPE_I8, 1);

    return AddBuffer( (u8*)&val, sizeof( i8 ) );
}

u32 BasicBuffer::AddU8( u8 val, const tchar* dbgStr, ... )
{
    ADD_DEBUG_INFO_SKIP(BasicBufferDebugInfo::BLOCK_TYPE_U8, 1);

    return AddBuffer( (u8*)&val, sizeof( u8 ) );
}

u32 BasicBuffer::AddU16( u16 val, const tchar* dbgStr, ... )
{
    ADD_DEBUG_INFO_SKIP(BasicBufferDebugInfo::BLOCK_TYPE_U16, 2);

    val = ConvertEndian(val,IsPlatformBigEndian());
    return AddBuffer( (u8*)&val, sizeof( u16 ) );
}

u32 BasicBuffer::AddI16( i16 val, const tchar* dbgStr, ... )
{
    ADD_DEBUG_INFO_SKIP(BasicBufferDebugInfo::BLOCK_TYPE_I16, 2);

    val = ConvertEndian(val,IsPlatformBigEndian());
    return AddBuffer( (u8*)&val, sizeof( i16 ) );
}

u32 BasicBuffer::AddI32( i32 val, const tchar* dbgStr, ... )
{
    ADD_DEBUG_INFO_SKIP(BasicBufferDebugInfo::BLOCK_TYPE_I32, 4);

    val = ConvertEndian(val,IsPlatformBigEndian());
    return AddBuffer( (u8*)&val, sizeof( i32 ) );
}

u32 BasicBuffer::AddU32( u32 val, const tchar* dbgStr, ... )
{
    ADD_DEBUG_INFO_SKIP(BasicBufferDebugInfo::BLOCK_TYPE_U32, 4);

    val = ConvertEndian(val,IsPlatformBigEndian());
    return AddBuffer( (u8*)&val, sizeof( u32 ) );
}

u32 BasicBuffer::AddI64( i64 val, const tchar* dbgStr, ... )
{
    ADD_DEBUG_INFO_SKIP(BasicBufferDebugInfo::BLOCK_TYPE_I64, 8);

    val = ConvertEndian(val,IsPlatformBigEndian());
    return AddBuffer( (u8*)&val, sizeof( i64 ) );
}

u32 BasicBuffer::AddU64( u64 val, const tchar* dbgStr, ... )
{
    ADD_DEBUG_INFO_SKIP(BasicBufferDebugInfo::BLOCK_TYPE_U64, 8);

    val = ConvertEndian(val,IsPlatformBigEndian());
    return AddBuffer( (u8*)&val, sizeof( u64 ) );
}

u32 BasicBuffer::AddF16( f32 val, const tchar* dbgStr, ... )
{
    i16 half = Math::FloatToHalf( val );
    ADD_DEBUG_INFO_SKIP(BasicBufferDebugInfo::BLOCK_TYPE_I16, 2);

    half = ConvertEndian(half,IsPlatformBigEndian());
    return AddBuffer( (u8*)&half, sizeof( i16 ) );
}

u32 BasicBuffer::AddF32( f32 val, const tchar* dbgStr, ... )
{
    u32 i = *(reinterpret_cast<u32 *>(&val));

    // handle case for "negative" zero..  this was
    // causing diffs to vary between reflect/non-xml
    //
    // IEEE format for a f32 is this:
    // Sign   Exponent  Fraction
    //    0   00000000  00000000000000000000000
    //Bit 31 [30 -- 23] [22 -- 0]
    if ( (i & 0x7FFFFFFF) == 0x0 )
    {
        i = 0x0;
    }

    ADD_DEBUG_INFO_SKIP(BasicBufferDebugInfo::BLOCK_TYPE_F32, 4);

    return AddU32( i );
}

u32 BasicBuffer::AddF64( f64 val, const tchar* dbgStr, ... )
{
    u64 i = *(reinterpret_cast<u64 *>(&val));

    // handle case for "negative" zero..  this was
    // causing diffs to vary between reflect/non-xml
    //
    // IEEE format for a f32 is this:
    // Sign   Exponent  Fraction
    //Bit 64 [63 -- 56] [55 -- 0]
    if ( (i & 0x7FFFFFFFFFFFFFFF) == 0x0 )
    {
        i = 0x0;
    }

    ADD_DEBUG_INFO_SKIP(BasicBufferDebugInfo::BLOCK_TYPE_F64, 8);

    return AddU64( i );
}

u32 BasicBuffer::AddVector3( const Math::Vector3& v, const tchar* debugStr )
{
    u32 ret = AddF32(v.x, debugStr);
    AddF32(v.y, debugStr);
    AddF32(v.z, debugStr);
    return ret;
}

u32 BasicBuffer::AddVector4( const Math::Vector4& v, const tchar* debugStr )
{
    u32 ret = AddF32(v.x, debugStr);
    AddF32(v.y, debugStr);
    AddF32(v.z, debugStr);
    AddF32(v.w, debugStr);
    return ret;
}

u32 BasicBuffer::AddVector4( const Math::Vector3& v, f32 w, const tchar* debugStr )
{
    u32 ret = AddF32(v.x, debugStr);
    AddF32(v.y, debugStr);
    AddF32(v.z, debugStr);
    AddF32(w, debugStr);
    return ret;
}

u32 BasicBuffer::AddVector4( f32 x, f32 y, f32 z, f32 w, const tchar* debugStr )
{
    u32 ret = AddF32(x, debugStr);
    AddF32(y, debugStr);
    AddF32(z, debugStr);
    AddF32(w, debugStr);
    return ret;
}

void BasicBuffer::AddAtLocI8( i8 val, const BufferLocation& destination )
{
    Write( destination, (u8*)&val, sizeof( i8 ) );
}

void BasicBuffer::AddAtLocU8( u8 val, const BufferLocation& destination )
{
    Write( destination, (u8*)&val, sizeof( u8 ) );
}

void BasicBuffer::AddAtLocI16( i16 val, const BufferLocation& destination )
{
    val = ConvertEndian(val,IsPlatformBigEndian());
    Write( destination, (u8*)&val, sizeof( i16 ) );
}

void BasicBuffer::AddAtLocU16( u16 val, const BufferLocation& destination )
{
    val = ConvertEndian(val,IsPlatformBigEndian());
    Write( destination, (u8*)&val, sizeof( u16 ) );
}

void BasicBuffer::AddAtLocI32( i32 val, const BufferLocation& destination )
{
    val = ConvertEndian(val,IsPlatformBigEndian());
    Write( destination, (u8*)&val, sizeof( i32 ) );
}

void BasicBuffer::AddAtLocU32( u32 val, const BufferLocation& destination )
{
    val = ConvertEndian(val,IsPlatformBigEndian());
    Write( destination, (u8*)&val, sizeof( u32 ) );
}

void BasicBuffer::AddAtLocI64( i64 val, const BufferLocation& destination )
{
    val = ConvertEndian(val,IsPlatformBigEndian());
    Write( destination, (u8*)&val, sizeof( i64 ) );
}

void BasicBuffer::AddAtLocU64( u64 val, const BufferLocation& destination )
{
    val = ConvertEndian(val,IsPlatformBigEndian());
    Write( destination, (u8*)&val, sizeof( u64 ) );
}

void BasicBuffer::AddAtLocF32( f32 val, const BufferLocation& destination )
{
    u32 i = *(reinterpret_cast<u32 *>(&val));

    // handle case for "negative" zero..  this was
    // causing diffs to vary between reflect/non-xml
    //
    // IEEE format for a f32 is this:
    // Sign   Exponent  Fraction
    //    0   00000000  00000000000000000000000
    //Bit 31 [30 -- 23] [22 -- 0]
    if ( (i & 0x7FFFFFFF) == 0x0 )
    {
        i = 0x0;
    }

    AddAtLocU32( i, destination );
}

void BasicBuffer::AddAtLocF64( f64 val, const BufferLocation& destination )
{
    u64 i = *(reinterpret_cast<u64 *>(&val));

    // handle case for "negative" zero..  this was
    // causing diffs to vary between reflect/non-xml
    //
    // IEEE format for a f32 is this:
    // Sign   Exponent  Fraction
    //Bit 64 [63 -- 56] [55 -- 0]
    if ( (i & 0x7FFFFFFFFFFFFFFF) == 0x0 )
    {
        i = 0x0;
    }

    AddAtLocU64( i, destination );
}

void BasicBuffer::AddPad( u32 pad_length )
{
    if ( (pad_length + m_Size) > m_Capacity )
    {
        GrowBy( pad_length );
    }

    for ( u32 i = 0; i < pad_length; ++i )
    {
        AddU8( 0 );
    }
}

void BasicBuffer::PadToArb( u32 align_size )
{
    i32 pad_length = (i32)align_size - ( m_Size % align_size );
    if ( pad_length != align_size )
    {
        HELIUM_ASSERT( pad_length > 0 );
        AddPad( (u32)pad_length );
    }
}

void BasicBuffer::SetCapacity(u32 capacity)
{
    if (capacity > m_Capacity)
    {
        GrowBy(capacity - m_Capacity);
    }  
}

BufferLocation BasicBuffer::Reserve(u32 size, const tchar* dbgStr, ...)
{
    ADD_DEBUG_INFO(BasicBufferDebugInfo::BLOCK_TYPE_RESERVE, size);

    BufferLocation return_val = GetCurrentLocation();

    if ( (size + m_Size) > m_Capacity )
    {
        GrowBy( size );
    }

    memset( m_Data + m_Size, 0, size );

    m_Size += size;

    return return_val;
}

void BasicBuffer::Reserve(BufferLocation& loc, u32 size, const tchar* dbgStr, ...)
{
#if BASIC_BUFFER_DEBUG_INFO
    {
        if (dbgStr)
        {
            va_list args;
            va_start(args, dbgStr);
            assert(_vscprintf(dbgStr, args)+1 <= sizeof(g_DebugString));
            vsprintf(g_DebugString, dbgStr, args);
            dbgStr = g_DebugString;
        }
    }
#endif //BASIC_BUFFER_DEBUG_INFO

    loc = Reserve(size, dbgStr);
}

BufferLocation BasicBuffer::ReservePointer(u32 size, const tchar* dbgStr, ... )
{
    if (size==0)
    {
        size = GetPlatformPtrSize();
    }

    // 8 byte pointers are aligned to 8 byte addressess
    if ( size == 8 )
    {
        PadToArb(8);
    }

    ADD_DEBUG_INFO_SKIP(BasicBufferDebugInfo::BLOCK_TYPE_POINTER, size);

    return Reserve( size );
}

void BasicBuffer::ReservePointer(BufferLocation& loc, u32 size, const tchar* dbgStr, ... )
{
#if BASIC_BUFFER_DEBUG_INFO
    {
        if (dbgStr)
        {
            va_list args;
            va_start(args, dbgStr);
            assert(_vscprintf(dbgStr, args)+1 <= sizeof(g_DebugString));
            vsprintf(g_DebugString, dbgStr, args);
            dbgStr = g_DebugString;
        }
    }
#endif //BASIC_BUFFER_DEBUG_INFO

    loc = ReservePointer(size, dbgStr);
}

BufferLocation BasicBuffer::ReserveOffset( const tchar* dbgStr, ... )
{
    ADD_DEBUG_INFO_SKIP(BasicBufferDebugInfo::BLOCK_TYPE_OFFSET, 4);

    return Reserve( 4 );
}

void BasicBuffer::ReserveOffset(BufferLocation& loc, const tchar* dbgStr, ... )
{
#if BASIC_BUFFER_DEBUG_INFO
    {
        if (dbgStr)
        {
            va_list args;
            va_start(args, dbgStr);
            assert(_vscprintf(dbgStr, args)+1 <= sizeof(g_DebugString));
            vsprintf(g_DebugString, dbgStr, args);
            dbgStr = g_DebugString;
        }
    }
#endif //BASIC_BUFFER_DEBUG_INFO

    loc = ReserveOffset(dbgStr);
}

void BasicBuffer::WritePointer( const BufferLocation& destination )
{
    AddPointerFixup( ReservePointer(0), destination,GetPlatformPtrSize() );
}

void BasicBuffer::WritePointer32( const BufferLocation& destination )
{
    AddPointerFixup( ReservePointer(4), destination,4 );
}

void BasicBuffer::WritePointer64( const BufferLocation& destination )
{
    AddPointerFixup( ReservePointer(8), destination,8 );
}

void BasicBuffer::WriteOffset( const BufferLocation& destination, bool absolute )
{
    AddOffsetFixup( ReserveOffset(), destination, absolute );
}