/*----------------------------------------------------------
File Name  : xxx.c/.h
Description:
Author     : hhao020@gmail.com (bug fixing and consulting)
Date       : 2007-05-15
------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include "cshell_def.h"
#include "cshell.tab.h"


#if CSHELL_INCLUDE == 0
int gSymCount=0;

Sym_t gSymTable[SYM_MAX_NUM] =
{
  //example of data and func, refer to cshell::Sym_t.
  // address              name         type          module    size

  //size=sizeof(data)
  //{&idata,              "idata",    SYM_DATA,       0,       4},  

  //size=lineno(last line of func) - lineno(first line of func)
  //{vfun_sic,            "vfun_sic", SYM_FUNC,       0,       56}, 


};
#else
#include "csym.h"
#endif /*CSHELL_INCLUDE*/

void shellhelp()
{
  printf("USAGE:\n");
  printf("  SHELL is a C style interface for developing & debuging purpose. \n");
  printf("  It allows multi expressions in one line with ';'.\n");
  printf("  Also, () and {} are availabel to prioritise keys.\n");
  printf("\n");
  printf("  > prs>xxx to print symbol xxx as a string. May cause exception for non string symbols.\n");
  printf("  > prx>xxx to print any type symbol. Note, string is displayed as it's memory address.\n");
  printf("  > lk:xxx to display all symbols that contain xxx. same to lkup, ls, and list.\n");
  printf("    note: 1, no space before and after ':'; 2, no \"\" around symbol name\n");
  printf("  > list to display all symbols.\n");  
  printf("\n");
  printf("  > def m(a, b) if(a>b) then a else y\n");
  printf("    define macro expression m as in C style. Note: no tailed ';' allowed. \n");
  printf("\n");
  printf("  > q|quit to exit.\n");
  printf("\n");
  printf("  > ?|help to display this help.\n");
  printf("\n");
  
  return;
}


static char *tokenname(int token)
{
  static char s[16];
  switch(token)
  {
    case SYM_DATA: return "SYM_DATA";    
    case SYM_TEXT: return "SYM_TEXT";
    case SYM_FUNC: return "SYM_FUNC";
    
    case TYPEDOUBLE: return "TYPEDOUBLE";
    case TYPELONG: return "TYPELONG";
    case TYPESTRING: return "TYPESTRING";
    case TYPECHAR: return "TYPECHAR";
    case SYMBOL: return "SYMBOL";
    case EOL: return "EOL";
    case IF: return "IF";
    case THEN: return "THEN";
    case ELSE: return "ELSE";
    case WHILE: return "WHILE";
    case DO: return "DO";
    case DEF: return "DEF";
    case LE: return "LE";
    case GE: return "GE";
    case EQ: return "EQ";
    case NE: return "NE";
    case LSH: return "LSH";
    case RSH: return "RSH";
    case AND: return "AND";
    case OR:  return "OR";
    case INCR: return "INCR";
    case DECR: return "DECR";
    case XORA: return "XORA";
    case REVA: return "REVA";
    case ORA: return "ORA";
    case ANDA: return "ANDA";
    case RSHA: return "RSHA";
    case LSHA: return "LSHA";
    case MODA: return "MODA";
    case DIVA: return "DIVA";
    case MULA: return "MULA";
    case SUBA: return "SUBA";
    case ADDA: return "ADDA";
    case OP: return "OP";
    case UNARY: return "UNARY";
    case FUNCALL: return "FUNCALL";
    case LIST: return "LIST";
    default:
      if(token <= 0xFF) 
        sprintf(s, ":0x%02x :%d :'%c'", token, token, (char)token);
        else 
        sprintf(s, ":0x%02x :%d", token, token);      
      return s;
  }

  return "";
};

/* debugging: dump out an AST */
int debug_shell = 0;
void dumpast(ast_t *a, int level)
{
  if(!a)
  {
    printf("0\n");
    return;
  }

  printf("%*s", 2*level, "");	/* indent to this level */
  printf("TYPE: %s   SYMBOL: %s   VALUE: ", tokenname(a->type), a->name?a->name:"NA");
  switch(a->type)
  {
    case TYPECHAR:   printf(" '%c'\n", a->cval); break;
    case TYPESTRING: printf(" \"%s\"\n", a->sval); break;    
    case TYPEDOUBLE: printf(" %f\n", a->dval); break;
    case TYPELONG:   printf(" %p\n", a->addr); break;
    
    case SYMBOL:  case DEF:
             printf(" %p\n", a->addr); break;
             
    default: printf(" %p\n", a->addr); break;
  }
  
  printf("%*s", 2*level, "");	/* indent to this level */
  printf("left:\n");
  if(a->l) dumpast(a->l, level+1);
  
  printf("%*s", 2*level, "");	/* indent to this level */
  printf("right:\n");
  if(a->r) dumpast(a->r, level+1);

  return;
}


