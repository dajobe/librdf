/* rss2ical.c: Turn RSS into ical 
 *
 * Copyright (C) 2006, David Beckett http://purl.org/net/dajobe/
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
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <redland.h>


#undef RSS2ICAL_DEBUG

#define DO_ESCAPE_NL 0

static const unsigned char* get_items_query=(const unsigned char*)
"PREFIX rss: <http://purl.org/rss/1.0/>\n\
PREFIX dc: <http://purl.org/dc/elements/1.1/>\n\
PREFIX content: <http://web.resource.org/rss/1.0/modules/content/>\n\
SELECT ?item ?date ?title ?description ?htmldesc ?source ?creator\n\
WHERE {\n\
 ?item a rss:item;\n\
       dc:date ?date;\n\
       rss:title ?title;\n\
       rss:description ?description;\n\
       dc:creator ?creator;\n\
 OPTIONAL { ?item  dc:source ?source } \n\
}";

/*
  Removed for now:
  OPTIONAL { ?item  content:encoded ?htmldesc } \n      \
*/

/* %s-prod id %s-cal name id %s-rel cal id %s-tzone %s-tzone */
static const char *ical_header_format="\
BEGIN:VCALENDAR\n\
VERSION:2.0\n\
PRODID:%s\n\
X-WR-CALNAME:%s\n\
X-WR-RELCALID:%s\n\
X-WR-TIMEZONE:%s\n\
CALSCALE:GREGORIAN\n\
METHOD:PUBLISH\n\
BEGIN:VTIMEZONE\n\
TZID:%s\n\
BEGIN:DAYLIGHT\n\
DTSTART:20060326T020000\n\
TZOFFSETTO:+0100\n\
TZOFFSETFROM:+0000\n\
TZNAME:BST\n\
END:DAYLIGHT\n\
BEGIN:STANDARD\n\
DTSTART:20061029T020000\n\
TZOFFSETTO:+0000\n\
TZOFFSETFROM:+0100\n\
TZNAME:GMT\n\
END:STANDARD\n\
END:VTIMEZONE\n\
";

static const char *ical_footer_format="\
END:VCALENDAR\n\
";

static const char *tzone="Europe/London";

static  char *program=NULL;


static void
ical_print(FILE *fh, const char *line)  
{
  fputs(line, fh);
  fwrite("\n", 1, 1, fh);
}


static void
ical_format(FILE *fh, const char *key, const char *attr,
            const char *escapes, const unsigned char *value) 
{
  int col=0;
  int i=0;
  size_t len;
  int c;
  int lineno=0;
  
  len=strlen(key);
  fwrite(key, 1, len, fh);
  col += len;

  if(attr) {
    fputc(';', fh);
    col++;
    len=strlen(attr);
    fwrite(attr, 1, len, fh);
    col += len;
  }
  
  fputc(':', fh);
  col++;

  for(i=0; (c=value[i]); i++)  {
    if(col == 75) {
      fwrite("\n ", 1, 2, fh);
      col=0;
      lineno++;
    }
    if(c == '\\' || 
       (escapes && (strchr(escapes, c) != NULL))) {
      fputc('\\', fh);
      col++;
    }
    if(c == '\n') {
#ifdef DO_ESCAPE_NL
      fputc('\\', fh);
      col++;
      c='n';
#else
      c=' ';
#endif
    }
    fputc(c, fh);
    col++;
  }
  fputc('\n', fh);
}


static
unsigned char* iso2vcaldate(const unsigned char* iso_date) 
{
  unsigned char* vcaldate;
  unsigned char c;
  int i, j;
  
  /* YYYY-MM-DDTHH:MM:SSZ to YYYYMMDDTHHMMSSZ */
  vcaldate=(unsigned char*)malloc(17);
  for(i=0, j=0; (c=iso_date[i]); i++) {
    if(c != ':' && c != '-')
      vcaldate[j++]=iso_date[i];
  }
  vcaldate[j]='\0';

  return vcaldate;
}



