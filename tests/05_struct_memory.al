// Structs, heap/stack allocation, enums, field access, free.
enum Rank {
    NOVICE,
    VETERAN = 5,
    LEGENDARY
}

struct Point {
    int x;
    int y;
}

struct Agent {
    str name;
    int hp;
    int rank;
    Point pos;
}

function make_agent(str name, int hp, int rank) : (Agent a) {
    a = new(Agent, "Agent");
    a.name = name;
    a.hp = hp;
    a.rank = rank;
    a.pos = new(Point, "Agent pos");
    a.pos.x = 0;
    a.pos.y = 0;
    return a;
}

function distance_sq(Point a, Point b) : (int r) {
    int dx = a.x - b.x;
    int dy = a.y - b.y;
    return dx * dx + dy * dy;
}

void main() {
    Agent hero = make_agent("Abyss", 100, Rank.LEGENDARY);
    hero.pos.x = 3;
    hero.pos.y = 4;

    Point origin = stack(Point, "origin");
    origin.x = 0;
    origin.y = 0;

    print(hero.name);
    print(hero.hp);
    print(hero.rank);
    print(distance_sq(hero.pos, origin));

    hero.hp -= 30;
    hero.hp++;
    print(hero.hp);

    print(Rank.NOVICE);
    print(Rank.VETERAN);
    print(Rank.LEGENDARY);

    free(hero.pos);
    free(hero);
}
