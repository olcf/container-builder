#pragma once

#include <fstream>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/spawn.hpp>

namespace asio = boost::asio;
using asio::ip::tcp;

class FileWriter {
public:
    explicit FileWriter(std::string file_name) : file_name(file_name),
                                                file_size(0) {
        file.open(file_name, std::fstream::in | std::fstream::binary);

        // Get filesize
        file.seekg(0, std::ios::end);
        file_size = file.tellg();
    }

    ~FileWriter() {
        file.close();
    }

    void async_write(tcp::socket &socket, asio::yield_context yield);
    void write(tcp::socket &socket);
private:
    const std::string file_name;
    std::fstream file;
    std::streampos file_size;
    std::array<char, 1024> buffer;
};