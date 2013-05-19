#ifndef _TOKENS_
#define _TOKENS_

// This is pretty quick and dirty.  Not a lot of bounds checking but it gets the job done.
class TokenParser
{
private:
	char *whitespace;
	char *specials;
	char *base;
	char *cursor;
	int length;

	void skipWhitespace(void);

public:
	TokenParser();

	void initialize(char *ptr, int length, char *spchars, char *wsp);
	int bytesRemaining(void);

	bool getTokenInt(int &value);

	int getToken(char *buffer);
	int getLine(char *buffer);
	int getBytes(unsigned char *bytes, int numbytes);
	
	void skipBytes(int numbytes);
	void skipBraces(void);
};

int readfile(const char *fname, char *& buffer);
void freefile(char *& data);

#endif