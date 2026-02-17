void main() {
    float start = clock();
    int sum = 0;

    for (int i = 0; i < 1000000; i++) {
        sum += i;
    }

    float end = clock();
    float elapsed = end - start;

    print("Sum: %{int}", sum);
    print("Elapsed: %{float} sec", elapsed);
    print("Elapsed: %{float} ms", elapsed * 1000.0);
}
