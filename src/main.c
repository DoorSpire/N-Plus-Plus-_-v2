#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "chunk.h"
#include "vm.h"

static void repl() {
    char line[1024];
    for (;;) {
        printf("> ");

        if (!fgets(line, sizeof(line), stdin)) {
            printf("\n");
            break;
        }

        interpret(line);
    }
}

static void runMain(const char* path) {
    if (!hasSuffix(path, ".npp")) {
        fprintf(stderr, "Error: The file \"%s\" does not have the required \".npp\" extension.\n", path);
        exit(74);
    }

    char* source = readFile(path);
    InterpretResult result = interpret(source);
    free(source);

    if (result == INTERPRET_COMPILE_ERROR) exit(65);
    if (result == INTERPRET_RUNTIME_ERROR) exit(70);
}

int main(int argc, const char* argv[]) {
    initVM();
    const char* suffix = ".npp";

    if (argc == 2 && strcmp(argv[1], "help") == 0) {
        printf("Usage: nppc2 [main_file] // [args...]\n");
        exit(64);
    } else if (argc == 1) {
        repl();
    } else if (argc == 2 && hasSuffix(argv[1], suffix)) {
        runMain(argv[1]);
    } else if (argc >= 3 && hasSuffix(argv[1], suffix) && strcmp(argv[2], "//") == 0) {
        int argsCount = argc - 3;
        const char** args = &argv[3];
        
        init(args, argsCount);
        runMain(argv[1]);
    }

    freeVM();
    return 0;
}