int dumpstringsymbol(char *symbol)
{
  ast_t *sym = findsymbol(symbol);
  if(!sym)
  {
    printf("symbol %s not exist.\n", symbol);
    return -1;
  }

  if(sym->type == SYM_DATA)
  {    
    printf("%s = \"%s\"\n", symbol, *(char**)sym->addr);
  }
  else if(sym->type == TYPESTRING)
  {    
    printf("%s = \"%s\" STRING\n", symbol, sym->sval);
  }
  else
  {
    printf("symbol %s not string type.\n", symbol);
    return -1;
  }


  return 0;
}

int dumpplainsymbol(char *symbol)
{
  ast_t *sym = findsymbol(symbol);
  if(!sym)
  {
    printf("symbol %s not exist.\n", symbol);
    return -1;
  }

  switch(sym->type)
  {
    case SYM_DATA:
      printf("%s %p = %p %d ", symbol, sym->addr, *(long**)sym->addr, *(int*)sym->addr);
      if(*(long*)sym->addr < 0xFF) printf("'%c'", *(char*)sym->addr);
      printf("\n");
      break;
    case SYM_FUNC:    printf("%s = %p FUNCTION\n",  symbol, sym->addr); break;
    case TYPELONG:    printf("%s = %p %ld LONG\n",  symbol, (void*)sym->value, sym->lval); break;
    case TYPEDOUBLE:  printf("%s = %p %f DOUBLE\n", symbol, (void*)sym->value, sym->dval); break;
    case TYPECHAR:    printf("%s = %p %c CHAR\n",   symbol, (void*)sym->value, sym->cval); break;
    case TYPESTRING:  printf("%s = %p \"%s\" STRING\n",symbol, (void*)sym->value, sym->sval); break;
    default:
      printf("%s type %s unknown\n", symbol, tokenname(sym->type));
      break;
  }
  
  return 0;
}


ast_t *newopcode(int opcode)
{
  ast_t *a = newast(opcode, 0, 0);
  if(!a)
  {
    cshellError("out of space");
    return 0;
  }
  
  return a;
}


ast_t *newvalue(int type, char *string, char *format)
{
  ast_t *a = newast(type, 0, 0);
  if(!a)
  {
    cshellError("out of space");
    return 0;
  }

  switch(type)
  {
    case TYPEDOUBLE: a->lval = (long)(atof(string)); break;
    case TYPELONG:   sscanf(string, format, &a->lval); break;
    case TYPECHAR:   a->cval = string[0]; break;
    case TYPESTRING:
      {
        char *s = strdup(string);
        s[strlen(s)-1] = 0; //remove the last \"
        a->sval = s;
        if(!a->value)
        {
          cshellError("out of space");
          freeast(a);
          
          return 0;
        }
      }
      break;
      
    default: freeast(a); return 0;
  }
  
  return a;
}


ast_t *newsymbol(char *symbol)
{
  ast_t *a = newast(SYMBOL, 0, 0);
  if(!a)
  {
    cshellError("out of space");
    return 0;
  }

  a->name = strdup(symbol);
  if(!a->name)
  {
    cshellError("out of space");
    freeast(a);
    
    return 0;
  }
  

  return a;
}





ast_t *newcondition(ast_t *c, ast_t *e1, ast_t *e2)
{
  ast_t *ifr = newast(THEN, e1, e2);
  ast_t *ifa = newast(IF, c, ifr);
  if(!ifa || !ifr) {
    cshellError("out of space");

    freeast(ifa);
    freeast(ifr);
    
    return 0;
  }
    
  return ifa;
}


void newdef(ast_t *def, ast_t *args, ast_t *list)
{
  def->type = DEF; //refine SYMBOL to DEF
   
  //update l & r
  def->l = args;
  def->r = list;

  //link it to symbol hash
  addsymbol(def);
  
  return;
}


#define OP_INT(tree, op) do{ \
  tree->type = tree->l->type; \
  tree->value = tree->l->value op tree->r->value; \
}while(0)

