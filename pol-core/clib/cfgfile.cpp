/*
History
=======
2005/07/01 Shinigami: added ConfigFile::_modified (stat.st_mtime) to detect cfg file modification
2009/08/25 Shinigami: STLport-5.2.1 fix: elem->type() check will use strlen() now

Notes
=======

*/

#include "stl_inc.h"

#ifdef _MSC_VER
#	pragma warning( disable: 4786 )
#endif

#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "clib.h"
#include "clibopt.h"
#include "cfgelem.h"
#include "cfgfile.h"
#include "stlutil.h"
#include "strutil.h"

#undef CFGFILE_USES_TRANSLATION_TABLE
#define CFGFILE_USES_TRANSLATION_TABLE 0

#if CFGFILE_USES_TRANSLATION_TABLE
#	include "xlate.h"
#endif

bool commentline( const std::string& str )
{
#ifdef __GNUC__
    return ( (str[0] == '#') 
                  || 
             (str.substr(0,2) == "//") );
#else
    return ( (str[0] == '#')
                  || 
             (str.compare( 0,2,"//" ) == 0) );
#endif
}


ConfigProperty::ConfigProperty( const char *name, const char *value ) :
    name_(name),
	value_(value)
{
}

ConfigProperty::ConfigProperty( const string& name, const string& value ) :
    name_(name),
	value_(value)
{
}

ConfigProperty::ConfigProperty( string* pname, string* pvalue )
{
    pname->swap( name_ );
    pvalue->swap( value_ );
}

ConfigProperty::~ConfigProperty()
{
}

ConfigElemBase::ConfigElemBase() :
	type_(""),
	rest_(""),
    _source(NULL)
{
}

ConfigElem::ConfigElem() :
    ConfigElemBase()
{
}

ConfigElem::~ConfigElem()
{
}


const char *ConfigElemBase::type() const
{
	return type_.c_str();
}

const char *ConfigElemBase::rest() const
{
	return rest_.c_str();
}

void ConfigElem::set_rest( const char* rest )
{
    rest_ = rest;
}

void ConfigElem::set_type( const char* type )
{
    type_ = type;
}

void ConfigElem::set_source( const ConfigElem& elem )
{
    _source = elem._source;
}
void ConfigElem::set_source( const ConfigSource* source )
{
    _source = source;
}

bool ConfigElemBase::type_is( const char* type ) const
{
    return (stricmp( type_.c_str(), type ) == 0);
}

bool ConfigElem::remove_first_prop( std::string* propname,
                                   std::string* value )
{
	if (properties.empty())
		return false;
	
    Props::iterator itr = properties.begin();
    *propname = (*itr).first;
    *value = (*itr).second;
    properties.erase( itr );
    return true;
}

bool ConfigElem::has_prop( const char *propname ) const
{
	return properties.count( propname ) != 0;
}
bool VectorConfigElem::has_prop( const char *propname ) const
{
	
    Props::const_iterator itr = properties.begin(), end = properties.end();
	for( ; itr != end; ++itr )
	{
		ConfigProperty *prop = *itr;
		if ( stricmp( prop->name_.c_str(), propname) == 0 )
		{
            return true;
		}
	}
	return false;
}

bool ConfigElem::remove_prop( const char *propname, std::string* value )
{
	Props::iterator itr = properties.find( propname );
    if (itr != properties.end())
    {
		*value = (*itr).second;
        properties.erase( itr );
    	return true;
	}
    else
    {
        return false;
    }
}

bool VectorConfigElem::remove_prop( const char *propname, std::string* value )
{
    Props::iterator itr = properties.begin(), end = properties.end();
	for( ; itr != end; ++itr )
	{
		ConfigProperty *prop = *itr;
		if ( stricmp( prop->name_.c_str(), propname) == 0 )
		{
			*value = prop->value_;
			delete prop;
            properties.erase( itr );
			return true;
		}
	}
	return false;
}

bool ConfigElem::read_prop( const char *propname, std::string* value ) const
{
	Props::const_iterator itr = properties.find( propname );
    if (itr != properties.end())
    {
		*value = (*itr).second;
    	return true;
	}
    else
    {
        return false;
    }
}
bool VectorConfigElem::read_prop( const char *propname, std::string* value ) const
{
	
    Props::const_iterator itr = properties.begin(), end = properties.end();
	for( ; itr != end; ++itr )
	{
		const ConfigProperty *prop = *itr;
		if ( stricmp( prop->name_.c_str(), propname) == 0 )
		{
			*value = prop->value_;
			return true;
		}
	}
	return false;
}

