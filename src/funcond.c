#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <crow/types.h>
#include <crow/core.h>
#include <crow/funcond.h>

CRO_Value CRO_block (CRO_State *s, int argc, char**argv) {
  int x;
  CRO_Value v;
  CRO_Closure *lastScope, *scope;

  lastScope = s->scope;
  s->scope = CRO_createClosure(s);
#ifdef CROWLANG_SCOPE_DEBUG
  printf("Scope is now %x (upgraded from %x)\n", s->scope, lastScope);
#endif

  scope = s->scope;

  scope->active = 1;
  scope->depends = lastScope;

  for (x = 1; x <= argc; x++) {
    v = CRO_eval(s, argv[x]);
    
    /* If we have an exit code, this code block is OVER */
    if (s->exitCode >= s->exitContext) {
      if (s->exitCode == s->exitContext) {
        s->exitCode = 0;
      }
          
      break;
    }
  }

  scope->active = 0;
  s->scope = lastScope;
#ifdef CROWLANG_SCOPE_DEBUG
  printf("Scope is now %x (downgraded from %x)\n", s->scope, scope);
#endif

  return v;
}

CRO_Value CRO_func (CRO_State *s, int argc, char **argv) {
  CRO_Value v;
  char *args, *body;
  int arglen, x, bsize, bptr, i;
  allotok_t allotok;

  if (argc < 2) {
    printf("Error (func) requires at least 2 arguements\n");
  }

  args = argv[1];

  body = (char*)malloc(CRO_BUFFER_SIZE * sizeof(char));
  bsize = CRO_BUFFER_SIZE;
  bptr = 0;

  /* Do this or some systems may freak out */
  /*memset(body, 0, 512 * sizeof(char));*/
  body[0] = 0;
  
  body[bptr++] = '(';
  
  if (args[0] == '(') {
    int argc = 1;
    arglen = 0;
    for (x = 1; args[x] != 0; x++) {
      if (args[x] == ' ' || args[x] == ')') {
        if (arglen > 0) {
          
        }

        body[bptr++] = ' ';

        arglen++;
        argc++;
      }
      else {

        /* Only collect non space characters because spaces dont matter in
         * arguement definitions */
        if(args[x] > 32)
          body[bptr++] = args[x];
      }
      
      if (bptr >= bsize) {
        bsize *= 2;
        body = realloc(body, bsize * sizeof(char));
      }
    }
    body[--bptr] = ')';
    bptr++;
  }
  
  for (i = 2; i <= argc; i++) {
    for (x = 0; argv[i][x] != 0; x++) {
      body[bptr++] = argv[i][x];
      
      if (bptr >= bsize) {
        bsize *= 2;
        body = realloc(body, bsize * sizeof(char));
      }
    }
    body[bptr++] = ' ';
  }
  body[bptr - 1] = 0;
  
  allotok = CRO_malloc(s, body, free);

  v.type = CRO_LocalFunction;
  v.value.function = NULL;
  v.value.string = body;
  v.allotok = allotok;
  v.flags = CRO_FLAG_NONE;
  v.functionClosure = s->scope;
  return v;

}

CRO_Value CRO_subroutine (CRO_State *s, int argc, char **argv) {
  CRO_Value v;
  char *body;
  int x, bsize, bptr, i;
  allotok_t allotok;

  if (argc < 1) {
    printf("Error (subroutine) requires at least 1 arguement\n");
  }

  body = (char*)malloc(CRO_BUFFER_SIZE * sizeof(char));
  bsize = CRO_BUFFER_SIZE;
  bptr = 0;

  /* Do this or some systems may freak out */
  /*memset(body, 0, 512 * sizeof(char));*/
  body[0] = 0;
  
  body[bptr++] = '(';
  body[bptr++] = ')';
  
  for (i = 1; i <= argc; i++) {
    for (x = 0; argv[i][x] != 0; x++) {
      body[bptr++] = argv[i][x];
      
      if (bptr >= bsize) {
        bsize *= 2;
        body = realloc(body, bsize * sizeof(char));
      }
    }
    body[bptr++] = ' ';
  }
  body[bptr - 1] = 0;
  
  allotok = CRO_malloc(s, body, free);

  v.type = CRO_LocalFunction;
  v.value.function = NULL;
  v.value.string = body;
  v.allotok = allotok;
  v.flags = CRO_FLAG_NONE;
  v.functionClosure = s->scope;

  return v;

}

