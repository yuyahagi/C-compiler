// For automatically generate test case function names, concatenate line number.
// We need two layers of indirection to expand macros properly.
#define TEST_FUNCTION_WITH_NUMBER_(num) TEST_FUNCTION_ ## num
#define TEST_FUNCTION_WITH_NUMBER(num)  TEST_FUNCTION_WITH_NUMBER_(num)
#define TESTCASE_WITH_NUMBER_(num)  TESTCASE_ ## num
#define TESTCASE_WITH_NUMBER(num)   TESTCASE_WITH_NUMBER_(num)

// Generate test case function.
// The test function encloses a series of statements and declarations.
// The test case function invokes the inner function and verifies its return value.
#define EXPECT(expected) \
    /* We don't have forward declaration yet so simply call the test */     \
    /* function that is to be defined later. */                             \
    int TESTCASE_WITH_NUMBER(__LINE__)() {                                  \
        int e1;                                                             \
        e1 = (expected);                                                    \
        int e2;                                                             \
        e2 = TEST_FUNCTION_WITH_NUMBER(__LINE__)();                         \
        if (e1 == e2) {                                                     \
            printf(".");                                                    \
        } else {                                                            \
            printf("\nTest at line %d failed. Expected %d but got %d.\n",   \
                __LINE__, e1, e2);                                          \
            exit(1);                                                        \
        }                                                                   \
    }                                                                       \
    int TEST_FUNCTION_WITH_NUMBER(__LINE__)()


// =============================================================================
// Test cases.
// =============================================================================

EXPECT(0) { }
EXPECT(0) { ; }
EXPECT(0) { ;;; }
EXPECT(0) { 42; }
EXPECT(0) { return; }
EXPECT(42) { return 42; }
EXPECT(0) { 42; return; }
EXPECT(0) { return; 42; }
EXPECT(1) { return 1; 2; }
EXPECT(1) { return 1; return 2; }
EXPECT(21) { return 5+20-4; }
EXPECT(21) {    return  5 +20 - 4  ;  }
EXPECT(14) { return 2+3*4; }
EXPECT(10) { return 2*3+4; }
EXPECT(14) { return 5*4-3*2; }
EXPECT(5) { return 60/12; }
EXPECT(5) { return 1+60/12-1; }
EXPECT(20) { return (2+3)*4; }
EXPECT(7) { 1; 2; return 3+4; }

// Int local variables.
EXPECT(3) { int a; a=1; return a+2; }
EXPECT(3) { int x=1; x = x+2; return x; }
EXPECT(3) { int x = 2; int y = x + 2; return y - 1; }
EXPECT(4) { int _long_variable_1_; _long_variable_1_ = 2; return _long_variable_1_ * 2; }
EXPECT(5) { int foo; int bar; int baz; foo=1; bar=baz=foo+1; return foo+bar*baz; }
EXPECT(1) { int foo; foo = 0; return (foo = foo + 3) == 3; }
EXPECT(6) { int x = 5; ++x; return x; }
EXPECT(0-4) { int x = 0-5; return ++x; }
EXPECT(4) { int x = 5; --x; return x; }
EXPECT(0-6) { int x = 0-5; return --x; }

// Pointers.
EXPECT(3) { int x; int *p; p = &x; x = 3; return *p; }
EXPECT(3) { int x; int *p = &x; x = 3; return *p; }
EXPECT(3) { int x; int *p = &x; *p = 3; return x; }
EXPECT(3) { int x; int *p = &x; int **pp = &p; x = 3; return **pp; }
EXPECT(3) { int x; int *p = &x; int **pp = &p; **pp = 3; return x; }
EXPECT(3) { int x; int y; int *p = &x; x = 2; y = *p + 1; p = &y; return *p; }
EXPECT(3) { int x; int *p = &x; *p = 2; *p = 2**p-1; return x; }
EXPECT(4) { 
    int *p;
    alloc4(&p, 1, 2, 4, 8);
    int *q = p + 2;
    return *q;
}
EXPECT(4) {
    int *p;
    alloc4(&p, 1, 2, 4, 8);
    return *(2 + p);
}
EXPECT(3) {
    int *p;
    alloc4(&p, 1, 2, 4, 8);
    int *q = p + 1;
    *(p + 1) = 3;
    return *q;
}
EXPECT(2) {
    int *p;
    alloc4(&p, 1, 2, 4, 8);
    int offset = 1;
    int *q = two() + p - offset;
    return *q;
}
EXPECT(2) { 
    int *p;
    alloc4(&p, 1, 2, 4, 8);
    int offset = 1;
    return *(two() + p - offset);
}
EXPECT(4) { 
    int **pp;
    allocptr4(&pp, 1, 2, 4, 8);
    pp = pp + 2;
    return **pp;
}

