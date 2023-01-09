/*
 * bw_namespace.hpp
 *
 * BW namespace definitions
 */
#ifndef BW_NAMESPACE_HPP_
#define BW_NAMESPACE_HPP_

#define BW_ENABLE_NAMESPACE

#ifdef BW_ENABLE_NAMESPACE
	#define BW_BEGIN_NAMESPACE namespace BW {
	#define BW_END_NAMESPACE }
	#define BW_USE_NAMESPACE using namespace BW;
	#define BW_NAMESPACE BW::
	#define BW_NAMESPACE_NAME BW
#else
	#define BW_BEGIN_NAMESPACE
	#define BW_END_NAMESPACE
	#define BW_USE_NAMESPACE
	#define BW_NAMESPACE
	#define BW_NAMESPACE_NAME
#endif


#endif /* BW_NAMESPACE_HPP_ */
