/*
History
=======
2005/09/10 Shinigami: added mf_Root - calculates y Root of x as a Real
2006/10/07 Shinigami: GCC 3.4.x fix - added "template<>" to TmplExecutorModule
                      removed obsolete gcvt, used sprintf

Notes
=======

*/

#include "../../clib/stl_inc.h"

#include <math.h>

#include "../../bscript/berror.h"
#include "../../bscript/impstr.h"
#include "mathmod.h"

static double const_pi;
static double const_e;

class initer
{
public:
    initer();
};
initer::initer()
{
    const_pi = acos(double(-1));
    const_e = exp(double(1));
}
initer _initer;

template<>
TmplExecutorModule<MathExecutorModule>::FunctionDef   
TmplExecutorModule<MathExecutorModule>::function_table[] = 
{
	{ "Sin",                    &MathExecutorModule::mf_Sin },
	{ "Cos",                    &MathExecutorModule::mf_Cos },
	{ "Tan",                    &MathExecutorModule::mf_Tan },
	{ "ASin",                   &MathExecutorModule::mf_ASin },
	{ "ACos",                   &MathExecutorModule::mf_ACos },
	{ "ATan",                   &MathExecutorModule::mf_ATan },

	{ "Min",                    &MathExecutorModule::mf_Min },
	{ "Max",                    &MathExecutorModule::mf_Max },
	{ "Pow",                    &MathExecutorModule::mf_Pow },
	{ "Sqrt",                   &MathExecutorModule::mf_Sqrt },
	{ "Root",                   &MathExecutorModule::mf_Root },
	{ "Abs",                    &MathExecutorModule::mf_Abs },
	{ "Log10",                  &MathExecutorModule::mf_Log10 },
	{ "LogE",                   &MathExecutorModule::mf_LogE },

	{ "DegToRad",               &MathExecutorModule::mf_DegToRad },
	{ "RadToDeg",               &MathExecutorModule::mf_RadToDeg },

	{ "Ceil",                   &MathExecutorModule::mf_Ceil },
	{ "Floor",                  &MathExecutorModule::mf_Floor },

	{ "ConstPi",                &MathExecutorModule::mf_ConstPi },
	{ "ConstE",                 &MathExecutorModule::mf_ConstE },

	{ "FormatRealToString",     &MathExecutorModule::mf_FormatRealToString }
};

template<>
int TmplExecutorModule<MathExecutorModule>::function_table_size = arsize(function_table);

BObjectImp* MathExecutorModule::mf_Sin()
{
    double x;
    if (getRealParam( 0, x ))
    {
        return new Double( sin(x) );
    }
    else
    {
        return new BError( "Invalid parameter type" );
    }
}
BObjectImp* MathExecutorModule::mf_Cos()
{
    double x;
    if (getRealParam( 0, x ))
    {
        return new Double( cos(x) );
    }
    else
    {
        return new BError( "Invalid parameter type" );
    }
}
BObjectImp* MathExecutorModule::mf_Tan()
{
    double x;
    if (getRealParam( 0, x ))
    {
        return new Double( tan(x) );
    }
    else
    {
        return new BError( "Invalid parameter type" );
    }
}
BObjectImp* MathExecutorModule::mf_ASin()
{
    double x;
    if (getRealParam( 0, x ))
    {
        return new Double( asin(x) );
    }
    else
    {
        return new BError( "Invalid parameter type" );
    }
}
BObjectImp* MathExecutorModule::mf_ACos()
{
    double x;
    if (getRealParam( 0, x ))
    {
        return new Double( acos(x) );
    }
    else
    {
        return new BError( "Invalid parameter type" );
    }
}
BObjectImp* MathExecutorModule::mf_ATan()
{
    double x;
    if (getRealParam( 0, x ))
    {
        return new Double( atan(x) );
    }
    else
    {
        return new BError( "Invalid parameter type" );
    }
}

/*
math::Pow(x : number, y : number ) : real
	Function: calculates x raised to the power of y
		Returns:  x^y as a Real
	Parameters: x and y can be Reals or Integers
  
math::Pow(2,5) = 2^5 = 32
*/
BObjectImp* MathExecutorModule::mf_Pow()
{
    double x, y;
    if (getRealParam( 0, x ) &&
        getRealParam( 1, y ))
    {
        return new Double( pow(x,y) );
    }
    else
    {
        return new BError( "Invalid parameter type" );
    }
}

/*
math::Sqrt( x : number ) : real
	Returns: Square Root of x as a Real
*/
BObjectImp* MathExecutorModule::mf_Sqrt()
{
    double x;
    if (getRealParam( 0, x ))
    {
        return new Double( sqrt(x) );
    }
    else
    {
        return new BError( "Invalid parameter type" );
    }
}

/*
math::Root( x : number, y : number ) : real
	Function: calculates y Root of x as a Real
*/
BObjectImp* MathExecutorModule::mf_Root()
{
    double x, y;
    if (getRealParam( 0, x ) &&
        getRealParam( 1, y ))
    {
        return new Double( pow(x, 1.0/y) );
    }
    else
    {
        return new BError( "Invalid parameter type" );
    }
}

