/*
History
=======

Notes
=======

*/

#include "../clib/stl_inc.h"

// EPROG compiler-only functions
#include "../clib/clib.h"
#include "../clib/passert.h"
#include "../clib/stlutil.h"

#include "escriptv.h"
#include "filefmt.h"
#include "parser.h"
#include "userfunc.h"
#include "eprog.h"

extern int include_debug;

void EScriptProgram::erase()
{
    tokens.erase();
    symbols.erase();
    nglobals = 0;
    dbg_filenum.clear();
    dbg_linenum.clear();
    globalvarnames.clear();
    blocks.clear();
    dbg_functions.clear();
    dbg_ins_blocks.clear();
    dbg_ins_statementbegin.clear();

}

void EScriptProgram::update_dbg_pos( const Token& token )
{
    if (token.dbg_filenum != 0)
    {
        curfile = token.dbg_filenum;
        curline = token.dbg_linenum;
    }
}

void EScriptProgram::add_ins_dbg_info()
{
    dbg_filenum.push_back( curfile );
    dbg_linenum.push_back( curline );
    dbg_ins_blocks.push_back( curblock );
    dbg_ins_statementbegin.push_back( statementbegin );
    statementbegin = false;
    passert( tokens.count() == dbg_filenum.size() );
}

void EScriptProgram::setcontext( const CompilerContext& ctx )
{
    curfile = ctx.dbg_filenum;
    curline = ctx.line;
}

void EScriptProgram::append( const StoredToken& stoken )
{
    tokens.append_tok( stoken, 0 );
    
    add_ins_dbg_info();
}

void EScriptProgram::append( const StoredToken& stoken, unsigned* posn  )
{
    tokens.append_tok( stoken, posn );
    
    add_ins_dbg_info();
}

void EScriptProgram::append( const StoredToken& stoken, const CompilerContext& ctx )
{
    tokens.append_tok( stoken, 0 );
    
    setcontext( ctx );

    add_ins_dbg_info();
}

void EScriptProgram::append( const StoredToken& stoken, const CompilerContext& ctx, unsigned* posn  )
{
    tokens.append_tok( stoken, posn );

    setcontext( ctx );

    add_ins_dbg_info();
}

void EScriptProgram::addToken( const Token& token )
{
    unsigned tokpos = 0;
	if (token.id > 255)
	{
		throw runtime_error( "Trying to write an illegal token" );
	}

    update_dbg_pos( token );
    
    switch(token.type) 
	{
		case TYP_OPERAND: // is variable name, long, double, string lit
			{
				unsigned sympos = 0;
				switch(token.id) {
					case TOK_LONG:
						symbols.append(token.lval, sympos);
						break;
					case TOK_DOUBLE:
						symbols.append(token.dval, sympos);
						break;
					case TOK_STRING: // string literal
						symbols.append(token.tokval(), sympos);
						break;
                    case INS_ADDMEMBER2:
                    case INS_ADDMEMBER_ASSIGN:
                    case TOK_IDENT:  // unclassified (ick)
						symbols.append(token.tokval(), sympos);
						break;
                    case TOK_GLOBALVAR:
                    case TOK_LOCALVAR:
                        sympos = token.lval;
                        break;
                    
                    default:
                        break;
				}
				tokens.append_tok(	StoredToken(token.module, token.id, token.type, sympos),
								&tokpos);
			}
			break;

        case TYP_USERFUNC: // these don't do anything.
            return;

        case TYP_METHOD:
			{
                if(token.id != INS_CALL_METHOD_ID)
                {
				    unsigned sympos = 0;
                    passert( token.tokval() );

                    symbols.append(token.tokval(), sympos);
                
				    tokens.append_tok(
					    StoredToken(token.module,
						    token.id, 
						    static_cast<BTokenType>(token.lval),// # of params, stored in Token.lval, saved in StoredToken.type
						    sympos),
					    &tokpos);
                }
                else
                {
                    
				tokens.append_tok(
					StoredToken(token.module,
                        token.id,
                        static_cast<BTokenType>(token.lval),
                        static_cast<unsigned>(token.dval)),
				    &tokpos);
                    
                }
			}
			break;
        case TYP_FUNC:
			{
				unsigned sympos = 0;
				if (include_debug)
                {
                    if (token.tokval())
                        symbols.append(token.tokval(), sympos);
                }
				tokens.append_tok(
					StoredToken(token.module,
						token.id, 
						static_cast<BTokenType>(token.lval),// function index, stored in Token.lval, saved in StoredToken.type
						sympos),
					&tokpos);
                
			}
			break;
			
		case TYP_RESERVED:
		case TYP_OPERATOR:
		case TYP_UNARY_OPERATOR:
            if (token.id == TOK_ARRAY_SUBSCRIPT)
			{
				// array subscript operators store their
				// array index number in offset
				tokens.append_tok(
					StoredToken( token.module,
					             token.id,
								 token.type,
								 token.lval ) );
			}
            else if (token.id == INS_GET_MEMBER ||
                     token.id == INS_SET_MEMBER ||
                     token.id == INS_SET_MEMBER_CONSUME)
            {
                unsigned sympos = 0;
               	symbols.append(token.tokval(), sympos);
				tokens.append_tok(
					StoredToken(token.module,
                        token.id,
                        token.type,
                        sympos),
				    &tokpos);

            }
			else
			{
				tokens.append_tok(
					StoredToken(token.module,
                        token.id,
                        token.type,
                        token.lval),
				    &tokpos);
			}
			break;
		case TYP_CONTROL:
			switch( token.id )
			{
				case CTRL_MAKELOCAL:
					tokens.append_tok( StoredToken( token.module, 
						                         token.id,
												 token.type ) );
					break;
				
                case CTRL_JSR_USERFUNC:
				    tokens.append_tok( StoredToken( token.module,
												token.id,
												token.type,
												0 ), &tokpos );
       				token.userfunc->forward_callers.push_back( tokpos );
					break;
                
                case INS_POP_PARAM_BYREF:
                case INS_POP_PARAM:
					{
						unsigned sympos = 0;
						symbols.append( token.tokval(), sympos );
						tokens.append_tok(	StoredToken(token.module, token.id, token.type, sympos) );
					}
					break;
				
                default:
                    cerr << "AddToken: Can't handle TYP_CONTROL: " << token << endl;
					throw runtime_error( "Unexpected token in AddToken() (1)" );
					break;
			}
			break;
		default:
            cerr << "AddToken: Can't handle " << token << endl;
            throw runtime_error( "Unexpected Token passed to AddToken() (2)" );
        break;
    }

    add_ins_dbg_info();
}

