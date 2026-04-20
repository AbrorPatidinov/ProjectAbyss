// Exception handling: throw, catch, nested try/catch.
function safe_div(int a, int b) : (int r) {
    if (b == 0) { throw "division by zero"; }
    return a / b;
}

void main() {
    try {
        int x = safe_div(20, 5);
        print(x);
    } catch (err) {
        print("never");
    }

    try {
        int y = safe_div(10, 0);
        print(y);
    } catch (err) {
        print(err);
    }

    try {
        try {
            throw "inner";
        } catch (e1) {
            print(e1);
            throw "rethrown";
        }
    } catch (e2) {
        print(e2);
    }
}
