#pragma once

#include <functional>
#include <boost/asio.hpp>
#include <boost/asio/io_context.hpp>
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
    explicit Messenger(asio::io_context &io_context,
                       const std::string &host,
                       const std::string &port,
                       asio::yield_context yield,
                       std::error_code& error) : stream(io_context) {

        boost::system::error_code connect_error;
        do {
            tcp::resolver queue_resolver(io_context);
            asio::async_connect(stream.next_layer(), queue_resolver.resolve({host, port}), yield[connect_error]);
        } while (connect_error);

        if(connect_error) {
            logger::write("Initial connection failed: " + connect_error.message(), logger::severity_level::error);
            error = std::error_code(connect_error.value(), std::generic_category());
            return;
        }

        beast::error_code handshake_error;
        stream.async_handshake(host + ":" + port, "/", yield[handshake_error]);
        if(handshake_error) {
            logger::write("WebSocket handshake failed: " + handshake_error.message(), logger::severity_level::error);
            error = std::error_code(handshake_error.value(), std::generic_category());
            return;
        }

        stream.binary(true);
    }

    // Create a server messenger by doing an async block listen on the specified port
    explicit Messenger(asio::io_service &io_service,
                       const std::string &port,
                       asio::yield_context yield,
                       std::error_code& error) : stream(io_service) {

        tcp::acceptor acceptor(io_service, tcp::endpoint(tcp::v4(), std::stoi(port)));

        boost::system::error_code accept_error;
        acceptor.async_accept(stream.next_layer(), yield[accept_error]);
        if(accept_error) {
            logger::write("Initial connection failed: " + accept_error.message(), logger::severity_level::error);
            error = std::error_code(accept_error.value(), std::generic_category());
            return;
        }

        stream.async_accept(yield[accept_error]);
        if(accept_error) {
            logger::write("WebSocket handshake failed: " + accept_error.message(), logger::severity_level::error);
            error = std::error_code(accept_error.value(), std::generic_category());
            return;
        }

        stream.binary(true);
    }

    // Create a server messenger by doing an async block give the socket
    // The messenger will assume ownership of the socket
    explicit Messenger(tcp::acceptor &acceptor,
                       asio::yield_context yield,
                       std::error_code& error) : stream(acceptor.get_io_context()) {

        boost::system::error_code accept_error;
        acceptor.async_accept(stream.next_layer(), yield[accept_error]);
        if(accept_error) {
            logger::write("error with async_connect: " + accept_error.message(), logger::severity_level::error);
            error = std::error_code(accept_error.value(), std::generic_category());
            return;
        }

        stream.async_accept(yield[accept_error]);
        if(accept_error) {
            logger::write("error with async_connect: " + accept_error.message(), logger::severity_level::error);
            error = std::error_code(accept_error.value(), std::generic_category());
            return;
        }

        stream.binary(true);
    }

    std::string async_read_string(asio::yield_context yield,
                                  std::error_code &error);

    void async_write_string(const std::string &message,
                            asio::yield_context yield,
                            std::error_code &error);

    void async_read_file(boost::filesystem::path file_path,
                         asio::yield_context yield,
                         std::error_code &error);

    void async_write_file(boost::filesystem::path file_path,
                          asio::yield_context yield,
                          std::error_code &error);

    BuilderData async_read_builder(asio::yield_context yield,
                                   std::error_code &error);

    void async_write_builder(BuilderData builder,
                             asio::yield_context yield,
                             std::error_code &error);

    ClientData async_read_client_data(asio::yield_context yield,
                                      std::error_code &error);

    void async_write_client_data(ClientData client_data,
                                 asio::yield_context yield,
                                 std::error_code &error);


    void async_write_pipe(bp::async_pipe &pipe,
                          asio::yield_context yield,
                          std::error_code &error);

    void async_stream_print(asio::yield_context yield,
                            std::error_code &error);

private:
    websocket::stream<tcp::socket> stream;
};