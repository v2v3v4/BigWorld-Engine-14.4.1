#ifndef EFFECTPARAMETERCACHE_HPP
#define EFFECTPARAMETERCACHE_HPP


BW_BEGIN_NAMESPACE

namespace Moo
{
	class EffectMaterial;
};

#define DECLARE_TYPE_SETTER( X, Y )								\
	HRESULT set##X( const BW::string& name, Y v )				\
	{															\
		return spEffect_->Set##X( parameter(name), v );			\
	}															\


/**
 *	This class is a simple helper for setting manual parameters
 *	on effects.  It replaces the D3DXEffect->SetTYPE( paramName )
 *	which is not recommended for runtime use due to perforamance
 *	of the handle lookup, with a simple cache that allows you
 *	to do exactly the same thing.
 */
class EffectParameterCache
{
public:	
	EffectParameterCache():
	  spEffect_(NULL)
	{
	}

	~EffectParameterCache();

	void clear()
	{
		parameters_.clear();
	}

	bool hasEffect() const
	{
		return ( spEffect_.pComObject() != NULL );
	}

	void effect( ComObjectWrap<ID3DXEffect> spEffect )
	{
		if (spEffect.pComObject() != spEffect_.pComObject())
		{
			spEffect_ = spEffect;
			this->clear();
		}
	}

	void commitChanges()
	{
		spEffect_->CommitChanges();
	}

	DECLARE_TYPE_SETTER( Matrix, const Matrix* )
	DECLARE_TYPE_SETTER( Texture, DX::BaseTexture* )
	DECLARE_TYPE_SETTER( Vector, const Vector4* )
	DECLARE_TYPE_SETTER( Float, float )
	DECLARE_TYPE_SETTER( Bool, bool )

	HRESULT setVectorArray( const BW::string& name, const Vector4* v, size_t n )
	{
		MF_ASSERT( n <= UINT_MAX );
		return spEffect_->SetVectorArray( parameter(name), v, ( UINT ) n );
	}	

	HRESULT setFloatArray( const BW::string& name, const float *af, size_t n )
	{
		MF_ASSERT( n <= UINT_MAX );
		return spEffect_->SetFloatArray( parameter(name), af, ( UINT ) n );
	}

private:
	void cache(const BW::string& name );
	D3DXHANDLE parameter( const BW::string& key )
	{
		if (parameters_.find(key) == parameters_.end())
		{
			this->cache(key);
		}
		return parameters_[key];
	}
	BW::map<BW::string, D3DXHANDLE> parameters_;
	ComObjectWrap<ID3DXEffect> spEffect_;
};

BW_END_NAMESPACE

#endif //EFFECTPARAMETERCACHE_HPP
