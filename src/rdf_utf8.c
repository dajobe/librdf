/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_utf8.c - RDF UTF8 / Unicode chars helper routines Implementation
 *
 * Copyright (C) 2000-2008, David Beckett http://www.dajobe.org/
 * Copyright (C) 2000-2004, University of Bristol, UK http://www.bristol.ac.uk/
 * 
 * This package is Free Software and part of Redland http://librdf.org/
 * 
 * It is licensed under the following three licenses as alternatives:
 *   1. GNU Lesser General Public License (LGPL) V2.1 or any newer version
 *   2. GNU General Public License (GPL) V2 or any newer version
 *   3. Apache License, V2.0 or any newer version
 * 
 * You may not use this file except in compliance with at least one of
 * the above three licenses.
 * 
 * See LICENSE.html or LICENSE.txt at the top of this package for the
 * complete terms and further detail along with the license texts for
 * the licenses in COPYING.LIB, COPYING and LICENSE-2.0.txt respectively.
 * 
 * 
 */


#ifdef HAVE_CONFIG_H
#include <rdf_config.h>
#endif

#ifdef WIN32
#include <win32_rdf_config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <ctype.h> /* for isprint() */

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include <redland.h>
#include <rdf_utf8.h>


#ifndef STANDALONE

/**
 * librdf_unicode_char_to_utf8:
 * @c: Unicode character
 * @output: UTF-8 string buffer or NULL
 * @length: buffer size
 *
 * Convert a Unicode character to UTF-8 encoding.
 * 
 * @deprecated: Use raptor_unicode_utf8_string_put_char()
 *
 * If buffer is NULL, then will calculate the length rather than
 * perform it.  This can be used by the caller to allocate space
 * and then re-call this function with the new buffer.
 * 
 * Return value: bytes written to output buffer or <0 on failure
 **/
int
librdf_unicode_char_to_utf8(librdf_unichar c, unsigned char *output, int length)
{
  return raptor_unicode_utf8_string_put_char((raptor_unichar)c, output,
                                             (size_t)length);
}



/**
 * librdf_utf8_to_unicode_char:
 * @output: Pointer to the Unicode character or NULL
 * @input: UTF-8 string buffer
 * @length: buffer size
 *
 * Convert an UTF-8 encoded buffer to a Unicode character.
 * 
 * @deprecated: Use raptor_unicode_utf8_string_get_char() noting that the arg order has changed to input, length, output.
 *
 * If @output is NULL, then will calculate the number of bytes that
 * will be used from the input buffer and not perform the conversion.
 * 
 * Return value: bytes used from input buffer or <0 on failure
 **/
int
librdf_utf8_to_unicode_char(librdf_unichar *output, const unsigned char *input,
                            int length)
{
  return raptor_unicode_utf8_string_get_char(input, length, output);
}


/**
 * librdf_utf8_to_latin1:
 * @input: UTF-8 string buffer
 * @length: buffer size
 * @output_length: Pointer to variable to store resulting string length or NULL
 *
 * Convert a UTF-8 string to ISO Latin-1.
 * 
 * Converts the given UTF-8 string to the ISO Latin-1 subset of
 * Unicode (characters 0x00-0xff), discarding any out of range
 * characters.
 *
 * If the @output_length pointer is not NULL, the returned string
 * length will be stored there.
 *
 * Return value: pointer to new ISO Latin-1 string or NULL on failure
 **/
unsigned char*
librdf_utf8_to_latin1(const unsigned char *input, int length,
                      int *output_length)
{
  int utf8_char_length = 0;
  int utf8_byte_length = 0;
  int i;
  int j;
  unsigned char *output;

  i = 0;
  while(input[i]) {
    int size = raptor_unicode_utf8_string_get_char(&input[i], length-i, NULL);
    if(size <= 0)
      return NULL;
    utf8_char_length++;

    i += size;
  }

  /* This is a maximal length; since chars may be discarded, the
   * actual length of the resulting can be shorter
   */
  utf8_byte_length = i;


  output = LIBRDF_MALLOC(unsigned char*, utf8_byte_length + 1);
  if(!output)
    return NULL;
  

  i = 0; j = 0;
  while(i < utf8_byte_length) {
    librdf_unichar c;
    int size = raptor_unicode_utf8_string_get_char(&input[i], length - i, &c);
    if(size <= 0) {
      LIBRDF_FREE(byte_string, output);
      return NULL;
    }

    if(c < 0x100) /* Discards characters! */
      output[j++] = c;
    i += size;
  } 
  output[j] = '\0';

  if(output_length)
    *output_length = j;
  
  return output;
}


/**
 * librdf_latin1_to_utf8:
 * @input: ISO Latin-1 string buffer
 * @length: buffer size
 * @output_length: Pointer to variable to store resulting string length or NULL
 *
 * Convert an ISO Latin-1 encoded string to UTF-8.
 * 
 * Converts the given ISO Latin-1 string to an UTF-8 encoded string
 * representing the same content.  This is lossless.
 * 
 * If the output_length pointer is not NULL, the returned string
 * length will be stored there.
 *
 * Return value: pointer to new UTF-8 string or NULL on failure
 **/
