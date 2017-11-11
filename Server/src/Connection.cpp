#include "Connection.h"
#include "iostream"
#include <boost/asio/read_until.hpp>
#include "Builder.h"
#include <boost/asio/streambuf.hpp>

// Async read a line message into a string
std::string async_read_line(tcp::socket &socket, asio::yield_context &yield) {
    asio::streambuf reserve_buffer;
    asio::async_read_until(socket, reserve_buffer, '\n', yield);
    std::istream reserve_stream(&reserve_buffer);
    std::string reserve_string;
    std::getline(reserve_stream, reserve_string);
    return reserve_string;
}

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
                            throw std::system_error(EPERM, std::system_category(), reserve_message);
                    }
                    catch (std::exception &e) {
                        std::string except("Exception: ");
                        except += e.what();
                        logger::write(socket, except);
                    }
                });
}

// Handle server side logic to handle a request to build a new container
// TODO create builder class to handle cleaning up files on (ab)normal exit
void Connection::handle_build_request(asio::yield_context yield) {
    Builder builder(socket, queue, yield);
    builder.singularity_build();
}

void Connection::handle_diagnostic_request(asio::yield_context yield) {
    std::cout << "queue stuff here...\n";
}