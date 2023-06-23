#pragma once

#include <memory>

template<typename T>
using ref = std::shared_ptr<T>;

template<typename T>
using scope = std::unique_ptr<T>;

/**
 * A ptr type that nulls itself on move.
 * This is meant to be used on 1 of the member pointers of a movable class so that we can determine
 * if their destructor is being called on a moved instance so that we can return out of it.
 *
 * This also disallows the use of [], +, - operators
 */
template<typename T>
class tidy_ptr
{
public:
	tidy_ptr() = default;

	tidy_ptr(T *ptr): ptr(ptr)
	{
	}

	tidy_ptr(tidy_ptr &&other)
	{
		*this = std::move(other);
	}

	tidy_ptr &operator=(tidy_ptr &&other)
	{
		this->ptr = other.ptr;
		other.ptr = nullptr;

		return *this;
	}

	tidy_ptr(tidy_ptr const &other) = default;

	tidy_ptr &operator=(tidy_ptr const &other) = default;

	operator T *()
	{
		return ptr;
	}

	T *operator->()
	{
		return ptr;
	}

	T *operator->() const
	{
		return ptr;
	}

	operator bool() const
	{
		return !!ptr;
	}

	bool operator==(tidy_ptr &&other) const
	{
		return this->ptr == other.ptr;
	}

	bool operator!=(tidy_ptr &&other) const
	{
		return !(this == other);
	}

private:
	T *ptr = {};
};
