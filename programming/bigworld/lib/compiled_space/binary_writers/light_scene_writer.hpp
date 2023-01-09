#ifndef LIGHT_SCENE_WRITER_HPP
#define LIGHT_SCENE_WRITER_HPP

#include "cstdmf/bw_string.hpp"
#include "cstdmf/bw_vector.hpp"

#include "../light_scene_types.hpp"
#include "space_writer.hpp"

#include <moo/omni_light.hpp>
#include <moo/spot_light.hpp>
#include <moo/moo_math.hpp>

BW_BEGIN_NAMESPACE
class DataSection;
typedef SmartPointer<DataSection> DataSectionPtr;
BW_END_NAMESPACE

namespace BW {
namespace CompiledSpace {

class BinaryFormatWriter;
class StringTableWriter;

class LightSceneWriter :
	public ISpaceWriter
{
public:
	LightSceneWriter();
	~LightSceneWriter();
	
	virtual bool initialize( const DataSectionPtr& pSpaceSettings,
		const CommandLine& commandLine );
	virtual bool write( BinaryFormatWriter& writer );

	void convertOmniLight( const ConversionContext& ctx, 
		const DataSectionPtr& pItemDS, const BW::string& uid );
	void convertSpotLight( const ConversionContext& ctx, 
		const DataSectionPtr& pItemDS, const BW::string& uid );
	void convertPulseLight( const ConversionContext& ctx, 
		const DataSectionPtr& pItemDS, const BW::string& uid );

	void addOmniLight( const DataSectionPtr & pItemDS,
		const Matrix & worldTransform );
	void addSpotLight( const DataSectionPtr & pItemDS,
		const Matrix & worldTransform );
	void addPulseLight( const DataSectionPtr & pItemDS,
		const Matrix & worldTransform, StringTableWriter & stringTable_ );

	AABB boundBox() const;

private:
	BW::vector< LightSceneTypes::OmniLight > omniLights;
	BW::vector< LightSceneTypes::SpotLight > spotLights;
	BW::vector< LightSceneTypes::PulseLight > pulseLights;
	BW::vector< Vector2 > pulseLightsAnimFrames;
};

} // namespace CompiledSpace
} // namespace BW


#endif // LIGHT_SCENE_WRITER_HPP
