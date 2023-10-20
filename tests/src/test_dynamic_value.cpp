#include "catch2/catch_test_macros.hpp"

#include "gravel/dynamic_value.hpp"

using namespace gravel;

namespace {
	enum class CreationMethod
	{
		Basic,
		Moved,
		Copied
	};

	class Baseless
	{
	public:
		Baseless(int val)
		: m_creation(CreationMethod::Basic)
		, m_val(val)
		{

		}

		Baseless(const Baseless& other)
			: m_creation(CreationMethod::Copied)
			, m_val(other.m_val)
		{

		}

		Baseless(Baseless&& other) noexcept
			: m_creation(CreationMethod::Moved)
			, m_val(other.m_val)
		{

		}

		Baseless& operator=(const Baseless&) = delete;
		Baseless& operator=(Baseless&&) = delete;

		CreationMethod m_creation;
		int m_val;

	};

	class Base
	{
	public:
		Base(int val)
			: m_creation(CreationMethod::Basic)
			, m_val(val)
		{

		}

		Base(const Base& other)
			: m_creation(CreationMethod::Copied)
			, m_val(other.m_val)
		{

		}

		Base(Base&& other) noexcept
			: m_creation(CreationMethod::Moved)
			, m_val(other.m_val)
		{

		}

		virtual ~Base() = default;

		virtual int get_type_number() const
		{
			return 1;
		}

		Base& operator=(const Base&) = delete;
		Base& operator=(Base&&) = delete;

		CreationMethod m_creation;
		int m_val;

	};

	class ChildA : public Base
	{
	public:
		ChildA(int val)
			: Base(val)
		{

		}

		ChildA(const ChildA& other)
			: Base(other)
		{

		}

		ChildA(ChildA&& other) noexcept
			: Base(std::move(other))
		{

		}

		int get_type_number() const override
		{
			return 2;
		}

		ChildA& operator=(const ChildA&) = delete;
		ChildA& operator=(ChildA&&) = delete;
	};

	class ChildB : public Base
	{
	public:
		ChildB(int val)
			: Base(val)
		{

		}

		ChildB(const ChildB& other)
			: Base(other)
		{

		}

		ChildB(ChildB&& other) noexcept
			: Base(std::move(other))
		{

		}

		int get_type_number() const override
		{
			return 3;
		}

		ChildB& operator=(const ChildB&) = delete;
		ChildB& operator=(ChildB&&) = delete;
	};

	class CopyOnly
	{
	public:
		CopyOnly(int val = 0)
			: m_val(val)
		{

		}

		CopyOnly(const CopyOnly& other)
			: m_val(other.m_val)
		{

		}
		CopyOnly(CopyOnly&&) = delete;

		int m_val;
	};

	class MoveOnly
	{
	public:
		MoveOnly(int val = 0)
			: m_val(val)
		{

		}

		MoveOnly(const MoveOnly&) = delete;
		
		MoveOnly(MoveOnly&& other) noexcept
			: m_val(other.m_val)
		{
		}

		int m_val;
	};

	template <std::size_t BufferSize>
	class FlexibleSizeBase
	{
	public:
		FlexibleSizeBase(int* destructor_counter, int val = 0)
			: m_destructor_counter(destructor_counter)
			, m_val(val)
		{
			m_buffer.fill(0);
		}

		virtual ~FlexibleSizeBase()
		{
			if (m_destructor_counter)
			{
				*m_destructor_counter += 1;
			}
		}

		virtual int get_and_multiply(int val) const
		{
			return val;
		}

		int* m_destructor_counter;
		int m_val;
		std::array<std::uint8_t, BufferSize - sizeof(m_destructor_counter) - sizeof(m_val)> m_buffer;
	};

	template <std::size_t BufferSize, std::size_t ExtraSize>
	class FlexibleSizeChild : public FlexibleSizeBase<BufferSize>
	{
	public:
		FlexibleSizeChild(int* destructor_counter)
			: FlexibleSizeBase<BufferSize>(destructor_counter)
			, m_multiplier(2)
		{
			m_extra_buffer.fill(0);
		}

