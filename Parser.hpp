#ifndef PJPPROJECT_PARSER_HPP
#define PJPPROJECT_PARSER_HPP

#include <llvm/ADT/APFloat.h>
#include <llvm/ADT/STLExtras.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Verifier.h>

#include "Lexer.hpp"




class Parser {
public:
    Parser();
    ~Parser(){
        if(tree)
            delete tree;

    };

    bool Parse();                    // parse
    const llvm::Module& Generate();  // generate

    llvm::LLVMContext MilaContext;   // llvm context
    llvm::IRBuilder<> MilaBuilder;   // llvm builder
    llvm::Module MilaModule;         // llvm module
private:


    int getNextToken();

    Lexer m_Lexer;                   // lexer is used to read tokens
    int CurTok;                      // to keep the current token

    void Compare(int s);



    class SymbTable{
        std::map<std::string,llvm::Value*> ConstTable;
        std::map<std::string,llvm::Value*> VarTable;
        std::map<std::string,llvm::FunctionCallee> CallTable;
    public:
        llvm::BasicBlock *contbb=0;
        llvm::Value *ret=0;
        Parser *parser;
        int AddConst(std::string name,llvm::Value* value);
        int AddVar(std::string name,llvm::Value* value);
        int AddFunc(std::string name,llvm::FunctionCallee value);
        llvm::Value *GetVal(std::string name);
        llvm::Value *GetAddr(std::string name);
        llvm::FunctionCallee GetCallee(std::string name);
        SymbTable(Parser *p);
    };

    class ASTNode{
    public:
        virtual int Generate(SymbTable &SymbTable){return 0;}
    };
    ASTNode *tree;


    class NProgramName : public ASTNode{
        std::string Name;
    public:
        NProgramName(const std::string& name):Name(name){}
        void Write(Parser *p){
            std::cout<<"progname: "<<Name<<"\n";
        }
    };
    class NStatement : public ASTNode{

    };
    class NExpression : public ASTNode{
    public:
        virtual  llvm::Value * Value(SymbTable &SymbTable){return 0;};
    };
    class NDeclaration : public ASTNode{
    public:
        virtual void PreDeclare(SymbTable &SymbTable){}
        virtual void InDeclare(SymbTable &SymbTable){}
        virtual void GlobDeclare(SymbTable &SymbTable){}
    };
    class NProgram : public ASTNode{
        NProgramName* pName;
        std::vector<NDeclaration*> decs;
        std::vector<NStatement *> block;
    public:
        NProgram(NProgramName *pname,std::vector<NDeclaration*> d,std::vector<NStatement *> b):pName(pname),decs(d),block(b){}
        ~NProgram(){
            for(auto d:decs)
                delete d;
            for(auto s:block)
                delete s;
        }
        int Generate( SymbTable &SymbTable) override;
    };
    class NConstDeclaration : public NDeclaration{
        std::string name;
        int right;
    public:
        NConstDeclaration(const std::string &n,int r):name(n),right(r){}

        void PreDeclare(SymbTable &SymbTable){
            SymbTable.AddConst(name,llvm::ConstantInt::get(SymbTable.parser->MilaContext, llvm::APInt(32, right)));
        }
        void InDeclare(SymbTable &SymbTable){}
        void GlobDeclare(SymbTable &SymbTable){}
    };
    class NVarDeclaration : public NDeclaration{
    public:
        std::string name;
        std::string type;
        NVarDeclaration(const std::string &n,const std::string &t):name(n),type(t){}
        void PreDeclare(SymbTable &SymbTable){
        }
        void InDeclare(SymbTable &SymbTable){
            SymbTable.AddVar(name,SymbTable.parser->MilaBuilder.CreateAlloca(llvm::Type::getInt32Ty(SymbTable.parser->MilaContext), 0,name.c_str()));
        }
        void GlobDeclare(SymbTable &SymbTable){
            auto ptr=new llvm::GlobalVariable(SymbTable.parser->MilaModule,llvm::Type::getInt32Ty(SymbTable.parser->MilaContext),false,llvm::GlobalValue::CommonLinkage,llvm::ConstantInt::get(SymbTable.parser->MilaContext, llvm::APInt(32, 0)),name.c_str());
            SymbTable.AddVar(name,ptr);
        }
    };
    class NFunctionPrototype : public NDeclaration{
    public:
        std::string name;
        std::vector<NVarDeclaration*> args;

