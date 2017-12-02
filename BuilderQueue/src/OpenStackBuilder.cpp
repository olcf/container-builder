#include "OpenStackBuilder.h"
#include "boost/process.hpp"
#include "Logger.h"

namespace bp = boost::process;

namespace OpenStackBuilder {
    boost::optional<Builder> request_create(asio::io_service& io_service, asio::yield_context yield) {
        std::string create_command("/home/queue/RequestCreateBuilder");
        bp::group group;
        bp::async_pipe std_pipe(io_service);
        asio::streambuf buffer;

        // Asynchronously launch the create command
        std::error_code create_ec;
        logger::write("Running command: " + create_command);
        bp::child build_child(create_command, bp::std_in.close(), (bp::std_out & bp::std_err) > std_pipe, group, create_ec);
        if(create_ec) {
            logger::write("subprocess error: " + create_ec.message());
        }

        // Read the create_command output until we reach EOF, which is returned as an error
        boost::system::error_code read_ec;
        boost::asio::async_read(std_pipe, buffer, yield[read_ec]);
        if(read_ec != asio::error::eof) {
            logger::write("OpenStack create error: " + read_ec.message());
        }

        // Grab exit code  from builder
        build_child.wait();
        int exit_code = build_child.exit_code();

        boost::optional<Builder> opt_builder;

        // If successful parse the output for the newly created builders ID, IP address, and port
        if(exit_code == 0) {
            Builder builder;
            std::istream stream(&buffer);
            std::string line;

            std::getline(stream, builder.id);
            if(stream.fail() || stream.bad() || stream.eof()) {
               return opt_builder;
            }
            std::getline(stream, builder.host);
            if(stream.fail() || stream.bad() || stream.eof()) {
               return opt_builder;
            }
            std::getline(stream, builder.port);
            if(stream.fail() || stream.bad()) {
               return opt_builder;
            }

            opt_builder = builder;
        }

        return opt_builder;
    }

    void destroy(Builder builder, asio::io_service& io_service, asio::yield_context yield) {
        std::string destroy_command("/home/queue/DestroyBuilder " + builder.id);
        bp::group group;
        bp::async_pipe std_pipe(io_service);
        asio::streambuf buffer;

        // Asynchronously launch the destroy command
        std::error_code destroy_ec;
        logger::write("Running command: " + destroy_command);
        bp::child destroy_child(destroy_command, bp::std_in.close(), (bp::std_out & bp::std_err) > std_pipe, group, destroy_ec);
        if(destroy_ec) {
            logger::write("subprocess error: " + destroy_ec.message());
        }

        // Read the destroy_command output until we reach EOF, which is returned as an error
        boost::system::error_code ec;
        boost::asio::async_read(std_pipe, buffer, yield[ec]);
        if(ec != asio::error::eof) {
            logger::write("OpenStack destroy error: " + ec.message());
        }

        // Grab exit code from destroy command
        destroy_child.wait();
        int exit_code = destroy_child.exit_code();

        if(exit_code != 0) {
            logger::write("Builder with ID " + builder.id + " failed to be destroyed");
        }
    }
}