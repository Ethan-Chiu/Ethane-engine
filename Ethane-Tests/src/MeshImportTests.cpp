#include "Test.h"

#include "Ethane/Core/JobSystem.h"
#include "Ethane/Renderer/Mesh.h"

using namespace Ethane;

// Mesh::Import needs no GPU: it can be tested, and run on worker threads. Paths are relative to the
// repository root (where the tests run from).

TEST_CASE("Mesh::Import reads a model file without the GPU")
{
	std::optional<MeshImport> cube = Mesh::Import("Ethane-Editor/res/meshes/default/Cube.fbx");
	REQUIRE(cube);
	CHECK(cube->Vertices.size() == 24);
	CHECK(cube->Indices.size() == 36);
	CHECK(cube->Submeshes.size() == 1);
	CHECK(cube->Nodes.size() == 1);
	CHECK(!cube->Materials.empty());
	CHECK(cube->BoundingBox.Max.x > cube->BoundingBox.Min.x);

	CHECK(!Mesh::Import("Ethane-Editor/res/meshes/default/Missing.fbx"));
}

TEST_CASE("Mesh::Import decodes the model's textures")
{
	std::optional<MeshImport> floor = Mesh::Import("Ethane-Editor/assets/models/CobblestoneFloor/CobblestoneFloor.obj");
	REQUIRE(floor);
	bool hasBaseColor = false;
	for (const MeshImport::Material& material : floor->Materials)
	{
		if (material.BaseColor)
		{
			hasBaseColor = true;
			CHECK(material.BaseColor->Pixels.size() == size_t(material.BaseColor->Width) * material.BaseColor->Height * 4);
		}
		CHECK(!material.Factors.BaseColorTexture); // GPU textures are made by Mesh::Create
	}
	CHECK(hasBaseColor);
}

TEST_CASE("Mesh::Import runs in parallel on worker threads")
{
	JobSystem jobs(4);
	const char* names[] = { "Cube", "Sphere", "Cylinder", "Cone", "Torus", "Capsule" };
	std::vector<std::optional<MeshImport>> results(std::size(names));
	jobs.ParallelFor(static_cast<uint32_t>(std::size(names)), 1, [&](uint32_t begin, uint32_t end) {
		for (uint32_t i = begin; i < end; i++)
			results[i] = Mesh::Import(std::string("Ethane-Editor/res/meshes/default/") + names[i] + ".fbx");
	});
	for (const auto& result : results)
	{
		REQUIRE(result);
		CHECK(!result->Vertices.empty());
	}
}
