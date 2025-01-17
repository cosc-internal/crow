#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

#include <crow/types.h>
#include <crow/core.h>
#include <crow/data.h>

/* Define a variable */
CRO_Value defVar (CRO_State *s, int argc, char **argv) {
  unsigned int x = 0;
  char *name, *value;
  hash_t vhash;
  CRO_Variable var;
  CRO_Value vn;
  CRO_Closure *scope;

  if (argc < 1) {
    /*CRO_error("defVar, not enough arguements given");*/
  }
  if (argc == 1) {
    name = argv[1];
    CRO_toNone(vn);
  }
  else {
    name = argv[1];
    value = argv[2];
    vn = CRO_innerEval(s, value);
  }
  scope = s->scope;
  vhash = CRO_genHash(name);
  
  for (;x < scope->vptr; x++) {
    if (vhash == scope->variables[x].hash) {
      printf("Error: Variable exists\n");
    }
  }

  var.hash = vhash;
  var.value = vn;
  
#ifdef CROWLANG_VAR_DEBUG
  printf("Defined variable %ld in closure %x\n", vhash, s->scope);
#endif
  
  scope->variables[scope->vptr] = var;
  
  scope->vptr++;
  if (scope->vptr >= scope->vsize) {
    scope->vsize *= 2;
    scope->variables = (CRO_Variable*)realloc(scope->variables, scope->vsize * sizeof(CRO_Variable));
    #ifdef CROWLANG_ALLOC_DEBUG
    printf("[Alloc Debug]\t Variables size increased to %d\n", scope->vsize);
    #endif
  }

  return var.value;
}

/* Set a defined variable to a value */
CRO_Value set (CRO_State *s, int argc, char **argv) {
  unsigned int x;
  char *name, *value;
  hash_t vhash;
  CRO_Value vn;
  CRO_Closure *scope;

  if (argc == 2) {
    name = argv[1];
    value = argv[2];

    vhash = CRO_genHash(name);
    scope = s->scope;
    
    vn = CRO_innerEval(s, value);

    do {
      for (x = 0; x < scope->vptr; x++) {
        if (vhash == scope->variables[x].hash) {
          if (!(scope->variables[x].value.flags & CRO_FLAG_CONSTANT)) {
            scope->variables[x].value = vn;
            return vn;
          }
          else {
            CRO_error(s, "Attempting to overwrite constant variable");
          }
        }
      }
      
      scope = scope->depends;
    } while (scope != NULL);
    /*CRO_error("Could not find variable");*/
    CRO_toNone(vn);
    return vn;
  }
  /* Error: not enough args */
  CRO_toNone(vn);
  return vn;
}

CRO_Value CRO_const (CRO_State *s, int argc, CRO_Value *argv) {
  CRO_Value v;

  if (argc != 1) {
    printf("Error\n");
  }

  v = argv[1];

  v.flags |= CRO_FLAG_CONSTANT;

  return v;
}

/* Arrays */

CRO_Value CRO_array (CRO_State *s, int argc, CRO_Value *argv) {
  CRO_Value v, *array;
  int x;
  allotok_t tok;

  array = (CRO_Value*)malloc(argc * sizeof(CRO_Value));

  for (x = 0; x < argc; x++) {
    array[x] = argv[x + 1];
  }

  tok = CRO_malloc(s, (void*)array, free);

  v.type = CRO_Array;
  v.value.array = array;
  v.arraySize = argc;
  v.allotok = tok;
  v.flags = CRO_FLAG_SEARCH;

  return v;
}

CRO_Value CRO_length (CRO_State *s, int argc, CRO_Value *argv) {
  CRO_Value v, ret;
  
  if (argc == 1) {
    v = argv[1];
    if (v.type == CRO_String) {
      
      /* Since UTF8 isnt supported in strlen, we have our own strlen
       * implementation.  We also have the classic implementation in case a 
       * system is so old, it has no chance of using UTF8 */
      #ifdef CROWLANG_PEDANTIC_UTF8
        int i, size;
        unsigned char *str;
        
        str = (unsigned char*)v.value.string;
        i = 0;
        size = 0;
        
        while (str[i] != 0) {
          /* This is made according to the byte size of the character based on
           * the first byte of it.
           * https://en.wikipedia.org/wiki/UTF-8 */
          if (str[i] >= 240) {
            i += 4;
          }
          else if (str[i] >= 224) {
            i += 3;
          }
          else if (str[i] >= 192) {
            i += 2;
          }
          else {
            i += 1;
          }
          
          /* Increase the size by 1 as we read one "character" */
          size++;
        }
        
        CRO_toNumber(ret, size);
      #else
        /* This is ALL we _should_ need, but NO, can't be bothered to make
         * support for UTF8 without external libraries */
        CRO_toNumber(ret, strlen(v.value.string));
      #endif
    }
    else if (v.type == CRO_Array) {
      CRO_toNumber(ret, v.arraySize);
    }
    else {
      CRO_toNone(ret);
    }
  }
  else {
    CRO_toNone(ret);
  }
  
  return ret;
}

