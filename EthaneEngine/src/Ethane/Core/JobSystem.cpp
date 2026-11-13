#include "ethpch.h"
#include "Ethane/Core/JobSystem.h"

namespace Ethane {

	JobSystem::JobSystem(uint32_t workers)
	{
		if (workers == 0)
		{
			const uint32_t hardware = std::thread::hardware_concurrency();
			workers = hardware > 1 ? hardware - 1 : 1;
		}
		m_Workers.reserve(workers);
		for (uint32_t i = 0; i < workers; i++)
			m_Workers.emplace_back([this] { WorkerLoop(); });
	}

	JobSystem::~JobSystem()
	{
		{
			std::lock_guard lock(m_Mutex);
			m_Stopping = true;
		}
		m_WorkAvailable.notify_all();
		for (std::thread& worker : m_Workers)
			worker.join();
		while (RunOne()) {} // with no workers, or jobs queued while stopping
	}

	JobHandle JobSystem::Submit(std::function<void()> job)
	{
		auto pending = std::make_shared<std::atomic<uint32_t>>(1);
		Enqueue({ std::move(job), pending });
		return JobHandle(std::move(pending));
	}

	void JobSystem::Enqueue(Job job)
	{
		{
			std::lock_guard lock(m_Mutex);
			m_Queue.push_back(std::move(job));
		}
		m_WorkAvailable.notify_one();
	}

	bool JobSystem::RunOne()
	{
		Job job;
		{
			std::lock_guard lock(m_Mutex);
			if (m_Queue.empty())
				return false;
			job = std::move(m_Queue.front());
			m_Queue.pop_front();
		}
		Run(job);
		return true;
	}

	void JobSystem::Run(Job& job)
	{
		try
		{
			job.Work();
		}
		catch (const std::exception& e)
		{
			ETH_CORE_ERROR("Job threw: {}", e.what());
		}
		catch (...)
		{
			ETH_CORE_ERROR("Job threw an unknown exception");
		}
		job.Work = nullptr; // release what it captured before reporting it done
		if (job.Pending->fetch_sub(1, std::memory_order_acq_rel) == 1)
		{
			// Lock so a waiter can't miss the notification between checking and sleeping
			std::lock_guard lock(m_Mutex);
			m_JobFinished.notify_all();
		}
	}

	void JobSystem::WorkerLoop()
	{
		while (true)
		{
			Job job;
			{
				std::unique_lock lock(m_Mutex);
				m_WorkAvailable.wait(lock, [this] { return m_Stopping || !m_Queue.empty(); });
				if (m_Queue.empty())
					return; // stopping, nothing left
				job = std::move(m_Queue.front());
				m_Queue.pop_front();
			}
			Run(job);
		}
	}

	void JobSystem::Wait(const JobHandle& handle)
	{
		while (!handle.IsDone())
		{
			// Help instead of blocking: the job may be queued behind others, or there may be no workers
			if (RunOne())
				continue;
			std::unique_lock lock(m_Mutex);
			m_JobFinished.wait(lock, [&] { return handle.IsDone() || !m_Queue.empty(); });
		}
	}

	void JobSystem::ParallelFor(uint32_t count, uint32_t batch, const std::function<void(uint32_t begin, uint32_t end)>& body)
	{
		if (count == 0)
			return;
		batch = std::max(batch, 1u);
		const uint32_t ranges = (count + batch - 1) / batch;
		if (ranges == 1)
		{
			body(0, count);
			return;
		}

		// All ranges but the first are queued under one handle; the caller runs the first, then helps
		auto pending = std::make_shared<std::atomic<uint32_t>>(ranges - 1);
		for (uint32_t r = 1; r < ranges; r++)
		{
			const uint32_t begin = r * batch;
			const uint32_t end = std::min(begin + batch, count);
			Enqueue({ [&body, begin, end] { body(begin, end); }, pending });
		}
		body(0, std::min(batch, count));
		Wait(JobHandle(pending));
	}

}
