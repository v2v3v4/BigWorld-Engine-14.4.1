#ifndef EFFECT_HELPERS_HPP
#define EFFECT_HELPERS_HPP

#ifdef D3DXSHADER_USE_LEGACY_D3DX9_31_DLL
#define USE_LEGACY_D3DX9_DLL D3DXSHADER_USE_LEGACY_D3DX9_31_DLL
#else  // D3DXSHADER_USE_LEGACY_D3DX9_31_DLL
#define USE_LEGACY_D3DX9_DLL 0
#endif // D3DXSHADER_USE_LEGACY_D3DX9_31_DLL

BW_BEGIN_NAMESPACE

namespace Moo {
	namespace EffectHelpers {

	/**
	*	Transforms the given .fx resource ID into the appropriate .fxo ID.
	*/
	BW::string fxoName( const BW::string& resourceID );

	/**
	*	Transforms the given .fxo resource ID into the appropriate .fx ID.
	*   Will return false if the infix does not match the current global infix.
	*/
	bool fxName( const StringRef& resourceID, BW::string & effectResource );

	/**
	 *	Loads and returns the compiled binary for the given effect .fx.
	 *	This will return NULL if the compiled effect does not exist.
	 */
	BinaryPtr loadEffectBinary( const BW::string& resourceID );

	} // namespace EffectHelpers
} // namespace Moo

BW_END_NAMESPACE

#endif // EFFECT_HELPERS_HPP
