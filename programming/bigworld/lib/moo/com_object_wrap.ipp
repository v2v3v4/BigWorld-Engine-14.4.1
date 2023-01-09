#ifndef COM_OBJECT_WRAP_IPP
#define COM_OBJECT_WRAP_IPP

/**
 *	This is the ComObjectWrap constructor.  It sets the underyling COM object
 *	to NULL.
 */
template<typename COMOBJECTTYPE>
inline
ComObjectWrap<COMOBJECTTYPE>::ComObjectWrap():
	comObject_(NULL)
{
}


/**
 *	This is the ComObjectWrap constructor.
 *
 *	@param object			The object being wrapped.
 */
template<typename COMOBJECTTYPE>
inline
ComObjectWrap<COMOBJECTTYPE>::ComObjectWrap(ComObject *object):
	comObject_(NULL)
{
	pComObject(object);
}


/**
 *	This is the ComObjectWrap copy constructor.
 *
 *  @param other			The ComObjectWrap to copy from.
 */
template<typename COMOBJECTTYPE>
inline
ComObjectWrap<COMOBJECTTYPE>::ComObjectWrap(ComObjectWrap const &other):
	comObject_(NULL)
{
	copy(other);
}


/**
 *	This is the ComObjectWrap destructor.
 */
template<typename COMOBJECTTYPE>
inline
ComObjectWrap<COMOBJECTTYPE>::~ComObjectWrap()
{
	release();
}


/**
 *	This resets the ComObjectWrap to NULL.
 *
 *	@param n				This should be NULL.
 *	@returns				*this.
 */
template<typename COMOBJECTTYPE>
inline
ComObjectWrap<COMOBJECTTYPE> &
ComObjectWrap<COMOBJECTTYPE>::operator=(int n)
{
	if(n != 0)
		throw "n must be 0 here for a null pointer";
	release();
	return *this;
}


/**
 *	This is the ComObjectWrap assignment operator.
 *
 *  @param other			The ComObjectWrap to copy from.
 *  @returns				*this.
 */
template<typename COMOBJECTTYPE>
inline
ComObjectWrap<COMOBJECTTYPE>& 
ComObjectWrap<COMOBJECTTYPE>::operator=
(
	ComObjectWrap const &other
)
{
	if (comObject_ != other.comObject_)
		copy(other);
	return *this;
}


/**
 *	This compares the underlying COM objects.
 *
 *  @param other			The other ComObjectWrap to compare against.
 *	@returns				True if the wrappers point to the same
 *							COM object.
 */
template<typename COMOBJECTTYPE>
inline
bool ComObjectWrap<COMOBJECTTYPE>::operator==(ComObjectWrap const &other) const
{
	return comObject_ == other.comObject_;
}


/**
 *	This returns whether the wrapper points to anything.
 *
 *	@param n				Not used.  This should be set to NULL.
 *	@returns				True if the COM object is NULL.
 */
template<typename COMOBJECTTYPE>
inline
bool ComObjectWrap<COMOBJECTTYPE>::operator!=(int n) const
{
	MF_ASSERT(n == 0);
	return comObject_ != NULL;
}


/**
 *	This determines whether the ComObjectWrap points to something.
 *
 *  @returns				True if the ComObjectWrap is not to the
 *							NULL object.
 */
template<typename COMOBJECTTYPE>
inline
bool ComObjectWrap<COMOBJECTTYPE>::hasComObject() const
{
	return comObject_ != NULL;
}

#if ENABLE_RESOURCE_COUNTERS
/**
 *	This class keep tracks of all direct3d resources
 */
