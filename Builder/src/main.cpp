#include "Builder.h"

int main(int argc, char *argv[]) {

    try {
        Builder builder;
        builder.run();

    }
    catch (...) {
        logger::write("Unknown builder exception encountered");
    }

    logger::write("BuilderData shutting down");

    return 0;
}