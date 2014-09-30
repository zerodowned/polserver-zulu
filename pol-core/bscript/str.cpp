/*
History
=======
2007/12/09 Shinigami: removed ( is.peek() != EOF ) check from String::unpackWithLen()
                      will not work with Strings in Arrays, Dicts, etc.
2008/02/08 Turley:    String::unpackWithLen() will accept zero length Strings
2009/09/12 MuadDib:   Disabled 4244 in this file due to it being on a string iter. Makes no sense.

Notes
=======

*/

#include <typeinfo>

#include "../clib/stl_inc.h"
#include "../clib/base64.cpp"

#include <stdlib.h>
#include <string.h>

#ifdef __GNUG__
#	include <streambuf>
#endif

#include "berror.h"
#include "bobject.h"
#include "impstr.h"
#include "objmethods.h"

#include "executor.h"
#include "../pol/uoexhelp.h"
#include "../clib/random.h"
#include "../pol/polcfg.h";
//--

//class UObject;

const std::locale pol_locale( "ru_RU.1251" );

#ifdef _MSC_VER
#	pragma warning( disable: 4244 )
#endif


String::String(BObjectImp& objimp) : BObjectImp( OTString )
{
    value_ = objimp.getStringRep();
}

String::String(const char *s, int len) :
    BObjectImp( OTString ),
    value_( s, len )
{
}

String *String::StrStr(int begin, int len)
{
	return new String( value_.substr( begin-1, len ) );
}

String *String::ETrim(const char* CRSet, int type)
{
	string tmp = value_;

	if ( type == 1 ) // This is for Leading Only.
	{
		// Find the first character position after excluding leading blank spaces 
		size_t startpos = tmp.find_first_not_of( CRSet );
		if( string::npos != startpos )
			tmp = tmp.substr( startpos );
		return new String( tmp );
	}
	else if ( type == 2 ) // This is for Trailing Only.
	{
		// Find the first character position from reverse 
		size_t endpos = tmp.find_last_not_of( CRSet );
		if( string::npos != endpos ) 
			tmp = tmp.substr( 0, endpos+1 ); 
		return new String( tmp );
	}
	else if ( type == 3 )
	{
		// Find the first character position after excluding leading blank spaces  
		size_t startpos = tmp.find_first_not_of( CRSet );
		// Find the first character position from reverse af  
		size_t endpos = tmp.find_last_not_of( CRSet );

		// if all spaces or empty return an empty string  
		if(( string::npos == startpos ) || ( string::npos == endpos))  
		{  
			tmp = "";
		}  
		else  
			tmp = tmp.substr( startpos, endpos-startpos+1 );
		return new String( tmp );
	}
	else
		return new String( tmp );
}

 void String::EStrReplace(String* str1, String* str2)
 {
	string::size_type valpos = 0;
	
	while ( string::npos != (valpos = value_.find(str1->value_, valpos)) )
     {
 		value_.replace( valpos, str1->length(), str2->value_ );
		valpos += str2->length();
	}	
 }

void String::ESubStrReplace(String* replace_with, unsigned int index, unsigned int len)
{
	value_.replace(index-1, len, replace_with->value_);
}

string String::pack() const
{
    return "s" + value_;
}

void String::packonto( ostream& os ) const
{
    os << "S" << value_.size() << ":" << value_;
}
void String::packonto( ostream& os, const string& value )
{
    os << "S" << value.size() << ":" << value;
}

BObjectImp* String::unpack( const char* pstr )
{
    return new String( pstr );
}

BObjectImp* String::unpack( istream& is )
{
    string tmp;
    getline( is, tmp );

    return new String( tmp );
}

BObjectImp* String::unpackWithLen( istream& is )
{
    unsigned len;
	char colon; 
    if (! (is >> len >> colon))
    {
        return new BError( "Unable to unpack string length." );
    }
	if ( (int)len < 0 )
    {
		return new BError( "Unable to unpack string length. Invalid length!" );
    }
	if ( colon != ':' ) 
	{ 
		return new BError( "Unable to unpack string length. Bad format. Colon not found!" ); 
	}

    is.unsetf( ios::skipws );
    string tmp;
    tmp.reserve( len );
    while (len--)
    {
		char ch = '\0'; 
		if(!(is >> ch) || ch == '\0') 
		{ 
			return new BError( "Unable to unpack string length. String length excessive."); 
		}
		tmp += ch;
    }

	is.setf( ios::skipws );
    return new String( tmp );
}

