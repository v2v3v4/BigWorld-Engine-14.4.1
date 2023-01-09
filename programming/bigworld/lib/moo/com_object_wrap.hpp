#ifndef COM_OBJECT_WRAP_HPP
#define COM_OBJECT_WRAP_HPP


#include "cstdmf/resource_counters.hpp"

#include <d3dx9effect.h>

#if ENABLE_RESOURCE_COUNTERS
#include "moo/moo_dx.hpp"
#endif

#include "cstdmf/profiler.hpp"
#include "cstdmf/guard.hpp"
#include "cstdmf/bw_string.hpp"
#include "cstdmf/bw_string_ref.hpp"


BW_BEGIN_NAMESPACE

/**
 *	COM object wrapper class template used for safe access, automatic resource release 
 *	and memory accounting.
 */
template <typename COMOBJECTTYPE>
struct ComObjectWrap
{
	typedef COMOBJECTTYPE		ComObject;
	typedef ComObject			*ComObjectPtr;
	typedef ComObject			&ComObjectRef;

	ComObjectWrap();
	ComObjectWrap(ComObject *object);
	ComObjectWrap(ComObjectWrap const &other);
	virtual ~ComObjectWrap();

	ComObjectWrap &operator=(int n);
	ComObjectWrap &operator=(ComObjectWrap const &other);

	/**
	 *	This is the template version of operator=
	 *
	 *  @param other			The ComObjectWrap to copy from.
	 *  @returns				*this.
	 */
	template<typename OTHER>
		ComObjectWrap &operator=(ComObjectWrap<OTHER> const &other)
		{
			ComObjectWrap obj;
			if( SUCCEEDED( other->QueryInterface( __uuidof(COMOBJECTTYPE), (LPVOID*)&obj ) ) )
				copy( obj, false ); //QueryInterface increments the ref count
			return *this;
		}

	bool operator==(ComObjectWrap const &other) const;
	bool operator!=(int n) const;

	bool hasComObject() const;

	void addAlloc(const BW::string &desc);
	void addAlloc(const BW::StringRef &desc);

	void pComObject(ComObject *object);
	ComObjectPtr pComObject() const;

	ComObjectPtr operator->() const;
	ComObjectRef operator*() const;
	ComObjectPtr *operator&();

	operator void*() const
	{
		return comObject_;
	}
	bool operator !() const
	{
		return !comObject_;
	}

protected:
	void copy(ComObjectWrap const &other, bool bAddRef=true);

	void addRef();
	void release();

private:
	ComObject					*comObject_;

#if ENABLE_RESOURCE_COUNTERS

protected:
	uint pool() const;
	ResourceCounters::ResourceType resourceType() const;

	/**
	 * Size in memory.
	 */
	uint size() const;

	/**
	 * GPU memory size.
	 */
	uint gpuMemorySize() const;

#endif
};


#include "moo/com_object_wrap.ipp"

BW_END_NAMESPACE

#endif // COM_OBJECT_WRAP_HPP
