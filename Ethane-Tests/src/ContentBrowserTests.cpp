#include "Test.h"

#include "ContentBrowser/ContentBrowser.h"
#include "Ethane/Asset/AssetRegistry.h"

#include <filesystem>
#include <fstream>

using namespace Ethane;
namespace fs = std::filesystem;

namespace {

	// A small asset tree in a temporary folder; the tests work in it as their working directory, like the
	// editor works with relative paths
	struct TempTree
	{
		fs::path Previous = fs::current_path();
		fs::path Dir = fs::temp_directory_path() / "ethane-content-browser-test";

		TempTree()
		{
			fs::remove_all(Dir);
			for (const char* folder : { "assets/models/props", "assets/textures", "assets/cache", "res" })
				fs::create_directories(Dir / folder);
			for (const char* file : { "assets/models/Crate.fbx", "assets/models/props/Barrel.obj", "assets/textures/Wood.PNG",
			                          "assets/textures/notes.txt", "assets/Level.stele", "assets/.hidden", "assets/cache/shader.bin" })
				std::ofstream(Dir / file) << "x";
			fs::current_path(Dir);
		}
		~TempTree()
		{
			fs::current_path(Previous);
			fs::remove_all(Dir);
		}
	};

	std::vector<std::string> Names(ContentBrowser& browser)
	{
		std::vector<std::string> names;
		for (const ContentBrowser::Entry& entry : browser.Entries())
			names.push_back(entry.Name);
		return names;
	}

}

TEST_CASE("ContentBrowser: lists folders first, hides dot-files and hidden names")
{
	TempTree tree;
	AssetRegistry registry;
	ContentBrowser browser(registry, { "assets", "res" });

	CHECK(browser.CurrentDirectory() == fs::path("assets"));
	CHECK(Names(browser) == std::vector<std::string>{ "models", "textures", "Level.stele" });
	CHECK(browser.Entries()[0].Type == ContentBrowser::Kind::Folder);
	CHECK(browser.Entries()[2].Type == ContentBrowser::Kind::Stele);
}

TEST_CASE("ContentBrowser: navigates within its roots only")
{
	TempTree tree;
	AssetRegistry registry;
	ContentBrowser browser(registry, { "assets", "res" });

	CHECK(!browser.CanGoUp());
	CHECK(browser.Navigate("assets/models/props"));
	CHECK(browser.CurrentDirectory() == fs::path("assets/models/props"));
	CHECK(browser.Breadcrumbs() == std::vector<fs::path>{ "assets", "assets/models", "assets/models/props" });
	browser.GoUp();
	CHECK(browser.CurrentDirectory() == fs::path("assets/models"));
	CHECK(Names(browser) == std::vector<std::string>{ "props", "Crate.fbx" });

	CHECK(!browser.Navigate("."));                       // above the roots
	CHECK(!browser.Navigate("assets/models/Crate.fbx")); // not a folder
	CHECK(!browser.Navigate("elsewhere"));
	CHECK(browser.CurrentDirectory() == fs::path("assets/models"));

	CHECK(browser.Navigate("res")); // the other root
	CHECK(browser.Root() == fs::path("res"));
	CHECK(!browser.CanGoUp());
}

TEST_CASE("ContentBrowser: classifies files and imports them into the registry")
{
	TempTree tree;
	AssetRegistry registry;
	ContentBrowser browser(registry, { "assets" });
	browser.Navigate("assets/textures");

	auto entries = browser.Entries();
	REQUIRE(entries.size() == 2);
	CHECK(entries[0].Name == "notes.txt");
	CHECK(entries[0].Type == ContentBrowser::Kind::Other);
	CHECK(entries[1].Type == ContentBrowser::Kind::Texture); // extension case doesn't matter

	CHECK(!browser.Find(entries[1]).IsValid());
	const AssetId wood = browser.Import(entries[1]);
	CHECK(wood.IsValid());
	CHECK(browser.Find(entries[1]) == wood);
	CHECK(registry.GetPath(wood)->generic_string() == "assets/textures/Wood.PNG");
	CHECK(browser.Import(entries[1]) == wood); // the same id again
}

TEST_CASE("ContentBrowser: the filter searches the current folder and below")
{
	TempTree tree;
	AssetRegistry registry;
	ContentBrowser browser(registry, { "assets" });

	browser.SetFilter("BAR");
	CHECK(Names(browser) == std::vector<std::string>{ "Barrel.obj" });
	CHECK(browser.Entries()[0].Path == fs::path("assets/models/props/Barrel.obj"));
	browser.SetFilter("shader");
	CHECK(Names(browser).empty()); // inside the hidden cache folder
	browser.SetFilter("");
	CHECK(Names(browser).size() == 3);
}

TEST_CASE("ContentBrowser: notices files added on disk")
{
	TempTree tree;
	AssetRegistry registry;
	ContentBrowser browser(registry, { "assets" });
	browser.Navigate("assets/textures");
	CHECK(browser.Entries().size() == 2);

	std::ofstream("assets/textures/Stone.jpg") << "x";
	browser.Refresh(); // the folder's write time can have a coarse resolution: refresh to be sure
	CHECK(browser.Entries().size() == 3);
}