class D3DResourceTrack : public IUnknown
{
	ResourceCounters::Handle resCntHandle_;
	DWORD pool_;
	ResourceCounters::ResourceType resourceType_;
	DWORD size_;
	DWORD gpuSize_;
	ULONG refCount_;
public:
	D3DResourceTrack( const BW::string& allocator, DWORD pool, ResourceCounters::ResourceType resourceType, DWORD size, DWORD gpuSize )
		: pool_( pool ), resourceType_( resourceType ), size_( size ), gpuSize_( gpuSize ), refCount_( 0 )
	{
		resCntHandle_ = RESOURCE_COUNTER_NEWHANDLE(allocator);
		if (ResourceCounters::instance().isValid())
		{
			ResourceCounters::instance().add
			(
				ResourceCounters::DescriptionPool
				(
					ResourceCounters::instance().description(resCntHandle_),
					pool,
					resourceType_
				), 
				size,
				gpuSize
			);
		}
	}
	~D3DResourceTrack()
	{
		if (ResourceCounters::instance().isValid())
		{
			ResourceCounters::instance().subtract
			(
				ResourceCounters::DescriptionPool
				(
					ResourceCounters::instance().description(resCntHandle_),
					pool_,
					resourceType_
				), 
				size_,
				gpuSize_
			);
		}
	}
	ULONG STDMETHODCALLTYPE AddRef()
	{
		return InterlockedIncrement( (LONG*)&refCount_ );
	}
	HRESULT STDMETHODCALLTYPE QueryInterface( const IID & riid, void **ppvObj )
	{
		HRESULT hr = E_NOINTERFACE;
		if( riid == __uuidof(IUnknown) )
		{
			AddRef();
			*ppvObj = this;
			hr = S_OK;
		}
		return hr;
	}
	ULONG STDMETHODCALLTYPE Release()
	{
		if( InterlockedDecrement( (LONG*)&refCount_ ) == 0 )
		{
			delete this;
			return 0;
		}
		return refCount_;
	}
};
#endif

/**
 *	This sets the allocator (resource counter) for this COM object and
 *	adds the size of the object to the appropriate pool.
 *
 *  @param desc				The resource counter that the COM object
 *							should be using.
 */
template<typename COMOBJECTTYPE>
inline
void ComObjectWrap<COMOBJECTTYPE>::addAlloc(const BW::string &desc)
{
#if ENABLE_RESOURCE_COUNTERS
	IUnknown* unk = new D3DResourceTrack( desc, pool(), resourceType(), size(), gpuMemorySize() );
	pComObject()->SetPrivateData( __uuidof(IUnknown), unk, sizeof( unk ), D3DSPD_IUNKNOWN );
#endif
}

/**
 *	This sets the allocator (resource counter) for this COM object and
 *	adds the size of the object to the appropriate pool.
 *
 *  @param desc				The resource counter that the COM object
 *							should be using.
 */
template<typename COMOBJECTTYPE>
inline
void ComObjectWrap<COMOBJECTTYPE>::addAlloc(const BW::StringRef &desc)
{
#if ENABLE_RESOURCE_COUNTERS
	addAlloc( desc.to_string() );
#endif
}


/**
 *	This sets a new COM object to wrap.
 *
 *  @param object			The new COM object to wrap.
 */
template<typename COMOBJECTTYPE>
inline
void ComObjectWrap<COMOBJECTTYPE>::pComObject(ComObject *object)
{
	if (comObject_ == object)
		return;

	release();
	comObject_ = object;
	addRef();
}


/**
 *	This gets the COM object that is being wrapped.
 *
 *  @returns				The COM object that is being wrapped.
 */
template<typename COMOBJECTTYPE>
inline
typename ComObjectWrap<COMOBJECTTYPE>::ComObjectPtr 
ComObjectWrap<COMOBJECTTYPE>::pComObject() const
{
	return comObject_;
}


/**
 *	This gets the COM object that is begin wrapped.
 *
 *  @returns				The COM object that is being wrapped.
 */
template<typename COMOBJECTTYPE>
inline
typename ComObjectWrap<COMOBJECTTYPE>::ComObjectPtr 
ComObjectWrap<COMOBJECTTYPE>::operator->() const
{
	return comObject_;
}


/**
 *	This gets a reference to the COM object being wrapped.
 *
 *  @returns				A reference to the object being wrapped.
 */
template<typename COMOBJECTTYPE>
inline
typename ComObjectWrap<COMOBJECTTYPE>::ComObjectRef 
ComObjectWrap<COMOBJECTTYPE>::operator*() const
{
	return *comObject_;
}


/**
 *	This gets a pointer to the COM object being wrapped.
 *
 *  @returns				A pointer to the object being wrapped.
 */
template<typename COMOBJECTTYPE>
inline
typename ComObjectWrap<COMOBJECTTYPE>::ComObjectPtr * 
ComObjectWrap<COMOBJECTTYPE>::operator&()
{
	return &comObject_;
}


/**
 *	This protected function releases the current COM object if there is one and
 *	assigns this object to another.  It is used by the copy constructor and
 *	assignment operators.
 *
 *	@param other			The ComObjectWrap to copy.
 *  @param bAddRef			Boolean indicating whether to add a reference count
 *                          on copy. Default is true.
 */
template<typename COMOBJECTTYPE>
inline
void ComObjectWrap<COMOBJECTTYPE>::copy(ComObjectWrap const &other, bool bAddRef/*=true*/)
{
	release();

	comObject_    = other.comObject_;

	if (bAddRef)
		addRef();
}


