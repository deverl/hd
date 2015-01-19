// File: hd.cpp

// Program to perform a hexdump of a file.

// Included files.

#define _FILELENGTH

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <iomanip>

#include <algorithm>
#include <unistd.h>


// Macros.

#ifdef _DEBUG
#define DBGCODE(s)  s
#else   // _DEBUG
#define DBGCODE(list)
#endif  // _DEBUG


const unsigned int MAX_READ_BUF_SIZE(65536);


// Display flags.

enum DisplayFlags
{
    DF_SHOW_ADDRESS = 1,
    DF_SHOW_ASCII = 2,
    DF_PAGE_ALIGN = 4
};


inline char AsciiFromBin(unsigned char b)
{
    // return (char)( ( b > 0x1f && b < 0x80 ) ? (char) b : '.' );
    return (char)((b > 0x1f && b < 0x7f) ? (char) b : '.');
}


// Type definitions.

struct global_t
{
public:
    global_t()
        : num_args(0),
          num_bytes(0),
          offset(0),
          use_num_bytes(false),
          use_offset(false)
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

global_t global;





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

bool hex_dump_file(const std::string& path, unsigned int flags);

size_t format_hex(std::string& str,  unsigned int flags,
                  unsigned int offset, unsigned char const *buffer, unsigned int count);

void display_text(const char **str);






int main(int argc, char **argv)
{
    int result(0);
    int opt;
    unsigned int flags(DF_SHOW_ADDRESS | DF_SHOW_ASCII | DF_PAGE_ALIGN);

    // Process command line arguments.

    do
    {
        const char *optstring("aAhkn:s:");

        opt = getopt(argc, argv, optstring);

        switch(opt)
        {
        case 'a':
            flags &= ~DF_SHOW_ADDRESS;
            DBGCODE(std::cout << "Turning off address display" << std::endl;)
            break;

        case 'A':
            flags &= ~DF_SHOW_ASCII;
            DBGCODE(std::cout << "Turning off ASCII display" << std::endl;)
            break;

        case 'h':
            display_text(usage);
            exit(0);
            break;

        case 'k':
            flags &= ~DF_PAGE_ALIGN;
            DBGCODE(std::cout << "Turning off page alignment" << std::endl;)
            break;

        case 'n':
            global.num_bytes = static_cast<int>(strtoul(optarg, NULL, 16));
            global.use_num_bytes = true;
            DBGCODE(std::cout << "Got -n argument.  Number of bytes to dump: "
                              << global.num_bytes
                              << std::endl;)
            break;

        case 's':
            global.offset = static_cast<int>(strtoul(optarg, NULL, 16));
            global.use_offset = true;
            DBGCODE(std::cout << "Got -o argument.  Will dump from offset: "
                              << global.offset
                              << std::endl;)
            break;

        case '?':  // Unrecognized arguments.
        default:
            break;
        }

    }
    while(opt != EOF);

    DBGCODE(std::cout << "optind = "   << optind
                      << ", opterr = " << opterr
                      << ", optopt = " << optopt
                      << std::endl;)

    const int num_args(argc - optind);

    global.num_args = num_args;

    DBGCODE(std::cout << "num_args = " << num_args << std::endl;)

    if(num_args > 0)
    {
        char *arg;

        argv++;     // Step past the program name.

        while((arg = *argv++) != 0)
        {
            DBGCODE(std::cout << "Calling hex_dump_file(" << arg << ")" << std::endl;)

            if(num_args > 1)
            {
                printf("%s\n", arg);
            }

            result += hex_dump_file(arg, flags);

            if(num_args > 1)
            {
                printf("\n");
            }
        }
    }
    else
    {
        result = -1;
    }

    return result;
}





bool hex_dump_file(const std::string& path, unsigned int flags)
{
    std::ifstream file(path, std::ios::in|std::ios::binary|std::ios::ate);

    if(file.is_open())
    {
        unsigned int size(static_cast<unsigned int>(file.tellg()));
        unsigned int bytes_to_dump;

        if(global.use_num_bytes)
        {
            bytes_to_dump = global.num_bytes;
        }
        else
        {
            bytes_to_dump = size;
        }

        unsigned int buffer_size(std::min(MAX_READ_BUF_SIZE, bytes_to_dump));

        unsigned char *buffer(new unsigned char[ buffer_size ]);

        if(buffer != 0)
        {
            std::string str;
            int offset;

            offset = global.use_offset ? global.offset : 0L;

            file.seekg(offset, std::ios::beg);

            DBGCODE(std::cout << "Starting at offset: " << offset << std::endl;)

            while(bytes_to_dump > 0)
            {
                unsigned int num_read;

                file.read(reinterpret_cast<char *>(buffer), buffer_size);

                num_read = static_cast<unsigned int>(file.gcount());

                DBGCODE(std::cout << "fread() returned " << num_read << std::endl;)

                if(num_read == 0)
                {
                    bytes_to_dump = 0L;
                    break;
                }

                unsigned char *cur = buffer;

                bytes_to_dump -= num_read;

                while(num_read > 0)
                {
                    unsigned int count = (unsigned int)((num_read > 15) ? 16 : num_read);

                    format_hex(str, flags, offset, cur, count);

                    std::cout << str << std::endl;

                    num_read -= count;

                    cur += count;

                    offset += count;
                }
            }

            delete [] buffer;
        }
        else
        {
            DBGCODE(std::cout << "new " << buffer_size << " failed" << std::endl;)
        }

        file.close();
    }

    return true;    // to continue processing.
}




size_t format_hex(std::string& str, unsigned int flags,
                  unsigned int offset, unsigned char const *buffer, unsigned int count)
{
    std::stringstream ss;
    std::string ascii;
    unsigned char b;

    if(flags & DF_SHOW_ADDRESS)
    {
        ss << std::hex << std::setw(8) << std::setfill('0') << offset << "   ";
    }

    for(unsigned int i = 0; i < count; i++)
    {
        b = *(buffer + i);

        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<unsigned int>(b) << " ";

        if(i == 7)
        {
            ss << " ";
        }

        if(flags & DF_SHOW_ASCII)
        {
            ascii += AsciiFromBin(b);
        }
    }

    while(ss.str().length() < 62)
    {
        ss << " ";
    }


    // Add the ASCII portion.

    ss << ascii;

    str = ss.str();

    return str.length();
}






void display_text(const char **str)
{
    const char **p(str);

    while(p != 0 && *p != 0)
    {
        fprintf(stderr, "%s\n", *p);

        p++;
    }
}



