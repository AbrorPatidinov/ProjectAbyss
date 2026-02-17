// --- DATA STRUCTURES ---

struct File {
    int id;
    int size;
    str name;
    str content;
}

struct System {
    int file_count;
    int max_files;
    int boot_time;
}

// Global array to hold pointers to File structs
// In AbyssLang, pointers are 64-bit integers, so we use int[]
int[] disk;
System sys;

// --- HELPER FUNCTIONS ---

// Returns multiple values (Tuple Unpacking Showcase)
function get_file_stats(File f) : (int id, int size) {
    id = f.id;
    size = f.size;
    return id, size;
}

function find_file_index(int target_id) : (int index) {
    index = -1;
    for (int i = 0; i < sys.max_files; i++) {
        // Check if slot is not empty (0)
        if (disk[i] != 0) {
            File f = disk[i]; // Cast int pointer back to struct
            if (f.id == target_id) {
                return i;
            }
        }
    }
    return index;
}

function create_file(int id, str name, str content) : (int success) {
    if (sys.file_count >= sys.max_files) {
        throw "Disk Full! Cannot create file.";
    }

    // Find empty slot
    int slot = -1;
    for (int i = 0; i < sys.max_files; i++) {
        if (disk[i] == 0) {
            slot = i;
            // Break manually by setting iterator to max (since we don't have 'break' keyword exposed yet in logic)
            i = sys.max_files;
        }
    }

    // Manual Memory Allocation (Heap)
    File f = new(File);
    f.id = id;
    f.name = name;
    f.content = content;
    f.size = 1024; // Mock size

    disk[slot] = f;
    sys.file_count++;

    print("File '%{str}' created at memory address %{int}", name, f);
    return 1;
}

function delete_file(int id) : (int success) {
    int idx = find_file_index(id);

    if (idx == -1) {
        throw "File ID not found!";
    }

    File f = disk[idx];

    print("Deleting file: %{str}...", f.name);

    // Manual Memory Deallocation
    free(f);

    disk[idx] = 0; // Clear pointer
    sys.file_count--;

    return 1;
}

// Stack Allocation Showcase
function clipboard_test() : (int result) {
    print("--- Stack Allocation Test ---");
    // This allocates on the stack frame.
    // It does NOT need free() and vanishes when function returns.
    File clip = stack(File);
    clip.name = "Clipboard_Data";
    clip.content = "Temporary String";

    print("Clipboard (Stack) created at: %{int}", clip);
    print("Content: %{str}", clip.content);

    return 1;
}

// --- MAIN KERNEL ---

void main() {
    print("========================================");
    print("      A B Y S S   O S   v 1 . 0         ");
    print("========================================");

    // Initialize System
    sys = new(System);
    sys.max_files = 5;
    sys.file_count = 0;
    sys.boot_time = clock();

    // Initialize Disk (Array of pointers)
    disk = new(int, 5);

    // Init disk to 0
    for(int i=0; i<5; i++) { disk[i] = 0; }

    int running = 1;
    int next_id = 100;

    while (running) {
        print("");
        print("--- MENU ---");
        print("1. Create File (Heap)");
        print("2. List Files");
        print("3. Analyze File (Unpacking)");
        print("4. Delete File (Free)");
        print("5. Clipboard Test (Stack)");
        print("6. System Status (Abyss Eye)");
        print("7. Shutdown");
        print("Select > ");

        int choice = input_int();
        print("");

        try {
            if (choice == 1) {
                print("Enter dummy content ID (integer):");
                int dummy_content = input_int();

                // Create file
                create_file(next_id, "user_doc.txt", "Some content");
                next_id++;
            }
            else if (choice == 2) {
                print("--- FILE LIST ---");
                int found = 0;
                for (int k = 0; k < sys.max_files; k++) {
                    if (disk[k] != 0) {
                        File f = disk[k];
                        print("[ID: %{int}] Name: %{str}", f.id, f.name);
                        found++;
                    }
                }
                if (found == 0) { print("(Disk Empty)"); }
            }
            else if (choice == 3) {
                print("Enter File ID to analyze:");
                int target = input_int();
                int idx = find_file_index(target);

                if (idx == -1) { throw "File not found."; }

                File f = disk[idx];

                // TUPLE UNPACKING SHOWCASE
                int fid;
                int fsize;
                fid, fsize = get_file_stats(f);

                print("Analysis Result -> ID: %{int} | Size: %{int} bytes", fid, fsize);
            }
            else if (choice == 4) {
                print("Enter File ID to delete:");
                int target = input_int();
                delete_file(target);
                print("File deleted successfully.");
            }
            else if (choice == 5) {
                clipboard_test();
                print("(Back in main: Stack memory automatically reclaimed)");
            }
            else if (choice == 6) {
                // THE ULTIMATE VISUALIZER
                abyss_eye();
            }
            else if (choice == 7) {
                running = 0;
            }
        }
        catch (err) {
            print("!!! SYSTEM ERROR !!!");
            print("Details: %{str}", err);
        }
    }

    print("Shutting down...");

    // Cleanup OS memory
    free(disk);
    free(sys);

    print("System Halted.");
    abyss_eye(); // Should be empty
}
