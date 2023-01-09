#include "pch.hpp"
#include "light_scene_provider.hpp"
#include "string_table.hpp"

#include "scene/scene.hpp"
#include "scene/scene_object.hpp"
#include "space/client_space.hpp"

#include "cstdmf/string_builder.hpp"
#include "cstdmf/profiler.hpp"
#include "math/convex_hull.hpp"

#include "moo/light_container.hpp"
#include "moo/omni_light.hpp"
#include "moo/spot_light.hpp"
#include "moo/pulse_light.hpp"

#include "moo/debug_draw.hpp"

namespace BW {
namespace CompiledSpace {

typedef ExternalArray<LightSceneTypes::OmniLight> OmniLightExternalArray;
typedef ExternalArray<LightSceneTypes::SpotLight> SpotLightExternalArray;
typedef ExternalArray<LightSceneTypes::PulseLight> PulseLightExternalArray;

namespace {

void convertSerialisedToMooOmniLightPtrs(
	const OmniLightExternalArray & externalData,
	const Matrix & transform,
	LightSceneProvider::MooOmniLightPtrArray & mooLightArray )
{
	OmniLightExternalArray::const_iterator iter = externalData.begin(),
	                                       end = externalData.end();
	for (; iter != end; ++iter)
	{
		LightSceneTypes::OmniLight omniLight = *iter;
		omniLight.transform(transform);
		Moo::OmniLightPtr mooLight(
			new Moo::OmniLight( omniLight.colour,
			                    omniLight.position,
			                    omniLight.innerRadius,
			                    omniLight.outerRadius ) );
		mooLightArray.push_back( mooLight );
	}
}

void convertSerialisedToMooSpotLightPtrs(
	const SpotLightExternalArray & externalData,
	const Matrix & transform,
	LightSceneProvider::MooSpotLightPtrArray & mooLightArray )
{
	SpotLightExternalArray::const_iterator iter = externalData.begin(),
	                                       end = externalData.end();
	for (; iter != end; ++iter)
	{
		LightSceneTypes::SpotLight spotLight = *iter;
		spotLight.transform(transform);
		Moo::SpotLightPtr mooLight(
			new Moo::SpotLight( spotLight.colour,
			                    spotLight.position,
			                    spotLight.direction,
			                    spotLight.innerRadius,
			                    spotLight.outerRadius,
			                    spotLight.cosConeAngle ) );
		mooLightArray.push_back( mooLight );
	}
}

void convertSerialisedToMooPulseLightPtrs(
	const PulseLightExternalArray & externalData,
	const Matrix & transform,
	ExternalArray< Vector2 > & pulseLightsAnimFrames, const StringTable & strings,
	LightSceneProvider::MooPulseLightPtrArray & mooLightArray )
{
	PulseLightExternalArray::const_iterator iter = externalData.begin(),
	                                        end = externalData.end();
	for (; iter != end; ++iter)
	{
		LightSceneTypes::PulseLight pulseLight = *iter;
		pulseLight.transform(transform);
		const LightSceneTypes::OmniLight & omniLight = pulseLight.omniLight;
		Vector2 * pFirstFrame = &pulseLightsAnimFrames[0] + pulseLight.firstFrameIdx;
		const ExternalArray<Vector2> animFrames( pFirstFrame,
			pulseLight.numFrames, pulseLight.numFrames );
		Moo::PulseLightPtr mooLight(
			new Moo::PulseLight( omniLight.colour,
			                     omniLight.position,
			                     omniLight.innerRadius,
			                     omniLight.outerRadius,
			                     strings.entry(pulseLight.animationNameIdx),
			                     animFrames, pulseLight.duration ) );
		mooLightArray.push_back( mooLight );
	}
}

}
		
LightSceneProvider::LightSceneProvider() :
	pReader_( NULL ),
	pStream_( NULL )
{

}


LightSceneProvider::~LightSceneProvider()
{

}


bool LightSceneProvider::doLoadFromSpace( ClientSpace * pSpace,
                                          BinaryFormat& reader, 
                                          const DataSectionPtr& pSpaceSettings,
                                          const Matrix& transform, 
                                          const StringTable& strings )
{
	PROFILER_SCOPED( LightSceneProvider_doLoadFromSpace );

	MF_ASSERT( pReader_ == NULL );
	pReader_ = &reader;
	
	pStream_ = pReader_->findAndOpenSection(
		LightSceneTypes::FORMAT_MAGIC,
		LightSceneTypes::FORMAT_VERSION,
		"LightSceneProvider"  );
	if (!pStream_)
	{
		ASSET_MSG( "Failed to map '%s' into memory.\n",
			pReader_->resourceID() );
		this->unload();
		return false;
	}

	OmniLightExternalArray omniLightsData;
	SpotLightExternalArray spotLightsData;
	PulseLightExternalArray pulseLightsData;
	ExternalArray<Vector2> pulseLightsAnimFrames;
	if (!pStream_->read( omniLightsData ) ||
		!pStream_->read( spotLightsData ) ||
		!pStream_->read( pulseLightsData ) ||
		!pStream_->read( pulseLightsAnimFrames )
		)
	{
		ASSET_MSG( "LightSceneProvider in '%s' has incomplete data.\n",
			pReader_->resourceID() );
		this->unload();
		return false;
	}

	{
		PROFILER_SCOPED( LightSceneProvider_doLoadFromSpace_Load );

		convertSerialisedToMooOmniLightPtrs( omniLightsData, transform, omniLights_ );
		convertSerialisedToMooSpotLightPtrs( spotLightsData, transform, spotLights_ );
		convertSerialisedToMooPulseLightPtrs( pulseLightsData, transform,
			pulseLightsAnimFrames, strings, pulseLights_ );
	}
	
	return true;
}


bool LightSceneProvider::doBind()
{
	if (isValid())
	{
		space().scene().addProvider( this );
	}

	return true;
}


void LightSceneProvider::doUnload()
{
	space().scene().removeProvider( this );

	if (pStream_ && pReader_)
	{
		pReader_->closeSection( pStream_ );
	}

	pReader_ = NULL;
	pStream_ = NULL;
}


float LightSceneProvider::percentLoaded() const
{
	return 1.0f;
}

bool LightSceneProvider::isValid() const
{
	return pReader_ && pStream_;
}

void * LightSceneProvider::getView(
	const SceneTypeSystem::RuntimeTypeID & viewTypeID )
{
	void * result = NULL;

	exposeView<ILightSceneViewProvider>( this, viewTypeID, result );
	exposeView<ITickSceneProvider>( this, viewTypeID, result );

	return result;
}


void LightSceneProvider::forEachObject(
	const ConstSceneObjectCallback & function ) const
{

}


void LightSceneProvider::forEachObject(
	const SceneObjectCallback & function )
{

}

namespace {
	// ConvexHull intersections.
	// NOTE: We should eventually do a full specialisation for SpotLight types
	// since this default intersection is way too conservative.
	template<typename LightType>
	bool lightAndObjectIntersect(const ConvexHull & hull, const LightType & light)
	{
		return hull.intersects(light.position(), light.outerRadius());
	}

