/* 
 * Public Domain getopt header
 *
 * $Id$
 *
 */

#ifndef RDFPROC_GETOPT_H
#define RDFPROC_GETOPT_H

int getopt(int argc, char * const argv[], const char *optstring);
extern char *optarg;
extern int optind, opterr, optopt;

#endif
