# Task A5 – Design Recovery and Rethrowing (4 Marks)

## A5.1 – Where to Catch, Log/Rethrow, and Report (2 Marks)
A reliable hospital billing system separates **error detection**, **audit logging**, and **batch recovery** across three architectural layers so a single bad claim never aborts the entire day's batch:

1. **Detection Layer (Low-Level Validation & Adjudication — `Bill` / `Payer` / `approve()`):**
   * Detects invariant violations (negative charges, unknown package codes, claim exceeding cover limit) and **throws** the specific domain exception (`NegativeChargeError`, `UnknownPackageError`, `ClaimError`). Does **not** catch errors locally because it lacks UI or batch context.
2. **Audit & Telemetry Layer (Middle Layer — `settleClaim()` / `processBill()`):**
   * Catches domain-critical exceptions (such as `const ClaimError& e` or `const BillingError& e`) to record an immutable audit trail (timestamp, bill ID, failure reason).
   * Immediately rethrows the **exact active exception object** using **`throw;`** (never `throw e;`, which would slice derived exception types).
3. **Recovery & User Reporting Layer (Top-Level Batch Loop — `main()` / `runDailyBilling()`):**
   * Wraps each individual bill inside a `try { ... } catch (...)` block inside the batch loop.
   * Catches `const HospitalError&` and `const std::exception&` to log/display a clear rejection reason to the operator, increments a rejected-bill counter, and **continues processing the remaining bills** without losing valid data or crashing.
   * Includes a final `catch (...)` fallback to guarantee process survival against unexpected exceptions.

### SIMATS Pseudocode for Multi-Layer Catch, Rethrow & Batch Recovery
```text
ALGORITHM ProcessDailyBatch(billRecords, count)
    INPUT: billRecords (array of raw bill inputs), count (integer)
    OUTPUT: processedCount (integer), rejectedCount (integer)

    BEGIN
        processedCount <- 0
        rejectedCount <- 0

        FOR i <- 0 TO count - 1 DO
            TRY
                CALL SettleSingleBill(billRecords[i])
                processedCount <- processedCount + 1
            CATCH HospitalError AS e
                PRINT "Rejected Bill [", i, "]: ", e.what()
                rejectedCount <- rejectedCount + 1
            CATCH std::exception AS e
                PRINT "System Error on Bill [", i, "]: ", e.what()
                rejectedCount <- rejectedCount + 1
            CATCH ...
                PRINT "Unknown Critical Error on Bill [", i, "]"
                rejectedCount <- rejectedCount + 1
            END TRY
        END FOR

        RETURN processedCount, rejectedCount
    END
END ALGORITHM

ALGORITHM SettleSingleBill(record)
    INPUT: record (raw bill data)

    BEGIN
        TRY
            billPtr <- CALL CreateValidatedBill(record)    // Managed by RAII smart pointer
            covered <- CALL AdjudicateClaim(billPtr)
            CALL SaveReceipt(billPtr, covered)
        CATCH ClaimError AS e
            CALL WriteAuditLog("CLAIM_REJECTED", record.id, e.what())
            RETHROW    // 'throw;' preserves exact derived exception type
        END TRY
    END
END ALGORITHM
```

---

## A5.2 – No Leaks on Errors: Stack Unwinding & RAII (2 Marks)
* **Stack Unwinding:** When an exception is thrown, C++ halts normal sequential execution and unwinds the call stack frame by frame until it reaches a matching `catch` handler. As each stack frame exits, C++ guarantees that the destructors of all fully constructed automatic (stack-allocated) local objects in that scope are invoked in **reverse order of construction**.
* **How RAII Prevents Leaks:** By wrapping every dynamic resource inside a stack-allocated RAII owner—such as `std::unique_ptr<Bill>` for heap memory, `std::vector<std::unique_ptr<Bill>>` for collections, and `std::ofstream` for audit/receipt files—stack unwinding automatically triggers `~unique_ptr()` (calling `delete`) and `~ofstream()` (flushing and closing the file handle) even when an exception interrupts execution midway through a function.

---

## Predict the Output & Verification

### 1. Predicted Output (Written Before Running)
```text
~f ~main caught 42
```

### 2. Actual Output & Explanation
* **Actual Output:**
  ```text
  ~f ~main caught 42
  ```
* **Destructor Execution Order:**
  1. In `main()`, `Guard m("main")` is constructed first inside the `try` block.
  2. `main()` calls `f()`, where `Guard g("f")` is constructed second.
  3. `f()` executes `throw 42;`. Stack unwinding immediately begins in reverse order of construction:
     * `f()`'s frame unwinds first, calling `g.~Guard()` and printing **`~f `**.
     * Next, `main()`'s `try` block exits, calling `m.~Guard()` and printing **`~main `**.
  4. Control enters `catch (int x)` and prints **`caught 42`**.
* **Why `"not printed "` Never Appears:**
  Once `throw 42;` executes inside `f()`, normal control flow never returns to the line after `f();` in `main()`. Execution jumps directly through stack unwinding to the matching `catch (int x)` handler, bypassing `std::cout << "not printed ";`.

---

## Alternative Concept and Complexity Analysis

### 1. Alternative Design (Manual `delete` in Every `catch` Block Without RAII)
If raw pointers (`Bill* b = new OutPatientBill(...)`) were used instead of RAII (`std::unique_ptr<Bill>`), the programmer would have to manually call `delete` on both the normal path and inside every `catch` block before rethrowing:

```text
ALGORITHM SettleManualMemory(record)
    BEGIN
        rawBill <- NEW Bill(record)
        TRY
            covered <- CALL AdjudicateClaim(rawBill)
            CALL SaveReceipt(rawBill, covered)
            DELETE rawBill                    // Leak Point 1: forgotten on early return
        CATCH ClaimError AS e
            CALL WriteAuditLog(e.what())
            DELETE rawBill                    // Leak Point 2: duplicated cleanup
            RETHROW
        CATCH ...
            DELETE rawBill                    // Leak Point 3: duplicated cleanup
            RETHROW
        END TRY
    END
END ALGORITHM
```

### 2. Time, Code Size & Leak-Point Comparison

| Metric | RAII (`std::unique_ptr` / `std::vector`) | Manual `delete` in `catch` Blocks |
| :--- | :--- | :--- |
| **Time Complexity** | **\(O(1)\) cleanup** — Zero runtime overhead over raw pointers; destructor runs automatically on scope exit. | **\(O(1)\) cleanup** — Same runtime deletion cost, provided every path actually executes `delete`. |
| **Code Size & Duplication** | **Minimal & DRY** — Cleanup logic is written **once** inside the RAII destructor; `try/catch` blocks are only needed where errors are actually logged or handled. | **Bloated (`O(P × R)` cleanup statements)** — For \(P\) exit/catch paths and \(R\) allocated resources, cleanup code must be duplicated across every single `catch` block and early return. |
| **Leak Points When Adding a New Error Path** | **0 leak points** — Adding a new `throw` statement or a new `catch` block automatically invokes all RAII destructors during stack unwinding. | **\(N\) leak points (1 per active resource)** — If a new exception is thrown before the `try` block, or a new `catch` handler forgets `delete ptr;` before rethrowing, every active heap allocation leaks permanently. |
