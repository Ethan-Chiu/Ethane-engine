#include "Test.h"

#include "Ethane/Schema/FieldAccess.h"
#include "Ethane/Schema/TypeRegistry.h"

using namespace Ethane;

namespace {

	struct Light
	{
		glm::vec3 Color = { 1.0f, 1.0f, 1.0f };
		float Intensity = 1.0f;
	};

	struct Lamp
	{
		std::string Label = "lamp";
		Light Bulb;
		AssetId Mesh;
		ObjectId Target;
		int32_t Cached = 0;
	};

	void RegisterTestTypes(TypeRegistry& registry)
	{
		registry.Register<Light>("Test.Light")
			.Field("Color", &Light::Color, FieldFlags::Color)
			.Field("Intensity", &Light::Intensity, FieldFlags::None, { .Min = 0.0f, .Max = 100.0f });
		registry.Register<Lamp>("Test.Lamp", 3)
			.Field("Label", &Lamp::Label)
			.Field("Bulb", &Lamp::Bulb)
			.Field("Mesh", &Lamp::Mesh)
			.Field("Target", &Lamp::Target)
			.Field("Cached", &Lamp::Cached, FieldFlags::RuntimeOnly);
	}

}

TEST_CASE("Schema: TypeId is a stable hash of the name")
{
	constexpr TypeId id = TypeId::FromName("Transform");
	static_assert(id.IsValid());
	CHECK(id == TypeId::FromName("Transform"));
	CHECK(id != TypeId::FromName("transform"));
	CHECK(TypeId::FromName("").Value == 0xcbf29ce484222325ull); // FNV-1a offset basis
}

TEST_CASE("Schema: registration records layout, fields and flags")
{
	TypeRegistry registry;
	RegisterTestTypes(registry);

	const TypeInfo* lamp = registry.Find<Lamp>();
	REQUIRE(lamp);
	CHECK(lamp == registry.Find("Test.Lamp"));
	CHECK(lamp == registry.Find(TypeId::FromName("Test.Lamp")));
	CHECK(lamp->Size == sizeof(Lamp));
	CHECK(lamp->Align == alignof(Lamp));
	CHECK(lamp->Version == 3);
	REQUIRE(lamp->Fields.size() == 5);

	const FieldInfo& bulb = lamp->Fields[1];
	CHECK(bulb.Name == "Bulb");
	CHECK(bulb.Type == FieldType::Struct);
	CHECK(bulb.Offset == offsetof(Lamp, Bulb));
	CHECK(bulb.Struct == registry.Find<Light>());

	CHECK(lamp->FindField("Mesh")->Type == FieldType::Asset);
	CHECK(lamp->FindField("Target")->Type == FieldType::Object);
	CHECK(HasFlag(lamp->FindField("Cached")->Flags, FieldFlags::RuntimeOnly));
	CHECK(lamp->FindField("Missing") == nullptr);

	const FieldInfo& intensity = registry.Find<Light>()->Fields[1];
	CHECK(intensity.Meta.Max == 100.0f);
	CHECK(intensity.Offset == offsetof(Light, Intensity));

	const auto types = registry.Types();
	REQUIRE(types.size() == 2);
	CHECK(types[0]->Name == "Test.Lamp");
	CHECK(registry.Find<int>() == nullptr);
}

TEST_CASE("Schema: field paths resolve through nested structs")
{
	TypeRegistry registry;
	RegisterTestTypes(registry);
	const TypeInfo& lamp = *registry.Find<Lamp>();

	auto path = ParseFieldPath(lamp, "Bulb.Intensity");
	REQUIRE(path);
	CHECK(path->Indices == std::vector<uint16_t>{ 1, 1 });
	CHECK(FormatFieldPath(lamp, *path) == "Bulb.Intensity");

	auto resolved = Resolve(lamp, *path);
	REQUIRE(resolved);
	CHECK(resolved->Offset == offsetof(Lamp, Bulb) + offsetof(Light, Intensity));

	CHECK(!ParseFieldPath(lamp, "Bulb.Missing"));
	CHECK(!ParseFieldPath(lamp, "Label.Length")); // Label isn't a struct
	CHECK(!ParseFieldPath(lamp, ""));
	CHECK(!Resolve(lamp, FieldPath{ { 9 } }));
	CHECK(!Resolve(lamp, FieldPath{}));
}

