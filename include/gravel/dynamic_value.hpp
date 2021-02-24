#pragma once

#include <array>
#include <span>
#include <cstdint>
#include <cstring>

#include "detail/concepts.hpp"
#include "detail/dynamic_value_properties.hpp"
#include "detail/operations_table.hpp"

namespace gravel
{

	//!
	//! Supports dynamic object polymorphism in a value-based way leveraging your compilers standard method of handling 
	//! it (typically vtables).
	//!
	//! These objects abstractly work in a value based manner, with copies and moves, but it lets you also dynamically set subtypes in it
	//! and have them work with this interface too.
	//! It internally uses small object optimization, but is permitted to use heap allocation should any assigned object not fit. This
	//! should otherwise be transparent to the client programmer.
	//!
	//! NOTE: Just like other value objects, it's not safe to dereference a pointer or use a reference to the value held in the dynamic_value
	//! after it has been moved, gone out of scope or otherwise have been destroyed.
	//! 
	//! NOTE: dynamic_value has the following properties:
	//! 
	//! * After created by a constructor, it is guaranteed to hold a valid value. Thus there is no null value,
	//!   you should wrap it in a std::optional if you want that for a certain use case.
	//! * Whether the dynamic_value is moveable or copyable is controlled by the dynamic value types Property template argument
	//! * If copyable, only concrete types with copy constructors can be assigned to the dynamic value
	//! * If moveable, only concrete types with move constructors can be assigned to the dynamic value
	//! * Only non-abstract types of type BaseT or inheriting from a BaseT (directly or indirectly) can be assigned to the dynamic value
	//! * Any instance moved or copied into the dynamic_value will be copied by it's copy constructor and moved by either it's move constructor or by
	//!   internal pointer swap (if too large for small buffer optimization). The copy assignment operator and the move assignment will never be used.
	//! * A moved dynamic_value is safe to emplace or otherwise assign to, but NOT safe to use.
	//! 
	//! @tparam BaseT the Base type that the dynamic_value shall hold
	//! @tparam PropertiesT	must be a specialization of gravel::Properties which as the following configuration options:
	//!				* Attributes:	controls if the dynamic_value should be moveable, copyable, both or neither. Can be:
	//!							Attr::None - dynamic_value is not copyable nor moveable
	//!							Attr::Copyable	- dynamic_value is copyable but not moveable, moves will be treated as a copy
	//!							Attr::Moveable	- dynamic_value is moveable but not copyable
	//!							Attr::Default	- dynamic_value is copyable if BaseT is abstract or has a copy constructor, it is moveable
	//!                                                    if BaseT is abstract or has a move constructor
	//!				* SmallBufferSize:	objects with a size of this or smaller will not do a heap allocation if put into the dynamic value, if 0
	//!											an object appropiate size will automatically selected. Must be at least the same size as sizeof(BaseT*)
	//!				Defaults to Attr::Default and 0
	template <typename BaseT, typename PropertiesT = Properties<> >
	class dynamic_value 
	{
	public:
		using properties = DynamicValProperties<BaseT, PropertiesT>;
		static_assert(properties::small_buffer_size >= sizeof(BaseT*));


		//!
		//! Constructor, copy-constructs an object as the held value
		//! @tparam	T	the type of the object to copy construct, must be either the same as BaseT or a child-type of it
		//! @param other	the object to copy from
		//! 
		template <typename T> 
		explicit dynamic_value(const T& other) requires IsBaseOf<BaseT, std::decay_t<T>> && properties::copyable
			: m_local(false)
			, m_buffer({0})
			, m_op_table({0})
		{
			set(other); 
		}

		//!
		//! Constructor, move-constructs an object as the held value
		//! @tparam	T	the type of the object to move construct, must be either the same as BaseT or a child-type of it
		//! @param other	the object to move from
		//! 
		template <typename T> 
		explicit dynamic_value(T&& other) requires IsBaseOf<BaseT, std::decay_t<T>> && (!std::is_lvalue_reference<T>::value) && (properties::moveable || properties::copyable)
			: m_local(false)
			, m_buffer({0})
			, m_op_table({0})
		{
			if constexpr (properties::moveable)
			{
				set(std::forward<T>(other));
			}
			else
			{
				set(other);
			}
		}

		//!
		//! Constructor, copy-constructs from another dynamic value
		//! @param other	the dynamic_value to copy from
		//! 
		dynamic_value(const dynamic_value<BaseT, PropertiesT>& other) requires properties::copyable
			: m_local(false)
			, m_buffer({0})
			, m_op_table({0})
		{
			do_clone(other);
		}

