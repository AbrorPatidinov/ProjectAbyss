// benchmark.al
function is_prime(int n) : (int res) {
    if (n < 2) {
        return 0;
    }
    for (int i = 2; i * i <= n; i++) {
        if (n % i == 0) {
            return 0;
        }
    }
    return 1;
}

void main() {
    print("--- AbyssLang CPU Benchmark ---");

    int target = 100000;
    int count = 0;

    for (int i = 0; i <= target; i++) {
        int check = is_prime(i);
        if (check == 1) {
            count++;
        }
    }

    print("Total primes found up to %int: %int", target, count);
}