#define OP_BOOL(tree, op) do{ \
  tree->type = tree->l->type; \
  tree->value = (tree->l->value op tree->r->value) ? 1 : 0; \
}while(0)

#define OP_VAR_UN(tree, op) do{ \
  tree->value = tree->l->value; \
  tree->l->value op; \
  tree->type = tree->l->type; \
  update_value(tree->l); \
}while(0)
#define OP_UN_VAR(tree, op) do{ \
  tree->value = op tree->r->value; \
  tree->type = tree->r->type; \
  update_value(tree->r); \
}while(0)
#define OP_UN_DATA(tree, op) do{ \
  tree->value = op tree->r->value; \
  tree->type = tree->r->type; \
}while(0)


#define OP_ASSIGN(tree, op) do{ \
  tree->type = tree->l->type; \
  tree->l->value op tree->r->value; \
  tree->value = tree->l->value; \
}while(0)


static int debugstack(const char* fname, const char* func, int line, ast_t *tree)
{
  static int s = 0;
  
  printf("---------------------------------------------------------------------\n");
  printf("[%4d]eval debug %s %s %d\n", s++, fname, func, line);
  dumpast(tree, 0);
  return 0;
}
#define DEBUGSTACK(ast) do { if(DEBUG_DUMP_EVALSTACK()) debugstack(__FILE__, __FUNCTION__, __LINE__, ast); }while(0)

//copy symbol type and value to ast tree, only be called by assigning operations
int eval_symbol(ast_t *tree)
{
  int ret = -1;

  DEBUGSTACK(tree);
  
  if(!tree)
  {
    cshellError("assign can't be done between null asts: %p\n", tree);
    dumpast(tree, 0);
    return -1;
  }
 
  if(tree->name)
  {
    ast_t *sym = findsymbol(tree->name);
    if(!sym) //not defined, then new it
    {
      sym = newsymbol(tree->name);
      if(!sym)
      {
        cshellError("failed to new data symbol: %p.\n", tree);
        return -1;
      }
      
      ret = addsymbol(sym);
      if(ret < 0)
      {
        freetree(sym);
        return ret;
      }

      sym->sp = &sym->value;
    }

    tree->type = sym->type;
    tree->value = sym->value;
	tree->size = sym->size;
    
    return 0;
  } 
  else
  if(tree->type == '*')
  {
    ret = eval_tree(tree->r);
    if(ret < 0)
    {
      cshellError("failed to eval value from address: %p.\n", tree);
      return -1;
    }

    tree->value = tree->r->value;
    
    return 0;
  }

  cshellError("left of assign type=%s must be a symbol name: %p.", tokenname(tree->type), tree);
  return -1;
}


int eval_value(ast_t *tree)
{  
  DEBUGSTACK(tree);
  
  if(!tree || !tree->name)
  {
    cshellError("invalid symbol name: %p", tree);
    return -1;
  }
  
  ast_t *sym = findsymbol(tree->name);
  if(!sym)
  {
    cshellError("failed to find symbol: %s.\n", tree->name);
    return -1;    
  }

  //update type
  tree->type = sym->type;
  tree->size = sym->size;
  
  switch(sym->type)
  {
    case TYPECHAR:   tree->cval = sym->cval; break;
    case TYPEDOUBLE: tree->dval = sym->dval; break;
    case TYPELONG:   tree->lval = sym->lval; break;
    case TYPESTRING: tree->sval = sym->sval; break;
          
    case SYM_DATA: tree->value = *(long*)sym->addr; break;
      
    case SYM_TEXT: 
    case SYM_FUNC: tree->addr = sym->addr; break;
    
    case SYMBOL: //type is not determined yet.
    default: cshellError("unknown symbol data type: %p\n", tree); return -1;
  }
  
  return 0;
}

