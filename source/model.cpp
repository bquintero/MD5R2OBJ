#include "model.h"

static int decl_sizes[] = { sizeof(float)*2, sizeof(float)*3, sizeof(float)*4, 4, 4 };

static std::pair<const char*,VertexParamType> decl_names[] = 
{
	std::pair<const char*,VertexParamType>("Position", FLOAT3),
	std::pair<const char*,VertexParamType>("Normal", FLOAT3), 
	std::pair<const char*,VertexParamType>("Tangent", FLOAT3),
	std::pair<const char*,VertexParamType>("Binormal", FLOAT3),
	std::pair<const char*,VertexParamType>("DiffuseColor", UBYTE4),
	std::pair<const char*,VertexParamType>("TexCoord", FLOAT2),
	std::pair<const char*,VertexParamType>("", UBYTE4)
};

static std::pair<const char*,VertexParamType> decl_types[] =
{
	std::pair<const char*,VertexParamType>("2", FLOAT2),
	std::pair<const char*,VertexParamType>("3", FLOAT3),
	std::pair<const char*,VertexParamType>("4", FLOAT4),
	std::pair<const char*,VertexParamType>("Dec_11_11_10N", VEC11_11_10N),
	std::pair<const char*,VertexParamType>("", VEC11_11_10N)
};

bool VertexDecl::fromString(const char *s, const char *param)
{
	for(int i = 0; *decl_names[i].first != 0; i++)
	{
		if( !_stricmp(decl_names[i].first, s) )
		{
			name = (VertexParamName)i;
			type = decl_names[i].second;

			for(int i = 0; *decl_types[i].first != 0; i++)
			{
				if( !_stricmp(decl_types[i].first, param) )
				{
					type = decl_types[i].second;
					break;
				}
			}

			return true;
		}
	}

	return false;
}

int VertexFormat::getVertexSize(void) const
{
	int total = 0;
	for(size_t i = 0; i < decls.size(); i++)
	{
		total += decl_sizes[ decls[i].type ];		
	}

	return total;
}

int VertexFormat::getVertexOffset(VertexParamName name, int &index) const
{
	int total = 0;

	for(index = 0; index < (int)decls.size(); index++)
	{
		if( decls[index].name == name )
			return total;

		total += decl_sizes[ decls[index].type ];		
	}

	return -1;
}

float* VertexFormat::getVector(void *vtxptr, VertexParamName name) const
{
	int offset, index;
	
	offset = getVertexOffset(name, index);

	if(offset >= 0 && index < (int)decls.size())
	{
		if( decls[index].type == FLOAT2 || decls[index].type == FLOAT3 || decls[index].type == FLOAT4 )
		{
			return (float*)(((char*)vtxptr) + offset);
		}
		else if( decls[index].type == VEC11_11_10N )
		{
			static float norm[3];
			int value = *(int*)(((char*)vtxptr) + offset);
			norm[0] = (((value & 0x7FE00000) >> 21) / (float)(0x3FF-1)); *(int*)&norm[0] |= ((value & 0x80000000) << 00);
			norm[1] = (((value & 0x000FFC00) >> 10) / (float)(0x3FF-1)); *(int*)&norm[1] |= ((value & 0x00100000) << 11);
			norm[2] = (((value & 0x000001FF) >> 00) / (float)(0x1FF-1)); *(int*)&norm[2] |= ((value & 0x00000200) << 22);
			return norm;
		}
	}

	return NULL;
}

int WolfParseCount(TokenParser &parser)
{
	char token[1024];
	int count;

	parser.getToken(token); // skip '['
	parser.getToken(token);
	count = atoi(token);
	parser.getToken(token); // skip ']'

	return count;
}

int WolfParseBinary(std::vector<unsigned char> &data, const char *basepath, TokenParser &parser)
{
	TokenParser line;
	char linedata[2048], token[2048];
	int numElements;
	int count;

	numElements = WolfParseCount(parser);

	parser.getToken(token); // skip '{'
	while( parser.getToken(token) && token[0] != '}' )
	{
		if( !_stricmp(token, "file") )
		{
			char *buffer;

			parser.getToken(token);
			sprintf_s(linedata, "%s%s", basepath, token);
			count = readfile(linedata, buffer);

			data.resize(count);
			for(size_t i = 0; i < data.size(); i++)
			{
				data[i] = buffer[i];
			}
					
			freefile(buffer);
		}
		else if( !_stricmp(token, "bin") )
		{
			line.initialize(linedata, parser.getLine(linedata), "", NULL);
			line.getTokenInt(count);

			data.resize(count);
			parser.getBytes(&data.at(0), data.size());
		}
	}

	return numElements;
}