// Arrays.
EXPECT(0) { int ar[9]; }
EXPECT(0) { int ar[3]; int *p; p = ar; }
EXPECT(1) { int ar[3]; *ar=1; return *ar; }
EXPECT(2) { int ar[3]; int *p; p = ar; *(p+1) = 2; return *(ar+1); }
EXPECT(7) { int ar[3]; int *p; p = ar; *p = 1; *(p+1) = 2; *(p+2) = 4; return *ar + *(ar+1) + *(ar+2); }
EXPECT(7) { int ar[3]; *ar=1; *(ar+1) = 2; *(ar+2) = 4; return *ar + *(ar+1) + *(ar+2); }
EXPECT(7) { int ar[3]; *ar = 1; *(ar+1) = 2; *(ar+2) = 4; return ar[0] + ar[1] + ar[2]; }
EXPECT(7) { int ar[3]; ar[0] = 1; ar[1] = 2; ar[2] = 4; return *ar + *(ar+1) + *(ar+2); }
EXPECT(2) { int ar[3]; int i; i = 1; ar[i] = 2; return ar[i]; }
EXPECT(2) { int ar[2]; ar[1] = 2; ar[0] = 1; return ar[1]; }
EXPECT(7) { int ar[3]; ar[2] = 4; ar[1] = 2; ar[0] = 1; return ar[0] + ar[1] + ar[2]; }

// Other integer types.
EXPECT(3) { char c0; char c1; c0 = 0-5; c1 = c0+8; return c1; }
EXPECT(3) { char c0 = 0-5; char c1 = c0+8; return c1; }
EXPECT(3) { char c; char *p; p = &c; c = 3; return *p; }
EXPECT(255) { char c = 0; c = c - 1; return c; }
EXPECT(0) { char c = 255; c = c + 1; return c; }
EXPECT(6) { char x = 5; ++x; return x; }
EXPECT(0) { char x = 255; char y; y = ++x; return y; }
EXPECT(4) { char x = 5; --x; return x; }
EXPECT(255) { char x; x = 0; char y; y = --x; return y; }
EXPECT(0) {
    char arc[4];
    arc[3] = 4; arc[2] = 3; arc[1] = 2; arc[0] = 1;
    int *pi = arc;
    return *pi - (4*256*256*256 + 3*256*256 + 2*256 + 1);
}
EXPECT(4) {
    int i;
    i = 8*256*256*256 + 4*256*256 + 2*256 + 1;
    char *p = &i;
    return *(2 + p);
}
EXPECT(11) {
    int i;
    i = 8*256*256*256 + 4*256*256 + 2*256 + 1;
    char *p = &i;
    char *q = p + 1;
    *(two() + p - 1) = 3;
    return *q + p[3];
}
EXPECT(12) {
    int i[3];
    i[0] = 8*256*256*256 + 4*256*256 + 2*256 + 1;
    i[1] = 0;
    i[2] = 3 * i[0];
    char *p[2];
    p[0] = &i;
    p[1] = &i[2];
    char **pp = &p;
    ++pp;
    return (*pp)[2];
}

