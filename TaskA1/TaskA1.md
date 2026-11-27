# Task A1 – Design the Polymorphic Bill Hierarchy (7 Marks)

## A1.1 – Identify the Common Interface (2 Marks)
Every hospital bill shares core identifying attributes and must expose a uniform set of operations so the billing engine can process any bill without knowing its concrete type.

* **Common Data (Protected Attributes in `Bill`):**
  * `billId` (`String`): Unique identifier for the bill.
  * `patientName` (`String`): Name of the patient billed.
  * `billDate` (`String`): Date on which the bill was raised.
* **Common Operations (Public Interface of `Bill`):**
  * `total() : Real` *(pure virtual)* — Computes and returns the total payable amount for that specific bill category.
  * `type() : String` *(pure virtual)* — Returns the category label of the bill (e.g., `"OutPatient"`, `"InPatient"`, `"DayCareSurgery"`, `"Emergency"`).
  * `print() : Void` *(virtual / concrete helper)* — Displays formatted bill details by invoking `type()` and `total()` polymorphically.
  * `~Bill()` *(virtual destructor)* — Ensures derived class resources are safely cleaned up when deleted through a `Bill*` pointer.

---

## A1.2 – Design the Hierarchy (3 Marks)
The UML class diagram for the polymorphic `Bill` hierarchy is provided in **`uml_bill_hierarchy.png`** in this folder.

* **Abstract Base Class:** `Bill` (cannot be instantiated directly because `total()` and `type()` are pure virtual `= 0`).
* **Concrete Derived Classes:**
  1. **`OutPatientBill`**: Stores `consultationFee` and `testCharges`. Overrides `total()` (`consultationFee + testCharges`) and `type()`.
  2. **`InPatientBill`**: Stores `days`, `roomRatePerDay`, `procedureCharges`, and `medicineCharges`. Overrides `total()` (`(days × roomRatePerDay) + procedureCharges + medicineCharges`) and `type()`.
  3. **`DayCareSurgeryBill`**: Stores `packagePrice`. Overrides `total()` (`packagePrice`) and `type()`.
  4. **`EmergencyBill`**: Stores `baseCharge`, `hours`, and `hourlyRate`. Overrides `total()` (`baseCharge + (hours × hourlyRate)`) and `type()`.

---

## A1.3 – Process All Bills Together (2 Marks)
*(Written strictly following the SIMATS Engineering – Standard for Algorithmic Pseudocode)*

```text
ALGORITHM ComputeDailyTotal(bills, count)
    INPUT: bills (array of base-class pointers to Bill), count (integer number of bills)
    OUTPUT: grandTotal (real value representing total revenue for the day)

    BEGIN
        grandTotal <- 0.0
        FOR i <- 0 TO count - 1 DO
            IF bills[i] != NULL THEN
                CALL bills[i].print()
                grandTotal <- grandTotal + bills[i].total()
            END IF
        END FOR
        RETURN grandTotal
    END
END ALGORITHM
```

---

## Alternative Concept and Complexity Analysis

### 1. Alternative Design (`struct Bill` with `type` Tag and `switch`)
Instead of an abstract base class with virtual functions, all bills could be stored in a single `struct Bill` containing an `enum BillType` tag and member fields for every possible bill category, using a `switch` statement inside `total()`:

```text
ALGORITHM ComputeTotalSwitch(bills, count)
    INPUT: bills (array of Bill structures), count (integer)
    OUTPUT: grandTotal (real)

    BEGIN
        grandTotal <- 0.0
        FOR i <- 0 TO count - 1 DO
            SWITCH (bills[i].billType)
                CASE OUT_PATIENT:
                    grandTotal <- grandTotal + bills[i].consultationFee + bills[i].testCharges
                CASE IN_PATIENT:
                    grandTotal <- grandTotal + (bills[i].days * bills[i].roomRatePerDay) + bills[i].procedureCharges + bills[i].medicineCharges
                CASE DAY_CARE:
                    grandTotal <- grandTotal + bills[i].packagePrice
                CASE EMERGENCY:
                    grandTotal <- grandTotal + bills[i].baseCharge + (bills[i].hours * bills[i].hourlyRate)
            END SWITCH
        END FOR
        RETURN grandTotal
    END
END ALGORITHM
```

### 2. Time & Space Complexity Comparison

| Metric | Polymorphic Hierarchy (`virtual` functions) | Single `struct Bill` with `switch` |
| :--- | :--- | :--- |
| **Time Complexity (\(n\) bills)** | **\(O(n)\)** — Performs one indirect `vtable` lookup per bill (\(O(1)\) per call). | **\(O(n)\)** — Performs one branch/jump-table dispatch per bill (\(O(1)\) per call). |
| **Space per Bill** | **Exact size + 1 `vptr` (8 bytes on 64-bit)** — Each derived object allocates only the fields it actually uses. | **Max-Union Size + Tag (4 bytes)** — Every bill wastes memory holding unused fields (e.g., an `OutPatient` bill still carries `days`, `roomRate`, and `surgery` fields). |
| **Adding a New Bill Type** | **Zero changes to existing code** — Add a new derived class (`Open-Closed Principle`). | **High modification risk** — Must edit the `Bill` struct, `enum BillType`, and every `switch` statement across the entire codebase. |
