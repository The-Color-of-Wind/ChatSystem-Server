#include "ChatMapping.h"

// ��ӻ����ӳ��

void ChatMapping::addUserSocket(const string& userId, int subReactorId, chat_conn* conn) {

	unique_lock<shared_mutex> lock(map_rw_mutex);

	userSocketMap[userId] = make_pair(subReactorId, conn);

}

// ���� userId ��Ӧ�� socket
optional<pair<int, chat_conn*>> ChatMapping::getSocketByUserId(const string& userId) {
	shared_lock<shared_mutex> lock(map_rw_mutex);
	auto it = userSocketMap.find(userId);
	if (it != userSocketMap.end()) {
		int subReactorId = it->second.first;
		chat_conn* conn = it->second.second;
		return make_pair(subReactorId, conn);
	}
	return nullopt; // ���� -1 ��ʾû���ҵ���Ӧ�� socket
}

void ChatMapping::removeUserSocket(const string& userId)
{
	unique_lock<shared_mutex> lock(map_rw_mutex);
	userSocketMap.erase(userId);
	return;
}