#include "cstdmf/main_loop_task.hpp"
#include "pyscript/script_math.hpp"
#include "pyscript/script.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This main loop task class handles Player-Only-Traps.
 *
 *	A Pot is an (id, matrix provider, threshold distance, functor).  the
 *	functor is triggered ( 1 = enter, 0 = leave ) when the player
 *	crosses the threshold distance from the matrix provider origin.
 */
class Pots : public MainLoopTask
{
	typedef Pots This;

public:

	static uint32 addPot( MatrixProviderPtr source,
			float dist,
			SmartPointer<PyObject> callbackFunction );
	static void delPot( uint32 id );

	void fini();

	PY_AUTO_MODULE_STATIC_METHOD_DECLARE( RETDATA, addPot,
			NZARG( MatrixProviderPtr, \
			ARG( float, \
			CALLABLE_ARG( SmartPointer<PyObject>, END) ) ) )
	PY_AUTO_MODULE_STATIC_METHOD_DECLARE( RETVOID, delPot, ARG( uint32, END) )

/**
 *	This main loop task class handles Matrix-Traps.
 *
 *	A Mat is an (id, source matrix, target matrix, threshold distance, functor).  the
 *	functor is triggered ( 1 = enter, 0 = leave ) when the source matrix
 *	intersects with the threshold distance from the matrix provider origin.
 *	( i.e. when the source is "on the mat" haha )
 *
 *	The scale factor for the source is used as a bounding area.
 *	If the target matrix is NULL, the current camera is used.
 */
	static uint32 addMat( MatrixProviderPtr source,
			SmartPointer<PyObject> callbackFunction,
			MatrixProviderPtr target = NULL );
	static void delMat( uint32 handle );

	PY_AUTO_MODULE_STATIC_METHOD_DECLARE( RETDATA, addMat,
			ARG( MatrixProviderPtr, \
			CALLABLE_ARG( SmartPointer<PyObject>, \
			OPTARG( MatrixProviderPtr, NULL, END ) ) ) )
	PY_AUTO_MODULE_STATIC_METHOD_DECLARE( RETVOID, delMat, ARG( uint32, END) )

	/// @name Per-frame methods
	//@{
	void tick( float dGameTime, float dRenderTime );
	//@}

private:
	void checkPots();
	void checkMats();

	Pots();
	~Pots();

	class Pot
	{
	public:
		Pot( MatrixProviderPtr a, float b, SmartPointer<PyObject> c, MatrixProviderPtr d = NULL ):
			matrixProvider_(a),
			distanceSq_(b*b),
			function_(c),
			inside_ (false),
			target_(d)
		{
		};

		MatrixProviderPtr		matrixProvider_;
		float					distanceSq_;
		SmartPointer<PyObject>	function_;
		bool					inside_;
		MatrixProviderPtr		target_;
	};
	typedef BW::map< uint32, Pot* > PotMap;
	PotMap					pots_;
	PotMap					mats_;
	uint32					nextHandle_;

	static Pots				s_instance_;
};

BW_END_NAMESPACE

// pots.hpp

