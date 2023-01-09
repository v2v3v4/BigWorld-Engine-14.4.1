#include "pch.hpp"

#include "light_scene_writer.hpp"
#include "string_table_writer.hpp"
#include "chunk_converter.hpp"
#include "binary_format_writer.hpp"

#include "resmgr/datasection.hpp"

#include "chunk/chunk_light.hpp"
#include <moo/omni_light.hpp>
#include <moo/spot_light.hpp>

namespace BW {
namespace CompiledSpace {

namespace {

LightSceneTypes::OmniLight convertMooOmniLight( const Moo::OmniLight & mooLight )
{
	LightSceneTypes::OmniLight light;
	light.position = mooLight.worldPosition();
	light.innerRadius = mooLight.worldInnerRadius();
	light.outerRadius = mooLight.worldOuterRadius();
	light.colour = mooLight.colour();
	return light;
}

LightSceneTypes::SpotLight convertMooSpotLight( const Moo::SpotLight & mooLight )
{
	float dirLength = mooLight.worldDirection().length();
	MF_ASSERT((dirLength < (1.0f + 1e-4)) && dirLength > (1.0f - 1e-4));
	LightSceneTypes::SpotLight light;
	light.position = mooLight.worldPosition();
	light.direction = mooLight.worldDirection();
	light.innerRadius = mooLight.worldInnerRadius();
	light.outerRadius = mooLight.worldOuterRadius();
	light.cosConeAngle = mooLight.cosConeAngle();
	light.colour = mooLight.colour();
	return light;
}

template <class LightType>
void accumulatePositions( const BW::vector<LightType>& lights, AABB* bb )
{
	typedef BW::vector<LightType> LightVector;
	for (LightVector::const_iterator iter = lights.begin();
		iter != lights.end(); ++iter)
	{
		const LightType& light = *iter;
		bb->addBounds( light.position );
	}
}

template <>
void accumulatePositions<LightSceneTypes::PulseLight>( 
	const BW::vector<LightSceneTypes::PulseLight>& lights, AABB* bb )
{
	typedef LightSceneTypes::PulseLight LightType;
	typedef BW::vector<LightType> LightVector;
	for (LightVector::const_iterator iter = lights.begin();
		iter != lights.end(); ++iter)
	{
		const LightType& light = *iter;
		bb->addBounds( light.omniLight.position );
	}
}

}

LightSceneWriter::LightSceneWriter()
{
}

LightSceneWriter::~LightSceneWriter()
{
}

bool LightSceneWriter::initialize( const DataSectionPtr& pSpaceSettings,
	const CommandLine& commandLine )
{
	return true;
}

bool LightSceneWriter::write( BinaryFormatWriter& writer )
{
	BinaryFormatWriter::Stream * stream = writer.appendSection(
		LightSceneTypes::FORMAT_MAGIC,
		LightSceneTypes::FORMAT_VERSION );
	MF_ASSERT( stream != NULL );

	stream->write( omniLights );
	stream->write( spotLights );
	stream->write( pulseLights );
	stream->write( pulseLightsAnimFrames );

	return true;
}

void LightSceneWriter::convertOmniLight( const ConversionContext& ctx,
	const DataSectionPtr& pItemDS, const BW::string& uid )
{
	addOmniLight( pItemDS, ctx.chunkTransform );
}

void LightSceneWriter::convertSpotLight( const ConversionContext& ctx,
	const DataSectionPtr& pItemDS, const BW::string& uid )
{
	addSpotLight( pItemDS, ctx.chunkTransform );
}

void LightSceneWriter::convertPulseLight( const ConversionContext& ctx,
	const DataSectionPtr& pItemDS, const BW::string& uid )
{
	addPulseLight( pItemDS, ctx.chunkTransform, strings() );
}

void LightSceneWriter::addOmniLight( const DataSectionPtr & pItemDS,
		const Matrix & worldTransform )
{
	Moo::OmniLight light;
	ChunkOmniLight::load( light, pItemDS );
	light.worldTransform( worldTransform );
	omniLights.push_back(convertMooOmniLight(light));
}

void LightSceneWriter::addSpotLight( const DataSectionPtr & pItemDS,
		const Matrix & worldTransform )
{
	Moo::SpotLight light;
	ChunkSpotLight::load( light, pItemDS );
	light.worldTransform( worldTransform );
	spotLights.push_back(convertMooSpotLight(light));
}

void LightSceneWriter::addPulseLight( const DataSectionPtr & pItemDS,
		const Matrix & worldTransform , StringTableWriter & stringTable_ )
{
	Moo::OmniLight light;
	ChunkOmniLight::load( light, pItemDS );
	light.worldTransform( worldTransform );
	LightSceneTypes::PulseLight pulseLight;
	pulseLight.omniLight = convertMooOmniLight(light);
	// Now grab all the animation information from the DataSection
	BW::string animation = pItemDS->readString( "animation",
		ChunkPulseLight::defaultAnimation );
	pulseLight.animationNameIdx = stringTable_.addString(animation);
	DataSectionPtr pSection = pItemDS;
	if (!pItemDS->openSection( "timeScale"))
	{// load the legacy animations
		pSection = BWResource::openSection( ChunkPulseLight::defaultFrameSection );
	}
	BW::vector< Vector2 > animFrames;
	pulseLight.duration = -1.f; // -1 encodes no looping
	if (pSection)
	{
		const float timeScale = pSection->readFloat( "timeScale", 1.f );
		pulseLight.duration = pSection->readFloat( "duration", 0.f ) * timeScale;
		pSection->readVector2s( "frame", animFrames );
		BW::vector<Vector2>::iterator iter = animFrames.begin(),
		                              end = animFrames.end();
		for (; iter != end; ++iter)
		{
			Vector2 & frame = *iter;
			frame.x *= timeScale;
		}
	}
	if (animFrames.empty())
	{
		animFrames.push_back( BW::Vector2(0.f, 1.f) );
		animFrames.push_back( BW::Vector2(1.f, 1.f) );
	}
	pulseLight.firstFrameIdx = static_cast<uint32>(pulseLightsAnimFrames.size());
	pulseLight.numFrames = static_cast<uint32>(animFrames.size());
	std::copy(animFrames.begin(), animFrames.end(),
		std::back_inserter(pulseLightsAnimFrames));

	pulseLights.push_back(pulseLight);
}

AABB LightSceneWriter::boundBox() const
{
	AABB bounds;
	// Generate bounding box
	accumulatePositions( omniLights, &bounds );
	accumulatePositions( spotLights, &bounds );
	accumulatePositions( pulseLights, &bounds );

	return bounds;
}

} // namespace CompiledSpace
} // namespace BW