int update_value(ast_t *tree)
{  
  DEBUGSTACK(tree);
  
  if(!tree || !tree->name)
  {
    cshellError("invalid symbol name: %p\n", tree);
    return -1;
  }
  
  ast_t *sym = findsymbol(tree->name);
  if(!sym)
  {
    cshellError("failed to find symbol: %s.\n", tree->name);
    return -1;    
  }

  //update type
  sym->type = tree->type;
  
  switch(tree->type)
  {
    case TYPECHAR:   sym->cval = tree->cval; break;
    case TYPEDOUBLE: sym->dval = tree->dval; break;
    case TYPELONG:   sym->lval = tree->lval; break;
    case TYPESTRING: sym->sval = tree->sval; break;
          
    case SYM_DATA:
      if(sym->size==8) *(long*)sym->addr = tree->value;
      else if(sym->size==4) *(int*)sym->addr = tree->value;
      else if(sym->size==2) *(short*)sym->addr = tree->value;
      else if(sym->size==1) *(char*)sym->addr = tree->value;
      else cshellError("unknown symbol %s data size: %d\n", sym->name, sym->size);
      break;
      
    case SYM_TEXT: 
    case SYM_FUNC: tree->addr = sym->addr; break;
    
    case SYMBOL: //type is not determined yet.
    default: 
      cshellError("unknown symbol data type: %d\n", tree->type);
      dumpast(tree, 0);
      return -1;
  }
  
  return 0;
}


int eval_assign(ast_t *tree)
{
  int ret = -1;

  DEBUGSTACK(tree);
  
  if(!tree || !tree->l || !tree->r)
  {
    cshellError("assign can't be done between null asts: %p\n", tree);
    return -1;
  }
 
  ret = eval_tree(tree->r);
  if(ret < 0)
  {
    cshellError("failed to eval right tree: %p.\n", tree);
    return -1;
  }

  ret = eval_symbol(tree->l);
  if(ret < 0)
  {
    cshellError("failed to eval left tree: %p.\n", tree->l);
    return -1;
  }

  switch(tree->l->type)
  {
    case '*':
      *(int*)tree->l->value = tree->r->value;
      break;
    case SYM_DATA:
      tree->l->value = tree->r->value;
      break;
    case SYM_TEXT: case SYM_FUNC:
      cshellError("forbid to change function or text: %p.\n", tree->l);
      break;
    default:
      switch(tree->r->type)
      {
        case TYPELONG:  tree->l->lval = tree->r->lval; tree->l->type = TYPELONG; break;
        case TYPEDOUBLE:tree->l->dval = tree->r->dval; tree->l->type = TYPEDOUBLE; break;
        case TYPESTRING:tree->l->sval = tree->r->sval; tree->l->type = TYPESTRING; break;
        case TYPECHAR:  tree->l->cval = tree->r->cval; tree->l->type = TYPECHAR; break;        
        default: tree->l->value = tree->r->value; break;
      }
      break;
  }

  update_value(tree->l);

  tree->value = tree->l->value;
  
  return 0;
}

int xx=0;
int eval_unary(ast_t *tree)
{
  int ret = -1;

  DEBUGSTACK(tree);
  
  if(!tree)
  {
    cshellError("assign can't be done between null asts: %p.\n", tree);
    return -1;
  }
  
  if(tree->l == tree->r) //can't be both null or not null
  {
    cshellError("assign can only be done between null and non null leaves: %p.\n", tree);
    return -1;
  }

  //&var is address process, but not a value process
  if('&' == tree->type && tree->r->name)
  {    
    ast_t *sym = findsymbol(tree->r->name);
    if(!sym) {
      cshellError("failed to eval tree, unknown symbol: %s.\n", tree->r->name);
      return -1;
    }
    //dumpast(sym, 0);
    tree->type = TYPELONG;
    tree->value = (long)sym->sp;
    //dumpast(tree, 0);
    
    return 0;
  }

  if(tree->l)
  {
    ret = eval_tree(tree->l);
    if(ret < 0)
    {
      cshellError("failed to eval left tree: %p.\n", tree->l);
      return -1;
    }
  }
  else
  {
    ret = eval_tree(tree->r);
    if(ret < 0)
    {
      cshellError("failed to eval right tree: %p.\n", tree->r);
      return -1;
    }
  }
  
  switch(tree->type)
  {
    case INCR: if(tree->l) OP_VAR_UN(tree, ++); else OP_UN_VAR(tree, ++); break;
    case DECR: if(tree->l) OP_VAR_UN(tree, --); else OP_UN_VAR(tree, --); break;    
    case '+': OP_UN_DATA(tree, +); break;
    case '-': OP_UN_DATA(tree, -); break;
    case '~': OP_UN_DATA(tree, ~); break;
    case '*': { tree->value = *(long*)(tree->r->value); tree->type = TYPELONG; }; break;
    case '&': cshellError("invalid & to a non-variable\n"); return -1; break; /*pre-precess handle*/
    default:
      {
        cshellError("failed to eval tree, unknown type: %s %p.\n", tokenname(tree->type), tree);
        return -1;
      }
      break;
  }
  
  return 0;
}


