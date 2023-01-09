#include "pch.hpp"
#include "py_filter_quad.hpp"
#include "moo/render_context.hpp"
#include "moo/base_texture.hpp"
#include "moo/texture_manager.hpp"

#ifdef EDITOR_ENABLED
#include "gizmo/general_editor.hpp"
#include "gizmo/general_properties.hpp"
#include "resmgr/string_provider.hpp"
#include "../../tools/common/material_properties.hpp"
#endif //EDITOR_ENABLED

DECLARE_DEBUG_COMPONENT2( "PostProcessing", 0 )


BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "py_filter_quad.ipp"
#endif

namespace PostProcessing
{

// Python statics
PY_TYPEOBJECT( PyFilterQuad )

PY_BEGIN_METHODS( PyFilterQuad )	
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( PyFilterQuad )
	/*~ attribute PyFilterQuad.samples
	 *	@components{ client, tools }
	 *	Sample coordinates for sampling the source texture.  The samples are
	 *	all divided by the dimensions of the source texture.  This allows you
	 *	to specify the samples in texels, and this class will automatically
	 *	convert them into uv coordinates.  The source texture defaults to
	 *	the variable named "inputTexture" but can be changed in the shader
	 *	via the "filterQuadTexture" global string property.
	 *	Each sample is a Vector3, or tuple, (u offset, v offset, weight).
	 *	@type List of Vector3s.
	 */
	PY_ATTRIBUTE( samples )
PY_END_ATTRIBUTES()

/*~ class PostProcessing.PyFilterQuad
 *	@components{ client, tools }
 *	A PyFilterQuad can be used by a PyPhase object, and provides the geometry
 *	for the phase.  The FilterQuad performs n-taps on the source material's
 *	textures, in groups of 4.  If you specify more than 4 taps, then the filter
 *	quad will draw using n/4 passes.  For these operations, you should make
 *	sure the material you use is additive, so the results can accumulate.
 */
/*~ function PostProcessing.FilterQuad
 *	@components{ client, tools }
 *	Factory function to create and return a PostProcessing PyFilterQuad object.
 *	@return A new PostProcessing PyFilterQuad object.
 */
PY_FACTORY_NAMED( PyFilterQuad, "FilterQuad", _PostProcessing )

IMPLEMENT_FILTER_QUAD( PyFilterQuad, PyFilterQuad )


PyFilterQuad::PyFilterQuad( PyTypeObject *pType ) :
	FilterQuad( pType ),
	samplesHolder_( samples_, this, true ),
	pDecl4tap_(),
	pDecl8tap_()
#ifdef EDITOR_ENABLED
	, pCallback_( NULL )
#endif // EDITOR_ENABLED
{
}


PyFilterQuad::~PyFilterQuad()
{
}


/**
 *	This method adds a single vertex to the filter quad.
 *	@param	v		XYZNUV2 source vertex
 *	@param	rSample	pointer to tap offsets for the vertex
 *	@param	tapSize	number of uv coordinate taps.
 */
void PyFilterQuad::addVert( Moo::VertexXYZNUV2& v, FilterSample* rSample, size_t tapSize, const Vector2& srcDim )
{
	Moo::FourTapVertex curr;
	curr.pos_ = v.pos_;
	for (size_t t=0; t<tapSize; t++)
	{
		FilterSample& fs = rSample[t];
		curr.uv_[t][0] = v.uv2_[0] + (fs[0]/srcDim[0]);
		curr.uv_[t][1] = v.uv2_[1] + (fs[1]/srcDim[1]);
		curr.uv_[t][2] = fs[2];
	}

	Matrix view = Moo::rc().invView();
	view.translation(Vector3(0,0,0));
	curr.viewNormal_ = Moo::rc().camera().nearPlanePoint( curr.pos_.x, curr.pos_.y );
	curr.viewNormal_.normalise();		
	curr.worldNormal_ = view.applyVector( curr.viewNormal_ );

	verts4tap_.push_back(curr);
}


/**
 *	This method is called when the material has been set, and
 *	draw is about to be called.  For PyFilterQuad, we build
 *	our mesh and ascertain which texture map is used to derive
 *	the filter tap coordinates.
 */
void PyFilterQuad::preDraw( Moo::EffectMaterialPtr pMat )
{
	pDecl4tap_ = Moo::VertexDeclaration::get( "xyzuvw4" );
	pDecl8tap_ = Moo::VertexDeclaration::get( "xyzuvw8" );

	float w = 1.f;
	float h = 1.f;
	Vector3 fixup( -0.5f / Moo::rc().halfScreenWidth(), 0.5f / Moo::rc().halfScreenHeight(), 0.f );

	Moo::VertexXYZNUV2 v[4];

	v[0].pos_.set(-1.f,-1.f,0.1f);
	v[0].uv_.set(0.f,0.f);
	v[0].uv2_.set(0.f,h);

	v[1].pos_.set(-1.f,1.f,0.1f);
	v[1].uv_.set(0.f,1.f);
	v[1].uv2_.set(0.f,0.f);	

	v[2].pos_.set(1.f,1.f,0.1f);
	v[2].uv_.set(1.f,1.f);
	v[2].uv2_.set(w,0.f);	

	v[3].pos_.set(1.f,-1.f,0.1f);
	v[3].uv_.set(1.f,0.f);
	v[3].uv2_.set(w,h);

	for (uint32 i=0; i<4; i++)
	{
		v[i].pos_ = v[i].pos_ + fixup;
	}

	// Check the source dimensions for filter taps.
	// We assume the .fx file has its main texture named 'inputTexture',
	// however those wishing to break this convention can specify a
	// string global variable "filterQuadTexture" that names the
	// texture variable to be used.
	//
	// This allows us to specify a filter tap in pixels rather than uvs.
	Vector2 srcDim(1.f,1.f);
	const char * pPropName = "inputTexture";
	D3DXHANDLE handle = pMat->pEffect()->pEffect()->GetParameterByName( NULL, "filterQuadTexture" );
	if ( handle )
	{
		pMat->pEffect()->pEffect()->GetString( handle, &pPropName );
	}

	Moo::ConstEffectPropertyPtr pTexProp = pMat->getProperty(pPropName);
	if ( pTexProp )
	{
		BW::string srcTextureName;
		pTexProp->getResourceID(srcTextureName);
		Moo::BaseTexturePtr pTex = Moo::TextureManager::instance()->get(
			srcTextureName, true, false, false );
		if ( pTex )
		{
			srcDim.x = (float)pTex->width();
			srcDim.y = (float)pTex->height();
		}
	}

	//Now we have our 4 vertices that give us a full-screen quad,
	//create uvs from the provided filter kernel.
	verts4tap_.clear();
	uint32 tapSize = 4;

	for (size_t s=0; s<samples_.size(); s+=tapSize)
	{
		this->addVert( v[0], &samples_[s], tapSize, srcDim );
		this->addVert( v[1], &samples_[s], tapSize, srcDim );
		this->addVert( v[2], &samples_[s], tapSize, srcDim );
		this->addVert( v[0], &samples_[s], tapSize, srcDim );
		this->addVert( v[2], &samples_[s], tapSize, srcDim );
		this->addVert( v[3], &samples_[s], tapSize, srcDim );
	}
};


/**
 *	This method draws the filter quad using the current device state.
 */
void PyFilterQuad::draw()
{
	verts4tap_.drawEffect( pDecl4tap_ );
}


bool PyFilterQuad::save( DataSectionPtr pDS )
{
	DataSectionPtr pSect = pDS->newSection( "PyFilterQuad" );
	BW::vector<Vector3>::iterator it = samples_.begin();
	for ( ; it != samples_.end(); it++ )
	{
		Vector3 val = *it;
		DataSectionPtr pTapSect = pSect->newSection( "filterTap" );
		pTapSect->setVector3(val);
	}
	return true;
}


bool PyFilterQuad::load( DataSectionPtr pSect )
{
	BW::vector<DataSectionPtr> samples;
	pSect->openSections( "filterTap", samples );
	BW::vector<DataSectionPtr>::iterator it = samples.begin();
	samples_.clear();
	for ( ; it != samples.end(); it++ )
	{
		DataSectionPtr pChildSect = *it;
		samples_.push_back( pChildSect->asVector3() );
	}
	return true;
}



#ifdef EDITOR_ENABLED

namespace
{


/**
 *	This class is used for undoing sample editing
 */
class SampleUndo : public UndoRedo::Operation
{
public:
	SampleUndo( PyFilterQuad & filterQuad )
		: filterQuad_( filterQuad ), samples_( filterQuad.getSamples() ),
		UndoRedo::Operation( (size_t)(typeid(PyFilterQuad).name()) )
	{
	}

private:
	PyFilterQuad & filterQuad_;
	PyFilterQuad::Samples samples_;

