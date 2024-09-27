#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#include "native.h"
#include "common.h"
#include "compiler.h"
#include "object.h"
#include "memory.h"
#include "vm.h"

const char** globalArgs;
int globalArgsCount;

// Transfer the args so they can be used
void init(const char** args, int argsCountt) {
    globalArgs = args;
    globalArgsCount = argsCountt;
}

static Value clockNative(int argCount, Value* args) {
    if (argCount != 0) {
        runtimeError("Expected 0 arguments but got %d.", argCount);
    }

    return NUMBER_VAL((double)clock() / CLOCKS_PER_SEC);
}

static Value argcNative(int argCount, Value* args) {
    if (argCount != 0) {
        runtimeError("Expected 0 arguments but got %d.", argCount);
    }

    return NUMBER_VAL(globalArgsCount);
}

static Value argvNative(int argCount, Value* args) {
    if (argCount != 1) {
        runtimeError("Expected 1 argument but got %d.", argCount);
    }

    if (!IS_NUMBER(args[0])) {
        runtimeError("Argument must be a number.");
    }

    int index = (int)AS_NUMBER(args[0]);
    if (index < 0 || index >= globalArgsCount) {
        runtimeError("Index out of bounds. There are %d arguments.", globalArgsCount);
    }

    const char* arg = globalArgs[index];
    if (arg == NULL) {
        runtimeError("Argument at index %d is NULL.", index);
    }

    return OBJ_VAL(copyString(arg, (int)strlen(arg)));
}

static Value stringizeNative(int argCount, Value* args) {
    if (argCount != 1) {
        runtimeError("Expected 1 argument but got %d.", argCount);
    }

    if (IS_STRING(args[0])) {
        return args[0];
    } else if (IS_NUMBER(args[0])) {
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "%g", AS_NUMBER(args[0]));
        return OBJ_VAL(copyString(buffer, (int)strlen(buffer)));
    } else {
        runtimeError("Unsupported type for stringize.");
    }
}

static Value integizeNative(int argCount, Value* args) {
    if (argCount != 1) {
        runtimeError("Expected 1 argument but got %d.", argCount);
    }

    if (IS_STRING(args[0])) {
        char* end;
        const char* str = AS_CSTRING(args[0]);
        double number = strtod(str, &end);
        
        if (end != str && *end == '\0') {
            return NUMBER_VAL(number);
        } else {
            runtimeError("String could not be converted to a number.");
        }
    } else if (IS_NUMBER(args[0])) {
        return args[0];
    } else {
        runtimeError("Unsupported type for integize.");
    }
}

static Value isNumNative(int argCount, Value* args) {
    if (argCount != 1) {
        runtimeError("Expected 1 argument but got %d.", argCount);
    }

    return BOOL_VAL(IS_NUMBER(args[0]));
}

static Value isBoolNative(int argCount, Value* args) {
    if (argCount != 1) {
        runtimeError("Expected 1 argument but got %d.", argCount);
    }

    return BOOL_VAL(IS_BOOL(args[0]));
}

static Value isObjNative(int argCount, Value* args) {
    if (argCount != 1) {
        runtimeError("Expected 1 argument but got %d.", argCount);
    }

    return BOOL_VAL(IS_OBJ(args[0]));
}

static Value isStrNative(int argCount, Value* args) {
    if (argCount != 1) {
        runtimeError("Expected 1 argument but got %d.", argCount);
    }

    return BOOL_VAL(IS_STRING(args[0]));
}

static Value isInstanceNative(int argCount, Value* args) {
    if (argCount != 1) {
        runtimeError("Expected 1 argument but got %d.", argCount);
    }

    return BOOL_VAL(IS_INSTANCE(args[0]));
}

static Value isNullNative(int argCount, Value* args) {
    if (argCount != 1) {
        runtimeError("Expected 1 argument but got %d.", argCount);
    }

    return BOOL_VAL(IS_NULL(args[0]));
}

