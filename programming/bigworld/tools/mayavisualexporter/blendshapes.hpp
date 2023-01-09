#ifndef __blendshapes_hpp__
#define __blendshapes_hpp__

BW_BEGIN_NAMESPACE

class BlendShapes
{
public:
	BlendShapes();
	virtual ~BlendShapes();

	struct Object
	{
		BW::string name;
		BW::vector< vector3<float> > vertices;
		BW::vector< float > weights; // one for each frame, sequential
	};
	
	typedef BW::vector< Object > Objects;

	uint32 count();

	bool hasBlendShape( MObject& object );

	// implementation is inefficient, may need to be optimised (initialises each blend shape in turn)
	// NOTE: leaves the blend shape object in a finalised state
	bool isBlendShapeTarget( MObject& object );
	
	// initialise blend shape index
	bool initialise( uint32 index );

	// initialise blend shape for MObject, returns false if no Blendshape on object
	bool initialise( MObject& object );

	// free up any memory used by the blend shapes
	void finalise();

	// name of the current blend shape
	BW::string name();

	uint32 numBases();
	uint32 numTargets( uint32 base );
	
	uint32 countTargets();

	// get a reference to the base vertex positions
	Object& base( uint32 base );
	
	// get a reference to a target vertex positions
	Object& target( uint32 base, uint32 index );
	
	// returns a delta for vertex index between the base and target
	vector3<float> delta( uint32 base, uint32 target, uint32 index );

	// required to capture original mesh targets
	static void disable();
	
	// enable should be called when finished with blend shapes
	static void enable();

protected:
	BW::vector<MObject> _blendShapes;

	// name of the currently initialised blend shape
	BW::string _name;

	// first is base index, then the vertices
	Objects _bases;

	// first is base index, then target index, the vertices
	BW::vector< Objects > _targets;
 };

BW_END_NAMESPACE

#endif // __blendshapes_hpp__
