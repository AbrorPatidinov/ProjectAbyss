// Mutual recursion across definition order.
// is_even calls is_odd defined below. is_odd calls is_even defined above.
// Requires true forward references — single-pass would fail either direction.
int is_even(int n) {
    if (n == 0) { return 1; }
    return is_odd(n - 1);
}

int is_odd(int n) {
    if (n == 0) { return 0; }
    return is_even(n - 1);
}

void main() {
    print(is_even(0));
    print(is_even(1));
    print(is_even(10));
    print(is_even(11));
    print(is_odd(7));
    print(is_odd(12));
}
