#pragma once

#include <functional>
#include <boost/asio.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/filesystem.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <iostream>
#include "BuilderData.h"
#include <memory.h>
#include "Logger.h"
#include "ClientData.h"
#include <boost/process.hpp>

namespace asio = boost::asio;
using asio::ip::tcp;
namespace bp = boost::process;
namespace beast = boost::beast;
namespace websocket = beast::websocket;

class Messenger {

public:
    // Create a client messenger by asyncronously connecting to the specified host
    explicit Messenger(asio::io_service &io_service,
                       const std::string &host,
                       const std::string &port,
                       asio::yield_context yield,
                       boost::system::error_code error) : stream(io_service) {
        do {
            tcp::resolver queue_resolver(io_service);
            asio::async_connect(stream.next_layer(), queue_resolver.resolve({host, port}), yield[error]);
        } while (error);

        stream.async_handshake(host, "/", yield[error]);
        stream.binary(true);
    }

    // Create a server messenger by doing an async block listen on the specified port
    explicit Messenger(asio::io_service &io_service,
                       const std::string &port,
                       asio::yield_context yield,
                       boost::system::error_code error) : stream(io_service) {
        tcp::acceptor acceptor(io_service, tcp::endpoint(tcp::v4(), std::stoi(port)));
        acceptor.async_accept(stream.next_layer(), yield[error]);
        stream.binary(true);
    }

    // Create a server messenger by doing an async block give the socket
    // The messenger will assume ownership of the socket
    explicit Messenger(tcp::acceptor &acceptor,
                       asio::io_service &io_service,
                       asio::yield_context yield,
                       boost::system::error_code error) : stream(io_service) {
        acceptor.async_accept(stream.next_layer(), yield[error]);
        stream.binary(true);
    }

    std::string async_read_string(asio::yield_context yield,
                                  boost::system::error_code &error);

    void async_write_string(const std::string &message,
                            asio::yield_context yield,
                            boost::system::error_code &error);

    void async_read_file(boost::filesystem::path file_path,
                         asio::yield_context yield,
                         boost::system::error_code &error);

    void async_write_file(boost::filesystem::path file_path,
                          asio::yield_context yield,
                          boost::system::error_code &error);

    BuilderData async_read_builder(asio::yield_context yield,
                                   boost::system::error_code &error);

    void async_write_builder(BuilderData builder,
                             asio::yield_context yield,
                             boost::system::error_code &error);

    ClientData async_read_client_data(asio::yield_context yield,
                                      boost::system::error_code &error);

    void async_write_client_data(ClientData client_data,
                                 asio::yield_context yield,
                                 boost::system::error_code &error);


    void async_write_pipe(bp::async_pipe &pipe,
                          asio::yield_context yield,
                          boost::system::error_code &error);

    void async_stream_print(asio::yield_context yield,
                            boost::system::error_code &error);

private:
    websocket::stream <tcp::socket> stream;
};