/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_utf8.c - RDF UTF8 / Unicode chars helper routines Implementation
 *
 * $Id$
 *
 * Copyright (C) 2000-2001 David Beckett - http://purl.org/net/dajobe/
 * Institute for Learning and Research Technology - http://www.ilrt.org/
 * University of Bristol - http://www.bristol.ac.uk/
 * 
 * This package is Free Software or Open Source available under the
 * following licenses (these are alternatives):
 *   1. GNU Lesser General Public License (LGPL) Version 2
 *   2. GNU General Public License (GPL) Version 2
 *   3. Mozilla Public License (MPL) Version 1.1
 * and no other versions of those licenses.
 * 
 * See INSTALL.html or INSTALL.txt at the top of this package for the
 * full license terms.
 * 
 */


#include <rdf_config.h>

#include <stdio.h>
#include <ctype.h> /* for isprint() */

#include <librdf.h>

#include <rdf_types.h>
#include <rdf_utf8.h>



/* UTF-8 encoding of 32 bit Unicode chars
 *
 * Characters  0x00000000 to 0x0000007f are US-ASCII
 * Characters  0x00000080 to 0x000000ff are ISO Latin 1 (ISO 8859-1)
 *
 * incoming char| outgoing
 * bytes | bits | representation
 * ==================================================
 *     1 |    7 | 0xxxxxxx
 *     2 |   11 | 110xxxxx 10xxxxxx
 *     3 |   16 | 1110xxxx 10xxxxxx 10xxxxxx
 *     4 |   21 | 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
 *     5 |   26 | 111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
 *     6 |   31 | 1111110x 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
 *
 * The first byte is always in the range 0xC0-0xFD
 * Further bytes are all in the range 0x80-0xBF
 * No byte is ever 0xFE or 0xFF
 *
*/


/**
 * librdf_unicode_char_to_utf8 - Convert a Unicode character to UTF-8 encoding
 * @c: Unicode character
 * @output: UTF-8 string buffer or NULL
 * @length: buffer size
 * 
 * If buffer is NULL, then will calculate the length rather than
 * perform it.  This can be used by the caller to allocate space
 * and then re-call this function with the new buffer.
 * 
 * Return value: bytes written to output buffer or <0 on failure
 **/
int
librdf_unicode_char_to_utf8(librdf_unichar c, byte *output, int length)
{
  int size=0;
  
  if      (c < 0x00000080)
    size=1;
  else if (c < 0x00000800)
    size=2;
  else if (c < 0x00010000)
    size=3;
  else if (c < 0x00200000)
    size=4;
  else if (c < 0x04000000)
    size=5;
  else if (c < 0x80000000)
    size=6;
  else
    return -1;

  /* when no buffer given, return size */
  if(!output)
    return size;

  if(size > length)
    return -1;
  
  switch(size) {
    case 6:
      output[5]=0x80 | (c & 0x3F);
      c= c >> 6;
       /* set bit 2 (bits 7,6,5,4,3,2 less 7,6,5,4,3 set below) on last byte */
      c |= 0x4000000; /* 0x10000 = 0x04 << 24 */
      /* FALLTHROUGH */
    case 5:
      output[4]=0x80 | (c & 0x3F);
      c= c >> 6;
       /* set bit 3 (bits 7,6,5,4,3 less 7,6,5,4 set below) on last byte */
      c |= 0x200000; /* 0x10000 = 0x08 << 18 */
      /* FALLTHROUGH */
    case 4:
      output[3]=0x80 | (c & 0x3F);
      c= c >> 6;
       /* set bit 4 (bits 7,6,5,4 less 7,6,5 set below) on last byte */
      c |= 0x10000; /* 0x10000 = 0x10 << 12 */
      /* FALLTHROUGH */
    case 3:
      output[2]=0x80 | (c & 0x3F);
      c= c >> 6;
      /* set bit 5 (bits 7,6,5 less 7,6 set below) on last byte */
      c |= 0x800; /* 0x800 = 0x20 << 6 */
      /* FALLTHROUGH */
    case 2:
      output[1]=0x80 | (c & 0x3F);
      c= c >> 6;
      /* set bits 7,6 on last byte */
      c |= 0xc0; 
      /* FALLTHROUGH */
    case 1:
      output[0]=c;
  }

  return size;
}



