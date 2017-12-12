#include "OpenStackBuilder.h"
#include "boost/process.hpp"
#include "Logger.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

namespace bp = boost::process;
namespace pt = boost::property_tree;

namespace OpenStackBuilder {
    std::set<Builder> get_builders(asio::io_service &io_service, asio::yield_context yield) {
        std::string list_command("/home/queue/GetBuilders");
        bp::group group;
        bp::async_pipe std_pipe(io_service);
        asio::streambuf buffer;

        // Asynchronously launch the list command
        std::error_code list_error;
        logger::write("Running command: " + list_command);
        bp::child list_child(list_command, bp::std_in.close(), (bp::std_out & bp::std_err) > std_pipe, group, list_error);
        if (list_error) {
            logger::write("subprocess error: " + list_error.message());
        }

        // Read the list command output until we reach EOF, which is returned as an error
        boost::system::error_code read_error;
        boost::asio::async_read(std_pipe, buffer, yield[read_error]);
        if (read_error != asio::error::eof) {
            logger::write("OpenStack destroy error: " + read_error.message());
        }

        // Grab exit code from destroy command
        list_child.wait();
        int exit_code = list_child.exit_code();

        if (exit_code != 0) {
            logger::write("Failed to fetch server list");
        }

        // Read the json output into a property tree
        /*
           [
             {
               "Status": "ACTIVE",
               "Name": "Builder",
               "Image": "BuilderImage",
               "ID": "adeda126-18d4-423f-a499-84651937cdc0",
               "Flavor": "m1.medium",
               "Networks": "or_provider_general_extnetwork1=128.219.185.100"
              }
            ]
         */
        std::istream is_buffer(&buffer);
        pt::ptree builder_tree;
        pt::read_json(is_buffer, builder_tree);

        // Fill a set of builders from the property tree data
        std::set<Builder> builders;
        try {
            for (const auto &builder_node : builder_tree) {
                Builder builder;
                auto network = builder_node.second.get<std::string>("Networks");
                size_t eq_pos = network.find('=');
                builder.host = network.substr(eq_pos + 1);

                builder.id = builder_node.second.get<std::string>("ID");
                builder.port = "8080";
                builders.insert(builder);
            }
        } catch(const pt::ptree_error &e) {
            logger::write(std::string() + "Error parsing builders: " + e.what());
        }

        return builders;
    }

    void request_create(asio::io_service &io_service, asio::yield_context yield) {
        std::string create_command("/home/queue/RequestCreateBuilder");
        bp::group group;
        bp::async_pipe std_pipe(io_service);
        asio::streambuf buffer;

        // Asynchronously launch the create command
        std::error_code create_error;
        logger::write("Running command: " + create_command);
        bp::child build_child(create_command, bp::std_in.close(), (bp::std_out & bp::std_err) > std_pipe, group,
                              create_error);
        if (create_error) {
            logger::write("subprocess error: " + create_error.message());
        }

        // Read the create_command output until we reach EOF, which is returned as an error
        boost::system::error_code read_error;
        boost::asio::async_read(std_pipe, buffer, yield[read_error]);
        if (read_error != asio::error::eof) {
            logger::write("OpenStack create error: " + read_error.message());
        }

        // Grab exit code  from builder
        build_child.wait();
        int exit_code = build_child.exit_code();

        if (exit_code != 0) {
            logger::write("Error in making call to RequestBuilder");
        }
    }

    void destroy(Builder builder, asio::io_service &io_service, asio::yield_context yield) {
        std::string destroy_command("/home/queue/DestroyBuilder " + builder.id);
        bp::group group;
        bp::async_pipe std_pipe(io_service);
        asio::streambuf buffer;

        // Asynchronously launch the destroy command
        std::error_code destroy_error;
        logger::write("Running command: " + destroy_command);
        bp::child destroy_child(destroy_command, bp::std_in.close(), (bp::std_out & bp::std_err) > std_pipe, group,
                                destroy_error);
        if (destroy_error) {
            logger::write("subprocess error: " + destroy_error.message());
        }

        // Read the destroy_command output until we reach EOF, which is returned as an error
        boost::system::error_code read_ec;
        boost::asio::async_read(std_pipe, buffer, yield[read_ec]);
        if (read_ec != asio::error::eof) {
            logger::write("OpenStack destroy error: " + read_ec.message());
        }

        // Grab exit code from destroy command
        destroy_child.wait();
        int exit_code = destroy_child.exit_code();

        // TODO an error "can't" occur, if an error deleting is detected it should make all attempts to fix it
        if (exit_code != 0) {
            logger::write("Builder with ID " + builder.id + " failed to be destroyed");
        }
    }
}
