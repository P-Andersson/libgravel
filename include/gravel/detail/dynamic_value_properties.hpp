
#pragma once

#include <array>
#include <span>
#include <cstdint>
#include <cstring>

#include "detail/concepts.hpp"
#include "detail/operations_table.hpp"

namespace gravel
{

	template <typename T>
	std::size_t consteval default_buffer_size()
	{
		return std::max(sizeof(T) + sizeof(void*), 4 * sizeof(void*));
	}

	enum class Attr
	{
		None = 0x00,
		Copyable = 0x01,
		Movable = 0x02,
		Default = 0xFF,
	};

	template <typename T>
	Attr consteval default_attributes()
	{
		return static_cast<Attr>(
			static_cast<int>(CopyConstructible<T> ? Attr::Copyable : Attr::None) |
			static_cast<int>(MoveConstructible<T> ? Attr::Movable : Attr::None)
			);
	}


	template<Attr Attributes = Attr::Default, std::size_t SmallBufferSize = 0>
	class Properties
	{
	public:
		Properties() = delete;

		static const Attr attributes = Attributes;
		static const std::size_t small_buffer_size = SmallBufferSize;
	};

	template<std::size_t SmallBufferSize>
	using BufferSize = Properties<Attr::Default, SmallBufferSize>;

	template<typename BaseT, typename PropertiesT>
	struct DynamicValProperties
	{
		DynamicValProperties() = delete;

		static const bool copyable = (static_cast<int>(PropertiesT::attributes == Attr::Default ? default_attributes<BaseT>() : PropertiesT::attributes))
			& static_cast<int>(Attr::Copyable);
		static const bool moveable = (static_cast<int>(PropertiesT::attributes == Attr::Default ? default_attributes<BaseT>() : PropertiesT::attributes))
			& static_cast<int>(Attr::Movable);
		static const std::size_t small_buffer_size = PropertiesT::small_buffer_size == 0 ? default_buffer_size<BaseT>() : PropertiesT::small_buffer_size;
	};
}