CRO_Value CRO_makeArray (CRO_State *s, int argc, CRO_Value *argv) {
  CRO_Value v, nil, *array;
  int size, x;
  allotok_t tok;

  if (argc < 1) {
    size = 1;
  }
  else {
    CRO_Value sz;

    sz = argv[1];

    if (sz.type != CRO_Number) {
      printf("Error");
    }
    size = (int)sz.value.number;
  }
  #ifdef CROWLANG_GREEDY_MEMORY_ALLOCATION
  {
    int cap;
    
    for(cap = CROWLANG_BUFFER_SIZE; cap < size; cap *= 2);
    
    array = (CRO_Value*)calloc(cap, sizeof(CRO_Value));
    v.arrayCapacity = cap;
  }
  #else
  array = (CRO_Value*)calloc(size, sizeof(CRO_Value));
  #endif
  
  /* Make sure every element of the array is initiated to undefined */
  CRO_toNone(nil);
  
  for (x = 0; x < size; x++) {
    array[x] = nil;
  }

  tok = CRO_malloc(s, (void*)array, free);
  
  v.type = CRO_Array;
  v.value.array = array;
  v.arraySize = size;
  v.allotok = tok;
  v.flags = CRO_FLAG_SEARCH;

  return v;
}

CRO_Value CRO_resizeArray (CRO_State *s, int argc, CRO_Value *argv) {
  CRO_Value size, array, v, *newArray;
  int sz;
  allotok_t tok;

  if (argc == 2) {
    int cpySize;
    
    array = argv[1];
    size = argv[2];
    
    if (array.type != CRO_Array) {
      /* Error */
    }
    else if (size.type != CRO_Number) {
      /* Error */
    }
    
    sz = (int)size.value.number;
    newArray = (CRO_Value*)calloc(sz, sizeof(CRO_Value));
    
    if (sz > array.arraySize) {
      cpySize = array.arraySize * sizeof(CRO_Value);
    }
    else {
      cpySize = sz * sizeof(CRO_Value);
    }
    
    newArray = memcpy(newArray, array.value.array, cpySize);
    
    tok = CRO_malloc(s, (void*)newArray, free);

    v.type = CRO_Array;
    v.value.array = newArray;
    v.arraySize = sz;
    v.allotok = tok;
    v.flags = CRO_FLAG_SEARCH;

    return v;
  }
  else {
    char *err;
    err = malloc(128 * sizeof(char));
    
    sprintf(err, "[%s] Expected 3 arguements. (%d given)", argv[0].value.string, argc);
    v = CRO_error(s, err);
    return v;
    
  }
  
}

CRO_Value CRO_arraySet (CRO_State *s, int argc, CRO_Value *argv) {
  CRO_Value arr, arg, val;
  int index;

  if (argc != 3) {
    printf("Arguement Error\n");
  }

  arr = argv[1];
  arg = argv[2];
  val = argv[3];

  if (arr.type != CRO_Array) {
    printf("Array type Error\n");
  }

  if (arg.type != CRO_Number) {
    printf("Arg type Error\n");
  }

  index = (int)arg.value.number;
  
  /* TODO: Expand the array to fit the new size */
  if (index >= arr.arraySize) {
    printf("Error\n");
  }

  /* Make sure we are not trying to overwrite a constant value */
  if (!(arr.value.array[index].flags & CRO_FLAG_CONSTANT)) {
    arr.value.array[index] = val;
  }
  else {
    printf("Error\n");
  }
  return val;

}

CRO_Value CRO_arrayGet (CRO_State *s, int argc, CRO_Value *argv) {
  CRO_Value arr, arg, ret;
  int index;

  if (argc != 2) {
    printf("Error\n");
  }

  arr = argv[1];
  arg = argv[2];

  if (arr.type != CRO_Array) {
    printf("Error\n");
  }

  if (arg.type != CRO_Number) {
    printf("Error\n");
  }

  index = (int)arg.value.number;

  if (index > arr.arraySize) {
    CRO_toNone(ret);
    return ret;
  }

  ret = arr.value.array[index];
  return ret;
}

CRO_Value CRO_sample (CRO_State *s, int argc, CRO_Value *argv) {
  
  if (argc == 1) {
    CRO_Value array;
    array = argv[1];
    
    if (array.type == CRO_Array) {
      int index;
      
      /* TODO: Make this work for indexes larger than RANDMAX */
      index = rand() % array.arraySize;
      return array.value.array[index];
    }
    else {
      CRO_Value ret;
      char *err;
      err = malloc(128 * sizeof(char));
      
      sprintf(err, "[%s] Argument 1 is not an Array", argv[0].value.string);
      ret = CRO_error(s, err);
      return ret;
    }
  }
  else {
    CRO_Value ret;
    char *err;
    err = malloc(128 * sizeof(char));
    
    sprintf(err, "[%s] Expected 1 arguement. (%d given)", argv[0].value.string, argc);
    ret = CRO_error(s, err);
    return ret;
  }
}