unsigned long String::sizeEstimate() const
{
    return sizeof(String) + value_.capacity();
}

/*
	0-based string find
	find( "str srch", 2, "srch"):
    01^-- start
*/
int String::find(int begin, const char *target)
{
    // TODO: check what happens in string if begin position is out of range
    string::size_type pos;
    pos = value_.find( target, begin );
    if (pos == string::npos)
        return -1;
    else
        return pos;
}

// Returns the amount of alpha-numeric characters in string.
unsigned int String::alnumlen(void) const
{
	unsigned int c=0;
	while (isalnum(value_[c]))
	{
		c++;
	}
	return c;
}

unsigned int String::SafeCharAmt() const
{
	int strlen = this->length();
	for ( int i=0; i < strlen; i++ )
	{
		char tmp = value_[i];
		if ( isalnum(tmp) ) // a-z A-Z 0-9
			continue;
		else if ( ispunct(tmp) ) // !"#$%&'()*+,-./:;<=>?@{|}~
		{
			if ( tmp == '{' || tmp == '}' )
				return i;
			else
				continue;
		}
		else
		{
			return i;
		}
	}
	return strlen;
}

bool s_parse_int(int &i, string const &s)
{	
	if(s.empty())
		return false;

	char* end;
	i = strtol(s.c_str(), &end, 10);

	if (!*end) {
		return true;
	}
	else {		
		return false;
	}
}

// remove leading/trailing spaces
void s_trim(string &s)
{
	std::stringstream trimmer;
	trimmer << s;
	s.clear();
	trimmer >> s;
}


void int_to_binstr(int& value, std::stringstream &s)
{
	int i;
	for(i = 31; i > 0; i--) {
		if(value & (1 << i))
			break;
	}
	for(; i >= 0; i--) {
		if(value & (1 << i))
			s << "1";
		else
			s << "0";
	}
}

bool try_to_format( std::stringstream &to_stream, BObjectImp *what, string& frmt )
{
	if(frmt.empty()) {
		to_stream << what->getStringRep();
		return false;
	}

	if(frmt.find('b')!=string::npos) {
		if(!what->isa( BObjectImp::OTLong )) {
			to_stream << "<int required>";
			return false;
		}
		BLong* plong = static_cast<BLong*>(what);
		int n = plong->value();
		if(frmt.find('#')!=string::npos)
			to_stream << ((n<0)?"-":"") << "0b";
		int_to_binstr(n, to_stream);
	} else if(frmt.find('x')!=string::npos) {
		if(!what->isa( BObjectImp::OTLong )) {
			to_stream << "<int required>";
			return false;
		}
		BLong* plong = static_cast<BLong*>(what);
		int n = plong->value();
		if(frmt.find('#')!=string::npos)
			to_stream << "0x";
		to_stream << std::hex << n;
	} else if(frmt.find('o')!=string::npos) {
		if(!what->isa( BObjectImp::OTLong )) {
			to_stream << "<int required>";
			return false;
		}
		BLong* plong = static_cast<BLong*>(what);
		int n = plong->value();
		if(frmt.find('#')!=string::npos)
			to_stream << "0o";
		to_stream << std::oct << n;
	} else if(frmt.find('d')!=string::npos) {
		int n;
		if(what->isa( BObjectImp::OTLong )) {
			BLong* plong = static_cast<BLong*>(what);
			int n = plong->value();
		} else if(what->isa( BObjectImp::OTDouble )) {
			Double* pdbl = static_cast<Double*>(what);
			n = (int) pdbl->value();
		} else {
			to_stream << "<int or double required>";
			return false;
		}		
		to_stream << std::dec << n;
	} else {
		to_stream << "<unknown format: " << frmt << ">";
		return false;
	}		
	return true;
}

std::string upper_string(const std::string& str)
{
    string upper;
    transform(str.begin(), str.end(), std::back_inserter(upper), toupper);
    return upper;
}

std::string::size_type find_str_ci(const std::string& str, const std::string& substr)
{
    return upper_string(str).find(upper_string(substr) );
}

// templated version of my_equal so it could work with both char and wchar_t
template<typename charT>
struct my_equal {
    my_equal( const std::locale& loc ) : loc_(loc) {}
    bool operator()(charT ch1, charT ch2) {
        return std::toupper(ch1, loc_) == std::toupper(ch2, loc_);
    }
private:
    const std::locale& loc_;
};

