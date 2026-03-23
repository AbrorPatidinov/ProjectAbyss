// ═══════════════════════════════════════════════════════════════════════
//  ABYSS SECURITY TOOLKIT v1.0
//  A real-world security analysis engine written in pure AbyssLang.
//  Demonstrates: Shannon Entropy, XOR Cipher, Signature Scanning,
//                Manual Memory Management, and the Abyss Eye Profiler.
// ═══════════════════════════════════════════════════════════════════════

// --- DATA STRUCTURES ---

struct EntropyResult {
    int total_bytes;
    int unique_bytes;
    int entropy_x100;   // Shannon entropy * 100 (fixed-point, no FP needed for display)
    int verdict;         // 0 = normal, 1 = suspicious, 2 = encrypted/ransomware
}

struct ScanResult {
    int matched;
    int pattern_id;
    int position;
}

struct CipherBlock {
    int[] data;
    int length;
    int key;
}

enum Verdict {
    NORMAL,
    SUSPICIOUS,
    ENCRYPTED,
    RANSOMWARE
}

enum ThreatLevel {
    SAFE,
    LOW,
    MEDIUM,
    HIGH,
    CRITICAL
}

// ═══════════════════════════════════════════════════════════════════════
//  MODULE 1: SHANNON ENTROPY ENGINE
//  The exact algorithm used in Abyss Watcher to detect ransomware.
//  Measures information density (randomness) of a data buffer.
//  Entropy > 7.5 on an 8-bit scale = encrypted or compressed data.
// ═══════════════════════════════════════════════════════════════════════

// Integer-based log2 approximation * 1000 (lookup for values 1-32)
// This avoids floating-point entirely for portable entropy calculation.
function log2_approx(int numerator, int denominator) : (int result) {
    // Returns log2(numerator/denominator) * 1000
    // Using repeated squaring approximation
    int ratio_x1000 = (numerator * 1000) / denominator;

    // Piecewise linear approximation of -log2(p) * 1000
    // For p in [0.001, 1.0]
    if (ratio_x1000 >= 500) {
        result = 1000 - ((ratio_x1000 - 500) * 2);
        if (result < 0) { result = 0; }
        return result;
    }
    if (ratio_x1000 >= 250) {
        result = 2000 - ((ratio_x1000 - 250) * 4);
        return result;
    }
    if (ratio_x1000 >= 125) {
        result = 3000 - ((ratio_x1000 - 125) * 8);
        return result;
    }
    if (ratio_x1000 >= 62) {
        result = 4000 - ((ratio_x1000 - 62) * 16);
        return result;
    }
    if (ratio_x1000 >= 31) {
        result = 5000 - ((ratio_x1000 - 31) * 32);
        return result;
    }
    if (ratio_x1000 >= 15) {
        result = 6000 - ((ratio_x1000 - 15) * 62);
        return result;
    }
    if (ratio_x1000 >= 7) {
        result = 7000 - ((ratio_x1000 - 7) * 125);
        return result;
    }
    if (ratio_x1000 >= 3) {
        result = 8000 - ((ratio_x1000 - 3) * 250);
        return result;
    }
    return 9000; // Very rare byte = high information content
}

function calculate_entropy(int[] buffer, int length) : (EntropyResult res) {
    res = new(EntropyResult, "Entropy Analysis Result");

    // Step 1: Build frequency table (256 possible byte values)
    int[] freq = new(int, 256, "Byte Frequency Table");
    for (int i = 0; i < 256; i++) {
        freq[i] = 0;
    }

    // Step 2: Count occurrences
    for (int i = 0; i < length; i++) {
        int byte_val = buffer[i] & 255;  // Mask to byte range
        freq[byte_val] = freq[byte_val] + 1;
    }

    // Step 3: Count unique bytes and calculate entropy
    int unique = 0;
    int entropy_sum = 0;  // Accumulates entropy * 1000

    for (int i = 0; i < 256; i++) {
        if (freq[i] > 0) {
            unique++;
            // Shannon: H -= p * log2(p)
            // p = freq[i] / length
            // We compute: freq[i] * (-log2(freq[i]/length))
            int log_val = log2_approx(freq[i], length);
            entropy_sum = entropy_sum + ((freq[i] * log_val) / length);
        }
    }

    // entropy_sum is now in units of entropy * 1000
    // Convert to entropy * 100 for easier display (e.g., 750 = 7.50 bits)
    int entropy_x100 = entropy_sum / 10;

    res.total_bytes = length;
    res.unique_bytes = unique;
    res.entropy_x100 = entropy_x100;

    // Verdict based on entropy thresholds
    // These are the EXACT thresholds used in production ransomware detection
    if (entropy_x100 >= 750) {
        res.verdict = Verdict.RANSOMWARE;   // H >= 7.50: encrypted/ransomware
    } else if (entropy_x100 >= 650) {
        res.verdict = Verdict.ENCRYPTED;    // H >= 6.50: compressed or encrypted
    } else if (entropy_x100 >= 450) {
        res.verdict = Verdict.SUSPICIOUS;   // H >= 4.50: unusual density
    } else {
        res.verdict = Verdict.NORMAL;       // H < 4.50: normal text/data
    }

    free(freq);
    return res;
}

