// Forward reference: main() calls helpers defined below it.
// Before Phase 2 this failed with "Undefined function". Now it works.
void main() {
    print(square(5));
    print(double_it(21));
    print(sum_to(10));
}

int square(int x) {
    return x * x;
}

int double_it(int x) {
    return x * 2;
}

int sum_to(int n) {
    int s = 0;
    for (int i = 1; i <= n; i++) {
        s += i;
    }
    return s;
}
