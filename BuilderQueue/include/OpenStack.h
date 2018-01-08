#pragma once

#include <boost/process/child.hpp>
#include <boost/process/async_pipe.hpp>
#include <boost/process/group.hpp>
#include <boost/process/io.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/streambuf.hpp>
#include "BuilderData.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include "Logger.h"

namespace asio = boost::asio;
namespace bp = boost::process;
namespace pt = boost::property_tree;

class OpenStack : public std::enable_shared_from_this<OpenStack> {
public:
    OpenStack(asio::io_context &io_context) : io_context(io_context),
                                              output_pipe(io_context) {}

    // Request to create a new builder
    // The handler must have the form void(std::error_code error, BuilderData builder)
    // If the builder wasn't created the handlers error_code will be set and a default constructed builder will be returned
    template<typename CreateHandler>
    void request_create(CreateHandler handler) {
        std::string create_command("/usr/local/bin/CreateBuilder");

        Logger::info("Launching command: " + create_command);

        std::error_code create_error;
        process = bp::child(create_command, bp::std_in.close(), (bp::std_out & bp::std_err) > output_pipe, group,
                            create_error);
        if (create_error) {
            auto error = std::error_code(create_error.value(), std::generic_category());
            Logger::error("Error requesting builder: " + create_error.message());
            BuilderData no_builder;
            io_context.post(std::bind(handler, error, no_builder));
        }

        // Read the create command output until we reach EOF, which is returned as an error
        auto self(shared_from_this());
        asio::async_read(output_pipe, output_buffer,
                         [this, self, handler](boost::system::error_code error, std::size_t) {
                             if (error != asio::error::eof) {
                                 auto read_error = std::error_code(error.value(), std::generic_category());
                                 Logger::error("Error reading builder request output: " + read_error.message());
                                 BuilderData no_builder;
                                 io_context.post(std::bind(handler, read_error, no_builder));
                             } else {
                                 // Parse the JSON output to create BuilderData
                                 /*
                                     {
                                       "Status": "ACTIVE",
                                       "Name": "BuilderData",
                                       "Image": "BuilderImage",
                                       "ID": "adeda126-18d4-423f-a499-84651937cdc0",
                                       "Flavor": "m1.medium",
                                       "Networks": "or_provider_general_extnetwork1=128.219.185.100"
                                     }
                                 */
                                 std::istream is_buffer(&output_buffer);


                                 Logger::info("Parsing builder output");
                                 try {
                                     pt::ptree builder_tree;
                                     pt::read_json(is_buffer, builder_tree);

                                     // Parse IP address
                                     BuilderData builder;
                                     auto network = builder_tree.get<std::string>("addresses");
                                     size_t eq_pos = network.find('=');
                                     builder.host = network.substr(eq_pos + 1);

                                     // Parse OpenStack ID
                                     builder.id = builder_tree.get<std::string>("id");

                                     // Port for builder service
                                     builder.port = "8080";

                                     // Complete the handler
                                     io_context.post(std::bind(handler, std::error_code(), builder));

                                     Logger::info("New builder created: " + builder.id);
                                 } catch (const std::exception &ex) {
                                     Logger::error(std::string("Error parsing JSON builder output: ") + ex.what());
                                     auto parse_error = std::error_code(EBADMSG, std::generic_category());
                                     BuilderData no_builder;
                                     io_context.post(std::bind(handler, parse_error, no_builder));
                                 }
                             }

                             // Verify that the application exited cleanly
                             // If the parsing above worked this shouldn't happen
                             std::error_code wait_error;
                             process.wait(wait_error);
                             if (wait_error) {
                                 Logger::error("Error waiting for builder request to finish: " + wait_error.message());
                             }
                             int exit_code = process.exit_code();
                             if (exit_code != 0) {
                                 Logger::error("builder request returned non zero: " + std::to_string(exit_code));
                             }
                         });
    }

    // Request to destroy the specified builder
    // If the builder couldn't be destroyed the handlers error_code will be set
    template<typename DestroyHandler>
    void destroy(BuilderData builder, DestroyHandler handler) {
        const std::string destroy_command("/usr/local/bin/DestroyBuilder " + builder.id);

        Logger::info("Launching command: " + destroy_command);
        std::error_code destroy_error;
        process = bp::child(destroy_command, bp::std_in.close(),
                            (bp::std_out & bp::std_err) > output_pipe, group, destroy_error);
        if (destroy_error) {
            Logger::error("Error launching destroy command: " + destroy_error.message());
            io_context.post(std::bind(handler, destroy_error));
        }

        // Read the destroy command output until we reach EOF, which is returned as an error
        auto self(shared_from_this());
        asio::async_read(output_pipe, output_buffer,
                         [this, self, handler](boost::system::error_code error, std::size_t) {
                             if (error != asio::error::eof) {
                                 Logger::error("Error reading builder destroy command: " + error.message());
                                 auto read_error = std::error_code(error.value(), std::generic_category());
                                 io_context.post(std::bind(handler, read_error));
                             } else {
                                 Logger::error("Builder destroyed");
                                 io_context.post(std::bind(handler, std::error_code()));
                             }

                             // Verify that the application exited cleanly
                             // If the parsing above worked this shouldn't happen
                             std::error_code wait_error;
                             process.wait(wait_error);
                             if (wait_error) {
                                 Logger::error("Error waiting for builder delete to finish: " + wait_error.message());
                             }
                             int exit_code = process.exit_code();
                             if (exit_code != 0) {
                                 Logger::error("builder request returned non zero: " + std::to_string(exit_code));
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