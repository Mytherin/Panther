
#include "scheduler.h"


Scheduler::Scheduler() {
}

void Scheduler::RunThread() {
	Scheduler& scheduler = Scheduler::GetInstance();
	while (scheduler.running) {
		std::shared_ptr<Task> task = nullptr;
		if (!scheduler.urgent_queue.wait_dequeue_timed(task, std::chrono::milliseconds(50))) {
			// nothing found
			if (!scheduler.nonurgent_queue.try_dequeue(task)) {
				task = nullptr;
			}
		}
		if (task) {
			// task found, run the task
			if (task->active) {
				task->function(task, task->parameter);
			}
		}
	}
}

void Scheduler::_SetThreadCount(lng threads) {
	// FIXME: removing threads is not supported right now
	assert(threads > this->threads.size());
	for (lng current_threads = this->threads.size(); current_threads <= threads; current_threads++) {
		PGThreadHandle handle = CreateThread(RunThread);
		this->threads.push_back(handle);
	}
}

void Scheduler::_RegisterTask(std::shared_ptr<Task> task, PGTaskUrgency urgency) {
	if (urgency == PGTaskUrgent) {
		urgent_queue.enqueue(task);
	} else {
		nonurgent_queue.enqueue(task);
	}
}
