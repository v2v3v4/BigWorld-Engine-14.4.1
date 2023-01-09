#include "pch.hpp"
#include "quad_tree.hpp"


// -----------------------------------------------------------------------------
// Section: Testing
// -----------------------------------------------------------------------------

// #define TEST_QUAD

#ifdef TEST_QUAD

#include "math/vector2.hpp"

DECLARE_DEBUG_COMPONENT2( "Physics", 0 )

BW_BEGIN_NAMESPACE

/**
 *	This class is used to implement a test QuadTree.
 */
class TestTreeDesc
{
public:
	static const int DEPTH = 5;
	static const float RANGE;
};

const float TestTreeDesc::RANGE = 100.f;


/**
 *	This class is the type of object that is going to go in the test Quadtree.
 */
class TestElement
{
public:
	TestElement() :
		x_(),
		y_(),
		visited_( false )
	{}

	const Vector2 & minBounds() const	{ return minBounds_; }
	const Vector2 & maxBounds() const	{ return maxBounds_; }

	bool mark() const	{ return false; }

	Vector2 minBounds_;
	Vector2 maxBounds_;

	int x_;
	int y_;

	bool visited_;
};

// The test QuadTree.
typedef QuadTree< TestElement, TestTreeDesc > TestTree;

QTRange calculateQTRange( const TestElement & input, const Vector2 & origin,
						 float range, int depth )
{
	const float WIDTH = float(1 << depth);

	const Vector2 & minBounds = input.minBounds();
	const Vector2 & maxBounds = input.maxBounds();

	QTRange rv;

	rv.left_	= int(WIDTH * (minBounds.x - origin.x)/range);
	rv.right_	= int(WIDTH * (maxBounds.x - origin.x)/range);
	rv.bottom_	= int(WIDTH * (minBounds.y - origin.y)/range);
	rv.top_		= int(WIDTH * (maxBounds.y - origin.y)/range);

#if 0
	dprintf( "Adding (%f, %f), (%f, %f) - (%d, %d), (%d, %d)\n",
		bb.minBounds().x, bb.minBounds().z,
		bb.maxBounds().x, bb.maxBounds().z,
		rv.left_, rv.bottom_, rv.right_, rv.top_ );
#endif

	return rv;
}

bool containsTestPoint( const TestElement & input, const Vector3 & point )
{
	return true;
}


void printQTNode( const QuadTreeNode<TestElement> & node, const char * prefix )
{
	QuadTreeNode<TestElement>::Elements::const_iterator iter =
		node.elements().begin();

	while (iter != node.elements().end())
	{
		dprintf( "%s(%d, %d) 0x%08x\n", prefix,
			(*iter)->x_, (*iter)->y_, *iter );

		iter++;
	}
}

void printTraverse( TestTree & tree, const Vector3 & src, const Vector3 & dst )
{
	dprintf( "\nTree - (%f, %f) to (%f, %f)\n",
		src.x, src.z, dst.x, dst.z );
	TestTree::Traversal traverse = tree.traverse( src, dst );

	const TestElement * pElement;

	while ((pElement = traverse.next()) != NULL)
	{
		dprintf( "(%d, %d)\n", pElement->x_, pElement->y_ );
	}
}

void testQuadTree()
{
	Vector3 origin( 0.f, 0.f, 0.f );
	Vector2 origin2d( origin.x, origin.z );
	TestTree tree( origin.x, origin.z );
	const int WIDTH = (1 << TestTreeDesc::DEPTH);
	const float RANGE = TestTreeDesc::RANGE;
	const float CELL_SIZE = RANGE/WIDTH;
	TestElement elements[ WIDTH ][ WIDTH ];

	const Vector2 cellSize( CELL_SIZE - 0.001f, CELL_SIZE - 0.001f);

	for (int j = 0; j < WIDTH; j++)
	{
		for (int i = 0; i < WIDTH; i++)
		{
			TestElement & curr = elements[i][j];
			curr.x_ = i;
			curr.y_ = j;

			curr.minBounds_.set( i * CELL_SIZE, j * CELL_SIZE );
			curr.maxBounds_ = curr.minBounds_ + cellSize;

			curr.minBounds_ += origin2d;
			curr.maxBounds_ += origin2d;

			tree.add( curr );
		}
	}

	tree.print();

	printTraverse( tree, Vector3( 30.f, 0.f, 0.f ), Vector3( 30.f, 0.f, 100.f ) );
	printTraverse( tree, Vector3( 0.f, 0.f, 30.f ), Vector3( 100.f, 0.f, 30.f ) );
	printTraverse( tree, Vector3( 30.0001f, 0.f, 30.f ), Vector3( 30.f, 0.f, 30.f ) );
	return;

	Vector3 src( 0.f, 0.f, -1.f );
	Vector3 dst( 100.f, 0.f, 99.f );

	src.set( 30.f, 0.f, 30.f );
	dst.set( 30.f, 0.f, 30.f );

	src += origin;
	dst += origin;

	printTraverse( tree, src, dst );
	printTraverse( tree, dst, src );

//	src.set( 0.f, 0.f, 71.f );
//	dst.set( 70.f, 0.f, 1.f );
	src.set( 0.f, 0.f, 101.f );
	dst.set( 100.f, 0.f, 1.f );

	src += origin;
	dst += origin;

//	printTraverse( tree, src, dst );
//	printTraverse( tree, dst, src );

	src += Vector3( 0.f, 0.f, 50.f );
	dst += Vector3( 0.f, 0.f, 50.f );

	printTraverse( tree, src, dst );
	printTraverse( tree, dst, src );
}


void testQuadTree( const Vector3 & src, const Vector3 & dst, float radius = 0.f )
{
	TestTree tree( 0.f, 0.f );
	const int WIDTH = (1 << TestTreeDesc::DEPTH);
	const float RANGE = TestTreeDesc::RANGE;
	const float CELL_SIZE = RANGE/WIDTH;
	TestElement elements[ WIDTH ][ WIDTH ];

	const Vector2 cellSize( CELL_SIZE - 0.001f, CELL_SIZE - 0.001f);

	for (int j = 0; j < WIDTH; j++)
	{
		for (int i = 0; i < WIDTH; i++)
		{
			TestElement & curr = elements[i][j];
			curr.x_ = i;
			curr.y_ = j;

			curr.minBounds_.set( i * CELL_SIZE, j * CELL_SIZE );
			curr.maxBounds_ = curr.minBounds_ + cellSize;

			tree.add( curr );
		}
	}

	TestTree::Traversal traverse = tree.traverse( src, dst, radius );

	const TestElement * pElement;

	while ((pElement = traverse.next()) != NULL)
	{
		const_cast<TestElement *>(pElement)->visited_ = true;
	}

	for (int y = WIDTH - 1; y >= 0; y--)
	{
		for (int x = 0; x < WIDTH; x++)
		{
			dprintf( "%c", elements[x][y].visited_ ? '#' : 'O' );
		}

		dprintf( "\n" );
	}
}

BW_END_NAMESPACE

#endif // TEST_QUAD