		//!
		//! Constructor, move-constructs from another dynamic value
		//! @param other	the dynamic_value to move from
		//! @note	other should not be used without having a new value assigned to it first after this function is called
		//! 
		dynamic_value(dynamic_value<BaseT, PropertiesT>&& other) requires properties::copyable && properties::moveable
			: m_local(false)
			, m_buffer({0})
			, m_op_table({0})
		{
			if constexpr (properties::moveable)
			{
				do_move(std::move(other));
			}
			else
			{
				do_clone(other);
			}
		}

		//!
		//! Constructor, copy-constructs from another dynamic value
		//! @tparam	OtherBaseT	the Base of the other dynamic value, must be the same as BaseT or a child of it
		//! @param other	the dynamic_value to copy from
		//! 
		template <typename OtherBaseT, typename OtherPropertiesT>
		dynamic_value(const dynamic_value<OtherBaseT, OtherPropertiesT>& other) requires IsBaseOf<BaseT, OtherBaseT> && properties::copyable
			: m_local(false)
			, m_buffer({0})
			, m_op_table({0})
		{
			do_clone(other);
		}

		//!
		//! Constructor, move-constructs from another dynamic value
		//! @tparam	OtherBaseT	the Base of the other dynamic value, must be the same as BaseT or a child of it
		//! @param other	the dynamic_value to move from
 		//! @note	other should not be used without having a new value assigned to it first after this function is called
		//! 
		template <typename OtherBaseT, typename OtherPropertiesT>
		dynamic_value(dynamic_value<OtherBaseT, OtherPropertiesT>&& other) requires IsBaseOf<BaseT, OtherBaseT> && (properties::moveable || properties::copyable)
			: m_local(false)
			, m_buffer({0})
			, m_op_table({0})
		{
			if constexpr (properties::moveable)
			{
				do_move(std::move(other));
			}
			else
			{
				do_clone(other);
			}
		}

		//!
		//! Destructor, will also destroy held object
		//! 
		~dynamic_value()
		{
			destroy();
		}


		//!
		//! Constructs a dynamic value with a new value, in-place. Should avoid copy/move by using Return-Value Optimization
		//! @tparam T	the inner type to construct, must be the same as BaseT or a child type of it
		//! @tparam ArgT	the argument types to pass to T's constructor
		//! @param	args	the arguments to perfectly forward to T's constructor
		//! @return the created dynamic value
		//! 
		template <typename T, typename... ArgT>		
		static dynamic_value<BaseT, PropertiesT> make_emplaced(ArgT&&... arguments) requires IsBaseOf<BaseT, T>&& requires (ArgT&&... args) { T(std::forward<ArgT>(args)...); }
		{
			return dynamic_value<BaseT, PropertiesT>(static_cast<T*>(nullptr), std::forward<ArgT>(arguments)...);
		}

		//!
		//! Sets a new value in this dynamic value, constructing it in-place.
		//! @tparam T	the inner type to construct, must be the same as BaseT or a child type of it
		//! @tparam ArgT	the argument types to pass to T's constructor
		//! @param	args	the arguments to perfectly forward to T's constructor
		//! 
		template <typename T, typename... ArgT>
		void emplace(ArgT&&... arguments) requires IsBaseOf<BaseT, T>&& requires (ArgT&&... args) { T(std::forward<ArgT>(args)...); }
		{
			destroy();
			set_emplace<T>(std::forward<ArgT>(arguments)...);
		}

		//!
		//! Copy-assign a value to this dynamic_value
		//! @tparam	T	the type of the object to copy, must be either the same as BaseT or a child-type of it
		//! @param other	the object to copy, it's copy constructor will be used
		//! @return a reference to this
		//! 
		template <typename T> 
		dynamic_value& operator=(const T& other) requires IsBaseOf<BaseT, std::decay_t<T> > && properties::copyable
		{
			destroy();
			set(other);
			return *this;
		}

		//!
		//! Move-assign a value to this dynamic_value
		//! @tparam	T	the type of the object to move, must be either the same as BaseT or a child-type of it
		//! @param other	the object to move, it's move constructor will be used
		//! @return a reference to this
		//! 
		template <typename T> 
		dynamic_value& operator=(T&& other) requires IsBaseOf<BaseT, std::decay_t<T>> && (!std::is_lvalue_reference<T>::value) && (properties::moveable || properties::copyable)
		{
			using NakedT = std::decay_t<T>;
			destroy();
			if constexpr (properties::moveable)
			{
				set<NakedT>(std::forward<T>(other));
			}
			else
			{
				set<NakedT>(other);
			}

			return *this;
		}

