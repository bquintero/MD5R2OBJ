#ifndef _MODEL_
#define _MODEL_

#include <vector>
#include <string>

#include "tokens.h"

enum VertexParamName
{
	VPN_POSITION,
	VPN_NORMAL,
	VPN_TANGENT,
	VPN_BINORMAL,
	VPN_DIFFUSE,
	VPN_TEXCOORD0,
	VPN_TEXCOORD1,
};

enum VertexParamType
{
	FLOAT2,
	FLOAT3,
	FLOAT4,
	UBYTE4,
	VEC11_11_10N, // compressed 32 bit vector
};

struct VertexDecl
{
	VertexParamName name;
	VertexParamType type;

	bool fromString(const char *s, const char *param);
};

struct VertexFormat
{
	std::vector<VertexDecl> decls;

	int getVertexOffset(VertexParamName name, int &index) const;

	int getVertexSize(void) const;

	float* getVector(void *vtxptr, VertexParamName name) const;
};


struct VertexBuffer
{
	VertexFormat format;
	std::vector<unsigned char> data;
	int numVertexes;
};

struct IndexBuffer
{
	int bitdepth;
	std::vector<unsigned char> data;
	int numIndexes;
};

struct PrimBatch
{
	int minIndex; // DrawIndexedTriList MinIndex NumVertices StartIndex PrimitiveCount
	int numVertices;
	int startIndex;
	int primitiveCount;
};

struct Mesh
{
	std::string material;
	int buffers[2]; // vtx, idx
	std::vector<PrimBatch> batches; // is there ever more than PrimBatch[ 1 ] ?
};

struct Model
{
	std::string name;
	std::vector<Mesh> meshes;
};

typedef std::vector<VertexBuffer> VertexBufferArray;
typedef std::vector<IndexBuffer> IndexBufferArray;
typedef std::vector<Model> ModelArray;

bool WolfLoadPROCB(const char *inname, VertexBufferArray &vertexBuffers, IndexBufferArray &indexBuffers, ModelArray &models);


#endif