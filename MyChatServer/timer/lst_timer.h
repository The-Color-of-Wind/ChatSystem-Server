#ifndef LST_TIMER
#define LST_TIMER

#include<time.h>
#include <netinet/in.h>

class util_timer;

//连接资源
struct client_data
{
	sockaddr_in address;
	int sockfd;
	util_timer *timer;
};

//定时器类
class util_timer
{
public:
	util_timer() :prev(NULL), next(NULL) { }

public:
	//超时事件
	time_t expire;
	//回调函数
	void(*cb_func) (client_data *);
	//连接资源
	client_data* user_data;
	//前向定时器
	util_timer* prev;
	//后继定时器
	util_timer* next;
};

//定时器容器类
class sort_timer_lst
{
public:
	sort_timer_lst() :head(NULL), tail(NULL) {}
	~sort_timer_lst()
	{
		util_timer* tmp = head;
		while (tmp)
		{
			head = tmp->next;
			delete tmp;
			tmp = head;
		}
	}

	//添加定时器，内部调用私有成员add-timer
	void add_timer(util_timer * timer)
	{
		if (!timer)
			return;
		if (!head) {
			head = tail = timer;
			return;
		}
		//按定时器超时时间升序排序
		//如果小于头结点
		if (timer->expire < head->expire) {
			timer->next = head;
			head->prev = timer;
			head = timer;
			return;
		}
		//否则调用私有成员，调整内部结点
		add_timer(timer, head);
	}

	//调整定时器，任务发生变化时，调整定时器在链表中的位置
	void adjust_timer(util_timer* timer)
	{
		if (!timer) {
			return;
		}
		util_timer *tmp = timer->next;

		//如果在尾部，或者仍小于下一个定时器的超时值，不调整
		if (!tmp || (timer->expire < tmp->expire))
			return;
		//被调整定时器是链表头结点，将定时器取出，重新插入
		if (timer == head) {
			head = head->next;
			head->prev = NULL;
			timer->next = NULL;
			add_timer(timer, head);
		}
		//在内部，取出重新插入
		else {
			timer->prev->next = timer->next;
			timer->next->prev = timer->prev;
			add_timer(timer, timer->next);
		}
	}

	//删除定时器
	void del_timer(util_timer *timer)
	{
		if (!timer)
			return;
		//链表中只有一个定时器，需要删除该定时器
		if ((timer == head) && (timer == tail)) {
			delete timer;
			head = NULL;
			tail = NULL;
			return;
		}
		//被删除的定时器为头结点
		if (timer == head) {
			head = head->next;
			head->prev = NULL;
			delete timer;
			return;
		}
		//被删除的定时器为尾结点
		if (timer == tail) {
			tail = tail->prev;
			tail->next = NULL;
			delete timer;
			return;
		}
		//被删除的定时器在链表内部，常规链表结点删除
		timer->prev->next = timer->next;
		timer->next->prev = timer->prev;
		delete timer;
	}

	//定时任务处理函数
	void tick()
	{
		if (!head)
			return;
		//获取当前时间
		time_t cur = time(NULL);
		util_timer *tmp = head;
		while (tmp)
		{
			//升序排列，如果当前小于定时器超时时间，后面必然没有到期（定时器expire是超时时间）
			if (cur < tmp->expire)
				break;
			//当前定时器到期，则调用回调函数，执行定时事件
			tmp->cb_func(tmp->user_data);
			//将处理后的定时器从容器中删除，并重置头结点
			head = tmp->next;
			if (head)
				head->prev = NULL;
			delete tmp;
			tmp = head;
		}
	}


private:
	//私有成员，被公有成员add_timer和adjust_time调用
	void add_timer(util_timer* timer, util_timer* lst_head)
	{
		util_timer* prev = lst_head;
		util_timer* tmp = prev->next;

		//遍历当前结点之后的链表，按照超时时间找到目标定时器对应的位置，常规双向链表插入操作
		while (tmp)
		{
			if (timer->expire < tmp->expire) {
				prev->next = timer;
				timer->next = tmp;
				tmp->prev = timer;
				timer->prev = prev;
				break;
			}
			prev = tmp;
			tmp = tmp->next;
		}
		//遍历完发现，目标定时器需要放到尾结点处
		if (!tmp) {
			prev->next = timer;
			timer->prev = prev;
			timer->next = NULL;
			tail = timer;
		}
	}


private:
	//头尾结点
	util_timer* head;
	util_timer* tail;
};

#endif