int eval_bool(ast_t *tree)
{
  int ret = -1;

  DEBUGSTACK(tree);
  
  if(!tree || !tree->l || !tree->r)
  {
    cshellError("assign can't be done between null asts: %p.\n", tree);
    return -1;
  }
  
  ret = eval_tree(tree->l);
  if(ret < 0)
  {
    cshellError("failed to eval left tree: %p.\n", tree->l);
    return -1;
  }
  
  ret = eval_tree(tree->r);
  if(ret < 0)
  {
    cshellError("failed to eval right tree: %p.\n", tree->r);
    return -1;
  }
  
  switch(tree->type)
  {
    case '>': OP_BOOL(tree, >); break;
    case '<': OP_BOOL(tree, <); break;
    case NE:  OP_BOOL(tree, !=); break;
    case EQ:  OP_BOOL(tree, ==); break;
    case GE:  OP_BOOL(tree, >=); break;
    case LE:  OP_BOOL(tree, <=); break;
    case AND: OP_BOOL(tree, &&); break;
    case OR:  OP_BOOL(tree, ||); break;
      break;
    
    default:
      {
        cshellError("failed to eval tree, unknown type: %s %p.\n", tokenname(tree->type), tree);
        return -1;
      }
      break;
  }
  
  return 0;
}

int eval_op(ast_t *tree)
{
  int ret = -1;

  DEBUGSTACK(tree);
  
  if(!tree || !tree->l || !tree->r)
  {
    cshellError("assign can't be done between null asts: %p\n", tree);
    return -1;
  }
  
  ret = eval_tree(tree->l);
  if(ret < 0)
  {
    cshellError("failed to eval left tree: %p.\n", tree->l);
    return -1;
  }
  
  ret = eval_tree(tree->r);
  if(ret < 0)
  {
    cshellError("failed to eval right tree: %p.\n", tree->r);
    return -1;
  }
  
  switch(tree->type)
  {
    case '+': OP_INT(tree, +); break;
    case '-': OP_INT(tree, -); break;
    case '*': OP_INT(tree, *); break;
    case '/': OP_INT(tree, /); break;

    case '%': OP_INT(tree, %); break;
    
    case '&': OP_INT(tree, &); break;
    case '|': OP_INT(tree, |); break;
    case '^': OP_INT(tree, ^); break;

    case LSH: OP_INT(tree, <<); break;
    case RSH: OP_INT(tree, >>); break;
      break;
    
    default:
      {
        cshellError("failed to eval tree, unknown type: %s %p.\n", tokenname(tree->type), tree);
        return -1;
      }
      break;
  }
  
  return 0;
}

typedef int (*FUNPTR) (); 
#define MAXARG(array) (sizeof(array)/sizeof(array[0]))
int eval_funcall(ast_t *tree)
{
  int ret;
  
  int  argc = 0;
  long args[16];

  FUNPTR pCall;

  DEBUGSTACK(tree);

  memset(&args[0], 0, sizeof(args));

  if(!tree->l)
  {
    cshellError("eval funcall failed: null function: %p.\n", tree);
    return -1;
  }

  ret = eval_value(tree->l); //get function address
  if(ret < 0)
  {
    cshellError("eval funcall failed: not a defined function: %p.\n", tree->l);
    return -1;
  }

  pCall = (FUNPTR)tree->l->addr;

  //eval args.
  ast_t *next = tree->r;
  while(argc < MAXARG(args))
  {
    if(!next) break;

    if(next->type != SYMBOL)
    {
      cshellError("eval funcall failed: invalid right leaf type: %p.\n", next);
      return -1;
    }

    if(!next->l)
    {
      cshellError("eval funcall failed: invalid left leaf arg: %p.\n", next);
      return -1;
    }
    
    ret = eval_tree(next->l);
    if(ret < 0)
    {
      cshellError("eval funcall failed: left leaf arg invalid: %p.\n", next);
      return -1;
    }

    args[argc++] = next->l->value;

    next = next->r;    
  }

  if(argc >= MAXARG(args) && next)
  {
    cshellError("eval funcall failed: too many args. argc must be <=10.\n");
    return -1;
  }

  tree->value = pCall(args[0], args[1], args[2], args[3], args[4], 
                      args[5], args[6], args[7], args[8], args[9],
                      args[10], args[11], args[12], args[13], args[14],
                      args[15]);
  
  return 0;
}


