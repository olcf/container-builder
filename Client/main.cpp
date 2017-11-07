#include <cstdlib>
#include <cstring>
#include <iostream>
#include <fstream>
#include <boost/asio.hpp>
#include "FileWriter.h"
#include "FileReader.h"

namespace asio = boost::asio;
using asio::ip::tcp;

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

        // Read the container image
        FileReader image("container.img");
        image.read(socket);

        std::cout<<"Container build\n";

    }
    catch (std::exception &e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}