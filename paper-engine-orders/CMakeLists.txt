cmake_minimum_required(VERSION 3.10)
project(hft_paper_engine_orders)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(PkgConfig REQUIRED)
pkg_check_modules(PQXX REQUIRED libpqxx)
pkg_check_modules(PQ REQUIRED libpq)
find_package(nlohmann_json REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(CURL REQUIRED)
find_package(Threads REQUIRED)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Source files
set(SOURCES
    src/main.cpp
    src/PaperEngineOrders.cpp
    src/persistance/DatabaseSource.cpp
    src/persistance/DatabaseTarget.cpp
    src/queries/OrdersQueries.cpp
)

# Create executable
add_executable(paper-engine-orders ${SOURCES})

# Link libraries
target_link_libraries(paper-engine-orders
    PRIVATE
        ${PQXX_LIBRARIES}
        ${PQ_LIBRARIES}
        nlohmann_json::nlohmann_json
        OpenSSL::SSL
        OpenSSL::Crypto
        CURL::libcurl
        Threads::Threads
)

# Add include directories and compiler flags from pkgconfig
target_include_directories(paper-engine-orders PRIVATE ${PQXX_INCLUDE_DIRS} ${PQ_INCLUDE_DIRS})
target_compile_options(paper-engine-orders PRIVATE ${PQXX_CFLAGS_OTHER} ${PQ_CFLAGS_OTHER})

# Installation
install(TARGETS paper-engine-orders DESTINATION bin)
