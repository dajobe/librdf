/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_hash.c - RDF Hash Implementation
 *
 * $Id$
 *
 * (C) Dave Beckett 2000 ILRT, University of Bristol
 * http://www.ilrt.bristol.ac.uk/people/cmdjb/
 *
 *                                       
 * This program is free software distributed under either of these licenses:
 *   1. The GNU Lesser General Public License (LGPL)
 * OR ALTERNATIVELY
 *   2. The modified BSD license
 *
 * See LICENSE.html or LICENSE.txt for the full license terms.
 */


#include <rdf_config.h>

#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>

#define LIBRDF_INTERNAL 1
#include <librdf.h>

#include <rdf_hash.h>
#ifdef HAVE_GDBM_HASH
#include <rdf_hash_gdbm.h>
#endif
#ifdef HAVE_BDB_HASH
#include <rdf_hash_bdb.h>
#endif
#include <rdf_hash_memory.h>


/* prototypes for helper functions */
static void librdf_delete_hash_factories(void);


/**
 * librdf_init_hash - Initialise the librdf_hash module
 *
 * Initialises and registers all
 * compiled hash modules.  Must be called before using any of the hash
 * factory functions such as librdf_get_hash_factory()
 **/
void
librdf_init_hash(void) 
{
#ifdef HAVE_GDBM_HASH
  librdf_init_hash_gdbm();
#endif
#ifdef HAVE_BDB_HASH
  librdf_init_hash_bdb();
#endif
  /* Always have hash in memory implementation available */
  librdf_init_hash_memory(-1); /* use default load factor */
}

/**
 * librdf_finish_hash - Terminate the librdf_hash module
 **/
void
librdf_finish_hash(void) 
{
  librdf_delete_hash_factories();
}


/* statics */

/* list of hash factories */
static librdf_hash_factory* hashes;


/* helper functions */
static void
librdf_delete_hash_factories(void)
{
  librdf_hash_factory *factory, *next;
  
  for(factory=hashes; factory; factory=next) {
    next=factory->next;
    LIBRDF_FREE(librdf_hash_factory, factory->name);
    LIBRDF_FREE(librdf_hash_factory, factory);
  }
}


/* class methods */

/**
 * librdf_hash_register_factory - Register a hash factory
 * @name: the hash factory name
 * @factory: pointer to function to call to register the factory
 * 
 **/
void
librdf_hash_register_factory(const char *name,
                             void (*factory) (librdf_hash_factory*)) 
{
  librdf_hash_factory *hash, *h;
  char *name_copy;
  
  LIBRDF_DEBUG2(librdf_hash_register_factory,
		"Received registration for hash %s\n", name);
  
  hash=(librdf_hash_factory*)LIBRDF_CALLOC(librdf_hash_factory, 1,
                                           sizeof(librdf_hash_factory));
  if(!hash)
    LIBRDF_FATAL1(librdf_hash_register_factory, "Out of memory\n");
  
  name_copy=(char*)LIBRDF_CALLOC(cstring, strlen(name)+1, 1);
  if(!name_copy) {
    LIBRDF_FREE(librdf_hash, hash);
    LIBRDF_FATAL1(librdf_hash_register_factory, "Out of memory\n");
  }
  strcpy(name_copy, name);
  hash->name=name_copy;
  
  for(h = hashes; h; h = h->next ) {
    if(!strcmp(h->name, name_copy)) {
      LIBRDF_FATAL2(librdf_hash_register_factory,
		    "hash %s already registered\n", h->name);
    }
  }
  
  /* Call the hash registration function on the new object */
  (*factory)(hash);
  
  LIBRDF_DEBUG3(librdf_hash_register_factory, "%s has context size %d\n",
		name, hash->context_length);
  
  hash->next = hashes;
  hashes = hash;
}


/**
 * librdf_get_hash_factory - Get a hash factory by name
 * @name: the factory name or NULL for the default factory
 * 
 * Return value: the factory object or NULL if there is no such factory
 **/
librdf_hash_factory*
librdf_get_hash_factory (const char *name) 
{
  librdf_hash_factory *factory;

  /* return 1st hash if no particular one wanted - why? */
  if(!name) {
    factory=hashes;
    if(!factory) {
      LIBRDF_DEBUG1(librdf_get_hash_factory,
		    "No (default) hashes registered\n");
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
      LIBRDF_DEBUG2(librdf_get_hash_factory, "No hash with name %s found\n",
		    name);
      return NULL;
    }
  }
  
  return factory;
}



/**
 * librdf_new_hash -  Constructor - create a new librdf_hash object
 * @factory: the factory to use to construct the hash
 *
 * Return value: a new &librdf_hash object or NULL on failure
 */
