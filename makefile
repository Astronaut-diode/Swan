CC = g++
FLAG = -g -c

all: Swan

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

Swan: main.o Logger/LogStream.o Logger/AsyncLog.o Logger/AsyncLog.o Thread/Thread.o Logger/LogFile.o TaskScheduler/TaskScheduler.o TaskScheduler/TimerQueue.o TaskScheduler/Timer.o TcpConnection/TcpConnection.o Poller/Poller.o Channel/Channel.o
	$(CC) -g $^ -o $@ -lpthread -lmysqlclient -lhiredis

.PHONY:clean
clean:
	rm Swan
	rm -rf Other
	rm -rf `find . -name "*.o"`
	rm -rf `find . -name "core*"`
