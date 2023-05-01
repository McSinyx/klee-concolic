with import <nixpkgs> {};
let symbdiff = callPackage ./tools/symbdiff { };
in mkShell {
  packages = [ clang symbdiff ];
  shellHook = ''
    # clang -emit-llvm -c -DKLEE_RUNTIME metaprogram.c for bitcode
    # clang -o metaprogram{,.c} for binary
    alias clang='clang -g -O0 -Xclang -disable-O0-optnone'
 '';
}
