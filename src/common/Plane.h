#pragma once

#include <cstdint>
#include <limits>

#include "common/Vec2.h"

constexpr size_t DynamicExtent = std::numeric_limits<size_t>::max();

template<size_t Extent>
struct extentStorage
{
	constexpr extentStorage(size_t)
	{}

	constexpr size_t getExtent() const
	{
		return Extent;
	}

	constexpr void setExtent(size_t)
	{}
};

template<>
struct extentStorage<DynamicExtent>
{
	size_t extent;

	constexpr extentStorage(size_t extent):
		extent(extent)
	{}

	constexpr size_t getExtent() const
	{
		return extent;
	}

	constexpr void setExtent(size_t extent)
	{
		this->extent = extent;
	}
};

template<size_t Extent>
struct xExtent: extentStorage<Extent>
{
	using extentStorage<Extent>::extentStorage;
};

template<size_t Extent>
struct yExtent: extentStorage<Extent>
{
	using extentStorage<Extent>::extentStorage;
};

// A class that contains some container T and lets you index into it as if it
// were a 2D array of size Width x Height, in row-major order.
template<typename T, size_t Width = DynamicExtent, size_t Height = DynamicExtent>
class PlaneAdapter: xExtent<Width>, yExtent<Height>
{
	using value_type = std::remove_reference_t<decltype(std::declval<T>()[0])>;
	using iterator = decltype(std::begin(std::declval<T &>()));
	using const_iterator = decltype(std::begin(std::declval<T const &>()));

	size_t getWidth() const
	{
		return xExtent<Width>::getExtent();
	}

	size_t getHeight() const
	{
		return yExtent<Height>::getExtent();
	}

public:
	T Base;

	PlaneAdapter():
		xExtent<Width>(0),
		yExtent<Height>(0),
		Base()
	{}

	PlaneAdapter(PlaneAdapter const &) = default;

	PlaneAdapter(PlaneAdapter &&) = default;

	// PlaneAdapter<T> can be constructed from (Vec2, T &&)
	// PlaneAdapter<T &> can be constructed from (Vec2, T &)
	PlaneAdapter(Vec2<int> size, T &&base):
		xExtent<Width>(size.X),
		yExtent<Height>(size.Y),
		Base(std::forward<T>(base))
	{}

	template<typename... Args>
	PlaneAdapter(Vec2<int> size, Args&&... args):
		xExtent<Width>(size.X),
		yExtent<Height>(size.Y),
		Base(getWidth() * getHeight(), std::forward<Args>(args)...)
	{}

	PlaneAdapter &operator=(PlaneAdapter const &) = default;

	PlaneAdapter &operator=(PlaneAdapter &&) = default;

	Vec2<int> Size() const
	{
		return Vec2<int>(getWidth(), getHeight());
	}

	void SetSize(Vec2<int> size)
	{
		xExtent<Width>::setExtent(size.X);
		yExtent<Height>::setExtent(size.Y);
	}

	iterator RowIterator(Vec2<int> p)
	{
		return std::begin(Base) + (p.X + p.Y * getWidth());
	}

	const_iterator RowIterator(Vec2<int> p) const
	{
		return std::begin(Base) + (p.X + p.Y * getWidth());
	}

	value_type *data()
	{
		return std::data(Base);
	}

	value_type const *data() const
	{
		return std::data(Base);
	}

	value_type &operator[](Vec2<int> p)
	{
		return Base[p.X + p.Y * getWidth()];
	}

	value_type const &operator[](Vec2<int> p) const
	{
		return Base[p.X + p.Y * getWidth()];
	}
};
