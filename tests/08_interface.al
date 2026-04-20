// Interface-based dynamic dispatch.
interface Op {
    function run(int a, int b) : (int r);
}

function plus(int a, int b) : (int r) { return a + b; }
function times(int a, int b) : (int r) { return a * b; }
function minus(int a, int b) : (int r) { return a - b; }

void main() {
    Op op = stack(Op, "op");

    op.run = plus;
    print(op.run(3, 4));

    op.run = times;
    print(op.run(3, 4));

    op.run = minus;
    print(op.run(10, 3));
}
