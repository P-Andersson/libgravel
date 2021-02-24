#pragma once

#include <type_traits>

#include "gravel/dynamic_value.hpp"

namespace gravel
{

	//!
	//! Move-only std::function variant. It primarily serves the purpose of being able to hold move-only
	//! function objects, such as those containing an std::promise.
	//! 
	//! Notes:
	//! * Will never be created empty, if you need it to be conditional wrap it in an std::optional
	//! * After a move, it's not safe to use but it is safe to assign to. 
	//! 
	//! @tparam	Signature	the function signature of the unique_function (in a formation like: "bool(int, float)")
	//! 
	template <typename Signature>
	class unique_function;

	template <typename RetT, typename... ArgT>
	class unique_function<RetT(ArgT...)>
	{
	public:
		//!
		//! Constructor
		//! @tparam	FuncT	the type of the function to put in the unique_function 
		//! @param	function the the function, function object or lambda to put in the unique functionb
		//! 
		template <typename FuncT>
		explicit unique_function(FuncT&& function)
			: m_function(FunctionWrapper<std::decay_t<FuncT>>(std::forward<FuncT>(function)))
		{
		}
		
		unique_function(const unique_function<RetT(ArgT...)>& other) = delete;

		unique_function(unique_function<RetT(ArgT...)>&& other)
			: m_function(std::move(other.m_function))
		{
		}

		unique_function& operator=(const unique_function<RetT(ArgT...)>& other) = delete;

		unique_function& operator=(unique_function<RetT(ArgT...)>&& other)
		{
			m_function = std::move(other.m_function);
			return *this;
		}

		//!
		//! Move-assignment, putting a new function in this unique_function
		//! @tparam	FuncT	the type of the function to put in the unique_function 
		//! @param	function the the function, function object or lambda to put in the unique functionb
		//! @return	a reference to this	
		//! 
		template <typename FuncT>
		unique_function& operator=(FuncT&& function)
		{
			m_function.emplace<FunctionWrapper< std::decay_t<FuncT> >>(std::forward<FuncT>(function));
			return *this;
		}

		//!
		//! Invokes the contained function
		//! @params arg	the aguments to invoke the function with, must match the signature
		//! @return	the return value of the contained function
		//! 
		RetT operator()(ArgT... arg)
		{
			return (*m_function)(std::forward<ArgT>(arg)...);
		}
	private:
		class FunctionBase
		{
		public:
			FunctionBase() = default;
			FunctionBase(const FunctionBase& other) = delete;
			virtual ~FunctionBase() = default;
			virtual RetT operator()(ArgT... arg) = 0;
		};
		
		template <typename FuncT>
		class FunctionWrapper : public FunctionBase
		{
		public:
			FunctionWrapper(FuncT&& f)
				: m_function(std::move(f))
			{

			}
			FunctionWrapper(const FunctionWrapper& other) = delete;
			FunctionWrapper(FunctionWrapper&& f)
				: m_function(std::move(f.m_function))
			{

			}

			RetT operator()(ArgT... arg) override
			{
				return m_function(std::forward<ArgT>(arg)...);
			}
		private:
			FuncT m_function;
		};
		
		dynamic_value<FunctionBase, Properties<Attr::Movable>> m_function;
	};
}