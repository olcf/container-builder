#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/process.hpp>
#include <boost/regex.hpp>
#include <boost/asio.hpp>
#include <iostream>
#include "Messenger.h"

namespace asio = boost::asio;
using asio::ip::tcp;
namespace bp = boost::process;

std::string queue_host() {
    auto env = std::getenv("QUEUE_HOST");
    if (!env) {
        throw std::system_error(ENOTSUP, std::system_category(), "QUEUE_HOST not set!");
    }
    return std::string(env);
}

std::string queue_port() {
    auto env = std::getenv("QUEUE_PORT");
    if (!env) {
        throw std::system_error(ENOTSUP, std::system_category(), "QUEUE_PORT not set!");
    }
    return std::string(env);
}

// Simple waiting animation usable with async coroutines operations
class WaitingAnimation {
public:
    WaitingAnimation(asio::io_service &io_service) : io_service(io_service),
                                                     timer(io_service),
                                                     expire_time(250),
                                                     show_animation(false) {}

    // Start a coroutine animation that yields during animation
    // When stop() is called the coroutine will exit
    void start(std::string prefix) {
        show_animation = true;

        asio::spawn(io_service,
                    [&](asio::yield_context yield) {
                        std::cout << prefix;
                        while (show_animation) {
                            std::cout << "\b--" << std::flush;
                            timer.expires_from_now(expire_time);
                            timer.async_wait(yield);
                            std::cout << "\b\b\\ " << std::flush;
                            timer.expires_from_now(expire_time);
                            timer.async_wait(yield);
                            std::cout << "\b|" << std::flush;
                            timer.expires_from_now(expire_time);
                            timer.async_wait(yield);
                            timer.expires_from_now(expire_time);
                            std::cout << "\b/" << std::flush;
                            timer.async_wait(yield);
                        }
                        std::cout<<std::endl;
                    });
    }

    void stop() {
        show_animation=false;
    }

private:
    asio::io_service& io_service;
    boost::asio::deadline_timer timer;
    const boost::posix_time::milliseconds expire_time;
    bool show_animation;
};

int main(int argc, char *argv[]) {

    try {

        // Check for correct number of arguments
        if (argc != 3) {
            std::cerr << "Usage: ContainerBuilder <definition path> <container path>\n";
            return 1;
        }
        std::string definition_path(argv[1]);
        std::string container_path(argv[2]);

        asio::io_service io_service;
        WaitingAnimation waiting_animation(io_service);

        asio::spawn(io_service,
                    [&](asio::yield_context yield) {
                        waiting_animation.start("Connecting to BuilderQueue: ");
                        tcp::socket queue_socket(io_service);
                        tcp::resolver queue_resolver(io_service);
                        boost::system::error_code ec;

                        asio::async_connect(queue_socket, queue_resolver.resolve({queue_host(), queue_port()}), yield);
                        waiting_animation.stop();

                        Messenger queue_messenger(queue_socket);

                        // Initiate a build request
                        waiting_animation.start("Requesting Builder: ");
                        queue_messenger.async_send("checkout_builder_request", yield);

                        // Wait on a builder from the queue
                        auto builder = queue_messenger.async_receive_builder(yield);
                        waiting_animation.stop();

                        waiting_animation.start("Connecting to Builder: ");
                        tcp::socket builder_socket(io_service);
                        tcp::resolver builder_resolver(io_service);
                        do {
                            asio::async_connect(builder_socket, builder_resolver.resolve({builder.host, builder.port}),
                                                yield[ec]);
                        } while (ec != boost::system::errc::success);
                        waiting_animation.stop();

                        Messenger builder_messenger(builder_socket);

                        // Once we're connected to the builder start the client process
                        std::cout << "Sending definition: " << definition_path << std::endl;

                        // Send the definition file
                        builder_messenger.async_send_file(definition_path, yield, true);

                        std::cout << "Start of Singularity builder output:" << std::endl;

                        // Hide the cursor and disable buffering for cleaner output
                        std::cout << "\e[?25l" << std::flush;
                        std::cout.setf(std::ios::unitbuf);

                        std::string line;
                        do {
                            line = builder_messenger.async_receive(yield);
                            std::cout << line;
                        } while (!line.empty());

                        // Read the container image
                        std::cout << "Sending finished container: " << container_path << std::endl;

                        builder_messenger.async_receive_file(container_path, yield, true);

                        std::cout << "Container received: " << container_path << std::endl;

                        // Inform the queue we're done
                        queue_messenger.async_send(std::string("checkout_builder_complete"), yield);
                    });

        // Begin processing our connections and queue
        io_service.run();
    }
    catch (std::exception &e) {
        std::cout << std::string() + "Build exception: " + e.what() << std::endl;
    }

    std::cout << "Client shutting down\n";

    // Show the cursor
    std::cout << "\e[?25h" << std::flush;


    return 0;
}