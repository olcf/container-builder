cmake_minimum_required(VERSION 3.8)
project(ContainerBuilder)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wall -Wextra -fstack-protector-strong -D_FORTIFY_SOURCE=2 -fPIE -fpie")# -Wl,-z,noexecstack,-z,now,-z,relro,-z,nodlopen")

include_directories(ResourceQueue/include Common/include)

# Files related to the resource queue server
set(SOURCE_FILES_QUEUE
        ResourceQueue/src/main.cpp
        ResourceQueue/src/Connection.cpp
        ResourceQueue/src/Reservation.cpp
        ResourceQueue/src/ReservationRequest.cpp
        ResourceQueue/src/BuilderQueue.cpp
        ResourceQueue/include/Connection.h
        ResourceQueue/include/Reservation.h
        ResourceQueue/include/ReservationRequest.h
        ResourceQueue/include/BuilderQueue.h
        Common/include/Builder.h
        Common/src/Logger.cpp
        Common/include/Logger.h
        Common/src/Messenger.cpp
        Common/include/Messenger.h
        ResourceQueue/src/OpenStackBuilder.cpp
        ResourceQueue/include/OpenStackBuilder.h)

set(SOURCE_FILES_BUILDER
        Builder/main.cpp
        Common/include/Builder.h
        Common/src/Logger.cpp
        Common/include/Logger.h
        Common/src/Messenger.cpp
        Common/include/Messenger.h)

# Files related to the API
set(SOURCE_FILES_CLIENT Client/main.cpp
        Common/include/Builder.h
        Common/src/Logger.cpp
        Common/include/Logger.h
        Common/src/Messenger.cpp
        Common/include/Messenger.h)

# Create executables
add_executable(ResourceQueue ${SOURCE_FILES_QUEUE})
add_executable(ContainerBuilder ${SOURCE_FILES_BUILDER})
add_executable(ContainerBuilderClient ${SOURCE_FILES_CLIENT})

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(ResourceQueue ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(ContainerBuilder ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(ContainerBuilderClient ${CMAKE_THREAD_LIBS_INIT})

find_package(Boost 1.64.0 COMPONENTS system coroutine filesystem serialization regex log log_setup thread REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})
target_link_libraries(ResourceQueue ${Boost_LIBRARIES})
target_link_libraries(ContainerBuilder ${Boost_LIBRARIES})
target_link_libraries(ContainerBuilderClient ${Boost_LIBRARIES})

install (TARGETS ResourceQueue ContainerBuilder ContainerBuilderClient RUNTIME DESTINATION bin)