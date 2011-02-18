// File: hd.cpp

// Included files.

#define _FILELENGTH

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <fcntl.h>
#include <getopt.h>
#include <cerrno>

#ifdef LINUX
#include <unistd.h>
#endif


// Macros.

const int MAX_READ_BUF_SIZE = 16384;
const int STRSIZE = 128;
const int HFILE_ERROR =  -1;


// Display flags.

enum displayFlags
{
    DF_SHOW_ADDRESS = 1,
    DF_SHOW_ASCII = 2,
    DF_SEGMENTED_ADDRESS = 4,
    DF_PAGE_ALIGN = 8
};



#define AsciiFromBin(b) \
(char)( ( b > 0x1f && b < 0x80 ) ? (char) b : '.' )


#define HiWord(dw)      ((short)((((int)(dw)) >> 16) & 0xFFFF))

#define LoWord(dw)      ((short)(int)(dw))

#ifdef _DEBUG
#define DBGMSG(list)            printf list
#else   // _DEBUG
#define DBGMSG(list)
#endif  // _DEBUG





// Type definitions.

struct GLOBAL
{
public:
    GLOBAL()
      : numArgs( 0 ),
        dwNumBytes( 0 ),
        offset( 0 ),
        fUseNumBytes( false ),
        fUseOffset( false )
    {
    }
    
public:
    int numArgs;
    int dwNumBytes;
    int offset;
    bool fUseNumBytes;
    bool fUseOffset;
};



// Data.

GLOBAL global;


const char *usage[] =
{
    "",
    "usage: hd [opts] file ...",
    "   opts:",
    "     -a         Strip addresses from output",
    "     -A         Strip ASCII display from output",
    "     -h         Print this screen",
    //  "     -k         No page alignment",
    "     -n count   Dump only 'count' (hex) bytes from the file",
    "     -s offset  Start dump at 'offset' (hex) bytes into the file",
    "",
    0
};




// Function prototypes.

bool HexDumpFile( char const *path, int flags );

char *FormatHex( char *str,  int flags,
                 int offset, unsigned char const *pBuf, short count );

short ReadBufSize( int size )
{
    if ( size > MAX_READ_BUF_SIZE )
    {
        size = MAX_READ_BUF_SIZE;
    }
    
    return LoWord( size );
}

#ifdef _FILELENGTH
int file_length( int fd );
#endif

void DisplayText( const char **str );






int main( int argc, char **argv )
{
    int result = 0;
    int numArgs;
    int opt;
    int flags = DF_SHOW_ADDRESS | DF_SHOW_ASCII | DF_PAGE_ALIGN;
    const char *optstring = "aAhkn:s:";
    
    // Process command line arguments.
    
    do
    {
        opt = getopt( argc, argv, optstring );
        
        switch( opt )
        {
            case 'a':
                flags &= ~DF_SHOW_ADDRESS;
                DBGMSG(( "Turning off address display\n" ));
                break;
                
            case 'A':
                flags &= ~DF_SHOW_ASCII;
                DBGMSG(( "Turning off ASCII display\n" ));
                break;
                
            case 'h':
                DisplayText( usage );
                exit( 0 );
                break;
                
            case 'k':
                flags &= ~DF_PAGE_ALIGN;
                DBGMSG(( "Turning off page alignment\n" ));
                break;
                
            case 'n':
                global.dwNumBytes = strtoul( optarg, NULL, 16 );
                global.fUseNumBytes = true;
                DBGMSG(( "Got -n argument.  Number of bytes to dump: %lu\n", global.dwNumBytes ));
                break;
                
            case 's':
                global.offset = strtoul( optarg, NULL, 16 );
                global.fUseOffset = true;
                DBGMSG(( "Got -o argument.  Will dump from offset: %lu\n", global.offset ));
                break;
                
            case '?':  // Unrecognized arguments.
            default:
                break;
        }
        
    }
    while ( opt != EOF );
    
    DBGMSG(( "optind = %d, opterr = %d, optopt = %d\n", optind, opterr, optopt ));
    
    numArgs = argc - optind;
    global.numArgs = numArgs;
    
    DBGMSG(( "numArgs = %d\n", numArgs ));
    
    if( numArgs > 0 )
    {
        char * arg;
        
        argv++;     // Step past the program name.
        
        while( ( arg = *argv++ ) != 0 )
        {
            DBGMSG( ( "Calling HexDumpFile( %s )\n", arg ) );
            
            if ( numArgs > 1 )
            {
                printf( "%s\n", arg );
            }
            
            result += HexDumpFile( arg, flags );
            
            if ( numArgs > 1 )
            {
                printf( "\n" );
            }
        }
    }
    else
    {
        result = -1;
    }
    
    return result;
}





