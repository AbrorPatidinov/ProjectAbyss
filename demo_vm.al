// ═══════════════════════════════════════════════════════════════════════════
//  AbyssLang — VM Mode Demo (with Abyss Eye HUD)
//
//  Smaller workloads so it runs fast in the VM. Shows the Abyss Eye
//  memory profiler live, which is the feature no other language has.
//
//  Run with:  ./abyssc demo_vm.al demo_vm.aby && ./abyss_vm demo_vm.aby
// ═══════════════════════════════════════════════════════════════════════════

import std.math;
import std.time;
import std.io;
import std.array;

struct ThreatReport {
    str signature;
    int severity;
    int hash;
}

function forge_report(str sig, int sev) : (ThreatReport r) {
    r = new(ThreatReport, "threat report");
    r.signature = sig;
    r.severity = sev;
    r.hash = sev * 7919;
    return r;
}

void main() {
    std.io.banner("A B Y S S   E Y E   —   live demo");
    std.io.separator();
    std.io.newline();

    // ── Heap allocations with tagged comments ──
    ThreatReport t1 = forge_report("SSH_BRUTEFORCE", 8);
    ThreatReport t2 = forge_report("SQL_INJECTION", 9);
    ThreatReport t3 = forge_report("XSS_PAYLOAD", 6);

    print("Active threats:");
    print("  1. %str (sev %int)", t1.signature, t1.severity);
    print("  2. %str (sev %int)", t2.signature, t2.severity);
    print("  3. %str (sev %int)", t3.signature, t3.severity);
    std.io.newline();

    // ── Stack allocation, auto-freed when function returns ──
    int[] priority = new(int, 16, "priority queue");
    std.array.fill(priority, 16, 0);
    priority[0] = t1.severity;
    priority[1] = t2.severity;
    priority[2] = t3.severity;

    print("Max severity across reports: %int", std.array.max(priority, 16));
    print("Sum of severities: %int", std.array.sum(priority, 16));
    std.io.newline();

    // ── String concatenation (heap-allocated dynamic strings) ──
    str banner = "[ALERT] ";
    str msg = banner + t1.signature;
    print(msg);
    std.io.newline();

    // ── Deliberately leak one report to show Abyss Eye catches it ──
    free(t2);
    free(priority);
    // t1 and t3 intentionally not freed — Abyss Eye will flag them

    print("Summoning Abyss Eye — inspect every byte:");
    abyss_eye();

    free(t1);
    free(t3);
}
