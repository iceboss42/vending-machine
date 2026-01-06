#include <algorithm>
#include <cctype>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
using namespace std;
// ---------------------------- Money utilities -------------------------------
namespace Money {
    bool parseToPennies(const string& s, int& outPennies) {
        string x;
        for (size_t i = 0; i < s.size(); ++i) {
            unsigned char c = static_cast<unsigned char>(s[i]);
            if (!isspace(c)) x.push_back(static_cast<char>(c));
        }
        if (x.empty()) return false;
        if (!x.empty()) {
            unsigned char c0 = static_cast<unsigned char>(x[0]);
            if (!isdigit(c0) && c0 != '.' && c0 != '+' && c0 != '-') {
                x.erase(x.begin());
            }
        }
        if (!x.empty() && (x[0] == '-' || x[0] == '+')) return false;
        int dotCount = (int)count(x.begin(), x.end(), '.');
        if (dotCount > 1) return false;
        long long total = 0;
        if (dotCount == 0) {
            if (!all_of(x.begin(), x.end(), ::isdigit)) return false;
            long long pounds = 0;
            try { pounds = stoll(x); } catch (...) { return false; }
            total = pounds * 100;
        } else {
            size_t pos = x.find('.');
            string whole = x.substr(0, pos);
            string frac = x.substr(pos + 1);
            if (whole.empty()) whole = "0";
            if (frac.empty() || frac.size() > 2) return false;
            if (!all_of(whole.begin(), whole.end(), ::isdigit)) return false;
            if (!all_of(frac.begin(), frac.end(), ::isdigit)) return false;
            long long pounds = 0, pence = 0;
            try { pounds = stoll(whole); pence = stoll(frac); } catch (...) { return false; }
            if (frac.size() == 1) pence *= 10; 
            total = pounds * 100 + pence;
        }
        if (total < 0 || total > numeric_limits<int>::max()) return false;
        outPennies = static_cast<int>(total);
        return true;
    }
    string formatPennies(int p) {
        bool neg = p < 0;
        if (neg) p = -p;
        int pounds = p / 100;
        int pence  = p % 100;
        ostringstream oss;
        if (neg) oss << "-";
        oss << "£" << pounds << "." << setw(2) << setfill('0') << pence;
        return oss.str();
    }
} 
// ---------------------------- Domain model ----------------------------------
struct Item {
    string code;        
    string name;        
    string category;    
    int pricePennies;   
    int stock;          
    Item() : pricePennies(0), stock(0) {}
    Item(const string& c, const string& n, const string& cat, int price, int stk)
        : code(c), name(n), category(cat), pricePennies(price), stock(stk) {}
};
class Inventory {
private:
    unordered_map<string, Item> byCode_;          
    map<string, vector<string> > catsToCodes_;    
public:
    void addItem(const Item& item) {
        byCode_[item.code] = item;
        catsToCodes_[item.category].push_back(item.code);
    }
    bool hasCode(const string& code) const {
        return byCode_.find(code) != byCode_.end();
    }
    const Item* get(const string& code) const {
        unordered_map<string, Item>::const_iterator it = byCode_.find(code);
        return it == byCode_.end() ? NULL : &it->second;
    }
    Item* getMutable(const string& code) {
        unordered_map<string, Item>::iterator it = byCode_.find(code);
        return it == byCode_.end() ? NULL : &it->second;
    }
    bool inStock(const string& code) const {
        const Item* it = get(code);
        return it && it->stock > 0;
    }
    bool takeOne(const string& code) {
        Item* it = getMutable(code);
        if (!it || it->stock <= 0) return false;
        --(it->stock);
        return true;
    }
    const map<string, vector<string> >& categories() const { return catsToCodes_; }
};
class SuggestionEngine {
private:
    unordered_map<string, string> map_;
public:
    void set(const string& fromCode, const string& suggestCode) { map_[fromCode] = suggestCode; }
    bool get(const string& fromCode, string& outSuggested) const {
        unordered_map<string, string>::const_iterator it = map_.find(fromCode);
        if (it == map_.end()) return false;
        outSuggested = it->second;
        return true;
    }
};
class ChangeMaker {
private:
    int denoms_[8]; 
public:
    ChangeMaker() : denoms_{200, 100, 50, 20, 10, 5, 2, 1} {}
    vector<pair<int,int> > makeChange(int changePennies) const {
        vector<pair<int,int> > breakdown;
        for (size_t i = 0; i < sizeof(denoms_)/sizeof(denoms_[0]); ++i) {
            int d = denoms_[i];
            int cnt = changePennies / d;
            if (cnt > 0) {
                breakdown.push_back(make_pair(d, cnt));
                changePennies %= d;
            }
        }
        return breakdown;
    }
    static string denomToString(int p) {
        if (p >= 100) return string("£") + to_string(p / 100);
        return to_string(p) + "p";
    }
};
class VendingMachine {
private:
    Inventory inventory_;
    SuggestionEngine sugg_;
    ChangeMaker changer_;
    int balancePennies_;
    static void trim(string& t) {
        size_t i = 0;
        while (i < t.size() && isspace(static_cast<unsigned char>(t[i]))) ++i;
        t.erase(0, i);
        while (!t.empty() && isspace(static_cast<unsigned char>(t.back()))) t.pop_back();
    }
    void printHeader() const {
        cout << "=========================================\n";
        cout << "           VENDING MACHINE 3000          \n";
        cout << "=========================================\n";
    }
    void printMenu() const {
        printHeader();
        cout << "Your balance: " << Money::formatPennies(balancePennies_) << "\n\n";
        for (map<string, vector<string> >::const_iterator it = inventory_.categories().begin();
             it != inventory_.categories().end(); ++it)
        {
            const string& cat = it->first;
            vector<string> codes = it->second; 
            sort(codes.begin(), codes.end());
            cout << "[" << cat << "]\n";
            for (size_t i = 0; i < codes.size(); ++i) {
                const Item* itItem = inventory_.get(codes[i]);
                if (!itItem) continue;
                cout << "  " << setw(3) << left << itItem->code
                     << "  " << setw(20) << left << itItem->name
                     << "  " << setw(8) << left << Money::formatPennies(itItem->pricePennies)
                     << "  Stock: " << itItem->stock << "\n";
            }
            cout << "\n";
        }
        cout << "Commands:\n"
             << "  code (e.g., A1)   -> buy item\n"
             << "  add               -> insert money\n"
             << "  help              -> show commands\n"
             << "  quit              -> finish and get change\n";
        cout << "-----------------------------------------\n";
        cout << std::flush;
    }
    void handleAddMoney() {
        cout << "Enter amount to add (e.g., 1, 1.50, £2.00): ";
        string s; getline(cin, s);
        int p = 0;
        if (!Money::parseToPennies(s, p) || p <= 0) {
            cout << "Invalid amount. Please try again.\n";
            return;
        }
        balancePennies_ += p;
        cout << "Added " << Money::formatPennies(p)
             << ". New balance: " << Money::formatPennies(balancePennies_) << "\n";
    }
    void handlePurchase(const string& rawCode) {
        if (rawCode.empty()) return;
        string code = rawCode;
        for (size_t i = 0; i < code.size(); ++i) {
            code[i] = static_cast<char>(toupper(static_cast<unsigned char>(code[i])));
        }
        if (!inventory_.hasCode(code)) {
            cout << "Unknown code. Please check and try again.\n";
            return;
        }
        const Item* it = inventory_.get(code);
        if (!it) { cout << "Error fetching item.\n"; return; }

        if (it->stock <= 0) {
            cout << "Sorry, " << it->name << " is out of stock.\n";
            return;
        }
        if (balancePennies_ < it->pricePennies) {
            int shortfall = it->pricePennies - balancePennies_;
            cout << "Insufficient funds. You need " << Money::formatPennies(shortfall) << " more.\n";
            return;
        }
        balancePennies_ -= it->pricePennies;
        if (!inventory_.takeOne(code)) {
            cout << "Unexpected stock error. Purchase cancelled.\n";
            balancePennies_ += it->pricePennies;
            return;
        }
        cout << "Dispensing: " << it->name << " (" << it->code << ") ... Enjoy!\n";
        cout << "Remaining balance: " << Money::formatPennies(balancePennies_) << "\n";
        string suggested;
        if (sugg_.get(code, suggested)) {
            const Item* sit = inventory_.get(suggested);
            if (sit && sit->stock > 0) {
                cout << "You might also like: " << sit->name << " [" << sit->code << "] for "
                     << Money::formatPennies(sit->pricePennies) << "\n";
            }
        }
    }
    void returnChangeAndExit() {
        cout << "\nReturning change: " << Money::formatPennies(balancePennies_) << "\n";
        vector<pair<int,int> > breakdown = changer_.makeChange(balancePennies_);
        if (breakdown.empty()) {
            cout << "No change.\n";
        } else {
            cout << "Change breakdown:\n";
            for (size_t i = 0; i < breakdown.size(); ++i) {
                cout << "  " << ChangeMaker::denomToString(breakdown[i].first)
                     << " x " << breakdown[i].second << "\n";
            }
        }
        cout << "Thank you for using VENDING MACHINE 3000!\n";
    }
    static void printHelp() {
        cout << "HELP\n"
             << " • Enter an item code (e.g., A1) to buy an item if you have enough balance.\n"
             << " • Type 'add' to insert money (e.g., 1, 1.50, £2.00).\n"
             << " • Type 'quit' to finish your session and receive change.\n";
    }
public:
    VendingMachine() : balancePennies_(0) {}
    void seedDemoData() {
        inventory_.addItem(Item("A1", "Espresso",     "Hot Drinks", 150, 5));
        inventory_.addItem(Item("A2", "Tea",          "Hot Drinks", 120, 8));
        inventory_.addItem(Item("A3", "Latte",        "Hot Drinks", 190, 4));
        inventory_.addItem(Item("B1", "Cola",         "Cold Drinks", 180, 6));
        inventory_.addItem(Item("B2", "Orange Juice", "Cold Drinks", 200, 4));
        inventory_.addItem(Item("B3", "Water",        "Cold Drinks", 100, 9));
        inventory_.addItem(Item("C1", "Crisps",       "Snacks",      130, 10));
        inventory_.addItem(Item("C2", "Biscuits",     "Snacks",      140,  7));
        inventory_.addItem(Item("C3", "Nuts",         "Snacks",      150,  5));
        inventory_.addItem(Item("D1", "Chocolate Bar","Chocolate",   160,  5));
        inventory_.addItem(Item("D2", "Dark Choc",    "Chocolate",   170,  3));
        inventory_.addItem(Item("D3", "Milk Choc",    "Chocolate",   150,  6));
        sugg_.set("A1", "C2"); 
        sugg_.set("A2", "C2"); 
        sugg_.set("A3", "C3"); 
        sugg_.set("B1", "C1"); 
        sugg_.set("B2", "D1"); 
        sugg_.set("B3", "C1"); 
    }
    void run() {
        while (true) {
            printMenu();
            cout << "Enter command or code: ";
            string input;
            if (!getline(cin, input)) break;
            trim(input);
            string up = input;
            for (size_t i = 0; i < up.size(); ++i) {
                up[i] = static_cast<char>(toupper(static_cast<unsigned char>(up[i])));
            }

            if (up == "QUIT" || up == "Q" || up == "EXIT") {
                returnChangeAndExit();
                break;
            } else if (up == "HELP" || up == "H") {
                printHelp();
            } else if (up == "ADD") {
                handleAddMoney();
            } else if (input.empty()) {
                continue;
            } else {
                handlePurchase(input);
            }
        }
    }
};
// ---------------------------------- main ------------------------------------
int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(&std::cout);
    VendingMachine vm;
    vm.seedDemoData();
    vm.run();
    return 0;
}