/*
 * rdf-tree.c - Retrieve statements from persistent Redland storage.
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
#include <openssl/lhash.h>

const char *VERSION = "0.3";

struct options {
  librdf_node *context;
  char *database;
  char *directory;
  char *host;
  char *id;
  int level;
  int port;
  char *output;
  char *password;
  char *query;
  char *user;
} opts;

int main(int argc, char *argv[]);
int tree(librdf_world * world, librdf_node * node, librdf_model * model,
	 librdf_model * outputmodel, LHASH * table, int level);
void hash_free(void *data);
int getoptions(int argc, char *argv[], librdf_world * world);
int usage(char *argv0, int version);

int
main(int argc, char *argv[])
{
  /* Redland objects. */
  librdf_world *world;
  librdf_storage *storage;
  librdf_model *model;
  librdf_uri *uri = 0;
  librdf_storage *outputstorage;
  librdf_model *outputmodel;
  librdf_storage *contextstorage;
  librdf_model *contextmodel;
  librdf_stream *stream;
  librdf_serializer *serializer;
  int argnum;
  char *storage_type;
  char *storage_options;

  /* Create rdflib world. */
  world = librdf_new_world();
  if(!world) {
    fprintf(stderr, "%s: Failed to create Redland world\n", argv[0]);
    return (1);
  };
  librdf_world_open(world);

  /* Parse command line options (if possible). */
  argnum = getoptions(argc, argv, world);

  /* Check for URI argument. */
  if(argnum < argc) {
    uri = librdf_new_uri(world, argv[argnum]);
    if(!uri) {
      fprintf(stderr, "%s: Failed to create input uri\n", argv[0]);
      exit(1);
    };
  };

  /* Set storage options. */
  if(opts.database) {
    storage_type = strdup("mysql");
    storage_options =
      malloc(strlen(opts.host) + strlen(opts.database) + strlen(opts.user) +
	     strlen(opts.password) + 120);
    if(!storage_type || !storage_options) {
      fprintf(stderr, "%s: Failed to create 'mysql' storage options\n",
	      argv[0]);
      return (1);
    };
    sprintf(storage_options,
	    "host='%s',database='%s',port='%i',user='%s',password='%s',contexts='yes',write='no'",
	    opts.host, opts.database, opts.port, opts.user, opts.password);
  }
  else {
    storage_type = strdup("hashes");
    storage_options = malloc(strlen(opts.directory) + 120);
    if(!storage_type || !storage_options) {
      fprintf(stderr, "%s: Failed to create 'hashes' storage options\n",
	      argv[0]);
      return (1);
    };
    sprintf(storage_options,
	    "hash-type='bdb',dir='%s',contexts='yes',write='no'",
	    opts.directory);
  };

  /* Create storage. */
  storage = librdf_new_storage(world, storage_type, opts.id, storage_options);
  if(!storage) {
    fprintf(stderr, "%s: Failed to create storage (%s/%s/%s)\n", argv[0],
	    storage_type, opts.id, storage_options);
    return (1);
  }

  /* Create model. */
  model = librdf_new_model(world, storage, NULL);
  if(!model) {
    fprintf(stderr, "%s: Failed to create model\n", argv[0]);
    return (1);
  }

  /* Create output storage. */
  outputstorage = librdf_new_storage(world, "memory", "output", "");
  if(!outputstorage) {
    fprintf(stderr, "%s: Failed to create output storage\n", argv[0]);
    return (1);
  }

  /* Create output model. */
  outputmodel = librdf_new_model(world, outputstorage, NULL);
  if(!outputmodel) {
    fprintf(stderr, "%s: Failed to create output model\n", argv[0]);
    return (1);
  }

  /* Create serializer. */
  serializer = librdf_new_serializer(world, opts.output, NULL, NULL);
  if(!serializer) {
    fprintf(stderr, "%s: Failed to create serializer\n", argv[0]);
    return (1);
  }

  if(librdf_model_size(model) != -1)
    fprintf(stderr, "%s: Model '%s' contains %d statements.\n", argv[0],
	    opts.id, librdf_storage_size(storage));

