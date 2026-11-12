#include "Test.h"

#include "Ethane/Renderer/FramePacket.h"
#include "Ethane/Stele/Stele.h"
#include "Ethane/World/Components.h"
#include "Ethane/World/Entt/EnttWorld.h"

#include <glm/gtc/matrix_transform.hpp>

using namespace Ethane;

namespace {

	struct Fixture
	{
		static TypeRegistry& WithEngineComponents(TypeRegistry& types)
		{
			RegisterEngineComponents(types);
			return types;
		}

		TypeRegistry Types;
		Ethane::Stele Source{ WithEngineComponents(Types) };

		CommitResult Commit(Transaction tx)
		{
			CommitResult result = Source.Commit(std::move(tx));
			REQUIRE(result);
			return result;
		}
	};

	bool Near(const glm::vec3& a, const glm::vec3& b) { return glm::length(a - b) < 1e-4f; }

	const DrawItem* ItemFor(const FramePacket& packet, ElementId id)
	{
		for (const DrawItem& item : packet.DrawItems())
			if (item.Element == id)
				return &item;
		return nullptr;
	}

}

TEST_CASE("Components: rotation towards a direction points the forward axis there")
{
	for (glm::vec3 direction : { glm::vec3(-0.4f, -0.8f, -0.6f), glm::vec3(0, -1, 0), glm::vec3(1, 0, 0), glm::vec3(0, 0, 1) })
	{
		const glm::vec3 forward = glm::quat(RotationTowards(direction)) * glm::vec3(0, 0, -1);
		CHECK(Near(forward, glm::normalize(direction)));
	}

	TypeRegistry types;
	RegisterEngineComponents(types);
	RegisterEngineComponents(types); // twice is fine
	CHECK(types.Find("Transform") != nullptr);
	CHECK(types.Find<MeshComponent>()->FindField("Mesh")->Type == FieldType::Asset);
}

TEST_CASE("EnttWorld: builds from a snapshot and follows change sets")
{
	Fixture f;
	Transaction build("Build", f.Types);
	ElementId parent = build.CreateElement("Parent");
	build.AddComponent(parent, TransformComponent{ .Position = { 10.0f, 0.0f, 0.0f }, .Scale = glm::vec3(2.0f) });
	ElementId child = build.CreateElement("Child", parent);
	build.AddComponent(child, TransformComponent{ .Position = { 1.0f, 0.0f, 0.0f } });
	build.AddComponent(child, MeshComponent{ .Mesh = AssetId{ 5 } });
	f.Commit(std::move(build));

	EnttWorld world(f.Source.Read());
	SteleSubscription follow = f.Source.Subscribe([&](const ChangeSet& c) { world.Apply(c); });
	CHECK(world.Version() == 1);
	CHECK(world.EntityCount() == 2);

	FramePacket packet;
	world.Extract(packet);
	const DrawItem* item = ItemFor(packet, child);
	REQUIRE(item);
	CHECK(item->Mesh == AssetId{ 5 });
	// child at x = 1 inside a parent at x = 10 scaled by 2: world x = 12
	CHECK(Near(glm::vec3(item->Transform[3]), glm::vec3(12.0f, 0.0f, 0.0f)));
	CHECK(packet.ParamsOf(*item) == nullptr);

	// Edits arrive through change sets
	Transaction edit("Edit", f.Types);
	edit.SetField<TransformComponent>(parent, "Position", glm::vec3(0.0f));
	edit.AddComponent(child, MaterialComponent{ .BaseColor = { 0.0f, 1.0f, 0.0f, 1.0f } });
	f.Commit(std::move(edit));
	CHECK(world.Version() == 2);

	packet.Clear();
	world.Extract(packet);
	item = ItemFor(packet, child);
	REQUIRE(item);
	CHECK(Near(glm::vec3(item->Transform[3]), glm::vec3(2.0f, 0.0f, 0.0f)));
	REQUIRE(packet.ParamsOf(*item));
	CHECK(packet.ParamsOf(*item)->BaseColor == glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));

	// Moving to the top level drops the parent's transform; hiding removes the draw item
	Transaction move("Move", f.Types);
	move.Move(child, {});
	f.Commit(std::move(move));
	packet.Clear();
	world.Extract(packet);
	CHECK(Near(glm::vec3(ItemFor(packet, child)->Transform[3]), glm::vec3(1.0f, 0.0f, 0.0f)));

	Transaction hide("Hide", f.Types);
	hide.SetField<MeshComponent>(child, "Visible", false);
	f.Commit(std::move(hide));
	packet.Clear();
	world.Extract(packet);
	CHECK(packet.DrawItems().empty());

	Transaction destroy("Delete", f.Types);
	destroy.DestroyElement(child);
	f.Commit(std::move(destroy));
	CHECK(world.EntityCount() == 1);
	CHECK(world.RebuildCount() == 0);
}

