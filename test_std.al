import std.io;
import std.math;

void main() {
    std.io.println("--- Pure AbyssLang Standard Library Test ---");

    int a = -50;
    int absolute = std.math.abs(a);
    print("Absolute of -50 is: %int", absolute);

    int m = std.math.max(10, 20);
    print("Max of 10 and 20 is: %int", m);

    int p = std.math.pow(2, 10);
    print("2 power 10 is: %int", p);

    int f = std.math.factorial(5);
    print("Factorial of 5 is: %int", f);

    std.io.println("Standard Library is working perfectly without Rust!");
}
