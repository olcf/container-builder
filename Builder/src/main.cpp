#include "Builder.h"

int main(int argc, char *argv[]) {

    try {
        Builder builder;
        builder.run();
    }
    catch (const std::exception& ex) {
        logger::write(std::string() + "Builder exception encountered: " + ex.what(), logger::severity_level::fatal);
    } catch(...) {
        logger::write("Unknown exception caught!", logger::severity_level::fatal);
    }

    logger::write("BuilderData shutting down");

    return 0;
}