void ConfigElem::get_prop( const char *propname, unsigned long* plong ) const
{
    Props::const_iterator itr = properties.find( propname );
	if (itr != properties.end())
	{
		*plong = strtoul( (*itr).second.c_str(), NULL, 0 );
	}
    else
    {
        throw_error( "SERIAL property not found" );
    }
}

bool ConfigElem::remove_prop( const char *propname, unsigned long *plong )
{
    Props::iterator itr = properties.find( propname );
	if (itr != properties.end())
	{
		*plong = strtoul( (*itr).second.c_str(), NULL, 0 );
        properties.erase( itr );
		return true;
	}
    else
    {
        return false;
    }
}
bool VectorConfigElem::remove_prop( const char *propname, unsigned long *plong )
{
    Props::iterator itr = properties.begin(), end = properties.end();
	for( ; itr != end; ++itr )
	{
		ConfigProperty *prop = *itr;
		if (stricmp( prop->name_.c_str(), propname ) == 0)
		{
			*plong = strtoul( prop->value_.c_str(), NULL, 0 );
			delete prop;
            properties.erase( itr );
			return true;
		}
	}
	return false;
}

bool ConfigElem::remove_prop( const char *propname, unsigned short *psval )
{
    string temp;
    if (remove_prop( propname, &temp ))
	{
#if CFGFILE_USES_TRANSLATION_TABLE
        TranslationTable* tbl = translations.get_trans_table( propname );
        if (tbl != NULL)
            tbl->translate( &temp );
#endif

        // FIXME isdigit isxdigit - +
        // or, use endptr
        
        char *endptr = NULL;
		*psval = (unsigned short) strtoul( temp.c_str(), &endptr, 0 );
        if ((endptr != NULL) &&
            (*endptr != '\0') &&
            !isspace(*endptr))
        {
            string errmsg;
            errmsg = "Poorly formed number in property '";
            errmsg += propname;
            errmsg += "': " + temp;
            throw_error( errmsg );
        }
        // FIXME check range within unsigned short
		return true;
	}
	return false;
}

bool VectorConfigElem::remove_prop( const char *propname, unsigned short *psval )
{
    Props::iterator itr = properties.begin(), end = properties.end();
	for( ; itr != end; ++itr )
	{
		ConfigProperty *prop = *itr;
		if (stricmp( prop->name_.c_str(), propname ) == 0)
		{
#if CFGFILE_USES_TRANSLATION_TABLE
            TranslationTable* tbl = translations.get_trans_table( propname );
            if (tbl != NULL)
                tbl->translate( &prop->value_ );
#endif

            // FIXME isdigit isxdigit - +
            // or, use endptr
            
            char *endptr = NULL;
			*psval = (unsigned short) strtoul( prop->value_.c_str(), &endptr, 0 );
            if ((endptr != NULL) &&
                (*endptr != '\0') &&
                !isspace(*endptr))
            {
                string errmsg;
                errmsg = "Poorly formed number in property '";
                errmsg += propname;
                errmsg += "': " + prop->value_;
                throw_error( errmsg );
            }
            // FIXME check range within unsigned short
			delete prop;
            properties.erase( itr );
			return true;
		}
	}
	return false;
}

void ConfigElem::throw_error( const string& errmsg ) const
{
    if (_source != NULL)
        _source->display_error( errmsg, false, this );
    throw runtime_error( "Configuration file error" );
}
void VectorConfigElem::throw_error( const string& errmsg ) const
{
    if (_source != NULL)
        _source->display_error( errmsg, false, this );
    throw runtime_error( "Configuration file error" );
}

void ConfigElem::warn( const string& errmsg ) const
{
    if (_source != NULL)
        _source->display_error( errmsg, false, this, false );
}

void ConfigElem::warn_with_line( const string& errmsg ) const
{
    if (_source != NULL)
        _source->display_error( errmsg, true, this, false );
}

void ConfigElem::prop_not_found( const char *propname ) const
{
    string errmsg( "Property '" );
    errmsg += propname;
    errmsg += "' was not found.";
    throw_error( errmsg );
}

