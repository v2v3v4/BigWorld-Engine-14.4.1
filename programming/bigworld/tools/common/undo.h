//---------------------------------------------------------------------------
//
// How to use Undo.
//
// Setup and adding data
//
//	1. Add a Undo object
//			unsigned int 	addUndoObject( UndoFunc undoFunc );
//		this allows you to control how you process your undo/redo data
//
//	2. call addData to add your undo/redo data to the undo list
//		fill the UndoInfo block with the correct information.
//		Undo will COPY your UndoData. You are responsible for
//		creating and deleting any extra data referenced from
//		the data passed to addData(). 
//
//
// Undo / Redo of data
//
//	1. call undoObject.undo() to do an undo.
//		This will call your undo function for the current undo item in the list.
//		The undo function must now process the undo data you saved as needed.
//
//  2.	The same works for redo.
//
//---------------------------------------------------------------------------
#ifndef undoH
#define undoH
//---------------------------------------------------------------------------
#include "cstdmf/bw_list.hpp"
#include "common_utility.h"
//---------------------------------------------------------------------------
typedef char* UndoData;

typedef struct
{
	unsigned int 	id;
    unsigned int 	opCode;
    AnsiString 		description;
    UndoData		data;
    int				size;
    bool			linked;
} UndoInfo;

typedef struct
{
	bool			memoryLimited;
	unsigned int	memoryUsed;
    unsigned int	memoryLimit;
	bool			operationsLimited;
    unsigned int	operationsUsed;
    unsigned int	operationsLimit;
} UndoReport;

typedef bool(*UndoFunc)(unsigned int, UndoData);

typedef BW::list<UndoInfo*> UndoInfoVector;
typedef BW::list<UndoFunc>  UndoFuncsVector;

class Undo
{
private:

	void 			removeLast( void );		// remove the last undo and redo items

	UndoInfoVector::iterator	undoPos_;	// current position in undo list
	UndoInfoVector::iterator	redoPos_;	// current position in redo list
	UndoFuncsVector				undoFuncs_;	// list of undo functions
    UndoInfoVector	   			undoList_;	// list of undos
    UndoInfoVector	   			redoList_;	// list of redos

    bool			undoEnabled_;			// flag: undo enabled
    bool			redoEnabled_;			// flag: redo enabled
    bool			limitSteps_;			// flag: limit the number of undo steps
    bool			limitMemory_;			// flag: limit the amount of memory used
    bool            inProgress_;            // flag: user flag that is set when they are do an operation

    unsigned int	undoStepsLimit_;		// limit to the number of steps to add
    unsigned long	undoMemoryLimit_;		// limit to the amount of memory to use

    unsigned long	memoryUsed_;			// memory used so far
    UndoReport		report_;

    TAction*		undoAction_;
    TAction*		redoAction_;

	bool 			addData( unsigned int id, unsigned int opCode, AnsiString description, UndoData data, size_t size, bool redo, bool linkToLast );
    void			updateReport( void );
	void 			updateActions( void );

public:
	Undo();
    ~Undo();

	// add an undo object
	unsigned int 	addUndoObject( UndoFunc undoFunc );

    // add undo/redo data
	bool 			addUndoData( unsigned int id, unsigned int opCode, AnsiString description, UndoData data, size_t size, bool linkToLast = false );
	bool 			addRedoData( unsigned int id, unsigned int opCode, AnsiString description, UndoData data, size_t size, bool linkToLast = false );
    //bool			addData( unsigned int id, unsigned int opCode, AnsiString description, UndoData data );

    // undo an the last operation
    bool			undo( void );
    // undo last operation for UndoObject(id)
    bool			undo( unsigned int id );
    // undo a specific operation - indexed into the undo list
    //bool			undo( unsigned int index );
    // redo the last operation
    bool			redo( void );
    // redo the last operation for UndoObject(id)
    bool			redo( int id );

    void			clear( void );

    // list the undo list
    void			list( TStrings* stringList );

    // set the limits
    void			setUndoEnable( bool enabled );
    void			setRedoEnable( bool enabled );
    void			setStepLimit( bool enabled, int steps );
    void			setMemoryLimit( bool enabled, int memory );
    bool			enabled( void ) { return undoEnabled_; };

    // in progress
    bool            isInProgress( void ) { return inProgress_; };
    void            isInProgress( bool state ) { inProgress_ = state; };

    // get the undo report information
    UndoReport*		report( void ) { return &report_; };

    // says whether the settings are different
    bool			hasChanged(	bool undo, bool redo );

    // vcl menu stuff
	void 			setActions( TAction* undo, TAction* redo );
};

//---------------------------------------------------------------------------
#endif
