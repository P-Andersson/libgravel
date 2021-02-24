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
   dynamic_value<MyBase> value = gravel::make_dynamic_value<MyBase, MyChild>(4);
   std::cout << value->get_my_id();
}
```

Output: 1004
