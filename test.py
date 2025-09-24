import sys
w = sys.stdout.write
N = 200000
w("Python benchmark start\n")
for i in range(N):
    w("42\n")
w("Python benchmark end\n")
