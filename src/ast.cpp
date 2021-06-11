#include "ast.h"

Node::Node(string nodeName, string nodeType, int lineNo) {
    this->nodeName = new string(nodeName);
    this->nodeType = new string(nodeType);
    this->lineNo = lineNo;
    this->childNum = 0;
}

Node::Node(string nodeName, string nodeType, int childNum, ...) {
    this->nodeName = new string(nodeName);
    this->nodeType = new string(nodeType);

    this->childNum = childNum;
    this->childNode = new Node * [childNum];

    va_list l;
    va_start(l, childNum);
    for (int i = 0; i < childNum; i++) {
        Node* node;
        node = va_arg(l, Node *);
        this->childNode[i] = node;
    }
    this->lineNo = this->childNode[0]->lineNo;
    va_end(l);
} 

int Node::getValueType() {
    return getValueType(this);
}

int Node::getValueType(Node *node) {
    if (node->nodeType->compare("Specifier") == 0) {
        // Specifier --> Type
        if (node->childNode[0]->nodeName->compare("int")) {
            return TYPE_INT;
        } 
        else if (node->childNode[0]->nodeName->compare("float")) {
            return TYPE_FLOAT;
        } 
        else if (node->childNode[0]->nodeName->compare("char")) {
            return TYPE_CHAR;
        } 
        else if (node->childNode[0]->nodeName->compare("boolean")) {
            return TYPE_BOOL;
        } 
        else {

        }
    } else if (node->nodeType->compare("Exp") == 0) {
        return node->valueType;
    }
    // Error
}

llvm::Type* Node::getLlvmType(int type, int arraySize) {
    switch (type) {
        case TYPE_INT:
            return llvm::Type::getInt32Ty(context);
            break;
        case TYPE_INT_ARRAY:
            return llvm::ArrayType::get(llvm::Type::getInt32Ty(context), arraySize);
            break;
        case TYPE_FLOAT:
            return llvm::Type::getFloatTy(context);
            break;
        case TYPE_FLOAT_ARRAY:
            return llvm::ArrayType::get(llvm::Type::getFloatTy(context), arraySize);
            break;
        case TYPE_BOOL:
            return llvm::Type::getInt1Ty(context);
            break;
        case TYPE_BOOL_ARRAY:
            return llvm::ArrayType::get(llvm::Type::getInt1Ty(context), arraySize);
            break;
        case TYPE_CHAR:
            return llvm::Type::getInt8Ty(context);
            break;
        case TYPE_CHAR_ARRAY:
            return llvm::ArrayType::get(llvm::Type::getInt8Ty(context), arraySize);
            break;
        default:
            break;
    }
}


// ExtDecList --> VarDec
// ExtDecList --> VarDec COMMA ExtDecList
// DecList --> VarDec
// DecList --> VarDec COMMA DecList
vector<pair<string, int>> *Node::getNameList() {
    if (this->nodeType->compare("ExtDecList") != 0 && this->nodeType->compare("DecList") != 0) {
        cout<<"Wrong function call : getNameList ."<<endl;
    }
    Node *temp = this;
    vector<pair<string, int>> *nameList = new vector<pair<string, int>>;
    while (true) {
        // VarDec --> ID[INT]
        if (temp->childNode[0]->childNum == 4) {
            int arraySize = stoi(*temp->childNode[0]->childNode[3]->nodeName);
            nameList->push_back(make_pair(*temp->childNode[0]->childNode[0]->nodeName, ARRAY + arraySize));
        }
        // VarDec --> ID
        else if (temp->childNode[0]->childNum == 1) {
            nameList->push_back(make_pair(*temp->childNode[0]->childNode[0]->nodeName, VAR));
        }
        else {
            cout<<"Error:"<<endl;
            return nameList;
        } 
        // ExtDecList --> VarDec COMMA ExtDecList
        // DecList --> VarDec COMMA DecList
        if (temp->childNum == 3)
            temp = temp->childNode[2];
        // ExtDecList --> VarDec
        // DecList --> VarDec
        else
            break;        
    }
    return nameList;
}