/*
stream=librdf_model_as_stream(model);
while (!librdf_stream_end(stream))
{
	librdf_statement *s;
	librdf_node *n;
	s=librdf_stream_get_object(stream);
	fprintf(stderr,"%s\n",librdf_statement_to_string(s));
	n=librdf_stream_get_context(stream);
	fprintf(stderr,"- context: %s\n",n?librdf_node_to_string(n):"-");
	librdf_stream_next(stream);
};
librdf_free_stream(stream);
*/

  /* Only statements with given context? */
  if(opts.context) {
    fprintf(stderr, "%s: Creating context storage...\n", argv[0]);

    /* Create context storage. */
    contextstorage = librdf_new_storage(world, "memory", "context", "");
    if(!contextstorage) {
      fprintf(stderr, "%s: Failed to create context storage\n", argv[0]);
      return (1);
    };
    /* Create context model. */
    contextmodel = librdf_new_model(world, contextstorage, NULL);
    if(!contextmodel) {
      fprintf(stderr, "%s: Failed to create context model\n", argv[0]);
      return (1);
    };
    /* Extract statements with given context. */
    if(!(stream = librdf_model_context_as_stream(model, opts.context))) {
      fprintf(stderr, "%s: Failed to serialize context model\n", argv[0]);
      return (1);
    };
    if(librdf_model_add_statements(contextmodel, stream)) {
      fprintf(stderr, "%s: Failed to add statements to context model\n",
	      argv[0]);
      return (1);
    };
    librdf_free_model(model);
    librdf_free_storage(storage);
    storage = contextstorage;
    model = contextmodel;
  };

  /* Populate output model... */
  if(uri) {
    int rc = 0;
    LHASH *table =
      lh_new((LHASH_HASH_FN_TYPE) lh_strhash, (LHASH_COMP_FN_TYPE) strcmp);
    if(!table) {
      fprintf(stderr, "%s: Failed to create hash table\n", argv[0]);
      return (1);
    };
    fprintf(stderr, "%s: Populating output model from uri...\n", argv[0]);
    /* Recursively extract statements about subject... */
    rc =
      tree(world, librdf_new_node_from_uri(world, uri), model, outputmodel,
	   table, opts.level);
    lh_doall(table, (LHASH_DOALL_FN_TYPE) hash_free);
    lh_free(table);
    if(rc) {
      fprintf(stderr, "%s: Failed to extract statements from model (%d)\n",
	      argv[0], rc);
      return (1);
    };
    librdf_free_model(model);
    librdf_free_storage(storage);
  }
  else if(opts.query) {
    librdf_free_model(model);
    librdf_free_storage(storage);
  }
  else {
    fprintf(stderr, "%s: Outputting entire model...\n", argv[0]);
    /* No restraints, use entire model as output. */
    librdf_free_model(outputmodel);
    librdf_free_storage(outputstorage);
    outputstorage = storage;
    outputmodel = model;
  };

  /* Serialize output... */
  fprintf(stderr, "%s: Serializing...\n", argv[0]);

  if(librdf_serializer_serialize_model(serializer, stdout, NULL, outputmodel)) {
    fprintf(stderr, "%s: Failed to serialize output model\n", argv[0]);
    return (1);
  };

  /* Clean up. */
  librdf_free_serializer(serializer);
  librdf_free_model(outputmodel);
  librdf_free_storage(outputstorage);
  librdf_free_world(world);

  /* keep gcc -Wall happy */
  return (0);
}

