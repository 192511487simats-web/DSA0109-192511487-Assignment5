#include <iostream>
#include <map>
#include <memory>
#include <new>
#include <stdexcept>
#include <string>
#include <vector>

class NegativeChargeError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class ClaimAboveCoverError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class UnknownPackageError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

struct TrackedResource {
    std::string label;
    explicit TrackedResource(std::string l) : label(std::move(l)) {
        std::cout << "  [Alloc] " << label << '\n';
    }
    ~TrackedResource() {
        std::cout << "  [Destroyed] " << label << '\n';
    }
};

double processClaim(const std::string& rawCharge, double cover, const std::string& pkgCode) {
    TrackedResource guard("TempClaimBuffer(" + pkgCode + ")");

    // 1. Parse numeric string (std::stod throws std::invalid_argument on non-numeric text)
    double amount = std::stod(rawCharge);

    // 2. Check negative charges
    if (amount < 0.0) {
        throw NegativeChargeError("Negative charge not allowed: " + std::to_string(amount));
    }

    // 3. Check package code
    static const std::map<std::string, double> packages = {{"PKG_OP", 2000.0}, {"PKG_SURG", 45000.0}};
    if (packages.find(pkgCode) == packages.end()) {
        throw UnknownPackageError("Unknown package code: " + pkgCode);
    }

    // 4. Check cover limit
    if (amount > cover) {
        throw ClaimAboveCoverError("Claim " + std::to_string(amount) + " exceeds cover " + std::to_string(cover));
    }

    return amount;
}

int main() {
    struct TestCase {
        std::string name;
        std::string amountText;
        double cover;
        std::string pkg;
        bool forceOOM;
        bool throwUnknownNonStd;
    };

    std::vector<TestCase> tests = {
        {"Test 1: Valid Claim",          "1500.0",  5000.0, "PKG_OP",   false, false},
        {"Test 2: Negative Charge",      "-250.0",  5000.0, "PKG_OP",   false, false},
        {"Test 3: Claim Above Cover",    "12000.0", 5000.0, "PKG_OP",   false, false},
        {"Test 4: Unknown Package Code", "1500.0",  5000.0, "PKG_999",  false, false},
        {"Test 5: Non-Numeric Input",    "ABC_ERR", 5000.0, "PKG_OP",   false, false},
        {"Test 6: Out-of-Memory (OOM)",  "1000.0",  5000.0, "PKG_OP",   true,  false},
        {"Test 7: Unexpected Throw",     "1000.0",  5000.0, "PKG_OP",   false, true}
    };

    for (const auto& t : tests) {
        std::cout << "=== " << t.name << " ===\n";
        try {
            TrackedResource batchGuard("BatchContext");
            if (t.forceOOM) {
                // Request impossibly large allocation to trigger std::bad_alloc
                volatile char* huge = new char[static_cast<std::size_t>(-1) / 2];
                delete[] huge;
            }
            if (t.throwUnknownNonStd) {
                throw 999; // Non-std::exception object
            }
            double approved = processClaim(t.amountText, t.cover, t.pkg);
            std::cout << "  Result: Approved Rs. " << approved << '\n';
        } catch (const std::bad_alloc& e) {
            std::cout << "  Caught std::bad_alloc: " << e.what() << '\n';
        } catch (const std::invalid_argument& e) {
            std::cout << "  Caught std::invalid_argument (stod): " << e.what() << '\n';
        } catch (const std::exception& e) {
            std::cout << "  Caught domain/std exception: " << e.what() << '\n';
        } catch (...) {
            std::cout << "  Caught unknown non-standard exception via catch(...)\n";
        }
    }

    std::cout << "\nAll test cases completed safely without crashing!\n";
    return 0;
}
