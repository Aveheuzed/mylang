# Presentation

This project is really threefold:
* a toy interpreter for a language I built;
* a toy compiler for another language I built, that targets brainfuck;
* a brainfuck virtual machine (well, two, one in C and the other in asm).

The VM and the compiler are bundled in the same executable. The interpeter stands alone.

This project serves me both as a sample compiler, to understand its design, and as a big codebase to play with (build process, moving functions/structures aroud…).

# Installation

The project used to be built with `make`. It now uses [doit](https://pydoit.org/).
The undelying compiler is `gcc`. `clang` seems to work too, but that's not officially guaranteed.

Use `doit list` to view the available builds.
Use `doit info <build>` to view the executables it can produce.

`$ doit list`
> debug     
> release


`$ doit info debug`
> debug
>
> status    : run
>  * The task has no dependencies.
>
> task_dep  :
>  - debug:dbg-interpreter
>  - debug:dbg-altvm_s
>  - debug:dbg-altvm_c
>  - debug:dbg-compile

Example: type `doit debug:dbg-compile` to build the debug version of the compiler, producing an executable named "dbg-compile" in the main folder.
When given no task, `doit` will make all the tasks of the debug build.


# The languages

## Interpreted languages

* dynamically typed
* braces and semicolons
* assignments:
        * statements, not expressions
        * including augmented assignment (`+=` etc)
        * variables don't need to be declared beforehand
* scope of variables:
        * stacks on function calls
        * outer scopes are read-only unless shadowed
        * augmented assigment can't create variables; regular assignment can. (e.g. assuming `x` refers to a variable in an outer scope: you can have `x=x+3;` but not `x+=3;`.)
* data types:
        * `None`
        * booleans (`True`, `False`)
        * integers
        * floats
        * strings
        * functions
* constructs:
        * if/else
        * while
        * functions
        * blocks

Sample code:
```
square = function(x) { return x*x; };
n = 3
if square(n) > 10 print("The square of 3 is bigger than 10.");
else print("The square of 3 is smaller or equal than 10.");
```
Note the lack of parentheses and braces on the third line. They are not mandatory since there is no ambiguity ("if <expression> <statement>").
Note also the way functions are declared: `function(<arglist>) <statement>` creates a function, which may then be assigned, or called on-the-go. (Thus, the braces are not mandatory in this example, either.)

## Compiled languages

* statically typed
* braces and semicolons
* assignments:
        * statements, not expressions
        * including augmented assignment (`+=` etc)
        * variables __do need to be declared__, C-like.
* scope of variables:
        * stacks on braces
        * outer scopes are read-write unless shadowed
* data types:
        * integers
        * strings maybe someday
* constructs:
        * if/else
        * while, do/while
        * blocks

Sample code:
```
int x = 0;
while (x <= 10) {
        print(x);
        x += 1;
}
```

# Acknowledgements

I learned compilation mostly from [Crafting Interpreters](https://www.craftinginterpreters.com/) (GitHub repo available [here](https://github.com/munificent/craftinginterpreters)). Thanks to Robert Nystrom for writing this book !