unsigned char*
librdf_latin1_to_utf8(const unsigned char *input, int length,
                      int *output_length)
{
  int utf8_length = 0;
  int i;
  int j;
  unsigned char *output;

  for(i = 0; input[i]; i++) {
    int size = raptor_unicode_utf8_string_put_char(input[i], NULL,
                                                   (size_t)(length - i));
    if(size <= 0)
      return NULL;
    utf8_length += size;
  }

  output = LIBRDF_MALLOC(unsigned char*, utf8_length + 1);
  if(!output)
    return NULL;
  

  j = 0;
  for(i = 0; input[i]; i++) {
    int size = raptor_unicode_utf8_string_put_char(input[i], &output[j],
                                                   (size_t)(length - i));
    if(size <= 0) {
      LIBRDF_FREE(byte_string, output);
      return NULL;
    }
    j += size;
  } 
  output[j] = '\0';

  if(output_length)
    *output_length=j;
  
  return output;
}


/**
 * librdf_utf8_print:
 * @input: UTF-8 string buffer
 * @length: buffer size
 * @stream: FILE* stream
 *
 * Print a UTF-8 string to a stream.
 * 
 * Pretty prints the UTF-8 string in a pseudo-C character
 * format like \u<emphasis>hex digits</emphasis> when the characters fail
 * the isprint() test.
 **/
void
librdf_utf8_print(const unsigned char *input, int length, FILE *stream)
{
  int i = 0;
  
  while(i < length && *input) {
    librdf_unichar c;
    int size = raptor_unicode_utf8_string_get_char(input, length - i, &c);
    if(size <= 0)
      return;

    if(c < 0x100) {
      int cchar = (int)c;
      if(isprint(cchar))
        fputc(cchar, stream);
      else
        fprintf(stream, "\\u%02X", cchar);
    } else if (c < 0x10000)
      fprintf(stream, "\\u%04X", (unsigned int)c);
    else
      fprintf(stream, "\\U%08X", (unsigned int)c);
    input += size;
    i += size;
  }
}

#endif


/* TEST CODE */


#ifdef STANDALONE

/* static prototypes */
void librdf_bad_string_print(const unsigned char *input, int length, FILE *stream);
int main(int argc, char *argv[]);

void
librdf_bad_string_print(const unsigned char *input, int length, FILE *stream)
{
  while(*input && length>0) {
    char c=*input;
    if(isprint(c))
      fputc(c, stream);
    else
      fprintf(stream, "\\x%02X", (c & 0xff));
    input++;
    length--;
  }
}


int
main(int argc, char *argv[]) 
{
  const char *program = librdf_basename((const char*)argv[0]);
  librdf_unichar c;
  const unsigned char test_utf8_string[] = "Lib" "\xc3\xa9" "ration costs " "\xe2\x82\xac" "3.50";
  int test_utf8_string_length = strlen((const char*)test_utf8_string);
  const unsigned char result_latin1_string[] = "Lib" "\xe9" "ration costs 3.50";
  int result_latin1_string_length=strlen((const char*)result_latin1_string);
  const unsigned char result_utf8_string[] = "Lib" "\xc3\xa9" "ration costs 3.50";
  int result_utf8_string_length = strlen((const char*)result_utf8_string);
  
  int i;
  unsigned char *latin1_string;
  int latin1_string_length;
  unsigned char *utf8_string;
  int utf8_string_length;
  int failures = 0;
  int verbose = 0;

  latin1_string=librdf_utf8_to_latin1(test_utf8_string, 
                                      test_utf8_string_length,
                                      &latin1_string_length);
  if(!latin1_string) {
    fprintf(stderr, "%s: librdf_utf8_to_latin1 FAILED to convert UTF-8 string '", program);
    librdf_bad_string_print(test_utf8_string, test_utf8_string_length, stderr);
    fputs("' to Latin-1\n", stderr);
    failures++;
  }

  if(memcmp(latin1_string, result_latin1_string, result_latin1_string_length)) {
    fprintf(stderr, "%s: librdf_utf8_to_latin1 FAILED to convert UTF-8 string '", program);
    librdf_utf8_print(test_utf8_string, test_utf8_string_length, stderr);
    fprintf(stderr, "' to Latin-1 - returned '%s' but expected '%s'\n",
            latin1_string, result_latin1_string);
    failures++;
  }

  if(verbose) {
    fprintf(stderr, "%s: librdf_utf8_to_latin1 converted UTF-8 string '",
            program);
    librdf_utf8_print(test_utf8_string, test_utf8_string_length, stderr);
    fprintf(stderr, "' to Latin-1 string '%s' OK\n", latin1_string);
  }
  

  utf8_string=librdf_latin1_to_utf8(latin1_string, latin1_string_length,
                                    &utf8_string_length);
  if(!utf8_string) {
    fprintf(stderr, "%s: librdf_latin1_to_utf8 FAILED to convert Latin-1 string '%s' to UTF-8\n", program, latin1_string);
    failures++;
  }

  if(memcmp(utf8_string, result_utf8_string, result_utf8_string_length)) {
    fprintf(stderr, "%s: librdf_latin1_to_utf8 FAILED to convert Latin-1 string '%s' to UTF-8 - returned '", program, latin1_string);
    librdf_utf8_print(utf8_string, utf8_string_length, stderr);
    fputs("' but expected '", stderr);
    librdf_utf8_print(result_utf8_string, result_utf8_string_length, stderr);
    fputs("'\n", stderr);
    failures++;
  }

  if(verbose) {
    fprintf(stderr, "%s: librdf_latin1_to_utf8 converted Latin-1 string '%s' to UTF-8 string '", program, latin1_string);
    librdf_utf8_print(utf8_string, utf8_string_length, stderr);
    fputs("' OK\n", stderr);
  }

  LIBRDF_FREE(char*, latin1_string);
  LIBRDF_FREE(char*, utf8_string);

#ifdef LIBRDF_MEMORY_DEBUG 
  librdf_memory_report(stderr);
#endif
 
  return failures;
}

#endif
