/* -*- Mode: java -*-
 *
 * PrintParser.java - Use streaming SiRPAC to parse RDF/XML content to triples
 *
 * $Id$
 *
 * Copyright (C) 2000 David Beckett - http://purl.org/net/dajobe/
 * Institute for Learning and Research Technology, University of Bristol.
 *
 *    This package is Free Software available under either of two licenses
 *    (see FAQS.html to see why):
 * 
 * 1. The GNU Lesser General Public License (LGPL)
 * 
 *    See http://www.gnu.org/copyleft/lesser.html or COPYING.LIB for the
 *    full license text.
 *      _________________________________________________________________
 * 
 *      Copyright (C) 2000 David Beckett, Institute for Learning and
 *      Research Technology, University of Bristol. All Rights Reserved.
 * 
 *      This library is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU Lesser General Public License
 *      as published by the Free Software Foundation; either version 2 of
 *      the License, or (at your option) any later version.
 * 
 *      This library is distributed in the hope that it will be useful, but
 *      WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 *      Lesser General Public License for more details.
 * 
 *      You should have received a copy of the GNU Lesser General Public
 *      License along with this library; if not, write to the Free Software
 *      Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 *      USA
 *      _________________________________________________________________
 * 
 *    NOTE - under Term 3 of the LGPL, you may choose to license the entire
 *    library under the GPL. See COPYING for the full license text.
 * 
 * 2. The Mozilla Public License
 * 
 *    See http://www.mozilla.org/MPL/MPL-1.1.html or MPL.html for the full
 *    license text.
 * 
 *    Under MPL section 13. I declare that all of the Covered Code is
 *    Multiple Licensed:
 *      _________________________________________________________________
 * 
 *      The contents of this file are subject to the Mozilla Public License
 *      version 1.1 (the "License"); you may not use this file except in
 *      compliance with the License. You may obtain a copy of the License
 *      at http://www.mozilla.org/MPL/
 * 
 *      Software distributed under the License is distributed on an "AS IS"
 *      basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 *      the License for the specific language governing rights and
 *      limitations under the License.
 * 
 *      The Initial Developer of the Original Code is David Beckett.
 *      Portions created by David Beckett are Copyright (C) 2000 David
 *      Beckett, Institute for Learning and Research Technology, University
 *      of Bristol. All Rights Reserved.
 * 
 *      Alternatively, the contents of this file may be used under the
 *      terms of the GNU Lesser General Public License, in which case the
 *      provisions of the LGPL License are applicable instead of those
 *      above. If you wish to allow use of your version of this file only
 *      under the terms of the LGPL License and not to allow others to use
 *      your version of this file under the MPL, indicate your decision by
 *      deleting the provisions above and replace them with the notice and
 *      other provisions required by the LGPL License. If you do not delete
 *      the provisions above, a recipient may use your version of this file
 *      under either the MPL or the LGPL License.
 */


import org.w3c.rdf.model.*;
import org.w3c.rdf.syntax.*;
import org.w3c.rdf.util.*;

import org.w3c.rdf.implementation.syntax.sirpac.*;
import org.w3c.rdf.implementation.model.*;

import org.xml.sax.*;


import java.util.*;
import java.io.*;
import java.net.*;


public class PrintParser {

  static void bailOut() {
    System.err.println("Usage: java -Dorg.xml.sax.parser=<classname> PrintParser [--streaming|--static] " +
                       "<URI | filename>");
    System.exit(1);
  }
  
  public static String normalizeURI(String uri) {

    // normalise uri
    URL url = null;
    try {
      url = new URL (uri);
    } catch (Exception e) {
      try {
        if(uri.indexOf(':') == -1)
          url = new URL ("file", null, uri);
      } catch (Exception e2) {}
    }
    return url != null ? url.toString() : uri;
  }


  public static void main(String[] args) throws Exception {
    if(args.length <1)
      bailOut();

    boolean streaming = true;
    int offset = 0;
    while(offset < args.length && args[offset].startsWith("--")) {
      if(args[offset].equals("--streaming"))
        streaming = true;
      else if(args[offset].equals("--static"))
        streaming = false;
      else
        bailOut();
      offset++;
    }

    if(offset != args.length - 1)
      bailOut();
    
    //    NodeFactory node_factory = new NodeFactoryImpl();
    Model model = new ModelImpl();

    SiRPAC parser=new SiRPAC();
    parser.setRobustMode(true);
    parser.setStreamMode(streaming);
    
    URL url=null;
    try {
      url=new URL (normalizeURI(args[offset]));
    } catch (Exception e) {
      System.err.println("PrintParser: Failed to create URL '" + url + "' - " + e);
      System.exit(1);
    }
    
    // Prepare input source
    InputSource source = null;
    
    try {
      source = new InputSource(url.openStream());
    } catch (Exception e) {
      System.err.println("PrintParser: Failed to open URL '" + url + "' - " + e);
      System.exit(1);
    }

    source.setSystemId( url.toString() );
    model.setSourceURI( url.toString() );

    // Create a consumer for the RDF triples    
    RDFConsumer consumer=null;
    if(streaming) {
      try {
        consumer = new PrintConsumer(model, System.out);
      } catch  (Exception e) {
        System.err.println("PrintParser: Failed to create PrintConsumer - " + e);
        System.exit(1);
      }
    } else {
      try {
        consumer = new org.w3c.rdf.util.ModelConsumer(model);
      } catch (Exception e) {
        System.err.println("PrintParser: Failed to create ModelConsumer - " + e);
        System.exit(1);
      }
      
    }
    
    // Parse the RDF/XML
    try {
      parser.parse(source,  consumer);
    } catch (Exception e) {
      System.err.println("PrintParser: RDF parsing failed - " + e);
      System.exit(1);
    }

    if(!streaming) {
      for(Enumeration en = model.elements(); en.hasMoreElements();) {
        Statement s = (Statement)en.nextElement();
        System.out.println("Statement " + s);
      }
    }
  }
}
