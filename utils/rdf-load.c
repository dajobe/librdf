/*
 * rdf-load.c - Load statements from URI and store in persistent
 * Redland storage.
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
#include <mysql.h>

const char *VERSION="0.0.1";

struct options
{
	librdf_uri *base;
	librdf_node *context;
	char *database;
	char *directory;
	char *host;
	char *id;
	int keep;
	char *is_new;
	int port;
	char *password;
	char *rdfparser;
	char *user;
} opts;

int main(int argc,char *argv[]);
int getoptions(int argc,char *argv[],librdf_world *world);
int usage(char *argv0,int version);

int main(int argc,char *argv[])
{
	/* Redland objects. */
	librdf_world *world;
	librdf_storage *storage;
	librdf_model *model;
	librdf_parser *parser;
	librdf_uri *uri;
	librdf_stream *stream;
	int argnum;
	char *storage_type;
	char *storage_options;

	/* Create rdflib world. */
	world=librdf_new_world();
	if(!world)
	{
		fprintf(stderr, "%s: Failed to create Redland world\n",argv[0]);
		return(1);
	};
	librdf_world_open(world);

	/* Parse command line options (if possible). */
	argnum=getoptions(argc,argv,world);

	/* Check for URI argument. */
	if (argnum>=argc)
	{
		fprintf(stderr,"%s: Missing URI argument\n",argv[0]);
		usage(argv[0],0);
	};

	/* Get URI. */
	uri=librdf_new_uri(world,(const unsigned char *)argv[argnum]);
	if(!uri)
	{
		fprintf(stderr, "%s: Failed to create input uri\n",argv[0]);
		exit(1);
	};

	/* Set defaults. */
	if (!opts.base)
	{
		opts.base=librdf_new_uri_from_uri(uri);
		if(!opts.base)
		{
			fprintf(stderr, "%s: Failed to create base uri\n",argv[0]);
			exit(1);
		};
	};
	if (!opts.context)
	{
		opts.context=librdf_new_node_from_uri(world,opts.base);
		if(!opts.context)
		{
			fprintf(stderr, "%s: Failed to create context node\n",argv[0]);
			exit(1);
		};
	};

	/* Set storage options. */
	if (opts.database && !strcmp(opts.database,"memory"))
	{
		storage_type=strdup("memory");
		storage_options=0;
		if (!storage_type)
		{
			fprintf(stderr, "%s: Failed to create 'memory' storage options\n",argv[0]);
			return(1);
		};
	}
	else
	if (opts.database)
	{
		storage_type=strdup("mysql");
		storage_options=(char*)malloc(strlen(opts.host)+strlen(opts.database)+strlen(opts.user)+strlen(opts.password)+120);
		if (!storage_type || !storage_options)
		{
			fprintf(stderr, "%s: Failed to create 'mysql' storage options\n",argv[0]);
			return(1);
		};
		sprintf(storage_options,"new='%s',host='%s',database='%s',port='%i',user='%s',password='%s',write='yes'",
			opts.is_new,opts.host,opts.database,opts.port,opts.user,opts.password);
	}
	else
	{
		storage_type=strdup("hashes");
		storage_options=(char*)malloc(strlen(opts.directory)+120);
		if (!storage_type || !storage_options)
		{
			fprintf(stderr, "%s: Failed to create 'hashes' storage options\n",argv[0]);
			return(1);
		};
		sprintf(storage_options,"new='%s',hash-type='bdb',dir='%s',contexts='yes',write='yes'",
			opts.is_new,opts.directory);
	};

	/* Create storage. */
	storage=librdf_new_storage(world,storage_type,opts.id,storage_options);
	if(!storage)
	{
		fprintf(stderr, "%s: Failed to create new storage (%s/%s/%s)\n",argv[0],storage_type,opts.id,storage_options);
		return(1);
	}

	/* Create model. */
	model=librdf_new_model(world,storage,NULL);
	if(!model)
	{
		fprintf(stderr, "%s: Failed to create model\n",argv[0]);
		return(1);
	}
	if (librdf_model_size(model)!=-1)
		fprintf(stderr, "%s: Model '%s' initially contained %d statements\n",argv[0],opts.id,librdf_model_size(model));

	/* Create parser. */
	parser=librdf_new_parser(world,opts.rdfparser,NULL,NULL);
	if(!parser)
	{
		fprintf(stderr, "%s: Failed to create parser\n",argv[0]);
		return(1);
	}

	/* Parse... */
	if (!(stream=librdf_parser_parse_as_stream(parser,uri,opts.base)))
	{
		fprintf(stderr, "%s: Failed to parse uri (%s)\n",argv[0],librdf_uri_as_string(uri));
		return(1);
	};

	/* Remove existing statements with context? */
	if (!opts.keep && librdf_model_size(model))
	{
		if (librdf_model_context_remove_statements(model,opts.context))
		{
			fprintf(stderr, "%s: Failed to remove existing statements from model\n",argv[0]);
			return(1);
		};
		if (librdf_model_size(model)!=-1)
			fprintf(stderr, "%s: After context truncation: %d statements\n",argv[0],librdf_model_size(model));
	};

	/* Add statements with context. */
	if (librdf_model_context_add_statements(model,opts.context,stream))
	{
		fprintf(stderr, "%s: Failed to add statements to model\n",argv[0]);
		return(1);
	};
	if (librdf_model_size(model)!=-1)
		fprintf(stderr, "%s: After insertion: %d statements\n",argv[0],librdf_model_size(model));

	/* Clean up. */
	librdf_free_parser(parser);
	librdf_free_model(model);
	librdf_free_storage(storage);
	librdf_free_world(world);

	/* keep gcc -Wall happy */
	return(0);
}

