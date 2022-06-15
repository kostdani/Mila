#include "Parser.hpp"

void CompareError(int s) {

    if(s>0)
        printf("Error while comparing, expected %c.\n",s);
    else

        printf("Error while comparing, expected %d.\n",s);
    exit(1);
}

void ExpansionError(std::string nonterminal, int s) {
    printf("Error while expanding nonterminal %s, unexpected token %d.\n", nonterminal.c_str(),s);
    exit(1);
}


void Parser::Compare(int s) {
    if (CurTok == s)
        getNextToken();
    else
        CompareError(s);
}
const bool DEBUG=false;

Parser::SymbTable::SymbTable(Parser *p){
parser=p;
}

int Parser::SymbTable::AddConst(std::string name,llvm::Value* value){
    ConstTable[name]=value;
    return 1;
}
int Parser::SymbTable::AddVar(std::string name,llvm::Value* value){
    VarTable[name]=value;
    return 1;
}
int Parser::SymbTable::AddFunc(std::string name,llvm::FunctionCallee value){
    CallTable[name]=value;
    return 1;
}
llvm::Value *Parser::SymbTable::GetVal(std::string name){
    if (ConstTable.find(name) != ConstTable.end())
        return ConstTable[name];
    if (VarTable.find(name) != VarTable.end()) {
        auto a= parser->MilaBuilder.CreateLoad(llvm::Type::getInt32Ty(parser->MilaContext),VarTable[name], name.c_str());
        return a;
    }
    return 0;
}
llvm::Value *Parser::SymbTable::GetAddr(std::string name){
    if (VarTable.find(name) != VarTable.end())
        return VarTable[name];
    return 0;
}
llvm::FunctionCallee Parser::SymbTable::GetCallee(std::string name){
    if (CallTable.find(name) != CallTable.end())
        return CallTable[name];
    return 0;
}

int Parser::NProgram::Generate( Parser::SymbTable &SymbTable){
    // create writeln function
    {
        std::vector<llvm::Type*> Ints(1, llvm::Type::getInt32Ty(SymbTable.parser->MilaContext));
        llvm::FunctionType * FT = llvm::FunctionType::get(llvm::Type::getInt32Ty(SymbTable.parser->MilaContext), Ints, false);
        llvm::Function * F = llvm::Function::Create(FT, llvm::Function::ExternalLinkage, "writeln", SymbTable.parser->MilaModule);
        for (auto & Arg : F->args())
            Arg.setName("x");
    }
    // create write function
    {
        std::vector<llvm::Type*> Ints(1, llvm::Type::getInt32Ty(SymbTable.parser->MilaContext));
        llvm::FunctionType * FT = llvm::FunctionType::get(llvm::Type::getInt32Ty(SymbTable.parser->MilaContext), Ints, false);
        llvm::Function * F = llvm::Function::Create(FT, llvm::Function::ExternalLinkage, "write", SymbTable.parser->MilaModule);
        for (auto & Arg : F->args())
            Arg.setName("x");
    }
    // create readln function
    {
        std::vector<llvm::Type*> Ints(1, llvm::Type::getInt32PtrTy(SymbTable.parser->MilaContext));
        llvm::FunctionType * FT = llvm::FunctionType::get(llvm::Type::getInt32Ty(SymbTable.parser->MilaContext), Ints, false);
        llvm::Function * F = llvm::Function::Create(FT, llvm::Function::ExternalLinkage, "readln", SymbTable.parser->MilaModule);
        for (auto & Arg : F->args())
            Arg.setName("x");
    }
    // create write function
    {
        std::vector<llvm::Type*> Ints(1, llvm::Type::getInt32Ty(SymbTable.parser->MilaContext));
        llvm::FunctionType * FT = llvm::FunctionType::get(llvm::Type::getInt32Ty(SymbTable.parser->MilaContext), Ints, false);
        llvm::Function * F = llvm::Function::Create(FT, llvm::Function::ExternalLinkage, "dec", SymbTable.parser->MilaModule);
        for (auto & Arg : F->args())
            Arg.setName("x");
    }

    for (int i =0;i<decs.size();++i){
        decs[i]->PreDeclare(SymbTable);
        //      decs[i]->GlobDeclare(SymbTable);
    }
    // create main function
    {
        llvm::FunctionType * FT = llvm::FunctionType::get(llvm::Type::getInt32Ty(SymbTable.parser->MilaContext), false);
        llvm::Function * MainFunction = llvm::Function::Create(FT, llvm::Function::ExternalLinkage, "main", SymbTable.parser->MilaModule);

        // block
        llvm::BasicBlock * BB = llvm::BasicBlock::Create(SymbTable.parser->MilaContext, "entry", MainFunction);
        SymbTable.parser->MilaBuilder.SetInsertPoint(BB);

        for (int i =0;i<decs.size();++i){
            //decs[i]->PreDeclare(SymbTable);
            decs[i]->GlobDeclare(SymbTable);
        }
        for (int i =block.size()-1;i>=0;--i){
            block[i]->Generate(SymbTable);
            //SymbTable.parser->MilaBuilder.SetInsertPoint(BB);
        }

        // return 0
        SymbTable.parser->MilaBuilder.CreateRet(llvm::ConstantInt::get(llvm::Type::getInt32Ty(SymbTable.parser->MilaContext), 0));
    }
    return 1;
}



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
    Parser::SymbTable st(this);

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


