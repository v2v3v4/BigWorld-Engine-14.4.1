#ifndef SPEEDTREE_CONFIG_LITE_HPP
#define SPEEDTREE_CONFIG_LITE_HPP

/**
 *	To enable support for speedtree in BigWorld, set BW_SPEEDTREE_SUPPORT to ON in 
 *	CMake. When speedtree suport is disabled, objects and classes from
 *  src/lib/speedtree can still be used, but they will have empty implementation.
 *  To avoid interrupting the execution of its client code, it will not generate
 *  errors, trigger asserts or throw exceptions, but it will print warnings to the
 *  debug output.
 *
 *	To build BigWorld with support for SpeedTree, you will need SpeedTreeRT SDK 
 *	4.2 or later installed and a current license. For detailed instructions on how 
 *	to setup SpeedTreeRT SDK, please refer to the Third Party Integrations document 
 *	in (bigworld/doc).
 */


//-- Set SPT_ENABLE_NORMAL_MAPS to 0 disable normal maps. Don't forget to do the same in speedtree.fxh
#define SPT_ENABLE_NORMAL_MAPS 1

//-- Set ENABLE_BB_OPTIMISER to 0 to disable the billboard optimizer from the build.
#define ENABLE_BB_OPTIMISER 1

//-- Define the current version of the .ctree files.
//-- Note: after rewriting speedtree's render for Deferred Shading pipeline some internal data
//--	   representations have been changed so we moved *.ctree files version from 103 to 105.
#define SPT_CTREE_VERSION 105

#endif // SPEEDTREE_CONFIG_LITE_HPP