		virtual int get_and_multiply(int val) const
		{
			return val * m_multiplier;
		}

		int m_multiplier;
		std::array<std::uint8_t, ExtraSize - sizeof(m_multiplier)> m_extra_buffer;
	};

	class AbstractBase
	{
	public:
		virtual int get_value() = 0;
	};

	class ConcreteChild : public AbstractBase
	{
	public:
		ConcreteChild(int value)
			: m_value(value)
		{

		}

		int get_value() override
		{
			return m_value;
		}
	private:
		int m_value;
	};

	template <typename T>
	bool is_inside(T* object, void* ptr)
	{
		return (ptr >= static_cast<void*>(object) && ptr < (reinterpret_cast<std::uint8_t*>(object) + sizeof(T)));
	}

	template<typename LeftT, typename RightT>
	concept ConceptCopyAssignable = requires (LeftT & l, const RightT & r) { l = r; };
	template<typename LeftT, typename RightT>
	concept ConceptMoveAssignable = requires (LeftT & l, RightT&& r) { l = std::move(r); };
}

TEST_CASE("Basic Tests") 
{
	dynamic_value<Baseless> dyn_val(Baseless(7));

	SECTION("Untouched Dereferencing")
	{
		REQUIRE(dyn_val->m_creation == CreationMethod::Moved);
		REQUIRE(dyn_val->m_val == 7);
		REQUIRE(dyn_val.get().m_val == 7);
		REQUIRE((*dyn_val).m_val == 7);

		REQUIRE_NOTHROW(dyn_val->m_val = 12);

		REQUIRE(dyn_val->m_val == 12);
		REQUIRE(dyn_val.get().m_val == 12);
		REQUIRE((*dyn_val).m_val == 12);
	}
	SECTION("Copy Value Into")
	{
		Baseless copyee(21);
		dyn_val = copyee;
		REQUIRE(dyn_val->m_creation == CreationMethod::Copied);
		REQUIRE(dyn_val->m_val == 21);
	}
	SECTION("Move Value Into")
	{
		dyn_val = Baseless(11);
		REQUIRE(dyn_val->m_creation == CreationMethod::Moved);
		REQUIRE(dyn_val->m_val == 11);
	}
}

TEST_CASE("Build by copy")
{
	Baseless baseless(7);
	dynamic_value<Baseless> dyn_val(baseless);
	REQUIRE(dyn_val->m_creation == CreationMethod::Copied);
	REQUIRE(dyn_val->m_val == 7);
}

TEST_CASE("Dynamic Binding Basics Tests")
{
	SECTION("Initialized with Base Class")
	{
		dynamic_value<Base> dyn_val(Base(4));

		SECTION("Untouched")
		{
			REQUIRE(dyn_val->m_creation == CreationMethod::Moved);
			REQUIRE(dyn_val->m_val == 4);
			REQUIRE(dyn_val->get_type_number() == 1);
			REQUIRE(typeid(*dyn_val) == typeid(Base));

		}
		SECTION("Copy Value Into")
		{
			ChildA copyee(21);
			dyn_val = copyee;
			REQUIRE(dyn_val->m_creation == CreationMethod::Copied);
			REQUIRE(dyn_val->m_val == 21);
			REQUIRE(dyn_val->get_type_number() == 2);
			REQUIRE(typeid(*dyn_val) == typeid(ChildA));
		}
		SECTION("Move Value Into")
		{
			dyn_val = ChildB(12);
			REQUIRE(dyn_val->m_creation == CreationMethod::Moved);
			REQUIRE(dyn_val->m_val == 12);
			REQUIRE(dyn_val->get_type_number() == 3);
			REQUIRE(typeid(*dyn_val) == typeid(ChildB));
		}
	}
	SECTION("Initialized with Child Class")
	{
		dynamic_value<Base> dyn_val(ChildB(44));

		SECTION("Untouched")
		{
			REQUIRE(dyn_val->m_creation == CreationMethod::Moved);
			REQUIRE(dyn_val->m_val == 44);
			REQUIRE(dyn_val->get_type_number() == 3);
		}
		SECTION("Copy Value Into")
		{
			Base copyee(21);
			dyn_val = copyee;
			REQUIRE(dyn_val->m_creation == CreationMethod::Copied);
			REQUIRE(dyn_val->m_val == 21);
			REQUIRE(dyn_val->get_type_number() == 1);
		}
		SECTION("Move Value Into")
		{
			dyn_val = ChildA(12);
			REQUIRE(dyn_val->m_creation == CreationMethod::Moved);
			REQUIRE(dyn_val->m_val == 12);
			REQUIRE(dyn_val->get_type_number() == 2);
		}
	}
}

