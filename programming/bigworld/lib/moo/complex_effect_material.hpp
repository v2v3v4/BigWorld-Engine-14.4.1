#pragma once

#include "effect_material.hpp"
#include "draw_context.hpp"

BW_BEGIN_NAMESPACE

namespace Moo
{

	//-- Special kind of EffectMaterial which acts as a container of a set of EffectMaterials each
	//-- for individual pipeline stage, like shadow cast pass, reflection pass. More over each
	//-- pipeline stage may be either instanced or not. This info also stored here and used later
	//-- to decide which one VisualBatcher will be used.
	//----------------------------------------------------------------------------------------------
	class ComplexEffectMaterial	: public SafeReferenceCount
	{
	public:
		ComplexEffectMaterial();
		~ComplexEffectMaterial();

		explicit ComplexEffectMaterial(const ComplexEffectMaterial& other);
		ComplexEffectMaterial & operator = (const ComplexEffectMaterial& other);

		bool						load(const DataSectionPtr& section, bool addDefault = true);
		bool						initFromEffect(const BW::string& effect, const BW::string& diffuseMap = "", int doubleSided = -1);
		
		const BW::string&			identifier() const						{ return m_material->identifier(); }
		void						identifier(const BW::string& id)		{ m_material->identifier(id); }
		bool						instanced()	const						{ return m_instanced; }
		bool						skinned() const							{ return m_skinned; }
		const EffectMaterialPtr&	baseMaterial() const					{ return m_material; }
		WorldTriangle::Flags		getFlags(int objectMaterialKind) const	{ return m_material->getFlags(objectMaterialKind); }
		
		//-- return effect material for desired rendering pass.
		const EffectMaterialPtr&	pass(ERenderingPassType type, bool instanced = false) const;
		void						pass( ERenderingPassType type,
										ManagedEffect*& outEffect,
										ManagedEffect::Handle& outTechnique,
										BW::map< D3DXHANDLE, SmartPointer<EffectProperty> >*& outMaterialProperties,
										bool& inoutIsInstanced ) const;

		bool						checkEffectRecompiled() const;

		ChannelType					channelType() const { return m_channelType; }

	private:
		bool						finishInit();

	private:
		bool			  m_instanced;
		bool			  m_skinned;
		EffectMaterialPtr m_material;
		ChannelType		  m_channelType;
		D3DXHANDLE		  m_techniques[RENDERING_PASS_COUNT][2];
	};

	//----------------------------------------------------------------------------------------------
	inline const EffectMaterialPtr& ComplexEffectMaterial::pass(ERenderingPassType type, bool instanced) const
	{
		BW_GUARD;
		this->checkEffectRecompiled();

		D3DXHANDLE technique = m_techniques[type][instanced];
		m_material->hTechnique( technique );
		return m_material;
	}

	//----------------------------------------------------------------------------------------------
	inline void ComplexEffectMaterial::pass( ERenderingPassType type,
		ManagedEffect*& outEffect,
		ManagedEffect::Handle& outTechnique,
		BW::map< D3DXHANDLE, SmartPointer<EffectProperty> >*& outMaterialProperties,
		bool& inoutIsInstanced ) const
	{
		BW_GUARD;
		this->checkEffectRecompiled();

		outEffect = m_material->pEffect().get();
		outMaterialProperties = &m_material->properties();
		outTechnique = inoutIsInstanced ? m_techniques[type][1] : NULL;
		if (!outTechnique)
		{
			// hw instancing technique is not available
			// fall back to non instanced technique
			outTechnique = m_techniques[type][0];
			inoutIsInstanced = false;
		}
	}

	//----------------------------------------------------------------------------------------------
	inline bool	ComplexEffectMaterial::checkEffectRecompiled() const
	{
		bool recompiled = m_material->checkEffectRecompiled();
		if (recompiled)
		{
			const_cast<ComplexEffectMaterial*>( this )->finishInit();
		}

		return recompiled;
	}


} //-- Moo

BW_END_NAMESPACE
