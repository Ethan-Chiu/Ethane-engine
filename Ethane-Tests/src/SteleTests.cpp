#include "Test.h"

#include "Ethane/Stele/Stele.h"
#include "Ethane/Stele/OrderKey.h"
#include "Ethane/Stele/UndoHistory.h"

#include <atomic>
#include <thread>

using namespace Ethane;

namespace {

	struct Position
	{
		glm::vec3 Value = { 0.0f, 0.0f, 0.0f };
	};

	struct Tint
	{
		glm::vec4 Color = { 1.0f, 1.0f, 1.0f, 1.0f };
		std::string Note;
	};

	struct Registry
	{
		TypeRegistry Types;
		Registry()
		{
			Types.Register<Position>("Test.Position").Field("Value", &Position::Value);
			Types.Register<Tint>("Test.Tint").Field("Color", &Tint::Color, FieldFlags::Color).Field("Note", &Tint::Note);
		}
	};

	std::vector<std::string> ChildNames(const SteleSnapshot& snapshot, ElementId parent = {})
	{
		std::vector<std::string> names;
		for (ElementId id : snapshot.Children(parent))
			names.push_back(snapshot.Find(id)->Name);
		return names;
	}

}

// --- Order keys ------------------------------------------------------------------------------------------

TEST_CASE("OrderKey: keys between neighbours stay ordered and valid")
{
	const std::string first = OrderKey::Between({}, {});
	CHECK(OrderKey::IsValid(first));

	// Repeated inserts at the front, at the back and in the middle
	std::vector<std::string> keys = { first };
	for (int i = 0; i < 200; i++)
	{
		keys.insert(keys.begin(), OrderKey::Between({}, keys.front()));
		keys.push_back(OrderKey::Between(keys.back(), {}));
		const size_t mid = keys.size() / 2;
		keys.insert(keys.begin() + mid, OrderKey::Between(keys[mid - 1], keys[mid]));
	}
	for (size_t i = 0; i < keys.size(); i++)
	{
		CHECK(OrderKey::IsValid(keys[i]));
		if (i > 0)
			CHECK(keys[i - 1] < keys[i]);
	}

	CHECK(OrderKey::Between("1", "2") == "1V");
	CHECK(OrderKey::Between("1", "3") == "2");
	CHECK(OrderKey::Between({}, "1") == "0V");
	CHECK(!OrderKey::IsValid("10"));
	CHECK(!OrderKey::IsValid("a-b"));
}

// --- Elements, hierarchy, components ----------------------------------------------------------------------

TEST_CASE("Stele: create elements, hierarchy and sibling order")
{
	Registry reg;
	Stele stele(reg.Types);
	CHECK(stele.Read().Version() == 0);

	Transaction tx("Build", reg.Types);
	ElementId root = tx.CreateElement("Root");
	ElementId b = tx.CreateElement("B", root);
	ElementId c = tx.CreateElement("C", root);
	tx.CreateElement("A", root, OrderKey::Between({}, "1")); // explicitly before everything else
	CommitResult result = stele.Commit(std::move(tx));
	REQUIRE(result);
	CHECK(result.Version == 1);

	SteleSnapshot snap = stele.Read();
	CHECK(snap.Version() == 1);
	CHECK(snap.ElementCount() == 4);
	CHECK(ChildNames(snap) == std::vector<std::string>{ "Root" });
	CHECK(ChildNames(snap, root) == std::vector<std::string>{ "A", "B", "C" });
	CHECK(snap.Find(b)->Parent == root);

	// Move C to the top level, before Root
	Transaction move("Move", reg.Types);
	move.Move(c, {}, OrderKey::Between({}, snap.Find(root)->Order));
	REQUIRE(stele.Commit(std::move(move)));
	snap = stele.Read();
	CHECK(ChildNames(snap) == std::vector<std::string>{ "C", "Root" });
	CHECK(ChildNames(snap, root) == std::vector<std::string>{ "A", "B" });

	std::vector<std::string> depthFirst;
	snap.ForEachDepthFirst([&](const SteleElement& o) { depthFirst.push_back(o.Name); });
	CHECK(depthFirst == std::vector<std::string>{ "C", "Root", "A", "B" });
}

