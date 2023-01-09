#include "pch.hpp"
#include "stack_tracker.hpp"
#include "string_builder.hpp"

BW_BEGIN_NAMESPACE

#if ENABLE_STACK_TRACKER
THREADLOCAL(StackTracker::StackItem) StackTracker::stack_[THREAD_STACK_DEPTH];
THREADLOCAL(uint) StackTracker::stackPos_ = 0;
THREADLOCAL(char) StackTracker::annotationBuffer_[ANNOTATION_BUFFER_SIZE];
THREADLOCAL(StackTracker::AnnotationEntry) StackTracker::annotationEntries_[MAX_ANNOTATION_STACK_ENTRIES];
THREADLOCAL(char*) StackTracker::nextAnnotationPtr_ = NULL;
THREADLOCAL(StackTracker::AnnotationEntry*) StackTracker::nextAnnotationEntryPtr_ = NULL;
THREADLOCAL(uint32) StackTracker::numAnnotationsToSkipPop_ = 0;

bool g_copyStackInfo = false;

#if defined( _DEBUG )
THREADLOCAL(uint) StackTracker::maxStackPos_ = 0;
#endif


void StackTracker::buildReport( char * buffer, size_t len )
{
	BW::StringBuilder builder( buffer, len );

	if ( stackSize() == 0 )
	{
		builder.append( "<empty>" );
	}
	else
	{
		for (uint i = 0; i < stackSize(); ++i)
		{
			builder.append( getStackItem(i).name );
			if (i < stackSize()-1)
			{
				builder.append( " <- " );
			}
		}
	}
}

#if defined( _DEBUG )
uint StackTracker::getMaxStackPos() { return maxStackPos_; }
#endif

/**
 *  This function pushes string annotation to the current callstack entry recorded by BW_GUARD.
 *  Annotation string is dynamic and will be copied in the thread local annotation buffer
 *  along with the current stack index
 *  collectCallstack functions will embed annotation data into the generated callstacks
 */
void StackTracker::pushAnnotation( const char* annotation )
{
	if (numAnnotationsToSkipPop_ > 0)
	{
		// early out if we are out memory and already skipped at least one annotation
		numAnnotationsToSkipPop_++;
		return;
	}
	if (nextAnnotationPtr_ == NULL)
	{
		nextAnnotationPtr_ = annotationBuffer_;
		nextAnnotationEntryPtr_ = annotationEntries_;
	}

	char* dst = nextAnnotationPtr_;
	char* dstEnd = annotationBuffer_ + ARRAY_SIZE(annotationBuffer_);
	// start recording annotation into the annotation buffer
	while (*annotation != '\0' && dst != dstEnd)
	{
		*dst++ = *annotation++;
	}
	// did we write an entire string ?
	// do we have a space for annotation index ?
	if (dst < dstEnd &&
		nextAnnotationEntryPtr_ != annotationEntries_ + ARRAY_SIZE(annotationEntries_))
	{
		// write string terminator
		*dst++ = '\0';
		// record annotation entry
		nextAnnotationEntryPtr_->stackPos = stackPos_ - 1;
		nextAnnotationEntryPtr_->annotation = nextAnnotationPtr_;
		stack_[stackPos_ - 1].annotation = nextAnnotationPtr_;
		// move both annotation pointers
		nextAnnotationEntryPtr_++;
		nextAnnotationPtr_ = dst;
	}
	else
	{
		numAnnotationsToSkipPop_++;
	}
}

/**
 *  This function pops string annotation of the stack.
 */
void StackTracker::popAnnotation()
{
	if (numAnnotationsToSkipPop_ > 0 )
	{
		numAnnotationsToSkipPop_--;
	}
	else
	{
		// walk back one annotation index
		MF_ASSERT(nextAnnotationEntryPtr_ > annotationEntries_);
		nextAnnotationEntryPtr_--;
		nextAnnotationPtr_ = nextAnnotationEntryPtr_->annotation;
	}
}

#endif // #if ENABLE_STACK_TRACKER

BW_END_NAMESPACE

// stack_tracker.cpp
