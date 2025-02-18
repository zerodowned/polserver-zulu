/*
History
=======

Notes
=======

*/

#ifndef BSCRIPT_BERROR_H
#define BSCRIPT_BERROR_H

#include "bstruct.h"

class BError : public BStruct
{
public:
    BError();
    explicit BError( const char *errortext ); 
    explicit BError( const string& errortext );

    static BObjectImp* unpack( istream& is );

    static unsigned long creations();

protected:
    BError( const BError& i );
    BError( istream& is, unsigned size );
    
    virtual BObjectImp* copy() const;
    virtual BObjectRef OperSubscript( const BObject& obj );
    virtual BObjectImp* array_assign( BObjectImp* idx, BObjectImp* target, bool copy );

    virtual char packtype() const;
    virtual const char* typetag() const;
    virtual const char* typeOf() const;
	virtual int typeOfInt() const;    

    virtual bool isEqual(const BObjectImp& objimp ) const;
    virtual bool isTrue() const;

    ContIterator* createIterator( BObject* pIterVal );

private:
    static unsigned long creations_;
};

#endif
