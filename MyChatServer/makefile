src1 = $(wildcard main.cpp ./ChatConn/chat_conn.cpp ./ChatConn/user.cpp ./MysqlConnectPool/sql_connect_pool.cpp ./log/log.cpp ./ChatMapping/ChatMapping.cpp ./ChatServer/ChatServer.cpp ./IniConfig/IniConfig.cpp)
obj1 = $(patsubst %.cpp, %.o, $(src1))

myArgs = -Wall -g -lpthread -I./conn -I/usr/include/jsoncpp -ljsoncpp -lmysqlclient

ALL:startserv

startserv:$(obj1)
	g++ $^ -o $@ $(myArgs)

%.o:%.cpp
	g++ -c $< -o $@ $(myArgs)

clean:
	-rm -rf serv $(obj1)

