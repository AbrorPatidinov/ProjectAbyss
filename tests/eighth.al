// --- ENUM TEST ---
enum State {
    IDLE,           // Should be 0
    RUNNING = 5,    // Should be 5
    STOPPED         // Should be 6 (auto-increments from previous)
}

// --- INTERFACE TEST ---
interface MathOp {
    // Defines a contract for a function that takes two ints and returns an int
    function execute(int a, int b) : (int res);
}

// Implementation 1
function add_op(int a, int b) : (int res) {
    res = a + b;
    return res;
}

// Implementation 2
function mul_op(int a, int b) : (int res) {
    res = a * b;
    return res;
}

void main() {
    print("=============================");
    print("    TESTING ENUMS            ");
    print("=============================");

    // Using the enum as a type (acts as an int under the hood)
    State s1 = State.IDLE;
    State s2 = State.RUNNING;
    State s3 = State.STOPPED;

    print("State.IDLE    = %int", s1);
    print("State.RUNNING = %int", s2);
    print("State.STOPPED = %int", s3);

    if (s2 == State.RUNNING) {
        print("-> Success: Enum comparison works!");
    }

    print("");
    print("=============================");
    print("    TESTING INTERFACES       ");
    print("=============================");

    // Allocate the interface on the stack (it's just a struct of function pointers)
    MathOp op = stack(MathOp);

    // 1. Dynamically assign the 'add' implementation
    op.execute = add_op;
    int res1 = op.execute(10, 20);
    print("Executing Add (10, 20) : %int", res1);

    // 2. Dynamically swap to the 'multiply' implementation
    op.execute = mul_op;
    int res2 = op.execute(10, 20);
    print("Executing Mul (10, 20) : %int", res2);

    print("");
    print("-> Success: Dynamic dispatch via OP_CALL_DYN_BOT works!");

    print("");
    abyss_eye(); // Let's look at the memory!
}
