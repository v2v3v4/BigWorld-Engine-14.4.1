#ifndef DEVICE_CALLBACK_HPP
#define DEVICE_CALLBACK_HPP

#include <iostream>
#include "cstdmf/bw_list.hpp"


BW_BEGIN_NAMESPACE

namespace Moo
{

/**
 *	Classes which implement this interface will receive notification of device
 *	loss/destruction through the various delete/create calls. This class should
 *	be inherited/implemented by any object utilising D3D resources. For examples
 *	of it's use, see the various texture / render target classes.
 */
class DeviceCallback
{
public:
	typedef BW::list< DeviceCallback * > CallbackList;

	DeviceCallback();
	virtual ~DeviceCallback();

	virtual void deleteUnmanagedObjects( );
	virtual void createUnmanagedObjects( );
	virtual void deleteManagedObjects( );
	virtual void createManagedObjects( );

	virtual bool recreateForD3DExDevice() const;

	static void deleteAllUnmanaged( bool forceRelease = false );
	static void createAllUnmanaged( bool forceCreate = false );
	static void deleteAllManaged( );
	static void createAllManaged( );

	static void fini();
private:
/*	DeviceCallback(const DeviceCallback&);
	DeviceCallback& operator=(const DeviceCallback&);*/

	friend std::ostream& operator<<(std::ostream&, const DeviceCallback&);
};

/**
 *	A Generic implementation of the DeviceCallback interface for unmanaged 
 *	resources. The function callbacks passed into the destructor are used 
 *	for the creation/destruction of the resources.
 */
class GenericUnmanagedCallback : public DeviceCallback
{
public:

	typedef void Function( );

	GenericUnmanagedCallback( Function* createFunction, Function* destructFunction  );
	~GenericUnmanagedCallback();

	void deleteUnmanagedObjects( );
	void createUnmanagedObjects( );

private:

	Function* createFunction_;
	Function* destructFunction_;

};

} // namespace Moo

#ifdef CODE_INLINE
#include "device_callback.ipp"
#endif

BW_END_NAMESPACE


#endif // DEVICE_CALLBACK_HPP
