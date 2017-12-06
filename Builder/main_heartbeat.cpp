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
        // Block until the initial connection is made
        asio::io_service io_service;
        tcp::acceptor acceptor(io_service, tcp::endpoint(tcp::v4(), 8080));
        std::shared_ptr<tcp::socket> socket = std::make_shared<tcp::socket>(io_service);
        acceptor.accept(*socket);

        std::shared_ptr<Messenger> messenger = std::make_shared<Messenger>(*socket);

        // Once we're connected start the build process in a coroutine
        asio::spawn(io_service,
                    [&](asio::yield_context yield) {
                        // Receive the definition file from the client
                        messenger->async_receive_file("container.def", yield);

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
                            logger::write("subprocess error: " + build_ec.message());
                        }

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
                            read_size = asio::async_read_until(std_pipe, buffer, line_matcher, yield);
                            // Write the buffer to our socket
                            // TODO, test if we receive a cancel, if so resend the buffer
                            messenger->async_send(buffer, yield);
                        } while (read_size > 0);

                        // Get the return value from the build subprocess
                        build_child.wait();
                        int build_code = build_child.exit_code();

                        logger::write("Sending image to client");

                        // Send the container to the client
                        if (build_code == 0) {
                            logger::write("Image failed to build");
                            messenger->async_send_file("container.img", yield);
                        };
                    });

        // Start the heartbeat which will let the client know we're alive every 15 seconds
        // The heartbeat is guarded by a watchdog that if triggered will cause the connection to be restarted
        asio::deadline_timer heartbeat(io_service);
        asio::deadline_timer watchdog(io_service);
        const auto pulse = boost::posix_time::seconds(15);
        const auto timeout = boost::posix_time::seconds(10);

        // When a hang is detected we destroy the socket and attempt to create a new one
        // This is handled outside of the heartbeat coroutine as if it fires that coroutine will be jammed up
        timer_callback_t heartbeat_hung = [&](const boost::system::error_code& ec) {
            if (ec == boost::asio::error::operation_aborted) {
                return;
            }
            // Destroy the current socket
            socket->cancel();
            socket->close();

            // Accept a new connection and reset the socket and messenger
            socket = std::make_shared<tcp::socket>(io_service);
            acceptor.accept(*socket);
            messenger = std::make_shared<Messenger>(*socket);
        };

        asio::spawn(io_service,
                    [&](asio::yield_context yield) {
                        for(;;) {
                            // arm the watchdog
                            watchdog.expires_from_now(timeout);
                            watchdog.async_wait(heartbeat_hung);

                            // Try to send our heartbeat
                            messenger->async_send_heartbeat(yield);

                            // Set the next heartbeat
                            heartbeat.expires_from_now(pulse);
                            // Wait until the next heartbeat
                            heartbeat.async_wait(yield);

                            // stand down watchdog
                            watchdog.cancel();
                        }
                    });

        // Begin processing our connections and queue
        io_service.run();
    }
    catch (std::exception &e) {
        logger::write(std::string() + "Build server exception: " + e.what());
    }

    logger::write("Server shutting down");

    return 0;
}