// VarList --> ParamDec COMMA VarList
// VarList --> ParamDec
// ParamDec --> Specifier VarDec
vector<pair<string, llvm::Type*>> *Node::getParam() {
    if (this->nodeType->compare("VarList") != 0) {
        cout<<"Wrong function call : getParam ."<<endl;
    }
    Node *temp0 = this;
    // ParamDec
    Node *temp1 = this->childNode[0];
    vector<pair<string, llvm::Type*>> *paramList = new vector<pair<string, llvm::Type*>>;
    while (true) {
        temp1 = this->childNode[0];
        // ParamDec --> Specifier VarDec
        // VarDec --> ID[] 
        if (temp1->childNode[1]->childNum == 3) {
            paramList->push_back(make_pair(*temp1->childNode[1]->childNode[0]->nodeName, getLlvmType(ARRAY + getValueType(temp1->childNode[0]), 0)));
        }
        // VarDec --> ID
        else if (temp1->childNode[1]->childNum == 1) {
            paramList->push_back(make_pair(*temp1->childNode[1]->childNode[0]->nodeName, getLlvmType(VAR + getValueType(temp1->childNode[0]), 0)));
        }
        else {
            cout<<"Error:"<<endl;
            return paramList;
        } 
        // VarList --> ParamDec COMMA VarList
        if (temp0->childNum == 3)
            temp0 = temp0->childNode[2];
        // VarList --> ParamDec
        else
            break;        
    }
    return paramList;
}

// Args --> Exp COMMA Args
// Args --> Exp
vector<llvm::Value *> *Node::getArgs() {
    vector<llvm::Value *> * args = new vector<llvm::Value *>;
    Node *node = this;
    while (true) {
        if (node->childNum == 1) {
            args->push_back(node->childNode[0]->irBuildExp());
            break;
        }
        else {
            args->push_back(node->childNode[0]->irBuildExp());
            node = node->childNode[2];
        }
    }
    return args;
}

llvm::Value * Node::irBuild() {
    if (this->nodeType->compare("ExtDef") != 0) {
        if (this->childNode[1]->nodeType->compare("ExtDecList") == 0) {
            return this->irBuildVar();
        } else {
            return this->irBuildFun();
        }
    } else if (this->nodeType->compare("Def") != 0) {
        return this->irBuildVar();
    }
} 

llvm::Value * Node::irBuildExp() {
    if (this->childNode[0]->nodeType->compare("INT") == 0) {
        return builder.getInt32(stoi(*this->childNode[0]->nodeName));
    } 
    else if (this->childNode[0]->nodeType->compare("FLOAT") == 0) {
        return llvm::ConstantFP::get(builder.getFloatTy(), llvm::APFloat(stof(*this->childNode[0]->nodeName)));
    }
    else if (this->childNode[0]->nodeType->compare("BOOL") == 0) {
        if (this->childNode[0]->nodeName->compare("true") == 0) {
            return builder.getInt1(true);
        } else {
            return builder.getInt1(false);
        }
    }
    else if (this->childNode[0]->nodeType->compare("CHAR") == 0) {
        // char --> '$ch'
        return builder.getInt8(this->childNode[0]->nodeName->at(1));
    }
    else if (this->childNode[0]->nodeType->compare("STRING") == 0) {
        // string --> "$ch"
        string str = this->childNode[0]->nodeName->substr(1, this->childNode[0]->nodeName->length() - 2);
        return llvm::ConstantDataArray::getString(context, str);
        //return builder.getInt8(this->childNode[0]->nodeName->at(1));
    }
    else if (this->childNode[0]->nodeType->compare("ID") == 0) {
        if (this->childNum == 1) {
            // var value
            return builder.CreateLoad(generator->findValue(*this->childNode[0]->nodeName), "tmpvar");
        }
        // ID() function
        // ID[] array or point
        else if (this->childNum == 3) {
            if (this->childNode[1]->nodeType->compare("LP") == 0) {
                llvm::Function *fun = generator->module->getFunction(*this->childNode[0]->nodeName);
                if (fun == nullptr) {
                    cout<<"[ERROR] Funtion not defined: "<<*this->childNode[0]->nodeName<<endl;
                }
                return builder.CreateCall(fun, nullptr, "calltmp");
            }
            else {
                // var addr
                return generator->findValue(*this->childNode[0]->nodeName);
            }
        }
        else if (this->childNum == 4) {
            // ID LP Args RP
            if (this->childNode[1]->nodeType->compare("LP") == 0) {
                llvm::Function *fun = generator->module->getFunction(*this->childNode[0]->nodeName);
                if (fun == nullptr) {
                    cout<<"[ERROR] Funtion not defined: "<<*this->childNode[0]->nodeName<<endl;
                }
                vector<llvm::Value*> *args = this->childNode[2]->getArgs();
                return builder.CreateCall(fun, *args, "calltmp");
            }
            else {
                llvm::Value * arrayValue = generator->findValue(*this->childNode[0]->nodeName);
                llvm::Value * indexValue = this->childNode[2]->irBuildExp();
                vector<llvm::Value*> indexList;
                indexList.push_back(builder.getInt32(0));
                indexList.push_back(indexValue);
                // var value
                return builder.CreateLoad(builder.CreateInBoundsGEP(arrayValue, llvm::ArrayRef<llvm::Value*>(indexList)), "tmpvar");
            }
        }
    }
    else if (this->childNode[0]->nodeType->compare("LP") == 0) {
        return this->childNode[0]->irBuildExp();
    }
    else if (this->childNode[0]->nodeType->compare("MINUS") == 0) {
        return builder.CreateNeg(this->childNode[1]->irBuildExp(), "tmpNeg");
        //int type = this->childNode[1]->getValueType();
        //return type == TYPE_INT ? builder.CreateSub(builder.getInt32(0), this->childNode[1]->irBuildExp()) : builder.CreateFSub(llvm::ConstantFP::get(builder.getFloatTy(), llvm::APFloat(0.0)), this->childNode[1]->irBuildExp());
    }
    else if (this->childNode[0]->nodeType->compare("NOT") == 0) {
        builder.CreateNot(this->childNode[1]->irBuildExp(), "tmpNot");
    }
    // Exp op Exp
    else {
        llvm::Value *left = this->childNode[0]->irBuildExp();
        llvm::Value *right = this->childNode[2]->irBuildExp();
        int type = this->childNode[0]->getValueType();
        if (this->childNode[1]->nodeType->compare("ASSIGNOP") == 0) {
            return builder.CreateStore(this->childNode[2]->irBuildExp(), generator->findValue(*this->childNode[0]->nodeName));
        }
        else if (this->childNode[1]->nodeType->compare("AND") == 0) {
            return builder.CreateAnd(left, right, "tmpAnd");
        }
        else if (this->childNode[1]->nodeType->compare("OR") == 0) {
            return builder.CreateOr(left, right, "tmpOR");
        }
        else if (this->childNode[1]->nodeType->compare("RELOP") == 0) {
            return this->irBuildRELOP();
        }
        else if (this->childNode[1]->nodeType->compare("PLUS") == 0) {
            return type == TYPE_FLOAT ? builder.CreateFAdd(left, right, "addtmpf") : builder.CreateAdd(left, right, "addtmpi");
        }
        else if (this->childNode[1]->nodeType->compare("MINUS") == 0) {
            return type == TYPE_FLOAT ? builder.CreateFSub(left, right, "subtmpf") : builder.CreateSub(left, right, "subtmpi");
        }
        else if (this->childNode[1]->nodeType->compare("STAR") == 0) {
            return type == TYPE_FLOAT ? builder.CreateFMul(left, right, "multmpf") : builder.CreateMul(left, right, "multmpi");
        }
        else if (this->childNode[1]->nodeType->compare("DIV") == 0) {
            return type == TYPE_FLOAT ? builder.CreateFDiv(left, right, "divtmpf") : builder.CreateSDiv(left, right, "divtmpi");
        }
    }
    
}

