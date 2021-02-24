#include "gravel/unique_function.hpp"

#include <future>
#include <iostream>

namespace {
	class FunctionObjectWithPromise
	{
	public:
		std::future<int> get_future()
		{
			return m_promise.get_future();
		}

		void operator()(int value)
		{
			m_promise.set_value(value * 3);
		}

	private:
		std::promise<int> m_promise;
	};
}

int main(int argc, char** argv)
{
	FunctionObjectWithPromise fobject;

	auto future = fobject.get_future();
   gravel::unique_function<void(int)> move_from(std::move(fobject));

	gravel::unique_function<void(int)> move_to(std::move(move_from));

	move_to(6);

   std::cout << future.get();
}