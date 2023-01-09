#include "pch.hpp"

#include "physics2/quad_tree.hpp"
#include "math/boundbox.hpp"

BW_BEGIN_NAMESPACE


struct TestMemPoolItem
{
	int anInt_;
	float aFloat_;
	bool aBool_;
};


TEST( QuadTreeMemPool_usage )
{
	//Create some memory pools.
	QuadTreeMemPool<TestMemPoolItem> memPool;
	QuadTreeMemPool<TestMemPoolItem> memPool1;
	QuadTreeMemPool<TestMemPoolItem> memPool2;

	memPool.init();
	memPool1.init();
	memPool2.init();

	//Make sure they are in initial state
	CHECK_EQUAL( 0, memPool.size() );
	CHECK_EQUAL( 0, memPool.capacity() );
	CHECK_EQUAL( reinterpret_cast<TestMemPoolItem*>(0), memPool.data_ );

	CHECK_EQUAL( 0, memPool1.size() );
	CHECK_EQUAL( 0, memPool1.capacity() );
	CHECK_EQUAL( reinterpret_cast<TestMemPoolItem*>(0), memPool1.data_ );

	CHECK_EQUAL( 0, memPool2.size() );
	CHECK_EQUAL( 0, memPool2.capacity() );
	CHECK_EQUAL( reinterpret_cast<TestMemPoolItem*>(0), memPool2.data_ );

	//Fill out the memory pool items with some values we can check later.
	const int maxItems = 16777;
	for( int i = 0; i < maxItems; ++i )
	{
		TestMemPoolItem& newItem = memPool.createNew();
		newItem.anInt_ = i;
		newItem.aFloat_ = static_cast<float>(i);
		newItem.aBool_ = i % 2 == 0;

		TestMemPoolItem& newItem1 = memPool1.createNew();
		newItem1.anInt_ = i;
		newItem1.aFloat_ = static_cast<float>(i);
		newItem1.aBool_ = i % 2 == 0;

		TestMemPoolItem& newItem2 = memPool2.createNew();
		newItem2.anInt_ = i;
		newItem2.aFloat_ = static_cast<float>(i);
		newItem2.aBool_ = i % 2 == 0;
	}

	//Make sure size and capacity are what we expect
	CHECK_EQUAL( maxItems, memPool.size() );
	CHECK_EQUAL( maxItems, memPool1.size() );
	CHECK_EQUAL( maxItems, memPool2.size() );

	CHECK( memPool.capacity() >= memPool.size() );
	CHECK( memPool1.capacity() >= memPool1.size() );
	CHECK( memPool2.capacity() >= memPool2.size() );

	bool sameCapacity = memPool.capacity() == memPool1.capacity();
	sameCapacity &= memPool.capacity() == memPool2.capacity();
	CHECK( sameCapacity );

	//Check the values are the same as those we added
	for( int i = 0; i < maxItems; ++i )
	{
		const TestMemPoolItem& item = memPool[i];
		CHECK_EQUAL( i, item.anInt_ );
		CHECK_EQUAL( static_cast<float>(i), item.aFloat_ );
		CHECK_EQUAL( i % 2 == 0, item.aBool_ );

		const TestMemPoolItem& item1 = memPool1[i];
		CHECK_EQUAL( i, item1.anInt_ );
		CHECK_EQUAL( static_cast<float>(i), item1.aFloat_ );
		CHECK_EQUAL( i % 2 == 0, item1.aBool_ );

		const TestMemPoolItem& item2 = memPool2[i];
		CHECK_EQUAL( i, item2.anInt_ );
		CHECK_EQUAL( static_cast<float>(i), item2.aFloat_ );
		CHECK_EQUAL( i % 2 == 0, item2.aBool_ );
	}

	//Destroy everything
	memPool.destroy();
	memPool1.destroy();
	memPool2.destroy();

	CHECK_EQUAL( reinterpret_cast<TestMemPoolItem*>(0), memPool.data_ );
	CHECK_EQUAL( reinterpret_cast<TestMemPoolItem*>(0), memPool1.data_ );
	CHECK_EQUAL( reinterpret_cast<TestMemPoolItem*>(0), memPool2.data_ );
}