// Exp RELOP Exp
llvm::Value * Node::irBuildRELOP() {
    int type = this->childNode[0]->getValueType();
    llvm::Value * left = this->childNode[0]->irBuildExp();
    llvm::Value * right = this->childNode[2]->irBuildExp();
    if (this->childNode[1]->nodeName->compare("==") == 0) {
        return (type == TYPE_INT) ? builder.CreateICmpEQ(left, right, "icmptmp") : builder.CreateFCmpOEQ(left, right, "fcmptmp");
    }
    else if (this->childNode[1]->nodeName->compare(">=") == 0) {
        return (type == TYPE_INT) ? builder.CreateICmpSGE(left, right, "icmptmp") : builder.CreateFCmpOGE(left, right, "fcmptmp");
    }
    else if (this->childNode[1]->nodeName->compare("<=") == 0) {
        return (type == TYPE_INT) ? builder.CreateICmpSLE(left, right, "icmptmp") : builder.CreateFCmpOLE(left, right, "fcmptmp");
    }
    else if (this->childNode[1]->nodeName->compare(">") == 0) {
        return (type == TYPE_INT) ? builder.CreateICmpSGT(left, right, "icmptmp") : builder.CreateFCmpOGT(left, right, "fcmptmp");
    }
    else if (this->childNode[1]->nodeName->compare("<") == 0) {
        return (type == TYPE_INT) ? builder.CreateICmpSLT(left, right, "icmptmp") : builder.CreateFCmpOLT(left, right, "fcmptmp");
    }
    else if (this->childNode[1]->nodeName->compare("!=") == 0) {
        return (type == TYPE_INT) ? builder.CreateICmpNE(left, right, "icmptmp") : builder.CreateFCmpONE(left, right, "fcmptmp");
    }
}

