#include <fstream>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/connect.hpp>
#include <boost/filesystem.hpp>
#include <boost/beast/core/error.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/exception/all.hpp>
#include <boost/program_options.hpp>
#include <regex>
#include <csignal>
#include "ClientData.h"
#include "WaitingAnimation.h"
#include "pwd.h"

namespace asio = boost::asio;
using asio::ip::tcp;
namespace beast = boost::beast;
namespace websocket = beast::websocket;

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
        WaitingAnimation wait_images("Requesting image list");

        Logger::debug("Writing image list request string");
        std::string request_string("image_list_request");
        queue_stream.write(asio::buffer(request_string));

        Logger::debug("Read image list string");
        std::string images_string;
        auto images_buffer = boost::asio::dynamic_buffer(images_string);
        queue_stream.read(images_buffer);

        wait_images.stop_success("Fetched");
        std::cout << images_string;

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