/*
 * RDF HASH Implementation
 *
 * $Source$
 * $Id$
 *
 * (C) Dave Beckett 2000 ILRT, University of Bristol
 * http://www.ilrt.bristol.ac.uk/people/cmdjb/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */

#include <config.h>

#include <stdio.h>
#include <sys/types.h>

#include <rdf_config.h>

#include <rdf_hash.h>
#ifdef HAVE_GDBM_HASH
#include <rdf_hash_gdbm.h>
#endif
#include <rdf_hash_list.h>


/** initialise module */
void
init_rdf_hash(void) 
{
#ifdef HAVE_GDBM_HASH
  init_rdf_hash_gdbm();
#endif
  init_rdf_hash_list();
}

/** list of hash factories */
static rdf_hash_factory* hashes;


/* class methods */

/** register a hash factory */
void rdf_hash_register_factory(const char *name,
                               void (*factory) (rdf_hash_factory*)
                               ) 
{
  rdf_hash_factory *hash, *h;
  char *name_copy;

  RDF_DEBUG2(rdf_hash_register_factory,
             "Received registration for hash %s\n", name);

  hash=(rdf_hash_factory*)RDF_CALLOC(rdf_hash_factory, sizeof(rdf_hash_factory), 1);
  if(!hash)
    RDF_FATAL(rdf_hash_register_factory, "Out of memory\n");

  name_copy=(char*)RDF_CALLOC(cstring, strlen(name)+1, 1);
  if(!name_copy) {
    RDF_FREE(rdf_hash, hash);
    RDF_FATAL(rdf_hash_register_factory, "Out of memory\n");
  }
  strcpy(name_copy, name);
  hash->name=name_copy;

  for(h = hashes; h; h = h->next ) {
    if(!strcmp(h->name, name_copy)) {
      RDF_FATAL2(rdf_hash_register_factory, "hash %s already registered\n", h->name);
    }
  }

  /* Call the hash registration function on the new object */
  (*factory)(hash);


  RDF_DEBUG3(rdf_hash_register_factory, "%s has context size %d\n", name, hash->context_length);

  hash->next = hashes;
  hashes = hash;
}


/** return the hash factory for a given name or NULL */
rdf_hash_factory*
get_rdf_hash_factory (const char *name) 
{
  rdf_hash_factory *factory;

  /* return 1st hash if no particular one wanted - why? */
  if(!name) {
    factory=hashes;
    if(!factory) {
      RDF_DEBUG(get_rdf_hash_factory, "No (default) hashes registered\n");
      return NULL;
    }
  } else {
    for(factory=hashes; factory; factory=factory->next) {
      if(!strcmp(factory->name, name)) {
        break;
      }
    }
    /* else FACTORY name not found */
    if(!factory) {
      RDF_DEBUG2(get_rdf_hash_factory, "No hash with name %s found\n",
              name);
      return NULL;
    }
  }
  
  return factory;
}



/* constructors */

/** constructor for rdf_hash
 * @param factory object from the factory
 */
rdf_hash*
new_rdf_hash (rdf_hash_factory* factory) {
  rdf_hash* h;

  h=(rdf_hash*)RDF_CALLOC(rdf_hash, sizeof(rdf_hash), 1);
  if(!h)
    return NULL;

  h->context=(char*)RDF_CALLOC(hash_context, factory->context_length, 1);
  if(!h->context) {
    free_rdf_hash(h);
    return NULL;
  }

  h->factory=factory;

  return h;
}


/** destructor for rdf_hash objects */
void
free_rdf_hash (rdf_hash* hash) 
{
  RDF_FREE(rdf_hash, hash);
}

/* methods */

/** open/create hash with identifier (usually file or URI), mode and options */
int
rdf_hash_open(rdf_hash* hash, char *identifier, void *mode, void *options) 
{
  return (*(hash->factory->open))(hash->context, identifier, mode, options);
}

/** end hash association */
int
rdf_hash_close(rdf_hash* hash)
{
  return (*(hash->factory->close))(hash->context);
}


/** retrieve / insert key/data pairs from hash according to flags */
int
rdf_hash_get(rdf_hash* hash, void *key, size_t key_len,
             void **value, size_t* value_len, unsigned int flags)
{
  rdf_hash_data hd_key, hd_value;
  int result;
  
  /* copy key pointers and lengths into rdf_hash_data structures */
  hd_key.data=key; hd_key.size=key_len;
  
  result=(*(hash->factory->get))(hash->context, &hd_key, &hd_value, flags);
  /* copy out results into user variables */
  *value=hd_value.data; *value_len=hd_value.size;
  return result;
}

/** insert key/value pairs into hash according to flags */
int
rdf_hash_put(rdf_hash* hash, void *key, size_t key_len,
             void *value, size_t value_len, unsigned int flags)
{
  rdf_hash_data hd_key, hd_value;

  /* copy pointers and lengths into rdf_hash_data structures */
  hd_key.data=key; hd_key.size=key_len;
  hd_value.data=value; hd_value.size=value_len;

  /* call generic routine using rdf_hash_data structs */
  return (*(hash->factory->put))(hash->context, &hd_key, &hd_value, flags);
}