int eval_tree(ast_t *tree)
{
  int ret = -1;

  DEBUGSTACK(tree);
  
  if(!tree)
  {
    return 0;
  }
    
  switch(tree->type)
  {
    case LIST:    ret = eval_tree(tree->l); ret += eval_tree(tree->r);  break;      
    case SYMBOL:  ret = eval_value(tree); break;
    
    case '+':  case '-':  case '*': case '/': case '%': case '&': case '|': case '~': case '^':     
    case INCR: case DECR: case RSH: case LSH:
      if(!tree->l || !tree->r) ret = eval_unary(tree);
        else ret = eval_op(tree);
      break;
      
    case AND: case OR: case '>': case '<': 
    case NE: case EQ: case GE: case LE: 
      ret = eval_bool(tree); 
      break;
      
    case '=': case MULA: case DIVA: case MODA: 
    case ADDA: case SUBA: case RSHA: case LSHA: 
    case ANDA: case ORA: case REVA: case XORA:
      ret = eval_assign(tree);
      break;
      
    case FUNCALL: ret = eval_funcall(tree); break;

    case TYPEDOUBLE: case TYPELONG: case TYPESTRING: case TYPECHAR: ret = 0; break;
    
    default: cshellError("unknown eval tree type: %d %s %p.\n", tree->type, tokenname(tree->type), tree); break;
  }
  
  return ret;
}

int eval_list(ast_t *tree)
{ 
  DEBUGSTACK(tree);
  
  if(!tree) return 0;
  
  int ret = eval_tree(tree);
  
  while(ret >= 0)
  {
    if(tree->type == LIST)
    {
      if(tree->l) printf(" = %ld (0x%lX) <size=%d>\n", tree->l->value, tree->l->value, tree->size);
      tree = tree->r;
      continue;
    }

    printf(" = %ld (0x%lX) <size=%d>\n", tree->value, tree->value, tree->size);
    break;
  }

  return 0;
}


ast_t *newast(int type, ast_t *l, ast_t *r)
{
  ast_t *a = malloc(sizeof(ast_t));
  
  if(!a) {
    cshellError("out of space");
    return 0;
  }

  memset(a, 0, sizeof(ast_t));
  
  a->type = type;
  a->l = l;
  a->r = r;
  
  return a;
}

void freeast(ast_t *a)
{
  if(a) free(a); //don't check any elements of the ast.
  
  return;
}

void freetree(ast_t *tree)
{
  if(!tree) return;

  if(tree->name) free(tree->name);
  if(tree->type == TYPESTRING) free(tree->sval);
  
  if(tree->l) freetree(tree->l);
  if(tree->r) freetree(tree->r);
  
  freeast(tree); /* always free the node itself */

  return;
}


static int yyerrorline  = 0;
void yyerror(char *s, ...)
{
  yyerrorline = yylineno;
  
  va_list ap;
  va_start(ap, s);

  fprintf(stderr, "%d: error: ", yylineno);
  vfprintf(stderr, s, ap);
  fprintf(stderr, "\n");
}



/* simple symbolhash of fixed size */
#define SYMBOL_NHASH 99991//9997
ast_t* symbolhash[SYMBOL_NHASH];

/* symbol table */
/* hash a symbol */
static unsigned symbolkey(char *symbol)
{
  unsigned int key = 0;
  unsigned c;

  while((c = *symbol++) != 0) key = key*9 ^ c;

  return key;
}


void listsymbol()
{
  int num;
  int count = 1;

  printf(" --   index   type       addr    name   size    value\n");
  for(num=0; num<SYMBOL_NHASH; num++)
  {
    ast_t *sp = symbolhash[num];
    if(!sp || !sp->type) continue;
    
    if(sp->type == DEF)
    {
      printf("[%5d : %8d] DEF:\n", count++, num);
      dumpast(sp, 1);
    }
    else
    {
      printf("[%5d : %8d] %10s: ", count++, num, tokenname(sp->type));
       
      //sym text, data, func. convert ast to zSym_t
      switch(sp->type)
      {
        case SYM_DATA:
        {
          printf("%p (%p, %d) %s %d %d\n", sp->sp, sp->addr, (int)sp->value, sp->name?sp->name:"NA", sp->size, *(int*)sp->addr);          
        }
        break;
          
        case SYM_FUNC:
          printf("%p (%p) %s %d\n", sp->sp, sp->addr, sp->name?sp->name:"NA", sp->size);
          break;
          
        default:        
          printf("%p (%p, %d) %s %d   ***\n", sp->sp, sp->addr, (int)sp->value, sp->name?sp->name:"NA", sp->size);
          break;
      }
    }
  }

  return;
}

