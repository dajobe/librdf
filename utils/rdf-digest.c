/*
 * rdf-digest.c - Create digest ID(s) for Redland MySQL storage.
 *
 * Copyright (C) 2003 Morten Frederiksen - http://purl.org/net/morten/
 *
 * and
 *
 * Copyright (C) 2000-2003 David Beckett - http://purl.org/net/dajobe/
 * Institute for Learning and Research Technology - http://www.ilrt.org/
 * University of Bristol - http://www.bristol.ac.uk/
 *
 * This package is Free Software or Open Source available under the
 * following licenses (these are alternatives):
 *   1. GNU Lesser General Public License (LGPL)
 *   2. GNU General Public License (GPL)
 *   3. Mozilla Public License (MPL)
 *
 * See LICENSE.html or LICENSE.txt at the top of this package for the
 * full license terms.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <redland.h>

int main(int argc,char *argv[])
{
	/* Redland objects. */
	librdf_world *world;
	librdf_digest* digest;
	char *id;
	unsigned long long int *id1;
	unsigned long long int *id2;

	/* Create rdflib world. */
	world=librdf_new_world();
	if(!world)
	{
		fprintf(stderr, "%s: Failed to create Redland world\n",argv[0]);
		return(1);
	};
	librdf_world_open(world);

	/* Prepare digest */
	if (!(digest=librdf_new_digest(world,"MD5")))
	{
		fprintf(stderr,"rdf-digest: unable to create digest.\n");
		return 1;
	};
	librdf_digest_init(digest);
	if (!(id=(char*)LIBRDF_MALLOC(cstring,16)))
		return 2;
	id1=(unsigned long long int*)id;
	id2=(unsigned long long int*)&id[8];
	librdf_digest_update(digest,(unsigned char *)argv[1],strlen(argv[1]));
	librdf_digest_final(digest);
	memcpy(id,librdf_digest_get_digest(digest),16);
	printf("%llu %llu\n",*id1,*id2);
	return 0;
}
