#include "Test.h"

#include "Ethane/Brick/BrickSerializer.h"

#include <filesystem>

using namespace Ethane;

namespace {

	struct Glow
	{
		glm::vec3 Color = { 1.0f, 1.0f, 1.0f };
		float Radius = 1.0f;
	};

	struct Everything
	{
		bool Flag = false;
		int32_t I32 = 0;
		uint32_t U32 = 0;
		int64_t I64 = 0;
		uint64_t U64 = 0;
		float F = 0.0f;
		double D = 0.0;
		glm::vec2 V2 = {};
		glm::vec4 V4 = {};
		glm::quat Q = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
		std::string Text;
		AssetId Asset;
		ObjectId Target;
		Glow Nested;
		int32_t Scratch = 0; // runtime only: never saved
	};

	struct Types
	{
		TypeRegistry Registry;
		Types(uint32_t everythingVersion = 1)
		{
			Registry.Register<Glow>("Test.Glow").Field("Color", &Glow::Color).Field("Radius", &Glow::Radius);
			Registry.Register<Everything>("Test.Everything", everythingVersion)
				.Field("Flag", &Everything::Flag).Field("I32", &Everything::I32).Field("U32", &Everything::U32)
				.Field("I64", &Everything::I64).Field("U64", &Everything::U64).Field("F", &Everything::F)
				.Field("D", &Everything::D).Field("V2", &Everything::V2).Field("V4", &Everything::V4)
				.Field("Q", &Everything::Q).Field("Text", &Everything::Text).Field("Asset", &Everything::Asset)
				.Field("Target", &Everything::Target).Field("Nested", &Everything::Nested)
				.Field("Scratch", &Everything::Scratch, FieldFlags::RuntimeOnly);
		}
	};

}

TEST_CASE("BrickSerializer: round trip keeps ids, hierarchy, order and every field type")
{
	Types types;
	Brick original(types.Registry);

	Everything e;
	e.Flag = true;
	e.I32 = -7;
	e.U32 = 4000000000u;
	e.I64 = -9000000000ll;
	e.U64 = 18000000000000000000ull;
	e.F = 0.1f;
	e.D = 1.0 / 3.0;
	e.V2 = { 1.5f, -2.5f };
	e.V4 = { 0.1f, 0.2f, 0.3f, 0.4f };
	e.Q = glm::quat(0.5f, 0.5f, -0.5f, 0.5f);
	e.Text = "multi\nline: \"quoted\" # not a comment";
	e.Asset = { 0xabcdef0123456789ull };
	e.Nested.Radius = 3.25f;
	e.Scratch = 99;

	Transaction tx("Build", types.Registry);
	ObjectId root = tx.CreateObject("Root");
	ObjectId second = tx.CreateObject("Second", root);
	ObjectId first = tx.CreateObject("First", root, "0V");
	e.Target = second;
	tx.AddComponent(first, e);
	tx.AddComponent<Glow>(second);
	REQUIRE(original.Commit(std::move(tx)));

	const std::string yaml = BrickSerializer::ToYaml(original.Read());
	CHECK(yaml.find("Scratch") == std::string::npos);
	CHECK(yaml.find("Test.Everything: 1") != std::string::npos);

	Brick loaded(types.Registry);
	auto result = BrickSerializer::FromYaml(yaml, loaded);
	REQUIRE(result);
	CHECK(result.Warnings.empty());

	BrickSnapshot snap = loaded.Read();
	CHECK(snap.ObjectCount() == 3);
	REQUIRE(snap.Find(first));
	CHECK(snap.Find(first)->Parent == root);
	CHECK(snap.Find(first)->Order == "0V");
	auto children = snap.Children(root);
	CHECK(std::vector<ObjectId>(children.begin(), children.end()) == std::vector<ObjectId>{ first, second });

	const Everything* l = snap.Get<Everything>(first);
	REQUIRE(l);
	CHECK(l->Flag == e.Flag);
	CHECK(l->I32 == e.I32);
	CHECK(l->U32 == e.U32);
	CHECK(l->I64 == e.I64);
	CHECK(l->U64 == e.U64);
	CHECK(l->F == e.F);   // exact: floats are written with enough digits to round-trip
	CHECK(l->D == e.D);
	CHECK(l->V2 == e.V2);
	CHECK(l->V4 == e.V4);
	CHECK(l->Q == e.Q);
	CHECK(l->Text == e.Text);
	CHECK(l->Asset == e.Asset);
	CHECK(l->Target == second);
	CHECK(l->Nested.Radius == 3.25f);
	CHECK(l->Scratch == 0);
	CHECK(snap.Get<Glow>(second) != nullptr);

	// Saving the loaded brick gives the same text
	CHECK(BrickSerializer::ToYaml(snap) == yaml);
}

