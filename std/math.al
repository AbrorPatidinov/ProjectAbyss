// ═══════════════════════════════════════════════════════════
//  std.math — AbyssLang Standard Math Library
//  Pure AbyssLang. Zero dependencies.
// ═══════════════════════════════════════════════════════════

// --- Absolute Value ---
function std.math.abs(int x) : (int result) {
    if (x < 0) { return -x; }
    return x;
}

// --- Power (integer exponentiation) ---
function std.math.pow(int base, int exp) : (int result) {
    result = 1;
    for (int i = 0; i < exp; i++) {
        result = result * base;
    }
    return result;
}

// --- Minimum ---
function std.math.min(int a, int b) : (int result) {
    if (a < b) { return a; }
    return b;
}

// --- Maximum ---
function std.math.max(int a, int b) : (int result) {
    if (a > b) { return a; }
    return b;
}

// --- Clamp (constrain value to range) ---
function std.math.clamp(int val, int lo, int hi) : (int result) {
    if (val < lo) { return lo; }
    if (val > hi) { return hi; }
    return val;
}

// --- Greatest Common Divisor (Euclid's Algorithm) ---
function std.math.gcd(int a, int b) : (int result) {
    if (a < 0) { a = -a; }
    if (b < 0) { b = -b; }
    while (b != 0) {
        int temp = b;
        b = a % b;
        a = temp;
    }
    return a;
}

// --- Least Common Multiple ---
function std.math.lcm(int a, int b) : (int result) {
    if (a == 0 || b == 0) { return 0; }
    int g = std.math.gcd(a, b);
    return std.math.abs((a / g) * b);
}

// --- Integer Square Root (Newton's Method) ---
function std.math.sqrt(int n) : (int result) {
    if (n <= 0) { return 0; }
    if (n == 1) { return 1; }

    int x = n;
    int y = (x + 1) / 2;
    while (y < x) {
        x = y;
        y = (x + n / x) / 2;
    }
    return x;
}

// --- Factorial ---
function std.math.factorial(int n) : (int result) {
    result = 1;
    for (int i = 2; i <= n; i++) {
        result = result * i;
    }
    return result;
}

// --- Is Prime ---
function std.math.is_prime(int n) : (int result) {
    if (n < 2) { return 0; }
    if (n < 4) { return 1; }
    if (n % 2 == 0) { return 0; }
    if (n % 3 == 0) { return 0; }

    int i = 5;
    while (i * i <= n) {
        if (n % i == 0) { return 0; }
        if (n % (i + 2) == 0) { return 0; }
        i += 6;
    }
    return 1;
}

// --- Fibonacci (Nth number) ---
function std.math.fib(int n) : (int result) {
    if (n <= 0) { return 0; }
    if (n == 1) { return 1; }
    int a = 0;
    int b = 1;
    for (int i = 2; i <= n; i++) {
        int temp = a + b;
        a = b;
        b = temp;
    }
    return b;
}

// --- Map value from one range to another ---
function std.math.map(int val, int in_lo, int in_hi, int out_lo, int out_hi) : (int result) {
    return out_lo + ((val - in_lo) * (out_hi - out_lo)) / (in_hi - in_lo);
}
