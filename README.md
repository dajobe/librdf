#Redland librdf RDF API Library
##Latest version: 1.0.17 (2013-12-28)
Download: [redland-1.0.17.tar.gz](http://download.librdf.org/source/redland-1.0.17.tar.gz)

##Configuration and build improvements
Fixed Issues: [0000529](http://bugs.librdf.org/mantis/view.php?id=529), [0000540](http://bugs.librdf.org/mantis/view.php?id=540), 
[0000541](http://bugs.librdf.org/mantis/view.php?id=541), [0000542](http://bugs.librdf.org/mantis/view.php?id=542) 
and [0000543](http://bugs.librdf.org/mantis/view.php?id=543)
See the [1.0.17 Release Notes](http://librdf.org/RELEASE.html#rel1_0_17) for the full details of the changes.

##Overview
[Redland librdf](http://librdf.org/) is a library that provides a high-level interface for the Resource Description Framework (RDF) allowing the RDF graph to be parsed from XML, stored, queried and manipulated. Redland librdf implements each of the RDF concepts in its own class via an object based API, reflected into the language APIs, currently C#, Java, Perl, PHP, Python, Ruby and Tcl. Several classes providing functionality such as for parsers, storage are built as modules that can be loaded at compile or run-time as required.

This is a mature and stable RDF library developed since 2000 used in multiple projects. See the [FAQS](http://librdf.org/FAQS.html) 
for general information and the [Redland issue tracker](http://bugs.librdf.org/) for known bugs and issues. 
A summary of the changes can be found in the [NEWS](http://librdf.org/NEWS.html) file, detailed API changes in the 
[release notes](http://librdf.org/RELEASE.html) and file-by-file changes in the Subversion [ChangeLog](http://librdf.org/ChangeLog).

Redland librdf provides:

A modular, [object based](http://librdf.org/docs/api/objects.html) library written in C
APIs for manipulating the RDF [graph](http://librdf.org/docs/api/model.html) and parts - 
[Statements](http://librdf.org/docs/api/statement.html), [Resources and Literals](http://librdf.org/docs/api/node.html)
Language Bindings in Perl, PHP, Python and Ruby via the [Redland Bindings](http://librdf.org/bindings/) package.
Reading and writing multiple RDF syntaxes with [Parsers](http://librdf.org/docs/api/parser.html) and 
[Serializers](http://librdf.org/docs/api/serializer.html) via the [Raptor RDF Parser Toolkit](http://librdf.org/raptor/).
[Storage](http://librdf.org/docs/api/storage.html) for graphs in memory and persistently with Sleepycat/Berkeley DB, MySQL 3-5, PostgreSQL, SQLite, Openlink Virtuoso, in-memory Trees, files or URIs.
[Query language](http://librdf.org/docs/api/query.html) support for SPARQL 1.0 (some 1.1) and RDQL using [Rasqal](http://librdf.org/rasqal/).
APIs for accessing the graph by Statement (triples) or by Nodes and Arcs
Contexts for managing aggregating named graphs and recording provenance.
[Statement Streams](http://librdf.org/docs/api/stream.html) for efficient construction, parsing and serialisation of graphs
[rdfproc](http://librdf.org/utils/rdfproc.html) RDF processor utility program
No memory leaks.

##Sources
The packaged sources are available from [http://download.librdf.org/source/](http://download.librdf.org/source/). The development GIT sources can also be [browsed on GitHub](http://github.com/dajobe/librdf) or checked out at git://github.com/dajobe/librdf.git

librdf requires [Raptor](http://librdf.org/raptor/) 1.4.19 or newer (or Raptor V2 1.9.0 or newer) and [Rasqal](http://librdf.org/rasqal/) 0.9.19 or newer to build and run, which can be downloaded from the same area as the librdf source and binaries.

##License
This library is free software / open source software released under the LGPL 2.1 (GPL) or Apache 2.0 licenses. See [LICENSE.html](http://librdf.org/LICENSE.html) for full details.

##Installation and Documentation
See [INSTALL.html](http://librdf.org/INSTALL.html) for general installation and configuration information.

Further documentation is available in the documents area including the [API reference document](http://librdf.org/docs/api/index.html) and [detailed storage modules information](http://librdf.org/docs/storage.html).

##Mailing Lists
The [Redland mailing lists](http://librdf.org/lists/) discuss the development and use of the Redland libraries as well as future plans and announcement of releases.
