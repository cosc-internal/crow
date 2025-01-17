#ifndef CRO_STRING_h
#define CRO_STRING_h

CRO_Value CRO_string(CRO_State *s, int argc, CRO_Value *argv);
CRO_Value CRO_strInsert(CRO_State *s, int argc, CRO_Value *argv);
CRO_Value CRO_charAt(CRO_State *s, int argc, CRO_Value *argv);
CRO_Value CRO_substr(CRO_State *s, int argc, CRO_Value *argv);
CRO_Value CRO_split(CRO_State *s, int argc, CRO_Value *argv);
CRO_Value CRO_startsWith(CRO_State *s, int argc, CRO_Value *argv);

#endif
