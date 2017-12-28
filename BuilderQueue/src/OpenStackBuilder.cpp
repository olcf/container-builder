#include "OpenStackBuilder.h"
#include <boost/process.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include "Logger.h"

namespace bp = boost::process;
namespace pt = boost::property_tree;

namespace OpenStackBuilder {
    std::set<BuilderData> get_builders(asio::io_context &io_context, asio::yield_context yield, boost::system::error_code &error) {
        std::string list_command("/usr/local/bin/GetBuilders");
        bp::group group;
        bp::async_pipe std_pipe(io_context);
        asio::streambuf buffer;

        // Asynchronously launch the list command
        std::error_code list_error;
        bp::child list_child(list_command, bp::std_in.close(), (bp::std_out & bp::std_err) > std_pipe, group, list_error);
        if (list_error) {
            logger::write("subprocess error: " + error.message());
        }

        // Read the list command output until we reach EOF, which is returned as an error
        asio::async_read(std_pipe, buffer, yield[error]);
        if (error != asio::error::eof) {
            logger::write("OpenStack destroy error: " + error.message());
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
               "Name": "BuilderData",
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
        std::set<BuilderData> builders;
        try {
            for (const auto &builder_node : builder_tree) {
                BuilderData builder;
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

    void request_create(asio::io_context &io_context, asio::yield_context yield, boost::system::error_code &error) {
        std::string create_command("/usr/local/bin/RequestCreateBuilder");
        bp::group group;
        bp::async_pipe std_pipe(io_context);
        asio::streambuf buffer;

        // Asynchronously launch the create command
        std::error_code create_error;
        bp::child build_child(create_command, bp::std_in.close(), (bp::std_out & bp::std_err) > std_pipe, group,
                              create_error);
        if (create_error) {
            logger::write("subprocess error: " + error.message());
        }

        // Read the create_command output until we reach EOF, which is returned as an error
        boost::asio::async_read(std_pipe, buffer, yield[error]);
        if (error != asio::error::eof) {
            logger::write("OpenStack create error: " + error.message());
        }

        // Grab exit code  from builder
        build_child.wait();
        int exit_code = build_child.exit_code();

        if (exit_code != 0) {
            logger::write("Error in making call to RequestBuilder");
        }
    }

    void destroy(BuilderData builder, asio::io_context &io_context, asio::yield_context yield, boost::system::error_code &error) {
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
        }

        // Read the destroy_command output until we reach EOF, which is returned as an error
        boost::asio::async_read(std_pipe, buffer, yield[error]);
        if (error != asio::error::eof) {
            logger::write("OpenStack destroy error: " + error.message());
        }

        // Grab exit code from destroy command
        destroy_child.wait();
        int exit_code = destroy_child.exit_code();

        if (exit_code != 0) {
            logger::write("BuilderData with ID " + builder.id + " failed to be destroyed");
        }
    }
}
