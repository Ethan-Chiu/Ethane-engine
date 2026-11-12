#include "Test.h"

#include "Ethane/Renderer/FramePacket.h"

using namespace Ethane;

TEST_CASE("FramePacket: overrides are stored per draw item and need an element")
{
	FramePacket packet;
	const MaterialOverride red{ .BaseColor = { 1.0f, 0.0f, 0.0f, 1.0f } };

	packet.AddMesh(AssetId{ 1 }, glm::mat4(2.0f), ElementId{ 7 }, &red);
	packet.AddMesh(AssetId{ 2 }, glm::mat4(1.0f));
	packet.AddMesh(AssetId{ 3 }, glm::mat4(1.0f), {}, &red); // no element: override ignored

	auto items = packet.DrawItems();
	REQUIRE(items.size() == 3);
	REQUIRE(packet.OverrideOf(items[0]));
	CHECK(*packet.OverrideOf(items[0]) == red);
	CHECK(items[0].Element == ElementId{ 7 });
	CHECK(items[0].Mesh == AssetId{ 1 });
	CHECK(items[0].Transform == glm::mat4(2.0f));
	CHECK(packet.OverrideOf(items[1]) == nullptr);
	CHECK(packet.OverrideOf(items[2]) == nullptr);
}

TEST_CASE("FramePacket: Clear empties it but keeps the capacity")
{
	FramePacket packet;
	packet.View.Position = { 1.0f, 2.0f, 3.0f };
	packet.DirectionalLights.push_back({});
	for (int i = 0; i < 100; i++)
		packet.AddMesh(AssetId{ 1 }, glm::mat4(1.0f));
	const size_t capacity = packet.DirectionalLights.capacity();

	packet.Clear();
	CHECK(packet.DrawItems().empty());
	CHECK(packet.DirectionalLights.empty());
	CHECK(packet.DirectionalLights.capacity() == capacity);
	CHECK(packet.View.Position == glm::vec3(0.0f));
}
