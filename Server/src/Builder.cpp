#include "Builder.h"
#include <iostream>
#include <limits>
#include "ReservationRequest.h"
#include <boost/asio/write.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/streambuf.hpp>
#include "WriteFile.h"
#include "ReadFile.h"
#include <boost/regex.hpp>
#include <boost/process.hpp>
#include "DockerBackend.h"
#include "WriteMessage.h"
#include <limits>
#include "Logger.h"

void Builder::singularity_build() {

    // Wait in the queue for a reservation to begin
    ReservationRequest reservation(socket.get_io_service(), queue);
    resource = reservation.async_wait(yield);

    // Copy the definition file from the client
    receive_definition();

    // Build the container and stream the output back to the client
    build_container();

    // Copy container to client
    send_image();
}

std::string Builder::definition_filename() {
    std::string def_filename(build_directory);
    def_filename += "/container.def";
    return def_filename;
}

// Copy definition file to server
void Builder::receive_definition() {
    ReadFile definition(definition_filename());
    definition.async_read(socket, yield);
}

void Builder::build_container() {
    // Start subprocess work
    boost::process::async_pipe std_pipe(socket.get_io_service());

    // Start the build process, stdout/err will be passed to std_pipe for reading
    DockerBackend docker(resource, std_pipe, definition_filename());
    docker.build_singularity_container();

    // Read process pipe output and write it to the client
    // EOF will be returned as an error code when the pipe is closed...I think
    // line buffer by reading from the pipe until we hit \n, \r, of the buffer max size
    boost::system::error_code ec;
    asio::streambuf buffer;
    uint64_t bytes_read;
    boost::regex line_matcher{"\\r|\\n"};

    do {
        bytes_read = asio::async_read_until(std_pipe, buffer, line_matcher, yield[ec]);

        // TODO figure out what to do with truncated messages

        // TODO prepend size to buffer for a single write?

        // Send the header indicating the number of bytes in the send message
        uint32_t bytes_to_send = static_cast<uint32_t>(bytes_read);
        asio::async_write(socket, asio::buffer(&bytes_to_send, sizeof(uint32_t)), yield);
        // Send the message
        asio::async_write(socket, buffer, asio::transfer_exactly(bytes_to_send), yield);
    } while (bytes_read > 0);
}

void Builder::send_image() {
    std::string container_file(build_directory);
    container_file += "/container.img";
    WriteFile container(container_file);
    container.async_write(socket, yield);
}