/**
 * librdf_utf8_to_unicode_char - Convert an UTF-8 encoded buffer to a Unicode character
 * @output: Pointer to the Unicode character or NULL
 * @input: UTF-8 string buffer
 * @length: buffer size
 * 
 * If output is NULL, then will calculate the number of bytes that
 * will be used from the input buffer and not perform the conversion.
 * 
 * Return value: bytes used from input buffer or <0 on failure
 **/
int
librdf_utf8_to_unicode_char(librdf_unichar *output, const byte *input, int length)
{
  byte in;
  int size;
  librdf_unichar c=0;
  
  if(length < 1)
    return -1;

  in=*input++;
  if((in & 0x80) == 0) {
    size=1;
    c= in & 0x7f;
  } else if((in & 0xe0) == 0xc0) {
    size=2;
    c= in & 0x1f;
  } else if((in & 0xf0) == 0xe0) {
    size=3;
    c= in & 0x0f;
  } else if((in & 0xf8) == 0xf0) {
    size=4;
    c = in & 0x07;
  } else if((in & 0xfc) == 0xf8) {
    size=5;
    c = in & 0x03;
  } else if((in & 0xfe) == 0xfc) {
    size=6;
    c = in & 0x01;
  } else
    return -1;


  if(!output)
    return size;

  if(length < size)
    return -1;

  switch(size) {
    case 6:
      in=*input++ & 0x3f;
      c= c << 6;
      c |= in;
      /* FALLTHROUGH */
    case 5:
      in=*input++ & 0x3f;
      c= c << 6;
      c |= in;
      /* FALLTHROUGH */
    case 4:
      in=*input++ & 0x3f;
      c= c << 6;
      c |= in;
      /* FALLTHROUGH */
    case 3:
      in=*input++ & 0x3f;
      c= c << 6;
      c |= in;
      /* FALLTHROUGH */
    case 2:
      in=*input++ & 0x3f;
      c= c << 6;
      c |= in;
      /* FALLTHROUGH */
    default:
      *output=c;
  }

  return size;
}


/**
 * librdf_utf8_to_latin1 - Convert a UTF-8 string to ISO Latin-1
 * @input: UTF-8 string buffer
 * @length: buffer size
 * @output_length: Pointer to variable to store resulting string length or NULL
 * 
 * Converts the given UTF-8 string to the ISO Latin-1 subset of
 * Unicode (characters 0x00-0xff), discarding any out of range
 * characters.
 *
 * If the output_length pointer is not NULL, the returned string
 * length will be stored there.
 *
 * Return value: pointer to new ISO Latin-1 string or NULL on failure
 **/
byte*
librdf_utf8_to_latin1(const byte *input, int length, int *output_length)
{
  int utf8_char_length=0;
  int utf8_byte_length=0;
  int i;
  int j;
  byte *output;

  i=0;
  while(input[i]) {
    int size=librdf_utf8_to_unicode_char(NULL, &input[i], length-i);
    if(size <= 0)
      return NULL;
    utf8_char_length++;
    i+= size;
  }

  /* This is a maximal length; since chars may be discarded, the
   * actual length of the resulting can be shorter
   */
  utf8_byte_length=i;


  output=(byte*)LIBRDF_MALLOC(cstring, utf8_byte_length+1);
  if(!output)
    return NULL;
  

  i=0; j=0;
  while(i < utf8_byte_length) {
    librdf_unichar c;
    int size=librdf_utf8_to_unicode_char(&c, &input[i], length-i);
    if(size <= 0)
      return NULL;
    if(c < 0x100) /* Discards characters! */
      output[j++]=c;
    i+= size;
  } 
  output[j]='\0';

  if(output_length)
    *output_length=j;
  
  return output;
}


/**
 * librdf_latin1_to_utf8 - Convert an ISO Latin-1 encoded string to UTF-8
 * @input: ISO Latin-1 string buffer
 * @length: buffer size
 * @output_length: Pointer to variable to store resulting string length or NULL
 * 
 * Converts the given ISO Latin-1 string to an UTF-8 encoded string
 * representing the same content.  This is lossless.
 * 
 * If the output_length pointer is not NULL, the returned string
 * length will be stored there.
 *
 * Return value: pointer to new UTF-8 string or NULL on failure
 **/
