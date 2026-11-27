#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <variant>
#include <vector>

using Clock = std::chrono::high_resolution_clock;

// ============================================================================
// 1. VIRTUAL POLYMORPHIC HIERARCHY
// ============================================================================
struct VBill {
    virtual ~VBill() = default;
    virtual double total() const = 0;
};

struct VOutPatient : VBill {
    double consult, tests;
    VOutPatient(double c, double t) : consult(c), tests(t) {}
    double total() const override { return consult + tests; }
};

struct VInPatient : VBill {
    int days;
    double rate, surgery;
    VInPatient(int d, double r, double s) : days(d), rate(r), surgery(s) {}
    double total() const override { return days * rate + surgery; }
};

struct VDayCare : VBill {
    double pkg;
    explicit VDayCare(double p) : pkg(p) {}
    double total() const override { return pkg; }
};

struct VEmergency : VBill {
    double base, hrs, rate;
    VEmergency(double b, double h, double r) : base(b), hrs(h), rate(r) {}
    double total() const override { return base + hrs * rate; }
};

// ============================================================================
// 2. SINGLE STRUCT + ENUM + SWITCH
// ============================================================================
enum class BillTag { OP, IP, DC, EM };

struct SBill {
    BillTag tag;
    double a = 0, b = 0, c = 0;

    double total() const noexcept {
        switch (tag) {
            case BillTag::OP: return a + b;
            case BillTag::IP: return a * b + c;
            case BillTag::DC: return a;
            case BillTag::EM: return a + b * c;
        }
        return 0.0;
    }
};

// ============================================================================
// 3. STD::VARIANT + STD::VISIT (ALTERNATIVE CONCEPT)
// ============================================================================
struct VarOP { double consult, tests; };
struct VarIP { int days; double rate, surgery; };
struct VarDC { double pkg; };
struct VarEM { double base, hrs, rate; };

using VarBill = std::variant<VarOP, VarIP, VarDC, VarEM>;

double variantTotal(const VarBill& vb) {
    return std::visit([](const auto& b) -> double {
        using T = std::decay_t<decltype(b)>;
        if constexpr (std::is_same_v<T, VarOP>) return b.consult + b.tests;
        else if constexpr (std::is_same_v<T, VarIP>) return b.days * b.rate + b.surgery;
        else if constexpr (std::is_same_v<T, VarDC>) return b.pkg;
        else return b.base + b.hrs * b.rate;
    }, vb);
}

// ============================================================================
// 4. VALIDATION: RETURN CODES vs. EXCEPTIONS (B3.3)
// ============================================================================
enum class ValStatus { OK, INVALID_AMOUNT };

ValStatus validateReturnCode(double amount, double& outValid) noexcept {
    if (amount < 0.0 || amount > 1e6) return ValStatus::INVALID_AMOUNT;
    outValid = amount;
    return ValStatus::OK;
}

double validateException(double amount) {
    if (amount < 0.0 || amount > 1e6) throw std::invalid_argument("invalid");
    return amount;
}

// Prevent compiler from optimizing away benchmark loops
volatile double g_sink = 0.0;

template <typename Fn>
double measureMs(Fn&& fn) {
    auto t0 = Clock::now();
    fn();
    auto t1 = Clock::now();
    return std::chrono::duration<double, std::milli>(t1 - t0).count();
}

