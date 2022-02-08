#include "tasksys.h"

IRunnable::~IRunnable() = default;

ITaskSystem::ITaskSystem(int num_threads) {}

ITaskSystem::~ITaskSystem() = default;

/*
 * ================================================================
 * Serial task system implementation
 * ================================================================
 */

const char *TaskSystemSerial::name() { return "Serial"; }

TaskSystemSerial::TaskSystemSerial(int num_threads) : ITaskSystem(num_threads) {}

TaskSystemSerial::~TaskSystemSerial() = default;

void TaskSystemSerial::run(IRunnable *runnable, int num_total_tasks) {
	for (int i = 0; i < num_total_tasks; i++) runnable->runTask(i, num_total_tasks);
}

TaskID TaskSystemSerial::runAsyncWithDeps(IRunnable *runnable, int num_total_tasks, const std::vector<TaskID> &deps) {
	return 0;
}

void TaskSystemSerial::sync() {}

/*
 * ================================================================
 * Parallel Task System Implementation
 * ================================================================
 */

const char *TaskSystemParallelSpawn::name() { return "Parallel + Always Spawn"; }

TaskSystemParallelSpawn::TaskSystemParallelSpawn(int num_threads) : ITaskSystem(num_threads) {
	//
	// TODO: CS149 student implementations may decide to perform setup
	// operations (such as thread pool construction) here.
	// Implementations are free to add new class member variables
	// (requiring changes to tasksys.h).
	//
	numOfThread = num_threads;
}

TaskSystemParallelSpawn::~TaskSystemParallelSpawn() = default;

void TaskSystemParallelSpawn::run(IRunnable *runnable, int num_total_tasks) {
	//
	// TODO: CS149 students will modify the implementation of this
	// method in Part A.  The implementation provided below runs all
	// tasks sequentially on the calling thread.
	//
	std::atomic<int> taskId(0);
	std::thread threads[numOfThread];

	for (auto &thread: threads) {
		thread = std::thread([&taskId, num_total_tasks, runnable] {
			for (int id = taskId++; id < num_total_tasks; id = taskId++)
				runnable->runTask(id, num_total_tasks);
		});
	}
	for (auto &thread: threads) { thread.join(); }
}

TaskID TaskSystemParallelSpawn::runAsyncWithDeps(IRunnable *runnable, int num_total_tasks,
                                                 const std::vector<TaskID> &deps) {
	return 0;
}

void TaskSystemParallelSpawn::sync() {}

/*
 * ================================================================
 * Parallel Thread Pool Spinning Task System Implementation
 * ================================================================
 */

const char *TaskSystemParallelThreadPoolSpinning::name() { return "Parallel + Thread Pool + Spin"; }

TaskSystemParallelThreadPoolSpinning::TaskSystemParallelThreadPoolSpinning(int num_threads) : ITaskSystem(num_threads) {
	//
	// TODO: CS149 student implementations may decide to perform setup
	// operations (such as thread pool construction) here.
	// Implementations are free to add new class member variables
	// (requiring changes to tasksys.h).
	//
	exitFlag = false;
	for (int i = 0; i < num_threads; ++i) threads.emplace_back(&TaskSystemParallelThreadPoolSpinning::func, this);
}

TaskSystemParallelThreadPoolSpinning::~TaskSystemParallelThreadPoolSpinning() {
	exitFlag = true;
	for (auto &thread: threads) { thread.join(); }
}

void TaskSystemParallelThreadPoolSpinning::func() {
	int taskId;
	while (!exitFlag) {
		taskId = -1;
		queueMutex.lock();
		if (!taskQueue.empty()) {
			taskId = taskQueue.front();
			taskQueue.pop();
		}
		queueMutex.unlock();

		if (taskId != -1) {
			myRunnable->runTask(taskId, numTotalTasks);
			taskRemained--;
		}
	}
}

void TaskSystemParallelThreadPoolSpinning::run(IRunnable *runnable, int num_total_tasks) {
	//
	// TODO: CS149 students will modify the implementation of this
	// method in Part A.  The implementation provided below runs all
	// tasks sequentially on the calling thread.
	//
	myRunnable = runnable;
	taskRemained = num_total_tasks;
	numTotalTasks = num_total_tasks;
	for (int i = 0; i < num_total_tasks; i++) {
		queueMutex.lock();
		taskQueue.push(i);
		queueMutex.unlock();
	}
	while (taskRemained);
}

TaskID TaskSystemParallelThreadPoolSpinning::runAsyncWithDeps(IRunnable *runnable, int num_total_tasks,
                                                              const std::vector<TaskID> &deps) {
	return 0;
}

void TaskSystemParallelThreadPoolSpinning::sync() {}

/*
 * ================================================================
 * Parallel Thread Pool Sleeping Task System Implementation
 * ================================================================
 */

const char *TaskSystemParallelThreadPoolSleeping::name() { return "Parallel + Thread Pool + Sleep"; }


TaskSystemParallelThreadPoolSleeping::TaskSystemParallelThreadPoolSleeping(int
                                                                           num_threads) : ITaskSystem(num_threads) {
	//
	// TODO: CS149 student implementations may decide to perform setup
	// operations (such as thread pool construction) here.
	// Implementations are free to add new class member variables
	// (requiring changes to tasksys.h).
	//
	exitFlag = false;
	for (int i = 0; i < num_threads; ++i) { threads.emplace_back(&TaskSystemParallelThreadPoolSleeping::func, this); }
}

TaskSystemParallelThreadPoolSleeping::~TaskSystemParallelThreadPoolSleeping() {
	exitFlag = true;
	queueCond.notify_all();
	for (auto &thread: threads) { thread.join(); }
}

void TaskSystemParallelThreadPoolSleeping::func() {
	int taskId;
	while (true) {
		/*
		 * 如果所有线程都在work的时候有新task入队（尤其是最后一个task入队时），则工作线程都没有收到唤醒信号
		 * 避免这种情况，每次有线程完成任务，就唤醒其他所有等待的线程去检查队列有无task
		 */
		queueCond.notify_all();
		while (true) {
			std::unique_lock<std::mutex> lock(queueMutex);
			queueCond.wait(lock, [] { return true; });
			if (exitFlag) return;
			if (taskQueue.empty()) continue;
			taskId = taskQueue.front();
			taskQueue.pop();
			break;
		}
		myRunnable->runTask(taskId, numTotalTasks);
		taskRemained--;
		counterCond.notify_one();
	}
}

void TaskSystemParallelThreadPoolSleeping::run(IRunnable *runnable, int num_total_tasks) {
	//
	// TODO: CS149 students will modify the implementation of this
	// method in Parts A and B.  The implementation provided below runs all
	// tasks sequentially on the calling thread.
	//
	myRunnable = runnable;
	taskRemained = num_total_tasks;
	numTotalTasks = num_total_tasks;
	for (int i = 0; i < num_total_tasks; i++) {
		queueMutex.lock();
		taskQueue.push(i);
		queueMutex.unlock();
		queueCond.notify_all();
	}
	while (true) {
		std::unique_lock<std::mutex> lock(counterLock);
		counterCond.wait(lock, [] { return true; });
		if (!taskRemained) return;
	}
}

TaskID TaskSystemParallelThreadPoolSleeping::runAsyncWithDeps(IRunnable *runnable, int num_total_tasks,
                                                              const std::vector<TaskID> &deps) {
	//
	// TODO: CS149 students will implement this method in Part B.
	//
	return 0;
}

void TaskSystemParallelThreadPoolSleeping::sync() {
	//
	// TODO: CS149 students will modify the implementation of this method in Part B.
	//
}