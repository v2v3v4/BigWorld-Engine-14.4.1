#if !defined(BWENTITY_BW_TYPES_HPP)
#define BWENTITY_BW_TYPES_HPP

#include "bwentity_namespace.hpp"

/**
 * BW & EntityDef includes
 */
#include "bwentity_api.hpp"

#include "bw_warnings_off.hpp"
#include "cstdmf/smartpointer.hpp"
#include "network/basictypes.hpp"
#include "bw_warnings_on.hpp"

/**
 * BW::EntityDef type declarations
 */
BWENTITY_BEGIN_NAMESPACE

/**
 * Map BW basic types to BWEntityDef
 */
typedef BW::EntityID		EntityID;
typedef BW::EntityTypeID 	EntityTypeID;

/**
 * Declare EntityDef smart pointers
 */
typedef BW::SmartPointer< EntityDescriptionMap >EntityDescriptionMapPtr;

BWENTITY_END_NAMESPACE

#endif /* BWENTITY_BW_TYPES_HPP */
