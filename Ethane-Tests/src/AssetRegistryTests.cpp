#include "Test.h"

#include "Ethane/Asset/AssetRegistry.h"

#include <filesystem>

using namespace Ethane;

TEST_CASE("AssetRegistry: import gives one stable id per path")
{
	AssetRegistry registry;
	CHECK(!registry.IsDirty());

	AssetId cube = registry.Import("res/meshes/default/Cube.fbx");
	CHECK(cube.IsValid());
	CHECK(registry.IsDirty());
	CHECK(registry.Import("res/meshes/default/Cube.fbx") == cube);
	CHECK(registry.Import("res/meshes/./default/../default/Cube.fbx") == cube); // same file, other spelling
	CHECK(registry.Find("res/meshes/default/Cube.fbx") == cube);
	CHECK(registry.GetPath(cube)->generic_string() == "res/meshes/default/Cube.fbx");

	AssetId sphere = registry.Import("res/meshes/default/Sphere.fbx");
	CHECK(sphere != cube);
	CHECK(registry.Size() == 2);
	auto entries = registry.Entries();
	REQUIRE(entries.size() == 2);
	CHECK(entries[0].first == cube); // sorted by path: Cube before Sphere
	CHECK(entries[1].second.generic_string() == "res/meshes/default/Sphere.fbx");
	CHECK(!registry.Find("missing.fbx").IsValid());
	CHECK(!registry.GetPath(AssetId{ 1 }));
}

TEST_CASE("AssetRegistry: relocating keeps the id")
{
	AssetRegistry registry;
	AssetId a = registry.Import("old/a.png");
	AssetId b = registry.Import("b.png");

	CHECK(registry.Relocate(a, "new/a.png"));
	CHECK(registry.GetPath(a)->generic_string() == "new/a.png");
	CHECK(registry.Find("new/a.png") == a);
	CHECK(!registry.Find("old/a.png").IsValid());

	CHECK(!registry.Relocate(a, "b.png"));          // taken by b
	CHECK(!registry.Relocate(AssetId{ 99 }, "c.png")); // unknown id
	CHECK(registry.Find("b.png") == b);
}

TEST_CASE("AssetRegistry: YAML round trip and file save / load")
{
	AssetRegistry registry;
	AssetId a = registry.Import("assets/textures/test.png");
	AssetId b = registry.Import("assets/models/Floor.obj");

	AssetRegistry copy;
	REQUIRE(copy.FromYaml(registry.ToYaml()));
	CHECK(copy.Find("assets/textures/test.png") == a);
	CHECK(copy.Find("assets/models/Floor.obj") == b);
	CHECK(!copy.IsDirty());

	const auto file = std::filesystem::temp_directory_path() / "ethane-asset-registry-test.yaml";
	REQUIRE(registry.Save(file));
	CHECK(!registry.IsDirty());
	AssetRegistry loaded;
	REQUIRE(loaded.Load(file));
	CHECK(loaded.GetPath(b)->generic_string() == "assets/models/Floor.obj");
	std::filesystem::remove(file);

	std::string error;
	CHECK(!loaded.FromYaml("Assets:\n  - Id: zz\n    Path: x", &error));
	CHECK(!error.empty());
	CHECK(!loaded.FromYaml("Assets:\n  - Id: 01\n    Path: x\n  - Id: 02\n    Path: x"));
	CHECK(loaded.Size() == 2); // failed loads leave it unchanged
}

TEST_CASE("AssetRegistry: the kind of a file comes from its extension")
{
	CHECK(AssetRegistry::KindOf("res/meshes/default/Cube.fbx") == AssetKind::Mesh);
	CHECK(AssetRegistry::KindOf("models/Floor.OBJ") == AssetKind::Mesh);
	CHECK(AssetRegistry::KindOf("textures/test.png") == AssetKind::Texture);
	CHECK(AssetRegistry::KindOf("textures/photo.JPEG") == AssetKind::Texture);
	CHECK(AssetRegistry::KindOf("steles/Demo.stele") == AssetKind::Any);
	CHECK(AssetRegistry::KindOf("no_extension") == AssetKind::Any);
}