unsigned short ConfigElem::remove_ushort( const char *propname )
{
    unsigned short temp;
    if (remove_prop( propname, &temp))
    {
        return temp;
    }
    else
    {
        prop_not_found( propname );
        return 0; // this is not reached, prop_not_found throws
    }
}

unsigned short ConfigElem::remove_ushort( const char *propname, unsigned short dflt )
{
    unsigned short temp;
    if (remove_prop( propname, &temp))
        return temp;
    else
        return dflt;
}

int ConfigElem::remove_int( const char *propname )
{
    string temp = remove_string( propname );

    return atoi( temp.c_str() );
}

int ConfigElem::remove_int( const char *propname, int dflt )
{
    string temp;
    if (remove_prop( propname, &temp ))
    {
        return atoi( temp.c_str() );
    }
    else
    {
        return dflt;
    }
}

unsigned ConfigElem::remove_unsigned( const char *propname )
{
    string temp = remove_string( propname );

    return strtoul( temp.c_str(), NULL, 0 ); // TODO check unsigned range
}

unsigned ConfigElem::remove_unsigned( const char *propname, int dflt )
{
    string temp;
    if (remove_prop( propname, &temp ))
    {
        return strtoul( temp.c_str(), NULL, 0  ); // TODO check unsigned range
    }
    else
    {
        return dflt;
    }
}


string ConfigElem::remove_string( const char *propname )
{
    string temp;
    if (remove_prop( propname, &temp))
    {
        return temp;
    }
    else
    {
        prop_not_found( propname );
        return ""; // this is not reached, prop_not_found throws
    }
}

string ConfigElem::read_string( const char *propname ) const
{
    string temp;
    if (read_prop( propname, &temp))
    {
        return temp;
    }
    else
    {
        prop_not_found( propname );
        return ""; // this is not reached, prop_not_found throws
    }
}
string ConfigElem::read_string( const char *propname, const char *dflt ) const
{
    string temp;
    if (read_prop( propname, &temp))
        return temp;
    else
        return dflt;
}

string ConfigElem::remove_string( const char *propname, const char *dflt )
{
    string temp;
    if (remove_prop( propname, &temp ))
        return temp;
    else
        return dflt;
}

bool ConfigElem::remove_bool( const char *propname )
{
    return remove_ushort( propname ) ? true : false;
}

bool ConfigElem::remove_bool( const char *propname, bool dflt )
{		
	return remove_ushort( propname, dflt ) ? true : false;

}

float ConfigElem::remove_float( const char *propname, float dflt )
{
    string tmp;
    if (remove_prop( propname, &tmp ))
    {
        return static_cast<float>(strtod( tmp.c_str(), NULL ));
    }
    else
    {
        return dflt;
    }
}
double ConfigElem::remove_double( const char *propname, double dflt )
{
    string tmp;
    if (remove_prop( propname, &tmp ))
    {
        return strtod( tmp.c_str(), NULL );
    }
    else
    {
        return dflt;
    }
}

unsigned long ConfigElem::remove_ulong( const char *propname )
{
    unsigned long temp;
    if (remove_prop( propname, &temp))
    {
        return temp;
    }
    else
    {
        prop_not_found( propname );
        return 0; // this is not reached, prop_not_found throws
    }
}

unsigned long ConfigElem::remove_ulong( const char *propname, unsigned long dflt )
{
    unsigned long temp;
    if (remove_prop( propname, &temp))
        return temp;
    else
        return dflt;
}

void ConfigElem::clear_prop( const char *propname )
{
    unsigned long dummy;
    while (remove_prop( propname, &dummy ))
        continue;
}

void ConfigElem::add_prop( const char* propname, const char* propval )
{
    properties.insert( make_pair(string(propname), string(propval)) );
}
void VectorConfigElem::add_prop( const char* propname, const char* propval )
{
    ConfigProperty* prop;

    prop = new ConfigProperty( propname, propval );

    properties.push_back( prop );
}

void ConfigElem::add_prop( const char* propname, unsigned short sval )
{
    OSTRINGSTREAM os;
    os << sval;

    properties.insert( make_pair(string(propname), OSTRINGSTREAM_STR(os)) );
}
void ConfigElem::add_prop( const char* propname, short sval )
{
    OSTRINGSTREAM os;
    os << sval;

    properties.insert( make_pair(string(propname), OSTRINGSTREAM_STR(os)) );
}

