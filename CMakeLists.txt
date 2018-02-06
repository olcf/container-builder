cmake_minimum_required(VERSION 3.8)
project(ContainerBuilder)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

SET(CMAKE_SKIP_INSTALL_ALL_DEPENDENCY TRUE)

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
        BuilderQueue/include/OpenStack.h
        Common/src/BuilderData.cpp
        Common/include/ClientData.h
        BuilderQueue/include/Server.h)

set(SOURCE_FILES_BUILDER
        Builder/src/main.cpp
        Common/include/BuilderData.h
        Common/src/Logger.cpp
        Common/include/Logger.h
        )

# Files related to the API
set(SOURCE_FILES_CLIENT
        Client/src/main.cpp
        Client/include/WaitingAnimation.h
        Common/include/BuilderData.h
        Common/src/Logger.cpp
        Common/include/Logger.h
        Common/src/BuilderData.cpp
        Common/include/ClientData.h)

# Scripts required for runtime
set(SCRIPTS
        Builder/scripts/create-builder.sh
        Builder/scripts/destroy-builder.sh
        Builder/scripts/singularity-builder-backend.sh
        Builder/scripts/docker-builder-backend.sh)

# Create executables
add_executable(builder-queue ${SOURCE_FILES_QUEUE})
add_executable(builder-server ${SOURCE_FILES_BUILDER})
add_executable(container-builder ${SOURCE_FILES_CLIENT})

# Ignore system boost and use module system boost
set(Boost_NO_BOOST_CMAKE TRUE)
#set(Boost_USE_STATIC_LIBS OFF)

# Add compile time hardening
set(HARDENING_FLAGS "-Wall -Wextra -fstack-protector-strong -D_FORTIFY_SOURCE=2 -fPIE -fpie -fno-omit-frame-pointer \
    -fsanitize=address,undefined -fno-sanitize=vptr")
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set(HARDENING_FLAGS "${HADENING_FLAGS} -Wl,-z,noexecstack,-z,now,-z,relro,-z,nodlopen")
endif()

set_target_properties(builder-queue PROPERTIES COMPILE_FLAGS "${COMPILE_FLAGS} ${HARDENING_FLAGS}")
set_target_properties(builder-queue PROPERTIES LINK_FLAGS "${LINK_FLAGS} ${HARDENING_FLAGS}")
set_target_properties(builder-server PROPERTIES COMPILE_FLAGS "${COMPILE_FLAGS} ${HARDENING_FLAGS}")
set_target_properties(builder-server PROPERTIES LINK_FLAGS "${LINK_FLAGS} ${HARDENING_FLAGS}")
set_target_properties(container-builder PROPERTIES COMPILE_FLAGS "${COMPILE_FLAGS} ${HARDENING_FLAGS}")
set_target_properties(container-builder PROPERTIES LINK_FLAGS "${LINK_FLAGS} ${HARDENING_FLAGS}")

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(builder-queue ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(builder-server ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(container-builder ${CMAKE_THREAD_LIBS_INIT})


find_package(Boost 1.66.0 COMPONENTS system filesystem serialization regex thread program_options REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})
target_link_libraries(builder-queue ${Boost_LIBRARIES})
target_link_libraries(builder-server ${Boost_LIBRARIES})
target_link_libraries(container-builder ${Boost_LIBRARIES})

# Install executables
install(TARGETS builder-queue COMPONENT builder-queue DESTINATION bin OPTIONAL)
install(TARGETS builder-server COMPONENT builder-server DESTINATION bin OPTIONAL)
install(TARGETS container-builder COMPONENT container-builder DESTINATION bin OPTIONAL)

# Install scripts if target the builder-server
install(FILES ${SCRIPTS}
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_READ
WORLD_EXECUTE DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" COMPONENT builder-server OPTIONAL)