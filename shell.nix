with import <nixpkgs> {};
let symbdiff = callPackage ./tools/symbdiff { };
in mkShell {
  packages = [ clang symbdiff ];
  shellHook = ''
    alias klang='clang -emit-llvm -c -g -O0 -Xclang -disable-O0-optnone'
 '';
}
