#pragma once

template<typename ptr_t>
struct OwningPtr
{
	OwningPtr(ptr_t* initalizer);
	~OwningPtr();
	operator ptr_t* ();
	ptr_t* ptr;
};

template<typename ptr_t>
OwningPtr<ptr_t>::operator ptr_t* ()
{
	return this->ptr;
}

template<typename ptr_t>
OwningPtr<ptr_t>::OwningPtr(ptr_t* initalizer)
{
	this->ptr = initalizer;
}

template<typename ptr_t>
OwningPtr<ptr_t>::~OwningPtr()
{}