// Relations
EXPECT(0) { return 2 == 2+1; }
EXPECT(1) { return 2 != 2+1; }
EXPECT(1) { return 3-1 == 2; }
EXPECT(0) { return 3-1 != 2; }
EXPECT(0) { return 4 < 3; }
EXPECT(0) { return 3 < 3; }
EXPECT(1) { return 2 < 3; }
EXPECT(0) { return 2 > 3; }
EXPECT(0) { return 3 > 3; }
EXPECT(1) { return 4 > 3; }
EXPECT(0) { return 4 <= 3; }
EXPECT(1) { return 3 <= 3; }
EXPECT(1) { return 2 <= 3; }
EXPECT(0) { return 2 >= 3; }
EXPECT(1) { return 3 >= 3; }
EXPECT(1) { return 4 >= 3; }
EXPECT(0) { return 1 < 10 != 10 > 1; }
EXPECT(1) { return 1 < 10 == 10 > 1; }
EXPECT(0) { return 1 <= 10 != 10 >= 1; }
EXPECT(1) { return 1 <= 10 == 10 >= 1; }
EXPECT(2) { int i; int j; i = j = 2+3*4 == 14; return i + j; }
EXPECT(0) { int i; int j; i = j = 2+3*4 != 14; return i + j; }
EXPECT(0) { char c0 = 1; char c1 = 2; return c0 > c1; }
EXPECT(1) { int i = 256 + 1; char c = 2; return i > c; }
EXPECT(1) { int i = 256 + 1; char *pc = &i; char c = 2; return *pc < c; }

// Global variables.
int gvar_i;
int gvar_i2;
int gvar_i_initialized = 500;
char gvar_c;
char gvar_c_initialized = 257;
int *gvar_pi;
int gvar_ari[3];
char gvar_arc[3];
int setlocal(int val) { int gvar_i; gvar_i = val; }
int setglobal(int val) { gvar_i = val; }

EXPECT(500) { return gvar_i_initialized; }
EXPECT(1) { return gvar_c_initialized; }
EXPECT(3) { setglobal(3); return gvar_i; }
EXPECT(3) { setglobal(3); setlocal(2); return gvar_i; }
EXPECT(5) { gvar_pi = &gvar_i; gvar_i = 1; gvar_c = 2; *gvar_pi = *gvar_pi + gvar_c + 2; return gvar_i; }
EXPECT(7) { gvar_ari[0] = 1; gvar_ari[1] = 2; gvar_ari[2] = 4; return gvar_ari[0] + gvar_ari[1] + gvar_ari[2]; }
EXPECT(7) { int *p = gvar_ari; *p = 1; *(p+1) = 2; *(p+2) = 4; return gvar_ari[0] + gvar_ari[1] + gvar_ari[2]; }
EXPECT(15) {
    gvar_i2 = 8;
    gvar_ari[2] = 4; gvar_ari[1] = 2; gvar_ari[0] = 1;
    return gvar_ari[0] + gvar_ari[1] + gvar_ari[2] + gvar_i2;
}
EXPECT(0) {
    gvar_arc[2] = 4; gvar_arc[1] = 2; gvar_arc[0] = 1;
    int *pi = &gvar_arc;
    return *pi - (4*256*256 + 2*256 + 1);
}

// Functions.
EXPECT(2) { return two(); }
EXPECT(6) { return 1 + (two)() + 3; }
EXPECT(10) { return 2 * (two() + 3); }
EXPECT(21) { return func2(1, 2); }
EXPECT(21) { return func2(3-2, 8/4); }
EXPECT(21) { int x = 2; return func2(3-x, 4/x); }
EXPECT(1) { return func8(1, 2, 3, 4, 5, 6, 7, 8) == 87654321; }
EXPECT(1) { int x = 2; return func8(1, (x), x+1, x*2, 5, 2*(x+1), 3*x+1, 8) == 87654321; }
int three() { return 3; }
int four() { return 4; }
int two_plus_one() { return two() + 1; }
EXPECT(3) { return three(); }
EXPECT(7) { return three() + four(); }
EXPECT(3) { return two_plus_one(); }
EXPECT(43) { return func2(three(), four()); }
int add(int x0, int x1) { return x0 + x1; }
EXPECT(3) { return add(1, 2); }
int myfunc6(int x0, int x1, int x2, int x3, int x4, int x5) {
    return x0 + 10*x1 + 100*x2 + 1000*x3 + 10000*x4 + 100000*x5;
}
int myfunc8(int x0, int x1, int x2, int x3, int x4, int x5, int x6, int x7) {
    return x0 + 10*x1 + 100*x2 + 1000*x3 + 10000*x4 + 100000*x5 + 1000000*x6 + 10000000*x7;
}
EXPECT(654321) { return myfunc6(1, 2, 3, 4, 5, 6); }
EXPECT(87654321) { return myfunc8(1, 2, 3, 4, 5, 6, 7, 8); }

