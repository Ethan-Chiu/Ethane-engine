#include "Test.h"

#include "Ethane/Renderer/FramePacket.h"

using namespace Ethane;

// Meshes and materials are GPU objects; the packet only stores references, so null ones do here

TEST_CASE("FramePacket: each draw item gets its own material range")
{
	FramePacket packet;
	const std::vector<Ref<Material>> two(2), three(3);

	packet.AddMesh(nullptr, glm::mat4(2.0f), two, ElementId{ 7 });
	packet.AddMesh(nullptr, glm::mat4(1.0f), {});
	packet.AddMesh(nullptr, glm::mat4(1.0f), three);

	auto items = packet.DrawItems();
	REQUIRE(items.size() == 3);
	CHECK(packet.MaterialsOf(items[0]).size() == 2);
	CHECK(packet.MaterialsOf(items[1]).empty());
	CHECK(packet.MaterialsOf(items[2]).size() == 3);
	CHECK(items[2].FirstMaterial == 2);
	CHECK(items[0].Element == ElementId{ 7 });
	CHECK(items[0].Transform == glm::mat4(2.0f));
}

TEST_CASE("FramePacket: Clear empties it but keeps the capacity")
{
	FramePacket packet;
	packet.View.Position = { 1.0f, 2.0f, 3.0f };
	packet.DirectionalLights.push_back({});
	for (int i = 0; i < 100; i++)
		packet.AddMesh(nullptr, glm::mat4(1.0f), {});
	const size_t capacity = packet.DirectionalLights.capacity();

	packet.Clear();
	CHECK(packet.DrawItems().empty());
	CHECK(packet.DirectionalLights.empty());
	CHECK(packet.DirectionalLights.capacity() == capacity);
	CHECK(packet.View.Position == glm::vec3(0.0f));
}
