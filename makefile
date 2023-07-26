CC = g++
FLAG = -g -c

all: Swan

TcpConnection/Response.o: TcpConnection/Response.cpp
	$(CC) $(FLAG) $^ -o $@

TcpConnection/Request.o: TcpConnection/Request.cpp
	$(CC) $(FLAG) $^ -o $@

TcpConnection/Address.o: TcpConnection/Address.cpp
	$(CC) $(FLAG) $^ -o $@

Acceptor/Acceptor.o: Acceptor/Acceptor.cpp
	$(CC) $(FLAG) $^ -o $@

Monitor/Monitor.o: Monitor/Monitor.cpp
	$(CC) $(FLAG) $^ -o $@

TcpServer/TcpServer.o: TcpServer/TcpServer.cpp
	$(CC) $(FLAG) $^ -o $@

Channel/Channel.o: Channel/Channel.cpp
	$(CC) $(FLAG) $^ -o $@

Poller/Poller.o: Poller/Poller.cpp
	$(CC) $(FLAG) $^ -o $@

TcpConnection/TcpConnection.o: TcpConnection/TcpConnection.cpp
	$(CC) $(FLAG) $^ -o $@

TaskScheduler/Timer.o: TaskScheduler/Timer.cpp
	$(CC) $(FLAG) $^ -o $@

TaskScheduler/TimerQueue.o: TaskScheduler/TimerQueue.cpp
	$(CC) $(FLAG) $^ -o $@

TaskScheduler/TaskScheduler.o: TaskScheduler/TaskScheduler.cpp
	$(CC) $(FLAG) $^ -o $@

Logger/LogFile.o: Logger/LogFile.cpp
	$(CC) $(FLAG) $^ -o $@

Thread/Thread.o: Thread/Thread.cpp
	$(CC) $(FLAG) $^ -o $@

Logger/AsyncLog.o: Logger/AsyncLog.cpp
	$(CC) $(FLAG) $^ -o $@

Logger/LogStream.o: Logger/LogStream.cpp
	$(CC) $(FLAG) $^ -o $@

main.o: main.cpp
	$(CC) $(FLAG) $^ -o $@

Swan: main.o Logger/LogStream.o Logger/AsyncLog.o Logger/AsyncLog.o Thread/Thread.o Logger/LogFile.o TaskScheduler/TaskScheduler.o TaskScheduler/TimerQueue.o TaskScheduler/Timer.o TcpConnection/TcpConnection.o Poller/Poller.o Channel/Channel.o TcpConnection/Address.o Acceptor/Acceptor.o Monitor/Monitor.o TcpServer/TcpServer.o TcpConnection/Request.o TcpConnection/Response.o
	$(CC) -g $^ -o $@ -lpthread -lmysqlclient -lhiredis

.PHONY:clean
clean:
	rm -rf `find . -name "*.o"`
	rm -rf `find . -name "core*"`
	rm -rf Other
	rm Swan