/** delete a key (and associated value) from hash */
int
rdf_hash_delete(rdf_hash* hash, void *key, size_t key_len)
{
  rdf_hash_data hd_key;

  /* copy key pointers and lengths into rdf_hash_data structures */
  hd_key.data=key; hd_key.size=key_len;

  return (*(hash->factory->delete_key))(hash->context, &hd_key);
}

/** retrieve a key/value pair via cursor-based/sequential access */
int
rdf_hash_get_seq(rdf_hash* hash, void **key, size_t* key_len,
                 unsigned int flags)
{
  rdf_hash_data hd_key;
  
  int result=(*(hash->factory->get_seq))(hash->context, &hd_key, flags);
  /* copy out results into user variables */
  *key=hd_key.data; *key_len=hd_key.size;
  return result;
}

/** flush any cached information to disk if appropriate */
int
rdf_hash_sync(rdf_hash* hash)
{
  return (*(hash->factory->sync))(hash->context);
}

/** get the file descriptor for the hash, if it is file based (for locking) */
int
rdf_hash_get_fd(rdf_hash* hash)
{
  return (*(hash->factory->get_fd))(hash->context);
}

/** return the 'first' hash key in a sequential access of the hash
 * Note: the key pointer will return newly allocated memory each time.
 * @param hash hash object
 * @param key pointer to void* where key will be stored
 * @param key_len pointer to size_t where length will be stored
 */

int
rdf_hash_first(rdf_hash* hash, void** key, size_t* key_len)
{
  return rdf_hash_get_seq(hash, key, key_len, RDF_HASH_FLAGS_FIRST);
}

/**
 * return the 'next' hash key in a sequential access of the hash as
 * started by rdf_hash_first
 * Note: the key pointer will return newly allocated memory each time.
 * @param hash hash object
 * @param key pointer to void* where key will be stored
 * @param key_len pointer to size_t where length will be stored
 * @see rdf_hash_first()
 */
int
rdf_hash_next(rdf_hash* hash, void** key, size_t* key_len)
{
  return rdf_hash_get_seq(hash, key, key_len, RDF_HASH_FLAGS_NEXT);
}



/** pretty-print the has to a file descriptior.
 * @param hash the hash
 * @param fh   file descriptork   actually a FILE*
 */
void
rdf_hash_print (rdf_hash* hash, FILE *fh) 

{
  void *key;
  size_t key_len;
  void *value;
  size_t value_len;

  fprintf(fh, "%s hash: {\n", hash->factory->name);
  for(rdf_hash_first(hash, &key, &key_len);
      key;
      rdf_hash_next(hash, &key, &key_len)) {
    rdf_hash_get(hash, key, key_len, &value, &value_len, 0);
    fputs("  '", fh);
    fwrite(key, key_len, 1, fh);
    fputs("'=>'", fh);
    fwrite(value, value_len, 1, fh);
    fputs("'\n", fh);

    /* key points to new data each time */
    RDF_FREE(cstring, key);
  }
  fputc('}', fh);
}





#ifdef RDF_HASH_TEST

/* one more prototype */
int main(int argc, char *argv[]);


int
main(int argc, char *argv[]) 
{
  rdf_hash_factory* factory;
  rdf_hash* h;
  char *test_hash_types[]={"GDBM", "FAKE", "LIST", NULL};
  char *test_hash_values[]={"colour", "yellow",
                            "age", "new",
                            "size", "large",
                            "fruit", "banana",
                            NULL, NULL};
  char *test_hash_delete_key="size";
  int i,j;
  char *type;
  char *key, *value;
  char *program=argv[0];
  
  
  /* initialise hash module */
  init_rdf_hash();

  for(i=0; (type=test_hash_types[i]); i++) {
    fprintf(stderr, "%s: Trying to create new %s hash\n", program, type);
    factory=get_rdf_hash_factory(type);
    if(!factory) {
      fprintf(stderr, "%s: No hash factory called %s\n", program, type);
      continue;
    }
    
    h=new_rdf_hash(factory);
    if(!h) {
      fprintf(stderr, "%s: Failed to create new hash type %s\n", program, type);
      continue;
    }

    rdf_hash_open(h, "test.gdbm", "mode", "options");

    for(j=0; test_hash_values[j]; j+=2) {
      key=test_hash_values[j];
      value=test_hash_values[j+1];
      fprintf(stderr, "%s: Adding key/value pair: %s=%s\n", program, key, value);
      
      rdf_hash_put(h, key, strlen(key), value, strlen(value),
                   0);
      
      fprintf(stderr, "%s: resulting ", program);    
      rdf_hash_print(h, stderr);
      fprintf(stderr, "\n");
    }

    fprintf(stderr, "%s: Deleting key '%s'\n", program, test_hash_delete_key);
    rdf_hash_delete(h, test_hash_delete_key, strlen(test_hash_delete_key));

    fprintf(stderr, "%s: resulting ", program);    
    rdf_hash_print(h, stderr);
    fprintf(stderr, "\n");
    
    rdf_hash_close(h);

    fprintf(stderr, "%s: Freeing hash\n", program);
    free_rdf_hash(h);
  }
  
    
  /* keep gcc -Wall happy */
  return(0);
}

#endif