// parsin functions


Parser::NProgram *Parser::Start() {
    NProgramName *pname;
    std::vector<NStatement*> b;
    std::vector<NDeclaration*> d;
    switch (CurTok) {
        case tok_program:
            if(DEBUG)
                printf("(1) S -> P D B .\n");
            pname=Program();
            d=Declarations();
            b=Block();
            Compare('.');
            // Compare(tok_eof);
            return new NProgram(pname,d,b);
        default:
            ExpansionError("S", CurTok);
            return 0;
    }
}

Parser::NProgramName *Parser::Program() {
    NProgramName *pname;
    switch (CurTok) {
        case tok_program:
            if(DEBUG)
                printf("(2) P -> program ident ;\n");
            Compare(tok_program);
            pname=new NProgramName(m_Lexer.identifierStr());
            Compare(tok_identifier);
            Compare(';');
            return pname;
        default:
            ExpansionError("P", CurTok);
            return 0;
    }
}

std::vector<Parser::NStatement *> Parser::Block() {
    std::vector<NStatement *> sl;
    switch (CurTok) {
        case tok_begin:
            if(DEBUG)
                printf("(3) B -> begin SL end\n");
            Compare(tok_begin);
            sl=StatementList({});
            Compare(tok_end);
            return sl;
        default:
            ExpansionError("B", CurTok);
            return {};
    }
}

std::vector<Parser::NExpression *> Parser::Expressionlist(std::vector<NExpression *> l){
    NExpression*expr;
    switch (CurTok) {
        case ')':
            if(DEBUG)
                printf("El->e\n");
            return l;
        case ',':
            if(DEBUG)
                printf("El->, E El\n");
            Compare(',');
            l.push_back(Expression());
            return Expressionlist(l);
        default:
            ExpansionError("El", CurTok);
            return {};
    }
}

