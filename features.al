function addition(int x, int y) : int {
    return x + y;
}

function multiply(int x, int y) : (int result) {
    result = x * y;
    return result;
}

function division(int x, int y) : (int result), (error divisionByZeroError) {
    result = x / y;
    if (y == 0) {
        throw divisionByZeroError;
    }

    return result;
}

void main() {
    print("--- Testing New Features ---");

    // 1. Increment / Decrement
    int x = 10;
    print("x is %{int}", x);
    x++;
    print("x++ is %{int}", x);
    x--;
    print("x-- is %{int}", x);

    // 2. Compound Assignment
    x += 5;
    print("x += 5 is %{int}", x);
    x -= 2;
    print("x -= 2 is %{int}", x);

    // 3. For Loop
    print("Counting with for loop:");
    for (int i = 0; i < 5; i++) {
        print("  i = %{int}", i);
    }

    print("More for loop !");
    for (int k = 10; k > 0; k--) {
        print("Minuses: %{integer}", k);
    }

    print("--- Done ---");
    abyss_eye();
}
