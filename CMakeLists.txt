cmake_minimum_required(VERSION 3.8)
project(ContainerBuilder)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wall -Wextra -fstack-protector-strong -D_FORTIFY_SOURCE=2 -fPIE -fpie")# -Wl,-z,noexecstack,-z,now,-z,relro,-z,nodlopen")

include_directories(Server/include Common/include)

# Files related to the server
set(SOURCE_FILES_SERVER Server/src/main.cpp
        Server/src/Connection.cpp
        Server/src/Reservation.cpp
        Server/src/ReservationRequest.cpp
        Server/src/Resource.cpp
        Server/src/ResourceQueue.cpp
        Server/include/Connection.h
        Server/include/Reservation.h
        Server/include/ReservationRequest.h
        Server/include/Resource.h
        Server/include/ResourceQueue.h
        Server/src/Builder.cpp
        Server/include/Builder.h
        Server/src/DockerBackend.cpp
        Server/include/DockerBackend.h
        Server/src/Logger.cpp
        Server/include/Logger.h
        Server/src/SingularityBackend.cpp
        Server/include/SingularityBackend.h
        Common/src/Messenger.cpp
        Common/include/Messenger.h)

# Files related to the API
set(SOURCE_FILES_CLIENT Client/main.cpp
        Server/src/Logger.cpp
        Server/include/Logger.h
        Common/src/Messenger.cpp Common/include/Messenger.h)

# Create executables
add_executable(ContainerBuilderServer ${SOURCE_FILES_SERVER})
add_executable(ContainerBuilderClient ${SOURCE_FILES_CLIENT})

# -fsanitize=undefined causing issues with launching application, removing for now
set_target_properties(ContainerBuilderServer
        PROPERTIES COMPILE_FLAGS "${COMPILE_FLAGS} -DDEBUG=1 -fsanitize=address -fno-omit-frame-pointer")
set_target_properties(ContainerBuilderServer
        PROPERTIES LINK_FLAGS "${LINK_FLAGS} -fsanitize=address -fno-omit-frame-pointer")
set_target_properties(ContainerBuilderClient
        PROPERTIES COMPILE_FLAGS "${COMPILE_FLAGS} -DDEBUG=1 -fsanitize=address -fno-omit-frame-pointer")
set_target_properties(ContainerBuilderClient
        PROPERTIES LINK_FLAGS "${LINK_FLAGS} -fsanitize=address -fno-omit-frame-pointer")


set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(ContainerBuilderServer ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(ContainerBuilderClient ${CMAKE_THREAD_LIBS_INIT})


find_package(Boost 1.64.0 COMPONENTS system coroutine filesystem serialization regex log log_setup thread REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})
target_link_libraries(ContainerBuilderServer ${Boost_LIBRARIES})
target_link_libraries(ContainerBuilderClient ${Boost_LIBRARIES})

install (TARGETS ContainerBuilderServer ContainerBuilderClient RUNTIME DESTINATION bin)