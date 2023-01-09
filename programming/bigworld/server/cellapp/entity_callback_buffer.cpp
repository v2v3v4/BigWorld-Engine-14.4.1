#include "script/first_include.hpp"

#include "entity_callback_buffer.hpp"

#include "entity.hpp"
#include "profile.hpp"

#include "cstdmf/debug.hpp"

BW_BEGIN_NAMESPACE

EntityCallbackBuffer::EntityCallbackBuffer()
:
	callbacksPrevented_(0),
	allowCallbacksOverride_(0)
{
}

void EntityCallbackBuffer::enableHighPriorityBuffering()
{
	++allowCallbacksOverride_;
}

void EntityCallbackBuffer::disableHighPriorityBuffering()
{
	--allowCallbacksOverride_;
}

void EntityCallbackBuffer::enterBufferedSection()
{
	MF_ASSERT_DEBUG( callbacksPrevented_ >= 0 );
	++callbacksPrevented_;
}

void EntityCallbackBuffer::leaveBufferedSection()
{
	if (--callbacksPrevented_ != 0)
	{
		MF_ASSERT_DEBUG( callbacksPrevented_ > 0 );
	}
	else
	{
		this->flush();
	}
}

void EntityCallbackBuffer::flush()
{
	if (!highPriorityBuffer_.empty() || !buffer_.empty())
	{
		SCOPED_PROFILE( SCRIPT_CALL_PROFILE )

		// be very careful about reentrancy here... if we do happen to
		// get back here from deeper in the stack, then the new calls
		// will be processed first (which makes sense since they would
		// have been done from the first callback in the ordinary order
		// of things)

		BufferedScriptCallbacks highPriorityLocalBuffer;
		highPriorityLocalBuffer.swap( highPriorityBuffer_ );

		BufferedScriptCallbacks localBuffer;
		localBuffer.swap( buffer_ );

		this->execute( highPriorityLocalBuffer );
		this->execute( localBuffer );
	}
}

void EntityCallbackBuffer::execute(
	BufferedScriptCallbacks & scriptCalls ) const
{
	for (BufferedScriptCallbacks::iterator i = scriptCalls.begin();
		i != scriptCalls.end();
		++i)
	{
		BufferedScriptCallback & bsc = *i;

		if (bsc.entity->isDestroyed())
		{
			WARNING_MSG(
				"EntityCallbackBuffer::execute: %s entity %u has been \
					destroyed\n",
				(bsc.errorPrefix != NULL) ? bsc.errorPrefix : "",
				bsc.entity->id() );
		}
		else
		{
			Entity::nominateRealEntity( *bsc.entity );

			bsc.callable.callFunction(
				bsc.args, ScriptErrorPrint(bsc.errorPrefix) );

			Entity::nominateRealEntityPop();
		}
	}
}

void EntityCallbackBuffer::storeCallback(
	EntityPtr entity,
	const ScriptObject & funcCallable, const ScriptArgs & args,
	const char * errorPrefix )
{
	MF_ASSERT_DEBUG( !isBuffering() );

	const bool highPriority = allowCallbacksOverride_ >= callbacksPrevented_;

	BufferedScriptCallbacks& buffer = 
		(highPriority) ? highPriorityBuffer_ : buffer_;

	buffer.push_back( BufferedScriptCallback() );

	BufferedScriptCallback & bsc = buffer.back();
	bsc.entity = entity;
	bsc.callable = funcCallable;
	bsc.args = args;
	bsc.errorPrefix = errorPrefix;
}

BW_END_NAMESPACE
