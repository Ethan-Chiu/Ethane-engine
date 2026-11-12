#pragma once

#include <cstdint>
#include <functional>
#include <string_view>

namespace Ethane {

	// Stable identifier of a registered type: a hash of its name, so it means the same thing in saved
	// files, on the network and across builds (unlike std::type_index)
	struct TypeId
	{
		uint64_t Value = 0;

		static constexpr TypeId FromName(std::string_view name)
		{
			uint64_t hash = 0xcbf29ce484222325ull; // FNV-1a 64
			for (char c : name)
			{
				hash ^= static_cast<uint8_t>(c);
				hash *= 0x100000001b3ull;
			}
			return { hash };
		}

		constexpr bool IsValid() const { return Value != 0; }
		constexpr auto operator<=>(const TypeId&) const = default;
	};

	// Stable identifier of an element in a stele; 0 is "no element".
	// New ids are random 64-bit, so independent editors can create elements without coordinating.
	struct ElementId
	{
		uint64_t Value = 0;

		static ElementId Generate();

		constexpr bool IsValid() const { return Value != 0; }
		constexpr auto operator<=>(const ElementId&) const = default;
	};

	// What kind of file an asset is, as far as its extension tells (AssetRegistry::KindOf)
	enum class AssetKind : uint8_t { Any, Mesh, Texture };

	// Reference to an asset by its handle; 0 is "no asset"
	struct AssetId
	{
		uint64_t Value = 0;

		constexpr bool IsValid() const { return Value != 0; }
		constexpr auto operator<=>(const AssetId&) const = default;
	};

}

template<> struct std::hash<Ethane::TypeId>
{
	size_t operator()(Ethane::TypeId id) const noexcept { return std::hash<uint64_t>()(id.Value); }
};

template<> struct std::hash<Ethane::ElementId>
{
	size_t operator()(Ethane::ElementId id) const noexcept { return std::hash<uint64_t>()(id.Value); }
};

template<> struct std::hash<Ethane::AssetId>
{
	size_t operator()(Ethane::AssetId id) const noexcept { return std::hash<uint64_t>()(id.Value); }
};
