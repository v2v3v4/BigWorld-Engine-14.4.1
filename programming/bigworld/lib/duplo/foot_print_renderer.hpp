#ifndef FOOT_PRINT_RENDERER_HPP
#define FOOT_PRINT_RENDERER_HPP

#include "math/vector3.hpp"


BW_BEGIN_NAMESPACE

typedef uint32 ChunkSpaceID;

/**
 *	Stores and renders foot prints.
 */
class FootPrintRenderer
{
public:
	FootPrintRenderer(ChunkSpaceID spaceID);
	~FootPrintRenderer();

	void addFootPrint(const Vector3 * vertices);
	void draw();

	static void init();
	static void fini();

	static void setFootPrintOption(int selectedOption);

private:
	int spaceID_;
};

BW_END_NAMESPACE

#endif // FOOT_PRINT_RENDERER_HPP
