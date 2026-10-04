#pragma once

namespace llvm {

class Function;
class IntegerType;
class Module;
class StructType;

} // namespace llvm

namespace avium {

struct RuntimeAbi {
    explicit RuntimeAbi(llvm::Module& module);

    llvm::StructType* textType;
    llvm::IntegerType* sizeType;
    llvm::Function* textCreate;
    llvm::Function* textCopy;
    llvm::Function* textDestroy;
    llvm::Function* textMoveAssign;
    llvm::Function* textConcat;
    llvm::Function* textCompare;
    llvm::Function* str;
    llvm::Function* strBool;
    llvm::Function* num;
    llvm::Function* textLength;
    llvm::Function* arrayCreate;
    llvm::Function* arrayDestroy;
    llvm::Function* arrayLength;
    llvm::Function* textArrayAt;
    llvm::Function* realArrayAt;
    llvm::Function* boolArrayAt;
    llvm::Function* printBool;
    llvm::Function* printReal;
    llvm::Function* printText;
    llvm::Function* input;
    llvm::Function* sqr;
};

} // namespace avium
