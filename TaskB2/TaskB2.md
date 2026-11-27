# Task B2 – Engineer Error Conditions (5 Marks)

## 1. Predict the Output (`predict_b2.cpp`)
* **Predicted Output (Written Before Running):**
  ```text
  ok done caught negative [log too large] caught too large 
  ```
* **Actual Output:**
  ```text
  ok done caught negative [log too large] caught too large 
  ```
* **Handler Explanation:**
  1. **`a = 100.0`:** `validate(100.0)` prints `ok `, returns normally to `process()`, and `main()` prints `done `.
  2. **`a = -5.0`:** `validate(-5.0)` throws `std::invalid_argument("negative")`. `process()` only has a `catch (const std::out_of_range&)` handler, which does not match `std::invalid_argument`. Therefore, `process()` does **not** log it; the exception propagates directly to `main()`'s `catch (const std::exception&)`, printing `caught negative `.
  3. **`a = 2e6`:** `validate(2e6)` throws `std::out_of_range("too large")`. `process()` catches `const std::out_of_range&`, prints `[log too large] `, and rethrows it via `throw;` to `main()`, which prints `caught too large `.

---

## 2. Complete the Code Analysis (`complete_b2.cpp`)
* **Difference Between `throw;` and `throw e;`:**
  * `throw;` rethrows the **exact active exception object** currently being handled without copying or altering its dynamic type.
  * `throw e;` constructs a **new copy** of `e` using `e`'s **static type** in the `catch` clause. If `settle()` caught a base class (e.g., `catch (const std::runtime_error& e)`) when a derived `ClaimError` was thrown, `throw e;` would **slice off** the `ClaimError` derived type—causing `main()`'s `catch (const ClaimError&)` to miss it and fall into the generic `catch (const std::exception&)` block instead.

---

## 3. Engineered Error Conditions (`error_tests.cpp`)
* **What Each Test Reveals:**
  1. **Negative Charge & Claim Above Cover:** Domain exceptions carry rich diagnostic context (`what()`) cleanly separating validation rules from normal return values.
  2. **Unknown Package & `std::stod` Failure:** Standard library exceptions (`std::invalid_argument`) integrate seamlessly alongside custom exceptions under a unified `std::exception` hierarchy.
  3. **`std::bad_alloc`:** Catching `const std::bad_alloc&` allows graceful degradation instead of an abrupt `SIGABRT` crash on allocation spikes.
  4. **RAII Destructor Logs (`[Destroyed]`):** Every stack-allocated `TrackedResource` is deterministically destroyed in reverse order during stack unwinding before any `catch` block runs, guaranteeing zero leaks.
  5. **Top-Level `catch(...)`:** Guarantees batch survival even if third-party code throws a non-`std::exception` type (like `throw 999;`).

---

## 4. Alternative Concept and Complexity Analysis (`std::optional<double>`)

### 1. Alternative Design (`std::optional<double>` Instead of Exceptions)
Instead of throwing exceptions when a claim is invalid, `approve()` returns `std::nullopt`:
```cpp
std::optional<double> approveOpt(double billed, double cover) noexcept {
    if (billed < 0.0 || cover < 0.0 || billed > cover) return std::nullopt;
    return billed;
}
```

### 2. Time & Space Comparison Across 1,000,000 Claims

| Metric / Scenario | Exceptions (`try / catch / throw`) | `std::optional<double>` (No Exceptions) |
| :--- | :--- | :--- |
| **0% Invalid Claims (1M valid)** | **~1.5 – 3.0 ms** — Zero-cost `.eh_frame` tables execute zero extra instructions on the happy path. | **~1.5 – 3.0 ms** — Slightly faster or equal (`has_value()` check is a single register comparison). |
| **1% Invalid Claims (10,000 throws)** | **~15 – 25 ms** — Each of the 10,000 throws incurs ~1.5–2 µs of stack unwinding and `.eh_frame` lookup overhead. | **~1.5 – 3.0 ms** — Returning `std::nullopt` takes nanoseconds with zero stack unwinding. |
| **50% Invalid Claims (500,000 throws)** | **~750 – 1,200 ms (Severe Degradation)** — Half a million stack unwinds dominate total runtime (~300× slower). | **~1.5 – 3.0 ms (Constant Performance)** — Unaffected by failure rate. |
| **Space per Result** | **8 bytes (`sizeof(double)`)** — No wrapper flag needed in the return register. | **16 bytes (`sizeof(std::optional<double>)`)** — 8 bytes for `double` + 1 byte `bool` flag + 7 bytes alignment padding (doubles return-value size), and **loses the specific error reason** (cannot distinguish negative charge vs. cover exceeded). |
