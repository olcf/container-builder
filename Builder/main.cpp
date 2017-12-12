#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/process.hpp>
#include <boost/regex.hpp>
#include <iostream>
#include "Logger.h"
#include "Messenger.h"
#include <memory>

namespace asio = boost::asio;
using asio::ip::tcp;
namespace bp = boost::process;

int main(int argc, char *argv[]) {

    try {
        // Block until the initial client connects
        asio::io_service io_service;
        tcp::acceptor acceptor(io_service, tcp::endpoint(tcp::v4(), 8080));
        tcp::socket socket(io_service);
        acceptor.accept(socket);

        logger::write(socket, "Client connected");

        Messenger messenger(socket);

        // Once we're connected start the build process
        asio::spawn(io_service,
                    [&](asio::yield_context yield) {
                        boost::system::error_code error;

                        // Receive the definition file from the client
                        messenger.async_receive_file("container.def", yield[error]);
                        if (error) {
                            logger::write("Error receiving definition file: " + error.message());
                            return;
                        }

                        logger::write(socket, "Received container.def");

                        // Create a pipe to communicate with our build subprocess
                        bp::async_pipe std_pipe(io_service);

                        // Launch our build as a subprocess
                        // We use "unbuffer" to fake the build into thinking it has a real TTY, which the command output eventually will
                        // This causes things like wget and color ls to work nicely
                        std::string build_command(
                                "/usr/bin/sudo /usr/bin/unbuffer /usr/local/bin/singularity build ./container.img ./container.def");

                        bp::group group;
                        std::error_code build_ec;
                        bp::child build_child(build_command, bp::std_in.close(), (bp::std_out & bp::std_err) > std_pipe,
                                              group, build_ec);
                        if (build_ec) {
                            logger::write(socket, "subprocess error: " + build_ec.message());
                            return;
                        }

                        logger::write(socket, "launched build process: " + build_command);

                        // Read process pipe output and write it to the client
                        // line buffer(ish) by reading from the pipe until we hit \n, \r
                        // NOTE: read_until will fill buffer until line_matcher is satisfied but generally will contain additional data.
                        // This is fine as all we care about is dumping everything from std_pipe to our buffer and don't require exact line buffering
                        // TODO: just call read_some perhaps?
                        asio::streambuf buffer;
                        boost::regex line_matcher{"\\r|\\n"};
                        std::size_t read_size = 0;
                        do {
                            // Read from the pipe into a buffer
                            read_size = asio::async_read_until(std_pipe, buffer, line_matcher, yield[error]);
                            if (error) {
                                logger::write(socket, "reading process pipe failed: " + error.message());
                            }
                            // Write the buffer to our socket
                            messenger.async_send(buffer, yield[error]);
                            if (error) {
                                logger::write(socket, "sending process pipe failed: " + error.message());
                            }
                        } while (read_size > 0 && !error);

                        // Get the return value from the build subprocess
                        logger::write(socket, "Waiting on build process to exit");
                        build_child.wait();
                        int build_code = build_child.exit_code();

                        // Send the container to the client
                        if (build_code == 0) {
                            logger::write(socket, "Build complete, sending container");
                            messenger.async_send_file("container.img", yield[error]);
                            if (error) {
                                logger::write(socket, "Sending file to client failed: " + error.message());
                            }
                        } else {
                            logger::write(socket, "Build failed, not sending container");
                        }
                    });

        // Start processing coroutine
        io_service.run();

    }
    catch (...) {
        logger::write("Unknown builder exception encountered");
    }

    logger::write("Builder shutting down");

    return 0;
}