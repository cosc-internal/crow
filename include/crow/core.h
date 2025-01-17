#ifndef CRO_CORE_h
#define CRO_CORE_h

#include <stdio.h>

extern int running;
extern char *errorMsg;

extern void (*CRO_printValue[64])(CRO_Value);

CRO_State *CRO_createState(void);
void CRO_exposeStandardFunctions(CRO_State *s);
void CRO_exposeArguments (CRO_State *s, int argc, char **argv);
void CRO_freeState(CRO_State *s);

CRO_Closure *CRO_createClosure (CRO_State *s);

char* CRO_printStd(CRO_Value v);

CRO_Value CRO_callFunction(CRO_State *s, CRO_Value func, int argc, CRO_Value *argv);

hash_t CRO_genHash(const char *name);
void CRO_exposeFunction(CRO_State *s, const char *name, CRO_Value (*func)(CRO_State *s, int argc, CRO_Value *argv));
void CRO_exposePrimitiveFunction(CRO_State *s, const char *name, CRO_Value (*func)(CRO_State *s, int argc, char **argv));
void CRO_exposeVariable(CRO_State *s, const char *name, CRO_Value v);

char *CRO_cloneStr(const char *str);

allotok_t CRO_malloc(CRO_State *s, void *memory, CRO_FreeData_Function *free);
void CRO_toggleMemoryUse (CRO_State *s, CRO_Value v);
void CRO_GC(CRO_State *s);

void CRO_freeFile (void* v);

CRO_Value CRO_error(CRO_State *s, const char *msg);
void CRO_printError();

CRO_Value CRO_innerEval(CRO_State *s, char *src);
CRO_Value CRO_eval(CRO_State *s, char *src);
CRO_Value CRO_evalFile(CRO_State *s, FILE *src);

#endif
