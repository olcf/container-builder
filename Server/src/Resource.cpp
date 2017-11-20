#include "Resource.h"
#include <boost/asio/read.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/asio/write.hpp>


void Resource::read(tcp::socket &socket) {
    // Read in 4 byte header
    const size_t header_size = 4;
    std::string header("", header_size);
    asio::read(socket, asio::buffer(&header[0], header_size));
    unsigned int resource_size = std::stoul(header);

    // Read in serialized Resource
    std::string serialized_resource("", resource_size);
    asio::read(socket, asio::buffer(&serialized_resource[0], resource_size));

    // de-serialize Resource
    std::istringstream archive_stream(serialized_resource);
    boost::archive::text_iarchive archive(archive_stream);
    archive >> *this;
}

void Resource::async_write(tcp::socket &socket, asio::yield_context yield) {
    // Serialize the data into a string
    std::ostringstream archive_stream;
    boost::archive::text_oarchive archive(archive_stream);
    archive << *this;
    auto serialized_resource = archive_stream.str();

    // Construct byte count header
    auto header = std::to_string(serialized_resource.size());
    header.resize(4);

    // Send header consisting of 4 byte size, in bytes, of archived Resource
    asio::async_write(socket, asio::buffer(header), yield);
    asio::async_write(socket, asio::buffer(serialized_resource), yield);
}