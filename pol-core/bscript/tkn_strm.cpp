/*
History
=======

Notes
=======

*/

#include "../clib/stl_inc.h"

#include <string.h>

#include "modules.h"
#include "tokens.h"
#include "token.h"
#include "objmembers.h"
#include "objmethods.h"


void Token::printOn(ostream& os) const
{

    switch( id )
    {
        case TOK_LONG:              os << lval << "L";      break;
        case TOK_DOUBLE:            os << dval << "LF";     break;
        case TOK_IDENT:             os << token;            break;
        case INS_ADDMEMBER2:        os << "addmember(" << token << ")"; break;
        case INS_ADDMEMBER_ASSIGN:        os << "addmember-assign(" << token << ")"; break;
        case TOK_STRING:            os << '\"' << token << '\"';    break;
        case TOK_LOCALVAR:          
            os << "local #" << lval; 
            if (token != NULL)
                os << " (" << token << ")";
            break;
        case TOK_GLOBALVAR:         
            os << "global #" << lval; 
            if (token != NULL)
                os << " (" << token << ")";
            break;
        
        case TOK_ERROR:             os << "error";   break;
        case TOK_DICTIONARY:        os << "dictionary"; break;

		case TOK_MULT:              os << "*";              break;
		case TOK_DIV:               os << "/";              break;
		case TOK_ADD:               os << "+";              break;
		
		case TOK_INSERTINTO:		os << "init{}";			break;
		case TOK_SUBTRACT:          os << "-";              break;
		case TOK_PLUSEQUAL:         os << "+=";             break;
		case TOK_MINUSEQUAL:        os << "-=";             break;
		case TOK_TIMESEQUAL:		os << "*=";				break;
		case TOK_DIVIDEEQUAL:		os << "/=";				break;
		case TOK_MODULUSEQUAL:		os << "%=";				break;
		
		case TOK_ASSIGN:            os << ":=";             break;
        case INS_ASSIGN_LOCALVAR:   os << "local" << lval;
                                    if (token) os << " (" << token << ")";
                                    os << " := "; break;
        case INS_ASSIGN_GLOBALVAR:  os << "global" << lval;
                                    if (token) os << " (" << token << ")";
                                    os << " := "; break;
        case INS_ASSIGN_CONSUME:    os << ":= #";           break;
        case INS_SUBSCRIPT_ASSIGN_CONSUME:    os << "[] := (" << lval << ") #";           break;
		case INS_SUBSCRIPT_ASSIGN:      os << "[] := (" << lval << ")";  break;
        case TOK_LESSTHAN:          os << "<";              break;
		case TOK_LESSEQ:            os << "<=";             break;
		case TOK_GRTHAN:            os << ">";              break;
		case TOK_GREQ:              os << ">=";             break;
		case TOK_EQUAL1:            os << "=";              break;
		case TOK_EQUAL:             os << "==";             break;
		case TOK_NEQ:               os << "<>";             break;
		case TOK_AND:               os << "&&";             break;
		case TOK_OR:                os << "||";             break;
		case TOK_ARRAY_SUBSCRIPT:   os << "[] " << lval;    break;
        case INS_MULTISUBSCRIPT: 
                os << "["; 
                for( long i=1;i<lval;++i) 
                    os << ",";
                os << "]"; 
                break;
        case INS_MULTISUBSCRIPT_ASSIGN: 
                os << "["; 
                for( long i=1;i<lval;++i) 
                    os << ",";
                os << "] :="; 
                break;
		case TOK_ADDMEMBER:         os << ".+";			    break;
		case TOK_DELMEMBER:         os << ".-";			    break;
		case TOK_CHKMEMBER:         os << ".?";			    break;
		case TOK_MEMBER:            os << ".";			    break;
        case INS_GET_MEMBER:        os << "get member '" << token << "'"; break;
        case INS_SET_MEMBER:        os << "set member '" << token << "'"; break;
        case INS_SET_MEMBER_CONSUME:os << "set member '" << token << "' #"; break;
        case INS_GET_MEMBER_ID:     os << "get member id '" << getObjMember(lval)->code << "' (" << lval << ")"; break;
        case INS_SET_MEMBER_ID:     os << "set member id '" << getObjMember(lval)->code << "' (" << lval << ")"; break;
        case INS_SET_MEMBER_ID_CONSUME: os << "set member id '" << getObjMember(lval)->code << "' (" << lval << ") #"; break;
        case INS_CALL_METHOD_ID:    
			os << "Call Method id ";
			//cout << "getObjMethod(" << (long)lval << ")\n";
			if( getObjMethod((long)lval) != NULL )
				os << getObjMethod((long)lval)->code;
			else
				os << "NULL" << "(" << lval << ")";
			os << " (#";
			os << lval;
			os	<< ", ";
			os << type;
			os << " params)";
			break;
        case TOK_IN:                os << "in";             break;
        case INS_DICTIONARY_ADDMEMBER: os << "add dictionary member"; break;
		
        case TOK_UNPLUS:            os << "unary +";        break;
		case TOK_UNMINUS:           os << "unary -";        break;
		case TOK_LOG_NOT:           os << "!";              break;
		case TOK_CONSUMER:          os << "#";              break;
        case TOK_REFTO:             os << "refto";          break;
        case TOK_BITAND:            os << "&";              break;
        case TOK_BITOR:             os << "|";              break;
		case TOK_BSRIGHT:			os << ">>";				break;
		case TOK_BSLEFT:			os << "<<";				break;
		case TOK_BITXOR:            os << "^";              break;
		case TOK_BITWISE_NOT:       os << "~";              break;
        case TOK_MODULUS:           os << "%";              break;

        case INS_INITFOREACH:       os << "initforeach @" << lval; break;
        case INS_STEPFOREACH:       os << "stepforeach @" << lval; break;
        case INS_CASEJMP:           os << "casejmp"; break;
        case INS_INITFOR:           os << "initfor @" << lval; break;
        case INS_NEXTFOR:           os << "nextfor @" << lval; break;

		case TOK_TERM:				os << "Terminator";		break;

		case TOK_LPAREN:            os << "(";              break;
		case TOK_RPAREN:            os << ")";              break;
		case TOK_LBRACKET:          os << "[";              break;
		case TOK_RBRACKET:          os << "]";              break;
        case TOK_LBRACE:            os << "{";              break;
        case TOK_RBRACE:            os << "}";              break;

		case RSV_JMPIFTRUE:         os << "if true goto " << lval;  break;
		case RSV_JMPIFFALSE:        os << "if false goto " << lval; break;
		case RSV_ST_IF:             os << "if"; break;
		case RSV_GOTO:              os << "goto" << lval;   break;
		case RSV_GOSUB:             os << "gosub" << lval;  break;
		case RSV_EXIT:              os << "exit";           break;
		case RSV_RETURN:            os << "return";         break;
		case RSV_LOCAL:             os << "decl local #" << lval;   break;
		case RSV_GLOBAL:            os << "decl global #" << lval;    break;
        case RSV_VAR:               os << "var";            break;
        case RSV_CONST:             os << "const";          break;
        case RSV_DECLARE:           os << "declare";        break;
        case RSV_FUNCTION:          os << "function";       break;
        case RSV_ENDFUNCTION:       os << "endfunction";    break;
		case RSV_DO:                os << "do";             break;
		case RSV_DOWHILE:           os << "dowhile";        break;
        case RSV_WHILE:             os << "while";          break;
        case RSV_ENDWHILE:          os << "endwhile";       break;
        case RSV_REPEAT:            os << "repeat";         break;
        case RSV_UNTIL:             os << "until";          break;
        case RSV_FOR:               os << "for";            break;
        case RSV_ENDFOR:            os << "endfor";         break;
        case RSV_FOREACH:           os << "foreach";        break;
        case RSV_ENDFOREACH:        os << "endforeach";     break;
        case INS_DECLARE_ARRAY:     os << "declare array";          break;
        case TOK_ARRAY:             os << "array";          break;
        case TOK_STRUCT:            os << "struct";         break;
        case INS_UNINIT:            os << "uninit";         break;
        case RSV_USE_MODULE:        os << "use module";     break;
        case RSV_INCLUDE_FILE:      os << "include file";   break;

        case CTRL_LABEL:            os<< token << ":";		break;

		case TOK_COMMA:             os << "','";  		break;
		case TOK_SEMICOLON:         os << "';'";  		break;

        case CTRL_STATEMENTBEGIN:	os << "[" << (token?token:"--source not available--") << "]";  break;
		case CTRL_PROGEND:			os << "progend";    break;
		case CTRL_MAKELOCAL:		os << "makelocal";  break;
		case CTRL_JSR_USERFUNC:     os << "jmp userfunc @" << lval;     break;
		case INS_POP_PARAM_BYREF:	os << "pop param byref '" << token << "'";break;
		case INS_POP_PARAM:		    os << "pop param '" << token << "'";break;
        case INS_GET_ARG:           os << "get arg '" << token << "'"; break;
        case CTRL_LEAVE_BLOCK:      os << "leave block(" << lval << ")";break;

        case INS_CALL_METHOD:       os << "Call Method " << token << " (" << lval << " params)"; break;
        case TOK_USERFUNC:          os << "User Function " << (token?token:"--function name not available--");    break;
        
        case RSV_COLON:             os << "':'"; break;
        case RSV_PROGRAM:           os << "program"; break;
        case RSV_ENDPROGRAM:        os << "endprogram"; break;
        case RSV_ENUM:              os << "enum"; break;
        case RSV_ENDENUM:           os << "endenum"; break;

            
        case TOK_FUNC:
	        {
		        os << "Func(" << (int)module << "," << lval << "): ";
		        if (token) 
                    os << token;
		        else 
                    os << "<unknown>";
		        return;
	        }

        default:
		    os << "Unknown Token: (" << int(id) << "," << int(type);
			if (token) os << ",'" << token << "'";
			os << ")";
		    break;
    }
}

ostream& operator << (ostream& os, const Token& tok)
{
   tok.printOn(os);
   return os;
}
