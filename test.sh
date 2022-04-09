#!/bin/bash
assert() {
  expected="$1"
  input="$2"

  ./9cc "$input" > tmp.s
  cc -o tmp tmp.s
  ./tmp
  actual="$?"

  if [ "$actual" = "$expected" ]; then
    echo "$input => $actual"
  else
    echo "$input => $expected expected, but got $actual"
    exit 1
  fi
}

assert 0 "0;"
assert 42 "42;"
assert 21 "5+20-4;"
assert 41 " 12 + 34 - 5 ;"
assert 47 '5+6*7;'
assert 15 '5*(9-6);'
assert 4 '(3+5)/2;'
assert 10 '-10+20;'
assert 0 '0==1;'
assert 1 '42==42;'
assert 1 '0!=1;'
assert 0 '42!=42;'

assert 1 '0<1;'
assert 0 '1<1;'
assert 0 '2<1;'
assert 1 '0<=1;'
assert 1 '1<=1;'
assert 0 '2<=1;'

assert 1 '1>0;'
assert 0 '1>1;'
assert 0 '1>2;'
assert 1 '1>=0;'
assert 1 '1>=1;'
assert 0 '1>=2;'

assert 0 'a=0;'
assert 14 'a = 3; b = 5 * 6 - 8; a + b / 2;'

assert 6 'foo = 1; bar = 2 + 3; foo + bar;'

assert 14 'a = 3; b = 5 * 6 - 8; return a + b / 2;'
assert 6 'foo = 1; bar = 2 + 3; return foo + bar;'
assert 5 'return 5; return 8;'

assert 7 'if(0) return 2; return 3 + 4;'
assert 2 'if(1) return 2; return 3 + 4;'
assert 7 'if(1>2) return 2; return 3 + 4;'
assert 2 'if(1<2) return 2; return 3 + 4;'
assert 2 'if(1) if(0) return 1; return 2; return 3;'

assert 3 'if(0) return 2; else return 3; return 3 + 4;'
assert 2 'if(1) return 2; else return 3; return 3 + 4;'
assert 2 'if(0) return 1; else if(1) return 2; return 3;'

assert 6 'a = 0; while(a<6) a = a + 1; return a;'
assert 6 'a = 0; while(1) if(a==6) return a; else a = a + 1;'

assert 6 'for(a=0;a<6;a=a+1) b=0; return a;'
assert 6 'a = 0; for(;;) if(a==6) return a; else a = a + 1;'

assert 1 '{return 1;}'
assert 6 'a = 0; while(a<5) {a = a + 1; a = a + 1;} return a;'

echo OK
