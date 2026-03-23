import time


def main():
    print("--- Python Database Benchmark ---")
    records = 1000000
    db = {}

    start_time = time.time()

    print("Inserting 1,000,000 records...")
    for i in range(records):
        # Pseudo-random hash key mixing
        key = (i * 73856093) ^ 8472
        db[key] = i

    print("Retrieving 1,000,000 records...")
    misses = 0
    for i in range(records):
        key = (i * 73856093) ^ 8472
        if key not in db:
            misses += 1

    print("Deleting 500,000 records...")
    for i in range(500000):
        key = (i * 73856093) ^ 8472
        if key in db:
            del db[key]

    end_time = time.time()

    print(f"Final DB Size: {len(db)}")
    print(f"Misses: {misses}")
    print(f"Total time taken: {end_time - start_time:.4f} seconds")


if __name__ == "__main__":
    main()