// CompSt --> LC DefList StmtList RC
// DefList --> Def DefList
// Def --> Specifier DecList SEMI
// StmtList --> Stmt StmtList
llvm::Value * Node::irBuildCompSt() {
    Node * defNodes = this->childNode[1];
    Node * stmtNodes = this->childNode[2];
    while (true) {
        if (defNodes->childNum == 2) {
            defNodes->childNode[0]->irBuildVar();
            defNodes = defNodes->childNode[1];
        }
        else {
            break;
        }
    } 
    while (true) {
        if (stmtNodes->childNum == 2) {
            stmtNodes->childNode[0]->irBuildStmt();
            stmtNodes = stmtNodes->childNode[1];
        }
        else {
            break;
        }
    }
    return NULL;
}

// ExtDef --> Specifier ExtDecList SEMI
// Def --> Specifier DecList SEMI
llvm::Value * Node::irBuildVar() {
    vector<pair<string, int>> *nameList = this->childNode[1]->getNameList();
    int type = this->childNode[0]->getValueType();
    llvm::Type *llvmType;
    for (auto it = nameList->begin(); it != nameList->end(); it++) {
        if (it->second == VAR) {
            llvmType = getLlvmType(type, 0);
        } else {
            llvmType = getLlvmType(type, it->second - ARRAY);
        }
        if (generator->getCurFunction() != nullptr) {
            llvm::GlobalVariable* array_global = new llvm::GlobalVariable(*generator->module, llvmType, true, llvm::GlobalValue::PrivateLinkage, 0, it->first);
        }
        else {
            llvm::Value* alloc = CreateEntryBlockAlloca(generator->getCurFunction(), it->first, llvmType);
        }
    }
}

// Specifier FunDec CompSt
llvm::Value * Node::irBuildFun() {
    vector<pair<string, llvm::Type*>> *params = getParam();

    vector<llvm::Type*> argTypes;
    for (auto it = params->begin(); it != params->end(); it ++) {
        argTypes.push_back(it->second);
    }
    llvm::FunctionType *funcType = llvm::FunctionType::get(getLlvmType(getValueType(this->childNode[0]), 0), argTypes, false);
    llvm::Function *function = llvm::Function::Create(funcType, llvm::GlobalValue::InternalLinkage, *this->childNode[1]->childNode[1]->nodeName, generator->module);
    generator->pushFunction(function);
    
    //Block
    llvm::BasicBlock *newBlock = llvm::BasicBlock::Create(context, "entrypoint", function, nullptr);
    builder.SetInsertPoint(newBlock);
    
    //Parameters
    llvm::Function::arg_iterator argIt = function->arg_begin();
    int index = 1;
    
    //Sub routine
    this->childNode[2]->irBuildCompSt();

    //Pop back
    generator->popFunction();
    builder.SetInsertPoint(&(generator->getCurFunction())->getBasicBlockList().back());
    return function;
}

// Stmt
llvm::Value *Node::irBuildStmt() {
    if (this->childNode[0]->nodeType->compare("Exp") == 0) {
        return this->irBuildExp();
    } else if (this->childNode[0]->nodeType->compare("IF") == 0) {
        return this->irBuildIf();
    } else if (this->childNode[0]->nodeType->compare("WHILE") == 0) {
        return this->irBuildWhile();
    } else if (this->childNode[0]->nodeType->compare("RETURN") == 0) {
        return this->irBuildReturn();
    } else if (this->childNode[0]->nodeType->compare("CompSt") == 0) {
        return this->irBuildCompSt();
    }
}

// WHILE LP Exp RP Stmt
llvm::Value *Node::irBuildWhile() {
    //this->forward(generator);
    llvm::Function *TheFunction = generator->getCurFunction();
    llvm::BasicBlock *condBB = llvm::BasicBlock::Create(context, "cond", TheFunction);
    llvm::BasicBlock *loopBB = llvm::BasicBlock::Create(context, "loop", TheFunction);
    llvm::BasicBlock *afterBB = llvm::BasicBlock::Create(context, "afterLoop", TheFunction);
    
    //Cond
    builder.CreateBr(condBB);
    builder.SetInsertPoint(condBB);
    // WHILE LP Exp RP Stmt
    llvm::Value *condValue = this->childNode[2]->irBuildExp();
    condValue = builder.CreateICmpNE(condValue, llvm::ConstantInt::get(llvm::Type::getInt1Ty(context), 0, true), "whileCond");
    auto branch = builder.CreateCondBr(condValue, loopBB, afterBB);
    condBB = builder.GetInsertBlock();
    
    //Loop
    builder.SetInsertPoint(loopBB);
    this->childNode[4]->irBuildStmt();
    builder.CreateBr(condBB);
    
    //After
    builder.SetInsertPoint(afterBB);
    //this->backward(generator);
    return branch;
}

