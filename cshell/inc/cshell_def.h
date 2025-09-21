/*
FILE: cshell_def.h

Contact with hhao020@gmail.com for bug reporting and supporting.
*/

#include "csym.h"
#include "cshell.h"


#ifndef C_SHELL_DEF_H
#define C_SHELL_DEF_H

#ifdef __cplusplus
    extern "C" {
#endif

typedef struct ast_s {
  int   type;
  char  *name;
  int   size;
  void  *sp;
  union{
  long   value;
  void  *addr;
  char   cval;
  double dval;
  long   lval;
  char  *sval;
  };
  struct ast_s *l;
  struct ast_s *r;
} ast_t;


/* interface to the lexer */
extern int yylineno; /* from lexer */
extern void yyerror(char *s, ...);

#define cshellError(fmt, arg...)  do{ \
  printf("CSHELL ERROR %s:%d, ", __FUNCTION__, __LINE__); \
  printf(fmt, ##arg); \
  }while(0);

extern int debug_shell;
#define DEBUG_TTY_INPUT() (debug_shell & 0x1) 
#define DEBUG_DUMP_TREE() (debug_shell & 0x2)
#define DEBUG_DUMP_EVAL() (debug_shell & 0x4)
#define DEBUG_DUMP_EVALSTACK() (debug_shell & 0x8)
#define DEBUG_NO_EVAL()   (debug_shell & 0x10)


extern void shellhelp();

extern void dumpast(ast_t *a, int level);
extern int dumpstringsymbol(char *symbol);
extern int dumpplainsymbol(char *symbol);

extern ast_t *newopcode(int opcode);
extern ast_t *newvalue(int type, char *string, char *format);
extern ast_t *newsymbol(char *symbol);
extern ast_t *newcondition(ast_t *c, ast_t *e1, ast_t *e2);
extern void newdef(ast_t *def, ast_t *args, ast_t *list);

extern int eval_symbol(ast_t *tree);
extern int eval_value(ast_t *tree);
extern int update_value(ast_t *tree);
extern int eval_assign(ast_t *tree);
extern int eval_unary(ast_t *tree);
extern int eval_bool(ast_t *tree);
extern int eval_op(ast_t *tree);
extern int eval_funcall(ast_t *tree);
extern int eval_tree(ast_t *tree);
extern int eval_list(ast_t *tree);

extern ast_t *newast(int type, ast_t *l, ast_t *r);
extern void freeast(ast_t *a);
extern void freetree(ast_t *tree);

extern void yyerror(char *s, ...);

extern void listsymbol();
extern void lookupsymbol(char* symbol);
extern ast_t *findsymbol(char* symbol);
extern int addsymbol(ast_t* sym);



extern int loadscript(const char*);

#ifdef __cplusplus
}
#endif

#endif /*C_SHELL_DEF_H*/