void VectorConfigElem::add_prop( const char* propname, unsigned short sval )
{
    ConfigProperty* prop;
    OSTRINGSTREAM os;
    os << sval;

    prop = new ConfigProperty( propname, OSTRINGSTREAM_STR(os) );

    properties.push_back( prop );
}

void ConfigElem::add_prop( const char* propname, unsigned long lval )
{
    OSTRINGSTREAM os;
    os << lval;

    properties.insert( make_pair(string(propname), OSTRINGSTREAM_STR(os)) );
}
void VectorConfigElem::add_prop( const char* propname, unsigned long lval )
{
    ConfigProperty* prop;
    OSTRINGSTREAM os;
    os << lval;

    prop = new ConfigProperty( propname, OSTRINGSTREAM_STR(os) );

    properties.push_back( prop );
}

bool VectorConfigElem::remove_first_prop( std::string* propname,
                                   std::string* value )
{
	if (properties.empty())
		return false;
	
	ConfigProperty *prop = properties.front();
	*propname = prop->name_;
	*value = prop->value_;
	delete prop;
    properties.erase( properties.begin() );
	return true;
}

ConfigFile::ConfigFile( const char *i_filename,
                        const char *allowed_types_str ) :
    _filename("<n/a>"),
    _modified(0),
#if CFGFILE_USES_IOSTREAMS
    ifs(),
#else
    fp(NULL),
#endif
    _element_line_start(0),
    _cur_line(0)
{
    init( i_filename, allowed_types_str );
}

ConfigFile::ConfigFile( const string& i_filename,
                        const char *allowed_types_str ) :
    _filename("<n/a>"),
    _modified(0),
#if CFGFILE_USES_IOSTREAMS
    ifs(),
#else
    fp(NULL),
#endif
    _element_line_start(0),
    _cur_line(0)
{
    init( i_filename.c_str(), allowed_types_str );
}

void ConfigFile::init( const char* i_filename, const char* allowed_types_str )
{
    if (i_filename)
    {
        open( i_filename );
    }

    if (allowed_types_str != NULL)
    {
        ISTRINGSTREAM is( allowed_types_str );
        string tag;
        while (is >> tag)
        {
            allowed_types_.insert( tag.c_str() );
        }
    }
}

const string& ConfigFile::filename() const
{
    return _filename;
}
const time_t ConfigFile::modified() const
{
    return _modified;
}
unsigned ConfigFile::element_line_start() const
{
    return _element_line_start;
}

void ConfigFile::open( const char *i_filename )
{
    _filename = i_filename;

#if CFGFILE_USES_IOSTREAMS
    ifs.open( i_filename, ios::in );
    
    if (!ifs.is_open())
    {
        cerr << "Unable to open configuration file " << _filename << endl;
        throw runtime_error( string( "Unable to open configuration file " ) + _filename );
    }
#else
    fp = fopen( i_filename, "rt" );
    if (!fp)
    {
        cerr << "Unable to open configuration file " << _filename << endl;
        throw runtime_error( string( "Unable to open configuration file " ) + _filename );
    }
#endif

    struct stat cfgstat;
    stat( i_filename, &cfgstat );
    _modified = cfgstat.st_mtime;
}

ConfigFile::~ConfigFile()
{
#if !CFGFILE_USES_IOSTREAMS
    if (fp)
        fclose(fp);
    fp = NULL;
#endif
}

#if !CFGFILE_USES_IOSTREAMS
char ConfigFile::buffer[ 1024 ];
#endif

#if CFGFILE_USES_IOSTREAMS
// returns true if ended on a }, false if ended on EOF.
bool ConfigFile::read_properties( ConfigElem& elem )
{
    string strbuf;
    string propname, propvalue;
	while (getline( ifs, strbuf )) // get
	{
        ++cur_line;

        ISTRINGSTREAM is( strbuf );

        
        splitnamevalue( strbuf, propname, propvalue );

		if (propname.empty() ||                     // empty line
            commentline( propname ))
        {
            continue;
        }
            
		if (propname == "}")
			return true;
		
        if (propvalue[0] == '\"')
            convertquotedstring( propvalue );

		ConfigProperty *prop = new ConfigProperty( &propname, &propvalue );
		elem.properties.push_back( prop );
	}
    return false;
}