static
unsigned char* remove_html_entities(unsigned char* html_desc, size_t len)
{
  int i, j;
  unsigned char* description;
  unsigned char c;
  
  description=malloc(len);
  for(i=0, j=0; (c=html_desc[i]); i++) {
    if(c == '\n')
      c=' ';
    else if(c == '&') {
      c=html_desc[++i];
      
      /* Expand &#123; to UTF-8 for codepoint decimal 123 */
      if(c == '#') {
        unsigned char *orig_p=&html_desc[i];
        unsigned long d=0;
        int ulen;
        
        i++;
        while(c) { 
          c=html_desc[i++];
          if(c<'0' || c>'9')
            break;
          d=d*10;
          d+= (c - '0');
        }
        if(c != ';') {
          fprintf(stderr, "%s: Expected ';' after &#NNN in '%s'\n",
                  program, orig_p);
          abort();
        }

#ifdef RSS2ICAL_DEBUG
        fprintf(stderr, "%s: Encoding char %d\n", program, d);
#endif
        ulen=raptor_unicode_char_to_utf8(d, &description[j]);
#ifdef RSS2ICAL_DEBUG
        fprintf(stderr, "%s: UTF-8 len was %d\n", program, ulen);
#endif
        j+= ulen;
        
      } else {
        const char* here=(const char*)&html_desc[i];

        if(!strncmp(here, "amp;", 4)) {
          i+= 4;
          c='&';
        } else if(!strncmp(here, "lt;", 3)) {
          i+= 3;
          c='<';
        } else if(!strncmp(here, "gt;", 3)) {
          i+= 3;
          c='>';
        }
          
        description[j++]=c;
      }
      continue;
    }
    
    description[j++]=c;
  }
  description[j]='\0';
  return description;
}


