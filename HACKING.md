Hacking Redland
===============

Draft

Dave Beckett
http://www.dajobe.org/
2010-08-24


Commits
=======

Should be:

  * Licensed with the same license as Redland.
  * GNU diff -u format patches prefered.
  * Include tests if they add new features.


Code Style
==========


Indenting
---------

2 spaces.  No tabs.


Expressions
-----------
Put spaces around operators in expressions, assignments, tests, conditions

GOOD:

  * a += 2 * x
  * if (a < 2)

BAD:

  * a+=2*x
  * if(a<2)


Blocks
------
In general add {}s around blocks in if/else when one of the blocks has
more than 1 line of code.  Try not to mix, but the final case if it is
one line, can be braceless.

    if(var == 1) {
      ... multiple lines of code ...
    } else {
      ... multiple lines of code ...
    }

or

    if(var == 1)
      ... one line of code
    else
      ... one line of code

or

    if(var == 1) {
      ... multiple lines of code ...
    } else if (var == 2) {
    } else
      ... one line of code ...


Switches
--------

If using if() else() blocks on an enumeration, don't do that, use a
switch() which GCC can use to find missing cases when they get added.

    switch(enum_var) {
      case ENUM_1:
        ... code ...
        break;
    
      case ENUM_2:
        ... code ...
        break;
    
      case ENUM_DONT_CARE:
      default:
        ... code ...
        break;
    }

There should ALWAYS be a default: case.



Commit Messages
===============

The general standard for redland libraries using GIT is a merge
of the GIT standards format and GNU ChangeLog

    First line summaries what commit does - this goes into the GIT short log
    
    (function1, function2): what changed
    
    (function3): Added, deprecating function4()
    
    (function4): Deleted, replaced by function3()
    
    struct foo gains field ...
    
    struct bar loses field ...
    
    enum blah gains new value BLAH_2 which ...

Use `name()` in the description for references to functions.
Make sure to do (function1, function2) NOT (function1,function2) as
it makes things easier to format later.

Sometimes it's short enough (good) that it all can be done in the first
line, pretty much only if it's a small change to a single function.

If the change is trivial / typo and *IMPORTANT* NOT a commit to code,
then the commit can start with '#'.  This may get filtered out of
commit log message notifications and ChangeLog.

e.g.  `#spelling`  or `#ws`
the latter is whitespace changes for some reason

The changes will semi-automatically be added to the ChangeLog files
following the GNU style, indented and word wrapped, which adds the list
of files at the start.  So the commit message above ends up looking something
like:

      * (dir/file1.c, dir2/file2.c): First line summaries what commit does -
      this goes into the GIT short log
      
      (function1, function2): what changed
      
      (function3): Added, deprecating function4()
      
      (function4): Deleted, replaced by function3()
      
      struct foo gains field ...
      
      struct bar loses field ...
      
      enum blah gains new value BLAH_2 which ...

