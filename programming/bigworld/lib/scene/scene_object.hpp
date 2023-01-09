#ifndef SCENE_OBJECT_HPP
#define SCENE_OBJECT_HPP

#include "forward_declarations.hpp"
#include "scene_type_system.hpp"
#include "scene_object_flags.hpp"

#include <cstddef>
#include "cstdmf/bw_hash.hpp"
#include "cstdmf/lookup_table.hpp"
#include "cstdmf/smartpointer.hpp"

namespace BW
{

class SCENE_API SceneObject
{
public:
	typedef uintptr ObjectHandle;

	SceneObject() :
		handle_(0),
		type_(0),
		flags_()
	{
	}

	SceneObject( const SceneObject& other) :
		handle_( other.handle_ ),
		type_( other.type_ ),
		flags_( other.flags_ ) 
	{
	}

	template <class ObjectType>
	explicit SceneObject( ObjectType * pObject, SceneObjectFlags flags ) :
		handle_( reinterpret_cast< ObjectHandle >( pObject ) ),
		type_( typeOf< ObjectType >() ),
		flags_( flags )
	{
	}

	template <class ObjectType>
	static SceneTypeSystem::RuntimeTypeID typeOf()
	{
		return SceneTypeSystem::getObjectRuntimeID< ObjectType >();
	}

	template <class ObjectType>
	ObjectType * getAs() const
	{
		MF_ASSERT( isType<ObjectType>() );
		return reinterpret_cast< ObjectType * >( handle_ ); 
	}

	SceneObjectFlags& flags()
	{
		return flags_;
	}

	const SceneObjectFlags& flags() const
	{
		return flags_;
	}

	SceneTypeSystem::RuntimeTypeID type() const
	{
		return type_;
	}

	void type( SceneTypeSystem::RuntimeTypeID type )
	{
		type_ = type;
	}

	template <class ObjectType>
	void type()
	{
		type_ = typeOf< ObjectType >();
	}

	template <class ObjectType>
	bool isType() const
	{
		return type_ == typeOf< ObjectType >();
	}

	ObjectHandle handle() const
	{
		return handle_;
	}

	void handle( const ObjectHandle& handle)
	{
		handle_ = handle;
	}

	template <class ObjectType>
	void handle( ObjectType * pObject )
	{
		MF_ASSERT( isType<ObjectType>() );
		handle_ = reinterpret_cast< ObjectHandle >( pObject ) ;
	}

	template <class ObjectType>
	void handleAndType( ObjectType * pObject )
	{
		type_ = typeOf<ObjectType>();
		handle_ = reinterpret_cast< ObjectHandle >( pObject ) ;
	}

	bool operator == ( const SceneObject & other ) const
	{
		return handle_ == other.handle_ &&
			type_ == other.type_;
	}

	bool isValid() const
	{
		return handle_ != 0;
	}

	const char * typeName() const
	{
		SceneTypeSystem::GlobalUniqueTypeID gutid = 
			SceneTypeSystem::sceneObjectTypeIDContext().getGlobalID( type_ );
		return SceneTypeSystem::fetchTypeName( gutid );
	}

private:
	friend struct BW_HASH_NAMESPACE hash<SceneObject>;

	ObjectHandle handle_;
	SceneTypeSystem::RuntimeTypeID type_;
	SceneObjectFlags flags_;
};

class SCENE_API ISceneObjectOperation :
	public SafeReferenceCount
{
public:
	virtual ~ISceneObjectOperation() {}
};


template < typename TypeHandlerInterface >
class SCENE_API SceneObjectOperation :
	public ISceneObjectOperation
{
	typedef BW::LookUpTable<TypeHandlerInterface *> TypeHandlerMap;
	typedef typename TypeHandlerMap::iterator TypeHandlerMap_iterator;
	
public:	

	virtual ~SceneObjectOperation()
	{
		for (TypeHandlerMap_iterator iter = typeHandlers_.begin();
			iter != typeHandlers_.end(); ++iter)
		{
			TypeHandlerInterface * handler = *iter;
			delete handler;
		}
	}

	template < class ObjectType, class TypeHandlerImplementation >
	TypeHandlerImplementation * addHandler( )
	{
		TypeHandlerImplementation * handler = new TypeHandlerImplementation();

		SceneTypeSystem::RuntimeTypeID typeID = 
			SceneTypeSystem::getObjectRuntimeID< ObjectType >();

		addHandler( typeID, handler );
		return handler;
	}

	template < class ObjectType >
	void removeHandler()
	{
		SceneTypeSystem::RuntimeTypeID typeID = 
			SceneTypeSystem::getObjectRuntimeID< ObjectType >();

		removeHandler( typeID );
	}

	template < class ObjectType >
	TypeHandlerInterface* getHandler()
	{
		SceneTypeSystem::RuntimeTypeID typeID = 
			SceneTypeSystem::getObjectRuntimeID< ObjectType >();

		return getHandler( typeID );
	}

	template < class ObjectType >
	const TypeHandlerInterface* getHandler() const
	{
		SceneTypeSystem::RuntimeTypeID typeID = 
			SceneTypeSystem::getObjectRuntimeID< ObjectType >();

		return getHandler( typeID );
	}

	TypeHandlerInterface* getHandler(
		const SceneTypeSystem::RuntimeTypeID & objectType )
	{
		TypeHandlerMap_iterator iter = typeHandlers_.find(objectType);
		if (iter != typeHandlers_.end())
		{
			return *iter;
		}
		return NULL;
	}

	const TypeHandlerInterface* getHandler(
		const SceneTypeSystem::RuntimeTypeID & objectType ) const
	{
		return const_cast<SceneObjectOperation*>(this)->
			getHandler( objectType );
	}

	bool isSupported( const SceneObject& obj )
	{
		return isSupported( obj.type() );
	}

	bool isSupported( const SceneTypeSystem::RuntimeTypeID & objectType )
	{
		return getHandler( objectType ) != NULL;
	}

private:

	void removeHandler( const SceneTypeSystem::RuntimeTypeID & objectType )
	{
		TypeHandlerMap_iterator findResult = typeHandlers_.find( objectType );
		if (findResult == typeHandlers_.end())
		{
			return;
		}

		TypeHandlerInterface * handler = *findResult;
		delete handler;
		typeHandlers_.erase( findResult );
	}

	void addHandler( const SceneTypeSystem::RuntimeTypeID & objectType, 
		TypeHandlerInterface * handler )
	{
		typeHandlers_[objectType] = handler;
	}

	TypeHandlerMap typeHandlers_; 
};


} // namespace BW


BW_HASH_NAMESPACE_BEGIN

template<>
struct hash<BW::SceneObject> : 
	public std::unary_function<BW::SceneObject, std::size_t>
{
	std::size_t operator()(const BW::SceneObject& s) const
	{
		size_t result = 0;
		BW::hash_combine( result, s.handle_ );
		BW::hash_combine( result, s.type_ );
		BW::hash_combine( result, s.flags_ );
		return result;
	}
};

BW_HASH_NAMESPACE_END

#endif // SCENE_OBJECT_HPP

