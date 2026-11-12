#include "Test.h"

#include "Ethane/Renderer/FramePacket.h"
#include "Ethane/Stele/Stele.h"
#include "Ethane/World/Components.h"
#include "Ethane/World/Entt/EnttWorld.h"
#include "Ethane/World/Script.h"

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

// --- Play mode: scripts and cameras ------------------------------------------------------------------------

namespace {

	// A game component the engine doesn't know: stored generically, reachable from scripts
	struct Speed
	{
		float Value = 1.0f;
	};

	std::vector<std::string> g_Events;

	// Moves its element along +x by Speed per second; records its lifecycle
	class Mover : public Script
	{
	public:
		void OnCreate() override { g_Events.push_back("create"); }
		void OnUpdate(float dt) override
		{
			const Speed* speed = Get<Speed>();
			Get<TransformComponent>()->Position.x += (speed ? speed->Value : 1.0f) * dt;
		}
		void OnDestroy() override { g_Events.push_back("destroy"); }
	};

	class Other : public Script
	{
	public:
		void OnCreate() override { g_Events.push_back("other"); }
	};

	struct PlayFixture : Fixture
	{
		ScriptRegistry Scripts;
		ElementId Box;

		PlayFixture()
		{
			Types.Register<Speed>("Test.Speed").Field("Value", &Speed::Value);
			Scripts.Register<Mover>("Mover");
			Scripts.Register<Other>("Other");
			g_Events.clear();

			Transaction tx("Build", Types);
			Box = tx.CreateElement("Box");
			tx.AddComponent(Box, TransformComponent{});
			tx.AddComponent(Box, MeshComponent{ .Mesh = AssetId{ 1 } });
			tx.AddComponent(Box, Speed{ .Value = 2.0f });
			tx.AddComponent(Box, NativeScriptComponent{ .Class = "Mover" });
			Commit(std::move(tx));
		}

		float BoxX(const EnttWorld& world) const
		{
			FramePacket packet;
			world.Extract(packet);
			return ItemFor(packet, Box)->Transform[3].x;
		}
	};

}

TEST_CASE("EnttWorld: only play worlds run scripts, and their changes stay in the world")
{
	PlayFixture f;
	EnttWorld edit(f.Source.Read(), WorldMode::Edit, &f.Scripts);
	EnttWorld play(f.Source.Read(), WorldMode::Play, &f.Scripts);

	edit.Update({ .DeltaTime = 1.0f });
	CHECK(edit.ScriptCount() == 0);
	CHECK(g_Events.empty());

	play.Update({ .DeltaTime = 0.5f }); // created, then updated: x = 2 * 0.5
	play.Update({ .DeltaTime = 0.5f });
	CHECK(play.ScriptCount() == 1);
	CHECK(g_Events == std::vector<std::string>{ "create" });
	CHECK(Near(glm::vec3(f.BoxX(play)), glm::vec3(2.0f)));
	CHECK(f.BoxX(edit) == 0.0f);
	CHECK(f.Source.Read().Get<TransformComponent>(f.Box)->Position.x == 0.0f); // the stele is untouched
}

TEST_CASE("EnttWorld: scripts follow their component, class and element")
{
	PlayFixture f;
	EnttWorld play(f.Source.Read(), WorldMode::Play, &f.Scripts);
	SteleSubscription follow = f.Source.Subscribe([&](const ChangeSet& c) { play.Apply(c); });
	play.Update({ .DeltaTime = 0.0f });

	// Live tweak of a game component while playing
	Transaction faster("Faster", f.Types);
	faster.SetField<Speed>(f.Box, "Value", 10.0f);
	f.Commit(std::move(faster));
	play.Update({ .DeltaTime = 1.0f });
	CHECK(Near(glm::vec3(f.BoxX(play)), glm::vec3(10.0f)));

	// Another class: the old script is destroyed, the new one created
	Transaction swap("Swap", f.Types);
	swap.SetField<NativeScriptComponent>(f.Box, "Class", std::string("Other"));
	f.Commit(std::move(swap));
	play.Update({ .DeltaTime = 1.0f });
	CHECK(g_Events == std::vector<std::string>{ "create", "destroy", "other" });

	// An unknown class makes no script (and doesn't fail every frame)
	Transaction unknown("Unknown", f.Types);
	unknown.SetField<NativeScriptComponent>(f.Box, "Class", std::string("Missing"));
	f.Commit(std::move(unknown));
	play.Update({ .DeltaTime = 1.0f });
	play.Update({ .DeltaTime = 1.0f });
	CHECK(play.ScriptCount() == 0);

	Transaction back("Back", f.Types);
	back.SetField<NativeScriptComponent>(f.Box, "Class", std::string("Mover"));
	f.Commit(std::move(back));
	play.Update({ .DeltaTime = 0.0f });
	CHECK(play.ScriptCount() == 1);

	// Deleting the element destroys its script
	g_Events.clear();
	Transaction destroy("Delete", f.Types);
	destroy.DestroyElement(f.Box);
	f.Commit(std::move(destroy));
	CHECK(g_Events == std::vector<std::string>{ "destroy" });
	CHECK(play.ScriptCount() == 0);
}

TEST_CASE("EnttWorld: destroying a play world destroys its scripts")
{
	PlayFixture f;
	{
		EnttWorld play(f.Source.Read(), WorldMode::Play, &f.Scripts);
		play.Update({ .DeltaTime = 0.0f });
	}
	CHECK(g_Events == std::vector<std::string>{ "create", "destroy" });
}

TEST_CASE("EnttWorld: the view comes from the primary camera")
{
	Fixture f;
	EnttWorld empty(f.Source.Read());
	PacketView view;
	CHECK(!empty.ExtractView(1.5f, view));

	Transaction tx("Camera", f.Types);
	ElementId camera = tx.CreateElement("Camera");
	// At (0, 2, 10), looking at the origin, scaled (scale must not distort the view)
	tx.AddComponent(camera, TransformComponent{ .Position = { 0.0f, 2.0f, 10.0f }, .Rotation = RotationTowards({ 0.0f, -2.0f, -10.0f }), .Scale = glm::vec3(3.0f) });
	tx.AddComponent(camera, CameraComponent{ .FieldOfView = glm::radians(60.0f) });
	ElementId secondary = tx.CreateElement("Secondary");
	tx.AddComponent(secondary, CameraComponent{ .Primary = false });
	f.Commit(std::move(tx));

	EnttWorld world(f.Source.Read());
	REQUIRE(world.ExtractView(1.5f, view));
	CHECK(Near(view.Position, glm::vec3(0.0f, 2.0f, 10.0f)));
	// The origin is straight ahead: in view space it's on the -Z axis, 10.2 units away
	const glm::vec3 origin = glm::vec3(view.View * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
	CHECK(Near(origin, glm::vec3(0.0f, 0.0f, -glm::length(glm::vec3(0.0f, 2.0f, 10.0f)))));
	CHECK(Near(glm::vec3(view.Projection[1][1]), glm::vec3(1.0f / std::tan(glm::radians(30.0f)))));
}

TEST_CASE("Components: asset fields say which kind of file they take")
{
	TypeRegistry types;
	RegisterEngineComponents(types);
	CHECK(types.Find<MeshComponent>()->FindField("Mesh")->Meta.Asset == AssetKind::Mesh);
	CHECK(types.Find<MaterialComponent>()->FindField("BaseColorTexture")->Meta.Asset == AssetKind::Texture);
}
