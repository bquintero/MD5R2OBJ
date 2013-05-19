#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "tokens.h"

int readfile(const char *fname, char *& buffer)
{
	size_t length;
	FILE *fp;

	buffer = NULL;
	length = 0;

	if( fopen_s(&fp, fname, "rb") == 0 )
	{
		fseek(fp, 0, SEEK_END);
		length = ftell(fp);
		fseek(fp, 0, SEEK_SET);
		buffer = new char[length + 1];
		fread(buffer, 1, length, fp);
		buffer[length] = 0;

		fclose(fp);
	}

	return length;
}

void freefile(char *& data)
{
	delete [] data;
	data = NULL;
}

TokenParser::TokenParser()
{
	whitespace = 0;
	specials = 0;
	base = cursor = 0;
	length = 0;
}

void TokenParser::initialize(char *ptr, int len, char *spchars, char *wsp)
{
	whitespace = (wsp) ? wsp : " \t\n\r";
	specials = spchars;
	base = cursor = ptr;
	length = len;
}

void TokenParser::skipWhitespace(void)
{
	for(; bytesRemaining() > 0; cursor++)
	{
		// skip whitespace
		if( strchr(whitespace, cursor[0]) )
			continue;

		break;
	}
}

bool TokenParser::getTokenInt(int &value)
{
	char buffer[128];

	if( !getToken(buffer) )
		return false;

	value = atoi(buffer);

	return true;
}

int TokenParser::getToken(char *buffer)
{
	int nchars;

	// skip leading white space
	skipWhitespace();

	// find a valid token
	for(nchars = 0; bytesRemaining() > 0; cursor++)
	{
		// skip cpp-style comments
		if( cursor[0] == '/' && cursor[1] == '/' )
		{
			if( nchars != 0 )
				break;

			for(;bytesRemaining() > 0; cursor++)
			{
				if( cursor[0] == '\n' )
				{
					break;
				}
			}

			// more white space can appear after a comment, so clear it out
			skipWhitespace();
			cursor--;
			continue;
		}

		// whitespace is the end of this token
		if( strchr(whitespace, cursor[0]) )
		{
			// we should have SOMETHING by this point..
			assert(nchars != 0);
			break;
		}

		// special characters are treated as terminators
		if( strchr(specials, cursor[0]) )
		{
			if( nchars == 0 )
			{
				buffer[nchars] = cursor[0];
				cursor++;
				nchars++;
			}
			break;
		}

		// data in quotes are treated as raw text
		if( cursor[0] == '"' )
		{
			cursor++;
			for(;bytesRemaining() > 0; cursor++)
			{
				if( cursor[0] == '"' )
				{
					cursor++;
					break;
				}
				buffer[nchars] = cursor[0];
				nchars++;
			}
			break;
		}

		buffer[nchars] = cursor[0];
		nchars++;
	}

	buffer[nchars] = 0;
	return nchars;
}

int TokenParser::getLine(char *buffer)
{
	int nchars;

	for(nchars = 0; bytesRemaining() > 0; cursor++)
	{
		if( cursor[0] == '\n' )
		{
			// skip multiple empty newlines if we find them
			for(;bytesRemaining() > 0; cursor++)
			{
				if( cursor[0] != '\n' )
				{
					break;
				}
			}
			break;
		}
		buffer[nchars] = cursor[0];
		nchars++;
	}

	buffer[nchars] = 0;
	return nchars;
}

int TokenParser::getBytes(unsigned char *bytes, int numbytes)
{
	assert(numbytes <= bytesRemaining());
	memcpy(bytes, cursor, numbytes);
	cursor += numbytes;

	return numbytes;
}

int TokenParser::bytesRemaining(void)
{
	return length - (cursor - base);
}

void TokenParser::skipBytes(int numbytes)
{
	cursor += numbytes;
}

void TokenParser::skipBraces(void)
{
	char token[1024];
	int depth;

	getToken(token);
	depth = (token[0] == '{');

	while( depth && getToken(token) )
	{
		if( token[0] == '{' ) depth++;
		if( token[0] == '}' ) depth--;
	}
}