// find substring (case insensitive)
template<typename T>
int ci_find_substr( const T& str1, const T& str2, const std::locale& loc = std::locale() )
{
    T::const_iterator it = std::search( str1.begin(), str1.end(), 
        str2.begin(), str2.end(), my_equal<T::value_type>(loc) );
    if ( it != str1.end() ) return it - str1.begin();
    else return -1; // not found
}

BObjectImp* String::call_method( const char* methodname, Executor& ex )
{
	/*
	if (!stricmp(methodname, "")) {
		
	} else 
	if (!stricmp(methodname, "")) {
		
	} else 
	if (!stricmp(methodname, "")) {
		
	} else 
	if (!stricmp(methodname, "")) {

	} else 
	if (!stricmp(methodname, "")) {

	} else 
	if (!stricmp(methodname, "")) {
						
	}
	*/

	std::stringstream result;	
	result << "Method '" << methodname << "' not found.";
	return new BError( result.str() );
}

BObjectImp* String::call_method_id( const int id, Executor& ex, bool forcebuiltin )
{
	switch (id)
	{
	case MTH_LENGTH:
	{
		if(ex.numParams()==0) {
			int strlen = this->length();
			return new BLong( strlen );			
		} else {
			return new BError( "string.length() doesn't take parameters." );
		}
	}
	case MTH_STARTSWITH:
	{
		if(ex.numParams()==1) {
			const String* bimp_substr;
			if ( !ex.getStringParam(0, bimp_substr) )
			{
				return new BError("Invalid parameter type.");
			}
			string subsr = bimp_substr->getStringRep();
			if(this->getStringRep().find(subsr)==0) {
				return new BLong( 1 );
			}
			return new BLong( 0 );	
		} else {
			return new BError( "string.startswith() takes exactly one parameter." );
		}
	}
	case MTH_STARTSWITH_IC:
	{
		if(ex.numParams()==1) {
			const String* bimp_substr;
			if ( !ex.getStringParam(0, bimp_substr) )
			{
				return new BError("Invalid parameter type.");
			}
			string subsr = bimp_substr->getStringRep();

			int res = ci_find_substr(this->getStringRep(), subsr, pol_locale);

			if(res==0)
				return new BLong( 1 );	
			else
				return new BLong( 0 );

		} else {
			return new BError( "string.startswith_ic() takes exactly one parameter." );
		}
	}
	case MTH_BASE64_DECODE:
	{
		if(ex.numParams()==0) {
			std::string s = this->getStringRep();			
			std::string decode = base64_decode(s);
			return new String( decode );			
		} else {
			return new BError( "string.base64_decode() doesn't take parameters." );
		}
	}
	case MTH_BASE64_ENCODE:
	{
		if(ex.numParams()==0) {
			std::string s = this->getStringRep();			
			std::string encoded = base64_encode(reinterpret_cast<const unsigned char*>(s.c_str()), this->length());
			return new String( encoded );			
		} else {
			return new BError( "string.base64_encode() doesn't take parameters." );
		}
	}
	case MTH_OBFUSCATE:
	{
		long percent_chance = 100;

		if( ex.numParams() > 0 ) {

			if (ex.getParam( 0, percent_chance ))
			{
				if( percent_chance > 100 )
					percent_chance = 100;
				else if( percent_chance < 0 )
					percent_chance = 0;
			}
			else
			{
				return new BError( "Invalid parameter type" );
			}
		}
		
		std::string obfuscated = this->getStringRep();		
		char replace_c;
		
		for(std::string::size_type i = 0; i < obfuscated.size(); ++i) {
			switch (obfuscated[i])
			{
			case 'e':
				replace_c = 'å';
				break;
			case 'o':
				replace_c = 'î';
				break;
			case 'a':
				replace_c = 'à';
				break;
			//case 'n':
			//	obfuscated[i] = 'ï';
			case 'y':
				replace_c = 'ó';		
				break;
			//case 'r':
			//	obfuscated[i] = 'ã';
			//case 'w':
			//	obfuscated[i] = 'ø';
			//case 'k':
			//	obfuscated[i] = 'ê';
			case 'p':
				replace_c = 'ð';
				break;
			case 'A':
				replace_c = 'À';
				break;
			case 'X':
				replace_c = 'Õ';
				break;
			case 'x':
				replace_c = 'x';
				break;
			case 'C':
				replace_c = 'Ñ';
				break;
			case 'c':
				replace_c = 'ñ';
				break;
			default:
				replace_c = '\0';
				break;
			}

			if( replace_c && (random_int(100) < percent_chance) ) {				
				obfuscated[i] = replace_c;
			}
		}

		return new String( obfuscated );
	}
	case MTH_JOIN:
		{
		BObject* cont;
		if (ex.numParams() == 1 &&
			(cont = ex.getParamObj( 0 )) != NULL )
		{
			if (! (cont->isa( OTArray )))
				return new BError( "string.join expects an array" );
			ObjArray* container = static_cast<ObjArray*>(cont->impptr());

			// no empty check here on purpose
			std::ostringstream joined;
			bool first=true;

			for( ObjArray::const_iterator itr = container->ref_arr.begin(), itrend = container->ref_arr.end(); itr != itrend; ++itr ) {
				BObject *bo = itr->get();

				if ( bo == NULL )
					continue;

				if (!first)
					joined << value_;
				else
					first = false;

				joined << bo->impptr()->getStringRep();
			}
			return new String(joined.str());
		} else
			return new BError( "string.join(array) requires a parameter." );
		}
	case MTH_EQUALS_IC:
		{
			const String* equals_to;
			if (ex.numParams() == 1 &&
				(ex.getStringParam(0, equals_to)!=false))
			{
				if( (this->length()==equals_to->length()) && (ci_find_substr(value_, equals_to->getStringRep(), pol_locale)==0) ) {
					return new BLong(1);
				}

				return new BLong(0);
			
			} else {
				return new BError( "string.equals_ic(String) requires a String parameter." );					
			}
		}
		case MTH_FIND:
		{
			if ( ex.numParams() > 2 )
				return new BError( "string.find(Search, [Start]) takes only two parameters" );
			if ( ex.numParams() < 1 )
				return new BError( "string.find(Search, [Start]) takes at least one parameter" );

			const char *s = ex.paramAsString( 0 );
			int d = 0;
			if ( ex.numParams() == 2 )
				d = ex.paramAsLong( 1 );
			int posn = find( d ? ( d - 1 ) : 0, s ) + 1;
			return new BLong( posn );
		}
		case MTH_FORMAT:
		{
			if(ex.numParams() > 0) {

				//string s = this->getStringRep(); // string itself
				std::stringstream result;

				unsigned int tag_start_pos; // the position of tag's start "{"
				unsigned int tag_stop_pos;  // the position of tag's end "}"
				unsigned int tag_dot_pos;				

				int tag_param_idx;			

				unsigned int str_pos=0; // current string position					
				unsigned int next_param_idx = 0; // next index of .format() parameter		

				char w_spaces[] = "\t ";

				while((tag_start_pos = value_.find("{", str_pos)) != string::npos) {
					if((tag_stop_pos=value_.find("}", tag_start_pos)) != string::npos) {

						result << value_.substr(str_pos, tag_start_pos - str_pos);
						str_pos = tag_stop_pos + 1;
						
						string tag_body = value_.substr(tag_start_pos+1, (tag_stop_pos - tag_start_pos)-1);

						tag_start_pos = tag_body.find_first_not_of( w_spaces );
						tag_stop_pos = tag_body.find_last_not_of( w_spaces );

						//cout << "' tag_body1: '" << tag_body << "'";
						
						// trim the tag of whitespaces (slightly faster code ~25%)
						if(tag_start_pos!=string::npos && tag_stop_pos!=string::npos)
							tag_body = tag_body.substr(tag_start_pos, (tag_stop_pos-tag_start_pos)+1);
						else if(tag_start_pos!=string::npos)
							tag_body = tag_body.substr(tag_start_pos);
						else if(tag_stop_pos!=string::npos)
							tag_body = tag_body.substr(0, tag_stop_pos+1);								
						
						// s_trim( tag_body ); // trim the tag of whitespaces

						//cout << "' tag_body2: '" << tag_body << "'";

						string frmt;
						size_t formatter_pos = tag_body.find(':');

						if( formatter_pos!=string::npos) {
							frmt = tag_body.substr(formatter_pos + 1, string::npos); //
							tag_body = tag_body.substr(0, formatter_pos); // remove property from the tag
						}					

						string prop_name;									
						// parsing {1.this_part}
						tag_dot_pos = tag_body.find(".", 0);

						// '.' is found within the tag, there is a property name
						if(tag_dot_pos!=string::npos) {
							prop_name = tag_body.substr(tag_dot_pos + 1, string::npos); //
							tag_body = tag_body.substr(0, tag_dot_pos); // remove property from the tag

							// if s_tag_body is numeric then use it as an index
							if(s_parse_int(tag_param_idx, tag_body)) {
								tag_param_idx -= 1; // sinse POL indexes are 1-based
							} else {
								result << "<idx required before: '"<< prop_name <<"'>";
								continue;
							}						
						} else {						
							if(s_parse_int(tag_param_idx, tag_body)) {
								tag_param_idx -= 1; // sinse POL indexes are 1-based
							}
							else { // non-integer body has just next idx in line
								prop_name = tag_body;
								tag_param_idx = next_param_idx++;
							}
						}

						// -- end of property parsing

						//cout << "prop_name: '" << prop_name << "' tag_body: '" << tag_body << "'";

						if( ex.numParams() <= tag_param_idx ) {						
							result << "<tag out of range: #" << (tag_param_idx + 1) << ">";						
							continue;
						}					

						BObjectImp *imp = ex.getParamImp(tag_param_idx);
						
						if(prop_name.empty()==false) { // accesing object member
							BObjectRef obj_member = imp->get_member(prop_name.c_str());
							BObjectImp *member_imp = obj_member->impptr();							
							try_to_format(result, member_imp, frmt);											
						} else {										
							try_to_format(result, imp, frmt);							
						}

					} else {
						break;
					}
				}

				if( str_pos < value_.length() ) {
					result << value_.substr( str_pos, string::npos ); 
				}					

				return new String( result.str() );
				
			} else {
				return new BError( "string.format() requires a parameter." );
			}		
		}
	default:
		return NULL;
	}
}


