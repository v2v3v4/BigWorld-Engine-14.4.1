#ifndef SCENE_PROVIDER_HPP
#define SCENE_PROVIDER_HPP

#include "forward_declarations.hpp"
#include "scene_type_system.hpp"

#include <cstdmf/bw_vector.hpp>
#include <cstdmf/smartpointer.hpp>

namespace BW
{

class SCENE_API SceneProvider
{
public:
	SceneProvider();
	virtual ~SceneProvider();

	template <class InterfaceType>
	InterfaceType * getView()
	{
		SceneTypeSystem::RuntimeTypeID typeID = 
			SceneTypeSystem::getSceneViewRuntimeID<InterfaceType>();
		return reinterpret_cast< InterfaceType * > ( getView( typeID ) );
	}

	void scene( Scene* pScene );
	Scene* scene();
	const Scene* scene() const;

protected:

	virtual void onSetScene( Scene* pOldScene, Scene* pNewScene );
	virtual void * getView(
		const SceneTypeSystem::RuntimeTypeID & sceneInterfaceTypeID);

	template < typename Target, typename Host >
	static void exposeView(Host * pHost,
		const SceneTypeSystem::RuntimeTypeID & typeID, 
		void *& outInterface)
	{
		if (!outInterface &&
			typeID == SceneTypeSystem::getSceneViewRuntimeID<Target>())
		{
			outInterface = static_cast< Target * >( pHost );
		}
	}

private:
	Scene* pScene_;
};

class SCENE_API ISceneView : 
	public SafeReferenceCount
{
public:
	ISceneView();
	void scene( Scene * pScene );
	Scene* scene() const;

	virtual void addProvider( SceneProvider* pProvider ) = 0;
	virtual void removeProvider( SceneProvider * pProvider ) = 0;

protected:
	virtual void onSetScene( Scene * pOldScene, Scene * pNewScene );

private:
	Scene * pOwner_;
};

typedef SmartPointer< ISceneView > ISceneViewPtr;

template <class TProviderInterface>
class SCENE_API SceneView : public ISceneView
{
public:
	typedef TProviderInterface ProviderInterface;
	typedef BW::vector< ProviderInterface * > ProviderCollection;

	virtual void addProvider( SceneProvider* pProvider )
	{
		ProviderInterface* pProviderInterface = 
			pProvider->getView<ProviderInterface>();

		if (pProviderInterface)
		{
			addProvider( pProviderInterface );
		}
	}

	virtual void removeProvider( SceneProvider* pProvider )
	{
		ProviderInterface* pProviderInterface = 
			pProvider->getView<ProviderInterface>();

		if (pProviderInterface)
		{
			removeProvider( pProviderInterface );
		}
	}

	void addProvider( ProviderInterface * pProviderInterface )
	{
		providers_.push_back( pProviderInterface );
	}

	void removeProvider( ProviderInterface * pProviderInterface )
	{
		providers_.erase( 
			std::remove(
				providers_.begin(), 
				providers_.end(), 
				pProviderInterface ), 
			providers_.end() );
	}

protected:

	ProviderCollection providers_;
};

} // namespace BW

#endif
