// ═══════════════════════════════════════════════════════════════════════
//  AbyssLang — Complete Feature Showcase
//  Every capability of the language demonstrated in a single file.
//  Author: Abrorbek Patidinov | Tashkent, Uzbekistan
// ═══════════════════════════════════════════════════════════════════════

import std.math;
import std.time;
import std.io;
import std.array;

// ─────────────────────────────────────────────────────────────────────
//  1. ENUMS — Named integer constants with auto-increment
// ─────────────────────────────────────────────────────────────────────

enum Element {
    FIRE,           // 0
    WATER,          // 1
    EARTH,          // 2
    AIR,            // 3
    VOID = 99       // Explicit value
}

enum Rarity {
    COMMON,
    RARE = 5,
    LEGENDARY,      // 6 (auto-increments from 5)
    MYTHIC          // 7
}

// ─────────────────────────────────────────────────────────────────────
//  2. STRUCTS — User-defined composite types
// ─────────────────────────────────────────────────────────────────────

struct Vec2 {
    float x;
    float y;
}

struct Weapon {
    str name;
    int damage;
    int element;
    int rarity;
    int durability;
}

struct Hero {
    str name;
    int hp;
    int max_hp;
    int level;
    int xp;
    Weapon weapon;
    Vec2 position;
}

struct BattleLog {
    int total_attacks;
    int total_damage;
    int crits;
    int misses;
}

// ─────────────────────────────────────────────────────────────────────
//  3. INTERFACES — Dynamic dispatch (runtime polymorphism)
// ─────────────────────────────────────────────────────────────────────

interface DamageFormula {
    function calculate(int base_dmg, int level) : (int result);
}

function formula_normal(int base_dmg, int level) : (int result) {
    return base_dmg + (level * 2);
}

function formula_crit(int base_dmg, int level) : (int result) {
    return (base_dmg + (level * 2)) * 3;
}

function formula_magic(int base_dmg, int level) : (int result) {
    return base_dmg * level;
}

// ─────────────────────────────────────────────────────────────────────
//  4. FUNCTIONS WITH MULTIPLE RETURN VALUES (Tuples)
// ─────────────────────────────────────────────────────────────────────

function calculate_level_and_remainder(int total_xp) : (int level, int remainder) {
    level = 1;
    int threshold = 100;
    remainder = total_xp;
    while (remainder >= threshold) {
        remainder = remainder - threshold;
        level++;
        threshold = threshold + 50;
    }
    return level, remainder;
}

function get_element_name(int elem) : (str name, int power) {
    if (elem == Element.FIRE)  { return "Fire",  120; }
    if (elem == Element.WATER) { return "Water", 100; }
    if (elem == Element.EARTH) { return "Earth", 110; }
    if (elem == Element.AIR)   { return "Air",    90; }
    return "Void", 150;
}

// ─────────────────────────────────────────────────────────────────────
//  5. STRING CONCATENATION — Dynamic heap-allocated strings
// ─────────────────────────────────────────────────────────────────────

function build_title(str name, int level) : (str result) {
    if (level >= 10) {
        result = "Legendary " + name;
    } else if (level >= 5) {
        result = "Veteran " + name;
    } else {
        result = "Novice " + name;
    }
    return result;
}

// ─────────────────────────────────────────────────────────────────────
//  6. ERROR HANDLING — try / catch / throw
// ─────────────────────────────────────────────────────────────────────

function forge_weapon(str name, int dmg, int elem, int rarity) : (Weapon w) {
    if (dmg <= 0) {
        throw "Cannot forge weapon with zero damage!";
    }
    if (dmg > 9999) {
        throw "Damage exceeds maximum forge capacity!";
    }

    w = new(Weapon, "Forged Weapon");
    w.name = name;
    w.damage = dmg;
    w.element = elem;
    w.rarity = rarity;
    w.durability = 100;
    return w;
}

function risky_enchant(Weapon w, int boost) : (int success) {
    if (boost > 50) {
        throw "Enchantment too powerful — weapon would shatter!";
    }
    w.damage += boost;
    w.durability -= boost;
    if (w.durability < 0) {
        w.durability = 0;
        throw "Weapon broke during enchantment!";
    }
    return 1;
}

// ─────────────────────────────────────────────────────────────────────
//  7. ARRAYS — Dynamic heap-allocated arrays with indexing
// ─────────────────────────────────────────────────────────────────────

