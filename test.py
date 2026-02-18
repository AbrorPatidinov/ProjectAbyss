import sys
import time

def main():
    w = sys.stdout.write
    N = 10000000
    w("Python benchmark start\n")
    for i in range(N):
        w("42\n")
    w("Python benchmark end\n")

if __name__ == "__main__":
    main()
