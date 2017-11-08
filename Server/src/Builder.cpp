#include "Builder.h"
#include <iostream>
#include "ReservationRequest.h"
#include <boost/asio/write.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/streambuf.hpp>
#include "FileWriter.h"
#include "FileReader.h"
#include <boost/regex.hpp>
#include <boost/process.hpp>

void Builder::singularity_build() {

    // Wait in the queue for a reservation to begin
    ReservationRequest reservation(socket.get_io_service(), queue);
    auto resource = reservation.async_wait(yield);

    // Copy the definition file from the client
    receive_definition();

    // Build the container and stream the output back to the client
    build_container();

    // Copy container to client
    send_image();
}

void Builder::receive_definition() {
    // Copy definition file to server
    std::string definition_file(build_directory);
    definition_file += "/container.def";
    FileReader definition(definition_file);
    definition.async_read(socket, yield);
}

void Builder::build_container() {
    // Start subprocess work
    boost::process::async_pipe pipe(socket.get_io_service());

    boost::process::child cv("/usr/bin/printenv", (boost::process::std_out & boost::process::std_err) > pipe);

    // Read process pipe output and write it to the client
    // EOF will be returned as an error code when the pipe is closed...I think
    // line buffer by reading from the pipe until we hit \n or \r
    boost::system::error_code ec;
    boost::asio::streambuf buffer;
    uint64_t bytes_read;
    boost::regex line_matcher{"\\r|\\n"};
    do {
        bytes_read = asio::async_read_until(pipe, buffer, line_matcher, yield[ec]);
        // TODO just prepend size to buffer(?)
        // Send the header indicating the number of bytes in the send message
        asio::async_write(socket, asio::buffer(&bytes_read, sizeof(uint64_t)), yield);

        // Send the message
        asio::async_write(socket, buffer, asio::transfer_exactly(bytes_read), yield);
        if (ec && ec != asio::error::eof) {
            std::cout << "some sort of error\n";
        }
    } while (bytes_read != 0);

    // Send termination to client so it knows we're streaming output done
    size_t end_header(0);
    asio::async_write(socket, asio::buffer(&end_header, sizeof(uint64_t)), yield);
}

void Builder::send_image() {
    std::string container_file(build_directory);
    container_file += "/container.img";
    FileWriter container(container_file);
    container.async_write(socket, yield);
}