std::vector<Parser::NExpression *> Parser::Expressionlistopt(){
    std::vector<NExpression *> list={};
    switch (CurTok) {
        case ')':
            if(DEBUG)
                printf("Elo->e\n");
            return {};
        case tok_identifier:
        case tok_number:
        case '(':
            if(DEBUG)
                printf("Elo->E El\n");
            list.push_back(Expression());
            return Expressionlist(list);
        default:
            ExpansionError("Elo", CurTok);
            return {};
    }
}
std::vector<Parser::NStatement *> Parser::StatementPrime(std::string ident){
    NExpression *expr;
    std::vector<NExpression *> list;
    switch (CurTok) {
        case tok_assign:
            if(DEBUG)
                printf("() S' -> := num\n");
            Compare(tok_assign);
            expr=Expression();
            //Compare(';');
            return {new NAssignment(ident, expr)};
        case '(':
            if(DEBUG)
                printf("() S' -> ( Elo )\n");
            Compare('(');
            list=Expressionlistopt();
            Compare(')');
            //Compare(';');
            return {new NCallStatement(ident, list)};
        default:
            ExpansionError("S'", CurTok);
            return {};
    }
}
std::vector<Parser::NStatement *> Parser::Statement(){
    std::string ident;
    std::vector<NStatement *>res;
    NExpression *a;
    NExpression *a2;
    std::vector<NStatement *>b;
    std::string varname;
    char dir;
    std::vector<NStatement *> c={};
    switch (CurTok) {
        case tok_begin:
            res=Block();
            //Compare(';');
            return res;
        case tok_identifier:
            if(DEBUG)
                printf("() S -> ident S'\n");
            ident=m_Lexer.identifierStr();
            Compare(tok_identifier);
            return StatementPrime(ident);
        case tok_exit:
            if(DEBUG)
                printf("() S -> exit'\n");
            // Compare(tok_exit);
            CurTok=';';
            return {new NExit()};
        case tok_break:
            if(DEBUG)
                printf("() S -> break'\n");
            // Compare(tok_exit);
            CurTok=';';
            return {new NBreak()};
        case tok_if:
            Compare(tok_if);
            a=Expression();
            Compare(tok_then);
            b=Statement();
            if(CurTok==tok_else){
                Compare(tok_else);
                c=Statement();
            }
            return {new NCondition(a,b,c)};
        case tok_while:
            Compare(tok_while);
            a=Expression();
            Compare(tok_do);
            b=Statement();
            return {new NWhile(a,b)};
        case tok_for:

            if(DEBUG)
                printf("() S -> for ident := E to E do S'\n");
            Compare(tok_for);
            varname=m_Lexer.identifierStr();
            Compare(tok_identifier);
            Compare(tok_assign);
            a=Expression();
            switch (CurTok){
                case tok_to:
                    dir='+';
                    Compare(tok_to);
                    break;
                case tok_downto:
                    dir='-';
                    Compare(tok_downto);
                    break;
                default:
                    ExpansionError("to-downto", CurTok);
                    return {};
            }
            a2=Expression();
            Compare(tok_do);
            b=Statement();
            return {new NFor(dir,varname,a,a2,b)};

        default:
            ExpansionError("S", CurTok);
            return {};
    }
}




std::vector<Parser::NStatement *> Parser::StatementList(std::vector<NStatement *> l) {
    std::vector<NStatement *>n;
    switch (CurTok) {
        case tok_end:
            if(DEBUG)
                printf("(4) L -> e\n");
            return l;
        case tok_identifier:
        case tok_exit:
        case tok_begin:
        case tok_if:
        case tok_while:
        case tok_for:
        case tok_break:
            if(DEBUG)
                printf("(4.1) L -> S; L\n");
            n=Statement();
            Compare(';');
            l.insert(l.begin(),n.begin(),n.end());
            return StatementList(l);
        case ';':
            Compare(';');
            return StatementList(l);
        default:
            ExpansionError("Sl", CurTok);
            return {};
    }
}
std::vector<Parser::NDeclaration*> Parser::Declarations() {
    std::vector<NDeclaration*> nlist={};
    std::vector<NDeclaration*> list={};
    std::vector<NVarDeclaration*> vlist={};
    switch (CurTok) {
        case tok_begin:
            if(DEBUG)
                printf("(5.1) D -> e\n");
            return {};
        case tok_const:
            if(DEBUG)
                printf("(5.2) D -> CD\n");
            list=Const();
            nlist=Declarations();
            list.insert(list.end(),nlist.begin(),nlist.end());
            return list;
        case tok_var:
            if(DEBUG)
                printf("(5.3) D -> VD\n");
            vlist=Var();
            for(int i=0;i<vlist.size();++i) {
                list.push_back(vlist[i]);
            }
            nlist= Declarations();
            list.insert(list.end(),nlist.begin(),nlist.end());
            return list;

        case tok_procedure:
            if(DEBUG)
                printf("(5.4) D -> PD\n");
            list=Procedure();
            nlist= Declarations();
            list.insert(list.end(),nlist.begin(),nlist.end());
            return list;
        case tok_function:
            if(DEBUG)
                printf("(5.5) D -> FD\n");
            list=Function();
            nlist= Declarations();
            list.insert(list.end(),nlist.begin(),nlist.end());
            return list;
        default:
            ExpansionError("D", CurTok);
            return {};
    }
}


