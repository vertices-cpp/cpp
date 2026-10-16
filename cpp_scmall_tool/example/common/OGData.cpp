#include "OGData.h"

OG_BEGIN
 
//-------------------------------------Data--------------------------
const Data Data::Null;

Data::Data() :
	_bytes(nullptr),
	_size(0)
{
	OGLOGINFO("In the empty constructor of Data.");
}

Data::Data(Data&& other) :
	_bytes(nullptr),
	_size(0)
{
	OGLOGINFO("In the move constructor of Data.");
	move(other);
}

Data::Data(const Data& other) :
	_bytes(nullptr),
	_size(0)
{
	OGLOGINFO("In the copy constructor of Data.");
	if (other._bytes && other._size)
	{
		copy(other._bytes, other._size);
	}
}

Data::~Data()
{
	OGLOGINFO("deallocing Data: %p", this);
	clear();
}

Data& Data::operator= (const Data& other)
{
	if (this != &other)
	{
	//	CCLOGINFO("In the copy assignment of Data.");
		copy(other._bytes, other._size);
	}
	return *this;
}

Data& Data::operator= (Data&& other)
{
	if (this != &other)
	{
	//	CCLOGINFO("In the move assignment of Data.");
		move(other);
	}
	return *this;
}

void Data::move(Data& other)
{
	if (_bytes != other._bytes) clear();

	_bytes = other._bytes;
	_size = other._size;

	other._bytes = nullptr;
	other._size = 0;
}

bool Data::isNull() const
{
	return (_bytes == nullptr || _size == 0);
}

unsigned char* Data::getBytes() const
{
	return _bytes;
}

size_t Data::getSize() const
{
	return _size;
}

size_t Data::copy(const unsigned char* bytes, const size_t size)
{
	OGASSERT(size >= 0, "copy size should be non-negative");
	OGASSERT(bytes, "bytes should not be nullptr");

	if (size <= 0) return 0;

	if (bytes != _bytes)
	{
		clear();
		_bytes = (unsigned char*)malloc(sizeof(unsigned char) * size);
		memcpy(_bytes, bytes, size);
	}

	_size = size;
	return _size;
}

void Data::fastSet(unsigned char* bytes, const size_t size)
{
	OGASSERT(size >= 0, "fastSet size should be non-negative");
	OGASSERT(bytes, "bytes should not be nullptr");
	_bytes = bytes;
	_size = size;
}

void Data::clear()
{
	if (_bytes) free(_bytes);
	_bytes = nullptr;
	_size = 0;
}

unsigned char* Data::takeBuffer(size_t* size)
{
	auto buffer = getBytes();
	if (size)
		*size = getSize();
	fastSet(nullptr, 0);
	return buffer;
}



OG_END