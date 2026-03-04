// std.io - Pure AbyssLang Input/Output

function std.io.println(str s) {
    print(s);
}

function std.io.print_int(int i) {
    print(i);
}

function std.io.print_float(float f) {
    print(f);
}

function std.io.read_int() : (int val) {
    val = input_int();
    return val;
}
