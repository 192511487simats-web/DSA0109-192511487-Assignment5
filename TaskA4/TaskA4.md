# Task A4 – Design the Exception Strategy (4 Marks)

## A4.1 – Identify the Errors & Design the Exception Hierarchy (2 Marks)

### 1. Domain & System Error Catalog
1. **Negative Charges (`NegativeChargeError`):** Consultation fee, room rate, days, surgery price, or cover limit is `< 0`.
2. **Claim Above Cover (`ClaimAboveCoverError`):** Claimed or billed amount exceeds the policy's `coverLimit` or scheme ceiling.
3. **Unknown Package (`UnknownPackageError`):** A `GovernmentScheme` or `DayCareSurgeryBill` references an unrecognized package code.
4. **Missing Patient Record (`MissingPatientError`):** Patient ID is empty, unregistered, or cannot be located in the hospital database.
5. **Invalid Input Format (`std::invalid_argument` / `ParseError`):** Non-numeric text encountered when parsing monetary values (e.g., via `std::stod`).
6. **Memory Allocation Failure (`std::bad_alloc`):** Heap exhaustion when allocating large bill batches or objects (`new` / `std::vector`).

### 2. Exception Class Hierarchy (Derived from `std::exception`)
All domain-specific exceptions inherit from `std::runtime_error` (which derives from `std::exception` and safely manages the `what()` error message string without throwing during copy).

```text
std::exception
 ├── std::bad_alloc                  (Standard C++ heap exhaustion)
 ├── std::logic_error
 │    └── std::invalid_argument      (Invalid numeric string parsing in std::stod)
 └── std::runtime_error
      └── HospitalError              (Base class for all hospital billing/claim errors)
           ├── BillingError          (Errors during bill construction or lookup)
           │    ├── NegativeChargeError
           │    ├── UnknownPackageError
           │    └── MissingPatientError
           └── ClaimError            (Errors during payer adjudication & settlement)
                └── ClaimAboveCoverError
```

### 3. SIMATS Pseudocode for Validated Bill Construction & Claim Settlement
```text
ALGORITHM ValidateAndCreateOutPatientBill(patientId, consultationFee, testCharges)
    INPUT: patientId (string), consultationFee (real), testCharges (real)
    OUTPUT: billObject (OutPatientBill)

    BEGIN
        IF patientId == "" THEN
            THROW MissingPatientError("Patient ID cannot be empty")
        END IF

        IF consultationFee < 0.0 OR testCharges < 0.0 THEN
            THROW NegativeChargeError("Consultation and test charges must be non-negative")
        END IF

        CREATE billObject WITH (patientId, consultationFee, testCharges)
        RETURN billObject
    END
END ALGORITHM
```

---

## A4.2 – Specify Exceptions (`noexcept` vs. Throwing) (2 Marks)

### 1. Exception Specification Classification

| Function / Operation | Specification | Justification |
| :--- | :--- | :--- |
| `Bill` / `Payer` Constructors | **May throw** | Must validate invariants (e.g., non-negative charges, valid patient ID) and throw `BillingError` if invalid so partially valid objects never exist. |
| `Payer::amountCovered(const Bill&)` | **May throw** | `GovernmentScheme` may throw `UnknownPackageError`; `Insurance` may throw `ClaimError` if cover rules are violated. |
| `settleClaim(...)` / `parseBillLine(...)` | **May throw** | Propagates domain exceptions (`ClaimError`) and parsing failures (`std::invalid_argument`) to caller layers. |
| `~Bill()` / `~Payer()` (Destructors) | **`noexcept`** | Implicitly `noexcept` in C++11/17; cleanup must never fail. |
| `Bill::type() const noexcept` | **`noexcept`** | Returns a fixed category identifier without allocating memory or failing. |
| `Bill::billId() const noexcept` | **`noexcept`** | Simple read-only getter returning a reference/view with zero failure modes. |
| Move Constructors / Move Assignment | **`noexcept`** | Allows `std::vector<std::unique_ptr<Bill>>` to move elements safely during reallocation without risking data loss. |

### 2. Why Destructors Should Never Throw
* **Double-Exception Termination:** When an exception is thrown, C++ begins **stack unwinding**, invoking the destructors of all local objects between the `throw` site and the matching `catch` block. If a destructor throws a second exception while the first exception is still active, C++ cannot propagate two simultaneous exceptions and immediately calls **`std::terminate()`**, abruptly crashing the entire program.
* **Resource Leaks:** If a destructor exits via an exception partway through its body, remaining cleanup steps (freeing memory, closing file handles, releasing locks) inside that destructor are skipped, guaranteeing resource leaks.

---

## Alternative Concept and Complexity Analysis

### 1. Alternative Design (Error Return Codes via `enum class Status`)
Instead of exceptions, every function could return an `enum class Status { OK, NEGATIVE_CHARGE, ABOVE_COVER, UNKNOWN_PACKAGE, MISSING_PATIENT }` and pass computed amounts back through out-parameters:

```text
ALGORITHM SettleWithReturnCode(bill, payer, outCoveredAmount)
    INPUT: bill (Bill), payer (Payer)
    OUTPUT: status (Status enum), outCoveredAmount (real via reference)

    BEGIN
        status <- CALL ValidateBill(bill)
        IF status != OK THEN
            RETURN status
        END IF

        status <- CALL payer.computeCovered(bill, outCoveredAmount)
        IF status != OK THEN
            RETURN status
        END IF

        RETURN OK
    END
END ALGORITHM
```

### 2. Time, Space & Code Overhead Comparison

| Metric | C++ Exceptions (`try / catch / throw`) | Return Codes (`enum Status` + Out-Params) |
| :--- | :--- | :--- |
| **Time on Normal (Happy) Path** | **\(O(1)\) — Zero-cost overhead:** Modern 64-bit compilers (Itanium ABI) use static `.eh_frame` lookup tables, executing **zero extra instructions** when no exception is thrown. | **\(O(1)\) — Explicit branching overhead:** Every intermediate function call must execute an explicit `if (status != OK) return status;` branch check on every call. |
| **Time on Error Path** | **High (\(O(d)\) stack depth):** Throwing unwinds \(d\) stack frames, looks up `.eh_frame` unwinding tables, and invokes destructors dynamically (microseconds per throw). | **Fast (\(O(1)\) per frame):** Simply returns an integer enum up the call stack via normal `ret` instructions (nanoseconds). |
| **Extra Space Needed** | **Zero per-call stack space:** Exception tables reside in read-only binary metadata (`.eh_frame`), not inside every object or return value. | **Extra return/output register or struct (`std::pair<Status, double>`)** passed across every function boundary in the call chain. |
| **Extra Code & Maintenance** | **Clean separation:** Business logic reads linearly without error-checking clutter; unhandled errors cannot be silently ignored. | **High boilerplate & bug risk:** Every caller at every layer must manually check and forward `Status`; forgetting one `if (status != OK)` silently corrupts billing totals. |
