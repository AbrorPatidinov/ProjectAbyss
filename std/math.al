// std.math - Pure AbyssLang Mathematics

function std.math.abs(int x) : (int res) {
    if (x < 0) {
        return -x;
    }
    return x;
}

function std.math.max(int a, int b) : (int res) {
    if (a > b) { return a; }
    return b;
}

function std.math.min(int a, int b) : (int res) {
    if (a < b) { return a; }
    return b;
}

function std.math.pow(int base, int exp) : (int res) {
    res = 1;
    for (int i = 0; i < exp; i++) {
        res = res * base;
    }
    return res;
}

function std.math.factorial(int n) : (int res) {
    if (n <= 1) { return 1; }
    return n * std.math.factorial(n - 1);
}
