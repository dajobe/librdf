/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_hash_memory.h - RDF Hash In Memory Interface definition
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



#ifndef LIBRDF_HASH_MEMORY_H
#define LIBRDF_HASH_MEMORY_H

#ifdef __cplusplus
extern "C" {
#endif


void librdf_init_hash_memory(int default_load_factor);


#ifdef __cplusplus
}
#endif

#endif