int getoptions(int argc,char *argv[],librdf_world *world)
{
	/* Define command line options. */
	struct option opts_long[]={
		{"help",no_argument,NULL,'?'},
		{"base",required_argument,NULL,'b'},
		{"context",required_argument,NULL,'c'},
		{"database",required_argument,NULL,'s'},
		{"directory",required_argument,NULL,'d'},
		{"host",required_argument,NULL,'h'},
		{"id",required_argument,NULL,'i'},
		{"keep",no_argument,NULL,'k'},
		{"new",no_argument,NULL,'n'},
		{"port",required_argument,NULL,'P'},
		{"password",required_argument,NULL,'p'},
		{"rdf-parser",required_argument,NULL,'r'},
		{"user",required_argument,NULL,'u'},
		{"version",no_argument,NULL,'v'},
		{0,0,0,0}};
	const char *opts_short="?b:c:D:d:h:i:knP:p:r:u:v";
	int i=1;
	char c;
	char *buffer;
	int ttypasswd=1;

	/* Set defaults. */
	opts.base=0;
	opts.context=0;
	opts.password=0;
	opts.port=3306;
	opts.user=0;
	opts.keep=0;
	if (!(opts.directory=strdup("./"))
			|| !(opts.host=strdup("mysql"))
			|| !(opts.id=strdup("redland"))
			|| !(opts.is_new=strdup("no"))
			|| !(opts.rdfparser=strdup("raptor"))
			|| !(opts.database=strdup("redland")))
	{
		fprintf(stderr,"%s: Failed to allocate default options\n",argv[0]);
		exit(1);
	};

	while ((c=getopt_long(argc,argv,opts_short,opts_long,&i))!=-1)
	{
		if (optarg)
		{
			buffer=(char*)malloc(strlen(optarg)+1);
			if (!buffer)
			{
				fprintf(stderr,"%s: Failed to allocate buffer for command line argument (%s)\n",argv[0],optarg);
				exit(1);
			};
			strncpy(buffer,optarg,strlen(optarg)+1);
		};
		switch (c)
		{
			case '?':
				usage(argv[0],0);
			case 'b':
				free(buffer);
				opts.base=librdf_new_uri(world,(const unsigned char *)optarg);
				if (!opts.base)
				{
					fprintf(stderr,"%s: Failed to create base URI (%s)\n",argv[0],optarg);
					exit(1);
				};
				break;
			case 'c':
				free(buffer);
				opts.context=librdf_new_node_from_uri_string(world,(const unsigned char *)optarg);
				if (!opts.context)
				{
					fprintf(stderr,"%s: Failed to create context node (%s)\n",argv[0],optarg);
					exit(1);
				};
				break;
			case 'D':
				free(opts.directory);
				opts.directory=0;
				opts.database=buffer;
				break;
			case 'd':
				free(opts.database);
				opts.database=0;
				opts.directory=buffer;
				break;
			case 'h':
				opts.host=buffer;
				break;
			case 'i':
				opts.id=buffer;
				break;
			case 'k':
				opts.keep=1;
				break;
			case 'n':
				opts.is_new=(char*)malloc(4);
				if (!opts.is_new)
				{
					fprintf(stderr,"%s: Failed to allocate buffer for new parameter\n",argv[0]);
					exit(1);
				};
				strncpy(opts.is_new,"yes",4);
				break;
			case 'P':
				free(buffer);
				opts.port=atoi(optarg);
				break;
			case 'p':
				opts.password=buffer;
				ttypasswd=0;
				break;
			case 'r':
				opts.rdfparser=buffer;
				break;
			case 'u':
				opts.user=buffer;
				break;
			case 'v':
				usage(argv[0],1);
			default:
				fprintf(stderr,"%s: Invalid option (%c)\n",argv[0],c);
				usage(argv[0],0);
		}
	}

	/* Flag missing user name. */
	if (opts.database && !opts.user)
	{
		fprintf(stderr,"%s: Missing user name for mysql storage\n",argv[0]);
		usage(argv[0],0);
		exit(1);
	};

	/* Read password from tty if not specified. */
	if (opts.database && ttypasswd)
	{
		buffer=(char*)malloc(strlen(opts.host)+strlen(opts.database)+strlen(opts.user)+42);
		snprintf(buffer,40+strlen(opts.host)+strlen(opts.database)+strlen(opts.user),"Enter password for %s@%s/%s: ",opts.user,opts.host,opts.database);
		opts.password=get_tty_password(buffer);
		free(buffer);
	};

	return optind;
}

