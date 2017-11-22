#include "OpenStackResource.h"
#include "boost/process.hpp"
#include "Logger.h"

// TODO make this an OpenStackResources class to handle keeping better track of what's going on

namespace bp = boost::process;

void OpenStackResource::create_new_builder() {
    bp::ipstream out_stream;
    bp::system("/home/queue/BringUpBuilder", (bp::std_out & bp::std_err) > out_stream);
    std::string output_string;
    out_stream >> output_string;

    logger::write("Created new builder through BringUpBuilder : " + output_string);
};