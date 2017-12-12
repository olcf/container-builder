#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/process.hpp>
#include <boost/regex.hpp>
#include <boost/asio.hpp>
#include <iostream>
#include "Messenger.h"
#include "Logger.h"

namespace asio = boost::asio;
using asio::ip::tcp;
namespace bp = boost::process;

std::string queue_host() {
    auto env = std::getenv("QUEUE_HOST");
    if (!env) {
        logger::write("QUEUE_HOST not set!");
        exit(EXIT_FAILURE);
    }
    return std::string(env);
}

std::string queue_port() {
    auto env = std::getenv("QUEUE_PORT");
    if (!env) {
        logger::write("QUEUE_PORT not set!");
        exit(EXIT_FAILURE);
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
                        boost::system::error_code error;

                        std::cout << prefix;
                        while (show_animation) {
                            std::cout << "\b-" << std::flush;
                            timer.expires_from_now(expire_time);
                            timer.async_wait(yield[error]);
                            std::cout << "\b\\" << std::flush;
                            timer.expires_from_now(expire_time);
                            timer.async_wait(yield[error]);
                            std::cout << "\b|" << std::flush;
                            timer.expires_from_now(expire_time);
                            timer.async_wait(yield[error]);
                            timer.expires_from_now(expire_time);
                            std::cout << "\b/" << std::flush;
                            timer.async_wait(yield[error]);
                            if(error && error != asio::error::operation_aborted) {
                                logger::write("Error animating spinner!");
                            }
                        }
                        std::cout<<std::endl;
                    });
    }

    void stop() {
        show_animation=false;
        std::cout<<std::endl;
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
                        // Hide the cursor and disable buffering for cleaner output
                        std::cout << "\e[?25l" << std::flush;
                        std::cout.setf(std::ios::unitbuf);

                        waiting_animation.start("Connecting to BuilderQueue: ");
                        tcp::socket queue_socket(io_service);
                        tcp::resolver queue_resolver(io_service);
                        boost::system::error_code error;

                        asio::async_connect(queue_socket, queue_resolver.resolve({queue_host(), queue_port()}), yield[error]);
                        waiting_animation.stop();
                        if(error) {
                            logger::write("Error connecting to builder queue: " + error.message());
                            return;
                        }
                        
                        Messenger queue_messenger(queue_socket);

                        // Initiate a build request
                        waiting_animation.start("Requesting Builder: ");
                        queue_messenger.async_send("checkout_builder_request", yield[error]);
                        if(error) {
                            logger::write("Error communicating with the builder queue!");
                            return;
                        }

                        // Wait on a builder from the queue
                        auto builder = queue_messenger.async_receive_builder(yield[error]);
                        waiting_animation.stop();
                        if(error) {
                            logger::write("Error obtaining VM builder from builder queue!" + error.message());
                            return;
                        }

                        waiting_animation.start("Connecting to Builder: ");
                        tcp::socket builder_socket(io_service);
                        tcp::resolver builder_resolver(io_service);
                        do {
                            asio::async_connect(builder_socket, builder_resolver.resolve({builder.host, builder.port}),
                                                yield[error]);
                        } while(error);
                        waiting_animation.stop();

                        Messenger builder_messenger(builder_socket);

                        // Once we're connected to the builder start the client process
                        logger::write("Sending definition: " + definition_path);

                        // Send the definition file
                        builder_messenger.async_send_file(definition_path, yield[error], true);
                        if(error) {
                            logger::write("Error sending definition file to builder!" + error.message());
                            return;
                        }

                        logger::write("Start of Singularity builder output:");

                        std::string line;
                        do {
                            line = builder_messenger.async_receive(yield[error]);
                            if(error) {
                                logger::write("Error streaming build output!" + error.message());
                                return;
                            }
                            std::cout << line;
                        } while (!line.empty());

                        // Read the container image
                        logger::write("Sending finished container: " + container_path);

                        builder_messenger.async_receive_file(container_path, yield[error], true);
                        if(error) {
                            logger::write("Error downloading container image!" + error.message());
                            return;
                        }

                        logger::write("Container received: " + container_path);

                        // Inform the queue we're done
                        queue_messenger.async_send(std::string("checkout_builder_complete"), yield[error]);
                        if(error) {
                            logger::write("Error ending build" + error.message());
                            return;
                        }
                    });

        // Begin processing our connections and queue
        io_service.run();
    }
    catch (...) {
        logger::write("Unknown ContainerBuilder exception: ");
    }

    // Restore the cursor
    std::cout << "\e[?25h" << std::flush;


    return 0;
}