		//!
		//! Copy-assign another dynaimc_value to this dynamic_value
		//! @param other	the dynamic_value to copy from
		//! @return a reference to this
		//! 
		dynamic_value& operator=(const dynamic_value<BaseT, PropertiesT>& other) requires properties::copyable
		{
			destroy();
			do_clone(other);
			return *this;
		}

		//!
		//! Move-assigns another dynamic_value to this dynamic_value
		//! @param other	the dynamic_value to move from
		//! @note	other should not be used without having a new value assigned to it first after this function is called
		//! @return a reference to this
		//! 
		dynamic_value& operator=(dynamic_value<BaseT, PropertiesT>&& other) noexcept requires properties::coyable || properties::moveable
		{
			destroy();
			if constexpr (properties::moveable)
			{
				do_move(std::move(other));
			}
			else
			{
				do_clone(other);
			}
			return *this;
		}

		//!
		//! Copy-assigns another dynamic value into this dynamic value
 		//! @tparam	OtherBaseT	the Base of the other dynamic value, must be the same as BaseT or a child of it
		//! @param other	the dynamic_value to copy from
		//! @return a reference to this
		//! 
		template <typename OtherBaseT, typename OtherPropertiesT>
		dynamic_value& operator=(const dynamic_value<OtherBaseT, OtherPropertiesT>& other)  requires IsBaseOf<BaseT, OtherBaseT> && properties::copyable
		{
			destroy();
			do_clone(other);
			return *this;
		}

		//!
		//! Move-assigns another dynamic value from another dynamic value
		//! @tparam	OtherBaseT	the Base of the other dynamic value, must be the same as BaseT or a child of it
		//! @param other	the dynamic_value to move from
		//! @note	other should not be used without having a new value assigned to it first after this function is called
		//! @return a reference to this
		//! 
		template <typename OtherBaseT, typename OtherPropertiesT>
		dynamic_value& operator=(dynamic_value<OtherBaseT, OtherPropertiesT>&& other)  requires IsBaseOf<BaseT, OtherBaseT> && (properties::moveable || properties::copyable)
		{
			destroy();
			if constexpr (properties::moveable)
			{
				do_move(std::move(other));
			}
			else
			{
				do_clone(other);
			}
			return *this;
		}
		
		//!
		//! Access the const value held in the dynamic value
		//! @return a pointer to the held value, of BaseT type
		//! 
		const BaseT* operator->() const
		{
			return &get();
		}

		//!
		//! Access the value held in the dynamic value
		//! @return a pointer to the held value, of BaseT type
		//! 
		BaseT* operator->()
		{
			return &get();
		}

		//!
		//! Access the const value held in the dynamic value
		//! @return a pointer to the held value, of BaseT type
		//! 
   	const BaseT& operator*() const
		{
			return get();
		}

		//!
		//! Access the value held in the dynamic value
		//! @return a pointer to the held value, of BaseT type
		//! 
		BaseT& operator*()
		{
			return get();
		}
		
		//!
		//! Access the const value held in the dynamic value
		//! @return a refernce to the held value, of BaseT type
		//! 
		const BaseT& get() const
		{
			if (m_local)
			{
				// TODO: Is std::launder needed here?
				return *reinterpret_cast<const BaseT*>(m_buffer.data());
			}
			else
			{
				return **reinterpret_cast<BaseT* const*>(m_buffer.data());
			}
		}

		//!
		//! Access the value held in the dynamic value
		//! @return a refernce to the held value, of BaseT type
		//! 
		BaseT& get()
		{
			if (m_local)
			{
				// TODO: Is std::launder needed here?
				return *reinterpret_cast<BaseT*>(m_buffer.data());
			}
			else
			{
				return **reinterpret_cast<BaseT**>(m_buffer.data());
			}
		}
	private:
		friend class dynamic_value;

		using IOperationsTable = detail::IOperationsTable<BaseT>;

		const static std::size_t op_table_size = (properties::copyable || properties::moveable) ? sizeof(IOperationsTable) : 0;

