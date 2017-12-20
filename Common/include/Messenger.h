#pragma once

#include <functional>
#include <boost/asio.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/filesystem.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/write.hpp>
#include <iostream>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include "BuilderData.h"
#include <boost/progress.hpp>
#include <boost/crc.hpp>
#include <memory.h>
#include "Logger.h"
#include "ClientData.h"

namespace asio = boost::asio;
using asio::ip::tcp;

// Enum to handle message type, used to ensure
enum class MessageType : unsigned char {
    string,
    builder,
    client_data,
    file,
    error
};

// Message header
struct Header {
    std::size_t size;
    MessageType type;
};

class Messenger {

public:
    // Create a client messenger by connecting to the specified host
    explicit Messenger(asio::io_service& io_service, std::string host, std::string port, asio::yield_context yield) : socket(io_service) {
        tcp::resolver queue_resolver(io_service);
        boost::system::error_code error;

        asio::async_connect(socket, queue_resolver.resolve({host, port}), yield[error]);
    }

    // Create a server messenger on the specified port
    explicit Messenger(asio::io_service io_service, std::string port) : socket(io_service) {
        tcp::resolver queue_resolver(io_service);
        boost::system::error_code error;

        tcp::acceptor acceptor(io_service, tcp::endpoint(tcp::v4(), std::stoi(port)));
        acceptor.accept(socket);
    }

    std::string async_receive(asio::yield_context yield, MessageType type=MessageType::string);
    std::string async_receive(asio::yield_context yield, MessageType* type);

    void async_send(const std::string &message, asio::yield_context yield, MessageType type=MessageType::string);
    void async_send(asio::streambuf &message_body, asio::yield_context yield);

    void async_receive_file(boost::filesystem::path file_path, asio::yield_context yield, const bool print_progress=false);
    void async_send_file(boost::filesystem::path file_path, asio::yield_context yield, const bool print_progress=false);

    BuilderData async_receive_builder(asio::yield_context yield);
    void async_send(BuilderData builder, asio::yield_context yield);

    ClientData async_receive_client_data(asio::yield_context yield);
    void async_send(ClientData client_data, asio::yield_context yield);

    tcp::socket socket;

private:
    void async_send_header(std::size_t message_size, MessageType type, asio::yield_context yield);
    Header async_receive_header(asio::yield_context yield);

    static constexpr std::size_t header_size() {
        return sizeof(Header);
    }
};