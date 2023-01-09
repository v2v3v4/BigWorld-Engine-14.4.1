#ifndef MANAGED_EFFECT_HPP
#define MANAGED_EFFECT_HPP

#include "moo_dx.hpp"
#include "cstdmf/smartpointer.hpp"
#include "cstdmf/concurrency.hpp"
#include "cstdmf/md5.hpp"
#include "cstdmf/singleton.hpp"
#include "cstdmf/profiler.hpp"
#include "cstdmf/guard.hpp"
#include "math/vector4.hpp"
#include "effect_constant_value.hpp"
#include "com_object_wrap.hpp"
#include "base_texture.hpp"
#include "graphics_settings.hpp"
#include "effect_manager.hpp"
#include "effect_property.hpp"


BW_BEGIN_NAMESPACE

class BinaryBlock;
typedef SmartPointer<BinaryBlock> BinaryPtr;

namespace Moo
{

// Forward declarations
class EffectTechniqueSetting;
class EffectMacroSetting;
class StateManager;
class EffectIncludes;

enum ChannelType
{
	OPAQUE_CHANNEL,
	TRANSPARENT_CHANNEL,
	TRANSPARENT_INTERNAL_SORT_CHANNEL,
	SHIMMER_CHANNEL,
	SHIMMER_INTERNAL_SORT_CHANNEL,
	NUM_CHANNELS,
	NOT_SUPPORTED_CHANNEL,
	NULL_CHANNEL // for cases where there is no rendering material
};
/**
*	A structure representing a D3D effect technique.
*/
struct TechniqueInfo
{
	TechniqueInfo();

	D3DXHANDLE			handle_;
	bool				supported_;

	BW::string			name_;
	BW::string			settingLabel_;
	BW::string			settingDesc_;
	ChannelType			channelType_;
	uint16				psVersion_;
	bool				skinned_;
	bool				bumpMapped_;
	bool				dualUV_;
	uint				renderPassType_;
	bool				instanced_;
};

/**
*	Used internally to register graphics settings created by annotating effect
*	files and their techniques. Effect files sometimes provide alternative 
*	techniques to render the same material. Normally, the techniques to be 
*	used is selected based on hardware capabilities. By annotating the effect
*	files with specific variables, you can allow the user to select between
*	the supported techniques via the graphics settings facility. 
*
*	To register an effect file as a graphics setting, you need (1) to add
*	a top level parameter named "graphicsSetting", with a string annotation 
*	named "label". This string will be used as the setting label property:
*
*	int graphicsSetting
*	<
*		string label = "PARALLAX_MAPPING";
*	>;
*	
*	And (2) have, for each selectable technique, a string annotation named
*	"label". This string will be used as the option entry label property:
*
*	technique lighting_shader1
*	< 
*		string label = "SHADER_MODEL_1";
*	>
*	{ (...) }
*
*	technique software_fallback
*	< 
*		string label = "SHADER_MODEL_0";
*	>
*	{ (...) }
*/
class EffectTechniqueSetting : public GraphicsSetting
{
public:
	typedef GraphicsSetting::GraphicsSettingPtr GraphicsSettingPtr;

	/**
	*	Interface for objects desiring callbacks on technique selection.
	*/
	class IListener
	{
	public:
		virtual void onSelectTechnique(
			ManagedEffect * effect, D3DXHANDLE hTec) = 0;
	};

	EffectTechniqueSetting(ManagedEffect * owner,
		const BW::string & label ,
		const BW::string & desc);

	void addTechnique(const TechniqueInfo &info);	
	virtual void onOptionSelected(int optionIndex);
	void setPSCapOption(int psVersionCap);

	D3DXHANDLE activeTechniqueHandle() const;

	int addListener(IListener * listener);
	void delListener(int listenerId);

private:	
	typedef std::pair<D3DXHANDLE, uint16>    D3DXHandlesPSVerPair;
	typedef BW::vector<D3DXHandlesPSVerPair> D3DXHandlesPSVerVector;
	typedef BW::map<int, IListener *>        ListenersMap;

	ManagedEffect        * owner_;
	D3DXHandlesPSVerVector techniques_;
	ListenersMap           listeners_;
	SimpleMutex	           listenersLock_;

