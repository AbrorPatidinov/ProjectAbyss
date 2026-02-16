// --- DATA STRUCTURES ---
struct Hero {
    int hp;
    int max_hp;
    int mana;
    int strength;
    str name;
}

struct Monster {
    int hp;
    int max_hp;
    int damage;
    str name;
}

// --- GLOBAL SEED FOR RNG ---
int seed = 54321;

// Function: Random Number Generator
function rand(int max) : (int val) {
    seed = (seed * 1103515245 + 12345);
    val = seed % max;
    if (val < 0) { val = val * -1; }
    return val;
}

// Function: Calculate Attack Damage
function calc_attack(int strength_stat) : (int dmg, int is_crit) {
    int variance = rand(5);
    dmg = strength_stat + variance;

    int luck = rand(100);
    is_crit = 0;

    if (luck < 20) {
        dmg = dmg * 2;
        is_crit = 1;
    }

    return dmg, is_crit;
}

// Function: Cast Spell
function cast_fireball(int current_mana) : (int cost) {
    cost = 15;
    if (current_mana < cost) {
        throw "Not enough mana to cast Fireball!";
    }
    return cost;
}

// --- MAIN GAME LOOP ---
void main() {
    seed = clock() * 1000; // Seed RNG

    print("--- ABYSS QUEST: THE DARK DEPTHS ---");

    Hero player = new(Hero);
    player.name = "Artorias";
    player.max_hp = 100;
    player.hp = 100;
    player.mana = 30;
    player.strength = 8;

    Monster mob = new(Monster);
    mob.name = "Abyss Watcher";
    mob.max_hp = 150;
    mob.hp = 150;
    mob.damage = 12;

    print("A wild %{str} appears!", mob.name);
    print("");

    int round = 1;
    int fighting = 1;

    while (fighting) {
        print("=== ROUND %{int} ===", round);
        print("%{str} HP: %{int} | Mana: %{int}", player.name, player.hp, player.mana);
        print("%{str} HP: %{int}", mob.name, mob.hp);
        print("1. Attack");
        print("2. Fireball (15 Mana)");
        print("3. Flurry (Multi-hit)");

        print("Choose action:");

        // FIX: Explicit assignment to ensure correct stack behavior
        int choice = 0;
        choice = input_int();

        // DEBUG: Verify input
        print("DEBUG: You chose %{int}", choice);

        int dmg = 0;
        int crit = 0;

        if (choice == 1) {
            dmg, crit = calc_attack(player.strength);

            if (crit) {
                print("CRITICAL HIT!");
            }
            print("You hit for %{int} damage.", dmg);
            mob.hp -= dmg;
        }
        else if (choice == 2) {
            try {
                int cost = cast_fireball(player.mana);
                player.mana -= cost;
                dmg = 40;
                print("FIREBALL EXPLODES! %{int} damage.", dmg);
                mob.hp -= dmg;
            }
            catch (err) {
                print("SPELL FAILED: %{str}", err);
            }
        }
        else if (choice == 3) {
            print("You unleash a flurry of blows!");
            for (int i = 0; i < 3; i++) {
                int hit = 4;
                print("  Hit %{int}: %{int} dmg", i + 1, hit);
                mob.hp -= hit;
            }
        }

        if (mob.hp > 0) {
            print("%{str} attacks you!", mob.name);
            int mob_dmg = mob.damage + rand(3);
            player.hp -= mob_dmg;
            print("You took %{int} damage.", mob_dmg);
        } else {
            print("");
            print("VICTORY! The %{str} has been defeated.", mob.name);
            fighting = 0;
        }

        if (player.hp <= 0) {
            print("");
            print("DEFEAT... You have died.");
            fighting = 0;
        }

        print("");
        round++;
    }

    print("--- Game Over ---");
    abyss_eye();

    free(player);
    free(mob);

    print("Memory cleaned.");
    abyss_eye();
}