TEST_CASE("Stele: components and fields")
{
	Registry reg;
	Stele stele(reg.Types);

	Transaction tx("Add", reg.Types);
	ElementId lamp = tx.CreateElement("Lamp");
	tx.AddComponent(lamp, Position{ { 1.0f, 2.0f, 3.0f } });
	tx.AddComponent<Tint>(lamp);
	REQUIRE(stele.Commit(std::move(tx)));

	SteleSnapshot before = stele.Read();
	REQUIRE(before.Get<Position>(lamp));
	CHECK(before.Get<Position>(lamp)->Value == glm::vec3(1.0f, 2.0f, 3.0f));
	CHECK(before.Get<Tint>(lamp)->Color == glm::vec4(1.0f));

	Transaction edit("Edit", reg.Types);
	edit.SetField<Tint>(lamp, "Color", glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
	edit.SetField<Tint>(lamp, "Note", std::string("red"));
	REQUIRE(stele.Commit(std::move(edit)));

	SteleSnapshot after = stele.Read();
	CHECK(after.Get<Tint>(lamp)->Color == glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
	CHECK(after.Get<Tint>(lamp)->Note == "red");

	// The old snapshot is untouched, and the unchanged component is shared, not copied
	CHECK(before.Get<Tint>(lamp)->Color == glm::vec4(1.0f));
	CHECK(before.Get<Position>(lamp) == after.Get<Position>(lamp));
	CHECK(before.Get<Tint>(lamp) != after.Get<Tint>(lamp));

	Transaction remove("Remove", reg.Types);
	remove.RemoveComponent<Tint>(lamp);
	REQUIRE(stele.Commit(std::move(remove)));
	CHECK(stele.Read().Get<Tint>(lamp) == nullptr);
	CHECK(stele.Read().Get<Position>(lamp) != nullptr);
}

TEST_CASE("Stele: unchanged elements are shared between snapshots")
{
	Registry reg;
	Stele stele(reg.Types);
	Transaction tx("Add", reg.Types);
	ElementId a = tx.CreateElement("A");
	ElementId b = tx.CreateElement("B");
	REQUIRE(stele.Commit(std::move(tx)));
	SteleSnapshot before = stele.Read();

	Transaction rename("Rename", reg.Types);
	rename.Rename(a, "A2");
	REQUIRE(stele.Commit(std::move(rename)));
	SteleSnapshot after = stele.Read();

	CHECK(before.Find(a)->Name == "A");
	CHECK(after.Find(a)->Name == "A2");
	CHECK(before.Find(b) == after.Find(b));
}

TEST_CASE("Stele: a failing op refuses the whole transaction")
{
	Registry reg;
	Stele stele(reg.Types);
	Transaction tx("Add", reg.Types);
	ElementId parent = tx.CreateElement("Parent");
	ElementId child = tx.CreateElement("Child", parent);
	tx.AddComponent<Position>(child);
	REQUIRE(stele.Commit(std::move(tx)));

	auto refused = [&](auto build) {
		Transaction bad("Bad", reg.Types);
		bad.Rename(parent, "renamed first"); // must not survive the failure
		build(bad);
		CommitResult result = stele.Commit(std::move(bad));
		return !result && !result.Error.empty();
	};

	CHECK(refused([&](Transaction& t) { t.SetField<Position>(child, "Value", 1.0f); }));          // wrong value type
	CHECK(refused([&](Transaction& t) { t.SetField<Position>(child, "Missing", 1.0f); }));        // unknown field
	CHECK(refused([&](Transaction& t) { t.SetField<Tint>(child, "Note", std::string()); }));      // component missing
	CHECK(refused([&](Transaction& t) { t.AddComponent<Position>(child); }));                     // already has it
	CHECK(refused([&](Transaction& t) { t.AddComponent<int>(child); }));                          // unregistered type
	CHECK(refused([&](Transaction& t) { t.DestroyElement(parent); }));                             // has children
	CHECK(refused([&](Transaction& t) { t.Move(parent, child); }));                               // cycle
	CHECK(refused([&](Transaction& t) { t.CreateElement(child, "Duplicate"); }));                  // id taken
	CHECK(refused([&](Transaction& t) { t.CreateElement("Orphan", ElementId{ 12345 }); }));         // no parent
	CHECK(refused([&](Transaction& t) { t.Move(child, {}, "10"); }));                             // invalid key

	CHECK(stele.Read().Version() == 1);
	CHECK(stele.Read().Find(parent)->Name == "Parent");
}

TEST_CASE("Stele: DestroyTree removes descendants")
{
	Registry reg;
	Stele stele(reg.Types);
	Transaction tx("Add", reg.Types);
	ElementId root = tx.CreateElement("Root");
	ElementId mid = tx.CreateElement("Mid", root);
	tx.CreateElement("Leaf", mid);
	ElementId other = tx.CreateElement("Other");
	REQUIRE(stele.Commit(std::move(tx)));

	Transaction destroy("Delete", reg.Types);
	destroy.DestroyTree(stele.Read(), root);
	REQUIRE(stele.Commit(std::move(destroy)));

	SteleSnapshot snap = stele.Read();
	CHECK(snap.ElementCount() == 1);
	CHECK(snap.Contains(other));
	CHECK(snap.Children(root).empty());
}

// --- Change sets and undo --------------------------------------------------------------------------------

TEST_CASE("Stele: change sets list applied ops and reach subscribers")
{
	Registry reg;
	Stele stele(reg.Types);

	std::vector<std::pair<uint64_t, uint64_t>> seen;
	SteleSubscription subscription = stele.Subscribe([&](const ChangeSet& c) {
		seen.emplace_back(c.FromVersion, c.ToVersion);
		CHECK(c.After.Version() == c.ToVersion);
	});

	Transaction tx("Add", reg.Types);
	ElementId a = tx.CreateElement("A");
	tx.AddComponent<Position>(a, {});
	CommitResult result = stele.Commit(std::move(tx));
	REQUIRE(result.Changes);

	// The applied ops carry what the commit filled in: the order key and the default component
	const auto& create = std::get<SteleOp::CreateElement>(result.Changes->Ops[0]);
	CHECK(!create.Order.empty());
	CHECK(create.Order == stele.Read().Find(a)->Order);
	CHECK(std::get<SteleOp::AddComponent>(result.Changes->Ops[1]).Value != nullptr);

	CHECK(stele.Commit(Transaction("Empty", reg.Types)));   // empty: nothing published
	subscription.Reset();
	Transaction later("Later", reg.Types);
	later.Rename(a, "B");
	REQUIRE(stele.Commit(std::move(later)));

	CHECK(seen == std::vector<std::pair<uint64_t, uint64_t>>{ { 0, 1 } });

	// A subscription may outlive its stele
	SteleSubscription orphan;
	{
		Stele temporary(reg.Types);
		orphan = temporary.Subscribe([](const ChangeSet&) {});
	}
	orphan.Reset();
}

TEST_CASE("UndoHistory: undo and redo restore exact states")
{
	Registry reg;
	Stele stele(reg.Types);
	UndoHistory history;
	auto commit = [&](Transaction tx) {
		CommitResult result = stele.Commit(std::move(tx));
		REQUIRE(result);
		history.Record(*result.Changes);
	};

	Transaction create("Create lamp", reg.Types);
	ElementId lamp = create.CreateElement("Lamp");
	ElementId bulb = create.CreateElement("Bulb", lamp);
	create.AddComponent(bulb, Tint{ { 0.0f, 1.0f, 0.0f, 1.0f }, "green" });
	commit(std::move(create));
	const SteleSnapshot created = stele.Read();

	Transaction destroy("Delete lamp", reg.Types);
	destroy.DestroyTree(stele.Read(), lamp);
	commit(std::move(destroy));
	CHECK(stele.Read().ElementCount() == 0);
	CHECK(history.UndoLabel() == "Delete lamp");

	// Undo the delete: same ids, names, order keys, hierarchy and component values
	REQUIRE(history.Undo(stele));
	SteleSnapshot restored = stele.Read();
	CHECK(restored.ElementCount() == 2);
	CHECK(restored.Find(bulb)->Parent == lamp);
	CHECK(restored.Find(lamp)->Order == created.Find(lamp)->Order);
	CHECK(restored.Get<Tint>(bulb)->Note == "green");
	CHECK(history.RedoLabel() == "Delete lamp");

	REQUIRE(history.Undo(stele));
	CHECK(stele.Read().ElementCount() == 0);
	CHECK(!history.CanUndo());

	REQUIRE(history.Redo(stele));
	REQUIRE(history.Redo(stele));
	CHECK(stele.Read().ElementCount() == 0);
	REQUIRE(history.Undo(stele));
	CHECK(stele.Read().Get<Tint>(bulb)->Note == "green");

	// A new change clears the redo stack
	Transaction rename("Rename", reg.Types);
	rename.Rename(lamp, "Lamp 2");
	commit(std::move(rename));
	CHECK(!history.CanRedo());
}

TEST_CASE("UndoHistory: commits with the same merge key are one step")
{
	Registry reg;
	Stele stele(reg.Types);
	UndoHistory history;

	Transaction create("Create", reg.Types);
	ElementId a = create.CreateElement("A");
	create.AddComponent<Position>(a);
	history.Record(*stele.Commit(std::move(create)).Changes);

	// A slider drag: one commit per frame, all with the gesture's key
	for (int frame = 1; frame <= 5; frame++)
	{
		Transaction drag("Drag position", reg.Types);
		drag.MergeKey = 77;
		drag.SetField<Position>(a, "Value", glm::vec3(float(frame)));
		history.Record(*stele.Commit(std::move(drag)).Changes);
	}
	CHECK(stele.Read().Get<Position>(a)->Value == glm::vec3(5.0f));

	REQUIRE(history.Undo(stele));
	CHECK(stele.Read().Get<Position>(a)->Value == glm::vec3(0.0f)); // back to before the drag
	CHECK(history.UndoLabel() == "Create");

	REQUIRE(history.Redo(stele));
	CHECK(stele.Read().Get<Position>(a)->Value == glm::vec3(5.0f));
}

TEST_CASE("UndoHistory: a step that no longer applies is dropped")
{
	Registry reg;
	Stele stele(reg.Types);
	UndoHistory mine;

	Transaction create("Create", reg.Types);
	ElementId a = create.CreateElement("A");
	stele.Commit(std::move(create));

	Transaction rename("Rename", reg.Types);
	rename.Rename(a, "Mine");
	mine.Record(*stele.Commit(std::move(rename)).Changes);

	// Someone else deletes the element; my rename can't be undone any more
	Transaction theirs("Delete", reg.Types);
	theirs.DestroyElement(a);
	REQUIRE(stele.Commit(std::move(theirs)));

	CHECK(!mine.Undo(stele));
	CHECK(!mine.CanUndo());
}

// --- Threads ---------------------------------------------------------------------------------------------

TEST_CASE("Stele: readers see consistent snapshots while another thread commits")
{
	Registry reg;
	Stele stele(reg.Types);
	constexpr int kCommits = 300;

	std::atomic<bool> done = false;
	std::atomic<int> inconsistent = 0;
	auto reader = [&] {
		while (!done)
		{
			// Every commit adds one element with a Position whose value is its version
			SteleSnapshot snap = stele.Read();
			if (snap.ElementCount() != snap.Version())
				inconsistent++;
			for (ElementId id : snap.Children())
				if (const Position* p = snap.Get<Position>(id); !p || p->Value.x > float(snap.Version()))
					inconsistent++;
		}
	};
	std::thread r1(reader), r2(reader);

	for (int i = 1; i <= kCommits; i++)
	{
		Transaction tx("Add", reg.Types);
		ElementId id = tx.CreateElement("Element");
		tx.AddComponent(id, Position{ glm::vec3(float(i)) });
		CHECK(stele.Commit(std::move(tx))); // not REQUIRE: the readers must be joined
	}
	done = true;
	r1.join();
	r2.join();

	CHECK(inconsistent == 0);
	CHECK(stele.Read().ElementCount() == kCommits);
}