	static int listenersId;
};

/**
 *	This class manages an individual d3d effect and its standard properties.
 */
class ManagedEffect : 
	public	SafeReferenceCount,
	public	EffectTechniqueSetting::IListener,
	public	EffectManager::IListener
{
public:
	typedef std::pair<D3DXHANDLE, EffectConstantValuePtr*> MappedConstant;
	typedef BW::vector<MappedConstant> MappedConstants;
	typedef BW::vector<TechniqueInfo> TechniqueInfoCache;

	typedef uint32 CompileMark;
	typedef D3DXHANDLE Handle;

public:
	ManagedEffect(bool isEffectPool = false);
	~ManagedEffect();

	const BW::string& resourceID() const;
	ID3DXEffect* pEffect();

	CompileMark compileMark() const;

	bool load( const BW::string& resourceName );
	bool reload();

	EffectPropertyMappings& defaultProperties();

	virtual void setAutoConstants();

	bool registerGraphicsSettings( const BW::string & effectResource );
	EffectTechniqueSetting* graphicsSettingEntry();

	const TechniqueInfoCache& techniques();
	Handle currentTechnique() const;
	virtual bool currentTechnique( Handle hTec, bool setExplicit );
	const BW::string currentTechniqueName();

	virtual bool begin( bool setAutoProps );
	virtual bool beginPass( uint32 pass );
	virtual bool endPass();
	virtual bool end();
	virtual bool commitChanges();
	virtual uint32 numPasses() const;

	bool skinned( D3DXHANDLE techniqueOverride = NULL );
	bool bumpMapped( D3DXHANDLE techniqueOverride = NULL );
	bool dualUV( D3DXHANDLE techniqueOverride = NULL );
	bool instanced( D3DXHANDLE techniqueOverride = NULL );

	ChannelType getChannelType( Handle techniqueOverride = NULL );

	// Direct access to properties
	Handle getParameterByName( const BW::string& name );
	Handle getParameterBySematic( const BW::string& semantic );

	void setBool( Handle h, bool b );
	void setInt( Handle h, int i );
	void setFloat( Handle h, float f );
	void setVector( Handle h, const Vector2& vec );
	void setVector( Handle h, const Vector3& vec );
	void setVector( Handle h, const Vector4& vec );
	void setMatrix( Handle h, const Matrix& mat );
	void setTexture( Handle h, const BaseTexturePtr& tex );

	template<typename T>
	void setValue( Handle h, const T& val )
	{
		BW_GUARD;
		pEffect_->SetValue( h, &val, sizeof(T) );
	}

private:
	bool beginCalled() { return numPasses_ != 0; }

	void loadTechniqueInfo();

private:
	typedef SmartPointer<EffectTechniqueSetting>	EffectTechniqueSettingPtr;

	// Functions
	// Override default constructors so we don't accidentally copy this class.
	ManagedEffect( const ManagedEffect& );
	ManagedEffect& operator=( const ManagedEffect& );

	INLINE TechniqueInfo*	findTechniqueInfo( D3DXHANDLE handle );

	// overrides for  EffectTechniqueSetting::listener
	virtual void onSelectTechnique(ManagedEffect * effect, D3DXHANDLE hTec);
	virtual void onSelectPSVersionCap(int psVerCap);

	// Member Data
	ComObjectWrap<ID3DXEffect>	pEffect_;
	Handle						currentTechnique_;
	bool						techniqueExplicitlySet_;
	bool						isEffectPool_;

	int							settingsListenerId_;

	EffectPropertyMappings		defaultProperties_;
	MappedConstants				mappedConstants_;
	BW::string					resourceID_;
	TechniqueInfoCache			techniques_;

	BW::string					settingName_;
	BW::string					settingDesc_;
	EffectTechniqueSettingPtr   settingEntry_;
	bool						settingsAdded_;

	uint32						numPasses_;

	// Used to check if handles to effect are valid if it has been reloaded
	CompileMark compileMark_;
};


} // namespace Moo

#ifdef CODE_INLINE
#include "managed_effect.ipp"
#endif

BW_END_NAMESPACE

#endif // MANAGED_EFFECT_HPP
