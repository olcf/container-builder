#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/process.hpp>
#include <boost/regex.hpp>
#include <boost/asio.hpp>
#include <iostream>
#include "Messenger.h"
#include <boost/program_options.hpp>

namespace asio = boost::asio;
using asio::ip::tcp;
namespace bp = boost::process;
namespace po = boost::program_options;

std::string queue_host() {
    auto env = std::getenv("QUEUE_HOST");
    if (!env) {
        logger::write("QUEUE_HOST not set!", logger::severity_level::fatal);
        exit(EXIT_FAILURE);
    }
    return std::string(env);
}

std::string queue_port() {
    auto env = std::getenv("QUEUE_PORT");
    if (!env) {
        logger::write("QUEUE_PORT not set!", logger::severity_level::fatal);
        exit(EXIT_FAILURE);
    }
    return std::string(env);
}

// Print animated ellipses, used to indicate to the user we're waiting on an async routine
class WaitingAnimation {
public:
    WaitingAnimation(asio::io_service &io_service, std::string prefix) : io_service(io_service),
                                                                         timer(io_service),
                                                                         expire_time(
                                                                                 boost::posix_time::milliseconds(500)),
                                                                         prefix(prefix) {
        asio::spawn(io_service,
                    [this, prefix](asio::yield_context yield) {
                        boost::system::error_code error;

                        for (;;) {
                            if (expire_time.is_not_a_date_time())
                                break;
                            std::cout << "\r" << prefix << ".  ";
                            timer.expires_from_now(expire_time);
                            timer.async_wait(yield[error]);

                            if (expire_time.is_not_a_date_time())
                                break;
                            std::cout << "\b\b.";
                            timer.expires_from_now(expire_time);
                            timer.async_wait(yield[error]);

                            if (expire_time.is_not_a_date_time())
                                break;
                            std::cout << ".";
                            timer.expires_from_now(expire_time);
                            timer.async_wait(yield[error]);
                        }
                    });
    }

    // Cancel any outstanding timers and set the expire time to not a date, stopping the animation
    // The suffix string will be printed in place of the animated ellipses
    void stop(std::string suffix, logger::severity_level level) {
        boost::system::error_code error;
        expire_time = boost::posix_time::not_a_date_time;
        timer.cancel(error);
        std::cout << "\r" << std::flush;
        logger::write(prefix + suffix, level);
    }

private:
    asio::io_service &io_service;
    boost::asio::deadline_timer timer;
    boost::posix_time::time_duration expire_time;
    std::string prefix;
};

