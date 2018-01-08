#include <fstream>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/core/buffers_to_string.hpp>
#include <boost/exception/all.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/crc.hpp>
#include <boost/process/child.hpp>
#include <boost/process/async_pipe.hpp>
#include <boost/process/group.hpp>
#include <boost/process/io.hpp>
#include "ClientData.h"
#include "Logger.h"

namespace asio = boost::asio;
using asio::ip::tcp;
namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace bp = boost::process;

using callback_type = std::function<void(const boost::system::error_code&, std::size_t size)>;

ClientData read_client_data(websocket::stream<tcp::socket>& client_stream) {
    Logger::info("Reading client data");

    Logger::info("Reading serialized client data");
    std::string client_data_string;
    auto client_data_buffer = asio::dynamic_buffer(client_data_string);
    client_stream.read(client_data_buffer);

    Logger::info("Deserializing client data string");
    ClientData client_data;
    std::istringstream archive_stream(client_data_string);
    boost::archive::text_iarchive archive(archive_stream);
    archive >> client_data;

    Logger::success("Read client data");
    return client_data;
}

void read_file(websocket::stream<tcp::socket>& client_stream,
               const std::string& file_name) {
    Logger::info("Opening " + file_name + " for writing");
    std::ofstream file;
    file.exceptions ( std::ofstream::failbit | std::ofstream::badbit );
    file.open(file_name, std::fstream::out | std::fstream::binary | std::fstream::trunc);

    // Read in file from websocket in chunks
    const std::size_t max_chunk_size = 4096;
    std::array<char, max_chunk_size> chunk_buffer;
    boost::crc_32_type file_csc;
    Logger::info("Begin reading chunks from stream");
    do {
        // Read chunk
        auto chunk_size = client_stream.read_some(asio::buffer(chunk_buffer));
        // Write chunk to disk
        file.write(chunk_buffer.data(), chunk_size);
        // Checksum chunk
        file_csc.process_bytes(chunk_buffer.data(), chunk_size);
    } while (!client_stream.is_message_done());
    file.close();

    Logger::info("Done reading chunks and closing " + file_name);

    Logger::info("Read remote checksum and verify with local checksum");
    auto local_checksum = std::to_string(file_csc.checksum());
    std::string remote_checksum;
    auto remote_checksum_buffer = boost::asio::dynamic_buffer(remote_checksum);
    client_stream.read(remote_checksum_buffer);
    if (local_checksum != remote_checksum) {
        throw std::runtime_error(file_name + " checksums do not match");
    }
    Logger::info(file_name + " successfully read file");
}

std::string build_command(const ClientData& client_data) {
    if (client_data.arch == Architecture::ppc64le) {
        // A dirty hack but the ppc64le qemu executable must be in the place the kernel expects it
        // Modify the definition to copy this executable in during %setup
        // NOTE: singularity currently doesn't have a way to inject this file in before bootstrap
        //       and so DockerHub or local singularity files are the only supported ppc64le bootstrapper
        std::ofstream def{"container.def", std::ios::app};
        std::string copy_qemu(
                "\n%setup\ncp /usr/bin/qemu-ppc64le  ${SINGULARITY_ROOTFS}/usr/bin/qemu-ppc64le");
        def << copy_qemu;
    }
    std::string build_command("/usr/bin/sudo ");
    // If the client is called from a TTY we use "unbuffer" to fake the build into thinking it has a real TTY
    // This allows utilities like wget and color ls to work nicely
    if (client_data.tty) {
        build_command += "/usr/bin/unbuffer ";
    }
    build_command += "/usr/local/bin/singularity build ./container.img ./container.def";

    Logger::info("Build command prepared: " + build_command);
    return build_command;
}

