#include "catch2/catch.hpp"

#include <future>

#include "gravel/unique_function.hpp"

using namespace gravel;

namespace
{
	int x2(int val)
	{
		return val * 2;
	}

	class FunctorWithPromise
	{
	public:
		std::future<std::string> get_future()
		{
			return m_promise.get_future();
		}

		void operator()(std::string&& str)
		{
			m_promise.set_value(std::move(str));
		}

	private:
		std::promise<std::string> m_promise;
	};

	class BasicFunctionObject
	{
	public:
		BasicFunctionObject(int myval)
			: m_myval(myval)
		{

		}

		int operator()(int other) const
		{
			return m_myval + other;
		}

	private:
		int m_myval;
	};
}

TEST_CASE("free functions")
{
	SECTION("construct and call")
	{
		unique_function<int(int)> f(x2);

		REQUIRE(f(8) == 16);
	}
	SECTION("construct, move and call")
	{
		unique_function<int(int)> f(x2);
		unique_function<int(int)> f2 = std::move(f);

		REQUIRE(f2(7) == 14);
	}
}

TEST_CASE("functors")
{
	SECTION("construct and call")
	{
		FunctorWithPromise functor;
		auto future = functor.get_future();
		unique_function<void(std::string&&)> f(std::move(functor));

		f(std::string("Fooo"));
		REQUIRE(future.get() == "Fooo");
		
	}
	SECTION("construct and call")
	{
		FunctorWithPromise functor;
		auto future = functor.get_future();
		unique_function<void(std::string&&)> f(std::move(functor));
		unique_function<void(std::string&&)> f2 = std::move(f);

		f2(std::string("Fooo"));
		REQUIRE(future.get() == "Fooo");
	}
}

TEST_CASE("lambdas")
{
	SECTION("construct and call")
	{
		unique_function<bool(int)> f([](const int& val) { return val > 7; });

		REQUIRE(f(6) == false);
	}
	SECTION("construct, move and call")
	{
		int limit = 20;
		unique_function<bool(int)> f([&limit](const int& val) { return val > limit; });
		unique_function<bool(int)> f2 = std::move(f);
		limit = 11;

		REQUIRE(f2(12) == true);
	}
}

TEST_CASE("primitive assignments")
{
	SECTION("free")
	{
		unique_function<int(int)> f([](const int& val) { return 1; });
		f = x2;

		REQUIRE(f(7) == 14);
	}
	SECTION("functor")
	{
		unique_function<int(int)> f([](const int& val) { return 1; });
		f = BasicFunctionObject(6);

		REQUIRE(f(3) == 9);
	}
	SECTION("lambda")
	{
		unique_function<int(int)> f(x2);
		f = [](int v) {return 2 + v; };

		REQUIRE(f(13) == 15);
	}
}
