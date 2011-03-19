/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#ifndef OWNER_H
#define OWNER_H

#include <cassert>
#include <cstddef>

/**
 * @class Owner
 * Smart pointer with deep copy semantics.
 */

template <class T>
class Owner {
public:
	Owner(T* ptr = NULL)
		: ptr(ptr)
	{}
	~Owner()
	{
		delete ptr;
	}
	Owner(const Owner& rhs)
	{
		if (rhs.ptr)
			ptr = new T(*rhs.ptr);
		else
			ptr = NULL;
	}
	Owner& operator=(const Owner& rhs)
	{
		if (ptr != rhs.ptr)
			delete ptr;
		if (rhs.ptr)
			ptr = new T(*rhs.ptr);
		else
			ptr = NULL;
		return *this;
	}
	T& operator*() const { return *ptr; }
	T* operator->() const { return ptr; }
	bool operator!() const { return !ptr; }
	T* get() const { return ptr; }
	// FIXME? or is it too painful to use, otherwise?
	operator T*() const { return ptr; }
	void release() {
		delete ptr;
		ptr = NULL;
	}
protected:
	T *ptr;
};

#endif