// IF LP Exp RP Stmt %prec LOWER_THAN_ELSE
// IF LP Exp RP Stmt ELSE Stmt
llvm::Value *Node::irBuildIf() {
    //this->forward(generator);
    
    llvm::Value *condValue = this->childNode[2]->irBuildExp(), *thenValue = nullptr, *elseValue = nullptr;
    condValue = builder.CreateICmpNE(condValue, llvm::ConstantInt::get(llvm::Type::getInt1Ty(context), 0, true), "ifCond");

    llvm::Function *TheFunction = generator->getCurFunction();
    llvm::BasicBlock *thenBB = llvm::BasicBlock::Create(context, "then", TheFunction);
    llvm::BasicBlock *elseBB = llvm::BasicBlock::Create(context, "else", TheFunction);
    llvm::BasicBlock *mergeBB = llvm::BasicBlock::Create(context, "merge", TheFunction);

    // Then
    auto branch = builder.CreateCondBr(condValue, thenBB, elseBB);
    builder.SetInsertPoint(thenBB);
    thenValue = this->childNode[4]->irBuildStmt();
    builder.CreateBr(mergeBB);
    thenBB = builder.GetInsertBlock();

    // else
    if (this->childNum == 7) {
        builder.SetInsertPoint(elseBB);
        elseValue = this->childNode[6]->irBuildStmt();
        builder.CreateBr(mergeBB);
        elseBB = builder.GetInsertBlock();
    }
    builder.SetInsertPoint(mergeBB);    
    //this->backward(generator);
    return branch;
}

// RETURN Exp SEMI
llvm::Value *Node::irBuildReturn() {
    auto returnInst = this->childNode[1]->irBuildExp();
    builder.CreateRet(returnInst);
}

llvm::Value *Node::irBuildPrintf() {
    string formatStr = "";
    vector<llvm::Value *> *args = getArgs();
    for (auto & arg : *args) {
        if (arg->getType() == builder.getInt32Ty()) {
            formatStr += "%d";
        }
        else if (arg->getType() == builder.getInt8Ty()) {
            formatStr += "%c";
        }
        else if (arg->getType() == builder.getInt1Ty()) {
            formatStr += "%d";
        }
        else if (arg->getType() == builder.getFloatTy()) {
            formatStr += "%f";
        }
        else {
            throw logic_error("[ERROR]Invalid type to write.");
        }
    }
    formatStr += "\n";
    auto formatConst = llvm::ConstantDataArray::getString(context, formatStr.c_str());
    auto formatStrVar = new llvm::GlobalVariable(*(generator->module), llvm::ArrayType::get(builder.getInt8Ty(), formatStr.size() + 1), true, llvm::GlobalValue::ExternalLinkage, formatConst, ".str");
    auto zero = llvm::Constant::getNullValue(builder.getInt32Ty());
    llvm::Constant* indices[] = {zero, zero};
    auto varRef = llvm::ConstantExpr::getGetElementPtr(formatStrVar->getType()->getElementType(), formatStrVar, indices);
    args->insert(args->begin(), varRef);
    return builder.CreateCall(generator->print, *args, "print");
}

// Args --> Exp COMMA Args
// Args --> Exp
llvm::Value *Node::irBuildScanf() {
    string formatStr = "";
    vector<llvm::Value*> *args = getArgs();
    llvm::Value *argAddr, *argValue;
    //Just common variable
    for (auto arg : *args) {
        if (arg->getType()->getPointerElementType() == builder.getInt32Ty()) {
            formatStr += "%d";
        }
        else if (arg->getType()->getPointerElementType() == builder.getInt8Ty()) {
            formatStr += "%c";
        }
        else if (arg->getType()->getPointerElementType() == builder.getInt1Ty()) {
            formatStr += "%d";
        }
        else if (arg->getType()->getPointerElementType() == builder.getFloatTy()) {
            formatStr += "%f";
        }
        else {
            throw logic_error("[ERROR]Invalid type to read.");
        }
    }
    args->insert(args->begin(), builder.CreateGlobalStringPtr(formatStr));
    return builder.CreateCall(generator->scan, *args, "scan");
}



llvm::AllocaInst *CreateEntryBlockAlloca(llvm::Function *TheFunction, llvm::StringRef VarName, llvm::Type* type) {
  llvm::IRBuilder<> TmpB(&TheFunction->getEntryBlock(), TheFunction->getEntryBlock().begin());
  return TmpB.CreateAlloca(type, nullptr, VarName);
}