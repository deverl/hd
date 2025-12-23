#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <unistd.h> // getopt

// ------------------------------------------------------------
// Constants & Flags
// ------------------------------------------------------------

constexpr std::size_t MAX_READ_BUF_SIZE = 65536;

enum DisplayFlags : unsigned int {
    DF_SHOW_ADDRESS = 1u << 0,
    DF_SHOW_ASCII   = 1u << 1
};

// ------------------------------------------------------------
// Options
// ------------------------------------------------------------

struct Options {
    std::uint64_t num_bytes     = 0;
    std::uint64_t offset        = 0;
    bool          use_num_bytes = false;
    bool          use_offset    = false;
};

// ------------------------------------------------------------
// Utility
// ------------------------------------------------------------

char ascii_from_bin(std::uint8_t b) {
    return (b >= 0x20 && b < 0x7F) ? static_cast<char>(b) : '.';
}

void print_usage() {
    static const std::vector<std::string> usage = {
        "",
        "usage: hd [opts] file ...",
        "",
        " opts:",
        "     -a         Strip addresses from output",
        "     -A         Strip ASCII display from output",
        "     -h         Print this screen",
        "     -n count   Dump only 'count' (hex) bytes from the file",
        "     -s offset  Start dump at 'offset' (hex) bytes into the file",
        ""
    };

    for (const auto& line : usage) {
        std::cerr << line << '\n';
    }
}

// ------------------------------------------------------------
// Formatting
// ------------------------------------------------------------

std::string format_hex_line(
    unsigned int        flags,
    std::uint64_t       offset,
    const std::uint8_t* buffer,
    std::size_t         count) {
    std::ostringstream out;
    std::string        ascii;

    if (flags & DF_SHOW_ADDRESS) {
        out << std::hex << std::setw(8) << std::setfill('0')
            << offset << "   ";
    }

    for (std::size_t i = 0; i < count; ++i) {
        out << std::hex << std::setw(2) << std::setfill('0')
            << static_cast<unsigned int>(buffer[i]) << ' ';

        if (i == 7) {
            out << ' ';
        }

        if (flags & DF_SHOW_ASCII) {
            ascii += ascii_from_bin(buffer[i]);
        }
    }

    if (flags & DF_SHOW_ASCII) {
        constexpr std::size_t HEX_FIELD_WIDTH = 62;
        const auto            current_len     = static_cast<std::size_t>(out.tellp());

        if (current_len < HEX_FIELD_WIDTH) {
            out << std::string(HEX_FIELD_WIDTH - current_len, ' ');
        }

        out << ascii;
    }

    return out.str();
}

// ------------------------------------------------------------
// File Dump
// ------------------------------------------------------------

bool hex_dump_file(
    const std::string& path,
    unsigned int       flags,
    const Options&     opts) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);

    if (!file) {
        std::cerr << "hd: cannot open file '" << path << "'\n";
        return false;
    }

    const std::uint64_t file_size       = static_cast<std::uint64_t>(file.tellg());

    std::uint64_t       bytes_remaining = opts.use_num_bytes ? opts.num_bytes : file_size;

    std::uint64_t       offset          = opts.use_offset ? opts.offset : 0;

    if (offset > file_size) {
        return true; // nothing to dump
    }

    file.seekg(static_cast<std::streamoff>(offset), std::ios::beg);

    std::vector<std::uint8_t> buffer(
        std::min<std::uint64_t>(MAX_READ_BUF_SIZE, bytes_remaining));

    while (bytes_remaining > 0 && file) {
        const std::size_t to_read = static_cast<std::size_t>(
            std::min<std::uint64_t>(buffer.size(), bytes_remaining));

        file.read(reinterpret_cast<char*>(buffer.data()), to_read);
        const std::size_t bytes_read = static_cast<std::size_t>(file.gcount());

        if (bytes_read == 0) {
            break;
        }

        bytes_remaining -= bytes_read;

        std::size_t pos = 0;
        while (pos < bytes_read) {
            const std::size_t line_count = std::min<std::size_t>(16, bytes_read - pos);

            std::cout << format_hex_line(
                flags,
                offset,
                buffer.data() + pos,
                line_count)
                      << '\n';

            pos += line_count;
            offset += line_count;
        }
    }

    return true;
}

// ------------------------------------------------------------
// main
// ------------------------------------------------------------

int main(int argc, char* argv[]) {
    unsigned int flags = DF_SHOW_ADDRESS | DF_SHOW_ASCII;
    Options      opts;

    int          opt;
    const char*  optstring = "aAhn:s:";

    while ((opt = getopt(argc, argv, optstring)) != -1) {
        switch (opt) {
        case 'a':
            flags &= ~DF_SHOW_ADDRESS;
            break;

        case 'A':
            flags &= ~DF_SHOW_ASCII;
            break;

        case 'h':
            print_usage();
            return 0;

        case 'n':
            opts.num_bytes     = std::stoull(optarg, nullptr, 16);
            opts.use_num_bytes = true;
            break;

        case 's':
            opts.offset     = std::stoull(optarg, nullptr, 16);
            opts.use_offset = true;
            break;

        default:
            print_usage();
            return 1;
        }
    }

    if (optind >= argc) {
        print_usage();
        return 1;
    }

    bool      ok        = true;
    const int num_files = argc - optind;

    for (int i = optind; i < argc; ++i) {
        if (num_files > 1) {
            std::cout << argv[i] << '\n';
        }

        ok &= hex_dump_file(argv[i], flags, opts);

        if (num_files > 1 && i + 1 < argc) {
            std::cout << '\n';
        }
    }

    return ok ? 0 : 1;
}