        std::string type;
        NFunctionPrototype(const std::string &n,std::vector<NVarDeclaration*>& a,const std::string& t):name(n),args(a),type(t){}
        ~NFunctionPrototype(){
            for(auto a:args)
                delete a;
        }
        int Declare(SymbTable &SymbTable){
            std::vector<llvm::Type*> fargs(args.size(),
                                          llvm::Type::getInt32Ty(SymbTable.parser->MilaContext));
            llvm::FunctionType *FT =llvm::FunctionType::get(llvm::Type::getInt32Ty(SymbTable.parser->MilaContext), fargs, false);
            llvm::Function *F = llvm::Function::Create(FT, llvm::Function::ExternalLinkage, name, SymbTable.parser->MilaModule);
            unsigned Idx = 0;
            for (auto &Arg : F->args())
                Arg.setName(args[Idx++]->name);

            SymbTable.AddFunc(name,llvm::FunctionCallee(FT,F));
            return 1;
            //    SymbTable[name]=llvm::ConstantInt::get(SymbTable.parser->MilaContext, llvm::APInt(32, 0));
        }
    };
    class NAssignment : public NStatement{
        std::string left;
        NExpression *right;
    public:
        NAssignment(const std::string &l,NExpression *r):left(l),right(r){}
        ~NAssignment(){
            if(right)
                delete right;
        }
        int Generate(SymbTable &SymbTable){
            auto ptr=SymbTable.GetAddr(left);
            SymbTable.parser->MilaBuilder.CreateStore(right->Value(SymbTable),ptr);

            return 1;
        }
    };
    class NExit:public NStatement{
    public:
        int Generate(SymbTable &SymbTable){
            if(!SymbTable.ret){
                SymbTable.parser->MilaBuilder.CreateRet(llvm::ConstantInt::get(llvm::Type::getInt32Ty(SymbTable.parser->MilaContext), 0));
            }else {
                // auto strref =SymbTable.parser->MilaBuilder.GetInsertBlock()->getParent()->getName();
                auto a = SymbTable.parser->MilaBuilder.CreateLoad(llvm::Type::getInt32Ty(SymbTable.parser->MilaContext),SymbTable.ret, "tmpret");
                SymbTable.parser->MilaBuilder.CreateRet(a);
            }
            llvm::Function *TheFunction = SymbTable.parser->MilaBuilder.GetInsertBlock()->getParent();
            llvm::BasicBlock *afterret =llvm::BasicBlock::Create(SymbTable.parser->MilaContext, "afterret");
            TheFunction->getBasicBlockList().push_back(afterret);
            SymbTable.parser->MilaBuilder.SetInsertPoint(afterret);
            return 0;
        }
    };
    class NBreak:public NStatement{
    public:
        int Generate(SymbTable &SymbTable){
            if(SymbTable.contbb)
                SymbTable.parser->MilaBuilder.CreateBr(SymbTable.contbb);
            else
                printf("Error in use of break\n");
            return 1;
        }
    };
    class NFunctionDeclaration : public NDeclaration{
        NFunctionPrototype* prototype;
        std::vector<NDeclaration*> decs;
        std::vector<NStatement*> block;
    public:
        NFunctionDeclaration(NFunctionPrototype* p,std::vector<NDeclaration*> d, std::vector<NStatement*> b): prototype(p),decs(d),block(b){}
        ~NFunctionDeclaration(){
            if(prototype)
                delete prototype;
            for(auto d:decs)
                delete d;
            for(auto s:block)
                delete s;
        }
        void PreDeclare(SymbTable &SymbTable){
            auto curblock =SymbTable.parser->MilaBuilder.GetInsertBlock();
            auto C=SymbTable.GetCallee(prototype->name);
            if(!C.getCallee()) {
                prototype->Declare(SymbTable);
                C=SymbTable.GetCallee(prototype->name);
            }
            llvm::Function *F=(llvm::Function *)C.getCallee();
            auto st=SymbTable;
            for (int i =0;i<decs.size();++i){
                decs[i]->PreDeclare(st);
            }
            llvm::BasicBlock *BB = llvm::BasicBlock::Create(SymbTable.parser->MilaContext, "entry", F);
            SymbTable.parser->MilaBuilder.SetInsertPoint(BB);
            st.AddVar(prototype->name,st.parser->MilaBuilder.CreateAlloca(llvm::Type::getInt32Ty(SymbTable.parser->MilaContext), 0,prototype->name.c_str()));

            st.ret=st.GetAddr(prototype->name);

            for (auto arg:prototype->args){
                arg->InDeclare(st);
            }
            for (auto &Arg : F->args())
                st.parser->MilaBuilder.CreateStore(&Arg,st.GetAddr(std::string(Arg.getName().begin(),Arg.getName().end())));

            for (int i =0;i<decs.size();++i){
                decs[i]->InDeclare(st);
                //SymbTable.parser->MilaBuilder.SetInsertPoint(BB);
            }
            for (int i =block.size()-1;i>=0;--i){
                block[i]->Generate(st);
                //SymbTable.parser->MilaBuilder.SetInsertPoint(BB);
            }
            SymbTable.parser->MilaBuilder.CreateRet(st.GetVal(prototype->name));
            SymbTable.parser->MilaBuilder.SetInsertPoint(curblock);
            // return 0
        }
        void InDeclare(SymbTable &SymbTable){
        }
        void GlobDeclare(SymbTable &SymbTable){
        }
    };
    class NProcedurePrototype : public NDeclaration{
    public:
        std::string name;
        std::vector<NVarDeclaration*> args;
        NProcedurePrototype(const std::string &n,std::vector<NVarDeclaration*>& a):name(n),args(a){}
        ~NProcedurePrototype(){
            for(auto a:args)
                delete a;
        }
        void Declare(SymbTable &SymbTable){
            //SymbTable[name]=llvm::ConstantInt::get(SymbTable.parser->MilaContext, llvm::APInt(32, 0));
        }
    };
    class NProcedureDeclaration : public NDeclaration{
        NProcedurePrototype* prototype;
        std::vector<NStatement*> block;
        std::vector<NDeclaration*> decs;

