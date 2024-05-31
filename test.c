#define ASSERT(x, y) assert(x, y, #y)
// clang-format off

typedef int MyInt, MyInt2[4];
typedef int;

void assert(int expected, int actual, char *code); 
int printf(char *fmt, ...);
int sprintf(char *buf, char *fmt, ...);
int strcmp(char *p, char *q);
int memcmp(char *p, char *q, int n);

int ret3(void) {
    return 3;
    return 5;
}

int add2(int x, int y) {
    return x + y;
}

int sub2(int x, int y) {
    return x - y;
}

int add6(int a, int b, int c, int d, int e, int f) {
    return a + b + c + d + e + f;
}

int addx(int *x, int y) {
    return *x + y;
}

int sub_char(char a, char b, char c) {
    return a - b - c;
}

int fib(int x) {
    if (x<=1)
        return 1;
    return fib(x-1) + fib(x-2);
}

int _Alignas(512) ag1;
int _Alignas(512) ag2;
char ag3;
int ag4;
long ag5;
char ag6;

int g1, g2[4];

int *g1_ptr(void) {
    return &g1;
}

char int_to_char(int x) {
    return x;
}

int div_long(long a, long b) {
    return a/b;
}

_Bool bool_fn_add(_Bool x) { return x + 1; }

_Bool bool_fn_sub(_Bool x) { return x - 1; }

static int static_fn(void) { return 5; }

int param_decay(int x[]) { return x[0]; }

char g3 = 3;
short g4 = 4;
int g5 = 5;
long g6 = 6;
int g9[3] = {0, 1, 2};
struct {char a; int b;} g11[2] = {{1, 2}, {3, 4}};
struct {int a[2];} g12[2] = {{{1, 2}}};
union { int a; char b[8]; } g13[2] = {0x01020304, 0x05060708};
char g17[] = "foobar";
char g18[10] = "foobar";
char g19[3] = "foobar";
char *g20 = g17+0;
char *g21 = g17+3;
char *g22 = &g17-3;
char *g23[] = {g17+0, g17+3, g17-3};
int g24=3;
int *g25=&g24;
int g26[3] = {1, 2, 3};
int *g27 = g26 + 1;
int *g28 = &g11[1].a;
long g29 = (long)(long)g26;
struct { struct { int a[3]; } a; } g30 = {{{1,2,3}}};
int *g31=g30.a.a;
struct {int a[2];} g40[2] = {{1, 2}, 3, 4};
struct {int a[2];} g41[2] = {1, 2, 3, 4};
char g43[][4] = {'f', 'o', 'o', 0, 'b', 'a', 'r', 0};
char *g44 = {"foo"};

typedef char T60[];
T60 g60 = {1, 2, 3};
T60 g61 = {1, 2, 3, 4, 5, 6};

typedef struct { char a, b[]; } T65;
T65 g65 = {'f','o','o',0};
T65 g66 = {'f','o','o','b','a','r',0};

extern int ext1;
extern int *ext2;

typedef struct Tree {
    int val;
    struct Tree *lhs;
    struct Tree *rhs;
} Tree;

Tree *tree = &(Tree){
    1,
        &(Tree){
            2,
            &(Tree){ 3, 0, 0 },
            &(Tree){ 4, 0, 0 }
        },
        0
};

int counter() {
    static int i;
    static int j = 1+1;
    return i++ + j++;
}

void ret_none() {
    return;
}

static int sg1 = 3;

_Bool true_fn();
_Bool false_fn();
char char_fn();
short short_fn();

int add_all(int n, ...);

typedef struct {
    int gp_offset;
    int fp_offset;
    void *overflow_arg_area;
    void *reg_save_area;
} __va_elem;

typedef __va_elem va_list[1];

int add_all(int n, ...);
int sprintf(char *buf, char *fmt, ...);
int vsprintf(char *buf, char *fmt, va_list ap);

char *fmt(char *buf, char *fmt, ...) {
    va_list ap;
    *ap = *(__va_elem *)__va_area__;
    vsprintf(buf, fmt, ap);
}

unsigned char uchar_fn();
unsigned short ushort_fn();

char schar_fn();
short sshort_fn();

void funcy_type(int arg[restrict static 3]) {}

double add_double(double x, double y);
float add_float(float x, float y);

float add_float3(float x, float y, float z) {
  return x + y + z;
}

double add_double3(double x, double y, double z) {
  return x + y + z;
}

