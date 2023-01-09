/*
 * These are the chunk id's in the mfx format, all chunks ids are followed by a total 
 * size which includes size of the chunk and any sub chunks and is then followed by a 
 * size of the chunk, or that is the plan atleast, chunks are good...
 *
 */


#ifndef CHUNKIDS_HPP
#define CHUNKIDS_HPP

//#include "cstdmf/stdmf.hpp"

BW_BEGIN_NAMESPACE

const unsigned int CHUNK_HEADER_SIZE = 12;

typedef unsigned int ChunkID;

const ChunkID CHUNKID_MFX = 'MFX!';

const ChunkID CHUNKID_MATERIALREFERENCELIST = 'MRFL';
const ChunkID CHUNKID_MATERIAL = 'MTRL';
const ChunkID CHUNKID_TEXTURE = 'TXTR';

const ChunkID CHUNKID_MESH = 'MESH';
const ChunkID CHUNKID_MESH2 = 'MES2';
const ChunkID CHUNKID_ENVELOPE = 'ENVL';
const ChunkID CHUNKID_ENVELOPE2 = 'ENV2';
const ChunkID CHUNKID_BONE = 'BONE';
const ChunkID CHUNKID_BONE2 = 'BON2';

const ChunkID CHUNKID_NODE = 'NODE';

const ChunkID CHUNKID_ANIMATION = 'ANIM';

const ChunkID CHUNKID_ANIMATIONCHANNEL = 'ANCH';

const ChunkID CHUNKID_KEYFRAMEMATRIX34 = 'KF34';

const ChunkID CHUNKID_TRIANGLELIST = 'TRIS';
const ChunkID CHUNKID_VERTEXLIST = 'VRTS';
const ChunkID CHUNKID_TEXCOORDLIST = 'TCRS';
const ChunkID CHUNKID_TRIANGLELIST2 = 'TRI2';
const ChunkID CHUNKID_BONEVERTEXLIST = 'BNVL';

const ChunkID CHUNKID_PORTAL = 'PRTL';


const int MFX_FLOAT_SIZE = 4;
const int MFX_INT_SIZE = 4;

const int MFX_POINT_SIZE = MFX_FLOAT_SIZE * 3;
const int MFX_MATRIX_SIZE = MFX_POINT_SIZE * 4;
const int MFX_UV_SIZE = MFX_FLOAT_SIZE * 2;
const int MFX_COLOUR_SIZE = MFX_FLOAT_SIZE * 3;
const int MFX_TRIANGLE_SIZE = MFX_INT_SIZE * 3;
const int MFX_BONEVERTEX_SIZE = MFX_POINT_SIZE + MFX_INT_SIZE;
const int MFX_TRIANGLE2_SIZE = MFX_TRIANGLE_SIZE + MFX_INT_SIZE;

BW_END_NAMESPACE

#endif /*CHUNKIDS_HPP*/
