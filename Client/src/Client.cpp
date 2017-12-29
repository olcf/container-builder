#include "Client.h"
#include <iostream>
#include <boost/program_options.hpp>
#include <boost/process.hpp>
#include "WaitingAnimation.h"
#include "pwd.h"

namespace bp = boost::process;

Client::Client(int argc, char **argv) {
    parse_environment();
    parse_arguments(argc, argv);

    // Hide the cursor and disable buffering for cleaner builder output
    std::cout << "\e[?25l" << std::flush;
    std::cout.setf(std::ios::unitbuf);

    // Full client connection - this will not run until the io_context is started
    asio::spawn(io_context,
                [this](asio::yield_context yield) {

                    auto queue_messenger = connect_to_queue(yield);

                    auto builder_messenger = connect_to_builder(queue_messenger, yield);

                    std::error_code error;

                    // Send client data to builder
                    builder_messenger.async_write_client_data(client_data(), yield, error);
                    if (error) {
                        throw std::runtime_error("Error sending client data to builder!");
                    }

                    logger::write("Sending definition: " + definition_path);
                    builder_messenger.async_write_file(definition_path, yield, error);
                    if (error) {
                        throw std::runtime_error("Error sending definition file to builder: " + error.message());
                    }

                    // Stream the builder output
                    builder_messenger.async_stream_print(yield, error);
                    if (error) {
                        throw std::runtime_error("Error streaming builder output: " + error.message());
                    }

                    // Read the container from the builder
                    builder_messenger.async_read_file(container_path, yield, error);
                    if (error) {
                        throw std::runtime_error("Error downloading container image: " + error.message());
                    }

                    logger::write("Container received: " + container_path, logger::severity_level::success);

                    queue_messenger.async_write_string("checkout_builder_complete", yield, error);
                    if (error) {
                        throw std::runtime_error("Error ending build!");
                    }
                });
}

void Client::parse_arguments(int argc, char **argv) {
    namespace po = boost::program_options;

    // Supported arguments
    po::options_description desc("Allowed options");
    desc.add_options()
            ("help", "produce help message")
            ("arch", po::value<std::string>()->default_value("x86_64"),
             "select architecture, valid options are x86_64 and ppc64le")
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

    // Print help if requested or if minimum arguments aren't provided
    if (!vm.count("container") || !vm.count("definition") || vm.count("help")) {
        std::cerr << desc << std::endl;
        exit(EXIT_FAILURE);
    }

    // Set class variables based upon program arguments
    this->definition_path = vm["definition"].as<std::string>();
    this->container_path = vm["container"].as<std::string>();
    this->tty = vm["tty"].as<bool>();
    this->arch = Arch::to_arch(vm["arch"].as<std::string>());

    // Make sure variables are set as required
    po::notify(vm);
}

void Client::parse_environment() {
    struct passwd *pws;
    pws = getpwuid(getuid());
    if (pws == NULL) {
        throw std::runtime_error("Error get login name for client data!");
    }
    user_id = std::string(pws->pw_name);

    auto host = std::getenv("QUEUE_HOST");
    if (!host) {
        throw std::runtime_error("QUEUE_HOST not set!");
    }
    this->queue_host = std::string(host);

    auto port = std::getenv("QUEUE_PORT");
    if (!port) {
        throw std::runtime_error("QUEUE_PORT not set!");
    }
    this->queue_port = std::string(port);
}

Messenger Client::connect_to_queue(asio::yield_context yield) {
    // Start waiting animation
    WaitingAnimation wait_queue("Connecting to BuilderQueue: ");

    std::error_code error;
    Messenger queue_messenger(io_context, queue_host, queue_port, yield, error);
    if (error) {
        wait_queue.stop("Failed\n", logger::severity_level::fatal);
        throw std::runtime_error("The ContainerBuilder queue is currently unreachable.");
    } else {
        wait_queue.stop("Connected to queue: " + queue_host, logger::severity_level::success);
    }

    return queue_messenger;
}

Messenger Client::connect_to_builder(Messenger &queue_messenger, asio::yield_context yield) {
    WaitingAnimation wait_builder("Requesting BuilderData: ");

    std::error_code error;

    // Request a builder from the queue
    queue_messenger.async_write_string("checkout_builder_request", yield, error);
    if (error) {
        wait_builder.stop("Failed\n", logger::severity_level::fatal);
        throw std::runtime_error("Error communicating with the builder queue!");
    }

    // Wait to receive the builder from the queue
    auto builder = queue_messenger.async_read_builder(yield, error);
    if (error) {
        wait_builder.stop("Failed\n", logger::severity_level::fatal);
        throw std::runtime_error("Error obtaining VM builder from builder queue!");
    }

    // Create a messenger to the builder
    Messenger builder_messenger(io_context, builder.host, builder.port, yield, error);
    if (error) {
        wait_builder.stop("Failed\n", logger::severity_level::fatal);
        throw std::runtime_error("Failed to connect to builder!");
    }

    wait_builder.stop("Connected to builder: " + builder.host, logger::severity_level::success);
    return builder_messenger;
}

ClientData Client::client_data() {
    ClientData data;
    data.user_id = user_id;
    data.tty = tty;
    data.arch = arch;

    return data;
}

void Client::run() {
    io_context.run();
}