#include <fstream>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/connect.hpp>
#include <boost/filesystem.hpp>
#include <boost/beast/core/error.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/exception/all.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/process/system.hpp>
#include <boost/process/io.hpp>
#include <boost/crc.hpp>
#include <boost/program_options.hpp>
#include <regex>
#include <csignal>
#include "ClientData.h"
#include "BuilderData.h"
#include "WaitingAnimation.h"
#include "pwd.h"

namespace asio = boost::asio;
using asio::ip::tcp;
namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace bfs = boost::filesystem;
namespace bp = boost::process;

void hide_cursor() {
    std::cout << "\e[?25l";
}

void show_cursor() {
    std::cout << "\e[?25h";
}

void early_exit() {
    show_cursor();
    std::exit(EXIT_FAILURE);
}

void write_client_data(websocket::stream<tcp::socket> &builder_stream, ClientData client_data) {
    Logger::debug("Writing client data");

    Logger::debug("Serializing client data string");
    std::ostringstream archive_stream;
    boost::archive::text_oarchive archive(archive_stream);
    archive << client_data;
    auto serialized_client_data = archive_stream.str();

    Logger::debug("Writing serialized client data");
    builder_stream.write(asio::buffer(serialized_client_data));
}

void read_file(websocket::stream<tcp::socket> &stream, const std::string &file_name) {
    Logger::debug("Opening " + file_name + " for writing");
    std::ofstream file;
    file.exceptions(std::ofstream::failbit | std::ofstream::badbit);
    file.open(file_name, std::fstream::out | std::fstream::binary | std::fstream::trunc);

    // Read in file from websocket in chunks
    const std::size_t max_chunk_size = 4096;
    std::array<char, max_chunk_size> chunk_buffer;
    boost::crc_32_type file_csc;
    Logger::debug("Begin reading chunks from stream");
    do {
        // Read chunk
        auto chunk_size = stream.read_some(asio::buffer(chunk_buffer));
        // Write chunk to disk
        file.write(chunk_buffer.data(), chunk_size);
        // Checksum chunk
        file_csc.process_bytes(chunk_buffer.data(), chunk_size);
    } while (!stream.is_message_done());
    file.close();

    Logger::debug("Done reading chunks and closing " + file_name);

    Logger::debug("Read remote checksum and verify with local checksum");
    auto local_checksum = std::to_string(file_csc.checksum());
    std::string remote_checksum;
    auto remote_checksum_buffer = boost::asio::dynamic_buffer(remote_checksum);
    stream.read(remote_checksum_buffer);
    if (local_checksum != remote_checksum) {
        throw std::runtime_error(file_name + " checksums do not match");
    }
    Logger::debug(file_name + " successfully read");
}

void write_file(websocket::stream<tcp::socket> &stream,
                const std::string &file_name) {
    Logger::debug("Opening " + file_name + " for reading");

    std::ifstream container;
    container.exceptions(std::ofstream::failbit | std::ofstream::badbit);
    container.open(file_name, std::fstream::in | std::fstream::binary);
    if(container.fail()) {
        throw std::runtime_error("Failed to open file for reading: " + file_name);
    }
    const auto file_size = bfs::file_size(file_name);

    // Write the file in chunks to the client
    auto bytes_remaining = file_size;
    const std::size_t max_chunk_size = 4096;
    std::array<char, max_chunk_size> chunk_buffer;
    boost::crc_32_type container_csc;
    bool fin = false;
    Logger::debug("Begin writing chunks to stream");
    do {
        auto chunk_size = std::min(bytes_remaining, max_chunk_size);
        container.read(chunk_buffer.data(), chunk_size);
        container_csc.process_bytes(chunk_buffer.data(), chunk_size);
        bytes_remaining -= chunk_size;
        if (bytes_remaining == 0)
            fin = true;
        stream.write_some(fin, asio::buffer(chunk_buffer.data(), chunk_size));
    } while (!fin);
    container.close();
    Logger::debug("Done writing chunks and closing " + file_name);

    // Send the checksum to the client
    auto container_checksum = asio::buffer(std::to_string(container_csc.checksum()));
    stream.write(container_checksum);
    Logger::info(file_name + " successfully written");
}