int EScriptProgram::write( const char *fname )
{
    EScriptProgram& program = *this;
    FILE *fp = fopen(fname, "wb");
    if (!fp) 
		return -1;

	BSCRIPT_FILE_HDR hdr;
	hdr.magic2[0] = BSCRIPT_FILE_MAGIC0;
	hdr.magic2[1] = BSCRIPT_FILE_MAGIC1;
	hdr.version = ESCRIPT_FILE_VER_CURRENT; // auto-set to latest version (see filefmt.h)
    hdr.globals = static_cast<unsigned short>(globalvarnames.size());
	fwrite( &hdr, sizeof hdr, 1, fp );

	BSCRIPT_SECTION_HDR sechdr;

    if (haveProgram)
    {
        BSCRIPT_PROGDEF_HDR progdef_hdr;
        memset( &progdef_hdr, 0, sizeof progdef_hdr );
        sechdr.type = BSCRIPT_SECTION_PROGDEF;
        sechdr.length = sizeof progdef_hdr;
        fwrite( &sechdr, sizeof sechdr, 1, fp );
        progdef_hdr.expectedArgs = expectedArgs;
        fwrite( &progdef_hdr, sizeof progdef_hdr, 1, fp );
    }

	for( unsigned idx = 0; idx < program.modules.size(); idx++ )
	{
		FunctionalityModule* module = program.modules[ idx ];
		sechdr.type = BSCRIPT_SECTION_MODULE;
		sechdr.length = 0;
		fwrite( &sechdr, sizeof sechdr, 1, fp );

		BSCRIPT_MODULE_HDR modhdr;
		memset( &modhdr, 0, sizeof modhdr );
		strzcpy( modhdr.modulename, module->modulename.c_str(), sizeof modhdr.modulename );
		modhdr.nfuncs = module->used_functions.size();
		fwrite( &modhdr, sizeof modhdr, 1, fp );
		for( unsigned funcnum = 0; funcnum < module->used_functions.size(); funcnum++ )
		{
			BSCRIPT_MODULE_FUNCTION func;
			memset( &func, 0, sizeof func );
			passert( module->used_functions[funcnum]->used );
            
            strzcpy( func.funcname, module->used_functions[funcnum]->name.c_str(), sizeof func.funcname );
			func.nargs = static_cast<unsigned char>(module->used_functions[funcnum]->nargs);
            
			fwrite( &func, sizeof func, 1, fp );
		}
	}
	
	sechdr.type = BSCRIPT_SECTION_CODE;
	sechdr.length = program.tokens.get_write_length();
	fwrite( &sechdr, sizeof sechdr, 1, fp );
	program.tokens.write(fp);
    
	sechdr.type = BSCRIPT_SECTION_SYMBOLS;
	sechdr.length = program.symbols.get_write_length();
	fwrite( &sechdr, sizeof sechdr, 1, fp );
	program.symbols.write(fp);
    
    if (exported_functions.size())
    {
        BSCRIPT_EXPORTED_FUNCTION bef;
        sechdr.type = BSCRIPT_SECTION_EXPORTED_FUNCTIONS;
        sechdr.length = exported_functions.size() * sizeof bef;
        fwrite( &sechdr, sizeof sechdr, 1, fp );
        for( unsigned i = 0; i < exported_functions.size(); ++i )
        {
            strzcpy( bef.funcname, exported_functions[i].name.c_str(), sizeof bef.funcname );
            bef.nargs = exported_functions[i].nargs;
            bef.PC = exported_functions[i].PC;
            fwrite( &bef, sizeof bef, 1, fp );
        }
    }

	fclose(fp);
    return 0;
}


