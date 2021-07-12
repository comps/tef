#ifndef _PTEF_H
#define _PTEF_H

int ptef_runner(int argc, char **argv, char *basename, int jobs, int nomerge);
int ptef_report(char *status, char *name);
int ptef_mklog(char *testname);

#endif
