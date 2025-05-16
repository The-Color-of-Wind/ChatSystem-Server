#include "fun.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <iostream>
#include <cstring>
// �����ļ�������Ϊ������
int set_nonblocking(int fd) {
	int flags = fcntl(fd, F_GETFL);
	return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

#define connfdET	//��Ե����������
//#define connfdLT	//ˮƽ��������

//#define listenfdET //��Ե����������
#define listenfdLT //ˮƽ��������


//��epollʵ����ע����¼���ѡ���Ƿ���EPOLLONESHOT
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
//��epollʵ����ɾ���¼�
void remove_fd(int epollfd, int fd)
{
	epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, 0);
	close(fd);
}


//�޸��¼�
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


