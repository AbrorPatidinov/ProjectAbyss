void main() {
    print("--- LOGICAL & BITWISE TEST ---");

    int hp = 100;
    int mana = 50;

    if (hp > 0 && mana >= 50) {
        print("Player is ready to cast a spell!");
    }

    if (!0) {
        print("NOT operator works!");
    }

    print("--- BITWISE TEST ---");
    int flags = 1 | 2 | 8; // 11
    print("Flags (1 | 2 | 8) = %int", flags);

    int shifted = 1 << 4; // 16
    print("1 << 4 = %int", shifted);

    print("--- DYNAMIC STRING TEST ---");
    str first = "Hello, ";
    str second = "Mr. President!";

    // This allocates on the heap dynamically!
    str message = first + second;
    print(message);

    print("");
    print("Opening Abyss Eye to see the dynamic string...");
    abyss_eye();
}
