#include "Parser.hpp"





Parser::Parser()
    : MilaContext()
    , MilaBuilder(MilaContext)
    , MilaModule("mila", MilaContext)
{
}

bool Parser::Parse()
{
    getNextToken();
    tree=Start();
    return true;
}

const llvm::Module& Parser::Generate()
{
    //tree->Write(this);
    SymbTable st(this);
    tree->Generate(st);
    return this->MilaModule;
}

/**
 * @brief Simple token buffer.
 *
 * CurTok is the current token the parser is looking at
 * getNextToken reads another token from the lexer and updates curTok with ts result
 * Every function in the parser will assume that CurTok is the cureent token that needs to be parsed
 */
int Parser::getNextToken()
{

    return CurTok = m_Lexer.gettok();
}
