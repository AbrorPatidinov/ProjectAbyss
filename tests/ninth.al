struct Player {
    int hp;
    int x;
    int y;
}

struct Enemy {
    int hp;
    int damage;
}

void main() {
    print("Initializing Game State...");

    // 1. Stack Allocation with a Comment
    Player p = stack(Player, "Main character state");
    p.hp = 100;
    p.x = 50;
    p.y = 50;

    // 2. Heap Allocation with a Comment
    Enemy boss = new(Enemy, "Level 1 Boss Entity");
    boss.hp = 5000;
    boss.damage = 99;

    // 3. Array Allocation with a Comment
    int[] inventory = new(int, 10, "Player inventory slots");
    inventory[0] = 999; // Give player an item

    // 4. Un-commented allocation (to show it handles NULL gracefully)
    Enemy minion = new(Enemy);
    minion.hp = 10;

    print("Game State Initialized. Opening Abyss Eye...");

    // Behold the HUD
    abyss_eye();

    free(boss);
    free(inventory);
    free(minion);
}
