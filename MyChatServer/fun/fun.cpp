#include "fun.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <iostream>
#include <cstring>
// 设置文件描述符为非阻塞
int set_nonblocking(int fd) {
	int flags = fcntl(fd, F_GETFL);
	return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

#define connfdET	//边缘触发非阻塞
//#define connfdLT	//水平触发阻塞

//#define listenfdET //边缘触发非阻塞
#define listenfdLT //水平触发阻塞


//向epoll实例中注册读事件，选择是否开启EPOLLONESHOT
void add_fd(int epollfd, int fd, bool one_shot, void* ptr)
{
	//std::cout << "[add_fd] epollfd = " << epollfd << ", fd = " << fd << std::endl;

	epoll_event event;

	event.data.ptr = ptr;
#ifdef connfdET
	event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
#endif

#ifdef connfdLT
	event.events = EPOLLIN | EPOLLRDHUP;
#endif

#ifdef listenfdET
	event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
#endif

#ifdef listenfdLT
	event.events = EPOLLIN | EPOLLRDHUP;
#endif

	if (one_shot)
		event.events |= EPOLLONESHOT;
	

	epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
	set_nonblocking(fd);
}
//从epoll实例中删除事件
void remove_fd(int epollfd, int fd)
{
	epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, 0);
	close(fd);
}


//修改事件
void mod_fd(int epollfd, int fd, int ev)
{

	epoll_event event;
	event.data.fd = fd;


#ifdef connfdET
	event.events = ev | EPOLLET | EPOLLONESHOT | EPOLLRDHUP;
#endif

#ifdef connfdLT
	event.events = ev | EPOLLONESHOT | EPOLLRDHUP;
#endif

	epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
	return;
}


