#ifndef __TERRAIN_RESOURCE_HPP__
#define __TERRAIN_RESOURCE_HPP__

#include "cstdmf/smartpointer.hpp"
#include "cstdmf/bgtask_manager.hpp"


BW_BEGIN_NAMESPACE

namespace Terrain
{
	enum ResourceState 
	{
		RS_Unloaded,
		RS_Loading,
		RS_Loaded
	};

	enum ResourceRequired
	{
		RR_No,
		RR_Yes
	};

	enum ResourceStreamType
	{
		RST_Asyncronous,
		RST_Syncronous
	};

	/** The Resource type wraps an object that is used as a streamed resource.
		That is, an object that may be loaded and unloaded throughout the 
		application's lifetime. 

		To use this class, do the following:
		1.  Derive a resource class for your object, e.g.:

				class ResourceObject : public Resource<Object>
				{ ... }

		2.	Provide implementation for evalute() and load() functions.
		1.	At runtime, let the resource evaluate its object to decide whether
			the object should be loaded. Like so:

				resourceObject_->evaluate( dependencyInfo );

		2.	At runtime, let the resource load or unload the object. stream() 
			will call load() or unload() and by default does the load 
			asynchronously:

				resourceObject->stream();

		The evaluate function can also prepare the object for streaming by 
		setting other internal flags. It is entirely up to the derived resource
		class.

		The default implementation of this is when an object either exists or 
		not. At the bear minimum you only need to provide an implementation for 
		both evaluate() and load(). 

		However, if you override getState(), load() and unload() you can control
		an object that has some "partial state" like the blends in the editor. 
		Optionally you may also override the stream() method. 
		(See TerrainBlends class for example usage).
	**/
	class ResourceBase
	{
	public:
		static ResourceStreamType& defaultStreamType()
		{
			static ResourceStreamType s_type = RST_Asyncronous;

			return s_type;
		}

		static void defaultStreamType( ResourceStreamType type )
		{
			defaultStreamType() = type;
		}
	};

	template < typename O >
	class Resource : public SafeReferenceCount, public ResourceBase
	{
	public:
		// Types
		typedef O							ObjectType;
		typedef SmartPointer<ObjectType>	ObjectTypePtr;

		class ResourceTask : public BackgroundTask
		{
		public:
			ResourceTask( SmartPointer<Resource> resource ) :
				BackgroundTask( "ResourceTask" ),
				resource_( resource )
			{
				MF_ASSERT( resource_ );
				resource_->preAsyncLoad();
			};

			virtual ~ResourceTask()
			{
				// remove reference to task from host resource
				resource_->task_ = NULL;
			};

			virtual void doBackgroundTask( TaskManager & mgr )
			{
				resource_->load();
				mgr.addMainThreadTask( this );
			}

			virtual void doMainThreadTask( TaskManager & mgr )
			{
				resource_->postAsyncLoad();
			}

		private:
			SmartPointer<Resource> resource_;
		};

		// con/destructor
		Resource();
		virtual ~Resource();

		// Evaluate whether or not this resource is required now, sets required 
		// state. Subclasses should override this to provide streaming logic,
		// and set the "required_" member appropriately.
		// This is not pure virtual because overrides will have different 
		// dependency parameters.
		ResourceRequired evaluate( bool required = false )
		{ required_ = required ? RR_Yes : RR_No; return required_; } ;

		// Act on internal required state, eg load, unload or leave the object.
		virtual void		stream( ResourceStreamType streamType 
										= defaultStreamType() );

		// Return object, or NULL if not in loaded state. You cannot access
		// partially loaded objects.
		inline ObjectTypePtr		getObject();

		// Return required state of this resource
		inline bool					isRequired() const;

		// Return current state of this resource
		virtual ResourceState	 	getState() const;

	protected:
		// Override this to perform loading for object.
		virtual bool		load() = 0 ;
		// Override if you require custom unloading, by default this just 
		// dereferences the object.
		virtual void		unload();

		// Override  if you require custom asynchronous loading.
		virtual void		preAsyncLoad() {}
		virtual void		startAsyncTask();
		virtual void		postAsyncLoad() {}

		ResourceRequired	required_;
		ObjectTypePtr		object_;

		// How are we loading this?
		ResourceStreamType	streamType_;

	private:
		ResourceTask*		task_;
	};

	//
	// Resource implementation
	//
	template< typename O >
	inline Resource<O>::Resource() :
			required_( RR_No ),
			object_(NULL),
			streamType_( defaultStreamType() ),
			task_( NULL )
	{
	}

	template< typename O >
	inline Resource<O>::~Resource()
	{
	}

	template< typename O >
	inline typename Resource<O>::ObjectTypePtr Resource<O>::getObject()
	{
		if ( getState() == RS_Loaded )
			return object_;
		else
			return NULL;
	}

	template< typename O >
	inline ResourceState Resource<O>::getState() const
	{
		if ( task_ == NULL )
			return object_ ? RS_Loaded : RS_Unloaded;
		else
			return RS_Loading;
	}

	template< typename O >
	inline bool Resource<O>::isRequired() const
	{
		return required_ == RR_Yes;
	}

	template< typename O >
	inline void Resource<O>::stream( ResourceStreamType streamType )
	{	
		// Remember how we loaded this
		streamType_ = streamType;

		if ( required_ == RR_No && getState() == RS_Loaded )
		{
			unload();
		}
		else if ( required_ == RR_Yes && getState() == RS_Unloaded )
		{
			// load object
			MF_ASSERT( object_ == NULL );

			if ( streamType == RST_Asyncronous )
			{
				startAsyncTask();
			}
			else
			{
				load();
			}
		}
	}

	template< typename O >
	inline void Resource<O>::unload()
	{
		// unload object
		MF_ASSERT( task_ == NULL );
		MF_ASSERT( object_ != NULL );

		object_ = NULL;
	}

	template< typename O >
	inline void Resource<O>::startAsyncTask()
	{
		MF_ASSERT( task_ == NULL );

		task_ = new ResourceTask( this );
		BgTaskManager::instance().addBackgroundTask( task_ );
	}

} // namespace Terrain

BW_END_NAMESPACE

#endif