byte*
librdf_latin1_to_utf8(const byte *input, int length, int *output_length)
{
  int utf8_length=0;
  int i;
  int j;
  byte *output;

  for(i=0; input[i]; i++) {
    int size=librdf_unicode_char_to_utf8(input[i], NULL, length-i);
    if(size <= 0)
      return NULL;
    utf8_length += size;
  }

  output=(byte*)LIBRDF_MALLOC(cstring, utf8_length+1);
  if(!output)
    return NULL;
  

  j=0;
  for(i=0; input[i]; i++) {
    int size=librdf_unicode_char_to_utf8(input[i], &output[j], length-i);
    if(size <= 0)
      return NULL;
    j+= size;
  } 
  output[j]='\0';

  if(output_length)
    *output_length=j;
  
  return output;
}


/**
 * librdf_utf8_print - Print a UTF-8 string to a stream
 * @input: UTF-8 string buffer
 * @length: buffer size
 * @stream: FILE* stream
 * 
 * Pretty prints the UTF-8 string in a pseudo-C character
 * format like \u<hex digits> when the characters fail
 * the isprint() test.
 **/
void
librdf_utf8_print(const byte *input, int length, FILE *stream)
{
  int i=0;
  
  while(*input) {
    librdf_unichar c;
    int size=librdf_utf8_to_unicode_char(&c, input, length-i);
    if(size <= 0)
      return;
    if(c < 0x100) {
      if(isprint(c))
        fputc(c, stream);
      else
        fprintf(stream, "\\u%02X", c);
    } else if (c < 0x10000)
      fprintf(stream, "\\u%04X", c);
    else
      fprintf(stream, "\\u%08X", c);
    input += size;
    i += size;
  }
}




#ifdef STANDALONE

/* one more prototype */
int main(int argc, char *argv[]);