bool ConfigFile::_read( ConfigElem& elem )
{
    while (!elem.properties.empty())
    {
        delete elem.properties.back();
        elem.properties.pop_back();
    }
    elem.type_ = "";
    elem.rest_ = "";

    element_line_start = 0;

    string strbuf;
	while ( getline( ifs, strbuf ))
	{
        ++cur_line;

        string type, rest;
        splitnamevalue( strbuf, type, rest );
		
		if (type.empty() || 					// empty line
            commentline( type ))
		{	
			continue;
		}
		
        element_line_start = cur_line;

		elem.type_ = type;

        if (!allowed_types_.empty())
        {
            if (allowed_types_.find( type.c_str() ) == allowed_types_.end())
            {
                e_ostringstream os;
                os << "Unexpected type '" << type << "'" << endl;
                os << "\tValid types are:";
                for( AllowedTypesCont::const_iterator itr = allowed_types_.begin();
                     itr != allowed_types_.end();
                     ++itr)
                {
                    os << " " << (*itr).c_str();
                }
                throw runtime_error( os.e_str() );
            }
        }

        elem.rest_ = rest;
		
		if (! getline( ifs, strbuf ))
			throw runtime_error( "File ends after element type -- expected a '{'" );
	    ++cur_line;

		if (strbuf.empty() ||
            strbuf[0] != '{')
		{
			throw runtime_error( "Expected '{' on a blank line after element type" );
		}

        if (read_properties( elem ))
            return true;
        else
            throw runtime_error( "Expected '}' on a blank line after element properties" );
	}
	return false;
}
#else

bool ConfigFile::readline( string& strbuf )
{
    if (!fgets( buffer, sizeof buffer, fp ))
        return false;

    strbuf = "";

    do
    {
        char* nl = strchr( buffer, '\n' );
        if (nl)
        {
            if (nl != buffer && nl[-1] == '\r')
                --nl;
            *nl = '\0';
            strbuf += buffer;
            return true;
        }
        else
        {
            strbuf += buffer;
        }
    } while (fgets( buffer, sizeof buffer, fp ));

    return true;
}

// returns true if ended on a }, false if ended on EOF.
bool ConfigFile::read_properties( ConfigElem& elem )
{
    static string strbuf;
    static string propname, propvalue;
	while (readline( strbuf ))
    {
        ++_cur_line;

        ISTRINGSTREAM is( strbuf );

        
        splitnamevalue( strbuf, propname, propvalue );

		if (propname.empty() ||                     // empty line
            commentline( propname ))
        {
            continue;
        }
            
		if (propname == "}")
			return true;
		
        if (propvalue[0] == '\"')
        {
            decodequotedstring( propvalue );
        }

        elem.properties.insert( make_pair(propname, propvalue) );
	}
    return false;
}
bool ConfigFile::read_properties( VectorConfigElem& elem )
{
    static string strbuf;
    static string propname, propvalue;
	while (readline( strbuf ))
    {
        ++_cur_line;

        ISTRINGSTREAM is( strbuf );

        
        splitnamevalue( strbuf, propname, propvalue );

		if (propname.empty() ||                     // empty line
            commentline( propname ))
        {
            continue;
        }
            
		if (propname == "}")
			return true;
		
        if (propvalue[0] == '\"')
        {
            decodequotedstring( propvalue );
        }

		ConfigProperty *prop = new ConfigProperty( &propname, &propvalue );
		elem.properties.push_back( prop );
	}
    return false;
}


bool ConfigFile::_read( ConfigElem& elem )
{
    elem.properties.clear();

    elem.type_ = "";
    elem.rest_ = "";

    _element_line_start = 0;

	string strbuf;
    while (readline( strbuf ))
    {
        ++_cur_line;

        string type, rest;
        splitnamevalue( strbuf, type, rest );
		
		if (type.empty() || 					// empty line
			commentline( type ))
		{	
			continue;
		}
		
        _element_line_start = _cur_line;

		elem.type_ = type;

        if (!allowed_types_.empty())
        {
            if (allowed_types_.find( type.c_str() ) == allowed_types_.end())
            {
                OSTRINGSTREAM os;
                os << "Unexpected type '" << type << "'" << endl;
                os << "\tValid types are:";
                for( AllowedTypesCont::const_iterator itr = allowed_types_.begin();
                     itr != allowed_types_.end();
                     ++itr)
                {
                    os << " " << (*itr).c_str();
                }
                throw runtime_error( OSTRINGSTREAM_STR(os) );
            }
        }

        elem.rest_ = rest;
		
		if (!fgets( buffer, sizeof buffer, fp ))
			throw runtime_error( "File ends after element type -- expected a '{'" );
	    strbuf = buffer;
        ++_cur_line;

		if (strbuf.empty() ||
            strbuf[0] != '{')
		{
			throw runtime_error( "Expected '{' on a blank line after element type" );
		}

        if (read_properties( elem ))
            return true;
        else
            throw runtime_error( "Expected '}' on a blank line after element properties" );
	}
	return false;
}

