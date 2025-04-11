#ifndef LST_TIMER
#define LST_TIMER

#include<time.h>
#include <netinet/in.h>

class util_timer;

//������Դ
struct client_data
{
	sockaddr_in address;
	int sockfd;
	util_timer *timer;
};

//��ʱ����
class util_timer
{
public:
	util_timer() :prev(NULL), next(NULL) { }

public:
	//��ʱ�¼�
	time_t expire;
	//�ص�����
	void(*cb_func) (client_data *);
	//������Դ
	client_data* user_data;
	//ǰ��ʱ��
	util_timer* prev;
	//��̶�ʱ��
	util_timer* next;
};

//��ʱ��������
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

	//��Ӷ�ʱ�����ڲ�����˽�г�Աadd-timer
	void add_timer(util_timer * timer)
	{
		if (!timer)
			return;
		if (!head) {
			head = tail = timer;
			return;
		}
		//����ʱ����ʱʱ����������
		//���С��ͷ���
		if (timer->expire < head->expire) {
			timer->next = head;
			head->prev = timer;
			head = timer;
			return;
		}
		//�������˽�г�Ա�������ڲ����
		add_timer(timer, head);
	}

	//������ʱ�����������仯ʱ��������ʱ���������е�λ��
	void adjust_timer(util_timer* timer)
	{
		if (!timer) {
			return;
		}
		util_timer *tmp = timer->next;

		//�����β����������С����һ����ʱ���ĳ�ʱֵ��������
		if (!tmp || (timer->expire < tmp->expire))
			return;
		//��������ʱ��������ͷ��㣬����ʱ��ȡ�������²���
		if (timer == head) {
			head = head->next;
			head->prev = NULL;
			timer->next = NULL;
			add_timer(timer, head);
		}
		//���ڲ���ȡ�����²���
		else {
			timer->prev->next = timer->next;
			timer->next->prev = timer->prev;
			add_timer(timer, timer->next);
		}
	}

	//ɾ����ʱ��
	void del_timer(util_timer *timer)
	{
		if (!timer)
			return;
		//������ֻ��һ����ʱ������Ҫɾ���ö�ʱ��
		if ((timer == head) && (timer == tail)) {
			delete timer;
			head = NULL;
			tail = NULL;
			return;
		}
		//��ɾ���Ķ�ʱ��Ϊͷ���
		if (timer == head) {
			head = head->next;
			head->prev = NULL;
			delete timer;
			return;
		}
		//��ɾ���Ķ�ʱ��Ϊβ���
		if (timer == tail) {
			tail = tail->prev;
			tail->next = NULL;
			delete timer;
			return;
		}
		//��ɾ���Ķ�ʱ���������ڲ�������������ɾ��
		timer->prev->next = timer->next;
		timer->next->prev = timer->prev;
		delete timer;
	}

	//��ʱ��������
	void tick()
	{
		if (!head)
			return;
		//��ȡ��ǰʱ��
		time_t cur = time(NULL);
		util_timer *tmp = head;
		while (tmp)
		{
			//�������У������ǰС�ڶ�ʱ����ʱʱ�䣬�����Ȼû�е��ڣ���ʱ��expire�ǳ�ʱʱ�䣩
			if (cur < tmp->expire)
				break;
			//��ǰ��ʱ�����ڣ�����ûص�������ִ�ж�ʱ�¼�
			tmp->cb_func(tmp->user_data);
			//�������Ķ�ʱ����������ɾ����������ͷ���
			head = tmp->next;
			if (head)
				head->prev = NULL;
			delete tmp;
			tmp = head;
		}
	}


private:
	//˽�г�Ա�������г�Աadd_timer��adjust_time����
	void add_timer(util_timer* timer, util_timer* lst_head)
	{
		util_timer* prev = lst_head;
		util_timer* tmp = prev->next;

		//������ǰ���֮����������ճ�ʱʱ���ҵ�Ŀ�궨ʱ����Ӧ��λ�ã�����˫������������
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
		//�����귢�֣�Ŀ�궨ʱ����Ҫ�ŵ�β��㴦
		if (!tmp) {
			prev->next = timer;
			timer->prev = prev;
			timer->next = NULL;
			tail = timer;
		}
	}


private:
	//ͷβ���
	util_timer* head;
	util_timer* tail;
};

#endif