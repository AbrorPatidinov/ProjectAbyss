// Heap-allocated arrays, indexing, in-place mutation, stdlib array ops.
import std.array;

void main() {
    int[] xs = new(int, 5, "xs");
    for (int i = 0; i < 5; i++) {
        xs[i] = (i + 1) * 10;
    }

    for (int i = 0; i < 5; i++) {
        print(xs[i]);
    }

    xs[2]++;
    xs[0]--;
    print(xs[0]);
    print(xs[2]);

    print(std.array.sum(xs, 5));
    print(std.array.min(xs, 5));
    print(std.array.max(xs, 5));

    std.array.reverse(xs, 5);
    for (int i = 0; i < 5; i++) {
        print(xs[i]);
    }

    free(xs);
}