std::vector<Parser::NDeclaration*>  Parser::Const() {
    std::vector<NDeclaration*> res={};
    std::vector<NDeclaration*> list={};
    switch (CurTok) {
        case tok_const:
            if(DEBUG)
                printf("(6) C -> const Celem Clist\n");
            Compare(tok_const);
            res.push_back(ConstElem());
            list= ConstList();
            res.insert(res.end(),list.begin(),list.end());
            return res;
        default:
            ExpansionError("C", CurTok);
            return {};
    }
}

Parser::NConstDeclaration *Parser::ConstElem() {
    std::string left;
    int n;
    switch (CurTok) {
        case tok_identifier:
            if(DEBUG)
                printf("(6.1) Ce -> ident = E;\n");
            left=m_Lexer.identifierStr();
            Compare(tok_identifier);
            Compare('=');
            n=m_Lexer.numVal();
            Compare(tok_number);
            //Expression();
            Compare(';');
            return new NConstDeclaration(left,n);
        default:
            ExpansionError("C", CurTok);
            return 0;
    }
}
std::vector<Parser::NDeclaration*> Parser::ConstList() {
    std::vector<NDeclaration*> res={};
    std::vector<NDeclaration*> list={};
    switch (CurTok) {
        case tok_begin:
        case tok_const:
        case tok_var:
        case tok_function:
        case tok_procedure:
            if(DEBUG)
                printf("(6.2) Cl -> e\n");
            return {};
        case tok_identifier:
            if(DEBUG)
                printf("(6.3) Cl -> Ce Cl\n");
            res.push_back(ConstElem());
            list= ConstList();
            res.insert(res.end(),list.begin(),list.end());
            return res;
        default:
            ExpansionError("C", CurTok);
            return {};
    }
}

std::string Parser::Type() {
    switch (CurTok) {
        case tok_integer:
            if(DEBUG)
                printf("T -> integer\n");
            Compare(tok_integer);
            return "int32";
        default:
            ExpansionError("T", CurTok);
            return "";
    }
}
std::vector<Parser::NVarDeclaration*> Parser::Var() {
    std::vector<NVarDeclaration*> res;
    std::vector<NVarDeclaration*> list;
    switch (CurTok) {
        case tok_var:
            if(DEBUG)
                printf("(7) V -> var Velem Vlist\n");
            Compare(tok_var);
            res=VarElem();
            list=VarList();
            res.insert(res.end(),list.begin(),list.end());
            return res;
        default:
            ExpansionError("V", CurTok);
            return {};
    }
}

