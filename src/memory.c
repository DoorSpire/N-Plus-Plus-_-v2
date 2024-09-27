#include <stdlib.h>
#include <string.h>
#include "compiler.h"
#include "memory.h"
#include "vm.h"

#define GC_HEAP_GROW_FACTOR 2
#define SMALL_OBJ_SIZE 64
#define SMALL_OBJ_POOL_SIZE 1024

typedef struct {
    void* blocks[SMALL_OBJ_POOL_SIZE];
    int freeIndex;
} PoolAllocator;

static PoolAllocator smallObjPool;

static void* allocateFromPool(PoolAllocator* pool) {
    if (pool->freeIndex > 0) {
        return pool->blocks[--pool->freeIndex];
    }
    return NULL;
}

static void deallocateToPool(PoolAllocator* pool, void* pointer) {
    if (pool->freeIndex < SMALL_OBJ_POOL_SIZE) {
        pool->blocks[pool->freeIndex++] = pointer;
    }
}

void* reallocate(void* pointer, size_t oldSize, size_t newSize) {
    vm.bytesAllocated += newSize - oldSize;

    if (newSize > oldSize && vm.bytesAllocated > vm.nextGC) {
        collectGarbage();
    }

    if (newSize == 0) {
        if (oldSize <= SMALL_OBJ_SIZE) {
            deallocateToPool(&smallObjPool, pointer);
        } else {
            free(pointer);
        }
        return NULL;
    }

    if (newSize <= SMALL_OBJ_SIZE) {
        void* result = allocateFromPool(&smallObjPool);
        if (result == NULL) {
            result = malloc(SMALL_OBJ_SIZE);
        }
        if (pointer) {
            memcpy(result, pointer, oldSize);
        }
        return result;
    } else {
        void* result = realloc(pointer, newSize);
        if (result == NULL) exit(1);
        return result;
    }
}

static void pushGrayStack(Obj* object) {
    if (vm.grayCapacity < vm.grayCount + 1) {
        vm.grayCapacity = GROW_CAPACITY(vm.grayCapacity);
        vm.grayStack = (Obj**)realloc(vm.grayStack, sizeof(Obj*) * vm.grayCapacity);
        if (vm.grayStack == NULL) exit(1);
    }

    vm.grayStack[vm.grayCount++] = object;
}

// * Sorry, this object has been marked for removal
void markObject(Obj* object) {
    if (object == NULL || object->isMarked) return;
    object->isMarked = true;
    pushGrayStack(object);
}

void markValue(Value value) {
    if (IS_OBJ(value)) markObject(AS_OBJ(value));
}

static void markArray(ValueArray* array) {
    for (int i = 0; i < array->count; i++) {
        markValue(array->values[i]);
    }
}

// ? What does blacken mean
static void blackenObject(register Obj* object) {
    switch (object->type) {
        case OBJ_BOUND_METHOD: {
            ObjBoundMethod* bound = (ObjBoundMethod*)object;
            markValue(bound->receiver);
            markObject((Obj*)bound->method);
            break;
        }
        case OBJ_CLOSURE: {
            ObjClosure* closure = (ObjClosure*)object;
            markObject((Obj*)closure->function);
            for (int i = 0; i < closure->upvalueCount; i++) {
                markObject((Obj*)closure->upvalues[i]);
            }
            break;
        }
        case OBJ_FUNCTION: {
            ObjFunction* function = (ObjFunction*)object;
            markObject((Obj*)function->name);
            markArray(&function->chunk.constants);
            break;
        }
        case OBJ_UPVALUE:
            markValue(((ObjUpvalue*)object)->closed);
            break;
        case OBJ_NATIVE:
        case OBJ_STRING:
            break;
    }
}

// Sorry, just me here.
// I think dumb things are frikkin' cool,
// AND I AM FREEEEEEEEEEEE!!!!
static void freeObject(register Obj* object) {
    switch (object->type) {
        case OBJ_BOUND_METHOD:
            FREE(ObjBoundMethod, object);
            break;
        case OBJ_CLOSURE: {
            ObjClosure* closure = (ObjClosure*)object;
            FREE_ARRAY(ObjUpvalue*, closure->upvalues, closure->upvalueCount);
            FREE(ObjClosure, object);
            break;
        }
        case OBJ_FUNCTION: {
            ObjFunction* function = (ObjFunction*)object;
            freeChunk(&function->chunk);
            FREE(ObjFunction, object);
            break;
        }
        case OBJ_INSTANCE: {
            ObjInstance* instance = (ObjInstance*)object;
            freeTable(&instance->fields);
            FREE(ObjInstance, object);
            break;
        }
        case OBJ_NATIVE:
            FREE(ObjNative, object);
            break;
        case OBJ_STRING: {
            ObjString* string = (ObjString*)object;
            FREE_ARRAY(char, string->chars, string->length + 1);
            FREE(ObjString, object);
            break;
        }
        case OBJ_UPVALUE:
            FREE(ObjUpvalue, object);
            break;
    }
}

static void markRoots() {
    for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
        markValue(*slot);
    }

    for (int i = 0; i < vm.frameCount; i++) {
        markObject((Obj*)vm.frames[i].closure);
    }

    for (ObjUpvalue* upvalue = vm.openUpvalues; upvalue != NULL; upvalue = upvalue->next) {
        markObject((Obj*)upvalue);
    }

    markTable(&vm.globals);
    markCompilerRoots();
    markObject((Obj*)vm.initString);
}

static void traceReferences() {
    while (vm.grayCount > 0) {
        Obj* object = vm.grayStack[--vm.grayCount];
        blackenObject(object);
    }
}

static void sweep() {
    Obj* previous = NULL;
    Obj* object = vm.objects;
    while (object != NULL) {
        if (object->isMarked) {
            object->isMarked = false;
            previous = object;
            object = object->next;
        } else {
            Obj* unreached = object;
            object = object->next;
            if (previous != NULL) {
                previous->next = object;
            } else {
                vm.objects = object;
            }

            freeObject(unreached);
        }
    }
}

// Ur a piece of garbaj!1!!!1!1
void collectGarbage() {
    markRoots();
    traceReferences();
    tableRemoveWhite(&vm.strings);
    sweep();

    vm.nextGC = vm.bytesAllocated * GC_HEAP_GROW_FACTOR;
}

void freeObjects() {
    Obj* object = vm.objects;
    while (object != NULL) {
        Obj* next = object->next;
        freeObject(object);
        object = next;
    }

    free(vm.grayStack);
}