int main(int argc, char *argv[]) {

    try {
        // Supported arguments
        po::options_description desc("Allowed options");
        desc.add_options()
                ("help", "produce help message")
                ("arch", po::value<std::string>()->default_value("x86_64"), "select architecture, valid options are x86_64 and ppc64le")
                ("tty", po::value<bool>()->default_value(isatty(fileno(stdout))),
                 "true if the data should be presented as if a tty is present")
                ("container", po::value<std::string>()->required(), "(required) the container name")
                ("definition", po::value<std::string>()->required(), "(required) the definition file");

        // Position arguments, the container output name and input definition
        po::positional_options_description pos_desc;
        pos_desc.add("container", 1);
        pos_desc.add("definition", 1);

        // Parse arguments
        po::variables_map vm;
        po::store(po::command_line_parser(argc, argv).options(desc).positional(pos_desc).run(), vm);

        // Print help if requested
        if (!vm.count("container") || !vm.count("definition") || vm.count("help")) {
            std::cerr << desc << std::endl;
            return 0;
        }

        // Set local variables based upon program arguments
        auto definition_path = vm["definition"].as<std::string>();
        auto container_path = vm["container"].as<std::string>();
        auto tty = vm["tty"].as<bool>();
        auto arch = vm["arch"].as<std::string>();

        // Make sure variables are set as required
        po::notify(vm);

        asio::io_service io_service;

        asio::spawn(io_service,
                    [&](asio::yield_context yield) {
                        // Hide the cursor and disable buffering for cleaner output
                        std::cout << "\e[?25l" << std::flush;
                        std::cout.setf(std::ios::unitbuf);

                        WaitingAnimation wait_queue(io_service, "Connecting to BuilderQueue: ");
                        tcp::socket queue_socket(io_service);
                        tcp::resolver queue_resolver(io_service);
                        boost::system::error_code error;

                        asio::async_connect(queue_socket, queue_resolver.resolve({queue_host(), queue_port()}),
                                            yield[error]);
                        if (error) {
                            wait_queue.stop("Failed\n", logger::severity_level::fatal);
                            logger::write("Error connecting to builder queue: " + error.message(),
                                          logger::severity_level::fatal);
                            return;
                        }
                        wait_queue.stop("", logger::severity_level::success);


                        Messenger queue_messenger(queue_socket);

                        // Initiate a build request
                        WaitingAnimation wait_get_builder(io_service, "Requesting Builder: ");
                        queue_messenger.async_send("checkout_builder_request", yield[error]);
                        if (error) {
                            logger::write("Error communicating with the builder queue!", logger::severity_level::fatal);
                            return;
                        }

                        // Wait on a builder from the queue
                        auto builder = queue_messenger.async_receive_builder(yield[error]);
                        if (error) {
                            wait_get_builder.stop("Failed\n", logger::severity_level::fatal);
                            logger::write("Error obtaining VM builder from builder queue!" + error.message());
                            return;
                        }
                        wait_get_builder.stop("", logger::severity_level::success);

                        WaitingAnimation wait_builder(io_service, "Connecting to Builder: ");
                        tcp::socket builder_socket(io_service);
                        tcp::resolver builder_resolver(io_service);
                        do {
                            asio::async_connect(builder_socket, builder_resolver.resolve({builder.host, builder.port}),
                                                yield[error]);
                        } while (error);
                        wait_builder.stop("", logger::severity_level::success);

                        Messenger builder_messenger(builder_socket);

                        // Send client information to builder
                        ClientData client_data;
                        char user[100];
                        int err = getlogin_r(user, 100);
                        if (err) {
                            logger::write("Error get login name for client data!" + error.message(),
                                          logger::severity_level::fatal);
                            return;
                        }
                        client_data.user_id = std::string(user);
                        client_data.tty = tty;
                        client_data.set_arch(arch);
                        builder_messenger.async_send(client_data, yield[error]);
                        if (error) {
                            logger::write("Error sending client data to builder!" + error.message(),
                                          logger::severity_level::fatal);
                            return;
                        }

                        // Once we're connected to the builder start the client process
                        logger::write("Sending definition: " + definition_path);

                        // Send the definition file
                        builder_messenger.async_send_file(definition_path, yield[error], true);
                        if (error) {
                            logger::write("Error sending definition file to builder!" + error.message(),
                                          logger::severity_level::fatal);
                            return;
                        }

                        logger::write("Start of Singularity builder output:");

                        std::string line;
                        do {
                            line = builder_messenger.async_receive(yield[error]);
                            if (error) {
                                logger::write("Error streaming build output!" + error.message(),
                                              logger::severity_level::fatal);
                                return;
                            }
                            std::cout << line;
                        } while (!line.empty());

                        // Read the container image
                        logger::write("Sending finished container: " + container_path);

                        builder_messenger.async_receive_file(container_path, yield[error], true);
                        if (error) {
                            logger::write("Error downloading container image!" + error.message(),
                                          logger::severity_level::fatal);
                            return;
                        }

                        logger::write("Container received: " + container_path, logger::severity_level::success);

                        // Inform the queue we're done
                        queue_messenger.async_send(std::string("checkout_builder_complete"), yield[error]);
                        if (error) {
                            logger::write("Error ending build" + error.message(), logger::severity_level::fatal);
                            return;
                        }
                    });

        // Begin processing our connections and queue
        io_service.run();
    }
    catch (std::exception &exception) {
        logger::write(exception.what(), logger::severity_level::fatal);
    }
    catch (...) {
        logger::write("Unknown ContainerBuilder exception: ", logger::severity_level::fatal);
    }

    // Restore the cursor
    std::cout << "\e[?25h" << std::flush;

    return 0;
}
