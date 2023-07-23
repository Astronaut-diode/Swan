cmake_minimum_required(VERSION 3.25)
project(Swan)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)

add_executable(Swan
        main.cpp
        Logger/FixedMemBlock.h
        Logger/AsyncLog.cpp Logger/AsyncLog.h
        Utils/Utils.h
        Thread/Thread.cpp Thread/Thread.h
        Logger/LogFile.cpp Logger/LogFile.h
        Logger/LogStream.cpp Logger/LogStream.h)

target_link_libraries(Swan pthread)