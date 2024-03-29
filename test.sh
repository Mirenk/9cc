#!/bin/bash
cat <<EOF | gcc -xc -c -o tmp2.o -
#include <stdlib.h>
int ret3() { return 3; }
int ret5() { return 5; }
int retarg(int a) { return a; }
int retadd(int a, int b) { return a + b; }

int add(int x, int y) { return x+y; }
int sub(int x, int y) { return x-y; }
int add6(int a, int b, int c, int d, int e, int f) {
  return a+b+c+d+e+f;
}
int alloc4(int **ptr, int a, int b, int c, int d) {
  *ptr = calloc(4, sizeof(int));
  (*ptr)[0] = a;
  (*ptr)[1] = b;
  (*ptr)[2] = c;
  (*ptr)[3] = d;
}
EOF

assert() {
  expected="$1"
  input="$2"

  ./9cc "$input" > tmp.s
  gcc -o tmp tmp.s tmp2.o -Wa,--noexecstac
  ./tmp
  actual="$?"

  if [ "$actual" = "$expected" ]; then
    echo "$input => $actual"
  else
    echo "$input => $expected expected, but got $actual"
    exit 1
  fi
}

assert 0 'int main(){0;}'
assert 42 'int main(){42;}'
assert 21 'int main(){5+20-4;}'
assert 41 'int main(){ 12 + 34 - 5 ;}'
assert 47 'int main(){5+6*7;}'
assert 15 'int main(){5*(9-6);}'
assert 4 'int main(){(3+5)/2;}'
assert 10 'int main(){-10+20;}'
assert 0 'int main(){0==1;}'
assert 1 'int main(){42==42;}'
assert 1 'int main(){0!=1;}'
assert 0 'int main(){42!=42;}'

assert 1 'int main(){0<1;}'
assert 0 'int main(){1<1;}'
assert 0 'int main(){2<1;}'
assert 1 'int main(){0<=1;}'
assert 1 'int main(){1<=1;}'
assert 0 'int main(){2<=1;}'

assert 1 'int main(){1>0;}'
assert 0 'int main(){1>1;}'
assert 0 'int main(){1>2;}'
assert 1 'int main(){1>=0;}'
assert 1 'int main(){1>=1;}'
assert 0 'int main(){1>=2;}'

assert 0 'int main(){int a; a=0;}'
assert 14 'int main(){int a; int b; a = 3; b = 5 * 6 - 8; a + b / 2;}'

assert 6 'int main(){int foo; int bar; foo = 1; bar = 2 + 3; foo + bar;}'

assert 14 'int main(){int a; int b; a = 3; b = 5 * 6 - 8; return a + b / 2;}'
assert 6 'int main(){int foo; int bar; foo = 1; bar = 2 + 3; return foo + bar;}'
assert 5 'int main(){return 5; return 8;}'

assert 7 'int main(){if(0) return 2; return 3 + 4;}'
assert 2 'int main(){if(1) return 2; return 3 + 4;}'
assert 7 'int main(){if(1>2) return 2; return 3 + 4;}'
assert 2 'int main(){if(1<2) return 2; return 3 + 4;}'
assert 2 'int main(){if(1) if(0) return 1; return 2; return 3;}'

assert 3 'int main(){if(0) return 2; else return 3; return 3 + 4;}'
assert 2 'int main(){if(1) return 2; else return 3; return 3 + 4;}'
assert 2 'int main(){if(0) return 1; else if(1) return 2; return 3;}'

assert 6 'int main(){int a; a = 0; while(a<6) a = a + 1; return a;}'
assert 6 'int main(){int a; a = 0; while(1) if(a==6) return a; else a = a + 1;}'

assert 6 'int main(){int a; int b; for(a=0;a<6;a=a+1) b=0; return a;}'
assert 6 'int main(){int a; a = 0; for(;;) if(a==6) return a; else a = a + 1;}'

assert 1 'int main(){{return 1;}}'
assert 6 'int main(){int a; a = 0; while(a<5) {a = a + 1; a = a + 1;} return a;}'

assert 3 'int main(){return ret3();}'
assert 5 'int main(){return ret5();}'

assert 3 'int main(){return retarg(3);}'
assert 5 'int main(){return retadd(3, 2);}'
assert 8 'int main(){ return add(3, 5); }'
assert 2 'int main(){ return sub(5, 3); }'
assert 21 'int main(){ return add6(1,2,3,4,5,6); }'
assert 66 'int main(){ return add6(1,2,add6(3,4,5,6,7,8),9,10,11); }'
assert 136 'int main(){ return add6(1,2,add6(3,add6(4,5,6,7,8,9),10,11,12,13),14,15,16); }'

assert 32 'int main() { return ret32(); } int ret32() { return 32; }'

assert 7 'int main() { return add2(3,4); } int add2(int x, int y) { return x+y; }'
assert 1 'int main() { return sub2(4,3); } int sub2(int x, int y) { return x-y; }'
assert 55 'int main() { return fib(9); } int fib(int x) { if (x<=1) return 1; return fib(x-1) + fib(x-2); }'