function print_verdict(int verdict) {
    if (verdict == Verdict.NORMAL) {
        print("[  SAFE  ] Normal data - entropy within expected range.");
    } else if (verdict == Verdict.SUSPICIOUS) {
        print("[ WATCH  ] Elevated entropy - non-trivial data density.");
    } else if (verdict == Verdict.ENCRYPTED) {
        print("[WARNING ] High entropy - data appears compressed or encrypted.");
    } else {
        print("[*ALERT*] CRITICAL entropy - matches ransomware encryption signature!");
    }
}


// ═══════════════════════════════════════════════════════════════════════
//  MODULE 2: XOR CIPHER ENGINE
//  Demonstrates: encryption/decryption, bitwise ops, memory management.
//  XOR is used by real malware for payload obfuscation.
// ═══════════════════════════════════════════════════════════════════════

function xor_encrypt(int[] plaintext, int length, int key) : (CipherBlock result) {
    result = new(CipherBlock, "XOR Cipher Output");
    result.data = new(int, length, "Encrypted Data Buffer");
    result.length = length;
    result.key = key;

    // Multi-byte XOR with key rotation
    for (int i = 0; i < length; i++) {
        // Rotate key bits for each position (simple stream cipher)
        int rotated_key = ((key << (i % 7)) | (key >> (8 - (i % 7)))) & 255;
        result.data[i] = (plaintext[i] ^ rotated_key) & 255;
    }

    return result;
}

function xor_decrypt(CipherBlock cipher) : (int[] plaintext) {
    plaintext = new(int, cipher.length, "Decrypted Data Buffer");

    for (int i = 0; i < cipher.length; i++) {
        int rotated_key = ((cipher.key << (i % 7)) | (cipher.key >> (8 - (i % 7)))) & 255;
        plaintext[i] = (cipher.data[i] ^ rotated_key) & 255;
    }

    return plaintext;
}


// ═══════════════════════════════════════════════════════════════════════
//  MODULE 3: SIGNATURE SCANNER
//  Pattern-matching engine that scans buffers for known threat markers.
//  Demonstrates: data structures, search algorithms, enum dispatch.
// ═══════════════════════════════════════════════════════════════════════

struct Signature {
    int id;
    int[] pattern;
    int pattern_len;
    int threat_level;
}

struct SignatureDB {
    Signature[] sigs;
    int count;
    int capacity;
}

function sig_db_init() : (SignatureDB db) {
    db = new(SignatureDB, "Signature Database");
    db.capacity = 16;
    db.count = 0;
    db.sigs = new(Signature, 16, "Signature Table");
    return db;
}

function sig_add(SignatureDB db, int id, int[] pattern, int plen, int threat) {
    Signature s = new(Signature, "Threat Signature");
    s.id = id;
    s.pattern = pattern;
    s.pattern_len = plen;
    s.threat_level = threat;
    db.sigs[db.count] = s;
    db.count++;
}