    public:
        NProcedureDeclaration(NProcedurePrototype* p,std::vector<NDeclaration*> d,  std::vector<NStatement*> b): prototype(p),decs(d),block(b){}
        ~NProcedureDeclaration(){
            if(prototype)
                delete prototype;
            for(auto d:decs)
                delete d;
            for(auto s:block)
                delete s;
        }
        void PreDeclare(SymbTable &SymbTable){

            auto curblock =SymbTable.parser->MilaBuilder.GetInsertBlock();
            std::vector<llvm::Type*> args(prototype->args.size(),
                                          llvm::Type::getInt32Ty(SymbTable.parser->MilaContext));
            llvm::FunctionType *FT =llvm::FunctionType::get(llvm::Type::getInt32Ty(SymbTable.parser->MilaContext), args, false);
            llvm::Function *F = llvm::Function::Create(FT, llvm::Function::ExternalLinkage, prototype->name, SymbTable.parser->MilaModule);
            unsigned Idx = 0;
            for (auto &Arg : F->args())
                Arg.setName(prototype->args[Idx++]->name);

            SymbTable.AddFunc(prototype->name,llvm::FunctionCallee(FT,F));
            auto st=SymbTable;
            for (int i =0;i<decs.size();++i){
                decs[i]->PreDeclare(st);
            }
            llvm::BasicBlock *BB = llvm::BasicBlock::Create(SymbTable.parser->MilaContext, "entry", F);
            SymbTable.parser->MilaBuilder.SetInsertPoint(BB);
            //st.AddVar(prototype->name,st.parser->MilaBuilder.CreateAlloca(llvm::Type::getInt32Ty(SymbTable.parser->MilaContext), 0,prototype->name.c_str()));
            st.ret=0;

            for (auto arg:prototype->args){
                arg->InDeclare(st);
            }
            for (auto &Arg : F->args())
                st.parser->MilaBuilder.CreateStore(&Arg,st.GetAddr(std::string(Arg.getName().begin(),Arg.getName().end())));

            for (int i =0;i<decs.size();++i){
                decs[i]->InDeclare(st);
                //SymbTable.parser->MilaBuilder.SetInsertPoint(BB);
            }
            for (int i =block.size()-1;i>=0;--i){
                block[i]->Generate(st);
                //SymbTable.parser->MilaBuilder.SetInsertPoint(BB);
            }
            // return 0
            SymbTable.parser->MilaBuilder.CreateRet(llvm::ConstantInt::get(llvm::Type::getInt32Ty(SymbTable.parser->MilaContext), 0));

            SymbTable.parser->MilaBuilder.SetInsertPoint(curblock);
        }
        void InDeclare(SymbTable &SymbTable){
        }
        void GlobDeclare(SymbTable &SymbTable){
        }
    };
    class NNumberExpression : public NExpression{
        int Val;
    public:
        NNumberExpression(int val):Val(val){}
        llvm::Value *Value( SymbTable &SymbTable){
            return llvm::ConstantInt::get(SymbTable.parser->MilaContext, llvm::APInt(32, Val));
        }
    };
    class NVarExpression : public NExpression{
    public:
        std::string Val;
        NVarExpression(std::string val):Val(val){}
        llvm::Value *Value(SymbTable &SymbTable){
            llvm::Value *v=SymbTable.GetVal(Val);
            if(!v){
                printf("Unknows variable name\n");
                return 0;
            }
            return v;
        }
    };
    class NUnaryExpression : public NExpression{
        std::string operation;
        NExpression* expr;
    public:
        NUnaryExpression(std::string o, NExpression* e):operation(o),expr(e){}
        ~NUnaryExpression(){
            if(expr)
                delete expr;
        }
    };
    class NBinaryExpression : public NExpression{
        char operation;
        NExpression* left;
        NExpression* right;
    public:
        NBinaryExpression(char o, NExpression* e1, NExpression* e2):operation(o),left(e1),right(e2){}
        ~NBinaryExpression(){
            if(left)
                delete left;
            if(right)
                delete right;
        }
        llvm::Value *Value( SymbTable &SymbTable){
            llvm::Value *L = left->Value(SymbTable);
            llvm::Value *R = right->Value(SymbTable);
            llvm::Value *res;
            if (!L || !R)
                return nullptr;

            switch(operation){
                case '+':
                    return SymbTable.parser->MilaBuilder.CreateAdd(L, R, "addtmp");
                case '-':
                    return SymbTable.parser->MilaBuilder.CreateSub(L, R, "subtmp");
                case '*':
                    return SymbTable.parser->MilaBuilder.CreateMul(L, R, "multmp");
                case '%':
                    return SymbTable.parser->MilaBuilder.CreateSRem(L, R, "multmp");
                case 'd':
                    return SymbTable.parser->MilaBuilder.CreateSDiv(L, R, "divtmp");
                case '=':
                    return SymbTable.parser->MilaBuilder.CreateIntCast(SymbTable.parser->MilaBuilder.CreateICmpEQ(L, R,  "tmpeq"),llvm::Type::getInt32Ty(SymbTable.parser->MilaContext),false,"tmpcast");
                case '!':
                    return SymbTable.parser->MilaBuilder.CreateIntCast(SymbTable.parser->MilaBuilder.CreateICmpNE(L, R,  "tmpneq"),llvm::Type::getInt32Ty(SymbTable.parser->MilaContext),false,"tmpcast");
                case '<':
                    return SymbTable.parser->MilaBuilder.CreateIntCast(SymbTable.parser->MilaBuilder.CreateICmpULT(L, R,  "tmplt"),llvm::Type::getInt32Ty(SymbTable.parser->MilaContext),false,"tmpcast");
                case '>':
                    return SymbTable.parser->MilaBuilder.CreateIntCast(SymbTable.parser->MilaBuilder.CreateICmpUGT(L, R,  "tmpgt"),llvm::Type::getInt32Ty(SymbTable.parser->MilaContext),false,"tmpcast");
                case '(':
                    return SymbTable.parser->MilaBuilder.CreateIntCast(SymbTable.parser->MilaBuilder.CreateICmpULE(L, R,  "tmple"),llvm::Type::getInt32Ty(SymbTable.parser->MilaContext),false,"tmpcast");
                case ')':
                    return SymbTable.parser->MilaBuilder.CreateIntCast(SymbTable.parser->MilaBuilder.CreateICmpUGE(L, R,  "tmpge"),llvm::Type::getInt32Ty(SymbTable.parser->MilaContext),false,"tmpcast");
                case '&':
                    return SymbTable.parser->MilaBuilder.CreateIntCast(SymbTable.parser->MilaBuilder.CreateAnd(L, R,  "tmpge"),llvm::Type::getInt32Ty(SymbTable.parser->MilaContext),false,"tmpcast");
                case '|':
                    return SymbTable.parser->MilaBuilder.CreateIntCast(SymbTable.parser->MilaBuilder.CreateOr(L, R,  "tmpge"),llvm::Type::getInt32Ty(SymbTable.parser->MilaContext),false,"tmpcast");
                default:
                    printf("Unknows variable name\n");
                    return 0;
            }
        }
    };
    class NCallExpression : public NExpression{
        std::string callee;
        std::vector<NExpression *> args;
    public:
        NCallExpression(const std::string &c, std::vector<NExpression *> &a):callee(c),args(a){}
        ~NCallExpression(){
            for(auto a:args)
                delete a;
        }
        llvm::Value *Value( SymbTable &SymbTable){
            // call writeln with value from lexel
            std::vector<llvm::Value *> argsv;

            for(int i =0;i<args.size();++i){
                argsv.push_back(args[i]->Value(SymbTable));
                if (!argsv.back())
                    return 0;
            }
            return SymbTable.parser->MilaBuilder.CreateCall(SymbTable.parser->MilaModule.getFunction(callee), argsv,"calltmp");

        }
    };

