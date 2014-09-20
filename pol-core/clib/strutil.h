/*
History
=======

Notes
=======

*/

#ifndef CLIB_STRUTIL
#define CLIB_STRUTIL

#include <string>

string hexint( unsigned short v );
string hexint( int v );
string hexint( unsigned v );
string hexint( long v );
string hexint( unsigned long v );

string decint( unsigned short v );
string decint( int v );
string decint( unsigned v );
string decint( long v );
string decint( unsigned long v );


int utf16tombs(const wchar_t* srcUcString, std::vector<char> &destMbBuffer, int mbchcount=-1, const char* locale=NULL);
int utf8tombs(const char* utf8str, std::vector<char> &buffer);
int mbtoutf8s(const char *mbstr, std::vector<char> &buffer);
int mbtoutf16s(const char* mbstr, std::vector<wchar_t> &buffer, int mbchcount=-1, const char* locale=NULL);

void splitnamevalue( const string& istr, string& propname, string& propvalue );

void decodequotedstring( string& str );
void encodequotedstring( string& str );
string getencodedquotedstring( const string& in );

void mklower( string& str );
void mkupper( string& str );

string strlower( const string& str );
string strupper( const string& str );

#endif