librdf_hash*
librdf_new_hash (librdf_hash_factory* factory) {
  librdf_hash* h;

  h=(librdf_hash*)LIBRDF_CALLOC(librdf_hash, sizeof(librdf_hash), 1);
  if(!h)
    return NULL;
  
  h->context=(char*)LIBRDF_CALLOC(librdf_hash_context, 1,
                                  factory->context_length);
  if(!h->context) {
    librdf_free_hash(h);
    return NULL;
  }
  
  h->factory=factory;
  
  return h;
}


/**
 * librdf_free_hash - Destructor - destroy a librdf_hash object
 *
 * @hash: hash object
 **/
void
librdf_free_hash (librdf_hash* hash) 
{
  if(hash->context)
    LIBRDF_FREE(librdf_hash_context, hash->context);
  LIBRDF_FREE(librdf_hash, hash);
}


/* methods */

/**
 * librdf_hash_open - Start a hash association 
 * @hash: hash object
 * @identifier: indentifier for the hash factory - usually a URI or file name
 * @mode: hash access mode
 * @options: a hash of options for the hash factory or NULL if there are none.
 * 
 * This method opens and/or creates a new hash with any resources it
 * needs.
 * 
 * Return value: non 0 on failure
 **/
int
librdf_hash_open(librdf_hash* hash, char *identifier, void *mode,
                 librdf_hash* options) 
{
  return hash->factory->open(hash->context, identifier, mode, options);
}


/**
 * librdf_hash_close - End a hash association
 * @hash: hash object
 * 
 * Return value: non 0 on failure
 **/
int
librdf_hash_close(librdf_hash* hash)
{
  return hash->factory->close(hash->context);
}


/**
 * librdf_hash_get - Retrieve data from hash given a key according to flags
 * @hash: hash object
 * @key: pointer to key
 * @key_len: key length in bytes
 * @value: pointer to variable to store value pointer
 * @value_len: pointer to variable to store value length
 * @flags: 0 at present
 * 
 * The value returned is from newly allocated memory which the
 * caller must free.
 * 
 * Return value: non 0 on failure
 **/
int
librdf_hash_get(librdf_hash* hash, void *key, size_t key_len,
                void **value, size_t* value_len, unsigned int flags)
{
  librdf_hash_data hd_key, hd_value;
  int result;
        
  /* copy key pointers and lengths into librdf_hash_data structures */
  hd_key.data=key; hd_key.size=key_len;
        
  result=hash->factory->get(hash->context, &hd_key, &hd_value, flags);
  /* copy out results into user variables */
  *value=hd_value.data; *value_len=hd_value.size;
  return result;
}


/**
 * librdf_hash_put - Insert key/value pairs into the hash according to flags
 * @hash: hash object
 * @key: pointer to key 
 * @key_len: length of key in bytes
 * @value: pointer to the value
 * @value_len: length of the value in bytes
 * @flags: 0 at present
 * 
 * The key and values are copied into the hash; the original pointers
 * can be deleted.
 * 
 * Return value: non 0 on failure
 **/
int
librdf_hash_put(librdf_hash* hash, void *key, size_t key_len,
                void *value, size_t value_len, unsigned int flags)
{
  librdf_hash_data hd_key, hd_value;
        
  /* copy pointers and lengths into librdf_hash_data structures */
  hd_key.data=key; hd_key.size=key_len;
  hd_value.data=value; hd_value.size=value_len;
        
  /* call generic routine using librdf_hash_data structs */
  return hash->factory->put(hash->context, &hd_key, &hd_value, flags);
}


/**
 * librdf_hash_exists - Check if a given key is in the hash
 * @hash: hash object
 * @key: pointer to the key
 * @key_len: length of key in bytes
 * 
 * Return value: non 0 if the key is present in the hash
 **/
int
librdf_hash_exists(librdf_hash* hash, void *key, size_t key_len) 
{
  librdf_hash_data hd_key;
  /* copy key pointers and lengths into librdf_hash_data structures */
  hd_key.data=key; hd_key.size=key_len;
        
  return hash->factory->exists(hash->context, &hd_key);
}


/**
 * librdf_hash_delete - Delete a key/value pair from the hash
 * @hash: hash object
 * @key: pointer to key
 * @key_len: length of key in bytes
 * 
 * Return value: non 0 on failure (including pair not present)
 **/
int
librdf_hash_delete(librdf_hash* hash, void *key, size_t key_len)
{
  librdf_hash_data hd_key;

  /* copy key pointers and lengths into librdf_hash_data structures */
  hd_key.data=key; hd_key.size=key_len;

  return hash->factory->delete_key(hash->context, &hd_key);
}


