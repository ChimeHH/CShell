/*
FILE: csym.h

Contact with hhao020@gmail.com for bug reporting and supporting.
*/

#ifndef C_SYM_H
#define C_SYM_H

#ifdef __cplusplus
    extern "C" {
#endif

#define CSHELL_INCLUDE 1

enum SymEnum_t{
  SYM_UNDEF,
  SYM_TEXT, //may not availabel, which depends on output of objdump/readelf
  SYM_DATA,
  SYM_FUNC,
};

typedef struct _SYM_ELEMENT_TYPE_
{
  void  *addr;
  char  *name;
  unsigned char type;
  unsigned char module;
  int   size;
} Sym_t;

#define SYM_MAX_NUM 10000

extern Sym_t gSymTable[SYM_MAX_NUM];
extern int   gSymCount;

#ifdef __cplusplus
}
#endif

#endif /*C_SYM_H*/



