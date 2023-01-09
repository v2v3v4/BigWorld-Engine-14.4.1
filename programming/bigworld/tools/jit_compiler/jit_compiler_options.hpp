#ifndef JIT_COMPILER_OPTIONS
#define JIT_COMPILER_OPTIONS

#include "asset_pipeline/compiler/asset_compiler_options.hpp"

BW_BEGIN_NAMESPACE



class JitCompilerOptions : public AssetCompilerOptions
{
public:
	JitCompilerOptions();
	explicit JitCompilerOptions( const AssetCompiler & compiler );

	// Accessors
	bool enableBalloonNotifications() const;
	void enableBalloonNotifications( bool enable );


protected:
	bool enableBalloonNotifications_;
};

BW_END_NAMESPACE

#endif // JIT_COMPILER_OPTIONS
