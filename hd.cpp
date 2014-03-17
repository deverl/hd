// File: hd.cpp

// Program to perform a hexdump of a file.

// Included files.

#define _FILELENGTH

#include <cstdio>
#include <cstring>
#include <cstdlib>
// #include <fcntl.h>
// #include <getopt.h>
// #include <cerrno>
#include <algorithm>
#include <unistd.h>


// Macros.

#ifdef _DEBUG
#define DBGCODE(s)  s
#else   // _DEBUG
#define DBGCODE(list)
#endif  // _DEBUG




const unsigned int MAX_READ_BUF_SIZE( 65536 );
const unsigned int STRSIZE( 128 );


// Display flags.

enum DisplayFlags
{
    DF_SHOW_ADDRESS = 1,
    DF_SHOW_ASCII = 2,
    DF_SEGMENTED_ADDRESS = 4,
    DF_PAGE_ALIGN = 8
};


inline char AsciiFromBin( unsigned char b )
{
    // return (char)( ( b > 0x1f && b < 0x80 ) ? (char) b : '.' );
    return (char)( ( b > 0x1f && b < 0x7f ) ? (char) b : '.' );
}


inline unsigned short hi_word( unsigned int i )
{
    return ( i >> 16 ) & 0x0000ffff;
}


inline unsigned short lo_word( unsigned int i )
{
    return i & 0x0000ffff;
}




// Type definitions.