	virtual void undo()
	{
		UndoRedo::instance().add( new SampleUndo( filterQuad_ ) );

		PropertyModifyGuard propertyModifyGuard;

		filterQuad_.setSamples( samples_ );
	}

	virtual bool iseq( const UndoRedo::Operation & oth ) const
	{		
		return false;
	}
};


/**
 *	This class is used for the editing of one sample of a filter quad.
 */
class SampleProxy : public Vector3Proxy
{
public:
	SampleProxy( Vector3 & value, PyFilterQuad & filterQuad ) :
		value_( value ), transient_( value ), filterQuad_( filterQuad )
	{
	}

	virtual Vector3 get() const
	{
		return transient_;
	}

	virtual void set( Vector3 value, bool transient, bool addBarrier )
	{
		transient_ = value;

		if (!transient)
		{
			UndoRedo::instance().add( new SampleUndo( filterQuad_ ) );

			if (addBarrier)
			{
				UndoRedo::instance().barrier(
					LocaliseUTF8( L"WORLDEDITOR/GUI/PAGE_POST_PROCESSING/PY_FILTERQUAD/MODIFY_FILTERQUAD" ),
					false );
			}

			value_ = value;
			filterQuad_.onSampleChanged();
		}
	}

private:
	Vector3 & value_;
	Vector3 transient_;
	PyFilterQuad & filterQuad_;
};


/**
 *	This class is used for managing the samples array of a filter quad.
 */
class SamplesArrayProxy : public ArrayProxy
{
public:
	SamplesArrayProxy( PyFilterQuad& filterQuad ) :
		filterQuad_( filterQuad )
	{
	}