function build_damage_table(int size) : (int[] table) {
    table = new(int, size, "Damage Lookup Table");
    for (int i = 0; i < size; i++) {
        // Quadratic growth curve
        table[i] = (i * i) + (i * 10) + 50;
    }
    return table;
}

function find_best_level(int[] table, int size, int target) : (int level) {
    level = 0;
    for (int i = 0; i < size; i++) {
        if (table[i] <= target) {
            level = i;
        } else {
            break;
        }
    }
    return level;
}

// ─────────────────────────────────────────────────────────────────────
//  8. STACK ALLOCATION — Auto-freed on function return
// ─────────────────────────────────────────────────────────────────────

function simulate_projectile() : (float final_x, float final_y) {
    // Stack-allocated — vanishes when function returns
    Vec2 pos = stack(Vec2, "Projectile Position");
    Vec2 vel = stack(Vec2, "Projectile Velocity");

    pos.x = 0.0;
    pos.y = 100.0;
    vel.x = 5.0;
    vel.y = 2.0;

    float gravity = 0.1;
    int steps = 50;

    for (int i = 0; i < steps; i++) {
        pos.x = pos.x + vel.x;
        pos.y = pos.y + vel.y;
        vel.y = vel.y - gravity;
    }

    // No free() needed — stack memory auto-reclaimed
    return pos.x, pos.y;
}

// ─────────────────────────────────────────────────────────────────────
//  9. BITWISE OPERATIONS — Low-level flag manipulation
// ─────────────────────────────────────────────────────────────────────

function encode_status(int poisoned, int burning, int frozen, int shielded) : (int flags) {
    flags = 0;
    if (poisoned == 1) { flags = flags | 1; }       // bit 0
    if (burning == 1)  { flags = flags | 2; }       // bit 1
    if (frozen == 1)   { flags = flags | 4; }       // bit 2
    if (shielded == 1) { flags = flags | 8; }       // bit 3
    return flags;
}

function decode_status(int flags) {
    if ((flags & 1) != 0)  { print("    [!] POISONED"); }
    if ((flags & 2) != 0)  { print("    [!] BURNING"); }
    if ((flags & 4) != 0)  { print("    [!] FROZEN"); }
    if ((flags & 8) != 0)  { print("    [+] SHIELDED"); }
    if (flags == 0)        { print("    [=] No status effects"); }
}

// ─────────────────────────────────────────────────────────────────────
//  10. COMPLEX ALGORITHM — Using std.math for real computation
// ─────────────────────────────────────────────────────────────────────

function damage_roll(int base, int level, int element, int rarity) : (int total) {
    // Use standard library functions
    int level_bonus = std.math.pow(2, std.math.min(level, 8));
    int elem_mult = 100;
    if (element == Element.FIRE)  { elem_mult = 130; }
    if (element == Element.VOID)  { elem_mult = 200; }

    int rarity_mult = 100 + (rarity * 25);

    total = ((base + level_bonus) * elem_mult * rarity_mult) / 10000;
    total = std.math.max(total, 1);
    return total;
}

// ═══════════════════════════════════════════════════════════════════════
//  MAIN — Run the complete showcase
// ═══════════════════════════════════════════════════════════════════════