bool WolfParseVertexBuffer(char *basepath, TokenParser &parser, VertexBufferArray &vertexBuffers)
{
	TokenParser line;
	char linedata[2048], token[2048];
	std::vector<std::string> args;
	VertexBuffer vb;

	vb.numVertexes = 0;

	while( parser.getToken(token) && token[0] != '}' )
	{
		if( !_stricmp(token, "VertexFormat") || !_stricmp(token, "LoadVertexFormat") )
		{
			args.clear();
			line.initialize(linedata, parser.getLine(linedata), "{}", NULL);

			line.getToken(token); // skip '{'
			while( line.getToken(token) && token[0] != '}' )
			{
				args.push_back(token);
			}

			vb.format.decls.clear();

			for(int i = 0; i < (int)args.size()-1; i++)
			{
				VertexDecl decl;

				if( decl.fromString(args[i].c_str(), args[i+1].c_str()) )
				{
					vb.format.decls.push_back(decl);
				}
			}
		}
		else if( !_stricmp(token, "Vertex") ) // Vertex[ ## ]
		{
			vb.numVertexes = WolfParseBinary(vb.data, basepath, parser);
		}
		else
		{
			// skip this line...
			parser.getLine(linedata);
		}
	}

	if( !vb.format.decls.empty() && vb.numVertexes > 0 )
	{
		vertexBuffers.push_back(vb);
		return true;
	}

	return false;
}

bool WolfParseIndexBuffer(char *basepath, TokenParser &parser, IndexBufferArray &indexBuffers)
{
	TokenParser line;
	char linedata[2048], token[2048];
	std::vector<std::string> args;
	IndexBuffer ib;

	ib.bitdepth = 0;
	ib.numIndexes = 0;

	while( parser.getToken(token) && token[0] != '}' )
	{
		if( !_stricmp(token, "BitDepth") )
		{
			parser.getTokenInt(ib.bitdepth);
		}
		else if( !_stricmp(token, "Index") ) // Index[ ## ]
		{
			ib.numIndexes = WolfParseBinary(ib.data, basepath, parser);
		}
		else
		{
			// skip this line...
			parser.getLine(linedata);
		}
	}

	if( !ib.data.empty() && ib.bitdepth > 0 && ib.numIndexes > 0)
	{
		indexBuffers.push_back(ib);
		return true;
	}

	return false;
}

bool WolfParseMeshes(const std::string &name, TokenParser &parser, std::vector<Mesh> &meshes)
{
	TokenParser line;
	char linedata[2048], token[2048];
	std::vector<std::string> args;
	int count;

	count = WolfParseCount(parser);
	parser.getToken(token); // skip '{'
	for(int i = 0; i < count; i++)
	{
		Mesh mesh;

		mesh.material = "";
		mesh.buffers[0] = -1;
		mesh.buffers[1] = -1;
		mesh.batches.clear();

		parser.getToken(token); // Mesh
		parser.getToken(token); // skip '{'
				
		while( parser.getToken(token) && token[0] != '}' )
		{
			if( !_stricmp(token, "Material") )
			{
				parser.getToken(token);
				mesh.material = token;
			}
			else if( !_stricmp(token, "DrawBuffers") )
			{
				parser.getTokenInt(mesh.buffers[0]);
				parser.getTokenInt(mesh.buffers[1]);
			}
			else if( !_stricmp(token, "PrimBatch") )
			{
				int batchCount = WolfParseCount(parser);

				parser.getToken(token); // skip '{'
				for(int j = 0; j < batchCount; j++)
				{
					parser.getToken(token); // PrimBatch
					parser.getToken(token); // skip '{'

					while( parser.getToken(token) && token[0] != '}' )
					{
						if( !_stricmp(token, "DrawIndexedTriList") )
						{
							PrimBatch batch;

							parser.getTokenInt(batch.minIndex);
							parser.getTokenInt(batch.numVertices);
							parser.getTokenInt(batch.startIndex);
							parser.getTokenInt(batch.primitiveCount);
									
							mesh.batches.push_back(batch);
						}
						// Transform[ ## ] is for animated models??
						else if( !_stricmp(token, "Transform") )
						{
							WolfParseCount(parser);
							parser.skipBraces();
						}
						else
						{
							// skip unused data
							parser.getLine(token);
						}
					}
				}
				parser.getToken(token); // skip '}'
			}
			else
			{
				// skip this line...
				parser.getLine(linedata);
			}
		}

		if( !mesh.batches.empty() && mesh.buffers[0] >= 0 && mesh.buffers[1] >= 0 )
		{
			printf("   mesh[%d]...%s\n", i, mesh.material.c_str());
			meshes.push_back(mesh);
		}
		// precomputes shadow models don't print errors
		else if( !strstr(name.c_str(), "_prelight") )
		{
			printf("could not complete : %s.Mesh[ %d ] (mtl: %s)\n", name.c_str(), i, mesh.material.c_str());
		}
	}
	parser.getToken(token); // skip '}'

	return true;
}

bool WolfParseModel(char *basepath, TokenParser &parser, ModelArray &models)
{
	char token[2048];
	Model mdl;

	parser.getToken(token);
	mdl.name = token;

	if( !strstr(mdl.name.c_str(), "_prelight") )
	{
		printf("   model: %s\n", mdl.name.c_str());
	}

	parser.getToken(token); // skip '{'
	while( parser.getToken(token) && token[0] != '}' )
	{
		if( !_stricmp(token, "Mesh") )
		{
			WolfParseMeshes(mdl.name, parser, mdl.meshes);
		}
		else
		{
			// skip this line...
			parser.getLine(token);
		}
	}

	if( !mdl.meshes.empty() )
	{
		models.push_back( mdl );
	}

	return true;
}

