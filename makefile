CC = g++
FLAG = -g -c

all: Swan


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

Swan: main.o Logger/LogStream.o Logger/AsyncLog.o Logger/AsyncLog.o Thread/Thread.o Logger/LogFile.o
	$(CC) -g $^ -o $@ -lpthread -lmysqlclient -lhiredis

.PHONY:clean
clean:
	rm Swan
	rm -rf Other
	rm -rf `find . -name "*.o"`
	rm -rf `find . -name "core*"`
