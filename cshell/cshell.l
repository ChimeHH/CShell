
%option noyywrap nodefault yylineno
%{
# include "cshell_def.h"
# include "cshell.tab.h"

int allow_exit = 1;
%}

/* float exponent */
FLOAT	([Ee][-+]?[0-9]+)

%option nounput
%option noinput

%%
 /* single character ops */

"," |
";" |
"(" |
")" |
"{" |
"}"   { return yytext[0]; }

"+" |
"-" |
"/" |
"*" |
"%" |
"&" |
"|" |
"^" |
"~"    { return yytext[0]; }

"++"   { return INCR; }
"--"   { return DECR; }
">>"   { return RSH; }
"<<"   { return LSH; }


">" |
"<"    { yylval.a = newopcode(yytext[0]); return CMP; }
"<>"   { yylval.a = newopcode(NE); return CMP; }
"=="   { yylval.a = newopcode(EQ); return CMP; }
">="   { yylval.a = newopcode(GE); return CMP; }
"<="   { yylval.a = newopcode(LE); return CMP; }
"&&"   { yylval.a = newopcode(AND);return CMP; }
"||"   { yylval.a = newopcode(OR); return CMP; }

"="    { yylval.a = newopcode(yytext[0]); return ASSIGN;}
"+="   { yylval.a = newopcode(ADDA); return ASSIGN; }
"-="   { yylval.a = newopcode(SUBA); return ASSIGN; }
"*="   { yylval.a = newopcode(MULA); return ASSIGN; }
"/="   { yylval.a = newopcode(DIVA); return ASSIGN; }
"%="   { yylval.a = newopcode(MODA); return ASSIGN; }
"<<="  { yylval.a = newopcode(LSHA); return ASSIGN; }
">>="  { yylval.a = newopcode(RSHA); return ASSIGN; }
"&="   { yylval.a = newopcode(ANDA); return ASSIGN; }
"|="   { yylval.a = newopcode(ORA); return ASSIGN; }
"~="   { yylval.a = newopcode(REVA); return ASSIGN; }
"^="   { yylval.a = newopcode(XORA); return ASSIGN; }



 /* keywords */

"if"    { return IF; }
"then"  { return THEN; }
"else"  { return ELSE; }
"while" { return WHILE; }
"do"    { return DO; }

"define" |
"def"   { return DEF;}

 /* built in functions */
"ls" |
"list"  { listsymbol(); }
"?" |
"help"  { shellhelp(); }
"loadsym"  { loadsymbols(); }


"prs:"[_a-zA-Z][_a-zA-Z0-9]*  { dumpstringsymbol(&yytext[4]); }
"prx:"[_a-zA-Z][_a-zA-Z0-9]*  { dumpplainsymbol(&yytext[4]); }
"ls:"[_a-zA-Z][_a-zA-Z0-9]*   |
"lk:"[_a-zA-Z][_a-zA-Z0-9]*   { lookupsymbol(&yytext[3]); }
"list:"[_a-zA-Z][_a-zA-Z0-9]* |
"lkup:"[_a-zA-Z][_a-zA-Z0-9]* { lookupsymbol(&yytext[5]); }

 /* debug asttree/eval_tree hack */
"debug"       { printf("DEBUG AST TREE is %d\n", debug_shell); } 
"debug"[0-9]+ { debug_shell = atoi(&yytext[5]); printf("DEBUG AST TREE set to %d\n", debug_shell); }



"q" |
"quit"  { /*if(DEBUG_TTY_INPUT())*/ if(allow_exit) exit(0); }


 /* names */
[_a-zA-Z][_a-zA-Z0-9]*  { yylval.a = newsymbol(yytext); return SYMBOL; }

[0-9]+       { yylval.a = newvalue(TYPELONG, yytext, "%ld"); return SYMBOL; }

[0-9]+"."[0-9]*{FLOAT}? |
"."?[0-9]+{FLOAT}? { yylval.a = newvalue(TYPEDOUBLE, yytext, 0); return SYMBOL; }

0[xX][0-9a-fA-F]+  { yylval.a = newvalue(TYPELONG, yytext, "%lX"); return SYMBOL; }

\'.\'        { yylval.a = newvalue(TYPECHAR, &yytext[1], 0); return SYMBOL; }
\"[^"]+\"    { yylval.a = newvalue(TYPESTRING, &yytext[1], 0);; return SYMBOL; }


"//".*  
[ \t]   /* ignore white space */ 
\\\n  { printf("c> "); } /* ignore line continuation */
[\n]  { return EOL; }

.  { cshellError("Mystery lex character %c\n", *yytext); }

%%


static YY_BUFFER_STATE lex_buf;

void read_cmd(const char *input)
{
  lex_buf = yy_scan_string(input);
  //yypush_buffer_state(lex_buf);
  yy_switch_to_buffer(lex_buf); //buffer change
}
#if !defined(YY_FLEX_MAJOR_VERSION) || YY_FLEX_MAJOR_VERSION < 2 \
    || (YY_FLEX_MAJOR_VERSION == 2 \
        && (!defined(YY_FLEX_MINOR_VERSION) || YY_FLEX_MINOR_VERSION < 5 \
            || (YY_FLEX_MINOR_VERSION == 5 \
                && (!defined(YY_FLEX_SUBMINOR_VERSION) \
                    || YY_FLEX_SUBMINOR_VERSION < 9))))
# define yylex_destroy() yy_delete_buffer (YY_CURRENT_BUFFER)
#endif
void end_cmd()
{
  //yy_delete_buffer(lex_buf);
  yylex_destroy(); //to avoid leakage 
}