	virtual void elect( GeneralProperty * parent )
	{
		PyFilterQuad::Samples & samples = filterQuad_.getSamples();

		static const int MAX_PROPNAME = 256;
		char propName[ MAX_PROPNAME ];
		int idx = 1;
		for (PyFilterQuad::Samples::iterator it = samples.begin();
			it != samples.end(); ++it)
		{
			bw_snprintf( propName, MAX_PROPNAME, "sample%03d", idx++ );
			properties_.push_back(
				new Vector3Property( Name( propName ),
					new SampleProxy( *it, filterQuad_ ) ) );
			properties_.back()->setGroup(
				parent->name()[0] ? parent->getGroup().str() + parent->name().c_str() :
				parent->getGroup() );
			properties_.back()->elect();
		}
	}

	virtual void expel( GeneralProperty* parent )
	{
		for (Properties::iterator iter = properties_.begin();
			iter != properties_.end(); ++iter)
		{
			(*iter)->expel();
			(*iter)->deleteSelf();
		}

		properties_.clear();
	}

	virtual void select( GeneralProperty* )
	{
	}

	virtual bool addItem()
	{
		PropertyModifyGuard propertyModifyGuard;
		PyFilterQuad::Samples & samples = filterQuad_.getSamples();

		UndoRedo::instance().add( new SampleUndo( filterQuad_ ) );

		UndoRedo::instance().barrier(
			LocaliseUTF8( L"WORLDEDITOR/GUI/PAGE_POST_PROCESSING/PY_FILTERQUAD/MODIFY_FILTERQUAD" ),
			false );

		if (samples.empty())
		{
			samples.push_back( Vector3( 0.f, 0.f, 0.0f ) );
		}
		else
		{
			samples.push_back( *samples.rbegin() );
		}

		filterQuad_.onSampleChanged();

		return true;
	}

	virtual void delItem( int index )
	{
		PropertyModifyGuard propertyModifyGuard;
		PyFilterQuad::Samples & samples = filterQuad_.getSamples();

		if (index >= 0 && index < (int)samples.size())
		{
			UndoRedo::instance().add( new SampleUndo( filterQuad_ ) );
			UndoRedo::instance().barrier(
				LocaliseUTF8( L"WORLDEDITOR/GUI/PAGE_POST_PROCESSING/PY_FILTERQUAD/MODIFY_FILTERQUAD" ),
				false );
			samples.erase( samples.begin() + index );
			filterQuad_.onSampleChanged();
		}
	}

	virtual bool delItems()
	{
		PropertyModifyGuard propertyModifyGuard;
		PyFilterQuad::Samples & samples = filterQuad_.getSamples();

		if (!samples.empty())
		{
			UndoRedo::instance().add( new SampleUndo( filterQuad_ ) );
			UndoRedo::instance().barrier(
				LocaliseUTF8( L"WORLDEDITOR/GUI/PAGE_POST_PROCESSING/PY_FILTERQUAD/MODIFY_FILTERQUAD" ),
				false );
			samples.clear();
			filterQuad_.onSampleChanged();
		}

		return true;
	}

private:
	typedef BW::vector< GeneralProperty * > Properties;
	Properties properties_;
	PyFilterQuad & filterQuad_;
};


} // anonymous namespace


void PyFilterQuad::edEdit( GeneralEditor * editor, FilterChangeCallback pCallback )
{
	pCallback_ = pCallback;

	STATIC_LOCALISE_NAME( s_samples, "samples" );

	editor->addProperty( new ArrayProperty( s_samples,
							new SamplesArrayProxy( *this ) ) );

}


PyFilterQuad::Samples & PyFilterQuad::getSamples()
{
	return samples_;
}


void PyFilterQuad::setSamples( const Samples & samples )
{
	samples_ = samples;

	onSampleChanged();
}


void PyFilterQuad::onSampleChanged()
{
	if (pCallback_)
	{
		(*pCallback_)( true );
	}
}


#endif // EDITOR_ENABLED

PyObject* PyFilterQuad::pyNew(PyObject* args)
{
	return new PyFilterQuad;
}

}

BW_END_NAMESPACE

// py_filter_quad.cpp