void String::reverse(void)
{
    ::reverse( value_.begin(), value_.end() );
}

void String::set( char *newstr )
{
    value_ = newstr;
    delete newstr;
}


BObjectImp* String::selfPlusObjImp(const BObjectImp& objimp) const
{
    return new String( value_ + objimp.getStringRep() );
}

void String::remove(const char *rm)
{
    int len = strlen(rm);
    string::size_type pos = value_.find( rm );
    if (pos != string::npos)
        value_.erase( pos, len );
}

BObjectImp* String::selfMinusObjImp(const BObjectImp& objimp) const
{
    String *tmp = (String *) copy();
    if (objimp.isa( OTString )) 
    {
        const String& to_remove = (String&) objimp;
        tmp->remove(to_remove.value_.data());
    } 
    else
    {
        tmp->remove( objimp.getStringRep().data() );
    }
    return tmp;
}


bool String::isEqual(const BObjectImp& objimp) const
{
    if (objimp.isa( OTString ))
    {
        return ( value_ == static_cast<const String&>(objimp).value_ );
    }
    else
    {
        return false;
    }
}

/* since non-Strings show up here as not equal, we make them less than. */
bool String::isLessThan(const BObjectImp& objimp) const
{
    if (objimp.isa( OTString ))
    {
        return ( value_ < static_cast<const String&>(objimp).value_ );
    }
    else if (objimp.isa( OTUninit ) || objimp.isa( OTError ))
    {
        return false;
    }
    else
    {
        return true;
    }
}

