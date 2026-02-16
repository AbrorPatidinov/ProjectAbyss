struct Point {
    int x;
    int y;
}

function create_point_on_stack(int x, int y) : (Point p) {
    // This allocates on the stack frame of this function
    p = stack(Point);
    p.x = x;
    p.y = y;
    print("Inside func: Point is at %{int}, %{int}", p.x, p.y);
    // When this function returns, 'p' is technically popped from the stack frame
    // But since we return it by value (copying the pointer),
    // If we return the pointer, it points to invalid stack memory!
    // This is the danger of stack allocation.
    // BUT, for local usage, it's perfect.
    return p;
}

void main() {
    print("--- Stack Allocation Test ---");

    // 1. Local Stack Allocation
    Point p1 = stack(Point);
    p1.x = 10;
    p1.y = 20;
    print("Stack Point: %{int}, %{int}", p1.x, p1.y);

    // 2. Verify Heap is Empty
    abyss_eye();

    print("--- End Test ---");
}