assert 3 'int main(){ int x; x=3; return *&x; }'
assert 3 'int main(){ int x; int *y; int **z; x=3; y=&x; z=&y; return **z; }'
#assert 3 'int main(){ int x; int y; x=3; y=5; return *(&y+8); }'
#assert 5 'int main(){ int x; int y; x=3; y=5; return *(&x-8); }'
assert 5 'int main(){ int x; int *y; x=3; y=&x; *y=5; return x; }'
#assert 7 'int main(){ int x; int y; x=3; y=5; *(&y+1)=7; return x; }'
#assert 7 'int main(){ int x; int y; x=3; y=5; *(&x-1)=7; return y; }'

assert 3 'int main(){ int x; int *y; y = &x; *y = 3; return x; }'
assert 8 'int main(){int *p; alloc4(&p, 1, 2, 4, 8); int *q; q = p + 3; return *q;}'

assert 4 'int main(){int x; return sizeof(x);}'
assert 8 'int main(){int *x; return sizeof(x);}'
assert 4 'int main(){return sizeof(1);}'
assert 4 'int main(){int x; return sizeof(x+1);}'
assert 4 'int main(){return sizeof(sizeof(1));}'
assert 8 'int main(){int *x; return sizeof x;}'

assert 16 'int main(){int a[4];return sizeof(a);}'
assert 16 'int main(){int a[2][2];return sizeof(a);}'
assert 3 'int main(){int a[2]; *a = 1; *(a + 1) = 2; int *p; p = a; return *p + *(p + 1);}'

assert 3 'int main() { int x[3]; *x=3; *(x+1)=4; *(x+2)=5; return *x; }'
assert 4 'int main() { int x[3]; *x=3; *(x+1)=4; *(x+2)=5; return *(x+1); }'
assert 5 'int main() { int x[3]; *x=3; *(x+1)=4; *(x+2)=5; return *(x+2); }'

assert 3 'int main() { int x[3]; *x=3; *(1+x)=4; *(2+x)=5; return *x; }'

assert 0 'int main() { int x[2][3]; int *y; y=x; *y=0; return **x; }'
assert 1 'int main() { int x[2][3]; int *y; y=x; *(y+1)=1; return *(*x+1); }'
assert 2 'int main() { int x[2][3]; int *y; y=x; *(y+2)=2; return *(*x+2); }'
assert 3 'int main() { int x[2][3]; int *y; y=x; *(y+3)=3; return **(x+1); }'
assert 4 'int main() { int x[2][3]; int *y; y=x; *(y+4)=4; return *(*(x+1)+1); }'
assert 5 'int main() { int x[2][3]; int *y; y=x; *(y+5)=5; return *(*(x+1)+2); }'

assert 3 'int main() { int x[3]; *x=3; x[1]=4; x[2]=5; return *x; }'
assert 4 'int main() { int x[3]; *x=3; x[1]=4; x[2]=5; return *(x+1); }'
assert 5 'int main() { int x[3]; *x=3; x[1]=4; x[2]=5; return *(x+2); }'
assert 5 'int main() { int x[3]; *x=3; x[1]=4; 2[x]=5; return *(x+2); }'

assert 0 'int main() { int x[2][3]; int *y; y=x; y[0]=0; return x[0][0]; }'
assert 1 'int main() { int x[2][3]; int *y; y=x; y[1]=1; return x[0][1]; }'
assert 2 'int main() { int x[2][3]; int *y; y=x; y[2]=2; return x[0][2]; }'
assert 3 'int main() { int x[2][3]; int *y; y=x; y[3]=3; return x[1][0]; }'
assert 4 'int main() { int x[2][3]; int *y; y=x; y[4]=4; return x[1][1]; }'
assert 5 'int main() { int x[2][3]; int *y; y=x; y[5]=5; return x[1][2]; }'

assert 3 'int main() { int x[3]; x[0]=-3; x[1]=4; x[2]=-x[0]; return x[2]; }'

assert 0 'int x; int main() { return x; }'
assert 3 'int x; int main() { x=3; return x; }'
assert 7 'int x; int y; int main() { x=3; y=4; return x+y; }'
assert 0 'int x[4]; int main() { x[0]=0; x[1]=1; x[2]=2; x[3]=3; return x[0]; }'
assert 1 'int x[4]; int main() { x[0]=0; x[1]=1; x[2]=2; x[3]=3; return x[1]; }'
assert 2 'int x[4]; int main() { x[0]=0; x[1]=1; x[2]=2; x[3]=3; return x[2]; }'
assert 3 'int x[4]; int main() { x[0]=0; x[1]=1; x[2]=2; x[3]=3; return x[3]; }'

assert 4 'int x; int main() { return sizeof(x); }'
assert 16 'int x[4]; int main() { return sizeof(x); }'

assert 1 'int main() { char x; x=1; return x; }'
assert 1 'int main() { char x; x=1; char y; y=2; return x; }'
assert 2 'int main() { char x; x=1; char y; y=2; return y; }'

assert 1 'int main() { char x; return sizeof(x); }'
assert 10 'int main() { char x[10]; return sizeof(x); }'
assert 1 'int main() { return sub_char(7, 3, 3); } int sub_char(char a, char b, char c) { return a-b-c; }'

assert 0 'int main() { return ""[0]; }'
assert 1 'int main() { return sizeof(""); }'

assert 97 'int main() { return "abc"[0]; }'
assert 98 'int main() { return "abc"[1]; }'
assert 99 'int main() { return "abc"[2]; }'
assert 0 'int main() { return "abc"[3]; }'
assert 4 'int main() { return sizeof("abc"); }'

echo OK
