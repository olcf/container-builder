#pragma once

#include "Messenger.h"
#include "Logger.h"
#include "Messenger.h"
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/process.hpp>
#include <boost/regex.hpp>
#include <boost/filesystem.hpp>

namespace asio = boost::asio;
using asio::ip::tcp;
namespace bp = boost::process;

class Builder {
public:
    explicit Builder() {

        // Full build connection - this will not run until the io_service is started
        asio::spawn(io_service,
                    [&](asio::yield_context yield) {
                        boost::system::error_code error;
                        Messenger client(io_service, "8080", yield, error);
                        if(error) {
                            throw std::runtime_error("Error connecting to client: " + error.message());
                        }

                        // Receive client data
                        ClientData client_data = client.async_read_client_data(yield, error);
                        if (error) {
                            throw std::runtime_error("Error receiving client data: " + error.message());
                        }

                        // Receive the definition file from the client
                        client.async_read_file("container.def", yield, error);
                        if (error) {
                            throw std::runtime_error("Error receiving definition file: " + error.message());
                        }

                        logger::write("Received container.def");

                        if(client_data.arch == Architecture::ppc64le) {
                            // A dirty hack but the ppc64le qemu executable must be in the place the kernel expects it
                            // Modify the definition to copy this executable in during %setup
                            boost::filesystem::ofstream def{"container.def", std::ios::app};
                            std::string copy_qemu("\n%setup\ncp /usr/bin/qemu-ppc64le  ${SINGULARITY_ROOTFS}/usr/bin/qemu-ppc64le");
                            def << copy_qemu;
                        }

                        // Create a pipe to communicate with our build subprocess
                        bp::async_pipe std_pipe(io_service);

                        // Launch our build as a subprocess
                        // We use "unbuffer" to fake the build into thinking it has a real TTY, which the command output eventually will
                        // This causes things like wget and color ls to work nicely
                        std::string build_command("/usr/bin/sudo ");
                        // If the cleint has a tty trick the subprocess into thinking that as well
                        if(client_data.tty) {
                            build_command += "/usr/bin/unbuffer ";
                        }
                        build_command += "/usr/local/bin/singularity build ./container.img ./container.def";

                        bp::group group;
                        std::error_code build_ec;
                        bp::child build_child(build_command, bp::std_in.close(), (bp::std_out & bp::std_err) > std_pipe,
                                              group, build_ec);
                        if (build_ec) {
                            throw std::runtime_error("subprocess error: " + build_ec.message());
                        }

                        logger::write("launched build process: " + build_command);

                        // stream from the async pipe process output to the socket
                        std::array<char, 4096> buffer;
                        std::size_t read_size = 0;
                        boost::system::error_code stream_error;
                        bool fin = false;
                        do {
                            // Read from the pipe into a buffer
                            auto read_bytes = std_pipe.async_read_some(buffer, yield[stream_error]);
                            if (stream_error != boost::system::errc::success && stream_error != boost::asio::error::eof) {
                                throw std::runtime_error("reading process pipe failed: " + stream_error.message());
                            }
                            // Wrap it up if we've hit EOF
                            if(stream_error == boost::asio::error::eof) {
                                fin = true;
                            }
                            // Write the buffer to our socket
                            client.async_write_some(fin, asio::buffer(buffer, read_bytes), yield, error);
                            if (error) {
                                throw std::runtime_error("sending process pipe failed: " + error.message());
                            }
                        } while (!fin);

                        // Get the return value from the build subprocess
                        logger::write("Waiting on build process to exit");
                        build_child.wait();
                        int build_code = build_child.exit_code();

                        // Send the container to the client
                        if (build_code == 0) {
                            logger::write("Build complete, sending container");
                            client.async_write_file("container.img", yield, error);
                            if (error) {
                                throw std::runtime_error("Sending file to client failed: " + error.message());
                            }
                        } else {
                            throw std::runtime_error("Build failed, not sending container");
                        }
                    });

    }

    // Start the IO service
    void run();

private:
    asio::io_service io_service;
};