void stream_build(websocket::stream<tcp::socket>& client_stream,
                            const std::string& build_command) {

    Logger::info("Starting subprocess and redirecting output to pipe");
    auto& io_context = client_stream.get_executor().context();
    bp::async_pipe std_pipe{io_context};
    bp::group group;
    bp::child build_child(build_command, bp::std_in.close(), (bp::std_out & bp::std_err) > std_pipe, group);

    // Write the pipe output to the websocket as it becomes available
    // boost process doesn't fully support istream interface as the documentation implies so async interface is used
    const auto max_read_bytes = 4096;
    std::array<char, max_read_bytes> std_buffer;

    callback_type stream_output = [&] (const boost::system::error_code& error,
                                       std::size_t bytes_read) {
        if(error == asio::error::eof) {
            client_stream.write_some(true, asio::buffer(std_buffer.data(), bytes_read));
            return;
        } else {
            client_stream.write_some(false, asio::buffer(std_buffer.data(), bytes_read));
            std_pipe.async_read_some(asio::buffer(std_buffer), stream_output);
        }
    };
    std_pipe.async_read_some(asio::buffer(std_buffer), stream_output);
    Logger::info("Running async pipe read of subprocess output");
    io_context.run();

    // Wait for the process to complete and grab exit code
    build_child.wait();

    // Send exit code to client, if non zero end the connection
    int exit_code = build_child.exit_code();
    auto exit_code_string = std::to_string(exit_code);
    client_stream.write(asio::buffer(exit_code_string));

    if(exit_code != 0) {
        throw std::runtime_error("Container build failed");
    }

    Logger::info("Done streaming subprocess output");
}

void write_file(websocket::stream<tcp::socket>& client_stream,
                const std::string& file_name) {
    Logger::info("Opening " + file_name + " for reading");

    std::ifstream container;
    container.exceptions ( std::ofstream::failbit | std::ofstream::badbit );
    container.open(file_name, std::fstream::in | std::fstream::binary);
    const auto file_size = boost::filesystem::file_size(file_name);

    // Write the file in chunks to the client
    auto bytes_remaining = file_size;
    const std::size_t max_chunk_size = 4096;
    std::array<char, max_chunk_size> chunk_buffer;
    boost::crc_32_type container_csc;
    bool fin = false;
    Logger::info("Begin writing chunks to stream");
    do {
        auto chunk_size = std::min(bytes_remaining, max_chunk_size);
        container.read(chunk_buffer.data(), chunk_size);
        container_csc.process_bytes(chunk_buffer.data(), chunk_size);
        bytes_remaining -= chunk_size;
        if(bytes_remaining == 0)
            fin = true;
        client_stream.write_some(fin, asio::buffer(chunk_buffer.data(), chunk_size));
    } while (!fin);
    container.close();
    Logger::info("Done writing chunks and closing " + file_name);

    // Send the checksum to the client
    auto container_checksum = asio::buffer(std::to_string(container_csc.checksum()));
    client_stream.write(container_checksum);
    Logger::info(file_name + " successfully written");
}

// A blocking I/O container building websocket server
int main(int argc, char *argv[]) {
    boost::ignore_unused(argc, argv);

    asio::io_context io_context;
    websocket::stream<tcp::socket> client_stream(io_context);
    tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), 8080));

    try {
        Logger::info("Accepting an in coming websocket connection");
        acceptor.accept(client_stream.next_layer());
        client_stream.accept();

        Logger::info("Setting the websocket stream to handle binary data and have an unlimited(uint64_t) message size");
        client_stream.binary(true);
        client_stream.read_message_max(0);

        Logger::info("Reading basic client data that can influence build");
        auto client_data = read_client_data(client_stream);

        Logger::info("Reading client definition file");
        read_file(client_stream, "container.def");

        Logger::info("Creating build command based upon the client data");
        auto build_string = build_command(client_data);

        Logger::info("Launch build process and stream the output to the client");
        stream_build(client_stream, build_string);

        Logger::info("Writing the finished container to the client");
        write_file(client_stream, "container.img");

    } catch (const boost::exception &ex) {
        auto diagnostics = diagnostic_information(ex);
        Logger::error(std::string() + "Builder exception encountered: " + diagnostics);
    } catch (const std::exception &ex) {
        Logger::error(std::string() + "Builder exception encountered: " + ex.what());
    } catch (...) {
        Logger::error("Unknown exception caught!");
    }

    // Close connection
    client_stream.close(websocket::close_code::normal);

    return 0;
}