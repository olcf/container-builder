cmake_minimum_required(VERSION 3.8)
project(ContainerBuilder)

set(CMAKE_CXX_STANDARD 17)

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
        Common/src/FileWriter.cpp
        Common/include/FileWriter.h
        Common/src/FileReader.cpp
        Common/include/FileReader.h
        Server/src/Builder.cpp
        Server/include/Builder.h
        Server/src/DockerBackend.cpp
        Server/include/DockerBackend.h
        Server/src/Logger.cpp
        Server/include/Logger.h Common/src/Message.cpp Common/include/Message.h)

# Files related to the API
set(SOURCE_FILES_CLIENT Client/main.cpp
        Common/src/FileReader.cpp
        Common/include/FileReader.h
        Common/src/FileWriter.cpp
        Common/include/FileWriter.h
        Server/src/Logger.cpp
        Server/include/Logger.h Common/src/Message.cpp Common/include/Message.h)

# Create executables
add_executable(ContainerBuilderServer ${SOURCE_FILES_SERVER})
add_executable(ContainerBuilderClient ${SOURCE_FILES_CLIENT})

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(ContainerBuilderServer ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(ContainerBuilderClient ${CMAKE_THREAD_LIBS_INIT})


find_package(Boost 1.64.0 COMPONENTS system coroutine filesystem serialization regex log log_setup thread REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})
target_link_libraries(ContainerBuilderServer ${Boost_LIBRARIES})
target_link_libraries(ContainerBuilderClient ${Boost_LIBRARIES})

install (TARGETS ContainerBuilderServer ContainerBuilderClient RUNTIME DESTINATION bin)