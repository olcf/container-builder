#include "Client.h"
#include "Messenger.h"
#include "WaitingAnimation.h"

int main(int argc, char *argv[]) {

    try {
        Client client(argc, argv);
        client.run();
    }
    catch (std::exception &exception) {
        logger::write(std::string() + "Client exception: " + exception.what(), logger::severity_level::fatal);
    }
    catch (...) {
        logger::write("Unknown ContainerBuilder exception: ", logger::severity_level::fatal);
    }

    return 0;
}
