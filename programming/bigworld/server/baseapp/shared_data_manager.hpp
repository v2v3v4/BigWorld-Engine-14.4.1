#ifndef SHARED_DATA_MANAGER_HPP
#define SHARED_DATA_MANAGER_HPP

#include "cstdmf/bw_namespace.hpp"
#include "cstdmf/shared_ptr.hpp"


BW_BEGIN_NAMESPACE

class BinaryIStream;
class BinaryOStream;
class Pickler;
class SharedData;

/**
 *	This class is responsible for handling the SharedData instances.
 */
class SharedDataManager
{
public:
	static shared_ptr< SharedDataManager > create( Pickler * pPickler );
	~SharedDataManager();

	void setSharedData( BinaryIStream & data );
	void delSharedData( BinaryIStream & data );

	void addToStream( BinaryOStream & stream );

private:
	SharedDataManager();

	bool init( Pickler * pPickler );

	SharedData *	pBaseAppData_;
	SharedData *	pGlobalData_;
};

BW_END_NAMESPACE

#endif // SHARED_DATA_MANAGER_HPP
