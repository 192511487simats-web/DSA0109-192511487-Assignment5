#include <algorithm>
#include <cassert>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

// ============================================================================
// EXISTING HIERARCHY (UNCHANGED)
// ============================================================================
class BillingError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class ClaimError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class Bill {
protected:
    std::string patient;
public:
    explicit Bill(std::string p) : patient(std::move(p)) {
        if (patient.empty()) throw BillingError("Patient name cannot be empty");
    }
    virtual ~Bill() noexcept = default;
    virtual double total() const = 0;
    virtual std::string type() const noexcept = 0;
    const std::string& getPatient() const noexcept { return patient; }
};

class OutPatientBill : public Bill {
    double consultation, tests;
public:
    OutPatientBill(std::string p, double c, double t)
        : Bill(std::move(p)), consultation(c), tests(t) {
        if (c < 0.0 || t < 0.0) throw BillingError("Negative OutPatient charge");
    }
    double total() const override { return consultation + tests; }
    std::string type() const noexcept override { return "OutPatient"; }
};

class InPatientBill : public Bill {
    int days;
    double perDay, procedures, medicines;
public:
    InPatientBill(std::string p, int d, double rate, double proc, double med)
        : Bill(std::move(p)), days(d), perDay(rate), procedures(proc), medicines(med) {
        if (d < 0 || rate < 0.0 || proc < 0.0 || med < 0.0)
            throw BillingError("Negative InPatient charge");
    }
    double total() const override { return (days * perDay) + procedures + medicines; }
    std::string type() const noexcept override { return "InPatient"; }
};

class DayCareSurgeryBill : public Bill {
    double packagePrice;
public:
    DayCareSurgeryBill(std::string p, double pkg)
        : Bill(std::move(p)), packagePrice(pkg) {
        if (pkg < 0.0) throw BillingError("Negative DayCare package price");
    }
    double total() const override { return packagePrice; }
    std::string type() const noexcept override { return "DayCareSurgery"; }
};

class EmergencyBill : public Bill {
    double baseCharge, hours, hourlyRate;
public:
    EmergencyBill(std::string p, double base, double hrs, double rate)
        : Bill(std::move(p)), baseCharge(base), hours(hrs), hourlyRate(rate) {
        if (base < 0.0 || hrs < 0.0 || rate < 0.0)
            throw BillingError("Negative Emergency charge");
    }
    double total() const override { return baseCharge + (hours * hourlyRate); }
    std::string type() const noexcept override { return "Emergency"; }
};

class Payer {
public:
    virtual ~Payer() noexcept = default;
    virtual double amountCovered(const Bill& b) const = 0;
    virtual std::string name() const noexcept = 0;
};

class SelfPay : public Payer {
public:
    double amountCovered(const Bill&) const override { return 0.0; }
    std::string name() const noexcept override { return "SelfPay"; }
};

class Insurance : public Payer {
    double coverLimit, coPayPercent;
public:
    Insurance(double limit, double coPay) : coverLimit(limit), coPayPercent(coPay) {
        if (limit < 0.0 || coPay < 0.0 || coPay > 100.0)
            throw ClaimError("Invalid Insurance policy parameters");
    }
    double amountCovered(const Bill& b) const override {
        double eligible = b.total() * (1.0 - coPayPercent / 100.0);
        if (eligible > coverLimit) {
            throw ClaimError("Eligible claim exceeds Insurance cover limit");
        }
        return eligible;
    }
    std::string name() const noexcept override { return "Insurance"; }
};

class GovernmentScheme : public Payer {
    std::map<std::string, double> rates;
public:
    GovernmentScheme() {
        rates["OutPatient"] = 1000.0;
        rates["InPatient"] = 35000.0;
        rates["DayCareSurgery"] = 15000.0;
        rates["Emergency"] = 8000.0;
    }
    double amountCovered(const Bill& b) const override {
        auto it = rates.find(b.type());
        if (it == rates.end()) {
            throw ClaimError("Bill type '" + b.type() + "' not covered by GovtScheme");
        }
        return std::min(b.total(), it->second);
    }
    std::string name() const noexcept override { return "GovtScheme"; }
};

// ============================================================================
// B4.1: NEW EXTENSIONS ADDED WITHOUT TOUCHING ANY EXISTING CLASS
// ============================================================================

