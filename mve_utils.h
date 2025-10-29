#pragma once

#include <functional>

namespace mve {
	//from: https://stackoverflow.com/a/57595105

	template <typename T, typename... Rest>
	void hashCombine(std::size_t& seed, const T& v, const Rest&... rest) {
		//^= is the XOR assignment operator, which performs a bitwise XOR operation between the current value of seed and the result of the expression on the right-hand side, and then assigns the result back to seed.
		// << and >> are bitwise shift operators that shift the bits of a number to the left or right, respectively. In this case, they are used to mix the bits of the seed value to help create a more unique hash value.
		seed ^= std::hash<T>{}(v)+0x9e3779b9 + (seed << 6) + (seed >> 2);
		(hashCombine(seed, rest), ...);
	};
}