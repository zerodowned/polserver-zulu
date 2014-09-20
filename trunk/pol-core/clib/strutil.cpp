/*
History
=======
2009/09/12 MuadDib:   Disabled 4244 in this file due to it being on a string iter. Makes no sense.

Notes
=======

*/

#include "stl_inc.h"

#include "stlutil.h"
#include "strutil.h"
#include "unittest.h"

#ifdef _WIN32
# include <windows.h>
#endif

#include <locale.h>     /* struct lconv, setlocale, localeconv */

#ifdef _MSC_VER
#	pragma warning( disable: 4244 )
#endif

// unicode to 1-byte string
int utf16tombs(const wchar_t* srcUcString, std::vector<char> &destMbBuffer, int mbchcount, const char* locale)
{
	int res;

#ifdef _WIN32
	if(mbchcount==-1)
		mbchcount = WideCharToMultiByte(CP_ACP, 0, srcUcString, -1, NULL, 0, NULL, NULL);

	if(destMbBuffer.size() != mbchcount)
		destMbBuffer.resize(mbchcount);

	res = WideCharToMultiByte(CP_ACP, 0, srcUcString, -1, &destMbBuffer[0], mbchcount, NULL, NULL);
#else
	if(locale!=NULL)
		setlocale( LC_ALL, locale );

	if(mbchcount==-1)
		mbchcount = wcslen(srcUcString);

	if(destMbBuffer.size() != mbchcount)
		destMbBuffer.resize(mbchcount);	

	res = wcstombs( &destMbBuffer[0], srcUcString, mbchcount );
#endif
	destMbBuffer.push_back('\0');
	return res;
}

// convert 1-byte string to Unicode
int mbtoutf16s(const char* mbstr, std::vector<wchar_t> &buffer, int mbchcount, const char* locale)
{		
	int res;

#ifdef _WIN32
	if(mbchcount==-1)
		mbchcount = MultiByteToWideChar( CP_ACP, 0, mbstr, -1, NULL, 0 );
	
	if(buffer.size()!=mbchcount)
		buffer.resize(mbchcount);

	res =  MultiByteToWideChar( CP_ACP, 0, mbstr, -1, &buffer[0], mbchcount );
#else
	if(mbchcount==-1)
		mbchcount = strlen(mbstr);

	if(buffer.size()!=mbchcount)
		buffer.resize(mbchcount);

	if( locale != NULL ) {
		setlocale( LC_ALL, locale );
	}
	res = mbstowcs( &buffer[0], mbstr, mbchcount );
#endif

	buffer.push_back(L'\0');	
	return res;
}

// decode utf8str to a multibyte string
int utf8tombs(const char* utf8str, std::vector<char> &buffer) {	
	// decode utf-8 to UTF-16
	int charsCount = MultiByteToWideChar(CP_UTF8, 0, utf8str, -1, NULL, 0);
	std::wstring uc16String(charsCount, L'\0');
	MultiByteToWideChar( CP_UTF8, 0, utf8str, -1, &uc16String[0], charsCount );

	size_t mbStrLen = WideCharToMultiByte( CP_ACP, 0, &uc16String[0], -1, 
                                               NULL, 0,  NULL, NULL);
	buffer.resize( mbStrLen );

	// encode UTF16 to multibyte
	return WideCharToMultiByte( CP_ACP, 0, &uc16String[0], -1, &buffer[0], mbStrLen, NULL, NULL );
}

// convert mbstr oem string to utf-8
// buffer - resulting utf-8 string
// return: number of character converted
int mbtoutf8s(const char *mbstr, std::vector<char> &buffer) {

	// convert multibyte to UTF16
	std::wstring uc16String(strlen(mbstr)+1, L'\0');	

	MultiByteToWideChar( CP_ACP, 0, mbstr, -1, &uc16String[0], strlen(mbstr) );
	size_t utf8StrLen = WideCharToMultiByte( CP_UTF8, 0, &uc16String[0], -1, 
                                               NULL, 0,  NULL, NULL);

	buffer.resize( utf8StrLen );

	//setlocale(LC_ALL, ".utf8");
	//return wcstombs( &buffer[0], &uc16String[0], utf8StrLen );

	//endcode UTF16 to utf-8
	return WideCharToMultiByte( CP_UTF8, 0, &uc16String[0], -1, &buffer[0], utf8StrLen, NULL, NULL );
}


string hexint( unsigned short v )
{
    OSTRINGSTREAM os;
    os << "0x" << hex << v;
    return OSTRINGSTREAM_STR(os);
}

string hexint( int v )
{
    OSTRINGSTREAM os;
    os << "0x" << hex << v;
    return OSTRINGSTREAM_STR(os);
}
string hexint( unsigned v )
{
    OSTRINGSTREAM os;
    os << "0x" << hex << v;
    return OSTRINGSTREAM_STR(os);
}

string hexint( long v )
{
    OSTRINGSTREAM os;
    os << "0x" << hex << v;
    return OSTRINGSTREAM_STR(os);
}
string hexint( unsigned long v )
{
    OSTRINGSTREAM os;
    os << "0x" << hex << v;
    return OSTRINGSTREAM_STR(os);
}

