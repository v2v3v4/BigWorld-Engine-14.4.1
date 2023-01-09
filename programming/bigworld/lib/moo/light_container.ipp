
#ifdef CODE_INLINE
#define INLINE inline
#else
#define INLINE
#endif

namespace Moo
{
	INLINE
	const Colour& LightContainer::ambientColour( ) const
	{
		return ambientColour_;
	}

	INLINE 
	void LightContainer::ambientColour( const Colour& colour )
	{
		ambientColour_ = colour;
	}

	INLINE 
	const DirectionalLightVector& LightContainer::directionals( ) const
	{
		return directionalLights_;
	}

	INLINE
	DirectionalLightVector& LightContainer::directionals( )
	{
		return directionalLights_;
	}

	INLINE
	bool LightContainer::addDirectional( const DirectionalLightPtr & pDirectional, bool checkExisting )
	{
		BW_GUARD;
		MF_ASSERT_DEV( pDirectional );
		
		if( !pDirectional )
			return false;
		
		if (checkExisting)
		{
			if (std::find( directionalLights_.begin(), directionalLights_.end(), pDirectional ) == directionalLights_.end())
			{
				directionalLights_.push_back( pDirectional );
				return true;
			}
			
			return false;
		}
		else
		{
			directionalLights_.push_back( pDirectional );
			return true;
		}
	}
	
	INLINE
	bool LightContainer::delDirectional( const DirectionalLightPtr & pDirectional )
	{
		BW_GUARD;
		MF_ASSERT_DEV( pDirectional );
		
		DirectionalLightVector::iterator it = 
			std::find( directionalLights_.begin(), directionalLights_.end(), pDirectional );
			
		if (it != directionalLights_.end())
		{
			directionalLights_.erase( it );
			return true;
		}
		else
		{
			return false;
		}
	}	

	INLINE
	uint32 LightContainer::nDirectionals( ) const
	{
		MF_ASSERT( directionalLights_.size() <= UINT_MAX );
		return ( uint32 ) directionalLights_.size();
	}

	INLINE
	const DirectionalLightPtr& LightContainer::directional( uint32 i ) const
	{
		BW_GUARD;
		IF_NOT_MF_ASSERT_DEV( directionalLights_.size() > i )
		{
			static DirectionalLightPtr nullPtr(NULL);
			return nullPtr;
		}
		return directionalLights_[ i ];
	}

	INLINE
	const SpotLightVector& LightContainer::spots( ) const
	{
		return spotLights_;
	}

	INLINE
	SpotLightVector& LightContainer::spots( )
	{
		return spotLights_;
	}

	INLINE
	bool LightContainer::addSpot( const SpotLightPtr & pSpot, bool checkExisting )
	{
		BW_GUARD;
		MF_ASSERT_DEV( pSpot );
		
		if( !pSpot )
			return false;
		
		if (checkExisting)
		{
			if (std::find( spotLights_.begin(), spotLights_.end(), pSpot ) == spotLights_.end())
			{
				spotLights_.push_back( pSpot );
				return true;
			}
			
			return false;
		}
		else
		{
			spotLights_.push_back( pSpot );
			return true;
		}
	}
	
	INLINE
	bool LightContainer::delSpot( const SpotLightPtr & pSpot )
	{
		BW_GUARD;
		MF_ASSERT_DEV( pSpot );
		
		SpotLightVector::iterator it = 
			std::find( spotLights_.begin(), spotLights_.end(), pSpot );
			
		if (it != spotLights_.end())
		{
			spotLights_.erase( it );
			return true;
		}
		else
		{
			return false;
		}
	}

	INLINE
	uint32 LightContainer::nSpots( ) const
	{
		return static_cast<uint32>( spotLights_.size() );
	}

	INLINE
	const SpotLightPtr& LightContainer::spot( uint32 i ) const
	{
		IF_NOT_MF_ASSERT_DEV( spotLights_.size() > i )
		{
			static SpotLightPtr nullPtr(NULL);
			return nullPtr;
		}
		return spotLights_[ i ];
	}

	INLINE
	const OmniLightVector& LightContainer::omnis( ) const
	{
		return omniLights_;
	}

	INLINE
	OmniLightVector& LightContainer::omnis( )
	{
		return omniLights_;
	}

	INLINE
	bool LightContainer::addOmni( const OmniLightPtr & pOmni, bool checkExisting )
	{
		BW_GUARD;
		MF_ASSERT_DEV( pOmni );
		
		if( !pOmni )
			return false;
		
		if (checkExisting)
		{
			if (std::find( omniLights_.begin(), omniLights_.end(), pOmni ) == omniLights_.end())
			{
				omniLights_.push_back( pOmni );
				return true;
			}
			
			return false;
		}
		else
		{
			omniLights_.push_back( pOmni );
			return true;
		}
	}
	
	INLINE
	bool LightContainer::delOmni( const OmniLightPtr & pOmni )
	{
		BW_GUARD;
		MF_ASSERT_DEV( pOmni );
		
		OmniLightVector::iterator it = 
			std::find( omniLights_.begin(), omniLights_.end(), pOmni );
			
		if (it != omniLights_.end())
		{
			omniLights_.erase( it );
			return true;
		}
		else
		{
			return false;
		}
	}

	INLINE
	uint32 LightContainer::nOmnis( ) const
	{
		return static_cast<uint32>( omniLights_.size() );
	}

	INLINE
	const OmniLightPtr& LightContainer::omni( uint32 i ) const
	{
		BW_GUARD;
		IF_NOT_MF_ASSERT_DEV( omniLights_.size() > i )
		{
			static OmniLightPtr nullPtr(NULL);
			return nullPtr;
		}
		return omniLights_[ i ];
	}
	
	INLINE
	bool LightContainer::addLight( const OmniLightPtr & pOmni, bool checkExisting )
	{
		return this->addOmni( pOmni, checkExisting );
	}
	
	INLINE
	bool LightContainer::delLight( const OmniLightPtr & pOmni )
	{
		return this->delOmni( pOmni );
	}
	
	INLINE
	bool LightContainer::addLight( const SpotLightPtr & pSpot, bool checkExisting )
	{
		return this->addSpot( pSpot, checkExisting );
	}
	
	INLINE
	bool LightContainer::delLight( const SpotLightPtr & pSpot )
	{
		return this->delSpot( pSpot );
	}
	
	INLINE
	bool LightContainer::addLight( const DirectionalLightPtr & pDir, bool checkExisting )
	{
		return this->addDirectional( pDir, checkExisting );
	}
	
	INLINE
	bool LightContainer::delLight( const DirectionalLightPtr & pDir )
	{
		return this->delDirectional( pDir );
	}
}

/*light_container.ipp*/
