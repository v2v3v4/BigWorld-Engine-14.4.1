#ifndef MATERIAL_UTILITY_HPP
#define MATERIAL_UTILITY_HPP

#include "moo/com_object_wrap.hpp"

BW_BEGIN_NAMESPACE

namespace Moo
{
	class EffectMaterial;
};

class EditorEffectProperty;

class MaterialUtility
{
public:

	/**
	 *	Technique helpers.
	 */
	static int numTechniques( Moo::EffectMaterialPtr material );

	static int listTechniques(
		Moo::EffectMaterialPtr m,
		BW::vector<BW::string>& retVector );

	static bool MaterialUtility::isTechniqueValid(
		Moo::EffectMaterialPtr m,
		int index );

	static bool MaterialUtility::viewTechnique(
		Moo::EffectMaterialPtr m,
		int index );

	static bool MaterialUtility::viewTechnique(
		Moo::EffectMaterialPtr m,
		const BW::string& name );

	static int MaterialUtility::techniqueByName(
		Moo::EffectMaterialPtr material,
		const BW::string& name );

	static int currentTechnique( Moo::EffectMaterialPtr material );

	/**
	 *	Tweakable Property helpers.
	 */
	static int numProperties( Moo::EffectMaterialPtr material );

	static int listProperties(
		Moo::EffectMaterialPtr m,
		BW::vector<BW::string>& retVector );

    static bool artistEditable( EditorEffectProperty* pProperty );
    static BW::string UIName( EditorEffectProperty* pPropert );
	static BW::string UIDesc( EditorEffectProperty* pPropert );
	static BW::string UIWidget( EditorEffectProperty* pPropert );

	static void MaterialUtility::setTexture( Moo::EffectMaterialPtr material,
		int idx, const BW::string& textureName );

	/**
	 *	Save a material to a data section.  Note that
	 *	materialProperties / runtimeInitMaterialProperties must have been
	 *	called during application startup in order for the save to work.
	 */
	static void save(
		Moo::EffectMaterialPtr m,
		DataSectionPtr pSection );
};

BW_END_NAMESPACE
#endif