TEST_CASE("BrickSerializer: unknown and invalid data is skipped with warnings")
{
	Types types;
	Brick brick(types.Registry);
	const char* yaml = R"(
Brick: 1
Types:
  Test.Glow: 5
Objects:
  - Id: 00000000000000b2
    Name: Child
    Parent: 00000000000000a1
    Order: V
    Components:
      Test.Glow:
        Radius: not a number
        Color: [0.5, 0.5, 0.5]
        Sparkle: 3
      Test.Gone:
        X: 1
  - Id: 00000000000000a1
    Name: Parent listed after its child
    Order: "10"
  - Id: 00000000000000c3
    Name: Orphan
    Parent: 00000000000000ff
)";
	auto result = BrickSerializer::FromYaml(yaml, brick);
	REQUIRE(result);
	CHECK(result.Warnings.size() == 6); // newer version, Radius, Sparkle, Test.Gone, order key "10", orphan
	for (const std::string& warning : result.Warnings)
		std::printf("      warning: %s\n", warning.c_str());

	BrickSnapshot snap = brick.Read();
	CHECK(snap.ObjectCount() == 3);
	CHECK(snap.Find(ObjectId{ 0xb2 })->Parent == ObjectId{ 0xa1 });
	CHECK(snap.Find(ObjectId{ 0xc3 })->Parent == ObjectId{});
	const Glow* glow = snap.Get<Glow>(ObjectId{ 0xb2 });
	REQUIRE(glow);
	CHECK(glow->Radius == 1.0f);
	CHECK(glow->Color == glm::vec3(0.5f));
}

TEST_CASE("BrickSerializer: broken files are refused and leave the brick alone")
{
	Types types;
	Brick brick(types.Registry);
	CHECK(!BrickSerializer::FromYaml("Objects: [", brick));                                // not YAML
	CHECK(!BrickSerializer::FromYaml("Something: else", brick));                           // not a brick
	CHECK(!BrickSerializer::FromYaml("Brick: 99", brick));                                 // newer format
	CHECK(!BrickSerializer::FromYaml("Brick: 1\nObjects:\n  - Name: no id", brick));
	CHECK(!BrickSerializer::FromYaml("Brick: 1\nObjects:\n  - Id: [1, 2]", brick));         // wrong shape
	CHECK(!BrickSerializer::FromYaml("Brick: 1\nObjects:\n  - Id: 01\n  - Id: 01", brick));  // duplicate id
	CHECK(!BrickSerializer::FromYaml(
		"Brick: 1\nObjects:\n  - Id: 01\n    Parent: 02\n  - Id: 02\n    Parent: 01", brick)); // cycle
	CHECK(brick.Read().Version() == 0);
}

TEST_CASE("BrickSerializer: save and load files")
{
	Types types;
	Brick brick(types.Registry);
	Transaction tx("Build", types.Registry);
	ObjectId lamp = tx.CreateObject("Lamp");
	tx.AddComponent(lamp, Glow{ { 1.0f, 0.5f, 0.0f }, 2.0f });
	REQUIRE(brick.Commit(std::move(tx)));

	const auto path = std::filesystem::temp_directory_path() / "ethane-brick-test.brick";
	REQUIRE(BrickSerializer::Save(brick.Read(), path));

	Brick loaded(types.Registry);
	REQUIRE(BrickSerializer::Load(path, loaded));
	CHECK(loaded.Read().Get<Glow>(lamp)->Radius == 2.0f);
	std::filesystem::remove(path);

	CHECK(!BrickSerializer::Load(path, loaded)); // gone
}