// Selection and iteration statements.
EXPECT(0) { if (0) return 1; return 0; }
EXPECT(1) { if (1) return 1; return 0; }
EXPECT(0) { if (0) return 1; else return 0; return 127; }
EXPECT(0) { char c = 0; if (c) return 1; else return 0; return 127; }
EXPECT(0) { int x = 3; int y; if (x + 1 != 2 * 2) y = 1; else y = 0; if (y) return 1; else return 0; }
EXPECT(1) { int x = 3; int y; if (x + 1 == 2 * 2) y = 1; else y = 0; if (y) return 1; else return 0; }
EXPECT(2) { int x = 0; if (1) { x = 1; x = x * 2; } return x; }
int add_upto(int x) { if (x == 1) return x; else return x + add_upto(x-1); }
EXPECT(15) { return add_upto(5); }
EXPECT(2) {
    int x = 0;
    if (0) {
        x = 1;
        x = x * 2;
    } else {
        x = 0;
        if (x) x = 1; else x = 2;
    }
    return x;
}
int fib(int x) {
    if (x == 2) return 1;
    if (x == 1) return 1;
    return fib(x-1) + fib(x-2);
}
EXPECT(13) { return fib(7); }

EXPECT(1) { int x = 1; while (0) x = 0; return x; }
EXPECT(5) { int i = 0; while (i < 5) i = i + 1; return i; }
EXPECT(5) { char i = 0; while (i < 5) i = i + 1; return i; }
EXPECT(10) { int i = 0; int sum = 0; while (i < 5) { sum = sum + i; i = i + 1; } return sum; }
EXPECT(5) { int i = 0; while ((i = i + 1) < 5) ; return i; }

EXPECT(10) { int sum = 0; int i; for (i = 0; i < 5; i = i + 1) sum = sum + i; return sum; }
EXPECT(10) { int sum = 0; char i; for (i = 0; i < 5; i = i + 1) sum = sum + i; return sum; }
EXPECT(10) { int sum = 0; int i; for (i = 4; i >= 0; i = i - 1) sum = sum + i; return sum; }
EXPECT(5) { int i = 0; for ( ; i < 5; i = i + 1) ; return i; }
EXPECT(5) { int i = 5; for ( ; i < 5; i = i + 1) ; return i; }
EXPECT(5) { int i = 0; for ( ; i < 5; ) i = i + 1; return i; }
EXPECT(34) {
    int sum = 0;
    int prod = 1;
    int i;
    for (i = 1; i < 5; i = i + 1) {
        sum = sum + i;
        prod = prod * i;
    }
    return sum + prod;
}
EXPECT(13) {
    int fib[7]; int i;
    fib[0] = 1;
    fib[1] = 1;
    for (i = 2; i < 7; i = i + 1)
        fib[i] = fib[i-1] + fib[i-2];
    return fib[6];
}

// String literals.
EXPECT(0) { char *s; s = ""; return *s; }
EXPECT(72) { char *s = "Hello, world!"; return s[0]; }
EXPECT(33) { char *s = "Hello, world!"; return *(s+12); }
EXPECT(0) { char *s = "Hello, world!"; return s[13]; }
EXPECT(32) { char *s1 = "ABC"; char *s2 = "abc"; return s2[1] - s1[1]; }
EXPECT(1) { return puts("Hello, world!") >= 0; }

