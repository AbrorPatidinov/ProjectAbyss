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

// Global pointers
int[] disk;
System sys;

// --- HELPER FUNCTIONS ---

// Feature: Multiple Return Values (Tuple)
function get_file_meta(File f) : (int id, int size, str name) {
  return f.id, f.size, f.name;
}

// Feature: Heap Allocation & Break
function create_file(int id, str name, str content) : (int success) {
  if (sys.file_count >= sys.max_files) {
    throw "Disk Full! Delete some files first.";
  }

  // Find empty slot using BREAK
  int slot = -1;
  for (int i = 0; i < sys.max_files; i++) {
    if (disk[i] == 0) {
      slot = i;
      break; // <--- NEW FEATURE: Exit loop immediately
    }
  }

  if (slot == -1) {
    throw "System Error: Count mismatch (Fragmentation).";
  }

  // Manual Memory Allocation (Heap)
  File f = new (File);
  f.id = id;
  f.name = name;
  f.content = content;
  f.size = 1024;

  disk[slot] = f;

  // Feature: Struct Field Increment
  sys.file_count++;

  print("File '%{str}' created at Index %{int}.", name, slot);
  return 1;
}

// Feature: Manual Free & Decrement
function delete_file(int id) : (int success) {
  int found = 0;

  for (int i = 0; i < sys.max_files; i++) {
    if (disk[i] == 0) {
      continue; // <--- NEW FEATURE: Skip empty slots
    }

    File f = disk[i];
    if (f.id == id) {
      print("Deleting '%{str}'...", f.name);

      free(f);     // <--- Manual Memory Management
      disk[i] = 0; // Nullify pointer

      // Feature: Struct Field Decrement
      sys.file_count--;

      found = 1;
      break; // Stop searching
    }
  }

  if (found == 0) {
    throw "File ID not found!";
  }

  return 1;
}

// Feature: Stack Allocation (Auto-Free)
function clipboard_demo() : (int result) {
  print("--- Stack Memory Test ---");

  // This allocates on the stack frame.
  // It vanishes automatically when function returns.
  File clip = stack(File);
  clip.id = 999;
  clip.name = "clipboard.tmp";
  clip.content = "Copied Data";

  print("Clipboard [Stack] Address: %{int}", clip);
  print("Data: %{str}", clip.content);

  return 1;
}

// --- MAIN KERNEL ---

void main() {
  print("========================================");
  print("      A B Y S S   O S   v 2 . 0         ");
  print("========================================");

  // Initialize System
  sys = new (System);
  sys.max_files = 5;
  sys.file_count = 0;
  sys.boot_time = clock();

  // Initialize Disk (Array of pointers)
  disk = new (int, 5);

  // Init disk to 0
  for (int i = 0; i < 5; i++) {
    disk[i] = 0;
  }

  int running = 1;
  int next_id = 100;

  while (running) {
    print("");
    print("--- MENU [Files: %{int}/%{int}] ---", sys.file_count, sys.max_files);
    print("1. New File");
    print("2. List Files");
    print("3. File Info (Unpacking)");
    print("4. Delete File");
    print("5. Clipboard (Stack)");
    print("6. Memory Map (Abyss Eye)");
    print("7. Shutdown");
    print("Select > ");

    int choice = input_int();
    print("");

    try {
      if (choice == 1) {
        create_file(next_id, "doc.txt", "Hello World");
        // Feature: Variable Increment
        next_id++;
      } else if (choice == 2) {
        print("--- FILE LIST ---");
        int empty = 1;
        for (int k = 0; k < sys.max_files; k++) {
          if (disk[k] == 0) {
            continue; // <--- Skip empty
          }
          File f = disk[k];
          print("[%{int}] %{str}", f.id, f.name);
          empty = 0;
        }
        if (empty) {
          print("(Disk Empty)");
        }
      } else if (choice == 3) {
        print("Enter File ID:");
        int target = input_int();
        int found = 0;

        for (int i = 0; i < sys.max_files; i++) {
          if (disk[i] == 0) {
            continue;
          }

          File f = disk[i];
          if (f.id == target) {
            // Feature: Tuple Unpacking
            int fid;
            int fsize;
            str fname;
            fid, fsize, fname = get_file_meta(f);

            print("ID: %{int} | Name: %{str} | Size: %{int}", fid, fname,
                  fsize);
            found = 1;
            break;
          }
        }
        if (found == 0) {
          throw "File not found.";
        }
      } else if (choice == 4) {
        print("Enter File ID to delete:");
        int target = input_int();
        delete_file(target);
        print("Deleted.");
      } else if (choice == 5) {
        clipboard_demo();
        print("(Stack memory reclaimed)");
      } else if (choice == 6) {
        abyss_eye();
      } else if (choice == 7) {
        running = 0;
      }
    } catch (err) {
      print("!!! ERROR: %{str} !!!", err);
    }
  }

  print("Shutting down...");
  free(disk);
  free(sys);
  print("Bye.");
}
