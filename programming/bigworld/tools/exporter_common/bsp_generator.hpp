#ifndef BSP_GENERATOR_HPP
#define BSP_GENERATOR_HPP

#include "cstdmf/bw_namespace.hpp"
#include "cstdmf/bw_string.hpp"
#include "cstdmf/bw_vector.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This function generates a bsp from a visual.
 *
 *	@param	visualName	The visual file to extract the BSP from.
 *	@param	bspName		The name of the BSP.
 *	@param	materialIDs	The list of materialIDs to be returned.
 *	@return	Success or failure.
 */
bool generateBSP( const BW::string & visualName,
		const BW::string & bspName,
		BW::vector< BW::string > & materialIDs );


BW_END_NAMESPACE

#endif // BSP_GENERATOR_HPP