BObjectImp* MathExecutorModule::mf_Min()
{
	BObjectImp* impX = getParamImp( 0 );
	BObjectImp* impY = getParamImp( 1 );
	if (((impX->isa(BObjectImp::OTDouble)) || (impX->isa(BObjectImp::OTLong)))
		&& ((impY->isa(BObjectImp::OTDouble)) || (impY->isa(BObjectImp::OTLong))))
	{
		if (impX->isLessThan(*impY))
			return impX->copy();
		else
			return impY->copy();
	}
	else if(impX->isa(BObjectImp::OTArray))
	{
		ObjArray* value = static_cast<ObjArray*>(impX);
		if (value->ref_arr.size()==0)
			return new BError("Array empty");

		BObjectImp *compare=NULL;
		for( std::vector<BObjectRef>::iterator itr = value->ref_arr.begin(); itr != value->ref_arr.end(); ++itr )
		{
			if ( itr->get() )
			{
				BObject *bo = (itr->get());

				if ( bo == NULL )
					continue;
				if ((bo->isa(BObjectImp::OTDouble)) || (bo->isa(BObjectImp::OTLong)))
				{
					if (compare==NULL)
						compare=bo->impptr();
					else if ( bo->impptr()->isLessThan(*compare))
						compare=bo->impptr();
				}
			}
		}
		if (compare!=NULL)
			return(compare->copy());
		else
			return new BError("No Integer/Double elements");
	}
	else
		return new BError( "Invalid parameter type" );
}

BObjectImp* MathExecutorModule::mf_Max()
{
	BObjectImp* impX = getParamImp( 0 );
	BObjectImp* impY = getParamImp( 1 );
	if (((impX->isa(BObjectImp::OTDouble)) || (impX->isa(BObjectImp::OTLong)))
		&& ((impY->isa(BObjectImp::OTDouble)) || (impY->isa(BObjectImp::OTLong))))
	{
		if (impX->isLessThan(*impY))
			return impY->copy();
		else
			return impX->copy();
	}
	else if(impX->isa(BObjectImp::OTArray))
	{
		ObjArray* value = static_cast<ObjArray*>(impX);
		if (value->ref_arr.size()==0)
			return new BError("Array empty");

		BObjectImp *compare=NULL;
		for( std::vector<BObjectRef>::iterator itr = value->ref_arr.begin(); itr != value->ref_arr.end(); ++itr )
		{
			if ( itr->get() )
			{
				BObject *bo = (itr->get());

				if ( bo == NULL )
					continue;
				if ((bo->isa(BObjectImp::OTDouble)) || (bo->isa(BObjectImp::OTLong)))
				{
					if (compare==NULL)
						compare=bo->impptr();
					else if ( !(bo->impptr()->isLessThan(*compare)))
						compare=bo->impptr();
				}
			}
		}
		if (compare!=NULL)
			return(compare->copy());
		else
			return new BError("No Integer/Double elements");
	}
	else
		return new BError( "Invalid parameter type" );
}

/*
math::Abs( x : number) : number
	Returns: the absolute value of x
		If a Real is passed, will return a Real
		If an Integer is passed, will return an Integer

math::Abs(-2) = 2
math::Abs(-1.5) = 1.5
*/
BObjectImp* MathExecutorModule::mf_Abs()
{
    BObjectImp* imp = getParamImp( 0 );
    if (imp->isa( BObjectImp::OTDouble ))
    {
        double value = static_cast<Double*>(imp)->value();
        return new Double( fabs(value) );
    }
    else if (imp->isa( BObjectImp::OTLong ))
    {
        long value = static_cast<BLong*>(imp)->value();
        return new BLong( labs( value ) );
    }
    else
    {
        double x;
        // just for debug.log
		getRealParam(0,x);
        return new BError( "Invalid parameter type" );
    }
}

BObjectImp* MathExecutorModule::mf_Log10()
{
    double x;
    if (getRealParam( 0, x ))
    {
        return new Double( log10( x ) );
    }
    else
    {
        return new BError( "Invalid parameter type" );
    }
}
BObjectImp* MathExecutorModule::mf_LogE()
{
    double x;
    if (getRealParam( 0, x ))
    {
        return new Double( log( x ) );
    }
    else
    {
        return new BError( "Invalid parameter type" );
    }
}

BObjectImp* MathExecutorModule::mf_ConstPi()
{
    return new Double( const_pi ); 
}
BObjectImp* MathExecutorModule::mf_ConstE()
{
    return new Double( const_e );
}

BObjectImp* MathExecutorModule::mf_FormatRealToString()
{
    double x;
    long digits;
	
    if (getRealParam( 0, x ) &&
        getParam( 1, digits ))
    {
		std::stringstream result;
		result << std::setprecision(digits) << std::fixed;
		result << x;

        //char buffer[ 200 ];
		//sprintf( buffer, "%.*g", static_cast<int>(digits), x);		

		//return new String( buffer );

		return new String( result.str() );		
    }
    else
    {
        return new BError( "Invalid parameter type" );
    }
}

BObjectImp* MathExecutorModule::mf_RadToDeg()
{
    double rad;
    if (!getRealParam(0,rad))
        return new BError( "Invalid parameter type" );

/*
	360 degrees <~> 2*pi radians
	<=> deg/360 = rad / 2*pi
	<=> deg = 360*rad / 2*pi
	<=> deg = 180*rad / pi
*/

    return new Double( (rad * 180.0) / const_pi );
}

BObjectImp* MathExecutorModule::mf_DegToRad()
{
    double deg;
    if (!getRealParam(0,deg))
        return new BError( "Invalid parameter type" );

/*
	2*pi radians <~> 360 degrees
	<=> rad / 2*pi = deg / 360
	<=> rad = 2*pi*deg / 360
	<=> rad = deg * pi / 180
*/
    return new Double( (deg * const_pi) / 180.0 );
}

BObjectImp* MathExecutorModule::mf_Ceil()
{
    double x;
    if (getRealParam( 0, x ))
    {
        return new Double( ceil(x) );
    }
    else
    {
        return new BError( "Invalid parameter type" );
    }
}
BObjectImp* MathExecutorModule::mf_Floor()
{
    double x;
    if (getRealParam( 0, x ))
    {
        return new Double( floor(x) );
    }
    else
    {
        return new BError( "Invalid parameter type" );
    }
}

