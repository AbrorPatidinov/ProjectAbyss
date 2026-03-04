struct Weapon {
    int damage;
    int durability;
}

void main() {
    print("--- ABYSS MEMORY RESONANCE TEST ---");

    // 1. Allocate and keep active (This will show as ACTIVE/LEAK)
    Weapon sword = new(Weapon, "Player's main sword");
    sword.damage = 50;

    // 2. Allocate and FREE correctly (This will show as FREED)
    Weapon dagger = new(Weapon, "Temporary dagger");
    dagger.damage = 15;
    free(dagger);

    // 3. Stack allocation (Active during abyss_eye, auto-freed later)
    Weapon shield = stack(Weapon, "Offhand shield");
    shield.durability = 100;

    // 4. Array allocation and FREE (This will show as FREED)
    int[] enemies = new(int, 5, "Enemy spawn list");
    free(enemies);

    print("Triggering Abyss Eye...");

    // Behold the HUD
    abyss_eye();

    // Free the sword after the eye so it shows as active during the snapshot
    free(sword);
}
