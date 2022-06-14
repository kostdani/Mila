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

const bool DEBUG=false;

class Parser {
public:
    Parser();
    ~Parser() = default;

    bool Parse();                    // parse
    const llvm::Module& Generate();  // generate

private:


    int getNextToken();

    Lexer m_Lexer;                   // lexer is used to read tokens
    int CurTok;                      // to keep the current token

    llvm::LLVMContext MilaContext;   // llvm context
    llvm::IRBuilder<> MilaBuilder;   // llvm builder
    llvm::Module MilaModule;         // llvm module

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

    void Compare(int s) {
        if (CurTok == s)
            getNextToken();
        else
            CompareError(s);
    }

    class SymbTable{
        std::map<std::string,llvm::Value*> ConstTable;
        std::map<std::string,llvm::Value*> VarTable;
        std::map<std::string,llvm::FunctionCallee> CallTable;
    public:
        llvm::BasicBlock *contbb=0;
        Parser *parser;
        SymbTable(Parser *p){
            parser=p;
        }
        int AddConst(std::string name,llvm::Value* value){
            ConstTable[name]=value;
            return 1;
        }
        int AddVar(std::string name,llvm::Value* value){
            VarTable[name]=value;
            return 1;
        }
        int AddFunc(std::string name,llvm::FunctionCallee value){
            CallTable[name]=value;
            return 1;
        }
        llvm::Value *GetVal(std::string name){
            if (ConstTable.find(name) != ConstTable.end())
                return ConstTable[name];
            if (VarTable.find(name) != VarTable.end()) {
                auto a= parser->MilaBuilder.CreateLoad(llvm::Type::getInt32Ty(parser->MilaContext),VarTable[name], name.c_str());
                return a;
            }
            return 0;
        }
        llvm::Value *GetAddr(std::string name){
            if (VarTable.find(name) != VarTable.end())
                return VarTable[name];
            return 0;
        }
        llvm::FunctionCallee GetCallee(std::string name){
            if (CallTable.find(name) != CallTable.end())
                return CallTable[name];
            return 0;
        }
    };
    class ASTNode{
    public:
        virtual void Write(Parser* p) {

        }
        virtual int Generate(SymbTable &SymbTable){/*
            printf("Default generation\n");
            // create writeln function
            {
                std::vector<llvm::Type*> Ints(1, llvm::Type::getInt32Ty(p->MilaContext));
                llvm::FunctionType * FT = llvm::FunctionType::get(llvm::Type::getInt32Ty(p->MilaContext), Ints, false);
                llvm::Function * F = llvm::Function::Create(FT, llvm::Function::ExternalLinkage, "writeln", p->MilaModule);
                for (auto & Arg : F->args())
                    Arg.setName("x");
            }

            // create main function
            {
                llvm::FunctionType * FT = llvm::FunctionType::get(llvm::Type::getInt32Ty(p->MilaContext), false);
                llvm::Function * MainFunction = llvm::Function::Create(FT, llvm::Function::ExternalLinkage, "main", p->MilaModule);

                // block
                llvm::BasicBlock * BB = llvm::BasicBlock::Create(p->MilaContext, "entry", MainFunction);
                p->MilaBuilder.SetInsertPoint(BB);

                // call writeln with value from lexel
                p->MilaBuilder.CreateCall(p->MilaModule.getFunction("writeln"), {
                        llvm::ConstantInt::get(p->MilaContext, llvm::APInt(32, 42))
                });

                // return 0
                p->MilaBuilder.CreateRet(llvm::ConstantInt::get(llvm::Type::getInt32Ty(p->MilaContext), 0));
            }*/
        return 1;
        }

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
        void Write(Parser *p){
            std::cout<<"PROGRAM:\n";
            pName->Write(p);
            for (int i =0;i<decs.size();++i){
                decs[i]->Write(p);
            }
            for (int i =0;i<block.size();++i){
                block[i]->Write(p);
            }
        }
        int Generate( SymbTable &SymbTable){
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

            for (int i =0;i<decs.size();++i){
                decs[i]->PreDeclare(SymbTable);
            }
            // create main function
            {
                llvm::FunctionType * FT = llvm::FunctionType::get(llvm::Type::getInt32Ty(SymbTable.parser->MilaContext), false);
                llvm::Function * MainFunction = llvm::Function::Create(FT, llvm::Function::ExternalLinkage, "main", SymbTable.parser->MilaModule);

                // block
                llvm::BasicBlock * BB = llvm::BasicBlock::Create(SymbTable.parser->MilaContext, "entry", MainFunction);
                SymbTable.parser->MilaBuilder.SetInsertPoint(BB);

                for (int i =decs.size()-1;i>=0;--i){
                    decs[i]->GlobDeclare(SymbTable);
                    //SymbTable.parser->MilaBuilder.SetInsertPoint(BB);
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
    };
    class NConstDeclaration : public NDeclaration{
        std::string name;
        int right;
    public:
        NConstDeclaration(const std::string &n,int r):name(n),right(r){}
        void Write(Parser *p){
            std::cout<<"constdec: "<<name<<"="<<right<<"\n";
        }

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
        void Write(Parser *p){
            std::cout<<"vardec: "<<name<<"\n";
        }

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

        int Declare(SymbTable &SymbTable){

            return 1;
        //    SymbTable[name]=llvm::ConstantInt::get(SymbTable.parser->MilaContext, llvm::APInt(32, 0));
        }
    };
    class NAssignment : public NStatement{
        std::string left;
        NExpression *right;
    public:
        NAssignment(const std::string &l,NExpression *r):left(l),right(r){}
        void Write(Parser *p){
            std::cout<<"assign: "<<left<<":=";
            right->Write(p);
            std::cout<<"\n";
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
            auto strref =SymbTable.parser->MilaBuilder.GetInsertBlock()->getParent()->getName();
            SymbTable.parser->MilaBuilder.CreateRet(SymbTable.GetVal(std::string(strref.begin(),strref.end())));
            return 0;
        }
    };
    class NBreak:public NStatement{
    public:
        int Generate(SymbTable &SymbTable){
            SymbTable.parser->MilaBuilder.CreateBr(SymbTable.contbb);
            return 1;
        }
    };
    class NFunctionDeclaration : public NDeclaration{
        NFunctionPrototype* prototype;
        std::vector<NDeclaration*> decs;
        std::vector<NStatement*> block;
    public:
        NFunctionDeclaration(NFunctionPrototype* p,std::vector<NDeclaration*> d, std::vector<NStatement*> b): prototype(p),decs(d),block(b){}
        void Write(Parser *p){
            std::cout<<"funcdec: "<<prototype->name<<" "<<prototype->args.size()<<" "<<prototype->type<<"\n";
            for (int i =0;i<decs.size();++i){
                decs[i]->Write(p);
            }
            for (int i =0;i<block.size();++i){
                block[i]->Write(p);
            }
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
            st.AddVar(prototype->name,SymbTable.parser->MilaBuilder.CreateAlloca(llvm::Type::getInt32Ty(SymbTable.parser->MilaContext), 0,prototype->name.c_str()));

            for (auto &Arg : F->args())
                st.AddConst(std::string(Arg.getName().begin(),Arg.getName().end()),&Arg);

            for (int i =decs.size()-1;i>=0;--i){
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
        void Write(Parser *p){
            std::cout<<"funcdec: "<<prototype->name<<" "<<prototype->args.size()<<"\n";
            for (int i =0;i<decs.size();++i){
                decs[i]->Write(p);
            }
            for (int i =0;i<block.size();++i){
                block[i]->Write(p);
            }
        }

        void PreDeclare(SymbTable &SymbTable){

            auto curblock =SymbTable.parser->MilaBuilder.GetInsertBlock();
            auto st=SymbTable;
            std::vector<llvm::Type*> args(prototype->args.size(),
                                          llvm::Type::getInt32Ty(SymbTable.parser->MilaContext));
            llvm::FunctionType *FT =llvm::FunctionType::get(llvm::Type::getInt32Ty(SymbTable.parser->MilaContext), args, false);

            llvm::Function *F = llvm::Function::Create(FT, llvm::Function::ExternalLinkage, prototype->name, SymbTable.parser->MilaModule);
            unsigned Idx = 0;
            for (auto &Arg : F->args())
                Arg.setName(prototype->args[Idx++]->name);

            for (int i =0;i<decs.size();++i){
                decs[i]->PreDeclare(st);
            }

            llvm::BasicBlock *BB = llvm::BasicBlock::Create(SymbTable.parser->MilaContext, "entry", F);
            SymbTable.parser->MilaBuilder.SetInsertPoint(BB);

            SymbTable.AddFunc(prototype->name,llvm::FunctionCallee(FT,F));
            for (auto &Arg : F->args())
                st.AddConst(std::string(Arg.getName().begin(),Arg.getName().end()),&Arg);
            for (int i =decs.size()-1;i>=0;--i){
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
        void Write(Parser *p){
            std::cout<<Val<<", ";
        }
        llvm::Value *Value( SymbTable &SymbTable){
            return llvm::ConstantInt::get(SymbTable.parser->MilaContext, llvm::APInt(32, Val));
        }
    };
    class NVarExpression : public NExpression{
    public:
        std::string Val;
        NVarExpression(std::string val):Val(val){}
        void Write(Parser *p){
            std::cout<<Val<<" ";
        }
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
    };
    class NBinaryExpression : public NExpression{
        char operation;
        NExpression* left;
        NExpression* right;
    public:
        NBinaryExpression(char o, NExpression* e1, NExpression* e2):operation(o),left(e1),right(e2){}
        void Write(Parser *p){
            std::cout<<"(";
            left->Write(p);
            std::cout<<operation;
            right->Write(p);
            std::cout<<")";
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
        void Write(Parser *p){
            std::cout<<"Call expression "<<callee<<" (";
            for(int i =0;i<args.size();++i){
                args[i]->Write(p);
            }
            std::cout<<")\n";
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
        void Write(Parser *p){
            std::cout<<"Call statement "<<callee<<" (";
            for(int i =0;i<args.size();++i){
                args[i]->Write(p);
            }
            std::cout<<")\n";
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
            SymbTable.contbb=MergeBB;
            SymbTable.parser->MilaBuilder.CreateCondBr(condv, ThenBB, ElseBB);
            SymbTable.parser->MilaBuilder.SetInsertPoint(ThenBB);
            for(int i =0;i<th.size();++i){
                th[i]->Generate(SymbTable);
            }
            SymbTable.parser->MilaBuilder.CreateBr(MergeBB);
            ThenBB = SymbTable.parser->MilaBuilder.GetInsertBlock();
            TheFunction->getBasicBlockList().push_back(ElseBB);
            SymbTable.parser->MilaBuilder.SetInsertPoint(ElseBB);
            for(int i =0;i<el.size();++i){
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
    };
    class NFor : public NStatement{
        NExpression* sexpr;
        NExpression* eexpr;
        char direction;
        std::vector<NStatement *> body;
    public:
        NFor(char d,NExpression* ex1,NExpression* ex2,std::vector<NStatement *> b):direction(d),sexpr(ex1),eexpr(ex2),body(b){}
    };

    NProgram *Start() {
        NProgramName *pname;
        std::vector<NStatement*> b;
        std::vector<NDeclaration*> d;
        switch (CurTok) {
            case tok_program:
                if(DEBUG)
                printf("(1) S -> P D B .\n");
                pname=Program();
                d=Declarations(d);
                b=Block();
                Compare('.');
                // Compare(tok_eof);
                return new NProgram(pname,d,b);
            default:
                ExpansionError("S", CurTok);
                return 0;
        }
    }

    NProgramName *Program() {
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

    std::vector<NStatement *> Block() {
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

    std::vector<NExpression *> Expressionlist(std::vector<NExpression *> l){
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

    std::vector<NExpression *> Expressionlistopt(){
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
    std::vector<NStatement *> StatementPrime(std::string ident){
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
    std::vector<NStatement *> Statement(){
        std::string ident;
        std::vector<NStatement *>res;
        NExpression *a;
        NExpression *a2;
        std::vector<NStatement *>b;
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
                Compare(tok_for);
                a=Expression();
                switch (CurTok){
                    case tok_to:
                        dir='+';
                        break;
                    case tok_downto:
                        dir='-';
                        break;
                    default:
                        ExpansionError("to-downto", CurTok);
                        return {};
                }
                a2=Expression();
                Compare(tok_do);
                b=Statement();
                return {new NFor(dir,a,a2,c)};

            default:
                ExpansionError("S", CurTok);
                return {};
        }
    }




    std::vector<NStatement *> StatementList(std::vector<NStatement *> l) {
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
    std::vector<NDeclaration*> Declarations(std::vector<NDeclaration*> list) {
        std::vector<NDeclaration*> nlist;
        std::vector<NVarDeclaration*> vlist;
        switch (CurTok) {
            case tok_begin:
                if(DEBUG)
                printf("(5.1) D -> e\n");
                return list;
            case tok_const:
                if(DEBUG)
                printf("(5.2) D -> CD\n");
                nlist=Const();
                list.insert(list.end(),nlist.begin(),nlist.end());
                return Declarations(list);
            case tok_var:
                if(DEBUG)
                printf("(5.3) D -> VD\n");
                vlist=Var();
                for(int i=0;i<vlist.size();++i)
                    list.push_back(vlist[i]);
                return Declarations(list);
            case tok_procedure:
                if(DEBUG)
                printf("(5.4) D -> PD\n");
                nlist=Procedure();
                list.insert(list.end(),nlist.begin(),nlist.end());
                return Declarations(list);
            case tok_function:
                if(DEBUG)
                printf("(5.5) D -> FD\n");
                nlist=Function();
                list.insert(list.end(),nlist.begin(),nlist.end());
                return Declarations(list);
            default:
                ExpansionError("D", CurTok);
                return {};
        }
    }


    std::vector<NDeclaration*>  Const() {
        std::vector<NDeclaration*> list={};
        switch (CurTok) {
            case tok_const:
                if(DEBUG)
                printf("(6) C -> const Celem Clist\n");
                Compare(tok_const);
                list.push_back(ConstElem());
                return ConstList(list);
            default:
                ExpansionError("C", CurTok);
                return {};
        }
    }

    NConstDeclaration *ConstElem() {
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
    std::vector<NDeclaration*> ConstList(std::vector<NDeclaration*> list) {
        switch (CurTok) {
            case tok_begin:
            case tok_const:
            case tok_var:
            case tok_function:
            case tok_procedure:
                if(DEBUG)
                printf("(6.2) Cl -> e\n");
                return list;
            case tok_identifier:
                if(DEBUG)
                printf("(6.3) Cl -> Ce Cl\n");
                list.push_back(ConstElem());
                return ConstList(list);
            default:
                ExpansionError("C", CurTok);
                return {};
        }
    }

    std::string Type() {
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
    std::vector<NVarDeclaration*> Var() {
        std::vector<NVarDeclaration*> list;
        switch (CurTok) {
            case tok_var:
                if(DEBUG)
                printf("(7) V -> var Velem Vlist\n");
                Compare(tok_var);
                list=VarElem();
                return VarList(list);
            default:
                ExpansionError("V", CurTok);
                return {};
        }
    }

    std::vector<NVarDeclaration*> VarElem() {
        std::vector<NVarDeclaration*> res;
        std::vector<std::string> ilist={};
        std::string type;
        switch (CurTok) {
            case tok_identifier:
                if(DEBUG)
                printf("(7.1) Ce -> ident VIl : T\n");
                ilist.push_back(m_Lexer.identifierStr());
                Compare(tok_identifier);
                ilist= VarIdentList(ilist);
                Compare(':');
                type=Type();
                if(CurTok==';')
                Compare(';');
                for(int i =0;i<ilist.size();++i){
                    res.push_back(new NVarDeclaration(ilist[i],type));
                }
                return res;
            default:
                ExpansionError("C", CurTok);
                return {};
        }
    }
    std::vector<NVarDeclaration*> VarElemOpt() {
        switch (CurTok) {
            case ')':
                return {};
            case tok_identifier:
                return VarList({});
            default:
                ExpansionError("C", CurTok);
                return {};
        }
    }
    std::vector<std::string> VarIdentList(std::vector<std::string> ilist) {
        switch (CurTok) {
            case ':':
                if(DEBUG)
                printf("(7.3) VIl -> e\n");
                return ilist;
            case ',':
                if(DEBUG)
                printf("(7.2) VIl -> , ident VIl\n");
                Compare(',');
                ilist.push_back(m_Lexer.identifierStr());
                Compare(tok_identifier);
                return VarIdentList(ilist);
            default:
                ExpansionError("VIl", CurTok);
                return {};
        }
    }
    std::vector<NVarDeclaration*> VarList(std::vector<NVarDeclaration*> vlist) {
        std::vector<NVarDeclaration*> nlist;
        switch (CurTok) {
            case tok_begin:
            case tok_const:
            case tok_var:
            case tok_function:
            case tok_procedure:
            case ')':
                if(DEBUG)
                printf("(7.4) Vl -> e\n");
                return vlist;
            case tok_identifier:
                if(DEBUG)
                printf("(7.5) CV -> Ve Vl\n");

                nlist=VarElem();
                vlist.insert(vlist.end(),nlist.begin(),nlist.end());
                //Compare(';');
                return VarList(vlist);
            default:
                ExpansionError("VL", CurTok);
                return {};
        }
    }

    std::vector<NDeclaration*> Function() {
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
                decs=Declarations({});
                block=Block();
                Compare(';');
                return {new NFunctionDeclaration(new NFunctionPrototype(name, args, type), decs, block)};
            default:
                ExpansionError("F", CurTok);
                return {};
        }
    }

    std::vector<NDeclaration*> Procedure() {
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
                decs=Declarations({});
                block=Block();
                Compare(';');
                return {new NProcedureDeclaration(new NProcedurePrototype(name, args), decs, block)};
            default:
                ExpansionError("P", CurTok);
                return {};
        }
    }
    NExpression *Expression(){
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

    NExpression *ExpressionPrime(NExpression * v){
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
    NExpression *LogExpression(){
        int n;
        switch (CurTok) {
            case tok_identifier:
            case tok_number:
            case '(':
                if(DEBUG)
                printf(" E -> T E'\n");
                return LogExpressionPrime(AlgExpression());
            default:
                ExpansionError("E", CurTok);
                return 0;
        }
    }

    NExpression *LogExpressionPrime(NExpression * v){
        int n;
        switch (CurTok) {
            case tok_mod:
                if(DEBUG)
                printf("(2a) E' -> mod T E'\n");
                Compare(tok_mod);
                return LogExpressionPrime(new NBinaryExpression('%',v,AlgExpression()));
            case '=':
                if(DEBUG)
                printf("(2b) E' -> = T E'\n");
                Compare('=');
                return LogExpressionPrime(new NBinaryExpression('=',v,AlgExpression()));
            case tok_notequal:
                if(DEBUG)
                    printf("(2b) E' -> != T E'\n");
                Compare(tok_notequal);
                return LogExpressionPrime(new NBinaryExpression('!',v,AlgExpression()));
            case '<':
                if(DEBUG)
                printf("(2b) E' -> < T E'\n");
                Compare('<');
                return LogExpressionPrime(new NBinaryExpression('<',v,AlgExpression()));
            case '>':
                if(DEBUG)
                printf("(2b) E' -> > T E'\n");
                Compare('>');
                return LogExpressionPrime(new NBinaryExpression('>',v,AlgExpression()));
            case tok_lessequal:
                if(DEBUG)
                printf("(2b) E' -> <= T E'\n");
                Compare(tok_lessequal);
                return LogExpressionPrime(new NBinaryExpression('(',v,AlgExpression()));
            case tok_greaterequal:
                if(DEBUG)
                printf("(2b) E' -> >= T E'\n");
                Compare(tok_greaterequal);
                return LogExpressionPrime(new NBinaryExpression(')',v,AlgExpression()));
            case ')':
            case tok_then:
            case tok_else:
            case tok_do:
            case tok_and:
            case tok_or:
            case ';':
            case ',':
                if(DEBUG)
                printf("(3) E' -> e\n");
                return v;
            default:
                ExpansionError("E'", CurTok);
                return 0;
        }
    }

    NExpression *AlgExpression(){
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

    NExpression *AlgExpressionPrime(NExpression * v){
        int n;
        switch (CurTok) {
            case '+':
                if(DEBUG)
                printf("(2a) E' -> + T E'\n");
                Compare('+');
                return ExpressionPrime(new NBinaryExpression('+',v,Term()));
            case '-':
                if(DEBUG)
                printf("(2b) E' -> - T E'\n");
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
                if(DEBUG)
                printf("(3) E' -> e\n");
                return v;
            default:
                ExpansionError("aE'", CurTok);
                return 0;
        }
    }

    NExpression *Term(){
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
    NExpression *TermPrime(NExpression * v){
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
                if(DEBUG)
                printf("(5) T' -> e\n");
                return v;
            default:
                ExpansionError("T'", CurTok);
                return 0;
        }
    }
    NExpression *G(){
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
    NExpression *GPrime(NExpression * v){
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
                if(DEBUG)
                printf("(5) G' -> e\n");
                return v;
            default:
                ExpansionError("G'", CurTok);
                return 0;
        }
    }

    NExpression *Factor(){
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
    NExpression *FactorPrime(std::string name){
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
};



#endif //PJPPROJECT_PARSER_HPP