TEST_CASE("Build child by copy")
{
	ChildA val(12);
	dynamic_value<Base> dyn_val(val);
	REQUIRE(dyn_val->m_creation == CreationMethod::Copied);
	REQUIRE(dyn_val->m_val == 12);
	REQUIRE(dyn_val->get_type_number() == 2);
}


TEST_CASE("Assignment of Flexible Dyn Vals")
{
	dynamic_value<Base> dyn_val(Base(2));
	SECTION("Copy Assignment")
	{
		SECTION("Base")
		{
			dynamic_value<Base> other(Base(12));
			dyn_val = other;
			REQUIRE(dyn_val->m_val == 12);
			REQUIRE(dyn_val->get_type_number() == 1);
			REQUIRE(dyn_val->m_creation == CreationMethod::Copied);
		}
		SECTION("ChildA")
		{
			dynamic_value<ChildA> other(ChildA(22));
			dyn_val = other;
			REQUIRE(dyn_val->m_val == 22);
			REQUIRE(dyn_val->get_type_number() == 2);
			REQUIRE(dyn_val->m_creation == CreationMethod::Copied);
		}
	}
	SECTION("Move Assignment")
	{
		SECTION("Base")
		{
			dyn_val = dynamic_value<Base>(Base(12));
			REQUIRE(dyn_val->m_val == 12);
			REQUIRE(dyn_val->get_type_number() == 1);
			REQUIRE(dyn_val->m_creation == CreationMethod::Moved);
		}
		SECTION("ChildA")
		{
			dyn_val = dynamic_value<ChildA>(ChildA(42));
			REQUIRE(dyn_val->m_val == 42);
			REQUIRE(dyn_val->get_type_number() == 2);
			REQUIRE(dyn_val->m_creation == CreationMethod::Moved);
		}
	}
}

TEST_CASE("Assignment of Copy Only Dyn Vals")
{
	dynamic_value<CopyOnly> dyn_val(CopyOnly(2));
	SECTION("Copy Assignment")
	{
		dynamic_value<CopyOnly> other(CopyOnly(3));
		dyn_val = other;
		REQUIRE(dyn_val->m_val == 3);
	}
	SECTION("Move Assignment")
	{
		dyn_val = dynamic_value<CopyOnly>(CopyOnly(6));
		REQUIRE(dyn_val->m_val == 6);
	}
}

