import std.io;
import std.math;

void main() {
    std.io.println("--- Standard Library Test ---");

    int a = 10;
    int b = 20;
    int m = std.math.max(a, b);

    std.io.println("Max of 10 and 20 is:");
    std.io.print_int(m);

    int p = std.math.pow(2, 10);
    std.io.println("2 power 10 is:");
    std.io.print_int(p);
}
