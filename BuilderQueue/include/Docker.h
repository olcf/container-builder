#pragma once

#include <boost/process/child.hpp>
#include <boost/process/async_pipe.hpp>
#include <boost/process/group.hpp>
#include <boost/process/io.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/streambuf.hpp>
#include "Logger.h"

namespace asio = boost::asio;
namespace bp = boost::process;

class Docker : public std::enable_shared_from_this<Docker> {
public:
    explicit Docker(asio::io_context &io_context) : io_context(io_context), output_pipe(io_context) {}

    // Request to get a list of docker builders
    // The handler must have the form void(std::error_code error, std::string image_list)
    template<typename ListHandler>
    void request_image_list(ListHandler handler) {
        std::string list_command("/usr/local/bin/list-images.sh");

        Logger::info("Launching command: " + list_command);

        std::error_code list_error;
        process = bp::child(list_command, bp::std_in.close(), (bp::std_out & bp::std_err) > output_pipe, group,
                            list_error);
        if (list_error) {
            auto error = std::error_code(list_error.value(), std::generic_category());
            Logger::error("Error requesting image list: " + list_error.message());
            io_context.post(std::bind(handler, error, std::string("image list error")));
        }

        // Read the list-images command output until we reach EOF, which is returned as an error
        auto self(shared_from_this());
        asio::async_read(output_pipe, output_buffer,
                         [this, self, handler](boost::system::error_code error, std::size_t) {
                             if (error != asio::error::eof) {
                                 auto read_error = std::error_code(error.value(), std::generic_category());
                                 Logger::error("Error reading image list output: " + read_error.message());
                                 std::string no_list("Error reading image list output");
                                 io_context.post(std::bind(handler, read_error, no_list));
                             } else {
                                 // Complete the handler
                                 std::string image_list((std::istreambuf_iterator<char>(&output_buffer)), std::istreambuf_iterator<char>());
                                 io_context.post(std::bind(handler, std::error_code(), image_list));
                                 Logger::info("image list posted");
                             }
                         });
    }

private:
    asio::io_context &io_context;
    bp::child process;
    bp::group group;
    bp::async_pipe output_pipe;
    asio::streambuf output_buffer;
};