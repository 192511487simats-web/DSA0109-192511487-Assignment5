# Task B4 – Final Engineering Decision (5 Marks)

## B4.1 – Practical Extension Without Modifying Existing Classes (2 Marks)
* **Added Extensions:**
  1. **`TelemedicineBill : public Bill`**: Calculates `videoConsultFee + digitalPrescriptionFee` and returns `"Telemedicine"`.
  2. **`CorporateTieUp : public Payer`**: Covers 80% (`coverageRatio = 0.80`) of any bill up to a ₹25,000 cap (`capLimit = 25000.0`).
* **Verification:**
  * Zero lines of code were changed in `Bill`, `OutPatientBill`, `InPatientBill`, `DayCareSurgeryBill`, `EmergencyBill`, `Payer`, `SelfPay`, `Insurance`, or `GovernmentScheme`.
  * All earlier `assert()` regression checks passed, and `settleBill()` processed `TelemedicineBill` and `CorporateTieUp` seamlessly via base interfaces.

---

## B4.2 – Resource Safety & AddressSanitizer Report (1 Mark)
* **Compilation & Execution:**
  ```bash
  g++ -std=c++17 -Wall -Wextra -g -fsanitize=address extended_system.cpp -o extended_system
  ./extended_system
  ```
* **Sanitizer Verification:** During execution, three exceptions were thrown (`ClaimError` on insurance cover overflow, `ClaimError` on unmapped scheme package, and `BillingError` inside a failed `TelemedicineBill` constructor) and caught across two layers (`settleBill` audit rethrow and `main` recovery). AddressSanitizer exited with **status 0 and zero leak/memory-error diagnostics**, proving that RAII (`std::unique_ptr`, `std::vector`, `std::string`) and virtual destructors released all stack and heap resources cleanly during stack unwinding.

---

## B4.3 – Production Decision for the 15-Hospital Group (2 Marks)
* **Decision:** **YES — Adopt the polymorphic C++ architecture with RAII and exception handling for the 15-hospital production system.**
* **Quantitative & Engineering Justification:**
  1. **Negligible Runtime Overhead at Scale:** The hospital group processes **200,000 bills per day** (~2.31 bills/second on average, or ~20–50 bills/second at peak). From our Task B3 benchmarks, virtual dispatch costs ~**3.6 nanoseconds per bill**—meaning 200,000 virtual calls take less than **0.75 milliseconds total CPU time per day** (under 0.001% of a single core).
  2. **Low Yearly Extension Effort (Open-Closed Principle):** Adding **2 new bill types and 3 new payers per year** requires writing only 5 small, isolated derived classes (~20–30 lines each) without touching or risking regressions in existing billing classes or settlement loops.
  3. **High Reliability & Fault Isolation:** Because invalid claims are a small minority (`< 1–2%`), zero-cost C++ exceptions incur virtually zero overhead on valid bills while guaranteeing via RAII that a malformed claim or unknown package never corrupts batch totals or leaks memory across any of the 15 hospitals.

---

## Alternative Concept and Complexity Analysis (Data-Driven Plug-Ins / Configuration Files)

### 1. Alternative Design (JSON / YAML Rule Engine Loaded at Runtime)
Instead of compiling new C++ subclasses for every new bill type or payer, billing formulas and payer rules could be stored in external configuration files (e.g., `rules.json`) and evaluated by a rule-interpreter engine at runtime.

### 2. Time, Space, Effort & Risk Comparison

| Metric | Compiled C++ Polymorphic Classes | Configuration-File / Plug-In Rule Engine |
| :--- | :--- | :--- |
| **Time per Bill** | **\(O(1)\) (~3.6 ns)** — Direct native machine code via 1 `vtable` lookup. | **\(O(R)\) (~200–2,000 ns)** — Must parse/traverse AST rule expressions, look up variable maps (`std::unordered_map`), and interpret conditions per bill. |
| **Space for Rules** | **Minimal (~24–48 bytes per object + 1 static `vtable` per class)** in read-only `.rodata`. | **High (KB–MB of heap memory)** to hold parsed AST nodes, rule dictionaries, string keys, and schema metadata. |
| **Effort to Add 2 Bills + 3 Payers/Year** | **Low-to-Moderate** — Requires writing C++ classes, running unit tests, and deploying a new binary release. | **Very Low (Zero Recompilation)** — Business analysts can update `rules.json` and hot-reload rules without rebuilding C++ binaries. |
| **Engineering Risk** | **Low (Compile-Time Type Safety)** — Syntax errors, missing overrides, and type mismatches are caught by `g++` before deployment. | **High (Runtime Logic Risk)** — Typographical errors in config formulas, division by zero, or missing edge-case guards fail only at runtime unless a heavy rule-validator sandbox is built. |
