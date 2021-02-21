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

Usage example:

> #inlude <gravel/dynamic_value.hpp>
>
....