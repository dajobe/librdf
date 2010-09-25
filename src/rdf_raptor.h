/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_raptor.h - librdf Raptor integration
 *
 * Copyright (C) 2008, David Beckett http://www.dajobe.org/
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



#ifndef LIBRDF_RAPTOR_H
#define LIBRDF_RAPTOR_H

#ifdef LIBRDF_INTERNAL
#include <rdf_raptor_internal.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

REDLAND_API
void librdf_world_set_raptor(librdf_world* world, raptor_world* raptor_world_ptr);
REDLAND_API
raptor_world* librdf_world_get_raptor(librdf_world* world);


#ifdef __cplusplus
}
#endif

#endif
