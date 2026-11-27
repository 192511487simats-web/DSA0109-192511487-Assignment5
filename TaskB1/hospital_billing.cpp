#include <algorithm>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

class Bill {
protected:
    std::string patient;
public:
    explicit Bill(std::string p) : patient(std::move(p)) {}
    virtual ~Bill() = default;
    virtual double total() const = 0;
    virtual std::string type() const = 0;
    const std::string& getPatient() const noexcept { return patient; }
};

class OutPatientBill : public Bill {
    double consultation, tests;
public:
    OutPatientBill(std::string p, double c, double t)
        : Bill(std::move(p)), consultation(c), tests(t) {}
    double total() const override { return consultation + tests; }
    std::string type() const override { return "OutPatient"; }
};

class InPatientBill : public Bill {
    int days;
    double perDay, procedures, medicines;
public:
    InPatientBill(std::string p, int d, double rate, double proc, double med)
        : Bill(std::move(p)), days(d), perDay(rate), procedures(proc), medicines(med) {}
    double total() const override { return (days * perDay) + procedures + medicines; }
    std::string type() const override { return "InPatient"; }
};

class DayCareSurgeryBill : public Bill {
    double packagePrice;
public:
    DayCareSurgeryBill(std::string p, double pkg)
        : Bill(std::move(p)), packagePrice(pkg) {}
    double total() const override { return packagePrice; }
    std::string type() const override { return "DayCareSurgery"; }
};

class EmergencyBill : public Bill {
    double baseCharge, hours, hourlyRate;
public:
    EmergencyBill(std::string p, double base, double hrs, double rate)
        : Bill(std::move(p)), baseCharge(base), hours(hrs), hourlyRate(rate) {}
    double total() const override { return baseCharge + (hours * hourlyRate); }
    std::string type() const override { return "Emergency"; }
};

class Payer {
public:
    virtual ~Payer() = default;
    virtual double amountCovered(const Bill& b) const = 0;
    virtual std::string name() const = 0;
};

class SelfPay : public Payer {
public:
    double amountCovered(const Bill&) const override { return 0.0; }
    std::string name() const override { return "SelfPay"; }
};

class Insurance : public Payer {
    double coverLimit, coPayPercent;
public:
    Insurance(double limit, double coPay) : coverLimit(limit), coPayPercent(coPay) {}
    double amountCovered(const Bill& b) const override {
        double eligible = b.total() * (1.0 - coPayPercent / 100.0);
        return std::min(eligible, coverLimit);
    }
    std::string name() const override { return "Insurance(50k,10%)"; }
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
        double fixedRate = (it != rates.end()) ? it->second : 0.0;
        return std::min(b.total(), fixedRate);
    }
    std::string name() const override { return "GovtScheme"; }
};

int main() {
    std::vector<std::unique_ptr<Bill>> bills;
    bills.push_back(std::make_unique<OutPatientBill>("Ravi", 500.0, 1200.0));          // Total: 1700
    bills.push_back(std::make_unique<InPatientBill>("Meena", 3, 2500.0, 40000.0, 2500.0)); // Total: 50000
    bills.push_back(std::make_unique<DayCareSurgeryBill>("Arun", 18000.0));            // Total: 18000
    bills.push_back(std::make_unique<EmergencyBill>("Priya", 2000.0, 4.0, 1500.0));    // Total: 8000

    std::vector<std::unique_ptr<Payer>> payers;
    payers.push_back(std::make_unique<SelfPay>());
    payers.push_back(std::make_unique<Insurance>(50000.0, 10.0));
    payers.push_back(std::make_unique<GovernmentScheme>());

    std::cout << std::fixed << std::setprecision(2);
    for (const auto& b : bills) {
        std::cout << "=== " << b->type() << " (" << b->getPatient()
                  << ") | Bill Total: " << b->total() << " ===\n";
        for (const auto& p : payers) {
            double cov = p->amountCovered(*b);
            double pat = b->total() - cov;
            std::cout << "  " << std::setw(20) << std::left << p->name()
                      << " -> Covered: " << std::setw(9) << cov
                      << " | Patient Pays: " << pat << '\n';
        }
    }
    return 0;
}
