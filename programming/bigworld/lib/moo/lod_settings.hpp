#ifndef LOD_SETTINGS_REGISTRY_HPP
#define LOD_SETTINGS_REGISTRY_HPP

#include "cstdmf/debug.hpp"
#include "moo/graphics_settings.hpp"
#include "cstdmf/bw_string.hpp"
#include "cstdmf/bw_vector.hpp"


BW_BEGIN_NAMESPACE

// Forward declarations
typedef SmartPointer<class DataSection> DataSectionPtr;


class LodSettings
{
public:
	void init(DataSectionPtr resXML);
	
	float applyLodBias(float distance);
	void applyLodBias(float & lodNear, float & lodFar);

	float calculateLod( const Matrix & matWorld );
	float calculateLod( const Vector3& position, float yScale  );


#ifdef EDITOR_ENABLED
	bool ignoreLods() const { return ignoreLods_; }
	void ignoreLods( bool ignore ) { ignoreLods_ = ignore; }
#endif

	static LodSettings & instance();

private:
	void setLodOption(int optionIdx);

	BW::vector<float> lodOptions_;
	Moo::GraphicsSetting::GraphicsSettingPtr lodSettings_;

#ifdef EDITOR_ENABLED
	bool ignoreLods_;
#endif

private:
	LodSettings() :
		lodOptions_(),
		lodSettings_(NULL)
#ifdef EDITOR_ENABLED
		,ignoreLods_( false )
#endif
	{}
};

BW_END_NAMESPACE

#endif // LOD_SETTINGS_REGISTRY_HPP
