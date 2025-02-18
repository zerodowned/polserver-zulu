/*
History
=======

Notes
=======

*/

#ifndef __TOKEN_H
#define __TOKEN_H

#ifndef __TOKENS_H
#include "tokens.h"
#endif
#ifndef __MODULES_H
#include "modules.h"
#endif

#include "options.h"

#include <iosfwd>
#include <set>

typedef struct {
    unsigned sourceFile;
    unsigned offset;
    unsigned strOffset;
} DebugToken;

class UserFunction;

class Token
{
  public:
    BTokenId id;
    BTokenType type;
    unsigned char module;

    union {
       int precedence;
       int sourceFile;
    };

	bool deprecated;

    int ownsStr;
    double dval;
    
    union {
        long lval;
        const unsigned char* dataptr;
    };

	UserFunction *userfunc;

    int dbg_filenum;
    int dbg_linenum;

    static unsigned long instances();
    static void show_instances();
  protected:
    const char *token;
#if STORE_INSTANCELIST
    typedef set<Token*> Instances;
    static Instances _instancelist;
#endif
    static unsigned long _instances;
    void register_instance();
    void unregister_instance();

  public:
  const char *tokval() const { return token; }

  Token();
  Token(const Token& tok);
  Token& operator=( const Token& tok );

    Token( ModuleID module,  BTokenId id, BTokenType type );
    Token( BTokenId id, BTokenType type );
	Token( ModuleID module, BTokenId id, BTokenType type, UserFunction *userfunc ); 
  void nulStr();
  void setStr(const char *s);
  void copyStr(const char *s);
  void copyStr(const char *s, int len);

  ~Token();

  void printOn(ostream& outputStream) const;
};

ostream& operator << (ostream&, const Token& );

#endif
