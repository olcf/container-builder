#include <fstream>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/connect.hpp>
#include <boost/filesystem.hpp>
#include <boost/beast/core/error.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/exception/all.hpp>
#include <boost/program_options.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <regex>
#include <csignal>
#include "BuilderData.h"
#include "ClientData.h"
#include "WaitingAnimation.h"
#include "pwd.h"

namespace asio = boost::asio;
using asio::ip::tcp;
namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace pt = boost::property_tree;

void parse_environment(ClientData &client_data) {
    Logger::debug("Parsing environment variables");

    struct passwd *pws;
    pws = getpwuid(getuid());
    if (pws == NULL) {
        throw std::runtime_error("Error get login name for client data!");
    }
    client_data.user_id = std::string(pws->pw_name);

    auto host = std::getenv("QUEUE_HOST");
    if (!host) {
        throw std::runtime_error("QUEUE_HOST not set!");
    }
    client_data.queue_host = std::string(host);
}

void print_status(websocket::stream<tcp::socket> &queue_stream) {
    Logger::debug("Writing status request string");
    std::string request_string("queue_status_request");
    queue_stream.write(asio::buffer(request_string));

    Logger::debug("Read json formatted status string");
    std::string queue_status_string;
    auto queue_status_buffer = boost::asio::dynamic_buffer(queue_status_string);
    queue_stream.read(queue_status_buffer);
    std::stringstream json_stream(queue_status_string);

    pt::ptree status_tree;
    pt::read_json(json_stream, status_tree);
    std::cout<<"-------------\n";
    std::cout<<"Active builders\n";
    std::cout<<"-------------\n\n";

    for(auto builder : status_tree.get_child("active")) {
        std::cout<<"ID: " << builder.second.get<std::string>("id") << std::endl;
        std::cout<<"HOST: " << builder.second.get<std::string>("host") << std::endl;
        std::cout<<"PORT: " << builder.second.get<std::string>("port") << std::endl;
        std::cout<<"-------------\n";
    }

    std::cout<<"\n-------------\n";
    std::cout<<"Reserve builders\n";
    std::cout<<"-------------\n\n";

    for(auto builder : status_tree.get_child("reserve")) {
        std::cout<<"ID: " << builder.second.get<std::string>("id") << std::endl;
        std::cout<<"HOST: " << builder.second.get<std::string>("host") << std::endl;
        std::cout<<"PORT: " << builder.second.get<std::string>("port") << std::endl;
        std::cout<<"-------------\n";
    }
}

int main(int argc, char *argv[]) {
    asio::io_context io_context;
    websocket::stream<tcp::socket> queue_stream(io_context);

    try {
        ClientData client_data;

        parse_environment(client_data);

        WaitingAnimation wait_queue("Connecting to BuilderQueue");
        // Open a WebSocket stream to the queue
        tcp::resolver queue_resolver(io_context);
        asio::connect(queue_stream.next_layer(), queue_resolver.resolve({client_data.queue_host, "8080"}));
        queue_stream.handshake(client_data.queue_host + ":8080", "/");
        wait_queue.stop_success("Connected to queue: " + client_data.queue_host);

        // Request a build host from the queue
        Logger::info("Requesting queue status");
        print_status(queue_stream);

    } catch (const boost::exception &ex) {
        auto diagnostics = diagnostic_information(ex);
        Logger::error(std::string() + "Container Builder exception encountered: " + diagnostics);
    } catch (const std::exception &ex) {
        Logger::error(std::string() + "Container Builder exception encountered: " + ex.what());
    } catch (...) {
        Logger::error("Unknown exception caught!");
    }

    // Attempt to disconnect from builder and queue
    try {
        Logger::debug("Attempting normal close");
        queue_stream.close(websocket::close_code::normal);
    } catch (...) {
        Logger::debug("Failed to cleanly close the WebSockets");
    }

    return 0;
}