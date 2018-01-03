#pragma once

#include <boost/process.hpp>
#include "boost/asio/io_context.hpp"
#include "BuilderData.h"
#include <set>

namespace asio = boost::asio;
namespace bp = boost::process;

//using CreateHandler = std::function<void (std::error_code error, BuilderData builder)>;
//using DestroyHandler = std::function<void (std::error_code error)>;

namespace OpenStack {
    // Request to create a new builder
    // The handler must have the form void(std::error_code error, BuilderData builder)
    // If the builder wasn't created the handlers error_code will be set
    template<typename CreateHandler>
    void request_create(asio::io_context &io_context, CreateHandler handler) {
        std::string create_command("/usr/local/bin/RequestCreateBuilder");
        bp::group group;
        bp::async_pipe std_pipe(io_context);
        asio::streambuf buffer;

        // Asynchronously launch the create command
        std::error_code create_error;
        bp::child build_child(create_command, bp::std_in.close(), (bp::std_out & bp::std_err) > std_pipe, group,
                              create_error);
        if (create_error) {
            logger::write("subprocess error: " + create_error.message());
            error = create_error;
            return;
        }

        // Read the create_command output until we reach EOF, which is returned as an error
        boost::system::error_code read_error;
        boost::asio::async_read(std_pipe, buffer, yield[read_error]);
        if (read_error != asio::error::eof) {
            logger::write("OpenStack create error: " + read_error.message());
            error = std::error_code(read_error.value(), std::generic_category());
            return;
        }

        // Grab exit code  from builder
        build_child.wait();
        int exit_code = build_child.exit_code();

        if (exit_code != 0) {
            logger::write("Error in making call to RequestBuilder");
            error = std::error_code(EBADMSG, std::generic_category());
            return;
        }
    }

    // Request to destroy the specified builder
    // If the builder couldn't be destroyed the handlers error_code will be set
    template<typename DestroyHandler>
    void destroy(BuilderData builder, asio::io_context &io_context, DestroyHandler handler) {
        std::string destroy_command("/usr/local/bin/DestroyBuilder " + builder.id);
        bp::group group;
        bp::async_pipe std_pipe(io_context);
        asio::streambuf buffer;

        // Asynchronously launch the destroy command
        std::error_code destroy_error;
        bp::child destroy_child(destroy_command, bp::std_in.close(), (bp::std_out & bp::std_err) > std_pipe, group,
                                destroy_error);
        if (destroy_error) {
            logger::write("subprocess error: " + error.message());
            error = destroy_error;
            return;
        }

        // Read the destroy_command output until we reach EOF, which is returned as an error
        boost::system::error_code read_error;
        boost::asio::async_read(std_pipe, buffer, yield[read_error]);
        if (read_error != asio::error::eof) {
            logger::write("OpenStack destroy error: " + read_error.message());
            error = std::error_code(read_error.value(), std::generic_category());
            return;
        }

        // Grab exit code from destroy command
        destroy_child.wait();
        int exit_code = destroy_child.exit_code();

        if (exit_code != 0) {
            logger::write("BuilderData with ID " + builder.id + " failed to be destroyed");
            error = std::error_code(EDOM, std::generic_category());
            return;
        }
    }

};