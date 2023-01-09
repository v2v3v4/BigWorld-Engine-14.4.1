#ifndef FOG_HELPER_HPP
#define FOG_HELPER_HPP

BW_BEGIN_NAMESPACE

namespace Moo
{

	//-- Fog parameters structure.
	//-- Fog is divided into two part. First depends on camera and the distance to the camera,
	//-- second one describes fog which hides space bounds by smoothly change it's density from the
	//-- inner box to the outer box. 
	//-- watch the structure memory alignment because we send this structure as is to
	//--	the GPU and GPU has 16 bytes alignment.
	//----------------------------------------------------------------------------------------------
	struct FogParams
	{
		FogParams()
			:	m_enabled(false), m_density(1.0f), m_start(0.0f), m_end(0.0f),
			m_outerBB(0,0,0,0), m_innerBB(0,0,0,0) { } 

		float	m_enabled;
		float	m_density;
		float	m_start;
		float	m_end;
		Colour	m_color;
		Vector4	m_outerBB; //-- fog outer box.
		Vector4 m_innerBB; //-- fog inner box
	};

	/**
	 *	This class helps with fog setting.
	 */
	class FogHelper
	{
		static FogHelper* s_pInstance;
		FogParams m_fogParams;
	public:
		const FogParams& fogParams() const;
		void fogParams(const FogParams& params);

		void fogEnable(bool enable);
		bool fogEnabled() const;

		static FogHelper* pInstance();

		FogHelper();
		~FogHelper();
	};
} // namespace Moo

BW_END_NAMESPACE

#endif // FOG_HELPER_HPP
