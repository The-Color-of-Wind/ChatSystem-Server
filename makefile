src1 = $(wildcard main.cpp ./conn/chat_conn.cpp ./conn/user.cpp ./conn/Server.cpp ./CGImysql/sql_connect_pool.cpp ./log/log.cpp)
obj1 = $(patsubst %.cpp, %.o, $(src1))

myArgs = -Wall -g -lpthread -I./conn -I/usr/include/jsoncpp -ljsoncpp -lmysqlclient

ALL:startserv

startserv:$(obj1)
	g++ $^ -o $@ $(myArgs)

%.o:%.cpp
	g++ -c $< -o $@ $(myArgs)

clean:
	-rm -rf serv $(obj1)