	// BBox intersections.
	// NOTE: We should eventually do a full specialisation for SpotLight types
	// since this default intersection is way too conservative.
	template<typename LightType>
	bool lightAndObjectIntersect(const AABB & bbox, const LightType & light)
	{
		return (bbox.distance(light.position()) <= light.outerRadius());
	}

	void addLightToContainer( const Moo::OmniLightPtr & pLight,
		Moo::LightContainer & lightContainer )
	{
		lightContainer.addOmni( pLight );
	}

	void addLightToContainer( const Moo::SpotLightPtr & pLight,
		Moo::LightContainer & lightContainer )
	{
		lightContainer.addSpot( pLight );
	}

	void addLightToContainer( const Moo::PulseLightPtr & pLight,
		Moo::LightContainer & lightContainer )
	{
		const Moo::OmniLightPtr pOmniLight = pLight;
		lightContainer.addOmni( pOmniLight );
	}

	template <typename IntersectingObjectType, typename VectorT>
	size_t addIntersectingLights( const IntersectingObjectType & obj,
		const VectorT & lights, Moo::LightContainer & lightContainer )
	{
		size_t numLightsAdded = 0;
		typename VectorT::const_iterator iter = lights.begin(),
		                                 end = lights.end();
		for (; iter != end; ++iter)
		{
			const typename VectorT::value_type & pLight = *iter;
			if (lightAndObjectIntersect( (IntersectingObjectType)obj, *pLight ))
			{
				addLightToContainer( pLight, lightContainer );
				++numLightsAdded;
			}
		}
		return numLightsAdded;
	}
}

size_t LightSceneProvider::intersect( const ConvexHull & hull,
	Moo::LightContainer & lightContainer ) const
{
	size_t numLightsAdded = 0;

	numLightsAdded += addIntersectingLights(hull, omniLights_, lightContainer);
	numLightsAdded += addIntersectingLights(hull, spotLights_, lightContainer);
	numLightsAdded += addIntersectingLights(hull, pulseLights_, lightContainer);

	return numLightsAdded;
}

size_t LightSceneProvider::intersect( const AABB & bbox,
	Moo::LightContainer & lightContainer ) const
{
	size_t numLightsAdded = 0;

	numLightsAdded += addIntersectingLights(bbox, omniLights_, lightContainer);
	numLightsAdded += addIntersectingLights(bbox, spotLights_, lightContainer);
	numLightsAdded += addIntersectingLights(bbox, pulseLights_, lightContainer);

	return numLightsAdded;
}

namespace {
	template<typename LightType>
	AABB lightToBBox(const LightType & light)
	{
		AABB bb;
		bb.addBounds(light.position());
		const float outerRadius = light.outerRadius();
		bb.expandSymmetrically(outerRadius, outerRadius, outerRadius);
		return bb;
	}

	template<typename LightArrayType>
	void drawLightArray(const LightArrayType & lightArray, uint32 col)
	{
		typename LightArrayType::const_iterator iter = lightArray.begin(),
		                                        end = lightArray.end();
		for (; iter != end; ++iter)
		{
			DebugDraw::bboxAdd( lightToBBox(**iter), col );
		}
	}
}

void LightSceneProvider::debugDrawLights() const
{
	drawLightArray(omniLights_, Moo::PackedColours::RED);
	drawLightArray(spotLights_, Moo::PackedColours::GREEN);
	drawLightArray(pulseLights_, Moo::PackedColours::MAGENTA);
}

//static
void LightSceneProvider::registerHandlers( Scene & scene )
{
}

void LightSceneProvider::tick( float dTime )
{
	MooPulseLightPtrArray::iterator iter = pulseLights_.begin(),
	                                end = pulseLights_.end();
	for (; iter != end; ++iter)
	{
		Moo::PulseLightPtr & pPulseLight = *iter;
		pPulseLight->tick( dTime );
	}
}

void LightSceneProvider::updateAnimations( float dTime )
{

}

} // namespace CompiledSpace
} // namespace BW