void main() {
    std.io.banner("A B Y S S L A N G   —   Feature Showcase");
    print("  Every capability. One file. Zero dependencies.");
    std.io.separator();
    std.io.newline();

    float t_start = std.time.now();

    // ── SECTION 1: Enums ──
    print("[ 1 ] ENUMS");
    std.io.separator();
    print("  Element.FIRE  = %int", Element.FIRE);
    print("  Element.VOID  = %int", Element.VOID);
    print("  Rarity.LEGENDARY = %int", Rarity.LEGENDARY);
    print("  Rarity.MYTHIC    = %int", Rarity.MYTHIC);
    std.io.newline();

    // ── SECTION 2: Structs + Heap Allocation ──
    print("[ 2 ] STRUCTS & HEAP ALLOCATION (new)");
    std.io.separator();

    Hero player = new(Hero, "Player Character");
    player.name = "Artorias";
    player.hp = 100;
    player.max_hp = 100;
    player.level = 1;
    player.xp = 0;

    player.position = new(Vec2, "Player Position");
    player.position.x = 10.5;
    player.position.y = 20.0;

    print("  Hero: %str", player.name);
    print("  HP: %int/%int", player.hp, player.max_hp);
    print("  Position: (%float, %float)", player.position.x, player.position.y);
    std.io.newline();

    // ── SECTION 3: Error Handling ──
    print("[ 3 ] ERROR HANDLING (try/catch/throw)");
    std.io.separator();

    // Successful forge
    try {
        Weapon sword = forge_weapon("Abyssal Blade", 75, Element.VOID, Rarity.MYTHIC);
        player.weapon = sword;
        print("  Forged: %str (DMG: %int, Element: VOID)", sword.name, sword.damage);
    } catch (err) {
        print("  Forge Error: %str", err);
    }

    // Failed forge — should catch
    try {
        Weapon broken = forge_weapon("Impossible", -5, Element.FIRE, Rarity.COMMON);
        print("  This should not print.");
    } catch (err) {
        print("  Caught Error: %str", err);
    }

    // Risky enchant — should catch
    try {
        risky_enchant(player.weapon, 30);
        print("  Enchant +30: DMG is now %int", player.weapon.damage);
        risky_enchant(player.weapon, 999);
        print("  This should not print.");
    } catch (err) {
        print("  Caught Error: %str", err);
    }
    std.io.newline();

    // ── SECTION 4: Tuple Unpacking ──
    print("[ 4 ] MULTIPLE RETURN VALUES (Tuples)");
    std.io.separator();

    player.xp = 750;
    int lvl;
    int rem;
    lvl, rem = calculate_level_and_remainder(player.xp);
    player.level = lvl;
    print("  XP: %int -> Level %int (remainder: %int)", player.xp, lvl, rem);

    str elem_name;
    int elem_power;
    elem_name, elem_power = get_element_name(player.weapon.element);
    print("  Weapon element: %str (power: %int)", elem_name, elem_power);
    std.io.newline();

    // ── SECTION 5: String Concatenation ──
    print("[ 5 ] DYNAMIC STRING CONCATENATION");
    std.io.separator();
    str title = build_title(player.name, player.level);
    print("  Title: %str", title);
    str full = title + " the " + elem_name + " Wielder";
    print("  Full: %str", full);
    std.io.newline();

    // ── SECTION 6: Interfaces (Dynamic Dispatch) ──
    print("[ 6 ] INTERFACES (Dynamic Dispatch)");
    std.io.separator();

    DamageFormula formula = stack(DamageFormula);

    formula.calculate = formula_normal;
    int d1 = formula.calculate(player.weapon.damage, player.level);
    print("  Normal hit:  %int damage", d1);

    formula.calculate = formula_crit;
    int d2 = formula.calculate(player.weapon.damage, player.level);
    print("  Critical hit: %int damage", d2);

    formula.calculate = formula_magic;
    int d3 = formula.calculate(player.weapon.damage, player.level);
    print("  Magic hit:   %int damage", d3);
    std.io.newline();

    // ── SECTION 7: Arrays + std.array ──
    print("[ 7 ] ARRAYS & std.array");
    std.io.separator();

    int table_size = 20;
    int[] dmg_table = build_damage_table(table_size);
    print("  Damage table [0]: %int", dmg_table[0]);
    print("  Damage table [10]: %int", dmg_table[10]);
    print("  Damage table [19]: %int", dmg_table[19]);

    int best = find_best_level(dmg_table, table_size, 500);
    print("  Best level for DMG <= 500: Level %int", best);

    // Use std.array utilities
    int arr_sz = 10;
    int[] scores = new(int, arr_sz, "Score Array");
    scores[0] = 42; scores[1] = 17; scores[2] = 93;
    scores[3] = 8;  scores[4] = 65; scores[5] = 31;
    scores[6] = 77; scores[7] = 12; scores[8] = 56;
    scores[9] = 3;

    int total = std.array.sum(scores, arr_sz);
    int lo = std.array.min(scores, arr_sz);
    int hi = std.array.max(scores, arr_sz);
    print("  Scores sum: %int | min: %int | max: %int", total, lo, hi);

    std.array.sort(scores, arr_sz);
    int sorted = std.array.is_sorted(scores, arr_sz);
    print("  After sort — sorted: %int (1=yes)", sorted);
    print("  First: %int | Last: %int", scores[0], scores[9]);
    std.io.newline();

    // ── SECTION 8: Stack Allocation ──
    print("[ 8 ] STACK ALLOCATION (auto-free)");
    std.io.separator();

    float px;
    float py;
    px, py = simulate_projectile();
    print("  Projectile landed at (%float, %float)", px, py);
    print("  (Stack memory auto-reclaimed — no free needed)");
    std.io.newline();

    // ── SECTION 9: Bitwise Operations ──
    print("[ 9 ] BITWISE OPERATIONS");
    std.io.separator();

    int flags = encode_status(1, 1, 0, 1); // poisoned + burning + shielded
    print("  Status flags: %int (binary bitmask)", flags);
    decode_status(flags);

    // Bitwise manipulation
    int a = 170;      // 10101010
    int b = 85;       //  01010101
    print("  %int & %int = %int", a, b, a & b);
    print("  %int | %int = %int", a, b, a | b);
    print("  %int ^ %int = %int", a, b, a ^ b);
    print("  ~%int = %int", a, ~a);
    print("  %int << 2 = %int", a, a << 2);
    print("  %int >> 2 = %int", a, a >> 2);
    std.io.newline();

    // ── SECTION 10: std.math ──
    print("[ 10 ] std.math LIBRARY");
    std.io.separator();

    print("  pow(2, 16)     = %int", std.math.pow(2, 16));
    print("  sqrt(144)      = %int", std.math.sqrt(144));
    print("  factorial(10)  = %int", std.math.factorial(10));
    print("  gcd(48, 18)    = %int", std.math.gcd(48, 18));
    print("  lcm(12, 8)     = %int", std.math.lcm(12, 8));
    print("  fib(20)        = %int", std.math.fib(20));
    print("  is_prime(7919) = %int (1=yes)", std.math.is_prime(7919));
    print("  abs(-42)       = %int", std.math.abs(-42));
    print("  clamp(150,0,100) = %int", std.math.clamp(150, 0, 100));
    std.io.newline();

    // ── SECTION 11: Ternary Operator + Complex Expressions ──
    print("[ 11 ] TERNARY & COMPLEX EXPRESSIONS");
    std.io.separator();

    int result = (player.hp > 50) ? 1 : 0;
    print("  Player alive: %int", result);

    int final_dmg = damage_roll(
        player.weapon.damage, player.level,
        player.weapon.element, player.weapon.rarity
    );
    print("  Final damage roll: %int", final_dmg);

    // Compound assignment operators
    int x = 100;
    x += 50;
    print("  100 += 50 = %int", x);
    x -= 30;
    print("  150 -= 30 = %int", x);
    x++;
    print("  120++ = %int", x);
    x--;
    print("  121-- = %int", x);
    std.io.newline();

    // ── SECTION 12: Loops with break/continue ──
    print("[ 12 ] LOOPS (for, while, break, continue)");
    std.io.separator();

    // For loop with continue
    int even_sum = 0;
    for (int i = 0; i < 20; i++) {
        if (i % 2 != 0) { continue; }
        even_sum += i;
    }
    print("  Sum of evens 0-19: %int", even_sum);

    // While loop with break
    int fib_limit = 0;
    int fib_idx = 0;
    while (1) {
        int f = std.math.fib(fib_idx);
        if (f > 10000) { break; }
        fib_limit = f;
        fib_idx++;
    }
    print("  Largest Fibonacci <= 10000: %int (index %int)", fib_limit, fib_idx - 1);
    std.io.newline();

    // ── SECTION 13: Abyss Eye — Memory Profiler ──
    print("[ 13 ] ABYSS EYE — Built-In Memory Profiler");
    std.io.separator();
    print("  Summoning Abyss Eye to inspect ALL active allocations...");
    std.io.newline();

    abyss_eye();

    // ── SECTION 14: Clean Deallocation ──
    print("[ 14 ] MANUAL MEMORY DEALLOCATION");
    std.io.separator();

    free(scores);
    free(dmg_table);
    free(player.position);
    free(player.weapon);
    free(player);

    print("  All heap memory freed.");
    print("  Summoning Abyss Eye to verify zero leaks...");
    std.io.newline();

    abyss_eye();

    // ── TIMING ──
    float elapsed = std.time.elapsed(t_start);
    std.io.newline();
    std.io.banner("Showcase Complete");
    print("  Features demonstrated: 14");
    print("  Standard library modules used: 4");
    print("  Total execution time: %float seconds", elapsed);
    print("  Memory leaks: 0");
    std.io.separator();
    print("  AbyssLang | Made in Uzbekistan | Apache 2.0");
    std.io.separator();
}
