#include "ethpch.h"
#include "Ethane/Brick/OrderKey.h"

namespace Ethane::OrderKey {

	namespace {

		constexpr std::string_view kDigits = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
		constexpr int kBase = 62;

		int DigitValue(char c)
		{
			if (c >= '0' && c <= '9') return c - '0';
			if (c >= 'A' && c <= 'Z') return c - 'A' + 10;
			if (c >= 'a' && c <= 'z') return c - 'a' + 36;
			return -1;
		}

		// Midpoint of two fractions; `after` absent means 1.0 (after the last possible key)
		std::string Midpoint(std::string_view before, std::string_view after, bool hasAfter)
		{
			if (hasAfter)
			{
				// Shared leading digits (with `before` padded by zeros) are kept as they are
				size_t n = 0;
				while (n < after.size() && (n < before.size() ? before[n] : '0') == after[n])
					n++;
				if (n > 0)
					return std::string(after.substr(0, n)) +
						Midpoint(n < before.size() ? before.substr(n) : std::string_view(), after.substr(n), true);
			}

			const int low = before.empty() ? 0 : DigitValue(before[0]);
			const int high = hasAfter ? DigitValue(after[0]) : kBase;
			if (high - low > 1)
				return std::string(1, kDigits[(low + high + 1) / 2]);

			// Adjacent first digits: a longer `after` can be cut after its first digit; otherwise keep
			// `before`'s first digit and go one level deeper
			if (hasAfter && after.size() > 1)
				return std::string(1, after[0]);
			return std::string(1, kDigits[low]) + Midpoint(before.empty() ? before : before.substr(1), {}, false);
		}

	}

	std::string Between(std::string_view before, std::string_view after)
	{
		ETH_CORE_ASSERT(IsValid(before) && IsValid(after), "OrderKey::Between: invalid key");
		ETH_CORE_ASSERT(before.empty() || after.empty() || before < after, "OrderKey::Between: keys out of order");
		return Midpoint(before, after, !after.empty());
	}

	bool IsValid(std::string_view key)
	{
		if (!key.empty() && key.back() == '0')
			return false;
		for (char c : key)
			if (DigitValue(c) < 0)
				return false;
		return true;
	}

}
