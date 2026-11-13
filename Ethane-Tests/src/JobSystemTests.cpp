#include "Test.h"

#include "Ethane/Core/JobSystem.h"

#include <atomic>
#include <set>
#include <thread>

using namespace Ethane;

TEST_CASE("JobSystem: submitted jobs run, and Wait returns once they're done")
{
	JobSystem jobs(4);
	CHECK(jobs.WorkerCount() == 4);

	std::atomic<int> sum = 0;
	std::vector<JobHandle> handles;
	for (int i = 1; i <= 100; i++)
		handles.push_back(jobs.Submit([&sum, i] { sum += i; }));
	for (const JobHandle& handle : handles)
		jobs.Wait(handle);
	CHECK(sum == 5050);
	for (const JobHandle& handle : handles)
		CHECK(handle.IsDone());

	JobHandle none;
	CHECK(!none.IsValid());
	CHECK(none.IsDone());
	jobs.Wait(none); // returns at once
}

TEST_CASE("JobSystem: jobs run on worker threads")
{
	JobSystem jobs(3);
	std::mutex mutex;
	std::set<std::thread::id> threads;
	std::vector<JobHandle> handles;
	for (int i = 0; i < 64; i++)
		handles.push_back(jobs.Submit([&] {
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
			std::lock_guard lock(mutex);
			threads.insert(std::this_thread::get_id());
		}));
	for (const JobHandle& handle : handles)
		jobs.Wait(handle);
	CHECK(threads.size() > 1);
}

TEST_CASE("JobSystem: a job may wait for another job (no deadlock, even with one worker)")
{
	JobSystem jobs(1);
	std::atomic<int> order = 0;
	int innerSeen = -1, outerSeen = -1;
	JobHandle outer = jobs.Submit([&] {
		JobHandle inner = jobs.Submit([&] { innerSeen = order++; });
		jobs.Wait(inner); // the only worker is busy with this job: Wait runs `inner` itself
		outerSeen = order++;
	});
	jobs.Wait(outer);
	CHECK(innerSeen == 0);
	CHECK(outerSeen == 1);
}

TEST_CASE("JobSystem: ParallelFor covers every index exactly once")
{
	JobSystem jobs(4);
	for (uint32_t count : { 0u, 1u, 7u, 64u, 1000u })
	{
		std::vector<std::atomic<int>> hits(count);
		jobs.ParallelFor(count, 13, [&](uint32_t begin, uint32_t end) {
			CHECK(begin < end);
			CHECK(end - begin <= 13);
			for (uint32_t i = begin; i < end; i++)
				hits[i]++;
		});
		bool once = true;
		for (auto& hit : hits)
			once &= hit == 1;
		CHECK(once);
	}
}

TEST_CASE("JobSystem: a job that throws still finishes, and the destructor runs what's queued")
{
	std::atomic<int> ran = 0;
	{
		JobSystem jobs(2);
		JobHandle bad = jobs.Submit([] { throw std::runtime_error("expected test failure"); });
		jobs.Wait(bad);
		CHECK(bad.IsDone());
		for (int i = 0; i < 50; i++)
			jobs.Submit([&] { ran++; });
	}
	CHECK(ran == 50);
}
