#include "Connection.h"
#include "Builder.h"
#include "ReadMessage.h"
#include <boost/asio/streambuf.hpp>

void Connection::begin() {
    auto self(shared_from_this());
    asio::spawn(socket.get_io_service(),
                [this, self](asio::yield_context yield) {
                    try {
                        // Read initial request type from client
                        std::array<char, 127> initial_buffer;
                        message::async_read<std::size_t>(socket, asio::buffer(initial_buffer), yield);
                        std::string initial_message(initial_buffer.data());

                        if (initial_message == "build_request")
                            handle_build_request(yield);
                        else if (initial_message == "diagnostic_request")
                            handle_diagnostic_request(yield);
                        else
                            throw std::system_error(EPERM, std::system_category(), initial_message);
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