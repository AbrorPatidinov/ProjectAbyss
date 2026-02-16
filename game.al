struct Player {
    int hp;
    int gold;
}

struct Enemy {
    int hp;
    int damage;
}

void main() {
    print("--- ABYSS DUNGEON ---");

    Player p = new(Player);
    p.hp = 100;
    p.gold = 0;

    print("You enter the dungeon...");

    // FIX: Declare variables OUTSIDE the loop to prevent stack overflow/desync
    int playing = 1;
    int choice = 0;
    int act = 0;
    Enemy e = 0;

    while (playing) {
        print("----------------");
        print("Your HP:");
        print(p.hp);
        print("1. Explore");
        print("2. Rest (Heal)");
        print("3. Quit");

        choice = input_int(); // Assignment

        if (choice == 1) {
            print("You found an enemy!");
            e = new(Enemy); // Assignment
            e.hp = 30;
            e.damage = 10;

            while (e.hp > 0) {
                print("1. Attack");
                act = input_int();
                if (act == 1) {
                    print("You hit the enemy for 15 dmg.");
                    e.hp = e.hp - 15;
                    if (e.hp > 0) {
                        print("Enemy hits you!");
                        p.hp = p.hp - e.damage;
                    }
                }
            }
            print("Enemy died!");
            free(e);
            p.gold = p.gold + 10;
        }

        if (choice == 2) {
            print("You rested.");
            p.hp = p.hp + 20;
        }

        if (choice == 3) {
            playing = 0;
        }

        if (p.hp < 1) {
            print("YOU DIED.");
            playing = 0;
        }
    }

    print("Game Over. Final Gold:");
    print(p.gold);

    free(p);
    abyss_eye();
}