/**
 * librdf_hash_get_seq - Retrieve hash keys via sequential access
 * @hash: the hash 
 * @key: pointer to variable to store key pointer
 * @key_len: pointer to variable to store key length
 * @type: LIBRDF_HASH_SEQUENCE_FIRST to return first key, LIBRDF_HASH_SEQUENCE_NEXT to return next key, LIBRDF_HASH_SEQUENCE_CURRENT to return current key in sequence
 * 
 * Note; the key pointer will return newly allocated memory each time
 * which must be freed by the caller.
 * 
 * Return value: non 0 on failure
 **/
int
librdf_hash_get_seq(librdf_hash* hash, void **key, size_t* key_len,
                    librdf_hash_sequence_type type)
{
  librdf_hash_data hd_key;
  
  int result=hash->factory->get_seq(hash->context, &hd_key, type);
  /* copy out results into user variables */
  *key=hd_key.data; *key_len=hd_key.size;
  return result;
}


/**
 * librdf_hash_sync - Flush any cached information to disk if appropriate
 * @hash: hash object
 * 
 * Return value: non 0 on failure
 **/
int
librdf_hash_sync(librdf_hash* hash)
{
  return hash->factory->sync(hash->context);
}


/**
 * librdf_hash_get_fd - Get the file descriptor for the hash
 * @hash: hash object
 * 
 * This returns the file descriptor if it is file based for
 * use with file locking.
 * 
 * Return value: the file descriptor
 **/
int
librdf_hash_get_fd(librdf_hash* hash)
{
  return hash->factory->get_fd(hash->context);
}


/**
 * librdf_hash_first - Return the 'first' hash key in a sequential access of the hash
 * @hash: hash object
 * @key: pointer to variable to store key pointer
 * @key_len: pointer to variable to store key length
 * 
 * Note: the key pointer will return newly allocated memory each time
 * which must be freed by the caller.
 *
 * See also librdf_hash_next() and librdf_hash_get_seq()
 * 
 * Return value: non 0 on failure
 **/
int
librdf_hash_first(librdf_hash* hash, void** key, size_t* key_len)
{
  return librdf_hash_get_seq(hash, key, key_len, LIBRDF_HASH_SEQUENCE_FIRST);
}


/**
 * librdf_hash_next - Return the 'next' hash key in a sequential access of the hash
 * @hash: hash object
 * @key: pointer to variable to store key pointer
 * @key_len: pointer to variable to store key length
 * 
 * This depends on the sequence being started by calling librdf_hash_first().
 *
 * Note: the key pointer will return newly allocated memory each time which
 * must be freed by the caller.
 *
 * See also librdf_hash_next() and librdf_hash_get_seq()
 * 
 * Return value: non 0 on failure
 **/
int
librdf_hash_next(librdf_hash* hash, void** key, size_t* key_len)
{
  return librdf_hash_get_seq(hash, key, key_len, LIBRDF_HASH_SEQUENCE_NEXT);
}



/**
 * librdf_hash_print - pretty print the hash to a file descriptor
 * @hash: the hash
 * @fh: file handle
 **/
void
librdf_hash_print (librdf_hash* hash, FILE *fh) 
{
  void *key;
  size_t key_len;
  void *value;
  size_t value_len;
  
  fprintf(fh, "%s hash: {\n", hash->factory->name);
  for(librdf_hash_first(hash, &key, &key_len);
      key;
      librdf_hash_next(hash, &key, &key_len)) {
    librdf_hash_get(hash, key, key_len, &value, &value_len, 0);
    fputs("  '", fh);
    fwrite(key, key_len, 1, fh);
    fputs("'=>'", fh);
    fwrite(value, value_len, 1, fh);
    fputs("'\n", fh);
    
    /* key and value points to new memory each time */
    LIBRDF_FREE(cstring, key);
    LIBRDF_FREE(cstring, value);
  }
  fputc('}', fh);
}


/* private enum */
typedef enum {
  HFS_PARSE_STATE_INIT = 0,
  HFS_PARSE_STATE_KEY = 1,
  HFS_PARSE_STATE_SEP = 2,
  HFS_PARSE_STATE_EQ = 3,
  HFS_PARSE_STATE_VALUE = 4,
  HFS_PARSE_STATE_VALUE_BQ = 5
} librdf_hfs_parse_state;