function sig_scan(SignatureDB db, int[] buffer, int buf_len) : (int matches) {
    matches = 0;

    for (int s = 0; s < db.count; s++) {
        Signature sig = db.sigs[s];

        // Sliding window search
        for (int i = 0; i <= buf_len - sig.pattern_len; i++) {
            int match = 1;
            for (int j = 0; j < sig.pattern_len; j++) {
                if ((buffer[i + j] & 255) != (sig.pattern[j] & 255)) {
                    match = 0;
                    break;
                }
            }
            if (match == 1) {
                matches++;
                if (sig.threat_level == ThreatLevel.CRITICAL) {
                    print("[!] CRITICAL MATCH: Signature #%int at offset %int", sig.id, i);
                } else if (sig.threat_level == ThreatLevel.HIGH) {
                    print("[!] HIGH MATCH: Signature #%int at offset %int", sig.id, i);
                } else {
                    print("[*] Match: Signature #%int at offset %int", sig.id, i);
                }
            }
        }
    }

    return matches;
}


// ═══════════════════════════════════════════════════════════════════════
//  MODULE 4: FORENSIC REPORT GENERATOR
// ═══════════════════════════════════════════════════════════════════════

function hex_dump_line(int[] buffer, int offset, int length) {
    int end = offset + 16;
    if (end > length) { end = length; }

    // Print hex values
    for (int i = offset; i < end; i++) {
        int val = buffer[i] & 255;
        // Print each byte (simplified - AbyssLang prints as integers)
        if (val < 16) {
            print("0%int ", val);
        } else {
            print("%int ", val);
        }
    }
}


// ═══════════════════════════════════════════════════════════════════════
//  MAIN: FULL SECURITY ANALYSIS DEMONSTRATION
// ═══════════════════════════════════════════════════════════════════════

