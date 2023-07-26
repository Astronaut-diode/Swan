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
        Logger/LogStream.cpp Logger/LogStream.h
        TaskScheduler/TaskScheduler.cpp TaskScheduler/TaskScheduler.h
        TaskScheduler/TimerQueue.cpp TaskScheduler/TimerQueue.h
        TaskScheduler/Timer.cpp TaskScheduler/Timer.h
        TaskScheduler/CircleBuffer.h
        TcpConnection/TcpConnection.cpp TcpConnection/TcpConnection.h
        Poller/Poller.cpp Poller/Poller.h
        Channel/Channel.cpp Channel/Channel.h
        TcpServer/TcpServer.cpp TcpServer/TcpServer.h
        Monitor/Monitor.cpp Monitor/Monitor.h Acceptor/Acceptor.cpp Acceptor/Acceptor.h TcpConnection/Address.cpp TcpConnection/Address.h TcpConnection/Request.cpp TcpConnection/Request.h TcpConnection/Response.cpp TcpConnection/Response.h)

target_link_libraries(Swan pthread)