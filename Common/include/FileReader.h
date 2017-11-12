#pragma once

#include <fstream>
#include <vector>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/spawn.hpp>
#include "Logger.h"
#include <boost/asio/streambuf.hpp>

namespace asio = boost::asio;
using asio::ip::tcp;

class FileReader {
public:
    explicit FileReader(std::string file_name, std::size_t buffer_size=1024) : file_name(file_name),
                                                                               file_size(0),
                                                                               buffer(buffer_size)
    {
        file.exceptions(std::fstream::failbit | std::ifstream::badbit);
        try {
            file.open(file_name, std::fstream::out | std::fstream::binary | std::fstream::trunc);
        } catch (std::system_error &e) {
            logger::write(e.code().message());
        }
    }

    ~FileReader() {
        file.close();
    }

    void async_read(tcp::socket &socket, asio::yield_context yield);

    void read(tcp::socket &socket);

private:
    const std::string file_name;
    std::ofstream file;
    std::streampos file_size;
    std::vector<char> buffer;
};