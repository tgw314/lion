#define ASSERT(x, y) assert(x, y, #y)
// clang-format off

typedef int MyInt, MyInt2[4];
typedef int;

void assert(int expected, int actual, char *code); 
int printf();

int ret3() {
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

int g1, g2[4];

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


    printf("OK\n");
    return 0;
}
