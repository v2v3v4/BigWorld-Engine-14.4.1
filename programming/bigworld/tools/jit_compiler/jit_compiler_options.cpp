#include "jit_compiler_options.hpp"

BW_BEGIN_NAMESPACE

JitCompilerOptions::JitCompilerOptions()
	: AssetCompilerOptions()
	, enableBalloonNotifications_( true )
{
}

JitCompilerOptions::JitCompilerOptions( const AssetCompiler & compiler )
	: AssetCompilerOptions( compiler )
	, enableBalloonNotifications_( true )
{
}

bool JitCompilerOptions::enableBalloonNotifications() const
{
	return enableBalloonNotifications_;
}

void JitCompilerOptions::enableBalloonNotifications( bool enable )
{
	enableBalloonNotifications_ = enable;
}

BW_END_NAMESPACE