void lookupsymbol(char* symbol)
{ 
  int num;
  int count = 1;

  printf(" --   index   type       addr    name   size    value\n");
  for(num=0; num<SYMBOL_NHASH; num++)
  {
    ast_t *sp = symbolhash[num];
    if(!sp) continue;
    
    if(sp->name && strstr((char*)sp->name, symbol))
    {
      if(sp->type == DEF)
      {
        printf("[%5d : %8d] DEF:\n", count++, num);
        dumpast(sp, 1);
      }
      else
      {
        printf("[%5d : %8d] %10s: ", count++, num, tokenname(sp->type));
         
        //sym text, data, func. convert ast to zSym_t
        switch(sp->type)
        {
          case SYM_DATA:
          {
            printf("%p %s %d %d\n", sp->addr, sp->name?sp->name:"NA", sp->size, *(int*)sp->addr);          
          }
          break;
            
          case SYM_FUNC:
            printf("%p %s %d \n", sp->addr, sp->name?sp->name:"NA", sp->size);
            break;
            
          default:        
            printf("%p %s %d \n", sp->addr, sp->name?sp->name:"NA", sp->size);
            break;
        }
      }
    }
  }

  return;
}

ast_t *findsymbol(char* symbol)
{
  if(!symbol)
  {
    cshellError("invalid null symbol\n");
    return 0;
  }
  
  int num = symbolkey(symbol)%SYMBOL_NHASH;
  int scount = SYMBOL_NHASH;

  //check if exist
  while(--scount >= 0)
  {
    ast_t *sp = symbolhash[num];
    if(!sp || !sp->type) continue;
    
    if(sp->name && !strcmp(sp->name, symbol))
    { 
      //update it than warning
      return symbolhash[num];
    }

    if(++num >= SYMBOL_NHASH) num=0; /* try the next entry */
  }

  return 0;
}

int addsymbol(ast_t* sym)
{
  if(!sym->name)
  {
    cshellError("invalid null symbol");
    return -1;
  }
  
  int num = symbolkey(sym->name)%SYMBOL_NHASH;
  int scount = SYMBOL_NHASH;

  //check if exist
  while(--scount >= 0)
  {
    ast_t *sp = symbolhash[num];
    if(!sp || !sp->type) continue;
    
    if(sp->name && !strcmp(sp->name, sym->name))
    { 
      //update it than warning
      //if(sp->type == TYPESTRING && sp->sval) free(sp->sval); //not sure, need test later
      freeast(sp);
      symbolhash[num] = sym;
      return 0;
    }

    if(++num >= SYMBOL_NHASH) num=0; /* try the next entry */
  }

  //not found, then add
  scount = SYMBOL_NHASH;
  while(--scount >= 0)
  {
    ast_t *sp = symbolhash[num];
    if(!sp)
    { 
      symbolhash[num] = sym;
      return 1;
    }

    if(++num >= SYMBOL_NHASH) num=0; /* try the next entry */
  }

  cshellError("symbolhash overflow");
  return -1;
}


static int symAddrCmp(const void *a, const void *b)
{
  return ((Sym_t*)a)->addr - ((Sym_t*)b)->addr;
}

int loadsymbols()
{
  static int load = 0;
  
  int i;
  Sym_t *sym;

  if(load > 0) return 1;
  
  load = 1;

  qsort(&gSymTable[0], gSymCount, sizeof(Sym_t), symAddrCmp);

  for(i=0; i<SYM_MAX_NUM; i++)
  {
    sym = &gSymTable[i];
    
    if(!sym->addr || !sym->name) break;

    ast_t *p = findsymbol(sym->name);
    if(p) continue;

    p = newsymbol(sym->name);
    if(!p)
    {
      cshellError("out of space.\n");
      return -1;
    }

    p->type = sym->type;
    p->addr = sym->addr;
    p->size = sym->size;
    p->sp = p->addr;

    addsymbol(p);
  }

  return 0;
}

