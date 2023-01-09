#include "pch.hpp"
#include "effect_constant_value.hpp"


BW_BEGIN_NAMESPACE

namespace Moo
{

namespace
{
	typedef BW::map<BW::string, EffectConstantValuePtr*> Mappings;

	struct EffectConstantStatics
	{
		EffectConstantStatics()
		{
			REGISTER_SINGLETON( EffectConstantStatics )
		}

		Mappings	mappings_;

		static EffectConstantStatics & instance();
	};

	static EffectConstantStatics s_EffectConstantStatics;

	EffectConstantStatics & EffectConstantStatics::instance()
	{
		SINGLETON_MANAGER_WRAPPER( EffectConstantStatics )
		return s_EffectConstantStatics;
	}
}

// -----------------------------------------------------------------------------
// Section: EffectConstantValue
// -----------------------------------------------------------------------------

/*static*/ void EffectConstantValue::fini()
{
	BW_GUARD;
	EffectConstantStatics & statics = EffectConstantStatics::instance();
	Mappings::iterator it = statics.mappings_.begin();
	for (;it != statics.mappings_.end(); it++)
	{
		if (it->second->getObject())
		{
			if (it->second->getObject()->refCount() > 1)
				WARNING_MSG("EffectConstantValue not deleted properly : %s\n", it->first.c_str());

			*(it->second) = NULL;
		}
		bw_safe_delete(it->second);
	}

	statics.mappings_.clear();
}

/**
 * This method returns a pointer to a smartpointer to a EffectConstantValue, this is
 * done so that the material can store the constant value pointer without having
 * to worry about it changing. 
 * @param identifier the name of the constant to get the value for
 * @param createIfMissing create a new constant mapping if one does not exist.
 * @return pointer to a smartpointer to a constant value.
 */
EffectConstantValuePtr* EffectConstantValue::get( const BW::string& identifier, bool createIfMissing )
{
	BW_GUARD;
	EffectConstantStatics & statics = EffectConstantStatics::instance();
	Mappings::iterator it = statics.mappings_.find( identifier );
    if (it == statics.mappings_.end())
	{
		if (createIfMissing)
		{
			// TODO: make this managed, don't like allocating four byte blocks.
			EffectConstantValuePtr* ppProxy = new EffectConstantValuePtr;
			statics.mappings_.insert( std::make_pair( identifier, ppProxy ) );
			it = statics.mappings_.find( identifier );
		}
		else
		{
			return NULL;
		}
	}

	return it->second;
}

/**
 *	This method sets a constant value for the identifier provided.
 * @param identifier the name of the constant
 * @param pEffectConstantValue the value of the constant
 */
void EffectConstantValue::set( const BW::string& identifier, EffectConstantValue* pEffectConstantValue )
{
	BW_GUARD;
	*get(identifier) =  pEffectConstantValue;
}

} // namespace Moo

BW_END_NAMESPACE

// effect_constant_value.cpp
