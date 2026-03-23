// ═══════════════════════════════════════════════════════════
//  std.array — AbyssLang Standard Array Library
//  Array manipulation utilities.
// ═══════════════════════════════════════════════════════════

// --- Fill array with a value ---
function std.array.fill(int[] arr, int length, int value) {
    for (int i = 0; i < length; i++) {
        arr[i] = value;
    }
}

// --- Sum all elements ---
function std.array.sum(int[] arr, int length) : (int result) {
    result = 0;
    for (int i = 0; i < length; i++) {
        result += arr[i];
    }
    return result;
}

// --- Find minimum value ---
function std.array.min(int[] arr, int length) : (int result) {
    result = arr[0];
    for (int i = 1; i < length; i++) {
        if (arr[i] < result) { result = arr[i]; }
    }
    return result;
}

// --- Find maximum value ---
function std.array.max(int[] arr, int length) : (int result) {
    result = arr[0];
    for (int i = 1; i < length; i++) {
        if (arr[i] > result) { result = arr[i]; }
    }
    return result;
}

// --- Linear search (returns index or -1) ---
function std.array.find(int[] arr, int length, int target) : (int result) {
    for (int i = 0; i < length; i++) {
        if (arr[i] == target) { return i; }
    }
    return -1;
}

// --- Count occurrences of a value ---
function std.array.count(int[] arr, int length, int target) : (int result) {
    result = 0;
    for (int i = 0; i < length; i++) {
        if (arr[i] == target) { result++; }
    }
    return result;
}

// --- Reverse array in-place ---
function std.array.reverse(int[] arr, int length) {
    int lo = 0;
    int hi = length - 1;
    while (lo < hi) {
        int temp = arr[lo];
        arr[lo] = arr[hi];
        arr[hi] = temp;
        lo++;
        hi--;
    }
}

// --- Swap two elements ---
function std.array.swap(int[] arr, int i, int j) {
    int temp = arr[i];
    arr[i] = arr[j];
    arr[j] = temp;
}

// --- Insertion sort (for small arrays) ---
function std.array.sort(int[] arr, int length) {
    for (int i = 1; i < length; i++) {
        int key = arr[i];
        int j = i - 1;
        while (j >= 0 && arr[j] > key) {
            arr[j + 1] = arr[j];
            j--;
        }
        arr[j + 1] = key;
    }
}

// --- Check if array is sorted ---
function std.array.is_sorted(int[] arr, int length) : (int result) {
    for (int i = 1; i < length; i++) {
        if (arr[i] < arr[i - 1]) { return 0; }
    }
    return 1;
}

// --- Copy array ---
function std.array.copy(int[] src, int[] dst, int length) {
    for (int i = 0; i < length; i++) {
        dst[i] = src[i];
    }
}
