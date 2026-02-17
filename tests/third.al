function safe_div(int a, int b) : (int result) {
    if (b == 0) {
        throw "division by zero";
    }

    result = a / b;
    return result;
}

void main() {
    try {
        print("10 / 2 = %int", safe_div(10, 2));
        print("10 / 0 = %int", safe_div(10, 0));
    }
    catch (err) {
        print("Caught error: %str", err);
    }
}
