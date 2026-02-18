// std.io - Input/Output

function std.io.println(str s) {
    __bridge_print_str(s);
}

function std.io.print_int(int i) {
    __bridge_print_int(i);
}

function std.io.print_float(float f) {
    __bridge_print_float(f);
}

function std.io.read_int() : (int val) {
    val = input_int();
    return val;
}
