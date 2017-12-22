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
    // Create a client messenger by asyncronously connecting to the specified host
    explicit Messenger(asio::io_service &io_service, const std::string &host, const std::string &port,
                       asio::yield_context yield) : socket(io_service),
                                                    yield(yield) {
        do {
            tcp::resolver queue_resolver(io_service);
            asio::async_connect(socket, queue_resolver.resolve({host, port}), yield[error]);
        } while(error);
    }

    // Create a server messenger by doing an async block listen on the specified port
    explicit Messenger(asio::io_service &io_service, const std::string &port, asio::yield_context yield) :
            socket(io_service),
            yield(yield) {
        tcp::acceptor acceptor(io_service, tcp::endpoint(tcp::v4(), std::stoi(port)));
        acceptor.async_accept(socket, yield[error]);
    }

    // Create a server messenger by doing an async block give the socket
    // The messenger will assume ownership of the socket
    explicit Messenger(tcp::socket socket, asio::yield_context yield) : socket(std::move(socket)),
                                                                        yield(yield) {
    }

    std::string async_receive(MessageType type = MessageType::string);

    std::string async_receive(MessageType *type);

    void async_send(const std::string &message, MessageType type = MessageType::string);

    void async_send(asio::streambuf &message_body);

    void async_receive_file(boost::filesystem::path file_path, const bool print_progress = false);

    void async_send_file(boost::filesystem::path file_path, const bool print_progress = false);

    BuilderData async_receive_builder();

    void async_send(BuilderData builder);

    ClientData async_receive_client_data();

    void async_send(ClientData client_data);

    tcp::socket socket;
    // asio yield_context.ec_ is private and so i'd prefer to not (ab)use it, as such hold on to a single error variable that gets set
    // by any Messenger failure. Taking the yield_context and error seperately in the constructor is one way to handle this
    // This guards somewhat against passing yield[ec] to one of the messenger functions on accident
    asio::yield_context yield;
    boost::system::error_code error;
private:
    void async_send_header(std::size_t message_size, MessageType type);

    Header async_receive_header();

    static constexpr std::size_t header_size() {
        return sizeof(Header);
    }
};