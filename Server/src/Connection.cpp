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
    asio::streambuf buf;
    boost::process::child c("/usr/bin/nm", "a.out", (boost::process::std_out & boost::process::std_err) > buf, socket.get_io_service());
//        boost::asio::async_read(ap, boost::asio::buffer(buf), yield);

    c.wait();
    std::istream buf_stream(&buf);
    std::string buf_string;
    std::getline(buf_stream, buf_string);
    std::cout<<"shit: "<< buf_string<<std::endl;

//        asio::async_write(socket, buf, yield);

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