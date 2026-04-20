// Multi-value returns and tuple unpacking.
function divmod(int a, int b) : (int q, int r) {
    q = a / b;
    r = a % b;
    return q, r;
}

function minmax(int a, int b, int c) : (int lo, int hi) {
    lo = a;
    hi = a;
    if (b < lo) { lo = b; }
    if (c < lo) { lo = c; }
    if (b > hi) { hi = b; }
    if (c > hi) { hi = c; }
    return lo, hi;
}

void main() {
    int q;
    int r;
    q, r = divmod(17, 5);
    print(q);
    print(r);

    int a;
    int b;
    a, b = minmax(7, 2, 9);
    print(a);
    print(b);
}
