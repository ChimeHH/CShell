
%{
#  include <stdio.h>
#  include <stdlib.h>
#  include "cshell_def.h"

%}

%union {
  ast_t *a;
}

/* declare tokens */
%token <a> TYPEDOUBLE TYPELONG TYPESTRING TYPECHAR SYMBOL FUNCALL LIST UNARY OP CMP ASSIGN
%token EOL

%token IF THEN ELSE WHILE DO DEF
%token MULA DIVA MODA ADDA SUBA RSHA LSHA ANDA ORA REVA XORA NE EQ GE LE

%left ASSIGN
%left CMP
%left OP 
%right '?' ':'
%left OR
%left AND
%left '+' '-'
%left '*' '/' '%'
%left LSH RSH
%left INCR DECR
%left UNARY '&' '|' '~' '^'
%left '(' '{' ';'


%type <a> exp stmt list explist 

%start calclist


%%


list: /* nothing */ { $$ = NULL; }
   | ';'  { $$ = NULL; }
   | '{' list '}'   {$$ = $2; }
   | stmt {$$ = $1; }
   | stmt ';' list { if ($3 == NULL)
	                $$ = $1;
                      else
			$$ = newast(LIST, $1, $3);
                    }
   ;

stmt: IF exp THEN stmt   { $$ = newcondition($2, $4, NULL); }
   | IF exp THEN stmt  ELSE  stmt  { $$ = newcondition( $2, $4, $6); }
   | WHILE exp DO stmt     { $$ = newast(WHILE, $2, $4); }
   | '{' list '}' {$$ = $2; }
   | exp
;

exp: INCR exp %prec UNARY   { $$ = newast(INCR, 0, $2); }
   | DECR exp %prec UNARY   { $$ = newast(DECR, 0, $2); }   
   | '*'  exp %prec UNARY   { $$ = newast('*',  0, $2); }   
   | '&'  exp %prec UNARY   { $$ = newast('&',  0, $2); }
   | '-'  exp %prec UNARY   { $$ = newast('-',  0, $2); }
   | '+'  exp %prec UNARY   { $$ = newast('+',  0, $2); }
   | '~'  exp %prec UNARY   { $$ = newast('~',  0, $2); }
   | exp INCR %prec UNARY   { $$ = newast(INCR, $1, 0); }
   | exp DECR %prec UNARY   { $$ = newast(DECR, $1, 0); } 
   | exp LSH exp            { $$ = newast(LSH, $1, $3); }
   | exp RSH exp            { $$ = newast(RSH, $1, $3); }
   | exp LSHA exp            { $$ = newast(LSHA, $1, $3); }
   | exp RSHA exp            { $$ = newast(RSHA, $1, $3); }
   | exp '+' exp            { $$ = newast('+', $1, $3); }
   | exp '-' exp            { $$ = newast('-', $1, $3); }
   | exp '*' exp            { $$ = newast('*', $1, $3); }
   | exp '/' exp            { $$ = newast('/', $1, $3); }
   | exp '%' exp            { $$ = newast('%', $1, $3); }
   | exp '&' exp            { $$ = newast('&', $1, $3); }
   | exp '|' exp            { $$ = newast('|', $1, $3); }
   | exp '^' exp            { $$ = newast('^', $1, $3); }
   | exp '~' exp            { $$ = newast('~', $1, $3); }
   | exp CMP exp            { $$ = newast($2->type, $1, $3); }
   | exp ASSIGN exp         { $$ = newast($2->type, $1, $3); }
   | '(' exp ')'          { $$ = $2; }
   | SYMBOL                  { $$ = $1; }
   | SYMBOL '(' ')'          { $$ = newast(FUNCALL, $1, 0);  }
   | SYMBOL '(' explist ')'  { $$ = newast(FUNCALL, $1, $3); }
;

explist: exp { $$ = newast(SYMBOL, $1, 0); }
 | exp ',' explist { $$ = newast(SYMBOL, $1, $3); }
;

calclist: /* nothing */
  | calclist list EOL {
      if(DEBUG_DUMP_TREE())  dumpast($2, 0);
      if(!DEBUG_NO_EVAL())   eval_list($2);
      if(DEBUG_DUMP_EVAL()) dumpast($2, 0);     
      freetree($2);      
      if(DEBUG_TTY_INPUT()) printf("> ");
    }
  | calclist DEF SYMBOL '(' explist ')' stmt EOL {
                       newdef($3, $5, $7);
                       if(DEBUG_DUMP_TREE()) dumpast($3, 0);
                       printf("Defined %s\n> ", $3->name); 
                       if(DEBUG_TTY_INPUT()) printf("> ");
    }

  | calclist error EOL { 
    yyerrok; 
    if(DEBUG_TTY_INPUT()) printf("> "); 
    }
  | calclist EOL { 
    if(DEBUG_TTY_INPUT()) printf("> "); 
    }
 ;
%%
