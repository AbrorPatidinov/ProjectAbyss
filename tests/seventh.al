function split(int a) : (int half, int double_val) {
    half = a / 2;
    double_val = a * 2;
    return half, double_val;
}

void main() {
    int h;
    int d;

    h, d = split(8);

    print("half = %int, double = %int", h, d);
}