EScriptProgramCheckpoint::EScriptProgramCheckpoint( const EScriptProgram& prog )
{
    commit( prog );
}

void EScriptProgramCheckpoint::commit( const EScriptProgram& prog )
{
    module_count = prog.modules.size();
    tokens_count = prog.tokens.count();
    symbols_length = prog.symbols.length();
    sourcelines_count = prog.sourcelines.size();
    fileline_count = prog.fileline.size();
}

unsigned EScriptProgram::varcount( unsigned block )
{
    unsigned cnt = blocks[block].localvarnames.size();
    if (block != 0)
    {
        cnt += varcount( blocks[block].parentblockidx );
    }
    return cnt;
}
unsigned EScriptProgram::parentvariables( unsigned parent )
{
    unsigned cnt = 0;
    if (parent != 0)
    {
        cnt = blocks[parent].parentvariables + parentvariables(blocks[parent].parentblockidx);
    }
    return cnt;
}
int EScriptProgram::write_dbg( const char *fname, bool gen_txt )
{
    FILE* fp = fopen( fname, "wb" );
    if (!fp)
        return -1;
  
    FILE* fptext = NULL;
    if (gen_txt)
        fptext = fopen( (string(fname)+".txt").c_str(), "wt" );

    u32 count;
    
    // just a version number
    count = 3;
    fwrite( &count, sizeof count, 1, fp );

    count = dbg_filenames.size();
    fwrite( &count, sizeof count, 1, fp );
    for( unsigned i = 0; i < dbg_filenames.size(); ++i )
    {
        if (fptext)
            fprintf( fptext, "File %d: %s\n", i, dbg_filenames[i].c_str() );
        count = dbg_filenames[i].size()+1;
        fwrite( &count, sizeof count, 1, fp );
        fwrite( dbg_filenames[i].c_str(), count, 1, fp );
    }
    count = globalvarnames.size();
    fwrite( &count, sizeof count, 1, fp );
    for( unsigned i = 0; i < globalvarnames.size(); ++i )
    {
        if (fptext)
            fprintf( fptext, "Global %d: %s\n", i, globalvarnames[i].c_str() );
        count = globalvarnames[i].size()+1;
        fwrite( &count, sizeof count, 1, fp );
        fwrite( globalvarnames[i].c_str(), count, 1, fp );
    }
    count = tokens.count();
    fwrite( &count, sizeof count, 1, fp );
    for( unsigned i = 0; i < tokens.count(); ++i )
    {
        if (fptext)
            fprintf( fptext, 
                     "Ins %d: File %d, Line %d, Block %d %s\n", 
                     i, 
                     dbg_filenum[i], 
                     dbg_linenum[i], 
                     dbg_ins_blocks[i],
                     (dbg_ins_statementbegin[i]?"StatementBegin":""));
        BSCRIPT_DBG_INSTRUCTION ins;
        ins.filenum = dbg_filenum[i];
        ins.linenum = dbg_linenum[i];
        ins.blocknum = dbg_ins_blocks[i];
        ins.statementbegin = dbg_ins_statementbegin[i]?1:0;
        ins.rfu1 = 0;
        ins.rfu2 = 0;
        fwrite( &ins, sizeof ins, 1, fp );
    }
    count = blocks.size();
    fwrite( &count, sizeof count, 1, fp );
    for( unsigned i = 0; i < blocks.size(); ++i )
    {
        const EPDbgBlock& block = blocks[i];
        if (fptext)
        {
            fprintf( fptext, "Block %d:\n", i );
            fprintf( fptext, "  Parent block: %d\n", block.parentblockidx );
        }

        u32 tmp;
        tmp = block.parentblockidx;
        fwrite( &tmp, sizeof tmp, 1, fp );
        tmp = block.parentvariables;
        fwrite( &tmp, sizeof tmp, 1, fp );

        count = block.localvarnames.size();
        fwrite( &count, sizeof count, 1, fp );

        int varfirst = block.parentvariables;
        int varlast = varfirst + block.localvarnames.size()-1;
        if (varlast >= varfirst)
        {
            if (fptext)
                fprintf( fptext, "  Local variables %d-%d: \n", varfirst, varlast );
            for( unsigned j = 0; j < block.localvarnames.size(); ++j )
            {
                string name = block.localvarnames[j];
                if (fptext)
                    fprintf( fptext, "      %d: %s\n", varfirst+j, name.c_str() );

                count = name.size()+1;
                fwrite( &count, sizeof count, 1, fp );
                fwrite( name.c_str(), count, 1, fp );
            }
        }
    }
    count = dbg_functions.size();
    fwrite( &count, sizeof count, 1, fp );
    for( unsigned i = 0; i< dbg_functions.size(); ++i )
    {
        const EPDbgFunction& func = dbg_functions[i];
        if (fptext)
        {
            fprintf( fptext, "Function %d: %s\n", i, func.name.c_str() );
            fprintf( fptext, "  FirstPC=%u, lastPC=%u\n", func.firstPC, func.lastPC );
        }
        count = func.name.size()+1;
        fwrite( &count, sizeof count, 1, fp );
        fwrite( func.name.c_str(), count, 1, fp );
        u32 tmp;
        tmp = func.firstPC;
        fwrite( &tmp, sizeof tmp, 1, fp );
        tmp = func.lastPC;
        fwrite( &tmp, sizeof tmp, 1, fp );
    }

    if (fptext)
        fclose(fptext);
    fptext = NULL;
    fclose(fp);
    fp = NULL;

    return 0;
}