TEST_CASE("Schema: type-erased get and set")
{
	TypeRegistry registry;
	RegisterTestTypes(registry);
	const TypeInfo& lamp = *registry.Find<Lamp>();

	Lamp object;
	const FieldPath color = *ParseFieldPath(lamp, "Bulb.Color");
	const FieldPath label = *ParseFieldPath(lamp, "Label");
	const FieldPath target = *ParseFieldPath(lamp, "Target");

	CHECK(SetField(lamp, &object, color, glm::vec3(0.5f, 0.25f, 0.0f)));
	CHECK(object.Bulb.Color == glm::vec3(0.5f, 0.25f, 0.0f));
	CHECK(SetField(lamp, &object, label, std::string("desk lamp")));
	CHECK(object.Label == "desk lamp");
	CHECK(SetField(lamp, &object, target, ObjectId{ 42 }));
	CHECK(object.Target == ObjectId{ 42 });

	CHECK(!SetField(lamp, &object, color, 1.0f));                           // wrong value type
	CHECK(!SetField(lamp, &object, *ParseFieldPath(lamp, "Bulb"), 1.0f));   // whole struct
	CHECK(object.Bulb.Color == glm::vec3(0.5f, 0.25f, 0.0f));

	auto read = GetField(lamp, &object, label);
	REQUIRE(read);
	CHECK(std::get<std::string>(*read) == "desk lamp");
	CHECK(!GetField(lamp, &object, *ParseFieldPath(lamp, "Bulb")));
}

TEST_CASE("Schema: lifetime operations and comparison")
{
	TypeRegistry registry;
	RegisterTestTypes(registry);
	const TypeInfo& lamp = *registry.Find<Lamp>();

	alignas(Lamp) std::byte a[sizeof(Lamp)];
	alignas(Lamp) std::byte b[sizeof(Lamp)];
	lamp.Construct(a);
	reinterpret_cast<Lamp*>(a)->Label = "a long label that does not fit the small-string buffer";
	lamp.CopyConstruct(b, a);
	CHECK(reinterpret_cast<Lamp*>(b)->Label == reinterpret_cast<Lamp*>(a)->Label);
	CHECK(FieldsEqual(lamp, a, b));

	reinterpret_cast<Lamp*>(b)->Bulb.Intensity = 2.0f;
	CHECK(!FieldsEqual(lamp, a, b));
	lamp.CopyAssign(b, a);
	CHECK(FieldsEqual(lamp, a, b));

	lamp.Destroy(a);
	lamp.Destroy(b);
}

TEST_CASE("Schema: leaf visitor walks nested fields in order")
{
	TypeRegistry registry;
	RegisterTestTypes(registry);
	const TypeInfo& lamp = *registry.Find<Lamp>();

	std::vector<std::string> names;
	VisitLeafFields(lamp, [&](const FieldPath& path, const FieldInfo&, uint32_t offset) {
		names.push_back(FormatFieldPath(lamp, path));
		CHECK(offset == Resolve(lamp, path)->Offset);
	});
	CHECK(names == std::vector<std::string>{ "Label", "Bulb.Color", "Bulb.Intensity", "Mesh", "Target", "Cached" });
}

TEST_CASE("Schema: ObjectId::Generate never returns the null id")
{
	ObjectId a = ObjectId::Generate();
	ObjectId b = ObjectId::Generate();
	CHECK(a.IsValid());
	CHECK(b.IsValid());
	CHECK(a != b);
}
