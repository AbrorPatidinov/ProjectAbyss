struct Ball {
    float y;
    float vy;
}

void main() {
    Ball b = new(Ball);
    b.y = 20.0;

    // Physics Loop
    while (b.y > 0.0) {
        b.vy = b.vy - 0.05;
        b.y = b.y + b.vy;

        if (b.y < 0.0) { b.y = 0.0; }

        print(b.y);
    }

    abyss_eye(); // Visualize Memory
    free(b);
}
