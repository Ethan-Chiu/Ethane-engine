#include "Test.h"

#include "Ethane/GfxBackend/Vulkan/VulkanDeletionQueue.h"

#include <string>
#include <vector>

using namespace Ethane;

namespace {

	// Stands in for a Vulkan handle: records when it's destroyed
	struct Handle
	{
		std::string Name;
		std::vector<std::string>* Log = nullptr;

		Handle(std::string name, std::vector<std::string>& log) : Name(std::move(name)), Log(&log) {}
		Handle(Handle&& other) noexcept : Name(std::move(other.Name)), Log(std::exchange(other.Log, nullptr)) {}
		Handle& operator=(Handle&&) = delete;
		~Handle() { if (Log) Log->push_back(Name); }
	};

}

TEST_CASE("DeletionQueue: objects live until their frame has finished")
{
	std::vector<std::string> destroyed;
	VulkanDeletionQueue queue;

	queue.Retire(1, Handle("frame 1", destroyed));
	queue.Retire(2, Handle("frame 2", destroyed));
	queue.Retire(3, Handle("frame 3", destroyed));
	CHECK(queue.Size() == 3);
	CHECK(destroyed.empty()); // retiring never destroys

	queue.Collect(0);
	CHECK(destroyed.empty());
	queue.Collect(2);
	CHECK(destroyed == std::vector<std::string>{ "frame 1", "frame 2" });
	CHECK(queue.Size() == 1);

	queue.Flush();
	CHECK(destroyed == std::vector<std::string>{ "frame 1", "frame 2", "frame 3" });
	CHECK(queue.Size() == 0);
}

TEST_CASE("DeletionQueue: objects retired together are destroyed in argument order")
{
	std::vector<std::string> destroyed;
	VulkanDeletionQueue queue;
	// e.g. a framebuffer before its views, a buffer before its memory
	queue.Retire(1, Handle("framebuffer", destroyed), Handle("view", destroyed), Handle("image", destroyed), Handle("memory", destroyed));
	queue.Flush();
	CHECK(destroyed == std::vector<std::string>{ "framebuffer", "view", "image", "memory" });
}

TEST_CASE("DeletionQueue: calls run when collected, once")
{
	int calls = 0;
	VulkanDeletionQueue queue;
	queue.Retire(5, VulkanDeletionQueue::Call([&] { calls++; }));
	queue.Collect(4);
	CHECK(calls == 0);
	queue.Collect(5);
	CHECK(calls == 1);
	queue.Flush();
	CHECK(calls == 1);
}

TEST_CASE("DeletionQueue: a destructor may retire more")
{
	VulkanDeletionQueue queue;
	int inner = 0;
	queue.Retire(1, VulkanDeletionQueue::Call([&] {
		queue.Retire(2, VulkanDeletionQueue::Call([&] { inner++; }));
	}));
	queue.Collect(1); // runs the first call, which retires another (no deadlock)
	CHECK(queue.Size() == 1);
	CHECK(inner == 0);
	queue.Collect(2);
	CHECK(inner == 1);
}
