## LIBGRAVEL

ligravel is a library of basic c++ data-structures and functionality. It is not intended to replace 
the STL, or boost, merely provide a few general datas-tructures and 
highly general functions to make certain c++ constructs easier to work with.
It's not always going to be header only, but currently is.

### Included Features
The following features are currently a part of libgravel.

#### Dynamic Value

Provides runtime polymorphic value types, that uses small buffer optimization to reduce the number
of allocations. 

Usage example, when created using the make_dynamic_value function:

```
#include <gravel/dynamic_value.hpp>

#include <iostream>

class MyBase
{
public:
   virtual int get_my_id() const = 0;
};

class MyChild : public MyBase
{
public:
   MyChild(int id)
      : m_id(1000 + id)
   {
   }

   int get_my_id() const override
   {
      return m_id;
   }
private:
   int m_id;
};

int main(int argc, char** argv)
{
   gravel::dynamic_value<MyBase> value = gravel::make_dynamic_value<MyBase, MyChild>(4);
   std::cout << value->get_my_id();
}
```

Output: 1004

#### Unique Function

Move-only variant of an std::function. Uses small-buffer optimization to 
minimize needless heap allocations. Primarily intended to enable the user
to put a move only function object into it, such as a function object containing
a promise.

Usage example:

```
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
```

Output: 18