bool ConfigFile::_read( VectorConfigElem& elem )
{
    while (!elem.properties.empty())
    {
        delete elem.properties.back();
        elem.properties.pop_back();
    }
    elem.type_ = "";
    elem.rest_ = "";

    _element_line_start = 0;

	string strbuf;
    while (readline( strbuf ))
    {
        ++_cur_line;

        string type, rest;
        splitnamevalue( strbuf, type, rest );
		
		if (type.empty() || 					// empty line
			commentline( type ))
		{	
			continue;
		}
		
        _element_line_start = _cur_line;

		elem.type_ = type;

        if (!allowed_types_.empty())
        {
            if (allowed_types_.find( type.c_str() ) == allowed_types_.end())
            {
                OSTRINGSTREAM os;
                os << "Unexpected type '" << type << "'" << endl;
                os << "\tValid types are:";
                for( AllowedTypesCont::const_iterator itr = allowed_types_.begin();
                     itr != allowed_types_.end();
                     ++itr)
                {
                    os << " " << (*itr).c_str();
                }
                throw runtime_error( OSTRINGSTREAM_STR(os) );
            }
        }

        elem.rest_ = rest;
		
		if (!fgets( buffer, sizeof buffer, fp ))
			throw runtime_error( "File ends after element type -- expected a '{'" );
	    strbuf = buffer;
        ++_cur_line;

		if (strbuf.empty() ||
            strbuf[0] != '{')
		{
			throw runtime_error( "Expected '{' on a blank line after element type" );
		}

        if (read_properties( elem ))
            return true;
        else
            throw runtime_error( "Expected '}' on a blank line after element properties" );
	}
	return false;
}


#endif

void ConfigFile::display_error( const string& msg, 
                                bool show_curline,
                                const ConfigElemBase* elem,
                                bool error) const
{
    bool showed_elem_line = false;

    cerr << (error?"Error":"Warning")
         << " reading configuration file " << _filename << ":" << endl;

    cerr << "\t" << msg << endl;
    
    if (elem != NULL)
    {
        if (strlen(elem->type()) > 0)
        {
            cerr << "\tElement: " << elem->type() << " " << elem->rest();
            if (_element_line_start)
                cerr << ", found on line " << _element_line_start;
            cerr << endl;
            showed_elem_line = true;
        }
    }

    if (show_curline)
        cerr << "\tNear line: " << _cur_line << endl;
    if (_element_line_start && !showed_elem_line)
        cerr << "\tElement started on line: " << _element_line_start << endl;
}

void ConfigFile::display_and_rethrow_exception()
{
    try 
    {
        throw;
    }
    catch( const char * msg )
    {
        display_error( msg );
    }
    catch( string& str )
    {
        display_error( str );
    }
    catch( exception& ex )
    {
        display_error( ex.what() );
    }
    catch( ... )
    {
        display_error( "(Generic exception)" );
    }

    throw runtime_error( "Configuration file error." );
}

bool ConfigFile::read( ConfigElem& elem )
{
    try 
    {
        elem._source = this;
        return _read( elem );
    }
    catch( ... ) 
    {
        display_and_rethrow_exception();
        return false;
    }
}

void ConfigFile::readraw( ConfigElem& elem )
{
    try {
        elem._source = this;
        if (read_properties( elem ))
            throw runtime_error( "unexpected '}' in file" );
    }
    catch( ... )
    {
        display_and_rethrow_exception();
    }
}

void StubConfigSource::display_error( const std::string& msg, 
                        bool show_curline,
                        const ConfigElemBase* elem,
                        bool error ) const
{
    cerr << (error?"Error":"Warning")
        << " reading configuration element:"
        << "\t" << msg << endl;
}