void* symFindByName(const char *name, char **address, unsigned char *type, unsigned char *module)
{
  int i;

  if(!name) return 0;

  for(i=0; i<gSymCount; i++)
  {
    if(0==strcmp(gSymTable[i].name, name))
    {
      if(address) *address = (char*)gSymTable[i].addr;
      if(type) *type    = gSymTable[i].type;
      if(module) *module = gSymTable[i].module; 

      return gSymTable[i].addr;
    }
  }
  
  return 0;
}


int showSymTable()
{
  int i;

  printf("          ADDRESS    TYPE MODULE   SIZE      NAME\n");
  for(i=0; i<gSymCount; i++)
  {        
    if(!gSymTable[i].name) break;
  
    switch(gSymTable[i].type)
    {
      case SYM_FUNC: printf("FUNC    ");  break;
      case SYM_DATA: printf("DATA    ");  break; 
      default:       printf("UNKNOWN "); break;
    }
    printf("  %p   %3d   %3d    %4d       %s\n", gSymTable[i].addr, gSymTable[i].type, gSymTable[i].module, gSymTable[i].size, gSymTable[i].name);
  }
  
  return i;
}

const char* symFuncNameGet(void *addr)
{   
  Sym_t a, *b;

  if(!addr) return 0;
  
  a.addr = addr;

  b = bsearch(&a, &gSymTable[0], gSymCount, sizeof(Sym_t), symAddrCmp);
  if(b)
  {
    return b->name;
  }

  return 0;
}

const char* symFuncNameEval(unsigned addr)
{
  int i;

  if(!addr) return 0;
  
  for(i=0; i<gSymCount; i++)
  {        
    if(!gSymTable[i].name) break;

    if( ((*(unsigned*)&gSymTable[i].addr)&0xFFFFF) == addr ) return gSymTable[i].name;
  }

  return 0;
}


int loadscript(const char* s)
{
  printf("ERROR: attempt to load script %s\n", s);
  return 0;
}



unsigned *newTestArray(int len)
{
  len = (32 > len) ? 32 : len;
  len = (len < 0x1FFFF) ? len : 0x1FFFF;

  unsigned *p = (unsigned *)malloc(len + 4);
  if(!p) return p;

  *p = 0xFEDC0000 + len; //note: the last bit of 0xfedc is 0 
  
  printf("ptr: %p, %p %08x\n", p, p+1, *p);

  return p+1;
}

void freeTestArray(unsigned *p)
{
  free(p - 1);
  
  return;  
}

int setTestArray(unsigned *p, int offset, long value, int vsize)
{
  switch(vsize)
  {
    case 1: *((unsigned char*)p + offset) = value; break;
    case 2: *((unsigned short*)p + offset) = value; break;
    case 4: *((unsigned int*)p + offset) = value; break;
    case 8: *((unsigned long*)p + offset) = value; break;
    
    default: return -1;
  }

  return 0;
}

int printTestArray(unsigned *p, int len, int vsize)
{
  if(!p) return -1;

  if(len <= 0)
  {
    unsigned tag = *(p-1);
    len = tag & 0x1FFFF;
    tag = tag >> 16;
    if(tag != 0xFEDC)
    {
      printf("none standard tag: %04x test array: %p,%p %08x.\n", tag, p-1, p, *(p-1));
      return -1;
    }
    printf("test array size: %d.\n", len);
  }

  int i;
  switch(vsize)
  {    
    case 2: 
      for(i=0; i<len/2; i++) 
      { 
        printf(" %04x", *((unsigned short*)p+i)); 
        if((i&0x3)==0x3) printf("  "); 
        if((i&0xF)==0xF) printf("\n");
      }
      break;
      
    case 4: 
      for(i=0; i<len/4; i++) 
      { 
        printf(" %08x", *((unsigned*)p+i));
        if((i&0x3)==0x3) printf("  "); 
        if((i&0x7)==0x7) printf("\n"); 
      } 
      break;
      
    case 8: 
      for(i=0; i<len/8; i++) 
      { 
        //printf(" %016lx", *((unsigned long*)p+i)); 
        printf(" %08x-%08x", *((unsigned*)p+i*2), *((unsigned*)p+i*2+1));
        if((i&0x1)==0x1) printf("  "); 
        if((i&0x3)==0x3) printf("\n"); 
      }
      break;
    
    case 1: 
    default:
      for(i=0; i<len; i++) 
      { 
        printf(" %02x", *((unsigned char*)p+i)); 
        if((i&0x3)==0x3) printf("  "); 
        if((i&0x1F)==0x1F) printf("\n");
      }
      break;
  }

  return 0;
}


