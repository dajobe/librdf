/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_heuristics.h - Heuristic routines to guess things about RDF prototypes
 *
 * $Id$
 *
 * Copyright (C) 2000-2001 David Beckett - http://purl.org/net/dajobe/
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
 * 
 */


#ifndef LIBRDF_HEURISTICS_H
#define LIBRDF_HEURISTICS_H

#ifdef __cplusplus
extern "C" {
#endif

char* librdf_heuristic_gen_name(char *name);
int librdf_heuristic_object_is_literal(char *object);

#ifdef __cplusplus
}
#endif

#endif
