#include "Client.h"
#include <boost/program_options.hpp>
#include <boost/process.hpp>
#include "WaitingAnimation.h"

namespace bp = boost::process;

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
    char user[15];
    int err = getlogin_r(user, 15);
    if (err) {
        throw std::runtime_error("Error get login name for client data!");
    }
    user_id = std::string(user);

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
    WaitingAnimation wait_queue(io_service, "Connecting to BuilderQueue: ");

    boost::system::error_code error;
    Messenger queue_messenger(io_service, queue_host, queue_port, yield[error]);
    if (error) {
        wait_queue.stop("Failed\n", logger::severity_level::fatal);
        throw std::runtime_error("Failed to connect to builder queue!");
    }
    wait_queue.stop("", logger::severity_level::success);

    return queue_messenger;
}

Messenger Client::connect_to_builder(Messenger &queue_messenger, asio::yield_context yield) {
    WaitingAnimation wait_builder(io_service, "Requesting BuilderData: ");
    boost::system::error_code error;

    // Request a builder from the queue
    queue_messenger.async_send("checkout_builder_request", yield[error]);
    if (error) {
        wait_builder.stop("Failed\n", logger::severity_level::fatal);
        throw std::runtime_error("Error communicating with the builder queue!");
    }

    // Wait to receive the builder from the queue
    auto builder = queue_messenger.async_receive_builder(yield[error]);
    if (error) {
        wait_builder.stop("Failed\n", logger::severity_level::fatal);
        throw std::runtime_error("Error obtaining VM builder from builder queue!");
    }

    // Create a messenger to the builder
    Messenger builder_messenger(io_service, builder.host, builder.port, yield[error]);
    if (error) {
        wait_builder.stop("Failed\n", logger::severity_level::fatal);
        throw std::runtime_error("Failed to connect to builder!");
    }

    wait_builder.stop("", logger::severity_level::success);
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
    io_service.run();
}