CRO_Value CRO_defun (CRO_State *s, int argc, char **argv) {
  CRO_Value ret;
  
  if (argc >= 3) {
    hash_t vhash;
    CRO_Variable var;
    unsigned int x;
    char *name;
    CRO_Closure *scope;
    
    vhash = CRO_genHash(argv[1]);
    scope = s->scope;

    /* Swap name out with the name of the function 'func' */
    name = argv[1];
    argv[1] = "func";
    
    ret = CRO_func(s, argc - 1, &argv[1]);
    
    /* Now set it back */
    argv[1] = name;
    
    for (x = 0;x < scope->vptr; x++) {
      if (vhash == scope->variables[x].hash) {
        printf("Error: Variable exists\n");
      }
    }

    var.hash = vhash;
    var.value = ret;
    
    scope->variables[scope->vptr] = var;
    
    scope->vptr++;
    if (scope->vptr >= scope->vsize) {
      scope->vsize *= 2;
      scope->variables = (CRO_Variable*)realloc(scope->variables, scope->vsize * sizeof(CRO_Variable));
      #ifdef CROWLANG_ALLOC_DEBUG
      printf("[Alloc Debug]\t Variables size increased to %d\n", scope->vsize);
      #endif
    }

    return ret;
  }
  else {
    char *err;
    err = malloc(128 * sizeof(char));
    
    sprintf(err, "(%s): Expected 3 arguements (%d given)", argv[0], argc);
    ret = CRO_error(s, err);
    
    return ret;
  }

  CRO_toNone(ret);
  return ret;
}

CRO_Value CRO_andand (CRO_State *s, int argc, CRO_Value *argv) {
  CRO_Value ret;
  
  if (argc >= 2) {
    int i;
    
    for (i = 1; i <= argc; i++) {
      if (argv[i].type != CRO_Bool) {
        char *err;
        err = malloc(128 * sizeof(char));
        
        sprintf(err, "(%s): Argument %d is not an boolean", argv[0].value.string, i);
        ret = CRO_error(s, err);
        
        return ret;
      }
      else if (!(argv[i].value.integer)) {
        CRO_toBoolean(ret, 0);
        return ret;
      }
    }
    
    CRO_toBoolean(ret, 1);
  }
  else {
    char *err;
    err = malloc(128 * sizeof(char));
    
    sprintf(err, "(%s): 2 or more arguements expected", argv[0].value.string);
    ret = CRO_error(s, err);
    
  }
  return ret;
}

CRO_Value CRO_oror (CRO_State *s, int argc, CRO_Value *argv) {
  CRO_Value ret;
  
  if (argc >= 2) {
    int i;
    
    for (i = 1; i <= argc; i++) {
      
      if (argv[i].type != CRO_Bool) {
        char *err;
        err = malloc(128 * sizeof(char));
        
        sprintf(err, "(%s): Argument %d is not an boolean", argv[0].value.string, i);
        ret = CRO_error(s, err);
        
        return ret;
      }
      else if (argv[i].value.integer) {
        CRO_toBoolean(ret, 1);
        return ret;
      }
    }
    
    CRO_toBoolean(ret, 0);
  }
  else {
    char *err;
    err = malloc(128 * sizeof(char));
    
    sprintf(err, "(%s): 2 or more arguements expected", argv[0].value.string);
    ret = CRO_error(s, err);
    
  }
  return ret;
}

CRO_Value CRO_equals (CRO_State *s, int argc, CRO_Value *argv) {
  /* TODO: Make this work for any number of operands */
  CRO_Value a;
  CRO_Value b;
  CRO_Value ret;
  
  
  a = argv[1];
  b = argv[2];

  if (a.type == b.type) {
    if (a.type == CRO_Number) {
      if (a.value.number == b.value.number) {
        CRO_toBoolean(ret, 1);
      }
      else {
        CRO_toBoolean(ret, 0);
      }
    }
    else if (a.type == CRO_String) {
      if (strcmp(a.value.string, b.value.string) == 0) {
        CRO_toBoolean(ret, 1);
      }
      else {
        CRO_toBoolean(ret, 0);
      }
    }
    else {
      printf("Error: cannot do operation with that\n");
      CRO_toNone(ret);
    }
  }
  else {
    printf("Error: A is not the same type as B\n");
    CRO_toNone(ret);
  }

  return ret;
}

CRO_Value CRO_notEquals (CRO_State *s, int argc, CRO_Value *argv) {
  CRO_Value r;

  r = CRO_equals(s, argc, argv);
  r.value.integer = !r.value.integer;

  return r;
}