bool HexDumpFile( char const *path, int flags )
{
    int fd;
    unsigned char *pCur;
#ifndef _M_IX86
    int mode = O_RDONLY;
#else
    int mode = O_RDONLY | O_BINARY;
#endif
    
    DBGMSG(( "Calling open( %s, ... ) ...", path ));
    
    fd = open( path, mode );
    
    DBGMSG(( "returned %d\n", fd ));
    
    if ( fd != HFILE_ERROR )
    {
        int size = file_length( fd );
        int bytes_to_dump;
        int buffer_size;
        unsigned char *pBuf;
        
        if ( global.fUseNumBytes )
        {
            bytes_to_dump = global.dwNumBytes;
        }
        else
        {
            bytes_to_dump = size;
        }
        
        buffer_size = ReadBufSize( bytes_to_dump );
        
        DBGMSG(( "file_length(%d) returned %lu\n", fd, size ));
        
        pBuf = (unsigned char *) malloc( buffer_size );
        
        if ( pBuf )
        {
            char * pStr;
            
            DBGMSG( ( "malloc(%ld) was successful\n", buffer_size ) );
            
            pStr = ( char * ) malloc( STRSIZE );
            
            if ( pStr )
            {
                int offset;
                
                DBGMSG( ( "malloc(%d) was successful\n", STRSIZE ) );
                
                offset = global.fUseOffset ? global.offset : 0L;
                
                lseek( fd, offset, SEEK_SET );
                
                DBGMSG(( "Starting at offset: %lu\n", offset ));
                
                while( (long) bytes_to_dump > 0 )
                {
                    int num_read;
                    
                    num_read = read( fd, pBuf, buffer_size );
                    
                    DBGMSG(( "read( %d, ..., %d ) returned %d\n", fd, buffer_size, num_read ));
                    
                    if( num_read == -1 )
                    {
                        fprintf( stderr, "\terrno = %d\n", errno );
                    }
                    
                    if( num_read == 0 )
                    {
                        bytes_to_dump = 0L;
                        break;
                    }
                    
                    pCur = pBuf;
                    
                    bytes_to_dump -= num_read;
                    
                    while( num_read > 0 )
                    {
                        short wNum = (short) ( ( num_read > 15 ) ? 16 : num_read );
                        
                        FormatHex( pStr, flags, offset, pCur, wNum );
                        
                        fprintf( stdout, "%s\n", pStr );
                        
                        num_read -= 16;
                        
                        pCur += 16;
                        
                        offset += 16;
                    }
                }
                
                free( pStr );
            }
            else
            {
                DBGMSG( ( "malloc(%d) failed\n", STRSIZE ) );
            }
            
            free( pBuf );
        }
        else
        {
            DBGMSG( ( "malloc(%d) failed\n", buffer_size ) );
        }
        
        close( fd );
    }
    
    return true;    // to continue processing.
}




char *FormatHex( char *pszStr, int flags,
                int offset, unsigned char const *pBuf, short wNum )
{
    char szHex[ 60 ] = { 0 };
    char szAscii[ 20 ];
    unsigned char bCur;
    char szTmp[ 8 ];
    short i;
    
    
    for( i = 0; i < wNum; i++ )
    {
        bCur = *( pBuf + i );
        
        sprintf( szTmp, "%02x ", bCur );
        
        strcat( szHex, szTmp );
        
        if( i == 7 )
        {
            strcat( szHex, " " );
        }
        
        if( flags & DF_SHOW_ASCII )
        {
            szAscii[ i ] = AsciiFromBin( bCur );
        }
        else
        {
            szAscii[ i ] = 0;
        }
    }
    
    szAscii[ i ] = 0;
    
    if ( flags & DF_SHOW_ADDRESS )
    {
        if ( flags & DF_SEGMENTED_ADDRESS )
        {
            sprintf( pszStr, "%04x:%04x   ", HiWord( offset ), LoWord( offset ) );
        }
        else
        {
            sprintf( pszStr, "%08x   ", offset );
        }
    }
    else
    {
        // No Addresses.
        
        *pszStr = 0;
    }
    
    
    while ( strlen( szHex ) < 49 )
    {
        strcat( szHex, " " );
    }
    
    
    // Add the hex output to the result string.
    
    strcat( pszStr, szHex );
    
    
    if ( flags & DF_SHOW_ASCII )
    {
        // Leave two blank spaces.
        
        strcat( pszStr, "  " );
        
        strcat( pszStr, szAscii );
    }
    
    return pszStr;
}




#ifdef _FILELENGTH

int file_length( int fd )
{
    int current_offset = lseek( fd, 0, SEEK_CUR );
    int size = lseek( fd, 0, SEEK_END );
#ifdef _DEBUG
    
    int dwFinal =
#endif
    
    lseek( fd, current_offset, SEEK_SET );
    
    DBGMSG( ( "file_length(): Original = %ld, Size = %ld, Final = %ld\n",
             current_offset, size, dwFinal ) );
    
#ifdef _DEBUG
    
    if ( dwFinal != current_offset )
    {
        fprintf( stderr, "File pointer wasn't reset correctly\n" );
    }
#endif
    
    return size;
}

#endif





#ifdef _DEBUG

void DebugPrint( char const *fmt, ... )
{
    char buf[ 1024 ];
    int len;
    va_list ap;
    
    va_start( ap, fmt );
    
    vfprintf( stderr, fmt, ap );
    
    va_end( ap );
}

#endif  // _DEBUG





void DisplayText( const char **str )
{
    char **p = ( char ** ) str;
    
    while ( *p )
    {
        fprintf( stderr, "%s\n", *p );
        
        p++;
    }
}





// End of file: hd.cpp