static Value isNativeNative(int argCount, Value* args) {
    if (argCount != 1) {
        runtimeError("Expected 1 argument but got %d.", argCount);
    }

    return BOOL_VAL(IS_NATIVE(args[0]));
}

static Value isBoundMethodNative(int argCount, Value* args) {
    if (argCount != 1) {
        runtimeError("Expected 1 argument but got %d.", argCount);
    }

    return BOOL_VAL(IS_BOUND_METHOD(args[0]));
}


static Value isClassNative(int argCount, Value* args) {
    if (argCount != 1) {
        runtimeError("Expected 1 argument but got %d.", argCount);
    }

    return BOOL_VAL(IS_CLASS(args[0]));
}

static Value broadcastNative(int argCount, Value* args) {
    if (argCount != 1) {
        runtimeError("Expected 1 argument but got %d.", argCount);
    }

    printValue(args[0]);
    printf("\n");
}

static Value receiveNative(int argCount, Value* args) {
    if (argCount != 1) {
        runtimeError("Expected 1 argument but got %d.", argCount);
    }

    printValue(args[0]);

    char input[1024];
    int i = 0;
    int c = getchar();

    while (c != '\n' && i < 1023) {
        input[i] = c;
        i++;
        c = getchar();
    }

    input[i] = '\0';
    return OBJ_VAL(copyString(input, i));
}

static Value systemNative(int argCount, Value* args) {
    if (argCount != 1) {
        runtimeError("Expected 1 argument but got %d.", argCount);
    }

    if (!IS_STRING(args[0])) {
        runtimeError("Argument must be a number.");
    }

    char* cmd = AS_CSTRING(args[0]);
    system(cmd);
}

static Value sinNative(int argCount, Value* args) {
    if (argCount != 1) {
        runtimeError("Expected 1 argument but got %d.", argCount);
    }

    if (!IS_NUMBER(args[0])) {
        runtimeError("Argument must be a number.");
    }

    return NUMBER_VAL(sin(AS_NUMBER(args[0])));
}

static Value cosNative(int argCount, Value* args) {
    if (argCount != 1) {
        runtimeError("Expected 1 argument but got %d.", argCount);
    }

    if (!IS_NUMBER(args[0])) {
        runtimeError("Argument must be a number.");
    }

    return NUMBER_VAL(cos(AS_NUMBER(args[0])));
}

static Value tanNative(int argCount, Value* args) {
    if (argCount != 1) {
        runtimeError("Expected 1 argument but got %d.", argCount);
    }

    if (!IS_NUMBER(args[0])) {
        runtimeError("Argument must be a number.");
    }

    return NUMBER_VAL(tan(AS_NUMBER(args[0])));
}

static Value asinNative(int argCount, Value* args) {
    if (argCount != 1) {
        runtimeError("Expected 1 argument but got %d.", argCount);
    }

    if (!IS_NUMBER(args[0])) {
        runtimeError("Argument must be a number.");
    }

    return NUMBER_VAL(asin(AS_NUMBER(args[0])));
}

static Value acosNative(int argCount, Value* args) {
    if (argCount != 1) {
        runtimeError("Expected 1 argument but got %d.", argCount);
    }

    if (!IS_NUMBER(args[0])) {
        runtimeError("Argument must be a number.");
    }

    return NUMBER_VAL(acos(AS_NUMBER(args[0])));
}

static Value atanNative(int argCount, Value* args) {
    if (argCount != 1) {
        runtimeError("Expected 1 argument but got %d.", argCount);
    }

    if (!IS_NUMBER(args[0])) {
        runtimeError("Argument must be a number.");
    }

    return NUMBER_VAL(atan(AS_NUMBER(args[0])));
}

static Value absNative(int argCount, Value* args) {
    if (argCount != 1) {
        runtimeError("Expected 1 argument but got %d.", argCount);
    }

    if (!IS_NUMBER(args[0])) {
        runtimeError("Argument must be a number.");
    }

    return NUMBER_VAL(fabs(AS_NUMBER(args[0])));
}