int
tree(librdf_world * world, librdf_node * node, librdf_model * model,
     librdf_model * outputmodel, LHASH * table, int level)
{
  librdf_stream *instream;
  librdf_node *object;
  librdf_statement *statement;
  int rc;
  librdf_node_type ont;

  /* Add node to hash, to prevent cycles. */
  lh_insert(table, strdup(librdf_node_to_string(node)));
  if(lh_error(table))
    return 1;

  /* Find all statements about node. */
  if(!
     (statement =
      librdf_new_statement_from_nodes(world, librdf_new_node_from_node(node),
				      NULL, NULL)))
    return 2;

  if(!(instream = librdf_model_find_statements(model, statement)))
    return 3;

  while(!librdf_stream_end(instream)) {
    /* Add statement to output model. */
    statement = librdf_stream_get_object(instream);
    if(librdf_model_add_statement(outputmodel,
				  librdf_new_statement_from_statement
				  (statement)))
      return 4;

    /* Recurse? */
    if(level) {
      object = librdf_statement_get_object(statement);
      ont = librdf_node_get_type(object);
      if(ont == LIBRDF_NODE_TYPE_RESOURCE || ont == LIBRDF_NODE_TYPE_BLANK) {
	/* 
	   librdf_statement_print(statement,stderr);
	   fprintf(stderr,"%s","\n"); 
	 */
	/* Don't recurse if object is known... */
	if(!lh_retrieve(table, librdf_node_to_string(object))) {
	  rc = tree(world, object, model, outputmodel, table, level - 1);
	  if(rc)
	    return rc;
	}
      };
    };
    librdf_stream_next(instream);
  };
  librdf_free_stream(instream);

  return 0;
};

void
hash_free(void *data)
{
  free(data);
};

int
getoptions(int argc, char *argv[], librdf_world * world)
{
  /* Define command line options. */
  struct option opts_long[] = {
    {"help", no_argument, NULL, '?'},
    {"context", required_argument, NULL, 'c'},
    {"database", required_argument, NULL, 's'},
    {"directory", required_argument, NULL, 'd'},
    {"host", required_argument, NULL, 'h'},
    {"id", required_argument, NULL, 'i'},
    {"level", required_argument, NULL, 'l'},
    {"output", required_argument, NULL, 'o'},
    {"port", required_argument, NULL, 'P'},
    {"password", optional_argument, NULL, 'p'},
    {"query", required_argument, NULL, 'q'},
    {"user", required_argument, NULL, 'u'},
    {"version", no_argument, NULL, 'v'},
    {0, 0, 0, 0}
  };
  const char *opts_short = "?c:D:d:h:i:l:o:P:p:q:u:v";
  int i = 1;
  char c;
  char *buffer;
  int ttypasswd = 1;

  /* Set defaults. */
  opts.context = 0;
  opts.level = 1;
  opts.password = 0;
  opts.port = 3306;
  opts.query = 0;
  opts.user = 0;
  if(!(opts.directory = strdup("./"))
     || !(opts.host = strdup("mysql"))
     || !(opts.id = strdup("redland"))
     || !(opts.output = strdup("rdfxml"))
     || !(opts.database = strdup("redland"))) {
    fprintf(stderr, "%s: Failed to allocate default options\n", argv[0]);
    exit(1);
  };

  while((c = getopt_long(argc, argv, opts_short, opts_long, &i)) != -1) {
    if(optarg) {
      buffer = malloc(strlen(optarg) + 1);
      if(!buffer) {
	fprintf(stderr,
		"%s: Failed to allocate buffer for command line argument (%s)\n",
		argv[0], optarg);
	exit(1);
      };
      strncpy(buffer, optarg, strlen(optarg) + 1);
    };
    switch (c) {
      case '?':
	usage(argv[0], 0);
      case 'c':
	free(buffer);
	opts.context = librdf_new_node_from_uri_string(world, optarg);
	if(!opts.context) {
	  fprintf(stderr, "%s: Failed to create context node (%s)\n", argv[0],
		  optarg);
	  exit(1);
	};
	break;
      case 'D':
	free(opts.directory);
	opts.directory = 0;
	opts.database = buffer;
	break;
      case 'd':
	free(opts.database);
	opts.database = 0;
	opts.directory = buffer;
	break;
      case 'h':
	opts.host = buffer;
	break;
      case 'i':
	opts.id = buffer;
	break;
      case 'l':
	free(buffer);
	opts.level = atoi(optarg);
	break;
      case 'o':
	free(opts.output);
	opts.output = buffer;
	break;
      case 'P':
	free(buffer);
	opts.port = atoi(optarg);
	break;
      case 'p':
	opts.password = buffer;
	ttypasswd = 0;
	break;
      case 'q':
	opts.query = buffer;
	break;
      case 'u':
	opts.user = buffer;
	break;
      case 'v':
	usage(argv[0], 1);
      default:
	fprintf(stderr, "%s: Invalid option (%c)\n", argv[0], c);
	usage(argv[0], 0);
    }
  }

  /* Flag missing user name. */
  if(opts.database && !opts.user) {
    fprintf(stderr, "%s: Missing user name for mysql storage\n", argv[0]);
    usage(argv[0], 0);
    exit(1);
  };

  /* Read password from tty if not specified. */
  if(opts.database && ttypasswd) {
    char c2;
    int i2 = 0;
    opts.password = malloc(128);
    if(!opts.password) {
      fprintf(stderr, "%s: Failed to allocate buffer for password\n",
	      argv[0]);
      exit(1);
    };
    fprintf(stderr, "%s: Enter password for %s@%s/%s: ", argv[0], opts.user,
	    opts.host, opts.database);
    while((c2 = getchar()) != '\n') {
      opts.password[i2++] = c2;
      if(i2 == 127)
	break;
    };
    opts.password[i2++] = 0;
  };

  return optind;
}