int EScriptProgram::add_dbg_filename( const string& filename )
{
    for( unsigned i = 0; i < dbg_filenames.size(); ++i )
    {
        if (dbg_filenames[i] == filename)
            return i;
    }
    
    // file 0 is nothing
    if (dbg_filenames.empty())
        dbg_filenames.push_back( "" );

    dbg_filenames.push_back( filename );
    return dbg_filenames.size()-1;
}

string EScriptProgram::dbg_get_instruction( unsigned atPC ) const
{
    OSTRINGSTREAM os;
    os << instr[ atPC ].token;
    return OSTRINGSTREAM_STR(os);
}


void EScriptProgram::setstatementbegin()
{
    statementbegin = true;
}
void EScriptProgram::enterfunction()
{
    savecurblock = curblock;
    curblock = 0;
}
void EScriptProgram::leavefunction()
{
    curblock = savecurblock;
}
void EScriptProgram::enterblock()
{
    EPDbgBlock block;
    if (!blocks.size())
    {
        block.parentblockidx = 0;
        block.parentvariables = 0;
        blocks.push_back( block );
        curblock = 0;
    }

    block.parentblockidx = curblock;
    block.parentvariables = varcount( curblock );

    curblock = blocks.size();
    blocks.push_back( block );
}
void EScriptProgram::leaveblock()
{
    passert( curblock );
    bool remove = blocks[curblock].localvarnames.empty() &&
                  curblock==blocks.size()-1;
    curblock = blocks[curblock].parentblockidx;
    if (remove)
    {
        blocks.pop_back();
        for( unsigned i = 0; i < dbg_ins_blocks.size(); ++i )
        {
            if (dbg_ins_blocks[i] >= blocks.size())
                dbg_ins_blocks[i] = curblock;
        }
    }
}
void EScriptProgram::addlocalvar( const string& localvarname )
{
    blocks[curblock].localvarnames.push_back( localvarname );
}
void EScriptProgram::addfunction( string name, unsigned firstPC, unsigned lastPC )
{
    EPDbgFunction func;
    func.name = name;
    func.firstPC = firstPC;
    func.lastPC = lastPC;
    dbg_functions.push_back( func );
}
