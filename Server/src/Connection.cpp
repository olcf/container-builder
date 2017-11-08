#include "Connection.h"
#include "iostream"
#include <boost/asio/write.hpp>
#include "ReservationRequest.h"
#include <boost/asio/read_until.hpp>
#include <boost/asio/streambuf.hpp>
#include "Resource.h"
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/process.hpp>
#include "FileWriter.h"
#include "FileReader.h"

void Connection::begin() {
    auto self(shared_from_this());
    asio::spawn(socket.get_io_service(),
                [this, self](asio::yield_context yield) {
                    try {

                        // Read initial request type from client
                        auto reserve_message = async_read_line(socket, yield);
                        if (reserve_message == "build_request")
                            handle_build_request(yield);
                        else if (reserve_message == "diagnostic_request")
                            handle_diagnostic_request(yield);
                        else
                            throw std::system_error(EPERM, std::system_category());
                    }
                    catch (std::exception &e) {
                        std::cout << "Exception: " << e.what() << std::endl;
                    }
                });
}

// Handle server side logic to handle a request to build a new container
// TODO create builder class to handle cleaning up files on (ab)normal exit
void Connection::handle_build_request(asio::yield_context yield) {
    // Wait in the queue for a reservation to begin
    ReservationRequest reservation(socket.get_io_service(), queue);
    auto resource = reservation.async_wait(yield);

    // Create build directory consisting of remote endpoint and local enpoint
    // This should uniquely identify the connection as it includes the IP and port
    std::cout<<"not setting build directory\n";
    std::string build_dir("./");
    build_dir +=  boost::lexical_cast<std::string>(socket.remote_endpoint())
            += std::string("_")
            += boost::lexical_cast<std::string>(socket.local_endpoint());
    boost::filesystem::create_directory(build_dir);

    // Copy definition file to server
    std::string definition_file(build_dir);
    definition_file += "/container.def";
    FileReader definition(definition_file);
    definition.async_read(socket, yield);

    // Build the container


    // Start subprocess work
    boost::process::async_pipe pipe(socket.get_io_service());
    boost::process::child c("/usr/bin/printenv", (boost::process::std_out & boost::process::std_err) > pipe);

    // Read process pipe output and write it to the client
    // EOF will be returned as an error code when the pipe is closed...I think
    // This can also be line buffered by using async_read_until('\n')
    // TODO an extra write for each buffer is probably not good, explicit length buffers could fix this
    boost::system::error_code ec;
    boost::asio::streambuf buffer;
    uint64_t bytes_read;
    while( bytes_read = boost::asio::async_read(pipe, buffer, yield[ec]) ) {
        // Send the header indicating the number of bytes in the send message
        asio::async_write(socket, asio::buffer(&bytes_read, sizeof(uint64_t)), yield);
        asio::async_write(socket, buffer, asio::transfer_exactly(bytes_read), yield);
        if(ec && ec != asio::error::eof) {
            std::cout<< "some sort of error\n";
        }
    }

    // Send termination to client so it knows we're streaming output done
    asio::async_write(socket, asio::buffer('\0', 1), yield);

    // Copy container to client
    std::string container_file(build_dir);
    container_file += "/container.img";
    FileWriter container(container_file);
    container.async_write(socket, yield);

    // Remove the directory
}

void Connection::handle_diagnostic_request(asio::yield_context yield) {
    std::cout << "queue stuff here...\n";
}

// Async read a line message into a string
std::string async_read_line(tcp::socket &socket, asio::yield_context &yield) {
    asio::streambuf reserve_buffer;
    asio::async_read_until(socket, reserve_buffer, '\n', yield);
    std::istream reserve_stream(&reserve_buffer);
    std::string reserve_string;
    std::getline(reserve_stream, reserve_string);
    return reserve_string;
}