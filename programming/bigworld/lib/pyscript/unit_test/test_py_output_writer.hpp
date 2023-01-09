#ifndef TEST_PY_OUTPUT_WRITER_HPP
#define TEST_PY_OUTPUT_WRITER_HPP

#include "test_harness.hpp"
#include "cstdmf/bw_string.hpp"
#include "cstdmf/debug_filter.hpp"
#include "cstdmf/debug_message_callbacks.hpp"
#include "cstdmf/string_builder.hpp"

#include "script/script_output_hook.hpp"

/**
 * Define some utility classes and fixtures for testing various aspects
 * of the PyOutputWriter system
 */

namespace BW
{

/**
 * This class receives and processes Python's stdio (sys.stdout and sys.stderr)
 * message streams as they appear to the BigWorld Debug Message system.
 */
class ScriptOutputDebugCallback : public DebugMessageCallback
{
public:
	ScriptOutputDebugCallback()
	{
		DebugFilter::instance().addMessageCallback( this );
	}

	virtual ~ScriptOutputDebugCallback()
	{
		DebugFilter::instance().deleteMessageCallback( this );
	}

private:
	// Deliberately using the same interface as ScriptOutputHook
	virtual void onScriptOutput( const BW::string & output, bool isStderr ) = 0;

	/* DebugMessageCallback implementation */
	bool handleMessage(
		DebugMessagePriority messagePriority, const char * pCategory,
		DebugMessageSource messageSource, const LogMetaData & metaData,
		const char * pFormat, va_list argPtr )
	{
			// stdio messages appear as MESSAGE_SOURCE_SCRIPT with no category
			if ((messageSource != MESSAGE_SOURCE_SCRIPT) ||
				(pCategory && (pCategory[0] != '\0')))
			{
				return false;
			}
			// stdout messages appear at priority INFO
			// stderr messages appear at priority ERROR
			if (messagePriority != MESSAGE_PRIORITY_INFO &&
				messagePriority != MESSAGE_PRIORITY_ERROR )
			{
				return false;
			}

			StringBuilder builder( 2048 );
			builder.vappendf( pFormat, argPtr );
			BW::string result( builder.string(), builder.length() );
			this->onScriptOutput( result,
				/* isStderr */ messagePriority == MESSAGE_PRIORITY_ERROR );

			return false;
	}
};


/**
 * This fixture latches a boolean if it sees the expected string on the
 * expected channel
 */
class ScriptDebugMatchFixture : 
	public ScriptOutputDebugCallback, public PyScriptUnitTestHarness
{
public:
	ScriptDebugMatchFixture( const BW::string & target, bool isError ) :
		ScriptOutputDebugCallback(),
		PyScriptUnitTestHarness(),
		target_( target ),
		isError_( isError ),
		hasMatched_( false )
	{}

	bool hasMatched() const { return hasMatched_; }

private:
	/* ScriptOutputDebugCallback implementation */
	void onScriptOutput( const BW::string & output, bool isStderr )
	{
		if (isStderr == isError_ && output == target_)
		{
			hasMatched_ = true;
		}
	}

	const BW::string target_;
	const bool isError_;
	bool hasMatched_;
};


/**
 * This class triggers a virtual function if it sees the expected string on
 * the expected channel.
 */
class ScriptOutputHookMatcher : public ScriptOutputHook
{
public:
	ScriptOutputHookMatcher( const BW::string & target, bool isError ) :
		ScriptOutputHook(),
		pHookedWriter_( NULL ),
		wasUsedAfterFree_( false ),
		target_( target ),
		isError_( isError )
	{
	}

	virtual ~ScriptOutputHookMatcher()
	{
		this->unhook();
	}

	void hook()
	{
		if (pHookedWriter_ != NULL)
		{
			return;
		}

		pHookedWriter_ = ScriptOutputHook::hookScriptOutput( this );
	}

	void unhook()
	{
		if (pHookedWriter_ == NULL)
		{
			return;
		}

		ScriptOutputHook::unhookScriptOutput( pHookedWriter_, this );
		pHookedWriter_ = NULL;
	}

	bool isHooked() const { return pHookedWriter_ != NULL; }
	bool wasUsedAfterFree() const { return wasUsedAfterFree_; }

	virtual void onMatch() = 0;

	/* ScriptOutputHook implementation */
	void onScriptOutput( const BW::string & output, bool isStderr )
	{
		if (pHookedWriter_ == NULL)
		{
			wasUsedAfterFree_ = true;
		}
		else if (isStderr == isError_ && output == target_)
		{
			this->onMatch();
		}
	}

	void onOutputWriterDestroyed( ScriptOutputWriter * owner )
	{
		if (owner != pHookedWriter_)
		{
			// Real code would MF_ASSERT here, but it's not really
			// used-after-free so much as ScriptOutputWriter was copied
			// or something. But we _will_ trigger a use-after-free
			// since we'd later unhook from the wrong thing, or fail
			// to unhook from the right thing.
			wasUsedAfterFree_ = true;
		}
		else
		{
			pHookedWriter_ = NULL;
		}
	}

private:
	ScriptOutputWriter * pHookedWriter_;
	bool wasUsedAfterFree_;

	const BW::string target_;
	const bool isError_;
};


/**
 * This class latches a value when it is triggered
 */
class ScriptOutputHookLatchFixture : 
	public ScriptOutputHookMatcher, public PyScriptUnitTestHarness
{
public:
	ScriptOutputHookLatchFixture( const BW::string & target, bool isError ) :
		ScriptOutputHookMatcher( target, isError ),
		PyScriptUnitTestHarness(),
		hasMatched_( false )
	{}

	bool hasMatched() const { return hasMatched_; }

private:
	/* ScriptOutputHookMatcher implementation */
	void onMatch()
	{
		hasMatched_ = true;
	}

	bool hasMatched_;
};


/**
 * This class hooks the other given ScriptOutputHookMatcher when it is triggered
 */
class ScriptOutputAddOtherHook : public ScriptOutputHookMatcher
{
public:
	ScriptOutputAddOtherHook( ScriptOutputHookMatcher * pOther,
			const BW::string & target, bool isError ) :
		ScriptOutputHookMatcher( target, isError ),
		pOther_( pOther )
	{
	}

private:
	/* ScriptOutputHookMatcher implementation */
	void onMatch()
	{
		pOther_->hook();
	}

	ScriptOutputHookMatcher * pOther_;
};


/**
 * This class unhooks the other given ScriptOutputHookMatcher when it is triggered
 */
class ScriptOutputDelOtherHook : public ScriptOutputHookMatcher
{
public:
	ScriptOutputDelOtherHook( ScriptOutputHookMatcher * pOther,
			const BW::string & target, bool isError ) :
		ScriptOutputHookMatcher( target, isError ),
		pOther_( pOther )
	{
	}

private:
	/* ScriptOutputHookMatcher implementation */
	void onMatch()
	{
		pOther_->unhook();
	}

	ScriptOutputHookMatcher * pOther_;
};


/**
 * This class unhooks itself when it is triggered
 */
class ScriptOutputDelSelfHook : public ScriptOutputHookMatcher
{
public:
	ScriptOutputDelSelfHook( const BW::string & target, bool isError ) :
		ScriptOutputHookMatcher( target, isError )
	{
	}

private:
	/* ScriptOutputHookMatcher implementation */
	void onMatch()
	{
		this->unhook();
	}
};


} // namespace BW

#endif /* TEST_PY_OUTPUT_WRITER_HPP */
