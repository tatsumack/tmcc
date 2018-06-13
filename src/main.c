#include <llvm-c/Core.h>
#include <llvm-c/ExecutionEngine.h>
#include <llvm-c/Target.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/BitWriter.h>

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

typedef struct Buf {
    char* body;
    int len;
} Buf;

#define BUF_LENGTH 128

Buf* make_buf()
{
    Buf* b = (Buf*) malloc(8);
    b->body = (char*) malloc(BUF_LENGTH);
    b->len = 0;
    return b;
}

void write_buf(Buf* b, char c)
{
    if (b->len >= BUF_LENGTH) {
        printf("buffer over\n");
        exit(1);
    }
    b->body[b->len++] = c;
}


void gen()
{
    LLVMModuleRef mod = LLVMModuleCreateWithName("tmcc");

    LLVMTypeRef param_types[] = { };
    LLVMTypeRef ret_type = LLVMFunctionType(LLVMInt32Type(), param_types, 2, 0);
    LLVMValueRef sum = LLVMAddFunction(mod, "sum", ret_type);

    LLVMBasicBlockRef entry = LLVMAppendBasicBlock(sum, "entry");

    LLVMBuilderRef builder = LLVMCreateBuilder();
    LLVMPositionBuilderAtEnd(builder, entry);
    LLVMValueRef tmp = LLVMBuildAdd(builder, LLVMGetParam(sum, 0), LLVMGetParam(sum, 1), "tmp");
    LLVMBuildRet(builder, tmp);

    char *error = NULL;
    LLVMVerifyModule(mod, LLVMAbortProcessAction, &error);
    LLVMDisposeMessage(error);

    LLVMExecutionEngineRef engine;
    error = NULL;
    LLVMLinkInJIT();
    LLVMInitializeNativeTarget();
    if (LLVMCreateExecutionEngineForModule(&engine, mod, &error) != 0) {
        fprintf(stderr, "failed to create execution engine\n");
        abort();
    }
    if (error) {
        fprintf(stderr, "error: %s\n", error);
        LLVMDisposeMessage(error);
        exit(EXIT_FAILURE);
    }

    if (argc < 3) {
        fprintf(stderr, "usage: %s x y\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    long long x = strtoll(argv[1], NULL, 10);
    long long y = strtoll(argv[2], NULL, 10);

    LLVMGenericValueRef args[] = {
        LLVMCreateGenericValueOfInt(LLVMInt32Type(), x, 0),
        LLVMCreateGenericValueOfInt(LLVMInt32Type(), y, 0)
    };
    LLVMGenericValueRef res = LLVMRunFunction(engine, sum, 2, args);
    printf("%d\n", (int)LLVMGenericValueToInt(res, 0));

    // Write out bitcode to file
    if (LLVMWriteBitcodeToFile(mod, "sum.bc") != 0) {
        fprintf(stderr, "error writing bitcode to file, skipping\n");
    }

    LLVMDisposeBuilder(builder);
    LLVMDisposeExecutionEngine(engine);
}

int main(int argc, char* argv[])
{
    Buf* b = make_buf();

    for(;;)
    {
        char c = getc(stdin);
        if (isspace(c)) continue;

        if (isdigit(c))
        {
            write_buf(b, c);
        }

        if (c =='\n') break;
    }
    write_buf(b, '\0');
    printf("%s", b->body);

    return 0;
}

