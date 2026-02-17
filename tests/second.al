struct Vec2 {
    float x;
    float y;
}

function move(Vec2 v, float dx, float dy) : (Vec2 result) {
    v.x += dx;
    v.y += dy;
    return v;
}

void main() {
    Vec2 pos = new(Vec2);
    pos.x = 10.5;
    pos.y = -2.0;

    pos = move(pos, 3.0, 4.0);

    print("x = %float, y = %float", pos.x, pos.y);

    abyss_eye();
}
