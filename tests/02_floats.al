// Float literal emission regression. Exercises the path where the transpiler
// writes double bit patterns into the generated C. Covers negatives, small
// fractions, large magnitudes, and mixed int/float arithmetic.
void main() {
    float a = -1.5;
    float b = 1234567890.5;
    float c = 0.0001;
    float d = -2.0 * 3.0;
    float e = 10.0 / -4.0;
    print(a);
    print(b);
    print(c);
    print(d);
    print(e);
}
