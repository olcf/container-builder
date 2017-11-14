#include <cstdlib>
#include <cstring>
#include <iostream>
#include <fstream>
#include <boost/asio.hpp>
#include "WriteFile.h"
#include "ReadFile.h"
#include "WriteMessage.h"
#include "ReadMessage.h"

namespace asio = boost::asio;
using asio::ip::tcp;

int main(int argc, char *argv[]) {
    try {
        asio::io_service io_service;
        tcp::socket socket(io_service);
        tcp::resolver resolver(io_service);
        asio::connect(socket, resolver.resolve({std::string("localhost"), std::string("12345")}));

        // Initiate a build request
        std::string request_message("build_request");
        message::write(socket, asio::buffer(request_message), request_message.size());

        // Send the definition file
        std::cout<<"creating fake definition file\n";
        std::ofstream outfile("container.def");
        outfile << "i'm not really a definition" << std::endl;
        outfile.close();
        WriteFile definition("container.def");
        definition.write(socket);

        // Read the build output until a zero length message is sent
        uint32_t bytes_read = 0;
        do {
            std::array<char, 1024> buffer;
            bytes_read = message::read(socket, asio::buffer(buffer), [buffer](auto chunk_size) {
                std::string message;
                message.assign(buffer.data(), chunk_size);
                std::cout<<message;
            });
        } while(bytes_read > 0);

        // Read the container image
        ReadFile image("container.img");
        image.read(socket);

        std::cout<<"Container built!\n";

    }
    catch (std::exception &e) {
        std::cerr << "Failed to build container: " << e.what() << "\n";
    }

    return 0;
}