/**
 * librdf_hash_from_string - Initialise a hash from a string
 * @hash: hash object
 * @string: hash encoded as a string
 * 
 * The string format is something like:
 * key1='value1',key2='value2', key3='\'quoted value\''
 *
 * The 's are required and whitespace can appear around the = and ,s
 * 
 * Return value: non 0 on failure
 **/
int
librdf_hash_from_string (librdf_hash* hash, char *string) 
{
  char *p;
  char *key;
  size_t key_len;
  char *value;
  size_t value_len;
  int backslashes;
  librdf_hfs_parse_state state;
  int real_value_len;
  char *new_value;
  int i;
  char *to;

  
  LIBRDF_DEBUG2(librdf_hash_from_string, "Parsing >>%s<<\n", string);

  p=string;
  key=NULL; key_len=0;
  value=NULL; value_len=0;
  backslashes=0;
  state=HFS_PARSE_STATE_INIT;
  while(*p) {
    switch(state){
      /* start of config - before key */
      case HFS_PARSE_STATE_INIT:
        while(*p && (isspace((int)*p) || *p == ','))
          p++;
        if(!*p)
          break;
        key=p;
        /* fall through to next state */
        state=HFS_PARSE_STATE_KEY;
        
        /* collecting key characters */
      case HFS_PARSE_STATE_KEY:
        while(*p && isalnum((int)*p))
          p++;
        if(!*p)
          break;
        key_len=p-key;
        
        /* if 1st char is not space or alpha, move on */
        if(!key_len) {
          p++;
          state=HFS_PARSE_STATE_INIT;
          break;
        }
        
        state=HFS_PARSE_STATE_SEP;
        /* fall through to next state */
      
        /* got key, now skipping spaces */
      case HFS_PARSE_STATE_SEP:
        while(*p && isspace((int)*p))
          p++;
        if(!*p)
          break;
        /* expecting = now */
        if(*p != '=') {
          p++;
          state=HFS_PARSE_STATE_INIT;
          break;
        }
        p++;
        state=HFS_PARSE_STATE_EQ;
        /* fall through to next state */
        
        /* got key\s+= now skipping spaces " */
      case HFS_PARSE_STATE_EQ:
        while(*p && isspace((int)*p))
          p++;
        if(!*p)
          break;
        /* expecting ' now */
        if(*p != '\'') {
          p++;
          state=HFS_PARSE_STATE_INIT;
          break;
        }
        p++;
        value=p;
        backslashes=0;
        state=HFS_PARSE_STATE_VALUE;
        /* fall through to next state */
        
        /* got key\s+=\s+" now reading value */
      case HFS_PARSE_STATE_VALUE:
        while(*p) {
          if(*p == '\\') {
            /* backslashes are removed during value copy later */
            state=HFS_PARSE_STATE_VALUE_BQ;
            p++;
            break;
          } else if (*p != '\'') {
            /* keep searching for ' */
            p++;
            continue;
          }

          /* end of value found */
          value_len=p-value;
          real_value_len=value_len-backslashes;
          new_value=(char*)LIBRDF_MALLOC(cstring, real_value_len);
          if(!new_value)
            return 1;
          for(i=0, to=new_value; i<(int)value_len; i++){
            if(value[i]=='\\')
              i++;
            *to=value[i];
          }
          
          librdf_hash_put(hash, key, key_len, new_value, real_value_len, 0);
          
          LIBRDF_DEBUG1(librdf_hash_from_string,
                        "after decoding ");
#ifdef LIBRDF_DEBUG
          librdf_hash_print (hash, stderr) ;
          fputc('\n', stderr);
#endif
          state=HFS_PARSE_STATE_INIT;
          p++;
        }
        break;
        
      case HFS_PARSE_STATE_VALUE_BQ:
        if(!*p)
          break;
        backslashes++; /* reduces real length */
        p++;
        state=HFS_PARSE_STATE_VALUE;
        break;
        
      default:
        LIBRDF_FATAL2(librdf_hash_from_string, "No such state %d", state);
    }
  }
  return 0;
}


/**
 * librdf_hash_from_array_of_strings - Initialise a hash from an array of strings
 * @hash: hash object
 * @array: address of the start of the array of char* pointers
 * 
 * Return value: 
 **/
int
librdf_hash_from_array_of_strings (librdf_hash* hash, char **array) 
{
  int i;
  char *key;
  char *value;
  
  for(i=0; (key=array[i]); i+=2) {
    value=array[i+1];
    if(!value)
      LIBRDF_FATAL2(librdf_hash_from_array_of_strings,
                    "Array contains an odd number of strings - %d", i);
    librdf_hash_put(hash, key, strlen(key), value, strlen(value), 0);
  }
  return 0;
}