void parse_arguments(ClientData &client_data, int argc, char **argv) {
    Logger::debug("Parsing executable arguments");

    namespace po = boost::program_options;

    // Supported arguments
    po::options_description desc("Usage: container_builder [options] container.img container.def");
    desc.add_options()
            ("help", "produce help message")
            ("debug", po::bool_switch(), "enable debug information")
            ("force", po::bool_switch(), "force the build, overwriting any existing image file")
            ("transfer_context", po::bool_switch(), "transfer the contents of the current directory to the build host")
            ("arch", po::value<std::string>()->default_value("x86_64"),
             "select architecture, valid options are x86_64 and ppc64le(Currently this does nothing)")
            ("backend", po::value<std::string>()->default_value("unspecified"),
             "Override the auto-selected builder backend to use, valid options are singularity and docker")
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
        early_exit();
    }

    // Enable debug information
    if (vm["debug"].as<bool>()) {
        Logger::set_priority(LogPriority::debug);
        Logger::debug("Debug logging enabled");
    }

    // Set client data based upon program arguments
    client_data.force = vm["force"].as<bool>();
    client_data.definition_path = vm["definition"].as<std::string>();
    client_data.container_path = vm["container"].as<std::string>();
    client_data.tty = vm["tty"].as<bool>();
    client_data.arch = Arch::to_arch(vm["arch"].as<std::string>());
    client_data.backend = Backend::to_backend(vm["backend"].as<std::string>());
    client_data.log_priority = Logger::get_priority();
    client_data.transfer_context = vm["transfer_context"].as<bool>();

    // Make sure variables are set as required
    po::notify(vm);
}

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

BuilderData get_builder(websocket::stream<tcp::socket> &queue_stream) {
    Logger::debug("Writing builder request string");
    std::string request_string("checkout_builder_request");
    queue_stream.write(asio::buffer(request_string));

    Logger::debug("Read serialized builder data");
    std::string builder_data_string;
    auto builder_data_buffer = boost::asio::dynamic_buffer(builder_data_string);
    queue_stream.read(builder_data_buffer);

    Logger::debug("Deserialize builder data string");
    BuilderData builder_data;
    std::istringstream archive_stream(builder_data_string);
    boost::archive::text_iarchive archive(archive_stream);
    archive >> builder_data;

    return builder_data;
}

void stream_build(websocket::stream<tcp::socket> &builder_stream) {
    Logger::info("Beginning to stream build");
    const auto max_read_bytes = 4096;
    std::array<char, max_read_bytes> buffer;
    beast::error_code error;
    do {
        auto bytes_read = builder_stream.read_some(asio::buffer(buffer), error);
        std::cout.write(buffer.data(), bytes_read);
    } while (!builder_stream.is_message_done());
    Logger::info("Finished streaming build");

    Logger::debug("Checking exit status of build");
    std::string exit_code_string;
    auto exit_code_buffer = boost::asio::dynamic_buffer(exit_code_string);
    builder_stream.read(exit_code_buffer);
    Logger::debug("Build exit code: " + exit_code_string);
    if (exit_code_string != "0") {
        throw std::runtime_error("Build failed with exit code " + exit_code_string);
    }
}

bool is_docker_recipe(std::string file_path) {
    // Read file into string buffer
    std::ifstream t(file_path);
    std::stringstream file_buffer;
    file_buffer << t.rdbuf();

    // Search string buffer for line beginning with "FROM "
    std::regex docker_regex("^(FROM )");

    // Check for a match
    return std::regex_search(file_buffer.str(), docker_regex);
}

bool is_singularity_recipe(std::string file_path) {
    // Read file into string buffer
    std::ifstream t(file_path);
    std::stringstream file_buffer;
    file_buffer << t.rdbuf();

    // Search string buffer for line beginning with "FROM "
    std::regex docker_regex("(From: )");

    // Check for a match
    return std::regex_search(file_buffer.str(), docker_regex);
}

// Simple checks image and definition files provided on the command line
void check_input_files(ClientData &client_data) {

    // Check that definition exists
    if (!bfs::exists(client_data.definition_path)) {
        throw std::runtime_error(client_data.definition_path + " Does not exists");
    }

    // Detect the backend to use if not specified
    if (client_data.backend == BackendType::unspecified) {

        if (is_docker_recipe(client_data.definition_path)) {
            client_data.backend = BackendType::docker;
            Logger::success("Detected dockerfile");
        } else if (is_singularity_recipe(client_data.definition_path)) {
            client_data.backend = BackendType::singularity;
            Logger::success("Detected singularity file");
        } else {
            throw std::runtime_error("Unable to detect type of " + client_data.definition_path);
        }

    }

    // Check if image already exists
    if (boost::filesystem::exists(client_data.container_path)) {
        if (client_data.force) {
            Logger::info("Forcing build: image file " + client_data.container_path + " will be overwritten");
        } else {
            throw std::runtime_error(client_data.container_path + " : file already exists. Add --force flag to override");
        }
    }
}

