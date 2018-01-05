cmake_minimum_required(VERSION 3.8)
project(ContainerBuilder)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(BuilderQueue/include Builder/include Client/include Common/include)

set(SOURCE_FILES_QUEUE
        BuilderQueue/src/main.cpp
        BuilderQueue/src/Connection.cpp
        BuilderQueue/src/BuilderQueue.cpp
        BuilderQueue/include/Connection.h
        BuilderQueue/include/BuilderQueue.h
        Common/include/BuilderData.h
        Common/src/Logger.cpp
        Common/include/Logger.h
        Common/include/Messenger.h
        BuilderQueue/include/OpenStack.h
        Common/src/BuilderData.cpp
        Common/include/ClientData.h BuilderQueue/include/Server.h)

set(SOURCE_FILES_BUILDER
        Builder/src/main.cpp
        Common/include/BuilderData.h
        Common/src/Logger.cpp
        Common/include/Logger.h
        )

# Files related to the API
set(SOURCE_FILES_CLIENT
        Client/src/main.cpp
        Common/include/BuilderData.h
        Common/src/Logger.cpp
        Common/include/Logger.h
        Common/src/BuilderData.cpp
        Common/include/ClientData.h)

# Scripts required for runtime
set(SCRIPTS
        Scripts/RequestCreateBuilder
        Scripts/GetBuilders
        Scripts/DestroyBuilder)

# Create executables
add_executable(builder_queue ${SOURCE_FILES_QUEUE})
add_executable(builder_server ${SOURCE_FILES_BUILDER})
add_executable(container_builder ${SOURCE_FILES_CLIENT})

# Ignore system boost and use module system boost
set(Boost_NO_BOOST_CMAKE TRUE)
#set(Boost_USE_STATIC_LIBS OFF)

# ignore coroutines deprecation
add_definitions(-DBOOST_COROUTINES_NO_DEPRECATION_WARNING)

set(HARDENING_FLAGS "-Wall -Wextra -fstack-protector-strong -D_FORTIFY_SOURCE=2 -fPIE -fpie -fno-omit-frame-pointer")
# https://github.com/boostorg/coroutine/issues/30
# -fsanitize=address,undefined -fno-sanitize=vptr -Wl,-z,noexecstack,-z,now,-z,relro,-z,nodlopen

set_target_properties(builder_queue PROPERTIES COMPILE_FLAGS "${COMPILE_FLAGS} ${HARDENING_FLAGS}")
set_target_properties(builder_queue PROPERTIES LINK_FLAGS "${LINK_FLAGS} ${HARDENING_FLAGS}")
set_target_properties(builder_server PROPERTIES COMPILE_FLAGS "${COMPILE_FLAGS} ${HARDENING_FLAGS}")
set_target_properties(builder_server PROPERTIES LINK_FLAGS "${LINK_FLAGS} ${HARDENING_FLAGS}")
set_target_properties(container_builder PROPERTIES COMPILE_FLAGS "${COMPILE_FLAGS} ${HARDENING_FLAGS}")
set_target_properties(container_builder PROPERTIES LINK_FLAGS "${LINK_FLAGS} ${HARDENING_FLAGS}")

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(builder_queue ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(builder_server ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(container_builder ${CMAKE_THREAD_LIBS_INIT})


find_package(Boost 1.66.0 COMPONENTS system coroutine filesystem serialization regex thread program_options REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})
target_link_libraries(builder_queue ${Boost_LIBRARIES})
target_link_libraries(builder_server ${Boost_LIBRARIES})
target_link_libraries(container_builder ${Boost_LIBRARIES})

install(TARGETS builder_queue builder_server container_builder RUNTIME DESTINATION bin)

# Install scripts
install(FILES ${SCRIPTS}
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_READ
WORLD_EXECUTE DESTINATION "${CMAKE_INSTALL_PREFIX}/bin")