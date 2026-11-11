#include "Test.h"

#include "Ethane/Brick/Brick.h"
#include "Ethane/Brick/OrderKey.h"
#include "Ethane/Brick/UndoHistory.h"

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

	std::vector<std::string> ChildNames(const BrickSnapshot& snapshot, ObjectId parent = {})
	{
		std::vector<std::string> names;
		for (ObjectId id : snapshot.Children(parent))
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

// --- Objects, hierarchy, components ----------------------------------------------------------------------

TEST_CASE("Brick: create objects, hierarchy and sibling order")
{
	Registry reg;
	Brick brick(reg.Types);
	CHECK(brick.Read().Version() == 0);

	Transaction tx("Build", reg.Types);
	ObjectId root = tx.CreateObject("Root");
	ObjectId b = tx.CreateObject("B", root);
	ObjectId c = tx.CreateObject("C", root);
	tx.CreateObject("A", root, OrderKey::Between({}, "1")); // explicitly before everything else
	CommitResult result = brick.Commit(std::move(tx));
	REQUIRE(result);
	CHECK(result.Version == 1);

	BrickSnapshot snap = brick.Read();
	CHECK(snap.Version() == 1);
	CHECK(snap.ObjectCount() == 4);
	CHECK(ChildNames(snap) == std::vector<std::string>{ "Root" });
	CHECK(ChildNames(snap, root) == std::vector<std::string>{ "A", "B", "C" });
	CHECK(snap.Find(b)->Parent == root);

	// Move C to the top level, before Root
	Transaction move("Move", reg.Types);
	move.Move(c, {}, OrderKey::Between({}, snap.Find(root)->Order));
	REQUIRE(brick.Commit(std::move(move)));
	snap = brick.Read();
	CHECK(ChildNames(snap) == std::vector<std::string>{ "C", "Root" });
	CHECK(ChildNames(snap, root) == std::vector<std::string>{ "A", "B" });

	std::vector<std::string> depthFirst;
	snap.ForEachDepthFirst([&](const BrickObject& o) { depthFirst.push_back(o.Name); });
	CHECK(depthFirst == std::vector<std::string>{ "C", "Root", "A", "B" });
}

TEST_CASE("Brick: components and fields")
{
	Registry reg;
	Brick brick(reg.Types);

	Transaction tx("Add", reg.Types);
	ObjectId lamp = tx.CreateObject("Lamp");
	tx.AddComponent(lamp, Position{ { 1.0f, 2.0f, 3.0f } });
	tx.AddComponent<Tint>(lamp);
	REQUIRE(brick.Commit(std::move(tx)));

	BrickSnapshot before = brick.Read();
	REQUIRE(before.Get<Position>(lamp));
	CHECK(before.Get<Position>(lamp)->Value == glm::vec3(1.0f, 2.0f, 3.0f));
	CHECK(before.Get<Tint>(lamp)->Color == glm::vec4(1.0f));

	Transaction edit("Edit", reg.Types);
	edit.SetField<Tint>(lamp, "Color", glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
	edit.SetField<Tint>(lamp, "Note", std::string("red"));
	REQUIRE(brick.Commit(std::move(edit)));

	BrickSnapshot after = brick.Read();
	CHECK(after.Get<Tint>(lamp)->Color == glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
	CHECK(after.Get<Tint>(lamp)->Note == "red");

	// The old snapshot is untouched, and the unchanged component is shared, not copied
	CHECK(before.Get<Tint>(lamp)->Color == glm::vec4(1.0f));
	CHECK(before.Get<Position>(lamp) == after.Get<Position>(lamp));
	CHECK(before.Get<Tint>(lamp) != after.Get<Tint>(lamp));

	Transaction remove("Remove", reg.Types);
	remove.RemoveComponent<Tint>(lamp);
	REQUIRE(brick.Commit(std::move(remove)));
	CHECK(brick.Read().Get<Tint>(lamp) == nullptr);
	CHECK(brick.Read().Get<Position>(lamp) != nullptr);
}

TEST_CASE("Brick: unchanged objects are shared between snapshots")
{
	Registry reg;
	Brick brick(reg.Types);
	Transaction tx("Add", reg.Types);
	ObjectId a = tx.CreateObject("A");
	ObjectId b = tx.CreateObject("B");
	REQUIRE(brick.Commit(std::move(tx)));
	BrickSnapshot before = brick.Read();

	Transaction rename("Rename", reg.Types);
	rename.Rename(a, "A2");
	REQUIRE(brick.Commit(std::move(rename)));
	BrickSnapshot after = brick.Read();

	CHECK(before.Find(a)->Name == "A");
	CHECK(after.Find(a)->Name == "A2");
	CHECK(before.Find(b) == after.Find(b));
}

TEST_CASE("Brick: a failing op refuses the whole transaction")
{
	Registry reg;
	Brick brick(reg.Types);
	Transaction tx("Add", reg.Types);
	ObjectId parent = tx.CreateObject("Parent");
	ObjectId child = tx.CreateObject("Child", parent);
	tx.AddComponent<Position>(child);
	REQUIRE(brick.Commit(std::move(tx)));

	auto refused = [&](auto build) {
		Transaction bad("Bad", reg.Types);
		bad.Rename(parent, "renamed first"); // must not survive the failure
		build(bad);
		CommitResult result = brick.Commit(std::move(bad));
		return !result && !result.Error.empty();
	};

	CHECK(refused([&](Transaction& t) { t.SetField<Position>(child, "Value", 1.0f); }));          // wrong value type
	CHECK(refused([&](Transaction& t) { t.SetField<Position>(child, "Missing", 1.0f); }));        // unknown field
	CHECK(refused([&](Transaction& t) { t.SetField<Tint>(child, "Note", std::string()); }));      // component missing
	CHECK(refused([&](Transaction& t) { t.AddComponent<Position>(child); }));                     // already has it
	CHECK(refused([&](Transaction& t) { t.AddComponent<int>(child); }));                          // unregistered type
	CHECK(refused([&](Transaction& t) { t.DestroyObject(parent); }));                             // has children
	CHECK(refused([&](Transaction& t) { t.Move(parent, child); }));                               // cycle
	CHECK(refused([&](Transaction& t) { t.CreateObject(child, "Duplicate"); }));                  // id taken
	CHECK(refused([&](Transaction& t) { t.CreateObject("Orphan", ObjectId{ 12345 }); }));         // no parent
	CHECK(refused([&](Transaction& t) { t.Move(child, {}, "10"); }));                             // invalid key

	CHECK(brick.Read().Version() == 1);
	CHECK(brick.Read().Find(parent)->Name == "Parent");
}

TEST_CASE("Brick: DestroyTree removes descendants")
{
	Registry reg;
	Brick brick(reg.Types);
	Transaction tx("Add", reg.Types);
	ObjectId root = tx.CreateObject("Root");
	ObjectId mid = tx.CreateObject("Mid", root);
	tx.CreateObject("Leaf", mid);
	ObjectId other = tx.CreateObject("Other");
	REQUIRE(brick.Commit(std::move(tx)));

	Transaction destroy("Delete", reg.Types);
	destroy.DestroyTree(brick.Read(), root);
	REQUIRE(brick.Commit(std::move(destroy)));

	BrickSnapshot snap = brick.Read();
	CHECK(snap.ObjectCount() == 1);
	CHECK(snap.Contains(other));
	CHECK(snap.Children(root).empty());
}

// --- Change sets and undo --------------------------------------------------------------------------------

TEST_CASE("Brick: change sets list applied ops and reach subscribers")
{
	Registry reg;
	Brick brick(reg.Types);

	std::vector<std::pair<uint64_t, uint64_t>> seen;
	BrickSubscription subscription = brick.Subscribe([&](const ChangeSet& c) {
		seen.emplace_back(c.FromVersion, c.ToVersion);
		CHECK(c.After.Version() == c.ToVersion);
	});

	Transaction tx("Add", reg.Types);
	ObjectId a = tx.CreateObject("A");
	tx.AddComponent<Position>(a, {});
	CommitResult result = brick.Commit(std::move(tx));
	REQUIRE(result.Changes);

	// The applied ops carry what the commit filled in: the order key and the default component
	const auto& create = std::get<BrickOp::CreateObject>(result.Changes->Ops[0]);
	CHECK(!create.Order.empty());
	CHECK(create.Order == brick.Read().Find(a)->Order);
	CHECK(std::get<BrickOp::AddComponent>(result.Changes->Ops[1]).Value != nullptr);

	CHECK(brick.Commit(Transaction("Empty", reg.Types)));   // empty: nothing published
	subscription.Reset();
	Transaction later("Later", reg.Types);
	later.Rename(a, "B");
	REQUIRE(brick.Commit(std::move(later)));

	CHECK(seen == std::vector<std::pair<uint64_t, uint64_t>>{ { 0, 1 } });

	// A subscription may outlive its brick
	BrickSubscription orphan;
	{
		Brick temporary(reg.Types);
		orphan = temporary.Subscribe([](const ChangeSet&) {});
	}
	orphan.Reset();
}

TEST_CASE("UndoHistory: undo and redo restore exact states")
{
	Registry reg;
	Brick brick(reg.Types);
	UndoHistory history;
	auto commit = [&](Transaction tx) {
		CommitResult result = brick.Commit(std::move(tx));
		REQUIRE(result);
		history.Record(*result.Changes);
	};

	Transaction create("Create lamp", reg.Types);
	ObjectId lamp = create.CreateObject("Lamp");
	ObjectId bulb = create.CreateObject("Bulb", lamp);
	create.AddComponent(bulb, Tint{ { 0.0f, 1.0f, 0.0f, 1.0f }, "green" });
	commit(std::move(create));
	const BrickSnapshot created = brick.Read();

	Transaction destroy("Delete lamp", reg.Types);
	destroy.DestroyTree(brick.Read(), lamp);
	commit(std::move(destroy));
	CHECK(brick.Read().ObjectCount() == 0);
	CHECK(history.UndoLabel() == "Delete lamp");

	// Undo the delete: same ids, names, order keys, hierarchy and component values
	REQUIRE(history.Undo(brick));
	BrickSnapshot restored = brick.Read();
	CHECK(restored.ObjectCount() == 2);
	CHECK(restored.Find(bulb)->Parent == lamp);
	CHECK(restored.Find(lamp)->Order == created.Find(lamp)->Order);
	CHECK(restored.Get<Tint>(bulb)->Note == "green");
	CHECK(history.RedoLabel() == "Delete lamp");

	REQUIRE(history.Undo(brick));
	CHECK(brick.Read().ObjectCount() == 0);
	CHECK(!history.CanUndo());

	REQUIRE(history.Redo(brick));
	REQUIRE(history.Redo(brick));
	CHECK(brick.Read().ObjectCount() == 0);
	REQUIRE(history.Undo(brick));
	CHECK(brick.Read().Get<Tint>(bulb)->Note == "green");

	// A new change clears the redo stack
	Transaction rename("Rename", reg.Types);
	rename.Rename(lamp, "Lamp 2");
	commit(std::move(rename));
	CHECK(!history.CanRedo());
}

TEST_CASE("UndoHistory: commits with the same merge key are one step")
{
	Registry reg;
	Brick brick(reg.Types);
	UndoHistory history;

	Transaction create("Create", reg.Types);
	ObjectId a = create.CreateObject("A");
	create.AddComponent<Position>(a);
	history.Record(*brick.Commit(std::move(create)).Changes);

	// A slider drag: one commit per frame, all with the gesture's key
	for (int frame = 1; frame <= 5; frame++)
	{
		Transaction drag("Drag position", reg.Types);
		drag.MergeKey = 77;
		drag.SetField<Position>(a, "Value", glm::vec3(float(frame)));
		history.Record(*brick.Commit(std::move(drag)).Changes);
	}
	CHECK(brick.Read().Get<Position>(a)->Value == glm::vec3(5.0f));

	REQUIRE(history.Undo(brick));
	CHECK(brick.Read().Get<Position>(a)->Value == glm::vec3(0.0f)); // back to before the drag
	CHECK(history.UndoLabel() == "Create");

	REQUIRE(history.Redo(brick));
	CHECK(brick.Read().Get<Position>(a)->Value == glm::vec3(5.0f));
}

TEST_CASE("UndoHistory: a step that no longer applies is dropped")
{
	Registry reg;
	Brick brick(reg.Types);
	UndoHistory mine;

	Transaction create("Create", reg.Types);
	ObjectId a = create.CreateObject("A");
	brick.Commit(std::move(create));

	Transaction rename("Rename", reg.Types);
	rename.Rename(a, "Mine");
	mine.Record(*brick.Commit(std::move(rename)).Changes);

	// Someone else deletes the object; my rename can't be undone any more
	Transaction theirs("Delete", reg.Types);
	theirs.DestroyObject(a);
	REQUIRE(brick.Commit(std::move(theirs)));

	CHECK(!mine.Undo(brick));
	CHECK(!mine.CanUndo());
}

// --- Threads ---------------------------------------------------------------------------------------------

TEST_CASE("Brick: readers see consistent snapshots while another thread commits")
{
	Registry reg;
	Brick brick(reg.Types);
	constexpr int kCommits = 300;

	std::atomic<bool> done = false;
	std::atomic<int> inconsistent = 0;
	auto reader = [&] {
		while (!done)
		{
			// Every commit adds one object with a Position whose value is its version
			BrickSnapshot snap = brick.Read();
			if (snap.ObjectCount() != snap.Version())
				inconsistent++;
			for (ObjectId id : snap.Children())
				if (const Position* p = snap.Get<Position>(id); !p || p->Value.x > float(snap.Version()))
					inconsistent++;
		}
	};
	std::thread r1(reader), r2(reader);

	for (int i = 1; i <= kCommits; i++)
	{
		Transaction tx("Add", reg.Types);
		ObjectId id = tx.CreateObject("Object");
		tx.AddComponent(id, Position{ glm::vec3(float(i)) });
		CHECK(brick.Commit(std::move(tx))); // not REQUIRE: the readers must be joined
	}
	done = true;
	r1.join();
	r2.join();

	CHECK(inconsistent == 0);
	CHECK(brick.Read().ObjectCount() == kCommits);
}
