#pragma once

#include "blockingconcurrentqueue.h"
#include "utils.h"
#include "thread.h"
#include <vector>

typedef void(*PGThreadFunctionParams)(void*);

enum PGTaskUrgency {
	PGTaskUrgent,
	PGTaskNotUrgent
};

struct Task {
	PGThreadFunctionParams function;
	void* parameter;
	bool active;

	Task(PGThreadFunctionParams function, void* parameter) : function(function), parameter(parameter), active(true) { }
};

class Scheduler {
public:
	static void Initialize() { GetInstance(); }

	static void SetThreadCount(ssize_t count) { GetInstance()._SetThreadCount(count); }
	static ssize_t GetThreadCount() { return GetInstance().threads.size(); }

	static bool IsRunning() { return GetInstance().running; }

	static void RegisterTask(Task* task, PGTaskUrgency urgency) { GetInstance()._RegisterTask(task, urgency); }
private:
	Scheduler();
	void _SetThreadCount(ssize_t threads);
	void _RegisterTask(Task* task, PGTaskUrgency);
	static void RunThread(void);
	static Scheduler& GetInstance()
	{
		static Scheduler instance;
		return instance;
	}

	bool running = true;
	moodycamel::BlockingConcurrentQueue<Task*> urgent_queue;
	moodycamel::ConcurrentQueue<Task*> nonurgent_queue;
	std::vector<PGThreadHandle> threads;
};