struct GLOBAL
{
public:
    GLOBAL()
      : num_args( 0 ),
        num_bytes( 0 ),
        offset( 0 ),
        use_num_bytes( false ),
        use_offset( false )
    {
    }
    
public:
    int num_args;
    int num_bytes;
    int offset;
    bool use_num_bytes;
    bool use_offset;
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

bool hex_dump_file( char const *path, unsigned int flags );

char *format_hex( char *str,  unsigned int flags,
                  unsigned int offset, unsigned char const *buffer, unsigned int count );

#ifdef _FILELENGTH
unsigned int file_length( FILE *fp );
#endif

void display_text( const char **str );






int main( int argc, char **argv )
{
    int result(0);
    int opt;
    unsigned int flags( DF_SHOW_ADDRESS | DF_SHOW_ASCII | DF_PAGE_ALIGN );

    // Process command line arguments.
    
    do
    {
        const char *optstring( "aAhkn:s:" );

        opt = getopt( argc, argv, optstring );
        
        switch( opt )
        {
            case 'a':
                flags &= ~DF_SHOW_ADDRESS;
                DBGCODE( printf( "Turning off address display\n" ); )
                break;
                
            case 'A':
                flags &= ~DF_SHOW_ASCII;
                DBGCODE( printf( "Turning off ASCII display\n" ); )
                break;
                
            case 'h':
                display_text( usage );
                exit( 0 );
                break;
                
            case 'k':
                flags &= ~DF_PAGE_ALIGN;
                DBGCODE( printf( "Turning off page alignment\n" ); )
                break;
                
            case 'n':
                global.num_bytes = strtoul( optarg, NULL, 16 );
                global.use_num_bytes = true;
                DBGCODE( printf( "Got -n argument.  Number of bytes to dump: %u\n", global.num_bytes ); )
                break;
                
            case 's':
                global.offset = strtoul( optarg, NULL, 16 );
                global.use_offset = true;
                DBGCODE( printf( "Got -o argument.  Will dump from offset: %u\n", global.offset ); )
                break;
                
            case '?':  // Unrecognized arguments.
            default:
                break;
        }
        
    }
    while( opt != EOF );

    DBGCODE( printf( "optind = %d, opterr = %d, optopt = %d\n", optind, opterr, optopt ); )
    
    const int num_args( argc - optind );

    global.num_args = num_args;
    
    DBGCODE( printf( "num_args = %d\n", num_args ); )
    
    if( num_args > 0 )
    {
        char *arg;
        
        argv++;     // Step past the program name.
        
        while( ( arg = *argv++ ) != 0 )
        {
            DBGCODE( printf( "Calling hex_dump_file( %s )\n", arg ) ; )
            
            if ( num_args > 1 )
            {
                printf( "%s\n", arg );
            }
            
            result += hex_dump_file( arg, flags );
            
            if ( num_args > 1 )
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





bool hex_dump_file( char const *path, unsigned int flags )
{
    FILE *fp( fopen( path, "rb" ) );
    
    if( fp != 0 )
    {
        unsigned int size( file_length( fp ) );
        unsigned int bytes_to_dump;
        
        if( global.use_num_bytes )
        {
            bytes_to_dump = global.num_bytes;
        }
        else
        {
            bytes_to_dump = size;
        }
        
        unsigned int buffer_size( std::min( MAX_READ_BUF_SIZE, bytes_to_dump ) );
        
        unsigned char *buffer( new unsigned char[ buffer_size ] );
        
        if( buffer != 0 )
        {
            char *str( new char[ STRSIZE ] );
            
            if( str != 0 )
            {
                int offset;
                
                offset = global.use_offset ? global.offset : 0L;
                
                fseek( fp, offset, SEEK_SET );
                
                DBGCODE( printf( "Starting at offset: %u\n", offset ); )
                
                while( bytes_to_dump > 0 )
                {
                    unsigned int num_read;
                    
                    num_read = fread( buffer, sizeof(unsigned char), buffer_size, fp );
                    
                    DBGCODE( printf( "fread() returned %d\n", num_read ); )
                    
                    if( num_read == 0 )
                    {
                        bytes_to_dump = 0L;
                        break;
                    }
                    
                    unsigned char *cur = buffer;
                    
                    bytes_to_dump -= num_read;
                    
                    while( num_read > 0 )
                    {
                        unsigned int count = (unsigned int) ( ( num_read > 15 ) ? 16 : num_read );
                        
                        format_hex( str, flags, offset, cur, count );
                        
                        fprintf( stdout, "%s\n", str );
                        
                        num_read -= count;
                        
                        cur += count;
                        
                        offset += count;
                    }
                }
                
                delete [] str;
            }
            else
            {
                DBGCODE( fprintf( stderr, "new(%d) failed\n", STRSIZE ); )
            }
            
            delete [] buffer;
        }
        else
        {
            DBGCODE( printf( "new(%d) failed\n", buffer_size ); )
        }
        
        DBGCODE( int ret = )

        fclose( fp );

        DBGCODE(
            if( ret != 0 )
            {
                fprintf( stderr, "error closing file\n" );
            }
        )
    }
    
    return true;    // to continue processing.
}




char *format_hex( char *str, unsigned int flags,
                  unsigned int offset, unsigned char const *buffer, unsigned int count )
{
    char hex_str[ 60 ] = { 0 };
    char ascii_string[ 20 ];
    char temp_str[ 8 ];
    unsigned char b;
    unsigned int i;

    
    for( i = 0; i < count; i++ )
    {
        b = *( buffer + i );
        
        sprintf( temp_str, "%02x ", b );
        
        strcat( hex_str, temp_str );
        
        if( i == 7 )
        {
            strcat( hex_str, " " );
        }
        
        if( flags & DF_SHOW_ASCII )
        {
            ascii_string[ i ] = AsciiFromBin( b );
        }
        else
        {
            ascii_string[ i ] = 0;
        }
    }
    
    ascii_string[ i ] = 0;
    
    if( flags & DF_SHOW_ADDRESS )
    {
        if( flags & DF_SEGMENTED_ADDRESS )
        {
            sprintf( str, "%04x:%04x   ", hi_word( offset ), lo_word( offset ) );
        }
        else
        {
            sprintf( str, "%08x   ", offset );
        }
    }
    else
    {
        // No Addresses.
        
        *str = 0;
    }
    
    
    while ( strlen( hex_str ) < 49 )
    {
        strcat( hex_str, " " );
    }
    
    
    // Add the hex output to the result string.
    
    strcat( str, hex_str );
    
    
    if ( flags & DF_SHOW_ASCII )
    {
        // Leave two blank spaces.
        
        strcat( str, "  " );
        
        strcat( str, ascii_string );
    }
    
    return str;
}




#ifdef _FILELENGTH

unsigned int file_length( FILE *fp )
{
    unsigned int len(0);

    // Remember the current position in the file.
    unsigned int cur = ftell( fp );

    // Seek to the end of the file.
    fseek( fp, 0, SEEK_END );

    // Take the current position of the file (the end) as the size of the file.
    len = ftell( fp );

    // Return the file position to the initial location.
    fseek( fp, cur, SEEK_SET );

    return len;
}

#endif





void display_text( const char **str )
{
    const char **p( str );
    
    while( p != 0 && *p != 0 )
    {
        fprintf( stderr, "%s\n", *p );
        
        p++;
    }
}



