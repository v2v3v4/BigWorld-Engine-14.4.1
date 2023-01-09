#ifndef PRIMITIVE_MANAGER_HPP
#define PRIMITIVE_MANAGER_HPP

#include "cstdmf/concurrency.hpp"
#include "primitive.hpp"
#include "device_callback.hpp"


BW_BEGIN_NAMESPACE

namespace Moo
{

/**
 *	This singleton class keeps track of and loads Primitives.
 */
class PrimitiveManager : public Moo::DeviceCallback
{
public:
	typedef BW::map< BW::string, Primitive * > PrimitiveMap;

	~PrimitiveManager();

	static PrimitiveManager* instance();

	PrimitivePtr				get( const BW::string& resourceID );

	virtual void				deleteManagedObjects( );
	virtual void				createManagedObjects( );

	static void init();
	static void fini();

	void find( const BW::string & primitiveFile, PrimitivePtr primitives[], size_t& howMany );

private:
	PrimitiveManager();
	PrimitiveManager(const PrimitiveManager&);
	PrimitiveManager& operator=(const PrimitiveManager&);

	static void tryDestroy( const Primitive * pPrimitive, bool isInPrimativeMap );

	void addInternal( Primitive * pPrimitive );
	void delInternal( const Primitive * pPrimitive );

	PrimitivePtr find( const BW::string & resourceID );

	PrimitiveMap				primitives_;
	SimpleMutex					primitivesLock_;

	static PrimitiveManager*	pInstance_;

	friend void Primitive::destroy() const;
};

} // namespace Moo

BW_END_NAMESPACE


#endif // PRIMITIVE_MANAGER_HPP
