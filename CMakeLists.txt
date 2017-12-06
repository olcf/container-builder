cmake_minimum_required(VERSION 3.8)
project(ContainerBuilder)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(BuilderQueue/include Common/include)

set(SOURCE_FILES_QUEUE
        BuilderQueue/src/main.cpp
        BuilderQueue/src/Connection.cpp
        BuilderQueue/src/Reservation.cpp
        BuilderQueue/src/ReservationRequest.cpp
        BuilderQueue/src/BuilderQueue.cpp
        BuilderQueue/include/Connection.h
        BuilderQueue/include/Reservation.h
        BuilderQueue/include/ReservationRequest.h
        BuilderQueue/include/BuilderQueue.h
        Common/include/Builder.h
        Common/src/Logger.cpp
        Common/include/Logger.h
        Common/src/Messenger.cpp
        Common/include/Messenger.h
        BuilderQueue/src/OpenStackBuilder.cpp
        BuilderQueue/include/OpenStackBuilder.h)

set(SOURCE_FILES_BUILDER
        Builder/main_heartbeat.cpp
        Common/include/Builder.h
        Common/src/Logger.cpp
        Common/include/Logger.h
        Common/src/Messenger.cpp
        Common/include/Messenger.h)

# Files related to the API
set(SOURCE_FILES_CLIENT
        Client/main_heartbeat.cpp
        Common/include/Builder.h
        Common/src/Logger.cpp
        Common/include/Logger.h
        Common/src/Messenger.cpp
        Common/include/Messenger.h)

# Create executables
add_executable(BuilderQueue ${SOURCE_FILES_QUEUE})
add_executable(ContainerBuilder ${SOURCE_FILES_BUILDER})
add_executable(ContainerBuilderClient ${SOURCE_FILES_CLIENT})

set(HARDENING_FLAGS "-Wall -Wextra -fstack-protector-strong -D_FORTIFY_SOURCE=2 -fPIE -fpie -fno-omit-frame-pointer")
# https://github.com/boostorg/coroutine/issues/30
# -fsanitize=address,undefined -fno-sanitize=vptr -Wl,-z,noexecstack,-z,now,-z,relro,-z,nodlopen


set_target_properties(BuilderQueue PROPERTIES COMPILE_FLAGS "${COMPILE_FLAGS} ${HARDENING_FLAGS}")
set_target_properties(BuilderQueue
        PROPERTIES LINK_FLAGS "${LINK_FLAGS} ${HARDENING_FLAGS}")
set_target_properties(ContainerBuilder
        PROPERTIES COMPILE_FLAGS "${COMPILE_FLAGS} ${HARDENING_FLAGS}")
set_target_properties(ContainerBuilder
        PROPERTIES LINK_FLAGS "${LINK_FLAGS} ${HARDENING_FLAGS}")
set_target_properties(ContainerBuilderClient
        PROPERTIES COMPILE_FLAGS "${COMPILE_FLAGS} ${HARDENING_FLAGS}")
set_target_properties(ContainerBuilderClient
        PROPERTIES LINK_FLAGS "${LINK_FLAGS} ${HARDENING_FLAGS}")

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(BuilderQueue ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(ContainerBuilder ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(ContainerBuilderClient ${CMAKE_THREAD_LIBS_INIT})

find_package(Boost 1.64.0 COMPONENTS system coroutine filesystem serialization regex log log_setup thread REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})
target_link_libraries(BuilderQueue ${Boost_LIBRARIES})
target_link_libraries(ContainerBuilder ${Boost_LIBRARIES})
target_link_libraries(ContainerBuilderClient ${Boost_LIBRARIES})

install (TARGETS BuilderQueue ContainerBuilder ContainerBuilderClient RUNTIME DESTINATION bin)