std::vector<Parser::NVarDeclaration*> Parser::VarElem() {
    std::vector<NVarDeclaration*> res;
    std::vector<std::string> ilist={};
    std::vector<std::string> fident={};
    std::string type;
    switch (CurTok) {
        case tok_identifier:
            if(DEBUG)
                printf("(7.1) Ce -> ident VIl : T\n");
            fident.push_back(m_Lexer.identifierStr());
            Compare(tok_identifier);
            ilist= VarIdentList();
            Compare(':');
            type=Type();
            if(CurTok==';')
                Compare(';');
            fident.insert(fident.end(),ilist.begin(),ilist.end());
            for(int i =0;i<fident.size();++i){
                res.push_back(new NVarDeclaration(fident[i],type));
            }
            return res;
        default:
            ExpansionError("C", CurTok);
            return {};
    }
}
std::vector<Parser::NVarDeclaration*> Parser::VarElemOpt() {
    switch (CurTok) {
        case ')':
            return {};
        case tok_identifier:
            return VarList();
        default:
            ExpansionError("C", CurTok);
            return {};
    }
}
std::vector<std::string> Parser::VarIdentList() {
    std::vector<std::string> res={};
    std::vector<std::string> list;
    switch (CurTok) {
        case ':':
            if(DEBUG)
                printf("(7.3) VIl -> e\n");
            return {};
        case ',':
            if(DEBUG)
                printf("(7.2) VIl -> , ident VIl\n");
            Compare(',');
            res.push_back(m_Lexer.identifierStr());
            Compare(tok_identifier);
            list= VarIdentList();
            res.insert(res.end(),list.begin(),list.end());
            return res;
        default:
            ExpansionError("VIl", CurTok);
            return {};
    }
}
std::vector<Parser::NVarDeclaration*> Parser::VarList() {
    std::vector<NVarDeclaration*> res={};
    std::vector<NVarDeclaration*> list;
    switch (CurTok) {
        case tok_begin:
        case tok_const:
        case tok_var:
        case tok_function:
        case tok_procedure:
        case ')':
            if(DEBUG)
                printf("(7.4) Vl -> e\n");
            return {};
        case tok_identifier:
            if(DEBUG)
                printf("(7.5) CV -> Ve Vl\n");
            res=VarElem();
            list=VarList();
            res.insert(res.end(),list.begin(),list.end());
            //Compare(';');
            return res;
        default:
            ExpansionError("VL", CurTok);
            return {};
    }
}

std::vector<Parser::NDeclaration*> Parser::Function() {
    std::string name;
    std::vector<NVarDeclaration*> args;
    std::string type;
    std::vector<NDeclaration*> decs;
    std::vector<NStatement*> block;
    switch (CurTok) {
        case tok_function:
            if(DEBUG)
                printf("(8) F -> function ident (Vlist) : type ; D B;\n");
            Compare(tok_function);
            name=m_Lexer.identifierStr();
            Compare(tok_identifier);
            Compare('(');
            args=VarElemOpt();
            Compare(')');
            Compare(':');
            type=Type();
            Compare(';');
            decs=Declarations();
            block=Block();
            Compare(';');
            return {new NFunctionDeclaration(new NFunctionPrototype(name, args, type), decs, block)};
        default:
            ExpansionError("F", CurTok);
            return {};
    }
}

std::vector<Parser::NDeclaration*> Parser::Procedure() {
    std::string name;
    std::vector<NVarDeclaration*> args;
    std::string type;
    std::vector<NDeclaration*> decs;
    std::vector<NStatement*> block;
    switch (CurTok) {
        case tok_procedure:
            if(DEBUG)
                printf("(9) P -> procedure ident (Vlist); D B;\n");
            Compare(tok_procedure);
            name=m_Lexer.identifierStr();
            Compare(tok_identifier);
            Compare('(');
            args=VarElemOpt();
            Compare(')');
            Compare(';');
            decs=Declarations();
            block=Block();
            Compare(';');
            return {new NProcedureDeclaration(new NProcedurePrototype(name, args), decs, block)};
        default:
            ExpansionError("P", CurTok);
            return {};
    }
}
Parser::NExpression *Parser::Expression(){
    int n;
    switch (CurTok) {
        case tok_identifier:
        case tok_number:
        case '(':
            if(DEBUG)
                printf(" E -> T E'\n");
            return ExpressionPrime(LogExpression());
        default:
            ExpansionError("E", CurTok);
            return 0;
    }
}

