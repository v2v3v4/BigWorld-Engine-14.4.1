#ifndef SCENE_HPP
#define SCENE_HPP

#include "forward_declarations.hpp"
#include "scene_type_system.hpp"

#include <cstdmf/bw_vector.hpp>
#include <cstdmf/bw_map.hpp>
#include <cstdmf/smartpointer.hpp>

namespace BW
{

class SCENE_API Scene
{
public:
	Scene();
	~Scene();

	void addProvider( SceneProvider * pProvider );
	void removeProvider( SceneProvider * pProvider );

	template <class InterfaceType>
	InterfaceType * getView()
	{
		SceneTypeSystem::RuntimeTypeID interfaceTypeID = 
			SceneTypeSystem::getSceneViewRuntimeID<InterfaceType>();

		ViewMap::iterator findResult = views_.find( interfaceTypeID );

		if (findResult == views_.end())
		{
			InterfaceType* pInterface = createView<InterfaceType>();
			findResult = 
				views_.insert( std::make_pair( interfaceTypeID, pInterface ) ).first;
		}

		InterfaceType * pInterface = reinterpret_cast< InterfaceType * >(
			findResult->second.get() );
		return pInterface;
	}

	template <class InterfaceType>
	const InterfaceType * getView() const
	{
		return const_cast<Scene*>(this)->getView<InterfaceType>();
	}

	template <class ObjectOperationType>
	ObjectOperationType* getObjectOperation()
	{
		SceneTypeSystem::RuntimeTypeID objectOperationID = 
			SceneTypeSystem::getObjectOperationRuntimeID<ObjectOperationType>();

		ObjectOperationMap::iterator findResult = 
			objectOperations_.find( objectOperationID );

		if (findResult == objectOperations_.end())
		{
			ObjectOperationType* pObjectInterface = 
				createObjectInterface<ObjectOperationType>();
			findResult = objectOperations_.insert( 
				std::make_pair( objectOperationID, pObjectInterface ) ).first;
		}

		ObjectOperationType * pObjectInterface = 
			reinterpret_cast< ObjectOperationType * >( 
				findResult->second.get() );
		return pObjectInterface;
	}

	template <class ObjectOperationType>
	const ObjectOperationType* getObjectOperation() const
	{
		return const_cast<Scene*>(this)->
			getObjectOperation<ObjectOperationType>();
	}

private:	

	template <class ViewType>
	ViewType* createView()
	{
		ViewType* pView = new ViewType();
		pView->scene( this );

		for ( SceneProviderCollection::iterator iter = sceneProviders_.begin();
			iter != sceneProviders_.end(); ++iter )
		{
			SceneProvider * pProvider = *iter;

			pView->addProvider( pProvider );
		}

		return pView;
	}

	template <class ObjectOperationType>
	ObjectOperationType * createObjectInterface()
	{
		ObjectOperationType * pInterface = new ObjectOperationType();
		return pInterface;
	}

	typedef BW::map< SceneTypeSystem::RuntimeTypeID, 
		SmartPointer< ISceneView > > ViewMap;
	typedef BW::map< SceneTypeSystem::RuntimeTypeID, 
		SmartPointer< ISceneObjectOperation > > ObjectOperationMap;
	typedef BW::vector< SceneProvider * > SceneProviderCollection;
	typedef BW::vector< SceneListener * > ListenerCollection;

	ListenerCollection listeners_;
	SceneProviderCollection sceneProviders_;
	ViewMap views_;
	ObjectOperationMap objectOperations_;
};

} // namespace BW

#endif // SCENE_HPP