/* Structures */

CRO_Value CRO_makeStruct (CRO_State *s, int argc, CRO_Value *argv) {
  CRO_Value v, *str;
  int x;
  
  /* Structures are just arrays, but every even element is a string containing 
   * the name, and the odd containing the value */
  
  if (argc < 1) {
    printf("makestruct Error\n");
  }

  str = (CRO_Value*)malloc(((argc + 1) * 2) * sizeof(CRO_Value));
  memset(str, 0, (((argc + 1) * 2) * sizeof(CRO_Value)));

  for (x = 0; x < argc; x++) {
    str[x * 2] = argv[x + 1];
    str[x * 2].flags = CRO_FLAG_CONSTANT;
  }

  v.allotok = CRO_malloc(s, str, free);

  v.type = CRO_Struct;
  v.value.array = str;
  v.arraySize = (argc) * 2;
  v.flags = CRO_FLAG_SEARCH;

  return v;
  
}

CRO_Value CRO_setStruct (CRO_State *s, int argc, CRO_Value *argv) {
  CRO_Value str, name, v;
  int x;
  
  if (argc != 3) {
    printf("Error\n");
  }

  str = argv[1];
  name = argv[2];
  v = argv[3];

  for (x = 0; x < str.arraySize; x+= 2) {
    
    if (strcmp(name.value.string, str.value.array[x].value.string) == 0) {
      str.value.array[x + 1] = v;
      return v;
    }
  }
  CRO_toNone(v);
  printf("Cound not find value %s in structure\n", argv[2].value.string);
  return v;
}

CRO_Value CRO_getStruct (CRO_State *s, int argc, CRO_Value *argv) {
  CRO_Value str, name, v;
  int x;
  
  if (argc != 2) {
    printf("Error\n");
  }

  str = argv[1];
  name = argv[2];

  for (x = 0; x < str.arraySize; x+= 2) {
    if (strcmp(name.value.string, str.value.array[x].value.string) == 0) {
      return str.value.array[x + 1];
    }
  }
  CRO_toNone(v);
  printf("Cound not find\n");
  return v;
}

CRO_Value CRO_number (CRO_State *s, int argc, CRO_Value *argv) {
  CRO_Value ret;
  
  if (argc == 1) {
    CRO_Value value;
    value = argv[1];
    
    if (value.type == CRO_String) {
      double out;
      char *valuePtr;
      
      valuePtr = NULL;
      
      out = strtod(value.value.string, &valuePtr);
      CRO_toNumber(ret, out);
      
    }
    else if (value.type == CRO_Bool) {
      CRO_toNumber(ret, value.value.integer);
    }
    else if (value.type == CRO_Number) {
      /* Even though we could just return the number, we do this here to remove const
       qualifiers which may or may not exist on that value */
      CRO_toNumber(ret, value.value.number);
    }
    else {
      char *err;
      err = malloc(128 * sizeof(char));
      
      sprintf(err, "(%s): Invalid conversion", argv[0].value.string);
      ret = CRO_error(s, err);
    }
  }
  else {
    char *err;
    err = malloc(128 * sizeof(char));
    
    sprintf(err, "(%s): Expected 1 arguement (%d given)", argv[0].value.string, argc);
    ret = CRO_error(s, err);
  }
  return ret;
}

CRO_Value CRO_hash (CRO_State *s, int argc, CRO_Value *argv) {
  CRO_Value ret;
  
  if (argc == 1) {
    CRO_Value arg;
    arg = argv[1];
    
    if (arg.type == CRO_String) {
      CRO_toNumber(ret, CRO_genHash(arg.value.string));
      return ret;
    }
    else {
      char *err;
      err = malloc(128 * sizeof(char));
      
      sprintf(err, "(%s): Argument 1 is not a string", argv[0].value.string);
      ret = CRO_error(s, err);
    }
  }
  else {
    char *err;
    err = malloc(128 * sizeof(char));
    
    sprintf(err, "(%s): Expected 1 arguement (%d given)", argv[0].value.string, argc);
    ret = CRO_error(s, err);
  }
  return ret;
}

CRO_Value CRO_currentScope (CRO_State *s, int argc, char **argv) {
  CRO_Value v;

  if (argc == 1) {
    CRO_Closure *scope;
    hash_t vhash;
    int x;

    vhash = CRO_genHash(argv[1]);
    scope = s->scope;

    /* Go through every variable in ONLY our scope */
    for (x = scope->vptr - 1; x >= 0; x--) {
      if (vhash == scope->variables[x].hash) {
        return scope->variables[x].value;
      }
    }

    {
    char *errorMsg = malloc(64 * sizeof(char));
    sprintf(errorMsg, "Variable '%s' is not defined in the current scope", argv[1]);
    v = CRO_error(s, errorMsg);
    return v;
    }
  }
  else {
    char *err;
    err = malloc(128 * sizeof(char));

    sprintf(err, "Expected 1 arguement (%d given)", argc);
    v = CRO_error(s, err);
  }
  
  return v;
}
