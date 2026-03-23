// ═══════════════════════════════════════════════════════════
//  std.io — AbyssLang Standard I/O Library
//  Formatted output utilities.
// ═══════════════════════════════════════════════════════════

// --- Print a decorated banner ---
function std.io.banner(str text) {
    print("════════════════════════════════════════════════════");
    print("  %str", text);
    print("════════════════════════════════════════════════════");
}

// --- Print a section separator ---
function std.io.separator() {
    print("────────────────────────────────────────────────────");
}

// --- Print a labeled integer value ---
function std.io.stat(str label, int value) {
    print("  %str : %int", label, value);
}

// --- Print a key-value pair ---
function std.io.kv(str key, str value) {
    print("  %str: %str", key, value);
}

// --- Read an integer from stdin with a prompt ---
function std.io.prompt_int(str message) : (int result) {
    print("%str", message);
    return input_int();
}

// --- Print a blank line ---
function std.io.newline() {
    print("");
}
