#ifndef _TASKSYS_H
#define _TASKSYS_H

#include "itasksys.h"
#include <thread>
#include <atomic>
#include <queue>
#include <mutex>
#include <condition_variable>

/*
 * TaskSystemSerial: This class is the student's implementation of a
 * serial task execution engine.  See definition of ITaskSystem in
 * itasksys.h for documentation of the ITaskSystem interface.
 */
class TaskSystemSerial : public ITaskSystem {
public:
	explicit TaskSystemSerial(int num_threads);

	~TaskSystemSerial() override;

	const char *name() override;

	void run(IRunnable *runnable, int num_total_tasks) override;

	TaskID runAsyncWithDeps(IRunnable *runnable, int num_total_tasks, const std::vector<TaskID> &deps) override;

	void sync() override;
};


/*
 * TaskSystemParallelSpawn: This class is the student's implementation of a
 * parallel task execution engine that spawns threads in every run()
 * call.  See definition of ITaskSystem in itasksys.h for documentation
 * of the ITaskSystem interface.
 */
class TaskSystemParallelSpawn : public ITaskSystem {
public:
	explicit TaskSystemParallelSpawn(int num_threads);

	~TaskSystemParallelSpawn() override;

	const char *name() override;

	void run(IRunnable *runnable, int num_total_tasks) override;

	TaskID runAsyncWithDeps(IRunnable *runnable, int num_total_tasks, const std::vector<TaskID> &deps) override;

	void sync() override;

private:
	int numOfThread;
};


/*
 * TaskSystemParallelThreadPoolSpinning: This class is the student's
 * implementation of a parallel task execution engine that uses a
 * thread pool. See definition of ITaskSystem in itasksys.h for
 * documentation of the ITaskSystem interface.
 */
class TaskSystemParallelThreadPoolSpinning : public ITaskSystem {
public:
	explicit TaskSystemParallelThreadPoolSpinning(int num_threads);

	~TaskSystemParallelThreadPoolSpinning() override;

	const char *name() override;

	void run(IRunnable *runnable, int num_total_tasks) override;

	TaskID runAsyncWithDeps(IRunnable *runnable, int num_total_tasks, const std::vector<TaskID> &deps) override;

	void sync() override;

private:
	std::vector<std::thread> threads;
	std::mutex queueMutex;
	std::atomic<int> taskRemained{};
	std::queue<int> taskQueue;
	IRunnable *myRunnable{};
	bool exitFlag;
	int numTotalTasks{};

	void func();
};


/*
 * TaskSystemParallelThreadPoolSleeping: This class is the student's
 * optimized implementation of a parallel task execution engine that uses
 * a thread pool. See definition of ITaskSystem in
 * itasksys.h for documentation of the ITaskSystem interface.
 */
class TaskSystemParallelThreadPoolSleeping : public ITaskSystem {
public:
	explicit TaskSystemParallelThreadPoolSleeping(int num_threads);

	~TaskSystemParallelThreadPoolSleeping() override;

	const char *name() override;

	void run(IRunnable *runnable, int num_total_tasks) override;

	TaskID runAsyncWithDeps(IRunnable *runnable, int num_total_tasks, const std::vector<TaskID> &deps) override;

	void sync() override;

private:
	std::vector<std::thread> threads;
	std::mutex counterLock;
	std::condition_variable counterCond;
	std::mutex queueMutex;
	std::condition_variable queueCond;
	std::queue<int> taskQueue{};
	IRunnable *myRunnable{};
	std::atomic<int> taskRemained{};
	int numTotalTasks{};
	bool exitFlag;
	void func();
};

#endif