TEST_CASE("EnttWorld: lights shine along the element's forward axis")
{
	Fixture f;
	const glm::vec3 direction = glm::normalize(glm::vec3(-0.4f, -0.8f, -0.6f));
	Transaction tx("Light", f.Types);
	ElementId sun = tx.CreateElement("Sun");
	tx.AddComponent(sun, TransformComponent{ .Rotation = RotationTowards(direction) });
	tx.AddComponent(sun, DirectionalLightComponent{ .Color = { 1.0f, 0.5f, 0.25f }, .Intensity = 0.9f });
	f.Commit(std::move(tx));

	EnttWorld world(f.Source.Read());
	FramePacket packet;
	world.Extract(packet);
	REQUIRE(packet.DirectionalLights.size() == 1);
	CHECK(Near(packet.DirectionalLights[0].Direction, direction));
	CHECK(packet.DirectionalLights[0].Intensity == 0.9f);
	CHECK(packet.DirectionalLights[0].Color == glm::vec3(1.0f, 0.5f, 0.25f));
}

TEST_CASE("EnttWorld: a missed change set makes it rebuild from the snapshot")
{
	Fixture f;
	EnttWorld world(f.Source.Read());

	Transaction first("First", f.Types);
	ElementId a = first.CreateElement("A");
	first.AddComponent(a, MeshComponent{ .Mesh = AssetId{ 1 } });
	f.Commit(std::move(first)); // not given to the world

	Transaction second("Second", f.Types);
	second.AddComponent(a, TransformComponent{ .Position = { 3.0f, 0.0f, 0.0f } });
	CommitResult result = f.Commit(std::move(second));

	world.Apply(*result.Changes); // version 0 -> can't apply 1 -> 2
	CHECK(world.RebuildCount() == 1);
	CHECK(world.Version() == 2);
	FramePacket packet;
	world.Extract(packet);
	REQUIRE(ItemFor(packet, a));
	CHECK(Near(glm::vec3(ItemFor(packet, a)->Transform[3]), glm::vec3(3.0f, 0.0f, 0.0f)));
}

TEST_CASE("EnttWorld: component types it doesn't know are ignored")
{
	struct Custom { float Value = 0.0f; };
	Fixture f;
	f.Types.Register<Custom>("Test.Custom").Field("Value", &Custom::Value);

	Transaction tx("Custom", f.Types);
	ElementId a = tx.CreateElement("A");
	tx.AddComponent(a, Custom{});
	f.Commit(std::move(tx));

	EnttWorld world(f.Source.Read());
	SteleSubscription follow = f.Source.Subscribe([&](const ChangeSet& c) { world.Apply(c); });
	Transaction edit("Edit", f.Types);
	edit.SetField<Custom>(a, "Value", 2.0f);
	f.Commit(std::move(edit));
	CHECK(world.RebuildCount() == 0);
	CHECK(world.Version() == 2);
}