struct TestQuadTreeItem
{
	int anInt_;
	float aFloat_;
	bool aBool_;

	BoundingBox bb_;
	Matrix transform_;
};


//This function is used when objects are added to a quad tree, it
//determines the position and extent of the object
QTRange calculateQTRange( const TestQuadTreeItem & input, const Vector2 & origin,
						 float range, int depth )
{
	const float WIDTH = float(1 << depth);

	BoundingBox bb = input.bb_;
	bb.transformBy( input.transform_ );

	QTRange rv;

	rv.left_	= int(WIDTH * (bb.minBounds().x - origin.x)/range);
	rv.right_	= int(WIDTH * (bb.maxBounds().x - origin.x)/range);
	rv.bottom_	= int(WIDTH * (bb.minBounds().z - origin.y)/range);
	rv.top_		= int(WIDTH * (bb.maxBounds().z - origin.y)/range);

	return rv;
}


TEST( QuadTree_usage )
{
	//Create a quad tree
	const float treeRange = 1000.f;
	const int treeMaxDepth = 5;
	const float treeOffsetX = 0.f;
	const float treeOffsetY = 0.f;
	QuadTree<TestQuadTreeItem> quadTree(
		treeOffsetX, treeOffsetY, treeMaxDepth, treeRange);

	//Quad tree should always have at least the root node.
	CHECK_EQUAL( 1, quadTree.countNodes() );

	//Calculate max number of nodes
	int maxNodes = 0;
	for (int i = 0, levelNodes = 1; i <= treeMaxDepth; ++i )
	{
		maxNodes += levelNodes;
		levelNodes *= 4;
	}

	//
	//Create some test objects. Also try and fill the tree
	//in terms of creating all the possible child nodes.
	//
	const int maxItemsX = 70;
	const int maxItemsZ = 70;
	const int maxItems = maxItemsX*maxItemsZ;
	TestQuadTreeItem* items = new TestQuadTreeItem[maxItems];

	for( int z = 0; z < maxItemsZ; ++z )
	{
		for( int x = 0; x < maxItemsX; ++x )
		{
			int i = z*maxItemsX + x;
			TestQuadTreeItem& newItem = items[i];
			newItem.anInt_ = i;
			newItem.aFloat_ = static_cast<float>(i);
			newItem.aBool_ = i % 2 == 0;

			float xPos = x*(treeRange / (float)maxItemsX);
			float zPos = z*(treeRange / (float)maxItemsZ);

			items[i].transform_.setIdentity();
			items[i].transform_.setTranslate( xPos, 0.f, zPos );
			items[i].bb_.setBounds( Vector3(0.f,0.f,0.f), Vector3(1.f,1.f,1.f) );
		}
	}

	//
	//Adding and deleting at the root. This also tests code that deals
	//with the element arrays.
	//
	quadTree.addToRoot( items[0] );
	size_t numElements = quadTree.getNumElements( quadTree.root() );
	CHECK_EQUAL( 1, numElements );
	bool removed = quadTree.removeFromRoot( items[0] );
	CHECK_EQUAL( true, removed );
	numElements = quadTree.getNumElements( quadTree.root() );
	CHECK_EQUAL( 0, numElements );

	//Add some items
	quadTree.addToRoot( items[0] );
	quadTree.addToRoot( items[1] );
	quadTree.addToRoot( items[2] );

	//Get all the unique ones, which should be what we just added.
	BW::set<const TestQuadTreeItem*> uniqueElements;
	quadTree.getUniqueElements( uniqueElements );
	CHECK_EQUAL( 3, uniqueElements.size() );

	//Check the function getting the number of elements from the root.
	numElements = quadTree.getNumElements( quadTree.root() );
	CHECK_EQUAL( 3, numElements );
	CHECK_EQUAL( &items[0], quadTree.getElement( quadTree.root(), 0 ));
	CHECK_EQUAL( &items[1], quadTree.getElement( quadTree.root(), 1 ));
	CHECK_EQUAL( &items[2], quadTree.getElement( quadTree.root(), 2 ));

	//Delete out of order. The way delete works in the quad tree,
	//the last element in the array is copied over the top of the
	//deleted element.
	quadTree.removeFromRoot( items[1] );
	numElements = quadTree.getNumElements( quadTree.root() );
	CHECK_EQUAL( 2, numElements );
	CHECK_EQUAL( &items[0], quadTree.getElement( quadTree.root(), 0 ));
	CHECK_EQUAL( &items[2], quadTree.getElement( quadTree.root(), 1 ));

	quadTree.removeFromRoot( items[0] );
	numElements = quadTree.getNumElements( quadTree.root() );
	CHECK_EQUAL( 1, numElements );
	CHECK_EQUAL( &items[2], quadTree.getElement( quadTree.root(), 0 ));

	quadTree.removeFromRoot( items[2] );
	numElements = quadTree.getNumElements( quadTree.root() );
	CHECK_EQUAL( 0, numElements );
	CHECK_EQUAL( 1, quadTree.countNodes() );

	//The quad tree does not support deleting of elements from
	//within the rest of the tree. Only dynamic objects are being
	//deleted from the tree in the game but they are only at the
	//root node, which is what the previous tests were doing.

	//
	//Create more than one tree and add objects to them
	//
	QuadTree<TestQuadTreeItem> quadTree1(
		treeOffsetX, treeOffsetY, treeMaxDepth, treeRange);

	QuadTree<TestQuadTreeItem> quadTree2(
		treeOffsetX, treeOffsetY, treeMaxDepth, treeRange);

	//At this point, the first tree is empty again.

	//Adding all the test objects. Create the three trees at the
	//same time. This allows for a different memory allocation
	//pattern as the quad trees grow.
	for( int i = 0; i < maxItems; ++i )
	{
		quadTree.add( items[i] );
		quadTree1.add( items[i] );
		quadTree2.add( items[i] );
	}

	//Check quad tree has all its nodes created.
	//The test objects added should cause all the nodes
	//to get created.
	int numNodes = quadTree.countNodes();
	CHECK( numNodes == maxNodes );
	numNodes = quadTree1.countNodes();
	CHECK( numNodes == maxNodes );
	numNodes = quadTree2.countNodes();
	CHECK( numNodes == maxNodes );

	//Elements might be associated with more than one node.
	//Possible because an element may overlap multiple nodes.
	numElements = quadTree.countElements();
	CHECK( maxItems <= numElements );
	numElements = quadTree1.countElements();
	CHECK( maxItems <= numElements );
	numElements = quadTree2.countElements();
	CHECK( maxItems <= numElements );

	//check each has correct number of elements
	//and that all the items exist in the sets.
	uniqueElements.clear();
	quadTree.getUniqueElements( uniqueElements );
	CHECK_EQUAL( maxItems, uniqueElements.size() );

	for( int i = 0; i < maxItems; ++i )
	{
		BW::set<const TestQuadTreeItem*>::const_iterator found =
			uniqueElements.find( &items[i] );
		CHECK( found != uniqueElements.end() );
	}

	uniqueElements.clear();
	quadTree1.getUniqueElements( uniqueElements );
	CHECK_EQUAL( maxItems, uniqueElements.size() );

	for( int i = 0; i < maxItems; ++i )
	{
		BW::set<const TestQuadTreeItem*>::const_iterator found =
			uniqueElements.find( &items[i] );
		CHECK( found != uniqueElements.end() );
	}

	uniqueElements.clear();
	quadTree2.getUniqueElements( uniqueElements );
	CHECK_EQUAL( maxItems, uniqueElements.size() );

	for( int i = 0; i < maxItems; ++i )
	{
		BW::set<const TestQuadTreeItem*>::const_iterator found =
			uniqueElements.find( &items[i] );
		CHECK( found != uniqueElements.end() );
	}

	delete[] items;
}

BW_END_NAMESPACE