import time


class Ball:
    def __init__(self):
        self.y = 1000.0
        self.vy = 0.0


def main():
    print("Python: Starting 100M iterations...")

    b = Ball()
    max_steps = 100000000  # 100 Million
    dt = 0.01
    g = 9.8

    start = time.time()

    i = 0
    while i < max_steps:
        # 1. Update Velocity
        b.vy = b.vy - (g * dt)

        # 2. Update Position
        b.y = b.y + (b.vy * dt)

        # 3. Collision Logic
        if b.y < 0.0:
            b.y = 0.0
            b.vy = b.vy * -0.8

        i += 1

    end = time.time()
    duration = end - start

    print("Python: Done.")
    print(f"Time (s): {duration}")


if __name__ == "__main__":
    main()
