#ifndef PARTICLE_SYSTEM_LOADER_HPP
#define PARTICLE_SYSTEM_LOADER_HPP

#include "cstdmf/stdmf.hpp"
#include "cstdmf/bw_vector.hpp"

#include "compiled_space/forward_declarations.hpp"
#include "particle_system_types.hpp"
#include "static_scene_type_handler.hpp"
#include "binary_format.hpp"
#include "loader.hpp"
#include "math/matrix_liason.hpp"
#include "space/dynamic_scene_provider_types.hpp"

namespace BW {
	
class MetaParticleSystem;
typedef SmartPointer<MetaParticleSystem> MetaParticleSystemPtr;

namespace CompiledSpace {

class COMPILED_SPACE_API ParticleSystemLoader : 
	public ILoader
{
public:
	static void registerHandlers( Scene & scene );

public:
	ParticleSystemLoader( DynamicSceneProvider& dynamicScene );
	virtual ~ParticleSystemLoader();

	void addToScene();
	void removeFromScene();

private:

	virtual bool doLoadFromSpace( ClientSpace * pSpace,
		BinaryFormat& reader,
		const DataSectionPtr& pSpaceSettings,
		const Matrix& mappingTransform,
		const StringTable& stringTable );

	virtual bool doBind();
	virtual void doUnload();
	virtual float percentLoaded() const;

private:
	void loadParticleSystems( const StringTable& strings );
	void freeParticleSystems();

	struct Instance;

	bool loadParticleSystem(
		ParticleSystemTypes::ParticleSystem& def,
		const StringTable& strings,
		Instance& instance );

private:
	BinaryFormat* pReader_;
	BinaryFormat::Stream* pStream_;

	ExternalArray<ParticleSystemTypes::ParticleSystem> systemData_;
	DynamicSceneProvider& dynamicScene_;

private:
	class DrawHandler;
	class TickHandler;

	struct Instance : 
		public MatrixLiaison
	{
		ParticleSystemLoader* pLoader_;
		ParticleSystemTypes::ParticleSystem* pDef_;
		MetaParticleSystemPtr system_;
		float seedTime_;
		uint32 index_;
		DynamicObjectHandle dynamicObjectHandle_;

		// MatrixLiasion
		virtual const Matrix & getMatrix() const;
		virtual bool setMatrix( const Matrix & m );
	};

	BW::vector<Instance> instances_;
};

} // namespace CompiledSpace
} // namespace BW


#endif // PARTICLE_SYSTEM_LOADER_HPP
