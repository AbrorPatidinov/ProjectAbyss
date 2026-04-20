// Bitwise operators, string concatenation, formatted print.
void main() {
    int flags = 0;
    flags = flags | 1;
    flags = flags | 4;
    flags = flags | 8;
    print(flags);

    print(flags & 1);
    print(flags & 2);
    print((flags ^ 15) & 15);
    print(~0 & 255);
    print(1 << 4);
    print(256 >> 3);

    str a = "Hello, ";
    str b = "Abyss";
    str c = a + b;
    print(c);

    int answer = 42;
    print("The answer is %int", answer);
    print("Name: %str, Value: %int", b, answer);
}