		// Note: Dummy us used to differentiate between the main constructors 
		//       and this emplacement constructor. Kept private to keep a nice API
		template <typename T, typename... ArgT>
		explicit dynamic_value(T* dummy, ArgT&&... arguments)
		{
			set_emplace<T>(std::forward<ArgT>(arguments)...);
		}

		template <typename T, typename... ArgT>
		void set_emplace(ArgT&&... arguments) requires IsBaseOf<BaseT, std::decay_t<T>>
		{
			if constexpr (sizeof(T) > properties::small_buffer_size)
			{
				m_local = false;
				T* created = new T(std::forward<ArgT>(arguments)...);
				std::memcpy(m_buffer.data(), &created, sizeof(T*));
			}
			else
			{
				m_local = true;
				new (m_buffer.data()) T(std::forward<ArgT>(arguments)...);
			}
			if constexpr (op_table_size > 0)
			{
				new (m_op_table.data()) detail::OperationsTable<BaseT, T, properties>();
			}
		}

		template <typename T> 
		void set(T&& value) requires IsBaseOf<BaseT, std::decay_t<T>>
		{
			using BareT = std::decay_t<T>;
			if constexpr (sizeof(T) > properties::small_buffer_size)
			{
				m_local = false;
				BareT* created = new BareT(std::forward<T>(value));
				std::memcpy(m_buffer.data(), &created, sizeof(BareT*));
			}
			else
			{
				m_local = true;
				new (m_buffer.data()) BareT(std::forward<T>(value));
			}
			if constexpr (op_table_size > 0)
			{
				new (m_op_table.data()) detail::OperationsTable<BaseT, BareT, properties>();
			}
		}

		template <typename OtherBaseT, typename OtherPropertiesT> requires properties::copyable && IsBaseOf<BaseT, OtherBaseT>
		void do_clone(const dynamic_value<OtherBaseT, OtherPropertiesT>& other)
		{
			const auto& optable = other.get_op_table();
			optable.clone(std::span<uint8_t>(m_buffer), std::span<uint8_t>(m_op_table), m_local, &other.get());
		}

		template <typename OtherBaseT, typename OtherPropertiesT> requires properties::moveable && IsBaseOf<BaseT, OtherBaseT>
		void do_move(dynamic_value<OtherBaseT, OtherPropertiesT>&& other)
		{
			const auto& optable = other.get_op_table();
			optable.move(std::span<uint8_t>(m_buffer), std::span<uint8_t>(m_op_table), m_local, &other.get(), other.m_local);
		}

		const IOperationsTable& get_op_table() const
		{
			return *reinterpret_cast<const IOperationsTable*>(m_op_table.data());
		}

		void destroy()
		{
			if (m_local)
			{
				reinterpret_cast<BaseT*>(m_buffer.data())->~BaseT();
			}
			else
			{
				BaseT* val = *reinterpret_cast<BaseT**>(m_buffer.data());
				delete val;
			}
		}

		alignas(BaseT) std::array<std::uint8_t, properties::small_buffer_size> m_buffer;
		alignas(IOperationsTable) std::array<std::uint8_t, op_table_size> m_op_table;
		bool m_local;
	};

	//!
	//! Creates a dynamic value, emplacing it's initial value so that no move or copy is needed
	//! @tparam	BaseT	the base type of the returned dynamic value
	//! @tparam ActualT	the type to constuct in the dynamic value, defaults to BaseT and must either be BaseT or a child of it
	//! @tparam SmallObjectSize	values of types equal or smaller than this value will be stored inside the dynamic value rather than be heap allocated outside of it
	//! @tparam ArgT	the types of the arguemnts sent to ActualTs constructor
	//! @param arguments	the arguemtns to construct ArgT with, will be perfectly forwarded
	//! 
	template<typename BaseT, typename ActualT = BaseT, typename PropertiesT = Properties<>, typename... ArgT>
	dynamic_value<BaseT, PropertiesT> make_dynamic_value(ArgT&&... arguments) requires IsBaseOf<BaseT, ActualT>&& requires (ArgT&&... args) { ActualT(std::forward<ArgT>(args)...); }
	{
		return dynamic_value<BaseT, PropertiesT>::template make_emplaced<ActualT>(std::forward<ArgT>(arguments)...);
	}

	// Deduction Guides
	template <typename T>
	dynamic_value(const T& other)->dynamic_value<std::decay_t<T>>;

	template <typename T>
	dynamic_value(T&& other)->dynamic_value<std::decay_t<T>>;
}