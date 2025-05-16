#include "ChatMapping.h"

// 添加或更新映射

void ChatMapping::addUserSocket(const string& userId, int subReactorId, chat_conn* conn) {

	unique_lock<shared_mutex> lock(map_rw_mutex);

	userSocketMap[userId] = make_pair(subReactorId, conn);

}

// 查找 userId 对应的 socket
optional<pair<int, chat_conn*>> ChatMapping::getSocketByUserId(const string& userId) {
	shared_lock<shared_mutex> lock(map_rw_mutex);
	auto it = userSocketMap.find(userId);
	if (it != userSocketMap.end()) {
		int subReactorId = it->second.first;
		chat_conn* conn = it->second.second;
		return make_pair(subReactorId, conn);
	}
	return nullopt; // 返回 -1 表示没有找到对应的 socket
}

void ChatMapping::removeUserSocket(const string& userId)
{
	unique_lock<shared_mutex> lock(map_rw_mutex);
	userSocketMap.erase(userId);
	return;
}