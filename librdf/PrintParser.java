/* -*- Mode: java -*-
 *
 * PrintParser.java - Use streaming SiRPAC to parse RDF/XML content to triples
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
