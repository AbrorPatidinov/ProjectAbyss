function factorial(int n) : (int result) {
    if (n <= 1) {
        return 1;
    }

    int prev = factorial(n - 1);
    result = n * prev;
    return result;
}

void main() {
    print("Factorial 6 = %int", factorial(6));
}
