/*
FILE: cshell.h

Contact with hhao020@gmail.com for bug reporting and supporting.
*/


#ifndef C_SHELL_H
#define C_SHELL_H



#ifdef __cplusplus
    extern "C" {
#endif

#define USE_CLI_PIPE 1
extern int allow_exit;

extern int yylex (void);
extern int yyparse ();
extern void read_cmd(const char *);
extern void end_cmd();


//EXTERNAL APIs and Definations
#define MAX_SHELL_LINE 512
#define EOS 0
extern int run_cmd(const char* input);
extern long run_script(const char* input, int *ex, int *sk);
extern int shell_call(char *cmd);


extern void* symFindByName(const char *name, char **address, unsigned char *type, unsigned char *module);
extern int showSymTable();
extern const char* symFuncNameGet(void *addr);

//eval addr is the last most mostsignificant 20bits
extern const char* symFuncNameEval(unsigned addr);


extern int loadsymbols();


#ifdef __cplusplus
}
#endif

#endif /*C_SHELL_H*/

