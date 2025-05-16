#include "MySqlTaskQueue.h"

MySqlTaskQueue::MySqlTaskQueue();



void MySqlTaskQueue::addTask(std::string& task);

void MySqlTaskQueue::processTasks();

void MySqlTaskQueue::start();


void MySqlTaskQueue::executeDatabaseTask();