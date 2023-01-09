#ifndef VERTICES_MANAGER_HPP
#define VERTICES_MANAGER_HPP

#include "cstdmf/concurrency.hpp"
#include "vertices.hpp"
#include "device_callback.hpp"


BW_BEGIN_NAMESPACE

namespace Moo
{

/**
 *	This singleton class keeps track of and loads Vertices.
 */
class VerticesManager : public Moo::DeviceCallback
{
public:
	typedef BW::map< BW::string, Vertices * > VerticesMap;

	~VerticesManager();

	static VerticesManager*		instance();

	VerticesPtr					get( const BW::string& resourceID, int numNodes = 0 );
	void populateResource( const BW::string& resourceID, const VerticesPtr& pVerts);

	virtual void				deleteManagedObjects();
	virtual void				createManagedObjects();

	static void			init();
	static void			fini();

	void find( const BW::string & primitiveFile, VerticesPtr vertices[], size_t& howMany );


private:
	VerticesManager();
	VerticesManager(const VerticesManager&);
	VerticesManager& operator=(const VerticesManager&);

	static void tryDestroy( const Vertices * pVertices, bool isInVerticesMap );
	void addInternal( Vertices* pVertices );
	void delInternal( const Vertices* pVertices );

	VerticesPtr find( const BW::string & resourceID );

	VerticesMap					vertices_;
	SimpleMutex					verticesLock_;

	friend void Vertices::destroy() const;

	static VerticesManager*	pInstance_;


};

} // namespace Moo

BW_END_NAMESPACE

#endif // VERTICES_MANAGER_HPP