TEST_CASE("Properties")
{
	SECTION("Moveable and Copyable")
	{
		REQUIRE(ConceptCopyAssignable<dynamic_value<Base>, Base>);
		REQUIRE(ConceptMoveAssignable<dynamic_value<Base>, Base>);
		REQUIRE(ConceptCopyAssignable<dynamic_value<Base>, dynamic_value<Base>>);
		REQUIRE(ConceptMoveAssignable<dynamic_value<Base>, dynamic_value<Base>>);
	}
	SECTION("Copy Only")
	{
		REQUIRE(ConceptCopyAssignable<dynamic_value<CopyOnly>, CopyOnly>);
		REQUIRE(ConceptMoveAssignable<dynamic_value<CopyOnly>, CopyOnly>);
		REQUIRE(ConceptCopyAssignable<dynamic_value<CopyOnly>, dynamic_value<CopyOnly>>);
		REQUIRE(ConceptMoveAssignable<dynamic_value<CopyOnly>, dynamic_value<CopyOnly>>);
	}
	SECTION("Move Only")
	{
		REQUIRE(!ConceptCopyAssignable<dynamic_value<MoveOnly>, MoveOnly>);
		REQUIRE(ConceptMoveAssignable<dynamic_value<MoveOnly>, MoveOnly>);
		REQUIRE(!ConceptCopyAssignable<dynamic_value<MoveOnly>, dynamic_value<MoveOnly>>);
		REQUIRE(ConceptMoveAssignable<dynamic_value<MoveOnly>, dynamic_value<MoveOnly>>);
	}
}

TEST_CASE("Make dynamic value")
{
	SECTION("Non-polymorphic")
	{
		dynamic_value<Baseless> target = make_dynamic_value<Baseless>(4);
		REQUIRE(target->m_val == 4);
		REQUIRE(target->m_creation == CreationMethod::Basic);
	}
	SECTION("Polymorphic Base")
	{
		dynamic_value<Base> target = make_dynamic_value<Base>(5);
		REQUIRE(target->get_type_number() == 1);
		REQUIRE(target->m_val == 5);
		REQUIRE(target->m_creation == CreationMethod::Basic);
	}

	SECTION("Polymorphic Child")
	{
		dynamic_value<Base> target = make_dynamic_value<Base, ChildB>(51);
		REQUIRE(target->get_type_number() == 3);
		REQUIRE(target->m_val == 51);
		REQUIRE(target->m_creation == CreationMethod::Basic);
	}
}

TEST_CASE("Emplace")
{
	SECTION("Non-polymorphic")
	{
		dynamic_value<Baseless> target(Baseless(0));
		target.emplace<Baseless>(9);
		REQUIRE(target->m_val == 9);
		REQUIRE(target->m_creation == CreationMethod::Basic);
	}
	SECTION("Polymorphic Base")
	{
		dynamic_value<Base> target(Base(0));
		target.emplace<Base>(71);
		REQUIRE(target->get_type_number() == 1);
		REQUIRE(target->m_val == 71);
		REQUIRE(target->m_creation == CreationMethod::Basic);
	}

	SECTION("Polymorphic Child")
	{
		dynamic_value<Base> target(Base(0));
		target.emplace<ChildA>(91);
		REQUIRE(target->get_type_number() == 2);
		REQUIRE(target->m_val == 91);
		REQUIRE(target->m_creation == CreationMethod::Basic);
	}
}

