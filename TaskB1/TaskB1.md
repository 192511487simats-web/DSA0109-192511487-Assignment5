# Task B1 – Implement and Validate the Polymorphic Billing (7 Marks)

## B1.1 – Hand-Verified Test Table (2 Marks)
Testing one bill of each category across `SelfPay`, `Insurance` (₹50,000 limit, 10% co-pay), and `GovernmentScheme` (fixed rates: OP ₹1,000, IP ₹35,000, DayCare ₹15,000, Emergency ₹8,000):

| Bill Type (Patient) | Hand Calculation | Bill Total | `SelfPay` Covered / Patient | `Insurance` (90% up to 50k) Covered / Patient | `GovtScheme` Covered / Patient |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **OutPatient** (Ravi) | `500 + 1200` | **1700.00** | `0.00` / `1700.00` | `1530.00` / `170.00` | `1000.00` / `700.00` |
| **InPatient** (Meena) | `(3 × 2500) + 40000 + 2500` | **50000.00** | `0.00` / `50000.00` | `45000.00` / `5000.00` | `35000.00` / `15000.00` |
| **DayCareSurgery** (Arun) | `18000` | **18000.00** | `0.00` / `18000.00` | `16200.00` / `1800.00` | `15000.00` / `3000.00` |
| **Emergency** (Priya) | `2000 + (4 × 1500)` | **8000.00** | `0.00` / `8000.00` | `7200.00` / `800.00` | `8000.00` / `0.00` |

Every program output matches the hand-calculated values exactly.

---

## B1.2 – Comparison with Part A Design (1 Mark)
* **Refinements Made During Implementation:**
  1. Replaced raw `Bill*` arrays from Part A pseudocode with `std::vector<std::unique_ptr<Bill>>` to guarantee exception-safe RAII cleanup.
  2. Marked `type()` as a pure virtual function (`= 0`) in `hospital_billing.cpp` (whereas `complete_b1.cpp` provides a default `"Bill"` implementation as instructed by `TODO 1`).
  3. Omitted `billId` and `billDate` fields in the minimal test harness to focus directly on numerical validation.

---

## B1.3 – Proving Runtime Binding (2 Marks)
1. **In `complete_b1.cpp` (removing `virtual` from `virtual double total() const = 0;`):**
   * **Compiler Error:**
     `error: initializer specified for non-virtual method 'double Bill::total() const'`
   * **Explanation:** C++ forbids the pure-specifier (`= 0`) on non-virtual member functions because pure virtual functions exist specifically to enforce runtime overriding via a `vtable`.
2. **If `total()` is given a dummy body (`double total() const { return 0; }`) without `virtual`:**
   * **Output Changes To:** `OP Ravi: 0`, `IP Meena: 0`, `Total 0`.
   * **Explanation:** Without `virtual`, `b->total()` uses static (early) binding on `Bill*` and calls `Bill::total()` every time instead of dispatching to `OutPatient::total()` or `InPatient::total()`.

---

## B1.4 – Proving the Virtual Destructor under AddressSanitizer (2 Marks)
When `virtual` is removed from `~Bill()` in `complete_b1.cpp` and compiled with `g++ -std=c++17 -Wall -Wextra -g -fsanitize=address`:
* **Compiler Warning:** `-Wdelete-non-virtual-dtor` warns that deleting polymorphic class `Bill` with a non-virtual destructor causes undefined behavior.
* **AddressSanitizer Result:** ASan immediately aborts on `delete b;` with **`ERROR: AddressSanitizer: new-delete-type-mismatch`** because `new OutPatient` allocated `sizeof(OutPatient)` (48 bytes on 64-bit libstdc++), whereas non-virtual `delete b` passes `sizeof(Bill)` (40 bytes) to `operator delete(void*, unsigned long)`.

---

## Alternative Concept and Complexity Analysis (Compile-Time Templates)

### 1. Alternative Design (Templates & Separate Homogeneous Containers)
Instead of runtime polymorphism (`std::vector<Bill*>`), each bill type could be stored in its own typed container (`std::vector<OutPatientBill>`, `std::vector<InPatientBill>`, etc.) and summed with a function template:

```cpp
template <typename BillType>
double sumContainer(const std::vector<BillType>& bills) {
    double sum = 0.0;
    for (const auto& b : bills) sum += b.total(); // Statically resolved & inlined
    return sum;
}
```

### 2. Time, Space & Flexibility Comparison

| Metric | Runtime Polymorphism (`Bill*` + `virtual`) | Compile-Time Templates (`std::vector<T>`) |
| :--- | :--- | :--- |
| **Time per Bill** | **\(O(1)\) with indirect call** (~1–2 ns `vtable` lookup; prevents cross-call inlining). | **\(O(1)\) with zero call overhead** — Compiler statically resolves and **inlines** `b.total()` inside the loop, enabling SIMD vectorization. |
| **Space Overhead** | **1 `vptr` (8 bytes) per object** + heap allocation overhead per object (`new`). | **Zero `vptr` overhead**; objects sit contiguously in cache-friendly arrays, though **\(k\) separate containers** are needed for \(k\) types. |
| **What Is Lost** | Nothing structurally—bills of all types coexist in a single chronological list. | **Heterogeneous ordering & runtime extensibility:** You cannot keep all bills in **one single chronological list** (e.g., sorted by arrival time) without type-erasure wrappers, and new types cannot be plugged in dynamically without recompiling template instantiations. |
