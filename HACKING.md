Hacking Redland librdf
======================

2011-08-23

[Dave Beckett](http://www.dajobe.org/)


Commits
-------

Should be:

  * Licensed with the same license as Redland librdf.
  * Patches in GNU `diff -u` unifed context format preferred.
  * Include tests if they add new features.


Code Style
----------

Do not make large commits that only change code style unless you have
previously had this agreed or it is in code under major refactoring
where a diff is not a large concern.  There is always `diff -b` when
large code style (whitespace) changes are made.


### Indenting

2 spaces.  No tabs.

All code must be wrapped to 80 chars as far as is possible.  Function
definitions or calls should indent the parameters to the left `(`.

Redland libraries use very long function names following the naming
convention which can make linebreaking very hard.  In this case,
indent function parameters on new lines 4 spaces after the function
name like this:

    var = function_name_with_very_long_name_that_is_hard_to_wrap_args(
              argument1_with_very_long_name_or_expression,
              argument2,
              ..)

Use no space between a keyword followed by braces argument.
For example, use `if(cond)` rather than `if (cond)` (ditto for
while, do etc.) and
`functionname(...)` rather than `functionname (...)` in
both definition and calls of functions.

There is nothing wrong with introducing a variable to break up a very
long function call argument.


### Expressions

Put spaces around operators in expressions, assignments, tests, conditions

GOOD:

  * `a += 2 * x`
  * `if(a < 2)`

BAD:

  * `a+=2*x`
  * `if(a<2)`

When comparing to `0` or a `NULL` pointer, use the idiomatic form
that has no comparison.

GOOD:

  * `if(!ptr)`
  * `if(!index)`

BAD:

  * `if(ptr == NULL)`
  * `if(index == 0)`
  * `if(0 == index)`

When comparing a variable to a constant, the code has currently used
`if(var == constant)` rather than the slightly safer, and easier to
compile check, `if(constant == var)`.


### Blocks

In general add {}s around blocks in if else chains when one of the blocks
has more than 1 line of code.  Try not to mix, but the final case if it
is one line, can be braceless.

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
    } else if(var == 2) {
      ... multiple lines of code and / or more if conditions ...
    } else
      ... one line of code ...


### Switches

If using if else chains on an enumeration, don't do that, use a
`switch()` which GCC can use to find missing cases when they get added.

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

There should ALWAYS be a `default:` case.


### Functions

Declare functions in this format:

    returntype
    functionname(type1 param1, type2 param2, ...)
    {
      type3 var1;
      type4 var2;
      
      ... first line of code ...
      
      tidy:
        ... cleanup code...
      
      return value;
     }

Notes:

   * Declare one variable per line
   * Declare all variables at the top of the function (K&R C style)
   * You may declare variables in inner `{}` blocks.  The
     form `if(1) { ... var decls ...; more code }` may be used but
     a code rewrite is preferable.
   * If a label is used it *MUST* be used only for cleanup, and going
     forward in the code to the end of the function.
   * Multiple `return` are allowed but for obvious error or result
     returns.  Do not twist the code to enable a single return.
   * `goto` may be used for resource cleanup and result return 
     where control flow only goes forward.

### C Pre-Processor (CPP) Macros

Always define macros for internal constants and name the macros with
the library prefix followed by a descriptive name in ALL CAPS such as:

    #define LIBRDF_FOOBAR_BUFFER_SIZE 1234

When evaluating macro symbols that may be undefined, always check the
symbol is defined first.  Like this:

    #if defined(LIBRDF_DEBUG) && LIBRDF_DEBUG > 42
	   ... do complex debugging stuff ...
	#endif

This is not needed for macros that are known to be defined, such as those
checked by `configure` e.g.

    #if RAPTOR_VERSION_DECIMAL > 20100
	   ... do stuff that requires a raptor2 version 2.1.0 or newer ...
	#endif
	
since the above would be checked implicitly by `configure` using
`pkg-config(1)` to validate Raptor 2 is present before getting to the
code that tries to evaluate the value from `raptor2.h`.

The debug macros that are used for printing out values when debugging
is enabled do not need protection by `#if` or `#ifdef` and should be
used like this:

    LIBRDF_DEBUG1("Something wonderful happened\n");

    LIBRDF_DEBUG2("Something %s happened\n", happening);


### Memory allocation

Allocating a zeroed out block of memory or a set of objects (calloc)

    var = LIBRDF_CALLOC(type, count, size)

 Prefering when `count` = 1 this form:

    var = LIBRDF_CALLOC(type, 1, sizeof(*var))

Allocating a block of memory:
    
    var = LIBRDF_MALLOC(type, size)

Freeing memory:    

    LIBRDF_FREE(type, var)
    
The reasoning here is to make allocs mostly fit into 1 line without
too much boilerplate and duplication of types.

The macro names vary by library such as `RAPTOR_CALLOC` and
`RASQAL_CALLOC` for Raptor and Rasqal respectively.


### Documentation

Public functions, types, enumerations and defines must have autodocs -
the structured comment block before the definition.  This is read by
`gtk-doc(1)` to generate reference API documentation.

Format:

    /**
     * functionname:
     * @param1: Description of first parameter
     * @param2: Description of second parameter (or NULL)
     * ... more params ...
     *
     * Short Description
     *
     * Long Description.
     *
     * Return value: return value
     */
     returntype
     functionname(...)
     {
       ... body ...
     }

The _Short Description_ have several commmon forms:

  * Constructor - creates a FOO object
  * Destructor - destroys a FOO object
  * Short description of regular function or method.
  * INTERNAL - short description

The latter is used for autodocs for internal functions either as
internal documentation or for APIs that may one day be public.

The _(or NULL)_ phrase is used for pointer parameters that may be
omitted.  This is usually tested by the function as an assertion.
In some functions there are more complex conditions on which optional
parameters are allowed, these are described in the _Long Description_.

The long description may also include a deprecation statement such as:

    * @Deprecated: Use new_function() with foo = BAR

This must be indented to the left and will be used by the
`gtk-doc(1)` document generator to provide a link to the replacement
function and usage.


Commit Messages
---------------

The general standard for Redland libraries using GIT is a merge
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

If the change is trivial or a typo and (this is *IMPORTANT*) NOT a
commit to code files, then the commit can start with '#'.  This may
get filtered out of commit log message notifications and ChangeLog.

e.g.  `#spelling`  or `#ws`
the latter is whitespace changes for some reason

The changes will semi-automatically be added to the ChangeLog files
following the GNU style, indented and word wrapped, and adding the list
of files at the start.  So the commit message above ends up looking
something like:

    2010-08-23  User Name <user@example.org>
    
            * dir/file1.c, dir2/file2.c: First line summaries what commit
              does - this goes into the GIT short log

              (function1, function2): what changed

              (function3): Added, deprecating function4()

              (function4): Deleted, replaced by function3()

              struct foo gains field ...

              struct bar loses field ...

              enum blah gains new value BLAH_2 which ...


<!--
Local variables:
mode: markdown
End:
-->