    class NCallStatement : public NStatement{
        std::string callee;
        std::vector<NExpression *> args;
    public:
        NCallStatement(const std::string &c, std::vector<NExpression *> &a):callee(c),args(a){}
        ~NCallStatement(){
            for(auto a:args)
                delete a;
        }
        int Generate(SymbTable &SymbTable){
            // call writeln with value from lexel
            std::vector<llvm::Value *> argsv={};
            if(callee=="readln"){
                SymbTable.parser->MilaBuilder.CreateCall(SymbTable.parser->MilaModule.getFunction(callee), {SymbTable.GetAddr(((NVarExpression *)args[0])->Val)});
                return 1;
            }
            for(int i =0;i<args.size();++i){
                argsv.push_back(args[i]->Value(SymbTable));
                if (!argsv.back()) {
                    printf("error with statement call args\n");
                    return 0;
                }
            }
            SymbTable.parser->MilaBuilder.CreateCall(SymbTable.parser->MilaModule.getFunction(callee), argsv);
            return 1;

        }


    };

    class NCondition : public NStatement{
        NExpression* expr;
        std::vector<NStatement *> th;
        std::vector<NStatement *> el;
    public:
        NCondition(NExpression* ex,std::vector<NStatement *> t,std::vector<NStatement *> e):expr(ex),th(t),el(e){}
        ~NCondition(){
            if(expr)
                delete expr;
            for(auto s:th)
                delete s;
            for(auto s:el)
                delete s;
        }
        int Generate(SymbTable &SymbTable){
            llvm::Value *condv=expr->Value(SymbTable);
            if(!condv)
                return 0;
            condv = SymbTable.parser->MilaBuilder.CreateICmpNE(condv, llvm::ConstantInt::get(SymbTable.parser->MilaContext, llvm::APInt(32,0)), "ifcond");
            llvm::Function *TheFunction = SymbTable.parser->MilaBuilder.GetInsertBlock()->getParent();

// Create blocks for the then and else cases.  Insert the 'then' block at the
// end of the function.
            llvm::BasicBlock *ThenBB =
                    llvm::BasicBlock::Create(SymbTable.parser->MilaContext, "then", TheFunction);
            llvm::BasicBlock *ElseBB = llvm::BasicBlock::Create(SymbTable.parser->MilaContext, "else");
            llvm::BasicBlock *MergeBB = llvm::BasicBlock::Create(SymbTable.parser->MilaContext, "ifcont");
            //SymbTable.contbb=MergeBB;
            SymbTable.parser->MilaBuilder.CreateCondBr(condv, ThenBB, ElseBB);
            SymbTable.parser->MilaBuilder.SetInsertPoint(ThenBB);
            for(int i =th.size()-1;i>=0;--i){
                th[i]->Generate(SymbTable);
            }
            SymbTable.parser->MilaBuilder.CreateBr(MergeBB);
            ThenBB = SymbTable.parser->MilaBuilder.GetInsertBlock();
            TheFunction->getBasicBlockList().push_back(ElseBB);
            SymbTable.parser->MilaBuilder.SetInsertPoint(ElseBB);
            for(int i =el.size()-1;i>=0;--i){
                el[i]->Generate(SymbTable);
            }
            SymbTable.parser->MilaBuilder.CreateBr(MergeBB);
            ElseBB = SymbTable.parser->MilaBuilder.GetInsertBlock();
            TheFunction->getBasicBlockList().push_back(MergeBB);
            SymbTable.parser->MilaBuilder.SetInsertPoint(MergeBB);
            //llvm::PHINode *PN = SymbTable.parser->MilaBuilder.CreatePHI(llvm::Type::getInt32Ty(SymbTable.parser->MilaContext), 2, "iftmp");

            //PN->addIncoming(ThenV, ThenBB);
            //PN->addIncoming(ElseV, ElseBB);
            return 1;
        }
    };
    class NWhile : public NStatement{
        NExpression* expr;
        std::vector<NStatement *> body;
    public:
        NWhile(NExpression* ex,std::vector<NStatement *> b):expr(ex),body(b){}
        ~NWhile(){
            if(expr)
                delete expr;
            for(auto s:body)
                delete s;
        }
        int Generate(SymbTable &SymbTable){
            llvm::Function *TheFunction = SymbTable.parser->MilaBuilder.GetInsertBlock()->getParent();

            llvm::BasicBlock *CondBB = llvm::BasicBlock::Create(SymbTable.parser->MilaContext, "condloop",TheFunction);
            llvm::BasicBlock *LoopBB =
                    llvm::BasicBlock::Create(SymbTable.parser->MilaContext, "bodyloop");
            llvm::BasicBlock *AfterLoopBB = llvm::BasicBlock::Create(SymbTable.parser->MilaContext, "afterloop");
            auto st=SymbTable;
            st.contbb=AfterLoopBB;
            SymbTable.parser->MilaBuilder.CreateBr(CondBB);
            SymbTable.parser->MilaBuilder.SetInsertPoint(CondBB);

            llvm::Value *condv=expr->Value(st);
            if(!condv)
                return 0;
            llvm::Value * bcondv = SymbTable.parser->MilaBuilder.CreateICmpNE(condv, llvm::ConstantInt::get(SymbTable.parser->MilaContext, llvm::APInt(32,0)), "ifcond");
            SymbTable.parser->MilaBuilder.CreateCondBr(bcondv, LoopBB, AfterLoopBB);
            CondBB = SymbTable.parser->MilaBuilder.GetInsertBlock();

            TheFunction->getBasicBlockList().push_back(LoopBB);

            SymbTable.parser->MilaBuilder.SetInsertPoint(LoopBB);
            for(int i =body.size()-1;i>=0;--i){
                body[i]->Generate(st);
            }
            //  bcondv = SymbTable.parser->MilaBuilder.CreateICmpNE(condv, llvm::ConstantInt::get(SymbTable.parser->MilaContext, llvm::APInt(32,0)), "ifcond");
            //    SymbTable.parser->MilaBuilder.CreateCondBr(bcondv, LoopBB, AfterLoopBB);
            SymbTable.parser->MilaBuilder.CreateBr(CondBB);
            LoopBB = SymbTable.parser->MilaBuilder.GetInsertBlock();
            TheFunction->getBasicBlockList().push_back(AfterLoopBB);
            SymbTable.parser->MilaBuilder.SetInsertPoint(AfterLoopBB);
            //llvm::PHINode *PN = SymbTable.parser->MilaBuilder.CreatePHI(llvm::Type::getInt32Ty(SymbTable.parser->MilaContext), 2, "iftmp");

            //PN->addIncoming(ThenV, ThenBB);
            //PN->addIncoming(ElseV, ElseBB);
            return 1;
        }
    };
    class NFor : public NStatement{
        NExpression* sexpr;
        NExpression* eexpr;
        char direction;
        std::vector<NStatement *> body;
        std::string varname;
    public:
        NFor(char d,const std::string &varn,NExpression* ex1,NExpression* ex2,std::vector<NStatement *> b):varname(varn),direction(d),sexpr(ex1),eexpr(ex2),body(b){}
        ~NFor(){
            if(sexpr)
                delete sexpr;
            if(eexpr)
                delete eexpr;
            for(auto s:body)
                delete s;
        }
        int Generate(SymbTable &SymbTable){
            llvm::Function *TheFunction = SymbTable.parser->MilaBuilder.GetInsertBlock()->getParent();

            llvm::BasicBlock *CondBB = llvm::BasicBlock::Create(SymbTable.parser->MilaContext, "condloop",TheFunction);
            llvm::BasicBlock *LoopBB =
                    llvm::BasicBlock::Create(SymbTable.parser->MilaContext, "bodyloop");
            llvm::BasicBlock *AfterLoopBB = llvm::BasicBlock::Create(SymbTable.parser->MilaContext, "afterloop");
            auto st=SymbTable;
            st.contbb=AfterLoopBB;

            auto addr=SymbTable.parser->MilaBuilder.CreateAlloca(llvm::Type::getInt32Ty(SymbTable.parser->MilaContext), 0,"fortmp");
            SymbTable.parser->MilaBuilder.CreateStore(sexpr->Value(SymbTable),addr);
            st.AddVar(varname,addr);

            SymbTable.parser->MilaBuilder.CreateBr(CondBB);
            SymbTable.parser->MilaBuilder.SetInsertPoint(CondBB);

            llvm::Value *start=SymbTable.parser->MilaBuilder.CreateLoad(llvm::Type::getInt32Ty(SymbTable.parser->MilaContext),addr, "fortmp2");
            llvm::Value *end=eexpr->Value(st);


            if(!start || !end)
                return 0;
            llvm::Value * bcondv =0;

            if(direction=='+')
                bcondv=SymbTable.parser->MilaBuilder.CreateICmpULE(start, end, "forcond");
            else if(direction=='-')
                bcondv=SymbTable.parser->MilaBuilder.CreateICmpUGE(start, end, "forcond");
            SymbTable.parser->MilaBuilder.CreateCondBr(bcondv, LoopBB, AfterLoopBB);
            CondBB = SymbTable.parser->MilaBuilder.GetInsertBlock();

            TheFunction->getBasicBlockList().push_back(LoopBB);

            SymbTable.parser->MilaBuilder.SetInsertPoint(LoopBB);
            for(int i =body.size()-1;i>=0;--i){
                body[i]->Generate(st);
            }
            // bcondv = SymbTable.parser->MilaBuilder.CreateICmpNE(condv, llvm::ConstantInt::get(SymbTable.parser->MilaContext, llvm::APInt(32,0)), "ifcond");
            //    SymbTable.parser->MilaBuilder.CreateCondBr(bcondv, LoopBB, AfterLoopBB);
            llvm::Value *v=0;
            if(direction=='+')
                v=SymbTable.parser->MilaBuilder.CreateAdd(start, llvm::ConstantInt::get(SymbTable.parser->MilaContext, llvm::APInt(32,1)), "inctmp");
            else if(direction=='-')
                v=SymbTable.parser->MilaBuilder.CreateSub(start, llvm::ConstantInt::get(SymbTable.parser->MilaContext, llvm::APInt(32,1)), "dectmp");

            SymbTable.parser->MilaBuilder.CreateStore(v,addr);
            SymbTable.parser->MilaBuilder.CreateBr(CondBB);
            LoopBB = SymbTable.parser->MilaBuilder.GetInsertBlock();
            TheFunction->getBasicBlockList().push_back(AfterLoopBB);
            SymbTable.parser->MilaBuilder.SetInsertPoint(AfterLoopBB);
            //llvm::PHINode *PN = SymbTable.parser->MilaBuilder.CreatePHI(llvm::Type::getInt32Ty(SymbTable.parser->MilaContext), 2, "iftmp");

            //PN->addIncoming(ThenV, ThenBB);
            //PN->addIncoming(ElseV, ElseBB);
            return 1;
        }
    };




