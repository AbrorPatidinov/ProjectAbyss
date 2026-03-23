// abyss_db.al - The Ultimate Infrastructure Test

struct Entry {
    int key;
    int value;
    int state; // 0 = empty, 1 = active, 2 = tombstone (deleted)
}

struct AbyssDB {
    Entry[] table;
    int capacity;
    int size;
}

// 1. Initialize the Database
function db_init(int capacity) : (AbyssDB db) {
    db = new(AbyssDB);
    db.capacity = capacity;
    db.size = 0;

    // Allocate the underlying hash table
    db.table = new(Entry, capacity, "DB Hash Table Array");

    // Pre-allocate slots to prevent null-pointer errors
    for (int i = 0; i < capacity; i++) {
        Entry e = new(Entry, "DB Slot");
        e.state = 0;
        db.table[i] = e;
    }
    return db;
}

// 2. High-speed integer hashing algorithm (Thomas Wang mix)
function hash(int key, int capacity) : (int idx) {
    key = (key ^ 61) ^ (key >> 16);
    key = key + (key << 3);
    key = key ^ (key >> 4);
    key = key * 668265261;
    key = key ^ (key >> 15);

    // Ensure index is positive
    if (key < 0) {
        key = -key;
    }
    return key % capacity;
}

// 3. Insert or Update Record (Linear Probing)
function db_set(AbyssDB db, int key, int value) : (int success) {
    int idx = hash(key, db.capacity);

    while (1) {
        Entry e = db.table[idx];

        // Empty slot or deleted slot
        if (e.state == 0 || e.state == 2) {
            e.key = key;
            e.value = value;
            e.state = 1;
            db.size++;
            return 1;
        }

        // Update existing key
        if (e.state == 1 && e.key == key) {
            e.value = value;
            return 1;
        }

        // Collision! Probe forward.
        idx++;
        if (idx >= db.capacity) {
            idx = 0;
        }
    }
    return 0;
}

// 4. Retrieve Record (Tuple Unpacking!)
function db_get(AbyssDB db, int key) : (int found, int value) {
    int idx = hash(key, db.capacity);

    while (1) {
        Entry e = db.table[idx];

        // Hit an empty slot -> Key doesn't exist
        if (e.state == 0) {
            return 0, 0;
        }
        // Found active key
        if (e.state == 1 && e.key == key) {
            return 1, e.value;
        }

        idx++;
        if (idx >= db.capacity) {
            idx = 0;
        }
    }
    return 0, 0;
}

// 5. Delete Record
function db_delete(AbyssDB db, int key) : (int success) {
    int idx = hash(key, db.capacity);

    while (1) {
        Entry e = db.table[idx];
        if (e.state == 0) {
            return 0; // Not found
        }
        if (e.state == 1 && e.key == key) {
            e.state = 2; // Mark as tombstone
            db.size--;
            return 1;
        }

        idx++;
        if (idx >= db.capacity) {
            idx = 0;
        }
    }
    return 0;
}

// 6. Graceful Memory Deallocation
function db_free(AbyssDB db) {
    for (int i = 0; i < db.capacity; i++) {
        Entry e = db.table[i];
        free(e); // Free every struct
    }
    free(db.table); // Free the array
    free(db);       // Free the database struct
}

void main() {
    print("--- AbyssDB High-Performance Benchmark ---");

    // Load Factor of 0.5 (2M slots for 1M records) to ensure fast probing
    int capacity = 2000000;
    int records  = 1000000;

    print("Allocating memory for 2,000,000 database slots...");
    AbyssDB db = db_init(capacity);

    print("Inserting 1,000,000 records...");
    for (int i = 0; i < records; i++) {
        int key = (i * 73856093) ^ 8472;
        db_set(db, key, i);
    }

    print("Retrieving 1,000,000 records...");
    int misses = 0;
    for (int j = 0; j < records; j++) {
        int key = (j * 73856093) ^ 8472;

        // Demonstrating native Tuple Unpacking!
        int found;
        int val;
        found, val = db_get(db, key);
        if (found == 0) {
            misses++;
        }
    }

    print("Deleting 500,000 records...");
    for (int k = 0; k < 500000; k++) {
        int key = (k * 73856093) ^ 8472;
        db_delete(db, key);
    }

    print("Final DB Size: %int", db.size);
    print("Misses: %int", misses);

    print("Freeing database memory...");
    db_free(db);

    print("--- DB Destroyed. Summoning Abyss Eye... ---");
    abyss_eye();
}
