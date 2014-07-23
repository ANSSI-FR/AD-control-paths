#ifndef GETOPT_H
#define GETOPT_H

#pragma warning(disable:4706) // warning : assignment within conditional expression

extern int opterr;  /* if error message should be printed */
extern int optind;  /* index into parent argv vector */
extern int optopt;  /* character checked for validity */
extern int optreset;/* reset getopt */
extern char *optarg;

int getopt(int nargc, char * const nargv[], const char *ostr);

#endif