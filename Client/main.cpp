#include <cstdlib>
#include <cstring>
#include <iostream>
#include <fstream>
#include <boost/asio.hpp>
#include "FileWriter.h"
#include "FileReader.h"

namespace asio = boost::asio;
using asio::ip::tcp;

std::string read_line(tcp::socket &socket) {
    asio::streambuf reserve_buffer;
    asio::read_until(socket, reserve_buffer, '\n');
    std::istream reserve_stream(&reserve_buffer);
    std::string reserve_string;
    std::getline(reserve_stream, reserve_string);
    return reserve_string;
}

int main(int argc, char *argv[]) {
    try {
        asio::io_service io_service;
        tcp::socket socket(io_service);
        tcp::resolver resolver(io_service);
        asio::connect(socket, resolver.resolve({std::string("localhost"), std::string("12345")}));

        // Initiate a build request
        std::string request_message("build_request\n");
        asio::write(socket, asio::buffer(request_message));

        // Send the definition file
        std::cout<<"creating fake definition file\n";
        std::ofstream outfile("container.def");
        outfile << "i'm not really a definition" << std::endl;
        outfile.close();
        FileWriter definition("container.def");
        definition.write(socket);

        // Read the build output
        asio::streambuf buf;
        size_t read_bytes = 0;
        boost::system::error_code ec;
        uint64_t message_size;
        do {
            // Read header
            asio::read(socket, asio::buffer(&message_size, sizeof(uint64_t)));
            // If message size is 0 the server is done streaming output
            if(message_size == 0) {
                break;
            }
            else {
                asio::read(socket, buf, asio::transfer_exactly(message_size));
                std::string s( (std::istreambuf_iterator<char>(&buf)), std::istreambuf_iterator<char>() );
                std::cout<<s;
            }
        } while(message_size > 0);

        // Read the container image
        FileReader image("container.img");
        image.read(socket);

        std::cout<<"Container built!\n";

    }
    catch (std::exception &e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}