void main() {
    print("================================================================");
    print("   A B Y S S   S E C U R I T Y   T O O L K I T   v 1 . 0");
    print("   Written in pure AbyssLang | Zero dependencies");
    print("================================================================");
    print("");

    // ─── TEST 1: ENTROPY ANALYSIS ON NORMAL TEXT ───

    print(">>> MODULE 1: Shannon Entropy Analysis Engine");
    print("---------------------------------------------------");
    print("");

    // Simulate ASCII text data ("Hello World, this is AbyssLang" repeated)
    int text_size = 512;
    int[] text_buf = new(int, text_size, "Normal Text Buffer");
    for (int i = 0; i < text_size; i++) {
        // Repeating pattern of common ASCII values (text-like)
        int mod = i % 26;
        text_buf[i] = 65 + mod;  // A-Z cycling
    }

    print("[*] Analyzing: Normal text data (%int bytes)...", text_size);
    EntropyResult r1 = calculate_entropy(text_buf, text_size);
    print("    Unique bytes: %int / 256", r1.unique_bytes);
    print("    Entropy: %int.%int bits/byte", r1.entropy_x100 / 100, r1.entropy_x100 % 100);
    print_verdict(r1.verdict);
    print("");

    // ─── TEST 2: ENTROPY ANALYSIS ON ENCRYPTED DATA ───

    int enc_size = 512;
    int[] enc_buf = new(int, enc_size, "Encrypted Data Buffer");

    // Simulate high-entropy data (pseudo-random using LCG)
    int seed = 1337;
    for (int i = 0; i < enc_size; i++) {
        seed = (seed * 1103515245 + 12345) & 255;
        enc_buf[i] = seed;
    }

    print("[*] Analyzing: Suspicious encrypted buffer (%int bytes)...", enc_size);
    EntropyResult r2 = calculate_entropy(enc_buf, enc_size);
    print("    Unique bytes: %int / 256", r2.unique_bytes);
    print("    Entropy: %int.%int bits/byte", r2.entropy_x100 / 100, r2.entropy_x100 % 100);
    print_verdict(r2.verdict);
    print("");

    // ─── TEST 3: XOR CIPHER ───

    print(">>> MODULE 2: XOR Cipher Engine");
    print("---------------------------------------------------");
    print("");

    // Create plaintext
    int plain_size = 64;
    int[] plaintext = new(int, plain_size, "Plaintext Payload");
    for (int i = 0; i < plain_size; i++) {
        plaintext[i] = 65 + (i % 26);  // A-Z
    }

    int xor_key = 42;
    print("[*] Encrypting %int bytes with XOR key 0x%int...", plain_size, xor_key);
    CipherBlock encrypted = xor_encrypt(plaintext, plain_size, xor_key);

    // Show entropy of encrypted data
    EntropyResult r3 = calculate_entropy(encrypted.data, encrypted.length);
    print("    Post-encryption entropy: %int.%int bits/byte", r3.entropy_x100 / 100, r3.entropy_x100 % 100);
    print_verdict(r3.verdict);
    print("");

    print("[*] Decrypting...");
    int[] decrypted = xor_decrypt(encrypted);

    // Verify decryption
    int verify_ok = 1;
    for (int i = 0; i < plain_size; i++) {
        if (decrypted[i] != plaintext[i]) {
            verify_ok = 0;
            break;
        }
    }
    if (verify_ok == 1) {
        print("[+] Decryption verified: plaintext matches original.");
    } else {
        print("[-] DECRYPTION FAILED - data mismatch!");
    }
    print("");

    // ─── TEST 4: SIGNATURE SCANNING ───

    print(">>> MODULE 3: Threat Signature Scanner");
    print("---------------------------------------------------");
    print("");

    SignatureDB db = sig_db_init();

    // Define threat signatures (byte patterns found in real malware)
    int[] sig1 = new(int, 4, "Ransomware Note Marker");
    sig1[0] = 82; sig1[1] = 65; sig1[2] = 78; sig1[3] = 83;  // "RANS"

    int[] sig2 = new(int, 3, "Shell Spawn Pattern");
    sig2[0] = 47; sig2[1] = 98; sig2[2] = 105;  // "/bi" (part of /bin/sh)

    int[] sig3 = new(int, 4, "Crypto Header");
    sig3[0] = 0; sig3[1] = 255; sig3[2] = 0; sig3[3] = 255;  // Alternating null/FF

    sig_add(db, 1001, sig1, 4, ThreatLevel.CRITICAL);
    sig_add(db, 1002, sig2, 3, ThreatLevel.HIGH);
    sig_add(db, 1003, sig3, 4, ThreatLevel.MEDIUM);

    // Create test buffer with embedded signatures
    int scan_size = 256;
    int[] scan_buf = new(int, scan_size, "Scan Target Buffer");
    for (int i = 0; i < scan_size; i++) {
        scan_buf[i] = 0;
    }

    // Plant "RANS" at offset 32
    scan_buf[32] = 82; scan_buf[33] = 65; scan_buf[34] = 78; scan_buf[35] = 83;

    // Plant "/bi" at offset 100
    scan_buf[100] = 47; scan_buf[101] = 98; scan_buf[102] = 105;

    // Plant crypto header at offset 200
    scan_buf[200] = 0; scan_buf[201] = 255; scan_buf[202] = 0; scan_buf[203] = 255;

    print("[*] Scanning %int byte buffer against %int signatures...", scan_size, db.count);
    print("");
    int total_matches = sig_scan(db, scan_buf, scan_size);
    print("");
    print("[*] Scan complete: %int threat signatures detected.", total_matches);
    print("");

    // ─── ABYSS EYE: SHOW THE WHOLE OPERATION ───

    print(">>> MODULE 4: Memory Forensics (Abyss Eye)");
    print("---------------------------------------------------");
    print("[*] Summoning Abyss Eye to inspect all active allocations...");
    print("");

    abyss_eye();

    // ─── CLEANUP ───

    print(">>> Secure Cleanup: Freeing all allocated memory...");
    print("");

    free(r1);
    free(r2);
    free(r3);
    free(encrypted.data);
    free(encrypted);
    free(decrypted);
    free(plaintext);
    free(text_buf);
    free(enc_buf);
    free(scan_buf);

    // Free signature data
    for (int i = 0; i < db.count; i++) {
        Signature s = db.sigs[i];
        free(s);
    }
    free(sig1);
    free(sig2);
    free(sig3);
    free(db.sigs);
    free(db);

    print("[+] All memory securely deallocated.");
    print("");

    // Final Abyss Eye: Prove zero leaks
    print(">>> Final Abyss Eye: Proving zero memory leaks...");
    print("");
    abyss_eye();

    print("================================================================");
    print("   Analysis complete. All systems nominal.");
    print("   Built with AbyssLang. Made in Uzbekistan.");
    print("================================================================");
}
