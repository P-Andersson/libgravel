#pragma once

#include <concepts>
#include <type_traits>
#include <cstring>
#include <cstdint>
#include <span>

namespace gravel
{
	namespace detail
	{
		template <typename BaseT>
		class IOperationsTable
		{
		public:
			virtual void clone(std::span<std::uint8_t> small_buffer, std::span<std::uint8_t> op_table, bool& out_local, const BaseT* src) const = 0;
			virtual void move(std::span<std::uint8_t> small_buffer, std::span<std::uint8_t> op_table, bool& out_local, BaseT* src, bool src_local) const = 0;
		};

		template <typename BaseT, typename SubT>
		class OperationsTable : public IOperationsTable<BaseT>
		{
		public:
			void move(std::span<std::uint8_t> small_buffer, std::span<std::uint8_t> op_table, bool& out_local, BaseT* src, bool src_local) const override
			{
				if constexpr (std::is_move_constructible<BaseT>::value)
				{
					SubT* casted_source = static_cast<SubT*>(src);
					if (sizeof(SubT) > small_buffer.size_bytes())
					{
						out_local = false;
						if (!src_local)
						{
							std::memcpy(small_buffer.data(), casted_source, sizeof(SubT*));
							std::memset(src, 0, sizeof(src));
						}
						else
						{
							SubT* created = new SubT(std::move(*casted_source));
							std::memcpy(small_buffer.data(), &created, sizeof(SubT*));
						}
					}
					else
					{
						out_local = true;
						new (small_buffer.data()) SubT(std::move(*casted_source));
					}
					new (op_table.data()) OperationsTable<BaseT, SubT>();
				}
			}

			void clone(std::span<std::uint8_t> small_buffer, std::span<std::uint8_t> op_table, bool& out_local, const BaseT* src) const override
			{
				if constexpr (std::is_copy_constructible<BaseT>::value)
				{
					const SubT* casted_source = static_cast<const SubT*>(src);
					if (sizeof(SubT) > small_buffer.size_bytes())
					{
						out_local = false;
						SubT* created = new SubT(*casted_source);
						std::memcpy(small_buffer.data(), &created, sizeof(SubT*));
					}
					else
					{
						out_local = true;
						new (small_buffer.data()) SubT(*casted_source);
					}
					new (op_table.data()) OperationsTable<BaseT, SubT>();
				}
			}
		};

	}
}