int
usage(char *argv0, int version)
{
  printf("\n\
%s  Version %s\n\
Retrieve statements from persistent Redland storage.\n\
* Copyright (C) 2003 Morten Frederiksen - http://purl.org/net/morten/\n\
* Copyright (C) 2000-2003 David Beckett - http://purl.org/net/dajobe/\n\
", argv0, VERSION);
  if(version)
    exit(0);
  printf("\n\
usage: %s [options] [ <URI> ]\n\
\n\
  -?, --help         Display this help message and exit.\n\
  -c<uri>, --context=<uri>\n\
                     Extract only statements with given context URI.\n\
  -D<database>, --database=<database>\n\
                     Name of MySQL database to use, default is 'redland'.\n\
  -d<directory>, --directory=<directory>\n\
                     Directory to use for BDB files. When provided implies use\n\
                     of 'hashes' storage type instead of 'mysql'.\n\
  -h<host name>, --host=<host name>\n\
                     Host to contact for MySQL connections, default is 'mysql'.\n\
  -i<storage id>, --id=<storage id>\n\
                     Identifier for (name of) storage (model name for storage\n\
                     type 'mysql', base file name for storage type 'hashes'),\n\
                     default is 'redland'.\n\
  -l<number>, --level=<number>\n\
                     The number of levels of statements to extract. Default is\n\
                     1, also returning statements about objects.\n\
  -o<syntax id>, --output=<syntax id>\n\
                     Syntax identifier for serialization, 'ntriples' or\n\
                     'rdfxml' (default).\n\
  -p<password>, --password=<password>\n\
                     Password to use when connecting to MySQL server.\n\
                     If password is not given it's asked from the tty.\n\
  -P<port number>, --port=<port number>\n\
                     The port number to use when connecting to MySQL server.\n\
                     Default port number is 3306.\n\
  -q<query language>, --query=<query language>\n\
                     Name of query language for query read from stdin. This\n\
                     overrides any subject URI given.\n\
  -u<user name>, --user=<user name>\n\
                     User name for MySQL server.\n\
  -v, --version      Output version information and exit.\n\
\n\
", argv0);
  exit(1);
}
