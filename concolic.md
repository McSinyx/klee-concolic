# Concolic Execution
This document explains the use of concolic mode execution in Klee. 
In order to use this behavior you need to use the custom Klee version at Github Repo: https://github.com/rshariffdeen/klee.git , Git Branch: concolic.
You can either build the custom Klee using standard build instructions or use the docker image hosted at
https://hub.docker.com/r/rshariffdeen/klee with tag:concolic. 

### Use Docker
To use the docker image pull the docker image to your PC
```
docker pull rshariffdeen/klee
```


## Example 1
Let's consider the following sample code which takes two user inputs and print
different statements according to the value range of the provided user inputs.


```c
#include <stdio.h>

int main(int argc, char** argv){
    int i = atoi(argv[1]);
    int j = atoi(argv[2]);
    int k;
    klee_make_symbolic(&k, sizeof(k), "k");
    if (i > 5)
        printf("Statement One\n");
    else
        printf("Statement Two\n");
    
    if (j > 10)
        printf("Statement Three\n");
    else
        printf("Statement Four\n");
    
    if (k > 20)
        printf("Statement Five\n");
    else
        printf("Statement Six\n");

    printf("\nValues: i=%d, j=%d, k=%d", i, j, k);
    return 0;

}
```
### Compiling Program
We compile the program using `wllvm` compiler to obtain the bitcode of our sample program
by using the following command:

```
export LLVM_COMPILER=clang
wllvm -o example -lkleeRuntest example.c
extract-bc example
```

We need to link with the Klee libraray `libkleeRuntest.so` which is located in the build 
directory of Klee. You can use `LD_LIBRARY_PATH` to link the lib directory for this purpose. 

### Preparing Input
We make use of the ktest reproducing feature in Klee to execute a test case (i.e. concrete values)
in Concolic execution. For this purpose we re-arrange our input in ktest file format using the binary `ktest-gen` and the 
following command:

```
ktest-gen --sym-arg <argument> --second-var <identifer> <size> <value>
```
`ktest-gen` can create ktest objects for system arguments using the `--sym-arg` flag followed by the 
argument itself. This can be used in Klee to concolically execute a program for command line arguments. 
Furthermore, we can use `ktest-gen` to create ktest objects for internal symbolic variables as well. In our example,
variable 'k' is an internal variable which we can define a pre-determined value and execute klee
concolically. Using the flag `--second-var` followed by the identifier of the variable (ie. same identifier
used when marking the variable symbolic using the intrinsic function `klee_make_symbolic`), the size of the variable
(ie. number of bytes / for 32 bit integers size=4) and the value that should be injected during concolic execution. 
Note that the values for internal variables should be supplied in their unsigned integer representation, which 
will be broken down to byte format by `ktest-gen`. 


For our example we'll use the following command:
```
ktest-gen --sym-arg 66 --sym-arg 34 --second-var k 4 23
```

#### Command Line Input
All preparations for the concolic execution are complete at this point,
the final step is to execute Klee in concolic mode. First, we will illustate
the use of symbolic mode in Klee, using the following command:

```
klee --write-smt2s --libc=uclibc --posix-runtime --external-calls=all example.bc --sym-arg 2 --sym-arg 2
```

This should execute the input in Klee symbolic environment exploring all paths. Now lets use our ktest
we generated to only execute one path concolically. 

```
klee --write-smt2s --libc=uclibc --posix-runtime --external-calls=all --seed-out=file.bout example.bc --sym-arg 2 --sym-arg 2
```

In the above command `--seed-out` argument is used to feed the ktest file and invoke
the concolic mode in Klee. We make use of the klee symbolic environment to mark
our input as symbolic while using the ktest to maintain its symbolic values. 
Once the concolic execution is complete, you can verify using the path
constraints collected at `klee-last/test00001.smt2`. 


## Example 2
Let's use an real-world example to show the usage and features of the concolic execution.
We will use a real-world application 'Jasper' (https://github.com/mdadams/jasper) for this purpose. Following demonstrate the use
of command line arguments and file inputs to be used for the concolic
execution of a program. 

### Preparing Application
In order to use the application we first need to obtain the LLVM bitcode
which KLEE can operate on. You have two options;

* Use clang compiler with `-emit-llvm` flag
* Use wllvm compiler to obtain the complete bitcode for the whole program

For our example program 'Jasper' we simply use the make command by substituting 
the compiler to wllvm. In our example we will use the binary `imginfo` which simply
takes an image as an input and outputs information about the image.

```
autoreconf -i
CC=wllvm ./configure CFLAGS='-g -O0 -static'
make -j32
cd src/appl/
extract-bc imginfo
```

Now that we have obtained the bitcode for the `imginfo` binary, we can verify
it works with Klee using the following command, which prints the help menu of `imginfo`. 

```
klee --libc=uclibc --posix-runtime imginfo.bc -h
```

### Preparing Input
We make use of the ktest reproducing feature in Klee to execute a test case (i.e. concrete values)
in Concolic execution. For this purpose we re-arrange our input in ktest file format using the binary `ktest-gen` and the 
following command:

```
ktest-gen --sym-file <path/to/file>
```

This will produce a output named `file.bout` which is a ktest file, and can be
verified using the following command:

```
ktest-tool file.bout
```


#### Command Line Input
All preparations for the concolic execution are complete at this point,
the final step is to execute Klee in concolic mode. First, we will illustate
the use of concrete mode in Klee, using the following command:

```
klee --write-smt2s --libc=uclibc --posix-runtime imginfo.bc /input/image/path
```

This should execute the concrete input in Klee environment without any symbolic
analysis. If we check the symbolic path for this run in `klee-last/test00001.smt2`, the 
content would be empty. Now lets run the same image in concolic method using the 
following command:

```
klee --write-smt2s --libc=uclibc --posix-runtime --seed-out=file.bout imginfo.bc -f A --sym-files 1 <size of file>
```

In the above command `--seed-out` argument is used to feed the ktest file and invoke
the concolic mode in Klee. We make use of the klee symbolic environment to mark
our input as symbolic while using the ktest to maintain its symbolic values. Since,
we are using a input file the input should also reflect that using `<A/B/C> --symbolic-files <NUM> <N>` where
A/B/C is a identifier for each file in the given order, NUM is the number of files and
N is the file size for each. 

Once the concolic execution is complete, you can verify using the path
constraints collected at `klee-last/test00001.smt2`. 

#### Additional Analysis
If you need to collect the symbolic values of program variables,
you should instrument your source code using the intrinsic function `klee_print_expr()`
which will print the symbolic expression for the current test case. Note that,
this will only print the expression in terms of the input we marked as symbolic. 
