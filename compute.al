struct Ball {
    float y;
    float vy;
}

void main() {
    print("AbyssLang: Starting 100M iterations...");

    Ball b = new(Ball);
    b.y = 1000.0;
    b.vy = 0.0;

    int i = 0;
    int max = 100000000; // 100 Million
    float dt = 0.01;
    float g = 9.8;

    float start = clock();

    while (i < max) {
        // 1. Update Velocity
        b.vy = b.vy - (g * dt);

        // 2. Update Position
        b.y = b.y + (b.vy * dt);

        // 3. Collision Logic
        if (b.y < 0.0) {
            b.y = 0.0;
            b.vy = b.vy * -0.8;
        }

        i = i + 1;
    }

    float end = clock();
    float duration = end - start;

    print("AbyssLang: Done.");
    print("Time (s):");
    print(duration);

    free(b);
}
