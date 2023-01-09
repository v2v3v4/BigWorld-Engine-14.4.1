//---------------------------------------------------------------------------
#include "pch.h"
#pragma hdrstop
#include "undo.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
//---------------------------------------------------------------------------
Undo::Undo( )
: undoEnabled_( true ),
  redoEnabled_( true ),
  limitSteps_( false ),
  limitMemory_( false ),
  undoStepsLimit_( 64 ),
  undoMemoryLimit_( 16  * 1048576 ),
  memoryUsed_( 0 )
{
	undoPos_ = undoList_.begin();
}
//---------------------------------------------------------------------------
Undo::~Undo( )
{
	clear( );
    
	undoList_.clear();
    redoList_.clear();
    undoFuncs_.clear();

    //updateReport( );
}
//---------------------------------------------------------------------------
unsigned int Undo::addUndoObject( UndoFunc undoFunc )
{
    unsigned int id = undoFuncs_.size();

    undoFuncs_.push_back( undoFunc );

    return id;
}
//---------------------------------------------------------------------------
bool Undo::addUndoData( unsigned int id, unsigned int opCode, AnsiString description, UndoData data, size_t size, bool linkToLast )
{
	return addData( id, opCode, description, data, size, false, linkToLast );
}
//---------------------------------------------------------------------------
bool Undo::addRedoData( unsigned int id, unsigned int opCode, AnsiString description, UndoData data, size_t size, bool linkToLast )
{
	return addData( id, opCode, description, data, size, true, linkToLast );
}
//---------------------------------------------------------------------------
bool Undo::addData( unsigned int id, unsigned int opCode, AnsiString description, UndoData data, size_t size, bool redo, bool linkToLast )
{
	bool result = false;

    static bool lastRedo = true;

	if ( undoEnabled_ )
    {
    	bool processOperation = true;

        UndoInfoVector::iterator it = undoList_.begin();
        UndoInfoVector::iterator rt = redoList_.begin();

    	if ( lastRedo == redo && redoEnabled_ )
        {
        	// we have an unbalanced operation here
            // ie. 2 undos or 2 redos in a row.

            // the undo stack is always either equal or 1 greater than the redo stack
			if ( redo && redoList_.size() == undoList_.size() )
            {
				// the matching undo data for this redo is missing so ignore the data
                processOperation = false;
            }

            if ( !redo && undoList_.size() >= redoList_.size() + 1 )
            {
				// the undo list will grow to be greater than the redo list
                // this means we have missed the matching redo for the last undo
                // we have to delete the last undo operation
                processOperation = false;
            }
        }

        if ( processOperation )
        {
            // create the undo information block
            UndoInfo* undoInfo = new UndoInfo;
            char* undoData = new char [size];

            // fill the data block
            memcpy( undoData, data, size );

            // fill it
            undoInfo->id = id;
            undoInfo->opCode = opCode;
            undoInfo->description = description;
            undoInfo->data = undoData;
            undoInfo->size = size;
            undoInfo->linked = linkToLast;

            it = undoList_.begin();
            rt = redoList_.begin();

            // erase the end of the list if its got data in it
            if ( undoPos_ != undoList_.begin() )
            {
                // pop all elements until we reach the current pos
                while ( it != undoPos_ )
                {
                    // pop the undo data
                    memoryUsed_ -= (*it)->size;

                    if ( (*it)->data ) delete[] ((*it)->data);
                    if ( *it ) 		   delete *it;

                    undoList_.pop_front();
                    it = undoList_.begin();

                    // pop the redo data
                    if ( redoEnabled_ && *rt )
                    {
                        memoryUsed_ -= (*rt)->size;

                        if ( (*rt)->data ) delete[] ((*rt)->data);
                        delete *rt;

                        redoList_.pop_front();
                        rt = redoList_.begin();
                    }
                }
            }

            // tally the memory usage
            memoryUsed_ += size;

            // if memory limit exceeded remove old undo's
            if ( limitMemory_ )
            {
                while( memoryUsed_ > undoMemoryLimit_ )
                    removeLast( );
            }

            // if steps limit exceeded remove old undo's
            if ( limitSteps_ )
            {
                while( undoList_.size() > undoStepsLimit_ )
                    removeLast( );
            }

            // push the data onto the end of the right list
            if ( !redo )
            {
                // add the undo data
                undoList_.push_front( undoInfo );
                undoPos_ = undoList_.begin();
            }
            else
            {
                // add the redo data
                redoList_.push_front( undoInfo );
                redoPos_ = redoList_.begin();
            }

            result = true;
        }
    }

    lastRedo = redo;

    updateReport( );

    return result;
}
//---------------------------------------------------------------------------
bool Undo::undo( void )
{
	bool result = false;

	if ( undoEnabled_ && undoList_.size( ) && undoPos_ != undoList_.end() )
    {
    	bool linked;

    	do
        {
            int id = (*undoPos_)->id;
            UndoFuncsVector::iterator it = undoFuncs_.begin();

            // get our function ptr
            while ( id )
            {
                it++;
                id--;
            }

            // call the undo callback function
            UndoFunc funcPtr = *(it);
            result = (funcPtr)( (*undoPos_)->opCode, (*undoPos_)->data );

            linked = (*undoPos_)->linked;

            // adjust the undo pointer if successful
            if ( result )
            {
                undoPos_++;
                redoPos_++;
            }
            else
            	linked = false;
        }
        while( linked );
    }

    updateReport( );

    return result;
}
//---------------------------------------------------------------------------
bool Undo::undo( unsigned int id )
{
	UNUSED_PARAMETER( id );

	return false;
}
//---------------------------------------------------------------------------
/*bool Undo::undo( unsigned int index )
{
	return false;
}*/
//---------------------------------------------------------------------------
bool Undo::redo( void )
{
	bool result = false;

	if ( redoEnabled_ && redoList_.size( ) && redoPos_ != redoList_.begin() )
    {
    	bool linked;

    	do
        {
            undoPos_--;
            redoPos_--;

            int id = (*redoPos_)->id;
            UndoFuncsVector::iterator it = undoFuncs_.begin();

            // get our function ptr
            while ( id )
            {
                it++;
                id--;
            }

            // call the undo callback function
            UndoFunc funcPtr = *(it);
            result |= (funcPtr)( (*redoPos_)->opCode, (*redoPos_)->data );

            UndoInfoVector::iterator rp = redoPos_;

            if ( rp != redoList_.begin( ) )
            {
            	rp--;
	            linked = (*rp)->linked;
            }
            else
	            linked = false;
        }
        while ( linked );
    }

    updateReport( );

    return result;
}
//---------------------------------------------------------------------------
bool Undo::redo( int id )
{
	UNUSED_PARAMETER( id );

	return false;
}
//---------------------------------------------------------------------------
void Undo::list( TStrings* stringList )
{
	stringList->Clear();

    AnsiString desc;

	if ( redoEnabled_ && redoList_.size( ) )//&& redoPos_ != redoList_.begin() )
    {
        UndoInfoVector::iterator it = redoList_.begin();

        while ( it != redoPos_ )
        {
        	if ( !(*it)->linked )
            {
            	// show only the unlinked items
	            desc = "R" + (*it)->description;
    	        stringList->Add( desc );
            }

			it++;
        }
    }

	if ( undoEnabled_ && undoList_.size( ) )//&& undoPos_ != undoList_.end() )
    {
        UndoInfoVector::iterator it = undoPos_;

        while ( it != undoList_.end() )
        {
        	if ( !(*it)->linked )
            {
            	// show only the unlinked items
	            desc = "U" + (*it)->description;
    	        stringList->Add( desc );
            }
            
			it++;
        }
    }
}
//---------------------------------------------------------------------------
void Undo::clear( void )
{
	// pop all elements until we reach the current pos
    UndoInfoVector::iterator it = undoList_.begin();
    UndoInfoVector::iterator rt = redoList_.begin();

    while ( undoList_.size() )
    {
        if ( (*it)->data ) delete[] ((*it)->data);
        if ( *it ) delete *it;

        undoList_.pop_front();
        it = undoList_.begin();
    }

    while ( redoList_.size() )
    {
        if ( (*rt)->data ) delete[] ((*rt)->data);
        if ( *rt ) delete *rt;

        redoList_.pop_front();
        rt = redoList_.begin();
    }

    undoPos_ = undoList_.begin();
    redoPos_ = redoList_.begin();

    memoryUsed_ = 0;

    updateReport( );
}
//---------------------------------------------------------------------------
void Undo::setUndoEnable( bool enabled )
{
	// TODO: this was added to force undo/redo on
	enabled = true;

	undoEnabled_ = enabled;

    // cannot use redo if undo is not on
    if ( !undoEnabled_ )
    	redoEnabled_ = false;

    clear( );

    updateReport( );
}
//---------------------------------------------------------------------------
void Undo::setRedoEnable( bool enabled )
{
	// TODO: this was added to force undo/redo on
	enabled = true;

	if ( undoEnabled_ )
    {
    	redoEnabled_ = enabled;

	    clear( );

    	updateReport( );
    }
    else
    {
    	redoEnabled_ = false;
    }

    updateActions( );
}
//---------------------------------------------------------------------------
void Undo::setStepLimit( bool enabled, int steps )
{
	if ( steps < 10 )
    	steps = 10;

	limitSteps_ = enabled;
    undoStepsLimit_ = steps;

    updateReport( );
}
//---------------------------------------------------------------------------
void Undo::setMemoryLimit( bool enabled, int memory )
{
	if ( memory < 4 )
    	memory = 4;

	// memory is in Mbytes, so 1-256 is good
	limitMemory_ = enabled;
    undoMemoryLimit_ = memory * 1048576;

    updateReport( );
}
//---------------------------------------------------------------------------
void Undo::removeLast( void )
{
	if ( undoEnabled_ &&  undoList_.size() )
    {
		// remove the last undo data
    	UndoInfo* ui = undoList_.back();

	    memoryUsed_ -= ui->size;

    	if ( ui->data ) delete[] ui->data;
        if ( ui ) delete ui;
        
	    undoList_.pop_back();

	    // remove the last redo
    	if ( redoEnabled_ && redoList_.size() )
	    {
    		ui = redoList_.back();

        	memoryUsed_ -= ui->size;

	        if ( ui->data ) delete[] ui->data;
            if ( ui ) delete ui;
            
    	    redoList_.pop_back();
	    }
    }

    updateReport( );
}
//---------------------------------------------------------------------------
bool Undo::hasChanged(	bool undo, bool redo )
{
	bool changed = false;

    changed |= undo != undoEnabled_;
    changed |= redo != redoEnabled_;

    return changed;
}
//---------------------------------------------------------------------------
void Undo::updateReport( void )
{
	report_.memoryLimited = limitMemory_;
    report_.memoryUsed = memoryUsed_;
    report_.memoryLimit = undoMemoryLimit_;
    report_.operationsLimited = limitSteps_;
    report_.operationsUsed = undoList_.size();
    report_.operationsLimit = undoStepsLimit_;
}
//---------------------------------------------------------------------------
void Undo::setActions( TAction* undo, TAction* redo )
{
	if ( undo )
		undoAction_ = undo;

    if ( redo )
    	redoAction_ = redo;

    updateActions( );
}
//---------------------------------------------------------------------------
void Undo::updateActions( void )
{
	if ( undoAction_ )
    	undoAction_->Enabled = undoEnabled_;

    if ( redoAction_ )
    	redoAction_->Enabled = redoEnabled_;
}
//---------------------------------------------------------------------------

