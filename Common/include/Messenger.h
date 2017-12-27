#pragma once

#include <functional>
#include <boost/asio.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/filesystem.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <iostream>
//#include <boost/archive/text_iarchive.hpp>
//#include <boost/archive/text_oarchive.hpp>
#include "BuilderData.h"
//#include <boost/progress.hpp>
//#include <boost/crc.hpp>
#include <memory.h>
#include "Logger.h"
#include "ClientData.h"

namespace asio = boost::asio;
using asio::ip::tcp;
namespace websocket = boost::beast::websocket;

class Messenger {

public:
    // Create a client messenger by asyncronously connecting to the specified host
    explicit Messenger(asio::io_service &io_service,
                       const std::string &host,
                       const std::string &port,
                       asio::yield_context yield,
                       boost::system::error_code error) : web_socket(io_service) {
        do {
            tcp::resolver queue_resolver(io_service);
            asio::async_connect(web_socket.next_layer(), queue_resolver.resolve({host, port}), yield[error]);
        } while (error);

        web_socket.async_handshake(host, "/", yield[error]);
    }

    // Create a server messenger by doing an async block listen on the specified port
    explicit Messenger(asio::io_service &io_service,
                       const std::string &port,
                       asio::yield_context yield,
                       boost::system::error_code error) : web_socket(io_service) {
        tcp::acceptor acceptor(io_service, tcp::endpoint(tcp::v4(), std::stoi(port)));
        acceptor.async_accept(web_socket.next_layer(), yield[error]);
    }

    // Create a server messenger by doing an async block give the socket
    // The messenger will assume ownership of the socket
    explicit Messenger(tcp::acceptor& acceptor,
            asio::io_service &io_service,
                       asio::yield_context yield,
                       boost::system::error_code error) : web_socket(io_service) {
        acceptor.async_accept(web_socket.next_layer(), yield[error]);
    }

    std::string async_receive();

    void async_send(const std::string &message,
                    asio::yield_context yield,
                    boost::system::error_code error);

    void async_send(asio::streambuf &message_body,
                    asio::yield_context yield,
                    boost::system::error_code error);

    void async_receive_file(boost::filesystem::path file_path,
                            asio::yield_context yield,
                            boost::system::error_code error,
                            const bool print_progress = false
    );

    void async_send_file(boost::filesystem::path file_path,
                         asio::yield_context yield,
                         boost::system::error_code error,
                         const bool print_progress = false
    );

    BuilderData async_receive_builder(asio::yield_context yield,
                                      boost::system::error_code error);

    void async_send(BuilderData builder,
                    asio::yield_context yield,
                    boost::system::error_code error);

    ClientData async_receive_client_data(asio::yield_context yield,
                                         boost::system::error_code error);

    void async_send(ClientData client_data,
                    asio::yield_context yield,
                    boost::system::error_code error);

private:
    websocket::stream<tcp::socket> web_socket;
};