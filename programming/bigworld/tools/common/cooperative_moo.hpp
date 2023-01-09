/**
 *	cooperative_moo.hpp
 */

#ifndef _COOPERATIVE_MOO_HPP_
#define _COOPERATIVE_MOO_HPP_

BW_BEGIN_NAMESPACE

class IEditorApp;

/**
 *	This class helps make Moo windowed apps cooperative with other DX apps.
 */
class CooperativeMoo
{
public:
	/// Operation mode enum type.
	enum MODE
	{
		OFF,
		ON,
		AUTO
	};

	static bool init( DataSectionPtr configSection = NULL );

	static void mode( MODE newMode );
	static MODE mode();

	static bool beginOnPaint( IEditorApp * editorApp );
	static void endOnPaint( IEditorApp * editorApp );

	static bool canUseMoo(
		IEditorApp * editorApp,
		bool isWindowActive, uint32 minTextureMemMB = 32 );

	struct MooState
	{
		MooState()
			: s_inited_( false )
			, s_wasPaused_( false )
			, s_mode_( CooperativeMoo::AUTO )
			, s_otherAppsRunning_( false )
			, s_lastCheckTime_ ( 0 )
		{
			REGISTER_SINGLETON_FUNC( MooState, &CooperativeMoo::mooState )
		}

		/// Guard to prevent calls before the class is inited.
		bool s_inited_;

		/// Flag used in begin/endPaint to know if we have temporarily acquired DX.
		bool s_wasPaused_;

		/// Variable to store the current mode of operation
		MODE s_mode_;

		/// This is used to know if there are other DX apps running.
		bool s_otherAppsRunning_;

		uint64 s_lastCheckTime_;

		/// This variable caches the current exe's name.
		BW::string s_thisAppName_;

		/// This vector contains the list of apps we want to cooperate with.
		BW::vector< BW::wstring > s_otherApps_;
	};

	static MooState & mooState();

private:
	static void tick();

	static void deactivate();

	static bool needsToCooperate();
	static bool isCooperativeApp( const BW::wstring & procName );
};

BW_END_NAMESPACE

#endif _COOPERATIVE_MOO_HPP_