String *String::midstring(int begin, int len) const
{
    return new String( value_.substr( begin-1, len ) );
}

void String::toUpper( void )
{
    for( string::iterator itr = value_.begin(); itr != value_.end(); ++itr )
    {
        *itr = std::toupper(*itr, pol_locale);
    }
}

void String::toLower( void )
{
    for( string::iterator itr = value_.begin(); itr != value_.end(); ++itr )
    {
		*itr = std::tolower(*itr, pol_locale);
    }
}

BObjectImp* String::array_assign( BObjectImp* idx, BObjectImp* target, bool copy )
{
	(void) copy;
    string::size_type pos, len;

    // first, determine position and length.
    if (idx->isa( OTString ))
    {
		String& rtstr = (String&) *idx;
        pos = value_.find( rtstr.value_ );
        len = rtstr.length();
    }
    else if (idx->isa( OTLong ))
    {
        BLong& lng = (BLong&) *idx;
        pos = lng.value()-1;
        len = 1;
    }
    else if (idx->isa( OTDouble ))
    {
        Double& dbl = (Double&)*idx;
        pos = static_cast<string::size_type>(dbl.value());
        len = 1;
    }
    else
    {
        return UninitObject::create();
    }

    if (pos != string::npos)
    {
        if (target->isa( OTString ))
        {
            String* target_str = (String*) target;
            value_.replace( pos, len, target_str->value_ );
        }
        return this;
    }
    else
    {
        return UninitObject::create();
    }
}

