// Regression for the double-import guard. Before Phase 1 this produced ~2x
// bytecode size because std.math's definitions were emitted twice. After the
// fix, the second import is silently skipped.
import std.math;
import std.math;
import std.math;

void main() {
    print(std.math.abs(-42));
    print(std.math.pow(2, 10));
    print(std.math.gcd(24, 36));
}
