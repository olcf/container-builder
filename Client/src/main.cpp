#include <fstream>
#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/exception/all.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/crc.hpp>
#include <boost/program_options.hpp>
#include "ClientData.h"
#include "BuilderData.h"
#include "WaitingAnimation.h"
#include "pwd.h"

namespace asio = boost::asio;
using asio::ip::tcp;
namespace beast = boost::beast;
namespace websocket = beast::websocket;

void write_client_data(websocket::stream<tcp::socket>& builder_stream, ClientData client_data) {
    // Serialize client data to string
    std::ostringstream archive_stream;
    boost::archive::text_oarchive archive(archive_stream);
    archive << client_data;
    auto serialized_client_data = archive_stream.str();

    // Write string to serialized client data
    builder_stream.write(asio::buffer(serialized_client_data));
}

void read_file(websocket::stream<tcp::socket>& stream,
               const std::string& file_name) {
    // Open the file
    std::ofstream file;
    file.exceptions ( std::ofstream::failbit | std::ofstream::badbit );
    file.open(file_name, std::fstream::out | std::fstream::binary | std::fstream::trunc);

    // Read in file from websocket in chunks
    const std::size_t max_chunk_size = 4096;
    std::array<char, max_chunk_size> chunk_buffer;
    boost::crc_32_type file_csc;
    do {
        // Read chunk
        auto chunk_size = stream.read_some(asio::buffer(chunk_buffer));
        // Write chunk to disk
        file.write(chunk_buffer.data(), chunk_size);
        // Checksum chunk
        file_csc.process_bytes(chunk_buffer.data(), chunk_size);
    } while (!stream.is_message_done());
    file.close();

    // Read remote checksum and verify with local checksum
    auto local_checksum = std::to_string(file_csc.checksum());
    std::string remote_checksum;
    auto remote_checksum_buffer = boost::asio::dynamic_buffer(remote_checksum);
    stream.read(remote_checksum_buffer);
    if (local_checksum != remote_checksum) {
        throw std::runtime_error(file_name + " checksums do not match");
    }
}

void write_file(websocket::stream<tcp::socket>& stream,
                const std::string& file_name) {
    std::ifstream container;
    container.exceptions ( std::ofstream::failbit | std::ofstream::badbit );
    container.open("container.img", std::fstream::in | std::fstream::binary);
    const auto file_size = boost::filesystem::file_size("container.img");

    // Write the file in chunks to the client
    auto bytes_remaining = file_size;
    const std::size_t max_chunk_size = 4096;
    std::array<char, max_chunk_size> chunk_buffer;
    boost::crc_32_type container_csc;
    bool fin = false;
    do {
        auto chunk_size = std::min(bytes_remaining, max_chunk_size);
        container.read(chunk_buffer.data(), chunk_size);
        container_csc.process_bytes(chunk_buffer.data(), chunk_size);
        bytes_remaining -= chunk_size;
        if(bytes_remaining == 0)
            fin = true;
        stream.write_some(fin, asio::buffer(chunk_buffer.data(), chunk_size));
    } while (!fin);
    container.close();

    // Send the checksum to the client
    auto container_checksum = asio::buffer(std::to_string(container_csc.checksum()));
    stream.write(container_checksum);
}

void parse_arguments(ClientData &client_data, int argc, char **argv) {
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
    client_data.definition_path = vm["definition"].as<std::string>();
    client_data.container_path = vm["container"].as<std::string>();
    client_data.tty = vm["tty"].as<bool>();
    client_data.arch = Arch::to_arch(vm["arch"].as<std::string>());

    // Make sure variables are set as required
    po::notify(vm);
}

void parse_environment(ClientData &client_data) {
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

BuilderData get_builder(websocket::stream<tcp::socket>& queue_stream) {
    // Write builder request string to the queue
    std::string request_string("checkout_builder_request");
    queue_stream.write(asio::buffer(request_string));

    // Read serialized builder data
    std::string builder_data_string;
    auto builder_data_buffer = boost::asio::dynamic_buffer(builder_data_string);
    queue_stream.read(builder_data_buffer);

    // Deserialize builder data string
    BuilderData builder_data;
    std::istringstream archive_stream(builder_data_string);
    boost::archive::text_iarchive archive(archive_stream);
    archive >> builder_data;

    return builder_data;
}

void stream_build(websocket::stream<tcp::socket>& builder_stream) {
    const auto max_read_bytes = 4096;
    std::array<char, max_read_bytes> buffer;
    beast::error_code error;
    do {
        auto bytes_read = builder_stream.read_some(asio::buffer(buffer), error);
        std::cout.write(buffer.data(), bytes_read);
    } while (!builder_stream.is_message_done());
}

int main(int argc, char *argv[]) {
    asio::io_context io_context;
    websocket::stream<tcp::socket> queue_stream(io_context);
    websocket::stream<tcp::socket> builder_stream(io_context);

    try {
        ClientData client_data;
        parse_environment(client_data);
        parse_arguments(client_data, argc, argv);

        WaitingAnimation wait_queue("Connecting to BuilderQueue: ");
        // Open a WebSocket stream to the queue
        tcp::resolver queue_resolver(io_context);
        asio::connect(queue_stream.next_layer(), queue_resolver.resolve({client_data.queue_host, "8080"}));
        queue_stream.handshake(client_data.queue_host + ":8080", "/");
        wait_queue.stop("Connected to queue: " + client_data.queue_host, logger::severity_level::success);

        WaitingAnimation wait_builder("Requesting remote builder: ");
        // Request a build host from the queue
        auto builder_data = get_builder(queue_stream);
        // Open a WebSocket stream to the builder
        tcp::resolver builder_resolver(io_context);
        asio::connect(builder_stream.next_layer(), builder_resolver.resolve({builder_data.host, "8080"}));
        builder_stream.handshake(builder_data.host + ":8080", "/");
        wait_builder.stop("Connected to remote builder: " + builder_data.host, logger::severity_level::success);

        // Write client data to builder
        write_client_data(builder_stream, client_data);

        // Write definition to builder
        write_file(builder_stream, client_data.definition_path);

        // stream builder output
        stream_build(builder_stream);

        // Read container from builder
        read_file(builder_stream, client_data.container_path);


    } catch (const boost::exception &ex) {
        auto diagnostics = diagnostic_information(ex);
        logger::write(std::string() + "Container Builder exception encountered: " + diagnostics, logger::severity_level::fatal);
    } catch (const std::exception &ex) {
        logger::write(std::string() + "Container Builder exception encountered: " + ex.what(), logger::severity_level::fatal);
    } catch (...) {
        logger::write("Unknown exception caught!", logger::severity_level::fatal);
    }

    // Disconnect from builder and queue
    builder_stream.close(websocket::close_code::normal);
    queue_stream.close(websocket::close_code::normal);

    return 0;
}