Parser::NExpression *Parser::ExpressionPrime(NExpression * v){
    int n;
    switch (CurTok) {
        case tok_and:
            if(DEBUG)
                printf("(2a) E' -> and T E'\n");
            Compare(tok_and);
            return ExpressionPrime(new NBinaryExpression('&',v,LogExpression()));
        case tok_or:
            if(DEBUG)
                printf("(2b) E' -> or T E'\n");
            Compare(tok_or);
            return ExpressionPrime(new NBinaryExpression('|',v,LogExpression()));
        case ')':
        case tok_then:
        case tok_else:
        case tok_do:
        case tok_to:
        case tok_downto:
        case ',':
        case ';':
            if(DEBUG)
                printf("(3) E' -> e\n");
            return v;
        default:
            ExpansionError("E'", CurTok);
            return 0;
    }
}
Parser::NExpression *Parser::LogExpression(){
    int n;
    switch (CurTok) {
        case tok_identifier:
        case tok_number:
        case '(':
            if(DEBUG)
                printf(" lE -> T lE'\n");
            return LogExpressionPrime(AlgExpression());
        default:
            ExpansionError("lE", CurTok);
            return 0;
    }
}

Parser::NExpression *Parser::LogExpressionPrime(NExpression * v){
    int n;
    switch (CurTok) {
        case tok_mod:
            if(DEBUG)
                printf("(2a) lE' -> mod aE lE'\n");
            Compare(tok_mod);
            return LogExpressionPrime(new NBinaryExpression('%',v,AlgExpression()));
        case '=':
            if(DEBUG)
                printf("(2b) lE' -> = aE lE'\n");
            Compare('=');
            return LogExpressionPrime(new NBinaryExpression('=',v,AlgExpression()));
        case tok_notequal:
            if(DEBUG)
                printf("(2b) lE' -> != aE lE'\n");
            Compare(tok_notequal);
            return LogExpressionPrime(new NBinaryExpression('!',v,AlgExpression()));
        case '<':
            if(DEBUG)
                printf("(2b) lE' -> < aE lE'\n");
            Compare('<');
            return LogExpressionPrime(new NBinaryExpression('<',v,AlgExpression()));
        case '>':
            if(DEBUG)
                printf("(2b) lE' -> > aE lE'\n");
            Compare('>');
            return LogExpressionPrime(new NBinaryExpression('>',v,AlgExpression()));
        case tok_lessequal:
            if(DEBUG)
                printf("(2b) lE' -> <= aE lE'\n");
            Compare(tok_lessequal);
            return LogExpressionPrime(new NBinaryExpression('(',v,AlgExpression()));
        case tok_greaterequal:
            if(DEBUG)
                printf("(2b) lE' -> >= aE lE'\n");
            Compare(tok_greaterequal);
            return LogExpressionPrime(new NBinaryExpression(')',v,AlgExpression()));
        case ')':
        case tok_then:
        case tok_else:
        case tok_do:
        case tok_and:
        case tok_or:
        case tok_to:
        case tok_downto:
        case ';':
        case ',':
            if(DEBUG)
                printf("(3) lE' -> e\n");
            return v;
        default:
            ExpansionError("E'", CurTok);
            return 0;
    }
}

Parser::NExpression *Parser::AlgExpression(){
    int n;
    switch (CurTok) {
        case tok_identifier:
        case tok_number:
        case '(':
            if(DEBUG)
                printf(" aE -> T aE'\n");
            return AlgExpressionPrime(Term());
        default:
            ExpansionError("aE", CurTok);
            return 0;
    }
}

Parser::NExpression *Parser::AlgExpressionPrime(NExpression * v){
    int n;
    switch (CurTok) {
        case '+':
            if(DEBUG)
                printf("(2a) aE' -> + T aE'\n");
            Compare('+');
            return ExpressionPrime(new NBinaryExpression('+',v,Term()));
        case '-':
            if(DEBUG)
                printf("(2b) aE' -> - T aE'\n");
            Compare('-');
            return ExpressionPrime(new NBinaryExpression('-',v,Term()));
        case ')':
        case ',':
        case ';':
        case tok_then:
        case tok_else:
        case tok_do:
        case tok_lessequal:
        case tok_greaterequal:
        case '=':
        case tok_notequal:
        case '>':
        case '<':
        case tok_and:
        case tok_or:
        case tok_mod:
        case tok_to:
        case tok_downto:
            if(DEBUG)
                printf("(3) aE' -> e\n");
            return v;
        default:
            ExpansionError("aE'", CurTok);
            return 0;
    }
}