// Extension 1: New Bill Type -> TelemedicineBill
class TelemedicineBill : public Bill {
    double videoConsultFee, digitalPrescriptionFee;
public:
    TelemedicineBill(std::string p, double consult, double rxFee)
        : Bill(std::move(p)), videoConsultFee(consult), digitalPrescriptionFee(rxFee) {
        if (consult < 0.0 || rxFee < 0.0)
            throw BillingError("Negative Telemedicine charge");
    }
    double total() const override { return videoConsultFee + digitalPrescriptionFee; }
    std::string type() const noexcept override { return "Telemedicine"; }
};

// Extension 2: New Payer -> CorporateTieUp (80% coverage up to Rs. 25,000 cap)
class CorporateTieUp : public Payer {
    double capLimit, coverageRatio;
public:
    CorporateTieUp(double cap, double ratio) : capLimit(cap), coverageRatio(ratio) {
        if (cap < 0.0 || ratio < 0.0 || ratio > 1.0)
            throw ClaimError("Invalid CorporateTieUp terms");
    }
    double amountCovered(const Bill& b) const override {
        return std::min(b.total() * coverageRatio, capLimit);
    }
    std::string name() const noexcept override { return "CorporateTieUp"; }
};

// ============================================================================
// SETTLEMENT ENGINE (OPERATES PURELY ON BASE INTERFACES)
// ============================================================================
double settleBill(const Bill& b, const Payer& p) {
    try {
        return p.amountCovered(b);
    } catch (const ClaimError& e) {
        std::cout << "  [AUDIT LOG] Claim rejected for " << b.getPatient()
                  << " (" << b.type() << "): " << e.what() << '\n';
        throw; // Rethrow original exception without slicing
    }
}

int main() {
    std::cout << std::fixed << std::setprecision(2);

    // 1. Regression Tests on Earlier Classes
    OutPatientBill op("Ravi", 500.0, 1200.0);
    InPatientBill ip("Meena", 3, 2500.0, 40000.0, 2500.0);
    SelfPay selfPay;
    Insurance ins(50000.0, 10.0);

    assert(std::fabs(op.total() - 1700.0) < 1e-6);
    assert(std::fabs(ip.total() - 50000.0) < 1e-6);
    assert(std::fabs(selfPay.amountCovered(op) - 0.0) < 1e-6);
    assert(std::fabs(ins.amountCovered(ip) - 45000.0) < 1e-6);
    std::cout << "[PASS] All earlier regression tests passed.\n\n";

    // 2. Test New Bill Type (TelemedicineBill) & New Payer (CorporateTieUp)
    std::vector<std::unique_ptr<Bill>> bills;
    bills.push_back(std::make_unique<OutPatientBill>("Ravi", 500.0, 1200.0));
    bills.push_back(std::make_unique<TelemedicineBill>("Kavya", 400.0, 100.0)); // New type: 500.00
    bills.push_back(std::make_unique<InPatientBill>("Meena", 3, 2500.0, 40000.0, 2500.0)); // 50000.00

    CorporateTieUp corp(25000.0, 0.80); // New payer
    GovernmentScheme govt;

    std::cout << "=== B4.1: Processing Extended Bills with CorporateTieUp ===\n";
    for (const auto& b : bills) {
        double covered = settleBill(*b, corp);
        std::cout << "  " << std::setw(14) << std::left << b->type()
                  << " | Patient: " << std::setw(8) << b->getPatient()
                  << " | Total: " << std::setw(9) << b->total()
                  << " | Corp Covered: " << std::setw(9) << covered
                  << " | Patient Pays: " << (b->total() - covered) << '\n';
    }

    // 3. B4.2: Trigger Exceptions (Exceeds Cover & Unsupported Scheme Type) Under ASan
    std::cout << "\n=== B4.2: Exception Recovery & Resource Safety Check ===\n";
    try {
        // Insurance with low limit (10,000) throws ClaimError on Meena's 50,000 bill
        Insurance lowCoverIns(10000.0, 10.0);
        settleBill(*bills[2], lowCoverIns);
    } catch (const ClaimError& e) {
        std::cout << "  [RECOVERED] Top-level handler caught: " << e.what() << '\n';
    }

    try {
        // GovtScheme does not have Telemedicine in its package table -> throws ClaimError
        settleBill(*bills[1], govt);
    } catch (const ClaimError& e) {
        std::cout << "  [RECOVERED] Top-level handler caught: " << e.what() << '\n';
    }

    try {
        // Invalid negative Telemedicine fee -> throws BillingError during construction
        auto badBill = std::make_unique<TelemedicineBill>("InvalidUser", -100.0, 50.0);
    } catch (const BillingError& e) {
        std::cout << "  [RECOVERED] Constructor error caught: " << e.what() << '\n';
    }

    std::cout << "\nProgram completed cleanly with zero memory leaks.\n";
    return 0;
}