static Value hypotNative(int argCount, Value* args) {
    if (argCount != 2) {
        runtimeError("Expected 2 arguments but got %d.", argCount);
    }

    if (!IS_NUMBER(args[0]) && !IS_NUMBER(args[1])) {
        runtimeError("Arguments must be a number.");
    }

    return NUMBER_VAL(hypot(AS_NUMBER(args[0]), AS_NUMBER(args[1])));
}

static Value sqrtNative(int argCount, Value* args) {
    if (argCount != 1) {
        runtimeError("Expected 2 arguments but got %d.", argCount);
    }

    if (!IS_NUMBER(args[0])) {
        runtimeError("Argument must be a number.");
    }

    return NUMBER_VAL(sqrt(AS_NUMBER(args[0])));
}

static Value powrNative(int argCount, Value* args) {
    if (argCount != 2) {
        runtimeError("Expected 2 arguments but got %d.", argCount);
    }

    if (!IS_NUMBER(args[0]) && !IS_NUMBER(args[1])) {
        runtimeError("Arguments must be a number.");
    }

    return NUMBER_VAL(pow(AS_NUMBER(args[0]), AS_NUMBER(args[1])));
}

static Value mdlsNative(int argCount, Value* args) {
    if (argCount != 2) {
        runtimeError("Expected 2 arguments but got %d.", argCount);
    }

    if (!IS_NUMBER(args[0]) && !IS_NUMBER(args[1])) {
        runtimeError("Arguments must be a number.");
    }

    int a = AS_NUMBER(args[0]);
    int b = AS_NUMBER(args[1]);
    return BOOL_VAL(a % b == 0);
}

static Value collectGarbageNative(int argCount, Value* args) {
    if (argCount != 0) {
        runtimeError("Expected 0 arguments but got %d.", argCount);
    }

    collectGarbage();
}

static Value runtimeErrorNative(int argCount, Value* args) {
    if (argCount != 1) {
        runtimeError("Expected 1 arguments but got %d.", argCount);
    }

    if (!IS_STRING(args[0])) {
        runtimeError("Argument 1 must be a string.");
    }

    runtimeError(AS_CSTRING(args[0]));
    exit(404);
}

static Value interpretNative(int argCount, Value* args) {
    if (argCount != 1) {
        runtimeError("Expected 1 arguments but got %d.", argCount);
    }

    if (!IS_STRING(args[0])) {
        runtimeError("Argument 1 must be a string.");
    }

    interpret(AS_CSTRING(args[0]));
}

// * Defines all the native functions
void defineNatives() {
    // Time section
    defineNative("clock", clockNative);

    // Args and value section
    defineNative("argc", argcNative);
    defineNative("argv", argvNative);
    defineNative("stringize", stringizeNative);
    defineNative("integize", integizeNative);

    // Value checking section
    defineNative("isNum", isNumNative);
    defineNative("isBool", isBoolNative);
    defineNative("isObj", isObjNative);
    defineNative("isStr", isStrNative);
    defineNative("isNull", isNullNative);
    defineNative("isInst", isInstanceNative);
    defineNative("isNative", isNativeNative);
    defineNative("isClass", isClassNative);
    defineNative("isBoundMethod", isBoundMethodNative);

    // I/O section
    defineNative("broadcast", broadcastNative);
    defineNative("receive", receiveNative);
    defineNative("system", systemNative);

    // Triginometry section
    defineNative("sin", sinNative);
    defineNative("cos", cosNative);
    defineNative("tan", tanNative);
    defineNative("abs", absNative);
    defineNative("asin", asinNative);
    defineNative("acos", acosNative);
    defineNative("atan", atanNative);
    defineNative("hypot", hypotNative);

    // Math section
    defineNative("sqrt", sqrtNative);
    defineNative("powr", powrNative);
    defineNative("mdls", mdlsNative);

    // Language development kit section
    defineNative("collectGarbage", collectGarbageNative);
    defineNative("runtimeError", runtimeErrorNative);
    defineNative("interpret", interpretNative);
}