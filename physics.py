import sys

# Use sys.stdout.write for maximum fairness (print() is slower)
w = sys.stdout.write


class Ball:
    def __init__(self):
        self.y = 0.0
        self.vy = 0.0
        self.bounce = 0.0


def print_spaces(count):
    w(" " * int(count))


def main():
    w("--- SIMULATION START ---\n")

    gravity = 0.5
    dt = 0.1

    b = Ball()
    b.y = 20.0
    b.vy = 0.0
    b.bounce = 0.75

    steps = 0
    max_steps = 300

    while steps < max_steps:
        b.vy = b.vy - (gravity * dt)
        b.y = b.y + (b.vy * dt)

        if b.y < 0.0:
            b.y = 0.0
            b.vy = b.vy * -1.0
            b.vy = b.vy * b.bounce

            if b.vy < 0.5:
                if b.vy > -0.5:
                    b.vy = 0.0

        if steps % 5 == 0:
            height_int = 0
            temp = b.y
            while temp > 1.0:
                height_int += 1
                temp -= 1.0

            print_spaces(height_int)
            w("O\n")

        steps += 1

    w("--- SIMULATION END ---\n")


if __name__ == "__main__":
    main()
