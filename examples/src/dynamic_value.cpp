#include <gravel/dynamic_value.hpp>

#include <iostream>

class MyBase
{
public:
   virtual ~MyBase() = default;
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
   gravel::dynamic_value<MyBase> value(MyChild(4));
   std::cout << value->get_my_id();
}