string decint( unsigned short v )
{
    OSTRINGSTREAM os;
    os << v;
    return OSTRINGSTREAM_STR(os);
}

string decint( unsigned v )
{
    OSTRINGSTREAM os;
    os << v;
    return OSTRINGSTREAM_STR(os);
}

string decint( int v )
{
    OSTRINGSTREAM os;
    os << v;
    return OSTRINGSTREAM_STR(os);
}

string decint( long v )
{
    OSTRINGSTREAM os;
    os << v;
    return OSTRINGSTREAM_STR(os);
}

string decint( unsigned long v )
{
    OSTRINGSTREAM os;
    os << v;
    return OSTRINGSTREAM_STR(os);
}

void splitnamevalue( const string& istr, 
                     string& propname, 
                     string& propvalue )
{
    string::size_type start = istr.find_first_not_of( " \t\r\n" );
    if (start != string::npos)
    {
        string::size_type delimpos = istr.find_first_of( " \t\r\n=", start+1 );
        if (delimpos != string::npos)
        {
            string::size_type valuestart = istr.find_first_not_of( " \t\r\n", delimpos+1 );
            string::size_type valueend = istr.find_last_not_of( " \t\r\n" );
            propname = istr.substr( start, delimpos-start );
            if (valuestart != string::npos && valueend != string::npos)
            {
                propvalue = istr.substr( valuestart, valueend-valuestart+1 );
            }
            else
            {
                propvalue = "";
            }
        }
        else
        {
            propname = istr.substr( start, string::npos );
            propvalue = "";
        }
    }
    else
    {
        propname = "";
        propvalue = "";
    }
}

void test_splitnamevalue( const string& istr, const string& exp_pn, const string& exp_pv )
{
    string pn, pv;
    splitnamevalue( istr, pn, pv );
    if (pn != exp_pn || pv != exp_pv)
    {
        cout << "splitnamevalue( \"" << istr << "\" ) fails!" << endl;
    }
}

void test_splitnamevalue()
{
    test_splitnamevalue( "a b", "a", "b" );
    test_splitnamevalue( "av bx", "av", "bx" );
    test_splitnamevalue( "nm=valu", "nm", "valu" );
    test_splitnamevalue( "nm:valu", "nm:valu", "" );
    test_splitnamevalue( "nm", "nm", "" );
    test_splitnamevalue( "  nm", "nm", "" );
    test_splitnamevalue( "  nm  ", "nm", "" );
    test_splitnamevalue( "  nm valu", "nm", "valu" );
    test_splitnamevalue( "  nm   value   ", "nm", "value" );
    test_splitnamevalue( "  nm  value is multiple words", "nm", "value is multiple words" );
    test_splitnamevalue( "  nm  value is multiple words\t ", "nm", "value is multiple words" );
}
UnitTest test_splitnamevalue_obj( test_splitnamevalue );

void decodequotedstring( string& str )
{
    string tmp;
    tmp.swap( str );
    const char* s = tmp.c_str();
    str.reserve( tmp.size() );
    ++s;
    while (*s)
    {
        char ch = *s++;

        switch( ch )
        {
        case '\\':
            ch = *s++;
            switch( ch )
            {
            case '\0':
                return;
            case 'n':               // newline
                str += "\n";
                break;
            default:                // slash, quote, etc
                str += ch;
                break;
            }
            break;
        
        case '\"':
            return;

        default:
            str += ch;
            break;
        }
    }
}
void encodequotedstring( string& str )
{
    string tmp;
    tmp.swap( str );
    const char* s = tmp.c_str();
    str.reserve( tmp.size()+2 );
    str += "\"";

    while (*s)
    {
        char ch = *s++;
        switch( ch )
        {
        case '\\':
            str += "\\\\";
            break;
        case '\"':
            str += "\\\"";
            break;
        case '\n':
            str += "\\n";
            break;
        default:
            str += ch;
            break;
        }
    }
    
    str += "\"";
}
string getencodedquotedstring( const string& in )
{
    string tmp = in;
    encodequotedstring(tmp);
    return tmp;
}
void test_dqs( const string& in, const string& out )
{
    string tmp = in;
    decodequotedstring(tmp);
    if (tmp != out)
    {
        cout << "decodequotedstring( " << in << " ) fails!" << endl;
    }
    encodequotedstring(tmp);
    if (tmp != in)
    {
        cout << "encodequotedstring( " << out << " ) fails!" << endl;
    }
}

void test_convertquotedstring()
{
    test_dqs( "\"hi\"", "hi" );
    test_dqs( "\"hi \"", "hi " );
    test_dqs( "\" hi \"", " hi " );
    test_dqs( "\" \\\"hi\"", " \"hi" );
}
UnitTest test_convertquotedstring_obj(test_convertquotedstring);


void mklower( string& str )
{
    for( string::iterator itr = str.begin(); itr != str.end(); ++itr )
    {
        *itr = tolower(*itr);
    }
}

void mkupper( string& str )
{
    for( string::iterator itr = str.begin(); itr != str.end(); ++itr )
    {
        *itr = toupper(*itr);
    }
}

string strlower( const string& str )
{
    string tmp = str;
    mklower(tmp);
    return tmp;
}

string strupper( const string& str )
{
    string tmp = str;
    mkupper(tmp);
    return tmp;
}
