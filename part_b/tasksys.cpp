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

TaskSystemSerial::TaskSystemSerial(int num_threads) : ITaskSystem(num_threads) {
}

TaskSystemSerial::~TaskSystemSerial() = default;

void TaskSystemSerial::run(IRunnable *runnable, int num_total_tasks) {
	for (int i = 0; i < num_total_tasks; i++) { runnable->runTask(i, num_total_tasks); }
}

TaskID TaskSystemSerial::runAsyncWithDeps(IRunnable *runnable, int num_total_tasks,
                                          const std::vector<TaskID> &deps) {
	for (int i = 0; i < num_total_tasks; i++) {
		runnable->runTask(i, num_total_tasks);
	}

	return 0;
}

void TaskSystemSerial::sync() {}

/*
 * ================================================================
 * Parallel Task System Implementation
 * ================================================================
 */

const char *TaskSystemParallelSpawn::name() {
	return "Parallel + Always Spawn";
}

TaskSystemParallelSpawn::TaskSystemParallelSpawn(int num_threads) : ITaskSystem(num_threads) {
	// NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
}

TaskSystemParallelSpawn::~TaskSystemParallelSpawn() = default;

void TaskSystemParallelSpawn::run(IRunnable *runnable, int num_total_tasks) {
	// NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
	for (int i = 0; i < num_total_tasks; i++) {
		runnable->runTask(i, num_total_tasks);
	}
}

TaskID TaskSystemParallelSpawn::runAsyncWithDeps(IRunnable *runnable, int num_total_tasks,
                                                 const std::vector<TaskID> &deps) {
	// NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
	for (int i = 0; i < num_total_tasks; i++) {
		runnable->runTask(i, num_total_tasks);
	}

	return 0;
}

void TaskSystemParallelSpawn::sync() {
	// NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
}

/*
 * ================================================================
 * Parallel Thread Pool Spinning Task System Implementation
 * ================================================================
 */

const char *TaskSystemParallelThreadPoolSpinning::name() {
	return "Parallel + Thread Pool + Spin";
}

TaskSystemParallelThreadPoolSpinning::TaskSystemParallelThreadPoolSpinning(int num_threads) : ITaskSystem(num_threads) {
	// NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
}

TaskSystemParallelThreadPoolSpinning::~TaskSystemParallelThreadPoolSpinning() = default;

void TaskSystemParallelThreadPoolSpinning::run(IRunnable *runnable, int num_total_tasks) {
	// NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
	for (int i = 0; i < num_total_tasks; i++) {
		runnable->runTask(i, num_total_tasks);
	}
}

TaskID TaskSystemParallelThreadPoolSpinning::runAsyncWithDeps(IRunnable *runnable, int num_total_tasks,
                                                              const std::vector<TaskID> &deps) {
	// NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
	for (int i = 0; i < num_total_tasks; i++) {
		runnable->runTask(i, num_total_tasks);
	}

	return 0;
}

void TaskSystemParallelThreadPoolSpinning::sync() {
	// NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
}

/*
 * ================================================================
 * Parallel Thread Pool Sleeping Task System Implementation
 * ================================================================
 */

const char *TaskSystemParallelThreadPoolSleeping::name() { return "Parallel + Thread Pool + Sleep"; }

TaskSystemParallelThreadPoolSleeping::TaskSystemParallelThreadPoolSleeping(int num_threads) : ITaskSystem(num_threads) {
	//
	// TODO: CS149 student implementations may decide to perform setup
	// operations (such as thread pool construction) here.
	// Implementations are free to add new class member variables
	// (requiring changes to tasksys.h).
	//
	numGroup = 0;
	exitFlag = false;
	for (int i = 0; i < num_threads; ++i) { threads.emplace_back(&TaskSystemParallelThreadPoolSleeping::func, this); }
}

TaskSystemParallelThreadPoolSleeping::~TaskSystemParallelThreadPoolSleeping() {
	//
	// TODO: CS149 student implementations may decide to perform cleanup
	// operations (such as thread pool shutdown construction) here.
	// Implementations are free to add new class member variables
	// (requiring changes to tasksys.h).
	//
	exitFlag = true;
	queueCond.notify_all();
	for (auto &thread: threads) { thread.join(); }
}

void TaskSystemParallelThreadPoolSleeping::func() {
	RunnableTask *task;
	TaskGroup *taskBelongTo;
	while (true) {
		while (true) {
			std::unique_lock<std::mutex> lock(queueMutex);
			queueCond.wait(lock, [] { return true; });
			if (exitFlag) return;
			if (taskQueue.empty()) continue;
			task = taskQueue.front();
			taskQueue.pop();
			break;
		}
		taskBelongTo = task->belongTo;
		taskBelongTo->runnable->runTask(task->id, taskBelongTo->numTotalTasks);
		taskBelongTo->taskRemained--;
		if (taskBelongTo->taskRemained <= 0) {
			for (auto taskGroup: taskGroupSet) { taskGroup->depending.erase(taskBelongTo->groupId); }
			counterCond.notify_one();
		} else queueCond.notify_all();
	}
}

void TaskSystemParallelThreadPoolSleeping::run(IRunnable *runnable, int num_total_tasks) {
	//
	// TODO: CS149 students will modify the implementation of this
	// method in Parts A and B.  The implementation provided below runs all
	// tasks sequentially on the calling thread.
	//
	runAsyncWithDeps(runnable, num_total_tasks, {});
	sync();
}

TaskID TaskSystemParallelThreadPoolSleeping::runAsyncWithDeps(IRunnable *runnable, int num_total_tasks,
                                                              const std::vector<TaskID> &deps) {
	//
	// TODO: CS149 students will implement this method in Part B.
	//
	auto newTaskGroup = new TaskGroup(numGroup, runnable, num_total_tasks, deps);
	taskGroupQueue.push(newTaskGroup);
	taskGroupSet.insert(newTaskGroup);
	return numGroup++;
}

void TaskSystemParallelThreadPoolSleeping::sync() {
	//
	// TODO: CS149 students will modify the implementation of this method in Part B.
	//
	TaskGroup *nowTaskGroup;
	RunnableTask *nowRunnableTask;
	while (!taskGroupQueue.empty()) {
		nowTaskGroup = taskGroupQueue.top();
		if (!nowTaskGroup->depending.empty()) continue;
		queueMutex.lock();
		for (int i = 0; i < nowTaskGroup->numTotalTasks; i++) {
			nowRunnableTask = new RunnableTask(nowTaskGroup, i);
			taskQueue.push(nowRunnableTask);
		}
		queueMutex.unlock();
		queueCond.notify_all();
		taskGroupQueue.pop();
	}
	while (true) {
		std::unique_lock<std::mutex> lock(counterLock);
		counterCond.wait(lock, [] { return true; });
		finishFlag = true;
		for (auto taskGroup: taskGroupSet) {
			if (taskGroup->taskRemained > 0) {
				finishFlag = false;
				break;
			}
		}
		if (finishFlag) return;
	}
}
