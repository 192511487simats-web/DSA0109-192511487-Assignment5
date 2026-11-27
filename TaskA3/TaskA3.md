# Task A3 – Design with Abstract Classes (5 Marks)

## A3.1 – Payer Interface & Derived Classes (3 Marks)
To decouple **how a bill is calculated** (`Bill`) from **who pays for it** (`Payer`), we define an abstract base class `Payer` that accepts any `Bill` by constant reference (`const Bill&`) and computes the covered amount polymorphically.

### 1. Abstract Base Class: `Payer`
* **Protected Data:**
  * `payerName` (`String`): Name of the payer entity (e.g., `"Self"`, `"Star Health"`, `"CMCHIS"`).
* **Public Operations:**
  * `virtual ~Payer() = default` — Virtual destructor for safe polymorphic cleanup.
  * `virtual amountCovered(bill : const Bill&) : Real = 0` *(pure virtual)* — Calculates how much of `bill.total()` this payer covers.
  * `patientBalance(bill : const Bill&) : Real` *(concrete helper)* — Returns `bill.total() - amountCovered(bill)`.

### 2. Concrete Derived Classes
1. **`SelfPay`:**
   * **Data:** None (or `payerName = "SelfPay"`).
   * **Behavior:** `amountCovered(bill)` always returns `0.0` (the patient pays 100% of `bill.total()`).
2. **`Insurance`:**
   * **Data:** `coverLimit` (`Real`), `coPayPercent` (`Real`, e.g., `10.0` for 10% co-payment).
   * **Behavior:** Computes `eligibleAmount = bill.total() * (1.0 - coPayPercent / 100.0)` and returns `MIN(eligibleAmount, coverLimit)`.
3. **`GovernmentScheme`:**
   * **Data:** `packageRates` (`Map<String, Real>` mapping each `bill.type()` to a fixed government-approved rate).
   * **Behavior:** Looks up `bill.type()` in `packageRates`. If found, returns `MIN(bill.total(), fixedRate)`; if the bill type is not covered by the scheme, raises an `UnknownPackageError`.

### 3. SIMATS Pseudocode for `Insurance::amountCovered`
```text
ALGORITHM InsuranceAmountCovered(bill, coverLimit, coPayPercent)
    INPUT: bill (constant reference to Bill), coverLimit (real), coPayPercent (real)
    OUTPUT: coveredAmount (real)

    BEGIN
        totalBill <- CALL bill.total()
        IF totalBill < 0.0 THEN
            THROW NegativeChargeError("Bill total cannot be negative")
        END IF

        eligibleAmount <- totalBill * (1.0 - (coPayPercent / 100.0))

        IF eligibleAmount > coverLimit THEN
            coveredAmount <- coverLimit
        ELSE
            coveredAmount <- eligibleAmount
        END IF

        RETURN coveredAmount
    END
END ALGORITHM
```

---

## A3.2 – Extension: Adding a New Payer Without Modifying Existing Code (2 Marks)
Suppose the hospital signs a **Corporate Tie-Up** agreement where corporate employees receive an 80% discount up to a ₹25,000 cap, with 100% coverage on `OutPatient` consultations.

* **How It Is Added:**
  We create a brand-new derived class **`CorporateTieUp`** inheriting from `Payer` in its own header/source file (`CorporateTieUp.h` / `CorporateTieUp.cpp`), overriding `amountCovered(const Bill& bill)`.
* **Why Zero Existing Code Changes:**
  1. `Bill` and its subclasses (`OutPatientBill`, `InPatientBill`, etc.) have **no dependency** on `Payer`.
  2. `Payer`, `SelfPay`, `Insurance`, and `GovernmentScheme` remain untouched and do not even need recompilation.
  3. The settlement engine (`settleBill(const Bill& b, const Payer& p)`) operates solely on the abstract `Payer&` reference, adhering strictly to the **Open-Closed Principle**.

### SIMATS Pseudocode for `CorporateTieUp` Extension
```text
ALGORITHM CorporateTieUpAmountCovered(bill, corporateCap, coverageRatio)
    INPUT: bill (constant reference to Bill), corporateCap (real), coverageRatio (real)
    OUTPUT: coveredAmount (real)

    BEGIN
        totalBill <- CALL bill.total()
        IF CALL bill.type() == "OutPatient" THEN
            coveredAmount <- MIN(totalBill, corporateCap)
        ELSE
            coveredAmount <- MIN(totalBill * coverageRatio, corporateCap)
        END IF
        RETURN coveredAmount
    END
END ALGORITHM
```

---

## Alternative Concept and Complexity Analysis

### 1. Alternative Design (`std::function<double(const Bill&)>` Policy Callables)
Instead of an abstract `Payer` class hierarchy, each payer could be represented at runtime by a `std::function<double(const Bill&)>` object holding a lambda or functor:

```text
ALGORITHM SettleWithPolicyFunction(bill, payerPolicyFn)
    INPUT: bill (constant reference to Bill), payerPolicyFn (callable function object)
    OUTPUT: coveredAmount (real)

    BEGIN
        coveredAmount <- CALL payerPolicyFn(bill)
        RETURN coveredAmount
    END
END ALGORITHM
```

### 2. Time, Space & State Storage Comparison

| Metric | Abstract `Payer` Class Hierarchy | `std::function<double(const Bill&)>` |
| :--- | :--- | :--- |
| **Time per Call** | **\(O(1)\)** — Single indirect `vtable` call (`1` pointer dereference). | **\(O(1)\)** — Single indirect call through `std::function`'s internal type-erased manager/invoker pointer. |
| **Space per Payer** | **Exact member size + 1 `vptr` (8 bytes)** — Predictable stack or heap footprint with zero hidden allocations. | **Fixed wrapper size (typically 32 bytes on 64-bit GCC/Clang) + potential heap allocation** if captured state exceeds Small Buffer Optimization (SBO, usually 16 bytes). |
| **How State Is Stored (e.g., `coverLimit`, `packageRates`)** | Stored explicitly as **named, typed class member variables** (`coverLimit`, `coPayPercent`) with clear invariants enforced in constructors. | Stored implicitly inside **lambda capture closures** (`[coverLimit, coPayPercent](const Bill& b) { ... }`). Large state (like a `GovernmentScheme` rate table map) forces dynamic heap allocation inside `std::function` or requires capturing a `std::shared_ptr`. |
| **Extensibility & Clarity** | Best when payers have **multiple related methods** (e.g., `amountCovered()`, `payerName()`, `requiresPreAuth()`) and structured state. | Best for **single-method stateless or small-state rules**, but becomes unwieldy if multiple behaviors are needed per payer. |