bool WolfLoadPROCB(const char *inname, VertexBufferArray &vertexBuffers, IndexBufferArray &indexBuffers, ModelArray &models)
{
	TokenParser parser;
	char *data, token[2048];
	char basepath[256];
	int length, count;

	if((length = readfile(inname, data)) == 0 || !data)
	{
		printf("failed to load %s\n", inname);
		return false;
	}

	strcpy_s(basepath, inname);
	count = (int)strlen(basepath);
	while( --count >= 0 )
	{
		if( basepath[count] == '/' || basepath[count] == '\\' )
			break;
	}
	basepath[count+1] = 0;

	printf("parsing %s...\n", inname);
	parser.initialize(data, length, "[]{}", 0);

	// skip past all of the header information.. though it may be useful to parse that in the future...
	parser.getLine(token); // MemSize ###
	parser.getLine(token); // MD5RProcVersion 7
	parser.getLine(token); // ### hash?

	if( !parser.bytesRemaining() )
		return false;

	while( parser.bytesRemaining() && parser.getToken(token) )
	{
		printf("Parsing %s...\n", token);

		// Joint[ ## ]
		if( !_stricmp(token, "Joint") )
		{
			count = WolfParseCount(parser);

			parser.skipBraces();
			//parser.getToken(token); // skip '{'
			//for(int i = 0; i < count && parser.bytesRemaining(); i++ )
			//{
			//	parser.getToken(token); // VertexBuffer
			//	parser.getToken(token); // '{'
			//	if( !WolfParseVertexBuffer(basepath, parser, vertexBuffers) )
			//		return false;
			//}
			//parser.getToken(token); // skip '}'
		}
		// LevelOfDetail[ ## ]
		else if( !_stricmp(token, "LevelOfDetail") )
		{
			count = WolfParseCount(parser);
			parser.skipBraces();
		}
		// VertexBuffer[ ## ]
		else if( !_stricmp(token, "VertexBuffer") )
		{
			count = WolfParseCount(parser);

			parser.getToken(token); // skip '{'
			for(int i = 0; i < count && parser.bytesRemaining(); i++ )
			{
				parser.getToken(token); // VertexBuffer
				parser.getToken(token); // '{'
				if( !WolfParseVertexBuffer(basepath, parser, vertexBuffers) )
					return false;
			}
			parser.getToken(token); // skip '}'
		}
		// IndexBuffer[ ## ]
		else if( !_stricmp(token, "IndexBuffer") )
		{
			count = WolfParseCount(parser);

			parser.getToken(token); // skip '{'
			for(int i = 0; i < count && parser.bytesRemaining(); i++ )
			{
				parser.getToken(token); // IndexBuffer
				parser.getToken(token); // '{'
				if( !WolfParseIndexBuffer(basepath, parser, indexBuffers) )
					return false;
			}
			parser.getToken(token); // skip '}'
		}
		// SilhouetteEdge[ ## ]
		else if( !_stricmp(token, "SilhouetteEdge") )
		{
			// don't care but need to property parse it to reach the model information
			std::vector<unsigned char> data;
			WolfParseBinary(data, basepath, parser);
		}
		// Model[ ## ]
		else if( !_stricmp(token, "Model") )
		{
			count = WolfParseCount(parser);

			parser.getToken(token); // skip '{'
			for(int i = 0; i < count && parser.bytesRemaining(); i++ )
			{
				parser.getToken(token); // Model
				if( !WolfParseModel(basepath, parser, models) )
					return false;
			}
			parser.getToken(token); // skip '}'
		}
		// Mesh[ ## ]
		else if( !_stricmp(token, "Mesh") )
		{
			Model mdl;

			mdl.name = inname;
			if( WolfParseMeshes(mdl.name, parser, mdl.meshes) )
			{
				models.push_back(mdl);
			}
		}
		else if( !_stricmp(token, "Bounds") )
		{
			// skip unused data...
			parser.getLine(token);
		}
		else if( !_stricmp(token, "portalAreas") || !_stricmp(token, "interAreaPortals") )
		{
			// skip unused data...
			parser.skipBraces();
		}
		else if( !_stricmp(token, "nodes") )
		{
			int numNodes;

			// skip unused data...
			parser.getToken(token); // skip '{'
			parser.getTokenInt(numNodes);
			parser.getToken(token);

			if( !_stricmp(token, "bin") )
			{
				TokenParser line;
				line.initialize(token, parser.getLine(token), "", NULL);
				line.getTokenInt(count);
				parser.skipBytes(count);
			}
			parser.getToken(token); // skip '}'
		}
		else
		{
			break;
		}
	}

	freefile(data);

	return true;
}