CRO_Value CRO_greaterThan (CRO_State *s, int argc, CRO_Value *argv) {
  CRO_Value a, b, ret;

  a = argv[1];
  b = argv[2];

  if (a.type == b.type) {
    if (a.type == CRO_Number) {
      if (a.value.number > b.value.number) {
        CRO_toBoolean(ret, 1);
      }
      else {
        CRO_toBoolean(ret, 0);
      }
    }
    else {
      printf("Error: cannot do operation with that\n");
      CRO_toNone(ret);
    }
  }
  else {
    printf("Error: A is not the same type as B\n");
    CRO_toNone(ret);
  }

  return ret;
}

CRO_Value CRO_lessThan (CRO_State *s, int argc, CRO_Value *argv) {
  CRO_Value a, b, ret;

  a = argv[1];
  b = argv[2];

  if (a.type == b.type) {
    if (a.type == CRO_Number) {
      if (a.value.number < b.value.number) {
        CRO_toBoolean(ret, 1);
      }
      else {
        CRO_toBoolean(ret, 0);
      }
    }
    else {
      printf("Error: cannot do operation with that\n");
      CRO_toNone(ret);
    }
  }
  else {
    printf("Error: A is not the same type as B\n");
    CRO_toNone(ret);
  }
  
  

  return ret;
}

CRO_Value CRO_defined (CRO_State *s, int argc, char **argv) {
  CRO_Value a, ret;
  
  if (argc == 1) {
    a = CRO_innerEval(s, argv[1]);
    if (s->exitCode == CRO_ErrorCode) {
      s->exitCode = CRO_None;
    }
    if (a.type != CRO_Undefined) {
      CRO_toBoolean(ret, 1);
    }
    else {
      CRO_toBoolean(ret, 0);
    }
  }
  else {
    CRO_toNone(ret);
    /* Error not enough args */
  }
  

  return ret;
}

CRO_Value CRO_if (CRO_State *s, int argc, char **argv) {
  int x;
  CRO_Value v;

  if (argc < 2) {
    printf("Error");
  }

  for (x = 1; x <= argc; x+=2) {
    /* If x is equal to argc, we are in the else statement (which is optional) */
    if (x == argc) {
      v = CRO_innerEval(s, argv[x]);
      return v;
    }
    /* Otherwise the first word is a conditional, and the second is the body */
    else {
      v = CRO_innerEval(s, argv[x]);
      if (v.type == CRO_Bool) {
        if (v.value.integer) {
          v = CRO_innerEval(s, argv[x + 1]);
          return v;
        }
      }
      else {
        printf("Error: not boolean\n");
      }
    }
  }
  CRO_toNone(v);
  return v;
}

CRO_Value CRO_not (CRO_State *s, int argc, CRO_Value *argv) {
  CRO_Value ret;
  if (argc == 1) {
    CRO_Value bolexpr = argv[1];
    if (bolexpr.type == CRO_Bool) {
      bolexpr.value.integer = !bolexpr.value.integer;
      return bolexpr;
    }
    else {
      char *err;
      err = malloc(128 * sizeof(char));
      
      sprintf(err, "(%s): Argument 1 is not a bool", argv[0].value.string);
      ret = CRO_error(s, err);
      
    }
  }
  else {
    char *err;
    err = malloc(128 * sizeof(char));
    
    sprintf(err, "(%s): Expected 1 arguement. (%d given)", argv[0].value.string, argc);
    ret = CRO_error(s, err);
    
  }
  return ret;
}

/* For reference with these next loops, the exit context is reset to whatever 
 * would be the MINIMUM to break the loop, for example, most of these use 
 * CRO_BreakCode, or the (break) statement.  If the context is equal to the code
 * the code is reset and the previous context is restored, however if the code 
 * is higher, for example in a loop a (return) is encountered, then the exit 
 * code is preserverd */

