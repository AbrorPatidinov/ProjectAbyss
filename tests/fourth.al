void main() {
    int[] arr = new(int, 5);

    for (int i = 0; i < 5; i++) {
        arr[i] = i * i;
    }

    for (int k = 0; k < 5; k++) {
        print("arr[%int] = %int", k, arr[k]);
    }

    free(arr);
    abyss_eye();
}