BObjectRef String::OperMultiSubscriptAssign( stack<BObjectRef>& indices, BObjectImp* target )
{
    BObjectRef start_ref = indices.top();
    indices.pop();
    BObjectRef length_ref = indices.top();
    indices.pop();

    BObject& length_obj = *length_ref;
    BObject& start_obj = *start_ref;

    BObjectImp& length = length_obj.impref();
    BObjectImp& start = start_obj.impref();

    // first deal with the start position.
    unsigned index;
    if (start.isa( OTLong ))
    {
        BLong& lng = (BLong&) start;
        index = (unsigned) lng.value();
        if (index == 0 || index > value_.size())
            return BObjectRef(new BError( "Subscript out of range" ));

    }
    else if (start.isa( OTString ))
    {
		String& rtstr = (String&) start;
        string::size_type pos = value_.find( rtstr.value_ );
        if (pos != string::npos)
            index = pos+1;
        else
            return BObjectRef(new UninitObject);
    }
    else
    {
        return BObjectRef(copy());
    }

    // now deal with the length.
    int len;
    if (length.isa( OTLong ))
    {
		BLong& lng = (BLong &) length;

        len = (int)lng.value();
    }
    else if (length.isa( OTDouble ))
    {
		Double& dbl = (Double &) length;

        len = (int)dbl.value();
    }
    else
    {
        return BObjectRef( copy() );
    }

    if (target->isa( OTString ))
    {
        String* target_str = (String*) target;
        value_.replace( index-1, len, target_str->value_ );
    }
    else
    {
        return BObjectRef( UninitObject::create() );
    }

    return BObjectRef( this );
}


BObjectRef String::OperMultiSubscript( stack<BObjectRef>& indices )
{
    BObjectRef start_ref = indices.top();
    indices.pop();
    BObjectRef length_ref = indices.top();
    indices.pop();

    BObject& length_obj = *length_ref;
    BObject& start_obj = *start_ref;

    BObjectImp& length = length_obj.impref();
    BObjectImp& start = start_obj.impref();

    // first deal with the start position.
    unsigned index;
    if (start.isa( OTLong ))
    {
        BLong& lng = (BLong&) start;
        index = (unsigned) lng.value();
        if (index == 0 || index > value_.size())
            return BObjectRef(new BError( "Subscript out of range" ));

    }
    else if (start.isa( OTString ))
    {
		String& rtstr = (String&) start;
        string::size_type pos = value_.find( rtstr.value_ );
        if (pos != string::npos)
            index = pos+1;
        else
            return BObjectRef(new UninitObject);
    }
    else
    {
        return BObjectRef(copy());
    }

    // now deal with the length.
    int len;
    if (length.isa( OTLong ))
    {
		BLong& lng = (BLong &) length;

        len = (int)lng.value();
    }
    else if (length.isa( OTDouble ))
    {
		Double& dbl = (Double &) length;

        len = (int)dbl.value();
    }
    else
    {
        return BObjectRef( copy() );
    }

    String* str = new String( value_, index-1, len );
    return BObjectRef( str );
}

BObjectRef String::OperSubscript( const BObject& rightobj )
{
    const BObjectImp& right = rightobj.impref();
    if (right.isa( OTLong ))
    {
		BLong& lng = (BLong& ) right;

		unsigned index = (unsigned)lng.value();
		
        if (index == 0 || index > value_.size())
            return BObjectRef(new BError( "Subscript out of range" ));

		return BObjectRef( new BObject( new String( value_.c_str()+index-1, 1 ) ) );
    }
    else if (right.isa( OTDouble ))
	{
		Double& dbl = (Double& ) right;

		unsigned index = (unsigned)dbl.value();
		
        if (index == 0 || index > value_.size())
            return BObjectRef(new BError( "Subscript out of range" ));

		return BObjectRef( new BObject( new String( value_.c_str()+index-1, 1 ) ) );
	}
	else if (right.isa( OTString ))
	{
		String& rtstr = (String&) right;
        string::size_type pos = value_.find( rtstr.value_ );
        if (pos != string::npos)
            return BObjectRef( new BObject( new String( value_, pos, 1 ) ) );
        else
            return BObjectRef(new UninitObject);
    }
    else
    {
        return BObjectRef(new UninitObject);
    }
}