// Write build context to builder
void write_context(websocket::stream<tcp::socket> &builder_stream, const ClientData &client_data) {
    // Create unique directory
    std::string temp_path = bfs::unique_path(bfs::temp_directory_path().string() + "/%%%%-%%%%-%%%%").string();
    std::string context_temp_path = temp_path + "/cb-context";
    bfs::create_directories(context_temp_path);
    Logger::debug("Created temp context directory: " + context_temp_path);

    // If requested copy current context directory to context tmp
    if(client_data.transfer_context) {
        int cp_rc = bp::system("cp -r . " + context_temp_path);
        if(cp_rc != 0) {
            throw std::runtime_error("Error copying build context");
        }
        Logger::debug("Copied full context");
    }

    // Copy definition file, renaming it to container.def
    bfs::copy_file(client_data.definition_path, context_temp_path + "/container.def");

    // Tar context tmp directory to cb-context.tar.gz
    std::string original_pwd = bfs::current_path().string();
    bfs::current_path(temp_path);
    int tar_rc = bp::system("tar zcvf cb-context.tar.gz cb-context",  bp::std_out > bp::null,  bp::std_err > bp::null);
    if(tar_rc != 0) {
        throw std::runtime_error("Error creating build context tarball");
    }
    bfs::current_path(original_pwd);
    Logger::debug("Created tarball of context directory");

    // Transfer context tar
    write_file(builder_stream, temp_path + "/cb-context.tar.gz");
    Logger::debug("Wrote context tarball");

    // Remove context tmp
    bfs::remove_all(temp_path);
    Logger::debug("Removing temp directory: " + temp_path);
}

int main(int argc, char *argv[]) {
    // Remove buffering from cout
    std::cout.setf(std::ios::unitbuf);

    // Catch ctrl-c and restore cursor
    std::signal(SIGINT, [](int signal) {
        std::cout << "\nUser requested exit\n";
        early_exit();
    });

    hide_cursor();

    asio::io_context io_context;
    websocket::stream<tcp::socket> queue_stream(io_context);
    websocket::stream<tcp::socket> builder_stream(io_context);

    try {
        ClientData client_data;
        parse_environment(client_data);
        parse_arguments(client_data, argc, argv);
        check_input_files(client_data);

        WaitingAnimation wait_queue("Connecting to BuilderQueue");
        // Open a WebSocket stream to the queue
        tcp::resolver queue_resolver(io_context);
        asio::connect(queue_stream.next_layer(), queue_resolver.resolve({client_data.queue_host, "8080"}));
        queue_stream.handshake(client_data.queue_host + ":8080", "/");
        wait_queue.stop_success("Connected to queue: " + client_data.queue_host);

        // Request a build host from the queue
        WaitingAnimation wait_builder("Requesting remote builder");
        auto builder_data = get_builder(queue_stream);

        // Open a WebSocket stream to the builder
        tcp::resolver builder_resolver(io_context);
        asio::connect(builder_stream.next_layer(), builder_resolver.resolve({builder_data.host, "8080"}));
        builder_stream.handshake(builder_data.host + ":8080", "/");
        wait_builder.stop_success("Connected to remote builder: " + builder_data.host);

        Logger::debug(
                "Setting the builder websocket stream to handle binary data and have an unlimited(uint64_t) message size");
        builder_stream.binary(true);
        builder_stream.read_message_max(0);

        // Write client data to builder
        write_client_data(builder_stream, client_data);

        // Write build context/recipe
        WaitingAnimation wait_context("Transferring build context to builder");
        write_context(builder_stream, client_data);
        wait_context.stop_success("Completed");

        // stream builder output
        stream_build(builder_stream);

        // Read container from builder
        WaitingAnimation wait_image("Transferring " + client_data.container_path);
        read_file(builder_stream, client_data.container_path);
        wait_image.stop_success("Completed");

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
        Logger::debug("Attempting normal close of builder");
        builder_stream.close(websocket::close_code::normal);
    } catch (...) {
        Logger::debug("Failed to cleanly close the WebSocket to builder");
    }

    try {
        Logger::debug("Attempting normal close of queue");
        queue_stream.close(websocket::close_code::normal);
    } catch (...) {
        Logger::debug("Failed to cleanly close the WebSocket to queue");
    }

    show_cursor();

    return 0;
}