Parser::NExpression *Parser::Term(){
    int n;
    switch (CurTok) {
        case tok_identifier:
        case tok_number:
        case '(':
            if(DEBUG)
                printf(" T -> G T'\n");
            return TermPrime(G());
        default:
            ExpansionError("T", CurTok);
            return 0;
    }
}
Parser::NExpression *Parser::TermPrime(NExpression * v){
    int n;
    switch (CurTok) {
        case '*':
            if(DEBUG)
                printf("(5a) T' -> * G T'\n");
            Compare('*');
            return TermPrime(new NBinaryExpression('*',v,G()));
        case '/':
            if(DEBUG)
                printf("(5b) T' -> / G T'\n");
            Compare('/');
            return TermPrime(new NBinaryExpression('/',v,G()));
        case tok_div:
            if(DEBUG)
                printf("(5b) T' -> div G T'\n");
            Compare(tok_div);
            return TermPrime(new NBinaryExpression('d',v,G()));
        case ')':
        case ',':
        case '+':
        case '-':
        case ';':
        case tok_then:
        case tok_else:
        case tok_do:
        case tok_lessequal:
        case tok_greaterequal:
        case tok_notequal:
        case '=':
        case '>':
        case '<':
        case tok_and:
        case tok_or:
        case tok_mod:
        case tok_to:
        case tok_downto:
            if(DEBUG)
                printf("(5) T' -> e\n");
            return v;
        default:
            ExpansionError("T'", CurTok);
            return 0;
    }
}
Parser::NExpression *Parser::G(){
    int n;
    switch (CurTok) {
        case tok_identifier:
        case tok_number:
        case '(':
            if(DEBUG)
                printf("(9) G -> F G'\n");
            return GPrime(Factor());
        default:
            ExpansionError("G", CurTok);
            return 0;
    }
}
Parser::NExpression *Parser::GPrime(NExpression * v){
    int n;
    switch (CurTok) {
        case '^':
            if(DEBUG)
                printf("(10) G' -> ^ F G'\n");
            Compare('^');
            return new NBinaryExpression('^',v,GPrime(Term()));
        case '/':
        case tok_div:
        case '*':
        case ')':
        case ',':
        case '+':
        case '-':
        case ';':
        case tok_then:
        case tok_else:
        case tok_do:
        case tok_lessequal:
        case tok_greaterequal:
        case tok_notequal:
        case '=':
        case '>':
        case '<':
        case tok_and:
        case tok_or:
        case tok_mod:
        case tok_to:
        case tok_downto:
            if(DEBUG)
                printf("(5) G' -> e\n");
            return v;
        default:
            ExpansionError("G'", CurTok);
            return 0;
    }
}

Parser::NExpression *Parser::Factor(){
    std::string name;
    NExpression *res;
    switch (CurTok) {
        case tok_identifier:
            if(DEBUG)
                printf("(7a) F -> ident (%s) F'\n", m_Lexer.identifierStr().c_str());
            name=m_Lexer.identifierStr();
            Compare(tok_identifier);
            return FactorPrime(name);
        case tok_number:
            res=new NNumberExpression(m_Lexer.numVal());
            if(DEBUG)
                printf("(7b) F -> numb (%d)\n", m_Lexer.numVal());
            Compare(tok_number);
            return res;
        case '(':
            if(DEBUG)
                printf("(8) F -> ( E )\n");
            Compare('(');
            res = Expression();
            Compare(')');
            return res;
        default:
            ExpansionError("F", CurTok);
            return  0;
    }
}
Parser::NExpression *Parser::FactorPrime(std::string name){
    std::vector<NExpression *> list;
    switch (CurTok) {
        case '(':
            if(DEBUG)
                printf("F' -> ( Elo )\n");
            Compare('(');
            list=Expressionlistopt();
            Compare(')');
            return new NCallExpression(name,list);
        default:
            if(DEBUG)
                printf("F' -> e\n");
            return new NVarExpression(name);
    }

}