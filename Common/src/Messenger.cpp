#include "Messenger.h"
#include "boost/asio/buffer.hpp"
#include <boost/progress.hpp>
#include <boost/crc.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include "Builder.h"

// Read a string message asynchronously
std::string Messenger::async_read_string(asio::yield_context yield,
                                         std::error_code &error) {
    std::string message;
    auto buffer = boost::asio::dynamic_buffer(message);
    beast::error_code read_error;
    stream.async_read(buffer, yield[read_error]);
    if(read_error) {
        logger::write("Failed to read string: " + read_error.message());
        error = std::error_code(read_error.value(), read_error.category());
        return message;
    }
    return message;
}

// Write a string message asynchronously
void Messenger::async_write_string(const std::string &message,
                                   asio::yield_context yield,
                                   std::error_code &error) {
    beast::error_code write_error;
    stream.async_write(asio::buffer(message), yield[write_error]);
    if(write_error) {
        logger::write("Failed to write string: " + write_error.message());
        error = std::error_code(write_error.value(), write_error.category());
        return;
    }
}

void Messenger::async_read_file(boost::filesystem::path file_path,
                                asio::yield_context yield,
                                std::error_code &error) {
    std::ofstream file;

    // Openfile for writing
    file.open(file_path.string(), std::fstream::out | std::fstream::binary | std::fstream::trunc);
    if (!file) {
        logger::write("Error opening file: " + file_path.string() + " " + std::strerror(errno),
                      logger::severity_level::error);
        error = std::error_code(errno, std::generic_category());
        return;
    }

    const auto chunk_size = 4096;
    std::array<char, chunk_size> buffer;
    boost::crc_32_type csc_result;

    // Read file in chunks
    beast::error_code read_error;
    do {
        auto bytes_read = stream.async_read_some(asio::buffer(buffer), yield[read_error]);
        if (read_error && read_error != asio::error::eof) {
            logger::write("Error file chunk read: " + read_error.message(), logger::severity_level::error);
            error = std::error_code(read_error.value(), read_error.category());
            file.close();
            return;
        }
        file.write(buffer.data(), bytes_read);
        csc_result.process_bytes(buffer.data(), bytes_read);
    } while (!stream.is_message_done());

    file.close();
    if (!file) {
        logger::write("Error closing file: " + file_path.string() + " " + std::strerror(errno),
                      logger::severity_level::error);
        error = std::error_code(errno, std::generic_category());
        return;
    }

    // Read remote file checksum and verify
    auto local_checksum = std::to_string(csc_result.checksum());
    auto remote_checksum = async_read_string(yield, error);
    if (error) {
        logger::write("Invalid file chunk read: " + error.message(), logger::severity_level::error);
        return;
    }
    if (local_checksum != remote_checksum) {
        logger::write("File checksums do not match, file corrupted!", logger::severity_level::fatal);
        error = std::error_code(errno, std::generic_category());
        return;
    }
}

// Send a file as a message asynchronously
void Messenger::async_write_file(boost::filesystem::path file_path,
                                 asio::yield_context yield,
                                 std::error_code &error) {
    std::ifstream file;

    // Open file and get size
    file.open(file_path.string(), std::fstream::in | std::fstream::binary);
    if (!file) {
        logger::write("Error opening file: " + file_path.string() + " " + std::strerror(errno),
                      logger::severity_level::error);
        error = std::error_code(errno, std::generic_category());
        return;
    }
    auto file_size = boost::filesystem::file_size(file_path);

    const std::size_t chunk_size = 4096;
    std::array<char, chunk_size> buffer;

    auto bytes_remaining = file_size;
    char buffer_storage[chunk_size];
    boost::crc_32_type csc_result;
    bool fin = false;

    // Send file in chunks
    beast::error_code write_error;
    do {
        auto bytes_to_send = std::min(bytes_remaining, chunk_size);
        file.read(buffer_storage, bytes_to_send);
        csc_result.process_bytes(buffer_storage, bytes_to_send);
        bytes_remaining -= bytes_to_send;
        if (bytes_remaining == 0) {
            fin = true;
        }
        stream.async_write_some(fin, asio::buffer(buffer, bytes_to_send), yield[write_error]);
        if (write_error) {
            logger::write("Bad file chunk send: " + write_error.message(), logger::severity_level::error);
            error = std::error_code(write_error.value(), write_error.category());
            return;
        }

    } while (bytes_remaining);

    file.close();
    if (!file) {
        logger::write("Error closing file: " + file_path.string() + " " + std::strerror(errno),
                      logger::severity_level::error);
        error = std::error_code(errno, std::generic_category());
        return;
    }

    // After we've sent the file we send the checksum
    // This would make more logical sense in the header but would require us to traverse a potentialy large file twice
    auto checksum = std::to_string(csc_result.checksum());
    async_write_string(checksum, yield, error);
    if (error) {
        logger::write("Bad file checksum send: " + error.message(), logger::severity_level::error);
        return;
    }
}

