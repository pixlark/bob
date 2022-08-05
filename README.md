# Bob the Builder

Bob is a build scripter. I don't like the term build system - if your build is so complex that it deserves the word *system*, then just use a real programming language to organize the build!

Bob is for quickly organizing builds with simple, powerful, readable syntax. That means:

 - No bizarre, archaic syntax!
 - No obscure build system behaviours!
 - No build systems that generate other build systems!

If you dread starting new projects just because writing Makefiles makes you want to tear your eyes out of their sockets, then Bob is for you!

# The Language

## Builders

A builder is a top-level definition following this format:

```
builderName:
    # ...
```

A builder can also take arguments like so:

```
builderName [Argument]:
    # ...
```

Stylistically, builder names are in `camelCase` while argument names (as well as flag names) are in `PascalCase`.

The `start` builder is special. Every bob script must have one. It describes what happens first when a bob script is invoked. It also cannot have any arguments.

## Commands

Inside a builder, commands can be listed. These act almost exactly like shell commands, except that there aren't the same shell extensions - i.e., no piping, no background jobs, no globbing, etc. Commands just call a program with some whitespace-separated arguments:

```
buildExecutable:
    gcc mySource.c -o myExecutable
```

Don't worry, most of those shell features can be replicated in bob.

## Directives

Directives start with a `.`. They describe basically anything that isn't a command.

The most common directive is `build`:

```
myBuilder:
    gcc myFile.c -o myExecutable

start:
    .build myBuilder
```

Using `build`, we can invoke other builders from our own builder, just like calling a function in a programming language.

A nice fact of builders is that they do not need to appear "in order" in the file. You can very well call a builder before you use it:

```
start:
    .build myBuilder

myBuilder:
    # ...
```

However, builders can *not* have circular dependency. This will fail before any commands are even run:

```
builderA:
    .build builderB

builderB:
    .build builderA
```

Calling a builder with arguments looks like this:

```
myBuilder [Source, Name]:
    # ...

start:
    .build myBuilder [myFile.c, myExecutable]
```

Other directives will come up later in this defintion. For now, let's learn how to use our arguments:

## Using Arguments

The dollar sign indicates that a variable is to be inserted into your command:

```
myBuilder [Source, Name]:
    gcc $Source -o $Name

start:
    .build mybuilder [myFile.c, myExecutable]
```

Similarly, you need to use the dollar sign to pass a variable on to another builder - or for that matter, as an argument in any directive. Without the dollar sign, bob will think you're just writing a normal word!

```
builder2 [Argument]:
    # ...

builder1 [Argument]:
    .build builder2 [$Argument]

start:
    .build builder1 [foo]
```

## List Arguments

Oftentimes, you want to represent lists - whether that be a list of source files, a list of objects, a list of directories - for this, bob has you covered!

To create a list-type value, we use curly braces:

```
myBuilder [Sources]:
    # ...
start:
    .build myBuilder [{a.c, b.c, c.c}]
```

When a list-type value is substituted into a command, it becomes space-separated:

```
myBuilder [Sources]:
    gcc $Sources # Expands to 'gcc a.c b.c c.c'
start:
    .build myBuilder [{a.c, b.c, c.c}]
```

## The buildMany directive

Very often when we have a list of files, we will want to execute a builder on every one of those files. It's in those cases that we use the `buildMany` directive:

```
makeObject [Source]:
    gcc -c $Source
start:
    .buildMany makeObject [{a.c, b.c, c.c}]
```

## Flags

While arguments are nice, sometimes we have a lot of information that we want to be passed everywhere, no matter what.

This is where flags come in handy. They act like implicit arguments - any builder invoked within a flag block has the flags passed in as implicit arguments. And any builder *those* builders invoke carry the same implicit flags.

Here's an example:

```
makeObject [Source]:
    gcc -c $CFlags $Source $Libraries

start:
    .flag [CFlags=-g -std=c99, Libraries=-lpthread]:
        .build makeObject [foo.c]
        .build makeObject [bar.c]
```

If a flag is used, but it was never defined, then nothing gets substituted in:

```
makeObject [Source]:
    # This will just expand to 'gcc -c foo.c'
    gcc -c $CFlags $Source $Library

start:
    .build makeObject [foo.c]
```

## Command-Line Arguments

Your bob file can take in command-line arguments. While the syntax for defining them is unique, *using* them is the same as using flags - all command-line arguments simply get passed onto start as flags.

```
start:
    gcc $CFlags foo.c

.options:
    # creates the option -c/--cflags which take an argument that CFlags gets set to
    c, cflags, CFlags
```

## Parallelism

If you want multiple parts of your build to run at once, as different processes, you can use the `parallel` directive:

```
start:
    .parallel:
        gcc foo.c
        gcc bar.c
```

Every command or build invocation within the parallel block will be run in its own separate process or thread.
