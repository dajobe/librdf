/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_storage_mysql.h - RDF Storage in MySQL DB interface definition.
 *
 * $Id$
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
 * 
 */



#ifndef LIBRDF_STORAGE_MYSQL_H
#define LIBRDF_STORAGE_MYSQL_H

#ifdef __cplusplus
extern "C" {
#endif


#ifdef HAVE_MYSQL
void librdf_init_storage_mysql(void);
#endif


#ifdef __cplusplus
}
#endif

#endif