/**
 *	This increases the reference count of the COM object by one.
 */
template<typename COMOBJECTTYPE>
inline
void ComObjectWrap<COMOBJECTTYPE>::addRef()
{
	if (comObject_ != NULL)
	{
		comObject_->AddRef();
	}
}

/**
 *	This decreases the reference count of the COM object by one and makes the
 *	wrapper point to NULL.
 */
template<typename COMOBJECTTYPE>
inline
void ComObjectWrap<COMOBJECTTYPE>::release()
{
	BW_GUARD;
	
	if (comObject_ != NULL)
	{
		comObject_->Release();
		comObject_ = NULL;
	}	
}

// Specialisations of pool and size for various DirectX components
#if ENABLE_RESOURCE_COUNTERS


/**
 *	This gets the DirectX pool of the object being wrapped.
 *
 *  @returns				The DirectX pool of the object being 
 *							wrapped.
 */
template<typename COMOBJECTTYPE>
inline uint ComObjectWrap<COMOBJECTTYPE>::pool() const
{
	return ResourceCounters::MP_DEFAULT;
}

/**
 *	This gets the resource type of the object being wrapped.
 *
 *  @returns				The resource type of the object being
 *							wrapped.
 */
template<typename COMOBJECTTYPE>
inline ResourceCounters::ResourceType ComObjectWrap<COMOBJECTTYPE>::resourceType() const
{
	return ResourceCounters::RT_OTHER;
}


/**
 *	This gets the memory size of the object being wrapped.
 *
 *  @returns				The size of the object being wrapped.
 *							This is specialised for many object types,
 *							but needs to be specialised for any
 *							future types.
 */
template<typename COMOBJECTTYPE>
inline uint ComObjectWrap<COMOBJECTTYPE>::size() const
{
	assert(0);
	return 0;
}

/**
 *	This gets the GPU memory size of the object being wrapped.
 *
 *  @returns				The size of the object being wrapped.
 *							This is specialised for many object types,
 *							but needs to be specialised for any
 *							future types.
 */
template<typename COMOBJECTTYPE>
inline uint ComObjectWrap<COMOBJECTTYPE>::gpuMemorySize() const
{
	return 0;
}


/**
 *	This is the specialisation of pool for effects.
 */
template<> inline uint ComObjectWrap<ID3DXEffect>::pool() const
{
	return ResourceCounters::MP_DEFAULT;
}

/**
 *	This is the specialisation of size for effects.
 */
template<> inline uint ComObjectWrap<ID3DXEffect>::size() const
{
	// Default size of 1KB
	return 1024;
}

/**
 *	This is the specialisation of pool for vertex buffers.
 */
template<> inline uint ComObjectWrap<DX::VertexBuffer>::pool() const
{
	// Get a vertex buffer description
	D3DVERTEXBUFFER_DESC vbDesc;
	comObject_->GetDesc(&vbDesc);

	return (uint)vbDesc.Pool;
}

/**
 *	This is the specialisation of resourceType for vertex buffers.
 */
template<> inline ResourceCounters::ResourceType ComObjectWrap<DX::VertexBuffer>::resourceType() const
{
	return ResourceCounters::RT_VERTEX_BUFFER;
}

/**
 *	This is the specialisation of size for vertex buffers.
 */
template<> inline uint ComObjectWrap<DX::VertexBuffer>::size() const
{
	return DX::vertexBufferSize(comObject_, ResourceCounters::MT_MAIN);
}

template<> inline uint ComObjectWrap<DX::VertexBuffer>::gpuMemorySize() const
{
	return DX::vertexBufferSize(comObject_, ResourceCounters::MT_GPU);
}

/**
 *	This is the specialisation of pool for index buffers.
 */
template<> inline uint ComObjectWrap<DX::IndexBuffer>::pool() const
{
	// Get a index buffer description
	D3DINDEXBUFFER_DESC ibDesc;
	comObject_->GetDesc(&ibDesc);

	return (uint)ibDesc.Pool;
}

/**
 *	This is the specialisation of resourceType for index buffers.
 */
template<> inline ResourceCounters::ResourceType ComObjectWrap<DX::IndexBuffer>::resourceType() const
{
	return ResourceCounters::RT_INDEX_BUFFER;
}

/**
 *	This is the specialisation of size for vertex buffers.
 */
template<> inline uint ComObjectWrap<DX::IndexBuffer>::size() const
{
	return DX::indexBufferSize(comObject_, ResourceCounters::MT_MAIN);
}

