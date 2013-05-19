#include <stdio.h>
#include <string.h>
#include <direct.h>
#include "tokens.h"
#include "model.h"

void replaceext(char *s, size_t size, const char *ext)
{
	char *token;

	if((token = strrchr(s, '.')) != NULL)
	{
		int length = size - (int)(token - s) - (int)strlen(ext);
		strcpy_s(token, length, ext);
	}
}

int main(int argc, char **argv)
{
	char inname[256];
	char outname[256];

	if( argc <= 1 )
	{
		printf("ex: wolf2obj <name>.procb\n");
		printf("ex: wolf2obj <name>.md5r\n");
		return -1;
	}

	strcpy_s(inname, argv[1]);

	_getcwd(outname, sizeof(outname));
	printf("cwd: %s\n", outname);

	VertexBufferArray vertexBuffers;
	IndexBufferArray indexBuffers;
	ModelArray models;

	if( !WolfLoadPROCB(inname, vertexBuffers, indexBuffers, models) )
	{
		printf("an error occured while reading %s...\n", inname);
	}

	// each vertex buffer will export all meshes that are using it
	for(size_t vidx = 0; vidx < vertexBuffers.size(); vidx++)
	{
		if( vertexBuffers[ vidx ].data.empty() )
			continue;

		strcpy_s(outname, inname);
		replaceext(outname, sizeof(outname), "");
		sprintf_s(outname, "%s_%d", outname, vidx);
		strcat_s(outname, ".obj");

		printf("writing %s...\n", outname);
		FILE *fp;

		if( fopen_s(&fp, outname, "wb") )
		{
			printf("failed to create %s...\n", outname);
			break;
		}

		int vertexSize = vertexBuffers[ vidx ].format.getVertexSize();
		for(int i = 0; i < vertexBuffers[ vidx ].numVertexes; i++)
		{
			void *vtxptr = &vertexBuffers[ vidx ].data[i * vertexSize];

			float *pos = vertexBuffers[ vidx ].format.getVector(vtxptr, VPN_POSITION);
			fprintf(fp, "v %f %f %f\n", pos[0], pos[1], pos[2]);

			float *norm = vertexBuffers[ vidx ].format.getVector(vtxptr, VPN_NORMAL);
			if( norm ) fprintf(fp, "vn %f %f %f\n", norm[0], norm[1], norm[2]);

			float *st = vertexBuffers[ vidx ].format.getVector(vtxptr, VPN_TEXCOORD0);
			if( st ) fprintf(fp, "vt %f %f\n", st[0], st[1]);
		}

		int totaltris = 0;

		for(size_t i = 0; i < models.size(); i++)
		{
			fprintf(fp, "g %s\n", models[i].name.c_str());
		
			for(size_t j = 0; j < models[i].meshes.size(); j++)
			{
				const IndexBuffer &ib = indexBuffers[ models[i].meshes[j].buffers[1] ];

				if( models[i].meshes[j].buffers[0] != vidx )
					continue;

				fprintf(fp, "usemtl %s\n", models[i].meshes[j].material.c_str());

				const PrimBatch &batch = models[i].meshes[j].batches[0];
				for(int k = 0; k < batch.primitiveCount; k++)
				{
					int base = batch.startIndex + (k * 3);

					totaltris++;

					if( ib.bitdepth == 8 ) 
					{
						fprintf(fp, "f %d %d %d\n", 
							*(unsigned char*)&ib.data[(base+0)*ib.bitdepth/8] + 1, 
							*(unsigned char*)&ib.data[(base+1)*ib.bitdepth/8] + 1, 
							*(unsigned char*)&ib.data[(base+2)*ib.bitdepth/8] + 1);
					}
					else if( ib.bitdepth == 16 ) 
					{
						fprintf(fp, "f %d %d %d\n", 
							*(unsigned short*)&ib.data[(base+0)*ib.bitdepth/8] + 1, 
							*(unsigned short*)&ib.data[(base+1)*ib.bitdepth/8] + 1, 
							*(unsigned short*)&ib.data[(base+2)*ib.bitdepth/8] + 1);
					}
					else if( ib.bitdepth == 32 ) 
					{
						fprintf(fp, "f %d %d %d\n", 
							*(unsigned int*)&ib.data[(base+0)*ib.bitdepth/8] + 1, 
							*(unsigned int*)&ib.data[(base+1)*ib.bitdepth/8] + 1, 
							*(unsigned int*)&ib.data[(base+2)*ib.bitdepth/8] + 1);
					}
				}				
			}
		}

		fclose(fp);

		printf("triangles exported: %d\n", totaltris);
	}

	return 0;
}