int
main(int argc, char *argv[]) 
{
  char *program=argv[0];
  librdf_unichar c;
  struct tv {
    const byte *string;
    const int length;
    const librdf_unichar result;
  };
  struct tv *t;
  struct tv test_values[]={
    /* what is the capital of England? 'E' */
    {"E", 1, 'E'},
    /* latin small letter e with acute, U+00E9 ISOlat1 */
    {"\xc3\xa9", 2, 0xE9},
    /*  euro sign, U+20AC NEW */
    {"\xe2\x82\xac", 3, 0x20AC}, 
    /* unknown char - U+1FFFFF (21 bits) */
    {"\xf7\xbf\xbf\xbf", 4, 0x1FFFFF},
    /* unknown char - U+3FFFFFF (26 bits) */
    {"\xfb\xbf\xbf\xbf\xbf", 5, 0x3FFFFFF},
    /* unknown char - U+7FFFFFFF (31 bits) */
    {"\xfd\xbf\xbf\xbf\xbf\xbf", 6, 0x7FFFFFFF},
    {NULL, 0, 0}
  };
  const byte test_utf8_string[]="Lib" "\xc3\xa9" "ration costs " "\xe2\x82\xac" "3.50";
  int test_utf8_string_length=strlen(test_utf8_string);
  const byte result_latin1_string[]="Lib" "\xe9" "ration costs 3.50";
  int result_latin1_string_length=strlen(result_latin1_string);
  const byte result_utf8_string[]="Lib" "\xc3\xa9" "ration costs 3.50";
  int result_utf8_string_length=strlen(result_utf8_string);
  
  int i;
  byte *latin1_string;
  int latin1_string_length;
  byte *utf8_string;
  int utf8_string_length;
  

  for(i=0; (t=&test_values[i]) && t->string; i++) {
    int size;
    const byte *buffer=t->string;
    int length=t->length;
#define OUT_BUFFER_SIZE 6
    byte out_buffer[OUT_BUFFER_SIZE];
    
    size=librdf_utf8_to_unicode_char(&c, buffer, length);
    if(size < 0) {
      fprintf(stderr, "%s: librdf_utf8_to_unicode_char failed to convert UTF-8 string '", program);
      librdf_utf8_print(buffer, length, stderr);
      fprintf(stderr, "' (length %d) to Unicode\n", length);
      return(1);
    }
    if(c != t->result) {
      fprintf(stderr, "%s: librdf_utf8_to_unicode_char failed conversion of UTF-8 string '", program);
      librdf_utf8_print(buffer, size, stderr);
      fprintf(stderr, "' to Unicode char U+%04X, expected U+%04X\n",
              (u32)c, (u32)t->result);
      return(1);
    }
    
    fprintf(stderr, "%s: librdf_utf8_to_unicode_char converted UTF-8 string '", program);
    librdf_utf8_print(buffer, size, stderr);
    fprintf(stderr, "' to Unicode char U+%04X correctly\n", (u32)c);

    size=librdf_unicode_char_to_utf8(t->result, out_buffer, OUT_BUFFER_SIZE);
    if(size <= 0) {
      fprintf(stderr, "%s: librdf_unicode_char_to_utf8 failed to convert U+%04X to UTF-8 string\n", program, (u32)t->result);
      return(1);
    }

    if(memcmp(out_buffer, buffer, length)) {
      fprintf(stderr, "%s: librdf_unicode_char_to_utf8 failed conversion U+%04X to UTF-8 - returned '", program, (u32)t->result);
      librdf_utf8_print(buffer, size, stderr);
      fputs("', expected '", stderr);
      librdf_utf8_print(out_buffer, t->length, stderr);
      fputs("'\n", stderr);
      return(1);
    }
    
    fprintf(stderr, "%s: librdf_unicode_char_to_utf8 converted U+%04X to UTF-8 string '", program, (u32)t->result);
    librdf_utf8_print(out_buffer, size, stderr);
    fputs("' correctly\n", stderr);
  }


  latin1_string=librdf_utf8_to_latin1(test_utf8_string, 
                                      test_utf8_string_length,
                                      &latin1_string_length);
  if(!latin1_string) {
    fprintf(stderr, "%s: librdf_utf8_to_latin1 failed to convert UTF-8 string '", program);
    librdf_utf8_print(test_utf8_string, test_utf8_string_length, stderr);
    fputs("' to Latin-1\n", stderr);
    return(1);
  }

  if(memcmp(latin1_string, result_latin1_string, result_latin1_string_length)) {
    fprintf(stderr, "%s: librdf_utf8_to_latin1 failed to convert UTF-8 string '", program);
    librdf_utf8_print(test_utf8_string, test_utf8_string_length, stderr);
    fprintf(stderr, "' to Latin-1 - returned '%s' but expected '%s'\n",
            latin1_string, result_latin1_string);
    LIBRDF_FREE(cstring, latin1_string);
    return(1);
  }

  fprintf(stderr, "%s: librdf_utf8_to_latin1 converted UTF-8 string '",
          program);
  librdf_utf8_print(test_utf8_string, test_utf8_string_length, stderr);
  fprintf(stderr, "' to Latin-1 string '%s' OK\n", latin1_string);
  

  utf8_string=librdf_latin1_to_utf8(latin1_string, latin1_string_length,
                                    &utf8_string_length);
  if(!utf8_string) {
    fprintf(stderr, "%s: librdf_latin1_to_utf8 failed to convert Latin-1 string '%s' to UTF-8\n", program, latin1_string);
    LIBRDF_FREE(cstring, latin1_string);
    return(1);
  }

  if(memcmp(utf8_string, result_utf8_string, result_utf8_string_length)) {
    fprintf(stderr, "%s: librdf_latin1_to_utf8 failed to convert Latin-1 string '%s' to UTF-8 - returned '", program, latin1_string);
    librdf_utf8_print(utf8_string, utf8_string_length, stderr);
    fputs("' but expected '", stderr);
    librdf_utf8_print(result_utf8_string, result_utf8_string_length, stderr);
    fputs("'\n", stderr);
    LIBRDF_FREE(cstring, latin1_string);
    LIBRDF_FREE(cstring, utf8_string);
    return(1);
  }

  fprintf(stderr, "%s: librdf_latin1_to_utf8 converted Latin-1 string '%s' to UTF-8 string '", program, latin1_string);
  librdf_utf8_print(utf8_string, utf8_string_length, stderr);
  fputs("' OK\n", stderr);
  

  LIBRDF_FREE(cstring, latin1_string);
  LIBRDF_FREE(cstring, utf8_string);

#ifdef LIBRDF_MEMORY_DEBUG 
  librdf_memory_report(stderr);
#endif
 
  /* keep gcc -Wall happy */
  return(0);
}

#endif