    NProgram *Start() ;
    NProgramName *Program();
    std::vector<NStatement *> Block() ;
    std::vector<NExpression *> Expressionlist(std::vector<NExpression *> l);
    std::vector<NExpression *> Expressionlistopt();
    std::vector<NStatement *> StatementPrime(std::string ident);
    std::vector<NStatement *> Statement();
    std::vector<NStatement *> StatementList(std::vector<NStatement *> l);
    std::vector<NDeclaration*> Declarations() ;
    std::vector<NDeclaration*>  Const();
    NConstDeclaration *ConstElem();
    std::vector<NDeclaration*> ConstList();
    std::string Type();
    std::vector<NVarDeclaration*> Var();
    std::vector<NVarDeclaration*> VarElem();
    std::vector<NVarDeclaration*> VarElemOpt();
    std::vector<std::string> VarIdentList() ;
    std::vector<NVarDeclaration*> VarList() ;
    std::vector<NDeclaration*> Function();
    std::vector<NDeclaration*> Procedure();
    NExpression *Expression();
    NExpression *ExpressionPrime(NExpression * v);
    NExpression *LogExpression();
    NExpression *LogExpressionPrime(NExpression * v);
    NExpression *AlgExpression();
    NExpression *AlgExpressionPrime(NExpression * v);
    NExpression *Term();
    NExpression *TermPrime(NExpression * v);
    NExpression *G();
    NExpression *GPrime(NExpression * v);
    NExpression *Factor();
    NExpression *FactorPrime(std::string name);


};



#endif //PJPPROJECT_PARSER_HPP