int usage(char *argv0,int version)
{
	printf("\
%s  Version %s\
Load statements from URI and store in persistent Redland storage.\
* Copyright (C) 2003 Morten Frederiksen - http://purl.org/net/morten/\
* Copyright (C) 2000-2003 David Beckett - http://purl.org/net/dajobe/\
",argv0,VERSION);
	if (version)
		exit(0);
	printf("\
usage: %s [options] <URI>\
\
  -?, --help         Display this help message and exit.\
  -b<uri>, --base=<uri>\
                     Base URI for parsing input statements.\
  -c<uri>, --context=<uri>\
                     URI to store with triples as context, default is base URI\
                     if present, otherwise source URI.\
  -D<database>, --database=<database>\
                     Name of MySQL database to use, default is 'redland'.\
  -d<directory>, --directory=<directory>\
                     Directory to use for BDB files. When provided implies use\
                     of 'hashes' storage type instead of 'mysql'.\
  -h<host name>, --host=<host name>\
                     Host to contact for MySQL connections, default is 'mysql'.\
  -i<storage id>, --id=<storage id>\
                     Identifier for (name of) storage (model name for storage\
                     type 'mysql', base file name for storage type 'hashes'),\
                     default is 'redland'.\
  -k, --keep         By default, existing statements in the given context will\
                     be removed prior to insertion of new statements, this\
                     option will skip the removal.\
  -n, --new          Truncate model before adding new statements.\
  -p<password>, --password=<password>\
                     Password to use when connecting to MySQL server.\
                     If password is not given it's asked from the tty.\
  -P<port number>, --port=<port number>\
                     The port number to use when connecting to MySQL server.\
                     Default port number is 3306.\
  -r<parser name>, --rdf-parser=<parser name>\
                     Name of parser to use ('raptor' for RDF/XML, or 'ntriples'\
                     for NTriples), default is 'raptor'.\
  -u<user name>, --user=<user name>\
                     User name for MySQL server.\
  -v, --version      Output version information and exit.\
\
",argv0);
	exit(1);
}