/**
 *	This is the specialisation of size for index buffers.
 */
template<> inline uint ComObjectWrap<DX::IndexBuffer>::gpuMemorySize() const
{
	return DX::indexBufferSize(comObject_, ResourceCounters::MT_GPU);
}


/**
 *	This is the specialisation of pool for textures.
 */
template<> inline uint ComObjectWrap<DX::Texture>::pool() const
{
	// Get a surface to determine pool
	D3DSURFACE_DESC surfaceDesc;
	comObject_->GetLevelDesc(0, &surfaceDesc);

	return (uint)surfaceDesc.Pool;
}

/**
 *	This is the specialisation of resourceType for textures.
 */
template<> inline ResourceCounters::ResourceType ComObjectWrap<DX::Texture>::resourceType() const
{
	return ResourceCounters::RT_TEXTURE;
}

/**
 *	This is the specialisation of size for vertex buffers.
 */
template<> inline uint ComObjectWrap<DX::Texture>::size() const
{
	return DX::textureSize(comObject_, ResourceCounters::MT_MAIN);
}

/**
 *	This is the specialisation of size for textures.
 */
template<> inline uint ComObjectWrap<DX::Texture>::gpuMemorySize() const
{
	return DX::textureSize(comObject_, ResourceCounters::MT_GPU);
}

/**
 *	This is the specialisation of pool for surfaces.
 */
template<> inline uint ComObjectWrap<DX::Surface>::pool() const
{
	// Get a surface to determine pool
	D3DSURFACE_DESC surfaceDesc;
	comObject_->GetDesc( &surfaceDesc );
	return (uint)surfaceDesc.Pool;
}

/**
 *	This is the specialisation of resourceType for surfaces.
 */
template<> inline ResourceCounters::ResourceType ComObjectWrap<DX::Surface>::resourceType() const
{
	return ResourceCounters::RT_TEXTURE;
}

/**
 *	This is the specialisation of size for surfaces.
 */
template<> inline uint ComObjectWrap<DX::Surface>::size() const
{
	D3DSURFACE_DESC surfaceDesc;
	comObject_->GetDesc( &surfaceDesc );
	return DX::surfaceSize(surfaceDesc);
}

/**
 *	This is the specialisation of pool for textures.
 */
template<> inline uint ComObjectWrap<DX::CubeTexture>::pool() const
{
	// Get a surface to determine pool
	D3DSURFACE_DESC surfaceDesc;
	comObject_->GetLevelDesc(0, &surfaceDesc);

	return (uint)surfaceDesc.Pool;
}

/**
 *	This is the specialisation of resourceType for textures.
 */
template<> inline ResourceCounters::ResourceType ComObjectWrap<DX::CubeTexture>::resourceType() const
{
	return ResourceCounters::RT_TEXTURE;
}

/**
 *	This is the specialisation of size for textures.
 */
template<> inline uint ComObjectWrap<DX::CubeTexture>::size() const
{
	// NOTE: Run a quick experiment on this, and possibly move into DX::textureSize.
	return DX::textureSize(comObject_, ResourceCounters::MT_MAIN) * 6;
}

/**
 *	This is the specialisation of size for textures.
 */
template<> inline uint ComObjectWrap<DX::CubeTexture>::gpuMemorySize() const
{
	// NOTE: Run a quick experiment on this, and possibly move into DX::textureSize.
	return DX::textureSize(comObject_, ResourceCounters::MT_GPU) * 6;
}

/**
 *	This is the specialisation of pool for textures.
 */
template<> inline uint ComObjectWrap<DX::BaseTexture>::pool() const
{
	// We don't want to use this specialisation.
	assert(0);
	return 0;
}

/**
 *	This is the specialisation of resourceType for textures.
 */
template<> inline ResourceCounters::ResourceType ComObjectWrap<DX::BaseTexture>::resourceType() const
{
	return ResourceCounters::RT_TEXTURE;
}

/**
 *	This is the specialisation of size for textures.
 */
template<> inline uint ComObjectWrap<DX::BaseTexture>::size() const
{
	// We don't want to use this specialisation.
	assert(0);
	return 0;
}

/**
 *	This is the specialisation of size for textures.
 */
template<> inline uint ComObjectWrap<DX::BaseTexture>::gpuMemorySize() const
{
	// We don't want to use this specialisation.
	assert(0);
	return 0;
}

#endif // ENABLE_RESOURCE_COUNTERS


#endif // COM_OBJECT_WRAP_IPP