BuilderData Messenger::async_read_builder(asio::yield_context yield,
                                          std::error_code &error) {
    BuilderData builder;

    // Read in the serialized BuilderData as a string
    auto serialized_builder = async_read_string(yield, error);
    if (error) {
        logger::write("Received bad builder: " + error.message(), logger::severity_level::error);
        return builder;
    }

    // de-serialize BuilderData
    std::istringstream archive_stream(serialized_builder);
    boost::archive::text_iarchive archive(archive_stream);
    archive >> builder;

    return builder;
}

void Messenger::async_write_builder(BuilderData builder,
                                    asio::yield_context yield,
                                    std::error_code &error) {
    // Serialize the builder into a string
    std::ostringstream archive_stream;
    boost::archive::text_oarchive archive(archive_stream);
    archive << builder;
    auto serialized_builder = archive_stream.str();

    async_write_string(serialized_builder, yield, error);
    if (error) {
        logger::write("Error sending builder: " + error.message(), logger::severity_level::error);
        return;
    }
}

ClientData Messenger::async_read_client_data(asio::yield_context yield,
                                             std::error_code &error) {
    ClientData client_data;

    // Read in the serialized client data as a string
    auto serialized_client_data = async_read_string(yield, error);
    if (error) {
        logger::write("Received bad client data: " + error.message(), logger::severity_level::error);
        return client_data;
    }

    // de-serialize BuilderData
    std::istringstream archive_stream(serialized_client_data);
    boost::archive::text_iarchive archive(archive_stream);
    archive >> client_data;

    return client_data;
}

void Messenger::async_write_client_data(ClientData client_data,
                                        asio::yield_context yield,
                                        std::error_code &error) {
    // Serialize the client data into a string
    std::ostringstream archive_stream;
    boost::archive::text_oarchive archive(archive_stream);
    archive << client_data;
    auto serialized_client_data = archive_stream.str();

    async_write_string(serialized_client_data, yield, error);
    if (error) {
        logger::write("Error sending client data: " + error.message(), logger::severity_level::error);
        return;
    }
}

void Messenger::async_write_pipe(bp::async_pipe &pipe,
                                 asio::yield_context yield,
                                 std::error_code &error) {
    std::array<char, 4096> buffer;
    bool fin = false;
    boost::system::error_code process_error;
    do {
        // Read from the pipe into a buffer
        auto read_bytes = pipe.async_read_some(asio::buffer(buffer), yield[process_error]);
        if (process_error && process_error != boost::asio::error::eof) {
            logger::write("reading process pipe failed: " + process_error.message());
            error = std::error_code(process_error.value(), process_error.category());
            return;
        }
        // Wrap it up if we've hit EOF on our process output
        if (process_error == boost::asio::error::eof) {
            fin = true;
        }
        // Write read_bytes of the buffer to our socket
        beast::error_code write_error;
        stream.async_write_some(fin, asio::buffer(buffer.data(), read_bytes), yield[write_error]);
        if (write_error) {
            logger::write("sending process pipe failed: " + write_error.message());
            error = std::error_code(write_error.value(), write_error.category());
            return;
        }
    } while (!fin);
}

//  Print a large message as it arrives to the socket
void Messenger::async_stream_print(asio::yield_context yield,
                                   std::error_code &error) {
    const auto max_read_bytes = 4096;
    std::array<char, max_read_bytes> buffer;
    beast::error_code read_error;
    do {
        auto bytes_read = stream.async_read_some(asio::buffer(buffer), yield[read_error]);
        if (read_error && read_error != asio::error::eof) {
            logger::write(std::string() + "Error reading process output: " + read_error.message());
            error = std::error_code(read_error.value(), read_error.category());
            return;
        }
        std::cout.write(buffer.data(), bytes_read);
    } while (!stream.is_message_done());
}