#ifdef STANDALONE

/* one more prototype */
int main(int argc, char *argv[]);


int
main(int argc, char *argv[]) 
{
  librdf_hash_factory *factory, *default_factory;
  librdf_hash *h, *h2;
  char *test_hash_types[]={"GDBM", "FAKE", "BDB", "memory", NULL};
  char *test_hash_values[]={"colour", "yellow",
			    "age", "new",
			    "size", "large",
			    "fruit", "banana",
			    NULL, NULL};
  char *test_hash_array[]={"shape", "cube",
			   "sides", "six",
			   "colours", "six",
			   "creator", "rubik",
			   NULL};
  char *test_hash_delete_key="size";
  int i,j;
  char *type;
  char *key, *value;
  char *program=argv[0];
  
  
  /* initialise hash module */
  librdf_init_hash();
  
  if(argc ==2) {
    type=argv[1];
    factory=librdf_get_hash_factory(NULL);
    if(!factory) {
      fprintf(stderr, "%s: No hash factory called '%s'\n", program, type);
      return(0);
    }
    
    h=librdf_new_hash(factory);
    if(!h) {
      fprintf(stderr, "%s: Failed to create new hash type '%s'\n",
	      program, type);
      return(0);
    }
    
    librdf_hash_open(h, "test", "mode", NULL);
    librdf_hash_from_string(h, argv[1]);
    fprintf(stderr, "%s: resulting ", program);    
    librdf_hash_print(h, stderr);
    fprintf(stderr, "\n");
    librdf_hash_close(h);
    librdf_free_hash(h);
    return(0);
  }
  
  
  for(i=0; (type=test_hash_types[i]); i++) {
    fprintf(stderr, "%s: Trying to create new %s hash\n", program, type);
    factory=librdf_get_hash_factory(type);
    if(!factory) {
      fprintf(stderr, "%s: No hash factory called '%s'\n", program, type);
      continue;
    }
    
    h=librdf_new_hash(factory);
    if(!h) {
      fprintf(stderr, "%s: Failed to create new hash type '%s'\n", program, type);
      continue;
    }

    /* delete DB file */
    remove("test");
    
    if(librdf_hash_open(h, "test", "mode", NULL)) {
      fprintf(stderr, "%s: Failed to open new hash type '%s'\n", program, type);
      continue;
    }
    
    
    for(j=0; test_hash_values[j]; j+=2) {
      key=test_hash_values[j];
      value=test_hash_values[j+1];
      fprintf(stderr, "%s: Adding key/value pair: %s=%s\n", program,
	      key, value);
      
      librdf_hash_put(h, key, strlen(key), value, strlen(value), 0);
      
      fprintf(stderr, "%s: resulting ", program);    
      librdf_hash_print(h, stderr);
      fprintf(stderr, "\n");
    }
    
    fprintf(stderr, "%s: Deleting key '%s'\n", program, test_hash_delete_key);
    librdf_hash_delete(h, test_hash_delete_key, strlen(test_hash_delete_key));
    
    fprintf(stderr, "%s: resulting ", program);    
    librdf_hash_print(h, stderr);
    fprintf(stderr, "\n");
    
    librdf_hash_close(h);
    
    fprintf(stderr, "%s: Freeing hash\n", program);
    librdf_free_hash(h);
  }
  
  
  fprintf(stderr, "%s: Getting default hash factory\n", program);
  default_factory=librdf_get_hash_factory(NULL);
  if(!default_factory) {
    fprintf(stderr, "%s: No default hash factory found\n", program);
    return(0);
  }

  fprintf(stderr, "%s: Creating new hash from default factory\n", program);
  h2=librdf_new_hash(default_factory);
  if(!h2) {
    fprintf(stderr, "%s: Failed to create new hash from default factory\n", program);
    return(0);
  }

  fprintf(stderr, "%s: Initialising hash from array of strings\n", program);
  if(librdf_hash_from_array_of_strings(h2, test_hash_array)) {
    fprintf(stderr, "%s: Failed to init hash from array of strings\n", program);
    return(0);
  }
  
  fprintf(stderr, "%s: resulting hash ", program);    
  librdf_hash_print(h2, stderr);
  fprintf(stderr, "\n");
  
  librdf_hash_close(h2);
  
  fprintf(stderr, "%s: Freeing hash\n", program);
  librdf_free_hash(h2);
  
  /* finish hash module */
  librdf_finish_hash();
  
  
#ifdef LIBRDF_DEBUG 
  librdf_memory_report(stderr);
#endif
  
  /* keep gcc -Wall happy */
  return(0);
}

#endif
