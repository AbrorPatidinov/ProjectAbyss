// Too many return values (>8)
function toomany() : (int a, int b, int c, int d) {
    return 1, 2, 3, 4, 5, 6, 7, 8, 9;
}
void main() { print(0); }
