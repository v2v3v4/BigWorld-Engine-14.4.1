#include "pch.hpp"

#if ENABLE_CONSOLES
#include "profile_graph.hpp"
#include "romp/xconsole.hpp"
#include "moo/geometrics.hpp"
#include "romp/font_manager.hpp"


BW_BEGIN_NAMESPACE

/**
  * ProfileGraph is a class for drawing simple graphs of float arrays, primarily
  * used by the console for profile output.
  *
  * @param topLeft		topleft corner of the graph in clip space
  * @param bottomRight	bottomRight corner of the graph in clip space
  *
  */
ProfileGraph::ProfileGraph(Vector2 topLeft, Vector2 bottomRight)
{
	tl_ = topLeft;
	br_ = bottomRight;
	points_=NULL;
	numGraphs_=0;
	upperLimit_=0;
	maxNumPoints_=0;
	maxVal_=0;;
};


ProfileGraph::~ProfileGraph()
{
	if(points_)
	{
		delete [] points_;
	}
};

/**
  *  @param numPoints  The max number of points that will be graphed.
  *
  */
void ProfileGraph::setNumPoints(int numPoints) 
{
	maxNumPoints_=numPoints;
	if(points_)
	{
		delete [] points_;
	}
	points_ = new Vector2[maxNumPoints_+2];
}

/**
  * Draw a graph. Note this is transitory and needs to be called every frame
  *
  * @param name			The name of the graph. Used in the legend below the graph
  * @param pointData	The float data to be plotted. Y values only.
  * @param offset		defines the first point to be plotted. (buffer is a ring buffer)
  * @param console		console to be rendered into.
  *
  */
void ProfileGraph::drawGraph(const char* name, float* pointData, int offset, XConsole & console)
{

	int i=0;
	float total=0;
	float localMax=0;
	for( ; i < maxNumPoints_; i++)
	{
		float val = pointData[(i + offset) % maxNumPoints_];
		// this is a hack to avoid spikes that occur due to PerfKit integration.
		// artificial spikes are introduced for some reason so I check for massive spikes and
		// replace them with the previous value.
		if(val > 100000)
		{
			val = pointData[( i + offset - 1 ) % maxNumPoints_];
			// set the value as well, just in case the spike is multiple frames in length
			pointData[( i + offset ) % maxNumPoints_ ] = val;
		}

		total += val;
		if(val > localMax)
			localMax = val;
		points_[i].y = (val / upperLimit_) * (tl_.y - br_.y) + br_.y;
		points_[i].x = (i / (float)maxNumPoints_) * (br_.x - tl_.x) + tl_.x;
	}
	// drawLinesInClip requires that the lines end where they start so we
	// add in a path back to the start via the border.
	points_[i++] = br_;
	points_[i].x = tl_.x;
	points_[i].y = br_.y;

	Geometrics::drawLinesInClip(points_, maxNumPoints_ + 2, ProfileGraph::getColour(numGraphs_));

	// draw the legend
	Moo::rc().setPixelShader( NULL );
	if ( FontManager::instance().begin(*console.font()) )
	{
		char buffer[256];
		sprintf(buffer, "%20s : %6.2fms \t[%6.2fms]", name, total / (float)maxNumPoints_, localMax);
		Vector2 fontSize = console.font()->screenCharacterSize();
		console.font()->colour(ProfileGraph::getColour(numGraphs_));
		console.font()->drawStringInClip(buffer,Vector3(tl_.x, br_.y - ((numGraphs_ * (fontSize.y) + 2.0f) / (float)Moo::rc().halfScreenHeight()),0));
		FontManager::instance().end();
	}

	if(localMax>maxVal_)
	{
		maxVal_ = localMax;
	}
	numGraphs_++;
}

/**
  * Draws a border around the graphs that have been drawn and finishes the 
  * drawing of this set of graphs. Subsequent drawGraph calls will be in a 
  * new graph (drawBorder() should only be called after rendering all graphs.
  *
  */
void ProfileGraph::drawBorder( XConsole & console)
{
	// draw the graph's border
	float points[8] = 
	{
		tl_.x, tl_.y, 
		br_.x, tl_.y, 
		br_.x, br_.y,
		tl_.x, br_.y
	};


	Geometrics::drawLinesInClip((Vector2*)points, 4, s_colours[0]); // border

	const float oneFrame = 1000.0f / 60.0f;
	int i = 1;
	int step = (int)upperLimit_ / 300 + 1;

	// draw in the scale lines every 16.6ms
	for(int i = step; i < (int(maxVal_ / (oneFrame)) + 1); i += step)
	{
		Vector2 line[4];
		line[0].x = tl_.x;
		line[1].x = br_.x;

		// 60fps line
		line[0].y = line[1].y = (oneFrame * i / upperLimit_) * (tl_.y - br_.y) + br_.y;
		line[2] = line[3] = line[0];

		Geometrics::drawLinesInClip((Vector2*)line, 3, Moo::Colour(1,0,1,0)); // border
		console.font()->colour(0xffffff00); // yellow

		Moo::rc().setPixelShader( NULL );
		// render the y axes labels
		if ( FontManager::instance().begin(*console.font()) )
		{
			char buffer[256];
			sprintf(buffer, "%2.2fms", i*oneFrame);
			console.font()->drawStringInClip( buffer, Vector3(line[1].x + 0.02f, line[1].y + 0.02f, 0));
			FontManager::instance().end();
		}
	}
	// calculate the optimum scale for the graph
	upperLimit_ = (int(maxVal_ / oneFrame) + 1) * oneFrame;

	maxVal_=0;
	numGraphs_=0;
}

BW_END_NAMESPACE
#endif // ENABLE_CONSOLES
// profile_graph.cpp

