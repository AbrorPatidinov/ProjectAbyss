// std.math - Mathematics

function std.math.abs(int x) : (int res) {
    res = __bridge_abs(x);
    return res;
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
    res = __bridge_pow(base, exp);
    return res;
}