int
main(int argc, char *argv[])
{
  librdf_world* world;
  librdf_storage* storage;
  librdf_model* model;
  librdf_parser* parser;
  librdf_query* query;
  librdf_query_results* results;
  librdf_uri *uri;
  char *p;
  char* calendar_name;
  
  program=argv[0];
  if((p=strrchr(program, '/')))
    program=p+1;
  else if((p=strrchr(program, '\\')))
    program=p+1;
  argv[0]=program;

  if(argc != 3) {
    fprintf(stderr, "USAGE: %s RSS-URI CALENDAR-NAME\n", program);
    return 1; 
  }
  
  world=librdf_new_world();
  librdf_world_open(world);

  storage=librdf_new_storage(world, "memory", NULL, NULL);
  model=librdf_new_model(world, storage, NULL);

  if(!model || !storage) {
    fprintf(stderr, "%s: Failed to make model or storage\n", program);
    return 1;
  }

  uri=librdf_new_uri(world, (unsigned char*)argv[1]);

  calendar_name=argv[2];

  fprintf(stderr, "%s: Reading RSS from %s\n", program,
          librdf_uri_as_string(uri));
  
  parser=librdf_new_parser(world, "rss-tag-soup", NULL, NULL);
  librdf_parser_parse_into_model(parser, uri, NULL, model);
  librdf_free_parser(parser);

  fprintf(stderr, "%s: Querying model for RSS items\n", program);
  
  query=librdf_new_query(world, "sparql", NULL, get_items_query, uri);
  
  results=librdf_model_query_execute(model, query);
  if(!results) {
    fprintf(stderr, "%s: Query of model with SPARQL query '%s' failed\n", 
            program, get_items_query);
    return 1;
  }

  fprintf(stderr, "%s: Processing results\n", program);

  fprintf(stdout, ical_header_format,
          "-//librdf/rss2ical Version 1.0//EN",
          calendar_name, 
          "8ED1A8D-8E65",
          tzone,
          tzone);

  while(!librdf_query_results_finished(results)) {
    unsigned char *item_uri=NULL;
    unsigned char *uid=NULL;
    unsigned char *summary=NULL;
    unsigned char *dtstart=NULL;
    unsigned char *location=NULL;
    unsigned char *html_desc=NULL;
    size_t html_desc_len;
    unsigned char *description=NULL;
    unsigned char *url=NULL;
    librdf_node* node;
    int i;
    unsigned char c;
    char *creator=NULL;

    node=librdf_query_results_get_binding_value_by_name(results, "item");
    if(!librdf_node_is_resource(node))
      break;

    url=librdf_uri_as_string(librdf_node_get_uri(node));

    /* uid is a new string */
    item_uri=librdf_uri_to_string(librdf_node_get_uri(node));

    /* 7=strlen("http://") */
    uid=item_uri+7;
    for(i=0; (c=uid[i]); i++) {
      if(c == '/' || c == '#' || c == '?')
        uid[i]='-';
    }
    

    node=librdf_query_results_get_binding_value_by_name(results, "date");
    if(!librdf_node_is_literal(node))
      break;
    dtstart=librdf_node_get_literal_value(node);
    dtstart=iso2vcaldate(dtstart);
    
    node=librdf_query_results_get_binding_value_by_name(results, "title");
    if(!librdf_node_is_literal(node))
      summary=(unsigned char*)"(No Title)";
    else
      summary=librdf_node_get_literal_value(node);

    node=librdf_query_results_get_binding_value_by_name(results, "htmldesc");
    if(node && librdf_node_is_literal(node))
        html_desc=librdf_node_get_literal_value_as_counted_string(node, 
                                                                  &html_desc_len);
    
    if(!description) {
      node=librdf_query_results_get_binding_value_by_name(results, "description");
      if(node && librdf_node_is_literal(node))
        html_desc=librdf_node_get_literal_value_as_counted_string(node, 
                                                                  &html_desc_len);
    }
    if(html_desc) {
      description=remove_html_entities(html_desc, html_desc_len);
    }
    
    node=librdf_query_results_get_binding_value_by_name(results, "source");
    if(node && librdf_node_is_literal(node))
      location=librdf_node_get_literal_value(node);

    node=librdf_query_results_get_binding_value_by_name(results, "creator");
    if(node && librdf_node_is_literal(node)) {
      unsigned char *value=librdf_node_get_literal_value(node);
      creator=malloc(strlen((const char*)value)+6);
      sprintf(creator, "CN=\"%s\"", value);
    }
    

    ical_print(stdout, "BEGIN:VEVENT");
    ical_format(stdout, "UID", NULL, NULL, uid);
    ical_format(stdout, "SUMMARY", NULL, NULL, summary);
    if(location)
      ical_format(stdout, "LOCATION", NULL, NULL, location);
    if(creator) {
      ical_format(stdout, "ATTENDEE", creator, NULL, 
                  (const unsigned char*)"invalid:nomail");
      free(creator);
    }
    ical_format(stdout, "DTSTART", NULL, NULL, dtstart);
    ical_format(stdout, "DTSTAMP", NULL, NULL, dtstart);
    ical_format(stdout, "LAST-MODIFIED", NULL, NULL, dtstart);
    ical_format(stdout, "DESCRIPTION", NULL, ";,\"", description);
    if(url)
      ical_format(stdout, "URL", "VALUE=URI", ";", url);
    ical_format(stdout, "CLASS", NULL, NULL, (const unsigned char*)"PUBLIC");

    ical_print(stdout, "END:VEVENT");

    free(description);

    free(item_uri);
    
    librdf_query_results_next(results);
  }

  fputs(ical_footer_format, stdout);

  librdf_free_query_results(results);
  librdf_free_query(query);

  librdf_free_uri(uri);

  librdf_free_model(model);
  librdf_free_storage(storage);

  librdf_free_world(world);

  /* keep gcc -Wall happy */
  return(0);
}
