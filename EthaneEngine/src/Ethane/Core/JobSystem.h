#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

namespace Ethane {

	// Tracks submitted work: done when every job counted in it has finished
	class JobHandle
	{
	public:
		JobHandle() = default;
		bool IsValid() const { return m_Pending != nullptr; }
		bool IsDone() const { return !m_Pending || m_Pending->load(std::memory_order_acquire) == 0; }

	private:
		explicit JobHandle(std::shared_ptr<std::atomic<uint32_t>> pending) : m_Pending(std::move(pending)) {}

		std::shared_ptr<std::atomic<uint32_t>> m_Pending;
		friend class JobSystem;
	};

	// A pool of worker threads running submitted jobs, first in, first out.
	//
	//     JobHandle handle = jobs.Submit([] { DecodeImage(); });
	//     ...
	//     jobs.Wait(handle);
	//
	//     jobs.ParallelFor(items.size(), 64, [&](uint32_t begin, uint32_t end) { ... });
	//
	// A thread waiting for a job runs queued jobs meanwhile, so jobs may wait for other jobs (and a
	// pool with no workers still makes progress: Wait runs the jobs itself). Jobs must not throw; an
	// escaping exception is logged and the job counts as finished. Submit / Wait from any thread.
	class JobSystem
	{
	public:
		// `workers`: worker threads to start; 0 means one per hardware thread, minus one for the caller
		explicit JobSystem(uint32_t workers = 0);
		// Runs the jobs still queued, then stops the workers
		~JobSystem();
		JobSystem(const JobSystem&) = delete;
		JobSystem& operator=(const JobSystem&) = delete;

		JobHandle Submit(std::function<void()> job);
		void Wait(const JobHandle& handle);

		// Calls `body(begin, end)` for consecutive ranges of at most `batch` indices covering [0, count),
		// in parallel (the caller takes part); returns when all are done
		void ParallelFor(uint32_t count, uint32_t batch, const std::function<void(uint32_t begin, uint32_t end)>& body);

		uint32_t WorkerCount() const { return static_cast<uint32_t>(m_Workers.size()); }

	private:
		struct Job
		{
			std::function<void()> Work;
			std::shared_ptr<std::atomic<uint32_t>> Pending;
		};

		void Enqueue(Job job);
		bool RunOne(); // runs a queued job if there is one
		void Run(Job& job);
		void WorkerLoop();

		std::mutex m_Mutex;
		std::condition_variable m_WorkAvailable; // a job was queued, or stopping
		std::condition_variable m_JobFinished;   // for Wait
		std::deque<Job> m_Queue;
		bool m_Stopping = false;
		std::vector<std::thread> m_Workers;
	};

}
