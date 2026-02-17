struct Point {
    int x;
    int y;
}

void main() {
    Point p = stack(Point);
    p.x = 7;
    p.y = 9;

    print("Point: %int, %int", p.x, p.y);
}