TEST_CASE("Local and Heap")
{
	SECTION("Basic Local")
	{
		int dcounter = 0;
		SECTION("Emplaced")
		{
			{
				auto value = make_dynamic_value<FlexibleSizeBase<16>>(&dcounter);
				REQUIRE(is_inside(&value, &value.get()));
			}
			REQUIRE(dcounter == 1);
		}
		SECTION("Copied")
		{
			{
				FlexibleSizeBase<16> orig(&dcounter);
				auto value = dynamic_value(orig);
				REQUIRE(is_inside(&value, &value.get()));
			}
			REQUIRE(dcounter == 2);
		}
		SECTION("Moved")
		{
			{
				auto value = dynamic_value(FlexibleSizeBase<16>(&dcounter));
				REQUIRE(is_inside(&value, &value.get()));
			}
			REQUIRE(dcounter == 2);
		}
	}
	SECTION("Basic Non-Local")
	{
		int dcounter = 0;
		SECTION("Emplaced")
		{
			{
				auto value = make_dynamic_value<FlexibleSizeBase<64>, FlexibleSizeBase<64>, BufferSize<32>>(&dcounter);
				REQUIRE(!is_inside(&value, &value.get()));
			}
			REQUIRE(dcounter == 1);
		}
		SECTION("Copied")
		{
			{
				FlexibleSizeBase<64> orig(&dcounter);
				auto value = dynamic_value<FlexibleSizeBase<64>, BufferSize<32>>(orig);
				REQUIRE(!is_inside(&value, &value.get()));
			}
			REQUIRE(dcounter == 2);
		}
		SECTION("Moved")
		{
			{
				auto value = dynamic_value<FlexibleSizeBase<64>, BufferSize<32>>(FlexibleSizeBase<64>(&dcounter));
				REQUIRE(!is_inside(&value, &value.get()));
			}
			REQUIRE(dcounter == 2);
		}
	}
	SECTION("Basic Access Non-Local")
	{
		SECTION("Emplaced")
		{
			auto value = make_dynamic_value<FlexibleSizeBase<64>, FlexibleSizeBase<64>, BufferSize<32>>(nullptr, 4);
			REQUIRE(!is_inside(&value, &value.get()));

			//REQUIRE(dyn_val->m_creation == CreationMethod::Moved);
			REQUIRE(value->m_val == 4);
			REQUIRE(typeid(*value) == typeid(FlexibleSizeBase<64>));

		}
		SECTION("Copied")
		{
			FlexibleSizeBase<64> orig(nullptr, 9);
			auto value = dynamic_value<FlexibleSizeBase<64>, BufferSize<32>>(orig);
			REQUIRE(!is_inside(&value, &value.get()));

			//REQUIRE(dyn_val->m_creation == CreationMethod::Moved);
			REQUIRE(value->m_val == 9);
			REQUIRE(typeid(*value) == typeid(FlexibleSizeBase<64>));

		}
		SECTION("Moved")
		{
			auto value = dynamic_value<FlexibleSizeBase<64>, BufferSize<32>>(FlexibleSizeBase<64>(nullptr, 18));
			REQUIRE(!is_inside(&value, &value.get()));
			//REQUIRE(dyn_val->m_creation == CreationMethod::Moved);
			REQUIRE(value->m_val == 18);
			REQUIRE(typeid(*value) == typeid(FlexibleSizeBase<64>));

		}
	}

	SECTION("Non-Local Child Type")
	{

		int dcounter = 0;
		SECTION("Emplaced")
		{
			{
				auto value = make_dynamic_value<FlexibleSizeBase<32>, FlexibleSizeChild<32, 32>, BufferSize<32>>(&dcounter);
				REQUIRE(!is_inside(&value, &value.get()));

				//REQUIRE(dyn_val->m_creation == CreationMethod::Moved);
				REQUIRE(value->m_val == 0);
				REQUIRE(value->get_and_multiply(3) == 6);
				REQUIRE(typeid(*value) == typeid(FlexibleSizeChild<32, 32>));
			}
			REQUIRE(dcounter == 1);
		}
		SECTION("Copied")
		{
			{
				FlexibleSizeChild<24, 32> orig(&dcounter);
				auto value = dynamic_value<FlexibleSizeBase<24>, BufferSize<32>>(orig);
				REQUIRE(!is_inside(&value, &value.get()));

				//REQUIRE(dyn_val->m_creation == CreationMethod::Moved);
				REQUIRE(value->m_val == 0);
				REQUIRE(value->get_and_multiply(28) == 56);
				REQUIRE(typeid(*value) == typeid(FlexibleSizeChild<24, 32>));
			}
			REQUIRE(dcounter == 2);
		}
		SECTION("Moved")
		{
			{
				auto value = dynamic_value<FlexibleSizeBase<24>, BufferSize<32>>(FlexibleSizeChild<24, 32>(&dcounter));
				REQUIRE(!is_inside(&value, &value.get()));

				//REQUIRE(dyn_val->m_creation == CreationMethod::Moved);
				REQUIRE(value->m_val == 0);
				REQUIRE(value->get_and_multiply(28) == 56);
				REQUIRE(typeid(*value) == typeid(FlexibleSizeChild<24, 32>));
			}
			REQUIRE(dcounter == 2);
		}
	}

	SECTION("Non-Local/Local Mixed Assignments")
	{
		SECTION("Emplaced Local")
		{
			auto value = make_dynamic_value<FlexibleSizeBase<32>, FlexibleSizeChild<32, 33>, BufferSize<64> >(nullptr);
			REQUIRE(!is_inside(&value, &value.get()));
			value.emplace<FlexibleSizeChild<32, 16>>(nullptr);
			REQUIRE(is_inside(&value, &value.get()));
		}
		SECTION("Emplaced Non-Local")
		{
			auto value = make_dynamic_value<FlexibleSizeBase<32>, FlexibleSizeChild<32, 16>, BufferSize<64>>(nullptr);
			REQUIRE(is_inside(&value, &value.get()));
			value.emplace<FlexibleSizeChild<32, 33>>(nullptr);
			REQUIRE(!is_inside(&value, &value.get()));
		}
		SECTION("Copy Local")
		{
			auto value = make_dynamic_value<FlexibleSizeBase<32>, FlexibleSizeChild<32, 33>, BufferSize<64>>(nullptr);
			REQUIRE(!is_inside(&value, &value.get()));
			FlexibleSizeChild<32, 16> cpy(nullptr);
			value = cpy;
			REQUIRE(is_inside(&value, &value.get()));
		}
		SECTION("Copy Non-Local")
		{
			auto value = make_dynamic_value<FlexibleSizeBase<32>, FlexibleSizeChild<32, 16>, BufferSize<64>>(nullptr);
			REQUIRE(is_inside(&value, &value.get()));
			FlexibleSizeChild<32, 33> cpy(nullptr);
			value = cpy;
			REQUIRE(!is_inside(&value, &value.get()));
		}
		SECTION("Move Local")
		{
			auto value = make_dynamic_value<FlexibleSizeBase<32>, FlexibleSizeChild<32, 33>, BufferSize<64>>(nullptr);
			REQUIRE(!is_inside(&value, &value.get()));
			value = FlexibleSizeChild<32, 16>(nullptr);
			REQUIRE(is_inside(&value, &value.get()));
		}
		SECTION("Move Non-Local")
		{
			auto value = make_dynamic_value<FlexibleSizeBase<32>, FlexibleSizeChild<32, 16>, BufferSize<64>>(nullptr);
			REQUIRE(is_inside(&value, &value.get()));
			value = FlexibleSizeChild<32, 33>(nullptr);
			REQUIRE(!is_inside(&value, &value.get()));
		}
	}
}

TEST_CASE("Abstract Base")
{
	SECTION("Make by emplace")
	{
		dynamic_value<AbstractBase> value = make_dynamic_value<AbstractBase, ConcreteChild>(12);

		REQUIRE(value->get_value() == 12);
	}
	SECTION("Make by copy")
	{
		ConcreteChild child(51);
		dynamic_value<AbstractBase> value(child);

		REQUIRE(value->get_value() == 51);
	}
	SECTION("Make by move")
	{
		dynamic_value<AbstractBase> value(ConcreteChild(51));

		REQUIRE(value->get_value() == 51);
	}

	SECTION("Assignment")
	{
		dynamic_value<AbstractBase> value = make_dynamic_value<AbstractBase, ConcreteChild>(1);
		SECTION("Emplace")
		{
			value.emplace<ConcreteChild>(13);
			REQUIRE(value->get_value() == 13);
		}
		SECTION("Copy")
		{
			ConcreteChild child(51);
			value = child;

			REQUIRE(value->get_value() == 51);
		}
		SECTION("Move")
		{
			value = ConcreteChild(51);

			REQUIRE(value->get_value() == 51);
		}
	}

}
