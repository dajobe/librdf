/* -*- Mode: java -*-
 *
 * PrintConsumer.java - SiRPAC RDF consumer that prints out triple as they are added
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


import org.w3c.rdf.model.*;
import org.w3c.rdf.syntax.*;

import java.io.*;


/**
 * Parse the RDF/XML and print it as this is done
 */

public class PrintConsumer implements RDFConsumer {
  Model model;
  NodeFactory nodeFactory;
  OutputStream output;

  /**
   * @param an RDF model to fill with triples. This frequently will be an empty model.
   */
  public PrintConsumer(Model model, OutputStream output) throws ModelException {
    this.model = model;
    this.nodeFactory = model.getNodeFactory();
    this.output = output;
  }

  /**
   * start is called when parsing of data is started
   */
  public void startModel () {}
  
  /**
   * end is called when parsing of data is ended
   */
  public void endModel () {}


  public NodeFactory getNodeFactory() {
    return nodeFactory;
  }
  
  /**
   * assert is called every time a new statement within
   * RDF data model is added
   */
  public void addStatement (Statement s) {
    System.out.println("Statement " + s);
    // immediately remove statement
    try {
      model.remove(s);
    } catch (Exception e) {
      System.err.println("Error in removing statement - " + e);
    }
  }

}
