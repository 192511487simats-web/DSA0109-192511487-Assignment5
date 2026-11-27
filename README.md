# DSA01 – Assignment 5 (CO5): Reliable Software with Runtime Polymorphism and Exceptions

## Student Details
* **Name:** [DHINESH KUMAR G]
* **Register Number:** [192511487]
* **Course Outcome:** CO5 – Build robust C++ applications using virtual functions, runtime polymorphism, and exception handling
* **Repository Name:** `DSA01-[192511487]-Assignment5`

---

## Compiler & Environment Information
* **Compiler:** `g++` (GCC 13.x / check with `g++ --version`)
* **C++ Standard:** C++17 (`-std=c++17`)
* **Operating System:** [Ubuntu Linux / Windows WSL / macOS]

---

## Compilation & Execution Commands

### 1. Standard Build with Warnings & AddressSanitizer (Tasks A2, A5, B1, B2, B4)
```bash
g++ -std=c++17 -Wall -Wextra -g -fsanitize=address main.cpp -o program
./program
```

### 2. Optimized Build for Performance Benchmarking (Task B3)
```bash
g++ -std=c++17 -O2 benchmark.cpp -o benchmark
./benchmark
```

---

## Repository Structure & Task Summaries

| Folder | Task | Description & Result Summary |
| :--- | :--- | :--- |
| `TaskA1/` | **Polymorphic Bill Hierarchy Design** | Identified common `Bill` interface, designed abstract base class and 4 derived bill types (`OutPatient`, `InPatient`, `DayCareSurgery`, `Emergency`) with UML diagrams, SIMATS pseudocode, and `switch`-based complexity analysis. |
| `TaskA2/` | **Runtime Binding & Virtual Tables** | Explained early vs. late binding, illustrated `vtable` layouts, verified virtual destructor behavior, and tested output prediction with and without `-fsanitize=address`. |
| `TaskA3/` | **Abstract Payer Design & Extension** | Designed abstract `Payer` interface with `SelfPay`, `Insurance`, and `GovernmentScheme`, demonstrated zero-modification extension with `CorporateTieUp`, and compared with `std::function`. |
| `TaskA4/` | **Exception Strategy & Specifications** | Categorized domain errors into a custom `std::runtime_error` hierarchy, applied `noexcept` specifications to destructors/getters, and compared against error return codes. |
| `TaskA5/` | **Recovery, Rethrowing & RAII** | Designed multi-layer `catch` and `throw;` audit logging, verified reverse-order stack unwinding via `Guard` destructors, and analyzed RAII vs. manual `delete` in `catch` blocks. |
| `TaskB1/` | **Implement & Validate Billing** | Implemented the polymorphic `Bill` and `Payer` hierarchies, completed `TODO 1–3`, verified output totals (`49200`), and proved compiler/ASan errors when removing `virtual`. |
| `TaskB2/` | **Engineer Error Conditions** | Tested negative charges, cover limit overflows, invalid `std::stod` inputs, and `std::bad_alloc`; completed `approve()`/`settle()` TODOs and demonstrated slicing differences between `throw;` and `throw e;`. |
| `TaskB3/` | **Virtual vs. `switch` vs. Exceptions** | Benchmarked \(10^3\), \(10^6\), and \(10^7\) bills under `-O2` across virtual dispatch, `switch`, and `std::variant`; measured exception vs. return-code overhead at 0%, 1%, and 50% error rates. |
| `TaskB4/` | **Final Engineering Decision** | Extended the system with a new bill type and payer without altering existing classes, verified zero memory leaks via AddressSanitizer, and justified production suitability for 15 hospitals. |

---

## Verification & Submission Checklist
- [x] Every C++ program compiled cleanly with `-Wall -Wextra` and verified under `-fsanitize=address`.
- [x] Output predictions written prior to program execution and differences explained.
- [x] Separate folders maintained for each task containing source files, input files, and output screenshots.
- [x] Progressive Git commits recorded (at least one commit per task).