CRO_Value CRO_each (CRO_State *s, int argc, CRO_Value *argv) {
  CRO_Value array, ret;
  char lastExitContext;
  
  lastExitContext = s->exitContext;
  s->exitContext = CRO_BreakCode;
  
  if (argc == 2) {
    array = argv[1];
    
    if (array.type == CRO_Array) {
      CRO_Value itemV, func, *argz;
      long index;
      
      func = argv[2];
      
      argz = (CRO_Value*)malloc(2 * sizeof(CRO_Value));
      CRO_toNone(argz[0]);
      
      if (func.type == CRO_Function || func.type == CRO_LocalFunction) {

        /* Toggle on that this memory is in use and shouldn't be free'd by the GC*/
        CRO_toggleMemoryUse(s, array);

        /* Make sure the function also doesn't get unloaded, this GC really sucks */
        if (func.type == CRO_LocalFunction)
          CRO_toggleMemoryUse(s, func);

        for (index = 0; index < array.arraySize; index++) {
          /* Get the value of the item in the array and set it to the var */
          itemV = array.value.array[index];
          
          argz[1] = itemV;
          
          CRO_callGC(s);
          ret = CRO_callFunction(s, func, 1, argz);
          
          if (s->exitCode >= s->exitContext) {
            if (s->exitCode == s->exitContext) {
              s->exitCode = 0;
            }
            
            break;
          }
        }

        /* Now we can free it if it isn't connected to anything*/
        CRO_toggleMemoryUse(s, array);

        /* We can undo this too */
        if (func.type == CRO_LocalFunction)
          CRO_toggleMemoryUse(s, func);

        free(argz);
      }
      else if (func.type == CRO_PrimitiveFunction) {
        char *err;
        
        free(argz);
        
        err = malloc(128 * sizeof(char));
        
        sprintf(err, "(%s): Argument 2 function cannot be used here", argv[0].value.string);
        ret = CRO_error(s, err);
      }
      else {
        char *err;
        
        free(argz);
        
        err = malloc(128 * sizeof(char));
        
        sprintf(err, "(%s): Argument 2 is not a function", argv[0].value.string);
        ret = CRO_error(s, err);
      }
    }
    
    else {
      char *err;
      err = malloc(128 * sizeof(char));
      
      sprintf(err, "(%s): Argument 1 is not an Array", argv[0].value.string);
      ret = CRO_error(s, err);
      
    }
  }
  else {
    char *err;
    err = malloc(128 * sizeof(char));
    
    sprintf(err, "(%s): Expected 3 arguements. (%d given)", argv[0].value.string, argc);
    ret = CRO_error(s, err);
    
  }
  s->exitContext = lastExitContext;
  
  return ret;
}

CRO_Value CRO_eachWithIterator (CRO_State *s, int argc, CRO_Value *argv) {
  CRO_Value array;
  CRO_Value ret;
  
  char lastExitContext;
  
  lastExitContext = s->exitContext;
  s->exitContext = CRO_BreakCode;
  
  if (argc == 2) {
    array = argv[1];
    
    if (array.type == CRO_Array) {
      CRO_Value func, item, counter, *callArgs;
      int index;
      
      callArgs = (CRO_Value*)malloc(3 * sizeof(CRO_Value));
      func = argv[2];
      
      CRO_toNone(callArgs[0])
      if (func.type == CRO_Function || func.type == CRO_LocalFunction) {

        CRO_toggleMemoryUse(s, array);

        /* Make sure the function also doesn't get unloaded, this GC really sucks */
        if (func.type == CRO_LocalFunction)
          CRO_toggleMemoryUse(s, func);

        for (index = 0; index < array.arraySize; index++) {
          
          CRO_toNumber(counter, index);
          
          /* Get the value of the item in the array and set it to the var */
          item = array.value.array[index];
          
          callArgs[1] = item;
          callArgs[2] = counter;
          
          CRO_callGC(s);

          /* Now execute it with the variable in place */
          ret = CRO_callFunction(s, func, 2, callArgs);
          
          if (s->exitCode >= s->exitContext) {
            if (s->exitCode == s->exitContext) {
              s->exitCode = 0;
            }
            
            break;
          }
          
        }
        
        CRO_toggleMemoryUse(s, array);
        if (func.type == CRO_LocalFunction)
          CRO_toggleMemoryUse(s, func);

      }
      else if (func.type == CRO_PrimitiveFunction) {
        char *err;
        err = malloc(128 * sizeof(char));
        
        sprintf(err, "(%s): Argument 2 function cannot be used here", argv[0].value.string);
        ret = CRO_error(s, err);
      }
      else {
        char *err;
        err = malloc(128 * sizeof(char));
        
        sprintf(err, "(%s): Argument 2 is not a function", argv[0].value.string);
        ret = CRO_error(s, err);
      }
    }
    else {
      char *err;
      err = malloc(128 * sizeof(char));
      
      sprintf(err, "(%s): Argument 1 is not an Array.", argv[0].value.string);
      ret = CRO_error(s, err);
      
    }
  }
  else {
    char *err;
    err = malloc(128 * sizeof(char));
    
    sprintf(err, "(%s): Expected 3 arguements. (%d given)", argv[0].value.string, argc);
    ret = CRO_error(s, err);
    
  }
  s->exitContext = lastExitContext;

  return ret;
}

