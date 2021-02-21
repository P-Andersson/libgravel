#pragma once

#include <concepts>

namespace gravel
{
	template<typename BaseT, typename T>
	concept IsBaseOf = std::is_same<BaseT, T>::value || std::is_base_of<BaseT, T>::value;

	template<typename T>
	concept CopyConstructible = std::is_copy_constructible<T>::value;

	template<typename T>
	concept MoveConstructible = std::is_move_constructible<T>::value;
}