int main() {
    std::cout << std::fixed << std::setprecision(4);

    // ------------------------------------------------------------------------
    // B3.1: Correctness Check on 10 Bills
    // ------------------------------------------------------------------------
    std::vector<std::unique_ptr<VBill>> v10;
    std::vector<SBill> s10;
    std::vector<VarBill> var10;

    for (int i = 0; i < 10; ++i) {
        int kind = i % 4;
        if (kind == 0) {
            v10.push_back(std::make_unique<VOutPatient>(500.0, 1200.0));
            s10.push_back({BillTag::OP, 500.0, 1200.0, 0.0});
            var10.emplace_back(VarOP{500.0, 1200.0});
        } else if (kind == 1) {
            v10.push_back(std::make_unique<VInPatient>(3, 2500.0, 40000.0));
            s10.push_back({BillTag::IP, 3.0, 2500.0, 40000.0});
            var10.emplace_back(VarIP{3, 2500.0, 40000.0});
        } else if (kind == 2) {
            v10.push_back(std::make_unique<VDayCare>(18000.0));
            s10.push_back({BillTag::DC, 18000.0, 0.0, 0.0});
            var10.emplace_back(VarDC{18000.0});
        } else {
            v10.push_back(std::make_unique<VEmergency>(2000.0, 4.0, 1500.0));
            s10.push_back({BillTag::EM, 2000.0, 4.0, 1500.0});
            var10.emplace_back(VarEM{2000.0, 4.0, 1500.0});
        }
    }

    double sumV = 0, sumS = 0, sumVar = 0;
    for (const auto& b : v10) sumV += b->total();
    for (const auto& b : s10) sumS += b.total();
    for (const auto& b : var10) sumVar += variantTotal(b);

    std::cout << "=== B3.1: Correctness Verification (10 Bills) ===\n";
    std::cout << "Virtual Total : " << sumV << '\n';
    std::cout << "Switch Total  : " << sumS << '\n';
    std::cout << "Variant Total : " << sumVar << '\n';
    std::cout << "Match Status  : "
              << ((std::fabs(sumV - sumS) < 1e-6 && std::fabs(sumV - sumVar) < 1e-6) ? "EXACT MATCH" : "MISMATCH")
              << "\n\n";

    // ------------------------------------------------------------------------
    // B3.2 & Alternative: Dispatch Scaling (1K, 1M, 10M Bills)
    // ------------------------------------------------------------------------
    std::cout << "=== B3.2: Dispatch Benchmark (-O2) ===\n";
    std::cout << "sizeof(VOutPatient)=" << sizeof(VOutPatient)
              << "B, sizeof(VInPatient)=" << sizeof(VInPatient)
              << "B, sizeof(SBill)=" << sizeof(SBill)
              << "B, sizeof(VarBill)=" << sizeof(VarBill) << "B\n\n";

    std::cout << std::left << std::setw(14) << "Bills (N)"
              << std::setw(18) << "Virtual (ms)"
              << std::setw(18) << "Switch (ms)"
              << std::setw(18) << "Variant (ms)" << '\n';
    std::cout << std::string(68, '-') << '\n';

    for (std::size_t n : {1000ULL, 1000000ULL, 10000000ULL}) {
        std::vector<std::unique_ptr<VBill>> vBills;
        std::vector<SBill> sBills;
        std::vector<VarBill> varBills;
        vBills.reserve(n);
        sBills.reserve(n);
        varBills.reserve(n);

        for (std::size_t i = 0; i < n; ++i) {
            int kind = i % 4;
            if (kind == 0) {
                vBills.push_back(std::make_unique<VOutPatient>(500.0, 1200.0));
                sBills.push_back({BillTag::OP, 500.0, 1200.0, 0.0});
                varBills.emplace_back(VarOP{500.0, 1200.0});
            } else if (kind == 1) {
                vBills.push_back(std::make_unique<VInPatient>(3, 2500.0, 40000.0));
                sBills.push_back({BillTag::IP, 3.0, 2500.0, 40000.0});
                varBills.emplace_back(VarIP{3, 2500.0, 40000.0});
            } else if (kind == 2) {
                vBills.push_back(std::make_unique<VDayCare>(18000.0));
                sBills.push_back({BillTag::DC, 18000.0, 0.0, 0.0});
                varBills.emplace_back(VarDC{18000.0});
            } else {
                vBills.push_back(std::make_unique<VEmergency>(2000.0, 4.0, 1500.0));
                sBills.push_back({BillTag::EM, 2000.0, 4.0, 1500.0});
                varBills.emplace_back(VarEM{2000.0, 4.0, 1500.0});
            }
        }

        double tVirt = measureMs([&]() {
            double acc = 0.0;
            for (const auto& b : vBills) acc += b->total();
            g_sink = acc;
        });

        double tSwitch = measureMs([&]() {
            double acc = 0.0;
            for (const auto& b : sBills) acc += b.total();
            g_sink = acc;
        });

        double tVar = measureMs([&]() {
            double acc = 0.0;
            for (const auto& b : varBills) acc += variantTotal(b);
            g_sink = acc;
        });

        std::cout << std::left << std::setw(14) << n
                  << std::setw(18) << tVirt
                  << std::setw(18) << tSwitch
                  << std::setw(18) << tVar << '\n';
    }

    // ------------------------------------------------------------------------
    // B3.3: Cost of Exceptions vs. Return Codes (1,000,000 Bills)
    // ------------------------------------------------------------------------
    std::cout << "\n=== B3.3: Validation Cost (1,000,000 Bills, -O2) ===\n";
    std::cout << std::left << std::setw(18) << "Invalid Rate"
              << std::setw(22) << "Return Code (ms)"
              << std::setw(22) << "Exceptions (ms)" << '\n';
    std::cout << std::string(62, '-') << '\n';

    const std::size_t M = 1000000ULL;
    for (int invalidPct : {0, 1, 50}) {
        std::vector<double> inputs(M, 1500.0);
        if (invalidPct > 0) {
            int step = 100 / invalidPct;
            for (std::size_t i = 0; i < M; i += step) {
                inputs[i] = -100.0;
            }
        }

        double tRet = measureMs([&]() {
            double acc = 0.0;
            std::size_t errs = 0;
            for (double x : inputs) {
                double validVal = 0.0;
                if (validateReturnCode(x, validVal) == ValStatus::OK) {
                    acc += validVal;
                } else {
                    ++errs;
                }
            }
            g_sink = acc + static_cast<double>(errs);
        });

        double tExc = measureMs([&]() {
            double acc = 0.0;
            std::size_t errs = 0;
            for (double x : inputs) {
                try {
                    acc += validateException(x);
                } catch (const std::invalid_argument&) {
                    ++errs;
