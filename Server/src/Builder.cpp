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
#include "SingularityBackend.h"
#include "WriteMessage.h"
#include <limits>
#include "Logger.h"

void Builder::singularity_build() {

    // Wait in the queue for a reservation to begin
    ReservationRequest reservation(socket, queue);
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
    logger::write(socket, "Receiving definition");
    ReadFile definition(definition_filename());
    definition.async_read(socket, yield);
}

void Builder::build_container() {
    // Start subprocess work
    boost::process::async_pipe std_pipe(socket.get_io_service());

    // Start the build process, stdout/err will be passed to std_pipe for reading
    SingularityBackend backend(socket, resource, std_pipe, definition_filename());
    backend.build_singularity_container();

    // Read process pipe output and write it to the client
    // EOF will be returned as an error code when the pipe is closed...I think
    // line buffer(ish) by reading from the pipe until we hit \n, \r
    // NOTE: read_until will fill buffer until line_matcher is satisfied but generally will contain additional data.
    // This is fine as all we care about is dumping everything from std_pipe to our buffer and don't require exact line buffering
    boost::system::error_code ec;
    asio::streambuf buffer;
    uint64_t pipe_bytes_read;
    boost::regex line_matcher{"\\r|\\n"};

    logger::write(socket, "Sending build output");

    do {
        pipe_bytes_read = asio::async_read_until(std_pipe, buffer, line_matcher, yield[ec]);
        if(ec && ec != asio::error::eof) {
            logger::write(socket, "Error: build output pipe read error: " + ec.message());
        }

        // Write the output grabbed from std_pipe
        messenger.async_send(buffer, yield[ec]);
        if(ec && ec != asio::error::eof) {
            logger::write(socket, "Error: build output pipe send error: " + ec.message());
        }
    } while (pipe_bytes_read > 0);

    logger::write(socket, "Finished build output");
}

void Builder::send_image() {
    logger::write(socket, "Sending image");

    std::string container_file(build_directory);
    container_file += "/container.img";
    WriteFile container(container_file);
    container.async_write(socket, yield);

    logger::write(socket, "Finish sending image");
}