int main() {
    ASSERT(0, 0);
    ASSERT(42, 42);
    ASSERT(21, 5+20-4);
    ASSERT(41,  12 + 34 - 5 );
    ASSERT(47, 5+6*7);
    ASSERT(15, 5*(9-6));
    ASSERT(4, (3+5)/2);
    ASSERT(10, -10+20);
    ASSERT(10, - -10);
    ASSERT(10, - - +10);

    ASSERT(0, 0==1);
    ASSERT(1, 42==42);
    ASSERT(1, 0!=1);
    ASSERT(0, 42!=42);

    ASSERT(1, 0<1);
    ASSERT(0, 1<1);
    ASSERT(0, 2<1);
    ASSERT(1, 0<=1);
    ASSERT(1, 1<=1);
    ASSERT(0, 2<=1);

    ASSERT(1, 1>0);
    ASSERT(0, 1>1);
    ASSERT(0, 1>2);
    ASSERT(1, 1>=0);
    ASSERT(1, 1>=1);
    ASSERT(0, 1>=2);

    ASSERT(3, ({ int x; if (0) x=2; else x=3; x; }));
    ASSERT(3, ({ int x; if (1-1) x=2; else x=3; x; }));
    ASSERT(2, ({ int x; if (1) x=2; else x=3; x; }));
    ASSERT(2, ({ int x; if (2-1) x=2; else x=3; x; }));

    ASSERT(55, ({ int i=0; int j=0; for (i=0; i<=10; i=i+1) j=i+j; j; }));

    ASSERT(10, ({ int i=0; while(i<10) i=i+1; i; }));

    ASSERT(3, ({ 1; {2;} 3; }));
    ASSERT(5, ({ ;;; 5; }));

    ASSERT(10, ({ int i=0; while(i<10) i=i+1; i; }));
    ASSERT(55, ({ int i=0; int j=0; while(i<=10) {j=i+j; i=i+1;} j; }));

    ASSERT(3, ret3());
    ASSERT(8, add2(3, 5));
    ASSERT(2, sub2(5, 3));
    ASSERT(21, add6(1,2,3,4,5,6));
    ASSERT(66, add6(1,2,add6(3,4,5,6,7,8),9,10,11));
    ASSERT(136, add6(1,2,add6(3,add6(4,5,6,7,8,9),10,11,12,13),14,15,16));

    ASSERT(7, add2(3,4));
    ASSERT(1, sub2(4,3));
    ASSERT(55, fib(9));

    ASSERT(1, ({ sub_char(7, 3, 3); }));

    ASSERT(3, ({ int x=3; *&x; }));
    ASSERT(3, ({ int x=3; int *y=&x; int **z=&y; **z; }));
    ASSERT(5, ({ int x=3; int y=5; *(&x+1); }));
    ASSERT(3, ({ int x=3; int y=5; *(&y-1); }));
    ASSERT(5, ({ int x=3; int y=5; *(&x-(-1)); }));
    ASSERT(5, ({ int x=3; int *y=&x; *y=5; x; }));
    ASSERT(7, ({ int x=3; int y=5; *(&x+1)=7; y; }));
    ASSERT(7, ({ int x=3; int y=5; *(&y-2+1)=7; x; }));
    ASSERT(5, ({ int x=3; (&x+2)-&x+3; }));
    ASSERT(8, ({ int x, y; x=3; y=5; x+y; }));
    ASSERT(8, ({ int x=3, y=5; x+y; }));

    ASSERT(3, ({ int x[2]; int *y=&x; *y=3; *x; }));

    ASSERT(3, ({ int x[3]; *x=3; *(x+1)=4; *(x+2)=5; *x; }));
    ASSERT(4, ({ int x[3]; *x=3; *(x+1)=4; *(x+2)=5; *(x+1); }));
    ASSERT(5, ({ int x[3]; *x=3; *(x+1)=4; *(x+2)=5; *(x+2); }));

    ASSERT(0, ({ int x[2][3]; int *y=x; *y=0; **x; }));
    ASSERT(1, ({ int x[2][3]; int *y=x; *(y+1)=1; *(*x+1); }));
    ASSERT(2, ({ int x[2][3]; int *y=x; *(y+2)=2; *(*x+2); }));
    ASSERT(3, ({ int x[2][3]; int *y=x; *(y+3)=3; **(x+1); }));
    ASSERT(4, ({ int x[2][3]; int *y=x; *(y+4)=4; *(*(x+1)+1); }));
    ASSERT(5, ({ int x[2][3]; int *y=x; *(y+5)=5; *(*(x+1)+2); }));

    ASSERT(3, ({ int x[3]; *x=3; x[1]=4; x[2]=5; *x; }));
    ASSERT(4, ({ int x[3]; *x=3; x[1]=4; x[2]=5; *(x+1); }));
    ASSERT(5, ({ int x[3]; *x=3; x[1]=4; x[2]=5; *(x+2); }));
    ASSERT(5, ({ int x[3]; *x=3; x[1]=4; x[2]=5; *(x+2); }));
    ASSERT(5, ({ int x[3]; *x=3; x[1]=4; 2[x]=5; *(x+2); }));

    ASSERT(0, ({ int x[2][3]; int *y=x; y[0]=0; x[0][0]; }));
    ASSERT(1, ({ int x[2][3]; int *y=x; y[1]=1; x[0][1]; }));
    ASSERT(2, ({ int x[2][3]; int *y=x; y[2]=2; x[0][2]; }));
    ASSERT(3, ({ int x[2][3]; int *y=x; y[3]=3; x[1][0]; }));
    ASSERT(4, ({ int x[2][3]; int *y=x; y[4]=4; x[1][1]; }));
    ASSERT(5, ({ int x[2][3]; int *y=x; y[5]=5; x[1][2]; }));

    ASSERT(0, ""[0]);
    ASSERT(1, sizeof(""));

    ASSERT(97, "abc"[0]);
    ASSERT(98, "abc"[1]);
    ASSERT(99, "abc"[2]);
    ASSERT(0, "abc"[3]);
    ASSERT(4, sizeof("abc"));

    ASSERT(7, "\a"[0]);
    ASSERT(8, "\b"[0]);
    ASSERT(9, "\t"[0]);
    ASSERT(10, "\n"[0]);
    ASSERT(11, "\v"[0]);
    ASSERT(12, "\f"[0]);
    ASSERT(13, "\r"[0]);
    ASSERT(27, "\e"[0]);

    ASSERT(106, "\j"[0]);
    ASSERT(107, "\k"[0]);
    ASSERT(108, "\l"[0]);

    ASSERT(7, "\ax\ny"[0]);
    ASSERT(120, "\ax\ny"[1]);
    ASSERT(10, "\ax\ny"[2]);
    ASSERT(121, "\ax\ny"[3]);

    ASSERT(0, "\0"[0]);
    ASSERT(16, "\20"[0]);
    ASSERT(65, "\101"[0]);
    ASSERT(104, "\1500"[0]);
    ASSERT(0, "\x00"[0]);
    ASSERT(119, "\x77"[0]);

    ASSERT(3, ({ int a; a=3; a; }));
    ASSERT(3, ({ int a=3; a; }));
    ASSERT(8, ({ int a=3; int z=5; a+z; }));

    ASSERT(3, ({ int a=3; a; }));
    ASSERT(8, ({ int a=3; int z=5; a+z; }));
    ASSERT(6, ({ int a; int b; a=b=3; a+b; }));
    ASSERT(3, ({ int foo=3; foo; }));
    ASSERT(8, ({ int foo123=3; int bar=5; foo123+bar; }));

    ASSERT(4, ({ int x; sizeof(x); }));
    ASSERT(4, ({ int x; sizeof x; }));
    ASSERT(8, ({ int *x; sizeof(x); }));
    ASSERT(16, ({ int x[4]; sizeof(x); }));
    ASSERT(48, ({ int x[3][4]; sizeof(x); }));
    ASSERT(16, ({ int x[3][4]; sizeof(*x); }));
    ASSERT(4, ({ int x[3][4]; sizeof(**x); }));
    ASSERT(5, ({ int x[3][4]; sizeof(**x) + 1; }));
    ASSERT(5, ({ int x[3][4]; sizeof **x + 1; }));
    ASSERT(4, ({ int x[3][4]; sizeof(**x + 1); }));
    ASSERT(4, ({ int x=1; sizeof(x=2); }));
    ASSERT(1, ({ int x=1; sizeof(x=2); x; }));

    ASSERT(0, g1);
    ASSERT(3, ({ g1=3; g1; }));
    ASSERT(0, ({ g2[0]=0; g2[1]=1; g2[2]=2; g2[3]=3; g2[0]; }));
    ASSERT(1, ({ g2[0]=0; g2[1]=1; g2[2]=2; g2[3]=3; g2[1]; }));
    ASSERT(2, ({ g2[0]=0; g2[1]=1; g2[2]=2; g2[3]=3; g2[2]; }));
    ASSERT(3, ({ g2[0]=0; g2[1]=1; g2[2]=2; g2[3]=3; g2[3]; }));

    ASSERT(4, sizeof(g1));
    ASSERT(16, sizeof(g2));

    ASSERT(1, ({ char x=1; x; }));
    ASSERT(1, ({ char x=1; char y=2; x; }));
    ASSERT(2, ({ char x=1; char y=2; y; }));

    ASSERT(1, ({ char x; sizeof(x); }));
    ASSERT(10, ({ char x[10]; sizeof(x); }));

    ASSERT(2, ({ int x=2; { int x=3; } x; }));
    ASSERT(2, ({ int x=2; { int x=3; } int y=4; x; }));
    ASSERT(3, ({ int x=2; { x=3; } x; }));

    ASSERT(3, (1,2,3));
    ASSERT(5, ({ int i=2, j=3; (i=5,j)=6; i; }));
    ASSERT(6, ({ int i=2, j=3; (i=5,j)=6; j; }));

    ASSERT(1, ({ struct {int a; int b;} x; x.a=1; x.b=2; x.a; }));
    ASSERT(2, ({ struct {int a; int b;} x; x.a=1; x.b=2; x.b; }));
    ASSERT(1, ({ struct {char a; int b; char c;} x; x.a=1; x.b=2; x.c=3; x.a; }));
    ASSERT(2, ({ struct {char a; int b; char c;} x; x.b=1; x.b=2; x.c=3; x.b; }));
    ASSERT(3, ({ struct {char a; int b; char c;} x; x.a=1; x.b=2; x.c=3; x.c; }));

    ASSERT(0, ({ struct {char a; char b;} x[3]; char *p=x; p[0]=0; x[0].a; }));
    ASSERT(1, ({ struct {char a; char b;} x[3]; char *p=x; p[1]=1; x[0].b; }));
    ASSERT(2, ({ struct {char a; char b;} x[3]; char *p=x; p[2]=2; x[1].a; }));
    ASSERT(3, ({ struct {char a; char b;} x[3]; char *p=x; p[3]=3; x[1].b; }));

    ASSERT(6, ({ struct {char a[3]; char b[5];} x; char *p=&x; x.a[0]=6; p[0]; }));
    ASSERT(7, ({ struct {char a[3]; char b[5];} x; char *p=&x; x.b[0]=7; p[3]; }));

    ASSERT(6, ({ struct { struct { char b; } a; } x; x.a.b=6; x.a.b; }));

    ASSERT(4, ({ struct {int a;} x; sizeof(x); }));
    ASSERT(8, ({ struct {int a; int b;} x; sizeof(x); }));
    ASSERT(8, ({ struct {int a, b;} x; sizeof(x); }));
    ASSERT(12, ({ struct {int a[3];} x; sizeof(x); }));
    ASSERT(16, ({ struct {int a;} x[4]; sizeof(x); }));
    ASSERT(24, ({ struct {int a[3];} x[2]; sizeof(x); }));
    ASSERT(2, ({ struct {char a; char b;} x; sizeof(x); }));
    ASSERT(0, ({ struct {} x; sizeof(x); }));
    ASSERT(8, ({ struct {char a; int b;} x; sizeof(x); }));
    ASSERT(8, ({ struct {int b; char a;} x; sizeof(x); }));

    ASSERT(7, ({ int x; int y; char z; char *a=&y; char *b=&z; b-a; }));
    ASSERT(1, ({ int x; char y; int z; char *a=&y; char *b=&z; b-a; }));

    ASSERT(8, ({ struct t {int a; int b;} x; struct t y; sizeof(y); }));
    ASSERT(8, ({ struct t {int a; int b;}; struct t y; sizeof(y); }));
    ASSERT(2, ({ struct t {char a[2];}; { struct t {char a[4];}; } struct t y; sizeof(y); }));
    ASSERT(3, ({ struct t {int x;}; int t=1; struct t y; y.x=2; t+y.x; }));

    ASSERT(3, ({ struct t {char a;} x; struct t *y = &x; x.a=3; y->a; }));
    ASSERT(3, ({ struct t {char a;} x; struct t *y = &x; y->a=3; x.a; }));

    ASSERT(8, ({ union { int a; char b[6]; } x; sizeof(x); }));
    ASSERT(3, ({ union { int a; char b[4]; } x; x.a = 515; x.b[0]; }));
    ASSERT(2, ({ union { int a; char b[4]; } x; x.a = 515; x.b[1]; }));
    ASSERT(0, ({ union { int a; char b[4]; } x; x.a = 515; x.b[2]; }));
    ASSERT(0, ({ union { int a; char b[4]; } x; x.a = 515; x.b[3]; }));

    ASSERT(3, ({ struct {int a,b;} x,y; x.a=3; y=x; y.a; }));
    ASSERT(7, ({ struct t {int a,b;}; struct t x; x.a=7; struct t y; struct t *z=&y; *z=x; y.a; }));
    ASSERT(7, ({ struct t {int a,b;}; struct t x; x.a=7; struct t y, *p=&x, *q=&y; *q=*p; y.a; }));
    ASSERT(5, ({ struct t {char a, b;} x, y; x.a=5; y=x; y.a; }));

    ASSERT(3, ({ struct {int a,b;} x,y; x.a=3; y=x; y.a; }));
    ASSERT(7, ({ struct t {int a,b;}; struct t x; x.a=7; struct t y; struct t *z=&y; *z=x; y.a; }));
    ASSERT(7, ({ struct t {int a,b;}; struct t x; x.a=7; struct t y, *p=&x, *q=&y; *q=*p; y.a; }));
    ASSERT(5, ({ struct t {char a, b;} x, y; x.a=5; y=x; y.a; }));

    ASSERT(3, ({ union {int a,b;} x,y; x.a=3; y.a=5; y=x; y.a; }));
    ASSERT(3, ({ union {struct {int a,b;} c;} x,y; x.c.b=3; y.c.b=5; y=x; y.c.b; }));

    ASSERT(16, ({ struct {char a; long b;} x; sizeof(x); }));
    ASSERT(8, ({ long x; sizeof(x); }));

    ASSERT(4, ({ struct {char a; short b;} x; sizeof(x); }));
    ASSERT(2, ({ short x = 9; sizeof(x); }));

    ASSERT(24, ({ char *x[3]; sizeof(x); }));
    ASSERT(8, ({ char (*x)[3]; sizeof(x); }));
    ASSERT(1, ({ char (x); sizeof(x); }));
    ASSERT(3, ({ char (x)[3]; sizeof(x); }));
    ASSERT(12, ({ char (x[3])[4]; sizeof(x); }));
    ASSERT(4, ({ char (x[3])[4]; sizeof(x[0]); }));
    ASSERT(3, ({ char *x[3]; char y; x[0]=&y; y=3; x[0][0]; }));
    ASSERT(4, ({ char x[3]; char (*y)[3]=x; y[0][0]=4; y[0][0]; }));

    { void *x; }

    ASSERT(1, ({ char x; sizeof(x); }));
    ASSERT(2, ({ short int x; sizeof(x); }));
    ASSERT(2, ({ int short x; sizeof(x); }));
    ASSERT(4, ({ int x; sizeof(x); }));
    ASSERT(8, ({ long int x; sizeof(x); }));
    ASSERT(8, ({ int long x; sizeof(x); }));
    ASSERT(8, ({ long long x; sizeof(x); }));

    ASSERT(1, ({ typedef int t; t x=1; x; }));
    ASSERT(1, ({ typedef struct {int a;} t; t x; x.a=1; x.a; }));
    ASSERT(2, ({ typedef struct {int a;} t; { typedef int t; } t x; x.a=2; x.a; }));
    ASSERT(4, ({ typedef t; t x; sizeof(x); }));
    ASSERT(3, ({ MyInt x=3; x; }));
    ASSERT(16, ({ MyInt2 x; sizeof(x); }));

    ASSERT(1, sizeof(char));
    ASSERT(2, sizeof(short));
    ASSERT(2, sizeof(short int));
    ASSERT(2, sizeof(int short));
    ASSERT(4, sizeof(int));
    ASSERT(8, sizeof(long));
    ASSERT(8, sizeof(long int));
    ASSERT(8, sizeof(long int));
    ASSERT(8, sizeof(char *));
    ASSERT(8, sizeof(int *));
    ASSERT(8, sizeof(long *));
    ASSERT(8, sizeof(int **));
    ASSERT(8, sizeof(int(*)[4]));
    ASSERT(32, sizeof(int*[4]));
    ASSERT(16, sizeof(int[4]));
    ASSERT(48, sizeof(int[3][4]));
    ASSERT(8, sizeof(struct {int a; int b;}));

    ASSERT(131585, (int)8590066177);
    ASSERT(513, (short)8590066177);
    ASSERT(1, (char)8590066177);
    ASSERT(1, (long)1);
    ASSERT(0, (long)&*(int *)0);
    ASSERT(513, ({ int x=512; *(char *)&x=1; x; }));
    ASSERT(5, ({ int x=5; long y=(long)&x; *(int*)y; }));

    (void)1;

    ASSERT(0, 1073741824 * 100 / 100);

    ASSERT(8, sizeof(-10 + (long)5));
    ASSERT(8, sizeof(-10 - (long)5));
    ASSERT(8, sizeof(-10 * (long)5));
    ASSERT(8, sizeof(-10 / (long)5));
    ASSERT(8, sizeof((long)-10 + 5));
    ASSERT(8, sizeof((long)-10 - 5));
    ASSERT(8, sizeof((long)-10 * 5));
    ASSERT(8, sizeof((long)-10 / 5));

    ASSERT((long)-5, -10 + (long)5);
    ASSERT((long)-15, -10 - (long)5);
    ASSERT((long)-50, -10 * (long)5);
    ASSERT((long)-2, -10 / (long)5);

    ASSERT(1, -2 < (long)-1);
    ASSERT(1, -2 <= (long)-1);
    ASSERT(0, -2 > (long)-1);
    ASSERT(0, -2 >= (long)-1);

    ASSERT(1, (long)-2 < -1);
    ASSERT(1, (long)-2 <= -1);
    ASSERT(0, (long)-2 > -1);
    ASSERT(0, (long)-2 >= -1);

    ASSERT(0, 2147483647 + 2147483647 + 2);
    ASSERT((long)-1, ({ long x; x=-1; x; }));

    ASSERT(1, ({ char x[3]; x[0]=0; x[1]=1; x[2]=2; char *y=x+1; y[0]; }));
    ASSERT(0, ({ char x[3]; x[0]=0; x[1]=1; x[2]=2; char *y=x+1; y[-1]; }));
    ASSERT(5, ({ struct t {char a;} x, y; x.a=5; y=x; y.a; }));

    g1 = 3;

    ASSERT(3, *g1_ptr());
    ASSERT(5, int_to_char(261));
    ASSERT(-5, div_long(-10, 2));

    ASSERT(0, ({ _Bool x=0; x; }));
    ASSERT(1, ({ _Bool x=1; x; }));
    ASSERT(1, ({ _Bool x=2; x; }));
    ASSERT(1, (_Bool)1);
    ASSERT(1, (_Bool)2);
    ASSERT(0, (_Bool)(char)256);

    ASSERT(1, bool_fn_add(3));
    ASSERT(0, bool_fn_sub(3));
    ASSERT(1, bool_fn_add(-3));
    ASSERT(0, bool_fn_sub(-3));
    ASSERT(1, bool_fn_add(0));
    ASSERT(1, bool_fn_sub(0));

    ASSERT(97, 'a');
    ASSERT(10, '\n');
    ASSERT(-128, '\x80');

    ASSERT(0, ({ enum { zero, one, two }; zero; }));
    ASSERT(1, ({ enum { zero, one, two }; one; }));
    ASSERT(2, ({ enum { zero, one, two }; two; }));
    ASSERT(5, ({ enum { five=5, six, seven }; five; }));
    ASSERT(6, ({ enum { five=5, six, seven }; six; }));
    ASSERT(0, ({ enum { zero, five=5, three=3, four }; zero; }));
    ASSERT(5, ({ enum { zero, five=5, three=3, four }; five; }));
    ASSERT(3, ({ enum { zero, five=5, three=3, four }; three; }));
    ASSERT(4, ({ enum { zero, five=5, three=3, four }; four; }));
    ASSERT(4, ({ enum { zero, one, two } x; sizeof(x); }));
    ASSERT(4, ({ enum t { zero, one, two }; enum t y; sizeof(y); }));

    ASSERT(5, static_fn());

    ASSERT(55, ({ int j=0; for (int i=0; i<=10; i=i+1) j=j+i; j; }));
    ASSERT(3, ({ int i=3; int j=0; for (int i=0; i<=10; i=i+1) j=j+i; i; }));

    ASSERT(7, ({ int i=2; i+=5; i; }));
    ASSERT(7, ({ int i=2; i+=5; }));
    ASSERT(3, ({ int i=5; i-=2; i; }));
    ASSERT(3, ({ int i=5; i-=2; }));
    ASSERT(6, ({ int i=3; i*=2; i; }));
    ASSERT(6, ({ int i=3; i*=2; }));
    ASSERT(3, ({ int i=6; i/=2; i; }));
    ASSERT(3, ({ int i=6; i/=2; }));

    ASSERT(3, ({ int i=2; ++i; }));
    ASSERT(2, ({ int a[3]; a[0]=0; a[1]=1; a[2]=2; int *p=a+1; ++*p; }));
    ASSERT(0, ({ int a[3]; a[0]=0; a[1]=1; a[2]=2; int *p=a+1; --*p; }));

    ASSERT(1, ({ char i; sizeof(++i); }));

    ASSERT(2, ({ int i=2; i++; }));
    ASSERT(2, ({ int i=2; i--; }));
    ASSERT(3, ({ int i=2; i++; i; }));
    ASSERT(1, ({ int i=2; i--; i; }));
    ASSERT(1, ({ int a[3]; a[0]=0; a[1]=1; a[2]=2; int *p=a+1; *p++; }));
    ASSERT(1, ({ int a[3]; a[0]=0; a[1]=1; a[2]=2; int *p=a+1; *p--; }));

    ASSERT(0, ({ int a[3]; a[0]=0; a[1]=1; a[2]=2; int *p=a+1; (*p++)--; a[0]; }));
    ASSERT(0, ({ int a[3]; a[0]=0; a[1]=1; a[2]=2; int *p=a+1; (*(p--))--; a[1]; }));
    ASSERT(2, ({ int a[3]; a[0]=0; a[1]=1; a[2]=2; int *p=a+1; (*p)--; a[2]; }));
    ASSERT(2, ({ int a[3]; a[0]=0; a[1]=1; a[2]=2; int *p=a+1; (*p)--; p++; *p; }));

    ASSERT(0, ({ int a[3]; a[0]=0; a[1]=1; a[2]=2; int *p=a+1; (*p++)--; a[0]; }));
    ASSERT(0, ({ int a[3]; a[0]=0; a[1]=1; a[2]=2; int *p=a+1; (*p++)--; a[1]; }));
    ASSERT(2, ({ int a[3]; a[0]=0; a[1]=1; a[2]=2; int *p=a+1; (*p++)--; a[2]; }));
    ASSERT(2, ({ int a[3]; a[0]=0; a[1]=1; a[2]=2; int *p=a+1; (*p++)--; *p; }));

    ASSERT(1, ({ char i; sizeof(i++); }));

    ASSERT(511, 0777);
    ASSERT(0, 0x0);
    ASSERT(10, 0xa);
    ASSERT(10, 0XA);
    ASSERT(48879, 0xbeef);
    ASSERT(48879, 0xBEEF);
    ASSERT(48879, 0XBEEF);
    ASSERT(0, 0b0);
    ASSERT(1, 0b1);
    ASSERT(47, 0b101111);
    ASSERT(47, 0B101111);

    ASSERT(0, !1);
    ASSERT(0, !2);
    ASSERT(1, !0);
    ASSERT(1, !(char)0);
    ASSERT(0, !(long)3);
    ASSERT(4, sizeof(!(char)0));
    ASSERT(4, sizeof(!(long)0));

    ASSERT(-1, ~0);
    ASSERT(0, ~-1);

    ASSERT(5, 17%6);
    ASSERT(5, ((long)17)%6);
    ASSERT(2, ({ int i=10; i%=4; i; }));
    ASSERT(2, ({ long i=10; i%=4; i; }));

    ASSERT(0, 0&1);
    ASSERT(1, 3&1);
    ASSERT(3, 7&3);
    ASSERT(10, -1&10);

    ASSERT(1, 0|1);
    ASSERT(0b10011, 0b10000|0b00011);

    ASSERT(0, 0^0);
    ASSERT(0, 0b1111^0b1111);
    ASSERT(0b110100, 0b111000^0b001100);

    ASSERT(2, ({ int i=6; i&=3; i; }));
    ASSERT(7, ({ int i=6; i|=3; i; }));
    ASSERT(10, ({ int i=15; i^=5; i; }));

    ASSERT(1, 0||1);
    ASSERT(1, 0||(2-2)||5);
    ASSERT(0, 0||0);
    ASSERT(0, 0||(2-2));

    ASSERT(0, 0&&1);
    ASSERT(0, (2-2)&&5);
    ASSERT(1, 1&&5);

    ASSERT(8, sizeof(int(*)[10]));
    ASSERT(8, sizeof(int(*)[][10]));

    ASSERT(3, ({ int x[2]; x[0]=3; param_decay(x); }));

    ASSERT(8, ({ struct foo *bar; sizeof(bar); }));
    ASSERT(4, ({ struct T *foo; struct T {int x;}; sizeof(struct T); }));
    ASSERT(1, ({ struct T { struct T *next; int x; } a; struct T b; b.x=1; a.next=&b; a.next->x; }));
    ASSERT(4, ({ typedef struct T T; struct T { int x; }; sizeof(T); }));

    ASSERT(3, ({ int i=0; goto a; a: i++; b: i++; c: i++; i; }));
    ASSERT(2, ({ int i=0; goto e; d: i++; e: i++; f: i++; i; }));
    ASSERT(1, ({ int i=0; goto i; g: i++; h: i++; i: i++; i; }));

    ASSERT(1, ({ typedef int foo; goto foo; foo:; 1; }));

    ASSERT(3, ({ int i=0; for(;i<10;i++) { if (i == 3) break; } i; }));
    ASSERT(4, ({ int i=0; while (1) { if (i++ == 3) break; } i; }));
    ASSERT(3, ({ int i=0; for(;i<10;i++) { for (;;) break; if (i == 3) break; } i; }));
    ASSERT(4, ({ int i=0; while (1) { while(1) break; if (i++ == 3) break; } i; }));

    ASSERT(10, ({ int i=0; int j=0; for (;i<10;i++) { if (i>5) continue; j++; } i; }));
    ASSERT(6, ({ int i=0; int j=0; for (;i<10;i++) { if (i>5) continue; j++; } j; }));
    ASSERT(10, ({ int i=0; int j=0; for(;!i;) { for (;j!=10;j++) continue; break; } j; }));
    ASSERT(11, ({ int i=0; int j=0; while (i++<10) { if (i>5) continue; j++; } i; }));
    ASSERT(5, ({ int i=0; int j=0; while (i++<10) { if (i>5) continue; j++; } j; }));
    ASSERT(11, ({ int i=0; int j=0; while(!i) { while (j++!=10) continue; break; } j; }));

    ASSERT(5, ({ int i=0; switch(0) { case 0:i=5;break; case 1:i=6;break; case 2:i=7;break; } i; }));
    ASSERT(6, ({ int i=0; switch(1) { case 0:i=5;break; case 1:i=6;break; case 2:i=7;break; } i; }));
    ASSERT(7, ({ int i=0; switch(2) { case 0:i=5;break; case 1:i=6;break; case 2:i=7;break; } i; }));
    ASSERT(0, ({ int i=0; switch(3) { case 0:i=5;break; case 1:i=6;break; case 2:i=7;break; } i; }));
    ASSERT(5, ({ int i=0; switch(0) { case 0:i=5;break; default:i=7; } i; }));
    ASSERT(7, ({ int i=0; switch(1) { case 0:i=5;break; default:i=7; } i; }));
    ASSERT(2, ({ int i=0; switch(1) { case 0: 0; case 1: 0; case 2: 0; i=2; } i; }));
    ASSERT(0, ({ int i=0; switch(3) { case 0: 0; case 1: 0; case 2: 0; i=2; } i; }));

    ASSERT(3, ({ int i=0; switch(-1) { case 0xffffffff: i=3; break; } i; }));

    ASSERT(1, 1<<0);
    ASSERT(8, 1<<3);
    ASSERT(10, 5<<1);
    ASSERT(2, 5>>1);
    ASSERT(-1, -1>>1);
    ASSERT(1, ({ int i=1; i<<=0; i; }));
    ASSERT(8, ({ int i=1; i<<=3; i; }));
    ASSERT(10, ({ int i=5; i<<=1; i; }));
    ASSERT(2, ({ int i=5; i>>=1; i; }));
    ASSERT(-1, -1);
    ASSERT(-1, ({ int i=-1; i; }));
    ASSERT(-1, ({ int i=-1; i>>=1; i; }));

    ASSERT(2, 0?1:2);
    ASSERT(1, 1?1:2);
    ASSERT(-1, 0?-2:-1);
    ASSERT(-2, 1?-2:-1);
    ASSERT(4, sizeof(0?1:2));
    ASSERT(8, sizeof(0?(long)1:(long)2));
    ASSERT(-1, 0?(long)-2:-1);
    ASSERT(-1, 0?-2:(long)-1);
    ASSERT(-2, 1?(long)-2:-1);
    ASSERT(-2, 1?-2:(long)-1);

    1 ? -2 : (void)-1;

    ASSERT(10, ({ enum { ten=1+2+3+4 }; ten; }));
    ASSERT(1, ({ int i=0; switch(3) { case 5-2+0*3: i++; } i; }));
    ASSERT(8, ({ int x[1+1]; sizeof(x); }));
    ASSERT(6, ({ char x[8-2]; sizeof(x); }));
    ASSERT(6, ({ char x[2*3]; sizeof(x); }));
    ASSERT(3, ({ char x[12/4]; sizeof(x); }));
    ASSERT(2, ({ char x[12%10]; sizeof(x); }));
    ASSERT(0b100, ({ char x[0b110&0b101]; sizeof(x); }));
    ASSERT(0b111, ({ char x[0b110|0b101]; sizeof(x); }));
    ASSERT(0b110, ({ char x[0b111^0b001]; sizeof(x); }));
    ASSERT(4, ({ char x[1<<2]; sizeof(x); }));
    ASSERT(2, ({ char x[4>>1]; sizeof(x); }));
    ASSERT(2, ({ char x[(1==1)+1]; sizeof(x); }));
    ASSERT(1, ({ char x[(1!=1)+1]; sizeof(x); }));
    ASSERT(1, ({ char x[(1<1)+1]; sizeof(x); }));
    ASSERT(2, ({ char x[(1<=1)+1]; sizeof(x); }));
    ASSERT(2, ({ char x[1?2:3]; sizeof(x); }));
    ASSERT(3, ({ char x[0?2:3]; sizeof(x); }));
    ASSERT(3, ({ char x[(1,3)]; sizeof(x); }));
    ASSERT(2, ({ char x[!0+1]; sizeof(x); }));
    ASSERT(1, ({ char x[!1+1]; sizeof(x); }));
    ASSERT(2, ({ char x[~-3]; sizeof(x); }));
    ASSERT(2, ({ char x[(5||6)+1]; sizeof(x); }));
    ASSERT(1, ({ char x[(0||0)+1]; sizeof(x); }));
    ASSERT(2, ({ char x[(1&&1)+1]; sizeof(x); }));
    ASSERT(1, ({ char x[(1&&0)+1]; sizeof(x); }));
    ASSERT(3, ({ char x[(int)3]; sizeof(x); }));
    ASSERT(15, ({ char x[(char)0xffffff0f]; sizeof(x); }));
    ASSERT(0x10f, ({ char x[(short)0xffff010f]; sizeof(x); }));
    ASSERT(4, ({ char x[(int)0xfffffffffff+5]; sizeof(x); }));
    ASSERT(8, ({ char x[(int*)0+2]; sizeof(x); }));
    ASSERT(12, ({ char x[(int*)16-1]; sizeof(x); }));
    ASSERT(3, ({ char x[(int*)16-(int*)4]; sizeof(x); }));

    ASSERT(1, ({ int x[3]={1,2,3}; x[0]; }));
    ASSERT(2, ({ int x[3]={1,2,3}; x[1]; }));
    ASSERT(3, ({ int x[3]={1,2,3}; x[2]; }));
    ASSERT(3, ({ int x[3]={1,2,3}; x[2]; }));

    ASSERT(2, ({ int x[2][3]={{1,2,3},{4,5,6}}; x[0][1]; }));
    ASSERT(4, ({ int x[2][3]={{1,2,3},{4,5,6}}; x[1][0]; }));
    ASSERT(6, ({ int x[2][3]={{1,2,3},{4,5,6}}; x[1][2]; }));

    ASSERT(0, ({ int x[3]={}; x[0]; }));
    ASSERT(0, ({ int x[3]={}; x[1]; }));
    ASSERT(0, ({ int x[3]={}; x[2]; }));

    ASSERT(2, ({ int x[2][3]={{1,2}}; x[0][1]; }));
    ASSERT(0, ({ int x[2][3]={{1,2}}; x[1][0]; }));
    ASSERT(0, ({ int x[2][3]={{1,2}}; x[1][2]; }));

    ASSERT('a', ({ char x[4]="abc"; x[0]; }));
    ASSERT('c', ({ char x[4]="abc"; x[2]; }));
    ASSERT(0, ({ char x[4]="abc"; x[3]; }));
    ASSERT('a', ({ char x[2][4]={"abc","def"}; x[0][0]; }));
    ASSERT(0, ({ char x[2][4]={"abc","def"}; x[0][3]; }));
    ASSERT('d', ({ char x[2][4]={"abc","def"}; x[1][0]; }));
    ASSERT('f', ({ char x[2][4]={"abc","def"}; x[1][2]; }));

    ASSERT(4, ({ int x[]={1,2,3,4}; x[3]; }));
    ASSERT(16, ({ int x[]={1,2,3,4}; sizeof(x); }));
    ASSERT(4, ({ char x[]="foo"; sizeof(x); }));

    ASSERT(4, ({ typedef char T[]; T x="foo"; T y="x"; sizeof(x); }));
    ASSERT(2, ({ typedef char T[]; T x="foo"; T y="x"; sizeof(y); }));
    ASSERT(2, ({ typedef char T[]; T x="x"; T y="foo"; sizeof(x); }));
    ASSERT(4, ({ typedef char T[]; T x="x"; T y="foo"; sizeof(y); }));

    ASSERT(1, ({ struct {int a; int b; int c;} x={1,2,3}; x.a; }));
    ASSERT(2, ({ struct {int a; int b; int c;} x={1,2,3}; x.b; }));
    ASSERT(3, ({ struct {int a; int b; int c;} x={1,2,3}; x.c; }));
    ASSERT(1, ({ struct {int a; int b; int c;} x={1}; x.a; }));
    ASSERT(0, ({ struct {int a; int b; int c;} x={1}; x.b; }));
    ASSERT(0, ({ struct {int a; int b; int c;} x={1}; x.c; }));

    ASSERT(1, ({ struct {int a; int b;} x[2]={{1,2},{3,4}}; x[0].a; }));
    ASSERT(2, ({ struct {int a; int b;} x[2]={{1,2},{3,4}}; x[0].b; }));
    ASSERT(3, ({ struct {int a; int b;} x[2]={{1,2},{3,4}}; x[1].a; }));
    ASSERT(4, ({ struct {int a; int b;} x[2]={{1,2},{3,4}}; x[1].b; }));

    ASSERT(0, ({ struct {int a; int b;} x[2]={{1,2}}; x[1].b; }));

    ASSERT(0, ({ struct {int a; int b;} x={}; x.a; }));
    ASSERT(0, ({ struct {int a; int b;} x={}; x.b; }));

    ASSERT(5, ({ typedef struct {int a,b,c,d,e,f;} T; T x={1,2,3,4,5,6}; T y; y=x; y.e; }));
    ASSERT(2, ({ typedef struct {int a,b;} T; T x={1,2}; T y, z; z=y=x; z.b; }));

    ASSERT(1, ({ typedef struct {int a,b;} T; T x={1,2}; T y=x; y.a;}));

    ASSERT(4, ({ union { int a; char b[4]; } x={0x01020304}; x.b[0]; }));
    ASSERT(3, ({ union { int a; char b[4]; } x={0x01020304}; x.b[1]; }));

    ASSERT(0x01020304, ({ union { struct { char a,b,c,d; } e; int f; } x={{4,3,2,1}}; x.f; }));

    ASSERT(3, g3);
    ASSERT(4, g4);
    ASSERT(5, g5);
    ASSERT(6, g6);

    ASSERT(0, g9[0]);
    ASSERT(1, g9[1]);
    ASSERT(2, g9[2]);

    ASSERT(1, g11[0].a);
    ASSERT(2, g11[0].b);
    ASSERT(3, g11[1].a);
    ASSERT(4, g11[1].b);

    ASSERT(1, g12[0].a[0]);
    ASSERT(2, g12[0].a[1]);
    ASSERT(0, g12[1].a[0]);
    ASSERT(0, g12[1].a[1]);

    ASSERT(4, g13[0].b[0]);
    ASSERT(3, g13[0].b[1]);
    ASSERT(8, g13[1].b[0]);
    ASSERT(7, g13[1].b[1]);

    ASSERT(7, sizeof(g17));
    ASSERT(10, sizeof(g18));
    ASSERT(3, sizeof(g19));

    ASSERT(0, memcmp(g17, "foobar", 7));
    ASSERT(0, memcmp(g18, "foobar\0\0\0", 10));
    ASSERT(0, memcmp(g19, "foo", 3));

    ASSERT(0, strcmp(g20, "foobar"));
    ASSERT(0, strcmp(g21, "bar"));
    ASSERT(0, strcmp(g22+3, "foobar"));

    ASSERT(0, strcmp(g23[0], "foobar"));
    ASSERT(0, strcmp(g23[1], "bar"));
    ASSERT(0, strcmp(g23[2]+3, "foobar"));

    ASSERT(3, g24);
    ASSERT(3, *g25);
    ASSERT(2, *g27);
    ASSERT(3, *g28);
    ASSERT(1, *(int *)g29);

    ASSERT(1, g31[0]);
    ASSERT(2, g31[1]);
    ASSERT(3, g31[2]);

    ASSERT(1, g40[0].a[0]);
    ASSERT(2, g40[0].a[1]);
    ASSERT(3, g40[1].a[0]);
    ASSERT(4, g40[1].a[1]);

    ASSERT(1, g41[0].a[0]);
    ASSERT(2, g41[0].a[1]);
    ASSERT(3, g41[1].a[0]);
    ASSERT(4, g41[1].a[1]);

    ASSERT(0, ({ int x[2][3]={0,1,2,3,4,5}; x[0][0]; }));
    ASSERT(3, ({ int x[2][3]={0,1,2,3,4,5}; x[1][0]; }));

    ASSERT(0, ({ struct {int a; int b;} x[2]={0,1,2,3}; x[0].a; }));
    ASSERT(2, ({ struct {int a; int b;} x[2]={0,1,2,3}; x[1].a; }));

    ASSERT(0, strcmp(g43[0], "foo"));
    ASSERT(0, strcmp(g43[1], "bar"));
    ASSERT(0, strcmp(g44, "foo"));

    ASSERT(3, ({ int a[]={1,2,3,}; a[2]; }));
    ASSERT(1, ({ struct {int a,b,c;} x={1,2,3,}; x.a; }));
    ASSERT(1, ({ union {int a; char b;} x={1,}; x.a; }));
    ASSERT(2, ({ enum {x,y,z,}; z; }));

    ASSERT(4, sizeof(struct { int x, y[]; }));

    ASSERT(3, sizeof(g60));
    ASSERT(6, sizeof(g61));

    ASSERT(4, sizeof(g65));
    ASSERT(7, sizeof(g66));
    ASSERT(0, strcmp(g65.b, "oo"));
    ASSERT(0, strcmp(g66.b, "oobar"));

    ASSERT(5, ext1);
    ASSERT(5, *ext2);

    extern int ext3;
    ASSERT(7, ext3);

    int ext_fn1(int x);
    ASSERT(5, ext_fn1(5));

    extern int ext_fn2(int x);
    ASSERT(8, ext_fn2(8));

    ASSERT(1, _Alignof(char));
    ASSERT(2, _Alignof(short));
    ASSERT(4, _Alignof(int));
    ASSERT(8, _Alignof(long));
    ASSERT(8, _Alignof(long long));
    ASSERT(1, _Alignof(char[3]));
    ASSERT(4, _Alignof(int[3]));
    ASSERT(1, _Alignof(struct {char a; char b;}[2]));
    ASSERT(8, _Alignof(struct {char a; long b;}[2]));

    ASSERT(1, ({ _Alignas(char) char x, y; &y-&x; }));
    ASSERT(8, ({ _Alignas(long) char x, y; &y-&x; }));
    ASSERT(32, ({ _Alignas(32) char x, y; &y-&x; }));
    ASSERT(32, ({ _Alignas(32) int *x, *y; ((char *)&y)-((char *)&x); }));
    ASSERT(16, ({ struct { _Alignas(16) char x, y; } a; &a.y-&a.x; }));
    ASSERT(8, ({ struct T { _Alignas(8) char a; }; _Alignof(struct T); }));

    ASSERT(0, (long)(char *)&ag1 % 512);
    ASSERT(0, (long)(char *)&ag2 % 512);
    ASSERT(0, (long)(char *)&ag4 % 4);
    ASSERT(0, (long)(char *)&ag5 % 8);

    ASSERT(1, ({ char x; _Alignof(x); }));
    ASSERT(4, ({ int x; _Alignof(x); }));
    ASSERT(1, ({ char x; _Alignof x; }));
    ASSERT(4, ({ int x; _Alignof x; }));

    ASSERT(2, counter());
    ASSERT(4, counter());
    ASSERT(6, counter());

    ASSERT(1, (int){1});
    ASSERT(2, ((int[]){0,1,2})[2]);
    ASSERT('a', ((struct {char a; int b;}){'a', 3}).a);
    ASSERT(3, ({ int x=3; (int){x}; }));
    (int){3} = 5;

    ASSERT(1, tree->val);
    ASSERT(2, tree->lhs->val);
    ASSERT(3, tree->lhs->lhs->val);
    ASSERT(4, tree->lhs->rhs->val);

    ret_none();

    ASSERT(3, sg1);

    ASSERT(7, ({ int i=0; int j=0; do { j++; } while (i++ < 6); j; }));
    ASSERT(4, ({ int i=0; int j=0; int k=0; do { if (++j > 3) break; continue; k++; } while (1); j; }));

    ASSERT(1, true_fn());
    ASSERT(0, false_fn());
    ASSERT(3, char_fn());
    ASSERT(5, short_fn());

    ASSERT(6, add_all(3,1,2,3));
    ASSERT(5, add_all(4,1,2,3,-1));

    { char buf[100]; fmt(buf, "%d %d %s", 1, 2, "foo"); printf("%s\n", buf); }

    ASSERT(0, ({ char buf[100]; sprintf(buf, "%d %d %s", 1, 2, "foo"); strcmp("1 2 foo", buf); }));

    ASSERT(0, ({ char buf[100]; fmt(buf, "%d %d %s", 1, 2, "foo"); strcmp("1 2 foo", buf); }));

    ASSERT(1, sizeof(char));
    ASSERT(1, sizeof(signed char));
    ASSERT(1, sizeof(signed char signed));
    ASSERT(1, sizeof(unsigned char));
    ASSERT(1, sizeof(unsigned char unsigned));

    ASSERT(2, sizeof(short));
    ASSERT(2, sizeof(int short));
    ASSERT(2, sizeof(short int));
    ASSERT(2, sizeof(signed short));
    ASSERT(2, sizeof(int short signed));
    ASSERT(2, sizeof(unsigned short));
    ASSERT(2, sizeof(int short unsigned));

    ASSERT(4, sizeof(int));
    ASSERT(4, sizeof(signed int));
    ASSERT(4, sizeof(signed));
    ASSERT(4, sizeof(signed signed));
    ASSERT(4, sizeof(unsigned int));
    ASSERT(4, sizeof(unsigned));
    ASSERT(4, sizeof(unsigned unsigned));

    ASSERT(8, sizeof(long));
    ASSERT(8, sizeof(signed long));
    ASSERT(8, sizeof(signed long int));
    ASSERT(8, sizeof(unsigned long));
    ASSERT(8, sizeof(unsigned long int));

    ASSERT(8, sizeof(long long));
    ASSERT(8, sizeof(signed long long));
    ASSERT(8, sizeof(signed long long int));
    ASSERT(8, sizeof(unsigned long long));
    ASSERT(8, sizeof(unsigned long long int));

    ASSERT(-1, (char)255);
    ASSERT(-1, (signed char)255);
    ASSERT(255, (unsigned char)255);
    ASSERT(-1, (short)65535);
    ASSERT(65535, (unsigned short)65535);
    ASSERT(-1, (int)0xffffffff);
    ASSERT(0xffffffff, (unsigned)0xffffffff);

    ASSERT(1, -1<1);
    ASSERT(0, -1<(unsigned)1);
    ASSERT(254, (char)127+(char)127);
    ASSERT(65534, (short)32767+(short)32767);
    ASSERT(-1, -1>>1);
    ASSERT(-1, (unsigned long)-1);
    ASSERT(2147483647, ((unsigned)-1)>>1);
    ASSERT(-50, (-100)/2);
    ASSERT(2147483598, ((unsigned)-100)/2);
    ASSERT(9223372036854775758, ((unsigned long)-100)/2);
    ASSERT(0, ((long)-1)/(unsigned)100);
    ASSERT(-2, (-100)%7);
    ASSERT(2, ((unsigned)-100)%7);
    ASSERT(6, ((unsigned long)-100)%9);

    ASSERT(65535, (int)(unsigned short)65535);
    ASSERT(65535, ({ unsigned short x = 65535; x; }));
    ASSERT(65535, ({ unsigned short x = 65535; (int)x; }));

    ASSERT(-1, ({ typedef short T; T x = 65535; (int)x; }));
    ASSERT(65535, ({ typedef unsigned short T; T x = 65535; (int)x; }));

    ASSERT(251, uchar_fn());
    ASSERT(65528, ushort_fn());
    ASSERT(-5, schar_fn());
    ASSERT(-8, sshort_fn());

    ASSERT(1, sizeof((char)1));
    ASSERT(2, sizeof((short)1));
    ASSERT(4, sizeof((int)1));
    ASSERT(8, sizeof((long)1));

    ASSERT(4, sizeof((char)1 + (char)1));
    ASSERT(4, sizeof((short)1 + (short)1));
    ASSERT(4, sizeof(1?2:3));
    ASSERT(4, sizeof(1?(short)2:(char)3));
    ASSERT(8, sizeof(1?(long)2:(char)3));

    ASSERT(4, sizeof(0));
    ASSERT(8, sizeof(0L));
    ASSERT(8, sizeof(0LU));
    ASSERT(8, sizeof(0UL));
    ASSERT(8, sizeof(0LL));
    ASSERT(8, sizeof(0LLU));
    ASSERT(8, sizeof(0Ull));
    ASSERT(8, sizeof(0l));
    ASSERT(8, sizeof(0ll));
    ASSERT(8, sizeof(0x0L));
    ASSERT(8, sizeof(0b0L));
    ASSERT(4, sizeof(2147483647));
    ASSERT(8, sizeof(2147483648));
    ASSERT(-1, 0xffffffffffffffff);
    ASSERT(8, sizeof(0xffffffffffffffff));
    ASSERT(4, sizeof(4294967295U));
    ASSERT(8, sizeof(4294967296U));

    ASSERT(3, -1U>>30);
    ASSERT(3, -1Ul>>62);
    ASSERT(3, -1ull>>62);

    ASSERT(1, 0xffffffffffffffffl>>63);
    ASSERT(1, 0xffffffffffffffffll>>63);

    ASSERT(-1, 18446744073709551615);
    ASSERT(8, sizeof(18446744073709551615));
    ASSERT(-1, 18446744073709551615>>63);

    ASSERT(-1, 0xffffffffffffffff);
    ASSERT(8, sizeof(0xffffffffffffffff));
    ASSERT(1, 0xffffffffffffffff>>63);

    ASSERT(-1, 01777777777777777777777);
    ASSERT(8, sizeof(01777777777777777777777));
    ASSERT(1, 01777777777777777777777>>63);

    ASSERT(-1, 0b1111111111111111111111111111111111111111111111111111111111111111);
    ASSERT(8, sizeof(0b1111111111111111111111111111111111111111111111111111111111111111));
    ASSERT(1, 0b1111111111111111111111111111111111111111111111111111111111111111>>63);

    ASSERT(8, sizeof(2147483648));
    ASSERT(4, sizeof(2147483647));

    ASSERT(8, sizeof(0x1ffffffff));
    ASSERT(4, sizeof(0xffffffff));
    ASSERT(1, 0xffffffff>>31);

    ASSERT(8, sizeof(040000000000));
    ASSERT(4, sizeof(037777777777));
    ASSERT(1, 037777777777>>31);

    ASSERT(8, sizeof(0b111111111111111111111111111111111));
    ASSERT(4, sizeof(0b11111111111111111111111111111111));
    ASSERT(1, 0b11111111111111111111111111111111>>31);

    ASSERT(-1, 1 << 31 >> 31);
    ASSERT(-1, 01 << 31 >> 31);
    ASSERT(-1, 0x1 << 31 >> 31);
    ASSERT(-1, 0b1 << 31 >> 31);

    ASSERT(1, _Alignof(char) << 31 >> 31);
    ASSERT(1, _Alignof(char) << 63 >> 63);
    ASSERT(1, ({ char x; _Alignof(x) << 63 >> 63; }));

    ASSERT(20, ({ int x; int *p=&x; p+20-p; }));
    ASSERT(1, ({ int x; int *p=&x; p+20-p>0; }));
    ASSERT(-20, ({ int x; int *p=&x; p-20-p; }));
    ASSERT(1, ({ int x; int *p=&x; p-20-p<0; }));

    ASSERT(15, (char *)0xffffffffffffffff - (char *)0xfffffffffffffff0);
    ASSERT(-15, (char *)0xfffffffffffffff0 - (char *)0xffffffffffffffff);

    ASSERT(1, sizeof(char) << 31 >> 31);
    ASSERT(1, sizeof(char) << 63 >> 63);

    ASSERT(4, ({ char x[(-1>>31)+5]; sizeof(x); }));
    ASSERT(255, ({ char x[(unsigned char)0xffffffff]; sizeof(x); }));
    ASSERT(0x800f, ({ char x[(unsigned short)0xffff800f]; sizeof(x); }));
    ASSERT(1, ({ char x[(unsigned int)0xfffffffffff>>31]; sizeof(x); }));
    ASSERT(1, ({ char x[(long)-1/((long)1<<62)+1]; sizeof(x); }));
    ASSERT(4, ({ char x[(unsigned long)-1/((long)1<<62)+1]; sizeof(x); }));
    ASSERT(1, ({ char x[(unsigned)1<-1]; sizeof(x); }));
    ASSERT(1, ({ char x[(unsigned)1<=-1]; sizeof(x); }));

    { volatile x; }
    { int volatile x; }
    { volatile int x; }
    { volatile int volatile volatile x; }
    { int volatile * volatile volatile x; }
    { auto ** restrict __restrict __restrict__ const volatile *x; }


    { const x; }
    { int const x; }
    { const int x; }
    { const int const const x; }
    ASSERT(5, ({ const x = 5; x; }));
    ASSERT(8, ({ const x = 8; int *const y=&x; *y; }));
    ASSERT(6, ({ const x = 6; *(const * const)&x; }));

    0.0;
    1.0;
    3e+8;
    0x10.1p0;
    .1E4f;

    ASSERT(4, sizeof(8f));
    ASSERT(4, sizeof(0.3F));
    ASSERT(8, sizeof(0.));
    ASSERT(8, sizeof(.0));
    ASSERT(8, sizeof(5.l));
    ASSERT(8, sizeof(2.0L));

    ASSERT(0, (_Bool)0.0);
    ASSERT(1, (_Bool)0.1);
    ASSERT(3, (char)3.0);
    ASSERT(1000, (short)1000.3);
    ASSERT(3, (int)3.99);
    ASSERT(2000000000000000, (long)2e15);
    ASSERT(3, (float)3.5);
    ASSERT(5, (double)(float)5.5);
    ASSERT(3, (float)3);
    ASSERT(3, (double)3);
    ASSERT(3, (float)3L);
    ASSERT(3, (double)3L);

    ASSERT(35, (float)(char)35);
    ASSERT(35, (float)(short)35);
    ASSERT(35, (float)(int)35);
    ASSERT(35, (float)(long)35);
    ASSERT(35, (float)(unsigned char)35);
    ASSERT(35, (float)(unsigned short)35);
    ASSERT(35, (float)(unsigned int)35);
    ASSERT(35, (float)(unsigned long)35);

    ASSERT(35, (double)(char)35);
    ASSERT(35, (double)(short)35);
    ASSERT(35, (double)(int)35);
    ASSERT(35, (double)(long)35);
    ASSERT(35, (double)(unsigned char)35);
    ASSERT(35, (double)(unsigned short)35);
    ASSERT(35, (double)(unsigned int)35);
    ASSERT(35, (double)(unsigned long)35);

    ASSERT(35, (char)(float)35);
    ASSERT(35, (short)(float)35);
    ASSERT(35, (int)(float)35);
    ASSERT(35, (long)(float)35);
    ASSERT(35, (unsigned char)(float)35);
    ASSERT(35, (unsigned short)(float)35);
    ASSERT(35, (unsigned int)(float)35);
    ASSERT(35, (unsigned long)(float)35);

    ASSERT(35, (char)(double)35);
    ASSERT(35, (short)(double)35);
    ASSERT(35, (int)(double)35);
    ASSERT(35, (long)(double)35);
    ASSERT(35, (unsigned char)(double)35);
    ASSERT(35, (unsigned short)(double)35);
    ASSERT(35, (unsigned int)(double)35);
    ASSERT(35, (unsigned long)(double)35);

    ASSERT(-2147483648, (double)(unsigned long)(long)-1);

    ASSERT(4, sizeof(float));
    ASSERT(8, sizeof(double));

    ASSERT(1, 2e3==2e3);
    ASSERT(0, 2e3==2e5);
    ASSERT(1, 2.0==2);
    ASSERT(0, 5.1<5);
    ASSERT(0, 5.0<5);
    ASSERT(1, 4.9<5);
    ASSERT(0, 5.1<=5);
    ASSERT(1, 5.0<=5);
    ASSERT(1, 4.9<=5);

    ASSERT(1, 2e3f==2e3);
    ASSERT(0, 2e3f==2e5);
    ASSERT(1, 2.0f==2);
    ASSERT(0, 5.1f<5);
    ASSERT(0, 5.0f<5);
    ASSERT(1, 4.9f<5);
    ASSERT(0, 5.1f<=5);
    ASSERT(1, 5.0f<=5);
    ASSERT(1, 4.9f<=5);

    ASSERT(6, 2.3+3.8);
    ASSERT(-1, 2.3-3.8);
    ASSERT(-3, -3.8);
    ASSERT(13, 3.3*4);
    ASSERT(2, 5.0/2);

    ASSERT(6, 2.3f+3.8f);
    ASSERT(6, 2.3f+3.8);
    ASSERT(-1, 2.3f-3.8);
    ASSERT(-3, -3.8f);
    ASSERT(13, 3.3f*4);
    ASSERT(2, 5.0f/2);

    ASSERT(0, 0.0/0.0 == 0.0/0.0);
    ASSERT(1, 0.0/0.0 != 0.0/0.0);

    ASSERT(0, 0.0/0.0 < 0);
    ASSERT(0, 0.0/0.0 <= 0);
    ASSERT(0, 0.0/0.0 > 0);
    ASSERT(0, 0.0/0.0 >= 0);

    ASSERT(4, sizeof(1f+2));
    ASSERT(8, sizeof(1.0+2));
    ASSERT(4, sizeof(1f-2));
    ASSERT(8, sizeof(1.0-2));
    ASSERT(4, sizeof(1f*2));
    ASSERT(8, sizeof(1.0*2));
    ASSERT(4, sizeof(1f/2));
    ASSERT(8, sizeof(1.0/2));

    ASSERT(0, 0.0 && 0.0);
    ASSERT(0, 0.0 && 0.1);
    ASSERT(0, 0.3 && 0.0);
    ASSERT(1, 0.3 && 0.5);
    ASSERT(0, 0.0 || 0.0);
    ASSERT(1, 0.0 || 0.1);
    ASSERT(1, 0.3 || 0.0);
    ASSERT(1, 0.3 || 0.5);
    ASSERT(5, ({ int x; if (0.0) x=3; else x=5; x; }));
    ASSERT(3, ({ int x; if (0.1) x=3; else x=5; x; }));
    ASSERT(5, ({ int x=5; if (0.0) x=3; x; }));
    ASSERT(3, ({ int x=5; if (0.1) x=3; x; }));
    ASSERT(10, ({ double i=10.0; int j=0; for (; i; i--, j++); j; }));
    ASSERT(10, ({ double i=10.0; int j=0; do j++; while(--i); j; }));

    ASSERT(0, !3.);
    ASSERT(1, !0.);
    ASSERT(0, !3.f);
    ASSERT(1, !0.f);

    ASSERT(5, 0.0 ? 3 : 5);
    ASSERT(3, 1.2 ? 3 : 5);

    ASSERT(6, add_float(2.3, 3.8));
    ASSERT(6, add_double(2.3, 3.8));

    ASSERT(7, add_float3(2.5, 2.5, 2.5));
    ASSERT(7, add_double3(2.5, 2.5, 2.5));

    ASSERT(0, ({ char buf[100]; sprintf(buf, "%.1f", (float)3.5); strcmp(buf, "3.5"); }));
    ASSERT(0, ({ char buf[100]; fmt(buf, "%.1f", (float)3.5); strcmp(buf, "3.5"); }));

    printf("OK\n");
    return 0;
}