CRO_Value CRO_while (CRO_State *s, int argc, char **argv) {
  CRO_Value cond, ret;
  char lastExitContext;
  
  lastExitContext = s->exitContext;
  s->exitContext = CRO_BreakCode;
  
  if (argc == 2) {
    cond = CRO_innerEval(s, argv[1]);
    
    while (cond.type == CRO_Bool && cond.value.integer) {
      ret = CRO_innerEval(s, argv[2]);
      
      if (s->exitCode >= s->exitContext) {
        if (s->exitCode == s->exitContext) {
          s->exitCode = 0;
        }
        
        break;
      }

      CRO_callGC(s);
      cond = CRO_innerEval(s, argv[1]);
    }
  }
  else {
    char *err;
    err = malloc(128 * sizeof(char));
    
    sprintf(err, "(%s): Expected 3 arguements. (%d given)", argv[0], argc);
    ret = CRO_error(s, err);
    
  }
  
  s->exitContext = lastExitContext;
  return ret;
}

CRO_Value CRO_doWhile (CRO_State *s, int argc, char **argv) {
  CRO_Value cond, ret;
  char lastExitContext;
  
  lastExitContext = s->exitContext;
  s->exitContext = CRO_BreakCode;
  
  if (argc == 2) {
    
    
    do {
      
      ret = CRO_innerEval(s, argv[1]);
      
      
      if (s->exitCode >= s->exitContext) {
        if (s->exitCode == s->exitContext) {
          s->exitCode = 0;
        }
        
        break;
      }
      CRO_callGC(s);
      
      cond = CRO_innerEval(s, argv[2]);
    } while(cond.type == CRO_Bool && cond.value.integer);
  }
  else {
    char *err;
    err = malloc(128 * sizeof(char));
    
    sprintf(err, "(%s): Expected 3 arguements. (%d given)", argv[0], argc);
    ret = CRO_error(s, err);
    
  }
  
  s->exitContext = lastExitContext;
  return ret;
}

CRO_Value CRO_loop (CRO_State *s, int argc, char **argv) {
  CRO_Value ret;
  char lastExitContext;
  
  lastExitContext = s->exitContext;
  s->exitContext = CRO_BreakCode;
  
  while (1) {
    
    ret = CRO_innerEval(s, argv[1]);
    
    if (s->exitCode >= s->exitContext) {
      if (s->exitCode == s->exitContext) {
        s->exitCode = 0;
      }
      break;
    }
    
    
    CRO_callGC(s);
  }
  
  s->exitContext = lastExitContext;
  return ret;
}

CRO_Value CRO_doTimes (CRO_State *s, int argc, CRO_Value *argv) {
  CRO_Value v;

  if (argc == 2) {
    CRO_Value func, times;

    func = argv[1];
    times = argv[2];

    if (func.type == CRO_Function || func.type == CRO_LocalFunction) {
      if (times.type == CRO_Number) {
        int i, timesToCall;

        timesToCall = times.value.number;

        for (i = 0; i < timesToCall; i++)
          v = CRO_callFunction(s, func, 0, NULL);
      }
      else {
        CRO_toNone(v);
        /* Error */
      }
    }
    else {
      CRO_toNone(v);
      /* TODO: Error */
    }
  }
  else {
    char *err;
    err = malloc(128 * sizeof(char));

    sprintf(err, "[%s] Expected 2 arguements. (%d given)", argv[0].value.string, argc);
    v = CRO_error(s, err);
    return v;
  }

  return v;
}

CRO_Value CRO_break (CRO_State *s, int argc, CRO_Value *argv) {
  CRO_Value v;
  if (argc == 1) {
    v = argv[1];
  }
  else {
    CRO_toNone(v);
  }
  
  s->exitCode = CRO_BreakCode;
  
  return v;
}

CRO_Value CRO_return (CRO_State *s, int argc, CRO_Value *argv) {
  CRO_Value v;
  if (argc == 1) {
    v = argv[1];
  }
  else {
    CRO_toNone(v);
  }
  
  s->exitCode = CRO_ReturnCode;
  
  return v;
}

CRO_Value CRO_exit (CRO_State *s, int argc, CRO_Value *argv) {
  CRO_Value v;
  if (argc == 1) {
    v = argv[1];
  }
  else {
    CRO_toNone(v);
  }
  
  s->exitCode = CRO_ExitCode;
  
  return v;
}
