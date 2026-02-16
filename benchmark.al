void main() {
    print("--- AbyssLang Benchmark ---");
    print("Enter number of iterations:");
    int n = input_int();

    float start = clock();

    int i = 0;
    while (i < n) {
        i = i + 1;
    }

    float end = clock();
    float duration = end - start;

    print("Done!");
    print("Time taken (seconds):");
    print(duration);
}
