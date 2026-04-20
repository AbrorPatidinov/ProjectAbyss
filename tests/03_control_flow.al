// Control flow: if/else, while, for, break, continue, nested scopes.
void main() {
    int score = 75;
    if (score >= 90) {
        print(1);
    } else if (score >= 70) {
        print(2);
    } else {
        print(3);
    }

    int i = 0;
    int sum = 0;
    while (i < 5) {
        sum += i;
        i++;
    }
    print(sum);

    int product = 1;
    for (int k = 1; k <= 5; k++) {
        product = product * k;
    }
    print(product);

    int found = -1;
    for (int k = 0; k < 20; k++) {
        if (k % 7 != 0) { continue; }
        if (k == 14) { break; }
        found = k;
    }
    print(found);
}
